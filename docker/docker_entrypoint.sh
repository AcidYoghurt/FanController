#!/usr/bin/env bash
set -e

if [[ "$(uname -m)" != x86_64 ]]; then
  echo "错误：该开发容器只支持 x86_64，当前为 $(uname -m)。" >&2
  exit 1
fi

if [[ ! -x "${QT_ROOT}/bin/qt-cmake" ]]; then
  echo "错误：Qt SDK 不完整：${QT_ROOT}" >&2
  exit 1
fi

exec "$@"
