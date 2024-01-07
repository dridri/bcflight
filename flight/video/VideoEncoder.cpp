#include "VideoEncoder.h"


VideoEncoder::VideoEncoder()
	: mRecorder( nullptr )
	, mLink( nullptr )
	, mRecorderTrackId( (uint32_t)-1 )
	, mInputWidth( 0 )
	, mInputHeight( 0 )
{
}


VideoEncoder::~VideoEncoder()
{
}


void VideoEncoder::SetInputResolution( int width, int height )
{
	mInputWidth = width;
	mInputHeight = height;
}
