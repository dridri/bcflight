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

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <mutex>
#include <pthread.h>

#include <cxxabi.h>
#include <malloc.h>
/*
class Debug
{
public:
	template<typename T> Debug& operator<<( T t )
	{
		std::stringstream ss;
		ss << t;
		std::string s = ss.str();
// 		std::cout << s << std::flush;
		printf( s.c_str() );
		fflush( stdout );
		SendControllerOutput( s );
		return *this;
	}

private:
	static std::string sBufferedData;
	static void SendControllerOutput( const std::string& s );
};
*/
class Debug
{
public:
	Debug() {
	}
	~Debug() {
		std::string s = mSS.str();// + "\n"; TODO : uncomment this, and remove <<"\n" everywhere..
		printf( "%s", s.c_str() );
		fflush( stdout );
		SendControllerOutput( s );
	}
	template<typename T> Debug& operator<<( const T& t ) {
		mSS << t;
		return *this;
	}

private:
	std::stringstream mSS;
	static std::mutex mMutex;
	static std::string sBufferedData;
	static void SendControllerOutput( const std::string& s );
};

#ifndef __DBG_CLASS
static inline std::string className(const std::string& prettyFunction)
{
	size_t colons = prettyFunction.find("::");
// 	if ( prettyFunction.find( "GE::" ) ) {
// 		colons += 2 + prettyFunction.substr(colons + 2).find( "::" );
// 	}
	if (colons == std::string::npos)
		return "<none>";
	size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
	size_t end = colons - begin;
	return prettyFunction.substr(begin,end);
}
// using namespace GE;

#pragma GCC system_header // HACK Disable unused-function warnings
static std::string self_thread() {
	if ( pthread_self() != 0 ) {
		std::stringstream ret;
		ret << "[0x" << std::hex << ( pthread_self() & 0xFFFFFFFF ) << std::dec << "] ";
		return ret.str();
	}
	return "";
}

#define __CLASS_NAME__ className(__PRETTY_FUNCTION__)

#define gDebug() Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() "
// #define fDebug( x ) Debug() << __CLASS_NAME__ << "::" << __FUNCTION__ << "( " << x << " )\n"

#pragma GCC system_header // HACK Disable unused-function warnings
static void fDebug_base( const char* end, bool f ) {
	Debug() << " " << end;
}

template<typename Arg1, typename... Args> static void fDebug_base( const char* end, bool first, const Arg1& arg1, const Args&... args ) {
	char* type = abi::__cxa_demangle(typeid(arg1).name(), nullptr, nullptr, nullptr);
	char cap = 0;
	if ( strstr( type, "char" ) ) {
		if ( strstr( type, "*" ) || ( strstr( type, "[" ) && strstr( type, "]" ) ) ) {
			cap = '\"';
		} else {
			cap = '\'';
		}
	}
	free(type);

	if (!first ) {
		Debug() << ", ";
	}
// 	Debug() << "[" << type << "]";
	if ( cap ) Debug() << cap;
	Debug() << arg1;
	if ( cap ) Debug() << cap;
	fDebug_base( end, false, args... );
}

#define fDebug( args... ) Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "( "; fDebug_base( ")\n", true, args )
#define fDebug0() Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "()\n"

#define aDebug( name, args... ) Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() " << name << " = { "; fDebug_base( "}\n", true, args )

#define vDebug( name, args... ) Debug() << self_thread() << __CLASS_NAME__ << "::" << __FUNCTION__ << "() " << name << "{ "; fDebug_base( "} ", true, args ); Debug() << ""

#ifndef fDebug // To avoid syntax analyzer error using KDevelop
extern void fDebug( auto a, ... );
#endif

#endif // __DBG_CLASS


#endif // DEBUG_H
