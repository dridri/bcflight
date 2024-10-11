#include "SmartAudio.h"
#include "Debug.h"
#include <Board.h>
#include <GPIO.h>

#define CMD_GET_SETTINGS	0x01
#define CMD_SET_POWER		0x02
#define CMD_SET_CHANNEL		0x03
#define CMD_SET_FREQUENCY	0x04
#define CMD_SET_MODE		0x05

// TODO : run in separate thread


typedef struct
{
	uint16_t header;
	uint8_t command;
	uint8_t length;
} __attribute__((packed)) SmartAudioCommand;

typedef struct {
	std::string name;
	uint16_t frequencies[8];
} Band;

static const std::vector< Band > vtxBandsFrequencies = {
	{ "Boscam A", { 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725 } },
	{ "Boscam B", { 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866 } },
	{ "Boscam E", { 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945 } },
	{ "FatShark", { 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880 } },
	{ "RaceBand", { 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 } },
};

static uint8_t crc8( const uint8_t* ptr, uint8_t len );


SmartAudio::SmartAudio( Serial* bus, uint8_t txpin, bool frequency_cmd_supported )
	: mBus( bus )
	, mTXPin( txpin )
	, mSetFrequencySupported( frequency_cmd_supported )
	, mTXStopBits( 2 )
	, mRXStopBits( 1 )
	, mLastCommandTick( 0 )
	, mVersion( 1 )
	, mFrequency( 0 )
	, mPower( 0 )
	, mChannel( 0 )
	, mBand( 0 )
{
}


SmartAudio::SmartAudio()
	: SmartAudio( nullptr, 0, false )
{
}


SmartAudio::~SmartAudio()
{
}


void SmartAudio::setStopBits( uint8_t tx, uint8_t rx )
{
	mTXStopBits = tx;
	mRXStopBits = rx;
}


void SmartAudio::Connect()
{
	mBus->Connect();
	Update();
}


int SmartAudio::SendCommand( uint8_t cmd_code, const uint8_t* data, const uint8_t datalen )
{
	uint64_t tick = Board::GetTicks();
	uint64_t diff = tick - mLastCommandTick;
	if ( diff < 200 * 1000 ) {
		usleep( 200*1000 - diff );
	}

	int ret = 0;

	uint8_t command[32] = { 0 };
	command[0] = 0x00;
	SmartAudioCommand* cmd = (SmartAudioCommand*)( command + 1 );
	cmd->header = 0x55AA;
	cmd->command = ( cmd_code << 1 ) + 1;
	cmd->length = datalen;
	memcpy( command + 1 + sizeof(SmartAudioCommand), data, datalen );
	uint8_t crc = crc8( (uint8_t*)cmd, sizeof(SmartAudioCommand) + datalen );
	command[ 1 + sizeof(SmartAudioCommand) + datalen ] = crc;

	// Enter TX mode
	mBus->setStopBits( mTXStopBits );
	GPIO::setMode( mTXPin, GPIO::Alt4 );

	// Send bytes
	ret = mBus->Write( command, 1 + sizeof(SmartAudioCommand) + datalen + 1 );

	// // Leave TX mode
	usleep( 1000 * 16 );
	GPIO::setMode( mTXPin, GPIO::Input );
	mBus->setStopBits( mRXStopBits );

	// Read self echo
	uint8_t dummy[16] = { 0x00 };
	uint8_t readi = 0;
	uint16_t ret2 = 0;
	do {
		int r = mBus->Read( dummy + readi, 1 );
		if ( r <= 0 ) {
			return r;
		}
		ret2 += r;
		readi++;
	} while ( dummy[readi-1] != crc && ret2 < 16);

	mLastCommandTick = Board::GetTicks();
	return ret;
}


void SmartAudio::Update()
{
	fDebug();

	SendCommand( CMD_GET_SETTINGS, nullptr, 0 );

	uint8_t response[ 32 ] = { 0 };
	int sz = mBus->Read( response, sizeof(response) );

	SmartAudioCommand* resp = (SmartAudioCommand*)response;
	if ( resp->header != 0x55AA or ( resp->command & 0b00000111 ) != CMD_GET_SETTINGS ) {
		gDebug() << "SmartAudio: invalid response (header=" << resp->header << ", command=" << resp->command << ")";
		return;
	}

	mVersion = resp->command >> 3;
	mChannel = response[ sizeof(SmartAudioCommand) ];
	mPower = response[ sizeof(SmartAudioCommand) + 1 ];
	uint8_t mode = response[ sizeof(SmartAudioCommand) + 2 ];
	uint8_t freq_mode = mode & 0b00000001;
	mFrequency = ( response[ sizeof(SmartAudioCommand) + 3 ] << 8 ) + response[ sizeof(SmartAudioCommand) + 4 ];

	mBand = mChannel / 8;
	if ( freq_mode == 0 ) {
		mFrequency = vtxBandsFrequencies[mBand].frequencies[mChannel % 8];
	}

	// Version 3
	if ( resp->command == 0x11 && resp->length > 10 ) {
		uint8_t maxPowers = ((uint8_t*)resp)[10];
		std::vector<int32_t> powers;
		for ( uint8_t i = 0; i <= maxPowers && i + 11 <= resp->length + 2; i++ ) {
			uint8_t power = ((uint8_t*)resp)[11 + i];
			powers.push_back( power );
		}
		mPowerTable = powers;
	}

	gDebug() << "SmartAudio Update :";
	gDebug() << "    version : " << (int)mVersion;
	gDebug() << "    mode : " << (int)mode;
	gDebug() << "    channel : " << (int)mChannel;
	gDebug() << "    power : " << (int)mPower;
	gDebug() << "    frequency : " << (int)mFrequency << " MHz";
	gDebug() << "    band : " << vtxBandsFrequencies[mBand].name << "(" << mBand << ")";
	if ( mPowerTable.size() > 0 ) {
		char powers[128] = "";
		for ( uint8_t i = 0; i < mPowerTable.size(); i++ ) {
			char p[16] = "";
			sprintf( p, "%s [%d]=%ddBm", i == 0 ? "" : ",", i, mPowerTable[i] );
			strcat( powers, p );
		}
		gDebug() << "    available powers : {" << powers << " }";
	}
}


void SmartAudio::setFrequency( uint16_t frequency )
{
	fDebug( (int)frequency );

	if ( not mSetFrequencySupported ) {
		setChannel( channelFromFrequency( frequency ) );
		return;
	}

	uint8_t data[2] = { 0 };
	data[0] = ( frequency >> 8 ) & 0b00111111;
	data[1] = frequency & 0xFF;
	SendCommand( CMD_SET_FREQUENCY, data, 2 );

	uint8_t response[ 32 ] = { 0 };
	int sz = mBus->Read( response, sizeof(response) );
	if ( sz <= 0 ) {
		gWarning() << "Setting frequency failed, no response from VTX";
		return;
	}

	mFrequency = ( response[4] << 8 ) + response[5];
	mChannel = channelFromFrequency( mFrequency );
	mBand = mChannel / 8;

	gDebug() << "VTX frequency now set to " << (int)mFrequency;
	gDebug() << "VTX band now set to : " << vtxBandsFrequencies[mBand].name << " (" << (int)mBand << ")";
	gDebug() << "VTX channel now set to " << (int)mChannel;
}


void SmartAudio::setPower( uint8_t power )
{
	fDebug( (int)power );

	uint8_t data[1] = { power };
	SendCommand( CMD_SET_POWER, data, 1 );

	uint8_t response[ 32 ] = { 0 };
	int sz = mBus->Read( response, sizeof(response) );
	if ( sz <= 0 ) {
		gWarning() << "Setting power failed, no response from VTX";
		return;
	}

	mPower = response[4];

	if ( mPower < mPowerTable.size() ) {
		gDebug() << "VTX power now set to " << (int)mPowerTable[mPower] << " dBm";
	}
}


void SmartAudio::setChannel( uint8_t channel )
{
	fDebug( (int)channel );

	uint8_t data[1] = { channel };
	SendCommand( CMD_SET_CHANNEL, data, 1 );

	uint8_t response[ 32 ] = { 0 };
	int sz = mBus->Read( response, sizeof(response) );
	if ( sz <= 0 ) {
		gWarning() << "Setting channel failed, no response from VTX";
		return;
	}

	mChannel = response[4];
	mBand = mChannel / 8;
	mFrequency = vtxBandsFrequencies[mBand].frequencies[mChannel % 8];

	gDebug() << "VTX channel now set to " << (int)mChannel;
	gDebug() << "VTX band now set to : " << vtxBandsFrequencies[mBand].name << " (" << (int)mBand << ")";
	gDebug() << "VTX frequency now set to " << (int)mFrequency;
}


void SmartAudio::setMode( uint8_t mode )
{
	fDebug( (int)mode );

	uint8_t data[1] = { mode };
	SendCommand( CMD_SET_MODE, data, 1 );

	uint8_t response[ 32 ] = { 0 };
	int sz = mBus->Read( response, sizeof(response) );
	if ( sz <= 0 ) {
		gWarning() << "Setting mode failed, no response from VTX";
		return;
	}

	mode = response[4];

	gDebug() << "VTX mode now set to " << (int)mode;
}


int SmartAudio::getFrequency() const
{
	return mFrequency;
}


int SmartAudio::getPower() const
{
	return mPower;
}


int SmartAudio::getPowerDbm() const
{
	if ( mPower < mPowerTable.size() ) {
		return (int)mPowerTable[mPower];
	}
	return -1;
}


int SmartAudio::getChannel() const
{
	return mChannel;
}


int SmartAudio::getBand() const
{
	return mBand;
}


std::string SmartAudio::getBandName() const
{
	return std::string( vtxBandsFrequencies[mBand].name );
}


std::vector<int> SmartAudio::getPowerTable() const
{
	return mPowerTable;
}


int8_t SmartAudio::channelFromFrequency( uint16_t frequency )
{
	for ( int band = 0; band < 5; band++ ) {
		for ( int freq = 0; freq < 8; freq++ ) {
			if ( vtxBandsFrequencies[band].frequencies[freq] == frequency ) {
				return band * 8 + freq;
			}
		}
	}

	return -1;
}


static const uint8_t crc8tab[256] = {
	0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
	0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
	0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
	0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
	0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
	0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
	0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
	0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
	0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
	0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
	0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
	0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
	0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
	0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
	0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
	0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

static uint8_t crc8( const uint8_t* ptr, uint8_t len )
{
	uint8_t crc = 0;

	for ( uint8_t i = 0; i < len; i++ ) {
		crc = crc8tab[crc ^ *ptr++];
	}

	return crc;
}
