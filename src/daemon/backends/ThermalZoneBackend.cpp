#include "ThermalZoneBackend.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

namespace {
QString readText(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}

double readTemperature(const QString &path) {
  bool ok = false;
  const auto raw = readText(path).toDouble(&ok);
  if (!ok)
    return -1.0;
  return raw > 1000.0 ? raw / 1000.0 : raw;
}

QString safeId(QString value) {
  value = value.toLower();
  value.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                QStringLiteral("_"));
  return value.isEmpty() ? QStringLiteral("unknown") : value;
}

Thermal::DeviceKind kindForType(const QString &type) {
  const auto lower = type.toLower();
  if (lower.contains(QStringLiteral("gpu")) ||
      lower.contains(QStringLiteral("amdgpu")) ||
      lower.contains(QStringLiteral("i915")) ||
      lower.contains(QStringLiteral("xe")))
    return Thermal::DeviceKind::Gpu;
  if (lower.contains(QStringLiteral("cpu")) ||
      lower.contains(QStringLiteral("pkg")) ||
      lower.contains(QStringLiteral("soc")) ||
      lower.contains(QStringLiteral("tctl")))
    return Thermal::DeviceKind::Cpu;
  return Thermal::DeviceKind::System;
}
} // namespace

ThermalZoneBackend::ThermalZoneBackend(
    QSet<Thermal::DeviceKind> acceptedKinds, QString sysfsRoot)
    : m_sysfsRoot(sysfsRoot.isEmpty()
                      ? qEnvironmentVariable("THERMAL_CONTROL_SYSFS_ROOT",
                                             QStringLiteral("/sys"))
                      : std::move(sysfsRoot)),
      m_acceptedKinds(std::move(acceptedKinds)) {}

Thermal::BackendInfo ThermalZoneBackend::info() const {
  const bool ready = !m_sensors.isEmpty();
  return {QStringLiteral("thermal-zone"),
          QStringLiteral("Linux thermal zones"),
          QStringLiteral("thermal_sys"),
          ready ? QStringLiteral("read-only") : QStringLiteral("unsupported"),
          QStringLiteral("Read-only fallback when CPU or GPU hwmon sensors are absent"),
          {ready, false, false, false}};
}

bool ThermalZoneBackend::probe() {
  m_sensors.clear();
  m_sensorPaths.clear();
  const QDir directory(QDir(m_sysfsRoot).filePath(QStringLiteral("class/thermal")));
  const auto entries = directory.entryInfoList(
      {QStringLiteral("thermal_zone*")}, QDir::Dirs | QDir::NoDotAndDotDot,
      QDir::Name);
  for (const auto &entry : entries) {
    const auto base = entry.absoluteFilePath();
    const auto type = readText(base + QStringLiteral("/type"));
    const auto kind = kindForType(type);
    if (!m_acceptedKinds.isEmpty() && !m_acceptedKinds.contains(kind))
      continue;
    const auto path = base + QStringLiteral("/temp");
    const auto temperature = readTemperature(path);
    if (temperature < 0.0)
      continue;
    const auto id = QStringLiteral("thermal:%1:%2")
                        .arg(safeId(type), safeId(entry.fileName()));
    m_sensors.push_back({id, QStringLiteral("thermal-zone"),
                         QStringLiteral("thermal:%1").arg(safeId(type)),
                         type.isEmpty() ? QStringLiteral("Thermal zone") : type,
                         kind, temperature, true});
    m_sensorPaths.insert(id, path);
  }
  return !m_sensors.isEmpty();
}

void ThermalZoneBackend::refresh() {
  for (auto &sensor : m_sensors) {
    sensor.temperature = readTemperature(m_sensorPaths.value(sensor.id));
    sensor.available = sensor.temperature >= 0.0;
  }
}

QList<Thermal::TemperatureSensor> ThermalZoneBackend::sensors() const {
  return m_sensors;
}
QList<Thermal::FanDevice> ThermalZoneBackend::fans() const { return {}; }
QList<Thermal::FanZone> ThermalZoneBackend::zones() const { return {}; }
Thermal::Result ThermalZoneBackend::setAutomatic(const QString &) {
  return Thermal::Result::fail(QStringLiteral("thermal-zone.read-only"),
                               QStringLiteral("Thermal zones are read-only."));
}
Thermal::Result ThermalZoneBackend::setFullSpeed(const QString &zoneId) {
  return setAutomatic(zoneId);
}
Thermal::Result ThermalZoneBackend::setDuty(const QString &zoneId, int) {
  return setAutomatic(zoneId);
}
