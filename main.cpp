#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

/**
 * The entry point of the application.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 */
int main( int argc, char* argv[] )
{
    QApplication a( argc, argv );
	MainWindow w;
	w.show();
	return a.exec();
}
