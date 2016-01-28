#include <unistd.h>
#include "Main.h"
#include "Controller.h"
#include <I2C.h>
#include <IMU.h>
#include <Sensor.h>
#include <Gyroscope.h>
#include <Accelerometer.h>
#include <Magnetometer.h>
#include <Voltmeter.h>
#include <CurrentSensor.h>
#include <fake_sensors/FakeAccelerometer.h>
#include <fake_sensors/FakeGyroscope.h>
#include <Servo.h>
#include <Stabilizer.h>
#include <Frame.h>
#ifdef CAMERA_INCLUDE
	#include CAMERA_INCLUDE // defined in CMakeLists.txt
#endif
#ifdef BUILD_SOCKET
	#include <Socket.h>
#endif
#ifdef BUILD_RAWWIFI
	#include <RawWifi.h>
#endif

int Main::flight_entry( int ac, char** av )
{
	Main* main = new Main();
	delete main;
	return 0;
}


Main::Main()
{
	uint64_t ticks = mBoard->GetTicks();
	uint64_t wait_ticks = mBoard->GetTicks();
	uint64_t fps_ticks = mBoard->GetTicks();
	int fps = 0;
	float dt = 0.0f;

#ifdef BOARD_generic
#pragma message "Adding noisy fake accelerometer and gyroscope"
	Sensor::AddDevice( new FakeAccelerometer( 3, Vector3f( 2.0f, 2.0f, 2.0f ) ) );
	Sensor::AddDevice( new FakeGyroscope( 3, Vector3f( 1.3f, 1.3f, 1.3f ) ) );
#endif

	mBoard = new Board( this );
	flight_register();
	DetectDevices();
	Board::InformLoading();

	mConfig = new Config( "config.lua" );
	mConfig->DumpVariable( "board" );
	mConfig->DumpVariable( "frame" );
	mConfig->DumpVariable( "controller" );
	mConfig->DumpVariable( "camera" );

	if ( mConfig->string( "board.type" ) != std::string( BOARD ) ) {
		gDebug() << "FATAL ERROR : Board type in configuration file ( \"" << mConfig->string( "board.type" ) << "\" ) differs from board being used ( \"" << BOARD << "\" ) !\n";
		return;
	}

	std::string frameName = mConfig->string( "frame.type" );
	if ( Frame::knownFrames().find( frameName ) == Frame::knownFrames().end() ) {
		gDebug() << "FATAL ERROR : unknown frame \"" << frameName << "\" !\n";
		return;
	}

	mPowerThread = new PowerThread( this );
	mPowerThread->Start();
	mPowerThread->setPriority( 95 );
	Board::InformLoading();

	mIMU = new IMU();
	Board::InformLoading();

	mFrame = Frame::Instanciate( frameName, mConfig );
	Board::InformLoading();

	mStabilizer = new Stabilizer( mFrame );
	Board::InformLoading();

#ifdef CAMERA
	Link* cameraLink = Link::Create( mConfig, "camera.link" );
	mCamera = new CAMERA( cameraLink );
	Board::InformLoading();
#endif

	Link* controllerLink = Link::Create( mConfig, "controller.link" );
	mController = new Controller( this, controllerLink );
	mController->setPriority( 97 );
	Board::InformLoading();

	Thread::setMainPriority( 99 );

	while ( 1 ) {
		dt = ((float)( mBoard->GetTicks() - ticks ) ) / 1000000.0f;
		ticks = mBoard->GetTicks();

		if ( std::abs( dt ) >= 1.0 ) {
			gDebug() << "Critical : dt too high !! ( " << dt << " )\n";
			continue;
		}

		mIMU->Loop( dt );
		if ( mIMU->state() == IMU::Calibrating or mIMU->state() == IMU::CalibratingAll ) {
			Board::InformLoading();
			mFrame->WarmUp();
		} else if ( mIMU->state() == IMU::CalibrationDone ) {
			Board::LoadingDone();
		} else {
// 			mController->UpdateSmoothControl( dt );
			mStabilizer->Update( mIMU, mController, dt );
			wait_ticks = mBoard->WaitTick( 2500, wait_ticks, -200 );
		}

		fps++;
		if ( mBoard->GetTicks() >= fps_ticks + 4 * 1000 * 1000 ) {
			gDebug() << "Update rate : " << ( fps / 4 ) << "\n";
			fps = 0;
			fps_ticks = mBoard->GetTicks();
		}
	}
}


Main::~Main()
{
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


void Main::DetectDevices()
{
	int countGyro = 0;
	int countAccel = 0;
	int countMagn = 0;
	int countVolt = 0;
	int countCurrent = 0;

	std::list< int > I2Cdevs = I2C::ScanAll();
	for ( int dev : I2Cdevs ) {
		Sensor::RegisterDevice( dev );
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
