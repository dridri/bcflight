#ifndef MINISTD_SSTREAM_H
#define MINISTD_SSTREAM_H

#include "string.h"

namespace ministd {

class stringstream
{
public:
	stringstream() : mString() {}
	stringstream( const stringstream& other ) : mString() {
		mString = other.mString;
	}
	~stringstream() {}

	const string& str( const string& s = string() ) {
		if ( s.length() > 0 ) {
			mString = s;
		}
		return mString;
	}

	stringstream& operator<<( const char* s ) {
		mString += s;
		return *this;
	}
	stringstream& operator<<( const string& s ) {
		mString += s;
		return *this;
	}
	stringstream& operator<<( int64_t x ) {
		mString += to_string(x);
		return *this;
	}
/*
	template<typename T> stringstream& operator<<( const T& t ) {
		board_printf( 0, "third with type '%s' (size: %d)\n", abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr), sizeof(T) );
		return *this;
	}
*/
protected:
	string mString;
};

}

#endif // MINISTD_SSTREAM_H
