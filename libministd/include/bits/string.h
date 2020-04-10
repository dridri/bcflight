#ifndef MINISTD_STRING_H
#define MINISTD_STRING_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

namespace ministd {

extern size_t _string_mem;

class string
{
public:
	static const size_t npos = -1;

	string() : capacity(0), str(nullptr) {}
	string( const string& other ) : capacity(0), str(nullptr) {
		if ( other.str and other.str[0] ) {
			capacity = strlen( other.str );
			str = (char*)malloc( capacity + 1 );
			strcpy( str, other.str );
			_string_mem += capacity + 1;
		}
	}

	string( const char* s ) : capacity(0), str(nullptr) {
		if ( s and s[0] ) {
			capacity = strlen(s);
			str = (char*)malloc( capacity + 1 );
			strcpy( str, s );
			_string_mem += capacity + 1;
		}
	}

	void operator=( const string& other ) {
		if ( str ) {
			memset( str, 0, capacity );
			free( str );
			_string_mem -= capacity + 1;
			str = nullptr;
			capacity = 0;
		}
		if ( other.str and other.str[0] ) {
			capacity = strlen( other.str );
			str = (char*)malloc( capacity + 1 );
			strcpy( str, other.str );
			_string_mem += capacity + 1;
		}
	}

	void operator=( const char* s ) {
		if ( str ) {
			memset( str, 0, capacity );
			free( str );
			_string_mem -= capacity + 1;
			str = nullptr;
			capacity = 0;
		}
		if ( s and s[0] ) {
			capacity = strlen( s );
			str = (char*)malloc( capacity + 1 );
			strcpy( str, s );
			_string_mem += capacity + 1;
		}
	}

	~string() {
		if ( str ) {
			memset( str, 0, capacity );
			free( str );
			_string_mem -= capacity + 1;
		}
		str = nullptr;
	}

	void reserve( size_t n ) {
		if ( str and n < capacity ) {
			return;
		}
		char* olddata = str;
		uint32_t oldcap = capacity;
		capacity = n;
		str = (char*)malloc( capacity + 1 );
		memset( str, 0, capacity + 1 );
		if ( olddata ) {
			strcpy( str, olddata );
			memset( olddata, 0, oldcap );
			free( olddata );
			_string_mem -= oldcap + 1;
		}
		_string_mem += capacity + 1;
	}

	size_t length() const {
		if ( not str ) {
			return 0;
		}
		return strlen( str );
	}

	char* data() const {
		return str;
	}

	const char* c_str() const {
		return str;
	}

	size_t find( char c ) const {
		if ( str ) {
			for ( uint32_t i = 0; str[i]; i++ ) {
				if ( str[i] == c ) {
					return i;
				}
			}
		}
		return npos;
	}
	size_t find( const char* s ) const {
		if ( str ) {
			char* f = strstr( str, s );
			if ( f ) {
				return f - str;
			}
		}
		return npos;
	}
	size_t find( const string& s ) const {
		if ( s.str ) {
			return find( s.str );
		}
		return npos;
	}

	size_t find_first_not_of( const char* s ) const {
		if ( str and s ) {
			return strcspn( str, s );
		}
		return npos;
	}

	size_t find_last_not_of( const char* s ) const {
		if ( str and s ) {
			for ( int32_t i = (int32_t)strlen(str)-1; i >= 0; i-- ) {
				if ( not strchr( s, str[i] ) ) {
					return i;
				}
			}
		}
		return npos;
	}

	size_t rfind( const char* s ) const {
		if ( str ) {
			size_t l = strlen(s);
			char* f = str + strlen(str) - 1;
			while ( f >= str ) {
				if ( strncmp( f, s, l ) == 0 ) {
					return f - str;
				}
				f--;
			}
		}
		return npos;
	}
	size_t rfind( const string& s ) const {
		if ( s.str ) {
			return rfind( s.str );
		}
		return npos;
	}

	string substr( size_t start, size_t len = npos ) const {
		string ret;
		if ( str ) {
			size_t l = strlen( str );
			if ( len == npos or start + len > l ) {
				len = l - start;
			}
			if ( len > 0 ) {
				ret.capacity = len;
				ret.str = (char*)malloc( ret.capacity + 1 );
				strncpy( ret.str, str + start, len );
				ret.str[len] = '\0';
				_string_mem += ret.capacity + 1;
			}
		}
		return ret;
	}

	string& erase( size_t pos = 0, size_t len = npos ) {
		if ( str ) {
			size_t l = strlen( str );
			if ( len == npos or pos + len > l ) {
				len = l - pos;
			}
			if ( l - len > 0 ) {
				memmove( str + pos, str + len, l - len );
			}
			str[pos + l - len] = '\0';
		}
		return *this;
	}

	char operator[]( uint32_t i ) const {
		if ( str and i < strlen(str) ) {
			return str[i];
		}
		return 0;
	}

	void operator+=( char c ) {
		size_t len = str ? strlen(str) : 0;
		reserve( len + 1 );
		str[len] = c;
	}
	void operator+=( const char* s ) {
		if ( s ) {
			size_t len = str ? strlen(str) : 0;
			reserve( len + strlen(s) );
			strcpy( str + len, s );
		}
	}
	void operator+=( const string& s ) {
		if ( s.str ) {
			size_t len = str ? strlen(str) : 0;
			reserve( len + strlen(s.str) );
			strcpy( str + len, s.str );
		}
	}

protected:
	uint32_t capacity;
	char* str;
};



string operator+( const string& s1, const char* s2 );
string operator+( const string& s1, const string& s2 );
string operator+( const char* s1, const string& s2 );
bool operator==( const string& s1, const string& s2 );
bool operator!=( const string& s1, const string& s2 );
string to_string( int64_t n );

}

#endif // MINISTD_STRING_H
