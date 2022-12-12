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
#include <Main.h>

using namespace std;

class PWM
{
public:
	typedef enum {
		MODE_NONE = 0,
		MODE_PWM = 1,
		MODE_BUFFER = 2,
	} PWMMode;

	PWM( uint32_t pin, uint32_t time_base, uint32_t period_time_us, uint32_t sample_us, PWMMode mode = MODE_PWM, bool loop = true );
	PWM( uint32_t pin, uint32_t time_base, uint32_t period_time_us, PWMMode mode = MODE_PWM, int32_t enablePin = -1 );
	~PWM();

	void SetPWMus( uint32_t width_us ); // TODO : Change to SetPWMValue
	void SetPWMBuffer( uint8_t* buffer, uint32_t len );
	void Update();

	static void terminate( int dummy );
	static void EnableTruePWM();
	static bool HasTruePWM();

private:
	class Channel {
	public:
		virtual ~Channel() {}
		virtual void SetPWMValue( uint32_t pin, uint32_t width ) = 0;
		virtual void SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len ) = 0;
		virtual void Update() = 0;
		virtual void Reset() = 0;

		uint32_t plldfreq_mhz;
		uint32_t periph_virt_base;
		uint32_t periph_phys_base;
		uint32_t mem_flag;

		struct {
			int handle;		/* From mbox_open() */
			unsigned mem_ref;	/* From mem_alloc() */
			unsigned bus_addr;	/* From mem_lock() */
			uint8_t *virt_addr;	/* From mapmem() */
		} mMbox;
		int mbox_open();
		void get_model( unsigned mbox_board_rev );
		void* map_peripheral( uint32_t base, uint32_t len );
		uint32_t mem_virt_to_phys( void* virt );
	};
	class MultiplexPWMChannel : public Channel {
	public:
		MultiplexPWMChannel( uint32_t time_base, uint32_t period, PWMMode mode = MODE_PWM );
		~MultiplexPWMChannel() noexcept;

		void SetPWMValue( uint32_t pin, uint32_t width );
		void SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len );
		void Update();
		void Reset();

		uint8_t mEngine;
		PWMMode mMode;
		bool mLoop;
		uint32_t mPwmCtl;
		uint32_t mNumOutputs;

		uint32_t mDMAChannel;
		uint32_t mNumCBs;
		uint32_t mNumPages;
		uint32_t* mSelectPinResetMask;
		uint32_t* mSelectPinMasks;
		uint32_t* mPWMSamples;
		uint32_t* mPinsSamples[64];

		uint32_t* pwm_reg;
		uint32_t* pwm1_reg;
		uint32_t* clk_reg;
		uint32_t* gpio_reg;
		volatile uint32_t* dma_virt_base;
		volatile uint32_t* dma_reg;

		typedef struct VirtualPin {
			int32_t pwmPin;
			int32_t enablePin;
			int32_t pwmIndex;
			VirtualPin(int32_t p, int32_t e, int32_t i) : pwmPin(p), enablePin(e), pwmIndex(i) {}
		} VirtualPin;
		std::map< int32_t, VirtualPin > mVirtualPins; // If key is a PWM pin, this means that all enablePin should be set to 0. If key=enablePin, this one has to be set to 1

		typedef struct dma_cb_s {
			uint32_t info;
			uint32_t src;
			uint32_t dst;
			uint32_t length;
			uint32_t stride;
			uint32_t next;
			uint32_t pad[2];
		} dma_cb_t;

		typedef struct dma_ctl_s {
			uint32_t* sample;
			dma_cb_t* cb;
		} dma_ctl_t;

		dma_ctl_t mCtls[2];
		uint32_t mCurrentCtl;
	};
	class PWMChannel : public Channel {
	public:
		PWMChannel( uint8_t engine, uint32_t time_base, uint32_t period, PWMMode mode = MODE_PWM, bool loop = true );
		~PWMChannel();

		void SetPWMValue( uint32_t pin, uint32_t width );
		void SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len );
		void Update();
		void Reset();
	
		uint8_t mEngine;
		PWMMode mMode;
		bool mLoop;
		uint32_t mPwmCtl;

		uint32_t* pwm_reg;
		uint32_t* clk_reg;
		uint32_t* gpio_reg;
	};
	class DMAChannel : public Channel {
	public:
		DMAChannel( uint8_t channel, uint32_t time_base, uint32_t period_time_us, uint32_t sample_us, PWMMode mode = MODE_PWM, bool loop = true );
		~DMAChannel();

		void SetPWMValue( uint32_t pin, uint32_t width );
		void SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len );
		void Update();
		void Reset();

		uint8_t mChannel;
		PWMMode mMode;
		bool mLoop;
		uint8_t mPinsCount;
		uint32_t mPinsMask;
		int8_t mPins[32];
		float mPinsPWMf[32];
		uint32_t mPinsPWM[32];
		uint8_t* mPinsBuffer[32];
		uint32_t mPinsBufferLength[32];
		uint32_t mTimeBase;
		uint32_t mCycleTime;
		uint32_t mSampleTime;
		uint32_t mNumSamples;
		uint32_t mNumCBs;
		uint32_t mNumPages;

		volatile uint32_t *pcm_reg;
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

		typedef struct dma_ctl_s {
			uint32_t* sample;
			dma_cb_t* cb;
		} dma_ctl_t;

		dma_ctl_t mCtls[2];
		uint32_t mCurrentCtl;

		void update_pwm();
		void update_pwm_buffer();
		void init_ctrl_data();
		void init_dma_ctl( dma_ctl_t* ctl );
		void init_hardware( uint32_t time_base );
	};

	Channel* mChannel;
	uint8_t mPin;

	static vector< Channel* > mChannels;
	static bool mSigHandlerOk;
	static bool sTruePWM;
};

#endif // PWM_H
