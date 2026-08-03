# VeloPulse

[English](README.md) | [Русский](README.ru.md)

VeloPulse is an open-source, battery-powered bicycle computer built around the
Super-nRF52840 board and SSD1306 OLED displays. The primary display is 128×64;
a compatible 128×32 firmware profile is maintained from the same renderer. It
measures speed, trip distance, average and maximum speed, moving time, wheel
revolutions, and the total odometer, with configuration through an Android BLE app.

The original product requirements are available in [`bike-tz.md`](bike-tz.md)
(Russian).

## Project status

The firmware provides wheel-pulse processing, trip metrics, redundant storage,
complete BLE protocol v1, and compile-time OLED profiles for SSD1306 128×64 and
128×32. The Flutter Android MVP scans, pairs, synchronizes, and displays live data.

- Native domain tests: 78 passing.
- Both nRF52840 OLED profiles build successfully; 128×32 is hardware-verified.
- The simulator verifies 18 pixel-golden frames across both display geometries.
- Storage fallback and reboot recovery are verified on a XIAO-compatible board.
- Android discovery/connect is verified with real firmware; the full hardware gate remains.

See [`STATUS.md`](STATUS.md) for verified progress and [`TODO.md`](TODO.md) for the
project roadmap.

## Versioning

Product versions live in [`version.toml`](version.toml) (BLE protocol, Arduino
firmware, Zephyr firmware, mobile app). After editing, run:

```bash
python3 tools/sync_versions.py
```

CI verifies that generated files match the manifest.

## Repository layout

```text
.
├── firmware/    PlatformIO firmware, domain library, and tests
├── simulator/   Headless/GUI OLED simulator and golden frames
├── protocol/    Firmware-to-application BLE contract
├── docs/        Architecture, hardware, planning, and acceptance documents
└── tasks/       Structured backlog grouped by subsystem
```

## Documentation

### Design and planning

| Document | Contents |
| --- | --- |
| [Project overview](docs/01-project-overview.md) | System scope, v1.0 boundaries, core decisions, units, glossary, repository structure, and release definition |
| [Development plan](docs/02-development-plan.md) | Eight development stages, estimates, dependencies, milestones, and Definition of Done |
| [Decisions and risks](docs/07-decisions-and-risks.md) | Architecture decision records and the project risk register |

### Architecture and hardware

| Document | Contents |
| --- | --- |
| [Firmware architecture](docs/03-firmware-architecture.md) | Firmware layers, cooperative scheduler, ISR pulse processing, fixed-point calculations, ride and power state machines, display, storage, BLE, and diagnostics |
| [Mobile application architecture](docs/04-mobile-app-architecture.md) | Planned Flutter/Riverpod stack, connection state machine, repository layer, configuration drafts, screens, permissions, and testing strategy |
| [Hardware design](docs/05-hardware-design.md) | BOM, wiring, pinout, battery measurement and calibration, charging, mechanical installation, power budget, and hardware checks |

### Firmware ↔ application contract

| Document | Contents |
| --- | --- |
| [BLE protocol](protocol/ble-protocol.md) | GATT service, characteristics, versioning, advertising, connection behavior, security, bonding, and error handling |
| [UUID registry](protocol/uuids.md) | UUIDs, characteristic properties, advertising data, and change rules |
| [Binary structures](protocol/data-structures.md) | Byte-level layouts, enumerations, command/status codes, dangerous-command confirmation, and fixtures |

### Verification

| Document | Contents |
| --- | --- |
| [Testing and acceptance](docs/06-testing-and-acceptance.md) | Native and embedded tests, test rigs, measurement procedures, 32 acceptance criteria, field testing, and release regression |

Most detailed design documents are currently written in Russian. The source code,
binary protocol names, and repository identifiers use English.

## Firmware quick start

The tested environment uses PlatformIO and the Seeed XIAO-compatible Adafruit nRF52
Arduino core.

```bash
cd firmware

# Run host-side domain tests
pio test -e native

# Build production firmware
pio run -e xiao_ble_sense
# Build the compatible 128x32 firmware
pio run -e xiao_ble_sense_128x32

# Upload to a connected board
pio run -e xiao_ble_sense -t upload

# Open the 115200 baud serial monitor
pio device monitor -b 115200
```

Wiring and device behavior are described in
[`firmware/README.md`](firmware/README.md). The firmware README is currently in
Russian.

## OLED simulator

The simulator renders the same domain formatting and layout code used by the
firmware.

```bash
cd simulator
./test.sh
```

Additional setup and GUI commands are documented in
[`simulator/README.md`](simulator/README.md).

## Mobile application

The Flutter Android MVP is implemented and successfully connected to real firmware.
The remaining work is the full hardware acceptance gate and extended v1.0 screens:

- [`docs/04-mobile-app-architecture.md`](docs/04-mobile-app-architecture.md)
- [`tasks/mobile/README.md`](tasks/mobile/README.md)

## Protocol change policy

The `protocol/` directory is the single source of truth shared by firmware and
the mobile application. Protocol changes must follow this order:

1. Update the tables and version in `protocol/` and add or update fixtures.
2. Update firmware and mobile codecs against the same fixtures.
3. Run golden tests on both sides before merging.

Changing implementation code before updating the protocol contract is considered
a process defect. See the
[protocol change procedure](docs/02-development-plan.md#3-управление-изменениями-протокола).

## Roadmap

The structured roadmap is split by subsystem:

- [Firmware](tasks/firmware/README.md)
- [Storage](tasks/firmware/storage.md)
- [BLE](tasks/firmware/ble.md)
- [Hardware](tasks/hardware/README.md)
- [Mobile](tasks/mobile/README.md)
- [Verification and release](tasks/verification/README.md)
- [Post-v1.0 ideas](tasks/future/README.md)
