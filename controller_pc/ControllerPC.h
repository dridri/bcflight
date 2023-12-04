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

#ifndef CONTROLLERPC_H
#define CONTROLLERPC_H

#include <QtCore/QThread>
#include <Controller.h>


class ControllerPC : public Controller
{
public:
	ControllerPC( Link* link, bool spectate = false );
	~ControllerPC();

	void setControlThrust( const float v );
	void setArmed( const bool armed );
	void setModeSwitch( const Controller::Mode& mode );
	void setNight( const bool night );
	void setRecording( const bool record );

protected:
	float ReadThrust( float dt );
	float ReadRoll( float dt );
	float ReadPitch( float dt );
	float ReadYaw( float dt );
	int8_t ReadSwitch( uint32_t id );


private:

#ifdef WIN32
	void initGamePad();
	int gamepadid = -1;
	JOYINFOEX padinfo;
#endif
	float mThrust;
	bool mArmed;
	bool mMode;
	bool mNightMode;
	bool mRecording;

};


class ControllerMonitor : public QThread
{
	Q_OBJECT
public:
	ControllerMonitor( ControllerPC* controller );
	~ControllerMonitor();

signals:
	void connected();

protected:
	virtual void run();

private:
	ControllerPC* mController;
	bool mKnowConnected;

};

#endif // CONTROLLERPC_H
