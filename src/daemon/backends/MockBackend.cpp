#include "MockBackend.h"

#include <QtMath>

Thermal::BackendInfo MockBackend::info() const {
  return {QStringLiteral("mock"), QStringLiteral("Simulated hardware"),
          QStringLiteral("development"), QStringLiteral("ready"),
          QStringLiteral("Intel CPU + NVIDIA-style dGPU development fixture"),
          {true, true, true, true}};
}

bool MockBackend::probe() {
  m_sensors = {
      {QStringLiteral("mock:cpu:package"), QStringLiteral("mock"),
       QStringLiteral("mock:cpu"), QStringLiteral("Intel Core processor"),
       Thermal::DeviceKind::Cpu, 58.0, true},
      {QStringLiteral("mock:gpu:core"), QStringLiteral("mock"),
       QStringLiteral("mock:gpu"), QStringLiteral("Discrete GPU"),
       Thermal::DeviceKind::Gpu, 47.0, true},
  };
  m_fans = {
      {QStringLiteral("mock:cpu:fan0"), QStringLiteral("mock"),
       QStringLiteral("mock:controller"), QStringLiteral("mock:cpu"),
       QStringLiteral("CPU fan"), Thermal::DeviceKind::Cpu, 1800, 35, true,
       true, true, true, true, false, true, QStringLiteral("controllable")},
      {QStringLiteral("mock:gpu:fan0"), QStringLiteral("mock"),
       QStringLiteral("mock:controller"), QStringLiteral("mock:gpu"),
       QStringLiteral("GPU fan"), Thermal::DeviceKind::Gpu, 1200, 30, true,
       true, true, true, true, false, true, QStringLiteral("controllable")},
      {QStringLiteral("mock:system:fan0"), QStringLiteral("mock"),
       QStringLiteral("mock:readonly"), QStringLiteral("mock:system"),
       QStringLiteral("Read-only system fan"), Thermal::DeviceKind::System,
       900, -1, true, false, false, false, false, false, true,
       QStringLiteral("read-only")},
  };

  Thermal::FanZone cpu;
  cpu.id = QStringLiteral("mock:cpu:zone");
  cpu.backendId = QStringLiteral("mock");
  cpu.controllerId = QStringLiteral("mock:controller");
  cpu.displayName = QStringLiteral("CPU cooling zone");
  cpu.sensorId = QStringLiteral("mock:cpu:package");
  cpu.targetFanIds = {QStringLiteral("mock:cpu:fan0")};
  cpu.kind = Thermal::DeviceKind::Cpu;
  cpu.capabilities = {true, true, true, true};
  cpu.minimumDuty = 30;
  cpu.safetyTemperature = 85;

  Thermal::FanZone gpu = cpu;
  gpu.id = QStringLiteral("mock:gpu:zone");
  gpu.displayName = QStringLiteral("GPU cooling zone");
  gpu.sensorId = QStringLiteral("mock:gpu:core");
  gpu.targetFanIds = {QStringLiteral("mock:gpu:fan0")};
  gpu.kind = Thermal::DeviceKind::Gpu;
  gpu.minimumDuty = 25;
  gpu.safetyTemperature = 88;
  m_zones = {cpu, gpu};
  m_duties.insert(cpu.id, 35);
  m_duties.insert(gpu.id, 30);
  return true;
}

void MockBackend::refresh() {
  ++m_tick;
  m_sensors[0].temperature = m_sensorFailure ? -1.0 : 61.0 + 9.0 * qSin(m_tick / 8.0);
  m_sensors[0].available = !m_sensorFailure;
  m_sensors[1].temperature = 50.0 + 6.0 * qSin(m_tick / 11.0);
  for (auto &fan : m_fans) {
    if (!fan.controllable)
      continue;
    const auto zoneId = fan.kind == Thermal::DeviceKind::Cpu
                            ? QStringLiteral("mock:cpu:zone")
                            : QStringLiteral("mock:gpu:zone");
    fan.duty = m_duties.value(zoneId);
    fan.rpm = fan.duty == 0 ? 0 : 700 + fan.duty * 44;
    fan.zeroRpmActive = fan.rpm == 0;
  }
}

QList<Thermal::TemperatureSensor> MockBackend::sensors() const { return m_sensors; }
QList<Thermal::FanDevice> MockBackend::fans() const { return m_fans; }
QList<Thermal::FanZone> MockBackend::zones() const { return m_zones; }

Thermal::Result MockBackend::command(const QString &zoneId, int duty) {
  if (!m_duties.contains(zoneId))
    return Thermal::Result::fail(QStringLiteral("zone.unknown"), QStringLiteral("Unknown simulated zone."));
  if (m_failNext) {
    m_failNext = false;
    return Thermal::Result::fail(QStringLiteral("mock.command-failed"),
                                 QStringLiteral("Simulated fan command failure."));
  }
  m_duties.insert(zoneId, duty);
  refresh();
  return Thermal::Result::ok();
}
Thermal::Result MockBackend::setAutomatic(const QString &zoneId) { return command(zoneId, 35); }
Thermal::Result MockBackend::setFullSpeed(const QString &zoneId) { return command(zoneId, 100); }
Thermal::Result MockBackend::setDuty(const QString &zoneId, int percent) {
  return command(zoneId, qBound(0, percent, 100));
}
void MockBackend::setSensorFailure(bool enabled) { m_sensorFailure = enabled; }
void MockBackend::failNextCommand() { m_failNext = true; }
