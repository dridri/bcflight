#ifndef GLOBALS_H
#define GLOBALS_H

#include <gammaengine/FramebufferWindow.h>
#include <gammaengine/Image.h>
#include <libbcui/Globals.h>
#include "../Controller.h"

class Globals : public BC::Globals
{
public:
	Globals( Instance* instance, Font* fnt );
	~Globals();
	void Run();

	void RenderDrawer();
	bool PageSwitcher( int x, int y );

	ProxyWindow< FramebufferWindow >* window() const { return mWindow; }
	Font* font() const { return mFont; }
	Image* icon( const std::string& name ) { return mIcons[ name ]; }
	Controller* controller() const { return mController; }
	void setController( Controller* c) { mController = c; }
	static Globals* instance() { return sInstance; }

private:
	bool update_( float t, float dt );
	static Globals* sInstance;
	ProxyWindow< FramebufferWindow >* mWindow;
	int mInputFD;
	Vector2i mCursor;
	bool mClicking;
	uint32_t mCursorCounter;
	Font* mFont;
	std::map< std::string, Image* > mIcons;
	Controller* mController;
};

static inline ::Globals* getGlobals() {
	return ::Globals::instance();
}

#endif // GLOBALS_H
