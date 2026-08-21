#pragma once

#include "IFanBackend.h"

#include <QMutex>

// 控制经过 DMI 与 ACPI 路径双重白名单验证的 Clevo CLV0001 实现。
// 未匹配的机器不会执行任何 WMBB 调用。
class ClevoBackend final : public IFanBackend {
public:
  ClevoBackend();

  [[nodiscard]] Thermal::BackendInfo info() const override;
  [[nodiscard]] bool probe() override;
  void refresh() override;
  [[nodiscard]] QList<Thermal::TemperatureSensor> sensors() const override;
  [[nodiscard]] QList<Thermal::FanDevice> fans() const override;
  [[nodiscard]] QList<Thermal::FanZone> zones() const override;
  [[nodiscard]] Thermal::Result setAutomatic(const QString &zoneId) override;
  [[nodiscard]] Thermal::Result setFullSpeed(const QString &zoneId) override;
  [[nodiscard]] Thermal::Result setDuty(const QString &zoneId,
                                        int percent) override;
  // 截断内核缓冲区残留，并返回规范化的小写 ACPI 响应。
  [[nodiscard]] static QByteArray normalizeAcpiResponse(QByteArray response);
  // 优先读取 CPU package 或 Tctl，找不到时才回退到最热核心。
  [[nodiscard]] static double
  cpuPackageTemperature(const QString &hwmonRoot =
                            QStringLiteral("/sys/class/hwmon"));
  // 只公开已确认存在的 CPU 与 GPU 两只物理风扇。
  [[nodiscard]] static QList<Thermal::FanDevice>
  verifiedFanDevices(const QString &acpiRoot =
                         QStringLiteral("/sys/bus/acpi/devices"));
  // 将百分比占空比编码到 WMBB 0x68 使用的四个 EC 字节。
  [[nodiscard]] static quint32 packedDutyPayload(int percent);
  // 返回仅把未使用槽位交还自动控制的 WMBB 0x69 掩码。
  [[nodiscard]] static constexpr quint32 unusedFanAutoMask() {
    return 0x0000000cu;
  }

private:
  [[nodiscard]] Thermal::Result call(const QByteArray &command,
                                     const QByteArray &expected) const;
  [[nodiscard]] bool validZone(const QString &zoneId) const;

  // 串行化 /proc/acpi/call 的写入与响应读取，防止命令互相覆盖。
  mutable QMutex m_mutex;
  QList<Thermal::TemperatureSensor> m_sensors;
  QList<Thermal::FanDevice> m_fans;
  QList<Thermal::FanZone> m_zones;
  QString m_detail;
};
