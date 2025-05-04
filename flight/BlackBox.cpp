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
		gDebug() << "mFile : " << mFile << ", filename : " << filename;
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
	Start();
}


BlackBox::~BlackBox()
{
}


void BlackBox::Start( bool enabled )
{
	if ( enabled and not Thread::running() ) {
		Thread::Start();
	} else {
		Thread::Stop();
	}
}


const uint32_t BlackBox::id() const
{
	if ( this == nullptr or not Thread::running() ) {
		return 0;
	}

	return mID;
}


void BlackBox::Enqueue( const string& data, const string& value )
{
	if ( this == nullptr or not Thread::running() ) {
		return;
	}

#ifdef SYSTEM_NAME_Linux
	uint64_t time = Board::GetTicks();
	string str = to_string(time) + "," + data + "," + value;

	mQueueMutex.lock();
	mQueue.emplace_back( str );
	mQueueMutex.unlock();
#endif
}


#define CHECK_ENABLED() \
	if ( this == nullptr or not Thread::running() ) { \
		return; \
	}


template<> void BlackBox::Enqueue( const string& data, const Vector<float, 1>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%.4f\"", v.x );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<float, 2>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%.4f,%.4f\"", v.x, v.y );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<float, 3>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%.4f,%.4f,%.4f\"", v.x, v.y, v.z );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<float, 4>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%.4f,%.4f,%.4f,%.4f\"", v.x, v.y, v.z, v.w );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<int, 1>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%d\"", v.x );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<int, 2>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%d,%d\"", v.x, v.y );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<int, 3>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%d,%d,%d\"", v.x, v.y, v.z );
}


template<> void BlackBox::Enqueue( const string& data, const Vector<int, 4>& v )
{
	CHECK_ENABLED();
	char stmp[64];
	sprintf( stmp, "\"%d,%d,%d,%d\"", v.x, v.y, v.z, v.w );
}


void BlackBox::Enqueue( const string* data, const string* values, int n )
{
	if ( this == nullptr or not Thread::running() ) {
		return;
	}

#ifdef SYSTEM_NAME_Linux
	uint64_t time = Board::GetTicks();

	mQueueMutex.lock();
	for ( int i = 0; i < n; i++ ) {
		string str = to_string(time) + "," + data[i] + "," + values[i];
		mQueue.emplace_back( str );
	}
	mQueueMutex.unlock();
#endif
}


void BlackBox::Enqueue( const char* data[], const char* values[], int n )
{
	if ( this == nullptr or not Thread::running() ) {
		return;
	}

#ifdef SYSTEM_NAME_Linux
	uint64_t time = Board::GetTicks();
	char str[2048] = "";

	mQueueMutex.lock();
	for ( int i = 0; i < n; i++ ) {
		sprintf( str, "%" PRIu64 ",%s,%s", time, data[i], values[i] );
		mQueue.emplace_back( string(str) );
	}
	mQueueMutex.unlock();
#endif
}


bool BlackBox::run()
{
	// exit(0);
	// return false;
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
