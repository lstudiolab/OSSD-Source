/*
 * Copyright (c) 2005-2006 Apple Computer, Inc. All rights reserved.
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

#include <kern/thread.h>

#include <sys/time.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <machine/atomic.h>
#include <libkern/OSKextLibPrivate.h>
#include <kern/kern_types.h>
#include <kern/timer_call.h>
#include <kern/thread_call.h>
#include <kern/task.h>
#include <kern/sched_prim.h>
#include <miscfs/devfs/devfs.h>
#include <kern/kalloc.h>

#include <mach/vm_param.h>
#include <mach/mach_vm.h>
#include <mach/task.h>
#include <vm/vm_map.h>                                                                        

  
           
   
                                                                       
#define proc_t struct proc                                                   

KALLOC_HEAP_DEFINE(KHEAP_DTRACE, "dtrace", KHEAP_ID_DEFAULT);

void
dtrace_sprlock(proc_t *p)
{
	lck_mtx_lock(&p->p_dtrace_sprlock);
}

void
dtrace_sprunlock(proc_t *p)
{
	lck_mtx_unlock(&p->p_dtrace_sprlock);
}

                                   
proc_t *
sprlock(pid_t pid)
{
	proc_t* p;

	if ((p = proc_find(pid)) == PROC_NULL) {
		return PROC_NULL;
	}

	task_suspend_internal(p->task);

	dtrace_sprlock(p);

	return p;
}

                                   
void
sprunlock(proc_t *p)
{
	if (p != PROC_NULL) {
		dtrace_sprunlock(p);

		task_resume_internal(p->task);

		proc_rele(p);
	}
}

  
               
   

                                        
extern kern_return_t vm_map_read_user(vm_map_t map, vm_map_address_t src_addr, void *dst_p, vm_size_t size);
extern kern_return_t vm_map_write_user(vm_map_t map, void *src_p, vm_map_address_t dst_addr, vm_size_t size);

                                   
int
uread(proc_t *p, void *buf, user_size_t len, user_addr_t a)
{
	kern_return_t ret;

	ASSERT(p != PROC_NULL);
	ASSERT(p->task != NULL);

	task_t task = p->task;

	  
                                                      
                                           
   
                                                              
                                                           
                                     
    
	vm_map_t map = get_task_map_reference(task);
	if (map) {
		ret = vm_map_read_user( map, (vm_map_address_t)a, buf, (vm_size_t)len);
		vm_map_deallocate(map);
	} else {
		ret = KERN_TERMINATED;
	}

	return (int)ret;
}


                                   
int
uwrite(proc_t *p, void *buf, user_size_t len, user_addr_t a)
{
	kern_return_t ret;

	ASSERT(p != NULL);
	ASSERT(p->task != NULL);

	task_t task = p->task;

	  
                                                      
                                           
   
                                                              
                                                           
                                     
    
	vm_map_t map = get_task_map_reference(task);
	if (map) {
		                                  
		uint32_t nestingDepth = 999999;
		vm_region_submap_short_info_data_64_t info;
		mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
		mach_vm_address_t address = (mach_vm_address_t)a;
		mach_vm_size_t sizeOfRegion = (mach_vm_size_t)len;

		ret = mach_vm_region_recurse(map, &address, &sizeOfRegion, &nestingDepth, (vm_region_recurse_info_t)&info, &count);
		if (ret != KERN_SUCCESS) {
			goto done;
		}

		vm_prot_t reprotect;

		if (!(info.protection & VM_PROT_WRITE)) {
			                                                               
			reprotect = info.protection;

			if (info.max_protection & VM_PROT_WRITE) {
				                                                                     
				ret = mach_vm_protect(map, (mach_vm_offset_t)a, (mach_vm_size_t)len, 0, (reprotect & ~VM_PROT_EXECUTE) | VM_PROT_WRITE);
			} else {
				  
                                                                                                     
      
                                                                         
       
				ret = mach_vm_protect(map, (mach_vm_offset_t)a, (mach_vm_size_t)len, 0, VM_PROT_COPY | VM_PROT_READ | VM_PROT_WRITE);
			}

			if (ret != KERN_SUCCESS) {
				goto done;
			}
		} else {
			                                      
			reprotect = VM_PROT_NONE;
		}

		ret = vm_map_write_user( map,
		    buf,
		    (vm_map_address_t)a,
		    (vm_size_t)len);

		dtrace_flush_caches();

		if (ret != KERN_SUCCESS) {
			goto done;
		}

		if (reprotect != VM_PROT_NONE) {
			ASSERT(reprotect & VM_PROT_EXECUTE);
			ret = mach_vm_protect(map, (mach_vm_offset_t)a, (mach_vm_size_t)len, 0, reprotect);
		}

done:
		vm_map_deallocate(map);
	} else {
		ret = KERN_TERMINATED;
	}

	return (int)ret;
}

  
         
   
LCK_MTX_DECLARE_ATTR(cpu_lock, &dtrace_lck_grp, &dtrace_lck_attr);
LCK_MTX_DECLARE_ATTR(cyc_lock, &dtrace_lck_grp, &dtrace_lck_attr);
LCK_MTX_DECLARE_ATTR(mod_lock, &dtrace_lck_grp, &dtrace_lck_attr);

dtrace_cpu_t *cpu_list;
cpu_core_t *cpu_core;                        

  
         
   

  
                                                                                               
                                                                                                  
   
cred_t *
dtrace_CRED(void)
{
	struct uthread *uthread = get_bsdthread_info(current_thread());

	if (uthread == NULL) {
		return NULL;
	} else {
		return uthread->uu_ucred;                                                 
	}
}

int
PRIV_POLICY_CHOICE(void* cred, int priv, int all)
{
#pragma unused(priv, all)
	return kauth_cred_issuser(cred);                                                             
}

int
PRIV_POLICY_ONLY(void *cr, int priv, int boolean)
{
#pragma unused(priv, boolean)
	return kauth_cred_issuser(cr);                                         
}

uid_t
crgetuid(const cred_t *cr)
{
	cred_t copy_cr = *cr; return kauth_cred_getuid(&copy_cr);
}

  
           
   

typedef struct wrap_timer_call {
	                     
	cyc_handler_t           hdlr;
	cyc_time_t              when;
	uint64_t                deadline;
	int                     cpuid;
	boolean_t               suspended;
	struct timer_call       call;

	                                  
	LIST_ENTRY(wrap_timer_call) entries;
} wrap_timer_call_t;

#define WAKEUP_REAPER           0x7FFFFFFFFFFFFFFFLL
#define NEARLY_FOREVER          0x7FFFFFFFFFFFFFFELL


typedef struct cyc_list {
	cyc_omni_handler_t cyl_omni;
	wrap_timer_call_t cyl_wrap_by_cpus[];
#if __arm__ && (__BIGGEST_ALIGNMENT__ > 4)
} __attribute__ ((aligned(8))) cyc_list_t;
#else
} cyc_list_t;
#endif

                                            
void (*dtrace_cpu_state_changed_hook)(int, boolean_t) = NULL;
void dtrace_cpu_state_changed(int, boolean_t);

void
dtrace_install_cpu_hooks(void)
{
	dtrace_cpu_state_changed_hook = dtrace_cpu_state_changed;
}

void
dtrace_cpu_state_changed(int cpuid, boolean_t is_running)
{
#pragma unused(cpuid)
	wrap_timer_call_t       *wrapTC = NULL;
	boolean_t               suspend = (is_running ? FALSE : TRUE);
	dtrace_icookie_t        s;

	                                                  
	s = dtrace_interrupt_disable();
	assert(cpuid == cpu_number());

	LIST_FOREACH(wrapTC, &(cpu_list[cpu_number()].cpu_cyc_list), entries) {
		assert(wrapTC->cpuid == cpu_number());
		if (suspend) {
			assert(!wrapTC->suspended);
			                                                              
			if (!timer_call_cancel(&wrapTC->call)) {
				panic("timer_call_set_suspend() failed to cancel a timer call");
			}
			wrapTC->suspended = TRUE;
		} else {
			                                                         
			assert(wrapTC->suspended);
			clock_deadline_for_periodic_event(wrapTC->when.cyt_interval, mach_absolute_time(),
			    &wrapTC->deadline);
			timer_call_enter1(&wrapTC->call, (void*) wrapTC, wrapTC->deadline,
			    TIMER_CALL_SYS_CRITICAL | TIMER_CALL_LOCAL);
			wrapTC->suspended = FALSE;
		}
	}

	                                           
	dtrace_interrupt_enable(s);
}

static void
_timer_call_apply_cyclic( void *ignore, void *vTChdl )
{
#pragma unused(ignore)
	wrap_timer_call_t *wrapTC = (wrap_timer_call_t *)vTChdl;

	(*(wrapTC->hdlr.cyh_func))( wrapTC->hdlr.cyh_arg );

	clock_deadline_for_periodic_event( wrapTC->when.cyt_interval, mach_absolute_time(), &(wrapTC->deadline));
	timer_call_enter1( &(wrapTC->call), (void *)wrapTC, wrapTC->deadline, TIMER_CALL_SYS_CRITICAL | TIMER_CALL_LOCAL );
}

static cyclic_id_t
timer_call_add_cyclic(wrap_timer_call_t *wrapTC, cyc_handler_t *handler, cyc_time_t *when)
{
	uint64_t now;
	dtrace_icookie_t s;

	timer_call_setup( &(wrapTC->call), _timer_call_apply_cyclic, NULL );
	wrapTC->hdlr = *handler;
	wrapTC->when = *when;

	nanoseconds_to_absolutetime( wrapTC->when.cyt_interval, (uint64_t *)&wrapTC->when.cyt_interval );

	now = mach_absolute_time();
	wrapTC->deadline = now;

	clock_deadline_for_periodic_event( wrapTC->when.cyt_interval, now, &(wrapTC->deadline));

	                                                                                   
	s = dtrace_interrupt_disable();
	wrapTC->cpuid = cpu_number();
	LIST_INSERT_HEAD(&cpu_list[wrapTC->cpuid].cpu_cyc_list, wrapTC, entries);
	timer_call_enter1(&wrapTC->call, (void*) wrapTC, wrapTC->deadline,
	    TIMER_CALL_SYS_CRITICAL | TIMER_CALL_LOCAL);
	wrapTC->suspended = FALSE;
	dtrace_interrupt_enable(s);

	return (cyclic_id_t)wrapTC;
}

  
                                               
   
static void
timer_call_remove_cyclic(wrap_timer_call_t *wrapTC)
{
	assert(wrapTC);
	assert(cpu_number() == wrapTC->cpuid);

	if (!timer_call_cancel(&wrapTC->call)) {
		panic("timer_call_remove_cyclic() failed to cancel a timer call");
	}

	LIST_REMOVE(wrapTC, entries);
}

static void *
timer_call_get_cyclic_arg(wrap_timer_call_t *wrapTC)
{
	return wrapTC ? wrapTC->hdlr.cyh_arg : NULL;
}

cyclic_id_t
cyclic_timer_add(cyc_handler_t *handler, cyc_time_t *when)
{
	wrap_timer_call_t *wrapTC = _MALLOC(sizeof(wrap_timer_call_t), M_TEMP, M_ZERO | M_WAITOK);
	if (NULL == wrapTC) {
		return CYCLIC_NONE;
	} else {
		return timer_call_add_cyclic( wrapTC, handler, when );
	}
}

void
cyclic_timer_remove(cyclic_id_t cyclic)
{
	ASSERT( cyclic != CYCLIC_NONE );

	                                                                            
	wrap_timer_call_t *wrapTC = (wrap_timer_call_t *) cyclic;
	dtrace_xcall(wrapTC->cpuid, (dtrace_xcall_t) timer_call_remove_cyclic, (void*) cyclic);

	_FREE((void *)cyclic, M_TEMP);
}

static void
_cyclic_add_omni(cyc_list_t *cyc_list)
{
	cyc_time_t cT;
	cyc_handler_t cH;
	cyc_omni_handler_t *omni = &cyc_list->cyl_omni;

	(omni->cyo_online)(omni->cyo_arg, CPU, &cH, &cT);

	wrap_timer_call_t *wrapTC = &cyc_list->cyl_wrap_by_cpus[cpu_number()];
	timer_call_add_cyclic(wrapTC, &cH, &cT);
}

cyclic_id_list_t
cyclic_add_omni(cyc_omni_handler_t *omni)
{
	cyc_list_t *cyc_list =
	    _MALLOC(sizeof(cyc_list_t) + NCPU * sizeof(wrap_timer_call_t), M_TEMP, M_ZERO | M_WAITOK);

	if (NULL == cyc_list) {
		return NULL;
	}

	cyc_list->cyl_omni = *omni;

	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)_cyclic_add_omni, (void *)cyc_list);

	return (cyclic_id_list_t)cyc_list;
}

static void
_cyclic_remove_omni(cyc_list_t *cyc_list)
{
	cyc_omni_handler_t *omni = &cyc_list->cyl_omni;
	void *oarg;
	wrap_timer_call_t *wrapTC;

	  
                                                                         
                                
    
	if ((wrapTC = &cyc_list->cyl_wrap_by_cpus[cpu_number()]) != NULL) {
		oarg = timer_call_get_cyclic_arg(wrapTC);
		timer_call_remove_cyclic(wrapTC);
		(omni->cyo_offline)(omni->cyo_arg, CPU, oarg);
	}
}

void
cyclic_remove_omni(cyclic_id_list_t cyc_list)
{
	ASSERT(cyc_list != NULL);

	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)_cyclic_remove_omni, (void *)cyc_list);
	_FREE(cyc_list, M_TEMP);
}

typedef struct wrap_thread_call {
	thread_call_t TChdl;
	cyc_handler_t hdlr;
	cyc_time_t when;
	uint64_t deadline;
} wrap_thread_call_t;

  
                                                                             
                                                                                       
   
static void
_cyclic_apply( void *ignore, void *vTChdl )
{
#pragma unused(ignore)
	wrap_thread_call_t *wrapTC = (wrap_thread_call_t *)vTChdl;

	(*(wrapTC->hdlr.cyh_func))( wrapTC->hdlr.cyh_arg );

	clock_deadline_for_periodic_event( wrapTC->when.cyt_interval, mach_absolute_time(), &(wrapTC->deadline));
	(void)thread_call_enter1_delayed( wrapTC->TChdl, (void *)wrapTC, wrapTC->deadline );

	                                                                                 
	if (wrapTC->when.cyt_interval == WAKEUP_REAPER) {
		thread_wakeup((event_t)wrapTC);
	}
}

cyclic_id_t
cyclic_add(cyc_handler_t *handler, cyc_time_t *when)
{
	uint64_t now;

	wrap_thread_call_t *wrapTC = _MALLOC(sizeof(wrap_thread_call_t), M_TEMP, M_ZERO | M_WAITOK);
	if (NULL == wrapTC) {
		return CYCLIC_NONE;
	}

	wrapTC->TChdl = thread_call_allocate( _cyclic_apply, NULL );
	wrapTC->hdlr = *handler;
	wrapTC->when = *when;

	ASSERT(when->cyt_when == 0);
	ASSERT(when->cyt_interval < WAKEUP_REAPER);

	nanoseconds_to_absolutetime(wrapTC->when.cyt_interval, (uint64_t *)&wrapTC->when.cyt_interval);

	now = mach_absolute_time();
	wrapTC->deadline = now;

	clock_deadline_for_periodic_event( wrapTC->when.cyt_interval, now, &(wrapTC->deadline));
	(void)thread_call_enter1_delayed( wrapTC->TChdl, (void *)wrapTC, wrapTC->deadline );

	return (cyclic_id_t)wrapTC;
}

static void
noop_cyh_func(void * ignore)
{
#pragma unused(ignore)
}

void
cyclic_remove(cyclic_id_t cyclic)
{
	wrap_thread_call_t *wrapTC = (wrap_thread_call_t *)cyclic;

	ASSERT(cyclic != CYCLIC_NONE);

	while (!thread_call_cancel(wrapTC->TChdl)) {
		int ret = assert_wait(wrapTC, THREAD_UNINT);
		ASSERT(ret == THREAD_WAITING);

		wrapTC->when.cyt_interval = WAKEUP_REAPER;

		ret = thread_block(THREAD_CONTINUE_NULL);
		ASSERT(ret == THREAD_AWAKENED);
	}

	if (thread_call_free(wrapTC->TChdl)) {
		_FREE(wrapTC, M_TEMP);
	} else {
		                                     
		wrapTC->hdlr.cyh_func = noop_cyh_func;
		wrapTC->when.cyt_interval = NEARLY_FOREVER;
	}
}

int
ddi_driver_major(dev_info_t     *devi)
{
	return (int)major(CAST_DOWN_EXPLICIT(int, devi));
}

int
ddi_create_minor_node(dev_info_t *dip, const char *name, int spec_type,
    minor_t minor_num, const char *node_type, int flag)
{
#pragma unused(spec_type,node_type,flag)
	dev_t dev = makedev( ddi_driver_major(dip), minor_num );

	if (NULL == devfs_make_node( dev, DEVFS_CHAR, UID_ROOT, GID_WHEEL, 0666, name, 0 )) {
		return DDI_FAILURE;
	} else {
		return DDI_SUCCESS;
	}
}

void
ddi_remove_minor_node(dev_info_t *dip, char *name)
{
#pragma unused(dip,name)
                                                           
}

major_t
getemajor( dev_t d )
{
	return (major_t) major(d);
}

minor_t
getminor( dev_t d )
{
	return (minor_t) minor(d);
}

extern void Debugger(const char*);

void
debug_enter(char *c)
{
	Debugger(c);
}

  
       
   

void *
dt_kmem_alloc_site(size_t size, int kmflag, vm_allocation_site_t *site)
{
#pragma unused(kmflag)

  
                                                                 
                                                            
   
	return kalloc_ext(KHEAP_DTRACE, size, Z_WAITOK, site).addr;
}

void *
dt_kmem_zalloc_site(size_t size, int kmflag, vm_allocation_site_t *site)
{
#pragma unused(kmflag)

  
                                                                 
                                                            
   
	return kalloc_ext(KHEAP_DTRACE, size, Z_WAITOK | Z_ZERO, site).addr;
}

void
dt_kmem_free(void *buf, size_t size)
{
	kheap_free(KHEAP_DTRACE, buf, size);
}



  
                            
                                 
   

void*
dt_kmem_alloc_aligned_site(size_t size, size_t align, int kmflag, vm_allocation_site_t *site)
{
	void *mem, **addr_to_free;
	intptr_t mem_aligned;
	size_t *size_to_free, hdr_size;

	                             
	assert(align != 0);
	assert((align & (align - 1)) == 0);

	  
                                                               
                                                         
    
	hdr_size = sizeof(size_t) + sizeof(void*);
	mem = dt_kmem_alloc_site(size + align + hdr_size, kmflag, site);
	if (mem == NULL) {
		return NULL;
	}

	mem_aligned = (intptr_t) (((intptr_t) mem + align + hdr_size) & ~(align - 1));

	                                              
	addr_to_free = (void**) (mem_aligned - sizeof(void*));
	*addr_to_free = mem;

	                                           
	size_to_free = (size_t*) (mem_aligned - hdr_size);
	*size_to_free = size + align + hdr_size;

	return (void*) mem_aligned;
}

void*
dt_kmem_zalloc_aligned_site(size_t size, size_t align, int kmflag, vm_allocation_site_t *s)
{
	void* buf;

	buf = dt_kmem_alloc_aligned_site(size, align, kmflag, s);

	if (!buf) {
		return NULL;
	}

	bzero(buf, size);

	return buf;
}

void
dt_kmem_free_aligned(void* buf, size_t size)
{
#pragma unused(size)
	intptr_t ptr = (intptr_t) buf;
	void **addr_to_free = (void**) (ptr - sizeof(void*));
	size_t *size_to_free = (size_t*) (ptr - (sizeof(size_t) + sizeof(void*)));

	if (buf == NULL) {
		return;
	}

	dt_kmem_free(*addr_to_free, *size_to_free);
}

  
                                                                                
                                                               
                                                                             
                                                                                   
   
kmem_cache_t *
kmem_cache_create(
	const char *name,                                            
	size_t bufsize,                                             
	size_t align,                                          
	int (*constructor)(void *, void *, int),                         
	void (*destructor)(void *, void *),                        
	void (*reclaim)(void *),                              
	void *private,                                                      
	vmem_t *vmp,                                                 
	int cflags)                               
{
#pragma unused(name,align,constructor,destructor,reclaim,private,vmp,cflags)
	return (kmem_cache_t *)bufsize;                                                   
}

void *
kmem_cache_alloc(kmem_cache_t *cp, int kmflag)
{
#pragma unused(kmflag)
	size_t bufsize = (size_t)cp;
	return (void *)_MALLOC(bufsize, M_TEMP, M_WAITOK);
}

void
kmem_cache_free(kmem_cache_t *cp, void *buf)
{
#pragma unused(cp)
	_FREE(buf, M_TEMP);
}

void
kmem_cache_destroy(kmem_cache_t *cp)
{
#pragma unused(cp)
}

  
                                                                                 
   
typedef unsigned int u_daddr_t;
#include "blist.h"

                                                                                       
struct blist_hdl {
	blist_t blist;
};

vmem_t *
vmem_create(const char *name, void *base, size_t size, size_t quantum, void *ignore5,
    void *ignore6, vmem_t *source, size_t qcache_max, int vmflag)
{
#pragma unused(name,quantum,ignore5,ignore6,source,qcache_max,vmflag)
	blist_t bl;
	struct blist_hdl *p = _MALLOC(sizeof(struct blist_hdl), M_TEMP, M_WAITOK);

	ASSERT(quantum == 1);
	ASSERT(NULL == ignore5);
	ASSERT(NULL == ignore6);
	ASSERT(NULL == source);
	ASSERT(0 == qcache_max);
	ASSERT(size <= INT32_MAX);
	ASSERT(vmflag & VMC_IDENTIFIER);

	size = MIN(128, size);                                                                                   

	p->blist = bl = blist_create((daddr_t)size);
	blist_free(bl, 0, (daddr_t)size);
	if (base) {
		blist_alloc( bl, (daddr_t)(uintptr_t)base );                                
	}
	return (vmem_t *)p;
}

void *
vmem_alloc(vmem_t *vmp, size_t size, int vmflag)
{
#pragma unused(vmflag)
	struct blist_hdl *q = (struct blist_hdl *)vmp;
	blist_t bl = q->blist;
	daddr_t p;

	p = blist_alloc(bl, (daddr_t)size);

	if (p == SWAPBLK_NONE) {
		blist_resize(&bl, (bl->bl_blocks) << 1, 1);
		q->blist = bl;
		p = blist_alloc(bl, (daddr_t)size);
		if (p == SWAPBLK_NONE) {
			panic("vmem_alloc: failure after blist_resize!");
		}
	}

	return (void *)(uintptr_t)p;
}

void
vmem_free(vmem_t *vmp, void *vaddr, size_t size)
{
	struct blist_hdl *p = (struct blist_hdl *)vmp;

	blist_free( p->blist, (daddr_t)(uintptr_t)vaddr, (daddr_t)size );
}

void
vmem_destroy(vmem_t *vmp)
{
	struct blist_hdl *p = (struct blist_hdl *)vmp;

	blist_destroy( p->blist );
	_FREE( p, sizeof(struct blist_hdl));
}

  
         
   

  
                                                                                 
                                                                                       
   

hrtime_t
dtrace_gethrestime(void)
{
	clock_sec_t             secs;
	clock_nsec_t    nanosecs;
	uint64_t                secs64, ns64;

	clock_get_calendar_nanotime_nowait(&secs, &nanosecs);
	secs64 = (uint64_t)secs;
	ns64 = (uint64_t)nanosecs;

	ns64 = ns64 + (secs64 * 1000000000LL);
	return ns64;
}

  
                                                                                        
                                                 
   

hrtime_t
dtrace_abs_to_nano(uint64_t elapsed)
{
	static mach_timebase_info_data_t    sTimebaseInfo = { 0, 0 };

	  
                                                          
                                                           
                                                          
                              
    

	if (sTimebaseInfo.denom == 0) {
		(void) clock_timebase_info(&sTimebaseInfo);
	}

	  
                           
                                                                                   
   
                                                                                     
                                                      
    
	if (sTimebaseInfo.denom == sTimebaseInfo.numer) {
		return elapsed;
	} else if (sTimebaseInfo.denom == 1) {
		return elapsed * (uint64_t)sTimebaseInfo.numer;
	} else {
		                                               
		uint64_t eta32 = elapsed >> 32;
		uint64_t eps32 = elapsed & 0x00000000ffffffffLL;

		uint32_t numer = sTimebaseInfo.numer, denom = sTimebaseInfo.denom;

		                                                       
		uint64_t mu64 = numer * eta32;
		uint64_t lambda64 = numer * eps32;

		                                       
		uint64_t q32 = mu64 / denom;
		uint64_t r32 = mu64 - (q32 * denom);                   

		return (q32 << 32) + ((r32 << 32) + lambda64) / denom;
	}
}

hrtime_t
dtrace_gethrtime(void)
{
	static uint64_t        start = 0;

	if (start == 0) {
		start = mach_absolute_time();
	}

	return dtrace_abs_to_nano(mach_absolute_time() - start);
}

  
                                
   
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{
	if (OSCompareAndSwap((UInt32)cmp, (UInt32)new, (volatile UInt32 *)target )) {
		return cmp;
	} else {
		return ~cmp;                                             
	}
}

void *
dtrace_casptr(void *target, void *cmp, void *new)
{
	if (OSCompareAndSwapPtr( cmp, new, (void**)target )) {
		return cmp;
	} else {
		return (void *)(~(uintptr_t)cmp);                                             
	}
}

  
                         
   
dtrace_icookie_t
dtrace_interrupt_disable(void)
{
	return (dtrace_icookie_t)ml_set_interrupts_enabled(FALSE);
}

void
dtrace_interrupt_enable(dtrace_icookie_t reenable)
{
	(void)ml_set_interrupts_enabled((boolean_t)reenable);
}

  
                  
   
static void
dtrace_sync_func(void)
{
}

  
                                                  
   
void
dtrace_sync(void)
{
	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
}

  
                                                                                            
   

extern kern_return_t dtrace_copyio_preflight(addr64_t);
extern kern_return_t dtrace_copyio_postflight(addr64_t);

static int
dtrace_copycheck(user_addr_t uaddr, uintptr_t kaddr, size_t size)
{
#pragma unused(kaddr)

	vm_offset_t recover = dtrace_set_thread_recover( current_thread(), 0 );                                       
	dtrace_set_thread_recover( current_thread(), recover );                                                         

	ASSERT(kaddr + size >= kaddr);

	if (uaddr + size < uaddr ||                                      
	    KERN_FAILURE == dtrace_copyio_preflight(uaddr)) {                                            
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		return 0;
	}
	return 1;
}

void
dtrace_copyin(user_addr_t src, uintptr_t dst, size_t len, volatile uint16_t *flags)
{
#pragma unused(flags)

	if (dtrace_copycheck( src, dst, len )) {
		if (copyin((const user_addr_t)src, (char *)dst, (vm_size_t)len)) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = src;
		}
		dtrace_copyio_postflight(src);
	}
}

void
dtrace_copyinstr(user_addr_t src, uintptr_t dst, size_t len, volatile uint16_t *flags)
{
#pragma unused(flags)

	size_t actual;

	if (dtrace_copycheck( src, dst, len )) {
		                                     
		int error = copyinstr((const user_addr_t)src, (char *)dst, (vm_size_t)len, &actual);

		  
                                                                                             
                                                                                        
                                                                                          
                   
     
		if (error && error != ENAMETOOLONG) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = src;
		}
		dtrace_copyio_postflight(src);
	}
}

void
dtrace_copyout(uintptr_t src, user_addr_t dst, size_t len, volatile uint16_t *flags)
{
#pragma unused(flags)

	if (dtrace_copycheck( dst, src, len )) {
		if (copyout((const void *)src, dst, (vm_size_t)len)) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = dst;
		}
		dtrace_copyio_postflight(dst);
	}
}

void
dtrace_copyoutstr(uintptr_t src, user_addr_t dst, size_t len, volatile uint16_t *flags)
{
#pragma unused(flags)

	size_t actual;

	if (dtrace_copycheck( dst, src, len )) {
		  
                                                                                              
                                                               
                                                                                          
                   
     
		if (copyoutstr((const void *)src, dst, (size_t)len, &actual)) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = dst;
		}
		dtrace_copyio_postflight(dst);
	}
}

extern const int copysize_limit_panic;

int
dtrace_copy_maxsize(void)
{
	return copysize_limit_panic;
}


int
dtrace_buffer_copyout(const void *kaddr, user_addr_t uaddr, vm_size_t nbytes)
{
	int maxsize = dtrace_copy_maxsize();
	  
                                                              
    
	while (nbytes >= (vm_size_t)maxsize) {
		if (copyout(kaddr, uaddr, maxsize) != 0) {
			return EFAULT;
		}

		nbytes -= maxsize;
		uaddr += maxsize;
		kaddr += maxsize;
	}
	if (nbytes > 0) {
		if (copyout(kaddr, uaddr, nbytes) != 0) {
			return EFAULT;
		}
	}

	return 0;
}

uint8_t
dtrace_fuword8(user_addr_t uaddr)
{
	uint8_t ret = 0;

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	if (dtrace_copycheck( uaddr, (uintptr_t)&ret, sizeof(ret))) {
		if (copyin((const user_addr_t)uaddr, (char *)&ret, sizeof(ret))) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		}
		dtrace_copyio_postflight(uaddr);
	}
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return ret;
}

uint16_t
dtrace_fuword16(user_addr_t uaddr)
{
	uint16_t ret = 0;

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	if (dtrace_copycheck( uaddr, (uintptr_t)&ret, sizeof(ret))) {
		if (copyin((const user_addr_t)uaddr, (char *)&ret, sizeof(ret))) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		}
		dtrace_copyio_postflight(uaddr);
	}
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return ret;
}

uint32_t
dtrace_fuword32(user_addr_t uaddr)
{
	uint32_t ret = 0;

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	if (dtrace_copycheck( uaddr, (uintptr_t)&ret, sizeof(ret))) {
		if (copyin((const user_addr_t)uaddr, (char *)&ret, sizeof(ret))) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		}
		dtrace_copyio_postflight(uaddr);
	}
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return ret;
}

uint64_t
dtrace_fuword64(user_addr_t uaddr)
{
	uint64_t ret = 0;

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	if (dtrace_copycheck( uaddr, (uintptr_t)&ret, sizeof(ret))) {
		if (copyin((const user_addr_t)uaddr, (char *)&ret, sizeof(ret))) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
			cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		}
		dtrace_copyio_postflight(uaddr);
	}
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return ret;
}

  
                                       
                                                                                         
   

int
fuword8(user_addr_t uaddr, uint8_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint8_t)) != 0) {
		return -1;
	}

	return 0;
}

int
fuword16(user_addr_t uaddr, uint16_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint16_t)) != 0) {
		return -1;
	}

	return 0;
}

int
fuword32(user_addr_t uaddr, uint32_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint32_t)) != 0) {
		return -1;
	}

	return 0;
}

int
fuword64(user_addr_t uaddr, uint64_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint64_t)) != 0) {
		return -1;
	}

	return 0;
}

void
fuword32_noerr(user_addr_t uaddr, uint32_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint32_t))) {
		*value = 0;
	}
}

void
fuword64_noerr(user_addr_t uaddr, uint64_t *value)
{
	if (copyin((const user_addr_t)uaddr, (char *)value, sizeof(uint64_t))) {
		*value = 0;
	}
}

int
suword64(user_addr_t addr, uint64_t value)
{
	if (copyout((const void *)&value, addr, sizeof(value)) != 0) {
		return -1;
	}

	return 0;
}

int
suword32(user_addr_t addr, uint32_t value)
{
	if (copyout((const void *)&value, addr, sizeof(value)) != 0) {
		return -1;
	}

	return 0;
}

  
                
   
extern boolean_t dtrace_tally_fault(user_addr_t);

boolean_t
dtrace_tally_fault(user_addr_t uaddr)
{
	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
	return DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT) ? TRUE : FALSE;
}

#define TOTTY   0x02
extern int prf(const char *, va_list, int, struct tty *);                          

int
vuprintf(const char *format, va_list ap)
{
	return prf(format, ap, TOTTY, NULL);
}

                                   
void
cmn_err( int level, const char *format, ... )
{
#pragma unused(level)
	va_list alist;

	va_start(alist, format);
	vuprintf(format, alist);
	va_end(alist);
	uprintf("\n");
}

const void*
bsearch(const void *key, const void *base0, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	const char *base = base0;
	size_t lim;
	int cmp;
	const void *p;
	for (lim = nmemb; lim != 0; lim >>= 1) {
		p = base + (lim >> 1) * size;
		cmp = (*compar)(key, p);
		if (cmp == 0) {
			return p;
		}
		if (cmp > 0) {                           
			base = (const char *)p + size;
			lim--;
		}                                   
	}
	return NULL;
}

  
                  
   
uintptr_t
dtrace_caller(int ignore)
{
#pragma unused(ignore)
	return -1;                                      
}

int
dtrace_getstackdepth(int aframes)
{
	struct frame *fp = (struct frame *)__builtin_frame_address(0);
	struct frame *nextfp, *minfp, *stacktop;
	int depth = 0;
	int on_intr;

	if ((on_intr = CPU_ON_INTR(CPU)) != 0) {
		stacktop = (struct frame *)dtrace_get_cpu_int_stack_top();
	} else {
		stacktop = (struct frame *)(dtrace_get_kernel_stack(current_thread()) + kernel_stack_size);
	}

	minfp = fp;

	aframes++;

	for (;;) {
		depth++;

		nextfp = *(struct frame **)fp;

		if (nextfp <= minfp || nextfp >= stacktop) {
			if (on_intr) {
				  
                                                
       
				vm_offset_t kstack_base = dtrace_get_kernel_stack(current_thread());

				minfp = (struct frame *)kstack_base;
				stacktop = (struct frame *)(kstack_base + kernel_stack_size);

				on_intr = 0;
				continue;
			}
			break;
		}

		fp = nextfp;
		minfp = fp;
	}

	if (depth <= aframes) {
		return 0;
	}

	return depth - aframes;
}

int
dtrace_addr_in_module(void* addr, struct modctl *ctl)
{
	return OSKextKextForAddress(addr) == (void*)ctl->mod_address;
}

  
               
   
void
dtrace_vtime_enable(void)
{
}

void
dtrace_vtime_disable(void)
{
}
