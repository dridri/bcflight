#include <pthread.h>
#include <libavutil/mathematics.h>
// #include "encode.h"
#include "audio.h"

static void* audio_input_thread( audio_context* ctx );
static void alsa_init( audio_context* ctx );
static void audio_encode_init( audio_context* ctx );

audio_context* audio_configure()
{
	audio_context* ctx = ( audio_context* )malloc( sizeof( audio_context ) );
	memset( ctx, 0, sizeof( audio_context ) );

	ctx->pcm.samples = 2048;
	ctx->pcm.size = ctx->pcm.samples * snd_pcm_format_width( SND_PCM_FORMAT_S16_LE ) / 8;
	ctx->pcm.ptr = malloc( ctx->pcm.size );

	ctx->mp3.ptr = malloc( sizeof( uint16_t ) * 2048 );

	alsa_init( ctx );
	audio_encode_init( ctx );

	pthread_create( &ctx->capture_thread, NULL, (void*(*)(void*))&audio_input_thread, ctx );
	pthread_setname_np( ctx->capture_thread, "audio_encode" );

	return ctx;
}


void audio_start( audio_context* ctx )
{
	ctx->running = 1;
}


void audio_stop( audio_context* ctx )
{
	ctx->running = 0;
}


int audio_fill_buffer( audio_context* ctx, AVCodec* enc )
{
	if ( !ctx->running ) {
		return 0;
	}

	pthread_mutex_lock( &ctx->lock );
	pthread_mutex_unlock( &ctx->capture_lock );
	pthread_mutex_unlock( &ctx->lock );
	return 0;
}


static void* audio_input_thread( audio_context* ctx )
{
	AVPacket pkt;
	int got_output = 0;

	pthread_mutex_lock( &ctx->capture_lock );

	while ( 1 ) {
// 		pthread_mutex_lock( &ctx->capture_lock );

		int frames = snd_pcm_readi( ctx->capture, ctx->pcm.ptr, ctx->pcm.samples );

		if ( got_output ) {
			av_free_packet( &pkt );
		}
		av_init_packet( &pkt );
		pkt.data = NULL;
		pkt.size = 0;
		int ret = avcodec_encode_audio2( ctx->av_context, &pkt, ctx->frame, &got_output );
		pthread_mutex_lock( &ctx->capture_lock );
		if ( got_output && ret >= 0 ) {
// 			ctx->mp3.ptr = (char*)pkt.data;
			memcpy( ctx->mp3.ptr, (char*)pkt.data, ctx->mp3.size );
			ctx->mp3.size = pkt.size;
			ctx->mp3.pts = ctx->av_context->coded_frame->pts;
			pthread_mutex_lock( &ctx->lock );
			ctx->encode_data_avail = 1;
			pthread_mutex_unlock( &ctx->lock );
		} else {
			printf( "encode error : %d (%d)\n", ret, got_output );
		}
	}

	return NULL;
}


static void audio_encode_init( audio_context* ctx )
{
	avcodec_register_all();
	printf( "1\n" );
	ctx->encode = avcodec_find_encoder( CODEC_ID_MP2 );
	printf( "2 %p\n", ctx->encode );
	ctx->av_context = avcodec_alloc_context3( ctx->encode );
	printf( "3\n" );
	ctx->av_context->bit_rate = 128000;
	ctx->av_context->sample_fmt = AV_SAMPLE_FMT_S16;
	ctx->av_context->sample_rate = 44100;
	ctx->av_context->channel_layout = AV_CH_LAYOUT_MONO;
	ctx->av_context->channels = 1;
	printf( "4\n" );

	if ( avcodec_open2( ctx->av_context, ctx->encode, NULL ) < 0 ) {
		fprintf( stdout, "Could not open audio codec\n" );
		exit(1);
	}

	ctx->frame = avcodec_alloc_frame();
	ctx->frame->nb_samples = ctx->av_context->frame_size;
	ctx->frame->format = ctx->av_context->sample_fmt;

	int ret = avcodec_fill_audio_frame( ctx->frame, ctx->av_context->channels, ctx->av_context->sample_fmt, (const uint8_t*)ctx->pcm.ptr, ctx->pcm.size, 0 );
	if (ret < 0) {
		fprintf( stdout, "Could not setup audio frame (%d)\n", ret );
		exit(1);
	}
}

static void alsa_init( audio_context* ctx )
{
	int err = 0;
	unsigned int rate = 44100;
	snd_pcm_hw_params_t* hw_params;

	if ( ( err = snd_pcm_open( &ctx->capture, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot open audio device %s (%s)\n", "plughw:1,0", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "audio interface opened\n");

	if ( ( err = snd_pcm_hw_params_malloc( &hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params allocated\n");

	if ( ( err = snd_pcm_hw_params_any( ctx->capture, hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params initialized\n");

	if ( ( err = snd_pcm_hw_params_set_access( ctx->capture, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 ) {
		fprintf( stderr, "cannot set access type (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params access setted\n");

	if ( ( err = snd_pcm_hw_params_set_format( ctx->capture, hw_params, SND_PCM_FORMAT_S16_LE ) ) < 0 ) {
		fprintf( stderr, "cannot set sample format (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params format setted\n");

	if ( ( err = snd_pcm_hw_params_set_rate_near( ctx->capture, hw_params, &rate, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot set sample rate (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params rate setted to %d\n", rate);

	if ( ( err = snd_pcm_hw_params_set_channels( ctx->capture, hw_params, 1 ) ) < 0 ) {
		fprintf( stderr, "cannot set channel count (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params channels setted\n");

	if ( ( err = snd_pcm_hw_params( ctx->capture, hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot set parameters (%s)\n", snd_strerror( err ) );
		exit(1);
	}
	fprintf( stdout, "hw_params setted\n" );

	snd_pcm_hw_params_free( hw_params );
	fprintf( stdout, "hw_params freed\n" );
		
	if ( ( err = snd_pcm_prepare ( ctx->capture ) ) < 0 ) {
		fprintf( stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror( err ) );
		exit(1);
	}

	fprintf( stdout, "audio interface prepared\n" );
}

/*
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

OMX_BUFFERHEADERTYPE* omx_allocbufs( OMX_HANDLETYPE handle, int port, int enable );
void omx_block_until_port_changed( OMX_HANDLETYPE* component, OMX_U32 nPortIndex, OMX_BOOL bEnabled );
void omx_block_until_state_changed( OMX_HANDLETYPE* component, OMX_STATETYPE state );
void omx_print_state( const char* name, OMX_HANDLETYPE comp );
void omx_print_port( OMX_HANDLETYPE* component, OMX_U32 port );

static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, audio_context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, audio_context* ctx, OMX_BUFFERHEADERTYPE* buf );
static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, audio_context* ctx, OMX_BUFFERHEADERTYPE* buf );

static void alsa_init( audio_context* ctx );

static OMX_CALLBACKTYPE genevents = {
	(void (*)) genericeventhandler,
	(void (*)) emptied,
	(void (*)) filled
};

audio_context* audio_configure()
{
	OMX_PARAM_PORTDEFINITIONTYPE portdef;
	OMX_AUDIO_PARAM_PORTFORMATTYPE format;
	OMX_AUDIO_PARAM_PCMMODETYPE pcm;
	OMX_CONFIG_BRCMAUDIOSOURCETYPE source;
	OMX_AUDIO_CONFIG_VOLUMETYPE volume;
	OMX_AUDIO_CONFIG_MUTETYPE mute;
	OMX_AUDIO_PARAM_MP3TYPE mp3;

	audio_context* ctx = ( audio_context* )malloc( sizeof( audio_context ) );
	memset( ctx, 0, sizeof( audio_context ) );

	OERR( OMX_GetHandle( &ctx->capture, "OMX.broadcom.audio_capture", ctx, &genevents ) );
	OERR( OMX_GetHandle( &ctx->encode, "OMX.broadcom.audio_encode", ctx, &genevents ) );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandPortDisable, 180, NULL ) );
	OERR( OMX_SendCommand( ctx->capture, OMX_CommandPortDisable, 181, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortDisable, 160, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortDisable, 161, NULL ) );
	omx_block_until_port_changed( ctx->capture, 180, OMX_FALSE );
	omx_block_until_port_changed( ctx->capture, 181, OMX_FALSE );
	omx_block_until_port_changed( ctx->encode, 160, OMX_FALSE );
	omx_block_until_port_changed( ctx->encode, 161, OMX_FALSE );

	// Capture
	OMX_INIT_STRUCTURE( portdef );
	portdef.nPortIndex = 180;
	OERR( OMX_GetParameter( ctx->capture, OMX_IndexParamPortDefinition, &portdef ) );
	portdef.nBufferSize = 16384;
	portdef.nBufferAlignment = 32;
	portdef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexParamPortDefinition, &portdef ) );

	OMX_INIT_STRUCTURE( format );
	format.nPortIndex = 180;
	format.eEncoding = OMX_AUDIO_CodingPCM;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexParamAudioPortFormat, &format ) );

	OMX_INIT_STRUCTURE( pcm );
	pcm.nPortIndex = 180;
	pcm.eNumData = OMX_NumericalDataSigned;
	pcm.ePCMMode = OMX_AUDIO_PCMModeLinear;
	pcm.bInterleaved = OMX_TRUE;
	pcm.eEndian = OMX_EndianLittle;
	pcm.nSamplingRate = 44100;
	pcm.nBitPerSample = 16;
	pcm.nChannels = 2;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexParamAudioPcm, &pcm ) );

	OMX_INIT_STRUCTURE( source );
	strcpy( (char*)source.sName, "plughw:1,0" );
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexConfigBrcmAudioSource, &source ) );

	OMX_INIT_STRUCTURE( volume );
	volume.nPortIndex = 180;
	volume.bLinear = OMX_TRUE;
	volume.sVolume.nValue = 100;
	volume.sVolume.nMin = 0;
	volume.sVolume.nMax = 100;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexConfigAudioVolume, &volume ) );

	OMX_INIT_STRUCTURE( mute );
	mute.nPortIndex = 180;
	mute.bMute = OMX_FALSE;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexConfigAudioMute, &mute ) );

	// Encode
	OMX_INIT_STRUCTURE( portdef );
	portdef.nPortIndex = 160;
	OERR( OMX_GetParameter( ctx->encode, OMX_IndexParamPortDefinition, &portdef ) );
	////
	portdef.eDir = OMX_DirInput;
	portdef.eDomain = OMX_PortDomainAudio;
	////
	portdef.nBufferAlignment = 32;
	portdef.nBufferSize = 16384;
	portdef.nBufferCountMin = 1;
	portdef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	OERR( OMX_SetParameter( ctx->encode, OMX_IndexParamPortDefinition, &portdef ) );

	OMX_INIT_STRUCTURE( format );
	format.nPortIndex = 160;
	format.eEncoding = OMX_AUDIO_CodingPCM;
	OERR( OMX_SetParameter( ctx->encode, OMX_IndexParamAudioPortFormat, &format ) );

	OMX_INIT_STRUCTURE( pcm );
	pcm.nPortIndex = 160;
	pcm.eNumData = OMX_NumericalDataSigned;
	pcm.ePCMMode = OMX_AUDIO_PCMModeLinear;
	pcm.bInterleaved = OMX_TRUE;
	pcm.eEndian = OMX_EndianLittle;
	pcm.nSamplingRate = 44100;
	pcm.nBitPerSample = 16;
	pcm.nChannels = 2;
	OERR( OMX_SetParameter( ctx->encode, OMX_IndexParamAudioPcm, &pcm ) );


	OMX_INIT_STRUCTURE( portdef );
	portdef.nPortIndex = 161;
	OERR( OMX_GetParameter( ctx->encode, OMX_IndexParamPortDefinition, &portdef ) );
	portdef.nBufferAlignment = 32;
	portdef.nBufferSize = 16384;
	portdef.nBufferCountMin = 4;
	portdef.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
	OERR( OMX_SetParameter( ctx->encode, OMX_IndexParamPortDefinition, &portdef ) );

	OMX_INIT_STRUCTURE( mp3 );
	mp3.nPortIndex = 161;
	mp3.nChannels = 1;
	mp3.nBitRate = 128000;
	mp3.eChannelMode = OMX_AUDIO_ChannelModeMono;
	mp3.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;
	OERR( OMX_SetParameter( ctx->encode, OMX_IndexParamAudioMp3, &mp3 ) );

	// Setup
	OERR( OMX_SetupTunnel( ctx->capture, 180, ctx->encode, 160 ) );
	omx_print_port( ctx->capture, 180 );
	omx_print_port( ctx->capture, 181 );
	omx_print_port( ctx->encode, 160 );
	omx_print_port( ctx->encode, 161 );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandStateSet, OMX_StateIdle, NULL ) );

	omx_block_until_state_changed( ctx->capture, OMX_StateIdle );
	omx_block_until_state_changed( ctx->encode, OMX_StateIdle );

	return ctx;
}


void audio_start( audio_context* ctx )
{
	omx_print_state( "audio_capture", ctx->capture );
	omx_print_state( "audio_encode", ctx->encode );

	omx_print_port( ctx->capture, 180 );
	omx_print_port( ctx->capture, 181 );
	omx_print_port( ctx->encode, 160 );
	omx_print_port( ctx->encode, 161 );

	ctx->capture_buffer = omx_allocbufs( ctx->encode, 160, 1 );
	ctx->encode_buffer = omx_allocbufs( ctx->encode, 161, 1 );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandPortEnable, 180, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortEnable, 160, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortEnable, 161, NULL ) );

	omx_block_until_port_changed( ctx->capture, 180, OMX_TRUE );
	omx_block_until_port_changed( ctx->encode, 160, OMX_TRUE );
	omx_block_until_port_changed( ctx->encode, 161, OMX_TRUE );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "audio_capture", ctx->capture );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandStateSet, OMX_StateExecuting, NULL ) );
	omx_print_state( "audio_encode", ctx->encode );

	OMX_CONFIG_BOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.bEnabled = OMX_TRUE;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexConfigCapturing, &capture ) );
}


void audio_stop( audio_context* ctx )
{
	OMX_CONFIG_BOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.bEnabled = OMX_FALSE;
	OERR( OMX_SetParameter( ctx->capture, OMX_IndexConfigCapturing, &capture ) );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	omx_print_state( "audio_capture", ctx->capture );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandStateSet, OMX_StateIdle, NULL ) );
	omx_print_state( "audio_encode", ctx->encode );

	OERR( OMX_SendCommand( ctx->capture, OMX_CommandPortDisable, 180, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortDisable, 160, NULL ) );
	OERR( OMX_SendCommand( ctx->encode, OMX_CommandPortDisable, 161, NULL ) );

	omx_block_until_port_changed( ctx->capture, 180, OMX_FALSE );
	omx_block_until_port_changed( ctx->encode, 160, OMX_FALSE );
	omx_block_until_port_changed( ctx->encode, 161, OMX_FALSE );

	OERR( OMX_FreeBuffer( ctx->encode, 160, ctx->capture_buffer ) );
	OERR( OMX_FreeBuffer( ctx->encode, 161, ctx->encode_buffer ) );
	ctx->encode_buffer = NULL;
}


static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE component, audio_context* ctx, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	printf( "event on 0x%08X type %d (%p, %p, %p)\n", (uint32_t)component, event, data1, data2, eventdata );
	return OMX_ErrorNone;
}


static OMX_ERRORTYPE emptied( OMX_HANDLETYPE component, audio_context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
	return OMX_ErrorNone;
}


static OMX_ERRORTYPE filled( OMX_HANDLETYPE component, audio_context* ctx, OMX_BUFFERHEADERTYPE* buf )
{
	pthread_mutex_lock( &ctx->lock );
	if ( component == ctx->encode ) {
		ctx->encode_data_avail = 1;
	}
	pthread_mutex_unlock( &ctx->lock );
	return OMX_ErrorNone;
}


int audio_fill_buffer( OMX_HANDLETYPE enc, OMX_BUFFERHEADERTYPE* bufs )
{
	return OMX_FillThisBuffer( enc, bufs );
}


char* audio_buffer_ptr( OMX_BUFFERHEADERTYPE* bufs )
{
	return (char*)bufs->pBuffer;
}


int audio_buffer_len( OMX_BUFFERHEADERTYPE* bufs )
{
	return bufs->nFilledLen;
}
*/
