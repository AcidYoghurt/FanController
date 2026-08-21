#pragma once

#include "IFanBackend.h"

// 读取 IdeaPad/ThinkBook 固件风扇模式和平台配置。
// 这些值没有跨机型稳定的占空比语义，因此后端只报告遥测，不执行写入。
class IdeaPadBackend final : public IFanBackend {
public:
  explicit IdeaPadBackend(QString sysfsRoot = {});

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
  QString m_fanModePath;
  QString m_profilePath;
  QString m_profileChoicesPath;
  QString m_fanMode;
  QString m_profile;
  QString m_profileChoices;
  QList<Thermal::FanDevice> m_fans;
};
