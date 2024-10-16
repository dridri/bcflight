/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef DSHOT_H
#define DSHOT_H

#if 0

#include <stdint.h>
#include <vector>
#include <Motor.h>

class DShot_ : public Motor
{
public:
	DShot( uint8_t pin );
	virtual ~DShot();

	virtual void Disarm();
	virtual void Disable();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );

private:
	static void terminate_dma( int dummy );

	// Pin used by *this* DShot instance
	uint8_t mPin;

	// Static variables used by all pins
	static bool mInitialized;
	static map< uint8_t, uint16_t > mPins;

	// Hardware-specific values (DMA config + pointers)
	static uint32_t mNumSamples;
	static uint32_t mNumCBs;
	static uint32_t mNumPages;
	static uint32_t periph_virt_base;
	static uint32_t periph_phys_base;
	static uint32_t mem_flag;
	static volatile uint32_t* pcm_reg;
	static volatile uint32_t* pwm_reg;
	static volatile uint32_t* clk_reg;
	static volatile uint32_t* dma_virt_base; /* base address of all PWM Channels */
	static volatile uint32_t* dma_reg; /* pointer to the PWM Channel registers we are using */
	static volatile uint32_t* gpio_reg;
	typedef struct {
		int handle;		/* From mbox_open() */
		unsigned mem_ref;	/* From mem_alloc() */
		unsigned bus_addr;	/* From mem_lock() */
		uint8_t *virt_addr;	/* From mapmem() */
	} Mbox;
	static Mbox mMbox;

	// DMA control-block structure
	typedef struct dma_cb_s {
		uint32_t info;
		uint32_t src;
		uint32_t dst;
		uint32_t length;
		uint32_t stride;
		uint32_t next;
		uint32_t pad[2];
	} dma_cb_t;

	// DMA control pairs list of < sample value i.e. GPIO mask + associated control-block >
	typedef struct dma_ctl_s {
		uint32_t* sample;
		dma_cb_t* cb;
	} dma_ctl_t;

	// Double-buffered control lists
	static dma_ctl_t mCtls[2];
	uint32_t mCurrentCtl;

	static void update_pwm();
	static void update_pwm_buffer();
	static int mbox_open();
	static void init_ctrl_data();
	static void init_dma_ctl( dma_ctl_t* ctl );
	static void init_hardware( uint64_t time_base );
	static uint32_t mem_virt_to_phys( void* virt );

	static void get_model( unsigned mbox_board_rev );
	static void* map_peripheral( uint32_t base, uint32_t len );
};

#endif // 0

#endif // DSHOT_H
