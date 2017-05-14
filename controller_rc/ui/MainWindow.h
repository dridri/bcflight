#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QTimer>

class Controller;

namespace Ui {
	class MainWindow;
	class PageMain;
	class PageCalibrate;
	class PageCamera;
	class PageNetwork;
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
	void Camera();
	void Settings();
	void CalibrateGyro();
	void CalibrateIMU();
	void ResetBattery();
	void CalibrationReset();
	void CalibrationApply();

	void VideoBrightnessIncrease();
	void VideoBrightnessDecrease();
	void VideoContrastIncrease();
	void VideoContrastDecrease();
	void VideoSaturationIncrease();
	void VideoSaturationDecrease();
	void VideoWhiteBalance();

private:
	Controller* mController;
	QTimer* mUpdateTimer;
	Ui::MainWindow* ui;
	Ui::PageMain* uiPageMain;
	Ui::PageCalibrate* uiPageCalibrate;
	Ui::PageCamera* uiPageCamera;
	Ui::PageNetwork* uiPageNetwork;
	QWidget* mPageMain;
	QWidget* mPageCalibrate;
	QWidget* mPageCamera;
	QWidget* mPageNetwork;

	typedef struct {
		uint16_t min;
		uint16_t center;
		uint16_t max;
	} CalibrationValues;
	CalibrationValues mCalibrationValues[4];
};

#endif // MAINWINDOW_H
