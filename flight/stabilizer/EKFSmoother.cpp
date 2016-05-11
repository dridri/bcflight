#include <Debug.h>
#include "EKFSmoother.h"


EKFSmoother::EKFSmoother( int n, const Vector4f& q, const Vector4f& r )
	: mDataCount( n )
	, mQ( Matrix( n, n ) )
	, mR( Matrix( n, n ) )
	, mC( Matrix( n, n ) )
	, mSigma( Matrix( n, n ) )
	, mState( Matrix( 1, n ) )
{
	int i, j;
	for ( i = 0, j = 0; i < n*n; i += n + 1, j++ ) {
		mQ.m[i] = q[j];
		mR.m[i] = r[j];
	}

	mC.Identity();
	memset( mSigma.m, 0, sizeof(float) * 16 );
	memset( mState.m, 0, sizeof(float) * 16 );
}

EKFSmoother::EKFSmoother( int n, float q, float r )
	: EKFSmoother( n, Vector4f( q, q, q, q ), Vector4f( r, r, r, r ) )
{
}


EKFSmoother::~EKFSmoother()
{
}


Vector4f EKFSmoother::state()
{
	Vector4f ret;
	for ( int i = 0; i < mDataCount; i++ ) {
		ret[i] = mState.m[i];
	}
	return ret;
}


void EKFSmoother::setState( const Vector4f& v )
{
	for ( int i = 0; i < mDataCount; i++ ) {
		mState.m[i] = v[i];
	}
}


void EKFSmoother::Predict( float dt )
{
	mSigma = mSigma + mQ;
}


Vector4f EKFSmoother::Update( float dt, const Vector4f& vdata )
{
	Matrix data( 1, mDataCount );
	for ( int i = 0; i < mDataCount; i++ ) {
		data.m[i] = vdata[i];
	}


	Matrix Gk = mSigma * mC.Transpose() * ( mC * mSigma * mC.Transpose() + mR ).Inverse();

	mSigma = ( Matrix( mDataCount, mDataCount ) - Gk * mC ) * mSigma;
	mState = mState + ( Gk * ( data - mC * mState ) );


	Vector4f ret;
	for ( int i = 0; i < mDataCount; i++ ) {
		ret[i] = mState.m[i];
	}
	return ret;
}
