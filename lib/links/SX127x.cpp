#include <unistd.h>
#include "SX127x.h"
#include "SX127xRegs.h"
#include "SX127xRegs-LoRa.h"
#include <Config.h>
#include <Debug.h>
#include <SPI.h>
#include <GPIO.h>
#include <Board.h>
#include <cmath>

#define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

#define TICKS ( Board::GetTicks() / 1000 )
#define TIMEOUT_MS 1000 // 1000 ms
#define XTAL_FREQ 32000000
#define FREQ_STEP 61.03515625f
#define PACKET_SIZE 32
// #define MAX_BLOCK_ID 256
#define MAX_BLOCK_ID 128

// TODO : LoRa see https://github.com/chandrawi/LoRaRF-Python/blob/main/LoRaRF/SX127x.py

static uint8_t GetFskBandwidthRegValue( uint32_t bandwidth );
static const char* GetRegName( uint8_t reg );

typedef struct FskBandwidth_t
{
	uint32_t bandwidth;
	uint8_t	 RegValue;
} FskBandwidth_t;

static const FskBandwidth_t FskBandwidths[] = {
	{ 2600	, 0x17 },
	{ 3100	, 0x0F },
	{ 3900	, 0x07 },
	{ 5200	, 0x16 },
	{ 6300	, 0x0E },
	{ 7800	, 0x06 },
	{ 10400 , 0x15 },
	{ 12500 , 0x0D },
	{ 15600 , 0x05 },
	{ 20800 , 0x14 },
	{ 25000 , 0x0C },
	{ 31300 , 0x04 },
	{ 41700 , 0x13 },
	{ 50000 , 0x0B },
	{ 62500 , 0x03 },
	{ 83333 , 0x12 },
	{ 100000, 0x0A },
	{ 125000, 0x02 },
	{ 166700, 0x11 },
	{ 200000, 0x09 },
	{ 250000, 0x01 },
	{ 300000, 0x00 }, // Invalid Badwidth
};

const struct {
	const char* name;
	uint8_t reg;
} regnames[] = {
	{ "REG_FIFO"           , 0x00 },
	{ "REG_OPMODE"         , 0x01 },
	{ "REG_BITRATEMSB"     , 0x02 },
	{ "REG_BITRATELSB"     , 0x03 },
	{ "REG_FDEVMSB"        , 0x04 },
	{ "REG_FDEVLSB"        , 0x05 },
	{ "REG_FRFMSB"         , 0x06 },
	{ "REG_FRFMID"         , 0x07 },
	{ "REG_FRFLSB"         , 0x08 },
	{ "REG_PACONFIG"       , 0x09 },
	{ "REG_PARAMP"         , 0x0A },
	{ "REG_OCP"            , 0x0B },
	{ "REG_LNA"            , 0x0C },
	{ "REG_RXCONFIG"       , 0x0D },
	{ "REG_RSSICONFIG"     , 0x0E },
	{ "REG_RSSICOLLISION"  , 0x0F },
	{ "REG_RSSITHRESH"     , 0x10 },
	{ "REG_RSSIVALUE"      , 0x11 },
	{ "REG_RXBW"           , 0x12 },
	{ "REG_AFCBW"          , 0x13 },
	{ "REG_OOKPEAK"        , 0x14 },
	{ "REG_OOKFIX"         , 0x15 },
	{ "REG_OOKAVG"         , 0x16 },
	{ "REG_RES17"          , 0x17 },
	{ "REG_RES18"          , 0x18 },
	{ "REG_RES19"          , 0x19 },
	{ "REG_AFCFEI"         , 0x1A },
	{ "REG_AFCMSB"         , 0x1B },
	{ "REG_AFCLSB"         , 0x1C },
	{ "REG_FEIMSB"         , 0x1D },
	{ "REG_FEILSB"         , 0x1E },
	{ "REG_PREAMBLEDETECT" , 0x1F },
	{ "REG_RXTIMEOUT1"     , 0x20 },
	{ "REG_RXTIMEOUT2"     , 0x21 },
	{ "REG_RXTIMEOUT3"     , 0x22 },
	{ "REG_RXDELAY"        , 0x23 },
	{ "REG_OSC"            , 0x24 },
	{ "REG_PREAMBLEMSB"    , 0x25 },
	{ "REG_PREAMBLELSB"    , 0x26 },
	{ "REG_SYNCCONFIG"     , 0x27 },
	{ "REG_SYNCVALUE1"     , 0x28 },
	{ "REG_SYNCVALUE2"     , 0x29 },
	{ "REG_SYNCVALUE3"     , 0x2A },
	{ "REG_SYNCVALUE4"     , 0x2B },
	{ "REG_SYNCVALUE5"     , 0x2C },
	{ "REG_SYNCVALUE6"     , 0x2D },
	{ "REG_SYNCVALUE7"     , 0x2E },
	{ "REG_SYNCVALUE8"     , 0x2F },
	{ "REG_PACKETCONFIG1"  , 0x30 },
	{ "REG_PACKETCONFIG2"  , 0x31 },
	{ "REG_PAYLOADLENGTH"  , 0x32 },
	{ "REG_NODEADRS"       , 0x33 },
	{ "REG_BROADCASTADRS"  , 0x34 },
	{ "REG_FIFOTHRESH"     , 0x35 },
	{ "REG_SEQCONFIG1"     , 0x36 },
	{ "REG_SEQCONFIG2"     , 0x37 },
	{ "REG_TIMERRESOL"     , 0x38 },
	{ "REG_TIMER1COEF"     , 0x39 },
	{ "REG_TIMER2COEF"     , 0x3A },
	{ "REG_IMAGECAL"       , 0x3B },
	{ "REG_TEMP"           , 0x3C },
	{ "REG_LOWBAT"         , 0x3D },
	{ "REG_IRQFLAGS1"      , 0x3E },
	{ "REG_IRQFLAGS2"      , 0x3F },
	{ "REG_DIOMAPPING1"    , 0x40 },
	{ "REG_DIOMAPPING2"    , 0x41 },
	{ "REG_VERSION"        , 0x42 },
	{ "REG_PLLHOP"         , 0x44 },
	{ "REG_TCXO"           , 0x4B },
	{ "REG_PADAC"          , 0x4D },
	{ "REG_FORMERTEMP"     , 0x5B },
	{ "REG_BITRATEFRAC"    , 0x5D },
	{ "REG_AGCREF"         , 0x61 },
	{ "REG_AGCTHRESH1"     , 0x62 },
	{ "REG_AGCTHRESH2"     , 0x63 },
	{ "REG_AGCTHRESH3"     , 0x64 },
	{ "REG_PLL"            , 0x70 },
	{ "unk"                , 0xFF },
};

static uint8_t crc8( const uint8_t* buf, uint32_t len );


SX127x::SX127x()
	: Link()
	, mSPI( nullptr )
	, mReady( false )
	, mDevice( "/dev/spidev0.0" )
	, mResetPin( -1 )
	, mTXPin( -1 )
	, mRXPin( -1 )
	, mIRQPin( -1 )
	, mLedPin( -1 )
	, mBlocking( true )
	, mDropBroken( true )
	, mEnableTCXO( false )
	, mModem( FSK )
	, mFrequency( 433000000 )
	, mInputPort( 0 )
	, mOutputPort( 1 )
	, mRetries( 1 )
	, mReadTimeout( 1000 )
	, mBitrate( 76800 )
	, mBandwidth( 250000 )
	, mBandwidthAfc( 500000 )
	, mFdev( 200000 )
	, mRSSI( 0 )
	, mRxQuality( 0 )
	, mPerfTicks( 0 )
	, mPerfLastRxBlock( 0 )
	, mPerfValidBlocks( 0 )
	, mPerfInvalidBlocks( 0 )
	, mPerfBlocksPerSecond( 0 )
	, mPerfMaxBlocksPerSecond( 0 )
	, mDiversitySpi( nullptr )
	, mDiversityDevice( "" )
	, mDiversityResetPin( -1 )
	, mDiversityIrqPin( -1 )
	, mDiversityLedPin( -1 )
	, mTXBlockID( 0 )
{
// 	mDropBroken = false; // TEST (using custom CRC instead)

}


SX127x::~SX127x()
{
}


void SX127x::init()
{
	fDebug();
	mReady = true;

	if ( mResetPin < 0 ) {
		gDebug() << "WARNING : No Reset-pin specified for SX127x, cannot create link !";
		return;
	}
	if ( mIRQPin < 0 ) {
		gDebug() << "WARNING : No IRQ-pin specified for SX127x, cannot create link !";
		return;
	}

	memset( &mRxBlock, 0, sizeof(mRxBlock) );

	mSPI = new SPI( mDevice, 5208333 ); // 7
	mSPI->Connect();
	GPIO::setMode( mResetPin, GPIO::Output );
	GPIO::Write( mResetPin, false );
	GPIO::setMode( mIRQPin, GPIO::Input );
// 	GPIO::setPUD( mIRQPin, GPIO::PullDown );
	if ( mLedPin >= 0 ) {
		GPIO::setMode( mLedPin, GPIO::Output );
	}
	GPIO::SetupInterrupt( mIRQPin, GPIO::Rising, [this](){
		this->Interrupt( mSPI, mLedPin );
		if ( mLedPin >= 0 ) {
			GPIO::Write( mLedPin, false );
		}
	});
	if ( mTXPin >= 0 ) {
		GPIO::Write( mTXPin, false );
		GPIO::setMode( mTXPin, GPIO::Output );
	}
	if ( mRXPin >= 0 ) {
		GPIO::Write( mRXPin, false );
		GPIO::setMode( mRXPin, GPIO::Output );
	}

	if ( mDiversityDevice.length() > 0 ) {
		if ( mDiversityResetPin < 0 ) {
			gDebug() << "WARNING : No Reset-pin specified for SX127x diversity, cannot create link !";
		} else {
			if ( mDiversityIrqPin < 0 ) {
				gDebug() << "WARNING : No IRQ-pin specified for SX127x diversity, cannot create link !";
			} else {
				mDiversitySpi = new SPI( mDiversityDevice, 5 * 1000 * 1000 ); // 7
				mDiversitySpi->Connect();
				GPIO::setMode( mDiversityResetPin, GPIO::Output );
				GPIO::Write( mDiversityResetPin, false );
				GPIO::setMode( mDiversityIrqPin, GPIO::Input );
				GPIO::setMode( mDiversityLedPin, GPIO::Output );
				GPIO::SetupInterrupt( mDiversityIrqPin, GPIO::Rising, [this](){
					this->Interrupt( mDiversitySpi, mDiversityLedPin );
					GPIO::Write( mDiversityLedPin, false );
				} );
			}
		}
	}

	gDebug() << "Reset pins : " << mResetPin << ", " << mDiversityResetPin;
	gDebug() << "LED pins : " << mLedPin << ", " << mDiversityLedPin;
}


int SX127x::Connect()
{
	fDebug();

	if ( !mReady ) {
		init();
	}

	// Reset chip
	reset();

	if ( not ping() ) {
		gDebug() << "Module online : " << ping();
		Board::defectivePeripherals()["SX127x"] = true;
		return -1;
	}

	int32_t ret = Setup( mSPI );
	if ( ret >= 0 and mDiversitySpi ) {
		ret = Setup( mDiversitySpi );
	}

	startReceiving();

	return ret;
}


int32_t SX127x::Setup( SPI* spi )
{
	// Cut the PA just in case, RFO output, power = -1 dBm
	writeRegister( spi, REG_PACONFIG, 0x00 );
	{
		// Start calibration in LF band
		writeRegister ( spi, REG_IMAGECAL, ( readRegister( spi, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
		while ( ( readRegister( spi, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING ) {
			usleep( 10 );
		}
	}
	{
		// Set frequency to mid-HF band
		setFrequency( spi, 868000000 );
		// Start calibration in HF band
		writeRegister( spi, REG_IMAGECAL, ( readRegister( spi, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
		while ( ( readRegister( spi, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING ) {
			usleep( 10 );
		}
	}

	// Start module in sleep mode
	setOpMode( spi, RF_OPMODE_SLEEP );

	// Set modem
	setModem( spi, mModem );

	// Switch to StandBy mode for setup
	setOpMode( spi, RF_OPMODE_STANDBY );

	// Set frequency
	setFrequency( spi, mFrequency );

	bool boosted_module_20dbm = true;

	// Set PA to maximum => TODO : make it more configurable
	writeRegister( spi, REG_PACONFIG, ( boosted_module_20dbm ? RF_PACONFIG_PASELECT_PABOOST : RF_PACONFIG_PASELECT_RFO ) | 0x70 | 0xF );
	writeRegister( spi, REG_PADAC, 0x80 | ( boosted_module_20dbm ? RF_PADAC_20DBM_ON : RF_PADAC_20DBM_OFF ) );
	writeRegister( spi, REG_OCP, RF_OCP_TRIM_240_MA | RF_OCP_OFF );
	writeRegister( spi, REG_PARAMP, RF_PARAMP_SHAPING_NONE | RF_PARAMP_0040_US | RF_PARAMP_LOWPNTXPLL_OFF );
// 	writeRegister( spi, REG_PARAMP, RF_PARAMP_SHAPING_NONE | ( mModem == LoRa ? RF_PARAMP_0050_US : RF_PARAMP_0010_US ) | RF_PARAMP_LOWPNTXPLL_OFF );

	// Set LNA
	writeRegister( spi, REG_LNA, RF_LNA_GAIN_G1 | RF_LNA_BOOST_ON );

	// Enable TCXO if available
	if ( mEnableTCXO ) {
		while ( ( readRegister( spi, REG_TCXO ) & RF_TCXO_TCXOINPUT_ON ) != RF_TCXO_TCXOINPUT_ON ) {
			usleep( 50 );
			writeRegister( spi, REG_TCXO, ( readRegister( REG_TCXO ) | RF_TCXO_TCXOINPUT_ON ) );
		}
	}

	if ( mModem == FSK ) {
		// Set RX/Sync config
// 		writeRegister( spi, REG_RXCONFIG, RF_RXCONFIG_RESTARTRXONCOLLISION_ON | RF_RXCONFIG_AFCAUTO_OFF | RF_RXCONFIG_AGCAUTO_OFF );
// 		writeRegister( spi, REG_RXCONFIG, RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT | RF_RXCONFIG_RESTARTRXONCOLLISION_ON | RF_RXCONFIG_AFCAUTO_OFF | RF_RXCONFIG_AGCAUTO_OFF );
		writeRegister( spi, REG_SYNCCONFIG, RF_SYNCCONFIG_SYNC_ON | RF_SYNCCONFIG_AUTORESTARTRXMODE_WAITPLL_ON | RF_SYNCCONFIG_PREAMBLEPOLARITY_AA | RF_SYNCCONFIG_SYNCSIZE_2 );
		writeRegister( spi, REG_SYNCVALUE1, 0x64 );
		writeRegister( spi, REG_SYNCVALUE2, 0x72 );
		// writeRegister( spi, REG_SYNCVALUE3, 0x69 );
		// writeRegister( spi, REG_SYNCVALUE4, 0x30 );
		writeRegister( spi, REG_PREAMBLEDETECT, RF_PREAMBLEDETECT_DETECTOR_ON | RF_PREAMBLEDETECT_DETECTORSIZE_1 | RF_PREAMBLEDETECT_DETECTORTOL_20 );
	}

	// Set RSSI smoothing
	writeRegister( spi, REG_RSSICONFIG, RF_RSSICONFIG_SMOOTHING_16 );

	if ( mModem == LoRa ) {
		mBandwidth = 125000;
		mBitrate = 50000;
	}

	TxRxConfig_t modemConfig = {
		.bandwidth = mBandwidth,
		.datarate = mBitrate,
		.coderate = 5,
		.preambleLen = 6,
		.crcOn = mDropBroken,
		.freqHopOn = false,
		.hopPeriod = 0,
		.iqInverted = false,
		.fdev = mFdev,
		.timeout = 2000,
		.bandwidthAfc = mBandwidthAfc,
		.symbTimeout = 1,
		.payloadLen = PACKET_SIZE,
	};
	SetupModem( spi, modemConfig );

	gDebug() << "Module online : " << ping( spi );
	gDebug() << "Modem : " << ( ( readRegister( spi, REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) ? "LoRa" : "FSK" );
	gDebug() << "Frequency : " << (uint32_t)((float)(((uint32_t)readRegister(spi, REG_FRFMSB) << 16 ) | ((uint32_t)readRegister(spi, REG_FRFMID) << 8 ) | ((uint32_t)readRegister(spi, REG_FRFLSB) )) * FREQ_STEP) << "Hz";
	gDebug() << "PACONFIG : 0x" << hex << (int)readRegister( spi, REG_PACONFIG );
	gDebug() << "PADAC : 0x" << hex << (int)readRegister( spi, REG_PADAC );
	gDebug() << "PARAMP : 0x" << hex << (int)readRegister( spi, REG_PARAMP );
	gDebug() << "OCP : 0x" << hex << (int)readRegister( spi, REG_OCP );
	gDebug() << "RXCONFIG : " << hex << (int)readRegister( spi, REG_RXCONFIG );
	gDebug() << "SYNCCONFIG : " << hex << (int)readRegister( spi, REG_SYNCCONFIG );

	mConnected = true;
	return 0;
}


void SX127x::SetupModem( SPI* spi, const TxRxConfig_t& conf )
{
	switch ( mModem ) {
		case FSK: {
			// Set the data rate (common for TX and RX)
			uint32_t datarate = (uint16_t)((double)XTAL_FREQ / (double)conf.datarate );
			writeRegister( spi, REG_BITRATEMSB, (uint8_t)(datarate >> 8) );
			writeRegister( spi, REG_BITRATELSB, (uint8_t)(datarate & 0xFF) );

			// TX: Set frequency deviation (fdev)
			if ( conf.fdev > 0 ) {
				uint32_t fdev = (uint16_t)((double)conf.fdev / (double)FREQ_STEP );
				writeRegister( spi, REG_FDEVMSB, (uint8_t)(fdev >> 8) );
				writeRegister( spi, REG_FDEVLSB, (uint8_t)(fdev & 0xFF) );
			}

			// RX: Set RX bandwidth and AFC bandwidth
			writeRegister( spi, REG_RXBW, GetFskBandwidthRegValue(conf.bandwidth) );
			if ( conf.bandwidthAfc > 0 ) {
				writeRegister( spi, REG_AFCBW, GetFskBandwidthRegValue(conf.bandwidthAfc) );
			}

			// Set the preamble length (common for TX and RX)
			writeRegister( spi, REG_PREAMBLEMSB, (uint8_t)((conf.preambleLen >> 8) & 0xFF) );
			writeRegister( spi, REG_PREAMBLELSB, (uint8_t)(conf.preambleLen & 0xFF) );

			// Set packet configuration (common for TX and RX)
			writeRegister( spi, REG_PACKETCONFIG1, ( readRegister( spi, REG_PACKETCONFIG1 ) & RF_PACKETCONFIG1_CRC_MASK & RF_PACKETCONFIG1_PACKETFORMAT_MASK ) | RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE | ( conf.crcOn << 4 ) );
			writeRegister( spi, REG_PACKETCONFIG2, ( readRegister( spi, REG_PACKETCONFIG2 ) & RF_PACKETCONFIG2_DATAMODE_MASK ) | RF_PACKETCONFIG2_DATAMODE_PACKET );

			writeRegister( spi, REG_PAYLOADLENGTH, 0xFF );

			break;
		}

		case LoRa: {
			// Set bandwidth
			uint32_t bandwidth = conf.bandwidth;
			if ( bandwidth == 41700 ) {
				bandwidth = RFLR_MODEMCONFIG1_BW_41_66_KHZ;
			} else if ( bandwidth == 62500 ) {
				bandwidth = RFLR_MODEMCONFIG1_BW_62_50_KHZ;
			} else if ( bandwidth == 125000 ) {
				bandwidth = RFLR_MODEMCONFIG1_BW_125_KHZ;
			} else if ( bandwidth == 250000 ) {
				bandwidth = RFLR_MODEMCONFIG1_BW_250_KHZ;
			} else if ( bandwidth == 500000 ) {
				bandwidth = RFLR_MODEMCONFIG1_BW_500_KHZ;
			} else {
				gError() << "Unsupported LoRa Bandwidth !";
				return;
			}

			// Set coding rate
			uint32_t codingrate = conf.coderate;
			if ( codingrate == 5 ) {
				codingrate = RFLR_MODEMCONFIG1_CODINGRATE_4_5;
			} else if ( codingrate == 6 ) {
				codingrate = RFLR_MODEMCONFIG1_CODINGRATE_4_6;
			} else if ( codingrate == 7 ) {
				codingrate = RFLR_MODEMCONFIG1_CODINGRATE_4_7;
			} else if ( codingrate == 8 ) {
				codingrate = RFLR_MODEMCONFIG1_CODINGRATE_4_8;
			} else {
				gError() << "Unsupported LoRa Coding Rate !";
				return;
			}

			// Calculate spreading factor based on datarate and bandwidth
			uint32_t spreading_factor = 6u;
			for ( uint32_t i = 0; i <= 6; i++ ) {
				uint32_t bitrate = spreading_factor * (4 / conf.coderate) / (pow(2, spreading_factor) / conf.bandwidth);
				if ( conf.datarate >= bitrate ) {
					break;
				}
				spreading_factor++;
			}

			// TEST
			bandwidth = RFLR_MODEMCONFIG1_BW_500_KHZ;
			codingrate = RFLR_MODEMCONFIG1_CODINGRATE_4_5;
			spreading_factor = 7;

			gDebug() << "LoRa BW : " << (conf.bandwidth / 1000) << "kHz";
			gDebug() << "LoRa CR : " << (4 + codingrate / 2);
			gDebug() << "LoRa SF : " << spreading_factor;

			// Set LoRa registers (common for TX and RX)
			writeRegister( spi, REG_LR_MODEMCONFIG1, (readRegister(spi, REG_LR_MODEMCONFIG1) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | bandwidth | codingrate | RFLR_MODEMCONFIG1_IMPLICITHEADER_OFF );
			writeRegister( spi, REG_LR_MODEMCONFIG2, (readRegister(spi, REG_LR_MODEMCONFIG2) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) | (spreading_factor << 4) | (conf.crcOn << 2) );

			// Set preamble length
			writeRegister( spi, REG_LR_PREAMBLEMSB, (uint8_t)((conf.preambleLen >> 8) & 0xFF) );
			writeRegister( spi, REG_LR_PREAMBLELSB, (uint8_t)(conf.preambleLen & 0xFF) );
			
			writeRegister( spi, REG_LR_SYNCWORD, 18 );

			if( spreading_factor == 6 ) {
				writeRegister( spi, REG_LR_DETECTOPTIMIZE, ( readRegister( spi, REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF6 );
				writeRegister( spi, REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6 );
			} else {
				writeRegister( spi, REG_LR_DETECTOPTIMIZE, ( readRegister( spi, REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
				writeRegister( spi, REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
			}

			// Frequency hopping settings (common for TX and RX)
			if ( conf.freqHopOn ) {
				writeRegister( spi, REG_LR_PLLHOP, (readRegister(spi, REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK) | RFLR_PLLHOP_FASTHOP_ON );
				writeRegister( spi, REG_LR_HOPPERIOD, conf.hopPeriod );
			}

			// ERRATA and specific settings based on bandwidth and frequency
			if ( bandwidth == RFLR_MODEMCONFIG1_BW_500_KHZ ) {
				writeRegister( spi, REG_LR_TEST36, 0x02 );
				writeRegister( spi, REG_LR_TEST3A, (mFrequency > 525000000) ? 0x64 : 0x7F );
				writeRegister( spi, REG_LR_DETECTOPTIMIZE, (readRegister(spi, REG_LR_DETECTOPTIMIZE) & ~0x80) | 0x80 );
			} else {
				writeRegister( spi, REG_LR_TEST36, 0x03 );
				writeRegister( spi, REG_LR_TEST3A, 0x65 );
				writeRegister( spi, REG_LR_DETECTOPTIMIZE, (readRegister(spi, REG_LR_DETECTOPTIMIZE) & ~0x80) | 0x00 );
			}

			writeRegister( spi, REG_LR_PAYLOADLENGTH, 12 );

			break;
		}
	}
}


void SX127x::setFrequency( float f )
{
	setFrequency( mSPI, f );
	if ( mDiversitySpi ) {
		setFrequency( mDiversitySpi, f );
	}
}


void SX127x::setFrequency( SPI* spi, float f )
{
	uint32_t freq = (uint32_t)( (float)f / FREQ_STEP );
	writeRegister( spi, REG_FRFMSB, (uint8_t)( (freq >> 16) & 0xFF ) );
	writeRegister( spi, REG_FRFMID, (uint8_t)( (freq >> 8) & 0xFF ) );
	writeRegister( spi, REG_FRFLSB, (uint8_t)( freq & 0xFF ) );
}


void SX127x::startReceiving( SPI* spi )
{
	auto start = [this]( SPI* spi ) {
		if ( mModem == LoRa ) {
			// writeRegister( spi, REG_LR_INVERTIQ, ( ( readRegister( spi, REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
			// writeRegister( spi, REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
			// writeRegister( spi, REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_VALIDHEADER | RFLR_IRQFLAGS_TXDONE | RFLR_IRQFLAGS_CADDONE | RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL | RFLR_IRQFLAGS_CADDETECTED );
			writeRegister( spi, REG_LR_DIOMAPPING1, ( readRegister( spi, REG_LR_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
			writeRegister( spi, REG_LR_DIOMAPPING2, ( readRegister( spi, REG_LR_DIOMAPPING2 ) & RFLR_DIOMAPPING2_MAP_MASK ) | RFLR_DIOMAPPING2_MAP_PREAMBLEDETECT );
			// writeRegister( spi, REG_LR_DETECTOPTIMIZE, readRegister( spi, REG_LR_DETECTOPTIMIZE ) | 0x80 );
			writeRegister( spi, REG_LR_FIFORXBASEADDR, 0 );
			writeRegister( spi, REG_LR_FIFOADDRPTR, 0 );
		}

		setOpMode( spi, RF_OPMODE_RECEIVER );
		writeRegister( spi, REG_LNA, RF_LNA_GAIN_G1 | RF_LNA_BOOST_ON );
	};
	if ( not spi or spi == mSPI ) {
		mSending = false;
		mSendingEnd = false;
		if ( mTXPin >= 0 ) {
			GPIO::Write( mTXPin, false );
		}
		if ( mRXPin >= 0 ) {
			GPIO::Write( mRXPin, true );
		}
	}
	if ( spi ) {
		start( spi );
	} else {
		start( mSPI );
		if ( mDiversitySpi ) {
			start( mDiversitySpi );
		}
	}
}


void SX127x::startTransmitting()
{
	mInterruptMutex.lock();
	if ( mDiversitySpi ) {
		setOpMode( mDiversitySpi, RF_OPMODE_STANDBY );
	}

	if ( mRXPin >= 0 ) {
		GPIO::Write( mRXPin, false );
	}
	if ( mTXPin >= 0 ) {
		GPIO::Write( mTXPin, true );
	}

	if ( mModem == LoRa ) {
		// writeRegister( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT | RFLR_IRQFLAGS_RXDONE | RFLR_IRQFLAGS_PAYLOADCRCERROR | RFLR_IRQFLAGS_VALIDHEADER | RFLR_IRQFLAGS_CADDONE | RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL | RFLR_IRQFLAGS_CADDETECTED );
		writeRegister( REG_LR_DIOMAPPING1, ( readRegister( REG_LR_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
	}

	setOpMode( mSPI, RF_OPMODE_TRANSMITTER );
	mInterruptMutex.unlock();
}


int SX127x::setBlocking( bool blocking )
{
	mBlocking = blocking;
	return 0;
}


void SX127x::setRetriesCount( int retries )
{
	mRetries = retries;
}


int SX127x::retriesCount() const
{
	return mRetries;
}


int32_t SX127x::Channel()
{
	return 0;
}


int32_t SX127x::Frequency()
{
	return mFrequency / 1000000;
}


int32_t SX127x::RxQuality()
{
	return mRxQuality;
}


int32_t SX127x::RxLevel()
{
	return mRSSI;
}


void SX127x::PerfUpdate()
{
	const uint64_t divider = 1; // Calculate RxQuality using packets received on the last 1000ms

	uint64_t tick = TICKS;
	mPerfMutex.lock();
	while ( mTotalHistory.size() > 0 and mTotalHistory.front() <= tick - ( 1000LLU / divider ) ) {
		mPerfTotalBlocks = max( 0, mPerfTotalBlocks - 1 );
		mTotalHistory.pop_front();
	}
	while ( mMissedHistory.size() > 0 and mMissedHistory.front() <= tick - ( 1000LLU / divider ) ) {
		mPerfMissedBlocks = max( 0, mPerfMissedBlocks - 1 );
		mMissedHistory.pop_front();
	}
	mPerfMutex.unlock();

	uint32_t receivedBlocks = mPerfTotalBlocks - mPerfMissedBlocks;
	mRxQuality = min( 100, 100 * receivedBlocks / mPerfTotalBlocks );
}


SyncReturn SX127x::Read( void* pRet, uint32_t len, int32_t timeout )
{
	bool timedout = false;
	uint64_t started_waiting_at = TICKS;
	if ( timeout <= 0 ) {
		timeout = mReadTimeout;
	}

	do {
		bool ok = false;
		mRxQueueMutex.lock();
		ok = ( mRxQueue.size() > 0 );
		mRxQueueMutex.unlock();
		if ( ok ) {
			break;
		}
		if ( timeout > 0 and TICKS - started_waiting_at > (uint32_t)timeout ) {
			timedout = true;
			break;
		}
		PerfUpdate();
		usleep( 50 );
	} while ( mBlocking );

	mRxQueueMutex.lock();
	if ( mRxQueue.size() == 0 ) {
		mRxQueueMutex.unlock();
		if ( timedout ) {
			if ( mLedPin ) {
				GPIO::Write( mLedPin, 1 );
				if ( mDiversityLedPin >= 0 ) {
					GPIO::Write( mDiversityLedPin, 1 );
				}
				usleep( 500 );
				GPIO::Write( mLedPin, 0 );
				if ( mDiversityLedPin >= 0 ) {
					GPIO::Write( mDiversityLedPin, 0 );
				}
			}
			if ( !ping() ) {
				gDebug() << "Module online : " << ping();
			}
			return TIMEOUT;
		}
		return 0;
	}

	pair< uint8_t*, uint32_t > data = mRxQueue.front();
	mRxQueue.pop_front();
	mRxQueueMutex.unlock();
	memcpy( pRet, data.first, data.second );
	delete data.first;
	return data.second;
}


SyncReturn SX127x::Write( const void* data, uint32_t len, bool ack, int32_t timeout )
{
	// fDebug( data, len, ack, timeout );
// 	while ( mSending ) {
// 		usleep( 10 );
// 	}
	mSending = true;

	uint8_t buf[PACKET_SIZE];
	memset( buf, 0, sizeof(buf) );
	Header* header = (Header*)buf;

	mTXBlockID = ( mTXBlockID + 1 ) % MAX_BLOCK_ID;
	header->block_id = mTXBlockID;

	uint8_t packets_count = (uint8_t)std::ceil( (float)len / (float)( PACKET_SIZE - sizeof(Header) ) );
	uint8_t header_len = sizeof(Header);
	if ( len <= PACKET_SIZE - sizeof(Header) ) {
		header->small_packet = 1;
		header_len = sizeof(HeaderMini);
		HeaderMini* small_header = (HeaderMini*)buf;
		small_header->crc = crc8( (uint8_t*)data, len );
		packets_count = 1;
	} else {
		header->packets_count = packets_count;
	}

	uint32_t offset = 0;
	for ( uint8_t packet = 0; packet < packets_count; packet++ ) {
		uint32_t plen = PACKET_SIZE - header_len;
		if ( offset + plen > len ) {
			plen = len - offset;
		}

		memcpy( buf + header_len, (uint8_t*)data + offset, plen );
		if ( header->small_packet == 0 ) {
			header->crc = crc8( (uint8_t*)data + offset, plen );
		}

// 		for ( int32_t retry = 0; retry < mRetries; retry++ )
		{
// 			while ( mSending ) {
// 				usleep( 1 );
// 			}
			uint8_t tx[64];
			uint8_t rx[64];
			memset( tx, 0, sizeof(tx) );
			memset( rx, 0, sizeof(rx) );

			if ( mModem == LoRa ) {
				writeRegister( REG_LR_PAYLOADLENGTH, plen + header_len );
				writeRegister( REG_LR_FIFOTXBASEADDR, 0 );
				writeRegister( REG_LR_FIFOADDRPTR, 0 );

				tx[0] = REG_LR_FIFO | 0x80;
				memcpy( &tx[1], buf, plen + header_len );
			} else {
				// writeRegister( REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTARTCONDITION_FIFOTHRESH | ( plen + header_len ) );
				writeRegister( REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTARTCONDITION_FIFOTHRESH );

				tx[0] = REG_FIFO | 0x80;
				tx[1] = plen + header_len;
				memcpy( &tx[2], buf, plen + header_len );
			}

			mSendingEnd = false;

			if ( header->small_packet ) {
				HeaderMini* small_header = (HeaderMini*)buf;
				// printf( "Sending [%u] { %d } [%d bytes]\n", plen + header_len, small_header->block_id, plen );
			} else {
				// printf( "Sending [%u] { %d %d %d } [%d bytes]\n", plen + header_len, header->block_id, header->packet_id, header->packets_count, plen );
			}

			mSendingEnd = true;
			mSendTime = TICKS;
			setOpMode( mSPI, RF_OPMODE_STANDBY );
			if ( mDiversitySpi ) {
				setOpMode( mDiversitySpi, RF_OPMODE_STANDBY );
			}
			mSPI->Transfer( tx, rx, plen + header_len + 1 + ( mModem == FSK ) );
			startTransmitting();

			// while ( mModem == LoRa and mSending ) {
			// 	usleep( 1 );
			// }
		}

		if ( header->small_packet == 0 ) {
			header->packet_id++;
		}
		offset += plen;
	}

	return len;
}


SyncReturn SX127x::WriteAck( const void* buf, uint32_t len )
{
	return 0;
}


uint32_t SX127x::fullReadSpeed()
{
	return 0;
}


int SX127x::Receive( uint8_t* buf, uint32_t buflen, void* pRet, uint32_t len )
{
	const auto updatePerfHistory = [this]( uint8_t block_id ) {
		int32_t deltaBlocks = 0;
		if ( block_id >= mRxBlock.block_id ) {
			deltaBlocks = block_id - mRxBlock.block_id;
		} else {
			deltaBlocks = block_id - ((int32_t)mRxBlock.block_id - MAX_BLOCK_ID);
		}
		mPerfMutex.lock();
		for ( int32_t i = 0; i < deltaBlocks; i++ ) {
			mTotalHistory.push_back( TICKS );
			mPerfTotalBlocks++;
		}
		for ( int32_t i = 0; i < deltaBlocks-1; i++ ) {
			mMissedHistory.push_back( TICKS );
			mPerfMissedBlocks++;
		}
		mPerfMutex.unlock();
	};

	Header* header = (Header*)buf;
	uint8_t* data = buf + sizeof(Header);
	int final_size = 0;
	int32_t datalen = buflen - sizeof(Header);

	if ( header->small_packet ) {
		data = buf + sizeof(HeaderMini);
		datalen = buflen - sizeof(HeaderMini);
	}

	if ( datalen <= 0 ) {
		gDebug() << "Packet smaller than 0 bytes !";
		mPerfInvalidBlocks++;
		return -1;
	}

	if ( header->small_packet ) {
		HeaderMini* small_header = (HeaderMini*)buf;
		if ( crc8( data, datalen ) != small_header->crc ) {
			gDebug() << "Invalid CRC " << (int)crc8( data, datalen ) << " != " << (int)small_header->crc;
			return -1;
		}
		if ( small_header->block_id == mRxBlock.block_id and mRxBlock.received ) {
			gTrace() << "Block (small) " << (int)header->block_id << " already received";
			return -1;
		}
		updatePerfHistory( small_header->block_id );
		mRxBlock.block_id = small_header->block_id;
		mRxBlock.received = true;
		mPerfValidBlocks++;
		mPerfMutex.lock();
		mPerfHistory.push_back( TICKS );
		mPerfMutex.unlock();
		memcpy( pRet, data, datalen );
		// gTrace() << "Received block (small) " << (int)header->block_id;
		return datalen;
	}

	uint8_t c = crc8( data, datalen );
	if ( c != header->crc ) {
		gDebug() << "Invalid CRC (" << (int)c << " != " << (int)header->crc << ")";
		mPerfInvalidBlocks++;
		return -1;
	}

	if ( header->block_id == mRxBlock.block_id and mRxBlock.received ) {
		gDebug() << "Block " << (int)header->block_id << " already received";
		return -1;
	}

	updatePerfHistory( header->block_id );

	if ( header->block_id != mRxBlock.block_id ) {
		memset( &mRxBlock, 0, sizeof(mRxBlock) );
		mRxBlock.block_id = header->block_id;
		mRxBlock.packets_count = header->packets_count;
	}

// 	printf( "Received [%d] { %d %d %d } [%d bytes]\n", buflen, header->block_id, header->packet_id, header->packets_count, datalen );

	if ( header->packets_count == 1 ) {
// 		printf( "small block\n" );
		mRxBlock.block_id = header->block_id;
		mRxBlock.received = true;
		mPerfValidBlocks++;
		mPerfMutex.lock();
		mPerfHistory.push_back( TICKS );
		mPerfMutex.unlock();
		memcpy( pRet, data, datalen );
		return datalen;
	}

	memcpy( mRxBlock.packets[header->packet_id].data, data, datalen );
	mRxBlock.packets[header->packet_id].size = datalen;

	if ( header->packet_id >= mRxBlock.packets_count - 1 ) {
		bool valid = true;
		for ( uint8_t i = 0; i < mRxBlock.packets_count; i++ ) {
			if ( mRxBlock.packets[i].size == 0 ) {
				mPerfInvalidBlocks++;
				valid = false;
				break;
			}
			if ( final_size + mRxBlock.packets[i].size >= (int)len ) {
				mPerfInvalidBlocks++;
				valid = false;
				break;
			}
			memcpy( &((uint8_t*)pRet)[final_size], mRxBlock.packets[i].data, mRxBlock.packets[i].size );
			final_size += mRxBlock.packets[i].size;
		}
		if ( mDropBroken and valid == false ) {
			mPerfInvalidBlocks++;
			return -1;
		}
		mRxBlock.received = true;
		mPerfValidBlocks++;
		mPerfMutex.lock();
		mPerfHistory.push_back( TICKS );
		mPerfMutex.unlock();
		return final_size;
	}

	return final_size;
}


void SX127x::Interrupt( SPI* spi, int32_t ledPin )
{
	// fDebug();

	int32_t rssi = 0;
	// Take RSSI first, before the value becomes invalidated
	if ( mModem == FSK ) {
		rssi = -( readRegister( spi, REG_RSSIVALUE ) >> 1 );
	} else {
		rssi = -157 + readRegister( spi, REG_LR_PKTRSSIVALUE ); // TBD : use REG_LR_RSSIVALUE ?
	}

// 	int module = spi->device()[spi->device().length() - 1] - '0';

	mInterruptMutex.lock();

	uint64_t tick_ms = TICKS;

	uint8_t opMode = getOpMode( spi );
	bool txDone = false;
	bool rxReady = false;
	bool crc_ok = false;
	// fDebug( txDone, rxReady, crc_ok );
	if ( mModem == LoRa ) {
		uint8_t irqFlags = readRegister( spi, REG_LR_IRQFLAGS );
		txDone = ( irqFlags & RFLR_IRQFLAGS_TXDONE );
		rxReady = ( irqFlags & RFLR_IRQFLAGS_RXDONE );
		crc_ok = !( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR );
	} else {
		uint8_t irqFlags = readRegister( spi, REG_IRQFLAGS1 );
		uint8_t irqFlags2 = readRegister( spi, REG_IRQFLAGS2 );
		txDone = ( irqFlags2 & RF_IRQFLAGS2_PACKETSENT );
		rxReady = ( irqFlags2 & RF_IRQFLAGS2_PAYLOADREADY );
		crc_ok = ( irqFlags2 & RF_IRQFLAGS2_CRCOK );
	}

	if ( txDone ) {
		if ( mModem == LoRa ) {
			writeRegister( REG_LR_IRQFLAGS, 0xFF );
		}
		if ( mSendingEnd ) {
			startReceiving();
		}
		mSending = false;
// 		gDebug() << "unlock";
		mInterruptMutex.unlock();
		return;
	}
// 	gDebug() << "SX127x::Interrupt(" << module << ")";

	if ( ledPin >= 0 ) {
		GPIO::Write( ledPin, true );
	}
	mRSSI = rssi;

	if( mDropBroken ) {
		if( not crc_ok ) {
			gDebug() << "SX127x::Interrupt() CRC error";
			// Clear Irqs
			if ( mModem == LoRa ) {
				writeRegister( REG_LR_IRQFLAGS, 0xFF );
			} else {
				writeRegister( spi, REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | RF_IRQFLAGS1_PREAMBLEDETECT | RF_IRQFLAGS1_SYNCADDRESSMATCH );
				writeRegister( spi, REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );
				uint8_t newreg = readRegister( spi, REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK;
				while ( writeRegister( spi, REG_RXCONFIG, newreg ) == false and TICKS - tick_ms < 50 ) {
					usleep( 1 );
				}
			}
			mInterruptMutex.unlock();
			return;
		}
	}

	uint8_t tx[128];
	uint8_t rx[128];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = REG_FIFO & 0x7f;

	uint8_t len = 0;
	if ( mModem == LoRa ) {
		len = readRegister( spi, REG_LR_RXNBBYTES );
	} else {
		len = readRegister( spi, REG_FIFO );
	}
	spi->Transfer( tx, rx, len + 1 );

	if ( len > 0 ) {
		PerfUpdate();
		uint8_t* buf = new uint8_t[32768];
		int ret = Receive( rx + 1, len, buf, 32768 );
		if ( ret > 0 ) {
			mRxQueueMutex.lock();
			mRxQueue.emplace_back( make_pair( buf, ret ) );
			mRxQueueMutex.unlock();
		} else {
			delete[] buf;
		}
	}

	if ( mModem == FSK ) {
		uint8_t newreg = readRegister( spi, REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHPLLLOCK;
		while ( writeRegister( spi, REG_RXCONFIG, newreg ) == false and TICKS - tick_ms < 50 ) {
			usleep( 1 );
		}
	} else {
		writeRegister( REG_LR_IRQFLAGS, 0xFF );
	}

// 	gDebug() << "SX127x::Interrupt(" << module << ") done";
	mInterruptMutex.unlock();
}


void SX127x::reset()
{
	GPIO::Write( mResetPin, false );
	if ( mDiversitySpi ) {
		GPIO::Write( mDiversityResetPin, false );
	}
	usleep( 1000 * 250 );
	GPIO::Write( mResetPin, true );
	if ( mDiversitySpi ) {
		GPIO::Write( mDiversityResetPin, true );
	}
	usleep( 1000 * 250 );
}


bool SX127x::ping( SPI* spi )
{
	if ( not spi ) {
		if ( mDiversitySpi ) {
			// gDebug() << "ping 0 : " << std::hex << (int)readRegister( mSPI, REG_VERSION );
			// gDebug() << "ping 1 : " << std::hex << (int)readRegister( mDiversitySpi, REG_VERSION );
			return ( readRegister( mSPI, REG_VERSION ) == SAMTEC_ID ) && ( readRegister( mDiversitySpi, REG_VERSION ) == SAMTEC_ID );
		} else {
			spi = mSPI;
		}
	}
	return readRegister( spi, REG_VERSION ) == SAMTEC_ID;
}


void SX127x::setModem( SPI* spi, Modem modem )
{
	switch( modem ) {
		default:
		case FSK:
			setOpMode( spi, RF_OPMODE_SLEEP );
			writeRegister( spi, REG_OPMODE, ( readRegister(spi, REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );
			setOpMode( spi, RF_OPMODE_STANDBY );
			writeRegister( spi, REG_DIOMAPPING1, ( readRegister( spi, REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK ) | RF_DIOMAPPING1_DIO0_00 );
			writeRegister( spi, REG_DIOMAPPING2, ( readRegister( spi, REG_DIOMAPPING2 ) & RF_DIOMAPPING2_MAP_MASK ) | RF_DIOMAPPING2_MAP_PREAMBLEDETECT );
			break;

		case LoRa:
			setOpMode( spi, RF_OPMODE_SLEEP );
			writeRegister( spi, REG_OPMODE, ( readRegister(spi, REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );
			setOpMode( spi, RF_OPMODE_STANDBY );
			writeRegister( spi, REG_LR_DIOMAPPING1, ( readRegister( spi, REG_LR_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
			writeRegister( spi, REG_LR_DIOMAPPING2, ( readRegister( spi, REG_LR_DIOMAPPING2 ) & RFLR_DIOMAPPING2_MAP_MASK ) | RFLR_DIOMAPPING2_MAP_PREAMBLEDETECT );
			break;
	}
}


uint32_t SX127x::getOpMode( SPI* spi )
{
	return readRegister( spi, REG_OPMODE ) & ~RF_OPMODE_MASK;
}


bool SX127x::setOpMode( SPI* spi, uint32_t opMode )
{
	writeRegister( spi, REG_OPMODE, ( readRegister(spi, REG_OPMODE) & RF_OPMODE_MASK ) | opMode );

	uint64_t tick = TICKS;
	uint32_t retry_tick = TICKS;
	bool stalling = false;
	while ( getOpMode(spi) != opMode and ( opMode == RF_OPMODE_RECEIVER or TICKS - tick < 250 ) ) {
		usleep( 1 );
		if ( TICKS - tick > 100 ) {
			if ( not stalling ) {
				gDebug() << "setOpMode(" << opMode << ") stalling ! (!=" << getOpMode(spi) << ")";
			}
			stalling = true;
		}
		if ( TICKS - retry_tick > 100 ) {
			writeRegister( spi, REG_OPMODE, ( readRegister(spi, REG_OPMODE) & RF_OPMODE_MASK ) | opMode );
			retry_tick = TICKS;
		}
	}
	if ( stalling ) {
		gDebug() << "setOpMode(" << opMode << ") stalled (" << (TICKS - tick) << "ms) !";
	}

	if ( getOpMode(spi) != opMode ) {
		gDebug() << "ERROR : cannot set SX127x to opMode 0x" << hex << (int)opMode << " (0x" << (int)getOpMode(spi) << ")" << dec;
		return false;
	} else {
// 		gDebug() << "SX127x : opMode now set to 0x" << hex << (int)opMode << dec;
	}

	return true; //time < TIMEOUT;
}


bool SX127x::writeRegister( uint8_t address, uint8_t value )
{
	uint8_t tx[32];
	uint8_t rx[32];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = address | 0x80;
	tx[1] = value;

	mSPI->Transfer( tx, rx, 2 );
/*
	if ( address != REG_FIFO and address != REG_LR_IRQFLAGS ) {
		uint8_t ret = readRegister( address );
		if ( address == REG_RXCONFIG ) {
			value &= ~( RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK | RF_RXCONFIG_RESTARTRXWITHPLLLOCK );
		}
		if ( ret != value and address != REG_OPMODE and address != REG_IRQFLAGS1 ) {
			gDebug() << "Error while setting register " << GetRegName(address) << " to 0x" << hex << (int)value << " (0x" << (int)ret << ")" << dec;
			return false;
		}
	}
*/
	return true;
}

uint8_t SX127x::readRegister( uint8_t address )
{
	uint8_t tx[32];
	uint8_t rx[32];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = address & 0x7F;
	tx[1] = 0;

	mSPI->Transfer( tx, rx, 2 );
	return rx[1];
}


bool SX127x::writeRegister( SPI* spi, uint8_t address, uint8_t value )
{
	uint8_t tx[32];
	uint8_t rx[32];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = address | 0x80;
	tx[1] = value;

	spi->Transfer( tx, rx, 2 );
/*
	if ( address != REG_FIFO and address != REG_LR_IRQFLAGS ) {
		uint8_t ret = readRegister( spi, address );
		if ( address == REG_RXCONFIG ) {
			value &= ~( RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK | RF_RXCONFIG_RESTARTRXWITHPLLLOCK );
		}
		if ( ret != value and address != REG_OPMODE and address != REG_IRQFLAGS1 ) {
			gDebug() << "[" << spi->device() << "]Error while setting register " << GetRegName(address) << " to 0x" << hex << (int)value << " (0x" << (int)ret << ")" << dec;
			return false;
		}
	}
*/
	return true;
}


uint8_t SX127x::readRegister( SPI* spi, uint8_t address )
{
	uint8_t tx[32];
	uint8_t rx[32];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = address & 0x7F;
	tx[1] = 0;

	spi->Transfer( tx, rx, 2 );
	return rx[1];
}


string SX127x::name() const
{
	return "SX127x";
}


LuaValue SX127x::infos() const
{
	LuaValue ret;

	ret["Bus"] = mSPI->infos();
	ret["Frequency"] = mFrequency;
	ret["Bandwidth"] = mBandwidth;
	ret["Bandwidth Afc"] = mBandwidthAfc;
	ret["Fdev"] = mFdev;
	ret["Reset Pin"] = mResetPin;
	ret["IRQ Pin"] = mIRQPin;
	ret["LED Pin"] = mLedPin;
	ret["TX Pin"] = mTXPin;
	ret["RX Pin"] = mRXPin;
	ret["Blocking"] = mBlocking;
	if ( mDiversitySpi ) {
		ret["Diversity"] = LuaValue();
		ret["Diversity"]["Bus"] = mDiversitySpi->infos();
		ret["Diversity"]["Reset Pin"] = mDiversityResetPin;
		ret["Diversity"]["IRQ Pin"] = mDiversityIrqPin;
		ret["Diversity"]["LED Pin"] = mDiversityLedPin;
	}

	return ret;
}


static uint8_t GetFskBandwidthRegValue( uint32_t bandwidth )
{
	for( uint8_t i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ ) {
		if( ( bandwidth >= FskBandwidths[i].bandwidth ) and ( bandwidth < FskBandwidths[i + 1].bandwidth ) ) {
			return FskBandwidths[i].RegValue;
		}
	}

	return 0x01; // 250khz default
}


static const char* GetRegName( uint8_t reg )
{
	for ( uint32_t i = 0; regnames[i].reg != 0xFF; i++ ) {
		if ( regnames[i].reg == reg ) {
			return regnames[i].name;
		}
	}

	return "unk";
}


static uint8_t crc8( const uint8_t* buf, uint32_t len )
{
	uint8_t crc = 0x00;
	while (len--) {
		uint8_t extract = *buf++;
		for ( uint8_t tempI = 8; tempI; tempI--)  {
			uint8_t sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum) {
				crc ^= 0x8C;
			}
			extract >>= 1;
		}
	}
	return crc;
}
