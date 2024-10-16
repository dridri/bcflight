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

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>
#include <QtCore/QLoggingCategory>
#include <QDebug>

#include <stdio.h>
#include <RawWifi.h>
#include <Socket.h>
#include <Debug.h>
#include "ControllerPC.h"

#include "MainWindow.h"

void msgHandler( QtMsgType type, const QMessageLogContext& ctx, const QString& msg )
{
	printf( "%s\n", msg.toStdString().c_str() );
	fflush( stdout );
}

int main( int ac, char** av )
{
#ifdef WIN32
	WSADATA WSAData;
	WSAStartup( MAKEWORD(2,0), &WSAData );
#endif

	Debug::setDebugLevel( Debug::Verbose );

	QApplication app( ac, av );
	qInstallMessageHandler( msgHandler );
	QLoggingCategory::defaultCategory()->setEnabled( QtDebugMsg, true );

	MainWindow win;
	win.show();

	return app.exec();
}
