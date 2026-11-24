/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_REALTEK_WIFI_H
#define OSSD_REALTEK_WIFI_H

#include <stdint.h>
#include <device/ossd_pci.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ossd_rtw_family {
	OSSD_RTW_FAMILY_NONE = 0,
	OSSD_RTW_FAMILY_LEGACY,
	OSSD_RTW_FAMILY_RTW88,
	OSSD_RTW_FAMILY_RTW89
};

struct ossd_rtw_pci_match {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id; /* 0xffff means any */
	uint16_t subsystem_id;        /* 0xffff means any */
	enum ossd_rtw_family family;
	const char *chip_name;
};

/*
 * Identifies a candidate PCIe chipset only. It does not probe hardware,
 * load firmware, or imply that a functional driver is available.
 */
const struct ossd_rtw_pci_match *ossd_rtw_pci_match_device(
    const struct ossd_pci_device *device);
const char *ossd_rtw_family_name(enum ossd_rtw_family family);

#ifdef __cplusplus
}
#endif

#endif /* OSSD_REALTEK_WIFI_H */
