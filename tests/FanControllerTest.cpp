#include "../src/daemon/FanController.h"
#include "../src/daemon/FanCurveConfig.h"
#include "../src/daemon/backends/ClevoBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QtMath>

#include <algorithm>

namespace {
QVariantMap testCurvePoint(int temperature, int duty) {
  return {{QStringLiteral("temperature"), temperature},
          {QStringLiteral("duty"), duty}};
}
} // namespace

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

  void defaultCurveMatchesYamlReference() {
    const auto curve = FanCurveConfig::loadFile(
        QStringLiteral(LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH));
    QCOMPARE(curve.size(), 10);
    QCOMPARE(curve.at(0).toMap().value(QStringLiteral("temperature")).toInt(), 35);
    QCOMPARE(curve.at(0).toMap().value(QStringLiteral("duty")).toInt(), 0);
    QCOMPARE(curve.at(1).toMap().value(QStringLiteral("temperature")).toInt(), 40);
    QCOMPARE(curve.at(1).toMap().value(QStringLiteral("duty")).toInt(), 30);
    QCOMPARE(curve.at(9).toMap().value(QStringLiteral("temperature")).toInt(), 100);
    QCOMPARE(curve.at(9).toMap().value(QStringLiteral("duty")).toInt(), 100);
  }

  void createsAndKeepsDefaultCurveBackup() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto configPath = directory.filePath(QStringLiteral("default-fan-curve.yaml"));
    QFile source(QStringLiteral(LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH));
    QVERIFY(source.open(QIODevice::ReadOnly));
    const auto original = source.readAll();
    source.close();

    QFile config(configPath);
    QVERIFY(config.open(QIODevice::WriteOnly));
    QCOMPARE(config.write(original), original.size());
    config.close();

    qputenv("LINUX_THERMAL_DEFAULT_CURVE_CONFIG", configPath.toUtf8());
    const auto backupPath =
        directory.filePath(QStringLiteral(".风扇曲线配置.backup"));
    QVERIFY(!QFileInfo::exists(backupPath));
    QString error;
    const auto first = FanCurveConfig::loadDefault(&error);
    QVERIFY2(!first.isEmpty(), qPrintable(error));
    QVERIFY(QFileInfo::exists(backupPath));

    QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray edited(
        "curve:\n"
        "  points:\n"
        "    - temperature: 30\n"
        "      duty: 100\n");
    QCOMPARE(config.write(edited), edited.size());
    config.close();
    const auto second = FanCurveConfig::loadDefault(&error);
    QCOMPARE(second, first);
    qunsetenv("LINUX_THERMAL_DEFAULT_CURVE_CONFIG");
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
    noEmergency[noEmergency.size() - 1] = last;
    QVERIFY(!FanController::validateCurve(noEmergency));
  }

  void fullSpeedToCustomCurveAppliesTargetImmediately() {
    FanController controller(true);
    QString error;
    QVERIFY2(controller.initialize(&error), qPrintable(error));
    QVERIFY2(controller.setMode(FanController::Mode::FullSpeed, &error),
             qPrintable(error));

    auto status = controller.status();
    auto zones = status.value(QStringLiteral("zones")).toList();
    QVERIFY(!zones.isEmpty());
    QCOMPARE(zones.constFirst().toMap()
                 .value(QStringLiteral("currentDuty"))
                 .toInt(),
             100);

    QVERIFY2(controller.setMode(FanController::Mode::CustomCurve, &error),
             qPrintable(error));
    status = controller.status();
    zones = status.value(QStringLiteral("zones")).toList();
    const auto duty = zones.constFirst().toMap()
                          .value(QStringLiteral("currentDuty"))
                          .toInt();
    QVERIFY2(duty >= 0 && duty < 100,
             qPrintable(QStringLiteral("Expected immediate curve duty, got %1%")
                            .arg(duty)));
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
    auto middle = savedCurve.at(4).toMap();
    middle.insert(QStringLiteral("duty"), 55);
    savedCurve[4] = middle;

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
    QCOMPARE(FanController::dutyForTemperature(curve, 50.0), 45);
    QCOMPARE(FanController::dutyForTemperature(curve, 70.0), 70);
    QCOMPARE(FanController::dutyForTemperature(curve, 85.0), 85);
    QCOMPARE(FanController::dutyForTemperature(curve, 95.0), 95);
  }

  void interpolatesEveryDefaultCurveSegment() {
    const auto curve = FanController::defaultCurve();
    QVERIFY(curve.size() >= 2);

    for (qsizetype index = 1; index < curve.size(); ++index) {
      const auto lower = curve.at(index - 1).toMap();
      const auto upper = curve.at(index).toMap();
      const int lowerTemperature =
          lower.value(QStringLiteral("temperature")).toInt();
      const int upperTemperature =
          upper.value(QStringLiteral("temperature")).toInt();
      const int lowerDuty = lower.value(QStringLiteral("duty")).toInt();
      const int upperDuty = upper.value(QStringLiteral("duty")).toInt();
      const double sampleTemperature =
          (lowerTemperature + upperTemperature) / 2.0;
      const int expectedDuty = qBound(
          40, qRound(lowerDuty + 0.5 * (upperDuty - lowerDuty)), 100);

      QCOMPARE(FanController::dutyForTemperature(curve, sampleTemperature),
               expectedDuty);
    }
  }

  void customCurveRuntimeStressCoversEverySegment() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FanController controller(
        true, directory.filePath(QStringLiteral("stress-curves.json")));
    QString error;
    QVERIFY2(controller.initialize(&error), qPrintable(error));

    const QString zoneId = QStringLiteral("mock:cpu:zone");
    const QString sensorId = QStringLiteral("mock:cpu:package");
    const QString fanId = QStringLiteral("mock:cpu:fan0");
    const QVariantList curve{
        testCurvePoint(35, 30), testCurvePoint(40, 35),
        testCurvePoint(45, 40), testCurvePoint(50, 45),
        testCurvePoint(55, 50), testCurvePoint(60, 55),
        testCurvePoint(65, 60), testCurvePoint(70, 70),
        testCurvePoint(75, 85), testCurvePoint(85, 100),
    };
    QVERIFY2(controller.setCurveForZone(zoneId, curve, &error),
             qPrintable(error));

    constexpr int SweepCount = 32;
    for (int sweep = 0; sweep < SweepCount; ++sweep) {
      for (qsizetype index = 1; index < curve.size(); ++index) {
        const auto lower = curve.at(index - 1).toMap();
        const auto upper = curve.at(index).toMap();
        const int lowerTemperature =
            lower.value(QStringLiteral("temperature")).toInt();
        const int upperTemperature =
            upper.value(QStringLiteral("temperature")).toInt();
        const int lowerDuty = lower.value(QStringLiteral("duty")).toInt();
        const int upperDuty = upper.value(QStringLiteral("duty")).toInt();
        const double sampleTemperature =
            (lowerTemperature + upperTemperature) / 2.0;
        const int expectedDuty =
            qRound(lowerDuty + 0.5 * (upperDuty - lowerDuty));

        QVERIFY2(controller.setMode(FanController::Mode::Automatic, &error),
                 qPrintable(error));
        QVERIFY(controller.setMockSensorTemperature(sensorId,
                                                    sampleTemperature));
        QVERIFY2(controller.setMode(FanController::Mode::CustomCurve, &error),
                 qPrintable(error));

        const auto status = controller.status();
        const auto zones = status.value(QStringLiteral("zones")).toList();
        const auto fans = status.value(QStringLiteral("fans")).toList();
        const auto zone = std::find_if(
            zones.cbegin(), zones.cend(), [&zoneId](const auto &value) {
              return value.toMap().value(QStringLiteral("zoneId")).toString() ==
                     zoneId;
            });
        const auto fan = std::find_if(
            fans.cbegin(), fans.cend(), [&fanId](const auto &value) {
              return value.toMap().value(QStringLiteral("fanId")).toString() ==
                     fanId;
            });
        QVERIFY(zone != zones.cend());
        QVERIFY(fan != fans.cend());
        QCOMPARE(zone->toMap().value(QStringLiteral("currentDuty")).toInt(),
                 expectedDuty);
        QCOMPARE(fan->toMap().value(QStringLiteral("duty")).toInt(),
                 expectedDuty);
        QCOMPARE(fan->toMap().value(QStringLiteral("rpm")).toInt(),
                 700 + expectedDuty * 44);
      }
    }

    QVERIFY(controller.setMockSensorTemperature(sensorId, 85.0));
    const auto status = controller.status();
    QVERIFY(status.value(QStringLiteral("safetyOverride")).toBool());
    const auto zones = status.value(QStringLiteral("zones")).toList();
    const auto cpuZone = std::find_if(
        zones.cbegin(), zones.cend(), [&zoneId](const auto &value) {
          return value.toMap().value(QStringLiteral("zoneId")).toString() ==
                 zoneId;
        });
    QVERIFY(cpuZone != zones.cend());
    QCOMPARE(cpuZone->toMap().value(QStringLiteral("currentDuty")).toInt(),
             100);
  }
};

QTEST_GUILESS_MAIN(FanControllerTest)
#include "FanControllerTest.moc"
