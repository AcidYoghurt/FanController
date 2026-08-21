#pragma once

#include "IFanBackend.h"

// 提供可重复的内存后端，供界面开发和 D-Bus 集成测试使用。
// 只有守护进程显式启用模拟模式时才会创建该后端。
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
  [[nodiscard]] bool setSensorTemperature(const QString &sensorId,
                                          double temperature);
  // 使下一条写命令失败一次，随后自动恢复正常。
  void failNextCommand();

private:
  [[nodiscard]] Thermal::Result command(const QString &zoneId, int duty);
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QHash<QString, int> m_duties;
  QHash<QString, double> m_forcedTemperatures;
  int m_tick = 0;
  bool m_sensorFailure = false;
  bool m_failNext = false;
};
