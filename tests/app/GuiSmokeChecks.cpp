#include "GuiSmokeChecks.h"

#include "platform/SingleInstanceController.h"
#include "platform/TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariant>
#include <QWindow>

namespace GuiSmokeChecks {
void addOptions(QCommandLineParser &parser) {
  parser.addOption(
      {QStringLiteral("test-tray-lifecycle"),
       QStringLiteral("Run the automated tray lifecycle test and exit.")});
  parser.addOption(
      {QStringLiteral("test-page-navigation"),
       QStringLiteral("Verify Control/Info page navigation and exit.")});
  parser.addOption(
      {QStringLiteral("test-single-instance-primary"),
       QStringLiteral(
           "Wait for a second launch, verify activation and exit.")});
}

void prepareEnvironment(const QCommandLineParser &parser) {
  if (!parser.isSet(QStringLiteral("test-tray-lifecycle")) &&
      !parser.isSet(QStringLiteral("test-page-navigation")) &&
      !parser.isSet(QStringLiteral("test-single-instance-primary")))
    return;
  static QTemporaryDir config;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
}

int install(const QCommandLineParser &parser, QApplication &app,
            QWindow *window, TrayController &trayController,
            SingleInstanceController &singleInstance, bool forceTrayForTests) {
  if (parser.isSet(QStringLiteral("test-single-instance-primary"))) {
    QTimer::singleShot(150, &trayController, &TrayController::hideWindow);
    QObject::connect(&singleInstance,
                     &SingleInstanceController::activationRequested, &app,
                     [window] {
                       QTimer::singleShot(150, window, [window] {
                         QCoreApplication::exit(window->isVisible() ? 0 : 13);
                       });
                     });
    QTimer::singleShot(5000, &app, [] { QCoreApplication::exit(14); });
  }

  if (parser.isSet(QStringLiteral("test-tray-lifecycle"))) {
    if (!forceTrayForTests)
      return 2;
    QTimer::singleShot(150, window, &QWindow::close);
    QTimer::singleShot(350, &app, [window, &trayController] {
      if (window->isVisible()) {
        QCoreApplication::exit(3);
        return;
      }
      trayController.showWindow();
    });
    QTimer::singleShot(550, &app, [window, &trayController] {
      if (!window->isVisible()) {
        QCoreApplication::exit(4);
        return;
      }
      trayController.hideWindow();
    });
    QTimer::singleShot(750, &app, [window, &trayController] {
      if (window->isVisible()) {
        QCoreApplication::exit(5);
        return;
      }
      trayController.quitApplication();
    });
  }

  if (parser.isSet(QStringLiteral("test-page-navigation"))) {
    QTimer::singleShot(300, &app, [window] {
      auto *stack = window->findChild<QObject *>(QStringLiteral("pageStack"));
      auto *controlButton = window->findChild<QObject *>(
          QStringLiteral("controlNavigationButton"));
      auto *infoButton =
          window->findChild<QObject *>(QStringLiteral("infoNavigationButton"));
      auto *currentItem =
          stack ? qvariant_cast<QObject *>(stack->property("currentItem"))
                : nullptr;
      if (!stack || !currentItem || !controlButton || !infoButton ||
          window->property("currentPage").toInt() != 0 ||
          !controlButton->property("selected").toBool() ||
          infoButton->property("selected").toBool() ||
          stack->property("depth").toInt() != 1 ||
          currentItem->objectName() != QStringLiteral("controlPage")) {
        QCoreApplication::exit(6);
        return;
      }
      QMetaObject::invokeMethod(controlButton, "click");
      QMetaObject::invokeMethod(controlButton, "click");
      if (!controlButton->property("selected").toBool() ||
          window->property("currentPage").toInt() != 0) {
        QCoreApplication::exit(15);
        return;
      }
      window->setProperty("currentPage", 1);
    });
    QTimer::singleShot(380, &app, [window] {
      auto *controlPage =
          window->findChild<QObject *>(QStringLiteral("controlPage"));
      auto *infoPage = window->findChild<QObject *>(QStringLiteral("infoPage"));
      if (!controlPage || !infoPage ||
          (controlPage->property("opacity").toDouble() > 0.01 &&
           infoPage->property("opacity").toDouble() > 0.01)) {
        QCoreApplication::exit(9);
      }
    });
    QTimer::singleShot(520, &app, [window] {
      auto *controlPage =
          window->findChild<QObject *>(QStringLiteral("controlPage"));
      auto *infoPage = window->findChild<QObject *>(QStringLiteral("infoPage"));
      if (!controlPage || !infoPage ||
          (controlPage->property("opacity").toDouble() > 0.01 &&
           infoPage->property("opacity").toDouble() > 0.01)) {
        QCoreApplication::exit(10);
      }
    });
    QTimer::singleShot(850, &app, [window] {
      auto *stack = window->findChild<QObject *>(QStringLiteral("pageStack"));
      auto *currentItem =
          stack ? qvariant_cast<QObject *>(stack->property("currentItem"))
                : nullptr;
      auto *infoButton =
          window->findChild<QObject *>(QStringLiteral("infoNavigationButton"));
      if (!stack || !currentItem || !infoButton ||
          window->property("currentPage").toInt() != 1 ||
          !infoButton->property("selected").toBool() ||
          stack->property("depth").toInt() != 2 ||
          currentItem->objectName() != QStringLiteral("infoPage")) {
        QCoreApplication::exit(7);
        return;
      }
      QMetaObject::invokeMethod(infoButton, "click");
      QMetaObject::invokeMethod(infoButton, "click");
      if (!infoButton->property("selected").toBool() ||
          window->property("currentPage").toInt() != 1) {
        QCoreApplication::exit(16);
        return;
      }
      window->setProperty("currentPage", 0);
    });
    QTimer::singleShot(930, &app, [window] {
      auto *controlPage =
          window->findChild<QObject *>(QStringLiteral("controlPage"));
      auto *infoPage = window->findChild<QObject *>(QStringLiteral("infoPage"));
      if (!controlPage || !infoPage ||
          (controlPage->property("opacity").toDouble() > 0.01 &&
           infoPage->property("opacity").toDouble() > 0.01)) {
        QCoreApplication::exit(11);
      }
    });
    QTimer::singleShot(1070, &app, [window] {
      auto *controlPage =
          window->findChild<QObject *>(QStringLiteral("controlPage"));
      auto *infoPage = window->findChild<QObject *>(QStringLiteral("infoPage"));
      if (!controlPage || !infoPage ||
          (controlPage->property("opacity").toDouble() > 0.01 &&
           infoPage->property("opacity").toDouble() > 0.01)) {
        QCoreApplication::exit(12);
      }
    });
    QTimer::singleShot(1400, &app, [window] {
      auto *stack = window->findChild<QObject *>(QStringLiteral("pageStack"));
      auto *currentItem =
          stack ? qvariant_cast<QObject *>(stack->property("currentItem"))
                : nullptr;
      if (!stack || !currentItem ||
          window->property("currentPage").toInt() != 0 ||
          stack->property("depth").toInt() != 1 ||
          currentItem->objectName() != QStringLiteral("controlPage")) {
        QCoreApplication::exit(8);
        return;
      }
      QCoreApplication::quit();
    });
  }

  return 0;
}
} // namespace GuiSmokeChecks
