#include <netinet/in.h>
#include <iwlib.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include <gammaengine/Image.h>
#include "Stream.h"
#include "Controller.h"

static const char hud_vertices_shader[] =
R"(	out vec2 ge_TextureCoord;

	vec2 VR_Distort( vec2 coords )
	{
		vec2 ret = coords;
		vec2 ofs = vec2(1280.0/4.0, 720.0/2.0);
		if ( coords.x >= 1280.0/2.0 ) {
			ofs.x = 3.0 * 1280.0/4.0;
		}
		vec2 offset = coords - ofs;
		float r2 = offset.x * offset.x + offset.y * offset.y;
// 		float r2 = dot( offset.xy, offset.xy );
		float r = sqrt(r2);
		float k1 = 1.95304;
		k1 = k1 / ((1280.0 / 4.0)*(1280.0 / 4.0) * 16.0);

		ret = ofs + ( 1.0 - k1 * r * r ) * offset;

		return ret;
	}


	void main()
	{
		ge_TextureCoord = ge_VertexTexcoord.xy;
		vec2 pos = ge_VertexPosition.xy;
// 		pos.xy = VR_Distort( pos.xy );
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_fragment_shader[] =
R"(	uniform vec4 color;
	in vec2 ge_TextureCoord;

	void main()
	{
		ge_FragColor = color;// * texture( ge_Texture0, ge_TextureCoord.xy );
	})"
;


Stream::Stream( Controller* controller, Font* font, const std::string& addr, uint16_t port )
	: Thread()
	, mInstance( nullptr )
	, mWindow( nullptr )
	, mController( controller )
	, mFont( font )
	, mVideoImage( nullptr )
	, mEGLVideoImage( 0 )
	, mVideoTexture( 0 )
	, mRx( nullptr )
	, mSocket( nullptr )
	, mDecodeInput( nullptr )
{
// 	mRx = rwifi_rx_init( "wlan0", 0, 1 );
	mSocket = new Socket( addr, port, Socket::UDPLite );
	mBlinkingViews = false;
	mFPS = 0;
	mFrameCounter = 0;

	uint32_t uid = htonl( 0x12345678 );
	mSocket->Send( &uid, sizeof(uid) );

	mIwSocket = iw_sockets_open();
	memset( &mIwStats, 0, sizeof( mIwStats ) );

	mFPSTimer = Timer();
	mScreenTimer = Timer();
	mHUDTimer = Timer();
	mHUDTimer.Start();
	Start();

	mSignalThread = new HookThread< Stream >( this, &Stream::SignalThreadRun );
	mSignalThread->Start();
}


Stream::~Stream()
{
}


bool Stream::run()
{
	if ( !mInstance and !mWindow ) {
		mInstance = Instance::Create( "flight::control", 1, false, "opengles20" );
		mWindow = mInstance->CreateWindow( "", -1, -1, Window::Fullscreen );

		mLayerDisplay = CreateNativeWindow( 2 );
		mWindow->SetNativeWindow( reinterpret_cast< EGLNativeWindowType >( &mLayerDisplay ) );
		mRenderer = mInstance->CreateRenderer2D( mWindow->width(), mWindow->height() );
// 		mRenderer->LoadVertexShader( hud_vertices_shader, sizeof(hud_vertices_shader) + 1 );
// 		mRenderer->LoadFragmentShader( hud_fragment_shader, sizeof(hud_fragment_shader) + 1 );
		mRenderer->setBlendingEnabled( false );

		mRendererHUD = new RendererHUD( mInstance, mFont );

		mDecodeContext = video_configure();
		video_start( mDecodeContext );

		mDecodeThread = new HookThread< Stream >( this, &Stream::DecodeThreadRun );
		mDecodeThread->Start();

		mFPSTimer.Start();
		mScreenTimer.Start();

		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	glClear( GL_COLOR_BUFFER_BIT );

	// Link status
	{
		mRendererHUD->RenderLink( (float)mIwStats.qual / 100.0f );
		float link_red = 1.0f - ((float)mIwStats.qual) / 100.0f;
		std::string link_quality_str = "Ch" + std::to_string( mIwStats.channel ) + " " + std::to_string( mIwStats.level ) + "dBm " + std::to_string( mIwStats.qual ) + "%";
		mRendererHUD->RenderText( (float)mWindow->width() * 0.03f, 140, link_quality_str, Vector4f( 0.5f + 0.5f * link_red, 1.0f - link_red * 0.25f, 0.5f - link_red * 0.5f, 1.0f ) );
	}

	// FPS + latency
	{
		int w = 0, h = 0;
		float fps_red = std::max( 0.0f, std::min( 1.0f, ((float)( 50 - mFPS ) ) / 50.0f ) );
		std::string fps_str = std::to_string( mFPS );
		mFont->measureString( fps_str, &w, &h );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 2.0f - 0.075f ) - w, 120, fps_str, Vector4f( 0.5f + 0.5f * fps_red, 1.0f - fps_red * 0.25f, 0.5f - fps_red * 0.5f, 1.0f ) );
		float latency_red = std::min( 1.0f, ((float)mController->ping()) / 50.0f );
		std::string latency_str = std::to_string( mController->ping() ) + "ms";
		mFont->measureString( latency_str, &w, &h );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 2.0f - 0.075f ) - w, 140, latency_str, Vector4f( 0.5f + 0.5f * latency_red, 1.0f - latency_red * 0.25f, 0.5f - latency_red * 0.5f, 1.0f ) );
	}

	// Battery
	{
		float level = mController->batteryLevel();
		mRendererHUD->RenderBattery( level );
		if ( level <= 0.25f and mBlinkingViews ) {
			mRendererHUD->RenderText( (float)mWindow->width() * 1.0f / 4.0f, mWindow->height() - 125, "Low Battery", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), true );
		}
		int w = 0, h = 0;
		std::string current_drawn = std::to_string( mController->totalCurrent() );
		mFont->measureString( current_drawn, &w, &h );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f ) - w - 8, mWindow->height() - 140, current_drawn, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
		std::string voltage = std::to_string( mController->batteryVoltage() );
		voltage = voltage.substr( 0, voltage.find( "." ) + 3 );
		mFont->measureString( voltage, &w, &h );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f ) - w - 8, mWindow->height() - 105, voltage, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	}

	// Controls
	{
		mRendererHUD->RenderThrust( mController->thrust() );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 4.0f + 0.05f ), 112, "ACRO", Vector4f( 0.35f, 0.35f, 0.35f, 1.0f ) );
		mRendererHUD->RenderText( (float)mWindow->width() * ( 1.0f / 4.0f + 0.05f ), 112 + mFont->size(), "HRZN", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	}

	if ( mHUDTimer.ellapsed() >= 1000 ) {
		mBlinkingViews = !mBlinkingViews;
		mHUDTimer.Stop();
		mHUDTimer.Start();
	}

	mWindow->SwapBuffers();
	return true;
}


bool Stream::SignalThreadRun()
{
	iwstats stats;
	wireless_config info;
	iwrange range;
	memset( &stats, 0, sizeof( stats ) );
	if ( iw_get_stats( mIwSocket, "wlan0", &stats, nullptr, 0 ) == 0 ) {
		mIwStats.qual = (int)stats.qual.qual * 100 / 70;
		union { int8_t s; uint8_t u; } conv = { .u = stats.qual.level };
		mIwStats.level = conv.s;
		mIwStats.noise = stats.qual.noise;
	}
	if ( iw_get_basic_config( mIwSocket, "wlan0", &info ) == 0 ) {
		if ( iw_get_range_info( mIwSocket, "wlan0", &range ) == 0 ) {
			mIwStats.channel = iw_freq_to_channel( info.freq, &range );
		}
	}
	usleep( 1000 * 1000 );
	return true;
}


bool Stream::DecodeThreadRun()
{
	unsigned int header[2];
// 	uint8_t frame[32768] = { 0 };
// 	uint8_t* frame = nullptr;
	uint32_t frameSize = 0;

	if ( mDecodeInput == nullptr ) {
		mDecodeInput = mDecodeContext->decinput->pBuffer;
	} else {
		mDecodeContext->decinput->pBuffer = (OMX_U8*)mDecodeInput;
	}

	if ( mDecodeContext->decinput->pBuffer != mDecodeInput ) {
		gDebug() << "GPU messed up decoder input buffer ! ( " << (void*)mDecodeContext->decinput->pBuffer << " != " << (void*)mDecodeInput << "\n";
	}

	if ( mRx ) {
		uint8_t buffer[2][65536] = { 0 };
		static uint32_t buf_i = 0;
		uint32_t xbuf = 0;

		{

			buf_i = rwifi_rx_recv( mRx, buffer[xbuf], mDecodeContext->decinput->nAllocLen );

			if ( buf_i > 0 ) {
				memcpy( mDecodeInput, buffer[xbuf], buf_i );
				mDecodeContext->decinput->nFilledLen = buf_i;
				video_decode_frame( mDecodeContext );

				mFrameCounter++;
				mBitrateCounter += buf_i;
				buf_i = 0;
				xbuf = ( xbuf + 1 ) % 2;
			}
		}
	}

	if ( mSocket ) {
		frameSize = mSocket->Receive( mDecodeInput, mDecodeContext->decinput->nAllocLen, false, 10 );
		if ( frameSize > 0 ) {
			mDecodeContext->decinput->nFilledLen = frameSize;
			video_decode_frame( mDecodeContext );
			mFrameCounter++;
			mBitrateCounter += frameSize;
			frameSize = 0;
		}
	}

	if ( mFPSTimer.ellapsed() >= 1000 ) {
		mFPS = mFrameCounter;
		mFPSTimer.Stop();
		mFPSTimer.Start();
// 		gDebug() << "fps : " << ( mFPS * 1 ) << "    bitrate : " << ( mBitrateCounter * 8 / 1024 ) << "kbps ( " << ( mBitrateCounter / 1024 ) << " KBps )\n";
		mFrameCounter = 0;
		mBitrateCounter = 0;

		if ( mSocket ) {
			uint32_t uid = htonl( 0x12345678 );
			mSocket->Send( &uid, sizeof(uid) );
		}
	}

	return true;
}


EGL_DISPMANX_WINDOW_T Stream::CreateNativeWindow( int layer )
{
	EGL_DISPMANX_WINDOW_T nativewindow;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t display_width;
	uint32_t display_height;

	int32_t success = 0;
	success = graphics_get_display_size( 5, &display_width, &display_height );
	gDebug() << "display size: " << display_width << " x "<< display_height << "\n";

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = display_width;
	dst_rect.height = display_height;
	
	display_width = 1280;
	display_height = 720;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = display_width << 16;
	src_rect.height = display_height << 16;

	dispman_display = vc_dispmanx_display_open( 5 );
	dispman_update = vc_dispmanx_update_start( 0 );

	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)( DISPMANX_FLAGS_ALPHA_FROM_SOURCE );
// 	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)( DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS | DISPMANX_FLAGS_ALPHA_PREMULT );
	alpha.opacity = 0;

	dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display, layer, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE );
	nativewindow.element = dispman_element;
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );   

	return nativewindow;
}

