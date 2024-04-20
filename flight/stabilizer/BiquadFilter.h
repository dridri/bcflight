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

	BiquadFilter( const float& q ) : mType( NOTCH ), mQ( q ), x1( V() ), x2( V() ), y1( V() ), y2( V() ) {
		fDebug( mType, V( q ) );
	}
	BiquadFilter( const LuaValue& q ) : mType( NOTCH ), mQ( V(q) ), x1( V() ), x2( V() ), y1( V() ), y2( V() ) {
		fDebug( mType, V( q ) );
	}

	virtual V filter( const V& input, float dt ) {
		// mCenterFrequency = 2073;
		// mCenterFrequency = 100;
		// fDebug( input, dt );
		V omega = 2.0f * float(M_PI) * mCenterFrequency * 0.000250f; //dt;
		V sn = std::sin( omega );
		V cs = std::cos( omega );
		V alpha = sn * ( 1.0f / ( 2.0f * mQ ) );
		V a0, a1, a2;
		V b0, b1, b2;

		switch ( mType ) {
			case LPF:
				b1 = V(1) - cs;
				b0 = b1 * 0.5f;
				b2 = b0;
				a1 = -2 * cs;
				a2 = V(1) - alpha;
				break;
			case NOTCH:
				b0 = 1;
				b1 = -2 * cs;
				b2 = 1;
				a1 = b1;
				a2 = V(1) - alpha;
				break;
		}

		a0 = V(1) + alpha;
		V a0_ = V(1) / a0;
		b0 = b0 * a0_;
		b1 = b1 * a0_;
		b2 = b2 * a0_;
		a1 = a1 * a0_;
		a2 = a2 * a0_;

		// printf( "coeffs : %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", a0, a1, a2, b0, b1, b2 );
		// printf( "prev : %.2f, %.2f, %.2f, %.2f\n", x1, x2, y2, y1 );

		V state = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

		x2 = x1;
		x1 = input;

		y2 = y1;
		y1 = state;

		return y1;
	}

	virtual V state() {
		return y1;
	}

	void setCenterFrequency( const V& centerFrequency ) {
		mCenterFrequency = centerFrequency;
	}

protected:
	Type mType;
	float mQ;
	V mCenterFrequency;
	V mState;
	V x1;
	V x2;
	V y1;
	V y2;
};

LUA_CLASS class BiquadFilter_1 : public BiquadFilter<float>
{
public:
	LUA_EXPORT BiquadFilter_1( float q ) : BiquadFilter( q ) {}
};

// LUA_CLASS class BiquadFilter_2 : public BiquadFilter<Vector2f>
// {
// public:
// 	LUA_EXPORT BiquadFilter_2( float q ) : BiquadFilter( q ) {}
// };
// 
// LUA_CLASS class BiquadFilter_3 : public BiquadFilter<Vector3f>
// {
// public:
// 	LUA_EXPORT BiquadFilter_3( float q ) : BiquadFilter( q ) {}
// };
