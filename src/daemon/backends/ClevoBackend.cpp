#include "ClevoBackend.h"

#include "Constants.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcess>
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

double cpuTemperature() {
  const QDir hwmon(QStringLiteral("/sys/class/hwmon"));
  const auto entries = hwmon.entryInfoList({QStringLiteral("hwmon*")},
                                           QDir::Dirs | QDir::NoDotAndDotDot);
  for (const auto &entry : entries) {
    const auto name = readText(entry.filePath() + QStringLiteral("/name"));
    if (name != QStringLiteral("coretemp") && name != QStringLiteral("k10temp") &&
        name != QStringLiteral("zenpower"))
      continue;
    const auto files = QDir(entry.filePath()).entryList(
        {QStringLiteral("temp*_input")}, QDir::Files, QDir::Name);
    double hottest = -1.0;
    for (const auto &file : files)
      hottest = qMax(hottest, readInteger(entry.filePath() + u'/' + file) / 1000.0);
    if (hottest >= 0.0)
      return hottest;
  }
  return -1.0;
}
} // namespace

ClevoBackend::ClevoBackend() = default;

Thermal::BackendInfo ClevoBackend::info() const {
  const bool ready = !m_zones.isEmpty();
  return {QStringLiteral("clevo-clv0001"),
          QStringLiteral("Clevo CLV0001 ACPI"),
          QStringLiteral("acpi_call"),
          ready ? QStringLiteral("ready") : QStringLiteral("unsupported"),
          m_detail,
          {ready, ready, ready, ready}};
}

bool ClevoBackend::probe() {
  m_sensors.clear();
  m_fans.clear();
  m_zones.clear();

  const auto vendor = readText(QStringLiteral("/sys/class/dmi/id/sys_vendor"));
  const auto product = readText(QStringLiteral("/sys/class/dmi/id/product_name"));
  const auto path = readText(QStringLiteral("/sys/bus/acpi/devices/CLV0001:00/path"));
  if (vendor.compare(QStringLiteral("COLORFUL"), Qt::CaseInsensitive) != 0 ||
      product.compare(QStringLiteral("P16 TA 24"), Qt::CaseInsensitive) != 0 ||
      (path != QStringLiteral("\\_SB_.DCHU") && path != QStringLiteral("\\_SB.DCHU"))) {
    m_detail = QStringLiteral("Verified CLV0001/DCHU interface not present");
    return false;
  }

  if (!QFile::exists(QString::fromLatin1(LinuxThermal::Constants::AcpiCallPath)))
    QProcess::execute(QStringLiteral("/usr/sbin/modprobe"),
                      {QStringLiteral("acpi_call")});
  const QFileInfo callInfo(QString::fromLatin1(LinuxThermal::Constants::AcpiCallPath));
  if (!callInfo.exists() || !callInfo.isWritable()) {
    m_detail = QStringLiteral("acpi_call is unavailable or not writable");
    return false;
  }

  const QString controllerId = QStringLiteral("clevo:CLV0001");
  const QString sensorId = controllerId + QStringLiteral(":cpu-package");
  m_sensors.push_back({sensorId, QStringLiteral("clevo-clv0001"), controllerId,
                       QStringLiteral("CPU package"), Thermal::DeviceKind::Cpu,
                       cpuTemperature(), true});
  QStringList fanIds;
  for (int index = 0; index < 4; ++index) {
    const auto id = QStringLiteral("%1:fan%2").arg(controllerId).arg(index);
    fanIds.push_back(id);
    const auto rpm = readInteger(
        QStringLiteral("/sys/bus/acpi/devices/INTC10A2:0%1/fan_speed_rpm").arg(index));
    m_fans.push_back({id, QStringLiteral("clevo-clv0001"), controllerId,
                      controllerId,
                      index == 0 ? QStringLiteral("CPU fan")
                                 : QStringLiteral("System fan %1").arg(index + 1),
                      index == 0 ? Thermal::DeviceKind::Cpu
                                 : Thermal::DeviceKind::System,
                      rpm, -1, rpm >= 0, true, true, true, index == 0,
                      rpm == 0, true, QStringLiteral("controllable")});
  }
  Thermal::FanZone zone;
  zone.id = controllerId + QStringLiteral(":cooling-zone");
  zone.backendId = QStringLiteral("clevo-clv0001");
  zone.controllerId = controllerId;
  zone.displayName = QStringLiteral("CPU and system cooling");
  zone.sensorId = sensorId;
  zone.targetFanIds = fanIds;
  zone.kind = Thermal::DeviceKind::Cpu;
  zone.capabilities = {true, true, true, true};
  zone.minimumDuty = 40;
  zone.safetyTemperature = 85;
  m_zones.push_back(zone);
  m_detail = QStringLiteral("Verified COLORFUL P16 TA 24 ACPI implementation");
  return true;
}

void ClevoBackend::refresh() {
  if (!m_sensors.isEmpty()) {
    const auto temperature = cpuTemperature();
    m_sensors[0].temperature = temperature;
    m_sensors[0].available = temperature >= 0;
  }
  for (int index = 0; index < m_fans.size(); ++index) {
    const auto rpm = readInteger(
        QStringLiteral("/sys/bus/acpi/devices/INTC10A2:0%1/fan_speed_rpm").arg(index));
    m_fans[index].rpm = rpm;
    m_fans[index].rpmAvailable = rpm >= 0;
    m_fans[index].zeroRpmActive = rpm == 0;
  }
}

QList<Thermal::TemperatureSensor> ClevoBackend::sensors() const { return m_sensors; }
QList<Thermal::FanDevice> ClevoBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> ClevoBackend::zones() const { return m_zones; }

bool ClevoBackend::validZone(const QString &zoneId) const {
  return !m_zones.isEmpty() && m_zones.constFirst().id == zoneId;
}

QByteArray ClevoBackend::normalizeAcpiResponse(QByteArray response) {
  const auto terminator = response.indexOf('\0');
  if (terminator >= 0)
    response.truncate(terminator);
  return response.trimmed();
}

Thermal::Result ClevoBackend::call(const QByteArray &command,
                                   const QByteArray &expected) const {
  QMutexLocker locker(&m_mutex);
  QFile file(QString::fromLatin1(LinuxThermal::Constants::AcpiCallPath));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return Thermal::Result::fail(QStringLiteral("clevo.open"), file.errorString());
  if (file.write(command + '\n') < 0)
    return Thermal::Result::fail(QStringLiteral("clevo.write"), file.errorString());
  file.close();
  if (!file.open(QIODevice::ReadOnly))
    return Thermal::Result::fail(QStringLiteral("clevo.read"), file.errorString());
  const auto response = normalizeAcpiResponse(file.readAll());
  if (response != expected)
    return Thermal::Result::fail(
        QStringLiteral("clevo.response"),
        QStringLiteral("Unexpected ACPI response: %1").arg(QString::fromLatin1(response)));
  return Thermal::Result::ok();
}

Thermal::Result ClevoBackend::setAutomatic(const QString &zoneId) {
  if (!validZone(zoneId))
    return Thermal::Result::fail(QStringLiteral("zone.unknown"), QStringLiteral("Unknown fan zone."));
  return call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                  " 0x0 0x69 0x0000000f",
              "0x69");
}

Thermal::Result ClevoBackend::setFullSpeed(const QString &zoneId) {
  if (!validZone(zoneId))
    return Thermal::Result::fail(QStringLiteral("zone.unknown"), QStringLiteral("Unknown fan zone."));
  return call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                  " 0x0 0x68 0xffffffff",
              "0x68");
}

Thermal::Result ClevoBackend::setDuty(const QString &zoneId, int percent) {
  if (!validZone(zoneId))
    return Thermal::Result::fail(QStringLiteral("zone.unknown"), QStringLiteral("Unknown fan zone."));
  percent = qBound(40, percent, 100);
  const auto raw = qRound(percent * 255.0 / 100.0);
  const auto payload = 0xffffff00u | static_cast<quint32>(raw);
  const auto payloadText = QByteArrayLiteral("0x") +
                           QByteArray::number(payload, 16).rightJustified(8, '0');
  auto result = call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                         " 0x0 0x68 " + payloadText,
                     "0x68");
  if (!result.success)
    return result;
  return call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                  " 0x0 0x69 0x0000000e",
              "0x69");
}
