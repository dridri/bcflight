#pragma once

#include <stdint.h>
#include "Serial.h"


LUA_CLASS class SmartAudio
{
public:
	SmartAudio( Serial* bus, uint8_t txpin, bool frequency_cmd_supported = false );
	LUA_EXPORT SmartAudio();
	~SmartAudio();

	void setStopBits( uint8_t tx, uint8_t rx );

	LUA_EXPORT void Connect();
	LUA_EXPORT int Update();

	LUA_EXPORT void setFrequency( uint16_t frequency );
	LUA_EXPORT void setPower( uint8_t power );
	LUA_EXPORT void setChannel( uint8_t channel );
	LUA_EXPORT void setMode( uint8_t mode );

	LUA_EXPORT int getFrequency() const;
	LUA_EXPORT int getPower() const;
	LUA_EXPORT int getPowerDbm() const;
	LUA_EXPORT int getChannel() const;
	LUA_EXPORT int getBand() const;
	LUA_EXPORT std::string getBandName() const;
	LUA_EXPORT std::vector<int32_t> getPowerTable() const;

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
	std::vector<int32_t> mPowerTable;

	int UpdateInternal();

	int SendCommand( uint8_t cmd_code, const uint8_t* data, const uint8_t datalen );
	int8_t channelFromFrequency( uint16_t freq );
};
