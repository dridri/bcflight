#ifndef PAGESETTINGS_H
#define PAGESETTINGS_H

#include <libbcui/ListMenu.h>

class PageSettings : public BC::ListMenu
{
public:
	PageSettings();
	~PageSettings();

	void gotFocus();
	void click( float x, float y, float force );
	void touch( float x, float y, float force );
	bool update( float t, float dt );
	void background();
};

#endif // PAGESETTINGS_H
