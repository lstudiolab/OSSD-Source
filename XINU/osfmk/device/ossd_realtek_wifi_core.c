/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_realtek_wifi_core.h>

static int
rtw_fail(struct ossd_rtw_device *device, int error)
{
	if (device->started && device->ops->stop != 0)
		device->ops->stop(device->backend_context);
	if (device->claimed && device->ops->release != 0)
		device->ops->release(device->backend_context);
	device->started = 0;
	device->claimed = 0;
	device->state = OSSD_RTW_STATE_FAILED;
	device->last_error = error;
	return error;
}

int
ossd_rtw_device_start(struct ossd_rtw_device *device)
{
	const struct ossd_rtw_backend_ops *ops;
	int error;

	if (device == 0 || device->match == 0 || device->ops == 0) return -1;
	if (device->state == OSSD_RTW_STATE_RUNNING) return 0;
	if (device->state != OSSD_RTW_STATE_MATCHED &&
	    device->state != OSSD_RTW_STATE_FAILED) return -2;
	ops = device->ops;
	if (ops->claim == 0 || ops->reset == 0 || ops->load_firmware == 0 ||
	    ops->initialize == 0 || ops->start == 0 || ops->release == 0 ||
	    ops->stop == 0 || device->match->firmware_name == 0) return -3;

	device->state = OSSD_RTW_STATE_MATCHED;
	device->last_error = 0;
	error = ops->claim(device->backend_context);
	if (error != 0) return rtw_fail(device, error);
	device->claimed = 1;
	device->state = OSSD_RTW_STATE_CLAIMED;

	error = ops->reset(device->backend_context);
	if (error != 0) return rtw_fail(device, error);
	device->state = OSSD_RTW_STATE_RESET;

	error = ops->load_firmware(device->backend_context,
	    device->match->firmware_name);
	if (error != 0) return rtw_fail(device, error);
	device->state = OSSD_RTW_STATE_FIRMWARE_LOADED;

	error = ops->initialize(device->backend_context, device->match);
	if (error != 0) return rtw_fail(device, error);
	device->state = OSSD_RTW_STATE_INITIALIZED;

	error = ops->start(device->backend_context);
	if (error != 0) return rtw_fail(device, error);
	device->started = 1;
	device->state = OSSD_RTW_STATE_RUNNING;
	return 0;
}

void
ossd_rtw_device_stop(struct ossd_rtw_device *device)
{
	if (device == 0 || device->ops == 0) return;
	if (device->started && device->ops->stop != 0) {
		device->ops->stop(device->backend_context);
	}
	if (device->claimed && device->ops->release != 0) {
		device->ops->release(device->backend_context);
	}
	device->started = 0;
	device->claimed = 0;
	device->state = OSSD_RTW_STATE_DETACHED;
	device->last_error = 0;
}

int
ossd_rtw_device_transmit(struct ossd_rtw_device *device,
    const void *frame, size_t length)
{
	if (device == 0 || frame == 0 || length == 0 ||
	    device->state != OSSD_RTW_STATE_RUNNING ||
	    device->ops == 0 || device->ops->transmit == 0) return -1;
	return device->ops->transmit(device->backend_context, frame, length);
}

int
ossd_rtw_device_scan(struct ossd_rtw_device *device)
{
	if (device == 0 || device->state != OSSD_RTW_STATE_RUNNING ||
	    device->ops == 0 || device->ops->scan == 0) return -1;
	return device->ops->scan(device->backend_context);
}
