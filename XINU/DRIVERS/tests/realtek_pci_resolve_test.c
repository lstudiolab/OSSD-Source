/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host regression tests for Realtek PCIe candidate-to-firmware resolution.
 * These tests validate identification only; they do not emulate the radio.
 */
#include <assert.h>
#include <string.h>
#include <device/ossd_realtek_wifi.h>

static struct ossd_pci_device
realtek_device(uint16_t device_id)
{
	struct ossd_pci_device device = {0};
	device.vendor_id = 0x10ec;
	device.device_id = device_id;
	device.class_code = 0x02;
	return device;
}

int
main(void)
{
	struct ossd_pci_device device;
	struct ossd_rtw_match resolved;

	device = realtek_device(0xc821);
	assert(ossd_rtw_pci_match_device(&device) != 0);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == 0);
	assert(resolved.transport == OSSD_RTW_TRANSPORT_PCIE);
	assert(resolved.family == OSSD_RTW_FAMILY_RTW88);
	assert(strcmp(resolved.chip_name, "RTL8821CE") == 0);
	assert(strcmp(resolved.firmware_name, "rtw8821c_fw.bin") == 0);

	device = realtek_device(0xb821);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == 0);
	assert(strcmp(resolved.firmware_name, "rtw8821c_fw.bin") == 0);

	device = realtek_device(0xb822);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == 0);
	assert(strcmp(resolved.chip_name, "RTL8822BE") == 0);
	assert(strcmp(resolved.firmware_name, "rtw8822b_fw.bin") == 0);

	device = realtek_device(0x8852);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == 0);
	assert(resolved.family == OSSD_RTW_FAMILY_RTW89);
	assert(strcmp(resolved.firmware_name, "rtw8852a_fw.bin") == 0);

	/* Candidate IDs without an approved firmware mapping must not proceed. */
	device = realtek_device(0x8176);
	assert(ossd_rtw_pci_match_device(&device) != 0);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == -2);

	device = realtek_device(0xc821);
	device.class_code = 0x03;
	assert(ossd_rtw_pci_match_device(&device) == 0);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == -2);

	device = realtek_device(0xffff);
	assert(ossd_rtw_pci_match_device(&device) == 0);
	assert(ossd_rtw_pci_resolve_device(&device, &resolved) == -2);
	assert(ossd_rtw_pci_resolve_device(0, &resolved) == -1);
	assert(ossd_rtw_pci_resolve_device(&device, 0) == -1);

	return 0;
}
