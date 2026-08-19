#include "../src/daemon/backends/HwmonBackend.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace {
void writeFile(const QString &path, const QByteArray &value) {
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  QCOMPARE(file.write(value), value.size());
}
QByteArray readFile(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll().trimmed();
}
}

class HwmonBackendTest final : public QObject {
  Q_OBJECT

private slots:
  void classifiesCommonCpuAndGpuDrivers_data() {
    QTest::addColumn<QString>("driver");
    QTest::addColumn<int>("kind");
    QTest::newRow("intel-coretemp") << QStringLiteral("coretemp")
                                    << static_cast<int>(Thermal::DeviceKind::Cpu);
    QTest::newRow("amd-k10temp") << QStringLiteral("k10temp")
                                 << static_cast<int>(Thermal::DeviceKind::Cpu);
    QTest::newRow("amd-zenpower") << QStringLiteral("zenpower")
                                  << static_cast<int>(Thermal::DeviceKind::Cpu);
    QTest::newRow("amd-gpu") << QStringLiteral("amdgpu")
                             << static_cast<int>(Thermal::DeviceKind::Gpu);
    QTest::newRow("intel-i915") << QStringLiteral("i915")
                                << static_cast<int>(Thermal::DeviceKind::Gpu);
    QTest::newRow("intel-xe") << QStringLiteral("xe")
                              << static_cast<int>(Thermal::DeviceKind::Gpu);
    QTest::newRow("nouveau") << QStringLiteral("nouveau")
                             << static_cast<int>(Thermal::DeviceKind::Gpu);
  }

  void classifiesCommonCpuAndGpuDrivers() {
    QFETCH(QString, driver);
    QFETCH(int, kind);
    QTemporaryDir root;
    const auto path = root.filePath(QStringLiteral("class/hwmon/hwmon0"));
    QVERIFY(QDir().mkpath(path));
    writeFile(path + QStringLiteral("/name"), driver.toUtf8() + '\n');
    writeFile(path + QStringLiteral("/temp1_input"), "50000\n");
    HwmonBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(static_cast<int>(backend.sensors().constFirst().kind), kind);
  }

  void keepsMultipleControllersDistinct() {
    QTemporaryDir root;
    for (int index = 0; index < 2; ++index) {
      const auto path = root.filePath(
          QStringLiteral("class/hwmon/hwmon%1").arg(index));
      QVERIFY(QDir().mkpath(path));
      writeFile(path + QStringLiteral("/name"),
                index == 0 ? QByteArray("nct6798\n")
                           : QByteArray("dell_smm\n"));
      writeFile(path + QStringLiteral("/temp1_input"), "50000\n");
      writeFile(path + QStringLiteral("/fan1_input"), "1200\n");
      writeFile(path + QStringLiteral("/pwm1"), "100\n");
      writeFile(path + QStringLiteral("/pwm1_enable"), "2\n");
    }
    HwmonBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.zones().size(), 2);
    QVERIFY(backend.zones().at(0).id != backend.zones().at(1).id);
    QVERIFY(backend.fans().at(0).id != backend.fans().at(1).id);
  }

  void rejectsUnsafeManualOnlyChannel() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto path = root.filePath(QStringLiteral("class/hwmon/hwmon0"));
    QVERIFY(QDir().mkpath(path));
    writeFile(path + QStringLiteral("/name"), "vendor_ec\n");
    writeFile(path + QStringLiteral("/temp1_input"), "48000\n");
    writeFile(path + QStringLiteral("/fan1_input"), "1000\n");
    writeFile(path + QStringLiteral("/pwm1"), "100\n");
    writeFile(path + QStringLiteral("/pwm1_enable"), "1\n");

    HwmonBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.fans().size(), 1);
    QVERIFY(!backend.fans().constFirst().controllable);
    QVERIFY(backend.zones().isEmpty());
  }

  void controlsAppleSmcTargetRpmAndRestoresAutomatic() {
    QTemporaryDir root;
    const auto path = root.filePath(QStringLiteral("class/hwmon/hwmon0"));
    QVERIFY(QDir().mkpath(path));
    writeFile(path + QStringLiteral("/name"), "applesmc\n");
    writeFile(path + QStringLiteral("/temp1_input"), "52000\n");
    writeFile(path + QStringLiteral("/fan1_input"), "2200\n");
    writeFile(path + QStringLiteral("/fan1_min"), "2000\n");
    writeFile(path + QStringLiteral("/fan1_max"), "6000\n");
    writeFile(path + QStringLiteral("/fan1_manual"), "0\n");
    writeFile(path + QStringLiteral("/fan1_output"), "2800\n");

    HwmonBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.zones().size(), 1);
    QVERIFY(backend.fans().constFirst().controllable);
    QCOMPARE(backend.fans().constFirst().duty, 20);
    const auto zoneId = backend.zones().constFirst().id;
    QVERIFY(backend.setDuty(zoneId, 50).success);
    QCOMPARE(readFile(path + QStringLiteral("/fan1_manual")), QByteArray("1"));
    QCOMPARE(readFile(path + QStringLiteral("/fan1_output")), QByteArray("4000"));
    QVERIFY(backend.setFullSpeed(zoneId).success);
    QCOMPARE(readFile(path + QStringLiteral("/fan1_output")), QByteArray("6000"));
    QVERIFY(backend.setAutomatic(zoneId).success);
    QCOMPARE(readFile(path + QStringLiteral("/fan1_manual")), QByteArray("0"));
  }

  void discoversAndControlsStandardPwmChannel() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto path = root.filePath(QStringLiteral("class/hwmon/hwmon0"));
    QVERIFY(QDir().mkpath(path));
    writeFile(path + QStringLiteral("/name"), "amdgpu\n");
    writeFile(path + QStringLiteral("/temp1_input"), "55000\n");
    writeFile(path + QStringLiteral("/temp1_label"), "edge\n");
    writeFile(path + QStringLiteral("/fan1_input"), "1400\n");
    writeFile(path + QStringLiteral("/pwm1"), "128\n");
    writeFile(path + QStringLiteral("/pwm1_enable"), "2\n");

    HwmonBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.sensors().size(), 1);
    QCOMPARE(backend.fans().size(), 1);
    QCOMPARE(backend.zones().size(), 1);
    QCOMPARE(backend.sensors().constFirst().kind, Thermal::DeviceKind::Gpu);
    QVERIFY(backend.fans().constFirst().controllable);

    const auto zoneId = backend.zones().constFirst().id;
    QVERIFY(backend.setFullSpeed(zoneId).success);
    QCOMPARE(readFile(path + QStringLiteral("/pwm1_enable")), QByteArray("1"));
    QCOMPARE(readFile(path + QStringLiteral("/pwm1")), QByteArray("255"));
    QVERIFY(backend.setAutomatic(zoneId).success);
    QCOMPARE(readFile(path + QStringLiteral("/pwm1_enable")), QByteArray("2"));
  }
};

QTEST_GUILESS_MAIN(HwmonBackendTest)
#include "HwmonBackendTest.moc"
