#ifndef PAGEMAIN_H
#define PAGEMAIN_H

#include <libbcui/Page.h>
#include "Globals.h"

using namespace BC;

class PageMain : public Page
{
public:
	PageMain();
	~PageMain();

	void gotFocus();
	void lostFocus();
	void click( float x, float y, float force = 1.0f );
	void touch( float x, float y, float force = 1.0f );
	bool update( float t, float dt );
	void render();
	void back();
};

#endif // PAGEMAIN_H
