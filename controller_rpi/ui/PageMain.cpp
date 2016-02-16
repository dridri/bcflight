#include <gammaengine/Debug.h>
#include "PageMain.h"
#include "Globals.h"

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

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
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
}


void PageMain::back()
{
}
