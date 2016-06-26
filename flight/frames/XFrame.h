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

#ifndef XFRAME_H
#define XFRAME_H

#include <Vector.h>
#include "Frame.h"

class IMU;

class XFrame : public Frame
{
public:
	XFrame( Config* config );
	virtual ~XFrame();

	void Arm();
	void Disarm();
	void WarmUp();
	virtual bool Stabilize( const Vector3f& pid_output, const float& thrust );

	static Frame* Instanciate( Config* config );
	static int flight_register( Main* main );

protected:
	float mStabSpeeds[4];
	Vector3f mPIDMultipliers[4];
	float mMaxSpeed;
	bool mArmed;
	bool mAirMode;
};

#endif // XFRAME_H
