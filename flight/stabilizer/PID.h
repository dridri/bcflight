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

#include <type_traits>
#include "Vector.h"

template< typename T > class PID
{
public:
	PID() {
		mIntegral = 0;
		mLastError = 0;
		mkPID = 0;
		mState = 0;
		mDeadBand = 0;
	}
	~PID() {
	}

	void Reset() {
		mIntegral = 0;
		mLastError = 0;
		mState = 0;
	}

	void setP( float p ) {
		mkPID.x = p;
	}
	void setI( float i ) {
		mkPID.y = i;
	}
	void setD( float d ) {
		mkPID.z = d;
	}
	void setDeadBand( const T& band ) {
		mDeadBand = band;
	}

	void Process( const T& command, const T& measured, float dt ) {
		T error = command - measured;
		ApplyDeadBand( error );

		mIntegral += error * dt;
		T derivative = ( error - mLastError ) / dt;
		T output = error * mkPID.x + mIntegral * mkPID.y + derivative * mkPID.z;

		mLastError = error;
		mState = output;
	}

	T state() const {
		return mState;
	}
	Vector3f getPID() const {
		return mkPID;
	}

private:
	void ApplyDeadBand( Vector3f& error ) {
		if ( abs( error.x ) < mDeadBand.x ) {
			error.x = 0.0f;
		}
		if ( abs( error.y ) < mDeadBand.y ) {
			error.y = 0.0f;
		}
		if ( abs( error.z ) < mDeadBand.z ) {
			error.z = 0.0f;
		}
	}
	void ApplyDeadBand( Vector2f& error ) {
		if ( abs( error.x ) < mDeadBand.x ) {
			error.x = 0.0f;
		}
		if ( abs( error.y ) < mDeadBand.y ) {
			error.y = 0.0f;
		}
	}
	void ApplyDeadBand( float& error ) {
		if ( abs( error ) < mDeadBand ) {
			error = 0.0f;
		}
	}

	T mIntegral;
	T mLastError;
	Vector3f mkPID;
	T mState;
	T mDeadBand;
};

#endif // PID_H
