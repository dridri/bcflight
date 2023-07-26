/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtWidgets/QMainWindow>
#include <QVBoxLayout>
#include <QProgressBar>
#include "Config.h"
#include "Thread.h"
#include "BlackBox.h"
#include "VideoEditor.h"
#include "Qsci/qsciscintilla.h"

namespace Ui {
	class MainWindow;
}

class Link;
class ControllerPC;
class ControllerMonitor;
class FirmwareUpdateThread;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

	bool RunFirmwareUpdate();
	ControllerPC* controller() { return mController; }
	Ui::MainWindow* getUi() const { return ui; }

public slots:
	void connected();
	void updateData();
	void ModeStabilized();
	void ModeRate();
	void ResetBattery();
	void Calibrate();
	void CalibrateAll();
	void CalibrateESCs();
	void ArmDisarm();
	void throttleChanged( int throttle );
	void setRatePIDRoll( double v );
	void setRatePIDPitch( double v );
	void setRatePIDYaw( double v );
	void setHorizonP( double v );
	void setHorizonI( double v );
	void setHorizonD( double v );
	void LoadConfig();
	void SaveConfig();
	void FirmwareBrowse();
	void FirmwareUpload();
	void firmwareFileSelected( QString path );
	void tunDevice();
	void VideoBrightnessIncrease();
	void VideoBrightnessDecrease();
	void VideoContrastIncrease();
	void VideoContrastDecrease();
	void VideoSaturationIncrease();
	void VideoSaturationDecrease();
	void VideoPause();
	void VideoRecord();
	void VideoTakePicture();
	void SetNightMode( int state );
	void RecordingsRefresh();
	void setFirmwareUpdateProgress( int val );
	void appendDebugOutput( const QString& str );
	void MotorTest();

signals:
	void firmwareUpdateProgress( int val );
	void debugOutput( const QString& str );

private:
	class FirmwareUpdateThread : public QThread {
	public:
		FirmwareUpdateThread( MainWindow* instance ) : QThread(), mInstance( instance ) {}
	protected:
		void run() {
			mInstance->RunFirmwareUpdate();
		}
		MainWindow* mInstance;
	};

	Ui::MainWindow* ui;
	Config* mConfig;
	BlackBox* mBlackBox;
	VideoEditor* mVideoEditor;
	ControllerPC* mController;
	ControllerMonitor* mControllerMonitor;
	Link* mStreamLink;
	QTimer* mUpdateTimer;
	FirmwareUpdateThread* mFirmwareUpdateThread;
	QVBoxLayout *motorSpeedLayout = NULL;
	QList<QProgressBar*> motorSpeedProgress;

	QElapsedTimer mTicks;
	QVector< double > mDataTrpy;
	QVector< double > mDataR;
	QVector< double > mDataP;
	QVector< double > mDataY;

	QVector< double > mDataTrates;
	QVector< double > mDataRatesX;
	QVector< double > mDataRatesY;
	QVector< double > mDataRatesZ;

	QVector< double > mDataTmagnetometer;
	QVector< double > mDataMagnetometerX;
	QVector< double > mDataMagnetometerY;
	QVector< double > mDataMagnetometerZ;

	QVector< double > mDataTaccelerometer;
	QVector< double > mDataAccelerometerX;
	QVector< double > mDataAccelerometerY;
	QVector< double > mDataAccelerometerZ;

	QVector< double > mDataTAltitude;
	QVector< double > mDataAltitude;

	QVector< double > mDataTratesdterm;
	QVector< double > mDataRatesdtermX;
	QVector< double > mDataRatesdtermY;
	QVector< double > mDataRatesdtermZ;

	bool mPIDsOk;
	bool mPIDsReading;
};

#endif // MAINWINDOW_H
