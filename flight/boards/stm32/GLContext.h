#ifndef GLCONTEXT_H
#define GLCONTEXT_H

#include <stdint.h>

class GLContext
{
public:
	GLContext();
	~GLContext();

	int32_t Initialize( uint32_t width, uint32_t height );
	void SwapBuffers();

	uint32_t glWidth();
	uint32_t glHeight();
	static uint32_t displayWidth();
	static uint32_t displayHeight();
	static uint32_t displayFrameRate();

protected:
};

#endif // GLCONTEXT_H
