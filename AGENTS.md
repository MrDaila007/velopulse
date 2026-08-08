# Repository Guidelines

## Project Structure & Module Organization

- `firmware/src/` contains Arduino-specific application and hardware adapters.
- `firmware/lib/domain/` contains platform-independent C++ logic. Keep Arduino and
  nRF52 headers out of this layer so it remains host-testable.
- `firmware/test/test_native/` contains Unity host tests;
  `firmware/test/test_embedded/` contains board tests.
- `simulator/` renders the OLED UI using shared firmware layout/formatting code;
  golden images live in `simulator/golden/`.
- `protocol/` is the normative BLE contract. Architecture and acceptance material
  lives in `docs/`; subsystem backlogs live in `tasks/`.

## Build, Test, and Development Commands

Run firmware commands from `firmware/`:

```bash
pio test -e native             # build and run host-side domain tests
pio run -e xiao_ble_sense      # build production nRF52840 firmware
pio test -e xiao_ble_sense     # upload/run embedded tests on a connected board
pio run -e xiao_ble_sense -t upload  # upload production firmware
pio device monitor -b 115200   # open the serial console
```

Run `./test.sh` from `simulator/` to execute simulator and pixel-golden tests.
After embedded tests, always restore the production firmware.

## Coding Style & Naming Conventions

Use C++17, two-space indentation, braces on the same line, and no tabs. Classes and
enums use `PascalCase`; functions and local variables use `camelCase`; fields use
`snake_case`; constants use the `kDescriptiveName` form. Keep hot-path calculations
integer/fixed-point and firmware loops non-blocking. Preserve existing Markdown style.

## Testing Guidelines

Tests use Unity for C++ and Python `unittest` for the simulator. Name C++ tests
`test_<behavior>_<expected_result>` and register them with `RUN_TEST`. Add native
tests for domain logic, boundaries, serialization, and recovery. Use embedded tests
only for hardware or InternalFS behavior; use isolated paths such as `/test_storage_a`
and clean them in setup/teardown. Do not modify production `/cfg_*` or `/odo_*` files from tests.

## Commit & Pull Request Guidelines

History follows short Conventional Commit-style subjects, for example
`feat: add redundant flash storage manager`. Keep commits scoped and testable. Pull
requests should explain motivation, verification results, and the relevant task;
include screenshots for UI changes. Hardware changes require board and serial
observations. Protocol changes must update `protocol/` first and synchronize fixtures.

## Configuration & Safety

Do not commit credentials, serial ports, or generated `.pio/` output. Treat
`protocol/` as the source of truth and `STATUS.md` as verified progress;
update the matching file under `tasks/` when an increment is completed.
