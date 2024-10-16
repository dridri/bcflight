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
};
