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
#include "ControllerClient.h"
#include "ui/GlobalUI.h"
#include "Config.h"
#include <RawWifi.h>
#include <Socket.h>
#include <nRF24L01.h>
#include <SX127x.h>

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
		::Socket::PortType type = ::Socket::TCP;
		if ( config->string( "controller.link.type" ) == "UDP" ) {
			type = ::Socket::UDP;
		} else if ( config->string( "controller.link.type" ) == "UDPLite" ) {
			type = ::Socket::UDPLite;
		}
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ), type );
	} else if ( config->string( "controller.link.link_type" ) == "nRF24L01" ) {
		std::string device = config->string( "controller.link.device", "spidev1." );
		int cspin = config->integer( "controller.link.cspin", -1 );
		int cepin = config->integer( "controller.link.cepin", -1 );
		int irqpin = config->integer( "controller.link.irqpin", -1 );
		int channel = config->integer( "controller.link.channel", 100 );
		int input_port = config->integer( "controller.link.input_port", 1 );
		int output_port = config->integer( "controller.link.output_port", 0 );
		bool drop_invalid_packets = config->boolean( "controller.link.drop", true );
		bool blocking = config->boolean( "controller.link.blocking", true );
		if ( cspin < 0 ) {
			std::cout << "WARNING : No CS-pin specified for nRF24L01, cannot create link !\n";
		} else if ( cepin < 0 ) {
			std::cout << "WARNING : No CE-pin specified for nRF24L01, cannot create link !\n";
		} else {
			controller_link = new nRF24L01( device, cspin, cepin, irqpin, channel, input_port, output_port, drop_invalid_packets );
			controller_link->setBlocking( blocking );
		}
	} else if ( config->string( "controller.link.link_type" ) == "SX127x" ) {
		SX127x::Config conf;
		conf.device = config->string( "controller.link.device", "spidev1.1" );
		conf.resetPin = config->integer( "controller.link.resetpin", -1 );
		conf.txPin = config->integer( "controller.link.txpin", -1 );
		conf.rxPin = config->integer( "controller.link.rxpin", -1 );
		conf.irqPin = config->integer( "controller.link.irqpin", -1 );
		conf.frequency = config->integer( "controller.link.frequency", 433000000 );
		conf.inputPort = config->integer( "controller.link.input_port", 1 );
		conf.outputPort = config->integer( "controller.link.output_port", 0 );
		conf.dropBroken = config->boolean( "controller.link.drop", true );
		conf.blocking = config->boolean( "controller.link.blocking", true );
		conf.retries = config->integer( "controller.link.retries", 1 );
		conf.bitrate = config->integer( "controller.link.bitrate", 76800 );
		conf.modem = SX127x::FSK;
		controller_link = new SX127x( conf );
	} else {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), config->integer( "controller.link.output_port", 0 ), config->integer( "controller.link.input_port", 1 ), -1 );
		static_cast< RawWifi* >( controller_link )->SetChannel( config->integer( "controller.link.channel", 11 ) );
	}

	controller = new ControllerClient( config, controller_link, config->boolean( "controller.spectate", false ) );
	controller->setUpdateFrequency( config->integer( "controller.update_frequency", 100 ) );

	GlobalUI* ui = new GlobalUI( config, controller );
	ui->Start();

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}
	std::cout << "Exiting (this should not happen)\n";
	return 0;
}
