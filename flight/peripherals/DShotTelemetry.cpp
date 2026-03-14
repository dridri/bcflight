#include "DShotTelemetry.h"
#include "DShot.h"
#include "Motor.h"
#include "Debug.h"
#include <Board.h>
#include <GPIO.h>
#include <iomanip>


DShotTelemetry* DShotTelemetry::sInstance = nullptr;


DShotTelemetry::DShotTelemetry()
	: Sensor()
	, Thread( "dshot_telemetry" )
	, mRPMFilter( nullptr )
{
	sInstance = this;
	mNames.emplace_back( "DShotTelemetry" );

	// Pre-allocate one voltmeter and current meter per DShot motor
	int idx = 0;
	for ( Motor* m : Motor::instances() ) {
		DShot* esc = dynamic_cast<DShot*>( m );
		if ( not esc ) continue;
		ESCData& d = mESCData[esc];
		d.voltmeter    = new ESCVoltmeter( idx );
		d.currentMeter = new ESCCurrentMeter( idx );
		mVoltmeters.push_back( d.voltmeter );
		mCurrentMeters.push_back( d.currentMeter );
		idx++;
	}

	mRPMFilter = new ESCRPMFilter( (int)mESCData.size() );

	Thread::setFrequency( 100 );
	Thread::Start();
	Thread::setPriority( 10 );
}


DShotTelemetry::~DShotTelemetry()
{
	delete mRPMFilter;
	mRPMFilter = nullptr;
	for ( auto& kv : mESCData ) {
		delete kv.second.voltmeter;
		delete kv.second.currentMeter;
	}
}


const DShotTelemetry::ESCData& DShotTelemetry::escData( DShot* esc ) const
{
	return mESCData.at( esc );
}

std::vector<Sensor*> DShotTelemetry::sensors() const
{
	std::vector<Sensor*> ret;
	for ( auto& kv : mESCData ) {
		if ( kv.second.voltmeter )    ret.push_back( kv.second.voltmeter );
		if ( kv.second.currentMeter ) ret.push_back( kv.second.currentMeter );
	}
	return ret;
}


bool DShotTelemetry::run()
{
	float dT = 1.0f / float(Thread::frequency());
	Serial* serial = dynamic_cast<Serial*>( Sensor::mBus );

	if ( not serial ) {
		Thread::usleep( 1000 * 1000 );
		return true;
	}
	if ( not serial->isConnected() ) {
		serial->Connect();
		serial->setReadTimeout( 20 );
		GPIO::setPUD( 15, GPIO::PullUp ); // Pull-up on RX pin for open-drain telemetry wire
	}
	if ( not serial->isConnected() ) {
		Thread::usleep( 1000 * 1000 );
		return true;
	}

	for ( Motor* m : Motor::instances() ) {
		DShot* esc = dynamic_cast<DShot*>( m );
		if ( not esc ) {
			continue;
		}

		serial->flushInput();
		esc->mRequestTelemetry.store( true );
		Thread::usleep( 200 );

		// Read bytes into a sliding window; accept first 10-byte window with valid CRC
		const int expected = 10;
		uint8_t window[expected * 2] = { 0 };
		int total = 0;
		int received = 0;
		while ( total < (int)sizeof(window) ) {
			// gDebug() << "serial : " << serial << " (total " << total << " received " << received << "/" << expected << ")";
			int n = serial->Read( window + total, 1 );
			if ( n > 0 ) {
				total++;
				if ( total >= expected ) {
					uint8_t* candidate = window + total - expected;
					if ( crc8( candidate, expected - 1 ) == candidate[expected - 1] ) {
						memcpy( window, candidate, expected );
						received = expected;
						break;
					}
				}
			} else {
				// gWarning() << "DShotTelemetry: ESC timeout " << esc->toString();
				mESCData[esc].valid = false;
				mESCData[esc].voltage = 0.0f;
				mESCData[esc].current = 0.0f;
				mESCData[esc].consumption = 0.0f;
				mESCData[esc].rpm = 0.0f;
				mESCData[esc].temperature = 0.0f;
				mESCData[esc].last_update_us = 0;
				break;
			}
		}
		uint8_t* buf = window;

		esc->mRequestTelemetry.store( false );
		serial->flushInput();

		stringstream hexdump;
		for ( int i = 0; i < received; i++ ) {
			hexdump << hex << setw(2) << setfill('0') << (int)buf[i] << " ";
		}
		// gDebug() << "DShotTelemetry: received " << received << " bytes from " << esc->toString() << ": " << hexdump.str();
		if ( received == expected && crc8( buf, expected - 1 ) == buf[expected - 1] ) {
			ESCData& d = mESCData[esc];
			d.temperature    = buf[0];
			d.voltage        = ( ( buf[1] << 8 ) | buf[2] ) / 100.0f;
			d.current        = ( ( buf[3] << 8 ) | buf[4] ) / 100.0f;
			d.consumption    = ( buf[5] << 8 ) | buf[6];
			d.rpm            = ( ( buf[7] << 8 ) | buf[8] ) * 100.0f;
			d.valid          = true;
			d.last_update_us = Board::GetTicks();
			if ( d.voltmeter )    d.voltmeter->mValue    = d.voltage;
			if ( d.currentMeter ) d.currentMeter->mValue = d.current;
			// gDebug()	<< "DShotTelemetry " << esc->toString() << ": "
			// 			<< "temp=" << d.temperature << "C "
			// 			<< "volt=" << d.voltage << "V "
			// 			<< "curr=" << d.current << "A "
			// 			<< "cons=" << d.consumption << "A "
			// 			<< "rpm=" << d.rpm;
		} else if ( received == expected ) {
			// gWarning() << "DShotTelemetry: CRC error " << esc->toString();
		}

		// Allow the stabilizer to send at least one non-telemetry frame before next request
		Thread::usleep( 200 );
	}

	if ( mPolesCount > 0 ) {
		vector<float> erpm_per_pole;
		for ( auto& escData : mESCData ) {
			erpm_per_pole.push_back( escData.second.rpm / ( (float)mPolesCount / 2.0f ) );
		}
		mRPMFilter->updateFilters( erpm_per_pole, dT );
	}

	return true;
}


std::vector<int32_t> DShotTelemetry::temperatures() const
{
	std::vector<int32_t> temps;
	for ( auto& kv : mESCData ) {
		temps.push_back( (int32_t)kv.second.temperature );
	}
	return temps;
}


std::string DShotTelemetry::toString()
{
	return "DShotTelemetry";
}


LuaValue DShotTelemetry::infos()
{
	LuaValue ret = LuaValue();

	ret["Motors"] = (int)mESCData.size();
	ret["Poles"] = mPolesCount;

	return ret;
}


uint8_t DShotTelemetry::crc8( const uint8_t* data, int len )
{
	uint8_t crc = 0;
	for ( int i = 0; i < len; i++ ) {
		crc ^= data[i];
		for ( int j = 0; j < 8; j++ ) {
			crc = ( crc & 0x80 ) ? 0x07 ^ ( crc << 1 ) : ( crc << 1 );
		}
	}
	return crc;
}
