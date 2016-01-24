#ifndef DECODEDIMAGE_H
#define DECODEDIMAGE_H

#include <gammaengine/Image.h>
#include <GLES2/gl2.h>

using namespace GE;

class DecodedImage : public Image
{
public:
	DecodedImage( Instance* instance, uint32_t width, uint32_t height, uint32_t backcolor );

protected:
	GLuint mTexture;
};

#endif // DECODEDIMAGE_H
