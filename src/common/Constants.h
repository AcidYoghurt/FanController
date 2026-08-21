#pragma once

namespace LinuxThermal::Constants {
// 对外报告的应用版本。Debian 修订号由打包脚本单独维护。
inline constexpr auto Version = "0.5.13";
// 系统 D-Bus 服务的名称、对象路径和接口名称。
inline constexpr auto ServiceName = "org.linuxthermal.Control1";
inline constexpr auto ObjectPath = "/org/linuxthermal/Control1";
inline constexpr auto InterfaceName = "org.linuxthermal.Control1";
// 改变硬件状态时检查的 Polkit 操作标识。
inline constexpr auto PolkitAction = "org.linuxthermal.control";
// 已验证 Clevo 后端使用的 ACPI 方法和 acpi_call 字符设备。
inline constexpr auto AcpiMethod = "\\_SB.WMI.WMBB";
inline constexpr auto AcpiCallPath = "/proc/acpi/call";
// 每个控制区域的持久化曲线目录。
inline constexpr auto CurveDirectory = "/var/lib/linux-thermal-control/curves";
} // namespace LinuxThermal::Constants
