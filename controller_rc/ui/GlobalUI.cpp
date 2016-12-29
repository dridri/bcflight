#include <unistd.h>
#include <linux/input.h>
#include <QtCore/QtPlugin>
#include "GlobalUI.h"
#include "MainWindow.h"
#include "Controller.h"

#ifndef BOARD_generic
Q_IMPORT_PLUGIN( QLinuxFbIntegrationPlugin );
#endif

GlobalUI::GlobalUI( Controller* controller )
	: Thread( "GlobalUI" )
	, mApplication( nullptr )
	, mController( controller )
{
}


GlobalUI::~GlobalUI()
{
}


bool GlobalUI::run()
{
	if ( mApplication == nullptr ) {
		putenv( (char*)"QT_QPA_FB_TSLIB=1" );
		putenv( (char*)"QT_LOGGING_RULES=qt.qpa.input=true" );

		int ac = 3;
		const char* av[4] = { "controller", "-platform", "linuxfb:fb=/dev/fb1", nullptr };
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
		palette.setColor( QPalette::Highlight, QColor( 142, 0, 0 ).lighter( ) );
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

	mMainWindow->Update();
	mApplication->processEvents();
#ifndef BOARD_generic
	usleep( 1000 * 1000 / 10 );
#endif
	return true;
}
