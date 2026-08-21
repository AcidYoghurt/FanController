#!/usr/bin/env bash
set -euo pipefail

readonly stage_dir="$1"
readonly app_root="${stage_dir}/usr/lib/linux-thermal-control"
readonly daemon="${stage_dir}/usr/libexec/linux-thermal-control/linux-thermal-daemon"
readonly gui="${stage_dir}/usr/bin/linux-thermal-control"
readonly curve_config="${stage_dir}/etc/linux-thermal-control/default-fan-curve.yaml"
readonly package_control="${stage_dir}/DEBIAN/control"

grep -Eq '^Depends: .*acpi-call-dkms([, ]|$)' "${package_control}"
grep -Eq '^Depends: .*gcc-12([, ]|$)' "${package_control}"
if grep -Eq '^Suggests: .*acpi-call-dkms([, ]|$)' "${package_control}"; then
  echo 'acpi-call-dkms must be a required package dependency' >&2
  exit 1
fi

[[ -f "${curve_config}" ]] || {
  echo 'default fan curve YAML is missing from the staged package' >&2
  exit 1
}
grep -Fq 'temperature: 35' "${curve_config}"
grep -Fq 'duty: 0' "${curve_config}"
grep -Fq 'temperature: 100' "${curve_config}"
grep -Fq 'duty: 100' "${curve_config}"

if [[ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]]; then
  exec dbus-run-session -- "$0" "${stage_dir}"
fi

export DBUS_SYSTEM_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS}"
export LINUX_THERMAL_ALLOW_MOCK=1
export LINUX_THERMAL_APP_ROOT="${app_root}"

"${daemon}" --mock >/tmp/linux-thermal-package-mock-daemon.log 2>&1 &
daemon_pid=$!

cleanup() {
  kill -TERM "${daemon_pid}" >/dev/null 2>&1 || true
  wait "${daemon_pid}" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

service_ready=false
for _ in $(seq 1 50); do
  if dbus-send --bus="${DBUS_SYSTEM_BUS_ADDRESS}" --print-reply \
      --dest=org.linuxthermal.Control1 /org/linuxthermal/Control1 \
      org.linuxthermal.Control1.GetVersion >/dev/null 2>&1; then
    service_ready=true
    break
  fi
  if ! kill -0 "${daemon_pid}" >/dev/null 2>&1; then
    cat /tmp/linux-thermal-package-mock-daemon.log >&2
    exit 1
  fi
  sleep 0.1
done

[[ "${service_ready}" == true ]] || {
  echo 'staged mock daemon did not acquire D-Bus name' >&2
  exit 1
}

QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software \
  xvfb-run -a "${gui}" --test-page-navigation \
  >/tmp/linux-thermal-package-pages.out \
  2>/tmp/linux-thermal-package-pages.err
if [[ -s /tmp/linux-thermal-package-pages.err ]]; then
  cat /tmp/linux-thermal-package-pages.err >&2
  exit 1
fi

LINUX_THERMAL_FORCE_TRAY_FOR_TESTS=1 QT_QPA_PLATFORM=xcb \
  QT_QUICK_BACKEND=software xvfb-run -a "${gui}" \
  --test-tray-lifecycle \
  >/tmp/linux-thermal-package-tray.out \
  2>/tmp/linux-thermal-package-tray.err
if [[ -s /tmp/linux-thermal-package-tray.err ]]; then
  cat /tmp/linux-thermal-package-tray.err >&2
  exit 1
fi

set +e
QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software \
  timeout 5s xvfb-run -a "${gui}" \
  >/tmp/linux-thermal-package-gui.out 2>/tmp/linux-thermal-package-gui.err
gui_rc=$?
set -e

if [[ ${gui_rc} -ne 124 ]]; then
  echo "staged GUI returned ${gui_rc}, expected timeout 124" >&2
  cat /tmp/linux-thermal-package-gui.err >&2
  exit 1
fi
if [[ -s /tmp/linux-thermal-package-gui.err ]]; then
  cat /tmp/linux-thermal-package-gui.err >&2
  exit 1
fi
