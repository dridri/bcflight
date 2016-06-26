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

#ifdef BOARD_rpi

#include <unistd.h>
#include <netinet/in.h>
#include <Debug.h>
#include <Link.h>
#include <GPIO.h>
#include "Raspicam.h"

typedef void* OMX_HANDLETYPE;
typedef struct OMX_BUFFERHEADERTYPE OMX_BUFFERHEADERTYPE;
#include "boards/rpi/raspi-cam/video.h"
#include "boards/rpi/raspi-cam/audio.h"
#include <Board.h>
extern "C" void OMX_Init();

Raspicam::Raspicam( Link* link )
	: Camera()
	, mLink( link )
	, mNeedNextEnc1ToBeFilled( false )
	, mNeedNextEnc2ToBeFilled( false )
	, mNeedNextAudioToBeFilled( false )
	, mLiveSkipNextFrame( false )
	, mLiveFrameCounter( 0 )
	, mLedTick( 0 )
	, mLedState( true )
	, mRecording( false )
	, mRecordContext( nullptr )
	, mRecordStream( nullptr )
{
	mLiveTicks = 0;
	mRecordTicks = 0;
	mRecordFrameData = nullptr;
// 	mRecordFrameData = (char*)malloc(1024 * 1024);
	mRecordFrameDataSize = 0;
	mRecordFrameSize = 0;

	OMX_Init();
// 	mAudioContext = audio_configure();
	mVideoContext = video_configure();
// 	SetupRecord();

// 	mLiveThread = new HookThread<Raspicam>( this, &Raspicam::MixedThreadRun );
	mLiveThread = new HookThread<Raspicam>( "cam_live", this, &Raspicam::LiveThreadRun );
	mLiveThread->Start();
	mLiveThread->setPriority( 99 );

// 	mRecordThread = new HookThread<Raspicam>( "cam_record", this, &Raspicam::RecordThreadRun );
// 	mRecordThread->Start();
}


Raspicam::~Raspicam()
{
}


const uint32_t Raspicam::brightness() const
{
	return mVideoContext->brightness;
}


void Raspicam::setBrightness( uint32_t value )
{
	video_set_brightness( mVideoContext, value );
}


void Raspicam::StartRecording()
{
	if ( not mRecording ) {
		mRecording = true;
	}
}


void Raspicam::StopRecording()
{
	if ( mRecording ) {
		mRecording = false;
		if ( mRecordStream ) {
			delete mRecordStream;
			mRecordStream = nullptr;
		}
	}
}


bool Raspicam::LiveThreadRun()
{
	int ret = 0;

	if ( mVideoContext->exiting ) {
		video_stop( mVideoContext );
		return false;
	}

	if ( !mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Raspicam connected !\n";
			mLink->setBlocking( false );
// 			audio_start( mAudioContext );
			video_start( mVideoContext );
			mLiveFrameCounter = 0;
			mNeedNextEnc1ToBeFilled = true;
			mNeedNextEnc2ToBeFilled = true;
			mNeedNextAudioToBeFilled = true;
		}
		return true;
	} else if ( mRecording and Board::GetTicks() - mLedTick >= 1000 * 500 ) {
		GPIO::Write( 32, ( mLedState = !mLedState ) );
		mLedTick = Board::GetTicks();
	}

	Packet in;
// 	uint32_t uid = 0;
// 	if ( mLink->Read( &uid, sizeof(uid), 0 ) > 0 ) {
// 		gDebug() << "Received uid " << uid << "\n";
// 	}

	if ( not mNeedNextEnc2ToBeFilled and not mVideoContext->enc2_data_avail ) {
		pthread_mutex_lock( &mVideoContext->mutex_enc2 );
		pthread_cond_wait( &mVideoContext->cond_enc2, &mVideoContext->mutex_enc2 );
		pthread_mutex_unlock( &mVideoContext->mutex_enc2 );
	}

	if ( mVideoContext->enc2_data_avail ) {
// 		uint64_t timestamp = video_buffer_timestamp( mVideoContext->enc2bufs );
		int datalen = video_buffer_len( mVideoContext->enc2bufs );
		char* data = new char[ datalen ];
		memcpy( data, video_buffer_ptr( mVideoContext->enc2bufs ), datalen );

		{
			mNeedNextEnc2ToBeFilled = false;
			pthread_mutex_lock( &mVideoContext->lock );
			mVideoContext->enc2_data_avail = 0;
			pthread_mutex_unlock( &mVideoContext->lock );
			if ( ( ret = video_fill_buffer( mVideoContext->enc2, mVideoContext->enc2bufs ) ) != 0 ) {
				gDebug() << "Failed to request filling of the output buffer on encoder 2 output : " << ret << "\n";
			}
		}

		ret = LiveSend( data, datalen );
		if ( mRecording ) {
			RecordWrite( data, datalen );
// 			gDebug() << "filling record (" << datalen << ")\n";
// 			mRecordTimestamp = timestamp;
// 			memcpy( mRecordFrameData, data, datalen );
// 			mRecordFrameDataSize = datalen;
		}
		delete data;

		if ( ret == -42 ) {
			gDebug() << "video: Remote complaining about frame loss ! Restarting...\n";
			video_recover( mVideoContext );
		} else if ( ret < 0 ) {
			gDebug() << "video: Disconnected ?!\n";
			/*
			video_stop( mVideoContext );
// 			audio_stop( mAudioContext );
			mVideoContext->enc1_data_avail = 0;
			mVideoContext->enc2_data_avail = 0;
			*/
		}
	}

	if ( mNeedNextEnc2ToBeFilled && mLink->isConnected() ) {
		mNeedNextEnc2ToBeFilled = false;
		pthread_mutex_lock( &mVideoContext->lock );
		mVideoContext->enc2_data_avail = 0;
		pthread_mutex_unlock( &mVideoContext->lock );
		if ( ( ret = video_fill_buffer( mVideoContext->enc2, mVideoContext->enc2bufs ) ) != 0 ) {
			gDebug() << "Failed to request filling of the output buffer on encoder 2 output : " << ret << "\n";
		}
	}

// 	if ( not mVideoContext->enc2_data_avail ) {
// 		mLiveTicks = Board::WaitTick( 1000 * 1000 / 100, mLiveTicks, -500 );
// 	}
	return true;
}


bool Raspicam::RecordThreadRun()
{
	return false;
/*
	int ret = 0;
	double audio_pts, video_pts;

	if ( mVideoContext->exiting ) {
		return false;
	}

	if ( !mRecording ) {
		usleep( 1000 * 100 );
		return true;
	}

	audio_pts = (double)mAudioStream->pts.val * mAudioStream->time_base.num / mAudioStream->time_base.den;
	video_pts = (double)mVideoStream->pts.val * mVideoStream->time_base.num / mVideoStream->time_base.den;

// 	gDebug() << "pts : " << audio_pts << ", " << video_pts << "\n";

	if ( audio_pts < video_pts ) {
		gDebug() << "audio 1\n";
		AVPacket pkt;
		static uint8_t data[1024*1024];
		av_init_packet( &pkt );
		gDebug() << "audio 2\n";

		audio_capture_raw( mAudioContext );
		gDebug() << "audio 3\n";
		pkt.size = avcodec_encode_audio( mAudioEncoderContext, data, sizeof(data), (const short*)mAudioContext->pcm.ptr );
		gDebug() << "audio 4\n";
// 		pkt.pts = av_rescale_q( Board::GetTicks() - mRecordPTSBase, mAudioStream->codec->time_base, mAudioStream->time_base );
		gDebug() << "audio 5\n";
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = mAudioStream->index;
		gDebug() << "audio 6\n";

		ret = av_interleaved_write_frame( mRecordContext, &pkt );
		gDebug() << "audio 7\n";
		avio_flush( mRecordContext->pb );
		gDebug() << "av_interleaved_write_frame (audio) returned " << ret << "\n";
	} else if ( mRecordFrameData != nullptr and mRecordFrameDataSize > 0 ) {
		gDebug() << "video 1\n";
		AVPacket pkt;
		av_init_packet( &pkt );
		gDebug() << "video 1\n";

		pkt.data = (uint8_t*)mRecordFrameData;
		pkt.size = mRecordFrameDataSize;
		gDebug() << "video 2\n";
// 		pkt.pts = av_rescale_q( mRecordTimestamp, mVideoStream->codec->time_base, mVideoStream->time_base );
		gDebug() << "video 3\n";
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = mVideoStream->index;
		gDebug() << "video 4\n";

		ret = av_interleaved_write_frame( mRecordContext, &pkt );
		gDebug() << "video 5\n";
		avio_flush( mRecordContext->pb );
		gDebug() << "av_interleaved_write_frame (video) returned " << ret << "\n";

		ret = pkt.size;
	}

	usleep( 1000 );
	return true;
*/
}


int Raspicam::LiveSend( char* data, int datalen )
{
	int err = 0;

	if ( datalen <= 0 ) {
		return datalen;
	}

	err = mLink->Write( (uint8_t*)data, datalen, 0 );
	if ( err < 0 ) {
		gDebug() << "Link->Write() error : " << strerror(errno) << " (" << errno << ")\n";
		return -1;
	}

// 	gDebug() << "sent " << err << " bytes\n";
	return 0;
}


int Raspicam::RecordWrite( char* data, int datalen, int64_t pts, bool audio )
{
	int ret = 0;
/*
	if ( mRecordContext ) {
// 		AVRational omxtimebase = { 1, 1000000 };
		AVPacket pkt;
		av_init_packet( &pkt );

		pkt.data = (uint8_t*)data;
		pkt.size = datalen;
	// 	pkt.pts = AV_NOPTS_VALUE;
		pkt.flags |= AV_PKT_FLAG_KEY;

		if ( audio ) {
			pkt.stream_index = mAudioStream->index;
			pkt.pts = ( Board::GetTicks() - mRecordPTSBase ) * 44100 / 1000000;
		} else {
			pkt.stream_index = mVideoStream->index;
			pkt.pts = ( Board::GetTicks() - mRecordPTSBase ) * 45 / 1000000;
		}

		pkt.dts = pkt.pts;
		ret = av_interleaved_write_frame( mRecordContext, &pkt );
		avio_flush( mRecordContext->pb );
		gDebug() << "av_interleaved_write_frame returned " << ret << "\n";

		ret = pkt.size;
	} else {
*/
		if ( !mRecordStream ) {
			char filename[256];
			Board::Date date = Board::localDate();
			sprintf( filename, "/data/VIDEO/video_%04d_%02d_%02d_%02d_%02d_%02d.h264", date.year, date.month, date.day, date.hour, date.minute, date.second );
			mRecordStream = new std::ofstream( filename );
		}
		auto pos = mRecordStream->tellp();
		mRecordStream->write( data, datalen );
		ret = mRecordStream->tellp() - pos;
		mRecordStream->flush();
// 	}

	return ret;
}


void Raspicam::SetupRecord()
{
/*
	avcodec_register_all();

	gDebug() << "1\n";
	AVOutputFormat* fmt;
	char filename[256];
	Board::Date date = Board::localDate();
	gDebug() << "2\n";

	sprintf( filename, "/data/VIDEO/video_%04d_%02d_%02d_%02d_%02d_%02d.mp4", date.year, date.month, date.day, date.hour, date.minute, date.second );
	av_register_all();
	fmt = av_guess_format( "MP4", filename, NULL );
	gDebug() << "3\n";
	fmt->audio_codec = CODEC_ID_MP2;
	fmt->video_codec = CODEC_ID_H264;
	mRecordContext = avformat_alloc_context();
	mRecordContext->oformat = fmt;
	gDebug() << "4\n";
	gDebug() << "5\n";

	mVideoStream = avformat_new_stream( mRecordContext, 0 );
	gDebug() << "6\n";
	AVCodecContext* vc = mVideoStream->codec;
	gDebug() << "7\n";
	avcodec_get_context_defaults2( vc, AVMEDIA_TYPE_VIDEO );
	vc->codec_type = AVMEDIA_TYPE_VIDEO;
	vc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	vc->codec_id = fmt->video_codec;
	vc->bit_rate = 2 * 1000 * 1000;
	vc->width = 800;
	vc->height = 600;
	vc->time_base.num = 1;
	vc->time_base.den = 1000;
	vc->pix_fmt = PIX_FMT_YUV420P;
	gDebug() << "8\n";

	mAudioStream = avformat_new_stream( mRecordContext, 0 );
	AVCodecContext* ac = mAudioStream->codec;
	ac->codec_type = AVMEDIA_TYPE_AUDIO;
	ac->flags |= CODEC_FLAG_GLOBAL_HEADER;
	ac->codec_id = fmt->audio_codec;
	ac->sample_fmt = AV_SAMPLE_FMT_S16;
	ac->bit_rate = 128000;
	ac->sample_rate = 44100;
	ac->channels = 1;
	ac->channel_layout = AV_CH_LAYOUT_MONO;
	ac->time_base.num = 1;
	ac->time_base.den = 44100;


	printf( "1\n" );
	mAudioEncoder = avcodec_find_encoder( CODEC_ID_MP2 );
	printf( "2 %p\n", mAudioEncoder );
	mAudioEncoderContext = avcodec_alloc_context3( mAudioEncoder );
	printf( "3\n" );
	mAudioEncoderContext->bit_rate = 128000;
	mAudioEncoderContext->sample_fmt = AV_SAMPLE_FMT_S16;
	mAudioEncoderContext->sample_rate = 44100;
	mAudioEncoderContext->channel_layout = AV_CH_LAYOUT_MONO;
	mAudioEncoderContext->channels = 1;
	printf( "4\n" );

	if ( avcodec_open2( mAudioEncoderContext, mAudioEncoder, NULL ) < 0 ) {
		fprintf( stdout, "Could not open audio codec\n" );
		exit(1);
	}



	snprintf( mRecordContext->filename, sizeof( mRecordContext->filename ), "%s", filename );
	avio_open( &mRecordContext->pb, filename, URL_WRONLY );
	avformat_write_header( mRecordContext, nullptr );

	mRecordPTSBase = Board::GetTicks();
	mRecordFrameCounter = 0;
*/
}

#endif // BOARD_rpi
