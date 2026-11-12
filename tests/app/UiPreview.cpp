#include "UiMotionCapture.h"
#include "client/BackendClient.h"

#include <QCommandLineParser>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setOrganizationName(QStringLiteral("LinuxThermalPreview"));
  QCoreApplication::setApplicationName(QStringLiteral("ui-preview"));
  QTemporaryDir config;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOption({QStringLiteral("output"),
                    QStringLiteral("Screenshot path."),
                    QStringLiteral("path")});
  parser.addOption(
      {QStringLiteral("dark"), QStringLiteral("Use dark appearance.")});
  parser.addOption(
      {QStringLiteral("info"), QStringLiteral("Capture device information.")});
  parser.addOption({QStringLiteral("appearance"),
                    QStringLiteral("Open appearance settings.")});
  parser.addOption({QStringLiteral("scroll-bottom"),
                    QStringLiteral("Scroll to curve actions.")});
  parser.addOption({QStringLiteral("width"), QStringLiteral("Window width."),
                    QStringLiteral("pixels"), QStringLiteral("1120")});
  parser.addOption({QStringLiteral("height"), QStringLiteral("Window height."),
                    QStringLiteral("pixels"), QStringLiteral("900")});
  parser.addOption({QStringLiteral("font-scale"),
                    QStringLiteral("System font multiplier."),
                    QStringLiteral("factor"), QStringLiteral("1")});
  parser.addOption({QStringLiteral("motion-output"),
                    QStringLiteral("Record mock UI animation frames."),
                    QStringLiteral("directory")});
  parser.process(app);
  if (!parser.isSet(QStringLiteral("output")) &&
      !parser.isSet(QStringLiteral("motion-output")))
    parser.showHelp(1);

  if (parser.isSet(QStringLiteral("motion-output")) &&
      (qEnvironmentVariable("LINUX_THERMAL_ALLOW_MOCK") !=
           QStringLiteral("1") ||
       qEnvironmentVariable("DBUS_SYSTEM_BUS_ADDRESS").isEmpty() ||
       qEnvironmentVariable("DBUS_SYSTEM_BUS_ADDRESS") !=
           qEnvironmentVariable("DBUS_SESSION_BUS_ADDRESS")))
    return 7;

  QFont font(QStringLiteral("Noto Sans CJK SC"));
  font.setPointSizeF(
      10 *
      qBound(1., parser.value(QStringLiteral("font-scale")).toDouble(), 2.));
  app.setFont(font);

  QQmlApplicationEngine engine;
  engine.loadFromModule(QStringLiteral("LinuxThermal.Control"),
                        QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty())
    return 2;
  auto *window =
      qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
  if (!window)
    return 3;
  auto *backend = engine.singletonInstance<BackendClient *>(
      QStringLiteral("LinuxThermal.Backend"), QStringLiteral("FanBackend"));
  auto *settings = engine.singletonInstance<QObject *>(
      QStringLiteral("LinuxThermal.Control"), QStringLiteral("AppSettings"));
  settings->setProperty("themeMode",
                        parser.isSet(QStringLiteral("dark")) ? 2 : 1);
  window->resize(
      qBound(760, parser.value(QStringLiteral("width")).toInt(), 4096),
      qBound(620, parser.value(QStringLiteral("height")).toInt(), 2160));

  // 等待真实客户端读取 mock 服务，避免截取尚未连接的空白状态。
  QTimer ready;
  QObject::connect(&ready, &QTimer::timeout, &app, [&] {
    if (!backend->connected())
      return;
    ready.stop();
    if (parser.isSet(QStringLiteral("motion-output"))) {
      captureMotionPreview(window,
                           parser.value(QStringLiteral("motion-output")));
      return;
    }
    if (parser.isSet(QStringLiteral("info")))
      window->setProperty("currentPage", 1);
    if (parser.isSet(QStringLiteral("appearance")))
      QMetaObject::invokeMethod(
          window->findChild<QObject *>(QStringLiteral("appearanceButton")),
          "click");
    QTimer::singleShot(800, window, [&] {
      if (parser.isSet(QStringLiteral("scroll-bottom"))) {
        auto *page =
            window->findChild<QObject *>(QStringLiteral("controlPage"));
        auto *flick =
            page ? qvariant_cast<QObject *>(page->property("contentItem"))
                 : nullptr;
        if (!flick) {
          app.exit(4);
          return;
        }
        flick->setProperty(
            "contentY", qMax(0., flick->property("contentHeight").toDouble() -
                                     flick->property("height").toDouble()));
      }
      QTimer::singleShot(800, window, [&] {
        app.exit(
            window->grabWindow().save(parser.value(QStringLiteral("output")))
                ? 0
                : 5);
      });
    });
  });
  ready.start(50);
  QTimer::singleShot(parser.isSet(QStringLiteral("motion-output")) ? 20000
                                                                   : 5000,
                     &app, [&] { app.exit(6); });
  return app.exec();
}
