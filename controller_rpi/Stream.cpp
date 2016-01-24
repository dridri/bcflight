#include <netinet/in.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include <gammaengine/Image.h>
#include "Stream.h"

static const char vertices_shader[] =
"	out vec2 ge_TextureCoord;\n"
"\n"
"	void main()\n"
"	{\n"
"		ge_TextureCoord = ge_VertexTexcoord.xy;\n"
"		ge_Position = ge_ProjectionMatrix * vec4(ge_VertexPosition.xy, 0.0, 1.0);\n"
"	}\n"
;

static const char fragment_shader[] =
"	in vec2 ge_TextureCoord;\n"
"\n"
"	void main()\n"
"	{\n"
"		ge_FragColor = texture( ge_Texture0, ge_TextureCoord.xy );\n"
"	}\n"
;


Stream::Stream( Font* font, const std::string& addr, uint16_t port )
	: Thread()
	, mInstance( nullptr )
	, mWindow( nullptr )
	, mFont( font )
	, mVideoImage( nullptr )
	, mEGLVideoImage( 0 )
	, mVideoTexture( 0 )
	, mRx( nullptr )
	, mSocket( nullptr )
{
// 	mRx = rwifi_rx_init( "wlan0", 0, 1 );
	mSocket = new Socket( addr, port, Socket::UDPLite );

	uint32_t uid = htonl( 0x12345678 );
	mSocket->Send( &uid, sizeof(uid) );

	mFPSTimer = Timer();
	mScreenTimer = Timer();
	Start();
}


Stream::~Stream()
{
}


bool Stream::run()
{
	if ( !mInstance and !mWindow ) {
		mInstance = Instance::Create( "flight::control", 1, false, "opengles20" );
		mWindow = mInstance->CreateWindow( "", -1, -1, Window::Fullscreen );
		mLayerDisplay = CreateNativeWindow( 0 );
		mWindow->SetNativeWindow( reinterpret_cast< EGLNativeWindowType >( &mLayerDisplay ) );
		mRenderer = mInstance->CreateRenderer2D( mWindow->width(), mWindow->height() );
		mRenderer->LoadVertexShader( vertices_shader, sizeof(vertices_shader) + 1 );
		mRenderer->LoadFragmentShader( fragment_shader, sizeof(fragment_shader) + 1 );
		mRenderer->setBlendingEnabled( false );
// 		mVideoImage = new Image( 1920, 1080, 0xFF000000, mInstance );
// 		mVideoImage = new DecodedImage( mInstance, 1280, 720, 0xFF000000 );
// 		mVideoImage = new DecodedImage( mInstance, 640, 480, 0xFF000000 );
// 		mVideoImage = new DecodedImage( mInstance, 640, 360, 0xFF000000 );
/*
		mEGLVideoImage = eglCreateImageKHR( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)mVideoImage->serverReference( mInstance ), nullptr );
		if ( mEGLVideoImage == EGL_NO_IMAGE_KHR ) {
			printf("eglCreateImageKHR failed.\n");
			exit(1);
		}
		mDecodeContext = video_configure( mEGLVideoImage );
*/
		mDecodeContext = video_configure( nullptr );
		video_start( mDecodeContext );

		mDecodeThread = new HookThread< Stream >( this, &Stream::DecodeThreadRun );
		mDecodeThread->Start();

		mFPSTimer.Start();
		mScreenTimer.Start();
	}

	if ( mDecodeContext->width != 0 and mDecodeContext->height != 0 and not mEGLVideoImage ) {
		mVideoImage = new DecodedImage( mInstance, mDecodeContext->width, mDecodeContext->height, 0xFF000000 );
		mEGLVideoImage = eglCreateImageKHR( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)mVideoImage->serverReference( mInstance ), nullptr );
		if ( mEGLVideoImage == EGL_NO_IMAGE_KHR ) {
			printf("eglCreateImageKHR failed.\n");
			exit(1);
		}
		mDecodeContext->eglImage = mEGLVideoImage;
	}

// 	mWindow->Clear( 0xFF202020 );

	if ( mVideoImage ) {
// 		mRenderer->Draw( 0, 0, mWindow->width(), mWindow->height(), mVideoImage, 0, 0, mVideoImage->width(), mVideoImage->height() );
		mRenderer->Draw( 0, 0, mWindow->width(), mWindow->height(), mVideoImage, 0, 0, mVideoImage->width() * 2, mVideoImage->height() );
	}

// 	mRenderer->DrawText( 20, 20, mFont, 0xFFFFFFFF, "fps : " + std::to_string( (int)mWindow->fps() ) );

	int s = mScreenTimer.ellapsed() / 1000;
	int ms = mScreenTimer.ellapsed() % 1000;
	static char str[128];
	sprintf( str, "%d:%03d", s, ms );
	mRenderer->DrawText( mWindow->width() / 4, 0, mFont, 0xFFFFFFFF, str, Renderer2D::HCenter );
	mRenderer->DrawText( mWindow->width() / 4, mWindow->height() - mFont->size(), mFont, 0xFFFFFFFF, str, Renderer2D::HCenter );

	mWindow->SwapBuffers();
	return true;
}


bool Stream::DecodeThreadRun()
{
	unsigned int header[2];
// 	uint8_t frame[32768] = { 0 };
// 	uint8_t* frame = nullptr;
	uint32_t frameSize = 0;

	if ( mRx ) {
		uint8_t buffer[2][65536] = { 0 };
		static uint32_t buf_i = 0;
		uint32_t xbuf = 0;

		{

			buf_i = rwifi_rx_recv( mRx, buffer[xbuf], mDecodeContext->decinput->nAllocLen );

			if ( buf_i > 0 ) {
				memcpy( mDecodeContext->decinput->pBuffer, buffer[xbuf], buf_i );
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
		frameSize = mSocket->Receive( mDecodeContext->decinput->pBuffer, mDecodeContext->decinput->nAllocLen, false, 10 );
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
		gDebug() << "fps : " << ( mFPS * 1 ) << "    bitrate : " << ( mBitrateCounter * 8 / 1024 ) << "kbps ( " << ( mBitrateCounter / 1024 ) << " KBps )\n";
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

	dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display, layer, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, 0, 0, DISPMANX_NO_ROTATE );
	nativewindow.element = dispman_element;
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );   

	return nativewindow;
}

