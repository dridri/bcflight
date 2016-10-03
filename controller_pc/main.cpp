/*
 * BCFlight
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>

#include <RawWifi.h>
#include <Socket.h>
#include "ControllerPC.h"

#include "MainWindow.h"

int main( int ac, char** av )
{
	const char* device = "wlan0";
	bool spectate = false;
	while ( ac > 1 ) {
		ac--;
		if ( av[ac][0] == '-' && av[ac][1] == '-' ) {
			if ( !strcmp( av[ac], "--spectate" ) ) {
				spectate = true;
			}
		} else {
			device = av[ac];
		}
	}

	QApplication app( ac, av );
	ControllerPC* controller = nullptr;

// 	if ( spectate == false ) {
		Link* controller_link = new RawWifi( device, 0, 1 );
		static_cast< RawWifi* >( controller_link )->setRetriesCount( 2 );
		static_cast< RawWifi* >( controller_link )->setCECMode( "weighted" );
	// 	Link* controller_link = new Socket( "127.0.0.1", 2020 );
	// 	Link* controller_link = new Socket( "192.168.32.1", 2020 );
		controller = new ControllerPC( controller_link, spectate );
// 	}

	Link* stream_link = new RawWifi( device, 10, 11 );
// 	Link* stream_link = new Socket( "127.0.0.1", 2021, Socket::UDPLite );
// 	Link* stream_link = new Socket( "192.168.32.1", 2021, Socket::UDPLite );

	MainWindow win( controller, stream_link );
	win.show();

	return app.exec();
}
