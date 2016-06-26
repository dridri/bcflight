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
#include <gammaengine/Font.h>
#include "PageCalibrate.h"
#include "Globals.h"
#include "../ControllerPi.h"


PageCalibrate::PageCalibrate()
{
	mCurrentAxis = 0;
	mAxies[0].name = "Thrust";
	mAxies[1].name = "Yaw";
	mAxies[2].name = "Pitch";
	mAxies[3].name = "Roll";
// 	mAxies[0].max = mAxies[1].max = mAxies[2].max = mAxies[3].max = 0;
// 	mAxies[0].center = mAxies[1].center = mAxies[2].center = mAxies[3].center = 2000;
// 	mAxies[0].min = mAxies[1].min = mAxies[2].min = mAxies[3].min = 65535;
}


PageCalibrate::~PageCalibrate()
{
}


void PageCalibrate::gotFocus()
{
// 	fDebug0();
	getGlobals()->controller()->Lock();

	for ( uint32_t i = 0; i < sizeof(mAxies) / sizeof(PageCalibrate::Axis); i++ ) {
		mAxies[i].min = getGlobals()->controller()->joystick(i)->min();
		mAxies[i].center = getGlobals()->controller()->joystick(i)->center();
		mAxies[i].max = getGlobals()->controller()->joystick(i)->max();
	}

	mCurrentAxis = 0;
	mApplyTimer = Timer();
	render();
}


void PageCalibrate::lostFocus()
{
	getGlobals()->controller()->Unlock();
}


void PageCalibrate::click( float _x, float _y, float force )
{
	int x = _x * getGlobals()->window()->width();
	int y = _y * getGlobals()->window()->height();
// 	fDebug( _x, _y, x, y );
	auto window = getGlobals()->window();
	GE::Font* font = getGlobals()->font();
	int icon_width_base = getGlobals()->icon( "selector" )->width();
	int icon_width = icon_width_base * 1.1;
	int icon_height = getGlobals()->icon( "selector" )->height();

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
	}

	if (x >= icon_width * 1.75 and y >= 8 and x <= icon_width * 1.75 + icon_width_base and y <= 8 + icon_height ) {
		if ( mCurrentAxis > 0 ) {
			mApplyTimer.Stop();
			mCurrentAxis--;
			render();
		}
	} else if (x >= icon_width * 1.75 * 2 and y >= 8 and x <= icon_width * 1.75 * 2 + icon_width_base and y <= 8 + icon_height ) {
		if ( mCurrentAxis < 3 ) {
			mApplyTimer.Stop();
			mCurrentAxis++;
			render();
		}
	}

	int apply_w = 0;
	int apply_h = 0;
	font->measureString( "Apply", &apply_w, &apply_h );
	int apply_x = getGlobals()->icon( "PageMain" )->width() * 2.5;
	int apply_y = window->height() - apply_h * 1.25;
	if ( x >= apply_x and y >= apply_y and x <= apply_x + apply_w ) {
		gDebug() << "Apply\n";
		getGlobals()->controller()->joystick(mCurrentAxis)->SetCalibratedValues( mAxies[mCurrentAxis].min, mAxies[mCurrentAxis].center, mAxies[mCurrentAxis].max );
		getGlobals()->setSetting( "Joystick:" + std::to_string( mCurrentAxis ) + ":min", mAxies[mCurrentAxis].min );
		getGlobals()->setSetting( "Joystick:" + std::to_string( mCurrentAxis ) + ":cen", mAxies[mCurrentAxis].center );
		getGlobals()->setSetting( "Joystick:" + std::to_string( mCurrentAxis ) + ":max", mAxies[mCurrentAxis].max );
		system( "mount -o remount,rw /" );
		getGlobals()->SaveSettings( "/root/settings.txt" );
		system( "mount -o remount,ro /" );
		mApplyTimer.Stop();
		mApplyTimer.Start();
	}

	int reset_w = 0;
	int reset_h = 0;
	font->measureString( "Reset", &reset_w, &reset_h );
	int reset_x = getGlobals()->icon( "PageMain" )->width() * 2.5;
	int reset_y = window->height() - reset_h * 3.0;
	if ( x >= reset_x and y >= reset_y and x <= reset_x + reset_w and y <= reset_y + reset_h ) {
		gDebug() << "Reset\n";
		mAxies[mCurrentAxis].max = 0;
		mAxies[mCurrentAxis].center = 2000;
		mAxies[mCurrentAxis].min = 65535;
	}
}


void PageCalibrate::touch( float x, float y, float force )
{
}


bool PageCalibrate::update( float t, float dt )
{
	auto window = getGlobals()->window();
	auto renderer = getGlobals()->mainRenderer();
	GE::Font* font = getGlobals()->font();
	auto controller = getGlobals()->controller();
	int tw = 0;
	int th = 0;

	uint16_t value = controller->joystick( mCurrentAxis )->LastRaw();
	mAxies[ mCurrentAxis ].max = std::min( (uint16_t)4096, std::max( mAxies[ mCurrentAxis ].max, value ) );
	mAxies[ mCurrentAxis ].min = std::max( (uint16_t)0, std::min( mAxies[ mCurrentAxis ].min, value ) );
	mAxies[ mCurrentAxis ].center = std::min( mAxies[ mCurrentAxis ].max, std::max( mAxies[ mCurrentAxis ].min, value ) );

	font->measureString( "max : ", &tw, &th );
	th = font->size() * 1.25;
	window->ClearRegion( 0xFF403030, window->width() / 2 + tw, window->height() / 2 + th * -1.5, window->width() / 2 - tw, th * 3 );
	renderer->DrawText( window->width() / 2 + tw, window->height() / 2 + th * ( -1.5 + 0 ), font, 0xFFFFFFFF, std::to_string( mAxies[ mCurrentAxis ].max ) );
	renderer->DrawText( window->width() / 2 + tw, window->height() / 2 + th * ( -1.5 + 1 ), font, 0xFFFFFFFF, std::to_string( mAxies[ mCurrentAxis ].center ) );
	renderer->DrawText( window->width() / 2 + tw, window->height() / 2 + th * ( -1.5 + 2 ), font, 0xFFFFFFFF, std::to_string( mAxies[ mCurrentAxis ].min ) );

	if ( mApplyTimer.isRunning() ) {
		int apply_w = 0;
		int apply_h = 0;
		font->measureString( "Apply", &apply_w, &apply_h );
		int apply_x = getGlobals()->icon( "PageMain" )->width() * 2.5;
		int apply_y = window->height() - apply_h * 1.25;
		if ( mApplyTimer.ellapsed() < 1000 ) {
			window->ClearRegion( apply_x, apply_y, apply_w, apply_h );
			renderer->DrawText( apply_x, apply_y, font, 0xFF00FF00, "Apply" );
		} else if ( mApplyTimer.ellapsed() < 1100 ) {
			window->ClearRegion( apply_x, apply_y, apply_w, apply_h );
			renderer->DrawText( apply_x, apply_y, font, 0xFFFFFFFF, "Apply" );
		}
	}

	return false;
}


void PageCalibrate::render()
{
	auto window = getGlobals()->window();
	auto renderer = getGlobals()->mainRenderer();
	GE::Font* font = getGlobals()->font();
	int tw = 0;
	int th = 0;

	window->Clear( 0xFF403030 );

	int icon_width = getGlobals()->icon( "PageMain" )->width() * 1.1;
	getGlobals()->RenderDrawer();
	renderer->Draw( getGlobals()->icon( "PageMain" )->width() * 1.75, 8, getGlobals()->icon( "left" ) );
	renderer->Draw( getGlobals()->icon( "PageMain" )->width() * 1.75 * 2, 8, getGlobals()->icon( "right" ) );

	font->measureString( mAxies[ mCurrentAxis ].name, &tw, &th );
// 	renderer->DrawText( window->width() - tw * 1.1, 0, font, 0xFFFFFFFF, mAxies[ mCurrentAxis ].name );
	renderer->DrawText( window->width() / 2 + icon_width, 0, font, 0xFFFFFFFF, mAxies[ mCurrentAxis ].name, Renderer2D::HCenter );

	th = font->size() * 1.25;
	renderer->DrawText( window->width() / 2, window->height() / 2 + th * ( -1.5 + 0 ), font, 0xFFFFFFFF, "max : " );
	renderer->DrawText( window->width() / 2, window->height() / 2 + th * ( -1.5 + 1 ), font, 0xFFFFFFFF, "cen : " );
	renderer->DrawText( window->width() / 2, window->height() / 2 + th * ( -1.5 + 2 ), font, 0xFFFFFFFF, "min : " );

	int apply_w = 0;
	int apply_h = 0;
	font->measureString( "Apply", &apply_w, &apply_h );
	int apply_x = getGlobals()->icon( "PageMain" )->width() * 2.5;
	int apply_y = window->height() - apply_h * 1.25;
	renderer->DrawText( apply_x, apply_y, font, 0xFFFFFFFF, "Apply" );

	int reset_w = 0;
	int reset_h = 0;
	font->measureString( "Reset", &reset_w, &reset_h );
	int reset_x = getGlobals()->icon( "PageMain" )->width() * 2.5;
	int reset_y = window->height() - reset_h * 3;
	renderer->DrawText( reset_x, reset_y, font, 0xFFFFFFFF, "Reset" );
}
