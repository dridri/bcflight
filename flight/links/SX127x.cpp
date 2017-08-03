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

#define TICKS ( Board::GetTicks() / 1000 )
#define TIMEOUT 1000 // 1000 ms
#define XTAL_FREQ 32000000
#define FREQ_STEP 61.03515625f
#define PACKET_SIZE 32

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


int SX127x::flight_register( Main* main )
{
	RegisterLink( "SX127x", &SX127x::Instanciate );
	return 0;
}


Link* SX127x::Instanciate( Config* config, const std::string& lua_object )
{
	return new SX127x( config, lua_object );
}


SX127x::SX127x( Config* config, const std::string& lua_object )
	: Link()
	, mDevice( config->string( lua_object + ".device", "/dev/spidev0.0" ) )
	, mResetPin( config->integer( lua_object + ".resetpin", -1 ) )
	, mTXPin( config->integer( lua_object + ".txpin", -1 ) )
	, mRXPin( config->integer( lua_object + ".rxpin", -1 ) )
	, mIRQPin( config->integer( lua_object + ".irqpin", -1 ) )
	, mBlocking( config->boolean( lua_object + ".blocking", true ) )
	, mDropBroken( config->boolean( lua_object + ".drop", true ) )
	, mModem( config->string( lua_object + ".modem", "FSK" ) == "LoRa" ? LoRa : FSK )
	, mFrequency( config->integer( lua_object + ".frequency", 433000000 ) )
	, mInputPort( config->integer( lua_object + ".input_port", 0 ) )
	, mOutputPort( config->integer( lua_object + ".output_port", 1 ) )
	, mRetries( config->integer( lua_object + ".retries", 1 ) )
	, mReadTimeout( config->integer( lua_object + ".timeout", 2000 ) )
	, mBitrate( config->integer( lua_object + ".bitrate", 76800 ) )
	, mBandwidth( config->integer( lua_object + ".bandwidth", 250000 ) )
	, mFdev( config->integer( lua_object + ".fdev", 200000 ) )
	, mRSSI( 0 )
	, mRxQuality( 0 )
	, mPerfTicks( 0 )
	, mPerfLastRxBlock( 0 )
	, mPerfValidBlocks( 0 )
	, mPerfInvalidBlocks( 0 )
	, mTXBlockID( 0 )
{
	if ( mResetPin < 0 ) {
		gDebug() << "WARNING : No Reset-pin specified for SX127x, cannot create link !\n";
		return;
	}
	if ( mIRQPin < 0 ) {
		gDebug() << "WARNING : No IRQ-pin specified for SX127x, cannot create link !\n";
		return;
	}

	memset( &mRxBlock, 0, sizeof(mRxBlock) );

	mSPI = new SPI( mDevice, 8000000 );


	GPIO::setMode( mResetPin, GPIO::Output );
	GPIO::Write( mResetPin, false );

	GPIO::setMode( mIRQPin, GPIO::Input );
	GPIO::SetupInterrupt( mIRQPin, GPIO::Rising, [this](){ this->Interrupt(); } );

	if ( mTXPin >= 0 ) {
		GPIO::Write( mTXPin, false );
		GPIO::setMode( mTXPin, GPIO::Output );
	}

	if ( mRXPin >= 0 ) {
		GPIO::Write( mRXPin, false );
		GPIO::setMode( mRXPin, GPIO::Output );
	}
}


SX127x::~SX127x()
{
}


int SX127x::Connect()
{
	// Reset chip
	reset();

	if ( not ping() ) {
		Board::defectivePeripherals()["SX127x"] = true;
		return -1;
	}

	// Cut the PA just in case, RFO output, power = -1 dBm
	writeRegister( REG_PACONFIG, 0x00 );
	{
		// Start calibration in LF band
		writeRegister ( REG_IMAGECAL, ( readRegister( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
		while ( ( readRegister( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING ) {
			usleep( 10 );
		}
	}
	{
		// Set frequency to mid-HF band
		setFrequency( 868000000 );
		// Start calibration in HF band
		writeRegister( REG_IMAGECAL, ( readRegister( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
		while ( ( readRegister( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING ) {
			usleep( 10 );
		}
	}

	// Start module in sleep mode
	setOpMode( RF_OPMODE_SLEEP );

	// Set frequency
	setFrequency( mFrequency );

	bool boosted_module_20dbm = false;

	// Set PA to maximum => TODO : make it more configurable
	writeRegister( REG_PACONFIG, ( boosted_module_20dbm ? RF_PACONFIG_PASELECT_PABOOST : RF_PACONFIG_PASELECT_RFO ) | 0x70 | 0xF );
	writeRegister( REG_PADAC, 0x80 | ( boosted_module_20dbm ? RF_PADAC_20DBM_ON : RF_PADAC_20DBM_OFF ) );
	writeRegister( REG_OCP, RF_OCP_TRIM_240_MA | RF_OCP_OFF );
	writeRegister( REG_PARAMP, RF_PARAMP_SHAPING_NONE | RF_PARAMP_0040_US | RF_PARAMP_LOWPNTXPLL_ON );

	// Set LNA
	writeRegister( REG_LNA, RF_LNA_GAIN_G1 | RF_LNA_BOOST_ON );

	// Set RX/Sync config
	writeRegister( REG_RXCONFIG, RF_RXCONFIG_RESTARTRXONCOLLISION_OFF | RF_RXCONFIG_AGCAUTO_OFF );
	writeRegister( REG_SYNCCONFIG, RF_SYNCCONFIG_SYNC_ON | RF_SYNCCONFIG_AUTORESTARTRXMODE_WAITPLL_ON | RF_SYNCCONFIG_PREAMBLEPOLARITY_AA | RF_SYNCCONFIG_SYNCSIZE_4 );

	// Set RSSI smoothing
	writeRegister( REG_RSSICONFIG, RF_RSSICONFIG_SMOOTHING_32 );
// 	writeRegister( REG_RSSICOLLISION, 40 );

	// Set modem to FSK mode
	setModem( mModem );

	TxConfig_t txconf = {
		.fdev = mFdev, // Hz
		.bandwidth = mBandwidth, // Hz
		.datarate = mBitrate, // bps
		.coderate = 1, // only for LoRa
		.preambleLen = 2,
		.crcOn = mDropBroken,
		.freqHopOn = false,
		.hopPeriod = 0,
		.iqInverted = false,
		.timeout = 2000,
	};
	SetupTX( txconf );

	RxConfig_t rxconf = {
		.bandwidth = txconf.bandwidth, // Hz
		.datarate = txconf.datarate, // bps
		.coderate = txconf.coderate, // only for LoRa
		.bandwidthAfc = txconf.bandwidth, // Hz
		.preambleLen = txconf.preambleLen,
		.symbTimeout = 0, // only for LoRa
		.payloadLen = PACKET_SIZE,
		.crcOn = mDropBroken,
		.freqHopOn = false,
		.hopPeriod = 0,
		.iqInverted = false,
	};
	SetupRX( rxconf );

	gDebug() << "Module online : " << ping() << "\n";
	gDebug() << "Frequency : " << (uint32_t)((float)(((uint32_t)readRegister(REG_FRFMSB) << 16 ) | ((uint32_t)readRegister(REG_FRFMID) << 8 ) | ((uint32_t)readRegister(REG_FRFLSB) )) * FREQ_STEP) << "Hz\n";
	gDebug() << "PACONFIG : 0x" << std::hex << (int)readRegister( REG_PACONFIG ) << "\n";
	gDebug() << "PADAC : 0x" << std::hex << (int)readRegister( REG_PADAC ) << "\n";
	gDebug() << "PARAMP : 0x" << std::hex << (int)readRegister( REG_PARAMP ) << "\n";
	gDebug() << "OCP : 0x" << std::hex << (int)readRegister( REG_OCP ) << "\n";
	gDebug() << "Modem : " << ( ( readRegister( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) ? "LoRa" : "FSK" ) << "\n";

	startReceiving();

	mConnected = true;
	return 0;
}


void SX127x::SetupTX( const TxConfig_t& conf )
{
	switch( mModem )
	{
		case FSK :
		{
			uint32_t fdev = ( uint16_t )( ( double )conf.fdev / ( double )FREQ_STEP );
			writeRegister( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
			writeRegister( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

			uint32_t datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )conf.datarate );
			writeRegister( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
			writeRegister( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

			writeRegister( REG_PREAMBLEMSB, ( conf.preambleLen >> 8 ) & 0x00FF );
			writeRegister( REG_PREAMBLELSB, conf.preambleLen & 0xFF );

			writeRegister( REG_PACKETCONFIG1, ( readRegister( REG_PACKETCONFIG1 ) & RF_PACKETCONFIG1_CRC_MASK & RF_PACKETCONFIG1_PACKETFORMAT_MASK ) | RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE | ( conf.crcOn << 4 ) );
			writeRegister( REG_PACKETCONFIG2, ( readRegister( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
		}
		break;
		case LoRa :
		{
			uint32_t bandwidth = conf.bandwidth;
			if ( bandwidth == 125000 ) {
				bandwidth = 0;
			} else if ( bandwidth == 250000 ) {
				bandwidth = 1;
			} else if ( bandwidth == 500000 ) {
				bandwidth = 2;
			} else {
				gDebug() << "ERROR: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported !\n";
				return;
			}
			bandwidth = conf.bandwidth + 7;
			uint32_t datarate = std::max( 6u, std::min( 12u, conf.datarate ) );
			uint32_t lowDatarateOptimize = ( ( ( bandwidth == 7 ) and ( ( datarate == 11 ) or ( datarate == 12 ) ) ) or ( ( bandwidth == 8 ) and ( datarate == 12 ) ) );

			if( conf.freqHopOn == true ) {
				writeRegister( REG_LR_PLLHOP, ( readRegister( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
				writeRegister( REG_LR_HOPPERIOD, conf.hopPeriod );
			}

			writeRegister( REG_LR_MODEMCONFIG1, ( readRegister( REG_LR_MODEMCONFIG1 ) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( bandwidth << 4 ) | ( conf.coderate << 1 ) | RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
			writeRegister( REG_LR_MODEMCONFIG2, ( readRegister( REG_LR_MODEMCONFIG2 ) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( datarate << 4 ) | ( conf.crcOn << 2 ) );
			writeRegister( REG_LR_MODEMCONFIG3, ( readRegister( REG_LR_MODEMCONFIG3 ) & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( lowDatarateOptimize << 3 ) );

			writeRegister( REG_LR_PREAMBLEMSB, ( conf.preambleLen >> 8 ) & 0x00FF );
			writeRegister( REG_LR_PREAMBLELSB, conf.preambleLen & 0xFF );

			if( datarate == 6 ) {
				writeRegister( REG_LR_DETECTOPTIMIZE, ( readRegister( REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF6 );
				writeRegister( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6 );
			} else {
				writeRegister( REG_LR_DETECTOPTIMIZE, ( readRegister( REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
				writeRegister( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
			}
		}
		break;
	}
}


void SX127x::SetupRX( const RxConfig_t& conf )
{
	switch( mModem )
	{
		case FSK :
		{
			uint32_t datarate = ( uint32_t )( ( double )XTAL_FREQ / ( double )conf.datarate );
			writeRegister( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
			writeRegister( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

			writeRegister( REG_RXBW, GetFskBandwidthRegValue( conf.bandwidth ) );
			writeRegister( REG_AFCBW, GetFskBandwidthRegValue( conf.bandwidthAfc ) );

			writeRegister( REG_PREAMBLEMSB, ( uint8_t )( ( conf.preambleLen >> 8 ) & 0xFF ) );
			writeRegister( REG_PREAMBLELSB, ( uint8_t )( conf.preambleLen & 0xFF ) );

// 			writeRegister( REG_PAYLOADLENGTH, conf.payloadLen );
			writeRegister( REG_PAYLOADLENGTH, 0xFF );

			writeRegister( REG_PACKETCONFIG1, ( readRegister( REG_PACKETCONFIG1 ) & RF_PACKETCONFIG1_CRC_MASK & RF_PACKETCONFIG1_PACKETFORMAT_MASK ) | RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE | ( conf.crcOn << 4 ) );
			writeRegister( REG_PACKETCONFIG2, ( readRegister( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
		}
		break;
		case LoRa :
		{
			uint32_t bandwidth = conf.bandwidth;
			if ( bandwidth == 125000 ) {
				bandwidth = 0;
			} else if ( bandwidth == 250000 ) {
				bandwidth = 1;
			} else if ( bandwidth == 500000 ) {
				bandwidth = 2;
			} else {
				gDebug() << "ERROR: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported !\n";
				return;
			}
			bandwidth = conf.bandwidth + 7;
			uint32_t datarate = std::max( 6u, std::min( 12u, conf.datarate ) );
			uint32_t lowDatarateOptimize = ( ( ( bandwidth == 7 ) and ( ( datarate == 11 ) or ( datarate == 12 ) ) ) or ( ( bandwidth == 8 ) and ( datarate == 12 ) ) );

			writeRegister( REG_LR_MODEMCONFIG1, ( readRegister( REG_LR_MODEMCONFIG1 ) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( bandwidth << 4 ) | ( conf.coderate << 1 ) | RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
			writeRegister( REG_LR_MODEMCONFIG2, ( readRegister( REG_LR_MODEMCONFIG2 ) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( datarate << 4 ) | ( conf.crcOn << 2 ) | ( ( conf.symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );
			writeRegister( REG_LR_MODEMCONFIG3, ( readRegister( REG_LR_MODEMCONFIG3 ) & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( lowDatarateOptimize << 3 ) );

			writeRegister( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( conf.symbTimeout & 0xFF ) );

			writeRegister( REG_LR_PREAMBLEMSB, ( uint8_t )( ( conf.preambleLen >> 8 ) & 0xFF ) );
			writeRegister( REG_LR_PREAMBLELSB, ( uint8_t )( conf.preambleLen & 0xFF ) );

// 			writeRegister( REG_PAYLOADLENGTH, conf.payloadLen );
			writeRegister( REG_PAYLOADLENGTH, 0xFF );

			if( conf.freqHopOn == true ) {
				writeRegister( REG_LR_PLLHOP, ( readRegister( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
				writeRegister( REG_LR_HOPPERIOD, conf.hopPeriod );
			}

			if( ( bandwidth == 9 ) and ( mFrequency > 525000000 ) ) {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				writeRegister( REG_LR_TEST36, 0x02 );
				writeRegister( REG_LR_TEST3A, 0x64 );
			} else if( bandwidth == 9 ) {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				writeRegister( REG_LR_TEST36, 0x02 );
				writeRegister( REG_LR_TEST3A, 0x7F );
			} else {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				writeRegister( REG_LR_TEST36, 0x03 );
			}

			if( datarate == 6 ) {
				writeRegister( REG_LR_DETECTOPTIMIZE, ( readRegister( REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF6 );
				writeRegister( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6 );
			} else {
				writeRegister( REG_LR_DETECTOPTIMIZE, ( readRegister( REG_LR_DETECTOPTIMIZE ) & RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
				writeRegister( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
			}
		}
		break;
	}
}


void SX127x::setFrequency( float f )
{
	uint32_t freq = (uint32_t)( (float)f / FREQ_STEP );
	writeRegister( REG_FRFMSB, (uint8_t)( (freq >> 16) & 0xFF ) );
	writeRegister( REG_FRFMID, (uint8_t)( (freq >> 8) & 0xFF ) );
	writeRegister( REG_FRFLSB, (uint8_t)( freq & 0xFF ) );
}


void SX127x::startReceiving()
{
	mSending = false;
	mSendingEnd = false;

	if ( mTXPin >= 0 ) {
		GPIO::Write( mTXPin, false );
	}
	if ( mRXPin >= 0 ) {
		GPIO::Write( mRXPin, true );
	}

	setOpMode( RF_OPMODE_RECEIVER );
	writeRegister( REG_LNA, RF_LNA_GAIN_G1 | RF_LNA_BOOST_ON );
}


void SX127x::startTransmitting()
{
	if ( mRXPin >= 0 ) {
		GPIO::Write( mRXPin, false );
	}
	if ( mTXPin >= 0 ) {
		GPIO::Write( mTXPin, true );
	}
	setOpMode( RF_OPMODE_TRANSMITTER );
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
	mPerfMutex.lock();
	if ( TICKS - mPerfTicks >= 250 ) {
		int32_t count = mRxBlock.block_id - mPerfLastRxBlock;
		if ( count == 0 or count == 1 ) {
			mRxQuality = 0;
		} else {
			if ( count < 0 ) {
				count = 255 + count;
			}
			mRxQuality = 100 * ( mPerfValidBlocks * 4 + mPerfInvalidBlocks ) / ( count * 4 );
			if ( mRxQuality > 100 ) {
				mRxQuality = 100;
			}
		}
		mPerfTicks = TICKS;
		mPerfLastRxBlock = mRxBlock.block_id;
		mPerfValidBlocks = 0;
		mPerfInvalidBlocks = 0;
	}
	mPerfMutex.unlock();
}


int SX127x::Read( void* pRet, uint32_t len, int32_t timeout )
{
	bool timedout = false;
	uint64_t started_waiting_at = TICKS;
	if ( timeout == 0 ) {
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
		usleep( 100 );
	} while ( mBlocking );

	mRxQueueMutex.lock();
	if ( mRxQueue.size() == 0 ) {
		mRxQueueMutex.unlock();
		if ( timedout ) {
			return LINK_ERROR_TIMEOUT;
		}
		return 0;
	}

	std::pair< uint8_t*, uint32_t > data = mRxQueue.front();
	mRxQueue.pop_front();
	mRxQueueMutex.unlock();
	memcpy( pRet, data.first, data.second );
	delete data.first;
	return data.second;
}


int SX127x::Write( const void* data, uint32_t len, bool ack, int32_t timeout )
{
// 	while ( mSending ) {
// 		usleep( 10 );
// 	}
	mSending = true;

	uint8_t buf[32];
	memset( buf, 0, sizeof(buf) );
	Header* header = (Header*)buf;

	header->block_id = ++mTXBlockID;
	header->packets_count = (uint8_t)std::ceil( (float)len / (float)( 32 - sizeof(Header) ) );

	uint32_t offset = 0;
	for ( uint8_t packet = 0; packet < header->packets_count; packet++ ) {
		uint32_t plen = 32 - sizeof(Header);
		if ( offset + plen > len ) {
			plen = len - offset;
		}

		memcpy( buf + sizeof(Header), (uint8_t*)data + offset, plen );

// 		for ( int32_t retry = 0; retry < mRetries; retry++ )
		{
// 			while ( mSending ) {
// 				usleep( 1 );
// 			}
			if ( not writeRegister( REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTARTCONDITION_FIFOTHRESH | ( plen + sizeof(Header) ) ) ) {
				return 0;
			}


			uint8_t tx[64];
			uint8_t rx[64];
			memset( tx, 0, sizeof(tx) );
			memset( rx, 0, sizeof(rx) );

			tx[0] = REG_FIFO | 0x80;
			tx[1] = plen + sizeof(Header);
			memcpy( &tx[2], buf, plen + sizeof(Header) );

			mSendingEnd = false;

// 			printf( "Sending [%u] { %d %d %d } [%d bytes]\n", plen + sizeof(Header), header->block_id, header->packet_id, header->packets_count, plen );

			mSendingEnd = true;
			mSendTime = TICKS;
			startTransmitting();
			mSPI->Transfer( tx, rx, plen + sizeof(Header) + 2 );
		}

		header->packet_id++;
		offset += plen;
	}

	return len;
}


int32_t SX127x::WriteAck( const void* buf, uint32_t len )
{
	return 0;
}


uint32_t SX127x::fullReadSpeed()
{
	return 0;
}


int SX127x::Receive( uint8_t* buf, uint32_t buflen, void* pRet, uint32_t len )
{
	Header* header = (Header*)buf;
	uint8_t* data = buf + sizeof(Header);
	int final_size = 0;
	uint32_t datalen = buflen - sizeof(Header);

	if ( header->block_id == mRxBlock.block_id and mRxBlock.received ) {
		return -1;
	}

	if ( header->block_id != mRxBlock.block_id ) {
		memset( &mRxBlock, 0, sizeof(mRxBlock) );
		mRxBlock.block_id = header->block_id;
		mRxBlock.packets_count = header->packets_count;
	}

// 		printf( "Received [%d] { %d %d %d } [%d bytes]\n", buflen, header->block_id, header->packet_id, header->packets_count, datalen );

	if ( header->packets_count == 1 ) {
// 		printf( "small block\n" );
		mRxBlock.block_id = header->block_id;
		mRxBlock.received = true;
		mPerfValidBlocks++;
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
		return final_size;
	}

	return final_size;
}


void SX127x::Interrupt()
{
	uint8_t opMode = getOpMode();
	if ( opMode == RF_OPMODE_TRANSMITTER or opMode == RF_OPMODE_SYNTHESIZER_TX ) {
// 		gDebug() << "SX127x::Interrupt() SendTime : " << std::dec << TICKS - mSendTime << "\n";
		if ( mSendingEnd ) {
			startReceiving();
		}
		mSending = false;
		return;
	}
// 	gDebug() << "SX127x::Interrupt()\n";

	mRSSI = -( readRegister( REG_RSSIVALUE ) >> 1 );

	if( mDropBroken ) {
		uint8_t irqFlags = readRegister( REG_IRQFLAGS2 );
		if( ( irqFlags & RF_IRQFLAGS2_CRCOK ) != RF_IRQFLAGS2_CRCOK ) {
			// Clear Irqs
			writeRegister( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | RF_IRQFLAGS1_PREAMBLEDETECT | RF_IRQFLAGS1_SYNCADDRESSMATCH );
			writeRegister( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );
			// Continuous mode restart Rx chain
			writeRegister( REG_RXCONFIG, readRegister( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
			return;
		}
	}

	uint8_t tx[64];
	uint8_t rx[64];
	memset( tx, 0, sizeof(tx) );
	memset( rx, 0, sizeof(rx) );
	tx[0] = REG_FIFO & 0x7f;

	uint8_t len = readRegister( REG_FIFO );
	mSPI->Transfer( tx, rx, len + 1 );

	if ( len > 0 ) {
		PerfUpdate();
		uint8_t* buf = new uint8_t[32768];
		int ret = Receive( rx + 1, len, buf, 32768 );
		if ( ret > 0 ) {
			mRxQueueMutex.lock();
			mRxQueue.emplace_back( std::make_pair( buf, ret ) );
			mRxQueueMutex.unlock();
		} else {
			delete[] buf;
		}
		/*
		uint8_t* buf = new uint8_t[len];
		memcpy( buf, rx + 1, len );
		mRxQueueMutex.lock();
		mRxQueue.emplace_back( std::make_pair( buf, len ) );
		mRxQueueMutex.unlock();
		*/
	}

	writeRegister( REG_RXCONFIG, readRegister( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
}


void SX127x::reset()
{
	GPIO::Write( mResetPin, false );
	usleep( 1000 * 50 );
	GPIO::Write( mResetPin, true );
}


bool SX127x::ping()
{
	return readRegister(REG_VERSION) == SAMTEC_ID;
}


void SX127x::setModem( Modem modem )
{
	switch( modem ) {
		default:
		case FSK:
			setOpMode( RF_OPMODE_SLEEP );
			writeRegister( REG_OPMODE, ( readRegister(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );
			break;

		case LoRa:
			setOpMode( RF_OPMODE_SLEEP );
			writeRegister( REG_OPMODE, ( readRegister(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );
			break;
	}

	writeRegister( REG_DIOMAPPING1, ( readRegister( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK ) | RF_DIOMAPPING1_DIO0_00 );
	writeRegister( REG_DIOMAPPING2, ( readRegister( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_MAP_MASK ) | RF_DIOMAPPING2_MAP_PREAMBLEDETECT );
}


uint32_t SX127x::getOpMode()
{
	return readRegister(REG_OPMODE) & ~RF_OPMODE_MASK;
}


bool SX127x::setOpMode( uint32_t opMode )
{
	writeRegister( REG_OPMODE, ( readRegister(REG_OPMODE) & RF_OPMODE_MASK ) | opMode );
	
	uint64_t tick = TICKS;
	bool stalling = false;
	while ( getOpMode() != opMode and TICKS - tick < 250 ) {
		usleep( 1 );
		if ( TICKS - tick > 100 ) {
			if ( not stalling ) {
				gDebug() << "setOpMode(" << opMode << ") stalling !\n";
			}
			stalling = true;
		}
	}
	if ( stalling ) {
		gDebug() << "setOpMode(" << opMode << ") stalled (" << std::to_string(TICKS - tick) << "ms) !\n";
	}

	if ( getOpMode() != opMode ) {
		gDebug() << "ERROR : cannot set SX127x to opMode 0x" << std::hex << (int)opMode << " (0x" << (int)getOpMode() << ")" << std::dec << "\n";
		return false;
	} else {
// 		gDebug() << "SX127x : opMode now set to 0x" << std::hex << (int)opMode << std::dec << "\n";
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

	if ( address != REG_FIFO ) {
		uint8_t ret = readRegister( address );
		if ( address == REG_RXCONFIG ) {
			value &= ~( RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK | RF_RXCONFIG_RESTARTRXWITHPLLLOCK );
		}
		if ( ret != value and address != REG_OPMODE ) {
			gDebug() << "Error while setting register " << GetRegName(address) << " to 0x" << std::hex << (int)value << " (0x" << (int)ret << ")" << std::dec << "\n";
			return false;
		}
	}

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
