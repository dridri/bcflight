#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>

void* sbrk(size_t);

extern void board_print( int level, const char* s );


static void _printf( int level, const char* fmt, ... )
{
/*
	char buffer[128];
	char* p = buffer;
	va_list args;
	va_start( args, fmt );

	while ( *fmt ) {
		if ( *fmt == '%' ) {
			fmt++;
			switch ( *fmt ) {
				case 'c' : {
					char c = va_arg( args, int );
					*p++ = c;
					break;
				}
				case 's' : {
					char* s = va_arg( args, char* );
					while ( s && *s ) {
						*p++ = *s++;
					}
					break;
				}
				case 'd' : {
					int d = va_arg( args, int );
					bool started = false;
					int32_t divider = 1000000000;
					if ( d < 0 ) {
						*p++ = '-';
						d = -d;
					}
					while ( divider > 0 ) {
						if ( divider == 1 || ( d / divider ) % 10 != 0 ) {
							started = true;
						}
						if ( started ) {
							*p++ = '0' + ( d / divider ) % 10;
						}
						divider /= 10;
					}
					break;
				}
				case 'p' : {
					*p++ = '0';
					*p++ = 'x';
					uint32_t d = (uintptr_t)va_arg( args, void* );
					uint32_t divider = 0x10000000;
					if ( d < 0 ) {
						*p++ = '-';
						d = -d;
					}
					while ( divider > 0 ) {
						int c = ( d / divider ) % 16;
						if ( c >= 10 ) {
							*p++ = 'a' + c - 10;
						} else {
							*p++ = '0' + c;
						}
						divider /= 16;
					}
					break;
				}
				case '%' : {
					*p++ = '%';
					break;
				}
				default:
					(void)va_arg( args, int ); // ignore
					break;
			}
			fmt++;
		} else {
			*p++ = *fmt++;
		}
	}
	*p = 0;

	board_print( level, buffer );
	va_end( args );
*/
}



typedef struct {
	uint32_t negsize : 28;
	uint32_t size : 28;
	uint8_t used : 1;
	uint8_t magic : 7;
// 	uint32_t zunused1;
// 	uint32_t zunused2;
} __attribute__((packed)) chunk;

#define ALIGN_LEN 8
#define MAGIC 0b1011001

/*
typedef struct {
	uint32_t negsize : 14;
	uint32_t size : 14;
	uint8_t used : 1;
	uint8_t magic : 3;
} __attribute__((packed)) chunk;

#define ALIGN_LEN 4
#define MAGIC 0b101
*/

#define ALIGN(size) ( size + ALIGN_LEN - ( size % ALIGN_LEN ) + ALIGN_LEN - sizeof(chunk) )
/*
static chunk* first = 0;
static chunk* last = 0;


static void print_map()
{
	_printf( 0, "\n", "" );
	uint32_t i = 0;
	chunk* c = first;

#define show_max 16

	chunk* rev = last;
	while ( i < show_max ) {
		if ( rev->negsize == 0 ) {
			break;
		}
		rev = (chunk*)( ((uint8_t*)rev) - rev->negsize - sizeof(chunk) );
		i++;
	}

	i = 0;
	int force_show = 0;

	while ( 1 ) {
		if ( c == rev ) {
			force_show = 1;
		}
		if ( force_show || i < show_max || c->size == 0 || c == last ) {
			_printf( 0, "block %d : %p #%d -#%d (%s)\n", i, c, c->size, c->negsize, c->used ? "in use" : "free" );
		} else if ( i == show_max ) {
			_printf( 0, "...\n" );
		}
		if ( c->size == 0 ) {
			_printf( 0, "WARNING : block %d is empty !\n", i );
			while(1);
		}
		if ( c == last ) {
			break;
		}
		c = (chunk*)( ((uint8_t*)c) + sizeof(chunk) + c->size );
		i++;
	}
}


void defrag( chunk* c1, chunk* c2 )
{
	_printf( 0, "defrag( %p[%d], %p[%d] )\n", c1, c1->size, c2, c2->size );
	if ( c1->used == 0 && c2->used == 0 ) {
		c1->size += c2->size + sizeof(chunk);
		c2->magic = 0;
		if ( c2 == last ) {
			last = c1;
		} else {
			chunk* c2_next = (chunk*)( ((uint8_t*)c2) + sizeof(chunk) + c2->size );
			c2_next->negsize = c1->size;
		}
	}
}


static void defrag_all()
{
	_printf( 0, "defrag_all()\n", "" );
	chunk* c = first;
	while ( c != last ) {
		chunk* c2 = (chunk*)( ((uint8_t*)c) + sizeof(chunk) + c->size );
		if ( c->used == 0 && c2->used == 0 ) {
			c->size += c2->size + sizeof(chunk);
			if ( c2 == last ) {
				last = c;
			}
		} else {
			c = (chunk*)( ((uint8_t*)c) + sizeof(chunk) + c->size );
		}
	}
}


static chunk* find_free_chunk( size_t s )
{
	_printf( 0, "find_free_chunk( %d )\n", s );
	chunk* c = first;
	bool ok = false;
	while ( 1 ) {
		if ( c->used == 0 && c->size >= s ) {
			ok = true;
			break;
		}
		if ( c == last ) {
			break;
		}
		c = (chunk*)( ((uint8_t*)c) + sizeof(chunk) + c->size );
	}

	if ( ok ) {
		_printf( 0, "find_free_chunk: found block : %p #%d -#%d (%s)\n", c, c->size, c->negsize, c->used ? "in use" : "free" );
		return c;
	}
	return 0;
}


static void shrink_chunk( chunk* block, size_t s )
{
	chunk* next = (chunk*)( ((uint8_t*)block) + sizeof(chunk) + s );
	next->magic = MAGIC;
	next->used = 0;
	next->negsize = s;
	next->size = block->size - s - sizeof(chunk);
	block->size = s;
	if ( block == last ) {
		next = last;
	}
	_printf( 0, "shrink_chunk: created new block : %p #%d -#%d (%s)\n", next, next->size, next->negsize, next->used ? "in use" : "free" );
	if ( next != last ) {
		chunk* next_of_next = (chunk*)( ((uint8_t*)next) + sizeof(chunk) + next->size );
		_printf( 0, "shrink_chunk: next_of_next : %p #%d -#%d (%s)\n", next_of_next, next_of_next->size, next_of_next->negsize, next_of_next->used ? "in use" : "free" );
		next_of_next->negsize = next->size;
		defrag( next, next_of_next );
	}
}


void* malloc( size_t s )
{
	_printf( 0, "\nmalloc: Allocating %d => %d bytes\n", s, ALIGN( s ) );
	s = ALIGN( s );
	chunk* block = 0;

	if ( first == 0 ) {
		first = (chunk*)sbrk( s + sizeof(chunk) );
		last = first;
		block = first;
		block->negsize = 0;
		block->size = s;
	} else {
		block = find_free_chunk( s );
		if ( block ) {
			if ( block->size >= s + sizeof(chunk) + ALIGN_LEN ) { // There is space left in the block, make it available
				shrink_chunk( block, s );
			}
		} else {
			if ( last->used == 0 ) {
				_printf( 0, "malloc: extending last block\n" );
				(void)sbrk( s - last->size ); // On embedded systems, sbrk should simply extend memory contiguously after its last call
				block = last;
			} else {
				_printf( 0, "malloc: creating new last block\n" );
// 				block = (chunk*)sbrk( s + sizeof(chunk) );
				sbrk( s + sizeof(chunk) );
				block = (chunk*)( ((uint8_t*)last) + sizeof(chunk) + last->size );
				block->negsize = last->size;
				last = block;
			}
			block->size = s;
		}
	}

	block->magic = MAGIC;
	block->used = 1;
	_printf( 0, "malloc: returning %p (b%p)\n", ((uint8_t*)block) + sizeof(chunk), block );
	print_map();
	return ((uint8_t*)block) + sizeof(chunk);
}


void* realloc( void* old, size_t s )
{
	_printf( 0, "\nrealloc: Reallocating %p (b%p) to %d => %d bytes\n", old, old != 0 ? ( ((uint8_t*)old) - sizeof(chunk) ) : 0, s, ALIGN( s ) );
	if ( old == 0 ) {
		return malloc( s );
	}
	s = ALIGN( s );

	chunk* block = (chunk*)( ((uint8_t*)old) - sizeof(chunk) );

	if ( ( s == block->size || s >= block->size - sizeof(chunk) || s >= block->size - sizeof(chunk) - ALIGN_LEN ) && s <= block->size ) {
		_printf( 0, "\nrealloc: Returning same block\n" );
		return old;
	}

	if ( s <= block->size ) {
		_printf( 0, "\nrealloc: Returning shrinked same block\n" );
		if ( block->size >= s + sizeof(chunk) + ALIGN_LEN ) {
			shrink_chunk( block, s );
			block->size = s;
		}
		print_map();
		return old;
	}

	if ( block == last ) {
		_printf( 0, "\nrealloc: Is last block, expanding\n" );
		block->used = 0;
		print_map();
		return malloc( s );
	}

	chunk* next = (chunk*)( ((uint8_t*)block) + sizeof(chunk) + block->size );
	if ( next->used == 0 && block->size + sizeof(chunk) + next->size >= s ) {
		_printf( 0, "\nrealloc: Returning merged with next block b%p\n", next );
		block->used = 0;
		defrag( block, next );
		block->used = 1;
		if ( block->size >= s + sizeof(chunk) + ALIGN_LEN ) {
			shrink_chunk( block, s );
			block->size = s;
		}
		print_map();
		return old;
	}

	_printf( 0, "\nrealloc: Full realloc\n" );
	block->used = 0;
	void* ret = malloc( s );
	memcpy( ret, old, s );
	print_map();
	return ret;
}


void* calloc( size_t num, size_t size )
{
	void* ptr = malloc( num * size );
	memset( ptr, 0, num * size );
	return ptr;
}


void free( void* p )
{
	if ( p == 0 ) {
		return;
	}
	chunk* block = (chunk*)( ((uint8_t*)p) - sizeof(chunk) );
	_printf( 0, "\nfree: freeing %p (b%p) of size %d\n", p, block, block->size );
	if ( block->magic != MAGIC ) {
		_printf( 0, "\nfree: double free or corruption !\n\rAborted.\n\r" );
#ifdef SYSTEM_NAME_Linux
		exit(0);
#else
		while ( 1 ) {
		}
#endif
	}
	block->used = 0;

	if ( block != last ) {
		chunk* next = (chunk*)( ((uint8_t*)block) + sizeof(chunk) + block->size );
		defrag( block, next );
	}
	if ( block != first ) {
		chunk* prev = (chunk*)( ((uint8_t*)block) - sizeof(chunk) - block->negsize );
		defrag( prev, block );
	}

	print_map();
}
*/
