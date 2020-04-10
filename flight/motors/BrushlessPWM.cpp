/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <cmath>
#include <Debug.h>
#include "BrushlessPWM.h"
#include "Config.h"

#ifdef BUILD_BrushlessPWM

int BrushlessPWM::flight_register( Main* main )
{
	RegisterMotor( "PWM", BrushlessPWM::Instanciate );
	return 0;
}


Motor* BrushlessPWM::Instanciate( Config* config, const string& object )
{
	int fl_pin = config->Integer( object + ".pin" );
	int fl_min = config->Integer( object + ".minimum_us", 1020 );
	int fl_max = config->Integer( object + ".maximum_us", 1860 );
	return new BrushlessPWM( fl_pin, fl_min, fl_max );
}


BrushlessPWM::BrushlessPWM( uint32_t pin, int us_min, int us_max )
	: Motor()
// 	, mPWM( new PWM( pin, 1000000, 2000, 2 ) )
	, mMinUS( us_min )
	, mMaxUS( us_max )
{
}


BrushlessPWM::~BrushlessPWM()
{
}


void BrushlessPWM::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( std::isnan( speed ) or std::isinf( speed ) ) {
		return;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	uint32_t us = mMinUS + (uint32_t)( ( mMaxUS - mMinUS ) * speed );
// 	mPWM->SetPWMus( us );

	if ( force_hw_update ) {
// 		mPWM->Update();
	}
}


void BrushlessPWM::Disarm()
{
// 	mPWM->SetPWMus( mMinUS - 1 );
// 	mPWM->Update();
}


void BrushlessPWM::Disable()
{
// 	mPWM->SetPWMus( 0 );
// 	mPWM->Update();
}

#endif // BUILD_BrushlessPWM
