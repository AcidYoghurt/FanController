#include "IdeaPadBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {
QString readText(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}
} // namespace

IdeaPadBackend::IdeaPadBackend(QString sysfsRoot)
    : m_sysfsRoot(sysfsRoot.isEmpty()
                      ? qEnvironmentVariable("THERMAL_CONTROL_SYSFS_ROOT",
                                             QStringLiteral("/sys"))
                      : std::move(sysfsRoot)) {}

Thermal::BackendInfo IdeaPadBackend::info() const {
  const bool detected = !m_fans.isEmpty();
  QString detail;
  if (detected) {
    detail = QStringLiteral(
                 "Firmware fan_mode=%1; platform profile=%2 (%3). Direct fan-mode writes are disabled because values are model-dependent and do not expose RPM or a verifiable automatic restore state.")
                 .arg(m_fanMode.isEmpty() ? QStringLiteral("unknown") : m_fanMode,
                      m_profile.isEmpty() ? QStringLiteral("unknown") : m_profile,
                      m_profileChoices.isEmpty() ? QStringLiteral("unknown")
                                                 : m_profileChoices);
  }
  return {QStringLiteral("ideapad-firmware"),
          QStringLiteral("Lenovo IdeaPad/ThinkBook firmware"),
          QStringLiteral("ideapad_laptop"),
          detected ? QStringLiteral("read-only") : QStringLiteral("unsupported"),
          detail, {detected, false, false, false}};
}

bool IdeaPadBackend::probe() {
  m_fans.clear();
  const QDir devices(QDir(m_sysfsRoot).filePath(QStringLiteral("bus/acpi/devices")));
  const auto entries = devices.entryList({QStringLiteral("VPC2004:*")},
                                         QDir::Dirs | QDir::NoDotAndDotDot,
                                         QDir::Name);
  for (const auto &entry : entries) {
    const auto candidate =
        devices.filePath(entry + QStringLiteral("/physical_node/fan_mode"));
    if (QFileInfo::exists(candidate)) {
      m_fanModePath = candidate;
      break;
    }
  }
  if (m_fanModePath.isEmpty())
    return false;

  m_profilePath =
      QDir(m_sysfsRoot).filePath(QStringLiteral("firmware/acpi/platform_profile"));
  m_profileChoicesPath = QDir(m_sysfsRoot).filePath(
      QStringLiteral("firmware/acpi/platform_profile_choices"));
  m_fans.push_back({QStringLiteral("ideapad:VPC2004:fan0"),
                    QStringLiteral("ideapad-firmware"),
                    QStringLiteral("ideapad:VPC2004"),
                    QStringLiteral("ideapad:embedded-controller"),
                    QStringLiteral("Firmware-controlled system fan"),
                    Thermal::DeviceKind::System, -1, -1, false, false, false,
                    false, false, false, true,
                    QStringLiteral("firmware-managed")});
  refresh();
  return true;
}

void IdeaPadBackend::refresh() {
  m_fanMode = readText(m_fanModePath);
  m_profile = readText(m_profilePath);
  m_profileChoices = readText(m_profileChoicesPath);
}

QList<Thermal::TemperatureSensor> IdeaPadBackend::sensors() const { return {}; }
QList<Thermal::FanDevice> IdeaPadBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> IdeaPadBackend::zones() const { return {}; }
Thermal::Result IdeaPadBackend::setAutomatic(const QString &) {
  return Thermal::Result::fail(
      QStringLiteral("ideapad.unverified"),
      QStringLiteral("IdeaPad firmware fan writes are disabled for this model."));
}
Thermal::Result IdeaPadBackend::setFullSpeed(const QString &zoneId) {
  return setAutomatic(zoneId);
}
Thermal::Result IdeaPadBackend::setDuty(const QString &zoneId, int) {
  return setAutomatic(zoneId);
}
