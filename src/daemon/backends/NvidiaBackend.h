#pragma once

#include "IFanBackend.h"

#include <memory>

class NvidiaBackend final : public IFanBackend {
public:
  NvidiaBackend();
  ~NvidiaBackend() override;

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
  struct Api;
  struct Device;

  [[nodiscard]] bool probeNvml();
  [[nodiscard]] bool querySmi();
  [[nodiscard]] Device *deviceForZone(const QString &zoneId,
                                      unsigned *fanIndex = nullptr);
  [[nodiscard]] Thermal::Result nvmlError(const QString &code,
                                          int result) const;

  std::unique_ptr<Api> m_api;
  QList<Device> m_devices;
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QString m_detail;
  bool m_usingNvml = false;
};
