#!/usr/bin/env bash

set -euo pipefail

DOCKER_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly DOCKER_DIR
PROJECT_ROOT="$(cd -- "${DOCKER_DIR}/.." && pwd)"
readonly PROJECT_ROOT
readonly ENV_FILE="${DOCKER_DIR}/.env"

if [[ ! -r "${ENV_FILE}" ]]; then
  echo "缺少 Docker 配置文件：${ENV_FILE}" >&2
  return 1 2>/dev/null || exit 1
fi

# 配置文件是 Docker 构建参数的唯一来源，避免脚本和 Dockerfile 各自维护版本。
set -a
# shellcheck disable=SC1090
source "${ENV_FILE}"
set +a

readonly DOCKER_IMAGE_VERSION="${DOCKER_IMAGE_VERSION:?docker/.env 缺少 DOCKER_IMAGE_VERSION}"
readonly DOCKER_PLATFORM="${DOCKER_PLATFORM:-linux/amd64}"
readonly DOCKER_REPOSITORY="${DOCKER_REPOSITORY:?docker/.env 缺少 DOCKER_REPOSITORY}"
readonly UBUNTU_BASE_IMAGE="${UBUNTU_BASE_IMAGE:?docker/.env 缺少 UBUNTU_BASE_IMAGE}"
readonly LOCAL_BASE_IMAGE="${LOCAL_BASE_IMAGE:?docker/.env 缺少 LOCAL_BASE_IMAGE}"
readonly UBUNTU_MIRROR="${UBUNTU_MIRROR:?docker/.env 缺少 UBUNTU_MIRROR}"
readonly UBUNTU_SECURITY_MIRROR="${UBUNTU_SECURITY_MIRROR:?docker/.env 缺少 UBUNTU_SECURITY_MIRROR}"
readonly QT_VERSION="${QT_VERSION:?docker/.env 缺少 QT_VERSION}"
readonly BUILD_CACHE_VOLUME="${BUILD_CACHE_VOLUME:?docker/.env 缺少 BUILD_CACHE_VOLUME}"
readonly DEV_IMAGE="${DOCKER_REPOSITORY}:${DOCKER_IMAGE_VERSION}"
readonly BASE_IMAGE="${LOCAL_BASE_IMAGE}"

log() {
  printf '[docker] %s\n' "$*"
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

port_is_open() {
  local host="$1" port="$2"
  timeout 1 bash -c "</dev/tcp/${host}/${port}" >/dev/null 2>&1
}

normalize_proxy() {
  local value="${1:-}"
  value="${value%/}"
  [[ -n "${value}" ]] && printf '%s\n' "${value}"
}

detect_proxy() {
  local candidate host port

  for candidate in \
    "${HTTPS_PROXY:-}" "${https_proxy:-}" \
    "${HTTP_PROXY:-}" "${http_proxy:-}"; do
    if [[ -n "${candidate}" ]]; then
      normalize_proxy "${candidate}"
      return 0
    fi
  done

  if command_exists gsettings; then
    host="$(gsettings get org.gnome.system.proxy.http host 2>/dev/null | tr -d "'" || true)"
    port="$(gsettings get org.gnome.system.proxy.http port 2>/dev/null || true)"
    if [[ -n "${host}" && "${port}" =~ ^[0-9]+$ && "${port}" -gt 0 ]] \
        && port_is_open "${host}" "${port}"; then
      printf 'http://%s:%s\n' "${host}" "${port}"
      return 0
    fi
  fi

  for port in 7897 7890 7891 7892 10809 8080 8118 3128; do
    if port_is_open 127.0.0.1 "${port}"; then
      printf 'http://127.0.0.1:%s\n' "${port}"
      return 0
    fi
  done

  return 1
}

detect_timezone() {
  local zone="${TZ:-}" resolved

  if [[ -n "${zone}" && -e "/usr/share/zoneinfo/${zone}" ]]; then
    printf '%s\n' "${zone}"
    return 0
  fi

  if [[ -L /etc/localtime ]]; then
    resolved="$(readlink -f /etc/localtime)"
    if [[ "${resolved}" == /usr/share/zoneinfo/* ]]; then
      printf '%s\n' "${resolved#/usr/share/zoneinfo/}"
      return 0
    fi
  fi

  if [[ -r /etc/timezone ]]; then
    zone="$(tr -d '[:space:]' </etc/timezone)"
    if [[ -n "${zone}" ]]; then
      printf '%s\n' "${zone}"
      return 0
    fi
  fi

  if command_exists timedatectl; then
    zone="$(timedatectl show --property=Timezone --value 2>/dev/null || true)"
    if [[ -n "${zone}" ]]; then
      printf '%s\n' "${zone}"
      return 0
    fi
  fi

  printf 'UTC\n'
}

proxy_value() {
  detect_proxy 2>/dev/null || true
}

ensure_crane() {
  local proxy="$1"
  local tools_dir="${XDG_CACHE_HOME:-${HOME}/.cache}/linux-thermal-control"
  local crane="${tools_dir}/crane"
  local archive="${tools_dir}/go-containerregistry.tar.gz"
  local version="v0.21.9"

  if [[ -x "${crane}" ]]; then
    printf '%s\n' "${crane}"
    return 0
  fi

  mkdir -p "${tools_dir}"
  local curl_args=(--fail --location --retry 5 --retry-all-errors --connect-timeout 20)
  [[ -n "${proxy}" ]] && curl_args+=(--proxy "${proxy}")
  curl "${curl_args[@]}" \
    "https://github.com/google/go-containerregistry/releases/download/${version}/go-containerregistry_Linux_x86_64.tar.gz" \
    -o "${archive}"
  tar -xzf "${archive}" -C "${tools_dir}" crane
  chmod 0755 "${crane}"
  printf '%s\n' "${crane}"
}

ensure_base_image() {
  docker image inspect "${BASE_IMAGE}" >/dev/null 2>&1 && return 0

  local proxy crane tools_dir image_archive attempt
  proxy="$(proxy_value)"

  if [[ -z "${proxy}" ]]; then
    log "未检测到代理，尝试由 Docker 直接拉取 ${UBUNTU_BASE_IMAGE}。"
    if docker pull --platform "${DOCKER_PLATFORM}" "${UBUNTU_BASE_IMAGE}"; then
      docker tag "${UBUNTU_BASE_IMAGE}" "${BASE_IMAGE}"
      return 0
    fi
  fi

  [[ -n "${proxy}" ]] || {
    echo '无法拉取基础镜像，并且未检测到可用 HTTP 代理。' >&2
    return 1
  }

  log "使用自动检测到的代理 ${proxy} 导入 ${UBUNTU_BASE_IMAGE}。"
  crane="$(ensure_crane "${proxy}")"
  tools_dir="${XDG_CACHE_HOME:-${HOME}/.cache}/linux-thermal-control"

  for attempt in 1 2 3 4; do
    image_archive="${tools_dir}/ubuntu-22.04-amd64-${attempt}.tar"
    if HTTP_PROXY="${proxy}" HTTPS_PROXY="${proxy}" ALL_PROXY='' \
        "${crane}" pull --platform "${DOCKER_PLATFORM}" \
          "${UBUNTU_BASE_IMAGE}" "${image_archive}" \
        && tar -tf "${image_archive}" >/dev/null 2>&1; then
      docker load -i "${image_archive}"
      docker tag "${UBUNTU_BASE_IMAGE}" "${BASE_IMAGE}"
      python3 -c 'import pathlib,sys; pathlib.Path(sys.argv[1]).unlink(missing_ok=True)' "${image_archive}"
      return 0
    fi
    sleep $((attempt * 3))
  done

  echo '通过代理导入 Ubuntu 22.04 amd64 基础镜像失败。' >&2
  return 1
}

build_image() {
  local force="${1:-false}"
  local proxy timezone
  proxy="$(proxy_value)"
  timezone="$(detect_timezone)"

  ensure_base_image

  if [[ "${force}" != true ]] && docker image inspect "${DEV_IMAGE}" >/dev/null 2>&1; then
    log "开发镜像已存在：${DEV_IMAGE}"
    return 0
  fi

  local args=(
    build
    --network host
    --platform "${DOCKER_PLATFORM}"
    --file "${DOCKER_DIR}/Dockerfile"
    --tag "${DEV_IMAGE}"
    --build-arg "BASE_IMAGE=${BASE_IMAGE}"
    --build-arg "QT_VERSION=${QT_VERSION}"
    --build-arg "UBUNTU_MIRROR=${UBUNTU_MIRROR}"
    --build-arg "UBUNTU_SECURITY_MIRROR=${UBUNTU_SECURITY_MIRROR}"
    --build-arg "DEV_UID=$(id -u)"
    --build-arg "DEV_GID=$(id -g)"
    --build-arg "TIMEZONE=${timezone}"
  )

  if [[ -n "${proxy}" ]]; then
    args+=(
      --build-arg "HTTP_PROXY=${proxy}"
      --build-arg "HTTPS_PROXY=${proxy}"
      --build-arg "NO_PROXY=localhost,127.0.0.1,::1"
    )
  fi

  args+=("${DOCKER_DIR}")
  log "构建 ${DEV_IMAGE}；时区 ${timezone}；代理 ${proxy:-未检测到}。"
  docker "${args[@]}"
}

run_dev() {
  local proxy timezone
  proxy="$(proxy_value)"
  timezone="$(detect_timezone)"
  build_image false

  local args=(
    run --rm --init
    --platform "${DOCKER_PLATFORM}"
    --network host
    --user "$(id -u):$(id -g)"
    --workdir /workspace
    --volume "${PROJECT_ROOT}:/workspace"
    --volume "${BUILD_CACHE_VOLUME}:/home/developer/.cache"
    --env "HOME=/home/developer"
    --env "TZ=${timezone}"
  )

  if [[ -n "${proxy}" ]]; then
    args+=(
      --env "HTTP_PROXY=${proxy}"
      --env "HTTPS_PROXY=${proxy}"
      --env "http_proxy=${proxy}"
      --env "https_proxy=${proxy}"
      --env 'NO_PROXY=localhost,127.0.0.1,::1'
    )
  fi

  docker "${args[@]}" "${DEV_IMAGE}" "$@"
}

detect_session_type() {
  if [[ -n "${XDG_SESSION_TYPE:-}" ]]; then
    printf '%s\n' "${XDG_SESSION_TYPE}"
    return 0
  fi

  if command_exists loginctl && [[ -n "${XDG_SESSION_ID:-}" ]]; then
    loginctl show-session "${XDG_SESSION_ID}" --property=Type --value 2>/dev/null \
      && return 0
  fi

  printf 'unknown\n'
}

x11_socket_exists() {
  local display="${DISPLAY:-}"
  local number="${display#*:}"
  number="${number%%.*}"
  [[ "${number}" =~ ^[0-9]+$ && -S "/tmp/.X11-unix/X${number}" ]]
}

wayland_socket_path() {
  local runtime="${XDG_RUNTIME_DIR:-}"
  local display="${WAYLAND_DISPLAY:-}"
  [[ -n "${runtime}" && -n "${display}" && -S "${runtime}/${display}" ]] || return 1
  printf '%s\n' "${runtime}/${display}"
}

prepare_xauthority() {
  command_exists xauth || {
    echo '宿主机缺少 xauth，无法安全透传 X11 cookie。' >&2
    return 1
  }

  local runtime="${XDG_RUNTIME_DIR:-/tmp}"
  local authority="${runtime}/linux-thermal-control-docker.xauth"
  : >"${authority}"
  chmod 0600 "${authority}"

  if ! xauth nlist "${DISPLAY}" 2>/dev/null \
      | sed -e 's/^..../ffff/' \
      | xauth -f "${authority}" nmerge -; then
    echo "无法为 DISPLAY=${DISPLAY} 生成 Xauthority cookie。" >&2
    return 1
  fi

  [[ -s "${authority}" ]] || {
    echo "DISPLAY=${DISPLAY} 没有可用的 Xauthority cookie。" >&2
    return 1
  }
  printf '%s\n' "${authority}"
}

append_gpu_arguments() {
  local -n destination="$1"
  [[ -d /dev/dri ]] || return 0

  destination+=(--device /dev/dri:/dev/dri)

  local gid
  while IFS= read -r gid; do
    [[ "${gid}" =~ ^[0-9]+$ ]] && destination+=(--group-add "${gid}")
  done < <(find /dev/dri -maxdepth 1 -type c -printf '%G\n' 2>/dev/null | sort -nu)
}

run_gui() {
  local app_command="$1"
  shift
  local proxy timezone session
  proxy="$(proxy_value)"
  timezone="$(detect_timezone)"
  session="$(detect_session_type)"
  build_image false

  local args=(
    run --rm --init
    --platform "${DOCKER_PLATFORM}"
    --network host
    --user "$(id -u):$(id -g)"
    --workdir /workspace
    --volume "${PROJECT_ROOT}:/workspace"
    --volume "${BUILD_CACHE_VOLUME}:/home/developer/.cache"
    --env "HOME=/home/developer"
    --env "TZ=${timezone}"
  )

  if [[ -n "${proxy}" ]]; then
    args+=(
      --env "HTTP_PROXY=${proxy}"
      --env "HTTPS_PROXY=${proxy}"
      --env "http_proxy=${proxy}"
      --env "https_proxy=${proxy}"
      --env 'NO_PROXY=localhost,127.0.0.1,::1'
    )
  fi

  append_gpu_arguments args

  local authority=""
  if [[ -n "${DISPLAY:-}" ]] && x11_socket_exists; then
    authority="$(prepare_xauthority)"
    args+=(
      --volume /tmp/.X11-unix:/tmp/.X11-unix:rw
      --volume "${authority}:/tmp/.docker.xauth:ro"
      --env "DISPLAY=${DISPLAY}"
      --env XAUTHORITY=/tmp/.docker.xauth
      --env QT_QPA_PLATFORM=xcb
      --env QT_X11_NO_MITSHM=1
    )
    log "使用 X11/XWayland 显示 Qt GUI（宿主会话：${session}，DISPLAY=${DISPLAY}）。"
  else
    local wayland_socket
    wayland_socket="$(wayland_socket_path)" || {
      echo '没有检测到可用的 X11/XWayland 或 Wayland socket。' >&2
      return 1
    }
    args+=(
      --mount "type=bind,src=${wayland_socket},dst=/tmp/linux-thermal-runtime/${WAYLAND_DISPLAY}"
      --env "WAYLAND_DISPLAY=${WAYLAND_DISPLAY}"
      --env XDG_RUNTIME_DIR=/tmp/linux-thermal-runtime
      --env QT_QPA_PLATFORM=wayland
    )
    log "使用原生 Wayland 显示 Qt GUI（${WAYLAND_DISPLAY}）。"
  fi

  set +e
  docker "${args[@]}" "${DEV_IMAGE}" "${app_command}" "$@"
  local result=$?
  set -e

  if [[ -n "${authority}" ]]; then
    python3 -c 'import pathlib,sys; pathlib.Path(sys.argv[1]).unlink(missing_ok=True)' "${authority}"
  fi
  return "${result}"
}
