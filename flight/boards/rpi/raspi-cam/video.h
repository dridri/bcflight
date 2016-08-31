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
// 	OMX_HANDLETYPE clk;
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
	uint32_t brightness;
	int32_t contrast;
	int32_t saturation;
	// Encoders
	OMX_BUFFERHEADERTYPE* enc1bufs;
	OMX_BUFFERHEADERTYPE* enc2bufs;
	OMX_BUFFERHEADERTYPE* enc2bufs2;
	int enc2_curr_buf;
	int enc1_data_avail;
	int enc2_data_avail;
} video_context;

video_context* video_configure( uint32_t fps, uint32_t live_width, uint32_t live_height, uint32_t live_kbps );
void video_start( video_context* ctx );
void video_stop( video_context* ctx );
void video_start_recording( video_context* ctx, const char* filename );
void video_set_brightness( video_context* ctx, uint32_t value );
void video_set_contrast( video_context* ctx, int32_t value );
void video_set_saturation( video_context* ctx, int32_t value );
void video_recover( video_context* ctx );
int video_fill_buffer( OMX_HANDLETYPE enc, OMX_BUFFERHEADERTYPE* bufs );
char* video_buffer_ptr( OMX_BUFFERHEADERTYPE* bufs );
int video_buffer_len( OMX_BUFFERHEADERTYPE* bufs );
int64_t video_buffer_timestamp( OMX_BUFFERHEADERTYPE* bufs );

#ifdef __cplusplus
};
#endif

#endif // RPI_VIDEO_H
