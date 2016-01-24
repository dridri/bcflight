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
	, mRecordContext( nullptr )
	, mRecordStream( nullptr )
{
	mLiveTicks = 0;
	mRecordTicks = 0;
	mRecordFrameData = nullptr;
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
	} else if ( Board::GetTicks() - mLedTick >= 1000 * 500 ) {
		GPIO::Write( 32, ( mLedState = !mLedState ) );
		mLedTick = Board::GetTicks();
	}

	uint32_t uid = 0;
	if ( mLink->Read( &uid, sizeof(uid) ) > 0 ) {
// 		gDebug() << "Received 2 uid " << ntohl( uid ) << "\n";
	}

	if ( not mNeedNextEnc2ToBeFilled and not mVideoContext->enc2_data_avail ) {
		pthread_mutex_lock( &mVideoContext->mutex_enc2 );
		pthread_cond_wait( &mVideoContext->cond_enc2, &mVideoContext->mutex_enc2 );
		pthread_mutex_unlock( &mVideoContext->mutex_enc2 );
	}

	if ( mVideoContext->enc2_data_avail ) {
		int datalen = video_buffer_len( mVideoContext->enc2bufs );
		char* data = new char[ datalen ];
		memcpy( data, video_buffer_ptr( mVideoContext->enc2bufs ), datalen );
// 		char* data = video_buffer_ptr( mVideoContext->enc2bufs );

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
		delete data;

		if ( ret == -42 ) {
			gDebug() << "video: Remote complaining about frame loss ! Restarting...\n";
			video_recover( mVideoContext );
		} else if ( ret < 0 ) {
			gDebug() << "video: Disconnected ?!\n";
			video_stop( mVideoContext );
// 			audio_stop( mAudioContext );
			mVideoContext->enc1_data_avail = 0;
			mVideoContext->enc2_data_avail = 0;
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
// 		mLiveTicks = Board::WaitTick( 1000 * 1000 / 60, mLiveTicks, -500 );
// 	}
	return true;
}


bool Raspicam::RecordThreadRun()
{
	int ret = 0;

	if ( mVideoContext->exiting ) {
		return false;
	}

	if ( !mLink->isConnected() ) {
		if ( mRecordStream ) {
			mRecordStream->close();
			delete mRecordStream;
			mRecordStream = nullptr;
		}
		usleep( 1000 * 10 );
		return true;
	}

	if ( not mNeedNextEnc1ToBeFilled and not mVideoContext->enc1_data_avail ) {
		pthread_mutex_lock( &mVideoContext->mutex_enc1 );
		pthread_cond_wait( &mVideoContext->cond_enc1, &mVideoContext->mutex_enc1 );
		pthread_mutex_unlock( &mVideoContext->mutex_enc1 );
	}

	if ( mVideoContext->enc1_data_avail ) {
		ret = RecordWrite( video_buffer_ptr( mVideoContext->enc1bufs ), video_buffer_len( mVideoContext->enc1bufs ), video_buffer_timestamp( mVideoContext->enc1bufs ) );
		mNeedNextEnc1ToBeFilled = true;
	}

	if ( mNeedNextEnc1ToBeFilled ) {
		mNeedNextEnc1ToBeFilled = false;
		pthread_mutex_lock( &mVideoContext->lock );
		mVideoContext->enc1_data_avail = 0;
		pthread_mutex_unlock( &mVideoContext->lock );
		if ( ( ret = video_fill_buffer( mVideoContext->enc1, mVideoContext->enc1bufs ) ) != 0 ) {
			gDebug() << "Failed to request filling of the output buffer on encoder 1 output : " << ret << "\n";
		}
	}
/*
	if ( mAudioContext->encode_data_avail ) {
		gDebug() << "Audio data length : " << mAudioContext->mp3.size << "\n";
		ret = RecordWrite( mAudioContext->mp3.ptr, mAudioContext->mp3.size, mAudioContext->mp3.pts, true );
		mNeedNextAudioToBeFilled = true;
	}

	if ( mNeedNextAudioToBeFilled ) {
		mNeedNextAudioToBeFilled = false;
		pthread_mutex_lock( &mAudioContext->lock );
		mAudioContext->encode_data_avail = 0;
		pthread_mutex_unlock( &mAudioContext->lock );
		if ( ( ret = audio_fill_buffer( mAudioContext, mAudioContext->encode ) ) != 0 ) {
			gDebug() << "Failed to request filling of the output buffer on audio encoder output : " << ret << "\n";
		}
	}
*/

// 	if ( not mVideoContext->enc1_data_avail ) {
// 		mRecordTicks = Board::WaitTick( 1000 * 1000 / 30, mRecordTicks, -100 );
// 	}
	return true;
}


int Raspicam::LiveSend( char* data, int datalen )
{
	int err;
	union {
		int32_t s;
		uint32_t u;
	} ret;

	if ( datalen <= 0 ) {
		return datalen;
	}

	err = mLink->Write( data, datalen );
	if ( err <= 0 ) {
		gDebug() << "Link->Write() error : " << strerror(errno) << " (" << errno << ")\n";
		return -1;
	}

	return ret.s;
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
	gDebug() << "1\n";
	AVOutputFormat* fmt;
	char filename[256];
	Board::Date date = Board::localDate();
	gDebug() << "2\n";

	sprintf( filename, "/data/VIDEO/video_%04d_%02d_%02d_%02d_%02d_%02d.avi", date.year, date.month, date.day, date.hour, date.minute, date.second );
	av_register_all();
	fmt = av_guess_format( "AVI", filename, NULL );
	gDebug() << "3\n";
	fmt->audio_codec = CODEC_ID_MP2;
	fmt->video_codec = CODEC_ID_H264;
// 	fmt->video_codec = CODEC_ID_NONE;
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
	vc->bit_rate = 10 * 1000 * 1000;
// 	vc->width = 1280;
// 	vc->height = 720;
	vc->width = 480;
	vc->height = 360;
	vc->time_base.num = 1;
// 	vc->time_base.den = 1000;
	vc->time_base.den = 45;
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

	snprintf( mRecordContext->filename, sizeof( mRecordContext->filename ), "%s", filename );
	avio_open( &mRecordContext->pb, filename, URL_WRONLY );
	avformat_write_header( mRecordContext, nullptr );

	mRecordPTSBase = Board::GetTicks();
	mRecordFrameCounter = 0;
}
