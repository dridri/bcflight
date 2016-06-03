#include <QtCore/QDebug>
#include <QtCore/QThread>
#include "ControllerPC.h"

ControllerPC::ControllerPC( Link* link )
	: Controller( link )
	, mThrust( 0.0f )
{
}


ControllerPC::~ControllerPC()
{
}


void ControllerPC::setControlThrust( const float v )
{
	mThrust = v;
}


float ControllerPC::ReadThrust()
{
	return mThrust;
}


float ControllerPC::ReadRoll()
{
	return 0.0f;
}


float ControllerPC::ReadPitch()
{
	return 0.0f;
}


float ControllerPC::ReadYaw()
{
	return 0.0f;
}


int8_t ControllerPC::ReadSwitch( uint32_t id )
{
	return 0;
}


ControllerMonitor::ControllerMonitor( ControllerPC* controller )
	: QThread()
	, mController( controller )
	, mKnowConnected( false )
{
}


ControllerMonitor::~ControllerMonitor()
{
}


void ControllerMonitor::run()
{
	struct sched_param sched;
	memset( &sched, 0, sizeof(sched) );
	sched.sched_priority = sched_get_priority_max( SCHED_RR );
	sched_setscheduler( 0, SCHED_RR, &sched );

	while ( 1 ) {
		if ( not mKnowConnected and mController->isConnected() ) {
			emit connected();
			mKnowConnected = true;
		}
		usleep( 1000 * 100 );
	}
}
