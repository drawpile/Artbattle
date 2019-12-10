/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2006-2019 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "config.h"

#include "main.h"
#include "mainwindow.h"

#include "utils/icon.h"
#include "utils/logging.h"
#include "utils/colorscheme.h"
#include "notifications.h"
#include "dialogs/versioncheckdialog.h"
#include "../libshared/util/paths.h"

#ifdef Q_OS_MAC
#include "widgets/macmenu.h"
#endif

#ifdef KIS_TABLET

#if defined(Q_OS_WIN)
#include "bundled/kis_tablet/kis_tablet_support_win8.h"
#include "bundled/kis_tablet/kis_tablet_support_win.h"
#else
#include "bundled/kis_tablet/kis_xi2_event_filter.h"
#include <QX11Info>
#endif

#endif

#include <QCommandLineParser>
#include <QSettings>
#include <QUrl>
#include <QTabletEvent>
#include <QLibraryInfo>
#include <QTranslator>
#include <QDateTime>
#include <QStyle>

#include <ColorWheel>

DrawpileApp::DrawpileApp(int &argc, char **argv)
	: QApplication(argc, argv)
{
	setOrganizationName("drawpile");
	setOrganizationDomain("drawpile.net");
	setApplicationName("drawpile");
	setApplicationDisplayName("Drawpile Art Battle Edition");
	setApplicationVersion(DRAWPILE_VERSION);
	setWindowIcon(QIcon(":/icons/drawpile.png"));
}

/**
 * Handle tablet proximity events. When the eraser is brought near
 * the tablet surface, switch to eraser tool on all windows.
 * When the tip leaves the surface, switch back to whatever tool
 * we were using before.
 *
 * Also, on MacOS we must also handle the Open File event.
 */
bool DrawpileApp::event(QEvent *e) {
	if(e->type() == QEvent::TabletEnterProximity || e->type() == QEvent::TabletLeaveProximity) {
		QTabletEvent *te = static_cast<QTabletEvent*>(e);
		if(te->pointerType()==QTabletEvent::Eraser)
			emit eraserNear(e->type() == QEvent::TabletEnterProximity);
		return true;

	} else if(e->type() == QEvent::FileOpen) {
		QFileOpenEvent *fe = static_cast<QFileOpenEvent*>(e);

		// Note. This is currently broken in Qt 5.3.1:
		// https://bugreports.qt-project.org/browse/QTBUG-39972
		openUrl(fe->url());

		return true;

	}
#ifdef Q_OS_MACOS
	else if(e->type() == QEvent::ApplicationStateChange) {
		QApplicationStateChangeEvent *ae = static_cast<QApplicationStateChangeEvent*>(e);
		if(ae->applicationState() == Qt::ApplicationActive && topLevelWindows().isEmpty()) {
			// Open a new window when application is activated and there are no windows.
			openBlankDocument();
		}
	}
#endif

	return QApplication::event(e);
}

void DrawpileApp::notifySettingsChanged()
{
	emit settingsChanged();
}

void DrawpileApp::setDarkTheme(bool dark)
{
	QPalette pal;
	if(dark) {
		const QString paletteFile = utils::paths::locateDataFile("nightmode.colors");
		if(paletteFile.isEmpty()) {
			qWarning("Cannot switch to night mode: couldn't find color scheme file!");
		} else {
			pal = colorscheme::loadFromFile(paletteFile);
#ifdef Q_OS_WIN
			// The Vista style does not work with customized palettes very well
			setStyle("fusion");
#endif
		}

	} else {
		pal = style()->standardPalette();
	}

	setPalette(pal);
	icon::selectThemeVariant();
}

void DrawpileApp::openUrl(QUrl url)
{
	// See if there is an existing replacable window
	MainWindow *win = nullptr;
	for(QWidget *widget : topLevelWidgets()) {
		MainWindow *mw = qobject_cast<MainWindow*>(widget);
		if(mw && mw->canReplace()) {
			win = mw;
			break;
		}
	}

	// No? Create a new one
	if(!win)
		win = new MainWindow;

	if(url.scheme() == "drawpile") {
		// Our own protocol: connect to a session
		win->join(url);

	} else {
		// Other protocols: load image
		win->open(url);
	}
}

void DrawpileApp::openBlankDocument()
{
	// Open a new window with a blank image
	QSettings cfg;

	const QSize maxSize {65536, 65536};
	QSize size = cfg.value("history/newsize").toSize();

	if(size.width()<100 || size.height()<100) {
		// No previous size, or really small size
		size = QSize(800, 600);
	} else {
		// Make sure previous size is not ridiculously huge
		size = size.boundedTo(maxSize);
	}

	QColor color = cfg.value("history/newcolor").value<QColor>();
	if(!color.isValid())
		color = Qt::white;

	MainWindow *win = new MainWindow;
	win->newDocument(size, color);
}

static void initTranslations(const QLocale &locale)
{
	const auto preferredLangs = locale.uiLanguages();
	if(preferredLangs.size()==0)
		return;

	// TODO we should work our way down the preferred language list
	// until we find one a translation exists for.
	QString preferredLang = preferredLangs.at(0);

	// On Windows, the locale name is sometimes in the form "fi-FI"
	// rather than "fi_FI" that Qt expects.
	preferredLang.replace('-', '_');

	// Special case: if english is preferred language, no translations are needed.
	if(preferredLang == "en")
		return;

	// Qt's own translations
	QTranslator *qtTranslator = new QTranslator;
	qtTranslator->load("qt_" + preferredLang, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	qApp->installTranslator(qtTranslator);

	// Our translations
	QTranslator *myTranslator = new QTranslator;

	for(const QString &datapath : utils::paths::dataPaths()) {
		if(myTranslator->load("drawpile_" + preferredLang,  datapath + "/i18n"))
			break;
	}

	if(myTranslator->isEmpty())
		delete myTranslator;
	else
		qApp->installTranslator(myTranslator);
}

// Initialize the application and return a list of files to be opened (if any)
static QStringList initApp(DrawpileApp &app)
{
	// Parse command line arguments
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();

	// --data-dir
	QCommandLineOption dataDir("data-dir", "Override read-only data directory.", "path");
	parser.addOption(dataDir);

	// --portable-data-dir
	QCommandLineOption portableDataDir("portable-data-dir", "Override settings directory.", "path");
	parser.addOption(portableDataDir);

	// URL
	parser.addPositionalArgument("url", "Filename or URL.");

	parser.process(app);

	// Override data directories
	if(parser.isSet(dataDir))
		utils::paths::setDataPath(parser.value(dataDir));

	if(parser.isSet(portableDataDir)) {
		utils::paths::setWritablePath(parser.value(portableDataDir));

		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(
			QSettings::IniFormat,
			QSettings::UserScope,
			utils::paths::writablePath(QStandardPaths::AppConfigLocation, QString())
		);
	}

	// Continue initialization (can use QSettings from now on)
	utils::initLogging();

	if(QSettings().value("settings/nightmode").toBool())
		app.setDarkTheme(true);
	else
		icon::selectThemeVariant();


#ifdef Q_OS_MAC
	// Mac specific settings
	app.setAttribute(Qt::AA_DontShowIconsInMenus);
	app.setQuitOnLastWindowClosed(false);

	// Global menu bar that is shown when no windows are open
	MacMenu::instance();
#endif

#ifdef KIS_TABLET
#ifdef Q_OS_WIN
	{
		bool useWindowsInk = false;
		// Enable Windows Ink tablet event handler
		// This was taken directly from Krita
		if(QSettings().value("settings/input/windowsink", true).toBool()) {
			KisTabletSupportWin8 *penFilter = new KisTabletSupportWin8();
			if (penFilter->init()) {
				app.installNativeEventFilter(penFilter);
				useWindowsInk = true;
				qDebug("Using Win8 Pointer Input for tablet support");

			} else {
				qWarning("No Win8 Pointer Input available");
				delete penFilter;
			}
		} else {
			qDebug("Win8 Pointer Input disabled");
		}

		if(!useWindowsInk) {
			// Enable modified Wintab support
			// This too was taken from Krita
			qDebug("Enabling custom Wintab support");
			KisTabletSupportWin::init();
		}
	}
#else
	if(QX11Info::isPlatformX11()) {
		// Qt's X11 tablet event handling is broken in different ways
		// in pretty much every Qt5 version, so we use this Krita's
		// modified event filter
		qInfo("Enabling custom XInput2 tablet event filter");
		app.installNativeEventFilter(kis_tablet::KisXi2EventFilter::instance());
	}
#endif
#endif // KIS_TABLET

	qsrand(QDateTime::currentMSecsSinceEpoch());

	color_widgets::ColorWheel::setDefaultDisplayFlags(color_widgets::ColorWheel::SHAPE_SQUARE | color_widgets::ColorWheel::ANGLE_FIXED | color_widgets::ColorWheel::COLOR_HSV);

	// Set override locale from settings, or use system locale if no override is set
	QLocale locale = QLocale::c();
	QString overrideLang = QSettings().value("settings/language").toString();
	if(!overrideLang.isEmpty())
		locale = QLocale(overrideLang);

	if(locale == QLocale::c())
		locale = QLocale::system();

	initTranslations(locale);

	return parser.positionalArguments();
}

int main(int argc, char *argv[]) {
	// Set attributes that must be set before QApplication is constructed
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	// CanvasView does not work correctly with this enabled.
	// (Scale factor must be taken in account when zooming)
	//QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	DrawpileApp app(argc, argv);

	{
		const auto files = initApp(app);

		if(files.isEmpty()) {
			app.openBlankDocument();

		} else {
			QUrl url(files.at(0));
			if(url.scheme().length() <= 1) {
				// no scheme (or a drive letter?) means this is probably a local file
				url = QUrl::fromLocalFile(files.at(0));
			}

			app.openUrl(url);
		}
	}

	dialogs::VersionCheckDialog::doVersionCheckIfNeeded();

	return app.exec();
}
