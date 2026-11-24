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
    uint16_t segment, uint8_t first_bus, uint8_t last_bus,
    ossd_pci_device_fn found, void *found_context)
{
	uint32_t count = 0;
	uint32_t bus;

	if (read_config == 0 || first_bus > last_bus) {
		return 0;
	}

	/*
	 * Scan the caller-provided bus-number range. This also discovers buses
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

uint32_t
ossd_pci_ecam_config_read(void *context, uint8_t bus, uint8_t device,
    uint8_t function, uint16_t offset)
{
	struct ossd_pci_ecam_access *ecam =
	    (struct ossd_pci_ecam_access *)context;
	uint64_t bus_offset, register_offset;
	volatile uint32_t *register_address;

	if (ecam == 0 || ecam->mapped_base == 0 || ecam->region == 0 ||
	    bus < ecam->region->start_bus || bus > ecam->region->end_bus ||
	    device >= 32 || function >= 8 || offset >= 4096 ||
	    (offset & 3) != 0) return PCI_ABSENT;
	bus_offset = ((uint64_t)(bus - ecam->region->start_bus) << 20);
	register_offset = bus_offset | ((uint64_t)device << 15) |
	    ((uint64_t)function << 12) | (uint64_t)offset;
	register_address = (volatile uint32_t *)(ecam->mapped_base +
	    register_offset);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return *register_address;
}

void
ossd_pci_ecam_config_write(void *context, uint8_t bus, uint8_t device,
    uint8_t function, uint16_t offset, uint32_t value)
{
	struct ossd_pci_ecam_access *ecam =
	    (struct ossd_pci_ecam_access *)context;
	uint64_t bus_offset, register_offset;
	volatile uint32_t *register_address;

	if (ecam == 0 || ecam->mapped_base == 0 || ecam->region == 0 ||
	    bus < ecam->region->start_bus || bus > ecam->region->end_bus ||
	    device >= 32 || function >= 8 || offset >= 4096 ||
	    (offset & 3) != 0) return;
	bus_offset = ((uint64_t)(bus - ecam->region->start_bus) << 20);
	register_offset = bus_offset | ((uint64_t)device << 15) |
	    ((uint64_t)function << 12) | (uint64_t)offset;
	register_address = (volatile uint32_t *)(ecam->mapped_base +
	    register_offset);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*register_address = value;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

uint32_t
ossd_pci_scan_ecam(volatile void *mapped_base,
    const struct ossd_pci_ecam_region *region, ossd_pci_device_fn found,
    void *found_context)
{
	struct ossd_pci_ecam_access context;

	if (mapped_base == 0 || region == 0 ||
	    region->start_bus > region->end_bus) {
		return 0;
	}
	context.mapped_base = (volatile uint8_t *)mapped_base;
	context.region = region;
	context.region = region;
	return ossd_pci_scan(ossd_pci_ecam_config_read, &context, region->segment,
	    region->start_bus, region->end_bus, found, found_context);
}

/*
 * Update only PCI command bits. The upper status half is deliberately zeroed
 * because several status bits are write-one-to-clear.
 */
int
ossd_pci_update_command(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t set_bits,
    uint16_t clear_bits)
{
	uint32_t command_status;
	uint16_t command;

	if (read_config == 0 || write_config == 0 ||
	    device >= 32 || function >= 8 ||
	    ((set_bits | clear_bits) & ~OSSD_PCI_COMMAND_ALLOWED_MASK) != 0 ||
	    (set_bits & clear_bits) != 0) {
		return -1;
	}
	command_status = read_config(context, bus, device, function, 0x04);
	if (command_status == PCI_ABSENT) return -2;
	command = (uint16_t)command_status;
	command = (uint16_t)((command | set_bits) & (uint16_t)~clear_bits);
	/* Upper 16 bits are zero to avoid clearing W1C status flags. */
	write_config(context, bus, device, function, 0x04, command);
	return 0;
}

/*
 * Probe a BAR size using the PCI-defined all-ones mask procedure. This is
 * intentionally separate from enumeration because config writes have side
 * effects and require the caller to serialize the complete transaction.
 */
int
ossd_pci_size_bar(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *context,
    const struct ossd_pci_device *device, uint8_t bar_index,
    struct ossd_pci_bar_info *result)
{
	uint16_t offset;
	uint32_t command_status, original_low, original_high = 0;
	uint32_t mask_low, mask_high = 0;
	uint32_t disabled_command;
	uint64_t address, mask, size;
	uint8_t bar_count, is_64 = 0, type, prefetch = 0, i;

	if (read_config == 0 || write_config == 0 || device == 0 ||
	    result == 0 || device->device >= 32 || device->function >= 8)
		return -1;

	bar_count = device->header_type == 0 ? 6U :
	    (device->header_type == 1 ? 2U : 0U);
	if (bar_index >= bar_count) return -1;
	/* A 64-bit BAR consumes the following slot; never size its upper half. */
	for (i = 0; i < bar_index; i++) {
		uint32_t previous = read_config(context, device->bus,
		    device->device, device->function,
		    (uint16_t)(PCI_BAR0 + (uint16_t)i * 4U));
		if (previous == PCI_ABSENT) return -2;
		if ((previous & 1U) == 0 && ((previous >> 1) & 3U) == 2U) {
			if ((uint8_t)(i + 1U) == bar_index) return -3;
			i++;
		}
	}

	offset = (uint16_t)(PCI_BAR0 + (uint16_t)bar_index * 4U);
	command_status = read_config(context, device->bus, device->device,
	    device->function, 0x04);
	original_low = read_config(context, device->bus, device->device,
	    device->function, offset);
	if (command_status == PCI_ABSENT || original_low == PCI_ABSENT)
		return -2;

	if ((original_low & 1U) != 0) {
		type = OSSD_PCI_BAR_IO;
		address = (uint64_t)(original_low & ~3U);
	} else {
		uint32_t memory_type = (original_low >> 1) & 3U;
		type = OSSD_PCI_BAR_MEMORY;
		prefetch = (uint8_t)((original_low >> 3) & 1U);
		if (memory_type == 3U) return -3;
		if (memory_type == 2U) {
			if (bar_index + 1U >= bar_count) return -3;
			is_64 = 1;
			original_high = read_config(context, device->bus,
			    device->device, device->function,
			    (uint16_t)(offset + 4U));
			address = ((uint64_t)original_high << 32) |
			    (uint64_t)(original_low & ~0x0fU);
		} else {
			address = (uint64_t)(original_low & ~0x0fU);
		}
	}

	/* Disable both decode paths while the sizing mask is sampled. */
	disabled_command = (command_status & 0xffffU) & ~3U;
	write_config(context, device->bus, device->device,
	    device->function, 0x04, disabled_command);

	if (is_64) {
		write_config(context, device->bus, device->device,
		    device->function, (uint16_t)(offset + 4U), 0xffffffffU);
	}
	write_config(context, device->bus, device->device,
	    device->function, offset, 0xffffffffU);

	mask_low = read_config(context, device->bus, device->device,
	    device->function, offset);
	if (is_64) {
		mask_high = read_config(context, device->bus, device->device,
		    device->function, (uint16_t)(offset + 4U));
	}

	/* Restore BARs before restoring decode. Restore even on bad masks. */
	if (is_64) {
		write_config(context, device->bus, device->device,
		    device->function, (uint16_t)(offset + 4U), original_high);
	}
	write_config(context, device->bus, device->device,
	    device->function, offset, original_low);
	write_config(context, device->bus, device->device,
	    device->function, 0x04, command_status & 0xffffU);

	/* All-ones is a valid upper mask for many 64-bit BARs. */
	if (type == OSSD_PCI_BAR_IO) {
		mask = (uint64_t)(mask_low & ~3U);
		size = ((~mask) & 0xffffffffULL) + 1ULL;
	} else if (is_64) {
		mask = ((uint64_t)mask_high << 32) |
		    (uint64_t)(mask_low & ~0x0fU);
		size = (~mask) + 1ULL;
	} else {
		mask = (uint64_t)(mask_low & ~0x0fU);
		size = ((~mask) & 0xffffffffULL) + 1ULL;
	}

	if (mask == 0 || size == 0 || (size & (size - 1ULL)) != 0)
		return -4;
	if (address != 0 && (address & (size - 1ULL)) != 0)
		return -5;

	result->address = address;
	result->size = size;
	result->index = bar_index;
	result->type = type;
	result->is_64_bit = is_64;
	result->prefetchable = prefetch;
	return 0;
}

/* PCI BAR mapping and bounded MMIO register access. */
int
ossd_pci_map_bar(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *config_context,
    const struct ossd_pci_device *device, uint8_t bar_index,
    uint64_t minimum_length, const struct ossd_pci_mmio_ops *ops,
    void *map_context, struct ossd_pci_mmio_region *region)
{
	struct ossd_pci_bar_info bar;
	volatile void *mapped = 0;
	int error;

	if (ops == 0 || ops->map == 0 || ops->unmap == 0 ||
	    region == 0 || minimum_length == 0) return -1;
	region->base = 0;
	region->physical_address = 0;
	region->length = 0;
	region->bar_index = 0;
	error = ossd_pci_size_bar(read_config, write_config, config_context,
	    device, bar_index, &bar);
	if (error != 0) return error;
	if (bar.type != OSSD_PCI_BAR_MEMORY || bar.address == 0 ||
	    bar.size < minimum_length) return -2;
	error = ops->map(map_context, bar.address, bar.size, &mapped);
	if (error != 0) return error;
	if (mapped == 0) return -3;
	region->base = (volatile uint8_t *)mapped;
	region->physical_address = bar.address;
	region->length = bar.size;
	region->bar_index = bar_index;
	return 0;
}

void
ossd_pci_unmap_bar(const struct ossd_pci_mmio_ops *ops,
    void *map_context, struct ossd_pci_mmio_region *region)
{
	if (ops == 0 || ops->unmap == 0 || region == 0) return;
	if (region->base != 0)
		ops->unmap(map_context, (volatile void *)region->base,
		    region->length);
	region->base = 0;
	region->physical_address = 0;
	region->length = 0;
	region->bar_index = 0;
}

static int
pci_mmio_valid(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint64_t width)
{
	return region != 0 && region->base != 0 && width != 0 &&
	    offset <= region->length && width <= region->length - offset &&
	    (offset & (width - 1ULL)) == 0;
}

int
ossd_pci_mmio_read8(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint8_t *v)
{
	if (v == 0 || !pci_mmio_valid(r, o, 1)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*v = *(volatile uint8_t *)(r->base + o);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
int
ossd_pci_mmio_read16(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint16_t *v)
{
	if (v == 0 || !pci_mmio_valid(r, o, 2)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*v = *(volatile uint16_t *)(void *)(r->base + o);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
int
ossd_pci_mmio_read32(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint32_t *v)
{
	if (v == 0 || !pci_mmio_valid(r, o, 4)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*v = *(volatile uint32_t *)(void *)(r->base + o);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
int
ossd_pci_mmio_write8(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint8_t v)
{
	if (!pci_mmio_valid(r, o, 1)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*(volatile uint8_t *)(r->base + o) = v;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
int
ossd_pci_mmio_write16(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint16_t v)
{
	if (!pci_mmio_valid(r, o, 2)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*(volatile uint16_t *)(void *)(r->base + o) = v;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
int
ossd_pci_mmio_write32(const struct ossd_pci_mmio_region *r, uint64_t o,
    uint32_t v)
{
	if (!pci_mmio_valid(r, o, 4)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	*(volatile uint32_t *)(void *)(r->base + o) = v;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return 0;
}
