/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
   
                                                                 
   

#include <mach/mach_types.h>
#include <kern/misc_protos.h>
#include <IOKit/IOHibernatePrivate.h>
#include <machine/pal_hibernate.h>
#include <pexpert/arm/dockchannel.h>
#include <ptrauth.h>
#include <arm/cpu_data_internal.h>
#include <arm/cpu_internal.h>
#include <libkern/section_keywords.h>


pal_hib_tramp_result_t gHibTramp;
pal_hib_globals_t gHibernateGlobals MARK_AS_HIBERNATE_DATA_CONST_LATE;

                                                                                                                           
                                                                                                     
extern IOHibernateImageHeader *gIOHibernateCurrentHeader __attribute__((visibility("hidden")));
extern const uint32_t ccsha256_initial_state[8] __attribute__((visibility("hidden")));
extern void AccelerateCrypto_SHA256_compress(ccdigest_state_t state, size_t numBlocks, const void *data) __attribute__((visibility("hidden")));
extern void ccdigest_final_64be(const struct ccdigest_info *di, ccdigest_ctx_t, unsigned char *digest) __attribute__((visibility("hidden")));
extern struct pmap_cpu_data_array_entry pmap_cpu_data_array[MAX_CPUS] __attribute__((visibility("hidden")));
extern bool hib_entry_pmap_lockdown __attribute__((visibility("hidden")));

uintptr_t
hibernate_restore_phys_page(uint64_t src, uint64_t dst, uint32_t len, __unused uint32_t procFlags)
{
	void *d = (void*)pal_hib_map(DEST_COPY_AREA, dst);
	__nosan_memcpy(d, (void*)src, len);
	return (uintptr_t)d;
}

uintptr_t
pal_hib_map(pal_hib_map_type_t virt, uint64_t phys)
{
	switch (virt) {
	case DEST_COPY_AREA:
	case COPY_PAGE_AREA:
	case SCRATCH_AREA:
	case WKDM_AREA:
		return phys + gHibTramp.memSlide;
	case BITMAP_AREA:
	case IMAGE_AREA:
	case IMAGE2_AREA:
		return phys;
	default:
		HIB_ASSERT(0);
	}
}

void
pal_hib_restore_pal_state(__unused uint32_t *arg)
{
}

void
pal_hib_resume_init(pal_hib_ctx_t *ctx, hibernate_page_list_t *map, uint32_t *nextFree)
{
}

void
pal_hib_restored_page(pal_hib_ctx_t *ctx, pal_hib_restore_stage_t stage, ppnum_t ppnum)
{
}

void
pal_hib_patchup(pal_hib_ctx_t *ctx)
{

	                                                                            
	ppl_hib_lock_reinit();

	                                                                                                              
	for (int i = 0; i < MAX_CPUS; i++) {
		pmap_cpu_data_array[i].cpu_data.ppl_state = PPL_STATE_KERNEL;
		pmap_cpu_data_array[i].cpu_data.ppl_kern_saved_sp = 0;
	}

	                                              
	init_ctrr_cluster_states();

	                                                                         
	                                                                     
	                                                                        
	hib_entry_pmap_lockdown = false;
}

void
pal_hib_decompress_page(void *src, void *dst, void *scratch, unsigned int compressedSize)
{
	const void *wkdmSrc;
	if (((uint64_t)src) & 63) {
		                                                                                                               
		__nosan_memcpy(scratch, src, compressedSize);
		wkdmSrc = scratch;
	} else {
		wkdmSrc = src;
	}
	HIB_ASSERT((((uint64_t)wkdmSrc) & 63) == 0);
	HIB_ASSERT((((uint64_t)dst) & PAGE_MASK) == 0);
	struct {
		uint32_t reserved:12;
		uint32_t status:3;
		uint32_t reserved2:17;
		uint32_t popcnt:18;
		uint32_t reserved3:14;
	} result = { .status = ~0u };
	__asm__ volatile ("wkdmd %0, %1" : "=r"(result): "r"(dst), "0"(wkdmSrc));
	HIB_ASSERT(result.status == 0);
}

                                                          
#define ARM_LPAE_NSTABLE             0x8000000000000000ULL

#define TOP_LEVEL                    1
#define LAST_TABLE_LEVEL             3
#define PAGE_GRANULE_SHIFT           14
#define PAGE_GRANULE_SIZE            ((size_t)1<<PAGE_GRANULE_SHIFT)
#define PAGE_GRANULE_MASK            (PAGE_GRANULE_SIZE-1)
#define LEVEL_SHIFT(level)           (47 - (level * 11))

#define PTE_EMPTY(ent)               ((ent) == 0)

typedef struct {
	hibernate_page_list_t *bitmap;
	uint32_t nextFree;
	uint64_t page_table_base;
} map_ctx;

static void
hib_bzero(volatile void *s, size_t n)
{
	                                                                  
	while (n > sizeof(uint64_t)) {
		*(volatile uint64_t *)s = 0;
		s += sizeof(uint64_t);
		n -= sizeof(uint64_t);
	}
	while (n > sizeof(uint32_t)) {
		*(volatile uint32_t *)s = 0;
		s += sizeof(uint32_t);
		n -= sizeof(uint32_t);
	}
	while (n) {
		*(volatile char *)s = 0;
		s++;
		n--;
	}
}

static uint64_t
allocate_page(map_ctx *ctx)
{
	                                                                               
	                                                                                
	                                                                                    
	              
	ppnum_t ppnum = hibernate_page_list_grab(ctx->bitmap, &ctx->nextFree);
	hibernate_page_bitset(ctx->bitmap, FALSE, ppnum);
	uint64_t result = ptoa_64(ppnum);
	hib_bzero((void *)result, PAGE_SIZE);
	return result;
}

static void
create_map_entries(map_ctx *ctx, uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t map_flags)
{
	                                                                                 
	                                                                       
	HIB_ASSERT(!gHibTramp.memSlide);

	int level = TOP_LEVEL;
	volatile uint64_t *table_base = (uint64_t *)ctx->page_table_base;
	if (map_flags == 0) {
		paddr = 0;                                         
	}

	while (size) {
		HIB_ASSERT(level >= 1);
		HIB_ASSERT(level <= LAST_TABLE_LEVEL);

		size_t level_shift = LEVEL_SHIFT(level);
		size_t level_entries = PAGE_GRANULE_SIZE / sizeof(uint64_t);
		size_t level_size = 1ull << level_shift;
		size_t level_mask = level_size - 1;
		size_t index = (vaddr >> level_shift) & (level_entries - 1);
		                                                            
		                                                           
		                                         
		if ((level >= 2) &&
		    size >= level_size &&
		    ((vaddr | paddr) & level_mask) == 0) {
			                         
			size_t num_entries = MIN(size / level_size, level_entries - index);
			if (map_flags) {
				uint64_t entry = map_flags | ((level < LAST_TABLE_LEVEL) ? ARM_TTE_TYPE_BLOCK : ARM_TTE_TYPE_L3BLOCK);
				for (size_t i = 0; i < num_entries; i++) {
					HIB_ASSERT(PTE_EMPTY(table_base[index + i]));
					table_base[index + i] = entry | paddr;
					paddr += level_size;
				}
			} else {
				                                                    
				for (size_t i = 0; i < num_entries; i++) {
					HIB_ASSERT(PTE_EMPTY(table_base[index + i]));
				}
			}
			size_t mapped = num_entries * level_size;
			size -= mapped;
			if (size) {
				                                     
				level = TOP_LEVEL;
				table_base = (uint64_t *)ctx->page_table_base;
				vaddr += mapped;
				                                               
			}
		} else {
			                                      
			HIB_ASSERT(level < LAST_TABLE_LEVEL);
			uint64_t entry = table_base[index];
			HIB_ASSERT((entry & (ARM_TTE_VALID | ARM_TTE_TYPE_MASK)) != (ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK));                                        
			uint64_t sub_base = entry & ARM_TTE_TABLE_MASK;
			if (!sub_base) {
				sub_base = allocate_page(ctx);
				HIB_ASSERT((sub_base & PAGE_GRANULE_MASK) == 0);
				table_base[index] = sub_base | ARM_LPAE_NSTABLE | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID;
			}
			                         
			level++;
			table_base = (uint64_t *)sub_base;
		}
	}
}

static void
map_range_start_end(map_ctx *ctx, uint64_t start, uint64_t end, uint64_t slide, uint64_t flags)
{
	HIB_ASSERT(end >= start);
	create_map_entries(ctx, start + slide, start, end - start, flags);
}

#define MAP_FLAGS_COMMON (ARM_PTE_AF | ARM_PTE_NS | ARM_TTE_VALID | ARM_PTE_SH(SH_OUTER_MEMORY) | ARM_PTE_ATTRINDX(CACHE_ATTRINDX_WRITEBACK))
#define MAP_DEVICE       (ARM_PTE_AF | ARM_TTE_VALID | ARM_PTE_PNX | ARM_PTE_NX | ARM_PTE_SH(SH_NONE) | ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DISABLE))
#define MAP_RO           (MAP_FLAGS_COMMON | ARM_PTE_PNX | ARM_PTE_NX | ARM_PTE_AP(AP_RONA))
#define MAP_RW           (MAP_FLAGS_COMMON | ARM_PTE_PNX | ARM_PTE_NX)
#define MAP_RX           (MAP_FLAGS_COMMON | ARM_PTE_AP(AP_RONA))

static void
map_register_page(map_ctx *ctx, vm_address_t regPage)
{
	uint64_t regBase = trunc_page(regPage);
	if (regBase) {
		map_range_start_end(ctx, regBase, regBase + PAGE_SIZE, 0, MAP_DEVICE);
	}
}

static void
iterate_bitmaps(const map_ctx *ctx, bool (^callback)(const hibernate_bitmap_t *bank_bitmap))
{
	hibernate_bitmap_t *bank_bitmap = &ctx->bitmap->bank_bitmap[0];
	for (uint32_t bank = 0; bank < ctx->bitmap->bank_count; bank++) {
		if (!callback(bank_bitmap)) {
			return;
		}
		bank_bitmap = (hibernate_bitmap_t*)&bank_bitmap->bitmap[bank_bitmap->bitmapwords];
	}
}

                                                                                                                            
                                                                                                                         
                                                                                                                          
                                    
void
pal_hib_resume_tramp(uint32_t headerPpnum)
{
	uint64_t header_phys = ptoa_64(headerPpnum);
	IOHibernateImageHeader *header = (IOHibernateImageHeader *)header_phys;
	IOHibernateHibSegInfo *seg_info = &header->hibSegInfo;
	uint64_t hib_text_start = ptoa_64(header->restore1CodePhysPage);

	__block map_ctx ctx = {};
	uint64_t map_phys = header_phys
	    + (offsetof(IOHibernateImageHeader, fileExtentMap)
	    + header->fileExtentMapSize
	    + ptoa_32(header->restore1PageCount)
	    + header->previewSize);
	ctx.bitmap = (hibernate_page_list_t *)map_phys;

	                                     
	__block uint64_t phys_start = 0, phys_end = 0;
	iterate_bitmaps(&ctx, ^bool (const hibernate_bitmap_t *bank_bitmap) {
		if ((bank_bitmap->first_page <= header->restore1CodePhysPage) &&
		(bank_bitmap->last_page >= header->restore1CodePhysPage)) {
		        phys_start = ptoa_64(bank_bitmap->first_page);
		        phys_end = ptoa_64(bank_bitmap->last_page) + PAGE_SIZE;
		        return false;
		}
		return true;
	});

	HIB_ASSERT(phys_start != 0);
	HIB_ASSERT(phys_end != 0);

	hib_bzero(&gHibTramp, sizeof(gHibTramp));

	                                                                                                                                     
	                                                                                                                                    
	                                                                                                                       
	                                                                                                                                          
	                                                                                                                        
	uint16_t rand = (uint16_t)(((header->imageHeaderHMAC[0]) << 8) | header->imageHeaderHMAC[1]);
	uint64_t mem_slide = gHibernateGlobals.kernelSlide - (phys_end - phys_start) * 4 - rand * 256 * PAGE_SIZE;

	                                                                  
	hibernate_reserve_restore_pages(header_phys, header, ctx.bitmap);

	                
	hibernate_page_list_grab(ctx.bitmap, &ctx.nextFree);

	                  
	ctx.page_table_base = allocate_page(&ctx);
	gHibTramp.ttbr1 = ctx.page_table_base;

	uint64_t first_seg_start = 0, last_seg_end = 0, hib_text_end = 0;
	for (size_t i = 0; i < NUM_HIBSEGINFO_SEGMENTS; i++) {
		uint64_t size = ptoa_64(seg_info->segments[i].pageCount);
		if (size) {
			uint64_t seg_start = ptoa_64(seg_info->segments[i].physPage);
			uint64_t seg_end = seg_start + size;
			uint32_t protection = seg_info->segments[i].protection;
			if (protection != VM_PROT_NONE) {
				                                     
				HIB_ASSERT(seg_start >= phys_start);
				HIB_ASSERT(seg_end <= phys_end);

				if (!first_seg_start) {
					first_seg_start = seg_start;
				}
				if (last_seg_end) {
					                       
					map_range_start_end(&ctx, last_seg_end, seg_start, mem_slide, MAP_RW);
				}
				                                                                           
				bool executable = (protection & VM_PROT_EXECUTE);
				bool writeable = (protection & VM_PROT_WRITE);
				uint64_t map_flags = executable ? MAP_RX : writeable ? MAP_RW : MAP_RO;
				map_range_start_end(&ctx, seg_start, seg_end, gHibernateGlobals.kernelSlide, map_flags);
				last_seg_end = seg_end;
			}
			if (seg_info->segments[i].physPage == header->restore1CodePhysPage) {
				                                                         
				hib_text_end = seg_end;
			}
		}
	}
	                                                                                          
	map_range_start_end(&ctx, phys_start, first_seg_start, mem_slide, MAP_RW);
	map_range_start_end(&ctx, last_seg_end, phys_end, mem_slide, MAP_RW);

	                                                                  
	iterate_bitmaps(&ctx, ^bool (const hibernate_bitmap_t *bank_bitmap) {
		uint64_t bank_start = ptoa_64(bank_bitmap->first_page);
		uint64_t bank_end = ptoa_64(bank_bitmap->last_page) + PAGE_SIZE;
		if (bank_start == phys_start) {
		                                                           
		} else {
		                          
		        map_range_start_end(&ctx, bank_start, bank_end, mem_slide, MAP_RW);
		}
		return true;
	});

	                  
	ctx.page_table_base = allocate_page(&ctx);
	gHibTramp.ttbr0 = ctx.page_table_base;

	                                                                        
	map_range_start_end(&ctx, hib_text_start, hib_text_end, 0, MAP_RX);

	                            
	uint64_t image_start = trunc_page(header_phys);
	uint64_t image_end = round_page(header_phys + header->image1Size);
	map_range_start_end(&ctx, image_start, image_end, 0, MAP_RW);

	                                
	image_start = ptoa_64(header->handoffPages);
	image_end = image_start + ptoa_64(header->handoffPageCount);
	map_range_start_end(&ctx, image_start, image_end, 0, MAP_RO);

	                                 
	if (gHibernateGlobals.dockChannelRegBase) {
#define dockchannel_uart_base gHibernateGlobals.dockChannelRegBase
		vm_address_t dockChannelRegBase = trunc_page(&rDOCKCHANNELS_DEV_WSTAT(DOCKCHANNEL_UART_CHANNEL));
		map_register_page(&ctx, dockChannelRegBase);
	}
	map_register_page(&ctx, gHibernateGlobals.hibUartRegBase);
	map_register_page(&ctx, gHibernateGlobals.hmacRegBase);

	gHibTramp.memSlide = mem_slide;
}
