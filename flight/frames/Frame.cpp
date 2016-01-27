#include "Frame.h"
#include <Config.h>

std::map< std::string, std::function< Frame* ( Config* ) > > Frame::mKnownFrames;

Frame::Frame()
	: mMotors( std::vector< Motor* >() )
{
}


Frame::~Frame()
{
}


Frame* Frame::Instanciate( const std::string& name, Config* config )
{
	if ( mKnownFrames.find( name ) != mKnownFrames.end() ) {
		return mKnownFrames[ name ]( config );
	}
	return nullptr;
}


const std::vector< Motor* >& Frame::motors() const
{
	return mMotors;
}


void Frame::RegisterFrame( const std::string& name, std::function< Frame* ( Config* ) > instanciate )
{
	mKnownFrames[ name ] = instanciate;
}
