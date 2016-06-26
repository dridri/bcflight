/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

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
