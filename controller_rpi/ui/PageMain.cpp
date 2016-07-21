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

#include <stdio.h>
#include <functional>
#include <gammaengine/Debug.h>
#include "PageMain.h"
#include "Globals.h"

PageMain::PageMain()
	: Page()
	, mLastT( 0.0f )
{
	int base_x = getGlobals()->icon( "PageMain" )->width() * 2.5;

	std::function<void()> fct_actionResetBattery = [this]() { this->actionResetBattery(); };
	mButtonResetBattery = new Button( getGlobals()->font(), L"Reset battery", fct_actionResetBattery );
	mButtonResetBattery->setGeometry( base_x, 32, mButtonResetBattery->width(), mButtonResetBattery->height() );

	std::function<void()> fct_actionCalibrate = [this]() { this->actionCalibrate(); };
	mButtonCalibrate = new Button( getGlobals()->font(), L"Calibrate", fct_actionCalibrate );
	mButtonCalibrate->setGeometry( base_x, 32 + getGlobals()->font()->size() * 2, mButtonCalibrate->width(), mButtonCalibrate->height() );

	std::function<void()> fct_actionCalibrateAll = [this]() { this->actionCalibrateAll(); };
	mButtonCalibrateAll = new Button( getGlobals()->font(), L"Calibrate all", fct_actionCalibrateAll );
	mButtonCalibrateAll->setGeometry( base_x, 32 + getGlobals()->font()->size() * 4, mButtonCalibrateAll->width(), mButtonCalibrateAll->height() );
/*
	std::function<void()> fct_actionCalibrateESCs = [this]() { this->actionCalibrateESCs(); };
	mButtonCalibrateESCs = new Button( getGlobals()->font(), L"Calibrate ESCs", fct_actionCalibrateESCs );
	mButtonCalibrateESCs->setGeometry( base_x, 16 + getGlobals()->font()->size() * 7, mButtonCalibrateESCs->width(), mButtonCalibrateESCs->height() );
*/
}


PageMain::~PageMain()
{
}


void PageMain::gotFocus()
{
	fDebug0();
	render();
}


void PageMain::lostFocus()
{
	fDebug0();
}


void PageMain::click( float _x, float _y, float force )
{
	int x = _x * getGlobals()->window()->width();
	int y = _y * getGlobals()->window()->height();

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
	}

	if ( x >= mButtonResetBattery->x() and y >= mButtonResetBattery->y() and x <= mButtonResetBattery->x() + mButtonResetBattery->width() and y <= mButtonResetBattery->y() + mButtonResetBattery->height() ) {
		mButtonResetBattery->Callback();
	}
	if ( x >= mButtonCalibrate->x() and y >= mButtonCalibrate->y() and x <= mButtonCalibrate->x() + mButtonCalibrate->width() and y <= mButtonCalibrate->y() + mButtonCalibrate->height() ) {
		mButtonCalibrate->Callback();
	}
	if ( x >= mButtonCalibrateAll->x() and y >= mButtonCalibrateAll->y() and x <= mButtonCalibrateAll->x() + mButtonCalibrateAll->width() and y <= mButtonCalibrateAll->y() + mButtonCalibrateAll->height() ) {
		mButtonCalibrateAll->Callback();
	}
/*
	if ( x >= mButtonCalibrateESCs->x() and y >= mButtonCalibrateESCs->y() and x <= mButtonCalibrateESCs->x() + mButtonCalibrateESCs->width() and y <= mButtonCalibrateESCs->y() + mButtonCalibrateESCs->height() ) {
		mButtonCalibrateESCs->Callback();
	}
*/
}


void PageMain::touch( float x, float y, float force )
{
}


bool PageMain::update( float t, float dt )
{
	if ( t - mLastT >= 1.0f / 3.0f ) {
		auto window = getGlobals()->window();
		auto renderer = getGlobals()->mainRenderer();
		GE::Font* font = getGlobals()->font();
		uint32_t clear_color = 0xFF303030;

		if ( getGlobals()->controller()->localBatteryVoltage() < 11.0f ) {
			clear_color = 0xFFFF3030;
		}
/*
		window->Clear( clear_color );
		getGlobals()->RenderDrawer();
		mButtonResetBattery->render();
		mButtonCalibrate->render();
		mButtonCalibrateAll->render();
	// 	mButtonCalibrateESCs->render();
*/
		std::string svbat = std::to_string( getGlobals()->controller()->localBatteryVoltage() );
		svbat = svbat.substr( 0, svbat.find( "." ) + 4 ) + "V";
		int w = 0;
		int h = 0;
		font->measureString( svbat, &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 1, w, h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 1, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( 0.01f * std::round( getGlobals()->controller()->batteryVoltage() * 100.0f ) );
		svbat = svbat.substr( 0, svbat.find( "." ) + 3 ) + "V";
		font->measureString( "12.60 V", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 2, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 2, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->controller()->totalCurrent() ) + "mAh";
		font->measureString( "1000 mAh", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 3, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 3, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->controller()->ping() ) + "ms";
		font->measureString( "1000 ms", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 4, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 4, font, 0xFFFFFFFF, svbat );
	
		svbat = std::to_string( getGlobals()->controller()->droneRxQuality() ) + "% TX";
		font->measureString( "100 % TX", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 5, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 5, font, 0xFFFFFFFF, svbat );
	
		svbat = std::to_string( getGlobals()->controller()->link()->RxQuality() ) + "% RX";
		font->measureString( "100 % RX", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 6, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 6, font, 0xFFFFFFFF, svbat );
	
		svbat = std::to_string( getGlobals()->stream()->link()->RxQuality() ) + "% RV";
		font->measureString( "100 % RV", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 7, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 7, font, 0xFFFFFFFF, svbat );
/*
		svbat = std::to_string( getGlobals()->stream()->linkQuality() ) + "%";
		font->measureString( "100 %", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 4 + font->size() * 4, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 4, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->stream()->linkLevel() ) + "dBm";
		font->measureString( "-100 dBm", &w, &h );
		window->ClearRegion( clear_color, window->width() - w * 1.05f, 5 + font->size() * 5, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 5, font, 0xFFFFFFFF, svbat );
*/
		mLastT = t;
	}
	return false;
}


void PageMain::render()
{

	auto window = getGlobals()->window();
	auto renderer = getGlobals()->mainRenderer();
	GE::Font* font = getGlobals()->font();

	window->Clear( 0xFF303030 );

	getGlobals()->RenderDrawer();
	mButtonResetBattery->render();
	mButtonCalibrate->render();
	mButtonCalibrateAll->render();
// 	mButtonCalibrateESCs->render();

	mLastT = 0.0f;
}


void PageMain::back()
{
}


void PageMain::actionResetBattery()
{
	fDebug0();
	getGlobals()->controller()->Lock();
	getGlobals()->controller()->ResetBattery();
	getGlobals()->controller()->Unlock();
}


void PageMain::actionCalibrate()
{
	fDebug0();
	getGlobals()->controller()->Lock();
	getGlobals()->controller()->Calibrate();
	getGlobals()->controller()->Unlock();
}


void PageMain::actionCalibrateAll()
{
	fDebug0();
	getGlobals()->controller()->Lock();
	getGlobals()->controller()->CalibrateAll();
	getGlobals()->controller()->Unlock();
}


void PageMain::actionCalibrateESCs()
{
	fDebug0();
/*
	getGlobals()->controller()->Lock();
	getGlobals()->controller()->CalibrateESCs();
	getGlobals()->controller()->Unlock();
*/
}
