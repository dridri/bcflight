#include <unistd.h>
#include <Debug.h>
#include <Servo.h>
#include "Frame.h"
#include <Config.h>

std::map< std::string, std::function< Frame* ( Config* ) > > Frame::mKnownFrames;

Frame::Frame()
	: mMotors( std::vector< Motor* >() )
{
}


Frame::~Frame()
{
}


void Frame::CalibrateESCs()
{
	fDebug0();

	for ( Motor* m : mMotors ) {
		m->setSpeed( 1.0f, true );
	}
	Servo::HardwareSync();

	usleep( 1000 * 1000 );
	for ( Motor* m : mMotors ) {
		m->setSpeed( 0.0f, true );
	}
	Servo::HardwareSync();

	usleep( 1000 * 1000 );
	for ( Motor* m : mMotors ) {
		m->Disarm();
	}
	Servo::HardwareSync();
}


Frame* Frame::Instanciate( const std::string& name, Config* config )
{
	if ( mKnownFrames.find( name ) != mKnownFrames.end() ) {
		return mKnownFrames[ name ]( config );
	}
	return nullptr;
}


const std::vector< Motor* >& Frame::motors() const
{
	return mMotors;
}


void Frame::RegisterFrame( const std::string& name, std::function< Frame* ( Config* ) > instanciate )
{
	mKnownFrames[ name ] = instanciate;
}
