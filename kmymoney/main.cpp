/***************************************************************************
                          main.cpp
                             -------------------
    copyright            : (C) 2001 by Michael Edwardes
    email                : mte@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <config-kmymoney.h>
#include <config-kmymoney-version.h>

// ----------------------------------------------------------------------------
// QT Includes

#include <QWidget>
#include <QDateTime>
#include <QStringList>
#include <QEventLoop>
#include <QApplication>
#include <QCommandLineParser>
#include <QResource>
#ifdef KMM_DBUS
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#endif
// ----------------------------------------------------------------------------
// KDE Includes

#include <KLocalizedString>
#include <ktip.h>
#include <KMessageBox>
#include <Kdelibs4ConfigMigrator>

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoney/mymoneyfile.h"
#include "kmymoney.h"
#include "kstartuplogo.h"
#include "kcreditswindow.h"
#include "kmymoneyutils.h"
#include "kmymoneyglobalsettings.h"


bool timersOn = false;

KMyMoneyApp* kmymoney;

static int runKMyMoney(QApplication *a, std::unique_ptr<QSplashScreen> splash, const QUrl & file, bool noFile);

int main(int argc, char *argv[])
{
  KLocalizedString::setApplicationDomain("kmymoney");

  {
    // Copy KDE 4 config files to the KF5 location
    Kdelibs4ConfigMigrator migrator(QStringLiteral("kmymoney"));
    migrator.setConfigFiles(QStringList{"kmymoneyrc"});
    migrator.setUiFiles(QStringList{"kmymoneyui.rc"});
    migrator.migrate();
  }

  /**
   * Create application first
   */
  QApplication app(argc, argv);

  /**
   * construct and register about data
   */
  KAboutData aboutData(QStringLiteral("kmymoney"), i18n("KMyMoney"), QStringLiteral(VERSION));
  aboutData.setOrganizationDomain("kde.org");
  KAboutData::setApplicationData(aboutData);

  QStringList fileUrls;
  bool isNoCatchOption = false;
  bool isNoFileOption = false;

#ifdef KMM_DEBUG
  bool isDumpActionsOption = false;
#endif

  if (argc != 0) {
    /**
   * Create command line parser and feed it with known options
   */
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    // language
    //    const QCommandLineOption langOption(QStringLiteral("lang"), i18n("language to be used"));
    //    parser.addOption(langOption);

    // no file
    const QCommandLineOption noFileOption(QStringLiteral("n"), i18n("do not open last used file"));
    parser.addOption(noFileOption);

    // timers
    const QCommandLineOption timersOption(QStringLiteral("timers"), i18n("enable performance timers"));
    parser.addOption(timersOption);

    // no catch
    const QCommandLineOption noCatchOption(QStringLiteral("nocatch"), i18n("do not globally catch uncaught exceptions"));
    parser.addOption(noCatchOption);

#ifdef KMM_DEBUG
    // The following options are only available when compiled in debug mode
    // trace
    const QCommandLineOption traceOption(QStringLiteral("trace"), i18n("turn on program traces"));
    parser.addOption(traceOption);

    // dump actions
    const QCommandLineOption dumpActionsOption(QStringLiteral("dump-actions"), i18n("dump the names of all defined QAction objects to stdout and quit"));
    parser.addOption(dumpActionsOption);
#endif

    // INSERT YOUR COMMANDLINE OPTIONS HERE
    // url to open
    parser.addPositionalArgument(QStringLiteral("url"), i18n("file to open"));

    /**
   * do the command line parsing
   */
    parser.parse(QApplication::arguments());

    bool ishelpSet = parser.isSet(QStringLiteral("help"));
    if (ishelpSet || parser.isSet(QStringLiteral("author")) || parser.isSet(QStringLiteral("license"))) {
      aboutData = initializeCreditsData();
      if (ishelpSet)
        parser.showHelp();
    }

    if (parser.isSet(QStringLiteral("version")))
      parser.showVersion();

    /**
   * handle standard options
   */
    aboutData.processCommandLine(&parser);

#ifdef KMM_DEBUG
    if (parser.isSet(traceOption))
      MyMoneyTracer::on();
    timersOn = parser.isSet(timersOption);
    isDumpActionsOption = parser.isSet(dumpActionsOption);
#endif

    isNoCatchOption = parser.isSet(noCatchOption);
    isNoFileOption = parser.isSet(noFileOption);
    fileUrls = parser.positionalArguments();
  }

  /**
   * enable high dpi icons
   */
  app.setAttribute(Qt::AA_UseHighDpiPixmaps);

  // create the singletons before we start memory checking
  // to avoid false error reports
  MyMoneyFile::instance();

  KMyMoneyUtils::checkConstants();

  // show startup logo
  std::unique_ptr<QSplashScreen> splash(KMyMoneyGlobalSettings::showSplash() ? createStartupLogo() : nullptr);
  app.processEvents();

  // setup the MyMoneyMoney locale settings according to the KDE settings
  MyMoneyMoney::setThousandSeparator(QLocale().groupSeparator());
  MyMoneyMoney::setDecimalSeparator(QLocale().decimalPoint());
  // TODO: port to kf5
  //MyMoneyMoney::setNegativeMonetarySignPosition(static_cast<MyMoneyMoney::signPosition>(KLocale::global()->negativeMonetarySignPosition()));
  //MyMoneyMoney::setPositiveMonetarySignPosition(static_cast<MyMoneyMoney::signPosition>(KLocale::global()->positiveMonetarySignPosition()));
  //MyMoneyMoney::setNegativePrefixCurrencySymbol(KLocale::global()->negativePrefixCurrencySymbol());
  //MyMoneyMoney::setPositivePrefixCurrencySymbol(KLocale::global()->positivePrefixCurrencySymbol());

//  QString language = parser.value(langOption);
//  if (!language.isEmpty()) {
    //if (!KLocale::global()->setLanguage(QStringList() << language)) {
    //  qWarning("Unable to select language '%s'. This has one of two reasons:\n\ta) the standard KDE message catalog is not installed\n\tb) the KMyMoney message catalog is not installed", qPrintable(language));
    //}
//  }

  kmymoney = new KMyMoneyApp();

#ifdef KMM_DEBUG
  if (isDumpActionsOption) {
    kmymoney->dumpActions();

    // Before we delete the application, we make sure that we destroy all
    // widgets by running the event loop for some time to catch all those
    // widgets that are requested to be destroyed using the deleteLater() method.
    //QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput, 10);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    delete kmymoney;
    exit(0);
  }
#endif

  const QUrl url = fileUrls.isEmpty() ? QUrl() : QUrl::fromUserInput(fileUrls.front());
  int rc = 0;
  if (isNoCatchOption) {
    qDebug("Running w/o global try/catch block");
    rc = runKMyMoney(&app, std::move(splash), url, isNoFileOption);
  } else {
    try {
      rc = runKMyMoney(&app, std::move(splash), url, isNoFileOption);
    } catch (const MyMoneyException &e) {
      KMessageBox::detailedError(0, i18n("Uncaught error. Please report the details to the developers"),
                                 i18n("%1 in file %2 line %3", e.what(), e.file(), e.line()));
      throw e;
    }
  }

  return rc;
}

int runKMyMoney(QApplication *a, std::unique_ptr<QSplashScreen> splash, const QUrl & file, bool noFile)
{
#ifdef KMM_DBUS
  if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kmymoney")) {
    const QList<QString> instances = kmymoney->instanceList();
    if (instances.count() > 0) {

      // If the user launches a second copy of the app and includes a file to
      // open, they are probably attempting a "WebConnect" session.  In this case,
      // we'll check to make sure it's an importable file that's passed in, and if so, we'll
      // notify the primary instance of the file and kill ourselves.

      if (file.isValid()) {
        if (kmymoney->isImportableFile(file)) {
          // if there are multiple instances, we'll send this to the first one
          QString primary = instances[0];

          // send a message to the primary client to import this file
          QDBusInterface remoteApp(primary, "/KMymoney", "org.kde.kmymoney");
          // TODO: port ot kf5
          //remoteApp.call("webConnect", file.path(), KStartupInfo::startupId());

          // Before we delete the application, we make sure that we destroy all
          // widgets by running the event loop for some time to catch all those
          // widgets that are requested to be destroyed using the deleteLater() method.
          QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

          delete kmymoney;
          return 0;
        }
      }

      if (KMessageBox::questionYesNo(0, i18n("Another instance of KMyMoney is already running. Do you want to quit?")) == KMessageBox::Yes) {
        delete kmymoney;
        return 1;
      }
    }
  } else {
    qDebug("D-Bus registration failed. Some functions are not available.");
  }
#else
  qDebug("D-Bus disabled. Some functions are not available.");
#endif
  kmymoney->show();
  kmymoney->centralWidget()->setEnabled(false);

  splash.reset();

  // force complete paint of widgets
  qApp->processEvents();

  QString importfile;
  QUrl url;
  // make sure, we take the file provided on the command
  // line before we go and open the last one used
  if (file.isValid()) {
    // Check to see if this is an importable file, as opposed to a loadable
    // file.  If it is importable, what we really want to do is load the
    // last used file anyway and then immediately import this file.  This
    // implements a "web connect" session where there is not already an
    // instance of the program running.

    if (kmymoney->isImportableFile(file)) {
      importfile = file.path();
      url = QUrl::fromUserInput(kmymoney->readLastUsedFile());
    }

  } else {
    url = QUrl::fromUserInput(kmymoney->readLastUsedFile());
  }

  KTipDialog::showTip(kmymoney, QString(), false);
  if (url.isValid() && !noFile) {
    kmymoney->slotFileOpenRecent(url);
  } else if (KMyMoneyGlobalSettings::firstTimeRun()) {
    kmymoney->slotFileNew();
  }
  KMyMoneyGlobalSettings::setFirstTimeRun(false);

  // TODO: port to kf5
  //if (!importfile.isEmpty())
  //  kmymoney->webConnect(importfile, KStartupInfo::startupId());

  kmymoney->updateCaption();
  kmymoney->centralWidget()->setEnabled(true);

  const int rc = a->exec();
  return rc;
}
