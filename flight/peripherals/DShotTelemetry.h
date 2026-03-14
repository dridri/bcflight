#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include <Thread.h>
#include <Serial.h>
#include <Voltmeter.h>
#include <CurrentSensor.h>
#include <Filter.h>
#include "Sensor.h"
#include <BiquadFilter.h>
#include <Main.h>

class DShot;

LUA_CLASS class DShotTelemetry : public Sensor, public Thread
{
public:
	LUA_EXPORT DShotTelemetry();
	~DShotTelemetry();

	void Calibrate( float dt, bool last_pass = false ) {}
	LuaValue infos();
	std::string toString();

	// Inner sensor classes — one instance per ESC, created in constructor
	class ESCVoltmeter : public Voltmeter {
		friend class DShotTelemetry;
	public:
		ESCVoltmeter( int iesc = 0 ) {
			mNames.push_back( "ESCVoltmeter[" + std::to_string(iesc) + "]" );
		}
		void Calibrate( float dt, bool last_pass = false ) {}
		float Read( int channel = 0 ) override { return mValue; }
		string toString() override { return "ESCVoltmeter"; }
		LuaValue infos() override {
			LuaValue ret = LuaValue();
			ret["type"] = "ESCVoltmeter";
			return ret;
		}
	private:
		float mValue = 0.0f;
	};

	class ESCCurrentMeter : public CurrentSensor {
		friend class DShotTelemetry;
	public:
		ESCCurrentMeter( int iesc = 0 ) {
			mNames.push_back( "ESCCurrentMeter[" + std::to_string(iesc) + "]" );
		}
		void Calibrate( float dt, bool last_pass = false ) {}
		float Read( int channel = 0 ) override { return mValue; }
		string toString() override { return "ESCCurrentMeter"; }
		LuaValue infos() override {
			LuaValue ret = LuaValue();
			ret["type"] = "ESCCurrentMeter";
			return ret;
		}
	private:
		float mValue = 0.0f;
	};

	class ESCRPMFilter : public Filter<Vector3f> {
		friend class DShotTelemetry;
	public:
		ESCRPMFilter( int motorsCount ) : mFixedDt( 0.0f ), mMotorsCount( motorsCount ) {
			mFilters.reserve( mMotorsCount * mHarmonicCount );
			for ( int i = 0; i < mMotorsCount * mHarmonicCount; i++ ) {
				mFilters.emplace_back( BiquadFilter_3( 4.0f, 0.0f ) );
			}
		}
		Vector3f filter( const Vector3f& input, float dt ) override {
			if ( mFixedDt <= 0.0f ) {
				uint32_t nSamples = 1000000 / Main::instance()->config()->Integer( "stabilizer.loop_time", 2000 );
				mFixedDt = 1.0f / nSamples;
				gDebug() << "mFixedDt : " << mFixedDt << "s (nSamples: " << nSamples << ")";
			}
			Vector3f v = input;
			for ( auto& f : mFilters ) {
				v = f.filter( v, mFixedDt );
			}
			// 0.00025
			return v;
		}
		Vector3f state() override { return mFilters.empty() ? Vector3f() : mFilters[0].state(); }
	private:
		float mFixedDt;
		const int mHarmonicCount = 4;
		int mMotorsCount = 0;
		vector<BiquadFilter_3> mFilters;
		void updateFilters( const vector<float>& erpm_per_pole, float dT ) {
			for ( int i = 0; i < mMotorsCount; i++ ) {
				for ( int h = 0; h < mHarmonicCount; h++ ) {
					// if ( i == 0 ) {
					// 	gDebug() << "ESCRPMFilter: Setting center frequency for harmonic " << h << " to " << erpm_per_pole[i] * ( h + 1 ) / 60.0f << " Hz (ERPM per pole: " << erpm_per_pole[i] << ")";
					// }
					if( erpm_per_pole[i] == 0.0f ) {
						mFilters[i * mHarmonicCount + h].setCenterFrequency( 0.0f );
						continue;
					}
					mFilters[i * mHarmonicCount + h].setCenterFrequency( erpm_per_pole[i] * ( h + 1 ) / 60.0f, dT );
				}
			}
		}
	};

	typedef struct {
		float temperature = 0.0f;
		float voltage = 0.0f;
		float current = 0.0f;
		float consumption = 0.0f;
		float rpm = 0.0f;
		bool valid = false;
		uint64_t last_update_us = 0;
		ESCVoltmeter* voltmeter = nullptr;
		ESCCurrentMeter* currentMeter = nullptr;
	} ESCData;

	const std::map<DShot*, ESCData>& escData() const { return mESCData; }
	const ESCData& escData( DShot* esc ) const;
	LUA_PROPERTY("sensors") std::vector<Sensor*> sensors() const;
	LUA_PROPERTY("rpm_filter") DShotTelemetry::ESCRPMFilter* rpmFilter() const { return mRPMFilter; }
	LUA_PROPERTY("voltmeters") std::vector<DShotTelemetry::ESCVoltmeter*> voltmeters() const { return mVoltmeters; }
	LUA_PROPERTY("current_sensors") std::vector<DShotTelemetry::ESCCurrentMeter*> currentSensors() const { return mCurrentMeters; }

	LUA_PROPERTY("temperatures") std::vector<int32_t> temperatures() const;

	static DShotTelemetry* instance() { return sInstance; }

protected:
	bool run() override;

protected:
	static uint8_t crc8( const uint8_t* data, int len );

	static DShotTelemetry* sInstance;
	std::map<DShot*, ESCData> mESCData;

	LUA_PROPERTY("poles_count") int mPolesCount;
	std::vector<ESCVoltmeter*> mVoltmeters;
	std::vector<ESCCurrentMeter*> mCurrentMeters;
	ESCRPMFilter* mRPMFilter;
};
