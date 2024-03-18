#pragma once

#include <stdint.h>
#include "Bus.h"


class SmartAudio
{
public:
	SmartAudio( Bus* bus );
	~SmartAudio();

	void Setup();
	void Update();

	void setFrequency( uint16_t frequency );
	void setPower( uint8_t power );
	void setChannel( uint8_t channel );
	void setBand( uint8_t band );

	uint16_t getFrequency();
	uint8_t getPower();
	uint8_t getChannel();
	uint8_t getBand();

private:
	Bus* mBus;
	uint8_t mVersion;
	uint16_t mFrequency;
	uint8_t mPower;
	uint8_t mChannel;
	uint8_t mBand;
};
