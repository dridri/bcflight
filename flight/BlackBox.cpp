#include <unistd.h>
#ifdef SYSTEM_NAME_Linux
#include <dirent.h>
#endif
#include <Board.h>
#include <Debug.h>
#include "BlackBox.h"

BlackBox::BlackBox()
	: Thread( "BlackBox" )
	, mID( 0 )
	, mFile( nullptr )
{
#ifdef SYSTEM_NAME_Linux
	char filename[256];
	DIR* dir;
	struct dirent* ent;
	if ( ( dir = opendir( "/var/BLACKBOX" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			string file = string( ent->d_name );
			uint32_t id = atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
			if ( id >= mID ) {
				mID = id + 1;
			}
		}
		closedir( dir );

		sprintf( filename, "/var/BLACKBOX/blackbox_%06u.csv", mID );
		mFile = fopen( filename, "wb" );
		gDebug() << "mFile : " << mFile << ", filename : " << filename << "\n";
	}
#elif defined( SYSTEM_NAME_Generic )
	// TODO
#endif

	if ( mFile == nullptr ) {
		Board::defectivePeripherals()["BlackBox"] = true;
		Stop();
		return;
	}

	setFrequency( 100 );
// 	Start();
	Stop();
}


BlackBox::~BlackBox()
{
}


const uint32_t BlackBox::id() const
{
// 	if ( this == nullptr ) {
		return 0;
// 	}

	return mID;
}


void BlackBox::Enqueue( const string& data, const string& value )
{
// 	if ( this == nullptr ) {
		return;
// 	}

#ifdef SYSTEM_NAME_Linux
	uint64_t time = Board::GetTicks();
	string str = to_string(time) + "," + data + "," + value;

	mQueueMutex.lock();
	mQueue.emplace_back( str );
	mQueueMutex.unlock();
#endif
}


bool BlackBox::run()
{
	exit(0);
	return false;
#ifdef SYSTEM_NAME_Linux
	string data;

	mQueueMutex.lock();
	while ( mQueue.size() > 0 ) {
		string str = mQueue.front();
		mQueue.pop_front();
		mQueueMutex.unlock();
		data += str + "\n";
		mQueueMutex.lock();
	}
	mQueueMutex.unlock();

	if ( data.length() > 0 ) {
		if ( fwrite( data.c_str(), data.length(), 1, mFile ) != data.length() ) {
			if ( errno == ENOSPC ) {
				Board::setDiskFull();
			}
		}
		if ( fflush( mFile ) < 0 or fsync( fileno( mFile ) ) < 0 ) {
			if ( errno == ENOSPC ) {
				Board::setDiskFull();
			}
		}
	}
#endif

	return true;
}
