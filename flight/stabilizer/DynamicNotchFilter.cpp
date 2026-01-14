#include <cassert>
#include <Main.h>
#include <IMU.h>
#include "DynamicNotchFilter.h"
#include "SVFFilter.h"
#include <fftw3.h>


template<typename V> _DynamicNotchFilterBase<V>::_DynamicNotchFilterBase( uint8_t n )
	: Filter<V>()
	, mMinFreq( 50 )
	, mMaxFreq( 600 )
	, mN( n )
	, mState( V() )
	, mFixedDt( 0.0f )
	, mInputIndex( 0 )
	, mAnalysisThread( new HookThread<_DynamicNotchFilterBase<V>>( "dnf", this, &_DynamicNotchFilterBase<V>::analysisRun ) )
{
	fDebug( (int)n );
	uint32_t nSamples = 1000000 / Main::instance()->config()->Integer( "stabilizer.loop_time", 2000 );
	mFixedDt = 1.0f / nSamples;
	mNumSamples = std::min( nSamples, 512U );
	mSampleResolution = nSamples / mNumSamples;
	mDFTs.resize( n );
	mDFTs.reserve( n );
	for ( uint8_t i = 0; i < n; i++ ) {
		mDFTs[i].input = (float*)fftwf_malloc( sizeof(float) * mNumSamples );
		mDFTs[i].inputBuffer = (float*)fftwf_malloc( sizeof(float) * mNumSamples );
		mDFTs[i].output = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex) * ( mNumSamples / 2 + 1 ) );
		mDFTs[i].plan = fftwf_plan_dft_r2c_1d( mNumSamples, mDFTs[i].inputBuffer, mDFTs[i].output, FFTW_ESTIMATE );
		mDFTs[i].magnitude = (float*)fftwf_malloc( sizeof(float) * ( mNumSamples / 2 + 1 ) );
		mPeakFilters.push_back( std::vector<PeakFilter>() );
		mPeakFilters[i].resize( DYNAMIC_NOTCH_COUNT );
		mPeakFilters[i].reserve( DYNAMIC_NOTCH_COUNT );
		for ( uint8_t p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			// mPeakFilters[i][p].filter = new SVFFilter<float>( 0.707f );
			mPeakFilters[i][p].filter = new BiquadFilter<float>( 0.707f );
		}
	}
	Start();
}


template<> DynamicNotchFilter<float>::DynamicNotchFilter()
	: _DynamicNotchFilterBase<float>( 1 )
{
	fDebug( (int)1 );
}


template<typename V> void _DynamicNotchFilterBase<V>::Start()
{
	uint32_t main_freq = 1000000 / Main::instance()->config()->Integer( "stabilizer.loop_time", 2000 );
	mAnalysisThread->setFrequency( main_freq / 12 );
	mAnalysisThread->Start();
}


template<> DynamicNotchFilter<Vector<float, 1>>::DynamicNotchFilter() : _DynamicNotchFilterBase<Vector<float,1>>( 1 ) {}
template<> DynamicNotchFilter<Vector<float, 2>>::DynamicNotchFilter() : _DynamicNotchFilterBase<Vector<float,2>>( 2 ) {}
template<> DynamicNotchFilter<Vector<float, 3>>::DynamicNotchFilter() : _DynamicNotchFilterBase<Vector<float,3>>( 3 ) {}
template<> DynamicNotchFilter<Vector<float, 4>>::DynamicNotchFilter() : _DynamicNotchFilterBase<Vector<float,4>>( 4 ) {}


template<> void DynamicNotchFilter<float>::pushSample( const float& sample )
{
	std::lock_guard<std::mutex> lock( mInputMutex );

	mDFTs[0].input[mInputIndex] = sample;

	mInputIndex = ( mInputIndex + 1 ) % mNumSamples;
}


template<> void DynamicNotchFilter<Vector3f>::pushSample( const Vector3f& sample )
{
	std::lock_guard<std::mutex> lock( mInputMutex );

	for ( uint8_t i = 0; i < 3; i++ ) {
		mDFTs[i].input[mInputIndex] = sample.ptr[i];
	}

	mInputIndex = ( mInputIndex + 1 ) % mNumSamples;
}


template<typename V> V DynamicNotchFilter<V>::filter( const V& input, float dt )
{
	(void)dt;
	return input;
}


template<> float DynamicNotchFilter<float>::filter( const float& input, float dt )
{
	(void)dt;
	pushSample( input );

	float result = input;
	for ( int p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
		const PeakFilter& peakFilter = mPeakFilters[0][p];
		if ( peakFilter.filter->centerFrequency() <= 0.0f ) {
			continue;
		}
		// result = peakFilter.filter->filter( result, dt ); // BiquadFilter tends to infinity with variable dt
		result = peakFilter.filter->filter( result, mFixedDt );
	}
	return result;
}


template<> Vector3f DynamicNotchFilter<Vector3f>::filter( const Vector3f& input, float dt )
{
	assert(mN == 3);
	(void)dt;
	pushSample( input );

	Vector3f result = Vector3f( input.x, input.y, input.z );

	for ( uint8_t i = 0; i < 3; i++ ) {
		for ( int p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			PeakFilter& peakFilter = mPeakFilters[i][p];
			if ( peakFilter.filter->centerFrequency() <= 0.0f ) {
				continue;
			}
			// result.ptr[i] = peakFilter.filter->filter( result.ptr[i], dt ); // BiquadFilter tends to infinity with variable dt
			result.ptr[i] = peakFilter.filter->filter( result.ptr[i], mFixedDt );
		}
	}

	return result;
}


template<typename V> V _DynamicNotchFilterBase<V>::state()
{
	return mState;
}


template<typename V> __attribute__((optimize("unroll-loops"))) bool _DynamicNotchFilterBase<V>::analysisRun()
{
	// printf( "\n" );
	const float dT = 1.0f / float(mAnalysisThread->frequency());
	const uint32_t binMin = mMinFreq / ( mSampleResolution * 2 );
	const uint32_t binMax = std::min( mMaxFreq / ( mSampleResolution * 2 ), mNumSamples / 2 + 1 );

	// Copy input samples to aligned FFTW3 buffer
	mInputMutex.lock();
	uint32_t startIdx = ( mInputIndex + 1 ) % mNumSamples;
	for ( uint8_t i = 0; i < mN; i++ ) {
		memcpy( mDFTs[i].inputBuffer, &mDFTs[i].input[startIdx], sizeof(float) * ( mNumSamples - startIdx ) );
		memcpy( &mDFTs[i].inputBuffer[( mNumSamples - startIdx )], mDFTs[i].input, sizeof(float) * startIdx );
	}
	mInputMutex.unlock();

	// Apply Hann window
	for ( uint32_t n = 0; n < mNumSamples; n++ ) {
		float window = 0.5f * ( 1.0f - std::cos(2.0f * float(M_PI) * n / (mNumSamples - 1)) );
		#pragma GCC unroll 4
		for ( uint8_t i = 0; i < mN; i++ ) {
			mDFTs[i].inputBuffer[n] *= window;
		}
	}

	// Execute FFT
	#pragma GCC unroll 4
	for ( uint8_t i = 0; i < mN; i++ ) {
		fftwf_execute( mDFTs[i].plan );
	}

	// Extract DFT magnitude components and initialize noise floor
	float* noise = new float[mN];
	memset( noise, 0, sizeof(float) * mN );
	for ( uint8_t i = 0; i < mN; i++ ) {
		for ( uint32_t j = 0; j < mNumSamples / 2 + 1; j++ ) {
			// mDFTs[i].magnitude[j] = std::sqrt( mDFTs[i].output[j][0] * mDFTs[i].output[j][0] + mDFTs[i].output[j][1] * mDFTs[i].output[j][1] );
			mDFTs[i].magnitude[j] = std::abs( mDFTs[i].output[j][0] );
			if ( j >= binMin && j<= binMax ) {
				noise[i] += mDFTs[i].magnitude[j];
			}
		}
	}

	// Detect top peaks
	const int maxPeaks = DYNAMIC_NOTCH_COUNT;
	const int peakDetectionSurround = 2;
	std::vector<Peak>* peaks = new std::vector<Peak>[mN];
	for ( uint8_t i = 0; i < mN; i++ ) {
		peaks[i].resize( DYNAMIC_NOTCH_COUNT );
		peaks[i].reserve( DYNAMIC_NOTCH_COUNT );
		memset( peaks[i].data(), 0, sizeof(Peak) * DYNAMIC_NOTCH_COUNT );
		for ( uint32_t j = binMin + peakDetectionSurround; j < binMax - peakDetectionSurround; j++ ) {
			float val = mDFTs[i].magnitude[j];
		
			bool above = true;
			// float localMean = 0.0f;
			for ( int k = -peakDetectionSurround; k <= peakDetectionSurround; k++ ) {
				// localMean += mDFTs[i].magnitude[j + k];
				if ( val < mDFTs[i].magnitude[j + k] ) {
					above = false;
					break;
				}
			}
			// localMean /= 2 * peakDetectionSurround + 1;
			if ( /*val > localMean &&*/ std::abs(val) > 0.0f && above ) {
				for ( uint8_t k = 0; k < maxPeaks; k++ ) {
					if ( val > peaks[i][k].magnitude ) {
						// if(i==1)printf( "found peak at % 4d (% 4d Hz) magnitude=%4.4f\n", j, j * mSampleResolution * 2, val );
						for ( uint8_t l = maxPeaks - 1; l > k; l-- ) {
							peaks[i][l].frequency = peaks[i][l - 1].frequency;
							peaks[i][l].magnitude = peaks[i][l - 1].magnitude;
							peaks[i][l].dftIndex = peaks[i][l - 1].dftIndex;
						}
						peaks[i][k].dftIndex = j;
						peaks[i][k].magnitude = val;
						peaks[i][k].frequency = (float)j / mNumSamples;
						break;
					}
				}
				j++;
			}
		}
	}

	// Sort peaks by index, ascending
	for ( uint8_t i = 0; i < mN; i++ ) {
		for ( uint8_t j = 0; j < maxPeaks; j++ ) {
			for ( uint8_t k = j + 1; k < maxPeaks; k++ ) {
				if ( peaks[i][j].dftIndex > peaks[i][k].dftIndex && peaks[i][k].dftIndex != 0 ) {
					Peak temp = peaks[i][j];
					peaks[i][j] = peaks[i][k];
					peaks[i][k] = temp;
				}
			}
		}
	}

	int* peaksCount = new int[mN];
	float* noiseFloor = new float[mN];
	memset( peaksCount, 0, sizeof(int) * mN );
	memset( noiseFloor, 0, sizeof(float) * mN );

	// Approximate noise floor
	for ( uint8_t i = 0; i < mN; i++ ) {
		noiseFloor[i] = noise[i];
		float* dft = mDFTs[i].magnitude;
		const std::vector<Peak>& axisPeaks = peaks[i];
		for (int p = 0; p < maxPeaks; p++) {
			if (axisPeaks[p].dftIndex == 0) {
				continue;
			}
			noiseFloor[i] -= 0.50f * dft[axisPeaks[p].dftIndex - 2];
			noiseFloor[i] -= 0.75f * dft[axisPeaks[p].dftIndex - 1];
			noiseFloor[i] -= dft[axisPeaks[p].dftIndex];
			noiseFloor[i] -= 0.75f * dft[axisPeaks[p].dftIndex + 1];
			noiseFloor[i] -= 0.50f * dft[axisPeaks[p].dftIndex + 2];
			peaksCount[i]++;
		}
		noiseFloor[i] /= ( mNumSamples / 2 ) - peaksCount[i] + 1;
		// noiseFloor[i] *= 2.0f;
	}

	BlackBox* bb = Main::instance()->blackbox();
	std::vector<float> centerFrequencies;
	centerFrequencies.resize( DYNAMIC_NOTCH_COUNT );

	//  Process peaks
	for ( uint8_t i = 0; i < mN; i++ ) {
		memset( centerFrequencies.data(), 0, sizeof(float) * DYNAMIC_NOTCH_COUNT );
		// const std::vector<float>& dft = mDFTs[i].magnitude;
		const std::vector<Peak>& axisPeaks = peaks[i];
		for ( int p = 0; p < peaksCount[i]; p++ ) {
			if ( axisPeaks[p].dftIndex == 0 || axisPeaks[p].magnitude <= noiseFloor[i] ) {
				continue;
			}
			PeakFilter* peakFilter = nullptr;
			peakFilter = &mPeakFilters[i][p];
			/*
			float minDistance = 0.0f;
			for ( uint8_t j = 0; j < DYNAMIC_NOTCH_COUNT; j++ ) {
				if ( mPeakFilters[i][j].centerFrequency == 0.0f ) {
					peakFilter = &mPeakFilters[i][j];
					break;
				}
				float distance = std::abs( mPeakFilters[i][j].centerFrequency - axisPeaks[p].frequency );
				if ( peakFilter == nullptr || distance < minDistance ) {
					peakFilter = &mPeakFilters[i][j];
					minDistance = distance;
				}
			}
			20/512*512*(4000/512)*2 = 312
			*/
			if ( peakFilter == nullptr ) {
				continue;
			}
			float smoothCutoff = 4.0f * std::clamp( axisPeaks[p].magnitude / noiseFloor[i], 1.0f, 10.0f );
			float centerFrequency = axisPeaks[p].frequency * float(mNumSamples * mSampleResolution); // * 2.0f;
			peakFilter->filter->setCenterFrequency( centerFrequency, 0.1f * smoothCutoff * dT );
			centerFrequencies[p] = peakFilter->filter->centerFrequency();
		}
		if ( bb != nullptr ) {
			bb->Enqueue( "DynamicNotchFilter:centerFrequencies[" + std::to_string(i) + "]", centerFrequencies );
		}
	}

	delete[] noiseFloor;
	delete[] peaksCount;
	delete[] peaks;
	delete[] noise;
	return true;
}


template class _DynamicNotchFilterBase<float>;
template class DynamicNotchFilter<float>;
template class _DynamicNotchFilterBase<Vector2f>;
template class DynamicNotchFilter<Vector<float, 2>>;
template class _DynamicNotchFilterBase<Vector3f>;
template class DynamicNotchFilter<Vector<float, 3>>;
template class _DynamicNotchFilterBase<Vector4f>;
template class DynamicNotchFilter<Vector<float, 4>>;


DynamicNotchFilter_3::DynamicNotchFilter_3()
	: DynamicNotchFilter<Vector<float, 3>>()
{
}
