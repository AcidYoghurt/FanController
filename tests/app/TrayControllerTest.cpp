#include "platform/TrayController.h"

#include <QApplication>
#include <QCloseEvent>
#include <QTest>
#include <QWindow>

class TrayControllerTest final : public QObject {
  Q_OBJECT

private slots:
  void closeHidesWindowAndExplicitActionsRestoreIt() {
    auto *application = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(application);
    QApplication::setQuitOnLastWindowClosed(false);

    QWindow window;
    window.resize(640, 480);
    window.show();
    QTRY_VERIFY(window.isVisible());

    TrayController controller(application, QIcon(), true);
    controller.setWindow(&window);
    QVERIFY(controller.trayAvailable());

    QCloseEvent closeEvent;
    QCoreApplication::sendEvent(&window, &closeEvent);
    QVERIFY(!closeEvent.isAccepted());
    QVERIFY(!window.isVisible());
    QVERIFY(!controller.quitRequested());

    controller.showWindow();
    QTRY_VERIFY(window.isVisible());
    controller.hideWindow();
    QVERIFY(!window.isVisible());
  }

  void failedQuitPreparationKeepsApplicationRunning() {
    auto *application =
        qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(application);
    TrayController controller(application, QIcon(), true);
    int preparationCalls = 0;
    controller.setQuitPreparation([&preparationCalls] {
      ++preparationCalls;
      return false;
    });

    controller.quitApplication();
    QCOMPARE(preparationCalls, 1);
    QVERIFY(!controller.quitRequested());

    controller.quitApplication();
    QCOMPARE(preparationCalls, 2);
    QVERIFY(!controller.quitRequested());
  }
};

QTEST_MAIN(TrayControllerTest)
#include "TrayControllerTest.moc"
