#ifndef RENDERERHUD_H
#define RENDERERHUD_H

#include <gammaengine/Instance.h>
#include <gammaengine/Font.h>
#include <gammaengine/Image.h>
#include <gammaengine/Matrix.h>
#include <string>

using namespace GE;

class RendererHUD
{
public:
	RendererHUD( Instance* instance, Font* font );
	~RendererHUD();

	void Compute();
	void RenderThrust( float thrust );
	void RenderLink( float quality );
	void RenderBattery( float level );
	void RenderText( int x, int y, const std::string& text, uint32_t color, bool hcenter = false );
	void RenderText( int x, int y, const std::string& text, const Vector4f& color, bool hcenter = false );

protected:
	typedef struct {
		float u, v;
		float x, y;
	} FastVertex;

	typedef struct {
		float u, v;
		uint32_t color;
		float x, y;
	} FastVertexColor;

	typedef struct {
		uint32_t mShader;
		uint32_t mVertexShader;
		uint32_t mFragmentShader;
		uint32_t mVertexTexcoordID;
		uint32_t mVertexColorID;
		uint32_t mVertexPositionID;
		uint32_t mMatrixProjectionID;
		uint32_t mOffsetID;
		uint32_t mColorID;
	} Shader;

	int LoadVertexShader( RendererHUD::Shader* target, const void* data, size_t size );
	int LoadFragmentShader( RendererHUD::Shader* target, const void* data, size_t size );
	uint8_t* loadShader( const std::string& filename, size_t* sz = 0 );
	void createPipeline( Shader* target );

	Instance* mInstance;
	Font* mFont;
	bool mReady;

	Matrix* mMatrixProjection;

	Shader mShader;
	Shader mColorShader;
	Shader mTextShader;

	uint32_t mThrustVBO;
	uint32_t mLinkVBO;
	uint32_t mBatteryVBO;
	uint32_t mTextVBO;
	uint32_t mTextIBO;

	uint32_t mTextIndices[256];
	int mTextAdv[256];
};

#endif // RENDERERHUD_H
