/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_realtek_wifi_firmware.h>

static int
rtw_is_hex_digest(const char *digest)
{
	size_t i;

	if (digest == 0 ||
	    digest[OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH] != '\0') {
		return 0;
	}
	for (i = 0; i < OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH; i++) {
		char c = digest[i];
		if (!((c >= '0' && c <= '9') ||
		    (c >= 'a' && c <= 'f') ||
		    (c >= 'A' && c <= 'F'))) {
			return 0;
		}
	}
	return 1;
}

static int
rtw_is_safe_filename(const char *filename)
{
	size_t i;

	if (filename == 0 || filename[0] == '\0') {
		return 0;
	}
	for (i = 0; filename[i] != '\0'; i++) {
		char c = filename[i];
		if (!((c >= 'a' && c <= 'z') ||
		    (c >= 'A' && c <= 'Z') ||
		    (c >= '0' && c <= '9') ||
		    c == '.' || c == '_' || c == '-')) {
			return 0;
		}
	}
	return 1;
}

static int
rtw_nonempty(const char *value)
{
	return value != 0 && value[0] != '\0';
}

int
ossd_rtw_firmware_validate(
    const struct ossd_rtw_firmware_manifest *manifest,
    const struct ossd_rtw_match *match,
    uint16_t silicon_revision,
    const void *bytes,
    size_t length,
    ossd_rtw_sha256_verify_fn verify_sha256,
    void *verify_context)
{
	if (manifest == 0 || match == 0 || bytes == 0) {
		return -1;
	}
	if (!rtw_is_safe_filename(manifest->filename) ||
	    !rtw_nonempty(manifest->chip_name) ||
	    !rtw_nonempty(manifest->source_uri) ||
	    !rtw_nonempty(manifest->license_id) ||
	    !rtw_is_hex_digest(manifest->sha256_hex)) {
		return -2;
	}
	/* Firmware provenance must be an authenticated, secure transport URI. */
	if (manifest->source_uri[0] != 'h' ||
	    manifest->source_uri[1] != 't' ||
	    manifest->source_uri[2] != 't' ||
	    manifest->source_uri[3] != 'p' ||
	    manifest->source_uri[4] != 's' ||
	    manifest->source_uri[5] != ':' ||
	    manifest->source_uri[6] != '/' ||
	    manifest->source_uri[7] != '/') {
		return -2;
	}
	if (manifest->byte_length == 0 ||
	    manifest->byte_length > OSSD_RTW_FIRMWARE_MAX_SIZE ||
	    length != manifest->byte_length) {
		return -3;
	}
	if (manifest->redistribution != OSSD_RTW_FIRMWARE_REDISTRIBUTABLE &&
	    manifest->redistribution != OSSD_RTW_FIRMWARE_USER_SUPPLIED) {
		return -4;
	}
	if (manifest->family != match->family ||
	    manifest->transport != match->transport ||
	    match->chip_name == 0 ||
	    manifest->chip_name[0] == '\0') {
		return -5;
	}
	{
		size_t i = 0;
		while (manifest->chip_name[i] != '\0' &&
		    match->chip_name[i] != '\0' &&
		    manifest->chip_name[i] == match->chip_name[i]) {
			i++;
		}
		if (manifest->chip_name[i] != match->chip_name[i]) {
			return -5;
		}
	}
	if (silicon_revision < manifest->revision_min ||
	    silicon_revision > manifest->revision_max ||
	    manifest->revision_min > manifest->revision_max) {
		return -6;
	}
	if (verify_sha256 == 0) {
		return -7;
	}
	if (verify_sha256(verify_context, bytes, length,
	    manifest->sha256_hex) != 0) {
		return -8;
	}
	return 0;
}
