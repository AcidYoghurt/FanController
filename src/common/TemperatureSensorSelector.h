#pragma once

#include <QVariantList>
#include <QVariantMap>

#include <limits>

namespace LinuxThermal::TemperatureSensorSelector {
namespace Detail {
// 给 CPU 传感器名称排序。包温度或 Tctl 比单核瞬时温度更适合界面展示和
// 风扇控制，因此优先级更高。
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

// 优先使用独立显卡驱动提供的 GPU 核心或边缘温度，避免把同设备的次要
// 传感器当成界面主读数。
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

// 选择指定设备类型的主传感器。先选择可用传感器，再比较语义优先级；
// 条件相同时取较高温度，避免掩盖热端读数。
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

// 最多返回一个 CPU 和一个 GPU 主传感器，并保持 CPU 在前的稳定顺序。
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
