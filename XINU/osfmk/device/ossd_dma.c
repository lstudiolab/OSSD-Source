/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device/ossd_dma.h>
#include <stdint.h>

static int
dma_power_of_two(uint64_t value)
{
	return value != 0 && (value & (value - 1ULL)) == 0;
}

static int
dma_valid_direction(enum ossd_dma_direction direction)
{
	return direction == OSSD_DMA_TO_DEVICE ||
	    direction == OSSD_DMA_FROM_DEVICE ||
	    direction == OSSD_DMA_BIDIRECTIONAL;
}

static int
dma_range_valid(const struct ossd_dma_mapping *mapping, uint64_t offset,
    uint64_t length)
{
	return mapping != 0 && mapping->active && mapping->cpu_address != 0 &&
	    length != 0 && offset <= mapping->length &&
	    length <= mapping->length - offset;
}

int
ossd_dma_allocate(const struct ossd_dma_provider_ops *ops, void *context,
    uint64_t length, uint64_t alignment, uint64_t boundary,
    uint8_t address_bits, enum ossd_dma_direction direction,
    struct ossd_dma_mapping *mapping)
{
	uint64_t max_address, last_address;
	int error;

	if (ops == 0 || ops->allocate == 0 || ops->release == 0 ||
	    ops->sync_for_device == 0 || ops->sync_for_cpu == 0 ||
	    mapping == 0 || length == 0 || !dma_power_of_two(alignment) ||
	    (boundary != 0 && !dma_power_of_two(boundary)) ||
	    (boundary != 0 && length > boundary) ||
	    address_bits == 0 || address_bits > 64 ||
	    !dma_valid_direction(direction)) return -1;

	mapping->cpu_address = 0;
	mapping->device_address = 0;
	mapping->length = 0;
	mapping->provider_cookie = 0;
	mapping->active = 0;
	max_address = address_bits == 64 ? UINT64_MAX :
	    ((1ULL << address_bits) - 1ULL);

	error = ops->allocate(context, length, alignment, boundary,
	    max_address, direction, mapping);
	if (error != 0) return error;
	if (mapping->cpu_address == 0 || mapping->length < length ||
	    (((uintptr_t)mapping->cpu_address) & (alignment - 1ULL)) != 0 ||
	    (mapping->device_address & (alignment - 1ULL)) != 0 ||
	    mapping->device_address > max_address ||
	    length - 1ULL > UINT64_MAX - mapping->device_address) {
		ops->release(context, mapping);
		mapping->cpu_address = 0;
		mapping->device_address = 0;
		mapping->length = 0;
		mapping->provider_cookie = 0;
		mapping->active = 0;
		return -2;
	}
	last_address = mapping->device_address + length - 1ULL;
	if (last_address > max_address ||
	    (boundary != 0 &&
	    mapping->device_address / boundary != last_address / boundary)) {
		ops->release(context, mapping);
		mapping->cpu_address = 0;
		mapping->device_address = 0;
		mapping->length = 0;
		mapping->provider_cookie = 0;
		mapping->active = 0;
		return -3;
	}
	mapping->length = length;
	mapping->active = 1;
	return 0;
}

void
ossd_dma_release(const struct ossd_dma_provider_ops *ops, void *context,
    struct ossd_dma_mapping *mapping)
{
	if (ops == 0 || ops->release == 0 || mapping == 0 ||
	    !mapping->active) return;
	ops->release(context, mapping);
	mapping->cpu_address = 0;
	mapping->device_address = 0;
	mapping->length = 0;
	mapping->provider_cookie = 0;
	mapping->active = 0;
}

int
ossd_dma_sync_for_device(const struct ossd_dma_provider_ops *ops,
    void *context, const struct ossd_dma_mapping *mapping, uint64_t offset,
    uint64_t length, enum ossd_dma_direction direction)
{
	int error;
	if (ops == 0 || ops->sync_for_device == 0 ||
	    !dma_range_valid(mapping, offset, length) ||
	    !dma_valid_direction(direction)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	error = ops->sync_for_device(context, mapping, offset, length, direction);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return error;
}

int
ossd_dma_sync_for_cpu(const struct ossd_dma_provider_ops *ops,
    void *context, const struct ossd_dma_mapping *mapping, uint64_t offset,
    uint64_t length, enum ossd_dma_direction direction)
{
	int error;
	if (ops == 0 || ops->sync_for_cpu == 0 ||
	    !dma_range_valid(mapping, offset, length) ||
	    !dma_valid_direction(direction)) return -1;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	error = ops->sync_for_cpu(context, mapping, offset, length, direction);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	return error;
}
