/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCI Configuration Mechanism #1 backend for x86-64.
 * Only configuration reads are performed; device state is not modified.
 */
#include <device/ossd_pci.h>
#include <i386/pio.h>

#define PCI_CONFIG_ADDRESS 0x0cf8
#define PCI_CONFIG_DATA    0x0cfc

static volatile uint32_t pci_config_lock;

static void
pci_lock(void)
{
	while (__sync_lock_test_and_set(&pci_config_lock, 1U) != 0U) {
		while (pci_config_lock != 0U) {
			__asm__ volatile("pause");
		}
	}
}

static void
pci_unlock(void)
{
	__sync_lock_release(&pci_config_lock);
}

static uint32_t
pci_x86_read(void *context __unused, uint8_t bus, uint8_t device,
    uint8_t function, uint16_t offset)
{
	uint32_t address, value;

	if (device >= 32 || function >= 8 || offset >= 256 ||
	    (offset & 3) != 0) {
		return 0xffffffffU;
	}

	address = 0x80000000U | ((uint32_t)bus << 16) |
	    ((uint32_t)device << 11) | ((uint32_t)function << 8) |
	    ((uint32_t)offset & 0xfcU);

	pci_lock();
	outl((i386_ioport_t)PCI_CONFIG_ADDRESS, address);
	value = inl((i386_ioport_t)PCI_CONFIG_DATA);
	pci_unlock();

	return value;
}

uint32_t
ossd_pci_platform_enumerate(ossd_pci_device_fn found, void *found_context)
{
	return ossd_pci_scan(pci_x86_read, 0, found, found_context);
}
