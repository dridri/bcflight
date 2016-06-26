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

#ifndef PAGEMAIN_H
#define PAGEMAIN_H

#include <libbcui/Page.h>
#include <libbcui/Button.h>

using namespace BC;

class PageMain : public Page
{
public:
	PageMain();
	~PageMain();

	void gotFocus();
	void lostFocus();
	void click( float x, float y, float force = 1.0f );
	void touch( float x, float y, float force = 1.0f );
	bool update( float t, float dt );
	void render();
	void back();

private:
	void actionResetBattery();
	void actionCalibrate();
	void actionCalibrateAll();
	void actionCalibrateESCs();

	Button* mButtonResetBattery;
	Button* mButtonCalibrate;
	Button* mButtonCalibrateAll;
	Button* mButtonCalibrateESCs;

	float mLastT;
};

#endif // PAGEMAIN_H
