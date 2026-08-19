#pragma once

#include "IFanBackend.h"

#include <QHash>

class HwmonBackend final : public IFanBackend {
public:
  explicit HwmonBackend(QString sysfsRoot = {});

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
  enum class ControlProtocol { Pwm, AppleSmc };

  struct Channel {
    QString zoneId;
    QString fanId;
    QString controllerId;
    QString deviceId;
    QString displayName;
    QString pwmPath;
    QString enablePath;
    QString rpmPath;
    QString sensorId;
    Thermal::DeviceKind kind = Thermal::DeviceKind::Unknown;
    int pwmMaximum = 255;
    int automaticValue = -1;
    ControlProtocol protocol = ControlProtocol::Pwm;
    QString manualPath;
    QString targetPath;
    int minimumRaw = 0;
    int maximumRaw = 255;
  };

  [[nodiscard]] const Channel *channelForZone(const QString &zoneId) const;
  [[nodiscard]] Thermal::Result writeValue(const QString &path, int value,
                                           const QString &code) const;

  QString m_sysfsRoot;
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QList<Channel> m_channels;
  QStringList m_drivers;
  QHash<QString, QString> m_sensorPaths;
};
