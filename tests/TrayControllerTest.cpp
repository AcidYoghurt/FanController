#include "../src/app/TrayController.h"

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
};

QTEST_MAIN(TrayControllerTest)
#include "TrayControllerTest.moc"
