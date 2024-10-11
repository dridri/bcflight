#include <catch2/catch_session.hpp>
#include <Debug.h>


int main( int argc, char* argv[] )
{
	Debug::setDebugLevel( Debug::Trace );
	Debug::setColors( false );
	int result = Catch::Session().run( argc, argv );
	return result;
}
