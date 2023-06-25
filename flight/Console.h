#ifndef CONSOLE_H
#define CONSOLE_H

#include <Thread.h>

class Config;

class Console : public Thread
{
public:
	Console( Config* config );
	~Console();

protected:
	virtual bool run();
	bool alnum( char c );

	Config* mConfig;
	vector<string> mFullHistory;
};

#endif // CONSOLE_H
