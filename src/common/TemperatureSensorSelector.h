#pragma once

#include <QVariantList>
#include <QVariantMap>

#include <limits>

namespace LinuxThermal::TemperatureSensorSelector {
namespace Detail {
inline int cpuPriority(const QVariantMap &sensor) {
  const auto name = sensor.value(QStringLiteral("displayName")).toString().trimmed();
  if (name.compare(QStringLiteral("Tctl"), Qt::CaseInsensitive) == 0)
    return 400;
  if (name.compare(QStringLiteral("Package id 0"), Qt::CaseInsensitive) == 0)
    return 350;
  if (name.compare(QStringLiteral("CPU package"), Qt::CaseInsensitive) == 0)
    return 300;
  if (name.contains(QStringLiteral("package"), Qt::CaseInsensitive))
    return 250;
  return 0;
}

inline int gpuPriority(const QVariantMap &sensor) {
  const auto name = sensor.value(QStringLiteral("displayName")).toString().trimmed();
  const auto backendId = sensor.value(QStringLiteral("backendId")).toString();
  const auto deviceId = sensor.value(QStringLiteral("deviceId")).toString();
  if (backendId.contains(QStringLiteral("nvidia"), Qt::CaseInsensitive) ||
      deviceId.contains(QStringLiteral("nvidia"), Qt::CaseInsensitive))
    return 400;
  if (name.compare(QStringLiteral("edge"), Qt::CaseInsensitive) == 0)
    return 350;
  if (name.contains(QStringLiteral("gpu"), Qt::CaseInsensitive) ||
      name.contains(QStringLiteral("core"), Qt::CaseInsensitive))
    return 300;
  return 0;
}

inline QVariantMap selectForKind(const QVariantList &sensors,
                                 const QString &kind) {
  QVariantMap selected;
  int selectedPriority = std::numeric_limits<int>::min();
  double selectedTemperature = -std::numeric_limits<double>::infinity();
  bool selectedAvailable = false;

  for (const auto &value : sensors) {
    const auto sensor = value.toMap();
    if (sensor.value(QStringLiteral("deviceKind")).toString() != kind)
      continue;

    const bool available = sensor.value(QStringLiteral("available")).toBool();
    const int priority = kind == QStringLiteral("cpu") ? cpuPriority(sensor)
                                                        : gpuPriority(sensor);
    const double temperature = sensor.value(QStringLiteral("temperature")).toDouble();

    if (selected.isEmpty() ||
        (available && !selectedAvailable) ||
        (available == selectedAvailable && priority > selectedPriority) ||
        (available == selectedAvailable && priority == selectedPriority &&
         temperature > selectedTemperature)) {
      selected = sensor;
      selectedAvailable = available;
      selectedPriority = priority;
      selectedTemperature = temperature;
    }
  }
  return selected;
}
} // namespace Detail

inline QVariantList selectPrimary(const QVariantList &sensors) {
  QVariantList result;
  const auto cpu = Detail::selectForKind(sensors, QStringLiteral("cpu"));
  const auto gpu = Detail::selectForKind(sensors, QStringLiteral("gpu"));
  if (!cpu.isEmpty())
    result.push_back(cpu);
  if (!gpu.isEmpty())
    result.push_back(gpu);
  return result;
}
} // namespace LinuxThermal::TemperatureSensorSelector
