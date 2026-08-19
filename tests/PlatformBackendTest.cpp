#include "../src/daemon/backends/ThermalZoneBackend.h"
#include "../src/daemon/backends/ThinkPadBackend.h"
#include "../src/daemon/backends/IdeaPadBackend.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace {
void writeFile(const QString &path, const QByteArray &value) {
  QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
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

class PlatformBackendTest final : public QObject {
  Q_OBJECT

private slots:
  void thermalZoneFallbackClassifiesCpuAndGpu() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto cpu = root.filePath(QStringLiteral("class/thermal/thermal_zone0"));
    const auto gpu = root.filePath(QStringLiteral("class/thermal/thermal_zone1"));
    writeFile(cpu + QStringLiteral("/type"), "x86_pkg_temp\n");
    writeFile(cpu + QStringLiteral("/temp"), "62000\n");
    writeFile(gpu + QStringLiteral("/type"), "gpu-thermal\n");
    writeFile(gpu + QStringLiteral("/temp"), "51000\n");

    ThermalZoneBackend backend(
        {Thermal::DeviceKind::Cpu, Thermal::DeviceKind::Gpu}, root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.sensors().size(), 2);
    QCOMPARE(backend.sensors().at(0).kind, Thermal::DeviceKind::Cpu);
    QCOMPARE(backend.sensors().at(1).kind, Thermal::DeviceKind::Gpu);
    QVERIFY(backend.zones().isEmpty());
  }

  void ideaPadFirmwareIsDetectedButNotWritten() {
    QTemporaryDir root;
    const auto base = root.filePath(
        QStringLiteral("bus/acpi/devices/VPC2004:00/physical_node"));
    writeFile(base + QStringLiteral("/fan_mode"), "133\n");
    writeFile(root.filePath(QStringLiteral("firmware/acpi/platform_profile")),
              "performance\n");
    writeFile(root.filePath(
                  QStringLiteral("firmware/acpi/platform_profile_choices")),
              "low-power balanced performance\n");

    IdeaPadBackend backend(root.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.fans().size(), 1);
    QVERIFY(!backend.fans().constFirst().controllable);
    QVERIFY(backend.zones().isEmpty());
    QCOMPARE(backend.info().status, QStringLiteral("read-only"));
    QVERIFY(backend.info().detail.contains(QStringLiteral("fan_mode=133")));
  }

  void thinkPadBackendUsesDocumentedLevels() {
    QTemporaryDir procRoot;
    QTemporaryDir sysRoot;
    QVERIFY(procRoot.isValid());
    QVERIFY(sysRoot.isValid());
    const auto fanPath = procRoot.filePath(QStringLiteral("acpi/ibm/fan"));
    writeFile(fanPath,
              "status:\tenabled\n"
              "speed:\t2450\n"
              "level:\tauto\n"
              "commands:\tlevel <level> (<level> is 0-7, auto, disengaged, full-speed)\n");
    writeFile(sysRoot.filePath(
                  QStringLiteral("module/thinkpad_acpi/parameters/fan_control")),
              "Y\n");

    ThinkPadBackend backend(procRoot.path(), sysRoot.path());
    QVERIFY(backend.probe());
    QCOMPARE(backend.fans().size(), 1);
    QCOMPARE(backend.fans().constFirst().rpm, 2450);
    QCOMPARE(backend.zones().size(), 1);
    const auto zoneId = backend.zones().constFirst().id;

    QVERIFY(backend.setFullSpeed(zoneId).success);
    QCOMPARE(readFile(fanPath), QByteArray("level full-speed"));
    QVERIFY(backend.setDuty(zoneId, 50).success);
    QCOMPARE(readFile(fanPath), QByteArray("level 4"));
    QVERIFY(backend.setAutomatic(zoneId).success);
    QCOMPARE(readFile(fanPath), QByteArray("level auto"));
  }

  void thinkPadWithoutKernelOptInIsReadOnly() {
    QTemporaryDir procRoot;
    QTemporaryDir sysRoot;
    const auto fanPath = procRoot.filePath(QStringLiteral("acpi/ibm/fan"));
    writeFile(fanPath,
              "status:\tenabled\nspeed:\t1800\nlevel:\tauto\n"
              "commands:\tlevel <level> (<level> is 0-7, auto, full-speed)\n");
    writeFile(sysRoot.filePath(
                  QStringLiteral("module/thinkpad_acpi/parameters/fan_control")),
              "N\n");
    ThinkPadBackend backend(procRoot.path(), sysRoot.path());
    QVERIFY(backend.probe());
    QVERIFY(!backend.fans().constFirst().controllable);
    QVERIFY(backend.zones().isEmpty());
  }
};

QTEST_GUILESS_MAIN(PlatformBackendTest)
#include "PlatformBackendTest.moc"
