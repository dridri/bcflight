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

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>

#include <sstream>
#include <string>
#include <typeinfo>
#include <mutex>
#include <pthread.h>

#include <cxxabi.h>
#include <malloc.h>
#include "Thread.h"

using namespace STD;


extern "C" void board_print( int level, const char* s );

class Debug
{
public:
	Debug() {
	}
	~Debug() {
		const string& s = mSS.str();
#ifdef SYSTEM_NAME_Linux
		printf( "%s", s.c_str() );
		fflush( stdout );
#else
		board_print( 0, s.c_str() );
#endif
		SendControllerOutput( s );
	}
	template<typename T> Debug& operator<<( const T& t ) {
		mSS << t;
		return *this;
	}
	template<typename... Args> void fDebug_top( const Args&... args );

private:
	stringstream mSS;
	static void SendControllerOutput( const string& s );
};

#ifndef __DBG_CLASS
static inline string className(const string& prettyFunction)
{
	size_t colons = prettyFunction.find("::");
// 	if ( prettyFunction.find( "GE::" ) ) {
// 		colons += 2 + prettyFunction.substr(colons + 2).find( "::" );
// 	}
	if (colons == string::npos)
		return "<none>";
	size_t begin = prettyFunction.substr(0,colons).rfind(" ");
	size_t end = colons - ( begin + 1 );
	return prettyFunction.substr(begin+1,end);
}
// using namespace GE;

#pragma GCC system_header // HACK Disable unused-function warnings
static string self_thread() {
	stringstream ret;
#ifdef SYSTEM_NAME_Linux
	static pthread_t main_thid = 0;
	if ( main_thid == 0 ) {
		main_thid = pthread_self();
	}
	if ( pthread_self() == main_thid ) {
		ret << "[main] ";
	} else if ( Thread::currentThread() != nullptr ) {
		ret << "[" << Thread::currentThread()->name() << "] ";
	} else if ( pthread_self() != 0 ) {
		ret << "[0x" << hex << ( pthread_self() & 0xFFFFFFFF ) << dec << "] ";
	}
#else
	if ( Thread::currentThread() != nullptr ) {
		ret << "[" << Thread::currentThread()->name() << "] ";
	} else {
		ret << "[main] ";
	}
#endif
	return ret.str();
}

#define __CLASS_NAME__ className(__PRETTY_FUNCTION__)

#define gDebug() Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() "
// #define fDebug( x ) Debug() << __CLASS_NAME__ << "::" << __FUNCTION__ << "( " << x << " )\n"

#pragma GCC system_header // HACK Disable unused-function warnings
static void fDebug_base( Debug& dbg, const char* end, bool f ) {
	dbg << " " << end;
}

template<typename Arg1, typename... Args> static void fDebug_base( Debug& dbg, const char* end, bool first, const Arg1& arg1, const Args&... args ) {
	char cap = 0;
#ifndef _NO_RTTI
	char* type = abi::__cxa_demangle(typeid(arg1).name(), nullptr, nullptr, nullptr);
	if ( strstr( type, "char" ) ) {
		if ( strstr( type, "*" ) || ( strstr( type, "[" ) && strstr( type, "]" ) ) ) {
			cap = '\"';
		} else {
			cap = '\'';
		}
	}
	free(type);
#endif

	if (!first ) {
		dbg << ", ";
	}
// 	dbg << "[" << type << "]";
	if ( cap ) dbg << cap;
	dbg << arg1;
	if ( cap ) dbg << cap;
	fDebug_base( dbg, end, false, args... );
}

#pragma GCC system_header // HACK Disable unused-function warnings
template<typename... Args> void Debug::fDebug_top( const Args&... args ) {
	if ( sizeof...(args) == 0 ) {
		operator<<( ")\n" );
	} else {
		operator<<( " " );
		fDebug_base( *this, ")\n", true, args... );
	}
}

#define fDebug( ... ) { Debug dbg; dbg << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "("; dbg.fDebug_top( __VA_ARGS__ ); }
#define fDebug0() Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "()\n"

#define aDebug( name, args... ) { Debug dbg; dbg << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() " << name << " = { "; fDebug_base( dbg, "}\n", true, args ); }

#define vDebug( name, args... ) { Debug dbg; dbg << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() " << name << "{ "; fDebug_base( dbg, "} ", true, args ); dbg << ""; }

#ifndef fDebug // To avoid syntax analyzer error using KDevelop
extern void fDebug( auto a, ... );
#endif

#endif // __DBG_CLASS


#endif // DEBUG_H
