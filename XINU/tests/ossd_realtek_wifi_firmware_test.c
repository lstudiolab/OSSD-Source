/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <device/ossd_realtek_wifi_firmware.h>

int main(void)
{
	const char blob[] = "wifi";
	const char abc[] = "abc";
	const struct ossd_rtw_match match = {
		OSSD_RTW_TRANSPORT_PCIE, OSSD_RTW_FAMILY_RTW88,
		"RTL8821CE", "rtl8821ce.bin"
	};
	struct ossd_rtw_firmware_manifest manifest = {
		"rtl8821ce.bin", "RTL8821CE",
		"https://firmware.example.invalid/rtl8821ce.bin",
		"LicenseRef-UserSupplied",
		"debdc6fdb6c19c94ff78b653767bfac108d84ed51a54a58a72866dc635b15729",
		4, 1, 3, OSSD_RTW_FAMILY_RTW88, OSSD_RTW_TRANSPORT_PCIE,
		OSSD_RTW_FIRMWARE_USER_SUPPLIED
	};

	assert(ossd_rtw_sha256_verify(0, abc, sizeof(abc) - 1,
	    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);
	assert(ossd_rtw_sha256_verify(0, 0, 0,
	    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0);
	assert(ossd_rtw_sha256_verify(0, abc, sizeof(abc) - 1,
	    "0000000000000000000000000000000000000000000000000000000000000000") == -2);
	assert(ossd_rtw_sha256_verify(0, abc, sizeof(abc) - 1, "bad") == -1);

	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == 0);
	manifest.sha256_hex = "0000000000000000000000000000000000000000000000000000000000000000";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -8);
	manifest.sha256_hex = "debdc6fdb6c19c94ff78b653767bfac108d84ed51a54a58a72866dc635b15729";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 4, blob,
	    sizeof(blob) - 1) == -6);

	manifest.filename = "../rtl8821ce.bin";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -2);
	manifest.filename = "rtl8821ce.bin";
	manifest.redistribution = OSSD_RTW_FIRMWARE_PROHIBITED;
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -4);
	manifest.redistribution = OSSD_RTW_FIRMWARE_USER_SUPPLIED;
	manifest.transport = OSSD_RTW_TRANSPORT_USB;
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -5);
	manifest.transport = OSSD_RTW_TRANSPORT_PCIE;
	manifest.sha256_hex = "short";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -2);
	manifest.sha256_hex = "debdc6fdb6c19c94ff78b653767bfac108d84ed51a54a58a72866dc635b15729";
	manifest.source_uri = "x";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -2);
	return 0;
}
