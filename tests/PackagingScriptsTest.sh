#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
postinst="${project_root}/packaging/debian/postinst"
postrm="${project_root}/packaging/debian/postrm"
service_file="${project_root}/packaging/systemd/linux-thermal-daemon.service"
dbus_service="${project_root}/packaging/dbus/system-services/org.linuxthermal.Control1.service"
dbus_policy="${project_root}/packaging/dbus/system.d/org.linuxthermal.Control1.conf"
polkit_policy="${project_root}/packaging/polkit/org.linuxthermal.control.policy"

sh -n "${postinst}" "${postrm}" "${project_root}/packaging/debian/prerm"
grep -Fq 'systemctl daemon-reload' "${postinst}"
grep -Fq 'org.freedesktop.DBus.ReloadConfig' "${postinst}"
grep -Fq 'deb-systemd-invoke restart' "${postinst}"
grep -Fq 'systemctl daemon-reload' "${postrm}"
grep -Fq 'org.freedesktop.DBus.ReloadConfig' "${postrm}"
grep -Fq 'BusName=org.linuxthermal.Control1' "${service_file}"
grep -Fq 'Name=org.linuxthermal.Control1' "${dbus_service}"
grep -Fq 'SystemdService=linux-thermal-daemon.service' "${dbus_service}"
grep -Fq '<allow own="org.linuxthermal.Control1"/>' "${dbus_policy}"
grep -Fq '<allow_any>no</allow_any>' "${polkit_policy}"
grep -Fq '<allow_inactive>no</allow_inactive>' "${polkit_policy}"
grep -Fq '<allow_active>yes</allow_active>' "${polkit_policy}"
