#include "ClevoBackend.h"

#include "Constants.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcess>
#include <QtMath>

#include <utility>

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

int controlTemperaturePriority(const QString &label) {
  if (label.startsWith(QStringLiteral("Package id"), Qt::CaseInsensitive) ||
      label.compare(QStringLiteral("Tctl"), Qt::CaseInsensitive) == 0)
    return 2;
  if (label.compare(QStringLiteral("Tdie"), Qt::CaseInsensitive) == 0)
    return 1;
  return 0;
}
} // namespace

double ClevoBackend::cpuPackageTemperature(const QString &hwmonRoot) {
  const QDir hwmon(hwmonRoot);
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
    double package = -1.0;
    int packagePriority = 0;
    for (const auto &file : files) {
      const int millidegrees = readInteger(entry.filePath() + u'/' + file);
      if (millidegrees < 0)
        continue;
      const double temperature = millidegrees / 1000.0;
      hottest = qMax(hottest, temperature);
      const auto labelFile =
          file.first(file.size() - QStringLiteral("_input").size()) +
          QStringLiteral("_label");
      const int priority = controlTemperaturePriority(
          readText(entry.filePath() + u'/' + labelFile));
      if (priority > packagePriority) {
        package = temperature;
        packagePriority = priority;
      } else if (priority > 0 && priority == packagePriority) {
        package = qMax(package, temperature);
      }
    }
    if (package >= 0.0)
      return package;
    if (hottest >= 0.0)
      return hottest;
  }
  return -1.0;
}

QList<Thermal::FanDevice>
ClevoBackend::verifiedFanDevices(const QString &acpiRoot) {
  const QString controllerId = QStringLiteral("clevo:CLV0001");
  QList<Thermal::FanDevice> fans;
  // 已验证机型只有两只物理风扇。固件同时公开了转速恒为 0 的 TFN3 和
  // 与 TFN2 重复的 TFN4；二者都不是额外风扇，公开它们会产生错误的故障
  // 遥测。
  for (int index = 0; index < 2; ++index) {
    const auto id = QStringLiteral("%1:fan%2").arg(controllerId).arg(index);
    const auto rpm = readInteger(
        QStringLiteral("%1/INTC10A2:0%2/fan_speed_rpm")
            .arg(acpiRoot)
            .arg(index));
    fans.push_back({id,
                    QStringLiteral("clevo-clv0001"),
                    controllerId,
                    controllerId,
                    index == 0 ? QStringLiteral("CPU fan")
                               : QStringLiteral("GPU fan"),
                    index == 0 ? Thermal::DeviceKind::Cpu
                               : Thermal::DeviceKind::Gpu,
                    rpm,
                    -1,
                    rpm >= 0,
                    true,
                    true,
                    true,
                    true,
                    rpm == 0,
                    true,
                    QStringLiteral("controllable")});
  }
  return fans;
}

quint32 ClevoBackend::packedDutyPayload(int percent) {
  percent = qBound(40, percent, 100);
  const auto raw = static_cast<quint32>(
      qRound(static_cast<double>(percent) * 255.0 / 100.0));
  // WMBB 方法 0x68 为每个 EC 风扇槽接收一个占空比字节。0xffffffXX 会
  // 把其余槽位设为 100%，导致 GPU 风扇在曲线转速与满速间跳变。这里把
  // 请求值复制到全部槽位；TFN3 未使用，TFN4 是第二只物理风扇的别名。
  return raw * 0x01010101u;
}

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
                       cpuPackageTemperature(), true});
  m_fans = verifiedFanDevices();
  QStringList fanIds;
  for (const auto &fan : std::as_const(m_fans))
    fanIds.push_back(fan.id);
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
    const auto temperature = cpuPackageTemperature();
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
  const auto payload = packedDutyPayload(percent);
  const auto payloadText = QByteArrayLiteral("0x") +
                           QByteArray::number(payload, 16).rightJustified(8, '0');
  auto result = call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                         " 0x0 0x68 " + payloadText,
                     "0x68");
  if (!result.success)
    return result;
  // 0x68 只写目标值，随后还要用 0x69 保持两只物理风扇处于手动模式。
  // 掩码仅把未使用的 EC 槽位交还固件。
  const auto autoMaskText =
      QByteArrayLiteral("0x") +
      QByteArray::number(unusedFanAutoMask(), 16).rightJustified(8, '0');
  return call(QByteArray(LinuxThermal::Constants::AcpiMethod) +
                  " 0x0 0x69 " + autoMaskText,
              "0x69");
}
