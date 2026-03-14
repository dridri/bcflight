#pragma once

#include <string>
#include <cstdint>
#include <vector>

class Font
{
public:
	// Generate atlas from a TTF file at the given pixel size.
	// Cell size = next power-of-2 >= pixelSize*1.3, atlas = 16*cellSize square.
	Font( const std::string& ttfPath, uint32_t pixelSize );

	// Build a Font from an already-decoded RGBA pixel buffer laid out as a 16x16 grid of cellW x cellH cells.
	// atlasW/H: total atlas dimensions. targetSize: desired render size (advances scaled from cellW).
	// Ownership of pixels is NOT taken — data is copied internally.
	static Font* fromRawPixels( const uint32_t* pixels, uint32_t atlasW, uint32_t atlasH,
	                            uint32_t cellW, uint32_t cellH, uint32_t targetSize );

	~Font();

	// Upload atlas to GL and build character VBO. Must be called on the GL thread.
	// spaceAdvance: override advance for space character (0 = auto-computed)
	void uploadGL( uint32_t spaceAdvance = 0 );

	// GL handles (valid after uploadGL)
	uint32_t glTextureID() const { return mGLTextureID; }
	uint32_t glVBO()       const { return mGLVBO; }

	// Per-character advance (pixels), public for direct access
	int adv[256];

	// Measure rendered dimensions of a string at the given scale.
	// width: sum of advances except last glyph uses actual glyphW.
	// height: max glyph height across all characters in the string.
	// Supports multi-line text (\n).
	// width: rendered width. height: visual height (from top of first pixel to bottom of last).
	// yOffset: distance from quad top to first visible pixel row (for precise vertical alignment).
	void measureText( const std::string& text, int* width, int* height, int* yOffset = nullptr, float scale = 1.0f ) const;

private:
	Font();

	// Atlas CPU data
	int                   mLineHeight;
	uint32_t              mAtlasSize;
	std::vector<uint32_t> mAtlasData;
	uint32_t mCellOffsetX;
	uint32_t mCellOffsetY;

	// Per-character UV and geometry (used by uploadGL to build the VBO)
	float    mUvX[256], mUvY[256], mUvW[256], mUvH[256];
	int      mGlyphW[256], mGlyphH[256]; // quad dimensions (mGlyphH = full cell height)
	int      mGlyphBitmapH[256];         // actual bitmap rows
	int      mGlyphYOffset[256];         // pixel offset from top of cell to first bitmap row

	int32_t  mBlurRadius;
	uint32_t mGLTextureID;
	uint32_t mGLVBO;
};
