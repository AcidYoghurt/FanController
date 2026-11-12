#include "Constants.h"
#include "client/BackendClient.h"
#include "platform/SingleInstanceController.h"
#include "platform/TrayController.h"

#ifdef LINUX_THERMAL_GUI_TESTS
#include "GuiSmokeChecks.h"
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStyle>
#include <QWindow>

int main(int argc, char *argv[]) {
  qunsetenv("DESKTOP_STARTUP_ID");
  qunsetenv("XDG_ACTIVATION_TOKEN");
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("linux-thermal-control"));
  QApplication::setApplicationDisplayName(QStringLiteral("Linux 热管理"));
  QApplication::setOrganizationName(QStringLiteral("Linux Thermal Control"));
  QApplication::setDesktopFileName(QStringLiteral("org.linuxthermal.Control"));
  QApplication::setApplicationVersion(
      QString::fromLatin1(LinuxThermal::Constants::Version));

  auto applicationIcon =
      QIcon::fromTheme(QStringLiteral("org.linuxthermal.Control"));
  if (applicationIcon.isNull())
    applicationIcon = app.style()->standardIcon(QStyle::SP_ComputerIcon);
  QApplication::setWindowIcon(applicationIcon);

  const auto preferredFont = QStringLiteral("Noto Sans CJK SC");
  if (QFontDatabase::families().contains(preferredFont))
    app.setFont(QFont(preferredFont));

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
#ifdef LINUX_THERMAL_GUI_TESTS
  GuiSmokeChecks::addOptions(parser);
#endif
  parser.process(app);
#ifdef LINUX_THERMAL_GUI_TESTS
  GuiSmokeChecks::prepareEnvironment(parser);
#endif

  SingleInstanceController singleInstance;
  if (!singleInstance.isPrimary())
    return singleInstance.activatePrimary() ? 0 : 1;

  QQuickStyle::setStyle(QStringLiteral("Basic"));

  const bool forceTrayForTests =
      qEnvironmentVariable("LINUX_THERMAL_FORCE_TRAY_FOR_TESTS") ==
      QStringLiteral("1");
  TrayController trayController(&app, applicationIcon, forceTrayForTests);
  QApplication::setQuitOnLastWindowClosed(!trayController.trayAvailable());

  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.loadFromModule(QStringLiteral("LinuxThermal.Control"),
                        QStringLiteral("Main"));

  if (engine.rootObjects().isEmpty())
    return -1;
  auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
  if (!window)
    return -1;
  trayController.setWindow(window);
  auto *backend = engine.singletonInstance<BackendClient *>(
      QStringLiteral("LinuxThermal.Backend"), QStringLiteral("FanBackend"));
  trayController.setQuitPreparation(
      [backend] { return backend && backend->restoreAutomaticForExit(); });
  QObject::connect(&singleInstance,
                   &SingleInstanceController::activationRequested,
                   &trayController, &TrayController::showWindow);
  if (singleInstance.takePendingActivation())
    trayController.showWindow();

#ifdef LINUX_THERMAL_GUI_TESTS
  if (const int result =
          GuiSmokeChecks::install(parser, app, window, trayController,
                                  singleInstance, forceTrayForTests);
      result != 0)
    return result;
#endif

  return app.exec();
}
