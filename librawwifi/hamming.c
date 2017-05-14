#include <stdlib.h>
#include <string.h>
#include "rawwifi.h"

// WARNING : this is actually a Hamming(7,4) code, with 8th bit for alignment

uint32_t rawwifi_hamming84_encode( uint8_t* dest, uint8_t* src, uint32_t len )
{
	//TEST
// 	memcpy( dest, src, len );
// 	return len;

	uint32_t b = 0;
	uint32_t out = 0;
	uint32_t i = 0;
	for ( b = 0; b < len; b++ )
	{
		{
			uint8_t d1 = ( src[b] >> 0 ) & 1;
			uint8_t d2 = ( src[b] >> 1 ) & 1;
			uint8_t d3 = ( src[b] >> 2 ) & 1;
			uint8_t d4 = ( src[b] >> 3 ) & 1;
			uint8_t p1 = d1 ^ d2 ^ d4;
			uint8_t p2 = d1 ^ d3 ^ d4;
			uint8_t p3 = d2 ^ d3 ^ d4;
			uint8_t p4 = 0; // unused for now
			dest[(b << 1) + 0] = ( p1 << 0 ) | ( p2 << 1 ) | ( d1 << 2 ) | ( p3 << 3 ) | ( d2 << 4 ) | ( d3 << 5 ) | ( d4 << 6 );
		}
		{
			uint8_t d1 = ( src[b] >> 4 ) & 1;
			uint8_t d2 = ( src[b] >> 5 ) & 1;
			uint8_t d3 = ( src[b] >> 6 ) & 1;
			uint8_t d4 = ( src[b] >> 7 ) & 1;
			uint8_t p1 = d1 ^ d2 ^ d4;
			uint8_t p2 = d1 ^ d3 ^ d4;
			uint8_t p3 = d2 ^ d3 ^ d4;
			uint8_t p4 = 0; // unused for now
			dest[(b << 1) + 1] = ( p1 << 0 ) | ( p2 << 1 ) | ( d1 << 2 ) | ( p3 << 3 ) | ( d2 << 4 ) | ( d3 << 5 ) | ( d4 << 6 );
		}
	}

	return len << 1;
}


uint32_t rawwifi_hamming84_decode( uint8_t* dest, uint8_t* src, uint32_t len )
{
	// TODO
// 	memcpy( dest, src, len );
// 	return len;

	uint32_t b = 0;
	uint32_t out = 0;
	uint32_t i = 0;
	for ( b = 0; b < len; b+=2 )
	{
		{
			uint8_t p1 = ( src[b] >> 0 ) & 1;
			uint8_t p2 = ( src[b] >> 1 ) & 1;
			uint8_t d1 = ( src[b] >> 2 ) & 1;
			uint8_t p3 = ( src[b] >> 3 ) & 1;
			uint8_t d2 = ( src[b] >> 4 ) & 1;
			uint8_t d3 = ( src[b] >> 5 ) & 1;
			uint8_t d4 = ( src[b] >> 6 ) & 1;
			dest[b >> 1] = ( dest[b >> 1] & 0xF0 ) | ( ( d1 << 0 ) | ( d2 << 1 ) | ( d3 << 2 ) | ( d4 << 3 ) );
		}
		{
			uint8_t p1 = ( src[b+1] >> 0 ) & 1;
			uint8_t p2 = ( src[b+1] >> 1 ) & 1;
			uint8_t d1 = ( src[b+1] >> 2 ) & 1;
			uint8_t p3 = ( src[b+1] >> 3 ) & 1;
			uint8_t d2 = ( src[b+1] >> 4 ) & 1;
			uint8_t d3 = ( src[b+1] >> 5 ) & 1;
			uint8_t d4 = ( src[b+1] >> 6 ) & 1;
			dest[b >> 1] = ( dest[b >> 1] & 0x0F ) | ( ( ( d1 << 0 ) | ( d2 << 1 ) | ( d3 << 2 ) | ( d4 << 3 ) ) << 4 );
		}
	}

	return len >> 1;
}
