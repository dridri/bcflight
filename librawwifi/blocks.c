#include <stdlib.h>
#include <string.h>
#include "rawwifi.h"

rawwifi_block_t* blocks_insert_front( rawwifi_block_t** list, uint16_t packets_count )
{
	if ( packets_count < 16 ) {
		packets_count = 16;
	}else if ( packets_count > 2048 ) {
		packets_count = 2048;
	}

	rawwifi_block_t* block = (rawwifi_block_t*)malloc( sizeof(rawwifi_block_t) );
	memset( block, 0, sizeof(rawwifi_block_t) );
	block->packets_count = packets_count;
	block->packets = (rawwifi_packet_t*)malloc( sizeof(rawwifi_packet_t) * packets_count );
	memset( block->packets, 0, sizeof(rawwifi_packet_t) * packets_count );

	block->next = *list;
	if ( *list ) {
		(*list)->prev = block;
	}
	*list = block;

	return block;
}


void blocks_pop_front( rawwifi_block_t** list )
{
}


void blocks_pop( rawwifi_block_t** list, rawwifi_block_t** _block )
{
	rawwifi_block_t* block = *_block;

	if ( block == (*list) ) {
		*list = block->next;
	}
	if ( block->prev ) {
		block->prev->next = block->next;
	}
	if ( block->next ) {
		block->next->prev = block->prev;
	}

	free( block->packets );
	free( block );
	*_block = 0;
}


void blocks_pop_back( rawwifi_block_t** list )
{
}
