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

#ifdef BUILD_DShot

#if 0

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE   700

#include <execinfo.h>
#include <signal.h>

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
#include "DShot.h"
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
#define PWM_BASE_OFFSET 0x0020C000
#define PWM_BASE		(periph_virt_base + PWM_BASE_OFFSET)
#define PWM_PHYS_BASE	(periph_phys_base + PWM_BASE_OFFSET)
#define PWM_LEN			0x28
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


#define DMA_SRC_IGNORE	(1<<11)
#define DMA_NO_WIDE_BURSTS	(1<<26)
#define DMA_WAIT_RESP		(1<<3)
#define DMA_TDMODE		(1<<1)
#define DMA_D_DREQ		(1<<6)
#define DMA_PER_MAP(x)		((x)<<16)
#define DMA_END			(1<<1)
#define DMA_RESET		(1<<31)
#define DMA_INT			(1<<2)

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

#define GPIO_MODE_IN		0
#define GPIO_MODE_OUT		1

#define PWM_CTL			(0x00/4)
#define PWM_PWMC		(0x08/4)
#define PWM_RNG1		(0x10/4)
#define PWM_FIFO		(0x18/4)

#define PWMCLK_CNTL		40
#define PWMCLK_DIV		41

#define PWMCTL_MODE1		(1<<1)
#define PWMCTL_PWEN1		(1<<0)
#define PWMCTL_CLRF		(1<<6)
#define PWMCTL_USEF1		(1<<5)

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
#define BOARD_REVISION_TYPE_PI3_B (8 << 4)
#define BOARD_REVISION_TYPE_ALPHA (5 << 4)
#define BOARD_REVISION_TYPE_CM (6 << 4)
#define BOARD_REVISION_REV_MASK (0xF)

#define LENGTH(x)  (sizeof(x) / sizeof(x[0]))

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define DEBUG 1
#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

map< uint8_t, uint16_t > DShot::mPins = map< uint8_t, uint16_t >();
bool DShot::mInitialized = false;
uint32_t DShot::mNumSamples = 0;
uint32_t DShot::mNumCBs = 0;
uint32_t DShot::mNumPages = 0;
uint32_t DShot::periph_virt_base = 0;
uint32_t DShot::periph_phys_base = 0;
uint32_t DShot::mem_flag = 0;
volatile uint32_t* DShot::pcm_reg = 0;
volatile uint32_t* DShot::pwm_reg = 0;
volatile uint32_t* DShot::clk_reg = 0;
volatile uint32_t* DShot::dma_virt_base = 0; /* base address of all PWM Channels */
volatile uint32_t* DShot::dma_reg = 0; /* pointer to the PWM Channel registers we are using */
volatile uint32_t* DShot::gpio_reg = 0;
DShot::Mbox DShot::mMbox = {  };
DShot::dma_ctl_t DShot::mCtls[2] = {};

static void fatal( const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
	exit(0);
}


DShot::DShot( uint8_t pin )
	: mPin( pin )
{
	if ( not mInitialized ) {
		mInitialized = true;
// 		static int sig_list[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 15, 16, 19, 20 };
		static int sig_list[] = { 2 };
		uint32_t i;
		for ( i = 0; i <= sizeof(sig_list)/sizeof(int); i++ ) {
			struct sigaction sa;
			memset( &sa, 0, sizeof(sa) );
			sa.sa_handler = &DShot::terminate_dma;
			sigaction( sig_list[i], &sa, NULL );
		}

		static const uint8_t channel = 6;
		mNumSamples = 16 * 3 * 2;
		mNumCBs = mNumSamples * 2;
		mNumPages = ( mNumCBs * sizeof(dma_cb_t) + mNumSamples * sizeof(uint32_t) + PAGE_SIZE - 1 ) >> PAGE_SHIFT;

		mMbox.handle = mbox_open();
		if ( mMbox.handle < 0 ) {
			fatal("Failed to open mailbox\n");
		}

		uint32_t mbox_board_rev = get_board_revision( mMbox.handle );
		gDebug() << "MBox Board Revision: " << mbox_board_rev;
		get_model( mbox_board_rev) ;
		uint32_t mbox_dma_channels = get_dma_channels( mMbox.handle );
		gDebug() << "DSHOT Channels Info: " << mbox_dma_channels << ", using PWM Channel: " << (int)channel;
		gDebug() << "DMA Base:                  " << DMA_BASE;

	// 	setup_sighandlers();

		/* map the registers for all PWM Channels */
		dma_virt_base = (uint32_t*)map_peripheral( DMA_BASE, ( DMA_CHAN_SIZE * ( DMA_CHAN_MAX + 1 ) ) );
		/* set dma_reg to point to the PWM Channel we are using */
		dma_reg = dma_virt_base + channel * ( DMA_CHAN_SIZE / sizeof(dma_reg) );
		pwm_reg = (uint32_t*)map_peripheral( PWM_BASE, PWM_LEN );
		pcm_reg = (uint32_t*)map_peripheral( PCM_BASE, PCM_LEN );
		clk_reg = (uint32_t*)map_peripheral( CLK_BASE, CLK_LEN );
		gpio_reg = (uint32_t*)map_peripheral( GPIO_BASE, GPIO_LEN );

		gpio_reg[GPIO_CLR0] = 1 << pin;
		uint32_t fsel = gpio_reg[GPIO_FSEL0 + pin/10];
		fsel &= ~(7 << ((pin % 10) * 3));
		fsel |= GPIO_MODE_OUT << ((pin % 10) * 3);
		gpio_reg[GPIO_FSEL0 + pin/10] = fsel;

		/* Use the mailbox interface to the VC to ask for physical memory */
		uint32_t cmd_count = 4;
		mMbox.mem_ref = mem_alloc( mMbox.handle, mNumPages * PAGE_SIZE * cmd_count, PAGE_SIZE, mem_flag );
		dprintf( "mem_ref %u\n", mMbox.mem_ref );
		mMbox.bus_addr = mem_lock( mMbox.handle, mMbox.mem_ref );
		dprintf( "bus_addr = %#x\n", mMbox.bus_addr );
		mMbox.virt_addr = (uint8_t*)mapmem( BUS_TO_PHYS(mMbox.bus_addr), mNumPages * PAGE_SIZE * cmd_count );
		dprintf( "virt_addr %p\n", mMbox.virt_addr );
		uint32_t offset = 0;
		mCtls[0].sample = (uint32_t*)( &mMbox.virt_addr[offset] );
		offset += sizeof(uint32_t) * mNumSamples;
		mCtls[0].cb = (dma_cb_t*)( &mMbox.virt_addr[offset] );
		offset += sizeof(dma_cb_t) * mNumSamples * 2;

		if ( (unsigned long)mMbox.virt_addr & ( PAGE_SIZE - 1 ) ) {
			fatal("pi-blaster: Virtual address is not page aligned\n");
		}

		/* we are done with the mbox */
		close( mMbox.handle );
		mMbox.handle = -1;

		init_ctrl_data();

		dprintf("Initializing PWM HW...\n");
/*
		// Initialise PWM
		pwm_reg[PWM_CTL] = 0;
		usleep(10);
		clk_reg[PWMCLK_CNTL] = 0x5A000006; // Source=PLLD (500MHz)
		usleep(100);
		clk_reg[PWMCLK_DIV] = 0x5A000000 | ( 550 << 12 );

		usleep(100);
		clk_reg[PWMCLK_CNTL] = 0x5A000016; // Source=PLLD and enable
		usleep(100);
		pwm_reg[PWM_RNG1] = 1;
		usleep(10);
		pwm_reg[PWM_PWMC] = PWMPWMC_ENAB | PWMPWMC_THRSHLD;
		usleep(10);
		pwm_reg[PWM_CTL] = PWMCTL_CLRF;
		usleep(10);
		pwm_reg[PWM_CTL] = PWMCTL_USEF1 | PWMCTL_PWEN1;
		usleep(10);
*/

		// Initialise PCM
		pcm_reg[PCM_CS_A] = 1;				// Disable Rx+Tx, Enable PCM block
		usleep(100);
		clk_reg[PCMCLK_CNTL] = 0x5A000006;		// Source=PLLD (500MHz)
		usleep(100);
		clk_reg[PCMCLK_DIV] = 0x5A000000 | (55<<12); // DSHOT150:55, DSHOT300:27
		usleep(100);
		clk_reg[PCMCLK_CNTL] = 0x5A000016;		// Source=PLLD and enable
		usleep(100);
		pcm_reg[PCM_TXC_A] = 0<<31 | 1<<30 | 0<<20 | 0<<16; // 1 channel, 8 bits
		usleep(100);
		pcm_reg[PCM_MODE_A] = (20 - 1) << 10; // 2=2x10 =>  10 with 55 PCMCLK_DIV is way better than 1 with 550 PCMCLK_DIV, 2 is to get proper value.. ^^"
		usleep(100);
		pcm_reg[PCM_CS_A] |= 1<<4 | 1<<3;		// Clear FIFOs
		usleep(100);
		pcm_reg[PCM_DREQ_A] = 64<<24 | 64<<8;		// DMA Req when one slot is free?
		usleep(100);
		pcm_reg[PCM_CS_A] |= 1<<9; // Enable DMA
	

		// Initialise the DMA
		dma_reg[DMA_CS] = DMA_RESET;
		usleep(10);
		dma_reg[DMA_CS] = DMA_INT | DMA_END;
		dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(mCtls[0].cb);
		dma_reg[DMA_DEBUG] = 7; // clear debug error flags
		dma_reg[DMA_CS] = 0x10770001;	// go, high priority, wait for outstanding writes

		pcm_reg[PCM_CS_A] |= 1<<2;			// Enable Tx
		dprintf( "Ok\n" );
	}

	GPIO::setPUD( pin, GPIO::PullDown );
	GPIO::setMode( pin, GPIO::Output );
	gpio_reg[GPIO_CLR0] = 1 << pin;
	uint32_t fsel = gpio_reg[GPIO_FSEL0 + pin/10];
	fsel &= ~(7 << ((pin % 10) * 3));
	fsel |= GPIO_MODE_OUT << ((pin % 10) * 3);
	gpio_reg[GPIO_FSEL0 + pin/10] = fsel;


	mPins.emplace( make_pair( mPin, 0 ) );



	uint32_t mask = 0;
	for ( auto pin : mPins ) {
		mask |= 1 << pin.first;
	}

	uint32_t i;
	int test = 0;
	for (i = 0; i < 16 * 3; i++) {
		if ( i % 3 == 0 ) {
			test++;
		}
		if ( i % 3 == 1 and test % 2 == 0 ) {
			mCtls[0].sample[i] = 0;
		} else {
			mCtls[0].sample[i] = mask;
		}
	}

	mCtls[0].sample[i] = mask;
}


DShot::~DShot()
{
}


void DShot::Disable()
{
}


void DShot::Disarm()
{
}


void DShot::setSpeedRaw( float speed, bool force_hw_update )
{
}


void DShot::terminate_dma( int sig )
{
	size_t i, j;
	printf( "pi-blaster::terminate( %d )\n", sig );

	void* array[16];
	size_t size;
	size = backtrace( array, 16 );
	fprintf( stderr, "Error: signal %d :\n", sig );
	char** trace = backtrace_symbols( array, size );

	dprintf( "Resetting DMA...\n" );
	if ( dma_reg && mMbox.virt_addr ) {
		dma_reg[DMA_CS] = DMA_RESET;
		usleep(10);
	}

	dprintf("Freeing mbox memory...\n");
	if ( mMbox.virt_addr != NULL ) {
		unmapmem( mMbox.virt_addr, mNumPages * PAGE_SIZE );
		if ( mMbox.handle <= 2 ) {
			mMbox.handle = mbox_open();
		}
		mem_unlock( mMbox.handle, mMbox.mem_ref );
		mem_free( mMbox.handle, mMbox.mem_ref );
// 		mbox_close( mMbox.handle );
	}

	dprintf( "Unlink %s...\n", DEVFILE_MBOX );
	unlink( DEVFILE_MBOX );

	if ( sig == 2 ) {
		exit(0);
	}
}

void DShot::init_ctrl_data()
{
	dprintf( "Initializing PWM ...\n" );
	init_dma_ctl( &mCtls[0] );
// 	init_dma_ctl( &mCtls[1] );
	dprintf( "Ok\n" );
}


void DShot::init_dma_ctl( dma_ctl_t* ctl )
{
	dma_cb_t* cbp = ctl->cb;
	uint32_t phys_fifo_addr;
	uint32_t phys_gpclr0 = GPIO_PHYS_BASE + 0x28;
	uint32_t phys_gpset0 = GPIO_PHYS_BASE + 0x1c;
	uint32_t i;

// 	uint32_t map = DMA_PER_MAP(5);
// 	phys_fifo_addr = PWM_PHYS_BASE + 0x18;
	uint32_t map = DMA_PER_MAP(2);
	phys_fifo_addr = PCM_PHYS_BASE + 0x04;

	memset( ctl->sample, 0, sizeof(uint32_t)*mNumSamples );

	/* Each bit is composed of 6 control blocks :
	 *  - GPIO set high
	 *  - timer wait
	 *  - GPIO set low/high depending on value
	 *  - timer wait
	 *  - GPIO set low
	 *  - timer wait
	 */
	for (i = 0; i < 16 * 3; i++) {
		// GPIO-Set/Clear command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(ctl->sample + i);
		if ( i % 3 == 0 ) {
			cbp->dst = phys_gpset0;
		} else {
			cbp->dst = phys_gpclr0;
		}
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
		// Timer trigger command
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | map;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_D_DREQ | map | DMA_TDMODE | DMA_SRC_IGNORE;
		cbp->src = mem_virt_to_phys(ctl->sample);	// Any data will do
		cbp->dst = phys_fifo_addr;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
	}

	// Write a long LOW pulse to terminate current command
	{
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(ctl->sample + i);
		cbp->dst = phys_gpclr0;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | map;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_D_DREQ | map | DMA_TDMODE | DMA_SRC_IGNORE;
		cbp->src = mem_virt_to_phys(ctl->sample);	// Any data will do
		cbp->dst = phys_fifo_addr;
		cbp->length = 20;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;
	}

	// Fence to prevent bugs caused by previous long pulse
	{
		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_TDMODE;
		cbp->src = mem_virt_to_phys(ctl->sample + i);
		cbp->dst = phys_gpclr0;
		cbp->length = 4;
		cbp->stride = 0;
		cbp->next = mem_virt_to_phys(cbp + 1);
		cbp++;

		cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_D_DREQ | map;
// 		cbp->info = DMA_NO_WIDE_BURSTS | DMA_D_DREQ | map | DMA_TDMODE | DMA_SRC_IGNORE;
		cbp->src = mem_virt_to_phys(ctl->sample);	// Any data will do
		cbp->dst = phys_fifo_addr;
		cbp->length = 4;
		cbp->stride = 0;
	}

	cbp->next = mem_virt_to_phys(ctl->cb); // loop back to first control block
	ctl->cb[0].dst = phys_gpset0;
	dprintf( "Ok\n" );
}


int DShot::mbox_open()
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


uint32_t DShot::mem_virt_to_phys( void* virt )
{
	uint32_t offset = (uint8_t*)virt - mMbox.virt_addr;
	return mMbox.bus_addr + offset;
}


void DShot::get_model( unsigned mbox_board_rev )
{
	int board_model = 0;

	if ( ( mbox_board_rev & BOARD_REVISION_SCHEME_MASK ) == BOARD_REVISION_SCHEME_NEW ) {
		if ( ( mbox_board_rev & BOARD_REVISION_TYPE_MASK ) == BOARD_REVISION_TYPE_PI2_B ) {
			board_model = 2;
		} else if ( ( mbox_board_rev & BOARD_REVISION_TYPE_MASK ) == BOARD_REVISION_TYPE_PI3_B ) {
			board_model = 3;
		} else {
			// no Pi 2 nor Pi 3, we assume a Pi 1
			board_model = 1;
		}
	} else {
		// if revision scheme is old, we assume a Pi 1
		board_model = 1;
	}

	gDebug() << "board_model : " << board_model;

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
		default:
			fatal( "Unable to detect Board Model from board revision: %#x", mbox_board_rev );
			break;
	}
}

void* DShot::map_peripheral( uint32_t base, uint32_t len )
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

#endif // 0

#endif // BUILD_DShot
