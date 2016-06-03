#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTime>
#include <iostream>

extern "C" {
#include <luajit.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};

#include "MainWindow.h"
#include "ControllerPC.h"
#include "Controller.h"
#include "ui_mainWindow.h"
#include "qcustomplot.h"
#include "ui/HStatusBar.h"

MainWindow::MainWindow( ControllerPC* ctrl, Link* streamLink )
	: QMainWindow()
	, mController( ctrl )
	, mStreamLink( streamLink )
{
	mControllerMonitor = new ControllerMonitor( mController );
	connect( mControllerMonitor, SIGNAL( connected() ), this, SLOT( connected() ) );
	mControllerMonitor->start();

	mUpdateTimer = new QTimer();
	connect( mUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateData() ) );
	mUpdateTimer->setInterval( 50 );
	mUpdateTimer->start();

	ui = new Ui::MainWindow;
	ui->setupUi(this);

	connect( ui->reset_battery, SIGNAL( pressed() ), this, SLOT( ResetBattery() ) );
	connect( ui->calibrate, SIGNAL( pressed() ), this, SLOT( Calibrate() ) );
	connect( ui->calibrate_all, SIGNAL( pressed() ), this, SLOT( CalibrateAll() ) );
	connect( ui->calibrate_escs, SIGNAL( pressed() ), this, SLOT( CalibrateESCs() ) );
	connect( ui->arm, SIGNAL( pressed() ), this, SLOT( ArmDisarm() ) );
	connect( ui->throttle, SIGNAL( valueChanged(int) ), this, SLOT( throttleChanged(int) ) );

	ui->statusbar->showMessage( "Disconnected" );

	ui->cpu_load->setSuffix( "%" );
	ui->temperature->setSuffix( "°C" );
	ui->temperature->setMaxValue( 80 );

	// Setup Roll-Pitch-Yaw graph
	ui->rpy->setBackground( QBrush( QColor( 0, 0, 0, 0 ) ) );
	ui->rpy->xAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
	ui->rpy->yAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
	ui->rpy->yAxis->setNumberFormat( "f" );
	ui->rpy->yAxis->setNumberPrecision( 2 );
	ui->rpy->xAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
	ui->rpy->yAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
	ui->rpy->yAxis->grid()->setZeroLinePen( QPen( QBrush( QColor( 96, 96, 96 ) ), 2, Qt::DashLine ) );
	ui->rpy->yAxis->setRange( -1.0f, 1.0f );
	ui->rpy->addGraph();
	ui->rpy->addGraph();
	ui->rpy->addGraph();
	ui->rpy->graph(0)->setPen( QPen( QBrush( QColor( 255, 128, 128 ) ), 2 ) );
	ui->rpy->graph(1)->setPen( QPen( QBrush( QColor( 128, 255, 128 ) ), 2 ) );
	ui->rpy->graph(2)->setPen( QPen( QBrush( QColor( 128, 128, 255 ) ), 2 ) );

	ui->rates->setBackground( QBrush( QColor( 0, 0, 0, 0 ) ) );

	ui->altitude->setBackground( QBrush( QColor( 0, 0, 0, 0 ) ) );
	ui->altitude->xAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
	ui->altitude->yAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
	ui->altitude->yAxis->setNumberFormat( "f" );
	ui->altitude->yAxis->setNumberPrecision( 3 );
	ui->altitude->xAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
	ui->altitude->yAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
	ui->altitude->yAxis->grid()->setZeroLinePen( QPen( QBrush( QColor( 96, 96, 96 ) ), 2, Qt::DashLine ) );
	ui->altitude->addGraph();
	ui->altitude->graph(0)->setPen( QPen( QBrush( QColor( 255, 255, 255 ) ), 2 ) );

	mTicks.start();

	ui->video->setLink( streamLink );
}


MainWindow::~MainWindow()
{
	delete ui;
}


static bool Recurse( lua_State* L, QTreeWidgetItem* parent, QString name, int index = -1, int indent = 0 )
{
	if ( name == "CurrentSensors" ) {
		name = "Current Sensors";
	}

	QVariant data;
	QTreeWidgetItem* item = nullptr;
	for ( uint32_t i = 0; i < parent->childCount(); i++ ) {
		if ( parent->child(i)->data( 0, 0 ) == name ) {
			item = parent->child(i);
			break;
		}
	}
	if ( not item ) {
		item = new QTreeWidgetItem();
		item->setData( 0, 0, name );
		parent->addChild( item );
	}


	if ( indent == 0 ) {
		lua_getfield( L, LUA_GLOBALSINDEX, name.toLatin1().data() );
	}
	if ( lua_isnil( L, index ) ) {
	} else if ( lua_isnumber( L, index ) ) {
		data = lua_tonumber( L, index );
	} else if ( lua_isboolean( L, index ) ) {
		data = ( lua_toboolean( L, index ) ? "true" : "false" );
	} else if ( lua_isstring( L, index ) ) {
		data = lua_tostring( L, index );
	} else if ( lua_iscfunction( L, index ) ) {
	} else if ( lua_isuserdata( L, index ) ) {
	} else if ( lua_istable( L, index ) ) {
		lua_pushnil( L );
		while( lua_next( L, -2 ) != 0 ) {
			const char* key = lua_tostring( L, index-1 );
			Recurse( L, item, key, index, indent + 1 );
			lua_pop( L, 1 );
		}
	}

	if ( data.isValid() ) {
		item->setData( 1, 0, data );
	}
}


void MainWindow::connected()
{
	ui->statusbar->showMessage( "Connected" );

	QTreeWidgetItem* board_item = ui->system->findItems( "Board", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first();
	QStringList board_infos = QString::fromStdString( mController->getBoardInfos() ).split("\n");
	for ( QString s : board_infos ) {
		if ( s.contains(":") ) {
			QStringList key_value = s.split(":");
			QString key = key_value.at(0);
			QString value = key_value.at(1);
			QTreeWidgetItem* item = new QTreeWidgetItem();
			item->setData( 0, 0, key );
			item->setData( 1, 0, value );
			board_item->addChild( item );
		}
	}

	QString sensors_infos = QString::fromStdString( mController->getSensorsInfos() );
	lua_State* L = luaL_newstate();
	luaL_dostring( L, sensors_infos.toLatin1().data() );
	Recurse( L, ui->system->findItems( "BCFlight", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first(), "Sensors" );
	lua_close(L);

	ui->system->expandAll();

}


void MainWindow::updateData()
{
	ui->statusbar->showMessage( QString( "Connected    |    RX Qual : %1 %    |    TX : %2 B/s    |    RX : %3 B/s    |    Camera : %4 KB/s    |    %5 FPS" ).arg( mController->link()->RxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->writeSpeed(), 4, 10, QChar(' ') ).arg( mController->link()->readSpeed(), 4, 10, QChar(' ') ).arg( mStreamLink->readSpeed() / 1024, 4, 10, QChar(' ') ).arg( ui->video->fps() ) );

	ui->latency->setText( QString::number( mController->ping() ) + " ms" );
	ui->voltage->setText( QString::number( mController->batteryVoltage(), 'f', 2 ) + " V" );
	ui->current->setText( QString::number( mController->currentDraw(), 'f', 2 ) + " A" );
	ui->cpu_load->setValue( mController->CPULoad() );
	ui->temperature->setValue( mController->CPUTemp() );

	if ( mDataT.size() >= 256 ) {
		mDataT.pop_front();
		mDataR.pop_front();
		mDataP.pop_front();
		mDataY.pop_front();
		mDataAltitude.pop_front();
	}

	mDataT.append( (double)mTicks.elapsed() / 1000.0 );
	mDataR.append( mController->rpy().x );
	mDataP.append( mController->rpy().y );
	mDataY.append( mController->rpy().z );
	mDataAltitude.append( mController->altitude() );

	ui->rpy->graph(0)->setData( mDataT, mDataR );
	ui->rpy->graph(1)->setData( mDataT, mDataP );
	ui->rpy->graph(2)->setData( mDataT, mDataY );
	ui->rpy->graph(0)->rescaleAxes();
	ui->rpy->graph(1)->rescaleAxes( true );
	ui->rpy->graph(2)->rescaleAxes( true );
	ui->rpy->xAxis->rescale();
	ui->rpy->replot();

	ui->altitude->graph(0)->setData( mDataT, mDataAltitude );
	ui->altitude->graph(0)->rescaleAxes();
	ui->altitude->xAxis->rescale();
	ui->altitude->replot();
}


void MainWindow::ResetBattery()
{
	mController->ResetBattery();
}


void MainWindow::Calibrate()
{
	mController->Calibrate();
}


void MainWindow::CalibrateAll()
{
	mController->CalibrateAll();
}


void MainWindow::CalibrateESCs()
{
	mController->CalibrateESCs();
}


void MainWindow::ArmDisarm()
{
	if ( mController->armed() ) {
		mController->Disarm();
		ui->arm->setText( "Arm" );
	} else {
		mController->Arm();
		ui->arm->setText( "Disarm" );
	}
}


void MainWindow::throttleChanged( int throttle )
{
	mController->setControlThrust( (double)throttle / 100.0f );
}
