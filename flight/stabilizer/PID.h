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

#ifndef PID_H
#define PID_H

#include "Vector.h"

class PID
{
public:
	PID();
	~PID();

	void Reset();

	void setP( float p );
	void setI( float i );
	void setD( float d );
	void setDeadBand( const Vector3f& band );

	void Process( const Vector3f& command, const Vector3f& measured, float dt );
	Vector3f state() const;

	Vector3f getPID() const;

private:
	Vector3f mIntegral;
	Vector3f mLastError;
	Vector3f mkPID;
	Vector3f mState;
	Vector3f mDeadBand;
};

#endif // PID_H
