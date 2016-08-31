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
#include <QtWidgets/QMainWindow>

namespace Ui {
	class MainWindow;
}

class Link;
class ControllerPC;
class ControllerMonitor;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( ControllerPC* ctrl, Link* streamLink );
	~MainWindow();

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
	void VideoBrightnessIncrease();
	void VideoBrightnessDecrease();
	void VideoContrastIncrease();
	void VideoContrastDecrease();
	void VideoSaturationIncrease();
	void VideoSaturationDecrease();
	void VideoRecord();
	void RecordingsRefresh();

private:
	Ui::MainWindow* ui;
	ControllerPC* mController;
	ControllerMonitor* mControllerMonitor;
	Link* mStreamLink;
	QTimer* mUpdateTimer;

	QElapsedTimer mTicks;
	QVector< double > mDataT;
	QVector< double > mDataR;
	QVector< double > mDataP;
	QVector< double > mDataY;
	QVector< double > mDataAltitude;

	bool mPIDsOk;
	bool mPIDsReading;
};

#endif // MAINWINDOW_H
