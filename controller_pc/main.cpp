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
	if ( ac > 1 ) {
		device = av[1];
	}

	QApplication app( ac, av );
/*
	QPalette palette;
	palette.setColor( QPalette::Window, QColor( 49, 54, 59 ) );
	palette.setColor( QPalette::WindowText, Qt::white );
	palette.setColor( QPalette::Base, QColor( 35, 38, 41 ) );
	palette.setColor( QPalette::AlternateBase, QColor( 35, 38, 41 ) );
	palette.setColor( QPalette::ToolTipBase, Qt::white );
	palette.setColor( QPalette::ToolTipText, Qt::white );
	palette.setColor( QPalette::Text, Qt::white );
	palette.setColor( QPalette::Button, QColor( 31, 36, 41 ) );
	palette.setColor( QPalette::ButtonText, Qt::white );
	palette.setColor( QPalette::BrightText, Qt::blue );
	palette.setColor( QPalette::Highlight, QColor( 142, 0, 0 ).lighter( ) );
	palette.setColor( QPalette::HighlightedText, Qt::black );
	palette.setColor( QPalette::Disabled, QPalette::Text, Qt::darkGray );
	palette.setColor( QPalette::Disabled, QPalette::ButtonText, Qt::darkGray );
	app.setPalette( palette );
*/
	Link* controller_link = new RawWifi( device, 0, 1 );
// 	Link* controller_link = new Socket( "127.0.0.1", 2020 );
// 	Link* controller_link = new Socket( "192.168.32.1", 2020 );
	ControllerPC* controller = new ControllerPC( controller_link );

	Link* stream_link = new RawWifi( device, 10, 11 );
// 	Link* stream_link = new Socket( "127.0.0.1", 2021, Socket::UDPLite );
// 	Link* stream_link = new Socket( "192.168.32.1", 2021, Socket::UDPLite );

	MainWindow win( controller, stream_link );
	win.show();

	return app.exec();
}
