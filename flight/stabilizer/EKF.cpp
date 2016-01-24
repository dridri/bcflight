#include <Debug.h>
#include "EKF.h"

EKF::EKF()
	: mQ( Matrix( 2, 2 ) )
	, mR( Matrix( 4, 4 ) )
	, mC( Matrix( 2, 4 ) )
	, mSigma( Matrix( 2, 2 ) )
// 	, mState( Matrix( 1, 2 ) )
	, mState( Matrix( 1, 3 ) )
{
	mQ.m[0] = 0.9f;
	mQ.m[3] = 0.9f;

	mR.m[0] = 0.5f;
	mR.m[5] = 50.0f;
	mR.m[10] = 0.5f;
	mR.m[15] = 50.0f;

	mC.m[0] = 1.0f; mC.m[1] = 0.0f;
	mC.m[2] = 1.0f; mC.m[3] = 0.0f;
	mC.m[4] = 0.0f; mC.m[5] = 1.0f;
	mC.m[6] = 0.0f; mC.m[7] = 1.0f;

	memset( mSigma.m, 0, sizeof(float) * 16 );
	memset( mState.m, 0, sizeof(float) * 16 );
}


EKF::~EKF()
{
}


void EKF::Predict( float dt, const Vector4f& data )
{
	// mState[k] = mState[k-1]
	mSigma = mSigma + mQ;
}

/*
static void pMatrix( const std::string& name, const Matrix& m )
{
	printf( "%s [%d, %d] = {\n", name.c_str(), m.width(), m.height() );
	for ( int j = 0; j < m.height(); j++ ) {
		printf( "\t" );
		for ( int i = 0; i < m.width(); i++ ) {
			printf( "%.4f ", m.m[ j * m.width() + i ] );
		}
		printf( "\n" );
	}
	printf( "}\n" );
}


static void pVector( const std::string& name, const Vector4f& v )
{
	printf( "%s = { %.4f %.4f %.4f %.4f }\n", name.c_str(), v[0], v[1], v[2], v[3] );
}
*/

Vector4f EKF::Update( float dt, const Vector4f& data1, const Vector4f& data2 )
{
/*
	Matrix data( 1, 4 );
	data.m[0] = mState.m[0] + data1.x * dt;
	data.m[1] = data2.x;
	data.m[2] = mState.m[1] + -data1.y * dt;
	data.m[3] = data2.y;

	Matrix Gk = mSigma * mC.Transpose() * ( mC * mSigma * mC.Transpose() + mR ).Inverse();

	mSigma = ( Matrix( 2, 2 ) - Gk * mC ) * mSigma;
	mState = mState + ( Gk * ( data - mC * mState ) );

	return Vector4f( mState.m[0], mState.m[1], 0, 0 );
*/

	float accel_f = 0.001f;
	float inv_accel_f = 1.0f - accel_f;
	float magn_f = 0.002f;
	float inv_magnl_f = 1.0f - magn_f;
	Vector3f integration = Vector3f( mState.m[0], mState.m[1], mState.m[2] ) + Vector3f( data1.x, -data1.y, data1.z ) * dt;
	Vector4f ret;
	ret.x = integration.x;
	ret.y = integration.y;
	ret.z = integration.z;// * inv_accel_f + data2.z * accel_f;
	if ( data2.x != 0.0f ) {
		ret.x = ret.x * inv_accel_f + data2.x * accel_f;
	}
	if ( data2.y != 0.0f ) {
		ret.y = ret.y * inv_accel_f + data2.y * accel_f;
	}
	if ( data2.z > 0.0f and ret.z < 0.0f and std::abs( ( data2.z - 360.0f ) - ret.z ) < std::abs( data2.z - ret.z ) ) {
		ret.z = integration.z * inv_accel_f + ( data2.z - 360.0f ) * accel_f;
	} else if ( data2.z < 0.0f and ret.z > 0.0f and std::abs( ( data2.z + 360.0f ) - ret.z ) < std::abs( data2.z - ret.z ) ) {
		ret.z = integration.z * inv_accel_f + ( data2.z + 360.0f ) * accel_f;
	} else {
		ret.z = integration.z * inv_magnl_f + data2.z * magn_f;
	}

	mState.m[0] = ret.x;
	mState.m[1] = ret.y;
	mState.m[2] = ret.z;

	float tmp = ret.x;
	ret.x = ret.y;
	ret.y = tmp;
	return ret;
}
