#include <gammaengine/Debug.h>
#include "PageMain.h"

PageMain::PageMain()
{
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
// 	fDebug( _x, _y, x, y );

	int icon_height = getGlobals()->icon( "home" )->height() * 1.5;
	if ( x >= 8 and x <= 16 + getGlobals()->icon( "selector" )->width() ) {
		if ( y >= 8 + icon_height * 0 and y <= 8 + icon_height * 1 ) {
			getGlobals()->setCurrentPage( "PageMain" );
		} else if ( y >= 8 + icon_height * 1 and y <= 8 + icon_height * 2 ) {
			getGlobals()->setCurrentPage( "PageCalibrate" );
		} else if ( y >= 8 + icon_height * 2 and y <= 8 + icon_height * 3 ) {
			getGlobals()->setCurrentPage( "PageSettings" );
		}
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

	int icon_width = getGlobals()->icon( "selector" )->width() * 1.1;
	int icon_height = getGlobals()->icon( "selector" )->height() * 1.5;
	renderer->DrawLine( 16 + icon_width, 0, 0xFFFFFFFF, 16 + icon_width, window->height(), 0xFFFFFFFF );
	renderer->Draw( 8, 8 + icon_height * 0, getGlobals()->icon( "home" ) );
	renderer->Draw( 8, 8 + icon_height * 0, getGlobals()->icon( "selector" ) );
	renderer->Draw( 8, 8 + icon_height * 1, getGlobals()->icon( "calibrate" ) );
	renderer->Draw( 8, 8 + icon_height * 2, getGlobals()->icon( "settings" ) );
}


void PageMain::back()
{
}
