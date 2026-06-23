#pragma once

#include "Lua.h"
#include "Filter.h"
#include "Debug.h"

LUA_CLASS template<typename V> class BiquadFilter : public Filter<V>
{
public:
	typedef enum {
		LPF,
		NOTCH
	} Type;

	BiquadFilter( const float& q, const float& centerFreq = 0.0f, Type type = NOTCH )
		: mType( type )
		, mQ( q )
		, mCenterFrequency(centerFreq)
		, mCenterFrequencyDtSmoothed(0)
		, mCenterFrequencyDtSmoothCutoff(1)
		, x1( V() )
		, x2( V() )
		, y1( V() )
		, y2( V() )
	{
		fDebug( mType, V( q ) );
	}

	virtual V filter( const V& input, float dt ) {
		bool isDerivativeFilter = ( mCenterFrequency == 60.0f );
		if ( mCenterFrequency <= 0.0f ) {
			x1 = input;
			x2 = input;
			y1 = input;
			y2 = input;
			return input;
		}
		// mCenterFrequency = 2073;
		// mCenterFrequency = 100;
		// fDebug( input, dt );
		// if ( mCenterFrequencyDtSmoothed == 0.0f ) {
		// 	mCenterFrequencyDtSmoothed = mCenterFrequency * dt;
		// } else {
		// 	mCenterFrequencyDtSmoothed = PT1Smooth( mCenterFrequencyDtSmoothed, mCenterFrequency * dt, dt );
		// }

		// V omega = 2.0f * float(M_PI) * mCenterFrequency * 0.000250f; //dt;
		float omega = 2.0f * float(M_PI) * mCenterFrequency * dt;
		// V omega = 2.0f * V(M_PI) * mCenterFrequencyDtSmoothed;
		float cs = std::cos( omega );
		float alpha = std::sin( omega ) * ( 1.0f / ( 2.0f * mQ ) );
		float a0, a1, a2;
		float b0, b1, b2;

		switch ( mType ) {
			case LPF:
				b1 = 1.0f - cs;
				b0 = b1 * 0.5f;
				b2 = b0;
				a0 = 1.0f + alpha;
				a1 = -2 * cs;
				a2 = 1.0f - alpha;
				break;
			case NOTCH:
			default:
				b0 = 1;
				b1 = -2 * cs;
				b2 = 1;
				a0 = 1.0f + alpha;
				a1 = b1;
				a2 = 1.0f - alpha;
				break;
		}

		float a0_inv = 1.0f / a0;
		b0 *= a0_inv;
		b1 *= a0_inv;
		b2 *= a0_inv;
		a1 *= a0_inv;
		a2 *= a0_inv;
		V state = V(b0) * input + V(b1) * x1 + V(b2) * x2 - V(a1) * y1 - V(a2) * y2;

		bool isfinite = false;
		if constexpr ( std::is_same<V, float>::value ) {
			isfinite = std::isfinite(state);
		} else if constexpr ( is_vector<V>::value ) {
			isfinite = true;
			for ( uint8_t i = 0; i < state.size(); i++ ) {
				isfinite = isfinite && std::isfinite(state[i]);
			}
		} else {
			gError() << "Unhandled state type";
			return input;
		}
		if ( !isfinite ) {
			gError() << "Numerical instability detected. Resetting state. (" << mType << ", " << mQ << ", " << mCenterFrequency << ")";
			x1 = input;
			x2 = input;
			y1 = input;
			y2 = input;
			exit(0);
			return input;
		}

		x2 = x1;
		x1 = input;

		y2 = y1;
		y1 = state;

		return y1;
	}

	virtual V state() {
		return y1;
	}

	void setCenterFrequency( float centerFrequency, float smoothCutoff = 1 ) {
		if ( mCenterFrequency == 0.0f ) {
			mCenterFrequency = centerFrequency;
			return;
		}
		mCenterFrequencyDtSmoothCutoff = smoothCutoff;
		if ( centerFrequency == 0.0f ) {
			mCenterFrequency = 0.0f;
			x1 = V();
			x2 = V();
			y1 = V();
			y2 = V();
			return;
		}
		mCenterFrequency = PT1Smooth( mCenterFrequency, centerFrequency );
	}

	const float centerFrequency() const {
		return mCenterFrequency;
	}

protected:
	Type mType;
	float mQ;
	float mCenterFrequency;
	float mCenterFrequencyDtSmoothed;
	float mCenterFrequencyDtSmoothCutoff;
	V x1;
	V x2;
	V y1;
	V y2;

	float PT1Smooth( float state, float input ) {
		float gain = 2.0f * float(M_PI) * mCenterFrequencyDtSmoothCutoff;
		gain = 0.5f * ( gain / ( gain + 1.0f ) );
		return state + gain * ( input - state );
	}
};

LUA_CLASS class BiquadFilter_1 : public BiquadFilter<float>
{
public:
	LUA_EXPORT BiquadFilter_1( const LuaValue& q, const LuaValue& centerFreq ) : BiquadFilter( q.toNumber(), centerFreq.toNumber() ) {}
};

LUA_CLASS class BiquadFilter_2 : public BiquadFilter<Vector2f>
{
public:
	LUA_EXPORT BiquadFilter_2( const LuaValue& q, const LuaValue& centerFreq ) : BiquadFilter( q.toNumber(), centerFreq.toNumber() ) {}
};

LUA_CLASS class BiquadFilter_3 : public BiquadFilter<Vector3f>
{
public:
	LUA_EXPORT BiquadFilter_3( const LuaValue& q, const LuaValue& centerFreq ) : BiquadFilter( q.toNumber(), centerFreq.toNumber() ) {}
};


// 2nd-order low-pass variants. q defaults to 0.7071 (Butterworth, no resonant peak).
LUA_CLASS class BiquadLPF_1 : public BiquadFilter<float>
{
public:
	LUA_EXPORT BiquadLPF_1( float cutoff, float q = 0.7071f ) : BiquadFilter( q, cutoff, LPF ) {}
};

LUA_CLASS class BiquadLPF_2 : public BiquadFilter<Vector2f>
{
public:
	LUA_EXPORT BiquadLPF_2( float cutoff, float q = 0.7071f ) : BiquadFilter( q, cutoff, LPF ) {}
};

LUA_CLASS class BiquadLPF_3 : public BiquadFilter<Vector3f>
{
public:
	LUA_EXPORT BiquadLPF_3( float cutoff, float q = 0.7071f ) : BiquadFilter( q, cutoff, LPF ) {}
};
