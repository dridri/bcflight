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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bcm_host.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Core.h>
#include <IL/OMX_Broadcom.h>

#define OMX_INIT_STRUCTURE(a) \
    memset(&(a), 0, sizeof(a)); \
    (a).nSize = sizeof(a); \
    (a).nVersion.nVersion = OMX_VERSION; \
    (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
    (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
    (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
    (a).nVersion.s.nStep = OMX_VERSION_STEP

typedef struct video_context {
	int running;
	int video_running;
	int decoder_valid;
	int first_frame;
	pthread_mutex_t lock;
	// Handles
	OMX_HANDLETYPE dec;
	OMX_HANDLETYPE fx;
	OMX_HANDLETYPE spl;
	OMX_HANDLETYPE rdr1;
	OMX_HANDLETYPE rdr2;
	// Buffers
	OMX_BUFFERHEADERTYPE* decbufs;
	OMX_BUFFERHEADERTYPE* decinput;

	int width;
	int height;
	int stereo;
} video_context;

video_context* video_configure( int width, int height, int stereo );
void video_start( video_context* ctx );
void video_decode_frame( video_context* ctx, int corrupt );
void video_stop( video_context* ctx );
