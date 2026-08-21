#pragma once

#include "IFanBackend.h"

// 通过 thinkpad_acpi 的 /proc/acpi/ibm/fan 接口控制风扇等级。
// 只有内核明确启用 fan_control 且列出相应命令时才公开写能力。
class ThinkPadBackend final : public IFanBackend {
public:
  explicit ThinkPadBackend(QString procRoot = {}, QString sysfsRoot = {});

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
  void readState();
  [[nodiscard]] Thermal::Result writeCommand(const QString &command) const;
  [[nodiscard]] bool validZone(const QString &zoneId) const;

  QString m_procRoot;
  QString m_sysfsRoot;
  QString m_fanPath;
  QString m_zoneId;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QString m_commands;
  QString m_detail;
  bool m_manualEnabled = false;
  bool m_fullSpeedSupported = false;
};
