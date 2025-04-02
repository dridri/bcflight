#pragma once

#include "Lua.h"
#include "Matrix.h"
#include "Thread.h"
#include <vector>

using namespace std;


LUA_CLASS class AdjusterAI : public Thread
{
public:
	AdjusterAI( int nInput, int nHidden, int nOutput, float lr = 0.01f );
	~AdjusterAI();

	vector<float> forward( const vector<float>& input );

protected:
	virtual bool run();
	void updateWeights( const vector<float>& input, const vector<float>& target, float reward );

private:
	Matrix mInputWeights;
	Matrix mHiddenBiases;
	Matrix mOutputWeights;
	Matrix mOutputBiases;
	float mLearningRate;

	vector<float> mLastOutput;

	static float relu( float x ) {
		return max( 0.0f, x );
	}

	static float sigmoid(float x) {
		return 1.0f / (1.0f + exp(-x));
	}

	static float sigmoid_derivative(float x) {
		return x * (1.0f - x);
	}
};
