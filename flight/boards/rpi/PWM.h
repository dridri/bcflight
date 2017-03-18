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

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <vector>

class PWM
{
public:
	PWM( uint32_t pin, uint32_t time_base, uint32_t period_time_us, uint32_t sample_us, bool loop = true );
	~PWM();

	void SetPWMus( uint32_t width_us );
// 	void SetPWMBuffer( uint8_t* buffer, uint32_t len );
	void Update();

private:
	class Channel {
	public:
		typedef enum {
			MODE_NONE = 0,
			MODE_PWM = 1,
			MODE_BUFFER = 2,
		} PinMode;

		Channel( uint8_t channel, uint32_t time_base, uint32_t period_time_us, uint32_t sample_us, bool loop = true );
		~Channel();

		void SetPWMus( uint32_t pin, uint32_t width_us );
		void SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len );
		void Update();

		uint8_t mChannel;
		bool mLoop;
		uint8_t mPinsCount;
		uint32_t mPinsMask;
		uint8_t mPins[32];
		float mPinsPWMf[32];
		uint32_t mPinsPWM[32];
		PinMode mPinsMode[32];
		uint8_t* mPinsBuffer[32];
		uint32_t mPinsBufferLength[32];
		uint32_t mTimeBase;
		uint32_t mCycleTime;
		uint32_t mSampleTime;
		uint32_t mNumSamples;
		uint32_t mNumCBs;
		uint32_t mNumPages;

		uint32_t periph_virt_base;
		uint32_t periph_phys_base;
		uint32_t mem_flag;

		volatile uint32_t *pwm_reg;
		volatile uint32_t *clk_reg;
		volatile uint32_t *dma_virt_base; /* base address of all PWM Channels */
		volatile uint32_t *dma_reg; /* pointer to the PWM Channel registers we are using */
		volatile uint32_t *gpio_reg;

		typedef struct dma_cb_s {
			uint32_t info;
			uint32_t src;
			uint32_t dst;
			uint32_t length;
			uint32_t stride;
			uint32_t next;
			uint32_t pad[2];
		} dma_cb_t;

		struct {
			int handle;		/* From mbox_open() */
			unsigned mem_ref;	/* From mem_alloc() */
			unsigned bus_addr;	/* From mem_lock() */
			uint8_t *virt_addr;	/* From mapmem() */
		} mMbox;
		typedef struct dma_ctl_s {
			uint32_t* sample;
			dma_cb_t* cb;
		} dma_ctl_t;

		dma_ctl_t mCtls[2];
		uint32_t mCurrentCtl;

		void update_pwm();
		int mbox_open();
		void init_ctrl_data();
		void init_dma_ctl( dma_ctl_t* ctl );
		void init_hardware( uint32_t time_base );
		uint32_t mem_virt_to_phys( void* virt );

		void get_model( unsigned mbox_board_rev );
		void* map_peripheral( uint32_t base, uint32_t len );

	};

	Channel* mChannel;
	uint8_t mPin;

	static std::vector< Channel* > mChannels;
	static void terminate( int dummy );
	static bool mSigHandlerOk;

};

#endif // PWM_H
