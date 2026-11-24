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
	char boundary[100];
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

	memset(boundary, 'a', sizeof(boundary));
	assert(ossd_rtw_sha256_verify(0, boundary, 55,
	    "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318") == 0);
	assert(ossd_rtw_sha256_verify(0, boundary, 56,
	    "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a") == 0);
	assert(ossd_rtw_sha256_verify(0, boundary, sizeof(boundary),
	    "2816597888e4a0d3a36b82b83316ab32680eb8f00f8cd3b904d681246d285a0e") == 0);
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
	manifest.filename = "..";
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
	manifest.source_uri = "https:///firmware.bin";
	assert(ossd_rtw_firmware_validate(&manifest, &match, 2, blob,
	    sizeof(blob) - 1) == -2);
	return 0;
}
