#include <sys/statvfs.h>
#include <dirent.h>

#include "RecorderAvformat.h"
#include "Board.h"
#include "Debug.h"
#include "Main.h"
#include "IMU.h"


RecorderAvformat::RecorderAvformat()
	: Recorder()
	, Thread( "RecorderAvformat" )
	, mRecordStartTick( 0 )
	, mRecordStartSystemTick( 0 )
	, mOutputContext( nullptr )
	, mMuxerReady( false )
	, mStopWrite( false )
	, mWriteTicks( 0 )
	, mGyroFile( nullptr )
	, mConsumerRegistered( false )
{
}


RecorderAvformat::~RecorderAvformat()
{
}


static std::string averr( int err )
{
	char str[AV_ERROR_MAX_STRING_SIZE] = "";
    return std::string( av_make_error_string( str, AV_ERROR_MAX_STRING_SIZE, err ) );
}


void RecorderAvformat::WriteSample( PendingSample* sample )
{
	AVPacket* pkt = av_packet_alloc();
	av_packet_from_data( pkt, sample->buf, sample->buflen );
	pkt->stream_index = sample->track->stream->id;
	pkt->dts = sample->record_time_us;
	pkt->pts = sample->record_time_us;
	pkt->pos = -1LL;
	pkt->flags = ( sample->keyframe ? AV_PKT_FLAG_KEY : 0 );
	// fDebug( sample->track->type );
	av_packet_rescale_ts( pkt, (AVRational){ 1, 1000000 }, sample->track->stream->time_base );
	if ( sample->track->type == TrackTypeVideo ) {
		pkt->flags = ( sample->keyframe ? AV_PKT_FLAG_KEY : 0 );
		if ( not mMuxerReady and sample->track->stream->codecpar->extradata == nullptr ) {
			sample->track->stream->codecpar->extradata = (uint8_t*)malloc( 50 );
			sample->track->stream->codecpar->extradata_size = 50;
			// memcpy( sample->track->stream->codecpar->extradata, sample->buf, 50 );
			memcpy( sample->track->stream->codecpar->extradata, mVideoHeader.data(), 50 );
			bool all_ready = true;
			for ( auto track : mTracks ) {
				all_ready &= ( track->stream->codecpar->extradata != nullptr );
			}
			if ( all_ready ) {
				int ret = avformat_write_header( mOutputContext, nullptr );
				if (ret < 0) {
					fprintf(stderr, "Error occurred when opening output file: %s\n", averr(ret).c_str());
					return;
				} else {
					gDebug() << "Avformat muxer ready";
					mMuxerReady = true;
				}
			}
		}
	}
	int errwrite = av_interleaved_write_frame( mOutputContext, pkt );
	avio_flush( mOutputContext->pb );
	// sync();
	av_packet_free(&pkt);
	if ( errwrite < 0 ) {
		if ( errwrite == ENOSPC or errno == ENOSPC ) {
			Board::setDiskFull();
		}
	}
}


bool RecorderAvformat::run()
{
	mWriteMutex.lock();
	while ( mPendingSamples.size() > 0 ) {
		PendingSample* sample = mPendingSamples.front();
		mPendingSamples.pop_front();
		mWriteMutex.unlock();
		WriteSample( sample );
		delete sample;
		mWriteMutex.lock();
	}
	mWriteMutex.unlock();

	mGyroMutex.lock();
	while ( mPendingGyros.size() > 0 ) {
		PendingGyro* gyro = mPendingGyros.front();
		mPendingGyros.pop_front();
		mGyroMutex.unlock();
		// fprintf( mGyroFile, "%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n", gyro->t, gyro->gx, gyro->gy, gyro->gz, gyro->ax, gyro->ay, gyro->az );
		fprintf( mGyroFile, "%llu,%.6f,%.6f,%.6f\n", gyro->t, gyro->gx, gyro->gy, gyro->gz );
		mGyroMutex.lock();
	}
	mGyroMutex.unlock();

	mWriteTicks = Board::WaitTick( 1000 * 1000 / 300, mWriteTicks );

	return not mStopWrite;
}



void RecorderAvformat::Start()
{
	fDebug();
	while ( mStopWrite ) {
		usleep( 1000 * 200 );
	}
	mActiveMutex.lock();
	if ( mActive ) {
		mActiveMutex.unlock();
		return;
	}
	mActive = true;
	mActiveMutex.unlock();

	uint32_t fileid = 0;
	DIR* dir;
	struct dirent* ent;
	if ( ( dir = opendir( mBaseDirectory.c_str() ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			string file = string( ent->d_name );
			if ( file.find( "record_" ) != file.npos ) {
				uint32_t id = std::atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
				if ( id >= fileid ) {
					fileid = id + 1;
				}
			}
		}
		closedir( dir );
	}
	mRecordId = fileid;

	char filename[256];
	sprintf( filename, (mBaseDirectory + "/record_%010u.mkv").c_str(), mRecordId );

	avformat_alloc_output_context2( &mOutputContext, nullptr, nullptr, filename );
	if ( !mOutputContext ) {
		gError() << "Failed to allocate avformat output context";
		mActiveMutex.lock();
		mActive = false;
		mActiveMutex.unlock();
		return;
	}

	for ( auto* track : mTracks ) {
		if ( track->type == TrackTypeAudio ) {
			track->stream = avformat_new_stream( mOutputContext, nullptr );
			track->stream->id = mOutputContext->nb_streams - 1;
			track->stream->time_base = (AVRational){ 1, 1000000 };
			track->stream->start_time = 0;
			track->stream->codecpar->codec_id = AV_CODEC_ID_MP3;
			track->stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
			track->stream->codecpar->channels = track->channels;
			track->stream->codecpar->sample_rate = track->sample_rate;
			track->stream->codecpar->bit_rate = 128 * 1024;
			track->stream->codecpar->codec_tag = 0;
			track->stream->codecpar->format = AV_SAMPLE_FMT_S16;
			track->stream->codecpar->extradata = nullptr;
		} else if ( track->type == TrackTypeVideo ) {
			track->stream = avformat_new_stream( mOutputContext, nullptr );
			track->stream->id = mOutputContext->nb_streams - 1;
			track->stream->time_base = (AVRational){ 1, 1000000 };
			track->stream->avg_frame_rate = (AVRational){ 30, 1 };
			track->stream->r_frame_rate = (AVRational){ 30, 1 };
			track->stream->start_time = 0;
			track->stream->codecpar->codec_id = AV_CODEC_ID_H264;
			track->stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			track->stream->codecpar->width = track->width;
			track->stream->codecpar->height = track->height;
			track->stream->codecpar->bit_rate = 0;
			track->stream->codecpar->codec_tag = 0;
			track->stream->codecpar->format = AV_PIX_FMT_YUV420P;
			track->stream->codecpar->extradata = nullptr;
		}
	}

	av_dump_format( mOutputContext, 0, filename, 1 );

	int ret = 0;
	if ( (ret = avio_open( &mOutputContext->pb, filename, AVIO_FLAG_WRITE) ) ) {
		gError() << "Could not open " << filename << " : " << averr(ret).c_str();
		mActiveMutex.lock();
		mActive = false;
		mActiveMutex.unlock();
		return;
	}

	sprintf( filename, (mBaseDirectory + "/record_%010u.gcsv").c_str(), mRecordId );
	mGyroFile = fopen( filename, "wb+" );
	if ( mGyroFile ) {
		fprintf( mGyroFile, "GYROFLOW IMU LOG\n" );
		fprintf( mGyroFile, "version,1.3\n" );
		fprintf( mGyroFile, "id,bcflight\n" );
		fprintf( mGyroFile, "orientation,ZYx\n" ); // TODO : use IMU orientation
		fprintf( mGyroFile, "note,development_test\n" );
		fprintf( mGyroFile, "fwversion,FIRMWARE_0.1.0\n" );
		// fprintf( mGyroFile, "timestamp,1644159993\n" ); // current datetime
		fprintf( mGyroFile, "vendor,bcflight\n" );
		fprintf( mGyroFile, "videofilename,record_%010u.mkv\n", mRecordId );
		fprintf( mGyroFile, "lensprofile,Raspberry Pi Foundation/Raspberry Pi Foundation_Camera Module 3 (Sony IMX708)_Stock wide (120°)_libcamera native 1080p (2304x1296-pBAA)_1080p_16by9_1920x1080-30.00fps.json\n" );
		fprintf( mGyroFile, "lens_info,wide\n" );
		fprintf( mGyroFile, "frame_readout_time,%.6f\n", 1.0f / 30.0f );
		fprintf( mGyroFile, "frame_readout_direction,0\n" );
		fprintf( mGyroFile, "tscale,0.000001\n" );
		fprintf( mGyroFile, "gscale,%.6f\n", M_PI / 180.0f ); // BCFlight is in Deg, GCSV is in Rad
		fprintf( mGyroFile, "ascale,1.0\n" );
		// fprintf( mGyroFile, "t,gx,gy,gz,ax,ay,az\n" );
		fprintf( mGyroFile, "t,gx,gy,gz\n" );
		fprintf( mGyroFile, "0,0.000000,0.000000,0.000000\n" );
	}

	if ( not mConsumerRegistered ) {
		mConsumerRegistered = true;
		Main::instance()->imu()->registerConsumer( [this](uint64_t t, const Vector3f& g, const Vector3f& a) {
			// fDebug( t, g.x, g.y, g.z );
			WriteGyro( t, g, a );
		});
	}

	Thread::Start();
}


void RecorderAvformat::Stop()
{
	if ( not recording() or not mActive ) {
		return;
	}

	mStopWrite = true;
	Thread::Stop();
	Thread::Join();

	mWriteMutex.lock();
	while ( mPendingSamples.size() > 0 ) {
		PendingSample* sample = mPendingSamples.front();
		mPendingSamples.pop_front();
		mWriteMutex.unlock();
		WriteSample( sample );
		delete sample;
		mWriteMutex.lock();
	}
	mWriteMutex.unlock();

	av_write_trailer( mOutputContext );
	avio_flush( mOutputContext->pb );
	avio_closep( &mOutputContext->pb );
	avformat_free_context( mOutputContext );
	mOutputContext = nullptr;
	for ( auto* track : mTracks ) {
		track->stream = nullptr;
	}
	mMuxerReady = false;
	mRecordStartTick = 0;
	mRecordStartSystemTick = 0;

	mGyroMutex.lock();
	while ( mPendingGyros.size() > 0 ) {
		PendingGyro* gyro = mPendingGyros.front();
		mPendingGyros.pop_front();
		mGyroMutex.unlock();
		// fprintf( mGyroFile, "%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n", gyro->t, gyro->gx, gyro->gy, gyro->gz, gyro->ax, gyro->ay, gyro->az );
		fprintf( mGyroFile, "%llu,%.6f,%.6f,%.6f\n", gyro->t, gyro->gx, gyro->gy, gyro->gz );
		mGyroMutex.lock();
	}
	mGyroMutex.unlock();
	if ( mGyroFile ) {
		fclose( mGyroFile );
		mGyroFile = nullptr;
	}

	mActiveMutex.lock();
	mStopWrite = false;
	mActive = false;
	mActiveMutex.unlock();
}


bool RecorderAvformat::recording()
{
	return Thread::running();
}


uint32_t RecorderAvformat::AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const std::string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeVideo;
	track->width = width;
	track->height = height;
	track->average_fps = average_fps;

	track->id = mTracks.size();
	mTracks.emplace_back( track );

	return track->id;
}


uint32_t RecorderAvformat::AddAudioTrack( uint32_t channels, uint32_t sample_rate, const std::string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeAudio;
	track->channels = channels;
	track->sample_rate = sample_rate;

	track->id = mTracks.size();
	mTracks.emplace_back( track );

	return track->id;
}


void RecorderAvformat::WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen, bool keyframe )
{
	// fDebug( track_id, record_time_us, buf, buflen );

	if ( mVideoHeader.size() == 0 ) {
		mVideoHeader.insert( mVideoHeader.end(), (uint8_t*)buf, (uint8_t*)buf + std::min( buflen, 50U ) );
	}

	if ( not recording() or mStopWrite ) {
		return;
	}
	Track* track = mTracks.at(track_id);
	if ( not track ) {
		return;
	}
	if ( mRecordStartTick == 0 ) {
		if ( track->type == TrackTypeVideo ) {
			mRecordStartTick = record_time_us;
			mRecordStartSystemTick = Board::GetTicks();
		} else {
			// Wait for the video stream to start first
			return;
		}
	}
	record_time_us -= mRecordStartTick;

	PendingSample* sample = new PendingSample;
	sample->track = track;
	sample->record_time_us = record_time_us;
	sample->buf = new uint8_t[buflen];
	sample->buflen = buflen;
	sample->keyframe = keyframe;
	memcpy( sample->buf, buf, buflen );

	mWriteMutex.lock();
	mPendingSamples.emplace_back( sample );
	mWriteMutex.unlock();
}


void RecorderAvformat::WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel )
{
	if ( not mGyroFile or record_time_us < mRecordStartSystemTick or not Thread::running() or mStopWrite ) {
		return;
	}

	PendingGyro* g = new PendingGyro();
	g->t = record_time_us - mRecordStartSystemTick;
	g->gx = gyro.x;
	g->gy = gyro.y;
	g->gz = gyro.z;
	g->ax = accel.x;
	g->ay = accel.y;
	g->az = accel.z;

	mGyroMutex.lock();
	mPendingGyros.emplace_back( g );
	mGyroMutex.unlock();
}

