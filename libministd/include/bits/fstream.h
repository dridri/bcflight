#ifndef MINISTD_FSTREAM_H
#define MINISTD_FSTREAM_H

#include <stdio.h>
#include "string.h"

namespace ministd {

class fstream
{
public:
	typedef uint8_t openmode;
	typedef uint8_t seekdir;
	
	static const openmode in = 0x01;
	static const openmode out = 0x02;
	static const openmode binary = 0x04;
	static const openmode ate = 0x08;
	static const openmode app = 0x10;
	static const openmode trunc = 0x20;
	static const seekdir beg = SEEK_SET;
	static const seekdir cur = SEEK_CUR;
	static const seekdir end = SEEK_END;

	fstream() : mFile(nullptr) {}
	fstream( const fstream& other ) : fstream() {}
	fstream( const string& filename, openmode mode = in | out ) : fstream() {
		open( filename, mode );
	}
	~fstream() {
		if ( mFile ) {
			fclose( mFile );
		}
	}

	void open( const string& filename, openmode mode = in | out ) {
		char md[3] = { 0, 0, 0 };
		char* p = md;
		if ( mode & app ) {
			*p++ = 'a';
		} else if ( mode & out ) {
			*p++ = 'w';
		} else {
			*p++ = 'r';
		}
		if ( mode & in and ( mode & app or mode & out ) ) {
			*p++ = '+';
		}
		if ( mode & binary ) {
			*p++ = 'b';
		}
		mFile = fopen( filename.c_str(), md );
	}

	void close() {
		if ( mFile ) {
			fclose( mFile );
			mFile = nullptr;
		}
	}

	bool is_open() const {
		return mFile != nullptr;
	}

	fstream& seekg( int32_t off, seekdir way ) {
		if ( mFile ) {
			fseek( mFile, off, way );
		}
		return *this;
	}

	int32_t tellg() {
		if ( mFile ) {
			return ftell( mFile );
		}
		return -1;
	}

	fstream& read( char* s, size_t len ) {
		if ( mFile ) {
			fread( s, 1, len, mFile );
		}
		return *this;
	}

	fstream& write( const char* s, size_t len ) {
		if ( mFile ) {
			fwrite( s, 1, len, mFile );
		}
		return *this;
	}

	FILE* __fptr() {
		return mFile;
	}

protected:
	FILE* mFile;
};

typedef fstream ifstream;
typedef fstream ofstream;


bool getline( fstream& f, string& s )
{
	char tmp[1024];
	FILE* file = f.__fptr();
	if ( file ) {
		char* ret = fgets( tmp, 1024, file );
		if ( ret ) {
			size_t len = strlen( ret );
			s.reserve( len );
			s.erase();
			s += ret;
			return true;
		}
	}
	return false;
}

}

#endif // MINISTD_FSTREAM_H
