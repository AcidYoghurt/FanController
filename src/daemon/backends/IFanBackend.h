#pragma once

#include "ThermalTypes.h"

#include <QList>

class IFanBackend {
public:
  virtual ~IFanBackend() = default;

  [[nodiscard]] virtual Thermal::BackendInfo info() const = 0;
  [[nodiscard]] virtual bool probe() = 0;
  virtual void refresh() = 0;
  [[nodiscard]] virtual QList<Thermal::TemperatureSensor> sensors() const = 0;
  [[nodiscard]] virtual QList<Thermal::FanDevice> fans() const = 0;
  [[nodiscard]] virtual QList<Thermal::FanZone> zones() const = 0;
  [[nodiscard]] virtual Thermal::Result setAutomatic(const QString &zoneId) = 0;
  [[nodiscard]] virtual Thermal::Result setFullSpeed(const QString &zoneId) = 0;
  [[nodiscard]] virtual Thermal::Result setDuty(const QString &zoneId,
                                                int percent) = 0;
};
