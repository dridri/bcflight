#include "Frame.h"

Frame::Frame()
	: mMotors( std::vector< Motor* >() )
{
}


Frame::~Frame()
{
}


const std::vector< Motor* >& Frame::motors() const
{
	return mMotors;
}
