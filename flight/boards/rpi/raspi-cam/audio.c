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
// 	audio_encode_init( ctx );

// 	pthread_create( &ctx->capture_thread, NULL, (void*(*)(void*))&audio_input_thread, ctx );
// 	pthread_setname_np( ctx->capture_thread, "audio_encode" );

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


int audio_capture_raw( audio_context* ctx )
{
	return snd_pcm_readi( ctx->capture, ctx->pcm.ptr, ctx->pcm.samples );
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
// 	ctx->encode = avcodec_find_encoder( AV_CODEC_ID_MP2 );
	ctx->encode = avcodec_find_encoder( CODEC_ID_MP2 );
	printf( "2 %p\n", ctx->encode );
	ctx->av_context = avcodec_alloc_context3( ctx->encode );
	printf( "3\n" );
	ctx->av_context->bit_rate = 128000;
	ctx->av_context->sample_fmt = AV_SAMPLE_FMT_S16;
	ctx->av_context->sample_rate = 44100;
	ctx->av_context->channel_layout = 4;
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
