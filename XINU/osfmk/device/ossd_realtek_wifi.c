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
	    OSSD_RTW_FAMILY_LEGACY, "RTL8188CE", 0 },
	{ REALTEK_VENDOR_ID, 0x8178, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192CE", 0 },
	{ REALTEK_VENDOR_ID, 0x8723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723AE", 0 },
	{ REALTEK_VENDOR_ID, 0x8812, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8812AE", "rtl8812aefw.bin" },
	{ REALTEK_VENDOR_ID, 0x8177, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8191CE", 0 },
	{ REALTEK_VENDOR_ID, 0x8191, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192CE", 0 },
	{ REALTEK_VENDOR_ID, 0x8179, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8188EE", 0 },
	{ REALTEK_VENDOR_ID, 0x818b, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8192EE", 0 },
	{ REALTEK_VENDOR_ID, 0xb723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723BE", 0 },
	{ REALTEK_VENDOR_ID, 0xd723, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8723DE", 0 },
	{ REALTEK_VENDOR_ID, 0x8821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_LEGACY, "RTL8821AE", "rtl8821aefw.bin" },
	{ REALTEK_VENDOR_ID, 0xb822, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8822BE", "rtw8822b_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xb821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8821CE", "rtw8821c_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xc821, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8821CE", "rtw8821c_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xc822, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW88, "RTL8822CE", "rtw8822c_fw.bin" },
	{ REALTEK_VENDOR_ID, 0x8852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852AE", "rtw8852a_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xb852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852BE", "rtw8852b_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xc852, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8852CE", "rtw8852c_fw.bin" },
	{ REALTEK_VENDOR_ID, 0xb851, ANY_SUBSYSTEM, ANY_SUBSYSTEM,
	    OSSD_RTW_FAMILY_RTW89, "RTL8851BE", "rtw8851b_fw.bin" }
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


int
ossd_rtw_pci_resolve_device(const struct ossd_pci_device *device,
    struct ossd_rtw_match *result)
{
	const struct ossd_rtw_pci_match *candidate;

	if (device == 0 || result == 0) return -1;
	candidate = ossd_rtw_pci_match_device(device);
	if (candidate == 0 || candidate->firmware_name == 0) return -2;

	result->transport = OSSD_RTW_TRANSPORT_PCIE;
	result->family = candidate->family;
	result->chip_name = candidate->chip_name;
	result->firmware_name = candidate->firmware_name;
	return 0;
}

/*
 * USB product IDs are limited to WLAN interfaces and are matched on the
 * vendor-specific interface tuple. These records identify candidates only.
 */
#define USB_VENDOR_REALTEK 0x0bdaU
#define USB_IFACE_VENDOR_SPECIFIC 0xffU

static const uint16_t rtw_usb_products[] = {
	0x8724, 0x1724, 0x0724, 0x818b, 0xb720, 0x8176, 0x8178,
	0x817f, 0x8179, 0xf179, 0xb711, 0xf192, 0x2006, 0xc811,
	0x8731, 0x8811, 0xb820, 0xb82b, 0xc80c, 0xc811, 0xc820,
	0xc821, 0xc82a, 0xc82b, 0x8832, 0x885a, 0x885c
};

/*
 * SDIO vendor/device identifiers are from the public SDIO ID registry.
 * They are not proof that the corresponding silicon backend is implemented.
 */
#define SDIO_VENDOR_REALTEK 0x024cU
static const struct ossd_rtw_sdio_device rtw_sdio_ids[] = {
	{ SDIO_VENDOR_REALTEK, 0xb723 }, { SDIO_VENDOR_REALTEK, 0xd723 },
	{ SDIO_VENDOR_REALTEK, 0xb821 }, { SDIO_VENDOR_REALTEK, 0xc821 },
	{ SDIO_VENDOR_REALTEK, 0xd821 }, { SDIO_VENDOR_REALTEK, 0xb822 },
	{ SDIO_VENDOR_REALTEK, 0xc822 }
};
static const struct ossd_rtw_match rtw_sdio_matches[] = {
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8723BS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8723DS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8821BS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8821CS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8821DS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8822BS", 0 },
	{ OSSD_RTW_TRANSPORT_SDIO, OSSD_RTW_FAMILY_RTW88, "RTL8822CS", 0 }
};

const struct ossd_rtw_match *
ossd_rtw_usb_match_device(const struct ossd_rtw_usb_device *device)
{
	uint32_t i;
	if (device == 0 || device->vendor_id != USB_VENDOR_REALTEK ||
	    device->interface_class != USB_IFACE_VENDOR_SPECIFIC ||
	    device->interface_subclass != USB_IFACE_VENDOR_SPECIFIC ||
	    device->interface_protocol != USB_IFACE_VENDOR_SPECIFIC) {
		return 0;
	}
	for (i = 0; i < sizeof(rtw_usb_products) / sizeof(rtw_usb_products[0]); i++) {
		if (device->product_id == rtw_usb_products[i]) {
			/* Product-to-family is intentionally resolved only for known groups. */
			static const struct ossd_rtw_match legacy =
			    { OSSD_RTW_TRANSPORT_USB, OSSD_RTW_FAMILY_LEGACY, "RTL8xxx USB", 0 };
			static const struct ossd_rtw_match rtw88 =
			    { OSSD_RTW_TRANSPORT_USB, OSSD_RTW_FAMILY_RTW88, "RTW88 USB", 0 };
			static const struct ossd_rtw_match rtw89 =
			    { OSSD_RTW_TRANSPORT_USB, OSSD_RTW_FAMILY_RTW89, "RTW89 USB", 0 };
			if (device->product_id == 0x2006 || device->product_id == 0xc811 ||
			    device->product_id == 0x8731 || device->product_id == 0x8811 ||
			    device->product_id == 0xb820 || device->product_id == 0xb82b ||
			    device->product_id == 0xc80c || device->product_id == 0xc820 ||
			    device->product_id == 0xc821 || device->product_id == 0xc82a ||
			    device->product_id == 0xc82b) return &rtw88;
			if (device->product_id == 0x8832 || device->product_id == 0x885a ||
			    device->product_id == 0x885c) return &rtw89;
			return &legacy;
		}
	}
	return 0;
}

const struct ossd_rtw_match *
ossd_rtw_sdio_match_device(const struct ossd_rtw_sdio_device *device)
{
	uint32_t i;
	if (device == 0) return 0;
	for (i = 0; i < sizeof(rtw_sdio_ids) / sizeof(rtw_sdio_ids[0]); i++) {
		if (device->vendor_id == rtw_sdio_ids[i].vendor_id &&
		    device->device_id == rtw_sdio_ids[i].device_id) {
			return &rtw_sdio_matches[i];
		}
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
