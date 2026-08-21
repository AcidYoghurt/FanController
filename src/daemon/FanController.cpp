#include "FanController.h"

#include "Constants.h"
#include "FanCurveConfig.h"
#include "backends/ClevoBackend.h"
#include "backends/HwmonBackend.h"
#include "backends/IdeaPadBackend.h"
#include "backends/IFanBackend.h"
#include "backends/MockBackend.h"
#include "backends/NvidiaBackend.h"
#include "backends/ThermalZoneBackend.h"
#include "backends/ThinkPadBackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QtMath>

#include <algorithm>
#include <numeric>

namespace {
constexpr int MinimumCurvePointCount = 2;
constexpr int MaximumCurvePointCount = 32;
constexpr int DutyDecreaseStep = 5;
constexpr int DutyHysteresis = 5;
constexpr int TemperatureAverageSampleCount = 8;
constexpr qint64 DutyDecreaseIntervalMs = 5000;

QVariantMap curvePoint(int temperature, int duty) {
  return {{QStringLiteral("temperature"), temperature},
          {QStringLiteral("duty"), duty}};
}
int pointTemperature(const QVariant &value) {
  return value.toMap().value(QStringLiteral("temperature"), -1).toInt();
}
int pointDuty(const QVariant &value) {
  return value.toMap().value(QStringLiteral("duty"), -1).toInt();
}
QString readTextFile(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}

bool verifiedClevoHardware(const QString &sysfsRoot) {
  const QDir root(sysfsRoot);
  const auto vendor = readTextFile(
      root.filePath(QStringLiteral("class/dmi/id/sys_vendor")));
  const auto product = readTextFile(
      root.filePath(QStringLiteral("class/dmi/id/product_name")));
  const auto path = readTextFile(root.filePath(
      QStringLiteral("bus/acpi/devices/CLV0001:00/path")));
  return vendor.compare(QStringLiteral("COLORFUL"), Qt::CaseInsensitive) == 0 &&
         product.compare(QStringLiteral("P16 TA 24"),
                         Qt::CaseInsensitive) == 0 &&
         (path == QStringLiteral("\\_SB_.DCHU") ||
          path == QStringLiteral("\\_SB.DCHU"));
}
QJsonArray curveToJson(const QVariantList &curve) {
  QJsonArray array;
  for (const auto &point : curve) {
    const auto map = point.toMap();
    array.push_back(QJsonObject{
        {QStringLiteral("temperature"), map.value(QStringLiteral("temperature")).toInt()},
        {QStringLiteral("duty"), map.value(QStringLiteral("duty")).toInt()}});
  }
  return array;
}
QVariantList curveFromJson(const QJsonArray &array) {
  QVariantList result;
  for (const auto &value : array) {
    const auto object = value.toObject();
    result.push_back(curvePoint(object.value(QStringLiteral("temperature")).toInt(-1),
                                object.value(QStringLiteral("duty")).toInt(-1)));
  }
  return result;
}
} // namespace

FanController::FanController(bool mockMode, QString curvePath, QObject *parent)
    : QObject(parent),
      m_curvePath(curvePath.isEmpty()
                      ? (mockMode
                             ? QDir::temp().filePath(
                                   QStringLiteral("thermal-control-mock-%1.json")
                                       .arg(QCoreApplication::applicationPid()))
                             : QStringLiteral("/var/lib/linux-thermal-control/curves.json"))
                      : std::move(curvePath)),
      m_mockMode(mockMode) {
  m_telemetryTimer.setInterval(1000);
  connect(&m_telemetryTimer, &QTimer::timeout, this,
          &FanController::refreshTelemetry);
}

bool FanController::initialize(QString *error) {
  m_backends.clear();
  m_mockBackend = nullptr;
  m_state = QStringLiteral("probing");
  refreshMokEnrollment();

  if (m_mockMode) {
    auto backend = std::make_unique<MockBackend>();
    m_mockBackend = backend.get();
    const auto mockReady = backend->probe();
    Q_UNUSED(mockReady)
    m_backends.push_back(std::move(backend));
  } else {
    auto clevo = std::make_unique<ClevoBackend>();
    if (clevo->probe())
      m_backends.push_back(std::move(clevo));

    auto thinkpad = std::make_unique<ThinkPadBackend>();
    if (thinkpad->probe())
      m_backends.push_back(std::move(thinkpad));

    auto ideapad = std::make_unique<IdeaPadBackend>();
    if (ideapad->probe())
      m_backends.push_back(std::move(ideapad));

    auto hwmon = std::make_unique<HwmonBackend>();
    if (hwmon->probe())
      m_backends.push_back(std::move(hwmon));

    QSet<Thermal::DeviceKind> missingKinds{
        Thermal::DeviceKind::Cpu, Thermal::DeviceKind::Gpu};
    for (const auto &sensor : sensors())
      missingKinds.remove(sensor.kind);
    if (!missingKinds.isEmpty()) {
      auto thermal = std::make_unique<ThermalZoneBackend>(missingKinds);
      if (thermal->probe())
        m_backends.push_back(std::move(thermal));
    }

    auto nvidia = std::make_unique<NvidiaBackend>();
    if (nvidia->probe())
      m_backends.push_back(std::move(nvidia));
  }

  for (const auto &zone : zones()) {
    const auto curve = defaultCurveForZone(zone);
    QString curveError;
    if (!validateCurveForZone(zone, curve, &curveError)) {
      m_state = QStringLiteral("fatal-error");
      if (error)
        *error = QStringLiteral("Unable to load the default fan curve: %1")
                     .arg(curveError);
      return false;
    }
    m_curves.insert(zone.id, curve);
    m_curveRevisions.insert(zone.id, 1);
    m_zoneDuties.insert(zone.id, -1);
  }
  const auto curvesLoaded = loadCurves();
  Q_UNUSED(curvesLoaded)

  const auto detectedFans = fans();
  const auto controllableCount = std::count_if(
      detectedFans.cbegin(), detectedFans.cend(),
      [](const Thermal::FanDevice &fan) { return fan.controllable; });
  m_partialCapability = controllableCount > 0 && controllableCount < detectedFans.size();
  if (m_backends.empty())
    m_state = QStringLiteral("unsupported");
  else if (zones().isEmpty())
    m_state = QStringLiteral("degraded");
  else
    m_state = m_partialCapability ? QStringLiteral("degraded") : QStringLiteral("ready");

  if (!zones().isEmpty() && !restoreAuto(error)) {
    m_state = QStringLiteral("fatal-error");
    return false;
  }
  emit statusChanged(status());
  return true;
}

void FanController::startTelemetry() {
  if (!m_telemetryTimer.isActive())
    m_telemetryTimer.start();
  refreshTelemetry();
}

QList<Thermal::TemperatureSensor> FanController::sensors() const {
  QList<Thermal::TemperatureSensor> result;
  for (const auto &backend : m_backends)
    result.append(backend->sensors());
  return result;
}
QList<Thermal::FanDevice> FanController::fans() const {
  QList<Thermal::FanDevice> result;
  for (const auto &backend : m_backends)
    result.append(backend->fans());
  return result;
}
QList<Thermal::FanZone> FanController::zones() const {
  QList<Thermal::FanZone> result;
  for (const auto &backend : m_backends)
    result.append(backend->zones());

  const auto allSensors = sensors();
  for (auto &zone : result) {
    if (!zone.sensorId.isEmpty())
      continue;
    auto sensor = std::find_if(
        allSensors.cbegin(), allSensors.cend(),
        [&zone](const Thermal::TemperatureSensor &candidate) {
          return candidate.available && candidate.kind == zone.kind;
        });
    if (sensor == allSensors.cend())
      sensor = std::find_if(
          allSensors.cbegin(), allSensors.cend(),
          [](const Thermal::TemperatureSensor &candidate) {
            return candidate.available && candidate.kind == Thermal::DeviceKind::Cpu;
          });
    if (sensor != allSensors.cend())
      zone.sensorId = sensor->id;
    if (zone.sensorId.isEmpty())
      zone.capabilities.customCurve = false;
  }
  return result;
}

IFanBackend *FanController::backendForZone(const QString &zoneId) const {
  for (const auto &backend : m_backends) {
    const auto backendZones = backend->zones();
    if (std::any_of(backendZones.cbegin(), backendZones.cend(),
                    [&zoneId](const Thermal::FanZone &zone) {
                      return zone.id == zoneId;
                    }))
      return backend.get();
  }
  return nullptr;
}

Thermal::FanZone FanController::zoneById(const QString &zoneId) const {
  const auto allZones = zones();
  const auto iterator = std::find_if(allZones.cbegin(), allZones.cend(),
                                    [&zoneId](const Thermal::FanZone &zone) {
                                      return zone.id == zoneId;
                                    });
  return iterator == allZones.cend() ? Thermal::FanZone{} : *iterator;
}

QString FanController::primaryZoneId() const {
  const auto allZones = zones();
  return allZones.isEmpty() ? QString() : allZones.constFirst().id;
}

QVariantList FanController::curve() const { return curveForZone(primaryZoneId()); }
QVariantList FanController::curveForZone(const QString &zoneId) const {
  return m_curves.value(zoneId);
}

bool FanController::setMode(Mode mode, QString *error) {
  if (mode == Mode::Unknown) {
    if (error)
      *error = QStringLiteral("Unknown thermal-control mode.");
    return false;
  }
  if (!applyModeToZones(mode, error))
    return false;
  m_mode = mode;
  m_lastError.clear();
  m_lastErrorCode.clear();
  emit modeChanged(modeToString(mode));
  emit statusChanged(status());
  return true;
}

bool FanController::applyModeToZones(Mode mode, QString *error) {
  const auto allZones = zones();
  int applicable = 0;
  for (const auto &zone : allZones) {
    IFanBackend *backend = backendForZone(zone.id);
    if (!backend)
      continue;
    Thermal::Result result;
    if (mode == Mode::Automatic && zone.capabilities.automatic) {
      ++applicable;
      result = backend->setAutomatic(zone.id);
      m_zoneDuties[zone.id] = -1;
    } else if (mode == Mode::FullSpeed && zone.capabilities.fullSpeed) {
      ++applicable;
      result = backend->setFullSpeed(zone.id);
      m_zoneDuties[zone.id] = 100;
    } else if (mode == Mode::CustomCurve && zone.capabilities.customCurve) {
      ++applicable;
      // A deliberate mode change must apply the curve target immediately.
      // Keeping the previous full-speed duty would otherwise invoke the
      // gradual 5%/5s cooldown path and leave the fan near 100% for a minute.
      m_zoneDuties[zone.id] = -1;
      m_controlTemperatures.remove(zone.id);
      m_temperatureSamples.remove(zone.id);
      m_lastDutyDecrease.remove(zone.id);
      continue;
    } else {
      continue;
    }
    if (!result.success) {
      m_lastErrorCode = result.code;
      m_lastError = result.message;
      if (error)
        *error = result.message;
      return false;
    }
  }
  if (applicable == 0) {
    const auto message = QStringLiteral("No detected fan zone supports the requested mode.");
    m_lastErrorCode = QStringLiteral("mode.unsupported");
    m_lastError = message;
    if (error)
      *error = message;
    return false;
  }
  if (mode == Mode::CustomCurve)
    return applyCustomControl(error);
  m_safetyOverride = false;
  return true;
}

bool FanController::setCurve(const QVariantList &curve, QString *error) {
  return setCurveForZone(primaryZoneId(), curve, error);
}

bool FanController::setCurveForZone(const QString &zoneId,
                                    const QVariantList &curve, QString *error) {
  const auto zone = zoneById(zoneId);
  if (zone.id.isEmpty()) {
    if (error)
      *error = QStringLiteral("Unknown fan zone.");
    return false;
  }
  if (!validateCurveForZone(zone, curve, error))
    return false;
  const auto previous = m_curves.value(zoneId);
  m_curves.insert(zoneId, normalizedCurve(curve));
  if (!saveCurves(error)) {
    m_curves.insert(zoneId, previous);
    return false;
  }
  if (m_mode == Mode::CustomCurve && !applyCustomControl(error)) {
    m_curves.insert(zoneId, previous);
    const auto previousSaved = saveCurves(nullptr);
    Q_UNUSED(previousSaved)
    return false;
  }
  m_curveRevisions[zoneId] = m_curveRevisions.value(zoneId, 0) + 1;
  emit curveChanged(m_curves.value(zoneId));
  emit curvesChanged(curveProfiles());
  emit statusChanged(status());
  return true;
}

bool FanController::resetCurve(QString *error) {
  return resetCurveForZone(primaryZoneId(), error);
}
bool FanController::resetCurveForZone(const QString &zoneId, QString *error) {
  const auto zone = zoneById(zoneId);
  if (zone.id.isEmpty()) {
    if (error)
      *error = QStringLiteral("Unknown fan zone.");
    return false;
  }
  const auto curve = defaultCurveForZone(zone);
  if (curve.isEmpty()) {
    if (error)
      *error = QStringLiteral("Unable to load .风扇曲线配置.backup.");
    return false;
  }
  return setCurveForZone(zoneId, curve, error);
}
bool FanController::restoreAuto(QString *error) {
  return setMode(Mode::Automatic, error);
}

void FanController::prepareForSleep(bool sleeping) {
  if (m_preparingForSleep == sleeping)
    return;
  m_preparingForSleep = sleeping;
  if (sleeping) {
    for (const auto &zone : zones())
      if (zone.capabilities.automatic)
        if (auto *backend = backendForZone(zone.id))
          backend->setAutomatic(zone.id);
    return;
  }
  QString error;
  const auto resumeMode = m_mode;
  if (!applyModeToZones(resumeMode, &error)) {
    m_lastErrorCode = QStringLiteral("resume.restore-failed");
    m_lastError = error;
    m_mode = Mode::Automatic;
  }
  emit statusChanged(status());
}

bool FanController::setMockSensorFailure(bool enabled) {
  if (!m_mockBackend)
    return false;
  m_mockBackend->setSensorFailure(enabled);
  refreshTelemetry();
  return true;
}
bool FanController::setMockSensorTemperature(const QString &sensorId,
                                             double temperature) {
  if (!m_mockBackend ||
      !m_mockBackend->setSensorTemperature(sensorId, temperature))
    return false;
  refreshTelemetry();
  return true;
}
bool FanController::setMockCommandFailure() {
  if (!m_mockBackend)
    return false;
  m_mockBackend->failNextCommand();
  return true;
}

void FanController::refreshTelemetry() {
  for (const auto &backend : m_backends)
    backend->refresh();
  if (m_mode == Mode::CustomCurve) {
    QString error;
    if (!applyCustomControl(&error)) {
      m_lastErrorCode = QStringLiteral("custom.apply-failed");
      m_lastError = error;
    }
  }
  emit statusChanged(status());
}

bool FanController::applyCustomControl(QString *error) {
  const auto allSensors = sensors();
  bool anyApplied = false;
  bool anySafety = false;
  for (const auto &zone : zones()) {
    if (!zone.capabilities.customCurve)
      continue;
    auto *backend = backendForZone(zone.id);
    if (!backend)
      continue;
    const auto sensor = std::find_if(
        allSensors.cbegin(), allSensors.cend(), [&zone](const auto &candidate) {
          return candidate.id == zone.sensorId;
        });
    if (sensor == allSensors.cend() || !sensor->available) {
      const auto result = zone.capabilities.fullSpeed
                              ? backend->setFullSpeed(zone.id)
                              : backend->setDuty(zone.id, zone.maximumDuty);
      if (!result.success) {
        if (error)
          *error = result.message;
        return false;
      }
      m_zoneDuties[zone.id] = 100;
      anySafety = true;
      anyApplied = true;
      continue;
    }

    auto &temperatureSamples = m_temperatureSamples[zone.id];
    if (temperatureSamples.isEmpty()) {
      // Start with a complete baseline window. A mode switch must not give the
      // first isolated reading disproportionate weight in the moving average.
      temperatureSamples.fill(sensor->temperature,
                              TemperatureAverageSampleCount);
    } else {
      temperatureSamples.push_back(sensor->temperature);
    }
    while (temperatureSamples.size() > TemperatureAverageSampleCount)
      temperatureSamples.removeFirst();

    auto &controlTemperature = m_controlTemperatures[zone.id];
    controlTemperature = std::accumulate(temperatureSamples.cbegin(),
                                         temperatureSamples.cend(), 0.0) /
                         temperatureSamples.size();
    const bool safetyTemperatureReached =
        sensor->temperature >= zone.safetyTemperature;
    int targetDuty = safetyTemperatureReached
                         ? 100
                         : dutyForTemperature(m_curves.value(zone.id),
                                              controlTemperature,
                                              zone.minimumDuty);
    const int currentDuty = m_zoneDuties.value(zone.id, -1);
    bool shouldWrite = currentDuty < 0 || targetDuty == 100 ||
                       targetDuty - currentDuty >= DutyHysteresis;
    if (targetDuty < currentDuty) {
      shouldWrite = false;
      if (currentDuty - targetDuty >= DutyHysteresis) {
        auto &timer = m_lastDutyDecrease[zone.id];
        if (!timer.isValid() || timer.elapsed() >= DutyDecreaseIntervalMs) {
          targetDuty = qMax(targetDuty, currentDuty - DutyDecreaseStep);
          shouldWrite = true;
          timer.restart();
        }
      }
    }
    if (shouldWrite) {
      const auto result = backend->setDuty(zone.id, targetDuty);
      if (!result.success) {
        if (error)
          *error = result.message;
        return false;
      }
      m_zoneDuties[zone.id] = targetDuty;
      m_lastDutyDecrease[zone.id].restart();
    }
    anySafety |= safetyTemperatureReached;
    anyApplied = true;
  }
  m_safetyOverride = anySafety;
  if (!anyApplied) {
    if (error)
      *error = QStringLiteral("No fan zone supports custom curves.");
    return false;
  }
  return true;
}

QVariantMap FanController::detectSecureBoot(QString sysfsRoot) {
  if (sysfsRoot.isEmpty())
    sysfsRoot = qEnvironmentVariable("THERMAL_CONTROL_SYSFS_ROOT",
                                     QStringLiteral("/sys"));
  const QDir root(sysfsRoot);
  const auto efiPath = root.filePath(QStringLiteral("firmware/efi"));
  if (!QFileInfo::exists(efiPath)) {
    return {{QStringLiteral("state"), QStringLiteral("unsupported")},
            {QStringLiteral("known"), true},
            {QStringLiteral("enabled"), false},
            {QStringLiteral("disabled"), false}};
  }

  const QDir variables(root.filePath(QStringLiteral("firmware/efi/efivars")));
  const auto entries = variables.entryList(
      {QStringLiteral("SecureBoot-*")}, QDir::Files, QDir::Name);
  if (entries.isEmpty()) {
    return {{QStringLiteral("state"), QStringLiteral("unknown")},
            {QStringLiteral("known"), false},
            {QStringLiteral("enabled"), false},
            {QStringLiteral("disabled"), false}};
  }

  QFile variable(variables.filePath(entries.constFirst()));
  if (!variable.open(QIODevice::ReadOnly)) {
    return {{QStringLiteral("state"), QStringLiteral("unknown")},
            {QStringLiteral("known"), false},
            {QStringLiteral("enabled"), false},
            {QStringLiteral("disabled"), false}};
  }
  const auto data = variable.read(5);
  if (data.size() < 5) {
    return {{QStringLiteral("state"), QStringLiteral("unknown")},
            {QStringLiteral("known"), false},
            {QStringLiteral("enabled"), false},
            {QStringLiteral("disabled"), false}};
  }
  const bool enabled = static_cast<unsigned char>(data.at(4)) != 0;
  return {{QStringLiteral("state"),
           enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")},
          {QStringLiteral("known"), true},
          {QStringLiteral("enabled"), enabled},
          {QStringLiteral("disabled"), !enabled}};
}

QVariantMap FanController::parseMokEnrollmentResult(
    const QByteArray &testKeyOutput, const QByteArray &pendingOutput,
    bool certificateExists) {
  if (!certificateExists) {
    return {{QStringLiteral("state"), QStringLiteral("not-generated")},
            {QStringLiteral("known"), true},
            {QStringLiteral("enrolled"), false}};
  }
  if (testKeyOutput.contains("is already enrolled")) {
    return {{QStringLiteral("state"), QStringLiteral("enrolled")},
            {QStringLiteral("known"), true},
            {QStringLiteral("enrolled"), true}};
  }
  if (!pendingOutput.trimmed().isEmpty()) {
    return {{QStringLiteral("state"), QStringLiteral("pending")},
            {QStringLiteral("known"), true},
            {QStringLiteral("enrolled"), false}};
  }
  if (testKeyOutput.contains("is not enrolled")) {
    return {{QStringLiteral("state"), QStringLiteral("not-enrolled")},
            {QStringLiteral("known"), true},
            {QStringLiteral("enrolled"), false}};
  }
  return {{QStringLiteral("state"), QStringLiteral("unknown")},
          {QStringLiteral("known"), false},
          {QStringLiteral("enrolled"), false}};
}

void FanController::refreshMokEnrollment() {
  if (m_mockMode)
    return;

  const auto certificate = qEnvironmentVariable(
      "THERMAL_CONTROL_MOK_CERTIFICATE",
      QStringLiteral("/var/lib/shim-signed/mok/MOK.der"));
  QByteArray testKeyOutput;
  QByteArray pendingOutput;
  if (QFileInfo::exists(certificate)) {
    QProcess testKey;
    testKey.start(QStringLiteral("/usr/bin/mokutil"),
                  {QStringLiteral("--test-key"), certificate});
    if (testKey.waitForFinished(5000))
      testKeyOutput =
          testKey.readAllStandardOutput() + testKey.readAllStandardError();

    QProcess pending;
    pending.start(QStringLiteral("/usr/bin/mokutil"),
                  {QStringLiteral("--list-new")});
    if (pending.waitForFinished(5000) && pending.exitCode() == 0)
      pendingOutput = pending.readAllStandardOutput();
  }

  const auto result = parseMokEnrollmentResult(
      testKeyOutput, pendingOutput, QFileInfo::exists(certificate));
  m_mokState = result.value(QStringLiteral("state")).toString();
  m_mokKnown = result.value(QStringLiteral("known")).toBool();
  m_mokEnrolled = result.value(QStringLiteral("enrolled")).toBool();
}

QVariantMap FanController::status() const {
  const auto sysfsRoot = qEnvironmentVariable(
      "THERMAL_CONTROL_SYSFS_ROOT", QStringLiteral("/sys"));
  auto secureBoot = detectSecureBoot(sysfsRoot);
  const bool clevoRelevant = verifiedClevoHardware(sysfsRoot);
  const auto procRoot = qEnvironmentVariable(
      "THERMAL_CONTROL_PROC_ROOT", QStringLiteral("/proc"));
  const bool acpiCallAvailable = QFileInfo::exists(
      QDir(procRoot).filePath(QStringLiteral("acpi/call")));
  const bool secureBootEnabled =
      secureBoot.value(QStringLiteral("enabled")).toBool();
  QString requirement = QStringLiteral("not-required");
  if (!secureBoot.value(QStringLiteral("known")).toBool())
    requirement = clevoRelevant ? QStringLiteral("unknown")
                                : QStringLiteral("not-required");
  else if (secureBootEnabled && clevoRelevant && !acpiCallAvailable)
    requirement = QStringLiteral("enroll-key");
  secureBoot.insert(QStringLiteral("requirement"), requirement);
  secureBoot.insert(QStringLiteral("clevoRelevant"), clevoRelevant);
  secureBoot.insert(QStringLiteral("acpiCallAvailable"), acpiCallAvailable);
  secureBoot.insert(QStringLiteral("mokState"), m_mokState);
  secureBoot.insert(QStringLiteral("mokKnown"), m_mokKnown);
  secureBoot.insert(QStringLiteral("mokEnrolled"), m_mokEnrolled);

  QVariantList sensorMaps;
  for (const auto &sensor : sensors())
    sensorMaps.push_back(Thermal::toMap(sensor));
  QVariantList fanMaps;
  const auto allZones = zones();
  for (auto fan : fans()) {
    const auto zone = std::find_if(allZones.cbegin(), allZones.cend(),
                                   [&fan](const Thermal::FanZone &candidate) {
                                     return candidate.targetFanIds.contains(fan.id);
                                   });
    if (zone != allZones.cend() && m_zoneDuties.contains(zone->id))
      fan.duty = m_zoneDuties.value(zone->id);
    fanMaps.push_back(Thermal::toMap(fan));
  }
  QVariantList zoneMaps;
  for (auto zone : zones()) {
    auto map = Thermal::toMap(zone);
    map.insert(QStringLiteral("currentDuty"), m_zoneDuties.value(zone.id, -1));
    map.insert(QStringLiteral("controlTemperature"),
               m_controlTemperatures.value(zone.id, -1.0));
    zoneMaps.push_back(map);
  }
  QVariantList backendMaps;
  for (const auto &backend : m_backends)
    backendMaps.push_back(Thermal::toMap(backend->info()));

  return {{QStringLiteral("version"), QString::fromLatin1(LinuxThermal::Constants::Version)},
          {QStringLiteral("systemVendor"),
           readTextFile(QDir(sysfsRoot).filePath(
               QStringLiteral("class/dmi/id/sys_vendor")))},
          {QStringLiteral("productName"),
           readTextFile(QDir(sysfsRoot).filePath(
               QStringLiteral("class/dmi/id/product_name")))},
          {QStringLiteral("state"), m_state},
          {QStringLiteral("mode"), modeToString(m_mode)},
          {QStringLiteral("partialCapability"), m_partialCapability},
          {QStringLiteral("secureBoot"), secureBoot},
          {QStringLiteral("safetyOverride"), m_safetyOverride},
          {QStringLiteral("sensors"), sensorMaps},
          {QStringLiteral("fans"), fanMaps},
          {QStringLiteral("zones"), zoneMaps},
          {QStringLiteral("backends"), backendMaps},
          {QStringLiteral("curveProfiles"), curveProfiles()},
          {QStringLiteral("lastErrorCode"), m_lastErrorCode},
          {QStringLiteral("lastError"), m_lastError}};
}

QVariantList FanController::curveProfiles() const {
  QVariantList profiles;
  for (const auto &zone : zones()) {
    auto profile = Thermal::toMap(zone);
    profile.insert(QStringLiteral("curveId"), zone.id + QStringLiteral(":curve"));
    profile.insert(QStringLiteral("points"), m_curves.value(zone.id));
    profile.insert(QStringLiteral("revision"), m_curveRevisions.value(zone.id, 1));
    profiles.push_back(profile);
  }
  return profiles;
}

FanController::Mode FanController::modeFromString(QStringView value) {
  switch (Thermal::modeFromString(value)) {
  case Thermal::ControlMode::Automatic: return Mode::Automatic;
  case Thermal::ControlMode::FullSpeed: return Mode::FullSpeed;
  case Thermal::ControlMode::CustomCurve: return Mode::CustomCurve;
  case Thermal::ControlMode::Unknown: return Mode::Unknown;
  }
  return Mode::Unknown;
}
QString FanController::modeToString(Mode mode) {
  switch (mode) {
  case Mode::Automatic: return QStringLiteral("automatic");
  case Mode::FullSpeed: return QStringLiteral("full-speed");
  case Mode::CustomCurve: return QStringLiteral("custom-curve");
  case Mode::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

QVariantList FanController::defaultCurve() {
  Thermal::FanZone zone;
  zone.minimumDuty = 40;
  FanController temporary(true);
  return temporary.defaultCurveForZone(zone);
}
QVariantList FanController::defaultCurveForZone(const Thermal::FanZone &zone) const {
  const auto reference = FanCurveConfig::loadDefault();
  if (reference.isEmpty())
    return {};

  QVariantList curve;
  curve.reserve(reference.size());
  for (const auto &value : reference) {
    const auto point = value.toMap();
    const int temperature = point.value(QStringLiteral("temperature")).toInt();
    const int duty = point.value(QStringLiteral("duty")).toInt();
    const int boundedTemperature =
        qBound(zone.minimumTemperature, temperature, zone.maximumTemperature);
    const int minimumDuty = zone.zeroRpmAllowed && duty == 0 ? 0 : zone.minimumDuty;
    const int boundedDuty = qBound(minimumDuty, duty, zone.maximumDuty);
    curve.push_back(curvePoint(boundedTemperature, boundedDuty));
  }
  return curve;
}

bool FanController::validateCurve(const QVariantList &curve, QString *error) {
  Thermal::FanZone zone;
  zone.minimumDuty = 40;
  zone.maximumDuty = 100;
  zone.minimumTemperature = 30;
  zone.maximumTemperature = 100;
  zone.safetyTemperature = 85;
  FanController temporary(true);
  return temporary.validateCurveForZone(zone, curve, error);
}

bool FanController::validateCurveForZone(const Thermal::FanZone &zone,
                                         const QVariantList &curve,
                                         QString *error) const {
  if (curve.size() < MinimumCurvePointCount ||
      curve.size() > MaximumCurvePointCount) {
    if (error)
      *error = QStringLiteral("A fan curve must contain between two and 32 points.");
    return false;
  }
  int previousTemperature = -1;
  int previousDuty = -1;
  for (const auto &point : curve) {
    const int temperature = pointTemperature(point);
    const int duty = pointDuty(point);
    if (temperature < zone.minimumTemperature || temperature > zone.maximumTemperature ||
        duty < zone.minimumDuty || duty > zone.maximumDuty ||
        temperature <= previousTemperature || duty < previousDuty) {
      if (error)
        *error = QStringLiteral("Curve points must increase and remain inside the zone safety limits.");
      return false;
    }
    previousTemperature = temperature;
    previousDuty = duty;
  }
  const int endpointTemperature = pointTemperature(curve.constLast());
  const bool validEndpoint =
      endpointTemperature == zone.maximumTemperature ||
      endpointTemperature == zone.safetyTemperature;
  if (!validEndpoint || pointDuty(curve.constLast()) != zone.maximumDuty) {
    if (error)
      *error = QStringLiteral("The curve endpoint must be fixed at the zone maximum temperature and maximum duty.");
    return false;
  }
  return true;
}

int FanController::dutyForTemperature(const QVariantList &curve,
                                      double temperature, int minimumDuty) {
  if (curve.isEmpty())
    return 100;
  if (temperature <= pointTemperature(curve.constFirst()))
    return qBound(minimumDuty, pointDuty(curve.constFirst()), 100);
  for (qsizetype index = 1; index < curve.size(); ++index) {
    const int lowerTemperature = pointTemperature(curve.at(index - 1));
    const int lowerDuty = pointDuty(curve.at(index - 1));
    const int upperTemperature = pointTemperature(curve.at(index));
    const int upperDuty = pointDuty(curve.at(index));
    if (temperature <= upperTemperature) {
      const auto ratio = (temperature - lowerTemperature) /
                         static_cast<double>(upperTemperature - lowerTemperature);
      return qBound(minimumDuty, qRound(lowerDuty + ratio * (upperDuty - lowerDuty)), 100);
    }
  }
  return 100;
}

QVariantList FanController::normalizedCurve(const QVariantList &curve) {
  QVariantList result;
  result.reserve(curve.size());
  for (const auto &point : curve)
    result.push_back(curvePoint(pointTemperature(point), pointDuty(point)));
  return result;
}

bool FanController::loadCurves() {
  QFile file(m_curvePath);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  const auto document = QJsonDocument::fromJson(file.readAll());
  if (document.isArray()) {
    const auto zoneId = primaryZoneId();
    const auto loaded = curveFromJson(document.array());
    const auto zone = zoneById(zoneId);
    if (!zone.id.isEmpty() && validateCurveForZone(zone, loaded, nullptr))
      m_curves.insert(zoneId, loaded);
    return true;
  }
  if (!document.isObject())
    return false;
  const auto object = document.object();
  for (const auto &zone : zones()) {
    const auto loaded = curveFromJson(object.value(zone.id).toArray());
    if (validateCurveForZone(zone, loaded, nullptr))
      m_curves.insert(zone.id, loaded);
  }
  return true;
}

bool FanController::saveCurves(QString *error) const {
  if (!QDir().mkpath(QFileInfo(m_curvePath).absolutePath())) {
    if (error)
      *error = QStringLiteral("Unable to create the curve configuration directory.");
    return false;
  }
  QJsonDocument document;
  if (m_curvePath != QStringLiteral("/var/lib/linux-thermal-control/curves.json") &&
      m_curves.size() == 1) {
    document = QJsonDocument(curveToJson(m_curves.constBegin().value()));
  } else {
    QJsonObject object;
    for (auto iterator = m_curves.cbegin(); iterator != m_curves.cend(); ++iterator)
      object.insert(iterator.key(), curveToJson(iterator.value()));
    document = QJsonDocument(object);
  }
  QSaveFile file(m_curvePath);
  if (!file.open(QIODevice::WriteOnly)) {
    if (error)
      *error = file.errorString();
    return false;
  }
  file.write(document.toJson(QJsonDocument::Indented));
  if (!file.commit()) {
    if (error)
      *error = file.errorString();
    return false;
  }
  return true;
}
