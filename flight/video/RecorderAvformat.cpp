#include <cstdio>
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
	, mActive( false )
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
	// fTrace( sample->track->type, sample->track->stream->id, sample->record_time_us, sample->buflen, sample->keyframe );
	AVPacket* pkt = av_packet_alloc();
	av_packet_from_data( pkt, sample->buf, sample->buflen );
	pkt->stream_index = sample->track->stream->id;
	pkt->dts = sample->record_time_us;
	pkt->pts = sample->record_time_us;
	pkt->pos = -1LL;
	pkt->flags = ( sample->keyframe ? AV_PKT_FLAG_KEY : 0 );
	if ( sample->track->type == TrackTypeVideo ) {
		pkt->flags = ( sample->keyframe ? AV_PKT_FLAG_KEY : 0 );
		pkt->duration = sample->track->stream->time_base.den / ( sample->track->average_fps * sample->track->stream->time_base.num );
		if ( not mMuxerReady and sample->track->stream->codecpar->extradata == nullptr ) {
			sample->track->stream->codecpar->extradata = (uint8_t*)malloc( 50 );
			sample->track->stream->codecpar->extradata_size = 50;
			// memcpy( sample->track->stream->codecpar->extradata, sample->buf, 50 );
			memcpy( sample->track->stream->codecpar->extradata, mVideoHeader.data(), 50 );
		}
	}
	if ( sample->track->type == TrackTypeAudio ) {
		pkt->flags = ( sample->keyframe ? AV_PKT_FLAG_KEY : 0 );
		if ( not mMuxerReady and sample->track->stream->codecpar->extradata == nullptr ) {
			sample->track->stream->codecpar->extradata = (uint8_t*)malloc( 0 );
			// sample->track->stream->codecpar->extradata_size = 15;
			// memcpy( sample->track->stream->codecpar->extradata, mAudioHeader.data(), 15 );
		}
	}
	if ( not mMuxerReady ) {
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
				Main::instance()->blackbox()->Enqueue( "Recorder:start", mRecordFilename );
			}
		} else {
			av_packet_free(&pkt);
			return;
		}
	}
	av_packet_rescale_ts( pkt, (AVRational){ 1, 1000000 }, sample->track->stream->time_base );
	gTrace() << "rescaled ts : " << pkt->pts << " " << pkt->dts << " " << pkt->duration << " " << sample->track->stream->time_base.num << "/" << sample->track->stream->time_base.den;
	int errwrite = av_interleaved_write_frame( mOutputContext, pkt );
	// avio_flush( mOutputContext->pb );
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
	mRecordFilename = std::string( filename );

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
			// track->stream->r_frame_rate = (AVRational){ 30, 1 };
			track->stream->start_time = 0;
			track->stream->codecpar->codec_id = std::string(track->format) == "mp3" ? AV_CODEC_ID_MP3 : AV_CODEC_ID_PCM_S16LE;
			track->stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
			track->stream->codecpar->channels = track->channels;
			track->stream->codecpar->sample_rate = track->sample_rate;
			if ( std::string(track->format) == "mp3" ) {
				track->stream->codecpar->bit_rate = 320 * 1024;
			}
			track->stream->codecpar->codec_tag = 0;
			track->stream->codecpar->format = AV_SAMPLE_FMT_S16;
			track->stream->codecpar->extradata = nullptr;
		} else if ( track->type == TrackTypeVideo ) {
			track->stream = avformat_new_stream( mOutputContext, nullptr );
			track->stream->id = mOutputContext->nb_streams - 1;
			track->stream->time_base = (AVRational){ 1, 1000000 };
			// track->stream->avg_frame_rate = (AVRational){ 30, 1 };
			track->stream->r_frame_rate = (AVRational){ 30, 1 };
			track->stream->start_time = 0;
			track->stream->codecpar->codec_id = std::string(track->format) == "h264" ? AV_CODEC_ID_H264 : AV_CODEC_ID_MJPEG;
			track->stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			track->stream->codecpar->width = track->width;
			track->stream->codecpar->height = track->height;
			track->stream->codecpar->bit_rate = 0;
			track->stream->codecpar->codec_tag = 0;
			track->stream->codecpar->format = std::string(track->format) == "h264" ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_RGB24;
			track->stream->codecpar->extradata = nullptr;
		}
	}

	av_dump_format( mOutputContext, 0, filename, 1 );

	for ( auto* track : mTracks ) {
		printf("track %d stream_index : %d\n", track->type, track->stream->id );
	}
/*
	int ret = 0;
	if ( (ret = avio_open( &mOutputContext->pb, filename, AVIO_FLAG_WRITE) ) ) {
		gError() << "Could not open " << filename << " : " << averr(ret).c_str();
		mActiveMutex.lock();
		mActive = false;
		mActiveMutex.unlock();
		return;
	}
*/
	gTrace() << "A";
	mOutputFile = fopen( filename, "wb+" );
	if ( !mOutputFile ) {
		gError() << "Could not open " << filename << " : " << strerror(errno);
		mActiveMutex.lock();
		mActive = false;
		mActiveMutex.unlock();
		return;
	}
	gTrace() << "B";

	mOutputContext->pb = avio_alloc_context(
		mOutputBuffer, sizeof(mOutputBuffer), 1, this,
		nullptr,
		[]( void* thiz, uint8_t* buf, int sz ) {
			FILE* file = static_cast<RecorderAvformat*>(thiz)->mOutputFile;
			return (int)fwrite( buf, 1, sz, file );
		},
		[]( void* thiz, int64_t offset, int whence ) {
			FILE* file = static_cast<RecorderAvformat*>(thiz)->mOutputFile;
			if ( whence & AVSEEK_SIZE ) {
				int64_t pos = ftello64( file );
				fseeko64( file, 0, SEEK_END );
				int64_t size = ftello64( file );
				fseeko64( file, pos, SEEK_SET );
				return size;
			}
			return (int64_t)fseeko64( file, offset, whence );
		}
	);
	gTrace() << "C";
	if ( !mOutputContext->pb ) {
		gError() << "Could not allocate AVFormat output context";
		mActiveMutex.lock();
		mActive = false;
		mActiveMutex.unlock();
		return;
	}

	gTrace() << "D";
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

	gTrace() << "E";
	if ( not mConsumerRegistered ) {
		mConsumerRegistered = true;
		Main::instance()->imu()->registerConsumer( [this](uint64_t t, const Vector3f& g, const Vector3f& a) {
			// fDebug( t, g.x, g.y, g.z );
			WriteGyro( t, g, a );
		});
	}
	gTrace() << "F";

	mMuxerReady = false;
	mRecordStartTick = 0;
	mRecordStartSystemTick = 0;
	Thread::Start();
}


void RecorderAvformat::Stop()
{
	if ( not recording() or not mActive ) {
		mWriteMutex.unlock();
		mGyroMutex.unlock();
		return;
	}

	mStopWrite = true;
	Thread::Stop();
	Thread::Join();

	gTrace() << "A";
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
	gTrace() << "B";
	Main::instance()->blackbox()->Enqueue( "Recorder:stop", mRecordFilename );

	for ( auto* track : mTracks ) {
		if ( track->stream && track->last_sample_time_us > 0 ) {
			track->stream->duration = av_rescale_q( track->last_sample_time_us - track->stream->start_time, (AVRational){ 1, 1000000 }, track->stream->time_base );
		}
	}
	av_write_trailer( mOutputContext );
	avio_flush( mOutputContext->pb );
	fflush( mOutputFile );
	fsync( fileno( mOutputFile ) );
	// avio_closep( &mOutputContext->pb );
	avio_context_free( &mOutputContext->pb );
	avformat_free_context( mOutputContext );
	mOutputContext = nullptr;
	for ( auto* track : mTracks ) {
		track->stream = nullptr;
	}
	fclose( mOutputFile );
	mOutputFile = nullptr;
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


uint32_t RecorderAvformat::AddVideoTrack( const std::string& format, uint32_t width, uint32_t height, uint32_t average_fps, const std::string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeVideo;
	strcpy( track->format, format.c_str() );
	track->width = width;
	track->height = height;
	track->average_fps = average_fps;
	track->last_sample_time_us = 0;

	track->id = mTracks.size();
	mTracks.emplace_back( track );

	return track->id;
}


uint32_t RecorderAvformat::AddAudioTrack( const std::string& format, uint32_t channels, uint32_t sample_rate, const std::string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeAudio;
	strcpy( track->format, format.c_str() );
	track->channels = channels;
	track->sample_rate = sample_rate;
	track->last_sample_time_us = 0;

	track->id = mTracks.size();
	mTracks.emplace_back( track );

	return track->id;
}


void RecorderAvformat::WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen, bool keyframe )
{
	// fTrace( track_id, record_time_us, buf, buflen, keyframe );

	Track* track = mTracks.at(track_id);
	if ( not track ) {
		return;
	}

	if ( mVideoHeader.size() == 0 and track->type == TrackTypeVideo ) {
		mVideoHeader.insert( mVideoHeader.end(), (uint8_t*)buf, (uint8_t*)buf + std::min( buflen, 50U ) );
	}
	if ( mAudioHeader.size() == 0 and track->type == TrackTypeAudio ) {
		mAudioHeader.insert( mAudioHeader.end(), (uint8_t*)buf, (uint8_t*)buf + std::min( buflen, 15U ) );
	}

	if ( not recording() or mStopWrite ) {
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

	if ( track->type == TrackTypeVideo ) {
		record_time_us -= mRecordStartTick;
	}
	if ( track->type == TrackTypeAudio ) {
		record_time_us -= mRecordStartSystemTick;
	}

	PendingSample* sample = new PendingSample;
	sample->track = track;
	sample->buf = new uint8_t[buflen];
	sample->buflen = buflen;
	sample->keyframe = keyframe;
	memcpy( sample->buf, buf, buflen );

	mWriteMutex.lock();
	sample->record_time_us = std::max( track->last_sample_time_us + 1, record_time_us );
	track->last_sample_time_us = sample->record_time_us;
	mPendingSamples.emplace_back( sample );
	mWriteMutex.unlock();
}


void RecorderAvformat::WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel )
{
	if ( not mGyroFile or record_time_us < mRecordStartSystemTick or mRecordStartSystemTick == 0 or not recording() or mStopWrite ) {
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

