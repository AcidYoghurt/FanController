#pragma once

namespace LinuxThermal::Constants {
inline constexpr auto Version = "0.5.12";
inline constexpr auto ServiceName = "org.linuxthermal.Control1";
inline constexpr auto ObjectPath = "/org/linuxthermal/Control1";
inline constexpr auto InterfaceName = "org.linuxthermal.Control1";
inline constexpr auto PolkitAction = "org.linuxthermal.control";
inline constexpr auto AcpiMethod = "\\_SB.WMI.WMBB";
inline constexpr auto AcpiCallPath = "/proc/acpi/call";
inline constexpr auto CurveDirectory = "/var/lib/linux-thermal-control/curves";
} // namespace LinuxThermal::Constants
