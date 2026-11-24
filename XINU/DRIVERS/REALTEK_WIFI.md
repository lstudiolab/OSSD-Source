# Realtek Wi-Fi support in XINU

## Status

**Experimental foundation; no Realtek Wi-Fi chipset is currently verified as functionally supported by XINU.**
This document is the implementation contract and coverage inventory. A PCI/USB
ID match, firmware file being present, or a successful compile is not evidence
of working Wi-Fi.

XINU is intended to support x86-64 and arm64 PCs and laptops. Realtek support
must be implemented against XINU's own bus, DMA, interrupt, firmware, network,
and power-management interfaces. Do not load Linux kernel modules or assume
Linux mac80211/cfg80211 APIs exist in XNU's I/O Kit.

## Driver families

Keep separate family backends. The silicon generations do not share one
universal register protocol.

| Family | Transport | Representative chipsets | Target driver |
|---|---|---|---|
| RTL8xxx legacy | PCIe / USB / SDIO | RTL8188, RTL8192, RTL8723, RTL8812, RTL8814 | `rtl8xxx` |
| RTW88 (802.11ac) | PCIe / USB / SDIO | RTL8723D, RTL8821C, RTL8822B, RTL8822C, RTL8812A, RTL8814A | `rtw88` |
| RTW89 (802.11ax/be) | PCIe / USB | RTL8851B, RTL8852A/B/C, RTL8922A | `rtw89` |

This is a family-level target list, not a claim that every transport or
subsystem variant is supported. Each exact chip/transport/revision combination
must have its own match record and test status.

## Implemented in XINU source

- `osfmk/device/ossd_realtek_wifi.h` defines family identifiers and the PCI
  candidate-match record.
- `osfmk/device/ossd_realtek_wifi.c` matches selected Realtek PCIe IDs and
  rejects non-network PCI class devices. Subsystem wildcard support is present.
- The module is included in the Mach source manifest and exported device
  headers. A host regression test checks positive and negative matches.
- PCIe, USB, and SDIO candidate-identification APIs and provisional match tables are present.
  USB matching checks the vendor-specific interface tuple; SDIO matching checks
  the Realtek SDIO vendor/device pair. IDs identify candidates only and are not a complete vendor hardware database.
- `ossd_realtek_wifi_core.c` provides a transactional lifecycle contract
  (claim, reset, firmware handoff, initialize, start, stop, transmit, scan)
  with failure unwind, including stopping partially started hardware. It requires
  a transport/chipset backend to provide every hardware operation. No Realtek
  hardware backend is implemented.
- `ossd_realtek_wifi_firmware.c` validates firmware metadata, HTTPS provenance,
  filename safety, license/redistribution state, chipset family, transport,
  exact chip name, silicon-revision range, byte length, and SHA-256 through
  an in-tree OSSD SHA-256 implementation. Tests include standard empty, short,
  block-boundary, and multi-block known-answer vectors. The digest comparison
  is constant time. A hash proves that bytes match a trusted manifest; it does
  not authenticate an untrusted manifest or HTTPS source. The lifecycle
  requires validation to pass before claiming hardware or loading firmware.
  No firmware manifest or binary is currently approved for a Realtek chipset.

## Existing XINU primitives and missing interfaces

The source tree contains reusable BSD `ifnet` networking interfaces,
`IODMACommand` DMA abstractions, and IOKit interrupt event-source APIs. PCI
configuration scanning is available through the x86-64 mechanism #1 backend
or a caller-mapped ECAM window. A bounded PCI driver registry now provides
vendor/device/subsystem/class matching, ordered probe attempts, duplicate
function rejection, and deterministic remove callbacks. Its tests cover
matching, failed probes, duplicate attachment, and detach cleanup. The x86-64
configuration-mechanism backend now exposes serialized config reads and writes;
the command-register helper updates only I/O, memory-decode, and bus-master
bits, writing zero to the write-one-to-clear status half. Callers must
serialize the complete read/modify/write sequence and must not enable bus
mastering until DMA mappings are ready.

The PCI resource layer now includes explicit BAR sizing, firmware-assigned
MMIO mapping, and portable ECAM configuration read/write callbacks. BAR sizing
temporarily disables I/O and memory decoding, reads 32-bit or 64-bit size
masks, restores BAR registers, and restores the PCI command register. It does
not assign addresses. BAR mapping accepts memory BARs only and delegates
mapping attributes to a platform callback; the callback must map the range as
device memory, not cacheable RAM. Bounded 8/16/32-bit MMIO accessors validate
alignment and range and issue sequentially consistent fences. The caller must
serialize BAR sizing against all other config-space access.

A platform-neutral DMA provider contract is also implemented. It validates
contiguous DMA segments against alignment, address-width, and boundary
constraints; records transfer direction; and requires explicit synchronization
for CPU/device ownership transitions. Actual physical DMA allocation and cache
maintenance remain platform-provider responsibilities.

The DMA contract now validates one contiguous device-visible segment against
alignment, address-width, and no-crossing-boundary constraints. It records
the transfer direction, rejects incompatible CPU/device synchronization,
and delegates cache maintenance and actual allocation to a platform
provider. The contract is tested, but there is not yet an x86-64 or arm64
physical DMA allocator/IOMMU provider wired into XCU; therefore this does not
yet enable Realtek DMA rings or bus mastering.

The registry remains a binding contract: it does not assign BAR addresses,
allocate DMA, configure MSI/MSI-X, or provide a concrete platform resource
provider. Registry mutation and device lifecycle calls must currently be
externally serialized.

The repository inventory does not currently contain a USB host-controller
stack, an SDIO host-controller stack, an IOKit PCI device provider/resource
binding path, an 802.11 management/security framework, or a Realtek chipset
backend. The BSD network interface APIs do not supply Wi-Fi scanning,
association, key management, or radio control.

Before a chipset backend can be considered functional, XINU must provide:

- PCI BAR assignment/resource-window management and a platform-backed device
  resource provider. BAR sizing and callback-backed MMIO mapping now exist,
  but platform-specific mapping, ownership arbitration, and cleanup integration
  still need implementation.
- USB and SDIO enumeration, host-controller transfers, resource assignment,
  and driver binding, including USB control/bulk/interrupt transfers.
- DMA allocation, mapping, cache maintenance, and teardown.
- MSI/MSI-X and legacy interrupt registration, synchronization, and teardown.
- A firmware loader with bounded reads, integrity checks, and provenance.
- Work queues, timers, locks, reset/recovery, and power-state transitions.
- A network-device contract for scan results, authentication/association,
  transmit/receive queues, link state, and packet ownership.
- An 802.11 security/control plane covering WPA2/WPA3, key installation,
  roaming, regulatory domain, and user-space supplicant integration.

The firmware validator and in-tree SHA-256 verifier are implemented, but they
do not fetch firmware or transfer it to hardware. A trusted, licensed firmware
source and a chipset-specific transfer implementation are still required.
Do not write a chipset backend against guessed APIs; first implement and test
the missing bus, resource, and network interfaces above.

## Bring-up sequence

1. Enumerate the device and bind a candidate driver through the PCI registry.
2. Validate vendor/device/subsystem/revision IDs, BAR type/size/address,
   interrupt capabilities, and DMA constraints before acquiring resources.
3. Quiesce/reset the device with bounded timeouts.
4. Load the exact firmware required by that chipset revision.
5. Initialize power, clocks, efuse/EEPROM calibration, RF, MAC, and PHY.
6. Configure channels, regulatory constraints, antenna paths, and rate control.
7. Register a network interface only after initialization fully succeeds.
8. Implement scan, authentication, association, key setup, TX/RX, disconnect,
   reset, suspend/resume, and hot-unplug handling.
9. Test sustained bidirectional traffic, packet loss, reconnect, suspend/resume,
   and recovery from firmware/transport errors on physical hardware.

A failed probe must unwind DMA, interrupts, mappings, work items, and firmware
state. Optional Wi-Fi failure must never prevent the machine from booting.

## Firmware and licensing

Firmware is separate from driver source. For every firmware blob, record its
filename, chipset/revision, upstream source, checksum, license, redistribution
permission, and required path. Only redistribute blobs whose license permits
it. If redistribution is not permitted, document a user-supplied firmware
installation path and fail clearly when the blob is missing.

A viable firmware source has now been identified: the pinned linux-firmware
WHENCE inventory lists the RTW88 firmware set (including RTL8821C and RTL8822C)
as redistributable under `LICENCE.rtlwifi_firmware.txt`. That license requires
the copyright notice and disclaimer to accompany binary redistribution,
prohibits endorsement using Realtek's name without permission, and prohibits
reverse engineering/decompilation/disassembly of the firmware binary. OSSD has
not yet imported or pinned any firmware blob, verified its SHA-256, or added
the required license notice to a firmware package; firmware loading therefore
remains disabled until those steps are completed.

Do not copy GPL-licensed Linux driver code into the XNU-derived source tree
without a deliberate licensing review. Prefer a clean-room XINU-native
implementation based on public hardware documentation and legally usable
firmware. If a separately licensed driver component is ever used, keep its
license and boundary explicit.

## Support-state rules

- **planned** — family is in scope; no implementation claim.
- **experimental** — code exists and compiles, but physical functional tests
  are incomplete.
- **partial** — a specific chip/transport/revision passed some, but not all,
  required tests; list the missing tests.
- **supported** — exact device variants passed initialization, scan,
  association, WPA2/WPA3 as applicable, sustained TX/RX, reconnect, and
  suspend/resume tests on physical hardware.
- **unsupported** — no usable implementation or required firmware.

Never promote an entire family to supported based on one device.

## Initial compatibility matrix

| Family / variant | Bus | Status | Evidence |
|---|---|---|---|
| RTL8188 / RTL8192 PCIe IDs listed in source | PCIe | experimental matching only | Match regression test; no hardware initialization |
| RTL8xxx USB candidate IDs | USB | experimental matching only | Interface and product-ID tests; no USB host binding |
| RTL8723/8821/8822 SDIO candidate IDs | SDIO | experimental matching only | Vendor/device-ID tests; no SDIO host binding |
| RTL8723 PCIe IDs listed in source | PCIe | experimental matching only | Match regression test; no hardware initialization |
| RTL8812 / RTL8814 PCIe variants | PCIe | planned | Not all PCI IDs mapped; no hardware test |
| RTL8821 / RTL8822 PCIe variants | PCIe | experimental matching only | Match regression test; no hardware initialization |
| RTL8851B / RTL8852 variants | PCIe / USB | planned | Candidate IDs only; no hardware initialization |
| RTL8922 variants | PCIe / USB | planned | No XINU driver or hardware test |

## Laptop validation

The HP ProBook 450 G-series name alone does not identify its Wi-Fi module:
configurations vary by generation and regional SKU. Keep the machine in the
physical test matrix, but identify its installed wireless controller from
PCI/USB enumeration before claiming a match. This does not limit general
development: all supported Realtek IDs should be covered by the compatibility
database, not only one laptop.

## Upstream technical references

- Linux RTW89 chipset/transport configuration:
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtw89/Kconfig
- RTW88 family and transport coverage reference:
  https://github.com/lwfinger/rtw88
- Firmware provenance and redistribution metadata:
  https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/tree/WHENCE

These are research references only. Their code and APIs are not XINU driver
implementations, and their licensing must be reviewed independently.


## Match-table references

The initial PCI identifiers are cross-checked against public PCI ID and
wireless-driver hardware tables. References are used for identification only;
no Linux driver source is imported into XINU:

- https://cateee.net/lkddb/web-lkddb/RTW88.html
- https://cateee.net/lkddb/web-lkddb/RTL8192CE.html
- https://wireless.docs.kernel.org/en/latest/en/users/drivers/rtl819x.html
