# XINU driver architecture and roadmap

## Purpose

This document defines the intended driver architecture for XINU. The goal is
broad PC and laptop compatibility across x86-64 and arm64, not a driver set
tied to one machine. Hardware-specific IDs from individual test machines are
inputs to the compatibility database and validation process, not gates on
general driver development.

This is an architecture and implementation roadmap. It does not claim that
the listed buses, classes, or devices are already implemented.

## Layering

1. **Platform discovery**
   - UEFI handoff and firmware tables
   - ACPI namespace and device enumeration
   - Device Tree discovery on applicable arm64 platforms
   - Platform identity, firmware revision, and boot configuration

2. **Bus and transport drivers**
   - PCI/PCIe configuration, BAR/resource assignment, bridges, MSI/MSI-X
   - USB host controllers and hubs
   - I2C and SPI controllers
   - Platform/ACPI devices and supported virtual buses

3. **Kernel device services**
   - DMA mapping and IOMMU integration
   - Interrupt registration, masking, synchronization, and teardown
   - MMIO/PIO resource ownership
   - Timers, work queues, locking, and deferred execution
   - Power-state transitions, reset, timeout, and recovery
   - Firmware request, validation, loading, and provenance tracking

4. **Device-class interfaces**
   - Network interfaces and packet queues
   - Audio controllers, codec abstractions, and stream queues
   - Block storage and command queues
   - Display and graphics device interfaces
   - HID/input event interfaces
   - Bluetooth transport interfaces
   - Sensor, battery, thermal, and platform-control interfaces

5. **Chipset-specific drivers**
   - Own register definitions, device quirks, firmware protocol, and
     hardware-specific state machines.
   - Register with the appropriate bus and device-class interfaces.
   - Must not duplicate generic DMA, interrupt, or power-management logic.

## Device matching

Use the identifiers appropriate to each transport:

- PCI/PCIe: vendor, device, subsystem vendor, subsystem device, revision,
  class, and programming interface.
- USB: vendor, product, device revision, interface class/subclass/protocol.
- ACPI: hardware IDs, compatible IDs, and firmware-provided resources.
- Device Tree: compatible strings and declared resources.
- I2C/SPI: firmware-described identity and bus-specific match data.

A match only selects a candidate driver. Initialization must validate the
hardware revision, resources, capabilities, and firmware before exposing a
working device. Quirks must be scoped to exact affected revisions and include
a reason and test reference.

## Driver lifecycle contract

Every driver should implement a consistent lifecycle:

1. Match and probe without taking ownership of unrelated resources.
2. Validate hardware identity and required capabilities.
3. Acquire MMIO/PIO, DMA, interrupt, and firmware resources.
4. Reset and initialize the device with bounded timeouts.
5. Register the device-class interface only after initialization succeeds.
6. Handle runtime I/O, errors, reset, and optional hot-plug.
7. Quiesce work, stop DMA, release interrupts and resources, and detach safely.

A failed probe must leave the device and shared kernel state safe. A failed
optional device must not stop boot or block unrelated devices.

## First coverage priorities

The order below prioritizes bootability and common PC/laptop utility. It is
not a promise of immediate support for every model in each family.

| Priority | Area | Initial families / work | Validation |
|---|---|---|---|
| P0 | Platform | UEFI, ACPI, PCI/PCIe, timers, interrupt controllers, DMA | QEMU/virtual machines and physical x86-64/arm64 systems |
| P0 | Storage | NVMe, AHCI/SATA, USB mass storage | Boot, read/write, error recovery |
| P1 | Wired network | Common Intel and Realtek Ethernet families | Link, DHCP/static IP, sustained RX/TX |
| P1 | USB/input | xHCI, USB hubs, HID keyboards/mice, touchpads where possible | Enumeration, input, hot-plug |
| P1 | Audio | HDA controller and codec framework; SST as a separate path | Playback, microphone capture, jack routing |
| P1 | Wireless | Intel and Realtek families with separate chipset drivers | Scan, association, RX/TX, reconnect, suspend/resume |
| P2 | Graphics | Firmware framebuffer first, then native GPU/display drivers | Multi-resolution display, modeset, recovery |
| P2 | Laptop platform | Battery, thermal sensors, fans, brightness, lid, sleep/resume | Power and thermal test matrix |
| P2 | Bluetooth | USB/UART transports and common controller families | Pairing, reconnect, audio/input profiles |

## Compatibility database

Keep a machine-readable inventory alongside human-readable test reports.
Each record should include:

- Vendor/device and subsystem identifiers or firmware compatible string
- Hardware revision and machine/platform context
- Driver and firmware versions
- Support state: unsupported, planned, experimental, partial, or supported
- Build result and test date
- Tests passed and known limitations
- Firmware license/provenance and redistribution status

Do not label a device supported because a driver matches its ID or compiles.
Support requires successful initialization and the relevant functional tests
on real hardware. Mark untested revisions separately.

## Test strategy

- Unit-test identifier matching, resource validation, state machines, and
  timeout/error paths.
- Test bus enumeration and generic services in emulators where available.
- Use virtual devices for deterministic boot, storage, and network regression
  tests where the emulator models the required behavior.
- Maintain a physical hardware matrix covering multiple vendors, generations,
  desktop systems, and laptops.
- For wireless and audio, require end-to-end physical tests; emulation alone
  cannot validate radio behavior, codec routing, microphones, or acoustics.
- Record failures as unsupported or partial rather than silently falling back
  to a false success state.

## Source and firmware policy

Prefer OSSD-authored implementations for the system foundation. If an
external specification, codebase, or firmware is needed, record its origin,
license, copyright notices, and redistribution terms before use. Do not copy
third-party driver code or redistribute firmware merely because it is
available online. Keep driver source licensing explicit and compatible with
the XINU source tree's applicable licensing obligations.

## Current implementation status

The imported XNU-derived source tree provides substantial kernel and I/O Kit
infrastructure, but this does not establish complete PC driver coverage.
The hardware inventory must be audited against actual compiled driver sources.
As of this roadmap, no claim is made that the HP ProBook 450 G6 Wi-Fi or audio
paths, or broad third-party PC hardware coverage, are working.
