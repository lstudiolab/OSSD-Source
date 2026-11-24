# XINU Kernel

XINU is the kernel project used by the Open Source System Distribution (OSSD).

## Origin and modification status

XINU is derived from the open-source XNU source tree maintained in the
`lstudiolab/darwin-xnu` repository, itself based on Apple's published XNU
source. This repository is an independent, modified project; it is not an
Apple product and is not endorsed by Apple.

The imported XNU source has been modified for OSSD, including source-tree
cleanup and subsequent OSSD-specific work. Files changed from the imported
upstream snapshot must retain the notices required by their applicable
licenses. See [NOTICE](NOTICE), [MODIFICATIONS.md](MODIFICATIONS.md), and
[LICENSE](LICENSE).

## Licensing

The XNU-derived covered code in this directory is subject to the Apple Public
Source License 2.0 (APSL 2.0), reproduced in [LICENSE](LICENSE). Do not assume
that every file in the wider OSSD repository uses the same license: retain and
follow the license and attribution notices attached to each component and
file. New OSSD-authored code must carry an explicit license notice before it
is distributed.

## Architecture direction

The current source base retains XNU's hybrid-kernel organization, including
Mach, BSD, and I/O Kit subsystems. OSSD's goal is to adapt this foundation into
XINU for OSSD rather than claim that the imported source is already a
completed, bootable OSSD kernel.

The initial supported CPU targets are:

- x86-64
- arm64

A target is not considered supported merely because source directories or
architecture headers exist. It must build, boot, and pass relevant hardware
tests.

## Hardware support policy

Hardware support is implemented through maintainable native drivers and documented device identifiers. See [the driver architecture and roadmap](DRIVERS/README.md) for the bus, device-class, lifecycle, compatibility-database, and validation strategy. The initial laptop validation target is the
HP ProBook 450 G6. Its Wi-Fi adapter varies by configuration; see
[HARDWARE-SUPPORT.md](HARDWARE-SUPPORT.md).

A device is marked **supported** only after its driver is implemented,
integrated into the build, and tested on physical hardware. Firmware
requirements and redistribution permissions must be documented separately.
