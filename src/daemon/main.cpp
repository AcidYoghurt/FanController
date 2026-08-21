#include "Constants.h"
#include "control/FanController.h"
#include "service/FanService.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QSocketNotifier>

#include <cerrno>
#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace {
int signalPipe[2]{-1, -1};

void forwardUnixSignal(int signalNumber) {
  const int savedErrno = errno;
  const auto ignored =
      ::write(signalPipe[0], &signalNumber, sizeof(signalNumber));
  Q_UNUSED(ignored)
  errno = savedErrno;
}

bool installUnixSignalHandlers() {
  if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0,
                   signalPipe) != 0)
    return false;

  struct sigaction action {};
  action.sa_handler = forwardUnixSignal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;
  return sigaction(SIGINT, &action, nullptr) == 0 &&
         sigaction(SIGTERM, &action, nullptr) == 0;
}
} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("linux-thermal-control-daemon"));
  QCoreApplication::setApplicationVersion(
      QString::fromLatin1(LinuxThermal::Constants::Version));

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("Capability-aware Linux thermal control daemon"));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(
      {QStringLiteral("mock"),
       QStringLiteral("Run the development-only simulated hardware backend.")});
  parser.addOption(
      {QStringLiteral("set-mode"),
       QStringLiteral("Set one mode and exit: automatic or full-speed."),
       QStringLiteral("mode")});
  parser.process(app);

  const bool mockMode = parser.isSet(QStringLiteral("mock"));
  if (mockMode &&
      qEnvironmentVariable("LINUX_THERMAL_ALLOW_MOCK") != QStringLiteral("1")) {
    qCritical() << "Mock mode requires LINUX_THERMAL_ALLOW_MOCK=1";
    return 2;
  }

  FanController controller(mockMode);
  QString error;
  if (!controller.initialize(&error)) {
    qCritical().noquote() << error;
    return 1;
  }

  if (parser.isSet(QStringLiteral("set-mode"))) {
    const auto mode =
        FanController::modeFromString(parser.value(QStringLiteral("set-mode")));
    if (mode == FanController::Mode::Unknown ||
        mode == FanController::Mode::CustomCurve) {
      qCritical()
          << "Invalid one-shot mode; custom-curve requires the resident daemon";
      return 2;
    }
    if (!controller.setMode(mode, &error)) {
      qCritical().noquote() << error;
      return 1;
    }
    return 0;
  }

  auto bus = QDBusConnection::systemBus();
  if (!bus.isConnected()) {
    qCritical() << "Cannot connect to the system D-Bus";
    return 1;
  }

  const auto sleepSignalConnected =
      bus.connect(QStringLiteral("org.freedesktop.login1"),
                  QStringLiteral("/org/freedesktop/login1"),
                  QStringLiteral("org.freedesktop.login1.Manager"),
                  QStringLiteral("PrepareForSleep"), &controller,
                  SLOT(prepareForSleep(bool)));
  if (!sleepSignalConnected && !mockMode)
    qWarning() << "Cannot subscribe to logind PrepareForSleep";

  FanService service(&controller);
  if (!bus.registerObject(QString::fromLatin1(LinuxThermal::Constants::ObjectPath),
                          &service,
                          QDBusConnection::ExportAllSlots |
                              QDBusConnection::ExportAllSignals)) {
    qCritical().noquote() << bus.lastError().message();
    return 1;
  }
  if (!bus.registerService(
          QString::fromLatin1(LinuxThermal::Constants::ServiceName))) {
    qCritical().noquote() << bus.lastError().message();
    return 1;
  }

  if (!installUnixSignalHandlers()) {
    qCritical() << "Cannot install Unix signal handlers; errno=" << errno;
    return 1;
  }

  QSocketNotifier signalNotifier(signalPipe[1], QSocketNotifier::Read);
  QObject::connect(&signalNotifier, &QSocketNotifier::activated, &app, [&app] {
    int signalNumber = 0;
    const auto ignored =
        ::read(signalPipe[1], &signalNumber, sizeof(signalNumber));
    Q_UNUSED(ignored)
    app.quit();
  });

  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&controller] {
    if (controller.mode() == FanController::Mode::Automatic)
      return;
    QString restoreError;
    if (!controller.restoreAuto(&restoreError))
      qCritical().noquote()
          << "Failed to restore automatic fan control:" << restoreError;
  });

  controller.startTelemetry();
  const auto exitCode = app.exec();
  ::close(signalPipe[0]);
  ::close(signalPipe[1]);
  return exitCode;
}
