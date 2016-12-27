#include "MainWindow.h"
#include "ui_main.h"

MainWindow::MainWindow()
	: QMainWindow()
{
	ui = new Ui::MainWindow;
	ui->setupUi(this);
	ui->gridLayout_3->setSpacing(0);
	ui->gridLayout_3->setMargin(0);
}


MainWindow::~MainWindow()
{
	delete ui;
}
