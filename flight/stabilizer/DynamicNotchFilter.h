#pragma once

#include <cassert>
#include "Lua.h"
#include "Filter.h"
#include "Debug.h"
#include <Thread.h>
#include <BiquadFilter.h>

typedef struct fftwf_plan_s* fftwf_plan;
typedef float fftwf_complex[2];

#define DYNAMIC_NOTCH_COUNT 4

template<typename V> class _DynamicNotchFilterBase : public Filter<V>
{
public:
	_DynamicNotchFilterBase( uint8_t n );
	virtual ~_DynamicNotchFilterBase() {}

	template<typename T, uint8_t N> Vector<T, N> filter( const Vector<T, N>& input, float dt );
	virtual V state();
	void Start();

	template<typename T, uint8_t N> std::vector<Vector<T, N>> dftOutput() {
		assert(N == mN);
		std::vector<Vector<T, N>> ret;
		ret.reserve( mNumSamples / 2 + 1 );
		for ( uint32_t j = 0; j < mNumSamples / 2 + 1; j++ ) {
			ret.push_back(Vector<T,N>());
			#pragma GCC unroll 4
			for ( uint8_t i = 0; i < N; i++ ) {
				ret[j][i] = mDFTs[i].magnitude[j];
			}
		}
		return ret;
	}

protected:
	typedef struct DFT {
		float* input;
		float* inputBuffer;
		fftwf_complex* output;
		fftwf_plan plan;
		float* magnitude;
	} DFT;
	typedef struct Peak {
		uint32_t dftIndex;
		float frequency;
		float magnitude;
	} Peak;
	typedef struct PeakFilter {
		// float centerFrequency;
		BiquadFilter<float>* filter;
	} PeakFilter;

	uint32_t mMinFreq;
	uint32_t mMaxFreq;
	uint8_t mN;
	V mState;
	float mFixedDt;
	uint32_t mNumSamples;
	uint32_t mSampleResolution;
	uint32_t mInputIndex;
	std::mutex mInputMutex;
	std::vector<DFT> mDFTs;
	std::vector<std::vector<PeakFilter>> mPeakFilters;
	HookThread<_DynamicNotchFilterBase<V>>* mAnalysisThread;

	bool analysisRun();
};


LUA_CLASS template<typename V> class DynamicNotchFilter : public _DynamicNotchFilterBase<V>
{
public:
	DynamicNotchFilter();
	~DynamicNotchFilter() {}

	void pushSample( const V& sample );

	virtual V filter( const V& input, float dt );
};


// LUA_CLASS class DynamicNotchFilter_1 : public DynamicNotchFilter<float>
// {
// public:
// 	LUA_EXPORT DynamicNotchFilter_1() : DynamicNotchFilter() {}
// };
// 
// LUA_CLASS class DynamicNotchFilter_2 : public DynamicNotchFilter<Vector2f>
// {
// public:
// 	LUA_EXPORT DynamicNotchFilter_2() : DynamicNotchFilter() {}
// };

LUA_CLASS class DynamicNotchFilter_3 : public DynamicNotchFilter<Vector<float, 3>>
{
public:
	LUA_EXPORT DynamicNotchFilter_3();

	LUA_PROPERTY("min_frequency") void setMinFrequency( uint32_t f ) { mMinFreq = f; }
	LUA_PROPERTY("max_frequency") void setMaxFrequency( uint32_t f ) { mMaxFreq = f; }
};
