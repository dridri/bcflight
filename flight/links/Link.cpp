#include "Link.h"

Link::Link()
	: mConnected( false )
{
}


Link::~Link()
{
}


bool Link::isConnected() const
{
	return mConnected;
}