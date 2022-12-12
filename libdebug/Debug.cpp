#include <time.h>

#include <mutex>
#include <map>

#define __DBG_CLASS
#include "Debug.h"

#ifdef ANDROID
#include <android/log.h>
#endif

pthread_t Debug::mMainThread = 0;
bool Debug::mStoreLog = false;
std::string Debug::mLog = std::string();
std::mutex Debug::mLogMutex;
std::ofstream Debug::mLogFile;
Debug::Level Debug::mDebugLevel = Debug::Warning;

void Debug::log( int level, const std::string& s ) {
#ifdef ANDROID
	__android_log_print( ANDROID_LOG_DEBUG + ( Level::Verbose - level ), "luaserver", "%s", s.c_str() );
#else
	if ( level <= Debug::Warning ) {
		std::cerr << s << std::flush;
	} else {
		std::cout << s << std::flush;
	}
#endif
	if ( mStoreLog ) {
		mLogMutex.lock();
		mLog += s;
		mLogMutex.unlock();
	}
	if ( mLogFile.is_open() ) {
		mLogFile.write( s.c_str(), s.size() );
		mLogFile.flush();
	}
}
