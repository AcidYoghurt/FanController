#!/usr/bin/env bash
set -euo pipefail

daemon="$1"
test_binary="$2"

# Child shell expands these variables intentionally.
# shellcheck disable=SC2016
exec dbus-run-session -- bash -c '
  set -euo pipefail
  export DBUS_SYSTEM_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS}"
  export LINUX_THERMAL_ALLOW_MOCK=1
  "$1" --mock >/tmp/linux-thermal-mock-test-daemon.log 2>&1 &
  daemon_pid=$!
  cleanup() {
    kill -TERM "${daemon_pid}" >/dev/null 2>&1 || true
    wait "${daemon_pid}" >/dev/null 2>&1 || true
  }
  trap cleanup EXIT INT TERM
  for attempt in $(seq 1 50); do
    if dbus-send --bus="${DBUS_SYSTEM_BUS_ADDRESS}" --print-reply \
        --dest=org.linuxthermal.Control1 /org/linuxthermal/Control1 \
        org.linuxthermal.Control1.GetVersion >/dev/null 2>&1; then
      "${@:2}"
      exit 0
    fi
    kill -0 "${daemon_pid}" >/dev/null 2>&1 || {
      cat /tmp/linux-thermal-mock-test-daemon.log >&2
      exit 1
    }
    sleep 0.1
  done
  echo "mock daemon did not acquire D-Bus name" >&2
  exit 1
' bash "${daemon}" "${test_binary}" "${@:3}"
