/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <device/ossd_pci_driver.h>

struct probe_state { int probes; int removes; int fail; };
static struct probe_state state;

static int
probe_network(const struct ossd_pci_device *device,
    const struct ossd_pci_id *id, void **data)
{
	assert(device != 0 && id != 0 && data != 0);
	state.probes++;
	if (state.fail) return -1;
	*data = &state;
	return 0;
}

static void
remove_network(const struct ossd_pci_device *device, void *data)
{
	assert(device != 0 && data == &state);
	state.removes++;
}

int main(void)
{
	static const struct ossd_pci_id ids[] = {{
		.vendor_id = 0x10ec, .device_id = 0xc821,
		.subsystem_vendor_id = OSSD_PCI_ANY_ID,
		.subsystem_id = OSSD_PCI_ANY_ID,
		.class_code = 0x02, .class_mask = 0xff,
		.subclass = 0x80, .subclass_mask = 0x80
	}};
	static const struct ossd_pci_driver driver = {
		.name = "test-realtek", .ids = ids, .id_count = 1,
		.probe = probe_network, .remove = remove_network
	};
	struct ossd_pci_registry registry;
	struct ossd_pci_device device;

	memset(&state, 0, sizeof(state));
	memset(&device, 0, sizeof(device));
	device.segment = 0; device.bus = 2; device.device = 3;
	device.function = 0; device.vendor_id = 0x10ec;
	device.device_id = 0xc821; device.class_code = 0x02;
	device.subclass = 0x80; device.subsystem_vendor_id = 0x103c;
	device.subsystem_id = 0x831a;

	ossd_pci_registry_init(&registry);
	assert(ossd_pci_register_driver(&registry, &driver) == 0);
	assert(ossd_pci_register_driver(&registry, &driver) == -2);
	assert(ossd_pci_attach_device(&registry, &device) == 0);
	assert(ossd_pci_bound_count(&registry) == 1);
	assert(state.probes == 1 && state.removes == 0);
	assert(ossd_pci_attach_device(&registry, &device) == -2);
	assert(ossd_pci_detach_device(&registry, 0, 2, 3, 0) == 0);
	assert(ossd_pci_bound_count(&registry) == 0);
	assert(state.removes == 1);
	assert(ossd_pci_detach_device(&registry, 0, 2, 3, 0) == -2);

	state.fail = 1;
	assert(ossd_pci_attach_device(&registry, &device) == -3);
	assert(ossd_pci_bound_count(&registry) == 0);
	state.fail = 0;
	assert(ossd_pci_attach_device(&registry, &device) == 0);
	ossd_pci_detach_all(&registry);
	assert(ossd_pci_bound_count(&registry) == 0);
	assert(state.removes == 2);

	/* Class mismatch and a different vendor must not bind. */
	device.class_code = 0x01;
	assert(ossd_pci_attach_device(&registry, &device) == -3);
	device.class_code = 0x02;
	device.vendor_id = 0x8086;
	assert(ossd_pci_attach_device(&registry, &device) == -3);
	assert(ossd_pci_register_driver(0, &driver) == -1);
	return 0;
}
