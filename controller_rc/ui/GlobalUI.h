#ifndef GLOBALUI_H
#define GLOBALUI_H

#include <QtWidgets/QApplication>
#include <Thread.h>

class MainWindow;

class Controller;

class GlobalUI : public Thread
{
public:
	GlobalUI( Controller* controller );
	~GlobalUI();

	void Run() { while( run() ); }

protected:
	virtual bool run();

private:
	QApplication* mApplication;
	Controller* mController;
	MainWindow* mMainWindow;
};

#endif // GLOBALUI_H
