/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_pci.h>

#define PCI_VENDOR_DEVICE 0x00
#define PCI_REVISION_CLASS 0x08
#define PCI_HEADER_TYPE 0x0c
#define PCI_SUBSYSTEM_IDS 0x2c
#define PCI_INTERRUPT 0x3c
#define PCI_BAR0 0x10
#define PCI_BAR_COUNT 6
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTIONS 8
#define PCI_ABSENT 0xffffffffU

static uint32_t
pci_read(ossd_pci_config_read_fn read_config, void *context,
    uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if (read_config == 0 || (offset & 3) != 0 || offset >= 4096) {
		return PCI_ABSENT;
	}
	return read_config(context, bus, device, function, offset);
}

uint32_t
ossd_pci_scan(ossd_pci_config_read_fn read_config, void *read_context,
    ossd_pci_device_fn found, void *found_context)
{
	uint32_t count = 0;
	uint32_t bus;

	if (read_config == 0 || first_bus > last_bus) {
		return 0;
	}

	/*
	 * Scan the complete 8-bit bus-number space. This also discovers buses
	 * behind bridges without assuming firmware assigned contiguous numbers.
	 * Reads are side-effect-free; this routine deliberately does not size
	 * BARs, enable bus mastering, or alter bridge windows.
	 */
	for (bus = first_bus; bus <= last_bus; bus++) {
		uint32_t slot;

		for (slot = 0; slot < PCI_MAX_DEVICES; slot++) {
			uint32_t id0 = pci_read(read_config, read_context,
			    (uint8_t)bus, (uint8_t)slot, 0, PCI_VENDOR_DEVICE);
			uint16_t vendor0 = (uint16_t)(id0 & 0xffffU);
			uint32_t header;
			uint32_t function_count;
			uint32_t function;

			if (id0 == PCI_ABSENT || vendor0 == 0xffffU ||
			    vendor0 == 0) {
				continue;
			}

			header = pci_read(read_config, read_context,
			    (uint8_t)bus, (uint8_t)slot, 0, PCI_HEADER_TYPE);
			function_count = ((header >> 16) & 0x80U) ? 8U : 1U;

			for (function = 0; function < function_count; function++) {
				struct ossd_pci_device info;
				uint32_t id, class_reg, hdr, subsystem, irq;
				uint32_t bar_count, bar;

				id = function == 0 ? id0 :
				    pci_read(read_config, read_context, (uint8_t)bus,
				    (uint8_t)slot, (uint8_t)function,
				    PCI_VENDOR_DEVICE);
				if (id == PCI_ABSENT || (id & 0xffffU) == 0xffffU ||
				    (id & 0xffffU) == 0) {
					continue;
				}

				class_reg = pci_read(read_config, read_context,
				    (uint8_t)bus, (uint8_t)slot,
				    (uint8_t)function, PCI_REVISION_CLASS);
				hdr = pci_read(read_config, read_context,
				    (uint8_t)bus, (uint8_t)slot,
				    (uint8_t)function, PCI_HEADER_TYPE);
				subsystem = pci_read(read_config, read_context,
				    (uint8_t)bus, (uint8_t)slot,
				    (uint8_t)function, PCI_SUBSYSTEM_IDS);
				irq = pci_read(read_config, read_context,
				    (uint8_t)bus, (uint8_t)slot,
				    (uint8_t)function, PCI_INTERRUPT);

				info.segment = segment;
				info.bus = (uint8_t)bus;
				info.device = (uint8_t)slot;
				info.function = (uint8_t)function;
				info.vendor_id = (uint16_t)(id & 0xffffU);
				info.device_id = (uint16_t)(id >> 16);
				info.revision = (uint8_t)(class_reg & 0xffU);
				info.programming_interface =
				    (uint8_t)((class_reg >> 8) & 0xffU);
				info.subclass = (uint8_t)((class_reg >> 16) & 0xffU);
				info.class_code = (uint8_t)((class_reg >> 24) & 0xffU);
				info.header_type = (uint8_t)((hdr >> 16) & 0x7fU);
				info.subsystem_vendor_id =
				    (uint16_t)(subsystem & 0xffffU);
				info.subsystem_id = (uint16_t)(subsystem >> 16);
				info.interrupt_line = (uint8_t)(irq & 0xffU);
				info.interrupt_pin = (uint8_t)((irq >> 8) & 0xffU);

				bar_count = info.header_type == 0 ? 6U :
				    (info.header_type == 1 ? 2U : 0U);
				for (bar = 0; bar < OSSD_PCI_MAX_BARS; bar++) {
					info.bars[bar] = bar < bar_count ?
					    pci_read(read_config, read_context,
					    (uint8_t)bus, (uint8_t)slot,
					    (uint8_t)function,
					    (uint16_t)(PCI_BAR0 + bar * 4U)) : 0;
				}

				count++;
				if (found != 0) {
					found(found_context, &info);
				}
			}
		}
	}
	return count;
}

struct ossd_pci_ecam_context {
	volatile uint8_t *base;
	const struct ossd_pci_ecam_region *region;
};

static uint32_t
pci_ecam_read(void *context, uint8_t bus, uint8_t device,
    uint8_t function, uint16_t offset)
{
	struct ossd_pci_ecam_context *ecam =
	    (struct ossd_pci_ecam_context *)context;
	uint64_t bus_offset;
	uint64_t register_offset;
	volatile uint32_t *register_address;

	if (ecam == 0 || ecam->base == 0 || ecam->region == 0 ||
	    bus < ecam->region->start_bus || bus > ecam->region->end_bus ||
	    device >= 32 || function >= 8 || offset >= 4096 ||
	    (offset & 3) != 0) {
		return PCI_ABSENT;
	}

	bus_offset = ((uint64_t)(bus - ecam->region->start_bus) << 20);
	register_offset = bus_offset | ((uint64_t)device << 15) |
	    ((uint64_t)function << 12) | (uint64_t)offset;
	register_address = (volatile uint32_t *)(ecam->base + register_offset);
	return *register_address;
}

uint32_t
ossd_pci_scan_ecam(volatile void *mapped_base,
    const struct ossd_pci_ecam_region *region, ossd_pci_device_fn found,
    void *found_context)
{
	struct ossd_pci_ecam_context context;

	if (mapped_base == 0 || region == 0 ||
	    region->start_bus > region->end_bus) {
		return 0;
	}
	context.base = (volatile uint8_t *)mapped_base;
	context.region = region;
	return ossd_pci_scan(pci_ecam_read, &context, region->segment,
	    region->start_bus, region->end_bus, found, found_context);
}
