#pragma once

#include "ThermalTypes.h"

#include <QList>

// 定义硬件后端向控制器提供的统一能力边界。
// probe() 成功后，调用方可以读取设备快照，并且只能对 zones() 明确声明
// 支持的模式执行写操作。
class IFanBackend {
public:
  virtual ~IFanBackend() = default;

  // 返回后端身份、驱动状态及聚合能力。
  [[nodiscard]] virtual Thermal::BackendInfo info() const = 0;
  // 探测硬件并建立初始设备拓扑；可用时返回 true。
  [[nodiscard]] virtual bool probe() = 0;
  // 刷新已有设备的遥测值，不改变已探测的控制语义。
  virtual void refresh() = 0;
  // 返回最近一次探测或刷新得到的温度传感器快照。
  [[nodiscard]] virtual QList<Thermal::TemperatureSensor> sensors() const = 0;
  // 返回最近一次探测或刷新得到的风扇快照。
  [[nodiscard]] virtual QList<Thermal::FanDevice> fans() const = 0;
  // 返回后端可控制的风扇区域及其安全边界。
  [[nodiscard]] virtual QList<Thermal::FanZone> zones() const = 0;
  // 将 `zoneId` 的控制权交还给固件或内核驱动。
  [[nodiscard]] virtual Thermal::Result setAutomatic(const QString &zoneId) = 0;
  // 将 `zoneId` 中所有受控风扇设为后端支持的满速状态。
  [[nodiscard]] virtual Thermal::Result setFullSpeed(const QString &zoneId) = 0;
  // 将 `zoneId` 设为手动控制，并写入百分比占空比 `percent`。
  [[nodiscard]] virtual Thermal::Result setDuty(const QString &zoneId,
                                                int percent) = 0;
};
