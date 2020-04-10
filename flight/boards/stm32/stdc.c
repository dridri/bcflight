#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

extern uint32_t board_ticks();
void board_printf( int level, const char* s, ... );
void* _sbrk( size_t s );

void __cxa_pure_virtual()
{
	while ( 1 );
}


/*
static int ___errno = 0;


int* __errno(void)
{
	return &___errno;
}

int* __errno_location(void)
{
	return &___errno;
}

char* getenv( const char* name )
{
	(void)name;
	return "";
}



void* memcpy( void* d_, const void* s_, size_t n )
{
	char* d = (char*)d_;
	const char* s = (const char*)s_;
	size_t i;
	for ( i = 0; i < n; *d++ = *s++, i++ );
	return d_;
}



size_t strlen( const char* s )
{
	const char* d = s;
	while ( *d++ );
	return d - s;
}

char* strcpy( char* d, const char* s )
{
	char* d_ = d;
	while ( *s ) {
		*d++ = *s++;
	}
	return d_;
}

int strcmp( const char* s1, const char* s2 )
{
	while ( *s1 && *s2 ) {
		if ( *s1++ != *s2++ ) {
			s1--;
			s2--;
			return ( *s1 - *s2 );
		}
	}
	return 0;
}

char* strchr( const char* s, int c )
{
	while ( *s ) {
		if ( *s++ == c ) {
			return (char*)( s - 1 );
		}
	}
	return NULL;
}






FILE* fopen( const char* fn, const char* md )
{
	(void)fn;
	(void)md;
	return NULL;
}


int fclose( FILE* fp )
{
	(void)fp;
	return 0;
}
*/
