/* BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <unistd.h>
#include <stdio.h>
#include <wiringPi.h>

#ifdef SYSTEM_NAME_Linux
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "Main.h"
#include "Config.h"
#include "Controller.h"
#include "Slave.h"
#include <SPI.h>
#include <I2C.h>
#include <IMU.h>
#include <Sensor.h>
#include <Gyroscope.h>
#include <Accelerometer.h>
#include <Magnetometer.h>
#include <Altimeter.h>
#include <GPS.h>
#include <Voltmeter.h>
#include <CurrentSensor.h>
#include <fake_sensors/FakeAccelerometer.h>
#include <fake_sensors/FakeGyroscope.h>
#include <Servo.h>
#include <Stabilizer.h>
// #include <StabilizerProxy.h>
#include <Frame.h>
#include <Camera.h>
#include <Microphone.h>
#include <Recorder.h>
#include <HUD.h>
#ifdef CAMERA_INCLUDE
	#include CAMERA_INCLUDE // defined in CMakeLists.txt
#endif
#ifdef BUILD_SOCKET
	#include <Socket.h>
#endif
#ifdef BUILD_RAWWIFI
	#include <RawWifi.h>
#endif

// #include "peripherals/WS2812.h" // TEST

Main* Main::mInstance = nullptr;


Main* Main::instance()
{
	return mInstance;
}


int Main::flight_entry( int ac, char** av )
{
	new Main();
	return 0;
}


Main::Main()
	: mReady( false )
	, mLPS( 0 )
	, mLPSCounter( 0 )
	, mSlave( nullptr )
	, mBlackBox( nullptr )
	, mStabilizer( nullptr )
	, mController( nullptr )
	, mCamera( nullptr )
	, mHUD( nullptr )
	, mMicrophone( nullptr )
	, mRecorder( nullptr )
	, mCameraType( "" )
{
	mInstance = this;
#ifdef BUILD_sensors
#ifdef BOARD_generic
#pragma message "Adding noisy fake accelerometer and gyroscope"
	Sensor::AddDevice( new FakeAccelerometer( 3, Vector3f( 2.0f, 2.0f, 2.0f ) ) );
	Sensor::AddDevice( new FakeGyroscope( 3, Vector3f( 1.3f, 1.3f, 1.3f ) ) );
#endif
#endif

#ifdef BUILD_blackbox
// 	mBlackBox = new BlackBox();
	Board::InformLoading();
#endif

	mBoard = new Board( this );
	// flight_register();
	Board::InformLoading();

#ifdef BOARD_generic
	mConfig = new Config( "config.lua", "settings.lua" );
#elif defined( SYSTEM_NAME_Linux )
	mConfig = new Config( "/var/flight/config.lua", "/var/flight/settings.lua" );
#else
// 	mConfig = new Config( "#0xADDRESS+length", "" ); // Access to config buffer at addess 0xADDRESS with length specified
	mConfig = new Config( "", "" );
#endif
	mConfig->Reload();
	Board::InformLoading();
/*
#ifdef FLIGHT_SLAVE
	mConfig->Execute( "_slave = " FLIGHT_SLAVE_CONFIG );
	gDebug() << mConfig->DumpVariable( "_slave" );
	mSlave = new Slave( mConfig, "_slave" );
#endif // FLIGHT_SLAVE

	if ( mConfig->ArrayLength( "slaves" ) > 0 ) {
		uint32_t len = mConfig->ArrayLength( "slaves" );
		for ( uint32_t i = 0; i < len; i++ ) {
			Slave* slave = new Slave( mConfig, "slaves[" + to_string(i) + "]", true );
			mSlaves.push_back( slave );
		}
	}
*/

	if ( mConfig->String( "board.type", BOARD ) != string( BOARD ) ) {
		gDebug() << "FATAL ERROR : Board type in configuration file ( \"" << mConfig->String( "board.type" ) << "\" ) does not match board currently in use ( \"" << BOARD << "\" ) !";
		return;
	}

	Board::InformLoading();
	DetectDevices();
	Board::InformLoading();

	Board::InformLoading();
	mConfig->Apply();
	Board::InformLoading();

	mUsername = mConfig->String( "username" );

#ifdef BUILD_power
	mPowerThread = new PowerThread( this );
	mPowerThread->setFrequency( 20 );
	mPowerThread->Start();
	mPowerThread->setPriority( 97 );
	Board::InformLoading();
#endif

/*
	string slavetype = mConfig->String( "stabilizer.proxy.type" );
	Bus* slaveBus = nullptr;
	if ( slavetype != "" ) {
		if ( slavetype == "SPI" ) {
			slaveBus = new SPI( mConfig->String( "stabilizer.proxy.address" ), mConfig->Integer( "stabilizer.proxy.speed", 500000 ) );
		}
		// TODO : handle other types of busses
// 		mIMU = new IMUProxy(); // TODO + create Slave class for slave-device, then periodically ask IMU values from master ( at max(10, telemetry_rate) )
	} else {
*/
#ifdef BUILD_stabilizer
		mIMU = mConfig->Object<IMU>( "imu" );
		mIMU->state();
		Board::InformLoading();
#endif
// 	}

#ifdef BUILD_frames
	mFrame = mConfig->Object<Frame>( "frame" );
	mFrame->WarmUp();
	Board::InformLoading();
#endif
/*
	if ( slaveBus ) {
		mStabilizer = new StabilizerProxy( this, slaveBus );
	} else {
*/
#ifdef BUILD_stabilizer
	mStabilizer = mConfig->Object<Stabilizer>( "stabilizer" );
	if ( mStabilizer->frame() == nullptr ) {
		mStabilizer->setFrame( mFrame );
	}
	Board::InformLoading();
#endif
// 	}

#ifdef CAMERA
	// mRecorder = new Recorder(); // TODO
	// mRecorder->Start();
#endif

#ifdef CAMERA
	mCamera = mConfig->Object<Camera>( "camera" );
	if ( mCamera ) {
		Board::InformLoading();
		mCamera->Start();
		Board::InformLoading();
	}
#endif


#ifdef BUILD_audio
	mMicrophone = mConfig->Object<Microphone>( "microphone" );
	Board::InformLoading();
#endif

	mHUD = mConfig->Object<HUD>( "hud" );
	Board::InformLoading();

#ifdef BUILD_controller
	mController = mConfig->Object<Controller>( "controller" );
	mController->setPriority( 98 );
	Board::InformLoading();
#endif


#ifdef BUILD_stabilizer
	mLoopTime = mConfig->Integer( "stabilizer.loop_time", 2000 );
	mTicks = 0;
	mWaitTicks = 0;
	mLPSTicks = 0;
	mLPS = 0;
	mStabilizerThread = new HookThread< Main >( "stabilizer", this, &Main::StabilizerThreadRun );
	mStabilizerThread->setFrequency( 100 );
	mStabilizerThread->Start();
	mStabilizerThread->setPriority( 99 );
#endif // BUILD_stabilizer

#ifdef SYSTEM_NAME_Linux
	// Must be the very last atexit() call
// 	atexit( &Thread::StopAll );
	atexit( []() {
		fDebug();
		Thread::StopAll();
	});
#endif

	mReady = true;
	Thread::setMainPriority( 1 );
}


const bool Main::ready() const
{
	return mReady;
}



bool Main::StabilizerThreadRun()
{
#ifdef BUILD_stabilizer
	float dt = ((float)( mBoard->GetTicks() - mTicks ) ) / 1000000.0f;
	mTicks = mBoard->GetTicks();

	if ( abs( dt ) >= 1.0 ) {
		gDebug() << "Critical : dt too high !! ( " << dt << " )";
// 		mFrame->Disarm();
		return true;
	}

	mIMU->Loop( dt );
	if ( mIMU->state() == IMU::Off ) {
		// Nothing to do
	} else if ( mIMU->state() == IMU::Calibrating or mIMU->state() == IMU::CalibratingAll ) {
		mStabilizerThread->setFrequency( 0 ); // Calibrate as fast as possible
		Board::InformLoading();
		mFrame->WarmUp();
	} else if ( mIMU->state() == IMU::CalibrationDone ) {
		Board::LoadingDone();
		mStabilizerThread->setFrequency( 1000000 / mLoopTime ); // Set frequency only when calibration is done
	} else {
		mStabilizer->Update( mIMU, mController, dt );
// 		mWaitTicks = mBoard->WaitTick( mLoopTime, mWaitTicks, -150 );
	}

	mLPSCounter++;
	if ( mBoard->GetTicks() >= mLPSTicks + 1000 * 1000 ) {
		mLPS = mLPSCounter;
		mLPSCounter = 0;
		mLPSTicks = mBoard->GetTicks();
		mBlackBox->Enqueue( "Stabilizer:lps", to_string(mLPS) );
	}

	return true;
#else // BUILD_stabilizer
	return false;
#endif // BUILD_stabilizer
}


Main::~Main()
{
}


string Main::getRecordingsList() const
{
	string ret;

#ifdef SYSTEM_NAME_Linux
	DIR* dir;
	struct dirent* ent;

	if ( ( dir = opendir( "/var/VIDEO/" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			struct stat st;
			stat( ( "/var/VIDEO/" + string( ent->d_name ) ).c_str(), &st );
			/*if ( mCamera ) {
				uint32_t width = 0;
				uint32_t height = 0;
				uint32_t bpp = 0;
				uint32_t* data = mCamera->getFileSnapshot( "/var/VIDEO/" + string( ent->d_name ), &width, &height, &bpp );
				string b64_data = base64_encode( (uint8_t*)data, width * height * ( bpp / 8 ) );
				ret += string( ent->d_name ) + ":" + to_string( st.st_size ) + ":" + to_string( width ) + ":" + to_string( height ) + ":" + to_string( bpp ) + ":" + b64_data + ";";
			} else */{
				ret += string( ent->d_name ) + ":" + to_string( st.st_size ) + ":::;";
			}
		}
		closedir( dir );
	}
#endif

	if ( ret.length() == 0 or ret == "" ) {
		ret = ";";
	}
	gDebug() << "Recordings : " << ret;
	return ret;
}


uint32_t Main::loopFrequency() const
{
	return mLPS;
}


Config* Main::config() const
{
	return mConfig;
}


PowerThread* Main::powerThread() const
{
	return mPowerThread;
}


Board* Main::board() const
{
	return mBoard;
}


BlackBox* Main::blackbox() const
{
	return mBlackBox;
}


IMU* Main::imu() const
{
	return mIMU;
}


Frame* Main::frame() const
{
	return mFrame;
}


Stabilizer* Main::stabilizer() const
{
	return mStabilizer;
}


Controller* Main::controller() const
{
	return mController;
}


Camera* Main::camera() const
{
	return mCamera;
}


HUD* Main::hud() const
{
	return mHUD;
}


Microphone* Main::microphone() const
{
	return mMicrophone;
}


Recorder* Main::recorder() const
{
	return mRecorder;
}


const string& Main::cameraType() const
{
	return mCameraType;
}


const string& Main::username() const
{
	return mUsername;
}


void Main::DetectDevices()
{
#ifdef BUILD_sensors
	int countGyro = 0;
	int countAccel = 0;
	int countMagn = 0;
	int countAlti = 0;
	int countGps = 0;
	int countVolt = 0;
	int countCurrent = 0;

	{
		list< Sensor::Device > knownDevices = Sensor::KnownDevices();
		gDebug() << "Supported sensors :";
		for ( Sensor::Device dev : knownDevices ) {
			if ( string(dev.name) != "" ) {
				Debug() << "    " << dev.name;
				if ( dev.iI2CAddr != 0 ) {
					Debug() << " [I2C 0x" << hex << dev.iI2CAddr << "]";
				}
				Debug() << "\n";
			}
		}
	}

	list< int > I2Cdevs = I2C::ScanAll();
	for ( int dev : I2Cdevs ) {
		string name = mConfig->String( "sensors_map_i2c[" + to_string(dev) + "]", "" );
// 		Sensor::RegisterDevice( dev, name, mConfig, "" );
		Sensor::RegisterDevice( dev, name );
	}
	// TODO : register SPI/1-wire/.. devices


	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Gyroscope* >( s ) != nullptr ) {
			countGyro++;
		}
		if ( dynamic_cast< Accelerometer* >( s ) != nullptr ) {
			countAccel++;
		}
		if ( dynamic_cast< Magnetometer* >( s ) != nullptr ) {
			countMagn++;
		}
		if ( dynamic_cast< Altimeter* >( s ) != nullptr ) {
			countAlti++;
		}
		if ( dynamic_cast< GPS* >( s ) != nullptr ) {
			countGps++;
		}
		if ( dynamic_cast< Voltmeter* >( s ) != nullptr ) {
			countVolt++;
		}
		if ( dynamic_cast< CurrentSensor* >( s ) != nullptr ) {
			countCurrent++;
		}
	}

	gDebug() << countGyro << " gyroscope(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Gyroscope* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countAccel << " accelerometer(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Accelerometer* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countMagn << " magnetometer(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Magnetometer* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countAlti << " altimeter(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Altimeter* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countGps << " GPS(es) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< GPS* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countVolt << " voltmeter(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Voltmeter* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}

	gDebug() << countCurrent << " current sensor(s) found";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< CurrentSensor* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front();
		}
	}
#endif // BUILD_sensors
}

static const string base64_chars = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
string Main::base64_encode( const uint8_t* buf, uint32_t size )
{
	string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while ( size-- ) {
		char_array_3[i++] = *(buf++);
		if ( i == 3 ) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			for ( i = 0; i < 4; i++ ) {
				ret += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if ( i ) {
		for(j = i; j < 3; j++)
		char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for ( j = 0; j < (i + 1); j++ ) {
			ret += base64_chars[char_array_4[j]];
		}

		while ( i++ < 3 ) {
			ret += '=';
		}
	}

	return ret;
}
