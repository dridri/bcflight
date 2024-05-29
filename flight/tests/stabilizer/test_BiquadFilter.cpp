#include <catch2/catch_all.hpp>
#include <stabilizer/BiquadFilter.h>
#include <random>

using namespace Catch::literals;


TEST_CASE("BiquadFilter Notch", "[BiquadFilter]") {
	BiquadFilter<float> filter( 0.707f );
	filter.setCenterFrequency( 100.0f );

	float input = 1.0f;
	float output = filter.filter( input, 0.002f );
	REQUIRE(output == Catch::Approx(0.6).epsilon(0.1));
}


TEST_CASE("BiquadFilter Notch sine", "[BiquadFilter]") {
	BiquadFilter<float> filter( 0.707f );
	filter.setCenterFrequency( 100.0f, 0.00001f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < 1000000; t++ ) {
		float input = std::sin( float(t) * 100.0f / 1000000.0f );
		output = filter.filter( input, 0.002f );
	}
	REQUIRE(output == Catch::Approx(-0.5).epsilon(0.1));
}

/*
TEST_CASE("BiquadFilter Notch sine variable dT", "[BiquadFilter]") {
	const int32_t frequency = 1000;
	const float base_dt = 1.0f / float(frequency);
	const float dt_variation = 0.25f; // 25%
	const float sine_frequency = 100.0f;
	const float sine_amplitude = 1.0f; // 5.0f;
	const float notch_frequency = 100.0f;

    std::mt19937 generator( 42 );
	std::uniform_real_distribution<float> distribution( -base_dt*dt_variation, base_dt*dt_variation );
	BiquadFilter<float> filter( 0.707f );
	filter.setCenterFrequency( notch_frequency, 0.1f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency; t++ ) {
		float input = sine_amplitude * std::sin( 2.0f * float(M_PI) * float(t) * sine_frequency / (float)frequency );
		float dt = base_dt + distribution(generator);
		output = filter.filter( input, dt );
		printf( "[%dHz] %f → %f\n", int(1.0f / dt), input, output );
	}
	REQUIRE(output == Catch::Approx(-42).epsilon(0.1));
}
*/


TEST_CASE("BiquadFilter 100Hz notch on 100Hz sine", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.0f / 1000.0f;
	const float sine_frequency = 100.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 100.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const float sine_max_filtered_amplitude = 2.0f;
	const float loopDT = 1.0f / float(frequency);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude );
		}
	}
}

TEST_CASE("BiquadFilter 10Hz notch on 10Hz sine", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.0f / 1000.0f;
	const float sine_frequency = 10.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 10.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const float sine_max_filtered_amplitude = 2.0f;
	const float loopDT = 1.0f / float(frequency);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude );
		}
	}
}

TEST_CASE("BiquadFilter 10Hz notch on 100Hz sine", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.0f / 1000.0f;
	const float sine_frequency = 100.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 10.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const auto sine_max_filtered_amplitude = (5.0_a).epsilon(0.1f);
	const float loopDT = 1.0f / float(frequency);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude );
		}
	}
}

TEST_CASE("BiquadFilter 100Hz notch on 100Hz sine, dt off by 10%", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.1f / 1000.0f;
	const float sine_frequency = 100.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 100.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const float sine_max_filtered_amplitude = 2.0f;
	const float loopDT = 1.0f / float(frequency);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude );
		}
	}
}

TEST_CASE("BiquadFilter 100Hz notch on 100Hz sine, dt off by 20%", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.2f / 1000.0f;
	const float sine_frequency = 100.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 100.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const float sine_max_filtered_amplitude = 2.5f;
	const float loopDT = 1.0f / float(frequency);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude );
		}
	}
}

TEST_CASE("BiquadFilter 20Hz notch on 20Hz sine, noisy", "[BiquadFilter]") {
	const uint32_t frequency = 1000;
	const float dt = 1.2f / 1000.0f;
	const float sine_frequency = 20.0f;
	const float sine_amplitude = 5.0f;
	const float notch_frequency = 20.0f;
	const float notch_frequency_var_freq = 40.0f;
	const float notch_frequency_var_amp = 20.0f;
	const float notch_frequency_gain = 10.0f;
	const float sine_max_filtered_amplitude = 2.5f;
	const float loopDT = 1.0f / float(frequency);

    std::mt19937 gen(42);
    std::normal_distribution<float> whiteNoiseGen(0.0f, 1.0f);

	BiquadFilter<float> filter( 0.707f );

	float output = 0.0f;
	for ( uint32_t t = 0; t < frequency * 100; t++ ) {
		float pi2t = 2.0f * float(M_PI) * float(t);
        float white_noise = std::clamp( whiteNoiseGen(gen) * 0.5f, -1.0f, 1.0f );
		float input = sine_amplitude * std::sin( pi2t * sine_frequency / (float)frequency );
		float centerFreq = notch_frequency + notch_frequency_var_amp * std::cos( pi2t * notch_frequency_var_freq / float(frequency) );
		filter.setCenterFrequency( centerFreq, notch_frequency_gain * loopDT );
		output = filter.filter( white_noise + input, dt );
		if ( t > frequency / int(notch_frequency) ) {
			REQUIRE( std::abs(output) <= sine_max_filtered_amplitude + std::abs(white_noise) );
		}
	}
}
