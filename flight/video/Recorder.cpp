#ifdef SYSTEM_NAME_Linux

#include <dirent.h>
#include <string.h>
#include "Recorder.h"
#include <Board.h>
#include "Debug.h"


std::set<Recorder*> Recorder::sRecorders;


Recorder::Recorder()
{
	sRecorders.insert( this );
}


Recorder::~Recorder()
{
}


const std::set<Recorder*>& Recorder::recorders()
{
	return sRecorders;
}


#endif // SYSTEM_NAME_Linux
