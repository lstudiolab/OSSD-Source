/*
 * Copyright (c) 2016 Apple Computer, Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                                          
                                 
  
                                                                            
                                                                            
                                                                           
                                                                         
                                                                                
                                                                              
                                     
  
                                                                                
                                                                       
  
                                                                             
                                                                               
                                                                                
                                                                              
                                                                 
   

#include <kern/kalloc.h>
#include <kperf/action.h>
#include <kperf/buffer.h>
#include <kperf/context.h>
#include <kperf/kdebug_trigger.h>
#include <kperf/kperf.h>
#include <sys/errno.h>

boolean_t kperf_kdebug_active = FALSE;
static void kperf_kdebug_update(void);

static uint8_t kperf_kdebug_action = 0;

static struct kperf_kdebug_filter {
	uint64_t types[2];
	uint32_t debugids[KPERF_KDEBUG_DEBUGIDS_MAX];
	uint8_t n_debugids;
} __attribute__((packed)) *kperf_kdebug_filter = NULL;

enum kperf_kdebug_filter_type {
	KPERF_KDEBUG_FILTER_CLASS,
	KPERF_KDEBUG_FILTER_CLASS_FN,
	KPERF_KDEBUG_FILTER_CSC,
	KPERF_KDEBUG_FILTER_CSC_FN,
	KPERF_KDEBUG_FILTER_DEBUGID,
	KPERF_KDEBUG_FILTER_DEBUGID_FN
};

const static uint32_t debugid_masks[] = {
	[KPERF_KDEBUG_FILTER_CLASS] = KDBG_CLASS_MASK,
	[KPERF_KDEBUG_FILTER_CLASS_FN] = KDBG_CLASS_MASK | KDBG_FUNC_MASK,
	[KPERF_KDEBUG_FILTER_CSC] = KDBG_CSC_MASK,
	[KPERF_KDEBUG_FILTER_CSC_FN] = KDBG_CSC_MASK | KDBG_FUNC_MASK,
	[KPERF_KDEBUG_FILTER_DEBUGID] = KDBG_EVENTID_MASK,
	[KPERF_KDEBUG_FILTER_DEBUGID_FN] = UINT32_MAX,
};

  
                                                                            
                                                                        
             
   

            
#define DECODE_TYPE(TYPES, I) ((((uint8_t *)(TYPES))[(I) / 2] >> ((I) % 2) * 4) & 0xf)

void
kperf_kdebug_setup(void)
{
	kperf_kdebug_filter = kalloc_tag(sizeof(*kperf_kdebug_filter),
	    VM_KERN_MEMORY_DIAG);
	bzero(kperf_kdebug_filter, sizeof(*kperf_kdebug_filter));
}

void
kperf_kdebug_reset(void)
{
	kperf_setup();

	kperf_kdebug_action = 0;
	bzero(kperf_kdebug_filter, sizeof(*kperf_kdebug_filter));
	kperf_kdebug_update();
}

boolean_t
kperf_kdebug_should_trigger(uint32_t debugid)
{
	                         
	if (KDBG_EXTRACT_CLASS(debugid) == DBG_PERF) {
		return FALSE;
	}

	  
                                                                      
                                                                        
                                                                     
    
	for (uint8_t i = 0; i < kperf_kdebug_filter->n_debugids; i++) {
		uint32_t check_debugid =
		    kperf_kdebug_filter->debugids[i];
		uint32_t mask = debugid_masks[DECODE_TYPE(kperf_kdebug_filter->types, i)];

		if ((debugid & mask) == check_debugid) {
			return TRUE;
		}
	}

	return FALSE;
}

int
kperf_kdebug_set_filter(user_addr_t user_filter, uint32_t user_size)
{
	uint32_t n_debugids_provided = 0;
	int err = 0;

	kperf_setup();

	n_debugids_provided = (uint32_t)KPERF_KDEBUG_N_DEBUGIDS(user_size);

	                                            
	if (n_debugids_provided == 0) {
		bzero(kperf_kdebug_filter, sizeof(*kperf_kdebug_filter));
		goto out;
	}

	if ((err = kperf_kdebug_set_n_debugids(n_debugids_provided))) {
		goto out;
	}

	if ((err = copyin(user_filter, (char *)kperf_kdebug_filter,
	    KPERF_KDEBUG_FILTER_SIZE(n_debugids_provided)))) {
		bzero(kperf_kdebug_filter, sizeof(*kperf_kdebug_filter));
		goto out;
	}

out:
	kperf_kdebug_update();

	return err;
}

uint32_t
kperf_kdebug_get_filter(struct kperf_kdebug_filter **filter)
{
	kperf_setup();

	assert(filter != NULL);

	*filter = kperf_kdebug_filter;
	return kperf_kdebug_filter->n_debugids;
}

int
kperf_kdebug_set_n_debugids(uint32_t n_debugids_in)
{
	kperf_setup();

	if (n_debugids_in > KPERF_KDEBUG_DEBUGIDS_MAX) {
		return EINVAL;
	}

	kperf_kdebug_filter->n_debugids = n_debugids_in;

	return 0;
}

int
kperf_kdebug_set_action(int action_id)
{
	if (action_id < 0 || (unsigned int)action_id > kperf_action_get_count()) {
		return EINVAL;
	}

	kperf_kdebug_action = action_id;
	kperf_kdebug_update();

	return 0;
}

int
kperf_kdebug_get_action(void)
{
	return kperf_kdebug_action;
}

static void
kperf_kdebug_update(void)
{
	kperf_setup();

	if (kperf_kdebug_action != 0 &&
	    kperf_kdebug_filter->n_debugids != 0) {
		kperf_kdebug_active = TRUE;
	} else {
		kperf_kdebug_active = FALSE;
	}
}
