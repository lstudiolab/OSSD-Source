/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ossd_pci.h"

static uint32_t
mock_read(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset)
{
	(void)context;
	if (bus == 0 && device == 1 && function == 0) {
		switch (offset) {
		case 0x00: return 0x12348086U;
		case 0x08: return 0x02000001U;
		case 0x0c: return 0;
		case 0x10: return 0xfebf0000U;
		case 0x2c: return 0x5678103cU;
		case 0x3c: return 0x0000010bU;
		default: return 0;
		}
	}
	if (bus == 0 && device == 2 && function == 0) {
		switch (offset) {
		case 0x00: return 0x56781234U;
		case 0x08: return 0x06040002U;
		case 0x0c: return 0x00810000U;
		case 0x2c: return 0x00000000U;
		default: return 0;
		}
	}
	if (bus == 0 && device == 2 && function == 1) {
		switch (offset) {
		case 0x00: return 0xc82110ecU;
		case 0x08: return 0x02800001U;
		case 0x0c: return 0;
		case 0x2c: return 0x831a103cU;
		default: return 0;
		}
	}
	return 0xffffffffU;
}

struct test_state {
	uint32_t count;
	int found_network;
	int found_bridge;
};

static void
record_device(void *context, const struct ossd_pci_device *device)
{
	struct test_state *state = (struct test_state *)context;
	state->count++;
	if (device->vendor_id == 0x8086 && device->device_id == 0x1234) {
		assert(device->bus == 0 && device->device == 1);
		assert(device->class_code == 0x02 && device->subclass == 0x00);
		assert(device->revision == 1);
		assert(device->subsystem_vendor_id == 0x103c);
		assert(device->subsystem_id == 0x5678);
		assert(device->bars[0] == 0xfebf0000U);
		assert(device->interrupt_line == 11 && device->interrupt_pin == 1);
		state->found_network = 1;
	}
	if (device->vendor_id == 0x10ec && device->device_id == 0xc821) {
		assert(device->bus == 0 && device->device == 2 &&
		    device->function == 1);
		assert(device->class_code == 0x02 && device->subclass == 0x80);
		assert(device->subsystem_vendor_id == 0x103c);
		assert(device->subsystem_id == 0x831a);
		state->found_bridge = 1;
	}
}

int
main(void)
{
	struct test_state state;
	uint32_t count;

	memset(&state, 0, sizeof(state));
	count = ossd_pci_scan(mock_read, NULL, record_device, &state);
	assert(count == 3);
	assert(state.count == 3);
	assert(state.found_network);
	assert(state.found_bridge);
	puts("XINU PCI enumeration tests passed");
	return 0;
}
