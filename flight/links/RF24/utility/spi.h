/*
 * TMRh20 2015
 * SPI layer for RF24 <-> BCM2835
 */
/**
 * @file spi.h
 * \cond HIDDEN_SYMBOLS
 * Class declaration for SPI helper files
 */
#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include <string>
#include <stdio.h>

typedef enum
{
    BCM2835_SPI_CS0 = 0,     /*!< Chip Select 0 */
    BCM2835_SPI_CS1 = 1,     /*!< Chip Select 1 */
    BCM2835_SPI_CS2 = 2,     /*!< Chip Select 2 (ie pins CS1 and CS2 are asserted) */
    BCM2835_SPI_CS_NONE = 3  /*!< No CS, control it yourself */
} bcm2835SPIChipSelect;

typedef enum
{
    BCM2835_SPI_BIT_ORDER_LSBFIRST = 0,  /*!< LSB First */
    BCM2835_SPI_BIT_ORDER_MSBFIRST = 1   /*!< MSB First */
}bcm2835SPIBitOrder;

/*! \brief SPI Data mode
  Specify the SPI data mode to be passed to bcm2835_spi_setDataMode()
*/
typedef enum
{
    BCM2835_SPI_MODE0 = 0,  /*!< CPOL = 0, CPHA = 0 */
    BCM2835_SPI_MODE1 = 1,  /*!< CPOL = 0, CPHA = 1 */
    BCM2835_SPI_MODE2 = 2,  /*!< CPOL = 1, CPHA = 0 */
    BCM2835_SPI_MODE3 = 3   /*!< CPOL = 1, CPHA = 1 */
}bcm2835SPIMode;

typedef enum
{
    BCM2835_SPI_CLOCK_DIVIDER_65536 = 0,       /*!< 65536 = 262.144us = 3.814697260kHz */
    BCM2835_SPI_CLOCK_DIVIDER_32768 = 32768,   /*!< 32768 = 131.072us = 7.629394531kHz */
    BCM2835_SPI_CLOCK_DIVIDER_16384 = 16384,   /*!< 16384 = 65.536us = 15.25878906kHz */
    BCM2835_SPI_CLOCK_DIVIDER_8192  = 8192,    /*!< 8192 = 32.768us = 30/51757813kHz */
    BCM2835_SPI_CLOCK_DIVIDER_4096  = 4096,    /*!< 4096 = 16.384us = 61.03515625kHz */
    BCM2835_SPI_CLOCK_DIVIDER_2048  = 2048,    /*!< 2048 = 8.192us = 122.0703125kHz */
    BCM2835_SPI_CLOCK_DIVIDER_1024  = 1024,    /*!< 1024 = 4.096us = 244.140625kHz */
    BCM2835_SPI_CLOCK_DIVIDER_512   = 512,     /*!< 512 = 2.048us = 488.28125kHz */
    BCM2835_SPI_CLOCK_DIVIDER_256   = 256,     /*!< 256 = 1.024us = 976.5625kHz */
    BCM2835_SPI_CLOCK_DIVIDER_128   = 128,     /*!< 128 = 512ns = = 1.953125MHz */
    BCM2835_SPI_CLOCK_DIVIDER_64    = 64,      /*!< 64 = 256ns = 3.90625MHz */
    BCM2835_SPI_CLOCK_DIVIDER_32    = 32,      /*!< 32 = 128ns = 7.8125MHz */
    BCM2835_SPI_CLOCK_DIVIDER_16    = 16,      /*!< 16 = 64ns = 15.625MHz */
    BCM2835_SPI_CLOCK_DIVIDER_8     = 8,       /*!< 8 = 32ns = 31.25MHz */
    BCM2835_SPI_CLOCK_DIVIDER_4     = 4,       /*!< 4 = 16ns = 62.5MHz */
    BCM2835_SPI_CLOCK_DIVIDER_2     = 2,       /*!< 2 = 8ns = 125MHz, fastest you can get */
    BCM2835_SPI_CLOCK_DIVIDER_1     = 1        /*!< 1 = 262.144us = 3.814697260kHz, same as 0/65536 */
} bcm2835SPIClockDivider;

/// \brief bcm2835SPISpeed
/// Specifies the divider used to generate the SPI clock from the system clock.
/// Figures below give the clock speed instead of clock divider.
#define BCM2835_SPI_SPEED_64MHZ BCM2835_SPI_CLOCK_DIVIDER_4
#define BCM2835_SPI_SPEED_32MHZ BCM2835_SPI_CLOCK_DIVIDER_8
#define BCM2835_SPI_SPEED_16MHZ BCM2835_SPI_CLOCK_DIVIDER_16
#define BCM2835_SPI_SPEED_8MHZ BCM2835_SPI_CLOCK_DIVIDER_32
#define BCM2835_SPI_SPEED_4MHZ BCM2835_SPI_CLOCK_DIVIDER_64
#define BCM2835_SPI_SPEED_2MHZ BCM2835_SPI_CLOCK_DIVIDER_128
#define BCM2835_SPI_SPEED_1MHZ BCM2835_SPI_CLOCK_DIVIDER_256
#define BCM2835_SPI_SPEED_512KHZ BCM2835_SPI_CLOCK_DIVIDER_512
#define BCM2835_SPI_SPEED_256KHZ BCM2835_SPI_CLOCK_DIVIDER_1024
#define BCM2835_SPI_SPEED_128KHZ BCM2835_SPI_CLOCK_DIVIDER_2048
#define BCM2835_SPI_SPEED_64KHZ BCM2835_SPI_CLOCK_DIVIDER_4096
#define BCM2835_SPI_SPEED_32KHZ BCM2835_SPI_CLOCK_DIVIDER_8192
#define BCM2835_SPI_SPEED_16KHZ BCM2835_SPI_CLOCK_DIVIDER_16384
#define BCM2835_SPI_SPEED_8KHZ BCM2835_SPI_CLOCK_DIVIDER_32768

namespace nRF24 {

#define SPI_HAS_TRANSACTION
#define MSBFIRST BCM2835_SPI_BIT_ORDER_MSBFIRST
#define SPI_MODE0 BCM2835_SPI_MODE0
#define RF24_SPI_SPEED BCM2835_SPI_SPEED_8MHZ
    
class SPISettings {
public:
	SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
        init(clock,bitOrder,dataMode);
	}
    SPISettings() { init(RF24_SPI_SPEED, MSBFIRST, SPI_MODE0); }

    uint32_t clck;
    uint8_t border;
    uint8_t dmode;
private:

	void init(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
                clck = clock;
                border = bitOrder;
                dmode = dataMode;
	}
	friend class SPIClass;
};


class SPI {
public:
	SPI();
	virtual ~SPI();
	
	static uint8_t transfer(uint8_t _data);
	static void transfernb(char* tbuf, char* rbuf, uint32_t len);
	static void transfern(char* buf, uint32_t len);  

	static void begin( const std::string& device );
	static void end();

	static void setBitOrder(uint8_t bit_order);
	static void setDataMode(uint8_t data_mode);
	static void setClockDivider(uint16_t spi_speed);
	static void chipSelect(int csn_pin);
	
	static void beginTransaction(SPISettings settings);
	static void endTransaction();
};

/**
 * \endcond
 */

}; //namespace nRF24

#endif
