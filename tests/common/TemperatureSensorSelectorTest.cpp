#include "TemperatureSensorSelector.h"

#include <QTest>

namespace {
QVariantMap sensor(const QString &id, const QString &name, const QString &kind,
                   double temperature, bool available = true,
                   const QString &backendId = QStringLiteral("hwmon"),
                   const QString &deviceId = {}) {
  return {{QStringLiteral("sensorId"), id},
          {QStringLiteral("backendId"), backendId},
          {QStringLiteral("deviceId"), deviceId},
          {QStringLiteral("displayName"), name},
          {QStringLiteral("deviceKind"), kind},
          {QStringLiteral("temperature"), temperature},
          {QStringLiteral("available"), available}};
}
} // namespace

class TemperatureSensorSelectorTest final : public QObject {
  Q_OBJECT

private slots:
  void selectsOneRepresentativeCpuAndGpu() {
    const QVariantList sensors{
        sensor(QStringLiteral("clevo-cpu"), QStringLiteral("CPU package"),
               QStringLiteral("cpu"), 72.0),
        sensor(QStringLiteral("package"), QStringLiteral("Package id 0"),
               QStringLiteral("cpu"), 68.0),
        sensor(QStringLiteral("core0"), QStringLiteral("Core 0"),
               QStringLiteral("cpu"), 74.0),
        sensor(QStringLiteral("core1"), QStringLiteral("Core 1"),
               QStringLiteral("cpu"), 76.0),
        sensor(QStringLiteral("gpu-edge"), QStringLiteral("edge"),
               QStringLiteral("gpu"), 61.0),
        sensor(QStringLiteral("system"), QStringLiteral("System"),
               QStringLiteral("system"), 45.0)};

    const auto selected = LinuxThermal::TemperatureSensorSelector::selectPrimary(sensors);

    QCOMPARE(selected.size(), 2);
    QCOMPARE(selected.at(0).toMap().value(QStringLiteral("sensorId")).toString(),
             QStringLiteral("package"));
    QCOMPARE(selected.at(1).toMap().value(QStringLiteral("sensorId")).toString(),
             QStringLiteral("gpu-edge"));
  }

  void prefersTctlAndNvidia() {
    const QVariantList sensors{
        sensor(QStringLiteral("cpu-package"), QStringLiteral("Package id 0"),
               QStringLiteral("cpu"), 75.0),
        sensor(QStringLiteral("tctl"), QStringLiteral("Tctl"),
               QStringLiteral("cpu"), 70.0),
        sensor(QStringLiteral("gpu-edge"), QStringLiteral("edge"),
               QStringLiteral("gpu"), 62.0),
        sensor(QStringLiteral("nvidia"), QStringLiteral("GeForce RTX 4060"),
               QStringLiteral("gpu"), 55.0, true, QStringLiteral("nvidia"),
               QStringLiteral("nvidia:gpu-0"))};

    const auto selected = LinuxThermal::TemperatureSensorSelector::selectPrimary(sensors);

    QCOMPARE(selected.at(0).toMap().value(QStringLiteral("sensorId")).toString(),
             QStringLiteral("tctl"));
    QCOMPARE(selected.at(1).toMap().value(QStringLiteral("sensorId")).toString(),
             QStringLiteral("nvidia"));
  }

  void fallsBackToHottestAvailableSensor() {
    const QVariantList sensors{
        sensor(QStringLiteral("cpu-unavailable"), QStringLiteral("Tctl"),
               QStringLiteral("cpu"), -1.0, false),
        sensor(QStringLiteral("cpu-cool"), QStringLiteral("Core 0"),
               QStringLiteral("cpu"), 66.0),
        sensor(QStringLiteral("cpu-hot"), QStringLiteral("Core 1"),
               QStringLiteral("cpu"), 73.0)};

    const auto selected = LinuxThermal::TemperatureSensorSelector::selectPrimary(sensors);

    QCOMPARE(selected.size(), 1);
    QCOMPARE(selected.constFirst().toMap().value(QStringLiteral("sensorId")).toString(),
             QStringLiteral("cpu-hot"));
  }
};

QTEST_GUILESS_MAIN(TemperatureSensorSelectorTest)
#include "TemperatureSensorSelectorTest.moc"
