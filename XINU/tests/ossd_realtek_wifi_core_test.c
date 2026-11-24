/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <string.h>
#include <device/ossd_realtek_wifi_core.h>

struct mock { int calls; int fail_at; };
static int step(struct mock *m) { m->calls++; return m->calls == m->fail_at ? -5 : 0; }
static int claim(void *p) { return step(p); }
static int reset(void *p) { return step(p); }
static int firmware(void *p, const struct ossd_rtw_firmware_manifest *manifest,
    const void *bytes, size_t length)
{
	assert(manifest != 0 && bytes != 0 && length == 4);
	return step(p);
}
static int initialize(void *p, const struct ossd_rtw_match *m) { assert(m != 0); return step(p); }
static int start(void *p) { return step(p); }
static void stop(void *p) { ((struct mock *)p)->calls += 100; }
static void release(void *p) { ((struct mock *)p)->calls += 1000; }
static int transmit(void *p, const void *f, size_t n) { (void)p; assert(f && n); return 0; }
static int scan(void *p) { (void)p; return 0; }

int main(void)
{
	struct ossd_rtw_usb_device usb = { 0x0bda, 0xc811, 0xff, 0xff, 0xff };
	struct ossd_rtw_usb_device wrong = { 0x0bda, 0xc811, 0x02, 0x02, 0x02 };
	struct ossd_rtw_sdio_device sdio = { 0x024c, 0xb822 };
	const struct ossd_rtw_match *um = ossd_rtw_usb_match_device(&usb);
	const struct ossd_rtw_match *sm = ossd_rtw_sdio_match_device(&sdio);
	struct mock m = { 0, 0 };
	const struct ossd_rtw_backend_ops ops = {
		.claim = claim, .reset = reset,
		.load_firmware = firmware, .initialize = initialize, .start = start,
		.stop = stop, .release = release, .transmit = transmit, .scan = scan
	};
	const struct ossd_rtw_match test_match = {
		OSSD_RTW_TRANSPORT_USB, OSSD_RTW_FAMILY_RTW88,
		"test-chip", "test-firmware.bin"
	};
	const struct ossd_rtw_firmware_manifest test_firmware = {
		"test-firmware.bin", "test-chip",
		"https://firmware.example.invalid/test-firmware.bin",
		"LicenseRef-UserSupplied",
		"fa2c8cc4f28176bbeed4b736df569a34c79cd3723e9ec42f9674b4d46ac6b8b8",
		4, 1, 3, OSSD_RTW_FAMILY_RTW88, OSSD_RTW_TRANSPORT_USB,
		OSSD_RTW_FIRMWARE_USER_SUPPLIED
	};
	const char firmware_bytes[] = "blob";
	struct ossd_rtw_device dev = {
		.match = &test_match, .firmware = &test_firmware,
		.firmware_bytes = firmware_bytes, .firmware_length = sizeof(firmware_bytes) - 1,
		.silicon_revision = 2, .ops = &ops, .backend_context = &m,
		.state = OSSD_RTW_STATE_MATCHED
	};
	char frame[64] = { 0 };

	assert(um && um->family == OSSD_RTW_FAMILY_RTW88);
	assert(sm && sm->family == OSSD_RTW_FAMILY_RTW88);
	assert(ossd_rtw_usb_match_device(&wrong) == 0);
	assert(ossd_rtw_usb_match_device(0) == 0);
	assert(ossd_rtw_sdio_match_device(0) == 0);
	assert(ossd_rtw_device_transmit(&dev, frame, sizeof(frame)) != 0);

	assert(ossd_rtw_device_start(&dev) == 0);
	assert(dev.state == OSSD_RTW_STATE_RUNNING);
	assert(ossd_rtw_device_transmit(&dev, frame, sizeof(frame)) == 0);
	assert(ossd_rtw_device_scan(&dev) == 0);
	ossd_rtw_device_stop(&dev);
	assert(dev.state == OSSD_RTW_STATE_DETACHED);

	m.calls = 0; m.fail_at = 3;
	dev.state = OSSD_RTW_STATE_MATCHED;
	assert(ossd_rtw_device_start(&dev) == -5);
	assert(dev.state == OSSD_RTW_STATE_FAILED && dev.last_error == -5);
	assert(m.calls == 3 + 1000);

	/* A failed start must invoke stop before releasing the transport. */
	m.calls = 0; m.fail_at = 5;
	dev.state = OSSD_RTW_STATE_MATCHED;
	assert(ossd_rtw_device_start(&dev) == -5);
	assert(dev.state == OSSD_RTW_STATE_FAILED);
	assert(m.calls == 5 + 100 + 1000);

	/* Missing firmware metadata must fail before claiming hardware. */
	{
		const struct ossd_rtw_firmware_manifest *saved = dev.firmware;
		dev.firmware = 0;
		m.calls = 0; m.fail_at = 0;
		dev.state = OSSD_RTW_STATE_MATCHED;
		assert(ossd_rtw_device_start(&dev) == -3);
		assert(m.calls == 0);
		dev.firmware = saved;
	}
	return 0;
}
