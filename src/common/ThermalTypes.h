#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace Thermal {

enum class ControlMode { Automatic, FullSpeed, CustomCurve, Unknown };

enum class DeviceKind { Cpu, Gpu, System, Unknown };

struct CapabilitySet {
  bool telemetry = false;
  bool automatic = false;
  bool fullSpeed = false;
  bool customCurve = false;
};

struct TemperatureSensor {
  QString id;
  QString backendId;
  QString deviceId;
  QString displayName;
  DeviceKind kind = DeviceKind::Unknown;
  double temperature = -1.0;
  bool available = false;
};

struct FanDevice {
  QString id;
  QString backendId;
  QString controllerId;
  QString deviceId;
  QString displayName;
  DeviceKind kind = DeviceKind::Unknown;
  int rpm = -1;
  int duty = -1;
  bool rpmAvailable = false;
  bool controllable = false;
  bool automaticSupported = false;
  bool fullSpeedSupported = false;
  bool curveSupported = false;
  bool zeroRpmActive = false;
  bool available = true;
  QString status;
};

struct FanZone {
  QString id;
  QString backendId;
  QString controllerId;
  QString displayName;
  QString sensorId;
  QStringList targetFanIds;
  DeviceKind kind = DeviceKind::Unknown;
  CapabilitySet capabilities;
  int minimumDuty = 40;
  int maximumDuty = 100;
  int minimumTemperature = 30;
  int maximumTemperature = 100;
  int safetyTemperature = 85;
  bool zeroRpmAllowed = false;
};

struct BackendInfo {
  QString id;
  QString displayName;
  QString driver;
  QString status;
  QString detail;
  CapabilitySet capabilities;
};

struct Result {
  bool success = true;
  QString code;
  QString message;

  static Result ok() { return {}; }
  static Result fail(QString code, QString message) {
    return {false, std::move(code), std::move(message)};
  }
};

inline QString deviceKindToString(DeviceKind kind) {
  switch (kind) {
  case DeviceKind::Cpu: return QStringLiteral("cpu");
  case DeviceKind::Gpu: return QStringLiteral("gpu");
  case DeviceKind::System: return QStringLiteral("system");
  case DeviceKind::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

inline QString modeToString(ControlMode mode) {
  switch (mode) {
  case ControlMode::Automatic: return QStringLiteral("automatic");
  case ControlMode::FullSpeed: return QStringLiteral("full-speed");
  case ControlMode::CustomCurve: return QStringLiteral("custom-curve");
  case ControlMode::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

inline ControlMode modeFromString(QStringView value) {
  if (value.compare(u"automatic", Qt::CaseInsensitive) == 0 ||
      value.compare(u"auto", Qt::CaseInsensitive) == 0)
    return ControlMode::Automatic;
  if (value.compare(u"full-speed", Qt::CaseInsensitive) == 0 ||
      value.compare(u"all-max", Qt::CaseInsensitive) == 0)
    return ControlMode::FullSpeed;
  if (value.compare(u"custom-curve", Qt::CaseInsensitive) == 0 ||
      value.compare(u"custom", Qt::CaseInsensitive) == 0)
    return ControlMode::CustomCurve;
  return ControlMode::Unknown;
}

inline QVariantMap capabilityMap(const CapabilitySet &capabilities) {
  return {{QStringLiteral("telemetry"), capabilities.telemetry},
          {QStringLiteral("automatic"), capabilities.automatic},
          {QStringLiteral("fullSpeed"), capabilities.fullSpeed},
          {QStringLiteral("customCurve"), capabilities.customCurve}};
}

inline QVariantMap toMap(const TemperatureSensor &sensor) {
  return {{QStringLiteral("sensorId"), sensor.id},
          {QStringLiteral("backendId"), sensor.backendId},
          {QStringLiteral("deviceId"), sensor.deviceId},
          {QStringLiteral("displayName"), sensor.displayName},
          {QStringLiteral("deviceKind"), deviceKindToString(sensor.kind)},
          {QStringLiteral("temperature"), sensor.temperature},
          {QStringLiteral("available"), sensor.available}};
}

inline QVariantMap toMap(const FanDevice &fan) {
  return {{QStringLiteral("fanId"), fan.id},
          {QStringLiteral("backendId"), fan.backendId},
          {QStringLiteral("controllerId"), fan.controllerId},
          {QStringLiteral("deviceId"), fan.deviceId},
          {QStringLiteral("displayName"), fan.displayName},
          {QStringLiteral("deviceKind"), deviceKindToString(fan.kind)},
          {QStringLiteral("rpm"), fan.rpm},
          {QStringLiteral("duty"), fan.duty},
          {QStringLiteral("rpmAvailable"), fan.rpmAvailable},
          {QStringLiteral("controllable"), fan.controllable},
          {QStringLiteral("automaticSupported"), fan.automaticSupported},
          {QStringLiteral("fullSpeedSupported"), fan.fullSpeedSupported},
          {QStringLiteral("curveSupported"), fan.curveSupported},
          {QStringLiteral("zeroRpmActive"), fan.zeroRpmActive},
          {QStringLiteral("available"), fan.available},
          {QStringLiteral("status"), fan.status}};
}

inline QVariantMap toMap(const FanZone &zone) {
  return {{QStringLiteral("zoneId"), zone.id},
          {QStringLiteral("backendId"), zone.backendId},
          {QStringLiteral("controllerId"), zone.controllerId},
          {QStringLiteral("displayName"), zone.displayName},
          {QStringLiteral("sensorId"), zone.sensorId},
          {QStringLiteral("targetFanIds"), zone.targetFanIds},
          {QStringLiteral("deviceKind"), deviceKindToString(zone.kind)},
          {QStringLiteral("automaticSupported"), zone.capabilities.automatic},
          {QStringLiteral("fullSpeedSupported"), zone.capabilities.fullSpeed},
          {QStringLiteral("curveSupported"), zone.capabilities.customCurve},
          {QStringLiteral("minimumDuty"), zone.minimumDuty},
          {QStringLiteral("maximumDuty"), zone.maximumDuty},
          {QStringLiteral("minimumTemperature"), zone.minimumTemperature},
          {QStringLiteral("maximumTemperature"), zone.maximumTemperature},
          {QStringLiteral("safetyTemperature"), zone.safetyTemperature},
          {QStringLiteral("zeroRpmAllowed"), zone.zeroRpmAllowed}};
}

inline QVariantMap toMap(const BackendInfo &backend) {
  return {{QStringLiteral("backendId"), backend.id},
          {QStringLiteral("displayName"), backend.displayName},
          {QStringLiteral("driver"), backend.driver},
          {QStringLiteral("status"), backend.status},
          {QStringLiteral("detail"), backend.detail},
          {QStringLiteral("capabilities"), capabilityMap(backend.capabilities)}};
}

} // namespace Thermal
