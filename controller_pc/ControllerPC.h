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
	ControllerPC( Link* link );
	~ControllerPC();

	void setControlThrust( const float v );
	void setArmed( const bool armed );

protected:
	float ReadThrust();
	float ReadRoll();
	float ReadPitch();
	float ReadYaw();
	int8_t ReadSwitch( uint32_t id );

private:
	float mThrust;
	bool mArmed;
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
