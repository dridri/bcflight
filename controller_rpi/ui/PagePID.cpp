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

#include <cmath>
#include <gammaengine/Debug.h>
#include <gammaengine/Font.h>
#include "PagePID.h"
#include "Globals.h"
#include "../ControllerPi.h"


PagePID::PagePID()
	: Page()
{
}

PagePID::~PagePID()
{
}


void PagePID::gotFocus()
{
	getGlobals()->controller()->ReloadPIDs();
}


void PagePID::lostFocus()
{
}


void PagePID::click( float _x, float _y, float force )
{
	auto window = getGlobals()->window();
	GE::Font* font = getGlobals()->font();
	int x = _x * window->width();
	int y = _y * window->height();

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
	}

	int tw = 0, th = 0;
	char p[32];
	sprintf( p, "-%+.6f+", getGlobals()->controller()->pid().x );
	font->measureString( p, &tw, &th );

	int bx = getGlobals()->icon( "PageMain" )->width() * 1.1 + 8;
	int obx = getGlobals()->icon( "PageMain" )->width() * 1.1 + 8 + tw + 16;
	int by = window->height() / 2 - font->size() * 3;
	int w = tw / 2;
	int h = font->size();

	bool changed = false;
	vec3 pid = getGlobals()->controller()->pid();
	vec3 outerPid = getGlobals()->controller()->outerPid();

	if ( x >= bx and y >= by + h * 0 and x <= bx + w and y <= by + h * 1 ) {
		pid.x -= 0.00001f;
		changed = true;
	} else if ( x >= bx + w and y >= by + h * 0 and x <= bx + tw and y <= by + h * 1 ) {
		pid.x += 0.00001f;
		changed = true;
	} else if ( x >= bx and y >= by + h * 2 and x <= bx + w and y <= by + h * 3 ) {
		pid.y -= 0.0001f;
		changed = true;
	} else if ( x >= bx + w and y >= by + h * 2 and x <= bx + tw and y <= by + h * 3 ) {
		pid.y += 0.0001f;
		changed = true;
	} else if ( x >= bx and y >= by + h * 4 and x <= bx + w and y <= by + h * 5 ) {
		pid.z -= 0.000001f;
		changed = true;
	} else if ( x >= bx + w and y >= by + h * 4 and x <= bx + tw and y <= by + h * 5 ) {
		pid.z += 0.000001f;
		changed = true;
	} else if ( x >= obx and y >= by + h * 0 and x <= obx + w and y <= by + h * 1 ) {
		outerPid.x -= 0.025f;
		changed = true;
	} else if ( x >= obx + w and y >= by + h * 0 and x <= obx + tw and y <= by + h * 1 ) {
		outerPid.x += 0.025f;
		changed = true;
	} else if ( x >= obx and y >= by + h * 2 and x <= obx + w and y <= by + h * 3 ) {
		outerPid.y -= 0.025f;
		changed = true;
	} else if ( x >= obx + w and y >= by + h * 2 and x <= obx + tw and y <= by + h * 3 ) {
		outerPid.y += 0.025f;
		changed = true;
	} else if ( x >= obx and y >= by + h * 4 and x <= obx + w and y <= by + h * 5 ) {
		outerPid.z -= 0.000001f;
		changed = true;
	} else if ( x >= obx + w and y >= by + h * 4 and x <= obx + tw and y <= by + h * 5 ) {
		outerPid.z += 0.000001f;
		changed = true;
	}

	if ( std::isnan( pid.x ) ) {
		pid.x = 0.0f;
		changed = true;
	}
	if ( std::isnan( pid.y ) ) {
		pid.y = 0.0f;
		changed = true;
	}
	if ( std::isnan( pid.z ) ) {
		pid.z = 0.0f;
		changed = true;
	}
	if ( std::isnan( outerPid.x ) ) {
		outerPid.x = 0.0f;
		changed = true;
	}
	if ( std::isnan( outerPid.y ) ) {
		outerPid.y = 0.0f;
		changed = true;
	}
	if ( std::isnan( outerPid.z ) ) {
		outerPid.z = 0.0f;
		changed = true;
	}

	if ( changed ) {
		getGlobals()->controller()->setPID( pid );
		getGlobals()->controller()->setOuterPID( outerPid );
	}
}


void PagePID::touch( float x, float y, float force )
{
}


bool PagePID::update( float t, float dt )
{
	auto window = getGlobals()->window();
	auto renderer = getGlobals()->mainRenderer();
	GE::Font* font = getGlobals()->font();
	int drawer_width = getGlobals()->icon( "PageMain" )->width() * 1.1;

	int tw = 0, th = 0;
	char p[32], i[32], d[32];
	char op[32], oi[32], od[32];
	sprintf( p, "-%+.6f+", getGlobals()->controller()->pid().x );
	sprintf( i, "-%+.6f+", getGlobals()->controller()->pid().y );
	sprintf( d, "-%+.6f+", getGlobals()->controller()->pid().z );
	sprintf( op, "-%+.6f+", getGlobals()->controller()->outerPid().x );
	sprintf( oi, "-%+.6f+", getGlobals()->controller()->outerPid().y );
	sprintf( od, "-%+.6f+", getGlobals()->controller()->outerPid().z );
	font->measureString( p, &tw, &th );

	window->ClearRegion( 0xFF403030, drawer_width + 8, window->height() / 2 - font->size() * 3, tw * 2 + 16, th * 5 );

	renderer->DrawText( drawer_width + 8, window->height() / 2 - font->size() * 3 + font->size() * 0, font, 0xFFFFFFFF, p );
	renderer->DrawText( drawer_width + 8, window->height() / 2 - font->size() * 3 + font->size() * 2, font, 0xFFFFFFFF, i );
	renderer->DrawText( drawer_width + 8, window->height() / 2 - font->size() * 3 + font->size() * 4, font, 0xFFFFFFFF, d );

	renderer->DrawText( drawer_width + 8 + tw + 16, window->height() / 2 - font->size() * 3 + font->size() * 0, font, 0xFFFFFFFF, op );
	renderer->DrawText( drawer_width + 8 + tw + 16, window->height() / 2 - font->size() * 3 + font->size() * 2, font, 0xFFFFFFFF, oi );
	renderer->DrawText( drawer_width + 8 + tw + 16, window->height() / 2 - font->size() * 3 + font->size() * 4, font, 0xFFFFFFFF, od );

	return false;
}


void PagePID::render()
{
	auto window = getGlobals()->window();

	window->Clear( 0xFF403030 );
	getGlobals()->RenderDrawer();
}
