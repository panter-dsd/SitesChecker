#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>

#include <QtGui/QApplication>

#include "mainwindow.h"

const QString version = "0.0.0.0";

int main (int argc, char **argv)
{
	QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("UTF-8"));

	QApplication app (argc, argv);
	QCoreApplication::addLibraryPath (app.applicationDirPath () + "/plugins/");

	app.setOrganizationDomain ("panter.dsd");
	app.setOrganizationName ("PanteR");
	app.setApplicationVersion (version);
	app.setApplicationName ("SitesChecker");

	app.setQuitOnLastWindowClosed (true);

	Gui::MainWindow win;
	win.show ();

	return app.exec();
}
