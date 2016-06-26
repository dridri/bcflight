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

#ifndef PAGECALIBRATE_H
#define PAGECALIBRATE_H

#include <libbcui/Page.h>
#include <gammaengine/Timer.h>

class PageCalibrate : public BC::Page
{
public:
	PageCalibrate();
	~PageCalibrate();
	virtual void gotFocus();
	virtual void lostFocus();
	virtual void click( float x, float y, float force );
	virtual void touch( float x, float y, float force );
	virtual bool update( float t, float dt );
	virtual void render();

private:
	typedef struct Axis {
		std::string name;
		uint16_t min;
		uint16_t center;
		uint16_t max;
	} Axis;
	Axis mAxies[4];
	int mCurrentAxis;
	Timer mApplyTimer;
};

#endif // PAGECALIBRATE_H
