#!/usr/bin/env bash
set -euo pipefail

if [[ "${1:-}" != --private-session ]]; then
  exec dbus-run-session -- "$0" --private-session "$@"
fi
shift
readonly daemon="$1"
readonly preview="$2"
readonly output="$3"
logs="$(mktemp -d)"
readonly logs
export DBUS_SYSTEM_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS}"
export LINUX_THERMAL_ALLOW_MOCK=1
export QT_QPA_PLATFORM=offscreen
export QT_QUICK_BACKEND=software
mkdir -p "${output}"

"${daemon}" --mock >"${logs}/daemon.log" 2>&1 &
daemon_pid=$!
cleanup() {
  kill -TERM "${daemon_pid}" >/dev/null 2>&1 || true
  wait "${daemon_pid}" >/dev/null 2>&1 || true
  rm -rf "${logs}"
}
trap cleanup EXIT INT TERM

"${preview}" --output "${output}/light.png"
"${preview}" --dark --output "${output}/dark.png"
"${preview}" --dark --info --output "${output}/devices.png"
"${preview}" --dark --scroll-bottom --output "${output}/curve.png"
"${preview}" --dark --appearance --output "${output}/appearance.png"
"${preview}" --width 760 --height 620 --output "${output}/compact.png"
"${preview}" --width 760 --font-scale 1.4 --scroll-bottom --output "${output}/large-font.png"
