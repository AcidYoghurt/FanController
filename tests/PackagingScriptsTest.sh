#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
postinst="${project_root}/packaging/debian/postinst"
postrm="${project_root}/packaging/debian/postrm"
service_file="${project_root}/packaging/systemd/linux-thermal-daemon.service"
dbus_service="${project_root}/packaging/dbus/system-services/org.linuxthermal.Control1.service"
dbus_policy="${project_root}/packaging/dbus/system.d/org.linuxthermal.Control1.conf"
polkit_policy="${project_root}/packaging/polkit/org.linuxthermal.control.policy"
curve_config="${project_root}/config/default-fan-curve.yaml"
conffiles="${project_root}/packaging/debian/conffiles"
build_package="${project_root}/packaging/debian/build-package.sh"
hardware_curve_test="${project_root}/tests/HardwareCurveStressTest.sh"
secure_boot_setup="${project_root}/tools/linux-thermal-secure-boot-setup"

sh -n "${postinst}" "${postrm}" "${project_root}/packaging/debian/prerm"
bash -n "${hardware_curve_test}" "${secure_boot_setup}"
"${secure_boot_setup}" --help >/dev/null
grep -Fq "expected_product='P16 TA 24'" "${hardware_curve_test}"
grep -Fq 'trap restore_original_state EXIT INT TERM' "${hardware_curve_test}"
grep -Fq 'dbus_call ApplyZoneCurve' "${hardware_curve_test}"
grep -Fq 'feedback="$(dbus_call GetStatus)"' "${hardware_curve_test}"
grep -Fq 'systemctl daemon-reload' "${postinst}"
grep -Fq 'org.freedesktop.DBus.ReloadConfig' "${postinst}"
grep -Fq 'deb-systemd-invoke restart' "${postinst}"
grep -Fq 'systemctl daemon-reload' "${postrm}"
grep -Fq 'org.freedesktop.DBus.ReloadConfig' "${postrm}"
grep -Fq 'BusName=org.linuxthermal.Control1' "${service_file}"
grep -Fq 'ProtectClock=no' "${service_file}"
grep -Fq 'Name=org.linuxthermal.Control1' "${dbus_service}"
grep -Fq 'SystemdService=linux-thermal-daemon.service' "${dbus_service}"
grep -Fq '<allow own="org.linuxthermal.Control1"/>' "${dbus_policy}"
grep -Fq '<allow_any>no</allow_any>' "${polkit_policy}"
grep -Fq '<allow_inactive>no</allow_inactive>' "${polkit_policy}"
grep -Fq '<allow_active>yes</allow_active>' "${polkit_policy}"
grep -Fq '/etc/linux-thermal-control/default-fan-curve.yaml' "${conffiles}"
grep -Fq 'temperature: 35' "${curve_config}"
grep -Fq 'duty: 0' "${curve_config}"
grep -Fq 'temperature: 100' "${curve_config}"
grep -Fq 'duty: 100' "${curve_config}"
grep -Fq 'curve_backup=/etc/linux-thermal-control/.风扇曲线配置.backup' "${postinst}"
grep -Fq 'if [ -f "$curve_config" ] && [ ! -e "$curve_backup" ]' "${postinst}"
grep -Fq 'cp -p "$curve_config" "$curve_backup"' "${postinst}"
grep -Fq 'rm -f "$curve_backup"' "${postrm}"
grep -Fq 'sudo linux-thermal-secure-boot-setup --prepare' "${postinst}"
grep -Fq 'update-secureboot-policy --new-key' "${secure_boot_setup}"
grep -Fq 'DEBIAN_FRONTEND=noninteractive' "${secure_boot_setup}"
grep -Fq 'dkms build --force -m acpi-call' "${secure_boot_setup}"
grep -Fq 'mokutil --list-new' "${secure_boot_setup}"
grep -Fq "grep -Fq 'is already enrolled'" "${secure_boot_setup}"
grep -Fq 'update-secureboot-policy --enroll-key' "${secure_boot_setup}"
if grep -Fq -- '--disable-validation' "${secure_boot_setup}" "${postinst}"; then
  echo 'Secure Boot support must enroll a signing key, not disable validation' >&2
  exit 1
fi
grep -Eq '^manual_dependencies=.*acpi-call-dkms.*gcc-12.*mokutil.*shim-signed' "${build_package}"
if grep -Eq '^Suggests: .*acpi-call-dkms' "${build_package}"; then
  echo 'acpi-call-dkms must not remain an optional dependency' >&2
  exit 1
fi
