#pragma once

#include "Lua.h"
#include "Filter.h"
#include "Debug.h"

LUA_CLASS template<typename V> class PT1 : public Filter<V>
{
public:
	PT1( const V& cutoff ) : mCutOff( cutoff ), mState( V() ) {
		fDebug( V( cutoff ) );
	}
	PT1( const LuaValue& cutoff ) : mCutOff( V(cutoff) ), mState( V() ) {
		fDebug( V( cutoff ) );
	}

	virtual V filter( const V& input, float dt ) {
		V RC = 1.0f / ( float(2.0f * M_PI) * mCutOff );
		V coeff = dt / ( RC + dt );
		mState += coeff * ( input - mState );
		return mState;
	}

	virtual V state() {
		return mState;
	}

protected:
	V mCutOff;
	V mState;
};

LUA_CLASS class PT1_1 : public PT1<float>
{
public:
	LUA_EXPORT PT1_1( float cutoff ) : PT1( cutoff ) {}
};

LUA_CLASS class PT1_2 : public PT1<Vector2f>
{
public:
	LUA_EXPORT PT1_2( float cutoff_x, float cutoff_y ) : PT1( Vector2f( cutoff_x, cutoff_y ) ) {}
};

LUA_CLASS class PT1_3 : public PT1<Vector3f>
{
public:
	LUA_EXPORT PT1_3( float cutoff_x, float cutoff_y, float cutoff_z ) : PT1( Vector3f( cutoff_x, cutoff_y, cutoff_z ) ) {}
};
