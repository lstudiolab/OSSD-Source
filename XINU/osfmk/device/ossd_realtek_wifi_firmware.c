/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_realtek_wifi_firmware.h>

static int
rtw_is_hex_digest(const char *digest)
{
	size_t i;

	if (digest == 0) return 0;
	for (i = 0; i < OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH; i++) {
		char c = digest[i];
		if (c == '\0' || !((c >= '0' && c <= '9') ||
		    (c >= 'a' && c <= 'f') ||
		    (c >= 'A' && c <= 'F'))) return 0;
	}
	return digest[OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH] == '\0';
}

static int
rtw_is_safe_filename(const char *filename)
{
	size_t i;

	if (filename == 0 || filename[0] == '\0' ||
	    (filename[0] == '.' && filename[1] == '\0') ||
	    (filename[0] == '.' && filename[1] == '.' && filename[2] == '\0')) return 0;
	for (i = 0; filename[i] != '\0'; i++) {
		char c = filename[i];
		if (!((c >= 'a' && c <= 'z') ||
		    (c >= 'A' && c <= 'Z') ||
		    (c >= '0' && c <= '9') ||
		    c == '.' || c == '_' || c == '-')) return 0;
	}
	return 1;
}

static int
rtw_has_https_prefix(const char *uri)
{
	static const char prefix[] = "https://";
	size_t i;

	if (uri == 0) return 0;
	for (i = 0; i < sizeof(prefix) - 1; i++) {
		if (uri[i] == '\0' || uri[i] != prefix[i]) return 0;
	}
	if (uri[i] == '\0' || uri[i] == '/' || uri[i] == '?' ||
	    uri[i] == '#' || uri[i] == ' ') return 0;
	for (; uri[i] != '\0' && uri[i] != '/' && uri[i] != '?' &&
	    uri[i] != '#'; i++) {
		if (uri[i] == ' ' || uri[i] == '\\') return 0;
	}
	return 1;
}

static int
rtw_nonempty(const char *value)
{
	return value != 0 && value[0] != '\0';
}

static int
rtw_same_string(const char *left, const char *right)
{
	size_t i;

	if (left == 0 || right == 0) return 0;
	for (i = 0; left[i] != '\0' && right[i] != '\0'; i++) {
		if (left[i] != right[i]) return 0;
	}
	return left[i] == right[i];
}

int
ossd_rtw_firmware_validate(
    const struct ossd_rtw_firmware_manifest *manifest,
    const struct ossd_rtw_match *match,
    uint16_t silicon_revision,
    const void *bytes,
    size_t length)
{
	if (manifest == 0 || match == 0 || bytes == 0) return -1;
	if (!rtw_is_safe_filename(manifest->filename) ||
	    !rtw_nonempty(manifest->chip_name) ||
	    !rtw_has_https_prefix(manifest->source_uri) ||
	    !rtw_nonempty(manifest->license_id) ||
	    !rtw_is_hex_digest(manifest->sha256_hex)) return -2;
	if (manifest->byte_length == 0 ||
	    manifest->byte_length > OSSD_RTW_FIRMWARE_MAX_SIZE ||
	    length != manifest->byte_length) return -3;
	if (manifest->redistribution != OSSD_RTW_FIRMWARE_REDISTRIBUTABLE &&
	    manifest->redistribution != OSSD_RTW_FIRMWARE_USER_SUPPLIED) return -4;
	if (manifest->family != match->family ||
	    manifest->transport != match->transport ||
	    !rtw_same_string(manifest->chip_name, match->chip_name) ||
	    !rtw_same_string(manifest->filename, match->firmware_name)) return -5;
	if (manifest->revision_min > manifest->revision_max ||
	    silicon_revision < manifest->revision_min ||
	    silicon_revision > manifest->revision_max) return -6;
	if (ossd_rtw_sha256_verify(0, bytes, length,
	    manifest->sha256_hex) != 0) return -8;
	return 0;
}
