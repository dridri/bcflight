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
#include <fftw3.h>

extern "C" {
#include <luajit.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};

#include <RawWifi.h>
#include <Socket.h>

#include <functional>
#include "MainWindow.h"
#include "ControllerPC.h"
#include "Controller.h"
#include "ui_mainWindow.h"
#include "qcustomplot.h"
#include "ui/HStatusBar.h"

MainWindow::MainWindow()
	: QMainWindow()
	, mController( nullptr )
	, mControllerMonitor( nullptr )
	, mStreamLink( nullptr )
	, mFirmwareUpdateThread( nullptr )
	, motorSpeedLayout( nullptr )
	, mRatesPlot( false )
	, mDnfPlot( false )
	, mRatesPlotSpectrum( false )
	, mPIDsOk( false )
	, mPIDsReading( true )
{
	ui = new Ui::MainWindow;
	ui->setupUi(this);

	mConfig = new Config();
	connect( ui->actionSettings, SIGNAL( triggered(bool) ), mConfig, SLOT( show() ) );

	mBlackBox = new BlackBox();
	connect( ui->actionInspect_blackbox, SIGNAL( triggered(bool) ), mBlackBox, SLOT( show() ) );

	mVideoEditor = new VideoEditor();
	connect( ui->actionVideo_Editor, SIGNAL( triggered(bool) ), mVideoEditor, SLOT( show() ) );

	Link* mControllerLink = nullptr;
	Link* mAudioLink = nullptr;

	if ( mConfig->value( "link/rawwifi", true ).toBool() == true ) {
#ifndef NO_RAWWIFI
		std::string device = mConfig->value( "rawwifi/device", "" ).toString().toStdString();
		if ( device != "" ) {
			RawWifi* controllerLink = new RawWifi( device, mConfig->value( "rawwifi/controller/outport", 0 ).toInt(), mConfig->value( "rawwifi/controller/inport", 1 ).toInt() );
			mControllerLink = controllerLink;
			// controllerLink->SetChannel( mConfig->value( "rawwifi/channel", 9 ).toInt() );
			// controllerLink->setRetriesCount( mConfig->value( "rawwifi/controller/retries", 1 ).toInt() );
			// controllerLink->setCECMode( mConfig->value( "rawwifi/controller/cec", "" ).toString().toLower().toStdString() );
			// controllerLink->setDropBroken( not mConfig->value( "rawwifi/controller/nodrop", false ).toBool() );
			mStreamLink = new RawWifi( device, mConfig->value( "rawwifi/video/outport", 10 ).toInt(), mConfig->value( "rawwifi/video/inport", 11 ).toInt() );
			// static_cast<RawWifi*>(mStreamLink)->SetChannel( mConfig->value( "rawwifi/channel", 9 ).toInt() );
			// static_cast<RawWifi*>(mStreamLink)->setRetriesCount( mConfig->value( "rawwifi/video/retries", 1 ).toInt() );
			// static_cast<RawWifi*>(mStreamLink)->setCECMode( mConfig->value( "rawwifi/video/cec", "" ).toString().toLower().toStdString() );
			// static_cast<RawWifi*>(mStreamLink)->setDropBroken( not mConfig->value( "rawwifi/video/nodrop", false ).toBool() );
			// mAudioLink = new RawWifi( device, mConfig->value( "rawwifi/audio/outport", 20 ).toInt(), mConfig->value( "rawwifi/audio/inport", 21 ).toInt() );
			// static_cast<RawWifi*>(mAudioLink)->SetChannel( mConfig->value( "rawwifi/channel", 9 ).toInt() );
			// static_cast<RawWifi*>(mAudioLink)->setRetriesCount( mConfig->value( "rawwifi/audio/retries", 1 ).toInt() );
			// static_cast<RawWifi*>(mAudioLink)->setCECMode( mConfig->value( "rawwifi/audio/cec", "" ).toString().toLower().toStdString() );
			// static_cast<RawWifi*>(mAudioLink)->setDropBroken( not mConfig->value( "rawwifi/audio/nodrop", false ).toBool() );
		}
#endif // NO_RAWWIFI
	} else {
		std::function<Socket::PortType(const QString&)> socket_type = [](const QString& type){ if ( type == "UDPLite" ) return Socket::UDPLite; else if ( type == "UDP" ) return Socket::UDP; else return Socket::TCP; };
		mControllerLink = new Socket( mConfig->value( "tcpip/address", "192.168.32.1" ).toString().toStdString(), mConfig->value( "tcpip/controller/port", 2020 ).toInt(), socket_type( mConfig->value( "tcpip/controller/type", "TCP" ).toString() ) );
		mStreamLink = new Socket( mConfig->value( "tcpip/address", "192.168.32.1" ).toString().toStdString(), mConfig->value( "tcpip/video/port", 2021 ).toInt(), socket_type( mConfig->value( "tcpip/video/type", "UDPLite" ).toString() ) );
		// mAudioLink = new Socket( mConfig->value( "tcpip/address", "192.168.32.1" ).toString().toStdString(), mConfig->value( "tcpip/audio/port", 2022 ).toInt(), socket_type( mConfig->value( "tcpip/audio/type", "TCP" ).toString() ) );
	}
	uint8_t test[2048];
	// mControllerLink->Read( test, 2048, 0 );
	mController = new ControllerPC( mControllerLink, mConfig->value( "controller/spectate", false ).toBool() );

	if ( mController ) {
		mControllerMonitor = new ControllerMonitor( mController );
		connect( mControllerMonitor, SIGNAL( connected() ), this, SLOT( connected() ) );
		mControllerMonitor->start();
	}

	mUpdateTimer = new QTimer();
	connect( mUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateData() ) );
	mUpdateTimer->setInterval( 20 );
	mUpdateTimer->start();

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
	connect( ui->tundev, SIGNAL( pressed() ), this, SLOT( tunDevice() ) );
	connect( ui->stabilized_mode, SIGNAL( pressed() ), this, SLOT( ModeStabilized() ) );
	connect( ui->rate_mode, SIGNAL( pressed() ), this, SLOT( ModeRate() ) );
	connect( ui->brightness_dec, SIGNAL( pressed() ), this, SLOT( VideoBrightnessDecrease() ) );
	connect( ui->brightness_inc, SIGNAL( pressed() ), this, SLOT( VideoBrightnessIncrease() ) );
	connect( ui->contrast_dec, SIGNAL( pressed() ), this, SLOT( VideoContrastDecrease() ) );
	connect( ui->contrast_inc, SIGNAL( pressed() ), this, SLOT( VideoContrastIncrease() ) );
	connect( ui->saturation_dec, SIGNAL( pressed() ), this, SLOT( VideoSaturationDecrease() ) );
	connect( ui->saturation_inc, SIGNAL( pressed() ), this, SLOT( VideoSaturationIncrease() ) );
	connect( ui->record, SIGNAL( pressed() ), this, SLOT( VideoRecord() ) );
	connect( ui->take_picture, SIGNAL( pressed() ), this, SLOT( VideoTakePicture() ) );
	connect( ui->white_balance_lock, SIGNAL( pressed() ), this, SLOT( VideoWhiteBalanceLock() ) );
	connect( ui->recordings_refresh, SIGNAL( pressed() ), this, SLOT( RecordingsRefresh() ) );
	connect( ui->night_mode, SIGNAL( stateChanged(int) ), this, SLOT( SetNightMode(int) ) );
	connect( ui->motorTestButton, SIGNAL(pressed()), this, SLOT(MotorTest()));
	connect( ui->motorsBeepStart, &QPushButton::pressed, [this]() { MotorsBeep(true); });
	connect( ui->motorsBeepStop, &QPushButton::pressed, [this]() { MotorsBeep(false); });
	connect( ui->gyroPlotButton, &QPushButton::pressed, [this]() { mRatesPlot = false; mDnfPlot = false; });
	connect( ui->ratesPlotButton, &QPushButton::pressed, [this]() { mRatesPlot = true; mDnfPlot = false; });
	connect( ui->dnfPlotButton, &QPushButton::pressed, [this]() { mDnfPlot = true; mRatesPlot = false; });
	connect( ui->gyroSpectrumButton, &QCheckBox::stateChanged, [this](int state) { mRatesPlotSpectrum = ( state == Qt::Checked ); });

	ui->statusbar->showMessage( "Disconnected" );

	ui->cpu_load->setSuffix( "%" );
	ui->temperature->setSuffix( "°C" );
	ui->temperature->setMaxValue( 80 );

	// Setup graphs
	std::list< QCustomPlot* > graphs;
	graphs.emplace_back( ui->rpy );
	graphs.emplace_back( ui->rates );
	graphs.emplace_back( ui->accelerometer );
	graphs.emplace_back( ui->magnetometer );
	graphs.emplace_back( ui->altitude );
	// graphs.emplace_back( ui->gps );
	graphs.emplace_back( ui->rates_dterm );
	for ( QCustomPlot* graph : graphs ) {
		graph->setBackground( QBrush( QColor( 0, 0, 0, 0 ) ) );
		graph->xAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
		graph->yAxis->setTickLabelColor( QColor( 255, 255, 255 ) );
		graph->yAxis->setNumberFormat( "f" );
		graph->yAxis->setNumberPrecision( 2 );
		graph->xAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
		graph->yAxis->grid()->setPen( QPen( QBrush( QColor( 64, 64, 64 ) ), 1, Qt::DashDotDotLine ) );
		graph->yAxis->grid()->setZeroLinePen( QPen( QBrush( QColor( 96, 96, 96 ) ), 2, Qt::DashLine ) );
		graph->yAxis->setRange( -1.0f, 1.0f );
		graph->addGraph();
		graph->addGraph();
		graph->addGraph();
		graph->graph(0)->setPen( QPen( QBrush( QColor( 255, 128, 128 ) ), 2 ) );
		graph->graph(1)->setPen( QPen( QBrush( QColor( 128, 255, 128 ) ), 2 ) );
		graph->graph(2)->setPen( QPen( QBrush( QColor( 128, 128, 255 ) ), 2 ) );
	}

	mTicks.start();

	ui->video->setMainWindow( this );
	ui->video->setLink( mStreamLink );
	// ui->video->setAudioLink( mAudioLink );
}


MainWindow::~MainWindow()
{
	delete ui;
	delete mConfig;
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
	std::cout << "lua_type( L, index ) = " << lua_type( L, index ) << "\n";
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


void MainWindow::appendDebugOutput( const QString& str )
{
	QTextCursor cursor( ui->terminal->textCursor() );
	ui->terminal->setPlainText( ui->terminal->toPlainText() + str );
	cursor.movePosition( QTextCursor::End );
	ui->terminal->setTextCursor( cursor );
}


void MainWindow::connected()
{
	qDebug() << "Fetching board data";
	ui->statusbar->showMessage( "Connected" );

	if ( mController and not mController->isSpectate() ) {
		lua_State* L = luaL_newstate();

		QString board_infos = QString::fromStdString( mController->getBoardInfos() );
		std::cout << "Board infos: " << board_infos.toStdString() << "\n";
		luaL_dostring( L, ("Board=" + board_infos.toUtf8()).data() );
		Recurse( L, ui->system->findItems( "BCFlight", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first(), "Board" );

		QString sensors_infos = QString::fromStdString( mController->getSensorsInfos() );
		std::cout << "sensors_infos : " << sensors_infos.toStdString() << "\n";
		luaL_dostring( L, ("Sensors=" + sensors_infos.toUtf8()).data() );
		Recurse( L, ui->system->findItems( "BCFlight", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first(), "Sensors" );

		QString cameras_infos = QString::fromStdString( mController->getCamerasInfos() );
		std::cout << "cameras_infos : " << cameras_infos.toStdString() << "\n";
		luaL_dostring( L, ("Cameras=" + cameras_infos.toUtf8()).data() );
		Recurse( L, ui->system->findItems( "BCFlight", Qt::MatchCaseSensitive | Qt::MatchRecursive, 0 ).first(), "Cameras" );

		lua_close(L);
	}

	ui->system->expandAll();
}


void MainWindow::updateData()
{
	if ( not mController or not mController->link() ) {
		if ( mStreamLink ) {
			ui->statusbar->showMessage( QString( "Camera : %1 KB/s    |    Quality : %2 %    |    %3 FPS" ).arg( mStreamLink->readSpeed() / 1024, 4, 10, QChar(' ') ).arg( mStreamLink->RxQuality(), 3, 10, QChar(' ') ).arg( ui->video->fps() ) );
		}
	} else {
		QString conn = mController->isConnected() ? "Connected" : "Disconnected";
		if ( mStreamLink ) {
			ui->statusbar->showMessage( conn + QString( "    |    RX Qual : %1 % (%2 dBm )    |    TX Qual : %3 %    |    TX : %4 B/s    |    RX : %5 B/s    |    Camera :%6 KB/s (%7 KB/s |%8 % |%9 dBm )    |    %10 FPS" ).arg( mController->link()->RxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->RxLevel(), 3, 10, QChar(' ') ).arg( mController->droneRxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->writeSpeed(), 4, 10, QChar(' ') ).arg( mController->link()->readSpeed(), 4, 10, QChar(' ') ).arg( mStreamLink->readSpeed() / 1024, 4, 10, QChar(' ') ).arg( mStreamLink->fullReadSpeed() / 1024, 4, 10, QChar(' ') ).arg( mStreamLink->RxQuality(), 3, 10, QChar(' ') ).arg( mStreamLink->RxLevel(), 3, 10, QChar(' ') ).arg( ui->video->fps() ) );
		} else {
			ui->statusbar->showMessage( conn + QString( "    |    RX Qual : %1 % (%2 dBm )    |    TX Qual : %3 %    |    TX : %4 B/s    |    RX : %5 B/s" ).arg( mController->link()->RxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->RxLevel(), 3, 10, QChar(' ') ).arg( mController->droneRxQuality(), 3, 10, QChar(' ') ).arg( mController->link()->writeSpeed(), 4, 10, QChar(' ') ).arg( mController->link()->readSpeed(), 4, 10, QChar(' ') ) );
		}

		if ( mController->ping() < 10000 ) {
			ui->latency->setText( QString::number( mController->ping() / 1000 ) + " ms" );
		}
		ui->voltage->setText( QString::number( mController->batteryVoltage(), 'f', 2 ) + " V" );
		ui->current->setText( QString::number( mController->currentDraw(), 'f', 2 ) + " A" );
		ui->current_total->setText( QString::number( mController->totalCurrent() ) + " mAh" );
		ui->cpu_load->setValue( mController->CPULoad() );
		ui->temperature->setValue( mController->CPUTemp() );
		ui->stabilizer_frequency->setText( QString::number( mController->stabilizerFrequency() ) + " Hz" );
		std::vector<float> motorSpeed = mController->motorsSpeed();
		if (motorSpeed.size() > 0) {
			if ( motorSpeedLayout == nullptr ) {
				motorSpeedLayout = new QVBoxLayout;
				for ( float speed : motorSpeed ) {
					QProgressBar *progress = new QProgressBar();
					motorSpeedProgress.append(progress);
					progress->setMaximum(100);
					progress->setMinimum(0);
					progress->setValue(speed);
					motorSpeedLayout->addWidget(progress);
				}
				ui->listMotors->setLayout(motorSpeedLayout);
			} else {
				for ( uint32_t i = 0; i < motorSpeed.size(); i++ ) {
					if ( i < motorSpeedProgress.size() and motorSpeedProgress.at(i) != nullptr ) {
						motorSpeedProgress.at(i)->setValue( motorSpeed.at(i) * 100 );
					}
				}
			}
		}

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

		auto plot = []( QCustomPlot* graph, const std::list< vec4 >& values, QVector< double >& t, QVector< double >& x, QVector< double >& y, QVector< double >& z, bool rescale = true ) {
			t.clear();
			x.clear();
			y.clear();
			z.clear();
			for ( vec4 v : values ) {
				if ( std::isnan( v.x ) or std::isinf( v.x ) or fabsf( v.x ) > 1000.0f ) {
					v.x = 0.0f;
				}
				if ( std::isnan( v.y ) or std::isinf( v.y ) or fabsf( v.y ) > 1000.0f ) {
					v.y = 0.0f;
				}
				if ( std::isnan( v.z ) or std::isinf( v.z ) or fabsf( v.z ) > 1000.0f ) {
					v.z = 0.0f;
				}
				t.append( v.w );
				x.append( v.x );
				y.append( v.y );
				z.append( v.z );
			}
			graph->graph(0)->setData( t, x );
			graph->graph(1)->setData( t, y );
			graph->graph(2)->setData( t, z );
			if ( rescale ) {
				graph->graph(0)->rescaleAxes();
				graph->graph(1)->rescaleAxes( true );
				graph->graph(2)->rescaleAxes( true );
			}
			graph->xAxis->rescale();
			graph->replot();
		};
		const std::list<vec4>& gyroData = mDnfPlot ? mController->dnfDftHistory() : (mRatesPlot ? mController->ratesHistory() : mController->gyroscopeHistory());
		if ( mRatesPlotSpectrum && !mDnfPlot ) {
			int numSamples = std::min( gyroData.size(), 512UL );
			if ( numSamples > 32 ) {
				float* input0 = (float*)fftwf_malloc( sizeof(float) * numSamples );
				float* input1 = (float*)fftwf_malloc( sizeof(float) * numSamples );
				float* input2 = (float*)fftwf_malloc( sizeof(float) * numSamples );
				fftwf_complex* output0 = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex) * ( numSamples / 2 + 1 ) );
				fftwf_complex* output1 = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex) * ( numSamples / 2 + 1 ) );
				fftwf_complex* output2 = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex) * ( numSamples / 2 + 1 ) );
				auto plan0 = fftwf_plan_dft_r2c_1d( numSamples, input0, output0, FFTW_ESTIMATE );
				auto plan1 = fftwf_plan_dft_r2c_1d( numSamples, input1, output1, FFTW_ESTIMATE );
				auto plan2 = fftwf_plan_dft_r2c_1d( numSamples, input2, output2, FFTW_ESTIMATE );
				int32_t n = numSamples - 1;
				double tMax = gyroData.back().w;
				double tMin = 0.0;
				std::vector<vec4> list;
				for ( vec4 v : gyroData ) {
					list.push_back(v);
				}
				for ( int32_t n = 0; n < numSamples; n++ ) {
					input0[n] = list[list.size() - 1 - n].x;
					input1[n] = list[list.size() - 1 - n].y;
					input2[n] = list[list.size() - 1 - n].z;
					tMin = list[list.size() - 1 - n].w;
				}
				float totalTime = tMax - tMin;
				// printf( "totalTime: %f\n", totalTime );
				fftwf_execute( plan0 );
				fftwf_execute( plan1 );
				fftwf_execute( plan2 );
				std::list<vec4> out;
				for ( uint32_t i = 0; i < numSamples / 2 + 1; i++ ) {
					vec4 v;
					v.x = std::abs( output0[i][0] );
					v.y = std::abs( output1[i][0] );
					v.z = std::abs( output2[i][0] );
					v.w = i;
					// v.w = 2 * i * list.size() / numSamples; // TODO : use IMU freq instead
					out.push_back( v );
				}
				ui->rates->yAxis->setRange( 0.0f, 128.0f );
				plot( ui->rates, out, mDataTrates, mDataRatesX, mDataRatesY, mDataRatesZ, false );
				fftwf_free( input0 );
				fftwf_free( input1 );
				fftwf_free( input2 );
				fftwf_free( output0 );
				fftwf_free( output1 );
				fftwf_free( output2 );
			}
		} else {
			if ( mDnfPlot ) {
				ui->rates->yAxis->setRange( 0.0f, 128.0f );
			}
			plot( ui->rates, gyroData, mDataTrates, mDataRatesX, mDataRatesY, mDataRatesZ, mDnfPlot == false );
		}
		plot( ui->rpy, mController->rpyHistory(), mDataTrpy, mDataR, mDataP, mDataY );
		plot( ui->accelerometer, mController->accelerationHistory(), mDataTaccelerometer, mDataAccelerometerX, mDataAccelerometerY, mDataAccelerometerZ );
		plot( ui->magnetometer, mController->magnetometerHistory(), mDataTmagnetometer, mDataMagnetometerX, mDataMagnetometerY, mDataMagnetometerZ );
		plot( ui->rates_dterm, mController->ratesDerivativeHistory(), mDataTratesdterm, mDataRatesdtermX, mDataRatesdtermY, mDataRatesdtermZ );

		if ( mController->altitudeHistory().size() > 0 ) {
			mDataTAltitude.append( mController->altitudeHistory().back().y );
			mDataAltitude.append( mController->altitudeHistory().back().x );
			ui->altitude->graph(0)->setData( mDataTAltitude, mDataAltitude );
			ui->altitude->graph(0)->rescaleAxes();
			ui->altitude->xAxis->rescale();
			ui->altitude->replot();
		}

		if ( mController->isConnected() and /*not mController->isSpectate() and*/ ( not mPIDsOk or ( ui->rateP->value() == 0.0f and ui->rateI->value() == 0.0f and ui->rateD->value() == 0.0f and ui->horizonP->value() == 0.0f and ui->horizonI->value() == 0.0f and ui->horizonD->value() == 0.0f ) ) ) {
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
}


void MainWindow::ResetBattery()
{
	if ( mController and not mController->isSpectate() ) {
		mController->ResetBattery();
	}
}


void MainWindow::Calibrate()
{
	if ( mController and not mController->isSpectate() ) {
		mController->Calibrate();
	}
}


void MainWindow::CalibrateAll()
{
	if ( mController and not mController->isSpectate() ) {
		mController->CalibrateAll();
	}
}


void MainWindow::CalibrateESCs()
{
	if ( mController and not mController->isSpectate() ) {
		mController->CalibrateESCs();
	}
}


void MainWindow::ArmDisarm()
{
	if ( mController and not mController->isSpectate() ) {
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
}


void MainWindow::throttleChanged( int throttle )
{
	if ( mController and not mController->isSpectate() ) {
		mController->setControlThrust( (double)throttle / 100.0f );
	}
}


void MainWindow::LoadConfig()
{
	if ( mController and not mController->isSpectate() ) {
		std::string conf = mController->getConfigFile();
		ui->config->setText( QString::fromStdString( conf ) );
	}
}


void MainWindow::SaveConfig()
{
	if ( mController and not mController->isSpectate() and ui->config->text().length() > 0 ) {
		std::string conf = ui->config->text().toStdString();
		mController->setConfigFile( conf );
	}
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
	if ( mFirmwareUpdateThread ) {
		if ( mFirmwareUpdateThread->isRunning() ) {
			return;
		}
		delete mFirmwareUpdateThread;
	}
	mFirmwareUpdateThread = new FirmwareUpdateThread( this );
	connect( this, SIGNAL( firmwareUpdateProgress(int) ), this, SLOT( setFirmwareUpdateProgress(int) ) );
	connect( this, SIGNAL( debugOutput(QString) ), this, SLOT( appendDebugOutput(QString) ) );
	mFirmwareUpdateThread->start();
}


void MainWindow::setFirmwareUpdateProgress( int val )
{
	ui->firmware_progress->setValue( val );
}


bool MainWindow::RunFirmwareUpdate()
{
	if ( mController and not mController->isSpectate() ) {
		emit firmwareUpdateProgress( 0 );

		if ( ui->firmware_path->text().length() > 0 and QFile::exists( ui->firmware_path->text() ) and mController->isConnected() ) {
			QFile f( ui->firmware_path->text() );
			if ( !f.open( QFile::ReadOnly ) ) return false;
			QByteArray ba = f.readAll();

			mController->UploadUpdateInit();
			uint32_t chunk_size = 1024; // 2048
			for ( uint32_t offset = 0; offset < (uint32_t)ba.size(); offset += chunk_size ) {
				uint32_t sz = chunk_size;
				if ( ba.size() - offset < chunk_size ) {
					sz = ba.size() - offset;
				}
				mController->UploadUpdateData( (const uint8_t*)&ba.constData()[offset], offset, sz );
				emit firmwareUpdateProgress( offset * 100 / ba.size() + 1 );
				QApplication::processEvents();
			}

			emit debugOutput( "\n====> Applying firmware update and restarting service, please wait... <====\n" );

			mController->UploadUpdateProcess( (const uint8_t*)ba.constData(), ba.size() );
		}

		emit firmwareUpdateProgress( 0 );
	}

	return false;
}


void MainWindow::tunDevice()
{
	if ( ui->tundev->text().contains( "nable" ) ) {
		qDebug() << "Enabling tun dev...";
		ui->tundev->setText( "Disable Tun Device" );
		mController->EnableTunDevice();
	} else {
		qDebug() << "Disabling tun dev...";
		ui->tundev->setText( "Enable Tun Device" );
		mController->DisableTunDevice();
	}
}


void MainWindow::ModeRate()
{
	if ( mController and not mController->isSpectate() ) {
		mController->setModeSwitch( Controller::Rate );
	}
}


void MainWindow::ModeStabilized()
{
	if ( mController and not mController->isSpectate() ) {
		mController->setModeSwitch( Controller::Stabilize );
	}
}


void MainWindow::setRatePIDRoll( double v )
{
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( not mController/* or mController->isSpectate()*/ ) {
		return;
	}
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
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoBrightnessDecrease();
	}
}


void MainWindow::VideoBrightnessIncrease()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoBrightnessIncrease();
	}
}


void MainWindow::VideoContrastDecrease()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoContrastDecrease();
	}
}


void MainWindow::VideoContrastIncrease()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoContrastIncrease();
	}
}


void MainWindow::VideoSaturationDecrease()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoSaturationDecrease();
	}
}


void MainWindow::VideoSaturationIncrease()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoSaturationIncrease();
	}
}


void MainWindow::SetNightMode( int mode )
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->setNight( ( mode != 0 ) );
	}
}


void MainWindow::VideoPause()
{
	/*
	if ( not mController or mController->isSpectate() ) {
		return;
	}
	qDebug() << "VideoPause()" << ui->video_pause->text();
	if ( ui->video_pause->text().mid( ui->video_pause->text().indexOf( "P" ) ) == QString( "Pause" ) ) {
		qDebug() << "VideoPause() Pause";
		mController->VideoPause();
		ui->video_pause->setText( "Resume" );
	} else {
		qDebug() << "VideoPause() Resume";
		mController->VideoResume();
		ui->video_pause->setText( "Pause" );
	}
	*/
}


void MainWindow::VideoTakePicture()
{
	if ( mController and mController->isConnected() ) {
		mController->VideoTakePicture();
	}
}


void MainWindow::VideoWhiteBalanceLock()
{
	if ( mController and mController->isConnected() and not mController->isSpectate() ) {
		mController->VideoLockWhiteBalance();
	}
}


void MainWindow::VideoRecord()
{
	if ( not mController or mController->isSpectate() ) {
		return;
	}
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
	if ( not mController or mController->isSpectate() ) {
		return;
	}
	ui->recordings->clearContents();
	ui->recordings->clear();
	for ( int32_t i = ui->recordings->rowCount() - 1; i >= 0; i-- ) {
		ui->recordings->removeRow( i );
	}

	std::vector< std::string > list = mController->recordingsList();
	std::sort( list.begin(), list.end() );
	for ( std::string input : list ) {
		QStringList split = QString::fromStdString(input).split( ":" );
		QString filename = split[0];
		QString size = split[1];
		int snap_width = split[2].toInt();
		int snap_height = split[3].toInt();
		int snap_bpp = split[4].toInt();
		QByteArray snap_raw = ( split.size() > 5 and split[5].length() > 0 ) ? QByteArray::fromBase64( split[5].toUtf8() ) : QByteArray();
		uint32_t* snap_raw32 = ( snap_raw.size() > 0 ) ? (uint32_t*)snap_raw.constData() : nullptr;
		if ( filename[0] != '.' ) {
			QLabel* snapshot_label = new QLabel();
			if ( snap_raw32 ) {
				QImage snapshot( snap_width, snap_height, QImage::Format_RGB16 );
				for ( int y = 0; y < snap_height; y++ ) {
					for ( int x = 0; x < snap_width; x++ ) {
						snapshot.setPixel( x, y, snap_raw32[ y * snap_width + x ] );
					}
				}
				snapshot_label->setPixmap( QPixmap::fromImage( snapshot ) );
			}
			ui->recordings->insertRow( ui->recordings->rowCount() );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 0, snapshot_label );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 1, new QLabel( filename ) );
// 			QLabel* size_label = new QLabel( QString::number( size.toInt() / 1024 ) + " KB" );
			QLabel* size_label = new QLabel( QLocale::system().toString( size.toInt() / 1024 ) + " KB" );
			size_label->setAlignment( Qt::AlignRight );
			ui->recordings->setCellWidget( ui->recordings->rowCount() - 1, 2, size_label );

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

	QStringList headers;
	headers << "Snapshot" << "Filename" << "Size" << "Date" << "Actions";
	ui->recordings->setHorizontalHeaderLabels( headers );
}


void MainWindow::MotorTest() {
	if ( not mController or mController->isSpectate() ) {
		return;
	}
	int id = ui->motorTestSpinBox->value();
	mController->MotorTest( id );
}


void MainWindow::MotorsBeep( bool enabled ) {
	std::cout << "MotorsBeep(" << enabled << " )\n";
	if ( not mController or mController->isSpectate() ) {
		return;
	}
	mController->MotorsBeep( enabled );
}
