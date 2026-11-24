/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <device/ossd_realtek_wifi_firmware.h>

static int verify_ok(void *context, const void *bytes, size_t length,
    const char *digest)
{
	(void)context;
	assert(bytes != 0 && length == 4);
	assert(strlen(digest) == OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH);
	return 0;
}

static int verify_fail(void *context, const void *bytes, size_t length,
    const char *digest)
{
	(void)context; (void)bytes; (void)length; (void)digest;
	return -1;
}

int main(void)
{
	const char blob[] = "wifi";
	const struct ossd_rtw_match match = {
		OSSD_RTW_TRANSPORT_PCIE, OSSD_RTW_FAMILY_RTW88,
		"RTL8821CE", "rtl8821ce.bin"
	};
	struct ossd_rtw_firmware_manifest manifest = {
		"rtl8821ce.bin", "RTL8821CE",
		"https://firmware.example.invalid/rtl8821ce.bin",
		"LicenseRef-UserSupplied",
		"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
		4, 1, 3, OSSD_RTW_FAMILY_RTW88, OSSD_RTW_TRANSPORT_PCIE,
		OSSD_RTW_FIRMWARE_USER_SUPPLIED
	};

	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == 0);
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_fail, 0) == -8);
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, 0, 0) == -7);
	assert(ossd_rtw_firmware_validate(&manifest, &match, 4, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -6);

	manifest.filename = "../rtl8821ce.bin";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -2);
	manifest.filename = "rtl8821ce.bin";
	manifest.redistribution = OSSD_RTW_FIRMWARE_PROHIBITED;
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -4);
	manifest.redistribution = OSSD_RTW_FIRMWARE_USER_SUPPLIED;
	manifest.transport = OSSD_RTW_TRANSPORT_USB;
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -5);
	manifest.transport = OSSD_RTW_TRANSPORT_PCIE;
	manifest.sha256_hex = "short";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -2);
	manifest.sha256_hex = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
	manifest.source_uri = "x";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1, verify_ok, 0) == -2);
	return 0;
}
