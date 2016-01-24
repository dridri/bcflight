#ifndef L3GD20H_H
#define L3GD20H_H

#include <Gyroscope.h>
#include <I2C.h>

class L3GD20H : public Gyroscope
{
public:
	L3GD20H();
	~L3GD20H();

	static Gyroscope* Instanciate();
	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	static int flight_register( Main* main );

private:
	I2C* mI2C;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


#define L3GD20_WHO_AM_I      0x0F   // 11010100   r
#define L3GD20_CTRL_REG1     0x20   // 00000111   rw
#define L3GD20_CTRL_REG2     0x21   // 00000000   rw
#define L3GD20_CTRL_REG3     0x22   // 00000000   rw
#define L3GD20_CTRL_REG4     0x23   // 00000000   rw
#define L3GD20_CTRL_REG5     0x24   // 00000000   rw
#define L3GD20_REFERENCE     0x25   // 00000000   rw
#define L3GD20_OUT_TEMP      0x26   //            r
#define L3GD20_STATUS_REG    0x27   //            r
#define L3GD20_OUT_X_L       0x28   //            r
#define L3GD20_OUT_X_H       0x29   //            r
#define L3GD20_OUT_Y_L       0x2A   //            r
#define L3GD20_OUT_Y_H       0x2B   //            r
#define L3GD20_OUT_Z_L       0x2C   //            r
#define L3GD20_OUT_Z_H       0x2D   //            r
#define L3GD20_FIFO_CTRL_REG 0x2E   // 00000000   rw
#define L3GD20_FIFO_SRC_REG  0x2F   //            r
#define L3GD20_INT1_CFG      0x30   // 00000000   rw
#define L3GD20_INT1_SRC      0x31   //            r
#define L3GD20_TSH_XH        0x32   // 00000000   rw
#define L3GD20_TSH_XL        0x33   // 00000000   rw
#define L3GD20_TSH_YH        0x34   // 00000000   rw
#define L3GD20_TSH_YL        0x35   // 00000000   rw
#define L3GD20_TSH_ZH        0x36   // 00000000   rw
#define L3GD20_TSH_ZL        0x37   // 00000000   rw
#define L3GD20_INT1_DURATION 0x38   // 00000000   rw

#endif // L3GD20H_H
