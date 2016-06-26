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

#ifndef PAGESETTINGS_H
#define PAGESETTINGS_H

#include <libbcui/ListMenu.h>

class PageSettings : public BC::ListMenu
{
public:
	PageSettings();
	~PageSettings();

	void gotFocus();
	void click( float x, float y, float force );
	void touch( float x, float y, float force );
	bool update( float t, float dt );
	void background();
};

#endif // PAGESETTINGS_H
