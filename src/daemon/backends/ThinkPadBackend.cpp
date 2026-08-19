#include "ThinkPadBackend.h"

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

QString fieldValue(const QString &text, const QString &field) {
  const QRegularExpression expression(
      QStringLiteral("(?:^|\\n)%1:\\s*([^\\n]+)")
          .arg(QRegularExpression::escape(field)));
  const auto match = expression.match(text);
  return match.hasMatch() ? match.captured(1).trimmed() : QString();
}
} // namespace

ThinkPadBackend::ThinkPadBackend(QString procRoot, QString sysfsRoot)
    : m_procRoot(procRoot.isEmpty() ? QStringLiteral("/proc")
                                    : std::move(procRoot)),
      m_sysfsRoot(sysfsRoot.isEmpty() ? QStringLiteral("/sys")
                                      : std::move(sysfsRoot)) {}

Thermal::BackendInfo ThinkPadBackend::info() const {
  const bool detected = !m_fans.isEmpty();
  Thermal::CapabilitySet capabilities{detected, m_manualEnabled,
                                      m_fullSpeedSupported,
                                      m_manualEnabled};
  return {QStringLiteral("thinkpad-acpi"),
          QStringLiteral("ThinkPad ACPI fan"),
          QStringLiteral("thinkpad_acpi"),
          !detected ? QStringLiteral("unsupported")
                    : m_manualEnabled ? QStringLiteral("ready")
                                      : QStringLiteral("read-only"),
          m_detail, capabilities};
}

bool ThinkPadBackend::probe() {
  m_fans.clear();
  m_zones.clear();
  m_fanPath = QDir(m_procRoot).filePath(QStringLiteral("acpi/ibm/fan"));
  if (!QFileInfo::exists(m_fanPath)) {
    m_detail = QStringLiteral("/proc/acpi/ibm/fan is not present");
    return false;
  }

  const auto parameter =
      readText(QDir(m_sysfsRoot).filePath(
          QStringLiteral("module/thinkpad_acpi/parameters/fan_control")));
  const auto state = readText(m_fanPath);
  m_commands = fieldValue(state, QStringLiteral("commands"));
  m_manualEnabled =
      (parameter.compare(QStringLiteral("Y"), Qt::CaseInsensitive) == 0 ||
       parameter == QStringLiteral("1")) &&
      QFileInfo(m_fanPath).isWritable() &&
      m_commands.contains(QStringLiteral("level"), Qt::CaseInsensitive);
  m_fullSpeedSupported =
      m_manualEnabled &&
      m_commands.contains(QStringLiteral("full-speed"), Qt::CaseInsensitive);

  const auto controllerId = QStringLiteral("thinkpad-acpi:ec");
  const auto fanId = controllerId + QStringLiteral(":fan0");
  m_zoneId = controllerId + QStringLiteral(":zone0");
  m_fans.push_back({fanId, QStringLiteral("thinkpad-acpi"), controllerId,
                    QStringLiteral("thinkpad:embedded-controller"),
                    QStringLiteral("ThinkPad system fan"),
                    Thermal::DeviceKind::System, -1, -1, false,
                    m_manualEnabled, m_manualEnabled, m_fullSpeedSupported,
                    m_manualEnabled, false, true,
                    m_manualEnabled ? QStringLiteral("controllable")
                                    : QStringLiteral("read-only")});

  if (m_manualEnabled) {
    Thermal::FanZone zone;
    zone.id = m_zoneId;
    zone.backendId = QStringLiteral("thinkpad-acpi");
    zone.controllerId = controllerId;
    zone.displayName = QStringLiteral("ThinkPad system cooling");
    zone.targetFanIds = {fanId};
    zone.kind = Thermal::DeviceKind::Cpu;
    zone.capabilities = {true, true, m_fullSpeedSupported, true};
    zone.minimumDuty = 15;
    zone.safetyTemperature = 90;
    m_zones.push_back(zone);
  }
  m_detail = m_manualEnabled
                 ? QStringLiteral("Kernel fan_control is enabled; using documented ACPI fan levels")
                 : QStringLiteral("Telemetry only; load thinkpad_acpi with fan_control=1 to enable writes");
  readState();
  return true;
}

void ThinkPadBackend::readState() {
  if (m_fans.isEmpty())
    return;
  const auto state = readText(m_fanPath);
  bool ok = false;
  const auto rpm = fieldValue(state, QStringLiteral("speed")).toInt(&ok);
  m_fans[0].rpm = ok ? rpm : -1;
  m_fans[0].rpmAvailable = ok;
  m_fans[0].zeroRpmActive = ok && rpm == 0;
  const auto level = fieldValue(state, QStringLiteral("level"));
  bool levelOk = false;
  const auto numericLevel = level.toInt(&levelOk);
  m_fans[0].duty = levelOk ? qRound(qBound(0, numericLevel, 7) * 100.0 / 7.0)
                            : -1;
}

void ThinkPadBackend::refresh() { readState(); }
QList<Thermal::TemperatureSensor> ThinkPadBackend::sensors() const { return {}; }
QList<Thermal::FanDevice> ThinkPadBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> ThinkPadBackend::zones() const { return m_zones; }

bool ThinkPadBackend::validZone(const QString &zoneId) const {
  return m_manualEnabled && zoneId == m_zoneId;
}

Thermal::Result ThinkPadBackend::writeCommand(const QString &command) const {
  QFile file(m_fanPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return Thermal::Result::fail(QStringLiteral("thinkpad.open"),
                                 file.errorString());
  if (file.write(command.toUtf8() + '\n') < 0)
    return Thermal::Result::fail(QStringLiteral("thinkpad.write"),
                                 file.errorString());
  return Thermal::Result::ok();
}

Thermal::Result ThinkPadBackend::setAutomatic(const QString &zoneId) {
  if (!validZone(zoneId))
    return Thermal::Result::fail(QStringLiteral("thinkpad.unsupported"),
                                 QStringLiteral("ThinkPad manual fan control is unavailable."));
  return writeCommand(QStringLiteral("level auto"));
}
Thermal::Result ThinkPadBackend::setFullSpeed(const QString &zoneId) {
  if (!validZone(zoneId) || !m_fullSpeedSupported)
    return Thermal::Result::fail(QStringLiteral("thinkpad.no-full-speed"),
                                 QStringLiteral("The ThinkPad driver did not advertise full-speed control."));
  return writeCommand(QStringLiteral("level full-speed"));
}
Thermal::Result ThinkPadBackend::setDuty(const QString &zoneId, int percent) {
  if (!validZone(zoneId))
    return Thermal::Result::fail(QStringLiteral("thinkpad.unsupported"),
                                 QStringLiteral("ThinkPad manual fan control is unavailable."));
  const int level = qBound(1, qCeil(qBound(1, percent, 100) * 7.0 / 100.0), 7);
  return writeCommand(QStringLiteral("level %1").arg(level));
}
