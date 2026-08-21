
ARG BASE_IMAGE=linux-thermal-ubuntu22-base:local
FROM ${BASE_IMAGE} AS qt-dev

ARG TARGETARCH
ARG QT_VERSION=6.11.2
ARG UBUNTU_MIRROR=http://archive.ubuntu.com/ubuntu
ARG UBUNTU_SECURITY_MIRROR=http://security.ubuntu.com/ubuntu
ARG DEV_USER=developer
ARG DEV_UID=1000
ARG DEV_GID=1000
ARG TIMEZONE=UTC
ARG HTTP_PROXY
ARG HTTPS_PROXY
ARG NO_PROXY

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=${TIMEZONE} \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

# 使用 .env 指定的软件源，同时保留主仓库与安全更新的独立配置。
RUN sed -i \
      -e "s|http://archive.ubuntu.com/ubuntu|${UBUNTU_MIRROR%/}|g" \
      -e "s|http://security.ubuntu.com/ubuntu|${UBUNTU_SECURITY_MIRROR%/}|g" \
      /etc/apt/sources.list

RUN test "${TARGETARCH}" = "amd64" \
    || (echo "This development image supports linux/amd64 (x86_64) only; got ${TARGETARCH}" >&2; exit 1)

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      gcc-12 \
      g++-12 \
      cmake \
      ninja-build \
      git \
      pkg-config \
      python3 \
      python3-pip \
      python3-venv \
      ca-certificates \
      curl \
      xz-utils \
      p7zip-full \
      patchelf \
      file \
      jq \
      sudo \
      dbus \
      dbus-x11 \
      libdbus-1-dev \
      policykit-1 \
      desktop-file-utils \
      libxml2-utils \
      shellcheck \
      clang-format \
      clang-tidy \
      cppcheck \
      lcov \
      dpkg-dev \
      devscripts \
      debhelper \
      fakeroot \
      lintian \
      libgl1-mesa-dev \
      libegl1-mesa-dev \
      libfontconfig1 \
      libfreetype6 \
      libx11-6 \
      libx11-xcb1 \
      libxext6 \
      libxi6 \
      libxrender1 \
      libxcb1 \
      libxcb-cursor0 \
      libxcb-glx0 \
      libxcb-icccm4 \
      libxcb-image0 \
      libxcb-keysyms1 \
      libxcb-randr0 \
      libxcb-render0 \
      libxcb-render-util0 \
      libxcb-shape0 \
      libxcb-shm0 \
      libxcb-sync1 \
      libxcb-util1 \
      libxcb-xfixes0 \
      libxcb-xinerama0 \
      libxcb-xkb1 \
      libxkbcommon0 \
      libxkbcommon-x11-0 \
      libwayland-client0 \
      libwayland-cursor0 \
      libwayland-egl1 \
      libasound2 \
      libpulse0 \
      libnss3 \
      libatspi2.0-0 \
      xvfb \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 120 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 120

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      fontconfig \
      fonts-noto-cjk \
    && fc-cache --force

COPY requirements.txt /tmp/requirements.txt
RUN python3 -m pip install --no-cache-dir --requirement /tmp/requirements.txt

# Qt is installed from the official Qt binary repositories through aqtinstall.
# Explicit archives keep the image reproducible and avoid unrelated modules.
# Install each Qt archive in its own cached layer. A transient proxy failure then
# retries only that archive instead of invalidating the complete Qt SDK download.
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives icu && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives qtbase && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives qtdeclarative && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives qtsvg && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives qttools && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done
RUN --mount=type=cache,target=/root/.cache/aqt,sharing=locked \
    for attempt in 1 2 3 4 5; do \
      aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --timeout 30 \
        --outputdir /opt/Qt --archives qtwayland && break; \
      test "$attempt" -eq 5 && exit 1; sleep $((attempt * 3)); \
    done

ENV QT_ROOT=/opt/Qt/${QT_VERSION}/gcc_64 \
    PATH=/opt/Qt/${QT_VERSION}/gcc_64/bin:${PATH} \
    CMAKE_PREFIX_PATH=/opt/Qt/${QT_VERSION}/gcc_64 \
    QML_IMPORT_PATH=/opt/Qt/${QT_VERSION}/gcc_64/qml \
    QML2_IMPORT_PATH=/opt/Qt/${QT_VERSION}/gcc_64/qml \
    LD_LIBRARY_PATH=/opt/Qt/${QT_VERSION}/gcc_64/lib \
    QT_QPA_PLATFORM=offscreen \
    QT_QUICK_BACKEND=software

RUN qtpaths --qt-version \
    && qmake -query QT_VERSION \
    && cmake --version \
    && g++ --version | head -n 1

RUN groupadd --gid "${DEV_GID}" "${DEV_USER}" \
    && useradd --uid "${DEV_UID}" --gid "${DEV_GID}" --create-home --shell /bin/bash "${DEV_USER}" \
    && printf '%s ALL=(ALL) NOPASSWD:ALL\n' "${DEV_USER}" > "/etc/sudoers.d/${DEV_USER}" \
    && chmod 0440 "/etc/sudoers.d/${DEV_USER}"

COPY docker_entrypoint.sh /usr/local/bin/linux-thermal-dev-entrypoint
RUN chmod 0755 /usr/local/bin/linux-thermal-dev-entrypoint

USER ${DEV_USER}
WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/linux-thermal-dev-entrypoint"]
CMD ["bash"]
