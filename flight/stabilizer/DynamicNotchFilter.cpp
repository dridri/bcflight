#include <Main.h>
#include <IMU.h>
#include "DynamicNotchFilter.h"
#include "BiquadFilter.h"
#include <fftw3.h>


template<typename V> DynamicNotchFilter<V>::DynamicNotchFilter()
	: mState( V() )
	, mInputIndex( 0 )
	, mAnalysisThread( new HookThread<DynamicNotchFilter<V>>( "dnf", this, &DynamicNotchFilter<V>::analysisRun ) )
{
	mNumSamples = 1000000 / Main::instance()->config()->Integer( "stabilizer.loop_time", 2000 );
	for ( uint8_t i = 0; i < 3; i++ ) {
		mDFTs[i].input = (float*)fftwf_malloc( sizeof(float) * mNumSamples );
		mDFTs[i].inputBuffer = (float*)fftwf_malloc( sizeof(float) * mNumSamples );
		mDFTs[i].output = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex) * ( mNumSamples / 2 + 1 ) );
		mDFTs[i].plan = fftwf_plan_dft_r2c_1d( mNumSamples, mDFTs[i].inputBuffer, mDFTs[i].output, FFTW_ESTIMATE );
		for ( uint8_t p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			mPeakFilters[i][p].filter = new BiquadFilter<float>( 1.0f );
		}
	}
}


template<typename V> void DynamicNotchFilter<V>::Start()
{
	Main::instance()->imu()->registerConsumer( std::bind( &DynamicNotchFilter<V>::imuConsumer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );
	mAnalysisThread->setFrequency( 200 );
	mAnalysisThread->Start();
}


template<typename V> V DynamicNotchFilter<V>::filter( const V& input, float dt )
{
	return input;
}


template<typename V> V DynamicNotchFilter<V>::state()
{
	return mState;
}


template<typename V> void DynamicNotchFilter<V>::imuConsumer( uint64_t tick, const Vector3f& gyro, const Vector3f& accel )
{
	std::lock_guard<std::mutex> lock( mInputMutex );
	mDFTs[0].input[mInputIndex] = gyro.x;
	mDFTs[1].input[mInputIndex] = gyro.y;
	mDFTs[2].input[mInputIndex] = gyro.z;
	mInputIndex = ( mInputIndex + 1 ) % mNumSamples;
}


template<typename V> bool DynamicNotchFilter<V>::analysisRun()
{
	const float dT = 1.0f / 200.0f;

	// Copy input samples to aligned FFTW3 buffer
	mInputMutex.lock();
	uint32_t startIdx = ( mInputIndex + 1 ) % mNumSamples;
	for ( uint8_t i = 0; i < 3; i++ ) {
		memcpy( mDFTs[i].inputBuffer, &mDFTs[i].input[startIdx], sizeof(float) * ( mNumSamples - startIdx ) );
		memcpy( &mDFTs[i].inputBuffer[( mNumSamples - startIdx )], mDFTs[i].input, sizeof(float) * startIdx );
	}
	mInputMutex.unlock();

	// Execute FFT
	for ( uint8_t i = 0; i < 3; i++ ) {
		fftwf_execute( mDFTs[i].plan );
	}

	std::vector<float> dfts[3];
	dfts[0] = std::vector<float>( mNumSamples / 2 + 1 );
	dfts[1] = std::vector<float>( mNumSamples / 2 + 1 );
	dfts[2] = std::vector<float>( mNumSamples / 2 + 1 );

	// Extract DFT magnitude components and initialize noise floor
	float noise[3] = { 0.0f, 0.0f, 0.0f };
	for ( uint8_t i = 0; i < 3; i++ ) {
		for ( uint32_t j = 0; j < mNumSamples / 2 + 1; j++ ) {
			dfts[i][j] = std::sqrt( mDFTs[i].output[j][0] * mDFTs[i].output[j][0] + mDFTs[i].output[j][1] * mDFTs[i].output[j][1] );
			noise[i] += dfts[i][j];
		}
	}

	// Detect top peaks
	const int maxPeaks = DYNAMIC_NOTCH_COUNT;
	const int peakDetectionSurround = 3;
	Peak peaks[3][DYNAMIC_NOTCH_COUNT];
	memset( peaks, 0, sizeof( peaks ) );
	for ( uint8_t i = 0; i < 3; i++ ) {
		for ( uint32_t j = peakDetectionSurround; j < mNumSamples / 2 + 1 - peakDetectionSurround; j++ ) {
			float localMean = 0.0f;
			for ( int k = -peakDetectionSurround; k <= peakDetectionSurround; k++ ) {
				localMean += dfts[i][j + k];
			}
			localMean /= 2 * peakDetectionSurround + 1;
			float val = dfts[i][j];
			if ( val > localMean ) {
				for ( uint8_t k = 0; k < maxPeaks; k++ ) {
					if ( val > peaks[i][k].magnitude ) {
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
			}
		}
	}

	// Sort peaks by index, ascending
	for ( uint8_t i = 0; i < 3; i++ ) {
		for ( uint8_t j = 0; j < maxPeaks; j++ ) {
			for ( uint8_t k = j + 1; k < maxPeaks; k++ ) {
				if ( peaks[i][j].dftIndex > peaks[i][k].dftIndex ) {
					Peak temp = peaks[i][j];
					peaks[i][j] = peaks[i][k];
					peaks[i][k] = temp;
				}
			}
		}
	}

	int peaksCount[3] = { 0, 0, 0 };
	float noiseFloor[3] = { 0.0f, 0.0f, 0.0f };

	// Approximate noise floor
	for ( uint8_t i = 0; i < 3; i++ ) {
		const std::vector<float>& dft = dfts[i];
		const Peak* axisPeaks = peaks[i];
		for (int p = 0; p < maxPeaks; p++) {
			if (axisPeaks[p].dftIndex > 0) {
				noiseFloor[i] -= 0.50f * dft[axisPeaks[p].dftIndex - 2];
				noiseFloor[i] -= 0.75f * dft[axisPeaks[p].dftIndex - 1];
				noiseFloor[i] -= dft[axisPeaks[p].dftIndex];
				noiseFloor[i] -= 0.75f * dft[axisPeaks[p].dftIndex + 1];
				noiseFloor[i] -= 0.50f * dft[axisPeaks[p].dftIndex + 2];
				peaksCount[i]++;
			}
		}
		noiseFloor[i] /= ( mNumSamples / 2 ) - peaksCount[i] + 1;
		noiseFloor[i] *= 2.0f;
	}

	//  Process peaks
	for ( uint8_t i = 0; i < 3; i++ ) {
		const std::vector<float>& dft = dfts[i];
		const Peak* axisPeaks = peaks[i];
		for ( int p = 0; p < peaksCount[i]; p++ ) {
			PeakFilter* peakFilter = nullptr;
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
			if ( peakFilter == nullptr ) {
				continue;
			}
			float cutoff = std::clamp( axisPeaks[p].magnitude / noiseFloor[i], 1.0f, 10.0f );
			float gain = 2.0f * float(M_PI) * cutoff * dT;
			gain = gain / ( gain + 1.0f );
			float centerFrequency = axisPeaks[p].frequency * mNumSamples / 2.0f;
			peakFilter->centerFrequency += gain * ( centerFrequency - peakFilter->centerFrequency );
			peakFilter->filter->setCenterFrequency( peakFilter->centerFrequency );
		}
	}

	{
		char speaks[1024] = "";
		for ( int p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			char s[16] = "";
			sprintf( s, "%.2f, ", mPeakFilters[0][p].centerFrequency );
			strcat( speaks, s );
		}
		gDebug() << "Peaks X : " << speaks;
	}
	{
		char speaks[1024] = "";
		for ( int p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			char s[16] = "";
			sprintf( s, "%.2f, ", mPeakFilters[1][p].centerFrequency );
			strcat( speaks, s );
		}
		gDebug() << "Peaks Y : " << speaks;
	}
	{
		char speaks[1024] = "";
		for ( int p = 0; p < DYNAMIC_NOTCH_COUNT; p++ ) {
			char s[16] = "";
			sprintf( s, "%.2f, ", mPeakFilters[2][p].centerFrequency );
			strcat( speaks, s );
		}
		gDebug() << "Peaks Z : " << speaks;
	}


	return true;
}


// template class DynamicNotchFilter<float>;
// template class DynamicNotchFilter<Vector2f>;
template class DynamicNotchFilter<Vector3f>;
// template class DynamicNotchFilter<Vector4f>;
