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
#include <dirent.h>
#include <sys/stat.h>
#include "Main.h"
#include "Controller.h"
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
#include <Frame.h>
#include <Microphone.h>
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
	Main* main = new Main();
	delete main;
	return 0;
}


#include <OneShot125.h>
#include <BrushlessPWM.h>
#include "video/Camera.h"

void Test()
{
	Motor* os125 = new OneShot125( 23 );
// 	Motor* os125 = new BrushlessPWM( 18 );
	float s = 0.0f;
	const float v = 0.005f;
	float w = v;

	printf( "s = 1.0\n" ); fflush(stdout);
	os125->setSpeed( 1.0f, true );
	usleep( 1000 * 1000 * 10 );

	printf( "s = 0.0\n" ); fflush(stdout);
	os125->setSpeed( 0.0f, true );
	usleep( 1000 * 1000 * 10 );

	while ( 1 ) {
		printf( "s = %.4f\n", s ); fflush(stdout);
		os125->setSpeed( s, true );
		usleep( 1000 * 25 );
		s += w;
		if ( s >= 1.0f ) {
			w = -v;
			s = 1.0f;
		} else if ( s <= 0.1f ) {
			w = v;
			s = 0.1f;
			os125->Disarm();
			usleep(1000*1000);
		}
	}
}


Main::Main()
	: mLPS( 0 )
	, mLPSCounter( 0 )
	, mController( nullptr )
	, mCamera( nullptr )
	, mHUD( nullptr )
	, mMicrophone( nullptr )
	, mCameraType( "" )
{
	mInstance = this;
#ifdef BOARD_generic
#pragma message "Adding noisy fake accelerometer and gyroscope"
	Sensor::AddDevice( new FakeAccelerometer( 3, Vector3f( 2.0f, 2.0f, 2.0f ) ) );
	Sensor::AddDevice( new FakeGyroscope( 3, Vector3f( 1.3f, 1.3f, 1.3f ) ) );
#endif

	mBoard = new Board( this );
	flight_register();
	Board::InformLoading();

// 	Test();

#ifdef BOARD_generic
	mConfig = new Config( "config.lua" );
#else
	mConfig = new Config( "/var/flight/config.lua" );
#endif
	Board::InformLoading();
	mConfig->DumpVariable( "username" );
	mConfig->DumpVariable( "board" );
	mConfig->DumpVariable( "frame" );
	mConfig->DumpVariable( "battery" );
	mConfig->DumpVariable( "controller" );
	mConfig->DumpVariable( "camera" );
	mConfig->DumpVariable( "hud" );
	mConfig->DumpVariable( "accelerometers" );
	mConfig->DumpVariable( "gyroscopes" );
	mConfig->DumpVariable( "magnetometers" );
	mConfig->DumpVariable( "user_sensors" );

	if ( mConfig->string( "board.type", BOARD ) != std::string( BOARD ) ) {
		gDebug() << "FATAL ERROR : Board type in configuration file ( \"" << mConfig->string( "board.type" ) << "\" ) differs from board currently in use ( \"" << BOARD << "\" ) !\n";
		return;
	}

	Board::InformLoading();
	DetectDevices();
	Board::InformLoading();

	std::string frameName = mConfig->string( "frame.type" );
	auto knownFrames = Frame::knownFrames();
	if ( knownFrames.find( frameName ) == knownFrames.end() ) {
		gDebug() << "ERROR : unknown frame \"" << frameName << "\" !\n";
	}

	Board::InformLoading();
	mConfig->Apply();
	Board::InformLoading();

	mUsername = mConfig->string( "username" );

	mPowerThread = new PowerThread( this );
	mPowerThread->Start();
	mPowerThread->setPriority( 97 );
	Board::InformLoading();

	mIMU = new IMU( this );
	Board::InformLoading();

	mFrame = Frame::Instanciate( frameName, mConfig );
	Board::InformLoading();

	mStabilizer = new Stabilizer( this, mFrame );
	Board::InformLoading();

	mCameraType = mConfig->string( "camera.type" );
#ifdef CAMERA
	if ( mCameraType != "" ) {
		mCamera = new CAMERA( mConfig, "camera" );
		Board::InformLoading();
	} else {
		mCamera = nullptr;
	}
#else
	mCamera = nullptr;
#endif

	if ( mConfig->string( "microphone.type" ) != "" ) {
		mMicrophone = Microphone::Create( mConfig, "microphone" );
	}

	if ( mConfig->boolean( "hud.enabled", false ) ) {
		mHUD = new HUD();
	}

	Link* controllerLink = Link::Create( mConfig, "controller.link" );
	mController = new Controller( this, controllerLink );
	mController->setPriority( 99, 1 );
	Board::InformLoading();

	mLoopTime = mConfig->integer( "stabilizer.loop_time", 2000 );
	mTicks = 0;
	mWaitTicks = 0;
	mLPSTicks = 0;
	mLPS = 0;
	mStabilizerThread = new HookThread< Main >( "stabilizer", this, &Main::StabilizerThreadRun );
	mStabilizerThread->Start();
	mStabilizerThread->setPriority( 99, 0 );

	// Must be the very last atexit() call
	atexit( &Thread::StopAll );

	Thread::setMainPriority( 1 );
	while ( 1 ) {
		usleep( 1000 * 1000 * 100 );
	}
}


bool Main::StabilizerThreadRun()
{
	float dt = ((float)( mBoard->GetTicks() - mTicks ) ) / 1000000.0f;
	mTicks = mBoard->GetTicks();

	if ( std::abs( dt ) >= 1.0 ) {
		gDebug() << "Critical : dt too high !! ( " << dt << " )\n";
// 		mFrame->Disarm();
		return true;
	}

	mIMU->Loop( dt );
	if ( mIMU->state() == IMU::Calibrating or mIMU->state() == IMU::CalibratingAll ) {
		Board::InformLoading();
		mFrame->WarmUp();
	} else if ( mIMU->state() == IMU::CalibrationDone ) {
		Board::LoadingDone();
	} else {
		mStabilizer->Update( mIMU, mController, dt );
		mWaitTicks = mBoard->WaitTick( mLoopTime, mWaitTicks, -150 );
	}

	mLPSCounter++;
	if ( mBoard->GetTicks() >= mLPSTicks + 1000 * 1000 ) {
		mLPS = mLPSCounter;
		mLPSCounter = 0;
		mLPSTicks = mBoard->GetTicks();
	}
	return true;
}


Main::~Main()
{
}


std::string Main::getRecordingsList() const
{
	std::string ret;
	DIR* dir;
	struct dirent* ent;

	if ( ( dir = opendir( "/var/VIDEO/" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			struct stat st;
			stat( ( "/var/VIDEO/" + std::string( ent->d_name ) ).c_str(), &st );
			/*if ( mCamera ) {
				uint32_t width = 0;
				uint32_t height = 0;
				uint32_t bpp = 0;
				uint32_t* data = mCamera->getFileSnapshot( "/var/VIDEO/" + std::string( ent->d_name ), &width, &height, &bpp );
				std::string b64_data = base64_encode( (uint8_t*)data, width * height * ( bpp / 8 ) );
				ret += std::string( ent->d_name ) + ":" + std::to_string( st.st_size ) + ":" + std::to_string( width ) + ":" + std::to_string( height ) + ":" + std::to_string( bpp ) + ":" + b64_data + ";";
			} else */{
				ret += std::string( ent->d_name ) + ":" + std::to_string( st.st_size ) + ":::;";
			}
		}
		closedir( dir );
	}

	if ( ret.length() == 0 or ret == "" ) {
		ret = ";";
	}
	gDebug() << "Recordings : " << ret << "\n";
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


const std::string& Main::cameraType() const
{
	return mCameraType;
}


const std::string& Main::username() const
{
	return mUsername;
}


void Main::DetectDevices()
{
	int countGyro = 0;
	int countAccel = 0;
	int countMagn = 0;
	int countAlti = 0;
	int countGps = 0;
	int countVolt = 0;
	int countCurrent = 0;

	{
		std::list< Sensor::Device > knownDevices = Sensor::KnownDevices();
		Debug sensors;
		sensors << "Supported sensors :\n";
		for ( Sensor::Device dev : knownDevices ) {
			if ( std::string(dev.name) != "" ) {
				sensors << "    " << dev.name;
				if ( dev.iI2CAddr != 0 ) {
					sensors << " [I2C 0x" << std::hex << dev.iI2CAddr << "]";
				}
				sensors << "\n";
			}
		}
	}

	std::list< int > I2Cdevs = I2C::ScanAll();
	for ( int dev : I2Cdevs ) {
		std::string name = mConfig->string( "sensors_map_i2c[" + std::to_string(dev) + "]", "" );
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

	gDebug() << countGyro << " gyroscope(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Gyroscope* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countAccel << " accelerometer(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Accelerometer* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countMagn << " magnetometer(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Magnetometer* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countAlti << " altimeter(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Altimeter* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countGps << " GPS(es) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< GPS* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countVolt << " voltmeter(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< Voltmeter* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}

	gDebug() << countCurrent << " current sensor(s) found\n";
	for ( Sensor* s : Sensor::Devices() ) {
		if ( dynamic_cast< CurrentSensor* >( s ) != nullptr ) {
			gDebug() << "    " << s->names().front() << "\n";
		}
	}
}

static const std::string base64_chars = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
std::string Main::base64_encode( const uint8_t* buf, uint32_t size )
{
	std::string ret;
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
