#ifndef EKF_H
#define EKF_H

#include <Matrix.h>

class EKF
{
public:
	EKF( uint32_t n_inputs, uint32_t n_outputs );
	~EKF();

	void setInputFilter( uint32_t row, float filter );
	void setOutputFilter( uint32_t row, float filter );
	void setSelector( uint32_t input_row, uint32_t output_row, float selector );

	Vector4f state( uint32_t offset ) const;

	void UpdateInput( uint32_t row, float input );
	void Process( float dt );

	void DumpInput();

protected:
	uint32_t mInputsCount;
	uint32_t mOutputsCount;
	Matrix mQ;
	Matrix mR;
	Matrix mC;
	Matrix mSigma;
	Matrix mInput;
	Matrix mState;
};

#endif // EKF_H
