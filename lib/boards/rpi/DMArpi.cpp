#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "mailbox.h"
#include "DMArpi.h"
#include "Debug.h"

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

std::map< uint32_t, DMArpi* > DMArpi::mUsedChannels;

DMArpi::DMArpi( uint32_t commands_count )
{
	for ( mChannel = DMA_CHAN_MAX - 1; mChannel > 0; mChannel-- ) {
		if ( mUsedChannels.find(mChannel) == mUsedChannels.end() ) {
			break;
		}
	}

	mMbox.handle = mbox_open();
	if ( mMbox.handle < 0 ) {
		gError() << "Failed to open mailbox";
		exit(1);
	}

	uint32_t mbox_board_rev = get_board_revision( mMbox.handle );
	gDebug() << "MBox Board Revision : " << mbox_board_rev;
	get_model( mbox_board_rev );
	uint32_t mbox_dma_channels = get_dma_channels( mMbox.handle );
	gDebug() << "PWM Channels Info: " << mbox_dma_channels << ", using PWM Channel: " << (int)mChannel;
	gDebug() << "DMA Base : " << DMA_BASE;

	dma_virt_base = (uint32_t*)map_peripheral( DMA_BASE, ( DMA_CHAN_SIZE * ( DMA_CHAN_MAX + 1 ) ) );
	dma_reg = dma_virt_base + mChannel * ( DMA_CHAN_SIZE / sizeof(dma_reg) );
/*
	uint32_t numPages = ( commands_count * sizeof(dma_cb_t) + ( mNumOutputs * 1 * sizeof(uint32_t) ) + PAGE_SIZE - 1 ) >> PAGE_SHIFT;
	mMbox.mem_ref = mem_alloc( mMbox.handle, numPages * PAGE_SIZE, PAGE_SIZE, mem_flag );
	dprintf( "mem_ref %u\n", mMbox.mem_ref );
	mMbox.bus_addr = mem_lock( mMbox.handle, mMbox.mem_ref );
	dprintf( "bus_addr = %#x\n", mMbox.bus_addr );
	mMbox.virt_addr = (uint8_t*)mapmem( BUS_TO_PHYS(mMbox.bus_addr), numPages * PAGE_SIZE );
	dprintf( "virt_addr %p\n", mMbox.virt_addr );
	uint32_t offset = 0;
	mCtls[0].sample = (uint32_t*)( &mMbox.virt_addr[offset] );
	offset += sizeof(uint32_t) * mNumSamples * ( 1 + ( mMode == MODE_BUFFER ) );
	mCtls[0].cb = (dma_cb_t*)( &mMbox.virt_addr[offset] );
	offset += sizeof(dma_cb_t) * mNumSamples * ( 2 + ( mMode == MODE_BUFFER ) );

	if ( (unsigned long)mMbox.virt_addr & ( PAGE_SIZE - 1 ) ) {
		fatal("pi-blaster: Virtual address is not page aligned\n");
	}

	close( mMbox.handle );
	mMbox.handle = -1;

	init_ctrl_data();
	init_hardware( time_base );
	update_pwm();
*/
}


DMArpi::~DMArpi()
{
}


void DMArpi::get_model( unsigned mbox_board_rev )
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
			gError() << "Unable to detect Board Model from board revision: " << mbox_board_rev;
			break;
	}
}



int DMArpi::mbox_open()
{
	int file_desc;

	file_desc = open( "/dev/vcio", 0 );
	if ( file_desc < 0 ) {
		gError() << "Failed to create mailbox device : " << strerror(errno);
	}

	return file_desc;
}


void* DMArpi::map_peripheral( uint32_t base, uint32_t len )
{
	int fd = open( "/dev/mem", O_RDWR | O_SYNC );
	void * vaddr;

	if ( fd < 0 ) {
		gError() << "Failed to open /dev/mem : " << strerror(errno);
	}
	vaddr = mmap( nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
	if ( vaddr == MAP_FAILED ) {
		gError() << "pi-blaster: Failed to map peripheral at " << std::hex << base << " : " << strerror(errno);
	}
	close( fd );

	return vaddr;
}
