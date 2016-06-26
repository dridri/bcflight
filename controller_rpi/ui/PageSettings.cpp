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

#include <gammaengine/Debug.h>
#include "PageSettings.h"
#include "Globals.h"

PageSettings::PageSettings() : BC::ListMenu()
{
}


PageSettings::~PageSettings()
{
}


void PageSettings::gotFocus()
{
	BC::ListMenu::gotFocus();
	render();
}


void PageSettings::click( float _x, float _y, float force )
{
	int x = _x * getGlobals()->window()->width();
	int y = _y * getGlobals()->window()->height();

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
	}

	BC::ListMenu::click( _x, _y, force );
}


void PageSettings::touch( float x, float y, float force )
{
	BC::ListMenu::touch( x, y, force );
}


bool PageSettings::update( float t, float dt )
{
	return BC::ListMenu::update( t, dt );
}


void PageSettings::background()
{
	auto window = getGlobals()->window();
// 	auto renderer = getGlobals()->mainRenderer();
// 	GE::Font* font = getGlobals()->font();

	window->Clear( 0xFF303030 );

	getGlobals()->RenderDrawer();
}
