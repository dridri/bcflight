#include "DShot600.h"

DShot600::DShot600( uint32_t pin )
	: Motor()
	, mPWM( /*TODO*/ )
{
}


DShot600::~DShot600()
{
}


void DShot600::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void DShot600::Disable()
{
	mPWM->Update();
}


void DShot600::Disarm()
{
	mPWM->Update();
}
