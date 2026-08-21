# Repository Guidelines

## Project Structure & Module Organization

`src/app/` contains the Qt Quick GUI, with C++ code grouped under `client/`, `models/`, and `platform/`. QML files follow the `LinuxThermal.Control` URI and are grouped by responsibility. `src/daemon/` contains `control/`, `config/`, `service/`, and hardware `backends/`. Shared types and constants live in `src/common/`. Tests mirror these responsibilities under `tests/`; packaging assets and maintainer scripts are in `packaging/`. Treat `build/` and `dist/` as generated output.

## Build, Test, and Development Commands

- `./scripts/build.sh` builds Debug targets in the pinned Ubuntu 22.04, GCC 12, Qt 6.11.2 Docker environment.
- `ctest --test-dir build/docker-debug --output-on-failure` runs the complete CTest suite after a build.
- `./scripts/quick_start.sh` launches a private D-Bus session, mock daemon, and GUI without touching host fan controls.
- `./scripts/build.sh --pack` performs the release/package workflow and writes a Debian package to `dist/`.

Use the Docker workflow for reproducible results. Do not test unverified EC or ACPI writes on contributor hardware.

## Coding Style & Naming Conventions

The project uses C++23 and Qt 6. Follow Google C++ style with the existing two-space indentation. Use `PascalCase` for classes, `camelCase` for methods and local variables, and `m_` prefixes for members. Keep C++ comments concise and written in Chinese; document interfaces, constraints, ownership, and non-obvious safety decisions rather than restating code. Use `QStringLiteral`/`QByteArrayLiteral` for fixed Qt strings. QML component filenames use `PascalCase.qml`; preserve declarative bindings and avoid unnecessary imperative state.

## Testing Guidelines

Tests use Qt Test and CTest. Name C++ test files `*Test.cpp`; shell integration tests use descriptive `*-test.sh` names. Add regression coverage for control-policy, backend, D-Bus, tray, or packaging changes. Hardware-independent behavior must use temporary sysfs trees, fake NVML, or `MockBackend`. Run the full CTest command before submitting; no fixed coverage percentage is required.

## Commit & Pull Request Guidelines

History follows Conventional Commit-style subjects such as `fix: ...`, `feat: ...`, and `docs: ...`, with short Chinese summaries. Keep each commit focused. Pull requests should explain the problem, safety impact, test evidence, and affected hardware/backend. Link relevant issues and include screenshots for visible QML changes. Never commit credentials, signing keys, machine-specific logs, or generated build artifacts.

## Hardware Safety

New backends must default to read-only. Expose write capability only after explicit device detection, bounded inputs, failure handling, and a verified path back to automatic firmware control.
