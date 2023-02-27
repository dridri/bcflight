#include "VideoEncoder.h"


VideoEncoder::VideoEncoder()
	: mRecorder( nullptr )
	, mLink( nullptr )
	, mRecorderTrackId( (uint32_t)-1 )
{
}


VideoEncoder::~VideoEncoder()
{
}
