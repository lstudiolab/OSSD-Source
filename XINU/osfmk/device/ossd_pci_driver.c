/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_pci_driver.h>

static int
pci_id_matches(const struct ossd_pci_id *id,
    const struct ossd_pci_device *device)
{
	if (id->vendor_id != OSSD_PCI_ANY_ID &&
	    id->vendor_id != device->vendor_id) return 0;
	if (id->device_id != OSSD_PCI_ANY_ID &&
	    id->device_id != device->device_id) return 0;
	if (id->subsystem_vendor_id != OSSD_PCI_ANY_ID &&
	    id->subsystem_vendor_id != device->subsystem_vendor_id) return 0;
	if (id->subsystem_id != OSSD_PCI_ANY_ID &&
	    id->subsystem_id != device->subsystem_id) return 0;
	if ((device->class_code & id->class_mask) !=
	    (id->class_code & id->class_mask)) return 0;
	if ((device->subclass & id->subclass_mask) !=
	    (id->subclass & id->subclass_mask)) return 0;
	if ((device->programming_interface &
	    id->programming_interface_mask) !=
	    (id->programming_interface & id->programming_interface_mask))
		return 0;
	return 1;
}

static int
pci_same_function(const struct ossd_pci_device *a,
    const struct ossd_pci_device *b)
{
	return a->segment == b->segment && a->bus == b->bus &&
	    a->device == b->device && a->function == b->function;
}

void
ossd_pci_registry_init(struct ossd_pci_registry *registry)
{
	size_t i;

	if (registry == 0) return;
	registry->driver_count = 0;
	registry->bound_count = 0;
	for (i = 0; i < OSSD_PCI_DRIVER_MAX; i++) registry->drivers[i] = 0;
	for (i = 0; i < OSSD_PCI_BOUND_DEVICE_MAX; i++) {
		registry->bound[i].occupied = 0;
		registry->bound[i].driver = 0;
		registry->bound[i].driver_data = 0;
	}
}

int
ossd_pci_register_driver(struct ossd_pci_registry *registry,
    const struct ossd_pci_driver *driver)
{
	size_t i;

	if (registry == 0 || driver == 0 || driver->name == 0 ||
	    driver->name[0] == '\0' || driver->ids == 0 ||
	    driver->id_count == 0 || driver->probe == 0 ||
	    driver->remove == 0) return -1;
	for (i = 0; i < registry->driver_count; i++) {
		if (registry->drivers[i] == driver) return -2;
	}
	if (registry->driver_count >= OSSD_PCI_DRIVER_MAX) return -3;
	registry->drivers[registry->driver_count++] = driver;
	return 0;
}

int
ossd_pci_attach_device(struct ossd_pci_registry *registry,
    const struct ossd_pci_device *device)
{
	size_t i, j, free_slot = OSSD_PCI_BOUND_DEVICE_MAX;

	if (registry == 0 || device == 0) return -1;
	for (i = 0; i < OSSD_PCI_BOUND_DEVICE_MAX; i++) {
		if (registry->bound[i].occupied) {
			if (pci_same_function(&registry->bound[i].device, device))
				return -2;
		} else if (free_slot == OSSD_PCI_BOUND_DEVICE_MAX) {
			free_slot = i;
		}
	}
	if (free_slot == OSSD_PCI_BOUND_DEVICE_MAX) return -4;

	/* Driver registration order is deterministic; first successful probe wins. */
	for (i = 0; i < registry->driver_count; i++) {
		const struct ossd_pci_driver *driver = registry->drivers[i];
		for (j = 0; j < driver->id_count; j++) {
			void *driver_data = 0;
			if (!pci_id_matches(&driver->ids[j], device)) continue;
			if (driver->probe(device, &driver->ids[j],
			    &driver_data) == 0) {
				struct ossd_pci_bound_device *bound =
				    &registry->bound[free_slot];
				bound->device = *device;
				bound->driver = driver;
				bound->driver_data = driver_data;
				bound->occupied = 1;
				registry->bound_count++;
				return 0;
			}
		}
	}
	return -3;
}

int
ossd_pci_detach_device(struct ossd_pci_registry *registry,
    uint16_t segment, uint8_t bus, uint8_t device, uint8_t function)
{
	size_t i;

	if (registry == 0) return -1;
	for (i = 0; i < OSSD_PCI_BOUND_DEVICE_MAX; i++) {
		struct ossd_pci_bound_device *bound = &registry->bound[i];
		if (bound->occupied && bound->device.segment == segment &&
		    bound->device.bus == bus && bound->device.device == device &&
		    bound->device.function == function) {
			const struct ossd_pci_driver *driver = bound->driver;
			struct ossd_pci_device copy = bound->device;
			void *driver_data = bound->driver_data;
			bound->occupied = 0;
			bound->driver = 0;
			bound->driver_data = 0;
			registry->bound_count--;
			driver->remove(&copy, driver_data);
			return 0;
		}
	}
	return -2;
}

void
ossd_pci_detach_all(struct ossd_pci_registry *registry)
{
	size_t i;

	if (registry == 0) return;
	for (i = OSSD_PCI_BOUND_DEVICE_MAX; i > 0; i--) {
		struct ossd_pci_bound_device *bound = &registry->bound[i - 1];
		if (bound->occupied) {
			const struct ossd_pci_driver *driver = bound->driver;
			struct ossd_pci_device copy = bound->device;
			void *driver_data = bound->driver_data;
			bound->occupied = 0;
			bound->driver = 0;
			bound->driver_data = 0;
			registry->bound_count--;
			driver->remove(&copy, driver_data);
		}
	}
}

size_t
ossd_pci_bound_count(const struct ossd_pci_registry *registry)
{
	return registry == 0 ? 0 : registry->bound_count;
}
