#pragma once

#include <stdint.h>
#include "Serial.h"


LUA_CLASS class SmartAudio
{
public:
	LUA_EXPORT SmartAudio( Serial* bus, uint8_t txpin, bool frequency_cmd_supported = false );
	~SmartAudio();

	void Setup();
	void Update();

	void setFrequency( uint16_t frequency );
	void setPower( uint8_t power );
	void setChannel( uint8_t channel );
	void setMode( uint8_t mode );

	uint16_t getFrequency();
	uint8_t getPower();
	uint8_t getChannel();
	uint8_t getBand();

private:
	LUA_PROPERTY("bus") Serial* mBus;
	LUA_PROPERTY("tx_pin") uint8_t mTXPin;
	LUA_PROPERTY("frequency_cmd_supported") bool mSetFrequencySupported;
	LUA_PROPERTY("tx_stop_bits") uint8_t mTXStopBits;
	LUA_PROPERTY("rx_stop_bits") uint8_t mRXStopBits;
	uint64_t mLastCommandTick;
	uint8_t mVersion;
	uint16_t mFrequency;
	uint8_t mPower;
	uint8_t mChannel;
	uint8_t mBand;
	std::vector<uint8_t> mPowerTable;

	int SendCommand( uint8_t cmd_code, const uint8_t* data, const uint8_t datalen );
	int8_t channelFromFrequency( uint16_t freq );
};
