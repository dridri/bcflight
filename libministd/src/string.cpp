#include "../include/string"

namespace ministd {

size_t _string_mem = 0;

static inline string op_plus( const string& s1, const char* s2 )
{
	if (not s1.c_str() ) {
		return string(s2);
	}
	string ret;
	ret.reserve( s1.length() + strlen(s2) );
	strcpy( ret.data(), s1.c_str() );
	strcat( ret.data(), s2 );
	return ret;
}


string operator+( const string& s1, const char* s2 )
{
	return op_plus( s1, s2 );
}


string operator+( const string& s1, const string& s2 )
{
	if ( not s2.c_str() ) {
		return s1;
	}
	return op_plus( s1, s2.c_str() );
}


string operator+( const char* s1, const string& s2 )
{
	if (not s2.c_str() ) {
		return string(s1);
	}
	string ret;
	ret.reserve( strlen(s1) + s2.length() );
	strcpy( ret.data(), s1 );
	strcat( ret.data(), s2.c_str() );
	return ret;
}


bool operator==( const string& s1, const string& s2 )
{
	if ( not s1.c_str() or not s2.c_str() ) {
		return false;
	}
	return strcmp( s1.c_str(), s2.c_str() ) == 0;
}


bool operator!=( const string& s1, const string& s2 )
{
	if ( not s1.c_str() or not s2.c_str() ) {
		return true;
	}
	return strcmp( s1.c_str(), s2.c_str() ) != 0;
}


string to_string( int64_t n ) {
	char s[16];
	sprintf( s, "%lld", n );
	return string(s);
}

} // namespace ministd
