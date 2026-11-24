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
 * Atomically with respect to the caller's config-space serialization,
 * update only the low 16-bit PCI command register. The high 16-bit status
 * register is written as zero to avoid acknowledging write-one-to-clear
 * status bits. Enable bus mastering only after DMA buffers and mappings are
 * ready. Returns 0 on success, negative on invalid access/input.
 */
int ossd_pci_update_command(ossd_pci_config_read_fn read_config,
    ossd_pci_config_write_fn write_config, void *context, uint8_t bus,
    uint8_t device, uint8_t function, uint16_t set_bits,
    uint16_t clear_bits);
struct ossd_pci_ecam_region {
	uint64_t base_address;
	uint16_t segment;
	uint8_t start_bus;
	uint8_t end_bus;
};

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
 * attributes. Mapping and ACPI MCFG/Device Tree discovery are platform work.
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
