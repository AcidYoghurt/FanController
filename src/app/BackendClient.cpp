#include "BackendClient.h"

#include "Constants.h"
#include "DbusUtils.h"
#include "TemperatureSensorSelector.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QtQml/QJSValue>

namespace {
QVariant scalarFromQml(const QVariant &value) {
  return value.metaType() == QMetaType::fromType<QJSValue>()
             ? value.value<QJSValue>().toVariant()
             : value;
}
QVariantList normalizeCurve(const QVariantList &curve) {
  QVariantList result;
  for (const auto &rawPoint : curve) {
    const auto point = scalarFromQml(rawPoint).toMap();
    result.push_back(QVariantMap{
        {QStringLiteral("temperature"), scalarFromQml(point.value(QStringLiteral("temperature"))).toInt()},
        {QStringLiteral("duty"), scalarFromQml(point.value(QStringLiteral("duty"))).toInt()}});
  }
  return result;
}
} // namespace

BackendClient::BackendClient(QObject *parent)
    : QObject(parent),
      m_interface(QString::fromLatin1(LinuxThermal::Constants::ServiceName),
                  QString::fromLatin1(LinuxThermal::Constants::ObjectPath),
                  QString::fromLatin1(LinuxThermal::Constants::InterfaceName),
                  QDBusConnection::systemBus()),
      m_sensors(this), m_fans(this), m_zones(this), m_backends(this),
      m_curvePoints(this) {
  auto *watcher = new QDBusServiceWatcher(
      QString::fromLatin1(LinuxThermal::Constants::ServiceName),
      QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange,
      this);
  connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
          &BackendClient::handleServiceOwnerChanged);
  auto bus = QDBusConnection::systemBus();
  bus.connect(QString::fromLatin1(LinuxThermal::Constants::ServiceName),
              QString::fromLatin1(LinuxThermal::Constants::ObjectPath),
              QString::fromLatin1(LinuxThermal::Constants::InterfaceName),
              QStringLiteral("StatusChanged"), this,
              SLOT(handleRemoteStatusChanged(QVariantMap)));
  bus.connect(QString::fromLatin1(LinuxThermal::Constants::ServiceName),
              QString::fromLatin1(LinuxThermal::Constants::ObjectPath),
              QString::fromLatin1(LinuxThermal::Constants::InterfaceName),
              QStringLiteral("CurvesChanged"), this,
              SLOT(handleRemoteCurvesChanged(QVariantList)));
  m_interface.setTimeout(130000);
  m_refreshTimer.setInterval(2000);
  connect(&m_refreshTimer, &QTimer::timeout, this,
          [this] { requestRefresh(false); });
  m_refreshTimer.start();
  requestRefresh(false);
}

QString BackendClient::systemName() const {
  const auto vendor = m_status.value(QStringLiteral("systemVendor")).toString();
  const auto product = m_status.value(QStringLiteral("productName")).toString();
  return QStringLiteral("%1 %2").arg(vendor, product).trimmed();
}

QVariantMap BackendClient::secureBootMap() const {
  return m_status.value(QStringLiteral("secureBoot")).toMap();
}

QString BackendClient::secureBootState() const {
  return secureBootMap().value(QStringLiteral("state"),
                               QStringLiteral("unknown")).toString();
}

QString BackendClient::secureBootRequirement() const {
  return secureBootMap().value(QStringLiteral("requirement"),
                               QStringLiteral("unknown")).toString();
}

bool BackendClient::secureBootKnown() const {
  return secureBootMap().value(QStringLiteral("known")).toBool();
}

bool BackendClient::secureBootEnabled() const {
  return secureBootMap().value(QStringLiteral("enabled")).toBool();
}

bool BackendClient::secureBootDisabled() const {
  return secureBootMap().value(QStringLiteral("disabled")).toBool();
}

bool BackendClient::secureBootDisableRequired() const {
  return secureBootRequirement() == QStringLiteral("disable-or-enroll-key");
}

QString BackendClient::safetyMessage() const {
  if (!safetyOverride())
    return {};
  return QStringLiteral("A temperature sensor reached its safety limit or became unavailable. Affected controllable fans are running at full speed.");
}
QVariantMap BackendClient::selectedProfile() const {
  for (const auto &value : m_profiles) {
    const auto profile = value.toMap();
    if (profile.value(QStringLiteral("zoneId")).toString() == m_selectedZoneId)
      return profile;
  }
  return {};
}
QString BackendClient::selectedZoneName() const {
  return selectedProfile().value(QStringLiteral("displayName")).toString();
}
int BackendClient::minimumDuty() const {
  return selectedProfile().value(QStringLiteral("minimumDuty"), 40).toInt();
}
int BackendClient::safetyTemperature() const {
  return selectedProfile().value(QStringLiteral("safetyTemperature"), 85).toInt();
}
bool BackendClient::curveSupported() const {
  return selectedProfile().value(QStringLiteral("curveSupported")).toBool();
}
bool BackendClient::automaticAvailable() const {
  for (const auto &value : m_status.value(QStringLiteral("zones")).toList())
    if (value.toMap().value(QStringLiteral("automaticSupported")).toBool())
      return true;
  return false;
}
bool BackendClient::fullSpeedAvailable() const {
  for (const auto &value : m_status.value(QStringLiteral("zones")).toList())
    if (value.toMap().value(QStringLiteral("fullSpeedSupported")).toBool())
      return true;
  return false;
}
bool BackendClient::customCurveAvailable() const {
  for (const auto &value : m_status.value(QStringLiteral("zones")).toList())
    if (value.toMap().value(QStringLiteral("curveSupported")).toBool())
      return true;
  return false;
}
bool BackendClient::canSetMode(const QString &targetMode) const {
  if (!m_connected || m_busy || targetMode == mode())
    return false;
  if (targetMode == QStringLiteral("automatic"))
    return automaticAvailable();
  if (targetMode == QStringLiteral("full-speed"))
    return fullSpeedAvailable();
  if (targetMode == QStringLiteral("custom-curve"))
    return customCurveAvailable();
  return false;
}
double BackendClient::selectedTemperature() const {
  const auto sensorId = selectedProfile().value(QStringLiteral("sensorId")).toString();
  const auto sensorValues = m_status.value(QStringLiteral("sensors")).toList();
  for (const auto &value : sensorValues) {
    const auto sensor = value.toMap();
    if (sensor.value(QStringLiteral("sensorId")).toString() == sensorId &&
        sensor.value(QStringLiteral("available")).toBool())
      return sensor.value(QStringLiteral("temperature")).toDouble();
  }
  return -1.0;
}
int BackendClient::selectedDuty() const {
  const auto zoneValues = m_status.value(QStringLiteral("zones")).toList();
  for (const auto &value : zoneValues) {
    const auto zone = value.toMap();
    if (zone.value(QStringLiteral("zoneId")).toString() == m_selectedZoneId)
      return zone.value(QStringLiteral("currentDuty"), -1).toInt();
  }
  return -1;
}

void BackendClient::refresh() { requestRefresh(true); }

void BackendClient::requestRefresh(bool visible) {
  if (visible)
    setRefreshing(true);
  if (m_refreshInFlight)
    return;

  m_refreshInFlight = true;
  auto *watcher = new QDBusPendingCallWatcher(
      m_interface.asyncCall(QStringLiteral("GetStatus")), this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
    const QDBusPendingReply<QVariantMap> reply = *watcher;
    watcher->deleteLater();
    m_refreshInFlight = false;
    setRefreshing(false);
    if (reply.isError()) {
      setConnected(false);
      return;
    }
    setConnected(true);
    applyStatus(LinuxThermal::Dbus::unwrapMap(reply.value()));
  });
}

void BackendClient::setMode(const QString &mode) {
  if (m_busy || !m_connected)
    return;
  setBusy(true);
  clearError();
  watchBooleanReply(m_interface.asyncCall(QStringLiteral("SetMode"), mode),
                    [this, mode](bool success) {
                      emit modeRequestFinished(success, mode);
                      if (success)
                        requestRefresh(false);
                    });
}

bool BackendClient::selectZone(const QString &zoneId) {
  if (zoneId == m_selectedZoneId)
    return true;
  if (m_curvePoints.dirty()) {
    setError(tr("Save or discard the current curve changes before switching zones."));
    return false;
  }
  m_selectedZoneId = zoneId;
  if (m_curveConflict) {
    m_curveConflict = false;
    emit curveConflictChanged();
  }
  loadSelectedCurve(true);
  emit selectedZoneChanged();
  emit selectedTelemetryChanged();
  return true;
}

void BackendClient::setCurvePoint(int row, int temperature, int duty) {
  const auto previous = m_curvePoints.pointAt(row - 1);
  const auto next = m_curvePoints.pointAt(row + 1);
  const int minimumTemperature = previous.isEmpty()
                                     ? selectedProfile().value(QStringLiteral("minimumTemperature"), 30).toInt()
                                     : previous.value(QStringLiteral("temperature")).toInt() + 1;
  const int maximumTemperature = next.isEmpty()
                                     ? safetyTemperature()
                                     : next.value(QStringLiteral("temperature")).toInt() - 1;
  const int minimumPointDuty = previous.isEmpty()
                                  ? minimumDuty()
                                  : previous.value(QStringLiteral("duty")).toInt();
  const int maximumPointDuty = next.isEmpty()
                                  ? 100
                                  : next.value(QStringLiteral("duty")).toInt();
  m_curvePoints.setPoint(row, qBound(minimumTemperature, temperature, maximumTemperature),
                         qBound(minimumPointDuty, duty, maximumPointDuty));
}

void BackendClient::saveCurve() {
  if (m_busy || m_selectedZoneId.isEmpty() || !m_curvePoints.dirty())
    return;
  setBusy(true);
  clearError();
  watchBooleanReply(
      m_interface.asyncCall(QStringLiteral("SetZoneCurve"), m_selectedZoneId,
                            normalizeCurve(m_curvePoints.toVariantList())),
      [this](bool success) {
        emit curveRequestFinished(success);
        if (success) {
          m_curvePoints.markClean();
          requestRefresh(false);
        }
      });
}

void BackendClient::applyCurve() {
  if (m_busy || m_selectedZoneId.isEmpty())
    return;
  setBusy(true);
  clearError();
  watchBooleanReply(
      m_interface.asyncCall(QStringLiteral("ApplyZoneCurve"), m_selectedZoneId,
                            normalizeCurve(m_curvePoints.toVariantList())),
      [this](bool success) {
        emit curveRequestFinished(success);
        if (success) {
          m_curvePoints.markClean();
          requestRefresh(false);
        }
      });
}

void BackendClient::resetCurve() {
  if (m_busy || m_selectedZoneId.isEmpty())
    return;
  setBusy(true);
  clearError();
  watchBooleanReply(
      m_interface.asyncCall(QStringLiteral("ResetZoneCurve"), m_selectedZoneId),
      [this](bool success) {
        emit curveRequestFinished(success);
        if (success) {
          m_curvePoints.markClean();
          requestRefresh(false);
        }
      });
}

void BackendClient::discardCurveEdits() { loadSelectedCurve(true); }
void BackendClient::clearError() { setError({}); }

void BackendClient::handleRemoteStatusChanged(const QVariantMap &status) {
  applyStatus(LinuxThermal::Dbus::unwrapMap(status));
}
void BackendClient::handleRemoteCurvesChanged(const QVariantList &profiles) {
  m_profiles = LinuxThermal::Dbus::unwrapList(profiles);
  loadSelectedCurve(false);
}
void BackendClient::handleServiceOwnerChanged(const QString &, const QString &,
                                               const QString &newOwner) {
  setConnected(!newOwner.isEmpty());
  if (newOwner.isEmpty()) {
    const auto previousMode = mode();
    const auto previousSafety = safetyOverride();
    const auto previousVersion = daemonVersion();
    const auto previousSystemName = systemName();
    m_status.clear();
    m_profiles.clear();
    m_curveBaseline.clear();
    m_selectedZoneId.clear();
    m_selectedCurveRevision = 0;
    m_curvePoints.load({}, 0);
    if (m_curveConflict) {
      m_curveConflict = false;
      emit curveConflictChanged();
    }
    m_sensors.replace({});
    m_fans.replace({});
    m_zones.replace({});
    m_backends.replace({});
    emit stateChanged();
    emit inventoryChanged();
    emit selectedZoneChanged();
    emit selectedTelemetryChanged();
    if (previousMode != mode())
      emit modeChanged();
    if (previousSafety != safetyOverride())
      emit safetyChanged();
    if (previousVersion != daemonVersion())
      emit versionChanged();
    if (previousSystemName != systemName())
      emit systemNameChanged();
  } else {
    requestRefresh(false);
  }
}

void BackendClient::applyStatus(const QVariantMap &status) {
  const auto previousState = state();
  const auto previousPartial = partialCapability();
  const auto previousMode = mode();
  const auto previousSafety = safetyOverride();
  const auto previousVersion = daemonVersion();
  const auto previousSystemName = systemName();
  m_status = status;
  m_sensors.replace(LinuxThermal::TemperatureSensorSelector::selectPrimary(
      status.value(QStringLiteral("sensors")).toList()));
  m_fans.replace(status.value(QStringLiteral("fans")).toList());
  m_zones.replace(status.value(QStringLiteral("zones")).toList());
  m_backends.replace(status.value(QStringLiteral("backends")).toList());
  m_profiles = status.value(QStringLiteral("curveProfiles")).toList();
  if (m_selectedZoneId.isEmpty() && !m_profiles.isEmpty())
    m_selectedZoneId = m_profiles.constFirst().toMap().value(QStringLiteral("zoneId")).toString();
  loadSelectedCurve(false);
  if (previousState != state() || previousPartial != partialCapability())
    emit stateChanged();
  if (previousMode != mode())
    emit modeChanged();
  if (previousSafety != safetyOverride())
    emit safetyChanged();
  if (previousVersion != daemonVersion())
    emit versionChanged();
  if (previousSystemName != systemName())
    emit systemNameChanged();
  emit inventoryChanged();
  emit selectedTelemetryChanged();
  emit selectedZoneChanged();
}

void BackendClient::loadSelectedCurve(bool force) {
  const auto profile = selectedProfile();
  if (profile.isEmpty())
    return;
  const auto points = profile.value(QStringLiteral("points")).toList();
  const int revision = profile.value(QStringLiteral("revision")).toInt();
  if (!force && m_curvePoints.dirty()) {
    if ((revision != m_selectedCurveRevision || points != m_curveBaseline) &&
        !m_curveConflict) {
      m_curveConflict = true;
      emit curveConflictChanged();
    }
    return;
  }
  m_curveBaseline = points;
  m_selectedCurveRevision = revision;
  m_curvePoints.load(points, revision);
  if (m_curveConflict) {
    m_curveConflict = false;
    emit curveConflictChanged();
  }
}

void BackendClient::setRefreshing(bool value) {
  if (m_refreshing == value)
    return;
  m_refreshing = value;
  emit refreshingChanged();
}

void BackendClient::setBusy(bool value) {
  if (m_busy == value)
    return;
  m_busy = value;
  emit busyChanged();
}
void BackendClient::setConnected(bool value) {
  if (m_connected == value)
    return;
  m_connected = value;
  emit connectedChanged();
  emit stateChanged();
}
void BackendClient::setError(const QString &message) {
  if (m_lastError == message)
    return;
  m_lastError = message;
  emit lastErrorChanged();
}
void BackendClient::watchBooleanReply(
    const QDBusPendingCall &call, const std::function<void(bool)> &finished) {
  auto *watcher = new QDBusPendingCallWatcher(call, this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, watcher, finished] {
            const QDBusPendingReply<bool> reply = *watcher;
            watcher->deleteLater();
            const bool success = !reply.isError() && reply.value();
            if (!success)
              setError(reply.isError() ? reply.error().message()
                                       : tr("The daemon rejected the request."));
            setBusy(false);
            finished(success);
          });
}
