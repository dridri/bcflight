#include <time.h>
#include <sys/time.h>
#include "encode.h"

/*
uint32_t fast_crc( char* data, int len )
{
	uint32_t ret = 0;
	int i = 0;
	uint32_t* d32 = (uint32_t*)data;
	uint8_t* d8 = (uint8_t*)data;

	for ( i = 0; i < len / 4; i++ ) {
		ret = ( ret << 1 ) ^ d32[i];
	}

	for ( i = len - ( len % 4 ); i < len; i++ ) {
		ret += d8[i];
	}

	return ret;
}

int WriteCallback( Context* ctx, Module* mod, int client, char* data, int datalen, uint32_t crc )
{
	if ( datalen <= 0 ) {
		return datalen;
	}

// 	unsigned int header[2] = { htonl( (unsigned int)datalen ), htonl( crc ) };
	unsigned int header[2] = { htonl( (unsigned int)datalen ), htonl( fast_crc( data, datalen ) ) };
	geSocketServerSend( mod->socket, client, &header, sizeof(header) );
//  	printf( "Frame size : %d\n", datalen );
	int ret = geSocketServerSend( mod->socket, client, data, datalen );
	int ret2 = geSocketServerReceive( mod->socket, client, &ret, sizeof(ret) );
	if ( ret2 <= 0 ) {
		printf( "video : geSocketServerReceive error %d\n", ret2 );
	}
	ret = ntohl( (unsigned int)ret );
// 	printf( "Remote received %d / %d bytes %s\n\n", ret, datalen, ( ret == datalen ) ? "" : "FAIL" );
	return ret;
}


void recorder( Context* ctx, Module* mod, void* argp )
{
	video_context* video = (video_context*)argp;
	int r;
	char filename[256] = "";
	time_t rawtime;
	struct tm* tm;
	time( &rawtime );
	tm = localtime( &rawtime );
	sprintf( filename, "/data/VIDEO/video_%04d_%02d_%02d_%02d_%02d_%02d.h264", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );

	printf( "recorder: Waiting for video module to run\n" );
	while ( !video->running ) {
		usleep( 1000 * 10 );
	}
	printf( "recorder: Go !\n" );

	FILE* save_fp = fopen( filename, "wb" );
	int need_next_enc1_to_be_filled = 1;

	while ( 1 ) {
		if ( !video->running ) {
			usleep( 1000 * 10 );
			need_next_enc1_to_be_filled = 1;
			continue;
		}

		if ( video->enc1_data_avail ) {
			if ( save_fp && video->enc1bufs && video->enc1bufs->pBuffer ) {
					fwrite( (char*)video->enc1bufs->pBuffer, 1, video->enc1bufs->nFilledLen, save_fp );
					fflush( save_fp );
			}
			need_next_enc1_to_be_filled = 1;
		}

		if ( need_next_enc1_to_be_filled ) {
			need_next_enc1_to_be_filled = 0;
			pthread_mutex_lock( &video->lock );
			video->enc1_data_avail = 0;
			pthread_mutex_unlock( &video->lock );
			if ( video->enc1bufs && ( r = OMX_FillThisBuffer( video->enc1, video->enc1bufs ) ) != OMX_ErrorNone ) {
				printf( "Failed to request filling of the output buffer on encoder 1 output port 201 : %d\n", r );
			}
		}

		usleep( 1000 * 1000 / 120 );
		pthread_yield();
	}
}


void video( Context* ctx, Module* mod, void* argp )
{
	printf( "video( %p, %p, %p )\n", ctx, mod, argp );

	mod->socket = geCreateSocket( GE_SOCKET_TYPE_SERVER, "", 41, GE_PORT_TYPE_TCP );
	int client = -1;
	int flag = 1;
	int r = 0;
	setsockopt( mod->socket->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int) );

	video_context* video = video_configure();
//	pthread_t recoder_thid = pthread_new( ctx, mod, recorder, video );

	int need_next_enc2_to_be_filled = 0;
	uint64_t ticks = GetTicks();

	while (1) {
		if ( client < 0 ) {
			mod->socket->nCsocks = 0;
			mod->socket->csock[0] = 0;
			client = geSocketWaitClient( mod->socket );
			if (client >= 0 ) {
				video_start( video );
				need_next_enc2_to_be_filled = 1;
			}
			pthread_yield();
			continue;
		}

		if ( video->enc2_data_avail ) {
			uint32_t crc = video_get_crc( video->enc2 );
			r = WriteCallback( ctx, mod, client, (char*)video->enc2bufs->pBuffer, video->enc2bufs->nFilledLen, crc );
			if ( r == -42 ) {
				printf( "video: Remote complaining about frame loss ! Restarting...\n" );
// 				pthread_mutex_lock( &video->lock );
//				video_stop( video );
				video_recover( video );
//				video_start( video );
// 				pthread_mutex_unlock( &video->lock );
			} else if ( r != video->enc2bufs->nFilledLen ) {
				printf( "video: Disconnected !\n" );
// 				pthread_mutex_lock( &video->lock );
				video_stop( video );
				video->enc1_data_avail = 0;
				video->enc2_data_avail = 0;
// 				pthread_mutex_unlock( &video->lock );
				client = -1;
			} else {
				need_next_enc2_to_be_filled = 1;
			}
		}

		if ( need_next_enc2_to_be_filled && client >= 0 ) {
			need_next_enc2_to_be_filled = 0;
			pthread_mutex_lock( &video->lock );
			video->enc2_data_avail = 0;
			pthread_mutex_unlock( &video->lock );
			if ( ( r = OMX_FillThisBuffer( video->enc2, video->enc2bufs ) ) != OMX_ErrorNone ) {
				printf( "Failed to request filling of the output buffer on encoder 2 output port 201 : %d\n", r );
			}
		}

		pthread_yield();
	}
}


void InitModule_video( Context* ctx )
{
	InsertModule( ctx, "video", video, 0, 0 );
}
*/
