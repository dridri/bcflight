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
	mButtonResetBattery->setGeometry( base_x, 16, mButtonResetBattery->width(), mButtonResetBattery->height() );

	std::function<void()> fct_actionCalibrate = [this]() { this->actionCalibrate(); };
	mButtonCalibrate = new Button( getGlobals()->font(), L"Calibrate", fct_actionCalibrate );
	mButtonCalibrate->setGeometry( base_x, 16 + getGlobals()->font()->size() * 2, mButtonCalibrate->width(), mButtonCalibrate->height() );

	std::function<void()> fct_actionCalibrateAll = [this]() { this->actionCalibrateAll(); };
	mButtonCalibrateAll = new Button( getGlobals()->font(), L"Calibrate all", fct_actionCalibrateAll );
	mButtonCalibrateAll->setGeometry( base_x, 16 + getGlobals()->font()->size() * 4, mButtonCalibrateAll->width(), mButtonCalibrateAll->height() );
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
// 	fDebug0();
	render();
}


void PageMain::lostFocus()
{
// 	fDebug0();
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

		std::string svbat = std::to_string( getGlobals()->controller()->localBatteryVoltage() );
		svbat = svbat.substr( 0, svbat.find( "." ) + 4 ) + "V";
		int w = 0;
		int h = 0;
		font->measureString( svbat, &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4 + font->size() * 0, w, h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 0, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->controller()->batteryVoltage() );
		svbat = svbat.substr( 0, svbat.find( "." ) + 3 ) + "V";
		font->measureString( "12.60 V", &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4 + font->size() * 1, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 1, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->controller()->totalCurrent() ) + "mAh";
		font->measureString( "1000 mAh", &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4 + font->size() * 2, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 2, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->controller()->ping() ) + "ms";
		font->measureString( "1000 ms", &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4 + font->size() * 3, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 3, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->stream()->linkQuality() ) + "%";
		font->measureString( "100 %", &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4 + font->size() * 4, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 4, font, 0xFFFFFFFF, svbat );

		svbat = std::to_string( getGlobals()->stream()->linkLevel() ) + "dBm";
		font->measureString( "-100 dBm", &w, &h );
		window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 5 + font->size() * 5, w, h );
		font->measureString( svbat, &w, &h );
		renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 5, font, 0xFFFFFFFF, svbat );

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
/*
	std::string svbat = std::to_string( getGlobals()->controller()->localBatteryVoltage() );
	svbat = svbat.substr( 0, svbat.find( "." ) + 4 ) + "V";
	int w = 0;
	int h = 0;
	font->measureString( svbat, &w, &h );
	window->ClearRegion( 0xFF303030, window->width() - w * 1.5f, 4 + font->size() * 0, w * 1.5f, h );
	renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 0, font, 0xFFFFFFFF, svbat );

	svbat = std::to_string( getGlobals()->controller()->batteryVoltage() ) + "V";
	font->measureString( svbat, &w, &h );
	window->ClearRegion( 0xFF303030, window->width() - w * 1.5f, 4 + font->size() * 1, w * 1.5f, h );
	renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 1, font, 0xFFFFFFFF, svbat );

	svbat = std::to_string( getGlobals()->controller()->totalCurrent() ) + "mAh";
	font->measureString( svbat, &w, &h );
	window->ClearRegion( 0xFF303030, window->width() - w * 1.5f, 4 + font->size() * 2, w * 1.5f, h );
	renderer->DrawText( window->width() - w * 1.05f, 4 + font->size() * 2, font, 0xFFFFFFFF, svbat );
*/
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
