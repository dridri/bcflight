#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

class Controller;

namespace Ui {
	class MainWindow;
	class PageMain;
	class PageCalibrate;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( Controller* controller );
	~MainWindow();

	void Update();

public slots:
	void Home();
	void Calibrate();
	void Network();
	void Settings();
	void CalibrateGyro();
	void CalibrateIMU();

private:
	Controller* mController;
	Ui::MainWindow* ui;
	Ui::PageMain* uiPageMain;
	Ui::PageCalibrate* uiPageCalibrate;
	QWidget* mPageMain;
	QWidget* mPageCalibrate;
};

#endif // MAINWINDOW_H
