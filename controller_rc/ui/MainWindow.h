#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QTimer>

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


public slots:
	void Update();
	void Home();
	void Calibrate();
	void Network();
	void Settings();
	void CalibrateGyro();
	void CalibrateIMU();
	void CalibrationReset();
	void CalibrationApply();

private:
	Controller* mController;
	QTimer* mUpdateTimer;
	Ui::MainWindow* ui;
	Ui::PageMain* uiPageMain;
	Ui::PageCalibrate* uiPageCalibrate;
	QWidget* mPageMain;
	QWidget* mPageCalibrate;

	typedef struct {
		uint16_t min;
		uint16_t center;
		uint16_t max;
	} CalibrationValues;
	CalibrationValues mCalibrationValues[4];
};

#endif // MAINWINDOW_H
