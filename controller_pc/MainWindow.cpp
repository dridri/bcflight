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

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTime>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QHBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerlua.h>
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
	, mPIDsOk( false )
	, mPIDsReading( true )
{
	mControllerMonitor = new ControllerMonitor( mController );
	connect( mControllerMonitor, SIGNAL( connected() ), this, SLOT( connected() ) );
	mControllerMonitor->start();

	mUpdateTimer = new QTimer();
	connect( mUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateData() ) );
	mUpdateTimer->setInterval( 20 );
	mUpdateTimer->start();

	ui = new Ui::MainWindow;
	ui->setupUi(this);

	QsciLexerLua* lexerLua = new QsciLexerLua;
	ui->config->setLexer( lexerLua );
	ui->config->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );

	connect( ui->reset_battery, SIGNAL( pressed() ), this, SLOT( ResetBattery() ) );
	connect( ui->calibrate, SIGNAL( pressed() ), this, SLOT( Calibrate() ) );
	connect( ui->calibrate_all, SIGNAL( pressed() ), this, SLOT( CalibrateAll() ) );
	connect( ui->calibrate_escs, SIGNAL( pressed() ), this, SLOT( CalibrateESCs() ) );
	connect( ui->arm, SIGNAL( pressed() ), this, SLOT( ArmDisarm() ) );
	connect( ui->throttle, SIGNAL( valueChanged(int) ), this, SLOT( throttleChanged(int) ) );
	connect( ui->config_load, SIGNAL( pressed() ), this, SLOT( LoadConfig() ) );
	connect( ui->config_save, SIGNAL( pressed() ), this, SLOT( SaveConfig() ) );
	connect( ui->firmware_browse, SIGNAL( pressed() ), this, SLOT( FirmwareBrowse() ) );
	connect( ui->firmware_upload, SIGNAL( pressed() ), this, SLOT( FirmwareUpload() ) );
	connect( ui->stabilized_mode, SIGNAL( pressed() ), this, SLOT( ModeStabilized() ) );
	connect( ui->rate_mode, SIGNAL( pressed() ), this, SLOT( ModeRate() ) );
	connect( ui->brightness_dec, SIGNAL( pressed() ), this, SLOT( VideoBrightnessDecrease() ) );
	connect( ui->brightness_inc, SIGNAL( pressed() ), this, SLOT( VideoBrightnessIncrease() ) );
	connect( ui->contrast_dec, SIGNAL( pressed() ), this, SLOT( VideoContrastDecrease() ) );
	connect( ui->contrast_inc, SIGNAL( pressed() ), this, SLOT( VideoContrastIncrease() ) );
	connect( ui->saturation_dec, SIGNAL( pressed() ), this, SLOT( VideoSaturationDecrease() ) );
	connect( ui->saturation_inc, SIGNAL( pressed() ), this, SLOT( VideoSaturationIncrease() ) );
	connect( ui->record, SIGNAL( pressed() ), this, SLOT( VideoRecord() ) );
	connect( ui->recordings_refresh, SIGNAL( pressed() ), this, SLOT( RecordingsRefresh() ) );

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


static void Recurse( lua_State* L, QTreeWidgetItem* parent, QString name, int index = -1, int indent = 0 )
{
	std::cout << "Recurse( L, parent, " << name.toStdString() << ", " << index << ", " << indent << ")\n";
	if ( name == "CurrentSensors" ) {
		name = "Current Sensors";
	}

	QVariant data;
	QTreeWidgetItem* item = nullptr;
	for ( int32_t i = 0; i < parent->childCount(); i++ ) {
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
	qDebug() << "sensors_infos : " << sensors_infos;
	lua_State* L = luaL_newstate();
	luaL_dostring( L, sensors_infos.toLatin1().data() );
	Recurse( L, ui->system->findItems( "BCFlight", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first(), "Sensors" );
	lua_close(L);

	ui->system->expandAll();
}

void MainWindow::updateData()
{
	QString conn = mController->isConnected() ? "Connected" : "Disconnected";
	ui->statusbar->showMessage( conn + QString( "    |    RX Qual : %1 %    |    TX Qual : %2 %    |    TX : %3 B/s    |    RX : %4 B/s    |    Camera : %5 KB/s ( %6 % )    |    %7 FPS" ).arg( mController->link()->RxQuality(), 3, 10, QChar(' ') ).arg( mController->droneRxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->writeSpeed(), 4, 10, QChar(' ') ).arg( mController->link()->readSpeed(), 4, 10, QChar(' ') ).arg( mStreamLink->readSpeed() / 1024, 4, 10, QChar(' ') ).arg( mStreamLink->RxQuality(), 3, 10, QChar(' ') ).arg( ui->video->fps() ) );

	ui->latency->setText( QString::number( mController->ping() ) + " ms" );
	ui->voltage->setText( QString::number( mController->batteryVoltage(), 'f', 2 ) + " V" );
	ui->current->setText( QString::number( mController->currentDraw(), 'f', 2 ) + " A" );
	ui->current_total->setText( QString::number( mController->totalCurrent() ) + " mAh" );
	ui->cpu_load->setValue( mController->CPULoad() );
	ui->temperature->setValue( mController->CPUTemp() );

	std::string dbg = mController->debugOutput();
	if ( dbg != "" ) {
		QTextCursor cursor( ui->terminal->textCursor() );
		bool move = true; //( ui->terminal->toPlainText().length() == 0 or cursor.position() == QTextCursor::End );
		ui->terminal->setPlainText( ui->terminal->toPlainText() + QString::fromStdString( dbg ) );
		if ( move ) {
			cursor.movePosition( QTextCursor::End );
			ui->terminal->setTextCursor( cursor );
		}
	}

	const std::list< vec4 > rpy;// = mController->rpyHistory();
	mDataT.clear();
	mDataR.clear();
	mDataP.clear();
	mDataY.clear();
	for ( vec4 v : rpy ) {
		mDataT.append( v.w );
		mDataR.append( v.x );
		mDataP.append( v.y );
		mDataY.append( v.z );
	}

	ui->rpy->graph(0)->setData( mDataT, mDataR );
	ui->rpy->graph(1)->setData( mDataT, mDataP );
	ui->rpy->graph(2)->setData( mDataT, mDataY );
	ui->rpy->graph(0)->rescaleAxes();
	ui->rpy->graph(1)->rescaleAxes( true );
	ui->rpy->graph(2)->rescaleAxes( true );
	ui->rpy->xAxis->rescale();
	ui->rpy->replot();
/*
	ui->rates->graph(0)->setData( mDataT, mDataR );
	ui->rates->graph(1)->setData( mDataT, mDataP );
	ui->rates->graph(2)->setData( mDataT, mDataY );
	ui->rates->graph(0)->rescaleAxes();
	ui->rates->graph(1)->rescaleAxes( true );
	ui->rates->graph(2)->rescaleAxes( true );
	ui->rates->xAxis->rescale();
	ui->rates->replot();
*/
	ui->altitude->graph(0)->setData( mDataT, mDataAltitude );
	ui->altitude->graph(0)->rescaleAxes();
	ui->altitude->xAxis->rescale();
	ui->altitude->replot();

	if ( mController->isConnected() and ( not mPIDsOk or ( ui->rateP->value() == 0.0f and ui->rateI->value() == 0.0f and ui->rateD->value() == 0.0f and ui->horizonP->value() == 0.0f and ui->horizonI->value() == 0.0f and ui->horizonD->value() == 0.0f ) ) ) {
		mPIDsReading = true;
		mController->ReloadPIDs();
		ui->rateP->setValue( mController->rollPid().x );
		ui->rateI->setValue( mController->rollPid().y );
		ui->rateD->setValue( mController->rollPid().z );
		ui->rateP_2->setValue( mController->pitchPid().x );
		ui->rateI_2->setValue( mController->pitchPid().y );
		ui->rateD_2->setValue( mController->pitchPid().z );
		ui->rateP_3->setValue( mController->yawPid().x );
		ui->rateI_3->setValue( mController->yawPid().y );
		ui->rateD_3->setValue( mController->yawPid().z );
		ui->horizonP->setValue( mController->outerPid().x );
		ui->horizonI->setValue( mController->outerPid().y );
		ui->horizonD->setValue( mController->outerPid().z );
		connect( ui->rateP, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDRoll(double) ) );
		connect( ui->rateI, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDRoll(double) ) );
		connect( ui->rateD, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDRoll(double) ) );
		connect( ui->rateP_2, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDPitch(double) ) );
		connect( ui->rateI_2, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDPitch(double) ) );
		connect( ui->rateD_2, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDPitch(double) ) );
		connect( ui->rateP_3, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDYaw(double) ) );
		connect( ui->rateI_3, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDYaw(double) ) );
		connect( ui->rateD_3, SIGNAL( valueChanged(double) ), this, SLOT( setRatePIDYaw(double) ) );
		connect( ui->horizonP, SIGNAL( valueChanged(double) ), this, SLOT( setHorizonP(double) ) );
		connect( ui->horizonI, SIGNAL( valueChanged(double) ), this, SLOT( setHorizonI(double) ) );
		connect( ui->horizonD, SIGNAL( valueChanged(double) ), this, SLOT( setHorizonD(double) ) );
		mPIDsOk = true;
		mPIDsReading = false;
	}
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
		std::cout << "Disarming...\n";
// 		mController->Disarm();
		mController->setArmed( false );
		ui->arm->setText( "Arm" );
	} else {
		std::cout << "Arming...\n";
// 		mController->Arm();
		mController->setArmed( true );
		ui->arm->setText( "Disarm" );
	}
}


void MainWindow::throttleChanged( int throttle )
{
	mController->setControlThrust( (double)throttle / 100.0f );
}


void MainWindow::LoadConfig()
{
	std::string conf = mController->getConfigFile();
	ui->config->setText( QString::fromStdString( conf ) );
}


void MainWindow::SaveConfig()
{
	std::string conf = ui->config->text().toStdString();
	mController->setConfigFile( conf );
}


void MainWindow::FirmwareBrowse()
{
	QFileDialog* diag = new QFileDialog( this );
	diag->setFileMode( QFileDialog::AnyFile );
	diag->show();
	connect( diag, SIGNAL( fileSelected(QString) ), this, SLOT( firmwareFileSelected(QString) ) );
}


void MainWindow::firmwareFileSelected( QString path )
{
	ui->firmware_path->setText( path );
}


void MainWindow::FirmwareUpload()
{
	ui->firmware_progress->setValue( 0 );

	if ( ui->firmware_path->text().length() > 0 and QFile::exists( ui->firmware_path->text() ) and mController->isConnected() ) {
		QFile f( ui->firmware_path->text() );
		if ( !f.open( QFile::ReadOnly ) ) return;
		QByteArray ba = f.readAll();

		mController->UploadUpdateInit();
		uint32_t chunk_size = 1024; // 2048
		for ( uint32_t offset = 0; offset < (uint32_t)ba.size(); offset += chunk_size ) {
			uint32_t sz = chunk_size;
			if ( ba.size() - offset < chunk_size ) {
				sz = ba.size() - offset;
			}
			mController->UploadUpdateData( (const uint8_t*)&ba.constData()[offset], offset, sz );
			ui->firmware_progress->setValue( offset * 100 / ba.size() + 1 );
			QApplication::processEvents();
		}

		QTextCursor cursor( ui->terminal->textCursor() );
		ui->terminal->setPlainText( ui->terminal->toPlainText() + "====> Applying firmware update and restarting service, please wait... <====" );
		cursor.movePosition( QTextCursor::End );
		ui->terminal->setTextCursor( cursor );

		mController->UploadUpdateProcess( (const uint8_t*)ba.constData(), ba.size() );
	}

	ui->firmware_progress->setValue( 0 );
}


void MainWindow::ModeRate()
{
	mController->setModeSwitch( Controller::Rate );
}


void MainWindow::ModeStabilized()
{
	mController->setModeSwitch( Controller::Stabilize );
}


void MainWindow::setRatePIDRoll( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec;
	vec.x = ui->rateP->value();
	vec.y = ui->rateI->value();
	vec.z = ui->rateD->value();
	mController->setRollPID( vec );
}


void MainWindow::setRatePIDPitch( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec;
	vec.x = ui->rateP_2->value();
	vec.y = ui->rateI_2->value();
	vec.z = ui->rateD_2->value();
	mController->setPitchPID( vec );
}


void MainWindow::setRatePIDYaw( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec;
	vec.x = ui->rateP_3->value();
	vec.y = ui->rateI_3->value();
	vec.z = ui->rateD_3->value();
	mController->setYawPID( vec );
}


void MainWindow::setHorizonP( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec = mController->outerPid();
	vec.x = v;
	mController->setOuterPID( vec );
}


void MainWindow::setHorizonI( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec = mController->outerPid();
	vec.y = v;
	mController->setOuterPID( vec );
}


void MainWindow::setHorizonD( double v )
{
	if ( mPIDsReading ) {
		return;
	}
	if ( mController->isConnected() ) {
		mPIDsOk = true;
	}
	vec3 vec = mController->outerPid();
	vec.z = v;
	mController->setOuterPID( vec );
}


void MainWindow::VideoBrightnessDecrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoBrightnessDecrease();
	}
}


void MainWindow::VideoBrightnessIncrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoBrightnessIncrease();
	}
}


void MainWindow::VideoContrastDecrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoContrastDecrease();
	}
}


void MainWindow::VideoContrastIncrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoContrastIncrease();
	}
}


void MainWindow::VideoSaturationDecrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoSaturationDecrease();
	}
}


void MainWindow::VideoSaturationIncrease()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoSaturationIncrease();
	}
}


void MainWindow::VideoRecord()
{
	qDebug() << "VideoRecord()" << ui->record->text();
	if ( ui->record->text().mid( ui->record->text().indexOf( "S" ) ) == QString( "Start" ) ) {
		qDebug() << "VideoRecord() Start";
		mController->setRecording( true );
		ui->record->setText( "Stop" );
	} else {
		qDebug() << "VideoRecord() Stop";
		mController->setRecording( false );
		ui->record->setText( "Start" );
	}
}


void MainWindow::RecordingsRefresh()
{
	ui->recordings->clearContents();
	ui->recordings->clear();
	for ( int32_t i = ui->recordings->rowCount() - 1; i >= 0; i-- ) {
		ui->recordings->removeRow( i );
	}

	std::vector< std::string > list = mController->recordingsList();
	for ( std::string input : list ) {
		std::string filename = input.substr( 0, input.find( ":" ) );
		std::string size = input.substr( input.find( ":" ) + 1 );
		if ( filename[0] != '.' ) {
// 			std::string snapshot_b64 = input.substr( input.find( ":" ) + 1 );
// 			QByteArray snapshot_raw = QByteArray::fromBase64( QString::fromStdString( snapshot_b64 ) );

			ui->recordings->insertRow( ui->recordings->rowCount() );
// 			ui->recordings->item( ui->recordings->rowCount() - 1, 0 )->setText( "" );
// 			ui->recordings->item( ui->recordings->rowCount() - 1, 1 )->setText( QString::fromStdString( filename ) );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 1, new QLabel( QString::fromStdString( filename ) ) );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 2, new QLabel( QString::fromStdString( size ) ) );

			QWidget* tools = new QWidget();
			QHBoxLayout* layout = new QHBoxLayout();
			tools->setLayout( layout );
			QPushButton* btn_save = new QPushButton();
			btn_save->setIcon( QIcon( ":icons/icon-download.png" ) );
			QPushButton* btn_delete = new QPushButton();
			btn_delete->setIcon( QIcon( ":icons/icon-delete.png" ) );
			layout->addWidget( btn_save );
			layout->addWidget( btn_delete );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 4, tools );
			ui->recordings->setRowHeight( ui->recordings->rowCount() - 1, 42 );
		}
	}
}
