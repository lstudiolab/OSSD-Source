/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_PCI_DRIVER_H
#define OSSD_PCI_DRIVER_H

#include <stddef.h>
#include <stdint.h>
#include <device/ossd_pci.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSSD_PCI_DRIVER_MAX 32U
#define OSSD_PCI_BOUND_DEVICE_MAX 64U
#define OSSD_PCI_ANY_ID 0xffffU

struct ossd_pci_id {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint8_t class_code;
	uint8_t class_mask;
	uint8_t subclass;
	uint8_t subclass_mask;
	uint8_t programming_interface;
	uint8_t programming_interface_mask;
};

struct ossd_pci_driver {
	const char *name;
	const struct ossd_pci_id *ids;
	size_t id_count;
	/*
	 * probe returns 0 only after complete ownership/setup. On failure the
	 * callback must unwind every resource it acquired before returning.
	 */
	int (*probe)(const struct ossd_pci_device *device,
	    const struct ossd_pci_id *matched_id, void **driver_data);
	/* remove is called exactly once for every successful probe. */
	void (*remove)(const struct ossd_pci_device *device, void *driver_data);
};

struct ossd_pci_bound_device {
	struct ossd_pci_device device;
	const struct ossd_pci_driver *driver;
	void *driver_data;
	uint8_t occupied;
};

struct ossd_pci_registry {
	const struct ossd_pci_driver *drivers[OSSD_PCI_DRIVER_MAX];
	size_t driver_count;
	struct ossd_pci_bound_device bound[OSSD_PCI_BOUND_DEVICE_MAX];
	size_t bound_count;
};

void ossd_pci_registry_init(struct ossd_pci_registry *registry);
int ossd_pci_register_driver(struct ossd_pci_registry *registry,
    const struct ossd_pci_driver *driver);
int ossd_pci_attach_device(struct ossd_pci_registry *registry,
    const struct ossd_pci_device *device);
int ossd_pci_detach_device(struct ossd_pci_registry *registry,
    uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
void ossd_pci_detach_all(struct ossd_pci_registry *registry);
size_t ossd_pci_bound_count(const struct ossd_pci_registry *registry);

#ifdef __cplusplus
}
#endif
#endif /* OSSD_PCI_DRIVER_H */
