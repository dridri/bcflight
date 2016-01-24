#ifndef PAGECALIBRATE_H
#define PAGECALIBRATE_H

#include <libbcui/Page.h>
#include <gammaengine/Timer.h>

class PageCalibrate : public BC::Page
{
public:
	PageCalibrate();
	~PageCalibrate();
	virtual void gotFocus();
	virtual void lostFocus();
	virtual void click( float x, float y, float force );
	virtual void touch( float x, float y, float force );
	virtual bool update( float t, float dt );
	virtual void render();

private:
	typedef struct Axis {
		std::string name;
		uint16_t min;
		uint16_t center;
		uint16_t max;
	} Axis;
	Axis mAxies[4];
	int mCurrentAxis;
	Timer mApplyTimer;
};

#endif // PAGECALIBRATE_H
