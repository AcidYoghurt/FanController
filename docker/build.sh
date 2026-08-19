#!/usr/bin/env bash
set -euo pipefail

# shellcheck disable=SC1091
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/common.sh"

if (( $# != 0 )); then
  echo "docker/build.sh 不接受参数；请直接运行 ./docker/build.sh。" >&2
  exit 2
fi

build_image false
run_dev cmake -S /workspace -B /workspace/build/docker-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
  -DCMAKE_INSTALL_PREFIX=/usr/lib/linux-thermal-control \
  -DBUILD_TESTING=ON
run_dev cmake --build /workspace/build/docker-debug --parallel
