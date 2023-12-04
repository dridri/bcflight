#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QTimer>
#include <ControllerClient.h>

namespace Ui {
	class MainWindow;
	class PageMain;
	class PageCalibrate;
	class PageCamera;
	class PageNetwork;
	class PageSettings;
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
	void VideoExposureMode();
	void VideoIsoIncrease();
	void VideoIsoDecrease();
// 	void VideoIsoAuto();
	void VideoShutterSpeedIncrease();
	void VideoShutterSpeedDecrease();
	void VideoShutterSpeedAuto();

	void SimulatorMode( bool enabled );

private:
	void CameraUpdateLensShader( bool send = true );

	ControllerClient* mController;
	QTimer* mUpdateTimer;
	Ui::MainWindow* ui;
	Ui::PageMain* uiPageMain;
	Ui::PageCalibrate* uiPageCalibrate;
	Ui::PageCamera* uiPageCamera;
	Ui::PageNetwork* uiPageNetwork;
	Ui::PageSettings* uiPageSettings;
	QWidget* mPageMain;
	QWidget* mPageCalibrate;
	QWidget* mPageCamera;
	QWidget* mPageNetwork;
	QWidget* mPageSettings;

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
