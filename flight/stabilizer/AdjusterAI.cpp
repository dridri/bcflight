#include "AdjusterAI.h"
#include "Main.h"
#include "Controller.h"
#include "IMU.h"


AdjusterAI::AdjusterAI( int nInput, int nHidden, int nOutput, float lr )
	: mInputWeights( nInput, nHidden )
	, mHiddenBiases( nHidden, 1 )
	, mOutputWeights( nHidden, nOutput )
	, mOutputBiases( nOutput, 1 )
	, mLearningRate( lr )
{
	mInputWeights.applyFunction([](float _) { return rand() / float(RAND_MAX) - 0.5f; });
	mOutputWeights.applyFunction([](float _) { return rand() / float(RAND_MAX) - 0.5f; });
	mHiddenBiases.applyFunction([](float _) { return rand() / float(RAND_MAX) - 0.5f; });
	mOutputBiases.applyFunction([](float _) { return rand() / float(RAND_MAX) - 0.5f; });
}


AdjusterAI::~AdjusterAI()
{
}


vector<float> AdjusterAI::forward( const vector<float>& input )
{
	Matrix hidden_inputs = mInputWeights.Transpose() * input + mHiddenBiases;
	hidden_inputs.applyFunction(sigmoid);

	Matrix outputs = mOutputWeights.Transpose() * hidden_inputs + mOutputBiases;
	outputs.applyFunction(sigmoid);

	return { outputs.data()[0], outputs.data()[1], outputs.data()[2] };
}


void AdjusterAI::updateWeights( const vector<float>& input, const vector<float>& target, float reward )
{
	Matrix inputs = Matrix( input );

	Matrix hidden_inputs = mInputWeights.Transpose() * inputs + mHiddenBiases;
	hidden_inputs.applyFunction(sigmoid);

	// Calculate hidden errors based on reward
	Matrix hidden_errors = hidden_inputs;
	hidden_errors.applyFunction([reward](float x) { return reward * x; });
	hidden_errors.applyFunction(sigmoid_derivative);

	// Update output weights
	Matrix output_deltas = target;
	output_deltas.applyFunction(sigmoid_derivative);
	output_deltas = output_deltas * (hidden_inputs.Transpose() * mLearningRate * reward);
	mOutputWeights += output_deltas;

	// Update input weights
	Matrix input_deltas = hidden_errors * (inputs.Transpose() * mLearningRate);
	mInputWeights += input_deltas;

	// Update biases
	mHiddenBiases += hidden_errors * mLearningRate;
	mOutputBiases += output_deltas * mLearningRate;
}


bool AdjusterAI::run()
{
	Main* main = Main::instance();
	Controller* controller = main->controller();
	IMU* imu = main->imu();

	// Simulate getting sensor data
	vector<float> sensor_data = imu->rate().toStdVector();

	// Calculate the instantaneous error
	float instant_error = /* calculate error based on command and sensor_data */;

	// Update the error history
	if (error_history.size() == window_size) {
		error_history.pop_front();
	}
	error_history.push_back(instant_error);

	// Calculate the moving average error
	float moving_average_error = 0.0f;
	for (const auto& error : error_history) {
		moving_average_error += error;
	}
	moving_average_error /= error_history.size();

	// Calculate the reward based on the moving average error
	float reward = -moving_average_error; // Negative error as reward

	// Update the neural network based on the reward
	updateWeights( sensor_data, mLastOutput, reward );

	// Get the new PID parameters
	Matrix new_pid_matrix = forward( sensor_data );
	vector<float> new_pid_params(new_pid_matrix.data(), new_pid_matrix.data() + new_pid_matrix.size());

	// Calculate the weight based on the commands
	float weight = calculate_weight(commands);

	// Combine the current PID parameters with the new PID parameters based on the weight
	lock_guard<mutex> lock(mtx);
	for (int i = 0; i < pid_params.size(); i++) {
		pid_params[i] = pid_params[i] * (1 - weight) + new_pid_params[i] * weight;
	}

	return true;
}
