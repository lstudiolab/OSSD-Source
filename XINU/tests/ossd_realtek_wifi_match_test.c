/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <string.h>
#include <device/ossd_realtek_wifi.h>

int
main(void)
{
	struct ossd_pci_device device;
	const struct ossd_rtw_pci_match *match;

	memset(&device, 0, sizeof(device));
	device.vendor_id = 0x10ec;
	device.class_code = 0x02;
	device.device_id = 0xc821;
	device.subsystem_vendor_id = 0x103c;
	device.subsystem_id = 0x831a;
	match = ossd_rtw_pci_match_device(&device);
	assert(match != 0);
	assert(match->family == OSSD_RTW_FAMILY_RTW88);
	assert(strcmp(match->chip_name, "RTL8821CE") == 0);
	assert(strcmp(ossd_rtw_family_name(match->family), "rtw88") == 0);

	device.vendor_id = 0x8086;
	assert(ossd_rtw_pci_match_device(&device) == 0);
	device.vendor_id = 0x10ec;
	device.device_id = 0xffff;
	assert(ossd_rtw_pci_match_device(&device) == 0);
	assert(ossd_rtw_pci_match_device(0) == 0);
	return 0;
}
