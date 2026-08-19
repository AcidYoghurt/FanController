#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly project_root
readonly docker_build="${project_root}/docker/build.sh"

usage() {
  cat <<'HELP'
用法：
  ./scripts/build.sh          使用 Ubuntu 22.04 amd64 Docker 环境编译 C++/Qt 项目
  ./scripts/build.sh --pack   编译、测试并生成 Debian .deb 软件包
HELP
}

case "${1:-}" in
  "")
    exec "${docker_build}"
    ;;
  --pack)
    if (( $# != 1 )); then
      usage >&2
      exit 2
    fi
    "${docker_build}"
    # shellcheck disable=SC1091
    source "${project_root}/docker/common.sh"
    run_dev /workspace/packaging/debian/build-package.sh
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "未知参数：$1" >&2
    usage >&2
    exit 2
    ;;
esac
