#include <unistd.h>
#include <dirent.h>
#include <Board.h>
#include <Debug.h>
#include "BlackBox.h"

BlackBox::BlackBox()
	: Thread( "BlackBox" )
	, mID( 0 )
	, mFile( nullptr )
{
	char filename[256];
	DIR* dir;
	struct dirent* ent;
	if ( ( dir = opendir( "/var/BLACKBOX" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			std::string file = std::string( ent->d_name );
			uint32_t id = std::atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
			if ( id >= mID ) {
				mID = id + 1;
			}
		}
		closedir( dir );

		sprintf( filename, "/var/BLACKBOX/blackbox_%06u.csv", mID );
		mFile = fopen( filename, "wb" );
		gDebug() << "mFile : " << mFile << ", filename : " << filename << "\n";
	}

	if ( mFile == nullptr ) {
		Board::defectivePeripherals()["BlackBox"] = true;
		return;
	}

	Start();
}


BlackBox::~BlackBox()
{
}


const uint32_t BlackBox::id() const
{
	if ( this == nullptr ) {
		return 0;
	}

	return mID;
}


void BlackBox::Enqueue( const std::string& data, const std::string& value )
{
	if ( this == nullptr ) {
		return;
	}

	uint64_t time = Board::GetTicks();
	std::string str = std::to_string(time) + "," + data + "," + value;

	mQueueMutex.lock();
	mQueue.emplace_back( str );
	mQueueMutex.unlock();
}


bool BlackBox::run()
{
	std::string data;

	mQueueMutex.lock();
	while ( mQueue.size() > 0 ) {
		std::string str = mQueue.front();
		mQueue.pop_front();
		mQueueMutex.unlock();
		data += str + "\n";
		mQueueMutex.lock();
	}
	mQueueMutex.unlock();

	if ( data.length() > 0 ) {
		fwrite( data.c_str(), data.length(), 1, mFile );
		fflush( mFile );
		fsync( fileno( mFile ) );
	}

	usleep( 1000 * 1000 / 50 ); // 50Hz update
	return true;
}
