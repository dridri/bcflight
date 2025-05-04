#include "MahonyAHRS.h"
#include <Debug.h>


MahonyAHRS::MahonyAHRS( float kp, float ki )
	: mRotationMatrix( Matrix( 3, 3 ) )
	, mState( Quaternion() )
	, mIntegral( Vector3f() )
	, mKp( kp )
	, mKi( ki )
{
	fDebug( kp, ki );
	memset( mRotationMatrix.m, 0, sizeof( float ) * 9 );
}


MahonyAHRS::~MahonyAHRS()
{
}


void MahonyAHRS::UpdateInput( uint32_t row, const Vector3f& input )
{
	if ( row >= mInputs.size() ) {
		mInputs.resize( row + 1 );
	}
	mInputs[row] = input;
}


void MahonyAHRS::Process( float dt )
{
	Vector3f gyro = mInputs[0] * M_PI / 180.0f;

	Vector3f e = Vector3f();

	if ( mInputs.size() >= 3 ) {
		Vector3f mag = mInputs[2].normalized();
		// TODO
	}

	if ( mInputs.size() >= 2 ) {
		Vector3f acc = mInputs[1].normalized();
		float tmp = acc.x;
		acc.x = -acc.y;
		acc.y = tmp;
		Vector3f rot = Vector3f( mRotationMatrix( 0, 2 ), mRotationMatrix( 1, 2 ), mRotationMatrix( 2, 2 ) );
		e += acc ^ rot;
	}

	// Apply feedback terms
	mIntegral += e * mKi * dt; // integral error scaled by Ki
	gyro += e * mKp + mIntegral; // apply integral feedback, proportional error + integral error

	// pre-multiply common factors
	gyro *= 0.5f * dt;

	// compute quaternion rate of change
	mState = mState + Quaternion(
		 mState.w * gyro.x + mState.y * gyro.z - mState.z * gyro.y,
		 mState.w * gyro.y - mState.x * gyro.z + mState.z * gyro.x,
		 mState.w * gyro.z + mState.x * gyro.y - mState.y * gyro.x,
		-mState.x * gyro.x - mState.y * gyro.y - mState.z * gyro.z
	);

	// normalise quaternion
	mState.normalize();

	// compute rotation matrix from quaternion
	computeMatrix();

	// compute final state
	float roll = std::atan2( mRotationMatrix( 1, 2 ), mRotationMatrix( 2, 2 ) ) * 180.0f / M_PI;
	float pitch = ( 0.5f * M_PI - std::acos( -mRotationMatrix( 0, 2 ) ) ) * 180.0f / M_PI;
	float yaw = std::atan2( mRotationMatrix( 0, 1 ), mRotationMatrix( 0, 0 ) ) * 180.0f / M_PI;
	mFinalState = Vector3f( roll, pitch, yaw );
}


const Vector3f& MahonyAHRS::state() const
{
	return mFinalState;
}


void MahonyAHRS::computeMatrix()
{
	// Extract quaternion components
	float qx = mState.x;
	float qy = mState.y;
	float qz = mState.z;
	float qw = mState.w;

	// Compute temporary variables to avoid redundant multiplications
	float qx2 = qx * qx;
	float qy2 = qy * qy;
	float qz2 = qz * qz;
	float qwqx = qw * qx;
	float qwqy = qw * qy;
	float qwqz = qw * qz;
	float qxqy = qx * qy;
	float qxqz = qx * qz;
	float qyqz = qy * qz;

	// Compute rotation matrix elements
	mRotationMatrix.m[0] = 1.0f - 2.0f * ( qy2 + qz2 );
	mRotationMatrix.m[1] = 2.0f * ( qxqy - qwqz );
	mRotationMatrix.m[2] = 2.0f * ( qxqz + qwqy );

	mRotationMatrix.m[3] = 2.0f * ( qxqy + qwqz );
	mRotationMatrix.m[4] = 1.0f - 2.0f * ( qx2 + qz2 );
	mRotationMatrix.m[5] = 2.0f * ( qyqz - qwqx );

	mRotationMatrix.m[6] = 2.0f * ( qxqz - qwqy );
	mRotationMatrix.m[7] = 2.0f * ( qyqz + qwqx );
	mRotationMatrix.m[8] = 1.0f - 2.0f * ( qx2 + qy2 );
}
