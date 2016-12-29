#include <QtWidgets/QGridLayout>
#include <QtGui/QFontDatabase>
#include "MainWindow.h"
#include "ui_window.h"
#include "ui_main.h"
#include "ui_calibrate.h"
#include <Controller.h>

MainWindow::MainWindow( Controller* controller )
	: QMainWindow()
	, mController( controller )
{
	ui = new Ui::MainWindow;
	ui->setupUi(this);
	ui->gridLayout_2->setMargin( 0 );
	ui->gridLayout_2->setSpacing( 0 );
	ui->centralLayout->setMargin( 0 );
	ui->centralLayout->setSpacing( 0 );
	connect( ui->btnHome, SIGNAL( clicked() ), this, SLOT( Home() ) );
	connect( ui->btnCalibrate, SIGNAL( clicked() ), this, SLOT( Calibrate() ) );
	connect( ui->btnNetwork, SIGNAL( clicked() ), this, SLOT( Network() ) );
	connect( ui->btnSettings, SIGNAL( clicked() ), this, SLOT( Settings() ) );

	uiPageMain = new Ui::PageMain;
	mPageMain = new QWidget();
	uiPageMain->setupUi( mPageMain );
	connect( uiPageMain->calibrate_gyro, SIGNAL( clicked() ), this, SLOT( CalibrateGyro() ) );
	connect( uiPageMain->calibrate_imu, SIGNAL( clicked() ), this, SLOT( CalibrateIMU() ) );

	uiPageCalibrate = new Ui::PageCalibrate;
	mPageCalibrate = new QWidget();
	uiPageCalibrate->setupUi( mPageCalibrate );

	int id = QFontDatabase::addApplicationFont( "data/FreeMonoBold.ttf" );
	QString family = QFontDatabase::applicationFontFamilies(id).at(0);
	QFont mono( family, 22 );
	mono.setBold( true );

	setFont( mono );
	mPageMain->setFont( mono );

	ui->btnHome->setChecked( true );
	ui->centralLayout->addWidget( mPageMain );
}


MainWindow::~MainWindow()
{
	delete ui;
}


void MainWindow::Update()
{
	if ( ui->btnHome->isChecked() ) {
		uiPageMain->localVoltage->setText( QString("%1 V").arg( (double)mController->localBatteryVoltage(), 5, 'f', 2, '0' ) );
		uiPageMain->voltage->setText( QString("%1 V").arg( (double)mController->batteryVoltage(), 5, 'f', 2, '0' ) );
		uiPageMain->totalCurrent->setText( QString("%1 mAh").arg( mController->totalCurrent() ) );
		uiPageMain->latency->setText( QString("%1 ms").arg( mController->ping() ) );
		uiPageMain->txquality->setText( QString("%1% TX").arg( mController->droneRxQuality() ) );
		uiPageMain->rxquality->setText( QString("%1% RX").arg( mController->link()->RxQuality() ) );
	} else if ( ui->btnCalibrate->isChecked() ) {
		if ( uiPageCalibrate->tabWidget->currentTabText() == "Thrust" ) {
		} else if ( uiPageCalibrate->tabWidget->currentTabText() == "Yaw" ) {
		} else if ( uiPageCalibrate->tabWidget->currentTabText() == "Pitch" ) {
		} else if ( uiPageCalibrate->tabWidget->currentTabText() == "Roll" ) {
		}
	}
}


void MainWindow::Home()
{
	ui->btnHome->setChecked( true );
	ui->btnCalibrate->setChecked( false );
	ui->btnNetwork->setChecked( false );
	ui->btnSettings->setChecked( false );
	mPageCalibrate->setVisible( false );
	mPageMain->setVisible( true );
	ui->centralLayout->removeWidget( mPageCalibrate );
	ui->centralLayout->addWidget( mPageMain );
}


void MainWindow::Calibrate()
{
	ui->btnHome->setChecked( false );
	ui->btnCalibrate->setChecked( true );
	ui->btnNetwork->setChecked( false );
	ui->btnSettings->setChecked( false );
	mPageMain->setVisible( false );
	mPageCalibrate->setVisible( true );
	ui->centralLayout->removeWidget( mPageMain );
	ui->centralLayout->addWidget( mPageCalibrate );
}


void MainWindow::Network()
{
	ui->btnHome->setChecked( false );
	ui->btnCalibrate->setChecked( false );
	ui->btnNetwork->setChecked( true );
	ui->btnSettings->setChecked( false );
	mPageMain->setVisible( false );
	mPageCalibrate->setVisible( false );
	ui->centralLayout->removeWidget( mPageMain );
	ui->centralLayout->removeWidget( mPageCalibrate );
}


void MainWindow::Settings()
{
	ui->btnHome->setChecked( false );
	ui->btnCalibrate->setChecked( false );
	ui->btnNetwork->setChecked( false );
	ui->btnSettings->setChecked( true );
	mPageMain->setVisible( false );
	mPageCalibrate->setVisible( false );
	ui->centralLayout->removeWidget( mPageMain );
	ui->centralLayout->removeWidget( mPageCalibrate );
}


void MainWindow::CalibrateGyro()
{
	mController->Calibrate();
}


void MainWindow::CalibrateIMU()
{
	mController->CalibrateAll();
}
