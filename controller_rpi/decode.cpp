/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <gammaengine/Debug.h>
#include <wiringPi.h>
#include "decode.h"

using namespace GE;

static OMX_VERSIONTYPE SpecificationVersion = {
	{ 1, 1, 2, 0 }
};

#define MAKEME( y, x ) \
	y = (x*)calloc( 1, sizeof( *y ) ); \
	y->nSize = sizeof( *y ); \
	y->nVersion = SpecificationVersion; \

#define MAKEME2( y, x, z ) \
	y = (x*)calloc( 1, sizeof( x ) + z ); \
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

static video_context* _ctx = nullptr;
static OMX_U8* allocated_bufs[128] = { NULL };
static int allocated_bufs_idx = 0;

OMX_BUFFERHEADERTYPE* omx_allocbufs( OMX_HANDLETYPE handle, int port, int enable );
void omx_block_until_port_changed( OMX_HANDLETYPE component, OMX_U32 nPortIndex, OMX_BOOL bEnabled );
void omx_block_until_state_changed( OMX_HANDLETYPE component, OMX_STATETYPE state );
void omx_print_port( OMX_HANDLETYPE component, OMX_U32 port );
static void omx_print_state( const char* name, OMX_HANDLETYPE comp );
static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def );
static void video_free_buffers();

static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, video_context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, video_context* ctx, OMX_BUFFERHEADERTYPE* buf );
static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, video_context* ctx, OMX_BUFFERHEADERTYPE* buf );

static OMX_CALLBACKTYPE genevents = {
	(OMX_ERRORTYPE (*)( OMX_HANDLETYPE, void*, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR )) genericeventhandler,
	(OMX_ERRORTYPE (*)( OMX_HANDLETYPE, void*, OMX_BUFFERHEADERTYPE* )) emptied,
	(OMX_ERRORTYPE (*)( OMX_HANDLETYPE, void*, OMX_BUFFERHEADERTYPE* )) filled
};


OMX_BUFFERHEADERTYPE* video_decoder_buffer( video_context* ctx )
{
	return 0;
}

// #define SHARPEN

void video_decode_frame( video_context* ctx )
{
// 	fDebug( ctx );

	pthread_mutex_lock( &ctx->lock );
	if ( ctx->video_running == 0 and ctx->decoder_valid == 1 ) {
		ctx->video_running = 1;
		piHiPri( 99 );
		pthread_mutex_unlock( &ctx->lock );
		OMX_CONFIG_DISPLAYREGIONTYPE region;
		OMX_INIT_STRUCTURE( region );
		region.nPortIndex = 90;
		region.set = (OMX_DISPLAYSETTYPE)( OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_DEST_RECT | OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_LAYER );
		region.fullscreen = OMX_FALSE;
		int reduce = 1;
		region.dest_rect.x_offset = ( reduce * ctx->width * 0.05 );
		region.dest_rect.y_offset = ctx->stereo * 56 + ( reduce * ctx->height * 0.1 );
		region.dest_rect.width = ctx->width / ( 1 + ctx->stereo ) - ( reduce * ctx->width * 0.1 );
		region.dest_rect.height = ctx->height - ( ctx->stereo * 112 ) - ( reduce * ctx->height * 0.2 );
		region.noaspect = OMX_TRUE;
		region.layer = 0;
		OERR( OMX_SetConfig( ctx->rdr1, OMX_IndexConfigDisplayRegion, &region ) );
		if ( ctx->stereo ) {
			region.dest_rect.x_offset = ctx->width / 2 + ( reduce * ctx->width * 0.05 );
			region.dest_rect.width = ctx->width / 2 - ( reduce * ctx->width * 0.1 );
			OERR( OMX_SetConfig( ctx->rdr2, OMX_IndexConfigDisplayRegion, &region ) );
		}

		// Configure mirror
		OMX_CONFIG_MIRRORTYPE mirror;
		OMX_INIT_STRUCTURE(mirror);
		mirror.nPortIndex = 90;
		mirror.eMirror = OMX_MirrorVertical;
		OERR( OMX_SetConfig( ctx->rdr1, OMX_IndexConfigCommonMirror, &mirror ) );
		if ( ctx->stereo ) {
			OERR( OMX_SetConfig( ctx->rdr2, OMX_IndexConfigCommonMirror, &mirror ) );
		}
#ifdef SHARPEN
		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = 131;
		OERR( OMX_GetParameter( ctx->dec, OMX_IndexParamPortDefinition, &def ) );
		def.eDomain = OMX_PortDomainVideo;
		print_def( def );
		def.nPortIndex = 190;
		OERR( OMX_SetParameter( ctx->fx, OMX_IndexParamPortDefinition, &def ) );

		OMX_CONFIG_IMAGEFILTERTYPE filter;
		OMX_INIT_STRUCTURE( filter );
		filter.nPortIndex = 191;
		filter.eImageFilter = OMX_ImageFilterSharpen;
		OERR( OMX_SetParameter( ctx->fx, OMX_IndexConfigCommonImageFilter, &filter ) );
#endif
		if ( ctx->stereo ) {
#ifdef SHARPEN
			OERR( OMX_SetupTunnel( ctx->dec, 131, ctx->fx, 190 ) );
			OERR( OMX_SetupTunnel( ctx->fx, 191, ctx->spl, 250 ) );
#else
			OERR( OMX_SetupTunnel( ctx->dec, 131, ctx->spl, 250 ) );
#endif
			OERR( OMX_SetupTunnel( ctx->spl, 251, ctx->rdr1, 90 ) );
			OERR( OMX_SetupTunnel( ctx->spl, 252, ctx->rdr2, 90 ) );
		} else {
#ifdef SHARPEN
			OERR( OMX_SetupTunnel( ctx->dec, 131, ctx->fx, 190 ) );
			OERR( OMX_SetupTunnel( ctx->fx, 191, ctx->rdr1, 90 ) );
#else
			OERR( OMX_SetupTunnel( ctx->dec, 131, ctx->rdr1, 90 ) );
#endif
		}

		OERR( OMX_SendCommand( ctx->dec, OMX_CommandPortEnable, 131, NULL ) );
#ifdef SHARPEN
		OERR( OMX_SendCommand( ctx->fx, OMX_CommandPortEnable, 190, NULL ) );
		OERR( OMX_SendCommand( ctx->fx, OMX_CommandPortEnable, 191, NULL ) );
#endif
		if ( ctx->stereo ) {
			OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 250, NULL ) );
			OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 251, NULL ) );
			OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortEnable, 252, NULL ) );
			OERR( OMX_SendCommand( ctx->rdr2, OMX_CommandPortEnable, 90, NULL ) );
		}
		OERR( OMX_SendCommand( ctx->rdr1, OMX_CommandPortEnable, 90, NULL ) );

		omx_block_until_port_changed( ctx->dec, 131, OMX_TRUE );
#ifdef SHARPEN
		omx_block_until_port_changed( ctx->fx, 190, OMX_TRUE );
		omx_block_until_port_changed( ctx->fx, 191, OMX_TRUE );
#endif
		if ( ctx->stereo ) {
			omx_block_until_port_changed( ctx->spl, 250, OMX_TRUE );
			omx_block_until_port_changed( ctx->spl, 251, OMX_TRUE );
			omx_block_until_port_changed( ctx->spl, 252, OMX_TRUE );
			omx_block_until_port_changed( ctx->rdr2, 90, OMX_TRUE );
		}
		omx_block_until_port_changed( ctx->rdr1, 90, OMX_TRUE );
		omx_print_port( ctx->dec, 130 );
		omx_print_port( ctx->dec, 131 );
#ifdef SHARPEN
		omx_print_port( ctx->fx, 190 );
		omx_print_port( ctx->fx, 191 );
#endif
		omx_print_port( ctx->spl, 250 );
		omx_print_port( ctx->spl, 251 );
		omx_print_port( ctx->spl, 252 );
		omx_print_port( ctx->rdr1, 90 );
		omx_print_port( ctx->rdr2, 90 );
		if ( ctx->stereo ) {
			OERR( OMX_SendCommand( ctx->spl, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
			omx_block_until_state_changed( ctx->spl, OMX_StateExecuting );
		}
#ifdef SHARPEN
		OERR( OMX_SendCommand( ctx->fx, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
		omx_block_until_state_changed( ctx->fx, OMX_StateExecuting );
#endif
		OERR( OMX_SendCommand( ctx->rdr1, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
		if ( ctx->stereo ) {
			OERR( OMX_SendCommand( ctx->rdr2, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
		}
		omx_block_until_state_changed( ctx->rdr1, OMX_StateExecuting );
		if ( ctx->stereo ) {
			omx_block_until_state_changed( ctx->rdr2, OMX_StateExecuting );
		}
		printf( "Video running !\n" );
	}
	pthread_mutex_unlock( &ctx->lock );

	ctx->decinput->nOffset = 0;
	if ( ctx->first_frame ) {
		ctx->first_frame = 0;
		ctx->decinput->nFlags = OMX_BUFFERFLAG_STARTTIME;
	} else {
		ctx->decinput->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
	}

	uint32_t err = OMX_ErrorNone;
// 	omx_print_state( "dec", ctx->dec );
	if ( ( err = OMX_EmptyThisBuffer( ctx->dec, ctx->decinput ) ) != OMX_ErrorNone ) {
		printf( "OMX_FillThisBuffer failed in video_decode_frame (0x%08X)\n", err );
		exit(1);
	}
}


void video_start( video_context* ctx )
{
	omx_print_state( "dec", ctx->dec );
	omx_print_state( "fx", ctx->fx );
	omx_print_state( "spl", ctx->spl );
	omx_print_state( "rdr1", ctx->rdr1 );
	omx_print_state( "rdr2", ctx->rdr2 );

	ctx->decbufs = omx_allocbufs( ctx->dec, 130, 1 );
	omx_block_until_port_changed( ctx->dec, 130, OMX_TRUE );
	ctx->decinput = ctx->decbufs;

	omx_block_until_port_changed( ctx->dec, 130, OMX_TRUE );

	omx_print_port( ctx->dec, 130 );
	omx_print_port( ctx->dec, 131 );
	omx_print_port( ctx->spl, 250 );
	omx_print_port( ctx->spl, 251 );
	omx_print_port( ctx->spl, 252 );
	omx_print_port( ctx->rdr1, 90 );
	omx_print_port( ctx->rdr2, 90 );

	OERR( OMX_SendCommand( ctx->dec, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_block_until_state_changed( ctx->dec, OMX_StateExecuting );
	omx_print_state( "dec", ctx->dec );

	ctx->running = 1;
}


void video_stop( video_context* ctx )
{
	// TODO (or not :P)
}


video_context* video_configure( int width, int height, int stereo )
{
	video_context* ctx;
	OMX_PARAM_PORTDEFINITIONTYPE* portdef;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pfmt;

	ctx = ( video_context* )malloc( sizeof( video_context ) );
	memset( ctx, 0, sizeof( video_context ) );
	_ctx = ctx;
	memset( allocated_bufs, 0, sizeof( allocated_bufs ) );
	ctx->first_frame = 1;

	ctx->width = width;
	ctx->height = height;
	ctx->stereo = stereo;

	atexit( video_free_buffers );

	MAKEME( portdef, OMX_PARAM_PORTDEFINITIONTYPE );
	MAKEME( pfmt, OMX_VIDEO_PARAM_PORTFORMATTYPE );

	OMX_Init();
	OERR( OMX_GetHandle( &ctx->dec, (char*)"OMX.broadcom.video_decode", ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->fx, (char*)"OMX.broadcom.image_fx", ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->spl, (char*)"OMX.broadcom.video_splitter", ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->rdr1, (char*)"OMX.broadcom.video_render", ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->rdr2, (char*)"OMX.broadcom.video_render", ctx, &genevents ) );

	OERR( OMX_SendCommand( ctx->dec, OMX_CommandPortDisable, 130, NULL ) );
	OERR( OMX_SendCommand( ctx->dec, OMX_CommandPortDisable, 131, NULL ) );
	OERR( OMX_SendCommand( ctx->fx, OMX_CommandPortDisable, 190, NULL ) );
	OERR( OMX_SendCommand( ctx->fx, OMX_CommandPortDisable, 191, NULL ) );
	OERR( OMX_SendCommand( ctx->rdr1, OMX_CommandPortDisable, 90, NULL ) );
	OERR( OMX_SendCommand( ctx->rdr2, OMX_CommandPortDisable, 90, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 250, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 251, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 252, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 253, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandPortDisable, 254, NULL ) );
/*
	OMX_CONFIG_IMAGEFILTERTYPE filter;
	OMX_INIT_STRUCTURE( filter );
	filter.nPortIndex = 191;
	filter.eImageFilter = OMX_ImageFilterSharpen;
	OERR( OMX_SetParameter( ctx->fx, OMX_IndexConfigCommonImageFilter, &filter ) );
*/
/*
	OMX_CONFIG_IMAGEFILTERPARAMSTYPE filter_params;
	OMX_INIT_STRUCTURE( filter_params );
	filter_params.nPortIndex = 191;
	filter_params.nNumParams = 1;
	filter_params.eImageFilter = OMX_ImageFilterSharpen;
	filter_params.nParams[0] = 1;
	filter_params.nParams[1] = 40;
	filter_params.nParams[2] = 20;
	OERR( OMX_SetParameter( ctx->fx, OMX_IndexConfigCommonImageFilterParameters, &filter_params ) );
*/
	omx_print_port( ctx->dec, 130 );
	omx_print_port( ctx->dec, 131 );

	omx_print_port( ctx->fx, 190 );
	omx_print_port( ctx->fx, 191 );

	omx_print_port( ctx->rdr1, 90 );
	omx_print_port( ctx->rdr2, 90 );

	OERR( OMX_SendCommand( ctx->dec, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->fx, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->rdr1, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->rdr2, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->spl, OMX_CommandStateSet, OMX_StateIdle, NULL ) );

	pfmt->nPortIndex = 130;
	pfmt->eCompressionFormat = OMX_VIDEO_CodingAVC;
	OERR( OMX_SetParameter( ctx->dec, OMX_IndexParamVideoPortFormat, pfmt ) );

	return ctx;
}


OMX_BUFFERHEADERTYPE* omx_allocbufs( OMX_HANDLETYPE handle, int port, int enable )
{
	printf( "omx_allocbufs( 0x%08X, %d, %d )\n", (uint32_t)handle, port, enable );
	uint32_t i;
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

		buf = (OMX_U8*)vcos_malloc_aligned( portdef->nBufferSize, portdef->nBufferAlignment, "buffer" );
		if ( buf ) {
			allocated_bufs[ allocated_bufs_idx++ ] = buf;
		}
		printf( "Allocated a buffer of %d bytes\n", portdef->nBufferSize );
		OERR( OMX_UseBuffer( handle, end, port, nullptr, portdef->nBufferSize, buf ) );
		end = ( OMX_BUFFERHEADERTYPE** )&( (*end)->pAppPrivate );
	}

	free( portdef );

	return list;
}


static void video_free_buffers()
{
	int i;
	for ( i = 0; i < 32; i++ ) {
		if ( allocated_bufs[ i ] ) {
			vcos_free( allocated_bufs[ i ] );
			printf( "Freed buffer 0x%08X\n", (uint32_t)allocated_bufs[ i ] );
		}
	}
	if ( _ctx->dec ) {
		printf( "FreeHandle( dec ) : %08X\n", OMX_FreeHandle( _ctx->dec ) );
	}
	if ( _ctx->rdr1 ) {
		printf( "FreeHandle( rdr1 ) : %08X\n", OMX_FreeHandle( _ctx->rdr1 ) );
	}
	if ( _ctx->rdr2 ) {
		printf( "FreeHandle( rdr2 ) : %08X\n", OMX_FreeHandle( _ctx->rdr2 ) );
	}
}


static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, video_context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( event != OMX_EventCmdComplete ) {
		printf( "event on 0x%08X type %d\n", (uint32_t)component, event );
	}
	if ( event == OMX_EventError ) {
		printf( "ERROR : %08X %08X, %p\n", data1, data2, eventdata );
// 		exit(1);
	}
	pthread_mutex_lock( &ctx->lock );
	if ( event == OMX_EventPortSettingsChanged and component == _ctx->dec and ctx->decoder_valid == 0 ) {
		ctx->decoder_valid = 1;
	}
	pthread_mutex_unlock( &ctx->lock );
	return OMX_ErrorNone;
}


static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, video_context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
	return OMX_ErrorNone;
}


static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, video_context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
	return OMX_ErrorNone;
}


static void omx_print_state( const char* name, OMX_HANDLETYPE comp )
{
	OMX_STATETYPE st;
	OERRq( OMX_GetState( comp, &st ) );
	printf( "%s state : 0x%08X\n", name, st );
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
	OMX_STATETYPE st = OMX_StateInvalid;
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
