#pragma once

#include <stdint.h>
#include <map>


#define DSHOT_MAX_OUTPUTS 8


class DShotDriver
{
public:
	static DShotDriver* instance( bool create_new = true );

	void setPinValue( uint32_t pin, uint16_t value, bool telemetry = false );
	void disablePinValue( uint32_t pin );
	void Update();

	void Kill() noexcept;

private:
	DShotDriver();
	~DShotDriver();

	std::map< uint32_t, bool > mFlatValues;
	std::map< uint32_t, uint16_t > mPinValues;
	/*
	uint16_t mValueMap[DSHOT_MAX_OUTPUTS];
	uint16_t mChannelMap[DSHOT_MAX_OUTPUTS];
	uint16_t mBitmaskMap[DSHOT_MAX_OUTPUTS];
	*/

	uint8_t* mDRMBuffer;
	uint32_t mDRMPitch;
	uint32_t mDRMWidth;
	uint32_t mDRMHeight;

	static DShotDriver* sInstance;
	static uint8_t sDPIMode;
	static uint8_t sDPIPinMap[8][28][2]; // [DpiMode][pinNumber][r/g/b, bitmask]

	static bool sKilled;
};
