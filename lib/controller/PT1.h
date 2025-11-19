#pragma once

#include "Filter.h"

template<typename V> class PT1 : public Filter<V>
{
public:
	PT1( const V& cutoff ) : mCutOff( cutoff ), mState( V() ) {}

	virtual V filter( const V& input, float dt ) {
		V RC = V(1.0f) / ( float(2.0f * M_PI) * mCutOff );
		V coeff = dt / ( RC + dt );
		mState += mul( coeff, ( input - mState ) );
		return mState;
	}

	virtual V state() {
		return mState;
	}

protected:
	V mCutOff;
	V mState;
	float mul( const float& a, const float& b ) {
		return a * b;
	}
	template<typename T, int n> Vector<T, n> mul( const Vector<T, n>& a, const Vector<T, n>& b ) {
		return a & b;
	}
};

class PT1_1 : public PT1<float>
{
public:
	PT1_1( float cutoff ) : PT1( cutoff ) {}
};

class PT1_2 : public PT1<Vector2f>
{
public:
	PT1_2( float cutoff_x, float cutoff_y ) : PT1( Vector2f( cutoff_x, cutoff_y ) ) {}
};

class PT1_3 : public PT1<Vector3f>
{
public:
	PT1_3( float cutoff_x, float cutoff_y, float cutoff_z ) : PT1( Vector3f( cutoff_x, cutoff_y, cutoff_z ) ) {}
};
