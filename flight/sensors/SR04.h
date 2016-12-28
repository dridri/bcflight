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

#ifndef SR04_H
#define SR04_H

#include "Altimeter.h"

class SR04 : public Altimeter
{
public:
	SR04( uint32_t gpio_trigger, uint32_t gpio_echo );
	~SR04();

	virtual Type type() const { return Proximity; }
	virtual void Calibrate( float dt, bool last_pass );
	virtual void Read( float* altitude );

	virtual std::string infos();

	static Sensor* Instanciate( Config* config, const std::string& object );
	static int flight_register( Main* main );

protected:
	uint32_t mTriggerPin;
	uint32_t mEchoPin;
	uint64_t mRiseTick;
	float mAltitude;
};

#endif // SR04_H
