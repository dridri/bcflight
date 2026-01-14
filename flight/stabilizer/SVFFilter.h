#pragma once

#include "Lua.h"
#include "Filter.h"
#include "Debug.h"

/**
 * State Variable Filter (SVF) - Notch implementation
 * More numerically stable than biquad for dynamic frequency changes
 */
LUA_CLASS template<typename V> class SVFFilter : public Filter<V>
{
public:
	SVFFilter( float q = 0.707f, float centerFreq = 0.0f )
		: mQ( q )
		, mCenterFrequency( centerFreq )
		, mK( 1.0f / q )
		, mIC1eq( V() )
		, mIC2eq( V() )
	{
		fDebug( q, centerFreq );
	}

	virtual V filter( const V& input, float dt ) {
		if ( mCenterFrequency <= 0.0f || dt <= 0.0f ) {
			return input;
		}

		// Prevent tan() overflow when freq*dt approaches 0.5 (Nyquist)
		float freqDt = std::min( mCenterFrequency * dt, 0.49f );

		// Update coefficients
		float g = std::tan( float(M_PI) * freqDt );
		float g1 = g / ( 1.0f + g * ( g + mK ) );
		float g2 = g * g1;
		float g3 = g * g2;

		// SVF tick
		V v3 = input - mIC2eq;
		V v1 = g1 * mIC1eq + g2 * v3;
		V v2 = mIC2eq + g2 * mIC1eq + g3 * v3;
		mIC1eq = V(2.0f) * v1 - mIC1eq;
		mIC2eq = V(2.0f) * v2 - mIC2eq;

		// Notch output = input - k * bandpass
		V notch = input - V(mK) * v1;

		// Check for numerical instability
		bool isfinite = false;
		if constexpr ( std::is_same<V, float>::value ) {
			isfinite = std::isfinite(notch) && std::isfinite(mIC1eq) && std::isfinite(mIC2eq);
		} else if constexpr ( is_vector<V>::value ) {
			for ( uint8_t i = 0; i < notch.size(); i++ ) {
				isfinite = isfinite && std::isfinite(notch[i]) && std::isfinite(mIC1eq[i]) && std::isfinite(mIC2eq[i]);
			}
		} else {
			gError() << "Unhandled state type";
			return input;
		}
		if ( !isfinite ) {
			gError() << "SVFFilter: Numerical instability detected (freq=" << mCenterFrequency << ", dt=" << dt << "). Resetting state.";
			mIC1eq = V();
			mIC2eq = V();
			return input;
		}

		return notch;
	}

	virtual V state() {
		return mIC2eq;
	}

	void setCenterFrequency( float centerFrequency, float smoothingSpeed = 1.0f ) {
		if ( mCenterFrequency == 0.0f || smoothingSpeed >= 1.0f ) {
			mCenterFrequency = centerFrequency;
		} else {
			// Simple PT1-style smoothing
			mCenterFrequency += smoothingSpeed * ( centerFrequency - mCenterFrequency );
		}
	}

	void setQ( float q ) {
		mQ = q;
		mK = 1.0f / q;
	}

	float centerFrequency() const {
		return mCenterFrequency;
	}

	float q() const {
		return mQ;
	}

protected:
	float mQ;
	float mCenterFrequency;
	float mK;
	V mIC1eq;  // integrator 1 state
	V mIC2eq;  // integrator 2 state
};

LUA_CLASS class SVFFilter_1 : public SVFFilter<float>
{
public:
	LUA_EXPORT SVFFilter_1( const LuaValue& q = 0.707f, const LuaValue& centerFreq = 0.0f )
		: SVFFilter( q.toNumber(), centerFreq.toNumber() ) {}
};

LUA_CLASS class SVFFilter_3 : public SVFFilter<Vector3f>
{
public:
	LUA_EXPORT SVFFilter_3( const LuaValue& q = 0.707f, const LuaValue& centerFreq = 0.0f )
		: SVFFilter( q.toNumber(), centerFreq.toNumber() ) {}
};

