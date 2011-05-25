#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QMessageBox>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QtWebKit/QWebView>

#include "mainwindow.h"
#include "ui_mainwindow.h"

const qint64 updateInterval = 1000 * 10; //10 seconds

namespace Gui
{

MainWindow::MainWindow (QWidget *parent)
	: QMainWindow (parent), ui_ (new Ui::MainWindow),
	  manager_ (new QNetworkAccessManager (this)),
	  tray_ (new QSystemTrayIcon (this))
{
	ui_->setupUi (this);

	startTimer (updateInterval);

	QMenu *menu = new QMenu (this);
	menu->addAction (ui_->actionIgnoreCurrentThread);
	tray_->setContextMenu (menu);

	tray_->setIcon (style ()->standardIcon (QStyle::SP_DesktopIcon));
	tray_->show ();

	connect (ui_->tableWidget, SIGNAL (itemDoubleClicked (QTableWidgetItem *)),
			 this, SLOT (openUrl (QTableWidgetItem *)));

	connect (ui_->tabWidget, SIGNAL (tabCloseRequested (int)),
			 this, SLOT (closeTab (int)));

	connect (ui_->actionIgnoreThread, SIGNAL (triggered ()), this, SLOT (ignoreThread ()));
	connect (ui_->actionIgnoreCurrentThread, SIGNAL (triggered ()), this, SLOT (ignoreCurrentThread ()));

	connect (tray_, SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
			 this, SLOT (trayClicked (QSystemTrayIcon::ActivationReason)));
	connect (tray_, SIGNAL (messageClicked ()),
			 this, SLOT (messageClicked ()));

	loadSettings ();
	timerEvent (0);
}

MainWindow::~MainWindow()
{
	saveSettings();
	delete ui_;
}

void MainWindow::changeEvent (QEvent *e)
{
	QMainWindow::changeEvent (e);

	switch (e->type()) {

		case QEvent::LanguageChange:
			ui_->retranslateUi (this);
			break;

		default:
			break;
	}
}

void MainWindow::loadSettings ()
{
	QSettings settings;
	settings.beginGroup ("GUI");
	settings.beginGroup ("MainWindow");

	const QPoint pos = settings.value ("pos").toPoint();

	if (!pos.isNull()) {
		move (pos);
	}

	const QSize size = settings.value ("size", QSize (640, 480)).toSize();

	resize (size);

	const bool isMaximized = settings.value ("IsMaximized", false).toBool();

	if (isMaximized) {
		setWindowState (Qt::WindowMaximized);
	}

	ui_->splitter->restoreState (settings.value ("SplitterState").toByteArray ());

	settings.endGroup();

	settings.endGroup();

	ignoredThreads_ = settings.value ("GLOBAL/IgnoredThreads").toStringList ();
}

void MainWindow::saveSettings ()
{
	QSettings settings;
	settings.beginGroup ("GUI");
	settings.beginGroup ("MainWindow");

	if (windowState() != Qt::WindowMaximized) {
		settings.setValue ("pos", pos());
		settings.setValue ("size", size());
		settings.setValue ("IsMaximized", false);
	} else {
		settings.setValue ("IsMaximized", true);
	}

	settings.setValue ("SplitterState", ui_->splitter->saveState ());

	settings.endGroup();

	settings.endGroup();
	settings.setValue ("GLOBAL/IgnoredThreads", ignoredThreads_);
}

void MainWindow::timerEvent (QTimerEvent *)
{
	QNetworkReply *reply = manager_->get (QNetworkRequest (QUrl ("http://www.linux.org.ru/tracker.jsp?filter=all")));

	connect (reply, SIGNAL (finished ()), this, SLOT (read ()));
}

void MainWindow::read ()
{
	QNetworkReply *reply = qobject_cast <QNetworkReply *> (sender ());

	if (!reply) {
		return;
	}

	const QByteArray data = reply->readAll ();
	reply->deleteLater ();

	QStringList list = QString::fromUtf8 (data).split ("\n");

	{
		QStringList::iterator it = qFind (list.begin (),
										  list.end (),
										  QLatin1String ("<div class=forum>"));
		if (it != list.end ()) {
			list.erase (list.begin (), it);
		}

		it = qFind (list.begin (),
					list.end (),
					QLatin1String ("</div>"));
		if (it != list.end ()) {
			list.erase (it, list.end ());
		}
	}

	list.replaceInStrings (QRegExp ("^ *"), "");
	list.replaceInStrings (QRegExp (" *$"), "");

	//parse

	QStringList::const_iterator it = qFind (list.constBegin (), list.constEnd (), "<tbody>");

	int row = 0;
	QTableWidgetItem *item = 0;
	static const QRegExp linkRegExp ("<a href=\"(.*)\">");

	while ( (it = qFind (it + 1, list.constEnd (), "<tr>")) != list.constEnd ()) {
		if (row >= ui_->tableWidget->rowCount ()) {
			ui_->tableWidget->insertRow (ui_->tableWidget->rowCount ());
		}

		QStringList::const_iterator begin = qFind (it, list.constEnd (), "<td>");
		Q_ASSERT (begin != list.constEnd ());
		QStringList::const_iterator end = qFind (begin, list.constEnd (), "</td>");
		Q_ASSERT (end != list.constEnd ());

		QStringList l;
		for (QStringList::const_iterator tmp = begin + 1; tmp != end; ++tmp) {
			l.append (*tmp);
		}
		l.replaceInStrings (QRegExp ("<.*>"), "");
		const QString group = l.join (" ").simplified ();
		item = new QTableWidgetItem (group);
		ui_->tableWidget->setItem (row, 0, item);
		l.clear ();


		begin = qFind (end, list.constEnd (), "<td>");
		Q_ASSERT (begin != list.constEnd ());
		end = qFind (begin, list.constEnd (), "</td>");
		Q_ASSERT (end != list.constEnd ());

		QString link;
		for (QStringList::const_iterator tmp = begin + 1; tmp != end; ++tmp) {
			l.append (*tmp);
			if (linkRegExp.exactMatch (*tmp)) {
				link = linkRegExp.cap (1);
			}
		}
		l.replaceInStrings (QRegExp ("<.*>"), "");
		const QString topic = l.join (" ").simplified ();
		item = new QTableWidgetItem (topic);
		item->setData (Qt::ToolTipRole, "http://www.linux.org.ru" + link);
		ui_->tableWidget->setItem (row, 1, item);
		l.clear ();

		item = new QTableWidgetItem (ignoredThreads_.contains (topic)
									 ? "Ignored" : "");
		ui_->tableWidget->setItem (row, 2, item);

		++row;
	}

	if (ui_->tableWidget->rowCount () > 0) {
		QTableWidgetItem *item = ui_->tableWidget->item (0, 1);

		const QString thread = item ? item->text () : QString ();

		if (!ignoredThreads_.contains (thread) && topThread_ != thread) {
			tray_->showMessage ("Top thread",
								thread,
								QSystemTrayIcon::Information,
								1000 * 5);
		}
		topThread_ = thread;
		topLink_ = ui_->tableWidget->item (0, 1)->data (Qt::ToolTipRole).toString ();
		tray_->setToolTip (topThread_);

		ui_->actionIgnoreCurrentThread->setText ("Ignore thread\n" + topThread_);
	}

	ui_->tableWidget->setCurrentIndex (QModelIndex ());
}

void MainWindow::openUrl (QTableWidgetItem *item)
{
	QTableWidgetItem *item_ = ui_->tableWidget->item (item->row (), 1);
	if (!item_) {
		return;
	}
	const QString url = item_->data (Qt::ToolTipRole).toString ();

	openUrl (url);
}

void MainWindow::ignoreThread ()
{
	QTableWidgetItem *item = ui_->tableWidget->item (ui_->tableWidget->currentRow (), 1);
	if (!item) {
		return;
	}
	const QString thread = item->text ();

	if (!ignoredThreads_.contains (thread)) {
		ignoredThreads_.push_back (thread);
	} else {
		ignoredThreads_.removeAll (thread);
	}
}

void MainWindow::ignoreCurrentThread ()
{
	if (!topThread_.isEmpty ()) {
		ignoredThreads_.push_back (topThread_);
	}
}

void MainWindow::openUrl (const QUrl &url)
{
	const QString url_ = url.toString (QUrl::RemoveQuery);

	for (int i = 0, count = ui_->tabWidget->count (); i < count; ++i) {
		QWebView *v = qobject_cast <QWebView *> (ui_->tabWidget->widget (i));

		Q_ASSERT (v);

		const QString webUrl = v->url ().toString (QUrl::RemoveQuery);

		if (webUrl == url_) {
			v->reload ();
			ui_->tabWidget->setCurrentIndex (i);
			return;
		}
	}

	QWebView *v = new QWebView (ui_->tabWidget);
	v->setUrl (url);

	ui_->tabWidget->addTab (v, url.toString ());
	ui_->tabWidget->setCurrentWidget (v);
}

void MainWindow::closeTab (int index)
{
	delete ui_->tabWidget->widget (index);
}

void MainWindow::trayClicked (QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger) {
		showHide ();
	}
}

void MainWindow::showAndActivate ()
{
	setWindowState (windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
	show ();
	activateWindow();
}

void MainWindow::showHide ()
{
	if (isHidden()) {
		showAndActivate ();
	} else {
		hide ();
	}
}

void MainWindow::messageClicked ()
{
	openUrl (topLink_);

	showAndActivate ();
}

}
