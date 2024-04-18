#pragma once

#include "Lua.h"
#include "Filter.h"
#include "Debug.h"

LUA_CLASS template<typename V> class BiquadFilter : public Filter<V>
{
public:
	BiquadFilter( const float& q ) : mQ( q ), mState( V() ) {
		fDebug( V( q ) );
	}
	BiquadFilter( const LuaValue& q ) : mQ( V(q) ), mState( V() ) {
		fDebug( V( q ) );
	}

	virtual V filter( const V& input, float dt ) {
		return mState;
	}

	virtual V state() {
		return mState;
	}

	void setCenterFrequency( const V& centerFrequency ) {
		mCenterFrequency = centerFrequency;
	}

protected:
	float mQ;
	V mCenterFrequency;
	V mState;
	float mul( const float& a, const float& b ) {
		return a * b;
	}
	template<typename T, int n> Vector<T, n> mul( const Vector<T, n>& a, const Vector<T, n>& b ) {
		return a & b;
	}
};

LUA_CLASS class BiquadFilter_1 : public BiquadFilter<float>
{
public:
	LUA_EXPORT BiquadFilter_1( float q ) : BiquadFilter( q ) {}
};

LUA_CLASS class BiquadFilter_2 : public BiquadFilter<Vector2f>
{
public:
	LUA_EXPORT BiquadFilter_2( float q ) : BiquadFilter( q ) {}
};

LUA_CLASS class BiquadFilter_3 : public BiquadFilter<Vector3f>
{
public:
	LUA_EXPORT BiquadFilter_3( float q ) : BiquadFilter( q ) {}
};
