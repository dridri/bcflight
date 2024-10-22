#include <iostream>
#include <unistd.h>
#include <linux/input.h>
#include <QtCore/QtPlugin>
#include "GlobalUI.h"
#include "MainWindow.h"
#include "Controller.h"
#include "Config.h"

#ifndef BOARD_generic
// Q_IMPORT_PLUGIN( QLinuxFbIntegrationPlugin );
#endif

GlobalUI::GlobalUI( Config* config, Controller* controller )
	: Thread( "GlobalUI" )
	, mApplication( nullptr )
	, mConfig( config )
	, mController( controller )
{
}


GlobalUI::~GlobalUI()
{
}


bool GlobalUI::run()
{
	if ( mApplication == nullptr ) {
		std::string fbdev = "linuxfb:fb=" + mConfig->String( "touchscreen.framebuffer", "/dev/fb0" );
		std::string rotate = "QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/event0:rotate=" + std::to_string( mConfig->Integer( "touchscreen.rotate", 0 ) );

		std::cout << fbdev << "\n";
		std::cout << rotate << "\n";

		putenv( (char*)rotate.c_str() );
		putenv( (char*)"QT_QPA_FB_DISABLE_INPUT=0" );
		putenv( (char*)"QT_QPA_FB_NO_LIBINPUT=0" );
		putenv( (char*)"QT_QPA_FB_USE_LIBINPUT=0" );
		putenv( (char*)"QT_LOGGING_RULES=qt.qpa.input=true" );

		int ac = 3;
		const char* av[4] = { "controller", "-platform", fbdev.c_str(), nullptr };
#ifdef BOARD_generic
		ac = 1;
#endif
		mApplication = new QApplication( ac, (char**)av );

		QPalette palette;
		palette.setColor( QPalette::Window, QColor( 49, 54, 59 ) );
		palette.setColor( QPalette::WindowText, Qt::white );
		palette.setColor( QPalette::Base, QColor( 35, 38, 41 ) );
		palette.setColor( QPalette::AlternateBase, QColor( 35, 38, 41 ) );
		palette.setColor( QPalette::ToolTipBase, Qt::white );
		palette.setColor( QPalette::ToolTipText, Qt::white );
		palette.setColor( QPalette::Text, Qt::white );
		palette.setColor( QPalette::Button, QColor( 31, 36, 41 ) );
		palette.setColor( QPalette::ButtonText, Qt::white );
		palette.setColor( QPalette::BrightText, Qt::blue );
		palette.setColor( QPalette::Highlight, QColor( 142, 0, 0 ) );
		palette.setColor( QPalette::HighlightedText, Qt::black );
		palette.setColor( QPalette::Disabled, QPalette::Text, Qt::darkGray );
		palette.setColor( QPalette::Disabled, QPalette::ButtonText, Qt::darkGray );
		mApplication->setPalette( palette );

		mMainWindow = new MainWindow( mController );
		mMainWindow->setGeometry( 0, 0, 480, 320 );
		mMainWindow->setMinimumSize( 480, 320 );
		mMainWindow->setMaximumSize( 480, 320 );
		mMainWindow->show();
	}

	printf( "UI running\n" );
	return mApplication->exec();

	mMainWindow->Update();
	mApplication->processEvents();
#ifndef BOARD_generic
	usleep( 1000 * 1000 / 10 );
#endif
	return true;
}
