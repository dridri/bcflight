#pragma once

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

protected:
	typedef struct DFT {
		float* input;
		float* inputBuffer;
		fftwf_complex* output;
		fftwf_plan plan;
	} DFT;
	typedef struct Peak {
		uint32_t dftIndex;
		float frequency;
		float magnitude;
	} Peak;
	typedef struct PeakFilter {
		float centerFrequency;
		BiquadFilter<float>* filter;
	} PeakFilter;

	uint8_t mN;
	V mState;
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
	// template<typename T, uint8_t N> void pushSample( const Vector<T, N>& sample );

	virtual V filter( const V& input, float dt );
};

/*
template<typename T, template <typename, uint8_t> class Vector, uint8_t N> class DynamicNotchFilter<Vector<T, N>> : public _DynamicNotchFilterBase<Vector<T, N>>
{
public:
	DynamicNotchFilter() : _DynamicNotchFilterBase<Vector<T,N>>( N ) {}
	~DynamicNotchFilter() {}
	void pushSample( const Vector<T, N>& sample );
	virtual Vector<T, N> filter( const Vector<T, N>& input, float dt );
protected:
	void _pushSample( const Vector<T, N>& sample );
};
*/


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
};
