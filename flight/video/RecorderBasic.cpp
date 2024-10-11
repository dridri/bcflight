#ifdef SYSTEM_NAME_Linux

#include <dirent.h>
#include <string.h>
#include "RecorderBasic.h"
#include <Board.h>
#include "Debug.h"


RecorderBasic::RecorderBasic()
	: Recorder()
	, Thread( "RecorderBasic" )
	, mBaseDirectory( "/var/VIDEO/" )
	, mRecordId( 0 )
	, mRecordSyncCounter( 0 )
{
}


RecorderBasic::~RecorderBasic()
{
}


bool RecorderBasic::recording()
{
	return Thread::running();
}


void RecorderBasic::Start()
{
	fDebug();
	if ( recording() ) {
		return;
	}

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
	sprintf( filename, (mBaseDirectory + "/record_%06u.csv").c_str(), mRecordId );
	mRecordFile = fopen( filename, "wb" );
	fprintf( mRecordFile, "# new_track,track_id,type(video/audio),filename\n" );
	fprintf( mRecordFile, "# track_id,record_time,pos_in_file,frame_size\n" );

	for ( auto& track : mTracks ) {
		track->recordStart = 0;
	}

	Thread::Start();
}


void RecorderBasic::Stop()
{
	fDebug();

	Thread::Stop();
	Thread::Join();

	for ( auto sample : mPendingSamples ) {
		delete sample->buf;
		delete sample;
	}
	mPendingSamples.clear();

	for ( auto track : mTracks ) {
		if ( track->file ) {
			fclose( track->file );
			track->file = nullptr;
		}
	}
}


uint32_t RecorderBasic::AddVideoTrack( const std::string& format, uint32_t width, uint32_t height, uint32_t average_fps, const string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeVideo;
	strcpy( track->format, format.c_str() );
	track->width = width;
	track->height = height;
	track->average_fps = average_fps;
	strcpy( track->extension, extension.c_str() );

	track->id = mTracks.size();
	mTracks.emplace_back( track );
/*
	Track* track = new Track;
	track->type = TrackTypeVideo;
	sprintf( track->filename, "video_%ux%u_%02ufps_%06u.%s", width, height, average_fps, mRecordId, extension.c_str() );
	track->file = fopen( ( string("/var/VIDEO/") + string(track->filename) ).c_str(), "wb" );

	mWriteMutex.lock();
	track->id = mTracks.size();
	fprintf( mRecordFile, "new_track,%u,video,%s\n", track->id, track->filename );
	mTracks.emplace_back( track );
	mWriteMutex.unlock();
*/
	return track->id;
}



uint32_t RecorderBasic::AddAudioTrack( const std::string& format, uint32_t channels, uint32_t sample_rate, const string& extension )
{
	Track* track = (Track*)malloc( sizeof(Track) );
	memset( track, 0, sizeof(Track) );

	track->type = TrackTypeAudio;
	strcpy( track->format, format.c_str() );
	track->channels = channels;
	track->sample_rate = sample_rate;
	strcpy( track->extension, extension.c_str() );

	track->id = mTracks.size();
	mTracks.emplace_back( track );
/*
	Track* track = new Track;
	track->type = TrackTypeAudio;
	sprintf( track->filename, "audio_%uhz_%uch_%06u.%s", sample_rate, channels, mRecordId, extension.c_str() );
	track->file = fopen( ( string("/var/VIDEO/") + string(track->filename) ).c_str(), "wb" );

	mWriteMutex.lock();
	track->id = mTracks.size();
	fprintf( mRecordFile, "new_track,%u,audio,%s\n", track->id, track->filename );
	mTracks.emplace_back( track );
	mWriteMutex.unlock();
*/
	return track->id;
}


void RecorderBasic::WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen, bool keyframe )
{
	if ( not recording() ) {
		return;
	}
	Track* track = mTracks.at(track_id);
	if ( not track ) {
		return;
	}
	if ( track->recordStart == 0 ) {
		track->recordStart = record_time_us;
	}
	record_time_us -= track->recordStart;

	// fDebug( track_id, record_time_us, buf, buflen );

	PendingSample* sample = new PendingSample;
	sample->track = track;
	sample->record_time_us = record_time_us;
	sample->buf = new uint8_t[buflen];
	sample->buflen = buflen;
	memcpy( sample->buf, buf, buflen );

	mWriteMutex.lock();
	mPendingSamples.emplace_back( sample );
	mWriteMutex.unlock();
}


void RecorderBasic::WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel )
{
	(void)record_time_us;
	(void)gyro;
	(void)accel;
}


bool RecorderBasic::run()
{
	// Wait until there is data to write (TBD : use pthread_cond for passive wait ?)
	mWriteMutex.lock();
	if ( mPendingSamples.size() == 0 ) {
		mWriteMutex.unlock();
		usleep( 1000 * 5 ); // wait 5 ms, allowing up to 200FPS video recording
		return true;
	}

	PendingSample* sample = mPendingSamples.front();
	mPendingSamples.pop_front();
	mWriteMutex.unlock();

	if ( sample->track->file == nullptr ) {
		Track* track = sample->track;
		if ( track->type == TrackTypeVideo ) {
			sprintf( track->filename, "video_%ux%u_%02ufps_%06u.%s", track->width, track->height, track->average_fps, mRecordId, track->extension );
			fprintf( mRecordFile, "new_track,%u,video,%s\n", track->id, track->filename );
		} else if ( track->type == TrackTypeAudio ) {
			sprintf( track->filename, "audio_%uhz_%uch_%06u.%s", track->sample_rate, track->channels, mRecordId, track->extension );
			fprintf( mRecordFile, "new_track,%u,audio,%s\n", track->id, track->filename );
		}
		track->file = fopen( ( mBaseDirectory + "/" + string(track->filename) ).c_str(), "wb" );
	}

	uint32_t pos = ftell( sample->track->file );
	if ( fwrite( sample->buf, 1, sample->buflen, sample->track->file ) != sample->buflen ) {
		goto err;
	}
	if ( fprintf( mRecordFile, "%u,%llu,%u,%u\n", sample->track->id, sample->record_time_us, pos, sample->buflen ) <= 0 ) {
		goto err;
	}

	if ( mRecordSyncCounter == 0 ) {
		if ( fflush( mRecordFile ) < 0 or fsync( fileno( mRecordFile ) ) < 0 ) {
			goto err;
		}
	}
	mRecordSyncCounter = ( mRecordSyncCounter + 1 ) % 10; // sync on disk every 10 samples (up to 10*1/FPS seconds)

	goto end;

err:
	if ( errno == ENOSPC ) {
		Board::setDiskFull();
	}

end:
	delete[] sample->buf;
	delete sample;
	return true;
}


#endif // SYSTEM_NAME_Linux
