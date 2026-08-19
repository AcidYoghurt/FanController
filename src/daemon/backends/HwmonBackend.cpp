#include "HwmonBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtMath>

namespace {
QString readText(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}

int readInteger(const QString &path, int fallback = -1) {
  bool ok = false;
  const auto value = readText(path).toInt(&ok);
  return ok ? value : fallback;
}

bool canWrite(const QString &path) {
  const QFileInfo info(path);
  return info.exists() && info.isFile() && info.isWritable();
}

QString safeId(QString value) {
  value = value.toLower();
  value.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                QStringLiteral("_"));
  while (value.startsWith(u'_'))
    value.removeFirst();
  while (value.endsWith(u'_'))
    value.chop(1);
  return value.isEmpty() ? QStringLiteral("unknown") : value;
}

Thermal::DeviceKind kindForDriver(const QString &driver) {
  const auto name = driver.toLower();
  if (name == QStringLiteral("coretemp") || name == QStringLiteral("k10temp") ||
      name == QStringLiteral("zenpower") || name.contains(QStringLiteral("cpu")))
    return Thermal::DeviceKind::Cpu;
  if (name == QStringLiteral("amdgpu") || name == QStringLiteral("nouveau") ||
      name == QStringLiteral("i915") || name == QStringLiteral("xe") ||
      name.contains(QStringLiteral("gpu")))
    return Thermal::DeviceKind::Gpu;
  return Thermal::DeviceKind::System;
}

QString kindLabel(Thermal::DeviceKind kind) {
  switch (kind) {
  case Thermal::DeviceKind::Cpu: return QStringLiteral("CPU");
  case Thermal::DeviceKind::Gpu: return QStringLiteral("GPU");
  case Thermal::DeviceKind::System: return QStringLiteral("System");
  case Thermal::DeviceKind::Unknown: return QStringLiteral("Device");
  }
  return QStringLiteral("Device");
}
} // namespace

HwmonBackend::HwmonBackend(QString sysfsRoot)
    : m_sysfsRoot(sysfsRoot.isEmpty()
                      ? qEnvironmentVariable("THERMAL_CONTROL_SYSFS_ROOT",
                                             QStringLiteral("/sys"))
                      : std::move(sysfsRoot)) {}

Thermal::BackendInfo HwmonBackend::info() const {
  Thermal::CapabilitySet capabilities;
  capabilities.telemetry = !m_sensors.isEmpty() || !m_fans.isEmpty();
  for (const auto &zone : m_zones) {
    capabilities.automatic |= zone.capabilities.automatic;
    capabilities.fullSpeed |= zone.capabilities.fullSpeed;
    capabilities.customCurve |= zone.capabilities.customCurve;
  }
  return {QStringLiteral("hwmon"),
          QStringLiteral("Linux hwmon"),
          m_drivers.join(QStringLiteral(", ")),
          m_sensors.isEmpty() && m_fans.isEmpty() ? QStringLiteral("unsupported")
                                                  : QStringLiteral("ready"),
          QStringLiteral("Standard kernel hwmon/sysfs interface"),
          capabilities};
}

bool HwmonBackend::probe() {
  m_sensors.clear();
  m_fans.clear();
  m_zones.clear();
  m_channels.clear();
  m_drivers.clear();
  m_sensorPaths.clear();

  const QDir directory(QDir(m_sysfsRoot).filePath(QStringLiteral("class/hwmon")));
  const auto entries = directory.entryInfoList(
      {QStringLiteral("hwmon*")}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  const QRegularExpression temperaturePattern(QStringLiteral("^temp(\\d+)_input$"));
  const QRegularExpression fanPattern(QStringLiteral("^fan(\\d+)_input$"));
  const QRegularExpression pwmPattern(QStringLiteral("^pwm(\\d+)$"));

  for (const auto &entry : entries) {
    const auto base = entry.absoluteFilePath();
    const auto driver = readText(base + QStringLiteral("/name"));
    if (driver.isEmpty())
      continue;
    if (!m_drivers.contains(driver))
      m_drivers.push_back(driver);

    QString physicalId = QFileInfo(base + QStringLiteral("/device")).canonicalFilePath();
    if (physicalId.isEmpty())
      physicalId = entry.fileName();
    else
      physicalId = QFileInfo(physicalId).fileName();
    const auto controllerId = QStringLiteral("hwmon:%1:%2")
                                  .arg(safeId(driver), safeId(physicalId));
    const auto kind = kindForDriver(driver);

    QHash<int, QString> sensorIds;
    const auto files = QDir(base).entryList(QDir::Files | QDir::Readable, QDir::Name);
    for (const auto &fileName : files) {
      const auto match = temperaturePattern.match(fileName);
      if (!match.hasMatch())
        continue;
      const auto index = match.captured(1).toInt();
      const auto sensorId = QStringLiteral("%1:temp%2").arg(controllerId).arg(index);
      const auto label = readText(base + QStringLiteral("/temp%1_label").arg(index));
      const auto raw = readInteger(base + u'/' + fileName);
      m_sensorPaths.insert(sensorId, base + u'/' + fileName);
      m_sensors.push_back({sensorId,
                           QStringLiteral("hwmon"),
                           controllerId,
                           label.isEmpty()
                               ? QStringLiteral("%1 %2 temperature")
                                     .arg(kindLabel(kind), QString::number(index))
                               : label,
                           kind,
                           raw >= 0 ? raw / 1000.0 : -1.0,
                           raw >= 0});
      sensorIds.insert(index, sensorId);
    }

    QHash<int, QString> rpmPaths;
    QHash<int, QString> pwmPaths;
    QHash<int, QString> appleTargetPaths;
    const QRegularExpression appleTargetPattern(
        QStringLiteral("^fan(\\d+)_(?:output|target)$"));
    for (const auto &fileName : files) {
      auto match = fanPattern.match(fileName);
      if (match.hasMatch()) {
        rpmPaths.insert(match.captured(1).toInt(), base + u'/' + fileName);
        continue;
      }
      match = pwmPattern.match(fileName);
      if (match.hasMatch()) {
        pwmPaths.insert(match.captured(1).toInt(), base + u'/' + fileName);
        continue;
      }
      match = appleTargetPattern.match(fileName);
      if (match.hasMatch())
        appleTargetPaths.insert(match.captured(1).toInt(), base + u'/' + fileName);
    }

    auto indices = rpmPaths.keys();
    for (const auto index : pwmPaths.keys())
      if (!indices.contains(index))
        indices.push_back(index);
    for (const auto index : appleTargetPaths.keys())
      if (!indices.contains(index))
        indices.push_back(index);
    std::sort(indices.begin(), indices.end());

    for (const auto index : indices) {
      const auto pwmPath = pwmPaths.value(index);
      const auto enablePath = base + QStringLiteral("/pwm%1_enable").arg(index);
      const int enableValue = readInteger(enablePath);
      const bool hasPwmControl = !pwmPath.isEmpty() && canWrite(pwmPath) &&
                                 canWrite(enablePath) && enableValue >= 2;

      const auto appleTargetPath = appleTargetPaths.value(index);
      const auto appleManualPath =
          base + QStringLiteral("/fan%1_manual").arg(index);
      const auto appleMinimum =
          readInteger(base + QStringLiteral("/fan%1_min").arg(index));
      const auto appleMaximum =
          readInteger(base + QStringLiteral("/fan%1_max").arg(index));
      const bool hasAppleControl =
          (driver == QStringLiteral("applesmc") ||
           driver == QStringLiteral("macsmc_hwmon")) &&
          !appleTargetPath.isEmpty() && canWrite(appleTargetPath) &&
          canWrite(appleManualPath) && appleMinimum >= 0 &&
          appleMaximum > appleMinimum;
      const bool hasSafeControl = hasPwmControl || hasAppleControl;
      const auto fanId = QStringLiteral("%1:fan%2").arg(controllerId).arg(index);
      const auto zoneId = QStringLiteral("%1:zone%2").arg(controllerId).arg(index);
      const auto label = readText(base + QStringLiteral("/fan%1_label").arg(index));
      const auto rpm = readInteger(rpmPaths.value(index));
      const auto dutyRaw = readInteger(hasAppleControl ? appleTargetPath
                                                        : pwmPath);
      const int dutyPercent = dutyRaw < 0
                                  ? -1
                                  : hasAppleControl
                                        ? qRound((dutyRaw - appleMinimum) *
                                                 100.0 /
                                                 (appleMaximum - appleMinimum))
                                        : qRound(dutyRaw * 100.0 / 255.0);
      const auto sensorId = sensorIds.value(index, sensorIds.isEmpty()
                                                     ? QString()
                                                     : sensorIds.constBegin().value());
      const auto displayName = label.isEmpty()
                                   ? QStringLiteral("%1 fan %2")
                                         .arg(kindLabel(kind), QString::number(index))
                                   : label;

      Channel channel{zoneId, fanId, controllerId, controllerId,
                      displayName, pwmPath, enablePath, rpmPaths.value(index),
                      sensorId, kind, 255,
                      hasPwmControl ? enableValue : -1};
      if (hasAppleControl) {
        channel.protocol = ControlProtocol::AppleSmc;
        channel.manualPath = appleManualPath;
        channel.targetPath = appleTargetPath;
        channel.minimumRaw = appleMinimum;
        channel.maximumRaw = appleMaximum;
      }
      m_channels.push_back(channel);
      m_fans.push_back({fanId,
                        QStringLiteral("hwmon"),
                        controllerId,
                        controllerId,
                        displayName,
                        kind,
                        rpm,
                        dutyPercent,
                        rpm >= 0,
                        hasSafeControl,
                        hasSafeControl,
                        hasSafeControl,
                        hasSafeControl && !sensorId.isEmpty(),
                        rpm == 0,
                        true,
                        hasSafeControl ? QStringLiteral("controllable")
                                       : QStringLiteral("read-only")});

      if (hasSafeControl) {
        Thermal::FanZone zone;
        zone.id = zoneId;
        zone.backendId = QStringLiteral("hwmon");
        zone.controllerId = controllerId;
        zone.displayName = displayName;
        zone.sensorId = sensorId;
        zone.targetFanIds = {fanId};
        zone.kind = kind;
        zone.capabilities = {true, true, true, !sensorId.isEmpty()};
        zone.minimumDuty = hasAppleControl ? 20 : 20;
        zone.safetyTemperature = kind == Thermal::DeviceKind::Gpu ? 90 : 85;
        m_zones.push_back(zone);
      }
    }
  }
  return !m_sensors.isEmpty() || !m_fans.isEmpty();
}

void HwmonBackend::refresh() {
  for (auto &sensor : m_sensors) {
    const auto raw = readInteger(m_sensorPaths.value(sensor.id));
    sensor.temperature = raw >= 0 ? raw / 1000.0 : -1.0;
    sensor.available = raw >= 0;
  }

  for (auto &fan : m_fans) {
    const auto iterator = std::find_if(m_channels.cbegin(), m_channels.cend(),
                                      [&fan](const Channel &channel) {
                                        return channel.fanId == fan.id;
                                      });
    if (iterator == m_channels.cend())
      continue;
    fan.rpm = readInteger(iterator->rpmPath);
    fan.rpmAvailable = fan.rpm >= 0;
    const auto dutyRaw = readInteger(
        iterator->protocol == ControlProtocol::AppleSmc
            ? iterator->targetPath
            : iterator->pwmPath);
    fan.duty = dutyRaw < 0
                   ? -1
                   : iterator->protocol == ControlProtocol::AppleSmc
                         ? qRound((dutyRaw - iterator->minimumRaw) * 100.0 /
                                  (iterator->maximumRaw - iterator->minimumRaw))
                         : qRound(dutyRaw * 100.0 / iterator->pwmMaximum);
    fan.zeroRpmActive = fan.rpmAvailable && fan.rpm == 0;
  }
}

QList<Thermal::TemperatureSensor> HwmonBackend::sensors() const { return m_sensors; }
QList<Thermal::FanDevice> HwmonBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> HwmonBackend::zones() const { return m_zones; }

const HwmonBackend::Channel *HwmonBackend::channelForZone(const QString &zoneId) const {
  const auto iterator = std::find_if(m_channels.cbegin(), m_channels.cend(),
                                    [&zoneId](const Channel &channel) {
                                      return channel.zoneId == zoneId;
                                    });
  return iterator == m_channels.cend() ? nullptr : &*iterator;
}

Thermal::Result HwmonBackend::writeValue(const QString &path, int value,
                                         const QString &code) const {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return Thermal::Result::fail(code, file.errorString());
  if (file.write(QByteArray::number(value) + '\n') < 0)
    return Thermal::Result::fail(code, file.errorString());
  return Thermal::Result::ok();
}

Thermal::Result HwmonBackend::setAutomatic(const QString &zoneId) {
  const auto *channel = channelForZone(zoneId);
  if (!channel)
    return Thermal::Result::fail(QStringLiteral("zone.unsupported"),
                                 QStringLiteral("Automatic control is unavailable for this zone."));
  if (channel->protocol == ControlProtocol::AppleSmc)
    return writeValue(channel->manualPath, 0,
                      QStringLiteral("applesmc.auto-write"));
  if (channel->enablePath.isEmpty() || channel->automaticValue < 2)
    return Thermal::Result::fail(QStringLiteral("zone.no-automatic"),
                                 QStringLiteral("The driver did not advertise an automatic PWM mode."));
  return writeValue(channel->enablePath, channel->automaticValue,
                    QStringLiteral("hwmon.auto-write"));
}

Thermal::Result HwmonBackend::setFullSpeed(const QString &zoneId) {
  return setDuty(zoneId, 100);
}

Thermal::Result HwmonBackend::setDuty(const QString &zoneId, int percent) {
  const auto *channel = channelForZone(zoneId);
  if (!channel)
    return Thermal::Result::fail(QStringLiteral("zone.unsupported"),
                                 QStringLiteral("Manual fan control is unavailable for this zone."));
  if (channel->protocol == ControlProtocol::AppleSmc) {
    auto result = writeValue(channel->manualPath, 1,
                             QStringLiteral("applesmc.manual-enable"));
    if (!result.success)
      return result;
    const auto raw = qRound(
        channel->minimumRaw + qBound(0, percent, 100) *
                                  (channel->maximumRaw - channel->minimumRaw) /
                                  100.0);
    return writeValue(channel->targetPath, raw,
                      QStringLiteral("applesmc.target-write"));
  }
  if (channel->pwmPath.isEmpty() || channel->enablePath.isEmpty())
    return Thermal::Result::fail(QStringLiteral("zone.unsupported"),
                                 QStringLiteral("Manual fan control is unavailable for this zone."));
  auto result = writeValue(channel->enablePath, 1,
                           QStringLiteral("hwmon.manual-enable"));
  if (!result.success)
    return result;
  const auto raw = qRound(qBound(0, percent, 100) * channel->pwmMaximum / 100.0);
  return writeValue(channel->pwmPath, raw, QStringLiteral("hwmon.pwm-write"));
}
