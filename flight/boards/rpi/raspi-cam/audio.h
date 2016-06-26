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

#ifndef RPI_AUDIO_H
#define RPI_AUDIO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

/*
typedef struct audio_context {
	int running;
	pthread_mutex_t lock;
	// Handles
// 	OMX_HANDLETYPE capture;
	OMX_HANDLETYPE encode;
	// Encoder
	OMX_BUFFERHEADERTYPE* capture_buffer;
	OMX_BUFFERHEADERTYPE* encode_buffer;
	int encode_data_avail;

	// ALSA
	snd_pcm_t* capture_handle;
	char* audio_buffer;
} audio_context;

audio_context* audio_configure();
void audio_start( audio_context* ctx );
void audio_stop( audio_context* ctx );
int audio_fill_buffer( OMX_HANDLETYPE enc, OMX_BUFFERHEADERTYPE* bufs );
char* audio_buffer_ptr( OMX_BUFFERHEADERTYPE* bufs );
int audio_buffer_len( OMX_BUFFERHEADERTYPE* bufs );
*/

// Generic audio buffer
typedef struct {
	// samples count, per channel
	int samples;
	// buffer pointer
	char* ptr;
	// buffer size, in bytes
	int size;
	int64_t pts;
} audio_buffer;

typedef struct audio_context {
	// State
	int running;
	pthread_mutex_t lock;
	pthread_mutex_t capture_lock;

	// Handles
	snd_pcm_t* capture;
	AVCodecContext* av_context;
	AVCodec* encode;
	AVFrame* frame;
	pthread_t capture_thread;

	// Encoder
	int encode_data_avail;

	// Data
	audio_buffer pcm;
	audio_buffer mp3;
} audio_context;

audio_context* audio_configure();
void audio_start( audio_context* ctx );
void audio_stop( audio_context* ctx );
int audio_capture_raw( audio_context* ctx );
int audio_fill_buffer( audio_context* ctx , AVCodec* enc );

#ifdef __cplusplus
};
#endif

#endif // RPI_AUDIO_H
