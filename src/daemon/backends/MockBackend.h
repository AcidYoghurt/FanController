#pragma once

#include "IFanBackend.h"

class MockBackend final : public IFanBackend {
public:
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

  void setSensorFailure(bool enabled);
  void failNextCommand();

private:
  [[nodiscard]] Thermal::Result command(const QString &zoneId, int duty);
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QHash<QString, int> m_duties;
  int m_tick = 0;
  bool m_sensorFailure = false;
  bool m_failNext = false;
};
