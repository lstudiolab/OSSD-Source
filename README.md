# OSSD — Open Source System Distribution

OSSD is an open-source operating-system project intended to become a broadly
compatible system for personal computers.

## Kernel

The kernel project is **XINU**, derived from the published XNU source tree
and being adapted for OSSD. The current source retains XNU's Mach, BSD, and
I/O Kit organization. This repository is an active development source tree;
it must not be described as a finished or hardware-validated operating
system until its build, boot, and device tests pass.

## Development goals

- x86-64 and arm64 CPU support.
- Native kernel and device-driver development.
- Broad hardware compatibility across multiple vendors and generations.
- Public source, documented provenance, and reproducible build/test steps.
- Clear separation between OSSD-authored code and third-party components,
  including their licenses and firmware requirements.

## Licensing and provenance

This repository contains multiple components and license regimes. Read the
license and attribution notices that apply to each component. XINU's imported
XNU-derived code is subject to APSL 2.0; see [XINU/LICENSE](XINU/LICENSE),
[XINU/NOTICE](XINU/NOTICE), and [XINU/README.md](XINU/README.md).
The repository-level license files do not replace or override licenses
attached to upstream files or separately licensed components.

## First hardware validation target

The HP ProBook 450 G6 is the first laptop target for Wi-Fi and audio driver
work. Its exact hardware configuration must be identified before compatibility
can be confirmed. See [XINU/HARDWARE-SUPPORT.md](XINU/HARDWARE-SUPPORT.md).

## Project status

OSSD is under active development. A feature is complete only when it is
implemented, integrated, built, and tested; planned support is not the same
as working support.
