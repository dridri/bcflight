#include "LiveOutput.h"

LiveOutput::LiveOutput()
	: VideoEncoder()
{
}


LiveOutput::~LiveOutput()
{
}


void LiveOutput::Setup()
{
}


void LiveOutput::EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd )
{
	// Nothing to do here
	(void)size;
	(void)mem;
	(void)timestamp_us;
	(void)fd;
}
