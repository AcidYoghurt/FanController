#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly project_root
readonly build_dir="${project_root}/build/docker-release"
readonly stage_dir="${project_root}/build/package-root"
readonly dist_dir="${project_root}/dist"
readonly package_name="linux-thermal-control"
readonly package_version="0.5.13-2"
readonly package_file="${dist_dir}/${package_name}_${package_version}_amd64.deb"

cd "${project_root}"
cmake -S "${project_root}" -B "${build_dir}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
  -DCMAKE_INSTALL_PREFIX=/usr/lib/linux-thermal-control \
  -DBUILD_TESTING=ON
cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure
cmake --build "${build_dir}" --target linux-thermal-control_qmllint

cmake -E remove_directory "${stage_dir}"
cmake -E make_directory "${stage_dir}" "${stage_dir}/DEBIAN" "${dist_dir}"
DESTDIR="${stage_dir}" cmake --install "${build_dir}" --strip

# Development and unused network/input plugins must never ship in the runtime package.
for plugin_dir in qmltooling generic networkinformation platforminputcontexts tls; do
  cmake -E remove_directory "${stage_dir}/usr/lib/linux-thermal-control/plugins/${plugin_dir}"
done
for image_plugin in libqgif.so libqico.so libqjpeg.so; do
  cmake -E rm -f "${stage_dir}/usr/lib/linux-thermal-control/plugins/imageformats/${image_plugin}"
done
cmake -E rm -f   "${stage_dir}/usr/lib/linux-thermal-control/plugins/platformthemes/libqgtk3.so"

# Runtime libraries copied by Qt's deploy API are stripped after staging.
while IFS= read -r -d '' library; do
  strip --strip-unneeded "${library}" 2>/dev/null || true
done < <(find "${stage_dir}/usr/lib/linux-thermal-control" -type f   \( -name '*.so' -o -name '*.so.*' \) -print0)

gzip -9n "${stage_dir}/usr/share/doc/linux-thermal-control/changelog.Debian"
gzip -9n "${stage_dir}/usr/share/man/man1/linux-thermal-control.1"
gzip -9n "${stage_dir}/usr/share/man/man8/linux-thermal-secure-boot-setup.8"

install -m 0755 packaging/debian/postinst "${stage_dir}/DEBIAN/postinst"
install -m 0755 packaging/debian/prerm "${stage_dir}/DEBIAN/prerm"
install -m 0755 packaging/debian/postrm "${stage_dir}/DEBIAN/postrm"
install -m 0644 packaging/debian/conffiles "${stage_dir}/DEBIAN/conffiles"

generate_runtime_dependencies() {
  local dependency_root
  dependency_root="$(mktemp -d)"
  mkdir -p "${dependency_root}/debian"
  cat >"${dependency_root}/debian/control" <<'DEPENDENCY_CONTROL'
Source: linux-thermal-control
Section: utils
Priority: optional
Maintainer: Build helper <build@example.com>
Standards-Version: 4.6.0

Package: linux-thermal-control
Architecture: any
Description: dependency calculation helper
DEPENDENCY_CONTROL

  local runtime_root="${stage_dir}/usr/lib/linux-thermal-control"
  local -a elf_files=()
  local file_path
  while IFS= read -r -d '' file_path; do
    if file -b "${file_path}" | grep -q 'ELF.*dynamically linked'; then
      elf_files+=("-e${file_path}")
    fi
  done < <(find "${runtime_root}" -type f -print0)

  local output
  output="$(cd "${dependency_root}" && dpkg-shlibdeps     --ignore-missing-info -O -l"${runtime_root}/lib" "${elf_files[@]}"     2>"${dependency_root}/warnings.log")"
  cat "${dependency_root}/warnings.log" >&2
  cmake -E remove_directory "${dependency_root}"
  printf '%s' "${output#shlibs:Depends=}"
}

runtime_dependencies="$(generate_runtime_dependencies)"
manual_dependencies="fonts-noto-cjk, dbus, init-system-helpers (>= 1.18~), kmod, policykit-1, acpi-call-dkms, gcc-12, mokutil, shim-signed"
installed_size="$(du -sk "${stage_dir}" | awk '{print $1}')"
cat > "${stage_dir}/DEBIAN/control" <<CONTROL
Package: ${package_name}
Version: ${package_version}
Section: utils
Priority: optional
Architecture: amd64
Maintainer: Linux Thermal Control Maintainers <cfys-028@users.noreply.github.com>
Installed-Size: ${installed_size}
Depends: ${runtime_dependencies}, ${manual_dependencies}
Provides: colorful-fan-control
Conflicts: colorful-fan-control
Replaces: colorful-fan-control
Description: capability-aware Qt Quick thermal and fan control
 A Qt 6 desktop application and privileged D-Bus daemon that discovers Linux
 hwmon devices and optional vendor backends. It provides automatic, full-speed
 and independent per-zone custom-curve modes with explicit partial support.
CONTROL

# Static package metadata validation before creating the archive.
desktop-file-validate "${stage_dir}/usr/share/applications/org.linuxthermal.Control.desktop"
xmllint --noout \
  "${stage_dir}/usr/share/polkit-1/actions/org.linuxthermal.control.policy" \
  "${stage_dir}/usr/share/dbus-1/system.d/org.linuxthermal.Control1.conf"

packaging/debian/test-staged-package.sh "${stage_dir}"

# Hardware support is validated by backend fixture and mock D-Bus tests above.
find "${stage_dir}" -type d -exec chmod 0755 {} +
dpkg-deb --build --root-owner-group "${stage_dir}" "${package_file}"
dpkg-deb --info "${package_file}"
dpkg-deb --contents "${package_file}" >/dev/null
lintian --no-tag-display-limit "${package_file}" || true
(cd "${dist_dir}" && sha256sum "$(basename "${package_file}")") > "${package_file}.sha256"

echo "Package: ${package_file}"
echo "SHA256: $(cut -d' ' -f1 "${package_file}.sha256")"
