#ifndef PAGEMAIN_H
#define PAGEMAIN_H

#include <libbcui/Page.h>
#include <libbcui/Button.h>

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

private:
	void actionResetBattery();
	void actionCalibrate();
	void actionCalibrateAll();
	void actionCalibrateESCs();

	Button* mButtonResetBattery;
	Button* mButtonCalibrate;
	Button* mButtonCalibrateAll;
	Button* mButtonCalibrateESCs;

	float mLastT;
};

#endif // PAGEMAIN_H
