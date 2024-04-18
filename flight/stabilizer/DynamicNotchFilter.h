#pragma once

#include "Lua.h"
#include "Filter.h"
#include "Debug.h"
#include <Thread.h>
#include <BiquadFilter.h>

typedef struct fftwf_plan_s* fftwf_plan;
typedef float fftwf_complex[2];

#define DYNAMIC_NOTCH_COUNT 8

LUA_CLASS template<typename V> class DynamicNotchFilter : public Filter<V>
{
public:
	DynamicNotchFilter();
	// DynamicNotchFilter();
	virtual V filter( const V& input, float dt );
	virtual V state();
	void Start();

protected:
	typedef struct {
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

	V mState;
	uint32_t mNumSamples;
	uint32_t mInputIndex;
	std::mutex mInputMutex;
	DFT mDFTs[3];
	PeakFilter mPeakFilters[3][DYNAMIC_NOTCH_COUNT];
	HookThread<DynamicNotchFilter<V>>* mAnalysisThread;

	void imuConsumer( uint64_t tick, const Vector3f& gyro, const Vector3f& accel );
	bool analysisRun();

	float mul( const float& a, const float& b ) {
		return a * b;
	}
	template<typename T, int n> Vector<T, n> mul( const Vector<T, n>& a, const Vector<T, n>& b ) {
		return a & b;
	}
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

LUA_CLASS class DynamicNotchFilter_3 : public DynamicNotchFilter<Vector3f>
{
public:
	LUA_EXPORT DynamicNotchFilter_3() : DynamicNotchFilter() {}
};
