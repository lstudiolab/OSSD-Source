/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_PCI_H
#define OSSD_PCI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSSD_PCI_MAX_BARS 6

struct ossd_pci_device {
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t revision;
	uint8_t programming_interface;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t header_type;
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint32_t bars[OSSD_PCI_MAX_BARS];
};

typedef uint32_t (*ossd_pci_config_read_fn)(
    void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset);
typedef void (*ossd_pci_device_fn)(
    void *context, const struct ossd_pci_device *device);

/*
 * Enumerates conventional PCI configuration space through a platform-supplied
 * configuration reader. The scanner never enables devices or writes config
 * registers. Returns the number of discovered functions.
 */
uint32_t ossd_pci_scan(ossd_pci_config_read_fn read_config,
    void *read_context, ossd_pci_device_fn found, void *found_context);

#if defined(__x86_64__)
/* x86 legacy PCI Configuration Mechanism #1 backend. */
uint32_t ossd_pci_platform_enumerate(ossd_pci_device_fn found,
    void *found_context);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OSSD_PCI_H */
