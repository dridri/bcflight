#include <unistd.h>
#include <QtCore/QtPlugin>
#include "GlobalUI.h"
#include "MainWindow.h"

Q_IMPORT_PLUGIN( QLinuxFbIntegrationPlugin );

GlobalUI::GlobalUI()
	: Thread( "GlobalUI" )
	, mApplication( nullptr )
{
}


GlobalUI::~GlobalUI()
{
}


bool GlobalUI::run()
{
	if ( mApplication == nullptr ) {
		int ac = 3;
		const char* av[4] = { "controller", "-platform", "linuxfb:fb=/dev/fb1", nullptr };
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

		MainWindow* test = new MainWindow();
		test->setGeometry( 0, 0, 480, 320 );
		test->setMinimumSize( 480, 320 );
		test->setMaximumSize( 480, 320 );
		test->show();
	}

	mApplication->processEvents();
	usleep( 1000 * 1000 / 10 );
	return true;
}
