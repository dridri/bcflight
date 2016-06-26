/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <gammaengine/FramebufferWindow.h>
#include <gammaengine/Image.h>
#include <libbcui/Globals.h>
#include "../ControllerPi.h"
#include "../Stream.h"

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
	Stream* stream() const { return mStream; }
	void setStream( Stream* s) { mStream = s; }
	ControllerPi* controller() const { return mController; }
	void setController( ControllerPi* c ) { mController = c; }
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
	Stream* mStream;
	ControllerPi* mController;
	float mUpdateLastT;
};

static inline ::Globals* getGlobals() {
	return ::Globals::instance();
}

#endif // GLOBALS_H
