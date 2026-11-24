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

enum ossd_rtw_transport {
	OSSD_RTW_TRANSPORT_PCIE = 1,
	OSSD_RTW_TRANSPORT_USB,
	OSSD_RTW_TRANSPORT_SDIO
};

struct ossd_rtw_usb_device {
	uint16_t vendor_id;
	uint16_t product_id;
	uint8_t interface_class;
	uint8_t interface_subclass;
	uint8_t interface_protocol;
};

struct ossd_rtw_sdio_device {
	uint16_t vendor_id;
	uint16_t device_id;
};

struct ossd_rtw_match {
	enum ossd_rtw_transport transport;
	enum ossd_rtw_family family;
	const char *chip_name;
	const char *firmware_name;
};

struct ossd_rtw_pci_match {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id; /* 0xffff means any */
	uint16_t subsystem_id;        /* 0xffff means any */
	enum ossd_rtw_family family;
	const char *chip_name;
	const char *firmware_name; /* NULL until an approved image is mapped. */
};

/*
 * Identifies a candidate PCIe chipset only. It does not probe hardware,
 * load firmware, or imply that a functional driver is available.
 */
const struct ossd_rtw_pci_match *ossd_rtw_pci_match_device(
    const struct ossd_pci_device *device);
/* Resolve a PCIe candidate into the common firmware-validation tuple.
 * Returns 0 on a fully mapped chipset, -1 for invalid arguments, or -2
 * when the candidate has no approved family/firmware mapping. */
int ossd_rtw_pci_resolve_device(const struct ossd_pci_device *device,
    struct ossd_rtw_match *result);
const char *ossd_rtw_family_name(enum ossd_rtw_family family);
const struct ossd_rtw_match *ossd_rtw_usb_match_device(
    const struct ossd_rtw_usb_device *device);
const struct ossd_rtw_match *ossd_rtw_sdio_match_device(
    const struct ossd_rtw_sdio_device *device);

#ifdef __cplusplus
}
#endif

#endif /* OSSD_REALTEK_WIFI_H */
