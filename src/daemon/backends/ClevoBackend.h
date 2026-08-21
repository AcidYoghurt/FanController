#pragma once

#include "IFanBackend.h"

#include <QMutex>

class ClevoBackend final : public IFanBackend {
public:
  ClevoBackend();

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
  [[nodiscard]] static QByteArray normalizeAcpiResponse(QByteArray response);
  [[nodiscard]] static double
  cpuPackageTemperature(const QString &hwmonRoot =
                            QStringLiteral("/sys/class/hwmon"));

private:
  [[nodiscard]] Thermal::Result call(const QByteArray &command,
                                     const QByteArray &expected) const;
  [[nodiscard]] bool validZone(const QString &zoneId) const;

  mutable QMutex m_mutex;
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QString m_detail;
};
