#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
public:
	MainWindow();
	~MainWindow();

private:
	Ui::MainWindow* ui;
};

#endif // MAINWINDOW_H
