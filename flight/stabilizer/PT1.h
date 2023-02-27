#pragma once

#include "Filter.h"
#include "Debug.h"

template<typename V> class PT1 : public Filter<V>
{
public:
	PT1( const V& cutoff, const V& initialState = V() ) : mCutOff( cutoff ), mState( initialState ) {}

	virtual V filter( const V& input, float dt ) {
		V RC = 1.0f / ( float(2.0f * M_PI) * mCutOff );
		V coeff = dt / ( RC + dt );
		mState += coeff & ( input - mState );
		return mState;
	}

	virtual V state() {
		return mState;
	}

protected:
	V mCutOff;
	V mState;
};
