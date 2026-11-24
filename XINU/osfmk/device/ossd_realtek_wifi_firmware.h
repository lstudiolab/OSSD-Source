/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_REALTEK_WIFI_FIRMWARE_H
#define OSSD_REALTEK_WIFI_FIRMWARE_H

#include <stddef.h>
#include <stdint.h>
#include <device/ossd_realtek_wifi.h>
#include <device/ossd_realtek_wifi_sha256.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSSD_RTW_FIRMWARE_SHA256_HEX_LENGTH 64U
#define OSSD_RTW_FIRMWARE_MAX_SIZE (32U * 1024U * 1024U)

enum ossd_rtw_firmware_redistribution {
	/* The license explicitly permits redistribution of this exact blob. */
	OSSD_RTW_FIRMWARE_REDISTRIBUTABLE = 1,
	/* The user must obtain/install the blob; OSSD does not redistribute it. */
	OSSD_RTW_FIRMWARE_USER_SUPPLIED = 2,
	/* The blob must never be installed or loaded. */
	OSSD_RTW_FIRMWARE_PROHIBITED = 3
};

/*
 * Immutable firmware provenance record. revision_min/revision_max refer to
 * the chipset's silicon revision, not the PCI revision field unless the
 * chipset backend explicitly documents that mapping.
 */
struct ossd_rtw_firmware_manifest {
	const char *filename;
	const char *chip_name;
	const char *source_uri;
	const char *license_id;
	const char *sha256_hex;
	uint32_t byte_length;
	uint16_t revision_min;
	uint16_t revision_max;
	enum ossd_rtw_family family;
	enum ossd_rtw_transport transport;
	enum ossd_rtw_firmware_redistribution redistribution;
};

/*
 * Validates metadata, exact chipset/transport/revision binding, byte length,
 * redistribution policy, and the blob digest. No firmware is loaded here.
 * Returns 0 on success; negative errno-style values on rejection.
 */
int ossd_rtw_firmware_validate(
    const struct ossd_rtw_firmware_manifest *manifest,
    const struct ossd_rtw_match *match,
    uint16_t silicon_revision,
    const void *bytes,
    size_t length);

#ifdef __cplusplus
}
#endif
#endif /* OSSD_REALTEK_WIFI_FIRMWARE_H */
