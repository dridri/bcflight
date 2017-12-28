#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QTimer>
#include <Controller.h>

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
	void VideoLockWhiteBalance();
	void VideoIsoIncrease();
	void VideoIsoDecrease();

private:
	void CameraUpdateLensShader( bool send = true );

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

	typedef struct {
		Controller::CameraLensShaderColor r;
		Controller::CameraLensShaderColor g;
		Controller::CameraLensShaderColor b;
		uint8_t grid[4 * 52 * 39];
	} CameraLensShader;
	CameraLensShader mCameraLensShader;
};

#endif // MAINWINDOW_H
