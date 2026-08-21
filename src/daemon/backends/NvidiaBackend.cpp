#include "NvidiaBackend.h"

#include <QFileInfo>
#include <QLibrary>
#include <QProcess>
#include <QStandardPaths>
#include <QtMath>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
using NvmlReturn = int;
using NvmlDevice = void *;
// NVML ABI 中固定的成功码、GPU 温度类型和风扇信息结构版本。
constexpr NvmlReturn NvmlSuccess = 0;
constexpr unsigned NvmlTemperatureGpu = 0;
constexpr unsigned NvmlFanInfoVersion = (1u << 24u) | (3u * sizeof(unsigned));

struct NvmlFanSpeedInfo {
  unsigned version;
  unsigned fan;
  unsigned speed;
};

template <typename Function>
Function resolve(QLibrary &library, const char *name) {
  return reinterpret_cast<Function>(library.resolve(name));
}
} // namespace

struct NvidiaBackend::Api {
  QLibrary library;
  NvmlReturn (*init)() = nullptr;
  NvmlReturn (*shutdown)() = nullptr;
  NvmlReturn (*getCount)(unsigned *) = nullptr;
  NvmlReturn (*getHandle)(unsigned, NvmlDevice *) = nullptr;
  NvmlReturn (*getName)(NvmlDevice, char *, unsigned) = nullptr;
  NvmlReturn (*getUuid)(NvmlDevice, char *, unsigned) = nullptr;
  NvmlReturn (*getTemperature)(NvmlDevice, unsigned, unsigned *) = nullptr;
  NvmlReturn (*getNumFans)(NvmlDevice, unsigned *) = nullptr;
  NvmlReturn (*getFanSpeed)(NvmlDevice, unsigned, unsigned *) = nullptr;
  NvmlReturn (*getFanRpm)(NvmlDevice, NvmlFanSpeedInfo *) = nullptr;
  NvmlReturn (*getMinMaxFanSpeed)(NvmlDevice, unsigned *, unsigned *) = nullptr;
  NvmlReturn (*setFanSpeed)(NvmlDevice, unsigned, unsigned) = nullptr;
  NvmlReturn (*setDefaultFanSpeed)(NvmlDevice, unsigned) = nullptr;
  const char *(*errorString)(NvmlReturn) = nullptr;
  bool initialized = false;
};

struct NvidiaBackend::Device {
  NvmlDevice handle = nullptr;
  QString id;
  QString name;
  QString sensorId;
  unsigned fanCount = 0;
  unsigned minimumSpeed = 0;
  unsigned maximumSpeed = 100;
  bool controllable = false;
};

NvidiaBackend::NvidiaBackend() = default;
NvidiaBackend::~NvidiaBackend() {
  if (m_api && m_api->initialized && m_api->shutdown)
    m_api->shutdown();
}

Thermal::BackendInfo NvidiaBackend::info() const {
  const bool ready = !m_sensors.isEmpty();
  Thermal::CapabilitySet capabilities{ready, false, false, false};
  for (const auto &zone : m_zones) {
    capabilities.automatic |= zone.capabilities.automatic;
    capabilities.fullSpeed |= zone.capabilities.fullSpeed;
    capabilities.customCurve |= zone.capabilities.customCurve;
  }
  return {QStringLiteral("nvidia"),
          m_usingNvml ? QStringLiteral("NVIDIA NVML")
                      : QStringLiteral("NVIDIA GPU telemetry"),
          m_usingNvml ? QStringLiteral("libnvidia-ml")
                      : QStringLiteral("nvidia-smi"),
          ready ? (m_zones.isEmpty() ? QStringLiteral("read-only")
                                     : QStringLiteral("ready"))
                : QStringLiteral("unsupported"),
          m_detail, capabilities};
}

bool NvidiaBackend::probe() {
  m_devices.clear();
  m_sensors.clear();
  m_fans.clear();
  m_zones.clear();
  m_usingNvml = false;
  if (probeNvml())
    return true;
  return querySmi();
}

bool NvidiaBackend::probeNvml() {
  auto api = std::make_unique<Api>();
  api->library.setFileName(qEnvironmentVariable(
      "THERMAL_CONTROL_NVML_LIBRARY", QStringLiteral("libnvidia-ml.so.1")));
  if (!api->library.load()) {
    m_detail = QStringLiteral("NVML library is unavailable");
    return false;
  }

  api->init = resolve<decltype(api->init)>(api->library, "nvmlInit_v2");
  api->shutdown = resolve<decltype(api->shutdown)>(api->library, "nvmlShutdown");
  api->getCount = resolve<decltype(api->getCount)>(api->library, "nvmlDeviceGetCount_v2");
  api->getHandle = resolve<decltype(api->getHandle)>(api->library, "nvmlDeviceGetHandleByIndex_v2");
  api->getName = resolve<decltype(api->getName)>(api->library, "nvmlDeviceGetName");
  api->getUuid = resolve<decltype(api->getUuid)>(api->library, "nvmlDeviceGetUUID");
  api->getTemperature = resolve<decltype(api->getTemperature)>(
      api->library, "nvmlDeviceGetTemperature");
  api->getNumFans = resolve<decltype(api->getNumFans)>(api->library,
                                                       "nvmlDeviceGetNumFans");
  api->getFanSpeed = resolve<decltype(api->getFanSpeed)>(
      api->library, "nvmlDeviceGetFanSpeed_v2");
  api->getFanRpm = resolve<decltype(api->getFanRpm)>(
      api->library, "nvmlDeviceGetFanSpeedRPM");
  api->getMinMaxFanSpeed = resolve<decltype(api->getMinMaxFanSpeed)>(
      api->library, "nvmlDeviceGetMinMaxFanSpeed");
  api->setFanSpeed = resolve<decltype(api->setFanSpeed)>(
      api->library, "nvmlDeviceSetFanSpeed_v2");
  api->setDefaultFanSpeed = resolve<decltype(api->setDefaultFanSpeed)>(
      api->library, "nvmlDeviceSetDefaultFanSpeed_v2");
  api->errorString = resolve<decltype(api->errorString)>(api->library,
                                                         "nvmlErrorString");

  if (!api->init || !api->shutdown || !api->getCount || !api->getHandle ||
      !api->getName || !api->getUuid || !api->getTemperature) {
    m_detail = QStringLiteral("NVML does not expose the required query symbols");
    return false;
  }
  if (api->init() != NvmlSuccess) {
    m_detail = QStringLiteral("NVML initialization failed");
    return false;
  }
  api->initialized = true;

  unsigned count = 0;
  if (api->getCount(&count) != NvmlSuccess || count == 0) {
    api->shutdown();
    api->initialized = false;
    m_detail = QStringLiteral("NVML returned no GPU devices");
    return false;
  }

  m_api = std::move(api);
  for (unsigned index = 0; index < count; ++index) {
    NvmlDevice handle = nullptr;
    if (m_api->getHandle(index, &handle) != NvmlSuccess)
      continue;
    char nameBuffer[256]{};
    char uuidBuffer[128]{};
    if (m_api->getName(handle, nameBuffer, sizeof(nameBuffer)) != NvmlSuccess)
      std::strncpy(nameBuffer, "NVIDIA GPU", sizeof(nameBuffer) - 1);
    if (m_api->getUuid(handle, uuidBuffer, sizeof(uuidBuffer)) != NvmlSuccess)
      std::snprintf(uuidBuffer, sizeof(uuidBuffer), "index-%u", index);

    Device device;
    device.handle = handle;
    device.id = QStringLiteral("nvidia:%1").arg(QString::fromLatin1(uuidBuffer));
    device.name = QString::fromUtf8(nameBuffer);
    device.sensorId = device.id + QStringLiteral(":temperature");
    unsigned temperature = 0;
    const bool temperatureAvailable =
        m_api->getTemperature(handle, NvmlTemperatureGpu, &temperature) ==
        NvmlSuccess;
    m_sensors.push_back({device.sensorId, QStringLiteral("nvidia"), device.id,
                         device.name, Thermal::DeviceKind::Gpu,
                         temperatureAvailable ? static_cast<double>(temperature)
                                              : -1.0,
                         temperatureAvailable});

    if (m_api->getNumFans &&
        m_api->getNumFans(handle, &device.fanCount) == NvmlSuccess) {
      unsigned minimum = 0;
      unsigned maximum = 100;
      device.controllable =
          device.fanCount > 0 && m_api->getMinMaxFanSpeed &&
          m_api->setFanSpeed && m_api->setDefaultFanSpeed &&
          m_api->getMinMaxFanSpeed(handle, &minimum, &maximum) == NvmlSuccess;
      if (device.controllable) {
        device.minimumSpeed = minimum;
        device.maximumSpeed = maximum;
      }
    }

    for (unsigned fanIndex = 0; fanIndex < device.fanCount; ++fanIndex) {
      unsigned speed = 0;
      const bool speedAvailable =
          m_api->getFanSpeed &&
          m_api->getFanSpeed(handle, fanIndex, &speed) == NvmlSuccess;
      int rpm = -1;
      if (m_api->getFanRpm) {
        NvmlFanSpeedInfo info{NvmlFanInfoVersion, fanIndex, 0};
        if (m_api->getFanRpm(handle, &info) == NvmlSuccess)
          rpm = static_cast<int>(info.speed);
      }
      const auto fanId =
          QStringLiteral("%1:fan%2").arg(device.id).arg(fanIndex);
      const auto zoneId =
          QStringLiteral("%1:zone%2").arg(device.id).arg(fanIndex);
      m_fans.push_back({fanId, QStringLiteral("nvidia"), device.id, device.id,
                        QStringLiteral("%1 fan %2")
                            .arg(device.name, QString::number(fanIndex + 1)),
                        Thermal::DeviceKind::Gpu, rpm,
                        speedAvailable ? static_cast<int>(speed) : -1, rpm >= 0,
                        device.controllable, device.controllable,
                        device.controllable, device.controllable,
                        speedAvailable && speed == 0, true,
                        device.controllable ? QStringLiteral("controllable")
                                            : QStringLiteral("read-only")});
      if (device.controllable) {
        Thermal::FanZone zone;
        zone.id = zoneId;
        zone.backendId = QStringLiteral("nvidia");
        zone.controllerId = device.id;
        zone.displayName = QStringLiteral("%1 fan %2")
                               .arg(device.name, QString::number(fanIndex + 1));
        zone.sensorId = device.sensorId;
        zone.targetFanIds = {fanId};
        zone.kind = Thermal::DeviceKind::Gpu;
        zone.capabilities = {true, true, true, true};
        zone.minimumDuty = static_cast<int>(device.minimumSpeed);
        zone.maximumDuty = static_cast<int>(device.maximumSpeed);
        zone.safetyTemperature = 90;
        m_zones.push_back(zone);
      }
    }
    m_devices.push_back(device);
  }

  if (m_sensors.isEmpty()) {
    m_api->shutdown();
    m_api.reset();
    m_detail = QStringLiteral("NVML returned no readable GPU telemetry");
    return false;
  }
  m_usingNvml = true;
  m_detail = m_zones.isEmpty()
                 ? QStringLiteral("NVML telemetry available; this GPU does not expose privileged fan control")
                 : QStringLiteral("NVML per-fan control available with default-policy restoration");
  return true;
}

bool NvidiaBackend::querySmi() {
  const auto executable = QStandardPaths::findExecutable(QStringLiteral("nvidia-smi"));
  if (executable.isEmpty()) {
    m_detail = QStringLiteral("Neither NVML nor nvidia-smi is available");
    return false;
  }
  QProcess process;
  process.start(executable,
                {QStringLiteral("--query-gpu=uuid,name,temperature.gpu,fan.speed"),
                 QStringLiteral("--format=csv,noheader,nounits")});
  if (!process.waitForStarted(1000) || !process.waitForFinished(2500) ||
      process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    m_detail = QStringLiteral("NVIDIA telemetry query failed");
    return false;
  }
  QList<Thermal::TemperatureSensor> sensors;
  QList<Thermal::FanDevice> fans;
  const auto lines = QString::fromUtf8(process.readAllStandardOutput())
                         .split(u'\n', Qt::SkipEmptyParts);
  for (const auto &line : lines) {
    const auto fields = line.split(u',');
    if (fields.size() < 4)
      continue;
    const auto deviceId = QStringLiteral("nvidia:%1").arg(fields[0].trimmed());
    const auto name = fields[1].trimmed();
    bool temperatureOk = false;
    const auto temperature = fields[2].trimmed().toDouble(&temperatureOk);
    bool dutyOk = false;
    const auto duty = fields[3].trimmed().toInt(&dutyOk);
    sensors.push_back({deviceId + QStringLiteral(":temperature"),
                         QStringLiteral("nvidia"), deviceId, name,
                         Thermal::DeviceKind::Gpu,
                         temperatureOk ? temperature : -1.0, temperatureOk});
    if (dutyOk)
      fans.push_back({deviceId + QStringLiteral(":fan0"),
                        QStringLiteral("nvidia"), deviceId, deviceId,
                        QStringLiteral("%1 fan").arg(name),
                        Thermal::DeviceKind::Gpu, -1, duty, false, false, false,
                        false, false, duty == 0, true, QStringLiteral("read-only")});
  }
  if (sensors.isEmpty())
    return false;
  m_sensors = std::move(sensors);
  m_fans = std::move(fans);
  m_detail = QStringLiteral("nvidia-smi fallback telemetry only");
  return true;
}

void NvidiaBackend::refresh() {
  if (!m_usingNvml) {
    const auto refreshed = querySmi();
    Q_UNUSED(refreshed)
    return;
  }
  for (auto &sensor : m_sensors) {
    const auto device = std::find_if(
        m_devices.cbegin(), m_devices.cend(), [&sensor](const Device &item) {
          return item.sensorId == sensor.id;
        });
    if (device == m_devices.cend())
      continue;
    unsigned temperature = 0;
    sensor.available =
        m_api->getTemperature(device->handle, NvmlTemperatureGpu,
                              &temperature) == NvmlSuccess;
    sensor.temperature = sensor.available ? temperature : -1.0;
  }
  for (auto &fan : m_fans) {
    for (const auto &device : m_devices) {
      if (!fan.id.startsWith(device.id + QStringLiteral(":fan")))
        continue;
      bool ok = false;
      const auto index = fan.id.section(u':', -1).mid(3).toUInt(&ok);
      if (!ok)
        continue;
      unsigned speed = 0;
      if (m_api->getFanSpeed &&
          m_api->getFanSpeed(device.handle, index, &speed) == NvmlSuccess) {
        fan.duty = static_cast<int>(speed);
        fan.zeroRpmActive = speed == 0;
      }
      if (m_api->getFanRpm) {
        NvmlFanSpeedInfo info{NvmlFanInfoVersion, index, 0};
        if (m_api->getFanRpm(device.handle, &info) == NvmlSuccess) {
          fan.rpm = static_cast<int>(info.speed);
          fan.rpmAvailable = true;
        }
      }
    }
  }
}

QList<Thermal::TemperatureSensor> NvidiaBackend::sensors() const { return m_sensors; }
QList<Thermal::FanDevice> NvidiaBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> NvidiaBackend::zones() const { return m_zones; }

NvidiaBackend::Device *NvidiaBackend::deviceForZone(const QString &zoneId,
                                                     unsigned *fanIndex) {
  for (auto &device : m_devices) {
    const auto prefix = device.id + QStringLiteral(":zone");
    if (!zoneId.startsWith(prefix))
      continue;
    bool ok = false;
    const auto index = zoneId.mid(prefix.size()).toUInt(&ok);
    if (!ok || index >= device.fanCount)
      return nullptr;
    if (fanIndex)
      *fanIndex = index;
    return &device;
  }
  return nullptr;
}

Thermal::Result NvidiaBackend::nvmlError(const QString &code,
                                         int result) const {
  const auto detail = m_api && m_api->errorString
                          ? QString::fromLatin1(m_api->errorString(result))
                          : QStringLiteral("NVML error %1").arg(result);
  return Thermal::Result::fail(code, detail);
}

Thermal::Result NvidiaBackend::setAutomatic(const QString &zoneId) {
  unsigned fan = 0;
  auto *device = deviceForZone(zoneId, &fan);
  if (!device || !device->controllable || !m_api->setDefaultFanSpeed)
    return Thermal::Result::fail(QStringLiteral("nvidia.read-only"),
                                 QStringLiteral("NVIDIA automatic fan restoration is unavailable."));
  const auto result = m_api->setDefaultFanSpeed(device->handle, fan);
  return result == NvmlSuccess ? Thermal::Result::ok()
                               : nvmlError(QStringLiteral("nvidia.restore"), result);
}

Thermal::Result NvidiaBackend::setFullSpeed(const QString &zoneId) {
  unsigned fan = 0;
  auto *device = deviceForZone(zoneId, &fan);
  if (!device || !device->controllable || !m_api->setFanSpeed)
    return Thermal::Result::fail(QStringLiteral("nvidia.read-only"),
                                 QStringLiteral("NVIDIA fan control is unavailable."));
  const auto result =
      m_api->setFanSpeed(device->handle, fan, device->maximumSpeed);
  return result == NvmlSuccess ? Thermal::Result::ok()
                               : nvmlError(QStringLiteral("nvidia.full-speed"), result);
}

Thermal::Result NvidiaBackend::setDuty(const QString &zoneId, int percent) {
  unsigned fan = 0;
  auto *device = deviceForZone(zoneId, &fan);
  if (!device || !device->controllable || !m_api->setFanSpeed)
    return Thermal::Result::fail(QStringLiteral("nvidia.read-only"),
                                 QStringLiteral("NVIDIA fan control is unavailable."));
  const auto speed = static_cast<unsigned>(qBound(
      static_cast<int>(device->minimumSpeed), percent,
      static_cast<int>(device->maximumSpeed)));
  const auto result = m_api->setFanSpeed(device->handle, fan, speed);
  return result == NvmlSuccess ? Thermal::Result::ok()
                               : nvmlError(QStringLiteral("nvidia.set-speed"), result);
}
