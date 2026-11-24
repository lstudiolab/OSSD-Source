/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_REALTEK_WIFI_CORE_H
#define OSSD_REALTEK_WIFI_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <device/ossd_realtek_wifi.h>
#include <device/ossd_realtek_wifi_firmware.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ossd_rtw_state {
	OSSD_RTW_STATE_DETACHED = 0,
	OSSD_RTW_STATE_MATCHED,
	OSSD_RTW_STATE_CLAIMED,
	OSSD_RTW_STATE_RESET,
	OSSD_RTW_STATE_FIRMWARE_LOADED,
	OSSD_RTW_STATE_INITIALIZED,
	OSSD_RTW_STATE_RUNNING,
	OSSD_RTW_STATE_FAILED
};

/*
 * Bus/chipset-specific implementations supply these operations. All methods
 * return 0 on success and a negative errno-style value on failure. Firmware
 * bytes are authenticated by the in-tree SHA-256 verifier before claim/reset/load; the loader
 * must still enforce the chipset's bounded transfer protocol and timeouts.
 */
struct ossd_rtw_backend_ops {
	int (*claim)(void *context);
	int (*reset)(void *context);
	int (*load_firmware)(void *context,
	    const struct ossd_rtw_firmware_manifest *manifest,
	    const void *bytes, size_t length);
	int (*initialize)(void *context, const struct ossd_rtw_match *match);
	int (*start)(void *context);
	void (*stop)(void *context);
	void (*release)(void *context);
	int (*transmit)(void *context, const void *frame, size_t length);
	int (*scan)(void *context);
};

struct ossd_rtw_device {
	const struct ossd_rtw_match *match;
	const struct ossd_rtw_firmware_manifest *firmware;
	const void *firmware_bytes;
	size_t firmware_length;
	uint16_t silicon_revision;
	const struct ossd_rtw_backend_ops *ops;
	void *backend_context;
	enum ossd_rtw_state state;
	uint8_t claimed;
	uint8_t started;
	int last_error;
};

int ossd_rtw_device_start(struct ossd_rtw_device *device);
void ossd_rtw_device_stop(struct ossd_rtw_device *device);
int ossd_rtw_device_transmit(struct ossd_rtw_device *device,
    const void *frame, size_t length);
int ossd_rtw_device_scan(struct ossd_rtw_device *device);

#ifdef __cplusplus
}
#endif
#endif
