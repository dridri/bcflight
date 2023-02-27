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
#include "PT1.h"
#include "Main.h"
#include "Config.h"

template< typename T > class PID
{
public:
	PID() {
		mIntegral = 0;
		mLastDerivativeError = 0;
		mkPID = 0;
		mState = 0;
		mDeadBand = 0;
	}
	~PID() {
	}
	PID( const LuaValue& v ) {
		if ( v.type() == LuaValue::Table ) {
			float kP = Main::instance()->config()->Number( "PID.pscale", 1.0f );
			float kI = Main::instance()->config()->Number( "PID.iscale", 1.0f );
			float kD = Main::instance()->config()->Number( "PID.dscale", 1.0f );
			const std::map<std::string, LuaValue >& t = v.toTable();
			mkPID.x = kP * t.at("p").toNumber();
			mkPID.y = kI * t.at("i").toNumber();
			mkPID.z = kD * t.at("d").toNumber();
		}
	}

	void Reset() {
		mIntegral = 0;
		mLastDerivativeError = 0;
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

	void Process( const T& deltaP, const T& deltaI, const T& deltaD, float dt ) {
		ApplyDeadBand( deltaP );
		ApplyDeadBand( deltaI );
		ApplyDeadBand( deltaD );

		T proportional = deltaP;
		mIntegral += deltaI * dt;
		T derivative = ( deltaD - mLastDerivativeError ) / dt;

/*
		T proportional = deltaP * dt;
		mIntegral += deltaI * dt * dt;
		T derivative = ( deltaD - mLastDerivativeError ); // * dt / dt;
*/
		T output = proportional * mkPID[0] + mIntegral * mkPID[1] + derivative * mkPID[2];

		mLastDerivativeError = deltaD;
		mState = output;
	}

	void Process( const T& command, const T& measured, float dt ) {
		T delta = command - measured;
		Process( delta, delta, delta, dt );
	}

	T state() const {
		return mState;
	}
	Vector3f getPID() const {
		return mkPID;
	}

private:
	Vector3f ApplyDeadBand( const Vector3f& error ) {
		Vector3f ret = error;
		if ( abs( error.x ) < mDeadBand.x ) {
			ret.x = 0.0f;
		}
		if ( abs( error.y ) < mDeadBand.y ) {
			ret.y = 0.0f;
		}
		if ( abs( error.z ) < mDeadBand.z ) {
			ret.z = 0.0f;
		}
		return ret;
	}
	Vector2f ApplyDeadBand( const Vector2f& error ) {
		Vector2f ret = error;
		if ( abs( error.x ) < mDeadBand.x ) {
			ret.x = 0.0f;
		}
		if ( abs( error.y ) < mDeadBand.y ) {
			ret.y = 0.0f;
		}
		return ret;
	}
	float ApplyDeadBand( const float& error ) {
		float ret = error;
		if ( abs( error ) < mDeadBand ) {
			ret = 0.0f;
		}
		return ret;
	}

	T mIntegral;
	T mLastDerivativeError;
	Vector3f mkPID;
	T mState;
	T mDeadBand;
};

#endif // PID_H
