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

#include <Debug.h>
#include "EKF.h"

EKF::EKF( uint32_t n_inputs, uint32_t n_outputs )
	: mInputsCount( n_inputs )
	, mOutputsCount( n_outputs )
	, mQ( Matrix( mOutputsCount, mOutputsCount ) )
	, mR( Matrix( mInputsCount, mInputsCount ) )
	, mC( Matrix( mOutputsCount, mInputsCount ) )
	, mSigma( Matrix( mOutputsCount, mOutputsCount ) )
	, mInput( Matrix( 1, mInputsCount ) )
	, mState( Matrix( 1, mOutputsCount ) )
{
	mC.Clear();
	mSigma.Clear();
	mInput.Clear();
	mState.Clear();
}


EKF::~EKF()
{
}


void EKF::setInputFilter( uint32_t row, float filter )
{
	mR.m[row * mR.width() + row] = filter;
}


void EKF::setOutputFilter( uint32_t row, float filter )
{
	mQ.m[row * mQ.width() + row] = filter;
}


void EKF::setSelector( uint32_t input_row, uint32_t output_row, float selector )
{
	mC.m[input_row * mC.width() + output_row] = selector;
}


Vector4f EKF::state( uint32_t offset ) const
{
	Vector4f ret;

	for ( uint32_t i = 0; i < 4 and offset + i < (uint32_t)mState.height(); i++ ) {
		ret[i] = mState.constData()[offset + i];
	}

	return ret;
}


void EKF::UpdateInput( uint32_t row, float input )
{
	mInput.m[row] = input;
}


void EKF::Process( float dt )
{
	// Predict
	mSigma = mSigma + mQ;

	// Update
	Matrix Gk = mSigma * mC.Transpose() * ( mC * mSigma * mC.Transpose() + mR ).Inverse();
	mSigma = ( Matrix( mOutputsCount, mOutputsCount ) - Gk * mC ) * mSigma;
	mState = mState + ( Gk * ( mInput - mC * mState ) );
}


void EKF::DumpInput()
{
	printf( "EKF Input [%d, %d] = {\n", mInput.width(), mInput.height() );
	for ( int j = 0; j < mInput.height(); j++ ) {
		printf( "\t" );
		for ( int i = 0; i < mInput.width(); i++ ) {
			printf( "%.4f ", mInput.m[ j * mInput.width() + i ] );
		}
		printf( "\n" );
	}
	printf( "}\n" );
}
