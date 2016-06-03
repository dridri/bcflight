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
	void ResetBattery();
	void Calibrate();
	void CalibrateAll();
	void CalibrateESCs();
	void ArmDisarm();
	void throttleChanged( int throttle );

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
};

#endif // MAINWINDOW_H
