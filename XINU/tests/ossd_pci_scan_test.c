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


static uint32_t command_register;
static uint32_t command_write_value;
static uint32_t
command_read(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset)
{
	(void)context; (void)bus; (void)device; (void)function;
	assert(offset == 0x04);
	return command_register;
}

static void
command_write(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset, uint32_t value)
{
	(void)context; (void)bus; (void)device; (void)function;
	assert(offset == 0x04);
	command_write_value = value;
	command_register = value;
}


static uint32_t bar_command = 0x0007U;
static uint32_t bar_low = 0xfebf0000U;
static uint32_t bar_high;
static uint32_t bar_mask_low = 0xffff0000U;
static uint32_t bar_mask_high;
static uint32_t bar_writes;
static int bar_sizing;

static uint32_t
bar_read(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset)
{
	(void)context; (void)bus; (void)device; (void)function;
	switch (offset) {
	case 0x00: return 0x12348086U;
	case 0x04: return bar_command;
	case 0x10: return bar_sizing ? bar_mask_low : bar_low;
	case 0x14: return bar_sizing ? bar_mask_high : bar_high;
	default: return 0xffffffffU;
	}
}

static void
bar_write(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset, uint32_t value)
{
	(void)context; (void)bus; (void)device; (void)function;
	bar_writes++;
	if (offset == 0x04) {
		bar_command = value;
	} else if (offset == 0x10) {
		bar_sizing = value == 0xffffffffU;
		if (!bar_sizing) bar_low = value;
	} else if (offset == 0x14) {
		if (value != 0xffffffffU) bar_high = value;
	}
}


static uint32_t mmio_storage[16];
static int mmio_map_calls;
static int mmio_unmap_calls;
static int
mock_mmio_map(void *context, uint64_t physical, uint64_t length,
    volatile void **virtual_address)
{
	(void)context;
	assert(physical == 0xfebf0000ULL);
	assert(length == 0x10000ULL);
	mmio_map_calls++;
	*virtual_address = mmio_storage;
	return 0;
}
static void
mock_mmio_unmap(void *context, volatile void *virtual_address,
    uint64_t length)
{
	(void)context;
	assert(virtual_address == mmio_storage);
	assert(length == 0x10000ULL);
	mmio_unmap_calls++;
}

static uint32_t msi_config[1024];
static int msi_allocations;
static int msi_releases;
static uint32_t
msi_read(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset)
{
	(void)context; (void)bus; (void)device; (void)function;
	if (offset >= sizeof(msi_config)) return 0xffffffffU;
	return msi_config[offset / 4U];
}
static void
msi_write(void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset, uint32_t value)
{
	(void)context; (void)bus; (void)device; (void)function;
	assert(offset < sizeof(msi_config));
	msi_config[offset / 4U] = value;
}
static int
msi_allocate(void *context, struct ossd_pci_msi_message *message)
{
	(void)context;
	msi_allocations++;
	message->address = 0xfee00000ULL;
	message->data = 0x45U;
	message->provider_cookie = &msi_config;
	return 0;
}
static void
msi_release(void *context, const struct ossd_pci_msi_message *message)
{
	(void)context;
	assert(message->address == 0xfee00000ULL);
	assert(message->data == 0x45U);
	msi_releases++;
}

struct test_state {
	uint32_t count;
	int found_network;
	int found_bridge;
	int found_ecam;
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
	if (device->vendor_id == 0xabcd && device->device_id == 0x1000) {
		assert(device->segment == 3 && device->bus == 5);
		assert(device->device == 4 && device->function == 0);
		state->found_ecam = 1;
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
	count = ossd_pci_scan(mock_read, NULL, 0, 0, 255, record_device, &state);
	assert(count == 3);
	assert(state.count == 3);
	assert(state.found_network);
	assert(state.found_bridge);
	{
		static uint32_t ecam_memory[0x100000 / sizeof(uint32_t)];
		struct ossd_pci_ecam_region region;
		uint32_t *config = &ecam_memory[(4U << 15) / sizeof(uint32_t)];

		memset(&state, 0, sizeof(state));
		memset(ecam_memory, 0xff, sizeof(ecam_memory));
		region.base_address = 0xe0000000ULL;
		region.segment = 3;
		region.start_bus = 5;
		region.end_bus = 5;
		config[0x00 / 4] = 0x1000abcdU;
		config[0x08 / 4] = 0x01080001U;
		config[0x0c / 4] = 0;
		config[0x2c / 4] = 0x00000000U;
		config[0x3c / 4] = 0;
		assert(ossd_pci_scan_ecam(ecam_memory, &region,
		    record_device, &state) == 1);
		assert(state.count == 1 && state.found_ecam);
		{
			struct ossd_pci_ecam_access access;
			access.mapped_base = (volatile uint8_t *)ecam_memory;
			access.region = &region;
			assert(ossd_pci_ecam_config_read(&access, 5, 4, 0, 0x00) ==
			    0x1000abcdU);
			ossd_pci_ecam_config_write(&access, 5, 4, 0, 0x04,
			    0x00000006U);
			assert(ossd_pci_ecam_config_read(&access, 5, 4, 0, 0x04) ==
			    0x00000006U);
			assert(ossd_pci_ecam_config_read(&access, 6, 4, 0, 0x00) ==
			    0xffffffffU);
		}
	}
	/* Command writes preserve low bits and never echo W1C status bits. */
	command_register = 0xa5a50040U;
	command_write_value = 0xffffffffU;
	assert(ossd_pci_update_command(command_read, command_write, 0, 0, 1, 0,
	    OSSD_PCI_COMMAND_MEMORY_SPACE, 0) == 0);
	assert(command_write_value == 0x0042U);
	assert(ossd_pci_update_command(command_read, command_write, 0, 0, 1, 0,
	    OSSD_PCI_COMMAND_BUS_MASTER, OSSD_PCI_COMMAND_MEMORY_SPACE) == 0);
	assert(command_write_value == 0x0044U);
	assert(ossd_pci_update_command(command_read, command_write, 0, 0, 1, 0,
	    0x8000U, 0) == -1);
	assert(ossd_pci_update_command(command_read, command_write, 0, 0, 1, 0,
	    OSSD_PCI_COMMAND_IO_SPACE, OSSD_PCI_COMMAND_IO_SPACE) == -1);
	assert(ossd_pci_update_command(command_read, 0, 0, 0, 1, 0, 0, 0) == -1);

	/* BAR sizing restores command and BAR registers after mask reads. */
	{
		struct ossd_pci_device d;
		struct ossd_pci_bar_info info;
		memset(&d, 0, sizeof(d));
		d.header_type = 0;
		d.vendor_id = 0x8086;
		d.device_id = 0x1234;
		d.device = 1;
		d.function = 0;
		bar_command = 0x0007U;
		bar_low = 0xfebf0000U;
		bar_high = 0;
		bar_mask_low = 0xffff0000U; /* 64 KiB memory BAR */
		bar_mask_high = 0;
		bar_sizing = 0;
		bar_writes = 0;
		assert(ossd_pci_size_bar(bar_read, bar_write, 0, &d, 0, &info) == 0);
		assert(info.type == OSSD_PCI_BAR_MEMORY && !info.is_64_bit);
		assert(info.address == 0xfebf0000ULL && info.size == 0x10000ULL);
		assert(bar_low == 0xfebf0000U && bar_command == 0x0007U);
		assert(bar_writes == 4);

		/* 64-bit prefetchable BAR consumes the following slot. */
		bar_command = 0x0007U;
		bar_low = 0x0000000cU;
		bar_high = 0x00000012U;
		bar_mask_low = 0xff00000cU;
		bar_mask_high = 0xffffffffU;
		bar_sizing = 0;
		bar_writes = 0;
		assert(ossd_pci_size_bar(bar_read, bar_write, 0, &d, 0, &info) == 0);
		assert(info.type == OSSD_PCI_BAR_MEMORY && info.is_64_bit);
		assert(info.prefetchable == 1);
		assert(info.address == 0x1200000000ULL && info.size == 0x1000000ULL);
		assert(bar_low == 0x0000000cU && bar_high == 0x00000012U);
		assert(bar_command == 0x0007U && bar_writes == 6);
		assert(ossd_pci_size_bar(bar_read, bar_write, 0, &d, 1, &info) == -3);
		assert(ossd_pci_size_bar(bar_read, bar_write, 0, &d, 6, &info) == -1);
	}


	/* MSI setup programs one routed vector and restores device state on detach. */
	{
		const struct ossd_pci_msi_provider_ops provider = {
			.allocate = msi_allocate, .release = msi_release
		};
		struct ossd_pci_device d;
		struct ossd_pci_msi_state msi_state;
		memset(&d, 0, sizeof(d));
		memset(&msi_state, 0, sizeof(msi_state));
		memset(msi_config, 0, sizeof(msi_config));
		d.vendor_id = 0x8086;
		d.device_id = 0x1234;
		d.device = 1;
		d.function = 0;
		d.header_type = 0;
		msi_config[0x00 / 4] = 0x12348086U;
		msi_config[0x04 / 4] = 0x00100006U; /* capability list + command */
		msi_config[0x34 / 4] = 0x50U;
		msi_config[0x50 / 4] = 0x01800005U; /* 64-bit MSI + vector mask */
		msi_config[0x60 / 4] = 0xa5U;
		msi_allocations = msi_releases = 0;
		assert(ossd_pci_msi_enable(msi_read, msi_write, 0, &d,
		    &provider, 0, &msi_state) == 0);
		assert(msi_allocations == 1 && msi_state.active);
		assert(msi_config[0x54 / 4] == 0xfee00000U);
		assert(msi_config[0x58 / 4] == 0U);
		assert(msi_config[0x5c / 4] == 0x45U);
		assert((msi_config[0x50 / 4] >> 16 & 1U) == 1U);
		assert(msi_config[0x60 / 4] == 0xa4U);
		assert((msi_config[0x04 / 4] & 0xffffU) == 0x0406U);
		ossd_pci_msi_disable(msi_read, msi_write, 0, &d,
		    &provider, 0, &msi_state);
		assert(msi_releases == 1 && !msi_state.active);
		assert((msi_config[0x50 / 4] >> 16 & 1U) == 0U);
		assert(msi_config[0x60 / 4] == 0xa5U);
		assert((msi_config[0x04 / 4] & 0xffffU) == 0x0006U);
	}

	/* BAR mapper and bounded MMIO accessors reject invalid register offsets. */
	{
		const struct ossd_pci_mmio_ops ops = {
			.map = mock_mmio_map, .unmap = mock_mmio_unmap
		};
		struct ossd_pci_device d;
		struct ossd_pci_mmio_region region;
		uint32_t value = 0;
		uint16_t value16 = 0;
		memset(&d, 0, sizeof(d));
		memset(mmio_storage, 0, sizeof(mmio_storage));
		d.header_type = 0;
		d.vendor_id = 0x8086;
		d.device_id = 0x1234;
		d.device = 1;
		d.function = 0;
		bar_command = 0x0007U;
		bar_low = 0xfebf0000U;
		bar_mask_low = 0xffff0000U;
		bar_sizing = 0;
		mmio_map_calls = mmio_unmap_calls = 0;
		assert(ossd_pci_map_bar(bar_read, bar_write, 0, &d, 0, 0x1000,
		    &ops, 0, &region) == 0);
		assert(mmio_map_calls == 1 && region.length == 0x10000ULL);
		assert(ossd_pci_mmio_write32(&region, 4, 0x12345678U) == 0);
		assert(ossd_pci_mmio_read32(&region, 4, &value) == 0);
		assert(value == 0x12345678U);
		assert(ossd_pci_mmio_read32(&region, 62, &value) == -1);
		assert(ossd_pci_mmio_write16(&region, 3, 0x1234U) == -1);
		/* A misaligned mapping base is rejected even at offset zero. */
		region.base = (volatile uint8_t *)mmio_storage + 1;
		assert(ossd_pci_mmio_read16(&region, 0, &value16) == -1);
		region.base = (volatile uint8_t *)mmio_storage;
		ossd_pci_unmap_bar(&ops, 0, &region);
		assert(mmio_unmap_calls == 1 && region.base == 0);
	}

	puts("XINU PCI and ECAM enumeration tests passed");
	return 0;
}
