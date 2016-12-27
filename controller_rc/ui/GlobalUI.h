#ifndef GLOBALUI_H
#define GLOBALUI_H

#include <QtWidgets/QApplication>
#include <Thread.h>

class GlobalUI : public Thread
{
public:
	GlobalUI();
	~GlobalUI();

	void Run() { while( run() ); }

protected:
	virtual bool run();

private:
	QApplication* mApplication;
};

#endif // GLOBALUI_H
