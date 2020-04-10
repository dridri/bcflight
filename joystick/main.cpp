#include <iostream>
#include <cmath>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <linux/uinput.h>
#include <libudev.h>
#include "Socket.h"

static int create_joystick()
{
	int fd = open( "/dev/uinput", O_WRONLY | O_NONBLOCK );

	ioctl( fd, UI_SET_EVBIT, EV_KEY );
	ioctl( fd, UI_SET_KEYBIT, BTN_A );

	ioctl( fd, UI_SET_EVBIT, EV_ABS );
	ioctl( fd, UI_SET_ABSBIT, ABS_X );
	ioctl( fd, UI_SET_ABSBIT, ABS_Y );
	ioctl( fd, UI_SET_ABSBIT, ABS_Z );
	ioctl( fd, UI_SET_ABSBIT, ABS_THROTTLE );

	struct uinput_user_dev uidev;
	memset( &uidev, 0, sizeof(uidev) );
	snprintf( uidev.name, UINPUT_MAX_NAME_SIZE, "BeyondChaos Flight Controller" );
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 3;
	uidev.id.product = 3;
	uidev.id.version = 2;

	for ( int i = 0; i < ABS_CNT; i++ ) {
		uidev.absmax[i] = 32767;
		uidev.absmin[i] = -32767;
		uidev.absfuzz[i] = 0;
		uidev.absflat[i] = 15;
	}

	write( fd, &uidev, sizeof(uidev) );
	ioctl( fd, UI_DEV_CREATE );

	return fd;
}

typedef struct {
	int8_t throttle;
	int8_t roll;
	int8_t pitch;
	int8_t yaw;
} __attribute__((packed)) ControllerData;


int main( int argc, char **argv )
{
	int uifd = create_joystick();
	struct input_event evt;

	Socket* sock = new Socket( "192.168.32.2", 5000 );
	ControllerData data;

	while ( 1 ) {
		while ( not sock->isConnected() and sock->Connect() < 0 ) {
			usleep( 1000 * 1000 * 4 );
		}
		sock->Receive( &data, sizeof(data) );

		memset( &evt, 0, sizeof(struct input_event) );
		evt.type = EV_ABS;
		evt.code = ABS_THROTTLE;
		evt.value = ( (int16_t)data.throttle * 255 ) * 2 - 32767;
		write( uifd, &evt, sizeof(evt) );

		memset( &evt, 0, sizeof(struct input_event) );
		evt.type = EV_ABS;
		evt.code = ABS_X;
		evt.value = ( (int16_t)data.roll * 255 );
		write( uifd, &evt, sizeof(evt) );

		memset( &evt, 0, sizeof(struct input_event) );
		evt.type = EV_ABS;
		evt.code = ABS_Y;
		evt.value = ( (int16_t)data.pitch * 255 );
		write( uifd, &evt, sizeof(evt) );

		memset( &evt, 0, sizeof(struct input_event) );
		evt.type = EV_ABS;
		evt.code = ABS_Z;
		evt.value = ( (int16_t)data.yaw * 255 );
		write( uifd, &evt, sizeof(evt) );
	}

	return 0;
}
