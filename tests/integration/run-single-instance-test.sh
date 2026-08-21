#!/usr/bin/env bash
set -euo pipefail

readonly app="$1"
readonly service="org.linuxthermal.Control.Gui"
readonly log_dir="$(mktemp -d)"

cleanup() {
  if [[ -n "${primary_pid:-}" ]]; then
    kill -TERM "${primary_pid}" >/dev/null 2>&1 || true
    wait "${primary_pid}" >/dev/null 2>&1 || true
  fi
  rm -rf "${log_dir}"
}
trap cleanup EXIT INT TERM

if [[ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]]; then
  exec dbus-run-session -- "$0" "$app"
fi

export QT_QPA_PLATFORM=xcb
export QT_QUICK_BACKEND=software

xvfb-run -a "${app}" --test-single-instance-primary \
  >"${log_dir}/primary.out" 2>"${log_dir}/primary.err" &
primary_pid=$!

ready=false
for _ in $(seq 1 50); do
  if dbus-send --session --print-reply \
      --dest=org.freedesktop.DBus / org.freedesktop.DBus.NameHasOwner \
      string:"${service}" 2>/dev/null | grep -q 'boolean true'; then
    ready=true
    break
  fi
  if ! kill -0 "${primary_pid}" >/dev/null 2>&1; then
    cat "${log_dir}/primary.err" >&2
    exit 1
  fi
  sleep 0.1
done

[[ "${ready}" == true ]] || {
  echo 'primary GUI did not register its session D-Bus service' >&2
  exit 1
}

sleep 0.3
timeout 3s xvfb-run -a "${app}" >"${log_dir}/secondary.out" 2>"${log_dir}/secondary.err"
wait "${primary_pid}"
primary_pid=

if [[ -s "${log_dir}/primary.err" ]]; then
  cat "${log_dir}/primary.err" >&2
  exit 1
fi
if [[ -s "${log_dir}/secondary.err" ]]; then
  cat "${log_dir}/secondary.err" >&2
  exit 1
fi
