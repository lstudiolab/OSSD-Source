# XINU hardware support plan

## Compatibility objective

XINU is intended to run on a broad range of x86-64 and arm64 PCs and laptops,
not only the HP ProBook 450 G6. The ProBook is one physical validation target.
Its installed component IDs help reproduce and debug that machine's hardware,
but they are not prerequisites for designing or implementing general support.

Hardware support must be layered. Generic bus, DMA, interrupt, power, firmware,
network, audio, storage, and input services should be reusable across devices.
Vendor- and chipset-specific drivers should implement only the hardware-specific
operations. Device matching must use published identifiers and revision data,
with quirk tables where required.

A device is **planned** until its driver is implemented, built, initialized,
and tested on physical hardware. Documentation or a device-ID match alone is
not proof of working support.

## Support scope

The long-term compatibility target includes:

- x86-64 and arm64 firmware and boot environments (UEFI first)
- PCI and PCI Express devices, including bridges and hot-plug where supported
- USB host controllers and USB peripherals
- ACPI platform devices, power management, battery, thermal sensors, and lid
- I2C and SPI-connected platform devices where the platform exposes them
- NVMe, AHCI/SATA, and USB mass-storage devices
- Ethernet and Wi-Fi adapters across multiple vendors and generations
- HDA and platform audio controllers, codecs, speakers, headphones, and mics
- USB, Bluetooth, PS/2, I2C-HID, and other common input devices
- Display controllers and common display outputs
- Common laptop features such as keyboard backlight, touchpad, brightness,
  suspend/resume, and power/thermal management

This is a coverage roadmap, not a claim that these classes are already
implemented. Each class and device family must have a tracked implementation
and validation status.

## HP ProBook 450 G6: first laptop validation target

HP's model documentation lists several possible wireless configurations:

| Adapter | Bus / interface | XINU status |
|---|---|---|
| Intel Dual Band Wireless-AC 9560 | Intel CNVi / CNVio | Not implemented or verified |
| Realtek RTL8821CE | PCIe Wi-Fi, USB Bluetooth | Not implemented or verified |
| Realtek RTL8822BE | PCIe Wi-Fi, USB Bluetooth | Not implemented or verified |

Candidate identifiers include:

| Adapter | PCI vendor:device | Known HP subsystem IDs | Notes |
|---|---|---|---|
| Intel Wireless-AC 9560 | 8086:9df0 | Must be read from the target machine | CNVi; not an ordinary PCIe WLAN card |
| Realtek RTL8821CE | 10ec:c821 (also 10ec:b821 on some revisions) | 103c:831a, 103c:884d | PCIe WLAN; separate chipset/firmware path |
| Realtek RTL8822BE | 10ec:b822 | 103c:831b, 103c:840b | PCIe WLAN; separate chipset/firmware path |

These are examples for the ProBook validation matrix, not an exhaustive
supported-device list. The exact IDs on one laptop are useful for confirming
that machine's test result; they must not restrict driver development to that
one configuration. The Intel 9560 CNVi interface must not be treated as
interchangeable with ordinary PCIe WLAN devices. Realtek adapters require
their own chipset-specific initialization, firmware handling, and radio
support.

## HP ProBook 450 G6: audio

The platform may expose an Intel High Definition Audio or Intel Smart Sound
Technology (SST) controller and a Realtek codec. A Realtek ALC236 codec ID
has been reported for some 450 G6 configurations, but the codec, subsystem
ID, and Intel audio-controller PCI ID for the actual target machine have not
been verified. ALC236 must remain a candidate, not an assumed match.

General audio support must separate controller drivers from codec drivers.
Required coverage includes controller discovery and initialization, codec
enumeration and verb execution, playback and capture DMA streams, interrupts,
microphone input, speaker/headphone routing, jack detection, and power-state
transitions. Intel SST and HDA are distinct controller paths and must not be
collapsed into one assumed implementation.

## Driver architecture requirements

- Keep bus enumeration and device matching separate from function drivers.
- Match on vendor, device, subsystem, revision, class, and compatible strings
  as appropriate to the bus; use narrowly scoped quirk tables.
- Provide generic PCI/PCIe, USB, ACPI, I2C, and SPI discovery interfaces.
- Provide reusable DMA mapping, interrupt, timer, locking, and power-management
  services with explicit ownership and teardown rules.
- Keep chipset-specific register programming and firmware protocols inside
  their respective drivers.
- Define stable interfaces for network, audio, storage, display, and input
  classes so drivers do not depend on unrelated device implementations.
- Support optional firmware with documented provenance, licensing, hashes,
  and redistribution rights. A first-party driver does not make vendor
  firmware first-party or freely redistributable.
- Fail safely when optional hardware or firmware is absent or initialization
  fails; do not prevent unrelated devices from initializing.
- Maintain a hardware compatibility database with supported, partial,
  experimental, and unsupported states and exact tested revisions.
- Add emulator/virtual-device tests where practical, then validate on physical
  hardware. Virtual tests do not replace physical validation.

## Acceptance criteria

A bus is supported only after enumeration, resource assignment, interrupt
delivery, DMA, error handling, and teardown tests pass on representative
hardware or a suitable validated emulator.

A Wi-Fi device is supported only after XINU can identify it, load any
permitted firmware, initialize the radio, scan, authenticate, associate,
transmit and receive data, recover from errors, and handle suspend/resume.

An audio device is supported only after XINU can enumerate the controller and
codec, play audio, capture microphone input, route speakers/headphones, and
recover from stream and power-state changes.

A laptop model is not declared supported based solely on its CPU or one
peripheral. Its boot, storage, network, audio, graphics/display, input,
battery, thermal, and power-management paths must be tested and the exact
configuration recorded.

## Per-machine diagnostic data

For a reproducible test report, record the machine model and firmware version,
CPU architecture, PCI/USB IDs, ACPI identifiers, controller and codec IDs,
driver versions, firmware filenames and hashes, and test results. On Windows,
Device Manager's **Properties → Details → Hardware Ids** is one way to collect
these identifiers. Equivalent inventory can be collected from other operating
systems or firmware diagnostics.

These details improve test accuracy; they are not a prerequisite for building
general-purpose driver infrastructure or expanding support for other PCs.
