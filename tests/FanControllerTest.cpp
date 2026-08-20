#include "../src/daemon/FanController.h"
#include "../src/daemon/backends/ClevoBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class FanControllerTest final : public QObject {
  Q_OBJECT

private slots:
  void modeParsing() {
    QCOMPARE(FanController::modeFromString(u"auto"), FanController::Mode::Automatic);
    QCOMPARE(FanController::modeFromString(u"FULL-SPEED"),
             FanController::Mode::FullSpeed);
    QCOMPARE(FanController::modeFromString(u"custom-curve"),
             FanController::Mode::CustomCurve);
    QCOMPARE(FanController::modeFromString(u"quiet"),
             FanController::Mode::Unknown);
  }

  void modeNames() {
    QCOMPARE(FanController::modeToString(FanController::Mode::Automatic),
             QStringLiteral("automatic"));
    QCOMPARE(FanController::modeToString(FanController::Mode::FullSpeed),
             QStringLiteral("full-speed"));
    QCOMPARE(FanController::modeToString(FanController::Mode::CustomCurve),
             QStringLiteral("custom-curve"));
  }

  void clevoAcpiResponseStopsAtFirstNull() {
    QByteArray response("0x69\0alled", 10);
    QCOMPARE(ClevoBackend::normalizeAcpiResponse(response),
             QByteArray("0x69"));
    QCOMPARE(ClevoBackend::normalizeAcpiResponse(QByteArray(" 0x68\n")),
             QByteArray("0x68"));
    QCOMPARE(ClevoBackend::normalizeAcpiResponse(
                 QByteArray("Error: AE_BAD_PARAMETER\0garbage", 31)),
             QByteArray("Error: AE_BAD_PARAMETER"));
  }

  void detectsSecureBootEfiVariable() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto variable = root.filePath(QStringLiteral(
        "firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c"));
    QVERIFY(QDir().mkpath(QFileInfo(variable).absolutePath()));
    QFile file(variable);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArray::fromHex("0600000001")), 5);
    file.close();

    auto status = FanController::detectSecureBoot(root.path());
    QCOMPARE(status.value(QStringLiteral("state")).toString(),
             QStringLiteral("enabled"));
    QVERIFY(status.value(QStringLiteral("enabled")).toBool());
    QVERIFY(!status.value(QStringLiteral("disabled")).toBool());

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(QByteArray::fromHex("0600000000")), 5);
    file.close();
    status = FanController::detectSecureBoot(root.path());
    QCOMPARE(status.value(QStringLiteral("state")).toString(),
             QStringLiteral("disabled"));
    QVERIFY(status.value(QStringLiteral("disabled")).toBool());
  }

  void reportsNonEfiBootSeparately() {
    QTemporaryDir root;
    const auto status = FanController::detectSecureBoot(root.path());
    QCOMPARE(status.value(QStringLiteral("state")).toString(),
             QStringLiteral("unsupported"));
    QVERIFY(!status.value(QStringLiteral("enabled")).toBool());
    QVERIFY(!status.value(QStringLiteral("disabled")).toBool());
  }

  void defaultCurveIsSafe() {
    QString error;
    QVERIFY2(
        FanController::validateCurve(FanController::defaultCurve(), &error),
        qPrintable(error));
  }

  void rejectsUnsafeCurves() {
    auto lowDuty = FanController::defaultCurve();
    auto first = lowDuty.first().toMap();
    first.insert(QStringLiteral("duty"), 20);
    lowDuty[0] = first;
    QVERIFY(!FanController::validateCurve(lowDuty));

    auto decreasing = FanController::defaultCurve();
    auto second = decreasing.at(1).toMap();
    second.insert(QStringLiteral("duty"), 39);
    decreasing[1] = second;
    QVERIFY(!FanController::validateCurve(decreasing));

    auto noEmergency = FanController::defaultCurve();
    auto last = noEmergency.last().toMap();
    last.insert(QStringLiteral("duty"), 95);
    noEmergency[4] = last;
    QVERIFY(!FanController::validateCurve(noEmergency));
  }

  void sleepResumeReappliesSelectedMode() {
    FanController controller(true);
    QString error;
    QVERIFY2(controller.initialize(&error), qPrintable(error));
    QVERIFY2(controller.setMode(FanController::Mode::FullSpeed, &error),
             qPrintable(error));
    controller.prepareForSleep(true);
    QCOMPARE(controller.mode(), FanController::Mode::FullSpeed);
    controller.prepareForSleep(false);
    QCOMPARE(controller.mode(), FanController::Mode::FullSpeed);
  }

  void persistsCurveAcrossRestart() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("curve.json"));

    auto savedCurve = FanController::defaultCurve();
    auto second = savedCurve.at(1).toMap();
    second.insert(QStringLiteral("duty"), 55);
    savedCurve[1] = second;

    {
      FanController controller(true, path);
      QString error;
      QVERIFY2(controller.initialize(&error), qPrintable(error));
      QVERIFY2(controller.setCurve(savedCurve, &error), qPrintable(error));
    }

    {
      FanController controller(true, path);
      QString error;
      QVERIFY2(controller.initialize(&error), qPrintable(error));
      QCOMPARE(controller.curve(), savedCurve);
    }
  }

  void interpolatesCurve() {
    const auto curve = FanController::defaultCurve();
    QCOMPARE(FanController::dutyForTemperature(curve, 35.0), 40);
    QCOMPARE(FanController::dutyForTemperature(curve, 45.0), 40);
    QCOMPARE(FanController::dutyForTemperature(curve, 50.0), 46);
    QCOMPARE(FanController::dutyForTemperature(curve, 70.0), 73);
    QCOMPARE(FanController::dutyForTemperature(curve, 85.0), 100);
    QCOMPARE(FanController::dutyForTemperature(curve, 95.0), 100);
  }
};

QTEST_GUILESS_MAIN(FanControllerTest)
#include "FanControllerTest.moc"
