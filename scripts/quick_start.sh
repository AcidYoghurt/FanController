#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly project_root
readonly gui_binary="${project_root}/build/docker-debug/src/app/linux-thermal-control"
readonly daemon_binary="${project_root}/build/docker-debug/src/daemon/linux-thermal-daemon"

for binary in "${gui_binary}" "${daemon_binary}"; do
  if [[ ! -x "${binary}" ]]; then
    echo "尚未找到已编译程序：${binary}" >&2
    echo "请先执行：./scripts/build.sh" >&2
    exit 1
  fi
done

# shellcheck disable=SC1091
source "${project_root}/docker/common.sh"

read -r -d '' launch_command <<'SCRIPT' || true
set -euo pipefail
export DBUS_SYSTEM_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS}"
export LINUX_THERMAL_ALLOW_MOCK=1

/workspace/build/docker-debug/src/daemon/linux-thermal-daemon --mock \
  >/tmp/linux-thermal-mock-daemon.log 2>&1 &
daemon_pid=$!

cleanup() {
  kill -TERM "${daemon_pid}" >/dev/null 2>&1 || true
  wait "${daemon_pid}" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

service_ready=false
for attempt in $(seq 1 50); do
  if dbus-send --bus="${DBUS_SYSTEM_BUS_ADDRESS}" --print-reply \
      --dest=org.linuxthermal.Control1 /org/linuxthermal/Control1 \
      org.linuxthermal.Control1.GetVersion >/dev/null 2>&1; then
    service_ready=true
    break
  fi
  if ! kill -0 "${daemon_pid}" >/dev/null 2>&1; then
    cat /tmp/linux-thermal-mock-daemon.log >&2
    exit 1
  fi
  sleep 0.1
done

if [[ "${service_ready}" != true ]]; then
  echo "mock daemon did not acquire D-Bus name" >&2
  cat /tmp/linux-thermal-mock-daemon.log >&2
  exit 1
fi

/workspace/build/docker-debug/src/app/linux-thermal-control "$@"
SCRIPT

run_gui dbus-run-session -- bash -lc "${launch_command}" bash "$@"
