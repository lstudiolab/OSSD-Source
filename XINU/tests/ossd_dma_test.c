/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <device/ossd_dma.h>

static _Alignas(64) uint8_t storage[4096];
static uint64_t supplied_address;
static int releases, device_syncs, cpu_syncs;
static int
alloc(void *ctx, uint64_t len, uint64_t align, uint64_t boundary,
    uint64_t max_address, enum ossd_dma_direction dir,
    struct ossd_dma_mapping *m)
{
	(void)ctx; (void)align; (void)boundary; (void)max_address; (void)dir;
	assert(len <= sizeof(storage));
	m->cpu_address = storage;
	m->device_address = supplied_address;
	m->length = len;
	m->provider_cookie = storage;
	return 0;
}
static void
release(void *ctx, struct ossd_dma_mapping *m)
{
	(void)ctx; (void)m; releases++;
}
static int
sync_device(void *ctx, const struct ossd_dma_mapping *m, uint64_t off,
    uint64_t len, enum ossd_dma_direction dir)
{
	(void)ctx; (void)m; (void)off; (void)len; (void)dir;
	device_syncs++; return 0;
}
static int
sync_cpu(void *ctx, const struct ossd_dma_mapping *m, uint64_t off,
    uint64_t len, enum ossd_dma_direction dir)
{
	(void)ctx; (void)m; (void)off; (void)len; (void)dir;
	cpu_syncs++; return 0;
}
int
main(void)
{
	const struct ossd_dma_provider_ops ops = {
		.allocate = alloc, .release = release,
		.sync_for_device = sync_device, .sync_for_cpu = sync_cpu
	};
	struct ossd_dma_mapping mapping;
	supplied_address = 0x100000ULL;
	releases = device_syncs = cpu_syncs = 0;
	assert(ossd_dma_allocate(&ops, 0, 1024, 64, 4096, 32,
	    OSSD_DMA_BIDIRECTIONAL, &mapping) == 0);
	assert(mapping.active && mapping.length == 1024);
	assert(ossd_dma_sync_for_device(&ops, 0, &mapping, 0, 512,
	    OSSD_DMA_TO_DEVICE) == 0);
	assert(ossd_dma_sync_for_cpu(&ops, 0, &mapping, 512, 512,
	    OSSD_DMA_FROM_DEVICE) == 0);
	assert(device_syncs == 1 && cpu_syncs == 1);
	assert(ossd_dma_sync_for_device(&ops, 0, &mapping, 0, 64,
	    OSSD_DMA_FROM_DEVICE) == 0);
	assert(ossd_dma_sync_for_cpu(&ops, 0, &mapping, 1000, 64,
	    OSSD_DMA_FROM_DEVICE) == -1);
	ossd_dma_release(&ops, 0, &mapping);
	assert(!mapping.active && releases == 1);

	/* A device address crossing a required boundary is rejected and freed. */
	supplied_address = 0x1ff800ULL;
	assert(ossd_dma_allocate(&ops, 0, 4096, 64, 4096, 32,
	    OSSD_DMA_BIDIRECTIONAL, &mapping) == -3);
	assert(releases == 2 && !mapping.active);

	/* A 32-bit DMA mask rejects an address above 4 GiB. */
	supplied_address = 0x100000000ULL;
	assert(ossd_dma_allocate(&ops, 0, 1024, 64, 0, 32,
	    OSSD_DMA_TO_DEVICE, &mapping) == -2);
	assert(releases == 3 && !mapping.active);

	assert(ossd_dma_allocate(&ops, 0, 0, 64, 0, 32,
	    OSSD_DMA_TO_DEVICE, &mapping) == -1);
	assert(ossd_dma_allocate(&ops, 0, 1024, 48, 0, 32,
	    OSSD_DMA_TO_DEVICE, &mapping) == -1);
	supplied_address = 0x200000ULL;
	assert(ossd_dma_allocate(&ops, 0, 1024, 64, 0, 32,
	    OSSD_DMA_TO_DEVICE, &mapping) == 0);
	assert(ossd_dma_sync_for_cpu(&ops, 0, &mapping, 0, 64,
	    OSSD_DMA_FROM_DEVICE) == -1);
	ossd_dma_release(&ops, 0, &mapping);
	return 0;
}
