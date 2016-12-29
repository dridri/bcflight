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

#include <execinfo.h>
#include <signal.h>

#include <iostream>
#include <Link.h>
#include <RawWifi.h>
#include <Socket.h>
#include "Config.h"
#include "ControllerClient.h"
#include "ui/GlobalUI.h"

int main( int ac, char** av )
{
	Controller* controller = nullptr;
	Link* controller_link = nullptr;

	if ( ac <= 1 ) {
		std::cout << "FATAL ERROR : No config file specified !\n";
		return -1;
	}

	Config* config = new Config( av[1] );
	config->Reload();
	config->LoadSettings();

	if ( config->string( "controller.link.link_type" ) == "Socket" ) {
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ) );
	} else {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), config->integer( "controller.link.output_port", 0 ), config->integer( "controller.link.input_port", 1 ), -1 );
	}

	controller = new ControllerClient( controller_link, config->boolean( "controller.spectate", false ) );

	GlobalUI* ui = new GlobalUI( controller );
	ui->Start();

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}
	std::cout << "Exiting (this should not happen)\n";
	return 0;
}
