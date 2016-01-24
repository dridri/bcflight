#ifndef RPI_VIDEO_H
#define RPI_VIDEO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct video_context {
	int running;
	int exiting;
	pthread_mutex_t lock;
	pthread_mutex_t mutex_enc1;
	pthread_mutex_t mutex_enc2;
	pthread_cond_t cond_enc1;
	pthread_cond_t cond_enc2;
	// Handles
	OMX_HANDLETYPE cam;
	OMX_HANDLETYPE spl;
	OMX_HANDLETYPE rsz;
	OMX_HANDLETYPE enc1;
	OMX_HANDLETYPE enc2;
	OMX_HANDLETYPE wrm;
	OMX_HANDLETYPE nll;
	// Camera
	int camera_ready;
	OMX_BUFFERHEADERTYPE* cambufs;
	// Encoders
	OMX_BUFFERHEADERTYPE* enc1bufs;
	OMX_BUFFERHEADERTYPE* enc2bufs;
	int enc1_data_avail;
	int enc2_data_avail;
} video_context;

video_context* video_configure();
void video_start( video_context* ctx );
void video_stop( video_context* ctx );
void video_start_recording( video_context* ctx, const char* filename );
void video_recover( video_context* ctx );
int video_fill_buffer( OMX_HANDLETYPE enc, OMX_BUFFERHEADERTYPE* bufs );
char* video_buffer_ptr( OMX_BUFFERHEADERTYPE* bufs );
int video_buffer_len( OMX_BUFFERHEADERTYPE* bufs );
int64_t video_buffer_timestamp( OMX_BUFFERHEADERTYPE* bufs );

#ifdef __cplusplus
};
#endif

#endif // RPI_VIDEO_H
