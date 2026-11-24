/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_realtek_wifi.h>

#define REALTEK_VENDOR_ID 0x10ecU
#define ANY_SUBSYSTEM 0xffffU

/*
 * PCI IDs are candidate matches collected for initial bring-up. The table
 * must be expanded only with verified silicon/revision documentation.
 * USB and SDIO products use their own transport-specific match tables.
 */
static const struct ossd_rtw_pci_match rtw_pci_ids[] = {
	{ REALTEK_VENDOR_ID, 0x8176, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8188CE" },
	{ REALTEK_VENDOR_ID, 0x8178, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192CE" },
	{ REALTEK_VENDOR_ID, 0x8723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723AE" },
	{ REALTEK_VENDOR_ID, 0x8812, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8812AE" },
	{ REALTEK_VENDOR_ID, 0x8177, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8191CE" },
	{ REALTEK_VENDOR_ID, 0x8191, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192CE" },
	{ REALTEK_VENDOR_ID, 0x8179, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8188EE" },
	{ REALTEK_VENDOR_ID, 0x818b, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192EE" },
	{ REALTEK_VENDOR_ID, 0xb723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723BE" },
	{ REALTEK_VENDOR_ID, 0xd723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723DE" },
	{ REALTEK_VENDOR_ID, 0x8821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8821AE" },
	{ REALTEK_VENDOR_ID, 0xb822, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8822BE" },
	{ REALTEK_VENDOR_ID, 0xb821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8821CE" },
	{ REALTEK_VENDOR_ID, 0xc821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8821CE" },
	{ REALTEK_VENDOR_ID, 0xc822, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8822CE" },
	{ REALTEK_VENDOR_ID, 0x8852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852AE" },
	{ REALTEK_VENDOR_ID, 0xb852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852BE" },
	{ REALTEK_VENDOR_ID, 0xc852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852CE" },
	{ REALTEK_VENDOR_ID, 0xb851, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8851BE" }
};

const struct ossd_rtw_pci_match *
ossd_rtw_pci_match_device(const struct ossd_pci_device *device)
{
	uint32_t i;

	if (device == 0 || device->vendor_id != REALTEK_VENDOR_ID ||
	    device->class_code != 0x02) {
		return 0;
	}
	for (i = 0; i < sizeof(rtw_pci_ids) / sizeof(rtw_pci_ids[0]); i++) {
		const struct ossd_rtw_pci_match *match = &rtw_pci_ids[i];
		if (device->device_id != match->device_id) {
			continue;
		}
		if (match->subsystem_vendor_id != ANY_SUBSYSTEM &&
		    device->subsystem_vendor_id != match->subsystem_vendor_id) {
			continue;
		}
		if (match->subsystem_id != ANY_SUBSYSTEM &&
		    device->subsystem_id != match->subsystem_id) {
			continue;
		}
		return match;
	}
	return 0;
}

const char *
ossd_rtw_family_name(enum ossd_rtw_family family)
{
	switch (family) {
	case OSSD_RTW_FAMILY_LEGACY: return "rtl8xxx";
	case OSSD_RTW_FAMILY_RTW88: return "rtw88";
	case OSSD_RTW_FAMILY_RTW89: return "rtw89";
	default: return "unknown";
	}
}
