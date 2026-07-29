# VeloPulse

[English](README.md) | [Русский](README.ru.md)

VeloPulse is an open-source, battery-powered bicycle computer built around the
Super-nRF52840 board and a 128×32 SSD1306 OLED display. It measures speed, trip
distance, average and maximum speed, moving time, wheel revolutions, and the
total odometer. Configuration and diagnostics through an Android BLE application
are planned for v1.0.

The original product requirements are available in [`bike-tz.md`](bike-tz.md)
(Russian).

## Project status

The current firmware provides wheel-pulse processing, fixed-point trip metrics,
the OLED interface, battery monitoring, display power saving, and redundant
InternalFS storage with A/B slots, CRC32, record versions, and corruption recovery.

- Native domain tests: 30 passing.
- nRF52840 production build: passing and verified on hardware.
- OLED simulator and pixel-golden tests: passing.
- Storage fallback and reboot recovery: verified on a XIAO-compatible board.
- Next milestone: wear-aware odometer autosaving during a ride.
- BLE firmware and the Flutter application are not implemented yet.

See [`STATUS.md`](STATUS.md) for verified progress and [`TODO.md`](TODO.md) for the
project roadmap.

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

The Android Flutter application is part of the v1.0 roadmap but has not been
scaffolded yet. Its architecture and task list are available in:

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
