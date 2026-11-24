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
	uint16_t segment;
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
typedef void (*ossd_pci_config_write_fn)(
    void *context, uint8_t bus, uint8_t device, uint8_t function,
    uint16_t offset, uint32_t value);

/* PCI command-register bits accepted by ossd_pci_update_command(). */
#define OSSD_PCI_COMMAND_IO_SPACE       0x0001U
#define OSSD_PCI_COMMAND_MEMORY_SPACE   0x0002U
#define OSSD_PCI_COMMAND_BUS_MASTER     0x0004U
#define OSSD_PCI_COMMAND_ALLOWED_MASK  0x0007U

/*
 * With the caller serializing the complete read/modify/write sequence,
 * update only the low 16-bit PCI command register. The high 16-bit status
 * register is written as zero to avoid acknowledging write-one-to-clear
 * status bits. Enable bus mastering only after DMA buffers and mappings are
 * ready. Returns 0 on success, negative on invalid access/input.
 */
int ossd_pci_update_command(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t set_bits,
    uint16_t clear_bits);
enum ossd_pci_bar_type {
	OSSD_PCI_BAR_IO = 1,
	OSSD_PCI_BAR_MEMORY = 2
};

struct ossd_pci_bar_info {
	uint64_t address;
	uint64_t size;
	uint8_t index;
	uint8_t type;
	uint8_t is_64_bit;
	uint8_t prefetchable;
};

/*
 * Explicitly sizes one firmware-assigned BAR. This temporarily disables
 * I/O and memory decoding, writes all ones to the selected BAR, reads its
 * size mask, restores the original BAR values, then restores the command
 * register. Caller must serialize config-space access for this function.
 * It never assigns an address or enables bus mastering.
 */
int ossd_pci_size_bar(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *context,
    const struct ossd_pci_device *device, uint8_t bar_index,
    struct ossd_pci_bar_info *result);

struct ossd_pci_mmio_ops {
	/* Map with device-memory attributes; never cache PCI register windows. */
	int (*map)(void *context, uint64_t physical_address, uint64_t length,
	    volatile void **virtual_address);
	void (*unmap)(void *context, volatile void *virtual_address,
	    uint64_t length);
};

struct ossd_pci_mmio_region {
	volatile uint8_t *base;
	uint64_t physical_address;
	uint64_t length;
	uint8_t bar_index;
};

int ossd_pci_map_bar(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *config_context,
    const struct ossd_pci_device *device, uint8_t bar_index,
    uint64_t minimum_length, const struct ossd_pci_mmio_ops *ops,
    void *map_context, struct ossd_pci_mmio_region *region);
void ossd_pci_unmap_bar(const struct ossd_pci_mmio_ops *ops,
    void *map_context, struct ossd_pci_mmio_region *region);
int ossd_pci_mmio_read8(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint8_t *value);
int ossd_pci_mmio_read16(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint16_t *value);
int ossd_pci_mmio_read32(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint32_t *value);
int ossd_pci_mmio_write8(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint8_t value);
int ossd_pci_mmio_write16(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint16_t value);
int ossd_pci_mmio_write32(const struct ossd_pci_mmio_region *region,
    uint64_t offset, uint32_t value);

struct ossd_pci_ecam_region {
	uint64_t base_address;
	uint16_t segment;
	uint8_t start_bus;
	uint8_t end_bus;
};

struct ossd_pci_ecam_access {
	volatile uint8_t *mapped_base;
	const struct ossd_pci_ecam_region *region;
};

/* ECAM configuration callbacks for x86-64 and arm64 platform providers. */
uint32_t ossd_pci_ecam_config_read(void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t offset);
void ossd_pci_ecam_config_write(void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t offset, uint32_t value);

typedef void (*ossd_pci_device_fn)(
    void *context, const struct ossd_pci_device *device);

/*
 * Enumerates conventional PCI configuration space through a platform-supplied
 * configuration reader. The scanner never enables devices or writes config
 * registers. Returns the number of discovered functions.
 */
uint32_t ossd_pci_scan(ossd_pci_config_read_fn read_config,
    void *read_context, uint16_t segment, uint8_t first_bus, uint8_t last_bus,
    ossd_pci_device_fn found, void *found_context);

/*
 * Scans an already-mapped PCIe ECAM region. mapped_base must map
 * region->base_address for (end_bus - start_bus + 1) MiB with device-memory
 * attributes and at least 4-byte alignment. Mapping and ACPI MCFG/Device
 * Tree discovery are platform work. Each access context must pair the mapping
 * with the exact segment-specific region used to create it.
 */
uint32_t ossd_pci_scan_ecam(volatile void *mapped_base,
    const struct ossd_pci_ecam_region *region, ossd_pci_device_fn found,
    void *found_context);

#if defined(__x86_64__)
/* x86 legacy PCI Configuration Mechanism #1 backend. */
uint32_t ossd_pci_platform_enumerate(ossd_pci_device_fn found,
    void *found_context);
uint32_t ossd_pci_platform_config_read(void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t offset);
void ossd_pci_platform_config_write(void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t offset, uint32_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OSSD_PCI_H */
