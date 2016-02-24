#include <functional>
#include <gammaengine/Debug.h>
#include "PageMain.h"
#include "Globals.h"

PageMain::PageMain()
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
}


void PageMain::touch( float x, float y, float force )
{
}


bool PageMain::update( float t, float dt )
{
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

	std::string svbat = std::to_string( getGlobals()->controller()->localBatteryVoltage() );
	svbat = svbat.substr( 0, svbat.find( "." ) + 4 ) + "V";
	int w = 0;
	int h = 0;
	font->measureString( svbat, &w, &h );
	window->ClearRegion( 0xFF303030, window->width() - w * 1.05f, 4, w, h );
	renderer->DrawText( window->width() - w * 1.05f, 4, font, 0xFFFFFFFF, svbat );
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
