/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_DMA_H
#define OSSD_DMA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ossd_dma_direction {
	OSSD_DMA_TO_DEVICE = 1,
	OSSD_DMA_FROM_DEVICE = 2,
	OSSD_DMA_BIDIRECTIONAL = 3
};

/*
 * One contiguous device-visible DMA segment. Provider implementations must
 * return memory with the requested alignment, address-width and boundary
 * constraints, and must implement cache maintenance in the sync callbacks.
 */
struct ossd_dma_mapping {
	void *cpu_address;
	uint64_t device_address;
	uint64_t length;
	void *provider_cookie;
	uint8_t active;
	uint8_t direction;
};

struct ossd_dma_provider_ops {
	int (*allocate)(void *context, uint64_t length, uint64_t alignment,
	    uint64_t boundary, uint64_t max_device_address,
	    enum ossd_dma_direction direction,
	    struct ossd_dma_mapping *mapping);
	void (*release)(void *context, struct ossd_dma_mapping *mapping);
	int (*sync_for_device)(void *context,
	    const struct ossd_dma_mapping *mapping, uint64_t offset,
	    uint64_t length, enum ossd_dma_direction direction);
	int (*sync_for_cpu)(void *context,
	    const struct ossd_dma_mapping *mapping, uint64_t offset,
	    uint64_t length, enum ossd_dma_direction direction);
};

int ossd_dma_allocate(const struct ossd_dma_provider_ops *ops,
    void *context, uint64_t length, uint64_t alignment, uint64_t boundary,
    uint8_t address_bits, enum ossd_dma_direction direction,
    struct ossd_dma_mapping *mapping);
void ossd_dma_release(const struct ossd_dma_provider_ops *ops,
    void *context, struct ossd_dma_mapping *mapping);
int ossd_dma_sync_for_device(const struct ossd_dma_provider_ops *ops,
    void *context, const struct ossd_dma_mapping *mapping,
    uint64_t offset, uint64_t length,
    enum ossd_dma_direction direction);
int ossd_dma_sync_for_cpu(const struct ossd_dma_provider_ops *ops,
    void *context, const struct ossd_dma_mapping *mapping,
    uint64_t offset, uint64_t length,
    enum ossd_dma_direction direction);

#ifdef __cplusplus
}
#endif
#endif
