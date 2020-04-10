/*
 * Copyright (c) 2013 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

extern char __heap_start __attribute__((weak)) __attribute__((section("._user_heap_stack")));
extern char __heap_end __attribute__((weak)) __attribute__((section("._user_heap_stack")));//set by linker
extern uint32_t board_ticks();

int _getpid(void) { return 1; }

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit(int status)
{
	while (1) {
	}
}

int _write(int file, char *ptr, int len)
{
	// TODO
	return len;
}

static uint32_t sMem = 0;
static uint32_t sMemSize = 0;

uint32_t _mem_usage()
{
	return sMem;
}


uint32_t _mem_size()
{
	return sMemSize;
}


void * _sbrk (int  incr)
{
	static char *_heap_end = &__heap_end;		/* Previous end of heap or 0 if none */
	static char *heap_end = &__heap_start;		/* Previous end of heap or 0 if none */
	char        *prev_heap_end;

	if ( sMemSize == 0 ) {
		sMemSize = &__heap_end - &__heap_start;
	}

	prev_heap_end  = heap_end;
	heap_end      += incr;
	//check
	if( heap_end < (_heap_end)) {

	} else {
		__asm__("bkpt #0");
		errno = ENOMEM;
		return (char*)-1;
	}
// 	__asm__("bkpt #0");
	sMem += incr;
	return (void *) prev_heap_end;

} 

int _close(int file) { return -1; }

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file) { return 1; }

int _lseek(int file, int ptr, int dir) { return 0; }

int _read(int file, char *ptr, int len) { return 0; }

int _open(char *path, int flags, ...)
{
	// Pretend like we always fail
	return -1;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _stat(char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}

int _gettimeofday( struct timeval *tv, void *tzvp )
{
	uint64_t t = board_ticks();  // get uptime in nanoseconds
	tv->tv_sec = t / 1000000;  // convert to seconds
	tv->tv_usec = ( t % 1000000 );  // get remaining microseconds
	return 0;  // return non-zero for error
}


clock_t _times( struct tms* buf )
{
	// TODO ?
	errno = EINVAL;
	return -1;
}

