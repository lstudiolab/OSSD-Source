#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
CC=${CC:-cc}
OUT=${TMPDIR:-/tmp}/ossd-realtek-pci-resolve-test
"$CC" -std=c11 -Wall -Wextra -Werror \
  -I"$ROOT/osfmk" \
  "$ROOT/DRIVERS/tests/realtek_pci_resolve_test.c" \
  "$ROOT/osfmk/device/ossd_realtek_wifi.c" \
  -o "$OUT"
"$OUT"
rm -f "$OUT"
printf '%s\n' "Realtek PCIe resolution tests passed"
