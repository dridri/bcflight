#ifndef STABILIZERPROXY_H
#define STABILIZERPROXY_H

#include "Stabilizer.h"
#include "Bus.h"


class StabilizerProxy : public Stabilizer
{
public:
	StabilizerProxy( Bus* bus );
	~StabilizerProxy();

	void setRollP( float p );
	void setRollI( float i );
	void setRollD( float d );
	void setPitchP( float p );
	void setPitchI( float i );
	void setPitchD( float d );
	void setYawP( float p );
	void setYawI( float i );
	void setYawD( float d );
	Vector3f lastPIDOutput() const;

	void setOuterP( float p );
	void setOuterI( float i );
	void setOuterD( float d );
	Vector3f lastOuterPIDOutput() const;

	void setHorizonOffset( const Vector3f& v );

	void setMode( uint32_t mode );
	void setAltitudeHold( bool enabled );

	void Arm();
	void Disarm();
	void setRoll( float value );
	void setPitch( float value );
	void setYaw( float value );
	void setThrust( float value );

	void CalibrateESCs();
	void MotorTest(uint32_t id);
	void Reset( const float& yaw );
	void Update( IMU* imu, Controller* ctrl, float dt );

protected:
	Bus* mBus;
};

#endif // STABILIZERPROXY_H
