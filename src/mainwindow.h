#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QModelIndex>
#include <QtCore/QUrl>

#include <QtGui/QMainWindow>
#include <QtGui/QSystemTrayIcon>

#include <auto_ptr.h>

namespace Ui
{

class MainWindow;
}

class QNetworkAccessManager;
class QSystemTrayIcon;
class QTableWidgetItem;

namespace Gui
{

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow (QWidget *parent = 0);
	~MainWindow();

protected:
	void changeEvent (QEvent *e);
	void timerEvent (QTimerEvent *);

private Q_SLOTS:
	void read ();
	void openUrl (QTableWidgetItem *item);
	void ignoreThread ();
	void ignoreCurrentThread ();
	void closeTab (int index);
	void trayClicked (QSystemTrayIcon::ActivationReason reason);
	void messageClicked ();

private:
	MainWindow (const MainWindow &);
	MainWindow &operator= (const MainWindow &);

	void loadSettings ();
	void saveSettings ();
	void openUrl (const QUrl &url);
	void showAndActivate ();
	void showHide ();

private:
	Ui::MainWindow *ui_;

	QNetworkAccessManager *manager_;
	QSystemTrayIcon *tray_;

	QString topThread_;
	QUrl topLink_;
	QStringList ignoredThreads_;
};
}
#endif // MAINWINDOW_H
