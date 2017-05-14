#ifndef GLOBALUI_H
#define GLOBALUI_H

#include <QtWidgets/QApplication>
#include <Thread.h>

class MainWindow;

class Controller;
class Config;

class GlobalUI : public Thread
{
public:
	GlobalUI( Config* config, Controller* controller );
	~GlobalUI();

	void Run() { while( run() ); }

protected:
	virtual bool run();

private:
	QApplication* mApplication;
	Config* mConfig;
	Controller* mController;
	MainWindow* mMainWindow;
};

#endif // GLOBALUI_H
