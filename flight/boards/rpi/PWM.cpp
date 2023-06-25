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

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE   700

#include <execinfo.h>
#include <signal.h>
#include <sys/sysmacros.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "mailbox.h"
#include <Debug.h>
#include <Config.h>
#include "PWM.h"
#include "GPIO.h"

// pin2gpio array is not setup as empty to avoid locking all GPIO
// inputs as PWM, they are set on the fly by the pin param passed.
// static uint8_t pin2gpio[8];

#define DEVFILE			"/dev/pi-blaster"
#define DEVFILE_MBOX    "/dev/pi-blaster-mbox"
#define DEVFILE_VCIO	"/dev/vcio"

#define PAGE_SIZE		4096
#define PAGE_SHIFT		12

#define DMA_BASE		(periph_virt_base + 0x00007000)
#define DMA_CHAN_SIZE	0x100 /* size of register space for a single DMA channel */
#define DMA_CHAN_MAX	14  /* number of DMA Channels we have... actually, there are 15... but channel fifteen is mapped at a different DMA_BASE, so we leave that one alone */
#define PWM0_BASE_OFFSET 0x0020C000
#define PWM0_BASE		(periph_virt_base + PWM0_BASE_OFFSET)
#define PWM0_PHYS_BASE	(periph_phys_base + PWM0_BASE_OFFSET)
#define PWM0_LEN			0x28
#define PWM1_BASE_OFFSET 0x0020C800
#define PWM1_BASE		(periph_virt_base + PWM1_BASE_OFFSET)
#define PWM1_PHYS_BASE	(periph_phys_base + PWM1_BASE_OFFSET)
#define PWM1_LEN			0x28
#define PWM_BASE_OFFSET PWM0_BASE_OFFSET
#define PWM_BASE PWM0_BASE
#define PWM_PHYS_BASE PWM0_PHYS_BASE
#define PWM_LEN PWM0_LEN
#define PCM_BASE_OFFSET	0x00203000
#define PCM_BASE		(periph_virt_base + PCM_BASE_OFFSET)
#define PCM_PHYS_BASE	(periph_phys_base + PCM_BASE_OFFSET)
#define PCM_LEN			0x24
#define CLK_BASE_OFFSET 0x00101000
#define CLK_BASE		(periph_virt_base + CLK_BASE_OFFSET)
#define CLK_LEN			0xA8
#define GPIO_BASE_OFFSET 0x00200000
#define GPIO_BASE		(periph_virt_base + GPIO_BASE_OFFSET)
#define GPIO_PHYS_BASE	(periph_phys_base + GPIO_BASE_OFFSET)
#define GPIO_LEN		0x100

#define DMA_NO_WIDE_BURSTS	(1<<26)
#define DMA_WAIT_RESP		(1<<3)
#define DMA_TDMODE		(1<<1)
#define DMA_D_DREQ		(1<<6)
#define DMA_PER_MAP(x)		((x)<<16)
#define DMA_END			(1<<1)
#define DMA_RESET		(1<<31)
#define DMA_INT			(1<<2)
#define DMAENABLE 0x00000ff0

#define DMA_CS			(0x00/4)
#define DMA_CONBLK_AD		(0x04/4)
#define DMA_SOURCE_AD		(0x0c/4)
#define DMA_DEBUG		(0x20/4)

#define GPIO_FSEL0		(0x00/4)
#define GPIO_SET0		(0x1c/4)
#define GPIO_CLR0		(0x28/4)
#define GPIO_LEV0		(0x34/4)
#define GPIO_PULLEN		(0x94/4)
#define GPIO_PULLCLK		(0x98/4)

#define GPIO_MODE_IN	0b000
#define GPIO_MODE_OUT	0b001
#define GPIO_MODE_ALT0	0b100
#define GPIO_MODE_ALT1	0b101
#define GPIO_MODE_ALT2	0b110
#define GPIO_MODE_ALT3	0b111
#define GPIO_MODE_ALT4	0b011
#define GPIO_MODE_ALT5	0b010

#define PWM_CTL			(0x00/4)
#define PWM_PWMC		(0x08/4)
#define PWM_RNG1		(0x10/4)
#define PWM_DAT1		(0x14/4)
#define PWM_FIFO		(0x18/4)
#define PWM_RNG2		(0x20/4)
#define PWM_DAT2		(0x24/4)

#define PWMCLK_CNTL		40
#define PWMCLK_DIV		41

#define PWMCTL_PWEN1	(1<<0)
#define PWMCTL_MODE1	(1<<1)
#define PWMCTL_RPTL1	(1<<2)
#define PWMCTL_SBIT1	(1<<3)
#define PWMCTL_POLA1	(1<<4)
#define PWMCTL_USEF1	(1<<5)
#define PWMCTL_CLRF		(1<<6)
#define PWMCTL_MSEN1	(1<<7)
#define PWMCTL_PWEN2	(1<<8)
#define PWMCTL_MODE2	(1<<9)
#define PWMCTL_RPTL2	(1<<10)
#define PWMCTL_SBIT2	(1<<11)
#define PWMCTL_POLA2	(1<<12)
#define PWMCTL_USEF2	(1<<13)
#define PWMCTL_MSEN2	(1<<15)

#define PWMPWMC_ENAB		(1<<31)
#define PWMPWMC_THRSHLD		((15<<8)|(15<<0))

#define PCM_CS_A		(0x00/4)
#define PCM_FIFO_A		(0x04/4)
#define PCM_MODE_A		(0x08/4)
#define PCM_RXC_A		(0x0c/4)
#define PCM_TXC_A		(0x10/4)
#define PCM_DREQ_A		(0x14/4)
#define PCM_INTEN_A		(0x18/4)
#define PCM_INT_STC_A		(0x1c/4)
#define PCM_GRAY		(0x20/4)
#define PCMCLK_CNTL		38
#define PCMCLK_DIV	39

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

/* New Board Revision format: 
SRRR MMMM PPPP TTTT TTTT VVVV

S scheme (0=old, 1=new)
R RAM (0=256, 1=512, 2=1024)
M manufacturer (0='SONY',1='EGOMAN',2='EMBEST',3='UNKNOWN',4='EMBEST')
P processor (0=2835, 1=2836)
T type (0='A', 1='B', 2='A+', 3='B+', 4='Pi 2 B', 5='Alpha', 6='Compute Module')
V revision (0-15)
*/
#define BOARD_REVISION_SCHEME_MASK (0x1 << 23)
#define BOARD_REVISION_SCHEME_OLD (0x0 << 23)
#define BOARD_REVISION_SCHEME_NEW (0x1 << 23)
#define BOARD_REVISION_RAM_MASK (0x7 << 20)
#define BOARD_REVISION_MANUFACTURER_MASK (0xF << 16)
#define BOARD_REVISION_MANUFACTURER_SONY (0 << 16)
#define BOARD_REVISION_MANUFACTURER_EGOMAN (1 << 16)
#define BOARD_REVISION_MANUFACTURER_EMBEST (2 << 16)
#define BOARD_REVISION_MANUFACTURER_UNKNOWN (3 << 16)
#define BOARD_REVISION_MANUFACTURER_EMBEST2 (4 << 16)
#define BOARD_REVISION_PROCESSOR_MASK (0xF << 12)
#define BOARD_REVISION_PROCESSOR_2835 (0 << 12)
#define BOARD_REVISION_PROCESSOR_2836 (1 << 12)
#define BOARD_REVISION_TYPE_MASK (0xFF << 4)
#define BOARD_REVISION_TYPE_PI1_A (0 << 4)
#define BOARD_REVISION_TYPE_PI1_B (1 << 4)
#define BOARD_REVISION_TYPE_PI1_A_PLUS (2 << 4)
#define BOARD_REVISION_TYPE_PI1_B_PLUS (3 << 4)
#define BOARD_REVISION_TYPE_PI2_B (4 << 4)
#define BOARD_REVISION_TYPE_ALPHA (5 << 4)
#define BOARD_REVISION_TYPE_PI3_B (8 << 4)
#define BOARD_REVISION_TYPE_PI3_BP (0xD << 4)
#define BOARD_REVISION_TYPE_PI3_AP (0x7 << 5)
#define BOARD_REVISION_TYPE_CM (6 << 4)
#define BOARD_REVISION_TYPE_CM3 (10 << 4)
#define BOARD_REVISION_TYPE_PI4_B (0x11 << 4)
#define BOARD_REVISION_TYPE_CM4 (20 << 4)
#define BOARD_REVISION_REV_MASK (0xF)

#define LENGTH(x)  (sizeof(x) / sizeof(x[0]))

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define DEBUG 1
#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

#define MAX_MULTIPLEX_CHANNELS 4

vector< PWM::Channel* > PWM::mChannels = vector< PWM::Channel* >();
bool PWM::mSigHandlerOk = false;
bool PWM::sTruePWM = false;

static void fatal( const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
	exit(0);
}


void PWM::EnableTruePWM()
{
	int handle = open( DEVFILE_VCIO, 0 );
	if ( handle < 0 ) {
		printf("Failed to open mailbox\n");
		return;
	}
	uint32_t mbox_board_rev = get_board_revision( handle );
	close( handle );

	const uint32_t type = ( mbox_board_rev & BOARD_REVISION_TYPE_MASK );
	if ( type != BOARD_REVISION_TYPE_PI4_B and type != BOARD_REVISION_TYPE_CM4 ) {
		gDebug() << "WARNING : TruePWM can only be enabled on Raspberry Pi >=4 models (older models only has 2 PWM channels)";
		gDebug() << "Detected board revision : " << type;
		return;
	}

	sTruePWM = true;
}


bool PWM::HasTruePWM()
{
	return sTruePWM;
}


PWM::PWM( uint32_t pin, uint32_t time_base, uint32_t period_time, uint32_t sample_time, PWMMode mode, bool loop )
	: mChannel( nullptr )
	, mPin( pin )
{
	fDebug( pin, time_base, period_time, sample_time, mode, loop );
	if ( not mSigHandlerOk ) {
		mSigHandlerOk = true;
// 		static int sig_list[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 15, 16, 19, 20 };
		/*
		static int sig_list[] = { 2 };
		uint32_t i;
		for ( i = 0; i < sizeof(sig_list)/sizeof(int); i++ ) {
			struct sigaction sa;
			memset( &sa, 0, sizeof(sa) );
			sa.sa_handler = &PWM::terminate;
			sigaction( sig_list[i], &sa, NULL );
		}
		*/
	}

	if ( sTruePWM ) {
		int8_t engine = -1;
		if ( mPin == 12 or mPin == 13 or mPin == 18 or mPin == 19 or mPin == 45 ) {
			engine = 0;
		} else if ( mPin == 40 or mPin == 41 ) {
			engine = 1;
		} else {
			return;
		}
		for ( Channel* chan_ : mChannels ) {
			PWMChannel* chan = dynamic_cast< PWMChannel* >( chan_ );
			if ( chan->mEngine == engine ) {
				mChannel = chan;
			}
		}
		if ( not mChannel ) {
			mChannel = new PWMChannel( engine, time_base, period_time, mode, loop );
			mChannels.emplace_back( mChannel );
		}
	} else {
	// 	uint8_t channel = 13;
		int8_t channel = 6; // DMA channels 7+ use LITE DMA engine, so we should use lowest channels
		for ( Channel* chan_ : mChannels ) {
			DMAChannel* chan = dynamic_cast< DMAChannel* >( chan_ );
			if ( chan->mTimeBase == time_base and chan->mCycleTime == period_time and chan->mSampleTime == sample_time and chan->mLoop == loop ) {
				mChannel = chan;
				break;
			}
			channel = chan->mChannel - 1;
			if ( channel < 0 ) {
				break;
			}
		}
		if ( not mChannel and channel >= 0 ) {
			mChannel = new DMAChannel( channel, time_base, period_time, sample_time, mode, loop );
			mChannels.emplace_back( mChannel );
		}
	}

	mChannel->SetPWMValue( mPin, 0 );
}


PWM::PWM( uint32_t pin, uint32_t time_base, uint32_t period_time, PWM::PWMMode mode, int32_t enablePin )
	: mChannel( nullptr )
	, mPin( pin )
{
	if ( !sTruePWM ) {
		gDebug() << "ERROR : TruePWM is not available !";
	}

	int8_t engine = -1;
	if ( mPin == 12 or mPin == 13 or mPin == 18 or mPin == 19 or mPin == 45 ) {
		engine = 0;
	// } else if ( mPin == 40 or mPin == 41 ) {
		// engine = 1;
	} else {
		return;
	}

	for ( Channel* chan_ : mChannels ) {
		MultiplexPWMChannel* chan = dynamic_cast< MultiplexPWMChannel* >( chan_ );
		if ( chan->mEngine ) {
			mChannel = chan;
			break;
		}
	}

	if ( !mChannel ) {
		mChannel = new MultiplexPWMChannel( time_base, period_time, mode );
		mChannels.emplace_back( mChannel );
	}

	const uint32_t alt = ( ( pin == 18 or pin == 19 ) ? GPIO_MODE_ALT5 : GPIO_MODE_ALT0 );
	dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_FSEL0 + pin/10] &= ~(7 << ((pin % 10) * 3));
	dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_FSEL0 + pin/10] |= alt << ((pin % 10) * 3);
	dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_CLR0] = 1 << pin;

	const int32_t idx = dynamic_cast<MultiplexPWMChannel*>(mChannel)->mVirtualPins.size();
	if ( enablePin >= 0 ) {
		dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_CLR0] = (1 << enablePin);
		uint32_t fsel = dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_FSEL0 + enablePin/10];
		fsel &= ~(7 << ((enablePin % 10) * 3));
		fsel |= GPIO_MODE_OUT << ((enablePin % 10) * 3);
		dynamic_cast<MultiplexPWMChannel*>(mChannel)->gpio_reg[GPIO_FSEL0 + enablePin/10] = fsel;
		dynamic_cast<MultiplexPWMChannel*>(mChannel)->mVirtualPins.emplace( std::make_pair(enablePin, MultiplexPWMChannel::VirtualPin(pin, enablePin, idx)) );
		dynamic_cast<MultiplexPWMChannel*>(mChannel)->mSelectPinMasks[idx] = ( 1 << enablePin );
		*dynamic_cast<MultiplexPWMChannel*>(mChannel)->mSelectPinResetMask |= ( 1 << enablePin );
	} else {
		dynamic_cast<MultiplexPWMChannel*>(mChannel)->mVirtualPins.emplace( std::make_pair(pin, MultiplexPWMChannel::VirtualPin(pin, enablePin, idx)) );
	}
}


PWM::~PWM()
{
}

void PWM::SetPWMus( uint32_t width )
{
	if ( not mChannel ) {
		return;
	}
	mChannel->SetPWMValue( mPin, width );
}


void PWM::SetPWMBuffer( uint8_t* buffer, uint32_t len )
{
	if ( not mChannel ) {
		return;
	}
	mChannel->SetPWMBuffer( mPin, buffer, len );
}


void PWM::Update()
{
	if ( not mChannel ) {
		return;
	}
	mChannel->Update();
}


static inline uint32_t hzToDivider( uint64_t plldfreq_mhz, uint64_t hz )
{
	fDebug( plldfreq_mhz, hz );
	const uint64_t pll = plldfreq_mhz * 1000000;
	const uint64_t ipart = pll / hz;
	const uint64_t fpart = ( pll * 2048 / hz ) - ipart * 2048;
	gDebug() << "  → " << ipart << ", " << fpart;
	return ( ipart << 12 ) | fpart;
}


PWM::MultiplexPWMChannel::MultiplexPWMChannel( uint32_t time_base, uint32_t period, PWM::PWMMode mode )
	: mMode( mode )
	, mLoop( false )
{
	fDebug( time_base, period, mode );

	int8_t dmaChannel = 5; // DMA channels 7+ use LITE DMA engine, so we should use lowest channels
	for ( Channel* chan_ : mChannels ) {
		DMAChannel* chan = dynamic_cast< DMAChannel* >( chan_ );
		dmaChannel = chan->mChannel - 1;
		if ( dmaChannel < 0 ) {
			break;
		}
	}
	if ( dmaChannel < 0 ) {
		return;
	}
	mDMAChannel = dmaChannel;
	mNumOutputs = MAX_MULTIPLEX_CHANNELS;
	mNumCBs = mNumOutputs * 3;
	mNumPages = ( mNumCBs * sizeof(dma_cb_t) + ( mNumOutputs * 1 * sizeof(uint32_t) ) + PAGE_SIZE - 1 ) >> PAGE_SHIFT;

	mMbox.handle = mbox_open();
	if ( mMbox.handle < 0 ) {
		fatal("Failed to open mailbox\n");
	}
	uint32_t mbox_board_rev = get_board_revision( mMbox.handle );
	gDebug() << "MBox Board Revision: " << mbox_board_rev;
	get_model(mbox_board_rev);
	uint32_t mbox_dma_channels = get_dma_channels( mMbox.handle );
	gDebug() << "PWM Channels Info: " << mbox_dma_channels << ", using DMA Channel: " << (int)mDMAChannel;

	dma_virt_base = (uint32_t*)map_peripheral( DMA_BASE, ( DMA_CHAN_SIZE * ( DMA_CHAN_MAX + 1 ) ) );
	dma_reg = dma_virt_base + mDMAChannel * ( DMA_CHAN_SIZE / sizeof(dma_reg) );
	pwm_reg = (uint32_t*)map_peripheral( PWM0_BASE, PWM0_LEN );
	pwm1_reg = (uint32_t*)map_peripheral( PWM1_BASE, PWM1_LEN );
	clk_reg = (uint32_t*)map_peripheral( CLK_BASE, CLK_LEN );
	gpio_reg = (uint32_t*)map_peripheral( GPIO_BASE, GPIO_LEN );

	uint32_t cmd_count = 32;
	uint32_t sz = mNumPages * PAGE_SIZE * cmd_count;
	mMbox.mem_ref = mem_alloc( mMbox.handle, sz, PAGE_SIZE, mem_flag );
	dprintf( "mem_ref %u\n", mMbox.mem_ref );
	mMbox.bus_addr = mem_lock( mMbox.handle, mMbox.mem_ref );
	dprintf( "bus_addr = %#x (sz = %d)\n", mMbox.bus_addr, sz );
	mMbox.virt_addr = (uint8_t*)mapmem( BUS_TO_PHYS(mMbox.bus_addr), sz );
	dprintf( "virt_addr %p\n", mMbox.virt_addr );

	mCtls[0].sample = (uint32_t*)( mMbox.virt_addr );
	mCtls[0].cb = (dma_cb_t*)( mMbox.virt_addr + sizeof(uint32_t)*16 );
/*
	mSelectPinResetMask = &mCtls[0].sample[0];
	mSelectPinMasks = &mCtls[0].sample[1];
	mPWMSamples = &mCtls[0].sample[1 + mNumOutputs];
	memset( mPinsSamples, 0, sizeof(mPinsSamples) );
*/
	// Reset();

	if ( (unsigned long)mMbox.virt_addr & ( PAGE_SIZE - 1 ) ) {
		fatal("pi-blaster: Virtual address is not page aligned\n");
	}

	/* we are done with the mbox */
	close( mMbox.handle );
	mMbox.handle = -1;


	dma_ctl_t* ctl = &mCtls[0];
	dma_cb_t* cbp = ctl->cb;
	uint32_t phys_fifo_addr;
	uint32_t phys_gpclr0 = GPIO_PHYS_BASE + 0x28;
	uint32_t phys_gpset0 = GPIO_PHYS_BASE + 0x1c;
	uint32_t i;

	phys_fifo_addr = PWM0_PHYS_BASE + 0x18;
	uint32_t phys_dat1_addr = PWM0_PHYS_BASE + 0x14;
	uint32_t phys_dat2_addr = PWM0_PHYS_BASE + 0x24;

	uint32_t pwm1_phys_fifo_addr = PWM1_PHYS_BASE + 0x18;
	uint32_t pwm1_phys_dat1_addr = PWM1_PHYS_BASE + 0x14;
	uint32_t pwm1_phys_dat2_addr = PWM1_PHYS_BASE + 0x24;

/*
	TODO : Multiplexer PWM output
	: PWM0_0 as output
	: PWM1_0 as DMA timer ?
	: pins A, B, C, D to control external multiplexer
	DMA commands buffer:
		→ reset switch pins (phys_gpclr0)
		→ loop over all channels
			→ set switch pins to pin X high (phys_gpset0)
			→ TBD : wait for switch to physically rise ? (GPIO rise time + external multiplexer IC rise time) (Use builtin DMA_WAIT_RESP, and just run same command 2~3× to wait IC ?)
			→ write to PWM0_0 FIFO channel X value (TBD)											\
			→ use PWM1_0 to wait for PWM0_0 to output ? TBD : just wait for FIFO end trigger ?		| → Reuse "// Timer trigger command" for both in one DMA command block (no need for PWM1_0)
			→ reset switch pins (phys_gpclr0)
			→ TBD : wait for switch to physically fall ? (GPIO fall time + external multiplexer IC fall time) (same as above)
*/

		gpio_reg[GPIO_CLR0] = 1 << 6;
		uint32_t fsel = gpio_reg[GPIO_FSEL0 + 6/10];
		fsel &= ~(7 << ((6 % 10) * 3));
		fsel |= GPIO_MODE_OUT << ((6 % 10) * 3);
		gpio_reg[GPIO_FSEL0 + 6/10] = fsel;
		gpio_reg[GPIO_SET0] = 1 << 6;

	ctl->sample[0] = (1 << 6);
	ctl->sample[1] = 0; // 3
	ctl->sample[2] = (1 << 6); // 0
	ctl->sample[3] = 0; // 1
	ctl->sample[4] = 0; // 2
	ctl->sample[5] = 100;
	ctl->sample[6] = 500;
	ctl->sample[7] = 1000;
	ctl->sample[8] = 1500;
	ctl->sample[16] = 1;
	printf( "%p, %p, %p, %p\n", ctl->sample, ctl->cb, cbp, mPWMSamples );
/*
	// GPIO-Clear All command
	cbp->info = DMA_NO_WIDE_BURSTS;
	cbp->src = mem_virt_to_phys(&ctl->sample[0]);
	cbp->dst = phys_gpclr0;
	cbp->length = 4;
	cbp->stride = 0;
	cbp->next = mem_virt_to_phys(cbp + 1);
	cbp++;
*/
	for (i = 0; i < mNumOutputs; i++) {

		// GPIO-Set command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ;
		cbp->src = mem_virt_to_phys(&ctl->sample[1 + i]);
		cbp->dst = phys_gpset0;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		// Timer wait command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | DMA_PER_MAP(5) | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(&ctl->sample[16]);
		cbp->dst = pwm1_phys_dat1_addr;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		// Timer trigger command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_D_DREQ | DMA_PER_MAP(5);
		// cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | DMA_PER_MAP(5) | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(&ctl->sample[5 + i]);
		cbp->dst = phys_dat1_addr;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		// Timer wait command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | DMA_PER_MAP(5) | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(&ctl->sample[16]);
		cbp->dst = pwm1_phys_fifo_addr;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		// GPIO-Clear All command
		cbp->info = DMA_NO_WIDE_BURSTS;
		cbp->src = mem_virt_to_phys(&ctl->sample[0]);
		cbp->dst = phys_gpclr0;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
	}
	cbp--;
	cbp->next = mem_virt_to_phys(ctl->cb);

	dprintf( "Ok\n" );



	{
		pwm_reg[PWM_CTL] = 0;
		usleep(10);

		clk_reg[PWMCLK_CNTL] = 0x5A000006; // Source=PLLD (plldfreq_mhz MHz)
		usleep(100);
		clk_reg[PWMCLK_DIV] = 0x5A000000 | hzToDivider( plldfreq_mhz, time_base );

		usleep(100);
		clk_reg[PWMCLK_CNTL] = 0x5A000016; // Source=PLLD (plldfreq_mhz MHz) and enable
		usleep(100);
		pwm_reg[PWM_RNG1] = period;
		usleep(10);
		pwm_reg[PWM_RNG2] = period;
		usleep(10);
		pwm_reg[PWM_DAT1] = 0;
		usleep(10);
		pwm_reg[PWM_DAT2] = 0;
		usleep(10);
		pwm_reg[PWM_PWMC] = 0;
		// pwm_reg[PWM_PWMC] = PWMPWMC_ENAB | PWMPWMC_THRSHLD;
		usleep(10);
		pwm_reg[PWM_CTL] = PWMCTL_CLRF;
		usleep(10);

		pwm_reg[PWM_CTL] = mPwmCtl = PWMCTL_MSEN1 | PWMCTL_MSEN2 | PWMCTL_PWEN1 | PWMCTL_PWEN2;// | PWMCTL_USEF1;
		usleep(10);
	}
	{
		pwm1_reg[PWM_CTL] = 0;
		usleep(10);

		clk_reg[PWMCLK_CNTL] = 0x5A000006; // Source=PLLD (plldfreq_mhz MHz)
		usleep(100);
		clk_reg[PWMCLK_DIV] = 0x5A000000 | hzToDivider( plldfreq_mhz, time_base );

		usleep(100);
		clk_reg[PWMCLK_CNTL] = 0x5A000016; // Source=PLLD (plldfreq_mhz MHz) and enable
		usleep(100);
		pwm1_reg[PWM_RNG1] = period * 0.001;
		usleep(10);
		pwm1_reg[PWM_RNG2] = period;
		usleep(10);
		pwm1_reg[PWM_DAT1] = 0;
		usleep(10);
		pwm1_reg[PWM_DAT2] = 0;
		usleep(10);
		// pwm1_reg[PWM_PWMC] = 0;
		usleep(10);
		pwm1_reg[PWM_PWMC] = PWMPWMC_ENAB | PWMPWMC_THRSHLD;
		usleep(10);
		pwm1_reg[PWM_CTL] = PWMCTL_CLRF;
		usleep(10);

		pwm1_reg[PWM_CTL] = mPwmCtl = PWMCTL_MSEN1 | PWMCTL_MSEN2 | PWMCTL_PWEN1 | PWMCTL_PWEN2 | PWMCTL_USEF2;
		usleep(10);
	}

	// Initialise the DMA
	dma_reg[DMA_CS] = DMA_RESET;
	usleep(10);
	dma_reg[DMA_CS] = DMA_INT | DMA_END;
	dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(ctl->cb);
	dma_reg[DMA_DEBUG] = 7; // clear debug error flags
	// dma_reg[DMA_CS] = 0x10770001;	// go, high priority, wait for outstanding writes
	dma_reg[DMA_CS] = 0x30770001;	// go, high priority, disable debug stop, wait for outstanding writes

}


PWM::MultiplexPWMChannel::~MultiplexPWMChannel() noexcept
{
}


void PWM::MultiplexPWMChannel::Reset()
{
	/*
	*mSelectPinResetMask = 0;
	memset( mSelectPinMasks, 0, sizeof(uint32_t)*mNumOutputs );
	memset( mPWMSamples, 0, sizeof(uint32_t)*mNumOutputs );
	*/
}


void PWM::MultiplexPWMChannel::SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len )
{
	(void)pin;
	(void)buffer;
	(void)len;
	// TODO
}


void PWM::MultiplexPWMChannel::SetPWMValue( uint32_t pin, uint32_t width )
{
	/*
	fDebug( pin, width );

	VirtualPin* vpin = &mVirtualPins.at(pin);

	fDebug( vpin->pwmPin, vpin->enablePin, vpin->pwmIndex );

	mPWMSamples[vpin->pwmIndex] = width;
	*/
/*
	if ( pin == vpin->pwmPin ) {
	} else if ( pin == vpin->enablePin ) {
	}


	uint32_t* ptr = mPinsSamples[pin];

	if ( ptr == nullptr ) {
		std::map< uint32_t*, bool > used;
		for ( uint32_t* ptr : mPinsSamples ) {
			if ( ptr ) {
				used.insert( std::make_pair( ptr, true ) );
			}
		}
		for ( uint32_t i = 0; i < mNumOutputs; i++ ) {
			if ( used.find( &mPWMSamples[i] ) == used.end() ) {
				mPinsSamples[pin] = ptr = &mPWMSamples[i];
				mSelectPinMasks[i] = (1 << pin);
				break;
			}
		}
		if ( ptr == nullptr ) {
			// No slot available ?
			ptr = (uint32_t*)0xDEADBEEF;
		} else {
		}
	}
	if ( ptr != (uint32_t*)0xDEADBEEF ) {
		*ptr = width;
	}
*/
}


void PWM::MultiplexPWMChannel::Update()
{
}


PWM::PWMChannel::PWMChannel( uint8_t engine, uint32_t time_base, uint32_t period, PWM::PWMMode mode, bool loop )
	: mMode( mode )
	, mLoop( loop )
{
	fDebug( (int)engine, time_base, period, mode, loop );
	mMbox.handle = mbox_open();
	if ( mMbox.handle < 0 ) {
		fatal("Failed to open mailbox\n");
	}
	uint32_t mbox_board_rev = get_board_revision( mMbox.handle );
	gDebug() << "MBox Board Revision: " << mbox_board_rev;
	get_model(mbox_board_rev);

	clk_reg = (uint32_t*)map_peripheral( CLK_BASE, CLK_LEN );
	gpio_reg = (uint32_t*)map_peripheral( GPIO_BASE, GPIO_LEN );
	if ( engine == 0 ) {
		pwm_reg = (uint32_t*)map_peripheral( PWM0_BASE, PWM0_LEN );
	} else if ( engine == 1 ) {
		pwm_reg = (uint32_t*)map_peripheral( PWM0_BASE, 0x1000 ) + ( PWM1_BASE - PWM0_BASE );
	} else {
		return;
	}

	pwm_reg[PWM_CTL] = 0;
	usleep(10);

	clk_reg[PWMCLK_CNTL] = 0x5A000006; // Source=PLLD (plldfreq_mhz MHz)
	usleep(100);
	clk_reg[PWMCLK_DIV] = 0x5A000000 | hzToDivider( plldfreq_mhz, time_base );

	usleep(100);
	clk_reg[PWMCLK_CNTL] = 0x5A000016; // Source=PLLD (plldfreq_mhz MHz) and enable
	usleep(100);
	pwm_reg[PWM_RNG1] = period;
	usleep(10);
	pwm_reg[PWM_RNG2] = period;
	usleep(10);
	pwm_reg[PWM_DAT1] = 0;
	usleep(10);
	pwm_reg[PWM_DAT2] = 0;
	usleep(10);
	pwm_reg[PWM_PWMC] = 0;
	usleep(10);
	pwm_reg[PWM_CTL] = PWMCTL_CLRF;
	usleep(10);

	pwm_reg[PWM_CTL] = mPwmCtl = PWMCTL_MSEN1 | PWMCTL_MSEN2 | ( (mode == MODE_BUFFER) ? (PWMCTL_USEF1 | PWMCTL_USEF2) : 0 );
	usleep(10);
}


PWM::PWMChannel::~PWMChannel()
{
}


void PWM::PWMChannel::SetPWMValue( uint32_t pin, uint32_t width )
{
	uint8_t chan = 0;

	if ( pin == 13 or pin == 19 or pin == 41 or pin == 45 ) {
		chan = 1;
	} else if ( pin != 12 and pin != 18 and pin != 40 ) {
		return;
	}

	if ( chan == 0 ) {
		if ( !( mPwmCtl & PWMCTL_PWEN1 ) ) {
			const uint32_t alt = ( ( pin == 18 or pin == 19 ) ? GPIO_MODE_ALT5 : GPIO_MODE_ALT0 );
			gpio_reg[GPIO_FSEL0 + pin/10] &= ~(7 << ((pin % 10) * 3));
			gpio_reg[GPIO_FSEL0 + pin/10] |= alt << ((pin % 10) * 3);
			gpio_reg[GPIO_CLR0] = 1 << pin;
			pwm_reg[PWM_CTL] = mPwmCtl |= PWMCTL_PWEN1;
			usleep(10);
		}
		pwm_reg[PWM_DAT1] = width;
	} else {
		if ( !( mPwmCtl & PWMCTL_PWEN2 ) ) {
			pwm_reg[PWM_CTL] = mPwmCtl |= PWMCTL_PWEN2;
			usleep(10);
		}
		pwm_reg[PWM_DAT2] = width;
	}
}


void PWM::PWMChannel::SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len )
{
	// TODO (used by DSHOT)
}


void PWM::PWMChannel::Update()
{
	// Nothing to do
}


void PWM::PWMChannel::Reset()
{
	pwm_reg[PWM_CTL] &= ~( PWMCTL_PWEN1 | PWMCTL_PWEN2 );
	pwm_reg[PWM_DAT1] = 0;
	pwm_reg[PWM_DAT2] = 0;
}


PWM::DMAChannel::DMAChannel( uint8_t channel, uint32_t time_base, uint32_t period_time, uint32_t sample_time, PWMMode mode, bool loop )
	: mChannel( channel )
	, mMode( mode )
	, mLoop( loop )
	, mPinsCount( 0 )
	, mPinsMask( 0 )
	, mTimeBase( time_base )
	, mCycleTime( period_time )
	, mSampleTime( sample_time )
	, mNumSamples( mCycleTime / mSampleTime )
	, mNumCBs( mNumSamples * ( 2 + ( mMode == MODE_BUFFER ) ) )
	, mNumPages( ( mNumCBs * sizeof(dma_cb_t) + ( mNumSamples * 1 + ( mMode == MODE_BUFFER ) ) * sizeof(uint32_t) + PAGE_SIZE - 1 ) >> PAGE_SHIFT )
	, mCurrentCtl( 0 )
{
	memset( mPins, 0xFF, sizeof(mPins) );
	memset( mPinsPWMf, 0, sizeof(mPinsPWMf) );
	memset( mPinsPWM, 0, sizeof(mPinsPWM) );
	memset( mPinsBuffer, 0, sizeof(mPinsBuffer) );
	memset( mPinsBufferLength, 0, sizeof(mPinsBufferLength) );

	mMbox.handle = mbox_open();
	if ( mMbox.handle < 0 ) {
		fatal("Failed to open mailbox\n");
	}

	uint32_t mbox_board_rev = get_board_revision( mMbox.handle );
	gDebug() << "MBox Board Revision: " << mbox_board_rev;
	get_model(mbox_board_rev);
	uint32_t mbox_dma_channels = get_dma_channels( mMbox.handle );
	gDebug() << "PWM Channels Info: " << mbox_dma_channels << ", using PWM Channel: " << (int)mChannel;

	gDebug() << "Number of channels:             " << (int)mPinsCount;
	gDebug() << "PWM frequency:               " << time_base / mCycleTime;
	gDebug() << "PWM steps:                      " << mNumSamples;
	gDebug() << "PWM step :                    " << mSampleTime;
	gDebug() << "Maximum period (100  %%):      " << mCycleTime;
	gDebug() << "Minimum period (" << 100.0 * mSampleTime / mCycleTime << "):      " << mSampleTime;
	gDebug() << "DMA Base:                  " << DMA_BASE;

// 	setup_sighandlers();

	// map the registers for all PWM Channels
	dma_virt_base = (uint32_t*)map_peripheral( DMA_BASE, ( DMA_CHAN_SIZE * ( DMA_CHAN_MAX + 1 ) ) );
	// set dma_reg to point to the PWM Channel we are using
	dma_reg = dma_virt_base + mChannel * ( DMA_CHAN_SIZE / sizeof(dma_reg) );
	pwm_reg = (uint32_t*)map_peripheral( PWM_BASE, PWM_LEN );
	pcm_reg = (uint32_t*)map_peripheral( PCM_BASE, PCM_LEN );
	clk_reg = (uint32_t*)map_peripheral( CLK_BASE, CLK_LEN );
	gpio_reg = (uint32_t*)map_peripheral( GPIO_BASE, GPIO_LEN );

	/* Use the mailbox interface to the VC to ask for physical memory */
	uint32_t cmd_count = 4 + ( mMode == MODE_BUFFER );// + ( mLoop == false );
	mMbox.mem_ref = mem_alloc( mMbox.handle, mNumPages * PAGE_SIZE * cmd_count, PAGE_SIZE, mem_flag );
	dprintf( "mem_ref %u\n", mMbox.mem_ref );
	mMbox.bus_addr = mem_lock( mMbox.handle, mMbox.mem_ref );
	dprintf( "bus_addr = %#x\n", mMbox.bus_addr );
	mMbox.virt_addr = (uint8_t*)mapmem( BUS_TO_PHYS(mMbox.bus_addr), mNumPages * PAGE_SIZE * cmd_count );
	dprintf( "virt_addr %p\n", mMbox.virt_addr );
	uint32_t offset = 0;
	mCtls[0].sample = (uint32_t*)( &mMbox.virt_addr[offset] );
	offset += sizeof(uint32_t) * mNumSamples * ( 1 + ( mMode == MODE_BUFFER ) );
	mCtls[0].cb = (dma_cb_t*)( &mMbox.virt_addr[offset] );
	offset += sizeof(dma_cb_t) * mNumSamples * ( 2 + ( mMode == MODE_BUFFER ) );

	if ( (unsigned long)mMbox.virt_addr & ( PAGE_SIZE - 1 ) ) {
		fatal("pi-blaster: Virtual address is not page aligned\n");
	}

	/* we are done with the mbox */
	close( mMbox.handle );
	mMbox.handle = -1;

	init_ctrl_data();
	init_hardware( time_base );
	update_pwm();
}


void PWM::DMAChannel::Reset()
{
	if ( dma_reg && mMbox.virt_addr ) {
// 		for ( i = 0; i < mPinsCount; i++ ) {
// 			mPinsPWM[i] = 0.0;
// 		}
// 		update_pwm();
// 		usleep( mCycleTime * 2 );
		dma_reg[DMA_CS] = DMA_RESET;
	}
}


PWM::DMAChannel::~DMAChannel()
{
	if ( mMbox.virt_addr != NULL ) {
		unmapmem( mMbox.virt_addr, mNumPages * PAGE_SIZE );
		if ( mMbox.handle <= 2 ) {
			mMbox.handle = mbox_open();
		}
		mem_unlock( mMbox.handle, mMbox.mem_ref );
		mem_free( mMbox.handle, mMbox.mem_ref );
// 		mbox_close( mMbox.handle );
	}
}


void PWM::terminate( int sig )
{
	fDebug();

	if ( mChannels.size() == 0 ) {
		// Not needed
		return;
	}

	size_t i, j;
	printf( "pi-blaster::terminate( %d )\n", sig );

	void* array[16];
	size_t size;
	size = backtrace( array, 16 );
	fprintf( stderr, "Error: signal %d :\n", sig );
	char** trace = backtrace_symbols( array, size );

	dprintf( "Resetting PWM/DMA (%u)...\n", (uint32_t)mChannels.size() );
	for ( j = 0; j < mChannels.size(); j++ ) {
		if ( mChannels[j] ) {
			mChannels[j]->Reset();
			usleep(10);
		}
	}

	dprintf("Freeing mbox memory...\n");
	for ( j = 0; j < mChannels.size(); j++ ) {
		if ( mChannels[j] ) {
			delete mChannels[j];
			mChannels[j] = nullptr;
		}
	}

	mChannels.clear();

	dprintf("Unlink %s...\n", DEVFILE_MBOX);
	unlink(DEVFILE_MBOX);
}


void PWM::DMAChannel::SetPWMValue( uint32_t pin, uint32_t width )
{
	bool found = false;
	uint32_t i = 0;
	for ( i = 0; i < mPinsCount; i++ ) {
		if ( mPins[i] == (int8_t)pin ) {
			found = true;
			break;
		}
	}
	if ( not found ) {
		i = mPinsCount;
		mPinsCount += 1;
		mPins[i] = pin;
		mPinsMask = mPinsMask | ( 1 << pin );

		GPIO::setPUD( pin, GPIO::PullDown );
		GPIO::setMode( pin, GPIO::Output );

		gpio_reg[GPIO_CLR0] = 1 << pin;
		uint32_t fsel = gpio_reg[GPIO_FSEL0 + pin/10];
		fsel &= ~(7 << ((pin % 10) * 3));
		fsel |= GPIO_MODE_OUT << ((pin % 10) * 3);
		gpio_reg[GPIO_FSEL0 + pin/10] = fsel;
	}

	mPinsPWMf[i] = (float)width / (float)mCycleTime;
	mPinsPWM[i] = width * mNumSamples / mCycleTime;
}


void PWM::DMAChannel::SetPWMBuffer( uint32_t pin, uint8_t* buffer, uint32_t len )
{
	bool found = false;
	uint32_t i = 0;
	for ( i = 0; i < mPinsCount; i++ ) {
		if ( mPins[i] == (int8_t)pin ) {
			found = true;
			break;
		}
	}
	if ( not found ) {
		i = mPinsCount;
		mPinsCount += 1;
		mPins[i] = pin;
		mPinsMask = mPinsMask | ( 1 << pin );

		gpio_reg[GPIO_CLR0] = 1 << pin;
		uint32_t fsel = gpio_reg[GPIO_FSEL0 + pin/10];
		fsel &= ~(7 << ((pin % 10) * 3));
		fsel |= GPIO_MODE_OUT << ((pin % 10) * 3);
		gpio_reg[GPIO_FSEL0 + pin/10] = fsel;
	}

	if ( mPinsBuffer[i] and mPinsBufferLength[i] < len ) {
		delete mPinsBuffer[i];
		mPinsBuffer[i] = nullptr;
	}
	if ( not mPinsBuffer[i] ) {
		mPinsBuffer[i] = new uint8_t[ len ];
	}
	memcpy( mPinsBuffer[i], buffer, len );
	mPinsBufferLength[i] = len;
}


void PWM::DMAChannel::Update()
{
	if ( mMode == MODE_PWM ) {
		update_pwm();
	} else if ( mMode == MODE_BUFFER ) {
		update_pwm_buffer();
	}
}


void PWM::DMAChannel::update_pwm()
{
	uint32_t i, j;
// 	uint32_t cmd_count = 2;// + ( mLoop == false );
	uint32_t phys_gpclr0 = GPIO_PHYS_BASE + 0x28;
	uint32_t phys_gpset0 = GPIO_PHYS_BASE + 0x1c;
	uint32_t phys_gplev0 = GPIO_PHYS_BASE + 0x34;
	uint32_t mask;
	dma_ctl_t ctl = mCtls[0];
	if ( mLoop ) {
// 		ctl = mCtls[ ( mCurrentCtl + 1 ) % 2 ];
	}

	// Wait until CMA_ACTIVE bit is false to prevent overlapping writes
	while ( mLoop == false and ( dma_reg[DMA_CS] & 1 ) ) {
		usleep(0);
	}

	ctl.cb[0].dst = phys_gpset0;
	mask = 0;
	for ( i = 0; i <= mPinsCount; i++ ) {
		if ( mPinsPWM[i] > 0 ) {
			mask |= 1 << mPins[i];
		}
	}
	ctl.sample[0] = mask;

	for ( j = 1; j < mNumSamples; j++ ) {
		ctl.cb[j*2].dst = phys_gpclr0;
		mask = 0;
		for ( i = 0; i <= mPinsCount; i++ ) {
			if ( mPins[i] >= 0 and j > mPinsPWM[i] ) {
				mask |= 1 << mPins[i];
			}
		}
		ctl.sample[j] = mask;
	}

	if ( mLoop == false ) {
		dma_reg[DMA_CS] = DMA_INT | DMA_END;
		dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(mCtls[0].cb);
		dma_reg[DMA_DEBUG] = 7; // clear debug error flags
		dma_reg[DMA_CS] = 0x10770001;	// go, high priority, wait for outstanding writes
	}
}


void PWM::DMAChannel::update_pwm_buffer()
{
// 	uint32_t cmd_count = 3;// + ( mLoop == false );
// 	uint32_t phys_gpclr0 = GPIO_PHYS_BASE + 0x28;
// 	uint32_t phys_gpset0 = GPIO_PHYS_BASE + 0x1c;
	uint32_t mask;
	dma_ctl_t ctl = mCtls[0];

	// Wait until CMA_ACTIVE bit is false to prevent overlapping writes
	while ( mLoop == false and ( dma_reg[DMA_CS] & 1 ) ) {
		usleep(0);
	}

	uint32_t i, j;
	for ( j = 0; j < mNumSamples; j++ ) {
		// Clear bits
		{
			mask = 0;
			for ( i = 0; i <= mPinsCount; i++ ) {
				if ( mPins[i] >= 0 and mPinsBufferLength[i] < j and mPinsBuffer[i][j] == 0 ) {
					mask |= 1 << mPins[i];
				}
			}
			ctl.sample[j] = mask;
		}
		// Set bits
		{
			mask = 0;
			for ( i = 0; i <= mPinsCount; i++ ) {
				if ( mPins[i] >= 0 and mPinsBufferLength[i] < j and mPinsBuffer[i][j] == 1 ) {
					mask |= 1 << mPins[i];
				}
			}
			ctl.sample[mNumSamples + j] = mask;
		}
	}

	if ( mLoop == false ) {
// 		mCurrentCtl = ( mCurrentCtl + 1 ) % 2;
		dma_reg[DMA_CS] = DMA_INT | DMA_END;
		dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(mCtls[0].cb);
		dma_reg[DMA_DEBUG] = 7; // clear debug error flags
		dma_reg[DMA_CS] = 0x10770001;	// go, high priority, wait for outstanding writes
	}
}


void PWM::DMAChannel::init_ctrl_data()
{
	dprintf( "Initializing PWM ...\n" );
	init_dma_ctl( &mCtls[0] );
// 	init_dma_ctl( &mCtls[1] );
	dprintf( "Ok\n" );
}


/*
	TODO : Multiplexer PWM output
	: PWM0_0 as output
	: PWM1_0 as DMA timer ?
	: pins A, B, C, D to control external multiplexer
	DMA commands buffer:
		→ reset switch pins (phys_gpclr0)
		→ loop over all channels
			→ set switch pins to pin X high (phys_gpset0)
			→ TBD : wait for switch to physically rise ? (GPIO rise time + external multiplexer IC rise time) (Use builtin DMA_WAIT_RESP, and just run same command 2~3× to wait IC ?)
			→ write to PWM0_0 FIFO channel X value (TBD)											\
			→ use PWM1_0 to wait for PWM0_0 to output ? TBD : just wait for FIFO end trigger ?		| → Reuse "// Timer trigger command" for both in one DMA command block (no need for PWM1_0)
			→ reset switch pins (phys_gpclr0)
			→ TBD : wait for switch to physically fall ? (GPIO fall time + external multiplexer IC fall time) (same as above)
*/


void PWM::DMAChannel::init_dma_ctl( dma_ctl_t* ctl )
{
	dma_cb_t* cbp = ctl->cb;
	uint32_t phys_fifo_addr;
	uint32_t phys_gpclr0 = GPIO_PHYS_BASE + 0x28;
	uint32_t phys_gpset0 = GPIO_PHYS_BASE + 0x1c;
	uint32_t phys_gplev0 = GPIO_PHYS_BASE + 0x34;
	uint32_t mask;
	uint32_t i;

	uint32_t map = DMA_PER_MAP(5);
	phys_fifo_addr = PWM_PHYS_BASE + 0x18;
// 	uint32_t map = DMA_PER_MAP(2);
// 	phys_fifo_addr = PCM_PHYS_BASE + 0x04;
	memset( ctl->sample, 0, sizeof(uint32_t)*mNumSamples );

	mask = 0;
	// Calculate a mask to turn off everything
	if ( mMode == MODE_BUFFER ) {
		// Clear bits
		{
			for ( i = 0; i <= mPinsCount; i++ ) {
				mask |= 1 << mPins[i];
			}
			for ( i = 0; i < mNumSamples; i++ ) {
				ctl->sample[i] = mask;
			}
		}
		// Set bits
		for ( i = 0; i < mNumSamples; i++ ) {
			ctl->sample[mNumSamples + i] = 0;
		}
	} else if ( mMode == MODE_PWM ) {
		for ( i = 0; i < mPinsCount; i++ ) {
			mask |= 1 << mPins[i];
		}
		for ( i = 0; i < mNumSamples; i++ ) {
			ctl->sample[i] = mask;
		}
	}

	/* Initialize all the PWM commands. They come in pairs.
	 *  - 1st command copies a value from the sample memory to a destination
	 *    address which can be either the gpclr0 register or the gpset0 register
	 *  - 2nd command waits for a trigger from an PWM source
	 *  - 3rd command (optional) resets pins to 0
	 */
	for (i = 0; i < mNumSamples; i++) {
		// GPIO-Clear command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(ctl->sample + i);
		cbp->dst = phys_gpclr0;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
		if ( mMode == MODE_BUFFER ) {
			// GPIO-Set command
// 			cbp->info = DMA_NO_WIDE_BURSTS;
			cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
			cbp->src = mem_virt_to_phys(ctl->sample + mNumSamples + i);
			cbp->dst = phys_gpset0;
			cbp->length = 4;
			cbp->stride = 0;
			cbp->next = mem_virt_to_phys(cbp + 1);
			cbp++;
		}
		// Timer trigger command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | map;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_D_DREQ | DMA_PER_MAP(5) | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(ctl->sample);	// Any data will do
		cbp->dst = phys_fifo_addr;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
/*
		if ( mLoop == false ) {
			// Third PWM command
			cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
			cbp->src = mem_virt_to_phys(mPinsMask);
			cbp->dst = phys_gpclr0;
			cbp->length = 4;
			cbp->stride = 0;
			cbp->next = mem_virt_to_phys(cbp + 1);
			cbp++;
		}
*/
	}

	cbp--;
	if ( mLoop ) {
		cbp->next = mem_virt_to_phys(ctl->cb);
	} else {
		cbp->next = 0;
	}

	ctl->cb[0].dst = phys_gpset0;
	dprintf( "Ok\n" );
}

static bool _init_ok = false;

void PWM::DMAChannel::init_hardware( uint32_t time_base )
{
	if ( _init_ok ) {
		return;
	}
	_init_ok = true;

	dprintf("Initializing PWM HW...\n");
	// Initialise PWM
	pwm_reg[PWM_CTL] = 0;
	usleep(10);
	clk_reg[PWMCLK_CNTL] = 0x5A000006; // Source=PLLD (500MHz)
	usleep(100);
	clk_reg[PWMCLK_DIV] = 0x5A000000 | ( ( 500000000 / time_base ) << 12 ); // setting pwm div to 500 gives 1MHz
	// clk_reg[PWMCLK_DIV] = 0x5A000000 | hzToDivider( plldfreq_mhz, time_base );

	usleep(100);
	clk_reg[PWMCLK_CNTL] = 0x5A000016;		// Source=PLLD and enable
	usleep(100);
	pwm_reg[PWM_RNG1] = ( mSampleTime - 1 );
	usleep(10);
	pwm_reg[PWM_PWMC] = PWMPWMC_ENAB | PWMPWMC_THRSHLD;
	usleep(10);
	pwm_reg[PWM_CTL] = PWMCTL_CLRF;
	usleep(10);
	pwm_reg[PWM_CTL] = PWMCTL_USEF1 | PWMCTL_PWEN1;
	usleep(10);


/*
	dprintf("Initializing PCM HW...\n");
	// Initialise PCM
	pcm_reg[PCM_CS_A] = 1;				// Disable Rx+Tx, Enable PCM block
	usleep(100);
	clk_reg[PCMCLK_CNTL] = 0x5A000006;		// Source=PLLD (500MHz)
	usleep(100);
	clk_reg[PCMCLK_DIV] = 0x5A000000 | ( ( 500000000 / time_base ) << 12 );
	usleep(100);
	clk_reg[PCMCLK_CNTL] = 0x5A000016;		// Source=PLLD and enable
	usleep(100);
	pcm_reg[PCM_TXC_A] = 0<<31 | 1<<30 | 0<<20 | 0<<16; // 1 channel, 8 bits
	usleep(100);
	pcm_reg[PCM_MODE_A] = (mSampleTime - 1) << 10;
	usleep(100);
	pcm_reg[PCM_CS_A] |= 1<<4 | 1<<3;		// Clear FIFOs
	usleep(100);
	pcm_reg[PCM_DREQ_A] = 64<<24 | 64<<8;		// DMA Req when one slot is free?
	usleep(100);
	pcm_reg[PCM_CS_A] |= 1<<9; // Enable DMA
*/

	// Initialise the DMA
	dma_reg[DMA_CS] = DMA_RESET;
	usleep(10);
	dma_reg[DMA_CS] = DMA_INT | DMA_END;
	dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(mCtls[0].cb);
	dma_reg[DMA_DEBUG] = 7; // clear debug error flags
	dma_reg[DMA_CS] = 0x10770001;	// go, high priority, wait for outstanding writes

// 	pcm_reg[PCM_CS_A] |= 1<<2;			// Enable Tx
	dprintf( "Ok\n" );
}


int PWM::Channel::mbox_open()
{
	int file_desc;

	// try to use /dev/vcio first (kernel 4.1+)
	file_desc = open( DEVFILE_VCIO, 0 );
	if ( file_desc < 0 ) {
		unlink( DEVFILE_MBOX );
		if ( mknod( DEVFILE_MBOX, S_IFCHR | 0600, makedev( MAJOR_NUM, 0 ) ) < 0 ) {
			fatal("Failed to create mailbox device\n");
		}
		file_desc = open( DEVFILE_MBOX, 0 );
		if ( file_desc < 0 ) {
			printf( "Can't open device file: %s\n", DEVFILE_MBOX );
			perror( NULL );
			exit( -1 );
		}
	}
	return file_desc;
}


uint32_t PWM::Channel::mem_virt_to_phys( void* virt )
{
	uintptr_t offset = (uintptr_t)virt - (uintptr_t)mMbox.virt_addr;
	return mMbox.bus_addr + offset;
}


void PWM::Channel::get_model( unsigned mbox_board_rev )
{
	int board_model = 0;

	if ( ( mbox_board_rev & BOARD_REVISION_SCHEME_MASK ) == BOARD_REVISION_SCHEME_NEW ) {
		if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI2_B) {
			board_model = 2;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI3_B) {
			board_model = 3;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI3_BP) {
			board_model = 3;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI3_AP) {
			board_model = 3;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_CM3) {
			board_model = 3;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI4_B) {
			board_model = 4;
		} else if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_CM4) {
			board_model = 4;
		} else {
			// no Pi 2, we assume a Pi 1
			board_model = 1;
		}
	} else {
		// if revision scheme is old, we assume a Pi 1
		board_model = 1;
	}

	gDebug() << "board_model : " << board_model;

	plldfreq_mhz = 500;
	switch ( board_model ) {
		case 1:
			periph_virt_base = 0x20000000;
			periph_phys_base = 0x7e000000;
			mem_flag         = MEM_FLAG_L1_NONALLOCATING | MEM_FLAG_ZERO;
			break;
		case 2:
		case 3:
			periph_virt_base = 0x3f000000;
			periph_phys_base = 0x7e000000;
			mem_flag         = MEM_FLAG_L1_NONALLOCATING | MEM_FLAG_ZERO;
			break;
		case 4:
			periph_virt_base = 0xfe000000;
			periph_phys_base = 0x7e000000;
			plldfreq_mhz = 750;
		//	max chan: 7
			mem_flag         = MEM_FLAG_L1_NONALLOCATING | MEM_FLAG_ZERO;
			break;
		default:
			fatal( "Unable to detect Board Model from board revision: %#x", mbox_board_rev );
			break;
	}
}

void* PWM::Channel::map_peripheral( uint32_t base, uint32_t len )
{
	int fd = open( "/dev/mem", O_RDWR | O_SYNC );
	void * vaddr;

	if ( fd < 0 ) {
		fatal( "pi-blaster: Failed to open /dev/mem: %m\n" );
	}
	vaddr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
	if ( vaddr == MAP_FAILED ) {
		fatal("pi-blaster: Failed to map peripheral at 0x%08x: %m\n", base );
	}
	close( fd );

	return vaddr;
}
