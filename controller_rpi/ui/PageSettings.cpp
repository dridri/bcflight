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
	auto renderer = getGlobals()->mainRenderer();
	GE::Font* font = getGlobals()->font();

	window->Clear( 0xFF303030 );

	getGlobals()->RenderDrawer();
}
