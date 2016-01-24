#include <wiringPi.h>
#include "encode.h"

typedef video_context context;

static OMX_VERSIONTYPE SpecificationVersion = {
	.s.nVersionMajor = 1,
	.s.nVersionMinor = 1,
	.s.nRevision     = 2,
	.s.nStep         = 0
};


static OMX_U8* allocated_bufs[32] = { NULL };
static int allocated_bufs_idx = 0;

#define MAKEME( y, x ) \
	y = calloc( 1, sizeof( *y ) ); \
	y->nSize = sizeof( *y ); \
	y->nVersion = SpecificationVersion; \

#define MAKEME2( y, x, z ) \
	y = calloc( 1, sizeof( x ) + z ); \
	y->nSize = sizeof( x ) + z; \
	y->nVersion = SpecificationVersion; \



#define OERR(cmd) { \
	OMX_ERRORTYPE oerr = cmd; \
	if (oerr != OMX_ErrorNone) { \
		fprintf(stderr, #cmd \
			" failed on line %d: %x\n", \
			__LINE__, oerr); \
		exit(1); \
	} else { \
		fprintf(stderr, #cmd \
			" completed at %d.\n", \
			__LINE__); \
	} }

#define OERRq(cmd) { \
	OMX_ERRORTYPE oerr = cmd; \
	if (oerr != OMX_ErrorNone) { \
		fprintf(stderr, #cmd \
			" failed: %x\n", oerr); \
		exit(1); \
	} }

#define CAMNAME "OMX.broadcom.camera"
#define ENCNAME "OMX.broadcom.video_encode"
#define RSZNAME "OMX.broadcom.resize"
#define SPLNAME "OMX.broadcom.video_splitter"
#define NLLNAME "OMX.broadcom.null_sink"

OMX_BUFFERHEADERTYPE* omx_allocbufs( OMX_HANDLETYPE handle, int port, int enable );
void omx_block_until_port_changed( OMX_HANDLETYPE component, OMX_U32 nPortIndex, OMX_BOOL bEnabled );
void omx_block_until_state_changed( OMX_HANDLETYPE component, OMX_STATETYPE state );
void omx_print_port( OMX_HANDLETYPE component, OMX_U32 port );

static void config_camera( context* ctx, OMX_PARAM_PORTDEFINITIONTYPE* def );
static void config_resize( context* ctx, OMX_PARAM_PORTDEFINITIONTYPE* def );
static void video_free_buffers();

static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, context* ctx, OMX_BUFFERHEADERTYPE* buf );
static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, context* ctx, OMX_BUFFERHEADERTYPE* buf );

static OMX_CALLBACKTYPE genevents = {
	(void (*)) genericeventhandler,
	(void (*)) emptied,
	(void (*)) filled
};

static context* _ctx = NULL;


void omx_print_state( const char* name, OMX_HANDLETYPE comp )
{
	OMX_STATETYPE st;
	OERRq( OMX_GetState( comp, &st ) );
	printf( "%s state : 0x%08X\n", name, st );
}


uint32_t video_get_crc( OMX_HANDLETYPE enc )
{
	OMX_PARAM_U32TYPE crc;
	OMX_INIT_STRUCTURE( crc );
	crc.nPortIndex = 201;
	OERRq( OMX_GetParameter( enc, OMX_IndexParamBrcmCRC, &crc ) );
	return crc.nU32;
}


int video_fill_buffer( OMX_HANDLETYPE enc, OMX_BUFFERHEADERTYPE* bufs )
{
	int ret = OMX_FillThisBuffer( enc, bufs );
/*
	if ( OMX_SendCommand( _ctx->cam, OMX_CommandFlush, 73, NULL ) != OMX_ErrorNone ) {
		printf( "OMX_CommandFlush failed for camera port 73\n" );
		exit(1);
	}
*/

	if ( OMX_SendCommand( _ctx->spl, OMX_CommandFlush, 250, NULL ) != OMX_ErrorNone ) {
		printf( "OMX_CommandFlush failed for spl port 250\n" );
		exit(1);
	}
	if ( OMX_SendCommand( _ctx->spl, OMX_CommandFlush, 252, NULL ) != OMX_ErrorNone ) {
		printf( "OMX_CommandFlush failed for spl port 252\n" );
		exit(1);
	}
	if ( OMX_SendCommand( _ctx->enc2, OMX_CommandFlush, 200, NULL ) != OMX_ErrorNone ) {
		printf( "OMX_CommandFlush failed for enc2\n" );
		exit(1);
	}

	return ret;
}


char* video_buffer_ptr( OMX_BUFFERHEADERTYPE* bufs )
{
	return (char*)bufs->pBuffer;
}


int video_buffer_len( OMX_BUFFERHEADERTYPE* bufs )
{
	return bufs->nFilledLen;
}


int64_t video_buffer_timestamp( OMX_BUFFERHEADERTYPE* bufs )
{
	return ((uint64_t)bufs->nTimeStamp.nHighPart) << 32 | bufs->nTimeStamp.nLowPart;
}


void video_start( context* ctx )
{
	omx_print_state( "cam", ctx->cam );
	omx_print_state( "spl", ctx->spl );
	omx_print_state( "rsz", ctx->rsz );
	omx_print_state( "enc1", ctx->enc1 );
	omx_print_state( "enc2", ctx->enc2 );
	omx_print_state( "nll", ctx->nll );

	ctx->cambufs = omx_allocbufs( ctx->cam, 73, 1 );
#ifdef DUAL_ENCODERS
	ctx->enc1bufs = omx_allocbufs( ctx->enc1, 201, 1 );
#endif
	ctx->enc2bufs = omx_allocbufs( ctx->enc2, 201, 1 );

	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortEnable, 73, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortEnable, 70, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortEnable, 71, NULL ) );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandPortEnable, 240, NULL ) );
#ifdef PREV_RSZ
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortEnable, 60, NULL ) );
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortEnable, 61, NULL ) );
#endif
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 250, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 251, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 252, NULL ) );
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandPortEnable, 200, NULL ) );
#endif
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortEnable, 200, NULL ) );


	omx_block_until_port_changed( ctx->cam, 70, OMX_TRUE );
	omx_block_until_port_changed( ctx->cam, 71, OMX_TRUE );

	omx_block_until_port_changed( ctx->nll, 240, OMX_TRUE );

#ifdef PREV_RSZ
	omx_block_until_port_changed( ctx->rsz, 60, OMX_TRUE );
	omx_block_until_port_changed( ctx->rsz, 61, OMX_TRUE );
#endif

#ifdef DUAL_ENCODERS
	omx_block_until_port_changed( ctx->spl, 250, OMX_TRUE );
	omx_block_until_port_changed( ctx->spl, 251, OMX_TRUE );
	omx_block_until_port_changed( ctx->spl, 252, OMX_TRUE );

	omx_block_until_port_changed( ctx->enc1, 200, OMX_TRUE );
#endif
	omx_block_until_port_changed( ctx->enc2, 200, OMX_TRUE );

	omx_print_port( ctx->cam, 70 );
	omx_print_port( ctx->cam, 71 );
	omx_print_port( ctx->cam, 73 );

	omx_print_port( ctx->nll, 240 );

#ifdef DUAL_ENCODERS
	omx_print_port( ctx->spl, 250 );
	omx_print_port( ctx->spl, 251 );
	omx_print_port( ctx->spl, 252 );
#endif

#ifdef PREV_RSZ
	omx_print_port( ctx->rsz, 60 );
	omx_print_port( ctx->rsz, 61 );
#endif

#ifdef DUAL_ENCODERS
	omx_print_port( ctx->enc1, 200 );
	omx_print_port( ctx->enc1, 201 );
#endif

	omx_print_port( ctx->enc2, 200 );
	omx_print_port( ctx->enc2, 201 );


	OERR( OMX_SendCommand( ctx->cam, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_block_until_state_changed( ctx->cam, OMX_StateExecuting );
	omx_print_state( "cam", ctx->cam );
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "spl", ctx->spl );
#endif
#ifdef PREV_RSZ
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "rsz", ctx->rsz );
#endif
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "enc1", ctx->enc1 );
#endif
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "enc2", ctx->enc2 );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "nll", ctx->nll );
	printf( "Switching on capture on camera video output port 71...\n" );
	OMX_CONFIG_PORTBOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.nPortIndex = 71;
	capture.bEnabled = OMX_TRUE;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexConfigPortCapturing, &capture ) );

	usleep( 1000 * 100 );
	ctx->running = 1;
}


void video_recover( context* ctx )
{
/*
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 201, NULL ) );
	usleep( 1000 * 100 );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortEnable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortEnable, 201, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
*/
// 	video_stop( ctx );
// 	video_start( ctx );

	OMX_CONFIG_BOOLEANTYPE req;
	OMX_INIT_STRUCTURE( req );
	req.bEnabled = OMX_TRUE;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexConfigBrcmVideoRequestIFrame, &req ) );
}


void video_stop( context* ctx )
{
	printf( "Switching off capture on camera video output port 71...\n" );
	OMX_CONFIG_PORTBOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.nPortIndex = 71;
	capture.bEnabled = OMX_FALSE;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexConfigPortCapturing, &capture ) );

	printf( "Idle 1\n" );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	printf( "Idle 2\n" );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	printf( "Idle 3\n" );
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	printf( "Idle 4\n" );
#endif
#ifdef PREV_RSZ
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
#endif
	printf( "Idle 5\n" );
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
#endif
	printf( "Idle 6\n" );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	printf( "Idle 7\n" );
// 	omx_block_until_state_changed( ctx->nll, OMX_StateIdle );
	omx_block_until_state_changed( ctx->enc2, OMX_StateIdle );
	omx_block_until_state_changed( ctx->enc1, OMX_StateIdle );
// 	omx_block_until_state_changed( ctx->rsz, OMX_StateIdle );
// 	omx_block_until_state_changed( ctx->spl, OMX_StateIdle );
	omx_block_until_state_changed( ctx->cam, OMX_StateIdle );

	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 73, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 70, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 71, NULL ) );

	OERR( OMX_SendCommand( ctx->nll, OMX_CommandPortDisable, 240, NULL ) );

	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortDisable, 60, NULL ) );
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortDisable, 61, NULL ) );

#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandPortDisable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandPortDisable, 201, NULL ) );
#endif
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 201, NULL ) );

#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 250, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 251, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 252, NULL ) );
#endif


	omx_block_until_port_changed( ctx->cam, 70, OMX_FALSE );
	omx_block_until_port_changed( ctx->cam, 71, OMX_FALSE );
	omx_block_until_port_changed( ctx->cam, 73, OMX_FALSE );

	omx_block_until_port_changed( ctx->nll, 240, OMX_FALSE );

#ifdef PREV_RSZ
	omx_block_until_port_changed( ctx->rsz, 60, OMX_FALSE );
	omx_block_until_port_changed( ctx->rsz, 61, OMX_FALSE );
#endif

#ifdef DUAL_ENCODERS
	omx_block_until_port_changed( ctx->enc1, 200, OMX_FALSE );
	omx_block_until_port_changed( ctx->enc1, 201, OMX_FALSE );
#endif
	omx_block_until_port_changed( ctx->enc2, 200, OMX_FALSE );
	omx_block_until_port_changed( ctx->enc2, 201, OMX_FALSE );

#ifdef DUAL_ENCODERS
	omx_block_until_port_changed( ctx->spl, 250, OMX_FALSE );
	omx_block_until_port_changed( ctx->spl, 251, OMX_FALSE );
	omx_block_until_port_changed( ctx->spl, 252, OMX_FALSE );
#endif

	OERR( OMX_FreeBuffer( ctx->cam, 73, ctx->cambufs ) );
#ifdef DUAL_ENCODERS
	OERR( OMX_FreeBuffer( ctx->enc1, 201, ctx->enc1bufs ) );
#endif
	OERR( OMX_FreeBuffer( ctx->enc2, 201, ctx->enc2bufs ) );
// 	video_free_buffers();
	ctx->cambufs = NULL;
	ctx->enc1bufs = NULL;
	ctx->enc2bufs = NULL;

	int i;
	for ( i = 0; i < 32; i++ ) {
		if ( allocated_bufs[ i ] ) {
			vcos_free( allocated_bufs[ i ] );
			printf( "Freed buffer 0x%08X\n", (uint32_t)allocated_bufs[ i ] );
			allocated_bufs[ i ] = NULL;
		}
	}

	omx_print_state( "cam", ctx->cam );
	omx_print_state( "spl", ctx->spl );
	omx_print_state( "rsz", ctx->rsz );
	omx_print_state( "enc1", ctx->enc1 );
	omx_print_state( "enc2", ctx->enc2 );
	omx_print_state( "nll", ctx->nll );

	usleep( 1000 * 250 );
	ctx->running = 0;
}


static void config_resize( context* ctx, OMX_PARAM_PORTDEFINITIONTYPE* def )
{
	OMX_PARAM_PORTDEFINITIONTYPE imgportdef;

	OMX_INIT_STRUCTURE( imgportdef );
	imgportdef.nPortIndex = 61;
	OERR( OMX_GetParameter( ctx->rsz, OMX_IndexParamPortDefinition, &imgportdef ) );
	imgportdef.format.image.nFrameWidth  = PREV_WIDTH;
	imgportdef.format.image.nFrameHeight = PREV_HEIGHT;
	imgportdef.format.image.nStride      = ( imgportdef.format.video.nFrameWidth + imgportdef.nBufferAlignment - 1 ) & ( ~(imgportdef.nBufferAlignment - 1) );
	imgportdef.format.video.nSliceHeight = 0;
	imgportdef.format.image.nStride      = 0;
	imgportdef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	OERR( OMX_SetParameter( ctx->rsz, OMX_IndexParamPortDefinition, &imgportdef ) );

/*
	OMX_PARAM_RESIZETYPE resize;
	OMX_INIT_STRUCTURE( resize );
	resize.nPortIndex = 61;
	resize.eMode = OMX_RESIZE_BOX;
	resize.nMaxWidth = PREV_WIDTH;
	resize.nMaxHeight = PREV_HEIGHT;
	resize.nMaxBytes = PREV_WIDTH * PREV_HEIGHT * 16;
	resize.bPreserveAspectRatio = OMX_TRUE;
	resize.bAllowUpscaling = OMX_FALSE;
	OERR( OMX_SetParameter( ctx->rsz, OMX_IndexParamResize, &resize ) );
*/

	usleep( 100 );
	imgportdef.nPortIndex = 61;
	OERR( OMX_GetParameter( ctx->rsz, OMX_IndexParamPortDefinition, &imgportdef ) );
	omx_print_port( ctx->rsz, 61 );

	def->format.video.nFrameWidth = imgportdef.format.image.nFrameWidth;
	def->format.video.nFrameHeight = imgportdef.format.image.nFrameHeight;
	def->format.video.nSliceHeight = imgportdef.format.image.nSliceHeight;
	def->format.video.nStride = imgportdef.format.image.nStride;
	def->format.video.eColorFormat = imgportdef.format.image.eColorFormat;
	def->format.video.eCompressionFormat = imgportdef.format.image.eCompressionFormat;
// 	def->format.video.xFramerate = FPS << 16;
	def->format.video.xFramerate = 0;
}

context* video_configure()
{
	context* ctx;
	OMX_CONFIG_FRAMERATETYPE* framerate;
	OMX_VIDEO_PARAM_BITRATETYPE* bitrate;
	OMX_PARAM_PORTDEFINITIONTYPE* portdef;
	OMX_PARAM_PORTDEFINITIONTYPE* portdef2;
	OMX_VIDEO_PORTDEFINITIONTYPE* viddef;
	OMX_VIDEO_PORTDEFINITIONTYPE* viddef2;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pfmt;
	int i;

	OMX_PARAM_U32TYPE* extra_buffers;
	OMX_CONFIG_IMAGEFILTERPARAMSTYPE* image_filter;

	ctx = ( context* )malloc( sizeof( context ) );
	memset( ctx, 0, sizeof( context ) );
	_ctx = ctx;
	pthread_mutex_init( &ctx->lock, NULL );
	pthread_mutex_init( &ctx->mutex_enc1, NULL );
	pthread_mutex_init( &ctx->mutex_enc2, NULL );
	pthread_cond_init( &ctx->cond_enc1, NULL );
	pthread_cond_init( &ctx->cond_enc2, NULL );

	atexit( video_free_buffers );

	MAKEME( portdef, OMX_PARAM_PORTDEFINITIONTYPE );
	MAKEME( portdef2, OMX_PARAM_PORTDEFINITIONTYPE );
	MAKEME( extra_buffers, OMX_PARAM_U32TYPE );
	MAKEME( image_filter, OMX_CONFIG_IMAGEFILTERPARAMSTYPE );
	MAKEME( bitrate, OMX_VIDEO_PARAM_BITRATETYPE );
	MAKEME( pfmt, OMX_VIDEO_PARAM_PORTFORMATTYPE );

	viddef = &portdef->format.video;
	viddef2 = &portdef2->format.video;

	OERR( OMX_GetHandle( &ctx->cam, CAMNAME, ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->spl, SPLNAME, ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->rsz, RSZNAME, ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->enc1, ENCNAME, ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->enc2, ENCNAME, ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->nll, NLLNAME, ctx, &genevents ) );
/*
	OMX_PRIORITYMGMTTYPE oPriority;
	OMX_INIT_STRUCTURE( oPriority );
	oPriority.nGroupPriority = 98;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamPriorityMgmt, &oPriority ) );
	OERR( OMX_SetParameter( ctx->nll, OMX_IndexParamPriorityMgmt, &oPriority ) );
*/
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 70, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 71, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 72, NULL ) );
	OERR( OMX_SendCommand( ctx->cam, OMX_CommandPortDisable, 73, NULL ) );
	for (i = 0; i < 5; i++) {
		OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 250 + i, NULL ) );
	}
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortDisable, 60, NULL ) );
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandPortDisable, 61, NULL ) );
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandPortDisable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandPortDisable, 201, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 200, NULL ) );
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandPortDisable, 201, NULL ) );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandPortDisable, 240, NULL ) );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandPortDisable, 241, NULL ) );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandPortDisable, 242, NULL ) );

	portdef2->nPortIndex = 200;
	OERR( OMX_GetParameter( ctx->enc2, OMX_IndexParamPortDefinition, portdef2 ) );

	config_camera( ctx, portdef );

	portdef->nPortIndex = 60;
	OERR( OMX_SetParameter( ctx->rsz, OMX_IndexParamPortDefinition, portdef ) );
#ifdef PREV_RSZ
	config_resize( ctx, portdef2 );
#endif

#ifdef DUAL_ENCODERS
	portdef->nPortIndex = 200;
	OERR( OMX_SetParameter( ctx->enc1, OMX_IndexParamPortDefinition, portdef ) );
#endif
	portdef2->nPortIndex = 200;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamPortDefinition, portdef2 ) );

	portdef->nPortIndex = 250;
	OERR( OMX_SetParameter( ctx->spl, OMX_IndexParamPortDefinition, portdef ) );
	portdef->nPortIndex = 251;
	OERR( OMX_SetParameter( ctx->spl, OMX_IndexParamPortDefinition, portdef ) );
	portdef->nPortIndex = 252;
	OERR( OMX_SetParameter( ctx->spl, OMX_IndexParamPortDefinition, portdef ) );

	OMX_CONFIG_BRCMUSEPROPRIETARYCALLBACKTYPE propCb;
	OMX_INIT_STRUCTURE( propCb );
	propCb.bEnable = OMX_TRUE;
	propCb.nPortIndex = 250;
	OERR( OMX_SetParameter( ctx->spl, OMX_IndexConfigBrcmUseProprietaryCallback, &propCb ) );

#ifdef DUAL_ENCODERS
	viddef->eCompressionFormat = OMX_VIDEO_CodingAVC;
	viddef->nStride = viddef->nFrameWidth;
	viddef->nSliceHeight = viddef->nFrameHeight;
	viddef->eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	portdef->nPortIndex = 201;
	OERR( OMX_SetParameter( ctx->enc1, OMX_IndexParamPortDefinition, portdef ) );
#endif

	viddef2->eCompressionFormat = OMX_VIDEO_CodingAVC;
	viddef2->nStride = viddef2->nFrameWidth;
	viddef2->nSliceHeight = viddef2->nFrameHeight;
	viddef2->eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	portdef2->nPortIndex = 201;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamPortDefinition, portdef2 ) );
/*
	MAKEME( framerate, OMX_CONFIG_FRAMERATETYPE );
	framerate->nPortIndex = 201;
	framerate->xEncodeFramerate = FPS << 16;
#ifdef DUAL_ENCODERS
	OERR( OMX_SetParameter( ctx->enc1, OMX_IndexConfigVideoFramerate, framerate ) );
#endif
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexConfigVideoFramerate, framerate ) );
*/
	pfmt->nPortIndex = 201;
	pfmt->eCompressionFormat = OMX_VIDEO_CodingAVC;
#ifdef DUAL_ENCODERS
	OERR( OMX_SetParameter( ctx->enc1, OMX_IndexParamVideoPortFormat, pfmt ) );
#endif
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamVideoPortFormat, pfmt ) );

	bitrate->nPortIndex = 201;
	bitrate->eControlRate = OMX_Video_ControlRateVariableSkipFrames;
	bitrate->nTargetBitrate = ( 10 * 1024 * 1024 );
#ifdef DUAL_ENCODERS
	OERR( OMX_SetParameter( ctx->enc1, OMX_IndexParamVideoBitrate, bitrate ) );
#endif
	bitrate->eControlRate = OMX_Video_ControlRateVariableSkipFrames;
	bitrate->nTargetBitrate = ( PREV_BITRATE * 1024 );
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamVideoBitrate, bitrate ) );
/*
	OMX_VIDEO_CONFIG_AVCINTRAPERIOD intraPeriod;
	OMX_INIT_STRUCTURE( intraPeriod );
	intraPeriod.nPortIndex = 201;
	OERR( OMX_GetParameter( ctx->enc2, OMX_IndexConfigVideoAVCIntraPeriod, &intraPeriod ) );
	printf( "nIDRPeriod : %d\n", intraPeriod.nIDRPeriod );
	printf( "nPFrames : %d\n", intraPeriod.nPFrames );
	intraPeriod.nIDRPeriod = 30;
	intraPeriod.nPFrames = 30;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexConfigVideoAVCIntraPeriod, &intraPeriod ) );
*/
	OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
	OMX_INIT_STRUCTURE( inlinePPSSPS );
	inlinePPSSPS.nPortIndex = 201;
	inlinePPSSPS.bEnabled = OMX_TRUE;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS ) );

	OMX_VIDEO_PARAM_PROFILELEVELTYPE profile;
	OMX_INIT_STRUCTURE( profile );
	profile.nPortIndex = 201;
	profile.eProfile = OMX_VIDEO_AVCProfileBaseline;
	profile.eLevel = OMX_VIDEO_AVCLevel4;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamVideoProfileLevelCurrent, &profile ) );

	OMX_VIDEO_PARAM_QUANTIZATIONTYPE quantization;
	OMX_INIT_STRUCTURE( quantization );
	quantization.nPortIndex = 201;
	quantization.nQpI = 1;
	quantization.nQpP = 12;
	quantization.nQpB = 0;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamVideoQuantization, &quantization ) );

	OMX_VIDEO_PARAM_AVCTYPE avc;
	OMX_INIT_STRUCTURE( avc );
	avc.nPortIndex = 201;
	OERR( OMX_GetParameter( ctx->enc2, OMX_IndexParamVideoAvc, &avc ) );
// 	avc.nSliceHeaderSpacing = 15; // ??
	avc.nPFrames = 12;
	avc.nBFrames = 0;
	avc.bUseHadamard = OMX_FALSE;
// 	avc.nRefFrames = 4; // ??
	avc.nRefIdx10ActiveMinus1 = 0;
	avc.nRefIdx11ActiveMinus1 = 0;
	avc.bEnableUEP = OMX_FALSE;
	avc.bEnableFMO = OMX_FALSE;
	avc.bEnableASO = OMX_TRUE;
	avc.bEnableRS = OMX_FALSE;
	avc.eProfile = OMX_VIDEO_AVCProfileBaseline;
	avc.eLevel = OMX_VIDEO_AVCLevel3;
	avc.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP | OMX_VIDEO_PictureTypeEI | OMX_VIDEO_PictureTypeEP;
	avc.bFrameMBsOnly = OMX_TRUE;
	avc.bMBAFF = OMX_FALSE;
	avc.bEntropyCodingCABAC = OMX_FALSE;
	avc.bWeightedPPrediction = OMX_TRUE; // ??
	avc.nWeightedBipredicitonMode = OMX_FALSE;
	avc.bconstIpred = OMX_FALSE;
	avc.bDirect8x8Inference = OMX_FALSE;
	avc.bDirectSpatialTemporal = OMX_FALSE;
	avc.nCabacInitIdc = 0;
	avc.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisable;
	OERR( OMX_SetParameter( ctx->enc2, OMX_IndexParamVideoAvc, &avc ) );

	OMX_CONFIG_BOOLEANTYPE headerOnOpen;
	OMX_INIT_STRUCTURE( headerOnOpen );
	headerOnOpen.bEnabled = OMX_TRUE;
	OERR( OMX_SetConfig( ctx->enc2, OMX_IndexParamBrcmHeaderOnOpen, &headerOnOpen ) );

	OMX_CONFIG_BOOLEANTYPE lowLatency;
	OMX_INIT_STRUCTURE( lowLatency );
	lowLatency.bEnabled = OMX_TRUE;
	OERR( OMX_SetConfig( ctx->enc2, OMX_IndexConfigBrcmVideoH264LowLatency, &lowLatency ) );

#ifdef PREV_RSZ
	OERR( OMX_SetupTunnel( ctx->cam, 70, ctx->nll, 240 ) );
	OERR( OMX_SetupTunnel( ctx->cam, 71, ctx->spl, 250 ) );
	OERR( OMX_SetupTunnel( ctx->spl, 251, ctx->enc1, 200 ) );
	OERR( OMX_SetupTunnel( ctx->spl, 252, ctx->rsz, 60 ) );
	OERR( OMX_SetupTunnel( ctx->rsz, 61, ctx->enc2, 200 ) );
#elifdef DUAL_ENCODERS
	OERR( OMX_SetupTunnel( ctx->cam, 70, ctx->nll, 240 ) );
	OERR( OMX_SetupTunnel( ctx->cam, 71, ctx->spl, 250 ) );
	OERR( OMX_SetupTunnel( ctx->spl, 251, ctx->enc1, 200 ) );
	OERR( OMX_SetupTunnel( ctx->spl, 252, ctx->enc2, 200 ) );
#else
	OERR( OMX_SetupTunnel( ctx->cam, 70, ctx->nll, 240 ) );
	OERR( OMX_SetupTunnel( ctx->cam, 71, ctx->enc2, 200 ) );
#endif

	omx_print_port( ctx->cam, 73 );
	omx_print_port( ctx->cam, 70 );
	omx_print_port( ctx->cam, 71 );
	omx_print_port( ctx->cam, 72 );

	omx_print_port( ctx->spl, 250 );
	omx_print_port( ctx->spl, 251 );
	omx_print_port( ctx->spl, 252 );

	omx_print_port( ctx->rsz, 60 );
	omx_print_port( ctx->rsz, 61 );

	omx_print_port( ctx->enc1, 200 );
	omx_print_port( ctx->enc1, 201 );

	omx_print_port( ctx->enc2, 200 );
	omx_print_port( ctx->enc2, 201 );

	omx_print_port( ctx->nll, 240 );


	OERR( OMX_SendCommand( ctx->cam, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	omx_block_until_state_changed( ctx->cam, OMX_StateIdle );

#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
#endif
#ifdef PREV_RSZ
	OERR( OMX_SendCommand( ctx->rsz, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
#endif
#ifdef DUAL_ENCODERS
	OERR( OMX_SendCommand( ctx->enc1, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
#endif
	OERR( OMX_SendCommand( ctx->enc2, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->nll, OMX_CommandStateSet, OMX_StateIdle, NULL ) );

	return ctx;
}


static void config_camera( context* ctx, OMX_PARAM_PORTDEFINITIONTYPE* def )
{
	// Enable callback for camera
	OMX_CONFIG_REQUESTCALLBACKTYPE cbtype;
	OMX_INIT_STRUCTURE( cbtype );
	cbtype.nPortIndex = OMX_ALL;
	cbtype.nIndex     = OMX_IndexParamCameraDeviceNumber;
	cbtype.bEnable    = OMX_TRUE;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigRequestCallback, &cbtype ) );

	// Configure device number
	OMX_PARAM_U32TYPE device;
	OMX_INIT_STRUCTURE( device );
	device.nPortIndex = OMX_ALL;
	device.nU32 = CAM_DEVICE_NUMBER;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDeviceNumber, &device ) );

	// Set port definition
	def->nPortIndex = 70;
	OERR( OMX_GetParameter( ctx->cam, OMX_IndexParamPortDefinition, def ) );
	def->format.video.nFrameWidth  = WIDTH;
	def->format.video.nFrameHeight = HEIGHT;
// 	def->format.video.xFramerate   = 0;
	def->format.video.xFramerate   = FPS << 16;
	def->format.video.nStride      = ( def->format.video.nFrameWidth + def->nBufferAlignment - 1 ) & ( ~(def->nBufferAlignment - 1) );
	def->format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamPortDefinition, def ) );

	// Also set it for port 71 (video port)
	OERR( OMX_GetParameter( ctx->cam, OMX_IndexParamPortDefinition, def ) );
	def->nPortIndex = 71;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamPortDefinition, def ) );

	OMX_PARAM_BRCMFRAMERATERANGETYPE fps_range;
	OMX_INIT_STRUCTURE( fps_range );
	fps_range.xFramerateLow = FPS << 16;
	fps_range.xFramerateHigh = 55 << 16;
	fps_range.nPortIndex = 71;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamBrcmFpsRange, &fps_range ) );
	fps_range.nPortIndex = 71;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamBrcmFpsRange, &fps_range ) );


	/*
	 * 0 = auto. Default and same as in the past.
	 * 1 = 1080P cropped mode.
	 * 2 = 5MPix
	 * 3 = 5MPix/s 0.1666-1 fps
	 * 4 = 2x2 binned (1296x972)
	 * 5 = 2x2 binned 16:9 (1296x730)
	 * 6 = 30-60 fps VGA
	 * 7 = 60-90 fps VGA
	*/
	OMX_PARAM_U32TYPE sensorMode;
	OMX_INIT_STRUCTURE( sensorMode );
	sensorMode.nPortIndex = OMX_ALL;
	sensorMode.nU32 = 7;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraCustomSensorConfig, &sensorMode ) );

/*
	OMX_PARAM_CAMERAIMAGEPOOLTYPE pool;
	OMX_INIT_STRUCTURE( pool );
	pool.nNumHiResVideoFrames = 1;
	pool.nHiResVideoWidth = WIDTH;
	pool.nHiResVideoHeight = HEIGHT;
	pool.eHiResVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumHiResStillsFrames = 1;
	pool.nHiResStillsWidth = WIDTH;
	pool.nHiResStillsHeight = HEIGHT;
	pool.eHiResStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumLoResFrames = 1;
	pool.nLoResWidth = WIDTH;
	pool.nLoResHeight = HEIGHT;
	pool.eLoResType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumSnapshotFrames = 1;
	pool.eSnapshotType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.eInputPoolMode = OMX_CAMERAIMAGEPOOLINPUTMODE_TWOPOOLS;
	pool.nNumInputVideoFrames = 1;
	pool.nInputVideoWidth = WIDTH;
	pool.nInputVideoHeight = HEIGHT;
	pool.eInputVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumInputStillsFrames = 0;
	pool.nInputStillsWidth = WIDTH;
	pool.nInputStillsHeight = HEIGHT;
	pool.eInputStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraImagePool, &pool ) );
*/

	/*
	 * OMX_CameraDisableAlgorithmFacetracking,
	 * OMX_CameraDisableAlgorithmRedEyeReduction,
	 * OMX_CameraDisableAlgorithmVideoStabilisation,
	 * OMX_CameraDisableAlgorithmWriteRaw,
	 * OMX_CameraDisableAlgorithmVideoDenoise,
	 * OMX_CameraDisableAlgorithmStillsDenoise,
	 * OMX_CameraDisableAlgorithmAntiShake,
	 * OMX_CameraDisableAlgorithmImageEffects,
	 * OMX_CameraDisableAlgorithmDarkSubtract,
	 * OMX_CameraDisableAlgorithmDynamicRangeExpansion,
	 * OMX_CameraDisableAlgorithmFaceRecognition,
	 * OMX_CameraDisableAlgorithmFaceBeautification,
	 * OMX_CameraDisableAlgorithmSceneDetection,
	 * OMX_CameraDisableAlgorithmHighDynamicRange,
	 */
	OMX_PARAM_CAMERADISABLEALGORITHMTYPE disableAlgorithm;
	OMX_INIT_STRUCTURE( disableAlgorithm );
	disableAlgorithm.bDisabled = OMX_TRUE;
	// Sure
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFacetracking;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmRedEyeReduction;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmStillsDenoise;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFaceRecognition;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFaceBeautification;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmSceneDetection;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	// Not sure
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmAntiShake;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmVideoStabilisation;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmVideoDenoise;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	// Dunno
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmImageEffects;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmDarkSubtract;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmDynamicRangeExpansion;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmHighDynamicRange;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm ) );

	OMX_CONFIG_ZEROSHUTTERLAGTYPE zero_shutter;
	OMX_INIT_STRUCTURE( zero_shutter );
// 	zero_shutter.bZeroShutterMode = OMX_TRUE;
	zero_shutter.bZeroShutterMode = OMX_FALSE; // TBD ??
	zero_shutter.bConcurrentCapture = OMX_TRUE;
	OERR( OMX_SetParameter( ctx->cam, OMX_IndexParamCameraZeroShutterLag, &zero_shutter ) );

	OMX_CONFIG_EXPOSUREVALUETYPE exposure_value;
	OMX_INIT_STRUCTURE(exposure_value);
	exposure_value.nPortIndex = OMX_ALL;
	exposure_value.xEVCompensation = CAM_EXPOSURE_VALUE_COMPENSATION << 16;
	exposure_value.nApertureFNumber = ( 28 << 16 ) / 10;
	exposure_value.bAutoSensitivity = OMX_TRUE;
	exposure_value.nSensitivity = CAM_EXPOSURE_ISO_SENSITIVITY;
	exposure_value.bAutoShutterSpeed = OMX_FALSE;
	exposure_value.nShutterSpeedMsec = 1000 * ( 1000 / 60 - 1 ); // Actually in µs
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonExposureValue, &exposure_value ) );

	// Configure exposure type
	OMX_CONFIG_EXPOSURECONTROLTYPE exposure_type;
	OMX_INIT_STRUCTURE(exposure_type);
	exposure_type.nPortIndex = OMX_ALL;
	exposure_type.eExposureControl = OMX_ExposureControlAuto;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonExposure, &exposure_type ) );
/*
	// Enable HDR
	OMX_CONFIG_BOOLEANTYPE hdr;
	OMX_INIT_STRUCTURE( hdr );
	hdr.bEnabled = OMX_TRUE;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigBrcmHighDynamicRange, &hdr ) );
*/
	// Configure brightness
	OMX_CONFIG_BRIGHTNESSTYPE brightness;
	OMX_INIT_STRUCTURE(brightness);
	brightness.nPortIndex = OMX_ALL;
	brightness.nBrightness = CAM_BRIGHTNESS;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonBrightness, &brightness ) );

	// Configure sharpness
	OMX_CONFIG_SHARPNESSTYPE sharpness;
	OMX_INIT_STRUCTURE(sharpness);
	sharpness.nPortIndex = OMX_ALL;
	sharpness.nSharpness = CAM_SHARPNESS;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonSharpness, &sharpness ) );

	// Configure saturation
	OMX_CONFIG_SATURATIONTYPE saturation;
	OMX_INIT_STRUCTURE(saturation);
	saturation.nPortIndex = OMX_ALL;
	saturation.nSaturation = CAM_SATURATION;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonSaturation, &saturation ) );

	// Configure contrast
	OMX_CONFIG_CONTRASTTYPE contrast;
	OMX_INIT_STRUCTURE(contrast);
	contrast.nPortIndex = OMX_ALL;
	contrast.nContrast = CAM_CONTRAST;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonContrast, &contrast ) );

	// Configure frame white balance control
	OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_control;
	OMX_INIT_STRUCTURE(white_balance_control);
	white_balance_control.nPortIndex = OMX_ALL;
	white_balance_control.eWhiteBalControl = CAM_WHITE_BALANCE_CONTROL;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonWhiteBalance, &white_balance_control ) );
/*
	// Configure frame stabilisation
	OMX_CONFIG_FRAMESTABTYPE frame_stabilisation_control;
	OMX_INIT_STRUCTURE(frame_stabilisation_control);
	frame_stabilisation_control.nPortIndex = OMX_ALL;
	frame_stabilisation_control.bStab = CAM_FRAME_STABILISATION;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonFrameStabilisation, &frame_stabilisation_control ) );
*/
	// Configure mirror
	OMX_MIRRORTYPE eMirror = OMX_MirrorNone;
	if(CAM_FLIP_HORIZONTAL && !CAM_FLIP_VERTICAL) {
		eMirror = OMX_MirrorHorizontal;
	} else if(!CAM_FLIP_HORIZONTAL && CAM_FLIP_VERTICAL) {
		eMirror = OMX_MirrorVertical;
	} else if(CAM_FLIP_HORIZONTAL && CAM_FLIP_VERTICAL) {
		eMirror = OMX_MirrorBoth;
	}
	OMX_CONFIG_MIRRORTYPE mirror;
	OMX_INIT_STRUCTURE(mirror);
	mirror.nPortIndex = 70;
	mirror.eMirror = eMirror;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonMirror, &mirror ) );
	mirror.nPortIndex = 71;
	OERR( OMX_SetConfig( ctx->cam, OMX_IndexConfigCommonMirror, &mirror ) );

	while ( !ctx->camera_ready ) {
		usleep( 100 * 1000 );
	}
}


OMX_BUFFERHEADERTYPE* omx_allocbufs( OMX_HANDLETYPE handle, int port, int enable )
{
	printf( "omx_allocbufs( 0x%08X, %d, %d )\n", (uint32_t)handle, port, enable );
	int i;
	OMX_BUFFERHEADERTYPE* list = NULL;
	OMX_BUFFERHEADERTYPE** end = &list;
	OMX_PARAM_PORTDEFINITIONTYPE* portdef;

	MAKEME( portdef, OMX_PARAM_PORTDEFINITIONTYPE );
	portdef->nPortIndex = port;
	OERR( OMX_GetParameter( handle, OMX_IndexParamPortDefinition, portdef ) );

	if ( enable ) {
		OERR(OMX_SendCommand( handle, OMX_CommandPortEnable, port, NULL ) );
		omx_block_until_port_changed( handle, port, OMX_TRUE );
	}

	printf( "   portdef->nBufferCountActual : %d\n   portdef->nBufferSize : %d\n", portdef->nBufferCountActual, portdef->nBufferSize );

	for ( i = 0; i < portdef->nBufferCountActual; i++ ) {
		OMX_U8* buf;

		buf = vcos_malloc_aligned( portdef->nBufferSize, portdef->nBufferAlignment, "buffer" );
		if ( buf ) {
			allocated_bufs[ allocated_bufs_idx++ ] = buf;
		}
		printf( "Allocated a buffer of %d bytes\n", portdef->nBufferSize );
		OERR( OMX_UseBuffer( handle, end, port, NULL, portdef->nBufferSize, buf ) );
		end = ( OMX_BUFFERHEADERTYPE** )&( (*end)->pAppPrivate );
	}

	free( portdef );

	return list;
}


static void video_free_buffers()
{
	printf( "video_free_buffers()\n" );

	_ctx->exiting = 1;
	_ctx->enc1_data_avail = 0;
	_ctx->enc2_data_avail = 0;
	while ( _ctx->running ) {
		pthread_mutex_lock( &_ctx->mutex_enc1 );
		pthread_cond_signal( &_ctx->cond_enc1 );
		pthread_mutex_unlock( &_ctx->mutex_enc1 );
		pthread_mutex_lock( &_ctx->mutex_enc2 );
		pthread_cond_signal( &_ctx->cond_enc2 );
		pthread_mutex_unlock( &_ctx->mutex_enc2 );
		usleep( 1000 * 10 );
	}

	int i;
	for ( i = 0; i < 32; i++ ) {
		if ( allocated_bufs[ i ] ) {
			vcos_free( allocated_bufs[ i ] );
			printf( "Freed buffer 0x%08X\n", (uint32_t)allocated_bufs[ i ] );
			allocated_bufs[ i ] = NULL;
		}
	}

	if ( _ctx->cam ) {
		printf( "FreeHandle( cam ) : %08X\n", OMX_FreeHandle( _ctx->cam ) );
	}

	if ( _ctx->spl ) {
		printf( "FreeHandle( spl ) : %08X\n", OMX_FreeHandle( _ctx->spl ) );
	}

#ifdef PREV_RSZ
	if ( _ctx->rsz ) {
		printf( "FreeHandle( spl ) : %08X\n", OMX_FreeHandle( _ctx->spl ) );
	}
#endif

	if ( _ctx->enc1 ) {
		printf( "FreeHandle( enc1 ) : %08X\n", OMX_FreeHandle( _ctx->enc1 ) );
	}
	if ( _ctx->enc2 ) {
		printf( "FreeHandle( enc2 ) : %08X\n", OMX_FreeHandle( _ctx->enc2 ) );
	}

	if ( _ctx->nll ) {
		printf( "FreeHandle( nll ) : %08X\n", OMX_FreeHandle( _ctx->nll ) );
	}

}


static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( event != OMX_EventCmdComplete ) {
		printf( "event on 0x%08X type %d\n", (uint32_t)component, event );
	}
	if ( component == ctx->cam ) {
		if ( event == OMX_EventParamOrConfigChanged ) {
			ctx->camera_ready = 1;
		}
	}
	if ( event == OMX_EventError ) {
		printf( "ERROR : %08X %08X, %p\n", data1, data2, eventdata );
	}

	return OMX_ErrorNone;
}


static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
	printf( "emptied( 0x%08X )\n", (uint32_t)component );
	return OMX_ErrorNone;
}


static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
// 	printf( "filled( 0x%08X )\n", (uint32_t)component );
	pthread_mutex_lock( &ctx->lock );
	if ( component == ctx->enc1 ) {
		pthread_mutex_lock( &ctx->mutex_enc1 );
		ctx->enc1_data_avail = 1;
		pthread_cond_signal( &ctx->cond_enc1 );
		pthread_mutex_unlock( &ctx->mutex_enc1 );
	}
	if ( component == ctx->enc2 ) {
		pthread_mutex_lock( &ctx->mutex_enc2 );
		ctx->enc2_data_avail = 1;
		pthread_cond_signal( &ctx->cond_enc2 );
		pthread_mutex_unlock( &ctx->mutex_enc2 );
	}
	pthread_mutex_unlock( &ctx->lock );
	return OMX_ErrorNone;
}


static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def )
{
	printf("Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u @%ufps %u\n",
		def.nPortIndex,
		def.eDir == OMX_DirInput ? "in" : "out",
		def.nBufferCountActual,
		def.nBufferCountMin,
		def.nBufferSize,
		def.nBufferAlignment,
		def.bEnabled ? "enabled" : "disabled",
		def.bPopulated ? "populated" : "not pop.",
		def.bBuffersContiguous ? "contig." : "not cont.",
		def.format.video.nFrameWidth,
		def.format.video.nFrameHeight,
		def.format.video.nStride,
		def.format.video.nSliceHeight,
		def.format.video.xFramerate >> 16, def.format.video.eColorFormat);
}


void omx_print_port( OMX_HANDLETYPE component, OMX_U32 port )
{
	OMX_PARAM_PORTDEFINITIONTYPE def;

	memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	def.nVersion.nVersion = OMX_VERSION;
	def.nPortIndex = port;

	if ( OMX_GetParameter( component, OMX_IndexParamPortDefinition, &def ) != OMX_ErrorNone ) {
		printf( "%s:%d: OMX_GetParameter() for port %d failed!\n", __FUNCTION__, __LINE__, port );
		exit(1);
	}

	print_def( def );
}


void omx_block_until_port_changed( OMX_HANDLETYPE component, OMX_U32 nPortIndex, OMX_BOOL bEnabled )
{
	printf( "Wating port %d to be %d\n", nPortIndex, bEnabled );
	OMX_ERRORTYPE r;
	OMX_PARAM_PORTDEFINITIONTYPE portdef;
	OMX_INIT_STRUCTURE(portdef);
	portdef.nPortIndex = nPortIndex;
	OMX_U32 i = 0;
	while ( i++ == 0 || portdef.bEnabled != bEnabled ) {
		if ( ( r = OMX_GetParameter( component, OMX_IndexParamPortDefinition, &portdef ) ) != OMX_ErrorNone ) {
			printf( "omx_block_until_port_changed: Failed to get port definition : %d", r );
		}
		if ( portdef.bEnabled != bEnabled ) {
			usleep(1000);
		}
		if ( i % 1000 == 0 ) {
// 			print_def( portdef );
		}
	}
}


void omx_block_until_state_changed( OMX_HANDLETYPE component, OMX_STATETYPE state )
{
	printf( "Wating state to be %d\n", state );
	OMX_ERRORTYPE r;
	OMX_STATETYPE st = -1;
	OMX_U32 i = 0;
	while ( i++ == 0 || st != state ) {
		if ( ( r = OMX_GetState( component, &st ) ) != OMX_ErrorNone ) {
			printf( "omx_block_until_port_changed: Failed to get port definition : %d", r );
		}
		if ( st != state ) {
			usleep(10000);
		}
	}
}
