# XINU hardware support plan

## Compatibility objective

XINU must not be limited to one laptop model. The HP ProBook 450 G6 is the
first physical validation target, while the driver architecture should allow
additional vendors, device IDs, and hardware revisions to be added without
changing generic kernel interfaces.

A driver is **planned** until it is implemented, built, initialized on the
device, and tested. Documentation or a PCI ID match alone is not proof of
working support.

## HP ProBook 450 G6: Wi-Fi variants

HP's model documentation lists several possible wireless configurations:

| Adapter | Bus / interface | XINU status |
|---|---|---|
| Intel Dual Band Wireless-AC 9560 | Intel CNVi / CNVio | Not implemented or verified |
| Realtek RTL8821CE | PCIe Wi-Fi, USB Bluetooth | Not implemented or verified |
| Realtek RTL8822BE | PCIe Wi-Fi, USB Bluetooth | Not implemented or verified |

The exact adapter installed in the target laptop must be identified by its
PCI hardware ID and subsystem ID. Candidate identifiers include:

| Adapter | PCI vendor:device | Known HP subsystem IDs | Notes |
|---|---|---|---|
| Intel Wireless-AC 9560 | 8086:9df0 | Must be read from the target machine | CNVi; platform-integrated connectivity, not an ordinary PCIe WLAN card |
| Realtek RTL8821CE | 10ec:c821 (also 10ec:b821 on some revisions) | 103c:831a, 103c:884d | PCIe WLAN; separate chipset/firmware path |
| Realtek RTL8822BE | 10ec:b822 | 103c:831b, 103c:840b | PCIe WLAN; separate chipset/firmware path |

These IDs are identification candidates, not proof that every listed
subsystem is installed in every ProBook 450 G6. Match the complete ID from
the actual laptop before enabling a hardware-specific initialization path.
The Intel 9560 is a CNVi device and must not be treated as interchangeable
with ordinary PCIe Intel WLAN devices. The two Realtek adapters require their
own chipset-specific initialization, firmware handling, and radio/transport
support.

## HP ProBook 450 G6: audio

The platform may expose an Intel High Definition Audio or Intel Smart Sound
Technology (SST) controller and a Realtek codec. A Realtek ALC236 codec ID
has been reported for some 450 G6 configurations, but the codec, subsystem
ID, and Intel audio-controller PCI ID for the actual target machine have not
yet been verified. Do not bind a driver to ALC236 until the full HDAUDIO ID
and controller ID are confirmed.

Required coverage includes:

- Controller discovery and initialization (HDA and, where present, Intel SST)
- Codec enumeration and verb execution
- Playback stream setup, DMA, and interrupt handling
- Capture stream setup and microphone input
- Speaker, headphone, and microphone jack routing
- Power management, suspend, resume, and error recovery

## Driver architecture requirements

- Match devices using vendor, device, subsystem, and revision IDs where
  required; do not rely on marketing names alone.
- Keep bus enumeration, DMA, interrupt, power-management, and firmware-loading
  interfaces generic.
- Keep chipset-specific register programming in individual drivers.
- Validate firmware provenance, license, and redistribution rights. A
  first-party driver does not automatically make vendor firmware first-party
  or freely redistributable.
- Provide safe failure paths when optional hardware or firmware is absent.
- Add device-specific tests and record the exact hardware revision tested.
- Do not copy third-party driver code into XINU without explicit provenance,
  license review, and project approval.

## Acceptance criteria

A Wi-Fi device is supported only after XINU can identify it, load any
permitted firmware, initialize the radio, scan, authenticate, associate,
transmit and receive data, recover from errors, and handle suspend/resume.

An audio device is supported only after XINU can enumerate the controller and
codec, play audio, capture microphone input, route speakers/headphones, and
recover from stream and power-state changes.

## Hardware IDs needed for the first physical target

On Windows, open Device Manager and copy **Properties → Details → Hardware
Ids** for:

1. The Wi-Fi adapter under Network adapters.
2. The audio controller under System devices or Sound, video and game
   controllers.
3. The Realtek audio codec, if it appears as a separate device (often an
   HDAUDIO\\FUNC_01 ID rather than a PCI ID).

Record all returned IDs, not only the first line. This distinguishes the
actual machine configuration from other ProBook 450 G6 variants.
