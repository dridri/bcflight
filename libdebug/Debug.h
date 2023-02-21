#ifndef DEBUG_H
#define DEBUG_H

#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <mutex>
#include <tuple>
#include <pthread.h>
#include <cxxabi.h>
#include <chrono>

#ifdef TARGET_OS_IPHONE
	#define _Debug_Color( color ) ""
#else
	#define _Debug_Color( color ) color
#endif

class Debug
{
public:
	typedef enum {
		Error = 0,
		Warning,
		Info,
		Verbose
	} Level;
	Debug( Level level = Verbose, bool nobreak = false ) : mLevel( level ), mNoBreak( nobreak ) {
	}
	~Debug() {
		if ( mLevel <= mDebugLevel ) {
			if ( not mNoBreak ) {
				mStream << "\n";
			}
			log( mLevel, mStream.str() );
		}
	}

	template<typename T> Debug& operator+( T t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << t;
		}
		return *this;
	}

	template<typename T> Debug& operator<<( T t )
	{
		if ( mLevel <= mDebugLevel ) {
			char* type = abi::__cxa_demangle(typeid(t).name(), nullptr, nullptr, nullptr);
			char cap = 0;
			std::string color = _Debug_Color("\x1B[0m");
			if ( strstr( type, "basic_string" ) ) {
				cap = '\"';
				color = _Debug_Color("\x1B[31m");
			} else if ( !strcmp( type, "char" ) ) {
				cap = '\'';
				color = _Debug_Color("\x1B[31m");
			}

			std::stringstream arg_ss;
			arg_ss << t;
			if ( arg_ss.str()[0] >= '0' && arg_ss.str()[0] <= '9' ) {
				color = _Debug_Color("\x1B[36m");
			}

			std::stringstream ss;
			ss << color;
			if ( cap ) ss << cap;
			ss << arg_ss.str();
			if ( cap ) ss << cap;
			ss << _Debug_Color("\x1B[0m");
			mStream << ss.str();
			free(type);
		}
		return *this;
	}
/*
	template<typename T> Debug& operator<<( uint32_t t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << _Debug_Color("\x1B[36m") << t << _Debug_Color("\x1B[0m");
		}
		return *this;
	}

	template<typename T> Debug& operator<<( uint64_t t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << _Debug_Color("\x1B[36m") << t << _Debug_Color("\x1B[0m");
		}
		return *this;
	}

	template<typename T> Debug& operator<<( const char t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << "'" << _Debug_Color("\x1B[31m") << t << _Debug_Color("\x1B[0m") << "'";
		}
		return *this;
	}

	template<typename T> Debug& operator<<( const std::string& t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << "\"" << _Debug_Color("\x1B[31m") << t << _Debug_Color("\x1B[0m") << "\"";
		}
		return *this;
	}

	template<typename T> Debug& operator<<( const char* t )
	{
		if ( mLevel <= mDebugLevel ) {
			mStream << "\"" << _Debug_Color("\x1B[31m") << t << _Debug_Color("\x1B[0m") << "\"";
		}
		return *this;
	}
*/
	template<typename... Args> void fDebug_top( const Args&... args );
	template<typename... Args> void fDebug_top2( const char* end, const Args&... args );

	static void StoreLog( bool st = false ) { mStoreLog = st; }
	static const std::string& DumpLog() { return mLog; }
	static void setDebugLevel( Level lvl ) { mDebugLevel = lvl; }
	static void setVerbose( bool en ) { if ( en ) mDebugLevel = Verbose; }
	static void setLogFile( const std::string& filename ) { mLogFile = std::ofstream(filename); }
	static bool verbose() { return ( mDebugLevel == Verbose ); }
	static pthread_t mMainThread;


	static uint64_t GetTickMicros()
	{
	#ifdef WIN32
		return timeGetTime();
	#elif __APPLE__
		struct timeval cTime;
		gettimeofday( &cTime, 0 );
		return ( cTime.tv_sec * 1000000 ) + cTime.tv_usec;
	#else
		struct timespec now;
		clock_gettime( CLOCK_MONOTONIC, &now );
		return now.tv_sec * 1000000 + now.tv_nsec / 1000;
	#endif
	}

private:
	static void log( int level, const std::string& s );
	static bool mStoreLog;
	static std::string mLog;
	static std::mutex mLogMutex;
	static std::ofstream mLogFile;
	static Level mDebugLevel;
	std::stringstream mStream;
	Level mLevel;
	bool mNoBreak;
};

#ifndef __DBG_CLASS
static inline std::string _dbg_className(const std::string& prettyFunction)
{
	size_t parenthesis = prettyFunction.find("(");
	size_t colons = prettyFunction.substr( 0, parenthesis ).rfind("::");
// 	if ( prettyFunction.find( "GE::" ) ) {
// 		colons += 2 + prettyFunction.substr(colons + 2).find( "::" );
// 	}
	if (colons == std::string::npos)
		return "<none>";
	size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
	size_t end = colons - begin;
	return _Debug_Color("\x1B[32m") + prettyFunction.substr(begin,end) + _Debug_Color("\x1B[0m");
}
// using namespace GE;

#pragma GCC system_header // HACK Disable unused-function warnings
static std::string self_thread() {
	if ( Debug::mMainThread == 0 ) {
		Debug::mMainThread = pthread_self();
	}
	std::stringstream ret;
	pthread_t thid = pthread_self();
	char name[64] = "";
#ifdef HAVE_PTHREAD_GETNAME_NP
	pthread_getname_np( thid, name, sizeof(name) );
#endif
	ret << _Debug_Color("\x1B[33m");
	if ( Debug::mMainThread == thid ) {
		ret << "[MainThread] ";
	} else if ( name[0] != 0 ) {
		ret << "[" << name << "] ";
	} else {
		ret << "[0x" << std::hex << thid << std::dec << "] ";
	}
	ret << _Debug_Color("\x1B[0m");
	return ret.str();
}

#pragma GCC system_header // HACK Disable unused-function warnings
static std::string _debug_date() {
#ifdef ANDROID
	return std::string();
#endif
	typedef std::chrono::system_clock Clock;
	auto now = Clock::now();
	auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	auto fraction = now - seconds;
	time_t cnow = Clock::to_time_t(now);
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);

	time_t rawtime;
	struct tm* timeinfo;
	char buffer[64];

	time( &rawtime );
	timeinfo = localtime( &rawtime );
	strftime( buffer, 63, "[%Y-%m-%d %H:%M:%S", timeinfo );
	sprintf( buffer, "%s:%03d] ", buffer, milliseconds.count() );
	return buffer;
}

#define __CLASS_NAME__ _dbg_className(__PRETTY_FUNCTION__)
// #define __CLASS_NAME__ __PRETTY_FUNCTION__
#define __FUNCTION_NAME__ ( std::string(_Debug_Color("\x1B[94m")) + __FUNCTION__ + _Debug_Color("\x1B[0m") )

#pragma GCC system_header // HACK Disable unused-function warnings
static void fDebug_base( Debug& dbg, const char* end, bool f ) {
	dbg << " " << end;
}

template<typename Arg1, typename... Args> static void fDebug_base( Debug& dbg, const char* end, bool first, const Arg1& arg1, const Args&... args ) {
	char* type = abi::__cxa_demangle(typeid(arg1).name(), nullptr, nullptr, nullptr);
	char cap = 0;
	std::string color = _Debug_Color("\x1B[0m");
	if ( strstr( type, "char" ) ) {
		if ( strstr( type, "*" ) || ( strstr( type, "[" ) && strstr( type, "]" ) ) || strstr( type, "string" ) ) {
			cap = '\"';
			color = _Debug_Color("\x1B[31m");
		} else {
			cap = '\'';
			color = _Debug_Color("\x1B[31m");
		}
	}
	free(type);

	std::stringstream arg_ss;
	arg_ss << arg1;
	std::string arg_ss_str = arg_ss.str();
	if ( arg_ss_str.length() > 0 && arg_ss_str[0] >= '0' && arg_ss_str[0] <= '9' ) {
		color = _Debug_Color("\x1B[36m");
	}

	if (!first ) {
		dbg << ", ";
	}
	std::stringstream ss;
	ss << color;
	if ( cap ) ss << cap;
	ss << arg_ss_str;
	if ( cap ) ss << cap;
	ss << _Debug_Color("\x1B[0m");
	dbg + ss.str();
	fDebug_base( dbg, end, false, args... );
}

#define _NUMARGS(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#pragma GCC system_header // HACK Disable unused-function warnings
template<typename... Args> void Debug::fDebug_top( const Args&... args ) {
	if ( sizeof...(args) == 0 ) {
		operator+( ")" );
	} else {
		operator+( " " );
		fDebug_base( *this, ")", true, args... );
	}
}

#pragma GCC system_header // HACK Disable unused-function warnings
template<typename... Args> void Debug::fDebug_top2( const char* end, const Args&... args ) {
	if ( sizeof...(args) == 0 ) {
		operator+( end );
	} else {
		operator+( " " );
		fDebug_base( *this, end, true, args... );
	}
}

#define gDebug() Debug(Debug::Verbose) + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "() "
#define gInfo() Debug(Debug::Info) + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "() "
#define gWarning() Debug(Debug::Warning) + _debug_date() + self_thread() + _Debug_Color("\x1B[1;41;93m") + "WARNING" + _Debug_Color("\x1B[0m") + " " + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "() "
#define gError() Debug(Debug::Error) + _debug_date() + self_thread() + _Debug_Color("\x1B[1;41m") + "ERROR" + _Debug_Color("\x1B[0m") + " " + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "() "
#define fDebug( ... ) { Debug dbg;dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top( __VA_ARGS__ ); }
#define aDebug( name, ... ) { Debug dbg;dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + " " + name + " = { "; dbg.fDebug_top2( "}", __VA_ARGS__ ); }
#define vDebug( name, ... ) Debug dbg;dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + " " + name + "{ "; dbg.fDebug_top2( "} ", __VA_ARGS__ ); dbg + ""

#endif // __DBG_CLASS


#endif // DEBUG_H
