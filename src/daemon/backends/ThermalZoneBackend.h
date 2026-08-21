#pragma once

#include "IFanBackend.h"

#include <QHash>
#include <QSet>

// 从 Linux thermal_zone 接口补充只读温度。
// `acceptedKinds` 用于避免与已有 hwmon CPU/GPU 传感器重复。
class ThermalZoneBackend final : public IFanBackend {
public:
  explicit ThermalZoneBackend(
      QSet<Thermal::DeviceKind> acceptedKinds = {}, QString sysfsRoot = {});

  [[nodiscard]] Thermal::BackendInfo info() const override;
  [[nodiscard]] bool probe() override;
  void refresh() override;
  [[nodiscard]] QList<Thermal::TemperatureSensor> sensors() const override;
  [[nodiscard]] QList<Thermal::FanDevice> fans() const override;
  [[nodiscard]] QList<Thermal::FanZone> zones() const override;
  [[nodiscard]] Thermal::Result setAutomatic(const QString &zoneId) override;
  [[nodiscard]] Thermal::Result setFullSpeed(const QString &zoneId) override;
  [[nodiscard]] Thermal::Result setDuty(const QString &zoneId,
                                        int percent) override;

private:
  QString m_sysfsRoot;
  QSet<Thermal::DeviceKind> m_acceptedKinds;
  QList<Thermal::TemperatureSensor> m_sensors;
  QHash<QString, QString> m_sensorPaths;
};
