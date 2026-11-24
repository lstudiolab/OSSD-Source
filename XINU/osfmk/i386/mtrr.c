/*
 * Copyright (c) 2000-2011 Apple Computer, Inc. All rights reserved.
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

#include <mach/kern_return.h>
#include <kern/zalloc.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <i386/cpuid.h>
#include <i386/mp.h>
#include <i386/proc_reg.h>
#include <i386/mtrr.h>
#include <i386/machine_check.h>

struct mtrr_var_range {
	uint64_t  base;                                           
	uint64_t  mask;                                           
	uint32_t  refcnt;                                       
};

struct mtrr_fix_range {
	uint64_t  types;                                    
};

typedef struct mtrr_var_range mtrr_var_range_t;
typedef struct mtrr_fix_range mtrr_fix_range_t;

static struct {
	uint64_t            MTRRcap;
	uint64_t            MTRRdefType;
	mtrr_var_range_t *  var_range;
	unsigned int        var_count;
	mtrr_fix_range_t    fix_range[11];
} mtrr_state;

static boolean_t mtrr_initialized = FALSE;

decl_simple_lock_data(static, mtrr_lock);
#define MTRR_LOCK()     simple_lock(&mtrr_lock, LCK_GRP_NULL);
#define MTRR_UNLOCK()   simple_unlock(&mtrr_lock);

                      
#if     MTRR_DEBUG
#define DBG(x...)       kprintf(x)
#else
#define DBG(x...)
#endif

                       
static void mtrr_get_var_ranges(mtrr_var_range_t * range, int count);
static void mtrr_set_var_ranges(const mtrr_var_range_t * range, int count);
static void mtrr_get_fix_ranges(mtrr_fix_range_t * range);
static void mtrr_set_fix_ranges(const mtrr_fix_range_t * range);
static void mtrr_update_setup(void * param);
static void mtrr_update_teardown(void * param);
static void mtrr_update_action(void * param);
static void var_range_encode(mtrr_var_range_t * range, addr64_t address,
    uint64_t length, uint32_t type, int valid);
static int  var_range_overlap(mtrr_var_range_t * range, addr64_t address,
    uint64_t length, uint32_t type);

#define CACHE_CONTROL_MTRR              (NULL)
#define CACHE_CONTROL_PAT               ((void *)1)

  
                       
   
#define IA32_MTRR_DEF_TYPE_MT           0x000000ff
#define IA32_MTRR_DEF_TYPE_FE           0x00000400
#define IA32_MTRR_DEF_TYPE_E            0x00000800

#define IA32_MTRRCAP_VCNT               0x000000ff
#define IA32_MTRRCAP_FIX                0x00000100
#define IA32_MTRRCAP_WC                 0x00000400

                    
#define PHYS_BITS_TO_MASK(bits) \
	((((1ULL << (bits-1)) - 1) << 1) | 1)

  
                                                      
                                     
   
static uint64_t mtrr_phys_mask = PHYS_BITS_TO_MASK(36);

#define IA32_MTRR_PHYMASK_VALID         0x0000000000000800ULL
#define IA32_MTRR_PHYSBASE_MASK         (mtrr_phys_mask & ~0x0000000000000FFFULL)
#define IA32_MTRR_PHYSBASE_TYPE         0x00000000000000FFULL

  
                                                  
   
#define MASK_TO_LEN(mask) \
	((~((mask) & IA32_MTRR_PHYSBASE_MASK) & mtrr_phys_mask) + 1)

#define LEN_TO_MASK(len)  \
	(~((len) - 1) & IA32_MTRR_PHYSBASE_MASK)

#define LSB(x)          ((x) & (~((x) - 1)))

  
                                            
   
static void
mtrr_get_var_ranges(mtrr_var_range_t * range, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		range[i].base = rdmsr64(MSR_IA32_MTRR_PHYSBASE(i));
		range[i].mask = rdmsr64(MSR_IA32_MTRR_PHYSMASK(i));

		                                                   
		if (range[i].mask & IA32_MTRR_PHYMASK_VALID) {
			range[i].refcnt = 1;
		} else {
			range[i].refcnt = 0;
		}
	}
}

  
                                             
   
static void
mtrr_set_var_ranges(const mtrr_var_range_t * range, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		wrmsr64(MSR_IA32_MTRR_PHYSBASE(i), range[i].base);
		wrmsr64(MSR_IA32_MTRR_PHYSMASK(i), range[i].mask);
	}
}

  
                                                                      
   
static void
mtrr_get_fix_ranges(mtrr_fix_range_t * range)
{
	int i;

	                                   
	range[0].types = rdmsr64(MSR_IA32_MTRR_FIX64K_00000);
	range[1].types = rdmsr64(MSR_IA32_MTRR_FIX16K_80000);
	range[2].types = rdmsr64(MSR_IA32_MTRR_FIX16K_A0000);
	for (i = 0; i < 8; i++) {
		range[3 + i].types = rdmsr64(MSR_IA32_MTRR_FIX4K_C0000 + i);
	}
}

  
                                 
   
static void
mtrr_set_fix_ranges(const struct mtrr_fix_range * range)
{
	int i;

	                                   
	wrmsr64(MSR_IA32_MTRR_FIX64K_00000, range[0].types);
	wrmsr64(MSR_IA32_MTRR_FIX16K_80000, range[1].types);
	wrmsr64(MSR_IA32_MTRR_FIX16K_A0000, range[2].types);
	for (i = 0; i < 8; i++) {
		wrmsr64(MSR_IA32_MTRR_FIX4K_C0000 + i, range[3 + i].types);
	}
}

static boolean_t
mtrr_check_fix_ranges(const struct mtrr_fix_range * range)
{
	int             i;
	boolean_t       match = TRUE;

	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);

	                                   
	match = range[0].types == rdmsr64(MSR_IA32_MTRR_FIX64K_00000) &&
	    range[1].types == rdmsr64(MSR_IA32_MTRR_FIX16K_80000) &&
	    range[2].types == rdmsr64(MSR_IA32_MTRR_FIX16K_A0000);
	for (i = 0; match && i < 8; i++) {
		match = range[3 + i].types ==
		    rdmsr64(MSR_IA32_MTRR_FIX4K_C0000 + i);
	}

	return match;
}

static boolean_t
mtrr_check_var_ranges(mtrr_var_range_t * range, int count)
{
	int             i;
	boolean_t       match = TRUE;

	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);

	for (i = 0; match && i < count; i++) {
		match = range[i].base == rdmsr64(MSR_IA32_MTRR_PHYSBASE(i)) &&
		    range[i].mask == rdmsr64(MSR_IA32_MTRR_PHYSMASK(i));
	}

	return match;
}

#if MTRR_DEBUG
static void
mtrr_msr_dump(void)
{
	int i;
	int count = rdmsr64(MSR_IA32_MTRRCAP) & IA32_MTRRCAP_VCNT;

	DBG("VAR -- BASE -------------- MASK -------------- SIZE\n");
	for (i = 0; i < count; i++) {
		DBG(" %02x    0x%016llx  0x%016llx  0x%llx\n", i,
		    rdmsr64(MSR_IA32_MTRR_PHYSBASE(i)),
		    rdmsr64(MSR_IA32_MTRR_PHYSMASK(i)),
		    MASK_TO_LEN(rdmsr64(MSR_IA32_MTRR_PHYSMASK(i))));
	}
	DBG("\n");

	DBG("FIX64K_00000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX64K_00000));
	DBG("FIX16K_80000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX16K_80000));
	DBG("FIX16K_A0000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX16K_A0000));
	DBG(" FIX4K_C0000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_C0000));
	DBG(" FIX4K_C8000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_C8000));
	DBG(" FIX4K_D0000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_D0000));
	DBG(" FIX4K_D8000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_D8000));
	DBG(" FIX4K_E0000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_E0000));
	DBG(" FIX4K_E8000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_E8000));
	DBG(" FIX4K_F0000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_F0000));
	DBG(" FIX4K_F8000: 0x%016llx\n", rdmsr64(MSR_IA32_MTRR_FIX4K_F8000));

	DBG("\nMTRRcap = 0x%llx MTRRdefType = 0x%llx\n",
	    rdmsr64(MSR_IA32_MTRRCAP), rdmsr64(MSR_IA32_MTRR_DEF_TYPE));
}
#endif                 

  
                                                                         
                                                                         
                                                                  
   
void
mtrr_init(void)
{
	                                      
	if (mtrr_initialized == TRUE) {
		return;
	}

	                                                         
	if ((cpuid_features() & CPUID_FEATURE_MTRR) == 0) {
		return;                       
	}
	                                          
	bzero((void *)&mtrr_state, sizeof(mtrr_state));
	simple_lock_init(&mtrr_lock, 0);

	mtrr_state.MTRRcap     = rdmsr64(MSR_IA32_MTRRCAP);
	mtrr_state.MTRRdefType = rdmsr64(MSR_IA32_MTRR_DEF_TYPE);
	mtrr_state.var_count   = (unsigned int)(mtrr_state.MTRRcap & IA32_MTRRCAP_VCNT);

	                                                       
	if (mtrr_state.var_count) {
		mtrr_state.var_range = (mtrr_var_range_t *)
		    zalloc_permanent(sizeof(mtrr_var_range_t) *
		    mtrr_state.var_count, ZALIGN(mtrr_var_range_t));
		if (mtrr_state.var_range == NULL) {
			mtrr_state.var_count = 0;
		}
	}

	                                                           
	if (mtrr_state.var_count) {
		mtrr_get_var_ranges(mtrr_state.var_range,
		    mtrr_state.var_count);
	}

	                                                        
	if (mtrr_state.MTRRcap & IA32_MTRRCAP_FIX) {
		mtrr_get_fix_ranges(mtrr_state.fix_range);
	}

	mtrr_initialized = TRUE;

#if MTRR_DEBUG
	mtrr_msr_dump();                                    
#endif
}

  
                                                                 
                                                                 
                                                                 
                                                                 
   
static void
mtrr_update_action(void * cache_control_type)
{
	uintptr_t cr0, cr4;
	uintptr_t tmp;

	cr0 = get_cr0();
	cr4 = get_cr4();

	                              
	tmp = cr0 | CR0_CD;
	tmp &= ~CR0_NW;
	set_cr0(tmp);

	                  
	wbinvd();

	                               
	if (cr4 & CR4_PGE) {
		set_cr4(cr4 & ~CR4_PGE);
	} else {
		set_cr3_raw(get_cr3_raw());
	}

	if (CACHE_CONTROL_PAT == cache_control_type) {
		                                      
		uint64_t pat = rdmsr64(MSR_IA32_CR_PAT);
		DBG("CPU%d PAT: was 0x%016llx\n", get_cpu_number(), pat);
		  
                      
                                                                             
                                                                              
                                                                                     
                                                           
     
		pat &= ~(0xFFULL << 48);
		pat |=  (0x01ULL << 48);
		wrmsr64(MSR_IA32_CR_PAT, pat);
		DBG("CPU%d PAT: is  0x%016llx\n",
		    get_cpu_number(), rdmsr64(MSR_IA32_CR_PAT));
	} else {
		                             
		wrmsr64(MSR_IA32_MTRR_DEF_TYPE,
		    mtrr_state.MTRRdefType & ~IA32_MTRR_DEF_TYPE_E);

		                         
		if (mtrr_state.var_count) {
			mtrr_set_var_ranges(mtrr_state.var_range,
			    mtrr_state.var_count);
		}

		if (mtrr_state.MTRRcap & IA32_MTRRCAP_FIX) {
			mtrr_set_fix_ranges(mtrr_state.fix_range);
		}

		                                                              
		wrmsr64(MSR_IA32_MTRR_DEF_TYPE,
		    mtrr_state.MTRRdefType | IA32_MTRR_DEF_TYPE_E);
	}

	                                             
	wbinvd();
	set_cr3_raw(get_cr3_raw());
	                               
	set_cr0(cr0);

	                      
	if (cr4 & CR4_PGE) {
		set_cr4(cr4);
	}

	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);
}

static void
mtrr_update_setup(__unused void * param_not_used)
{
	                                                 
	current_cpu_datap()->cpu_iflag = ml_set_interrupts_enabled(FALSE);
	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);
}

static void
mtrr_update_teardown(__unused void * param_not_used)
{
	                                                   
	ml_set_interrupts_enabled(current_cpu_datap()->cpu_iflag);
	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);
}

  
                                          
   
kern_return_t
mtrr_update_all_cpus(void)
{
	if (mtrr_initialized == FALSE) {
		return KERN_NOT_SUPPORTED;
	}

	MTRR_LOCK();
	mp_rendezvous(mtrr_update_setup,
	    mtrr_update_action,
	    mtrr_update_teardown, NULL);
	MTRR_UNLOCK();

	return KERN_SUCCESS;
}

  
                                                                            
                                                                       
                                                   
   
kern_return_t
mtrr_update_cpu(void)
{
	boolean_t       match = TRUE;

	if (mtrr_initialized == FALSE) {
		return KERN_NOT_SUPPORTED;
	}

	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);

	MTRR_LOCK();

	                                      
	match = mtrr_state.MTRRdefType == rdmsr64(MSR_IA32_MTRR_DEF_TYPE);

	                                
	if (match) {
		match = mtrr_state.MTRRcap == rdmsr64(MSR_IA32_MTRRCAP);
	}

	                           
	if (match && mtrr_state.var_count) {
		match = mtrr_check_var_ranges(mtrr_state.var_range,
		    mtrr_state.var_count);
	}

	                        
	if (match && (mtrr_state.MTRRcap & IA32_MTRRCAP_FIX)) {
		match = mtrr_check_fix_ranges(mtrr_state.fix_range);
	}

#if MTRR_DEBUG
	if (!match) {
		mtrr_msr_dump();
	}
#endif
	if (!match) {
		DBG("mtrr_update_cpu() setting MTRR for cpu %d\n",
		    get_cpu_number());
		mtrr_update_action(NULL);
	}
#if MTRR_DEBUG
	if (!match) {
		mtrr_msr_dump();
	}
#endif

	MTRR_UNLOCK();

	return KERN_SUCCESS;
}

  
                                                                    
                                    
   
kern_return_t
mtrr_range_add(addr64_t address, uint64_t length, uint32_t type)
{
	mtrr_var_range_t * vr;
	mtrr_var_range_t * free_range;
	kern_return_t      ret = KERN_NO_SPACE;
	int                overlap;
	unsigned int       i;

	DBG("mtrr_range_add base = 0x%llx, size = 0x%llx, type = %d\n",
	    address, length, type);

	if (mtrr_initialized == FALSE) {
		return KERN_NOT_SUPPORTED;
	}

	                                                           
	if ((type != MTRR_TYPE_UNCACHEABLE) &&
	    (type != MTRR_TYPE_WRITECOMBINE) &&
	    (type != MTRR_TYPE_WRITETHROUGH) &&
	    (type != MTRR_TYPE_WRITEPROTECT) &&
	    (type != MTRR_TYPE_WRITEBACK)) {
		return KERN_INVALID_ARGUMENT;
	}

	                                   
	if ((type == MTRR_TYPE_WRITECOMBINE) &&
	    (mtrr_state.MTRRcap & IA32_MTRRCAP_WC) == 0) {
		return KERN_NOT_SUPPORTED;
	}

	                                              
	if (address < 0x100000 || mtrr_state.var_count == 0) {
		return KERN_NOT_SUPPORTED;
	}

	  
                                                            
                                                                  
    
	if ((length < 0x1000) ||
	    (LSB(length) != length) ||
	    (address && (length > LSB(address)))) {
		return KERN_INVALID_ARGUMENT;
	}

	MTRR_LOCK();

	  
                                              
    
	for (i = 0, free_range = NULL; i < mtrr_state.var_count; i++) {
		vr = &mtrr_state.var_range[i];

		if (vr->refcnt == 0) {
			                                                   
			free_range = vr;
			continue;
		}

		overlap = var_range_overlap(vr, address, length, type);
		if (overlap > 0) {
			  
                                                       
                                  
      
			free_range = vr;
			break;
		}
		if (overlap < 0) {
			                                             
			free_range = NULL;
			break;
		}
	}

	if (free_range) {
		if (free_range->refcnt++ == 0) {
			var_range_encode(free_range, address, length, type, 1);
			mp_rendezvous(mtrr_update_setup,
			    mtrr_update_action,
			    mtrr_update_teardown, NULL);
		}
		ret = KERN_SUCCESS;
	}

#if MTRR_DEBUG
	mtrr_msr_dump();
#endif

	MTRR_UNLOCK();

	return ret;
}

  
                                                                           
                                           
   
kern_return_t
mtrr_range_remove(addr64_t address, uint64_t length, uint32_t type)
{
	mtrr_var_range_t * vr;
	int                result = KERN_FAILURE;
	int                cpu_update = 0;
	unsigned int       i;

	DBG("mtrr_range_remove base = 0x%llx, size = 0x%llx, type = %d\n",
	    address, length, type);

	if (mtrr_initialized == FALSE) {
		return KERN_NOT_SUPPORTED;
	}

	MTRR_LOCK();

	for (i = 0; i < mtrr_state.var_count; i++) {
		vr = &mtrr_state.var_range[i];

		if (vr->refcnt &&
		    var_range_overlap(vr, address, length, type) > 0) {
			                                    
			if (--mtrr_state.var_range[i].refcnt == 0) {
				var_range_encode(vr, address, length, type, 0);
				cpu_update = 1;
			}
			result = KERN_SUCCESS;
			break;
		}
	}

	if (cpu_update) {
		mp_rendezvous(mtrr_update_setup,
		    mtrr_update_action,
		    mtrr_update_teardown, NULL);
		result = KERN_SUCCESS;
	}

#if MTRR_DEBUG
	mtrr_msr_dump();
#endif

	MTRR_UNLOCK();

	return result;
}

  
                                 
   
static void
var_range_encode(mtrr_var_range_t * range, addr64_t address,
    uint64_t length, uint32_t type, int valid)
{
	range->base = (address & IA32_MTRR_PHYSBASE_MASK) |
	    (type    & (uint32_t)IA32_MTRR_PHYSBASE_TYPE);

	range->mask = LEN_TO_MASK(length) |
	    (valid ? IA32_MTRR_PHYMASK_VALID : 0);
}

static int
var_range_overlap(mtrr_var_range_t * range, addr64_t address,
    uint64_t length, uint32_t type)
{
	uint64_t  v_address, v_length;
	uint32_t  v_type;
	int       result = 0;                                 

	v_address = range->base & IA32_MTRR_PHYSBASE_MASK;
	v_type    = (uint32_t)(range->base & IA32_MTRR_PHYSBASE_TYPE);
	v_length  = MASK_TO_LEN(range->mask);

	                          
	if ((v_address >= address && v_address < (address + length)) ||
	    (address >= v_address && address < (v_address + v_length))) {
		if (v_address == address && v_length == length && v_type == type) {
			result = 1;                           
		} else if (v_type == MTRR_TYPE_UNCACHEABLE &&
		    type == MTRR_TYPE_UNCACHEABLE) {
			                           
		} else if ((v_type == MTRR_TYPE_UNCACHEABLE &&
		    type == MTRR_TYPE_WRITEBACK) ||
		    (v_type == MTRR_TYPE_WRITEBACK &&
		    type == MTRR_TYPE_UNCACHEABLE)) {
			                                                   
		} else {
			                                                
			result = -1;
		}
	}

	return result;
}

  
                                        
   
void
pat_init(void)
{
	boolean_t       istate;
	uint64_t        pat;

	if (!(cpuid_features() & CPUID_FEATURE_PAT)) {
		return;
	}

	istate = ml_set_interrupts_enabled(FALSE);

	pat = rdmsr64(MSR_IA32_CR_PAT);
	DBG("CPU%d PAT: was 0x%016llx\n", get_cpu_number(), pat);

	                                                  
	if ((pat & (0x07ULL << 48)) != (0x01ULL << 48)) {
		mtrr_update_action(CACHE_CONTROL_PAT);
	}
	ml_set_interrupts_enabled(istate);
}
