#include <gammaengine/Debug.h>
#include <gammaengine/Font.h>
#include "PageCalibrate.h"
#include "Globals.h"
#include "../Controller.h"


PageCalibrate::PageCalibrate()
{
	mCurrentAxis = 0;
	mAxies[0].name = "Thrust";
	mAxies[1].name = "Yaw";
	mAxies[2].name = "Pitch";
	mAxies[3].name = "Roll";
	mAxies[0].max = mAxies[1].max = mAxies[2].max = mAxies[3].max = 0;
	mAxies[0].center = mAxies[1].center = mAxies[2].center = mAxies[3].center = 2768;
	mAxies[0].min = mAxies[1].min = mAxies[2].min = mAxies[3].min = 65535;
}


PageCalibrate::~PageCalibrate()
{
}


void PageCalibrate::gotFocus()
{
// 	fDebug0();
	mCurrentAxis = 0;
	mApplyTimer = Timer();
	render();
}


void PageCalibrate::lostFocus()
{
// 	fDebug0();
	/*
	getGlobals()->controller()->joystick(0)->SetCalibratedValues( mAxies[0].min, mAxies[0].center, mAxies[0].max );
	getGlobals()->controller()->joystick(1)->SetCalibratedValues( mAxies[1].min, mAxies[1].center, mAxies[1].max );
	getGlobals()->controller()->joystick(2)->SetCalibratedValues( mAxies[2].min, mAxies[2].center, mAxies[2].max );
	getGlobals()->controller()->joystick(3)->SetCalibratedValues( mAxies[3].min, mAxies[3].center, mAxies[3].max );
	*/
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

	if ( x >= 8 and x <= 8 + getGlobals()->icon( "selector" )->width() ) {
		if ( y >= 8 + icon_height * 0 and y <= 8 + icon_height * 1.5 ) {
			getGlobals()->setCurrentPage( "PageMain" );
		} else if ( y >= 8 + icon_height * 1.5 * 1 and y <= 8 + icon_height * 1.5 * 2 ) {
			getGlobals()->setCurrentPage( "PageCalibrate" );
		} else if ( y >= 8 + icon_height * 1.5 * 2 and y <= 8 + icon_height * 1.5 * 3 ) {
			getGlobals()->setCurrentPage( "PageSettings" );
		}
	} else if (x >= icon_width * 1.75 and y >= 8 and x <= icon_width * 1.75 + icon_width_base and y <= 8 + icon_height ) {
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
	int apply_x = getGlobals()->icon( "selector" )->width() * 2.5;
	int apply_y = window->height() - apply_h * 1.25;
	if ( x >= apply_x and y >= apply_y and x <= apply_x + apply_w ) {
		getGlobals()->controller()->joystick(mCurrentAxis)->SetCalibratedValues( mAxies[mCurrentAxis].min, mAxies[mCurrentAxis].center, mAxies[mCurrentAxis].max );
		mApplyTimer.Stop();
		mApplyTimer.Start();
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

	uint16_t value = controller->joystick( mCurrentAxis )->ReadRaw();
	mAxies[ mCurrentAxis ].max = std::max( mAxies[ mCurrentAxis ].max, value );
	mAxies[ mCurrentAxis ].center = value;
	mAxies[ mCurrentAxis ].min = std::min( mAxies[ mCurrentAxis ].min, value );

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
		int apply_x = getGlobals()->icon( "selector" )->width() * 2.5;
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

	int icon_width = getGlobals()->icon( "selector" )->width() * 1.1;
	int icon_height = getGlobals()->icon( "selector" )->height() * 1.5;
	renderer->DrawLine( 16 + icon_width, 0, 0xFFFFFFFF, 16 + icon_width, window->height(), 0xFFFFFFFF );
	renderer->Draw( 8, 8 + icon_height * 0, getGlobals()->icon( "home" ) );
	renderer->Draw( 8, 8 + icon_height * 1, getGlobals()->icon( "calibrate" ) );
	renderer->Draw( 8, 8 + icon_height * 1, getGlobals()->icon( "selector" ) );
	renderer->Draw( 8, 8 + icon_height * 2, getGlobals()->icon( "settings" ) );
	renderer->Draw( getGlobals()->icon( "selector" )->width() * 1.75, 8, getGlobals()->icon( "left" ) );
	renderer->Draw( getGlobals()->icon( "selector" )->width() * 1.75 * 2, 8, getGlobals()->icon( "right" ) );

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
	int apply_x = getGlobals()->icon( "selector" )->width() * 2.5;
	int apply_y = window->height() - apply_h * 1.25;
	renderer->DrawText( apply_x, apply_y, font, 0xFFFFFFFF, "Apply" );
}
