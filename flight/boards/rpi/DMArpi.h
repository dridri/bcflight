#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <map>


typedef struct dma_cb_s {
	uint32_t info;
	uint32_t src;
	uint32_t dst;
	uint32_t length;
	uint32_t stride;
	uint32_t next;
	uint32_t pad[2];
} dma_cb_t;


class DMArpi
{
public:
	DMArpi( uint32_t commands_count = 1 );
	~DMArpi();

protected:
	struct {
		int handle;
		unsigned mem_ref;
		unsigned bus_addr;
		uint8_t* virt_addr;
	} mMbox;
	int mbox_open();
	void get_model( unsigned mbox_board_rev );
	void* map_peripheral( uint32_t base, uint32_t len );
	uint32_t mem_virt_to_phys( void* virt );

	uint32_t plldfreq_mhz;
	uint32_t periph_virt_base;
	uint32_t periph_phys_base;
	uint32_t mem_flag;

	volatile uint32_t* dma_virt_base;
	volatile uint32_t* dma_reg;

	uint32_t mChannel;
	uint32_t* mSamples;
	dma_cb_t* mCbs;

	static std::map< uint32_t, DMArpi* > mUsedChannels;
};

#endif // DMA_H
