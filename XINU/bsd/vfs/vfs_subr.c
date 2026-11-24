/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_subr.c	8.31 (Berkeley) 5/26/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
                                       
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/buf_internal.h>
#include <sys/errno.h>
#include <kern/kalloc.h>
#include <sys/uio_internal.h>
#include <sys/uio.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/ubc_internal.h>
#include <sys/vm.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#include <sys/event.h>
#include <sys/kdebug.h>
#include <sys/kauth.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/kern_memorystatus.h>
#include <sys/lockf.h>
#include <sys/reboot.h>
#include <miscfs/fifofs/fifo.h>

#include <nfs/nfs_conf.h>

#include <string.h>
#include <machine/machine_routines.h>

#include <kern/assert.h>
#include <mach/kern_return.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>

#include <miscfs/specfs/specdev.h>

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/memory_object_control.h>

#include <kern/kalloc.h>                              
#include <kern/clock.h>                                   
#include <libkern/OSAtomic.h>                      
#include <os/atomic_private.h>
#if defined(XNU_TARGET_OS_OSX)
#include <console/video_console.h>
#endif

#ifdef JOE_DEBUG
#include <libkern/OSDebug.h>
#endif

#include <vm/vm_protos.h>                                

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <vfs/vfs_disk_conditioner.h>
#include <libkern/section_keywords.h>

static LCK_GRP_DECLARE(vnode_lck_grp, "vnode");
static LCK_ATTR_DECLARE(vnode_lck_attr, 0, 0);

#if CONFIG_TRIGGERS
static LCK_GRP_DECLARE(trigger_vnode_lck_grp, "trigger_vnode");
static LCK_ATTR_DECLARE(trigger_vnode_lck_attr, 0, 0);
#endif

extern lck_mtx_t mnt_list_mtx_lock;

ZONE_DECLARE(specinfo_zone, "specinfo",
    sizeof(struct specinfo), ZC_NOENCRYPT | ZC_ZFREE_CLEARMEM);

ZONE_DECLARE(vnode_zone, "vnodes",
    sizeof(struct vnode), ZC_NOENCRYPT | ZC_NOGC | ZC_ZFREE_CLEARMEM);

enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
int     vttoif_tab[9] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT,
};

                                                                      
extern void             memory_object_mark_used(
	memory_object_control_t         control);

extern void             memory_object_mark_unused(
	memory_object_control_t         control,
	boolean_t                       rage);

extern void             memory_object_mark_io_tracking(
	memory_object_control_t         control);

                                                   
extern int       nfs_vinvalbuf(vnode_t, int, vfs_context_t, int);

extern int paniclog_append_noflush(const char *format, ...);

                                                                               
__private_extern__ void qsort(
	void * array,
	size_t nmembers,
	size_t member_size,
	int (*)(const void *, const void *));

__private_extern__ void vntblinit(void);
__private_extern__ int unlink1(vfs_context_t, vnode_t, user_addr_t,
    enum uio_seg, int);

static void vnode_list_add(vnode_t);
static void vnode_async_list_add(vnode_t);
static void vnode_list_remove(vnode_t);
static void vnode_list_remove_locked(vnode_t);

static void vnode_abort_advlocks(vnode_t);
static errno_t vnode_drain(vnode_t);
static void vgone(vnode_t, int flags);
static void vclean(vnode_t vp, int flag);
static void vnode_reclaim_internal(vnode_t, int, int, int);

static void vnode_dropiocount(vnode_t);

static vnode_t checkalias(vnode_t vp, dev_t nvp_rdev);
static int  vnode_reload(vnode_t);

static int unmount_callback(mount_t, __unused void *);

static void insmntque(vnode_t vp, mount_t mp);
static int mount_getvfscnt(void);
static int mount_fillfsids(fsid_t *, int );
static void vnode_iterate_setup(mount_t);
int vnode_umount_preflight(mount_t, vnode_t, int);
static int vnode_iterate_prepare(mount_t);
static int vnode_iterate_reloadq(mount_t);
static void vnode_iterate_clear(mount_t);
static mount_t vfs_getvfs_locked(fsid_t *);
static int vn_create_reg(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp,
    struct vnode_attr *vap, uint32_t flags, int fmode, uint32_t *statusp, vfs_context_t ctx);
static int vnode_authattr_new_internal(vnode_t dvp, struct vnode_attr *vap, int noauth, uint32_t *defaulted_fieldsp, vfs_context_t ctx);

errno_t rmdir_remove_orphaned_appleDouble(vnode_t, vfs_context_t, int *);

#ifdef JOE_DEBUG
static void record_vp(vnode_t vp, int count);
#endif

#if CONFIG_JETSAM && (DEVELOPMENT || DEBUG)
extern int bootarg_no_vnode_jetsam;                                            
#endif                                              

extern int bootarg_no_vnode_drain;                                            

boolean_t root_is_CF_drive = FALSE;

#if CONFIG_TRIGGERS
static int vnode_resolver_create(mount_t, vnode_t, struct vnode_trigger_param *, boolean_t external);
static void vnode_resolver_detach(vnode_t);
#endif

TAILQ_HEAD(freelst, vnode) vnode_free_list;                          
TAILQ_HEAD(deadlst, vnode) vnode_dead_list;                          
TAILQ_HEAD(async_work_lst, vnode) vnode_async_work_list;


TAILQ_HEAD(ragelst, vnode) vnode_rage_list;                               
struct timeval rage_tv;
int     rage_limit = 0;
int     ragevnodes = 0;

int   deadvnodes_low = 0;
int   deadvnodes_high = 0;

uint64_t newvnode = 0;
uint64_t newvnode_nodead = 0;

static  int vfs_unmountall_started = 0;

#define RAGE_LIMIT_MIN  100
#define RAGE_TIME_LIMIT 5

  
                   
                                                                     
                     
   
#define PLATFORM_DATA_VOLUME_MOUNT_POINT "/System/Volumes/Data"

  
                                                   
   
#define PLATFORM_PREBOOT_VOLUME_MOUNT_POINT "/System/Volumes/Preboot"
#define PLATFORM_RECOVERY_VOLUME_MOUNT_POINT "/System/Volumes/Recovery"

#if CONFIG_MOUNT_VM
#define PLATFORM_VM_VOLUME_MOUNT_POINT "/System/Volumes/VM"
#endif

struct mntlist mountlist;                                                    
static int nummounts = 0;

static int print_busy_vnodes = 0;                                                          

#if DIAGNOSTIC
#define VLISTCHECK(fun, vp, list)       \
	if ((vp)->v_freelist.tqe_prev == (struct vnode **)0xdeadb) \
	        panic("%s: %s vnode not on %slist", (fun), (list), (list));
#else
#define VLISTCHECK(fun, vp, list)
#endif                 

#define VLISTNONE(vp)   \
	do {    \
	        (vp)->v_freelist.tqe_next = (struct vnode *)0;  \
	        (vp)->v_freelist.tqe_prev = (struct vnode **)0xdeadb;   \
	} while(0)

#define VONLIST(vp)     \
	((vp)->v_freelist.tqe_prev != (struct vnode **)0xdeadb)

                                         
#define VREMFREE(fun, vp)       \
	do {    \
	        VLISTCHECK((fun), (vp), "free");        \
	        TAILQ_REMOVE(&vnode_free_list, (vp), v_freelist);       \
	        VLISTNONE((vp));        \
	        freevnodes--;   \
	} while(0)


                                         
#define VREMDEAD(fun, vp)       \
	do {    \
	        VLISTCHECK((fun), (vp), "dead");        \
	        TAILQ_REMOVE(&vnode_dead_list, (vp), v_freelist);       \
	        VLISTNONE((vp));        \
	        vp->v_listflag &= ~VLIST_DEAD;  \
	        deadvnodes--;   \
	} while(0)


                                               
#define VREMASYNC_WORK(fun, vp) \
	do {    \
	        VLISTCHECK((fun), (vp), "async_work");  \
	        TAILQ_REMOVE(&vnode_async_work_list, (vp), v_freelist); \
	        VLISTNONE((vp));        \
	        vp->v_listflag &= ~VLIST_ASYNC_WORK;    \
	        async_work_vnodes--;    \
	} while(0)


                                         
#define VREMRAGE(fun, vp)       \
	do {    \
	        if ( !(vp->v_listflag & VLIST_RAGE))                    \
	                panic("VREMRAGE: vp not on rage list");         \
	        VLISTCHECK((fun), (vp), "rage");                        \
	        TAILQ_REMOVE(&vnode_rage_list, (vp), v_freelist);       \
	        VLISTNONE((vp));                \
	        vp->v_listflag &= ~VLIST_RAGE;  \
	        ragevnodes--;                   \
	} while(0)

static void async_work_continue(void);
static void vn_laundry_continue(void);

  
                                                   
   
__private_extern__ void
vntblinit(void)
{
	thread_t        thread = THREAD_NULL;

	TAILQ_INIT(&vnode_free_list);
	TAILQ_INIT(&vnode_rage_list);
	TAILQ_INIT(&vnode_dead_list);
	TAILQ_INIT(&vnode_async_work_list);
	TAILQ_INIT(&mountlist);

	microuptime(&rage_tv);
	rage_limit = desiredvnodes / 100;

	if (rage_limit < RAGE_LIMIT_MIN) {
		rage_limit = RAGE_LIMIT_MIN;
	}

	deadvnodes_low = (desiredvnodes) / 100;
	if (deadvnodes_low > 300) {
		deadvnodes_low = 300;
	}
	deadvnodes_high = deadvnodes_low * 2;

	  
                         
    
	kernel_thread_start((thread_continue_t)async_work_continue, NULL, &thread);
	thread_deallocate(thread);
	kernel_thread_start((thread_continue_t)vn_laundry_continue, NULL, &thread);
	thread_deallocate(thread);
}

                                
int
vnode_waitforwrites(vnode_t vp, int output_target, int slpflag, int slptimeout, const char *msg)
{
	int error = 0;
	struct timespec ts;

	if (output_target < 0) {
		return EINVAL;
	}

	KERNEL_DEBUG(0x3010280 | DBG_FUNC_START, (int)vp, output_target, vp->v_numoutput, 0, 0);

	if (vp->v_numoutput > output_target) {
		slpflag |= PDROP;

		vnode_lock_spin(vp);

		while ((vp->v_numoutput > output_target) && error == 0) {
			if (output_target) {
				vp->v_flag |= VTHROTTLED;
			} else {
				vp->v_flag |= VBWAIT;
			}

			ts.tv_sec = (slptimeout / 100);
			ts.tv_nsec = (slptimeout % 1000)  * 10 * NSEC_PER_USEC * 1000;
			error = msleep((caddr_t)&vp->v_numoutput, &vp->v_lock, (slpflag | (PRIBIO + 1)), msg, &ts);

			vnode_lock_spin(vp);
		}
		vnode_unlock(vp);
	}
	KERNEL_DEBUG(0x3010280 | DBG_FUNC_END, (int)vp, output_target, vp->v_numoutput, error, 0);

	return error;
}


void
vnode_startwrite(vnode_t vp)
{
	OSAddAtomic(1, &vp->v_numoutput);
}


void
vnode_writedone(vnode_t vp)
{
	if (vp) {
		int need_wakeup = 0;

		OSAddAtomic(-1, &vp->v_numoutput);

		vnode_lock_spin(vp);

		if (vp->v_numoutput < 0) {
			panic("vnode_writedone: numoutput < 0");
		}

		if ((vp->v_flag & VTHROTTLED)) {
			vp->v_flag &= ~VTHROTTLED;
			need_wakeup = 1;
		}
		if ((vp->v_flag & VBWAIT) && (vp->v_numoutput == 0)) {
			vp->v_flag &= ~VBWAIT;
			need_wakeup = 1;
		}
		vnode_unlock(vp);

		if (need_wakeup) {
			wakeup((caddr_t)&vp->v_numoutput);
		}
	}
}



int
vnode_hasdirtyblks(vnode_t vp)
{
	struct cl_writebehind *wbp;

	  
                                             
                                                 
                                               
                                                    
                 
    
	if (vp->v_dirtyblkhd.lh_first) {
		return 1;
	}

	if (!UBCINFOEXISTS(vp)) {
		return 0;
	}

	wbp = vp->v_ubcinfo->cl_wbehind;

	if (wbp && (wbp->cl_number || wbp->cl_scmap)) {
		return 1;
	}

	return 0;
}

int
vnode_hascleanblks(vnode_t vp)
{
	  
                                             
                                                 
                                               
                                                    
                 
    
	if (vp->v_cleanblkhd.lh_first) {
		return 1;
	}
	return 0;
}

void
vnode_iterate_setup(mount_t mp)
{
	mp->mnt_lflag |= MNT_LITER;
}

int
vnode_umount_preflight(mount_t mp, vnode_t skipvp, int flags)
{
	vnode_t vp;
	int ret = 0;

	TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
		if (vp->v_type == VDIR) {
			continue;
		}
		if (vp == skipvp) {
			continue;
		}
		if ((flags & SKIPSYSTEM) && ((vp->v_flag & VSYSTEM) || (vp->v_flag & VNOFLUSH))) {
			continue;
		}
		if ((flags & SKIPSWAP) && (vp->v_flag & VSWAP)) {
			continue;
		}
		if ((flags & WRITECLOSE) && (vp->v_writecount == 0 || vp->v_type != VREG)) {
			continue;
		}

		                         
		if ((vp->v_usecount != 0) && ((vp->v_usecount - vp->v_kusecount) != 0)) {
			ret = 1;
			if (print_busy_vnodes && ((flags & FORCECLOSE) == 0)) {
				vprint("vnode_umount_preflight - busy vnode", vp);
			} else {
				return ret;
			}
		} else if (vp->v_iocount > 0) {
			                                                    
			tsleep(&vp->v_iocount, PVFS, "vnode_drain_network", 3 * hz);
			if (vp->v_iocount > 0) {
				ret = 1;
				if (print_busy_vnodes && ((flags & FORCECLOSE) == 0)) {
					vprint("vnode_umount_preflight - busy vnode", vp);
				} else {
					return ret;
				}
			}
			continue;
		}
	}

	return ret;
}

  
                                                                           
                              
   
int
vnode_iterate_prepare(mount_t mp)
{
	vnode_t vp;

	if (TAILQ_EMPTY(&mp->mnt_vnodelist)) {
		                   
		return 0;
	}

	vp = TAILQ_FIRST(&mp->mnt_vnodelist);
	vp->v_mntvnodes.tqe_prev = &(mp->mnt_workerqueue.tqh_first);
	mp->mnt_workerqueue.tqh_first = mp->mnt_vnodelist.tqh_first;
	mp->mnt_workerqueue.tqh_last = mp->mnt_vnodelist.tqh_last;

	TAILQ_INIT(&mp->mnt_vnodelist);
	if (mp->mnt_newvnodes.tqh_first != NULL) {
		panic("vnode_iterate_prepare: newvnode when entering vnode");
	}
	TAILQ_INIT(&mp->mnt_newvnodes);

	return 1;
}


                                 
int
vnode_iterate_reloadq(mount_t mp)
{
	int moved = 0;

	                                                                         
	if (!TAILQ_EMPTY(&mp->mnt_workerqueue)) {
		struct vnode * mvp;
		mvp = TAILQ_LAST(&mp->mnt_vnodelist, vnodelst);

		                                                        
		if (mvp) {
			mvp->v_mntvnodes.tqe_next = mp->mnt_workerqueue.tqh_first;
		} else {
			mp->mnt_vnodelist.tqh_first = mp->mnt_workerqueue.tqh_first;
		}
		mp->mnt_workerqueue.tqh_first->v_mntvnodes.tqe_prev = mp->mnt_vnodelist.tqh_last;
		mp->mnt_vnodelist.tqh_last = mp->mnt_workerqueue.tqh_last;
		TAILQ_INIT(&mp->mnt_workerqueue);
	}

	                                                       
	if (!TAILQ_EMPTY(&mp->mnt_newvnodes)) {
		struct vnode * nlvp;
		nlvp = TAILQ_LAST(&mp->mnt_newvnodes, vnodelst);

		mp->mnt_newvnodes.tqh_first->v_mntvnodes.tqe_prev = &mp->mnt_vnodelist.tqh_first;
		nlvp->v_mntvnodes.tqe_next = mp->mnt_vnodelist.tqh_first;
		if (mp->mnt_vnodelist.tqh_first) {
			mp->mnt_vnodelist.tqh_first->v_mntvnodes.tqe_prev = &nlvp->v_mntvnodes.tqe_next;
		} else {
			mp->mnt_vnodelist.tqh_last = mp->mnt_newvnodes.tqh_last;
		}
		mp->mnt_vnodelist.tqh_first = mp->mnt_newvnodes.tqh_first;
		TAILQ_INIT(&mp->mnt_newvnodes);
		moved = 1;
	}

	return moved;
}


void
vnode_iterate_clear(mount_t mp)
{
	mp->mnt_lflag &= ~MNT_LITER;
}

#if defined(__x86_64__)

#include <i386/panic_hooks.h>

struct vnode_iterate_panic_hook {
	panic_hook_t hook;
	mount_t mp;
	struct vnode *vp;
};

static void
vnode_iterate_panic_hook(panic_hook_t *hook_)
{
	struct vnode_iterate_panic_hook *hook = (struct vnode_iterate_panic_hook *)hook_;
	panic_phys_range_t range;
	uint64_t phys;

	if (panic_phys_range_before(hook->mp, &phys, &range)) {
		paniclog_append_noflush("mp = %p, phys = %p, prev (%p: %p-%p)\n",
		    hook->mp, phys, range.type, range.phys_start,
		    range.phys_start + range.len);
	} else {
		paniclog_append_noflush("mp = %p, phys = %p, prev (!)\n", hook->mp, phys);
	}

	if (panic_phys_range_before(hook->vp, &phys, &range)) {
		paniclog_append_noflush("vp = %p, phys = %p, prev (%p: %p-%p)\n",
		    hook->vp, phys, range.type, range.phys_start,
		    range.phys_start + range.len);
	} else {
		paniclog_append_noflush("vp = %p, phys = %p, prev (!)\n", hook->vp, phys);
	}
	panic_dump_mem((void *)(((vm_offset_t)hook->mp - 4096) & ~4095), 12288);
}
#endif                          

int
vnode_iterate(mount_t mp, int flags, int (*callout)(struct vnode *, void *),
    void *arg)
{
	struct vnode *vp;
	int vid, retval;
	int ret = 0;

	  
                                                                      
                                                                      
                                                 
                                                                 
                                                                  
                                                                        
                           
    
	mount_iterate_lock(mp);

	mount_lock(mp);

	vnode_iterate_setup(mp);

	                                                 
	retval = vnode_iterate_prepare(mp);

	if (retval == 0) {
		vnode_iterate_clear(mp);
		mount_unlock(mp);
		mount_iterate_unlock(mp);
		return ret;
	}

#if defined(__x86_64__)
	struct vnode_iterate_panic_hook hook;
	hook.mp = mp;
	hook.vp = NULL;
	panic_hook(&hook.hook, vnode_iterate_panic_hook);
#endif
	                                 
	while (!TAILQ_EMPTY(&mp->mnt_workerqueue)) {
		vp = TAILQ_FIRST(&mp->mnt_workerqueue);
#if defined(__x86_64__)
		hook.vp = vp;
#endif
		TAILQ_REMOVE(&mp->mnt_workerqueue, vp, v_mntvnodes);
		TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vp, v_mntvnodes);
		vid = vp->v_id;
		if ((vp->v_data == NULL) || (vp->v_type == VNON) || (vp->v_mount != mp)) {
			continue;
		}
		mount_unlock(mp);

		if (vget_internal(vp, vid, (flags | VNODE_NODEAD | VNODE_WITHID | VNODE_NOSUSPEND))) {
			mount_lock(mp);
			continue;
		}
		if (flags & VNODE_RELOAD) {
			  
                                    
                                     
      
			if (vnode_reload(vp)) {
				                                                 
				vnode_put(vp);
				mount_lock(mp);
				continue;
			}
		}

		retval = callout(vp, arg);

		switch (retval) {
		case VNODE_RETURNED:
		case VNODE_RETURNED_DONE:
			vnode_put(vp);
			if (retval == VNODE_RETURNED_DONE) {
				mount_lock(mp);
				ret = 0;
				goto out;
			}
			break;

		case VNODE_CLAIMED_DONE:
			mount_lock(mp);
			ret = 0;
			goto out;
		case VNODE_CLAIMED:
		default:
			break;
		}
		mount_lock(mp);
	}

out:
#if defined(__x86_64__)
	panic_unhook(&hook.hook);
#endif
	(void)vnode_iterate_reloadq(mp);
	vnode_iterate_clear(mp);
	mount_unlock(mp);
	mount_iterate_unlock(mp);
	return ret;
}

void
mount_lock_renames(mount_t mp)
{
	lck_mtx_lock(&mp->mnt_renamelock);
}

void
mount_unlock_renames(mount_t mp)
{
	lck_mtx_unlock(&mp->mnt_renamelock);
}

void
mount_iterate_lock(mount_t mp)
{
	lck_mtx_lock(&mp->mnt_iter_lock);
}

void
mount_iterate_unlock(mount_t mp)
{
	lck_mtx_unlock(&mp->mnt_iter_lock);
}

void
mount_lock(mount_t mp)
{
	lck_mtx_lock(&mp->mnt_mlock);
}

void
mount_lock_spin(mount_t mp)
{
	lck_mtx_lock_spin(&mp->mnt_mlock);
}

void
mount_unlock(mount_t mp)
{
	lck_mtx_unlock(&mp->mnt_mlock);
}


void
mount_ref(mount_t mp, int locked)
{
	if (!locked) {
		mount_lock_spin(mp);
	}

	mp->mnt_count++;

	if (!locked) {
		mount_unlock(mp);
	}
}


void
mount_drop(mount_t mp, int locked)
{
	if (!locked) {
		mount_lock_spin(mp);
	}

	mp->mnt_count--;

	if (mp->mnt_count == 0 && (mp->mnt_lflag & MNT_LDRAIN)) {
		wakeup(&mp->mnt_lflag);
	}

	if (!locked) {
		mount_unlock(mp);
	}
}


int
mount_iterref(mount_t mp, int locked)
{
	int retval = 0;

	if (!locked) {
		mount_list_lock();
	}
	if (mp->mnt_iterref < 0) {
		retval = 1;
	} else {
		mp->mnt_iterref++;
	}
	if (!locked) {
		mount_list_unlock();
	}
	return retval;
}

int
mount_isdrained(mount_t mp, int locked)
{
	int retval;

	if (!locked) {
		mount_list_lock();
	}
	if (mp->mnt_iterref < 0) {
		retval = 1;
	} else {
		retval = 0;
	}
	if (!locked) {
		mount_list_unlock();
	}
	return retval;
}

void
mount_iterdrop(mount_t mp)
{
	mount_list_lock();
	mp->mnt_iterref--;
	wakeup(&mp->mnt_iterref);
	mount_list_unlock();
}

void
mount_iterdrain(mount_t mp)
{
	mount_list_lock();
	while (mp->mnt_iterref) {
		msleep((caddr_t)&mp->mnt_iterref, &mnt_list_mtx_lock, PVFS, "mount_iterdrain", NULL);
	}
	                              
	mp->mnt_iterref = -1;
	mount_list_unlock();
}
void
mount_iterreset(mount_t mp)
{
	mount_list_lock();
	if (mp->mnt_iterref == -1) {
		mp->mnt_iterref = 0;
	}
	mount_list_unlock();
}

                                         
int
mount_refdrain(mount_t mp)
{
	if (mp->mnt_lflag & MNT_LDRAIN) {
		panic("already in drain");
	}
	mp->mnt_lflag |= MNT_LDRAIN;

	while (mp->mnt_count) {
		msleep((caddr_t)&mp->mnt_lflag, &mp->mnt_mlock, PVFS, "mount_drain", NULL);
	}

	if (mp->mnt_vnodelist.tqh_first != NULL) {
		panic("mount_refdrain: dangling vnode");
	}

	mp->mnt_lflag &= ~MNT_LDRAIN;

	return 0;
}

                                                                             
void
mount_set_noreaddirext(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_DENY_READDIREXT;
	mount_unlock(mp);
}

  
                                                                      
              
   
int
vfs_busy(mount_t mp, int flags)
{
restart:
	if (mp->mnt_lflag & MNT_LDEAD) {
		return ENOENT;
	}

	mount_lock(mp);

	if (mp->mnt_lflag & MNT_LUNMOUNT) {
		if (flags & LK_NOWAIT || mp->mnt_lflag & MNT_LDEAD) {
			mount_unlock(mp);
			return ENOENT;
		}

		  
                                                         
                                                        
                                                     
                                            
     
		mp->mnt_lflag |= MNT_LWAIT;
		msleep((caddr_t)mp, &mp->mnt_mlock, (PVFS | PDROP), "vfsbusy", NULL);
		return ENOENT;
	}

	mount_unlock(mp);

	lck_rw_lock_shared(&mp->mnt_rwlock);

	  
                                                                         
                                                              
    
	if (mp->mnt_lflag & (MNT_LDEAD | MNT_LUNMOUNT)) {
		lck_rw_done(&mp->mnt_rwlock);
		goto restart;
	}
	return 0;
}

  
                          
   
void
vfs_unbusy(mount_t mp)
{
	lck_rw_done(&mp->mnt_rwlock);
}



static void
vfs_rootmountfailed(mount_t mp)
{
	mount_list_lock();
	mp->mnt_vtable->vfc_refcount--;
	mount_list_unlock();

	vfs_unbusy(mp);

	mount_lock_destroy(mp);

#if CONFIG_MACF
	mac_mount_label_destroy(mp);
#endif

	zfree(mount_zone, mp);
}

  
                                                                 
                            
  
                                                        
   
static mount_t
vfs_rootmountalloc_internal(struct vfstable *vfsp, const char *devname)
{
	mount_t mp;

	mp = zalloc_flags(mount_zone, Z_WAITOK | Z_ZERO);
	                                           
	mp->mnt_maxreadcnt = mp->mnt_maxwritecnt = MAXPHYS;
	mp->mnt_segreadcnt = mp->mnt_segwritecnt = 32;
	mp->mnt_maxsegreadsize = mp->mnt_maxreadcnt;
	mp->mnt_maxsegwritesize = mp->mnt_maxwritecnt;
	mp->mnt_devblocksize = DEV_BSIZE;
	mp->mnt_alignmentmask = PAGE_MASK;
	mp->mnt_ioqueue_depth = MNT_DEFAULT_IOQUEUE_DEPTH;
	mp->mnt_ioscale = 1;
	mp->mnt_ioflags = 0;
	mp->mnt_realrootvp = NULLVP;
	mp->mnt_authcache_ttl = CACHED_LOOKUP_RIGHT_TTL;
	mp->mnt_throttle_mask = LOWPRI_MAX_NUM_DEV - 1;
	mp->mnt_devbsdunit = 0;

	mount_lock_init(mp);
	(void)vfs_busy(mp, LK_NOWAIT);

	TAILQ_INIT(&mp->mnt_vnodelist);
	TAILQ_INIT(&mp->mnt_workerqueue);
	TAILQ_INIT(&mp->mnt_newvnodes);

	mp->mnt_vtable = vfsp;
	mp->mnt_op = vfsp->vfc_vfsops;
	mp->mnt_flag = MNT_RDONLY | MNT_ROOTFS;
	mp->mnt_vnodecovered = NULLVP;
	                                          
	mp->mnt_flag |= vfsp->vfc_flags & MNT_VISFLAGMASK;

	mount_list_lock();
	vfsp->vfc_refcount++;
	mount_list_unlock();

	strlcpy(mp->mnt_vfsstat.f_fstypename, vfsp->vfc_name, MFSTYPENAMELEN);
	mp->mnt_vfsstat.f_mntonname[0] = '/';
	                                            
	(void) copystr((const void *)devname, mp->mnt_vfsstat.f_mntfromname, MAXPATHLEN - 1, NULL);

#if CONFIG_MACF
	mac_mount_label_init(mp);
	mac_mount_label_associate(vfs_context_kernel(), mp);
#endif
	return mp;
}

errno_t
vfs_rootmountalloc(const char *fstypename, const char *devname, mount_t *mpp)
{
	struct vfstable *vfsp;

	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (!strncmp(vfsp->vfc_name, fstypename,
		    sizeof(vfsp->vfc_name))) {
			break;
		}
	}
	if (vfsp == NULL) {
		return ENODEV;
	}

	*mpp = vfs_rootmountalloc_internal(vfsp, devname);

	if (*mpp) {
		return 0;
	}

	return ENOMEM;
}

#define DBG_MOUNTROOT (FSDBG_CODE(DBG_MOUNT, 0))

  
                                                                      
                                                                       
                                                                    
                                   
   
extern int (*mountroot)(void);

int
vfs_mountroot(void)
{
#if CONFIG_MACF
	struct vnode *vp;
#endif
	struct vfstable *vfsp;
	vfs_context_t ctx = vfs_context_kernel();
	struct vfs_attr vfsattr;
	int     error;
	mount_t mp;
	vnode_t bdevvp_rootvp;

	KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_START);
	if (mountroot != NULL) {
		  
                                                            
     
		error = (*mountroot)();

		KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_END, error, 0);
		return error;
	}
	if ((error = bdevvp(rootdev, &rootvp))) {
		printf("vfs_mountroot: can't setup bdevvp\n");

		KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_END, error, 1);
		return error;
	}
	  
                                                              
                                                
    
	bdevvp_rootvp = rootvp;

	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (vfsp->vfc_mountroot == NULL
		    && !ISSET(vfsp->vfc_vfsflags, VFC_VFSCANMOUNTROOT)) {
			continue;
		}

		mp = vfs_rootmountalloc_internal(vfsp, "root_device");
		mp->mnt_devvp = rootvp;

		if (vfsp->vfc_mountroot) {
			error = (*vfsp->vfc_mountroot)(mp, rootvp, ctx);
		} else {
			error = VFS_MOUNT(mp, rootvp, 0, ctx);
		}

		if (!error) {
			if (bdevvp_rootvp != rootvp) {
				  
                        
                                                      
                                                              
                                                         
                                                  
       
				vnode_getwithref(rootvp);
				mp->mnt_devvp = rootvp;

				vnode_rele(bdevvp_rootvp);
				vnode_put(bdevvp_rootvp);
			}
			mp->mnt_devvp->v_specflags |= SI_MOUNTEDON;

			vfs_unbusy(mp);

			mount_list_add(mp);

			  
                                                                    
                                                               
                                                                
                                                                 
      
			vfs_init_io_attributes(rootvp, mp);

			if (mp->mnt_ioflags & MNT_IOFLAGS_FUSION_DRIVE) {
				root_is_CF_drive = TRUE;
			}

			  
                                                                
      
			if (mp->mnt_vtable->vfc_vfsflags & VFC_VFSNATIVEXATTR) {
				mp->mnt_kern_flag |= MNTK_EXTENDED_ATTRS;
			}
			if (mp->mnt_vtable->vfc_vfsflags & VFC_VFSPREFLIGHT) {
				mp->mnt_kern_flag |= MNTK_UNMOUNT_PREFLIGHT;
			}

#if defined(XNU_TARGET_OS_OSX)
			uint32_t speed;

			if (MNTK_VIRTUALDEV & mp->mnt_kern_flag) {
				speed = 128;
			} else if (disk_conditioner_mount_is_ssd(mp)) {
				speed = 7 * 256;
			} else {
				speed = 256;
			}
			vc_progress_setdiskspeed(speed);
#endif                        
			  
                                                     
      
			(void)VFS_START(mp, 0, ctx);

			VFSATTR_INIT(&vfsattr);
			VFSATTR_WANTED(&vfsattr, f_capabilities);
			if (vfs_getattr(mp, &vfsattr, ctx) == 0 &&
			    VFSATTR_IS_SUPPORTED(&vfsattr, f_capabilities)) {
				if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR) &&
				    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR)) {
					mp->mnt_kern_flag |= MNTK_EXTENDED_ATTRS;
				}
#if NAMEDSTREAMS
				if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_NAMEDSTREAMS) &&
				    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_NAMEDSTREAMS)) {
					mp->mnt_kern_flag |= MNTK_NAMED_STREAMS;
				}
#endif
				if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_PATH_FROM_ID) &&
				    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_PATH_FROM_ID)) {
					mp->mnt_kern_flag |= MNTK_PATH_FROM_ID;
				}

				if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_DIR_HARDLINKS) &&
				    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_DIR_HARDLINKS)) {
					mp->mnt_kern_flag |= MNTK_DIR_HARDLINKS;
				}
			}

			  
                                           
                                                     
                                                
                                                
      
			vnode_put(rootvp);

#if CONFIG_MACF
			if ((vfs_flags(mp) & MNT_MULTILABEL) == 0) {
				KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_END, 0, 2);
				return 0;
			}

			error = VFS_ROOT(mp, &vp, ctx);
			if (error) {
				printf("%s() VFS_ROOT() returned %d\n",
				    __func__, error);
				dounmount(mp, MNT_FORCE, 0, ctx);
				goto fail;
			}
			error = vnode_label(mp, NULL, vp, NULL, 0, ctx);
			  
                                               
      
			vnode_put(vp);

			if (error) {
				printf("%s() vnode_label() returned %d\n",
				    __func__, error);
				dounmount(mp, MNT_FORCE, 0, ctx);
				goto fail;
			}
#endif
			KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_END, 0, 3);
			return 0;
		}
#if CONFIG_MACF
fail:
#endif
		vfs_rootmountfailed(mp);

		if (error != EINVAL) {
			printf("%s_mountroot failed: %d\n", vfsp->vfc_name, error);
		}
	}
	KDBG_RELEASE(DBG_MOUNTROOT | DBG_FUNC_END, error ? error : ENODEV, 4);
	return ENODEV;
}

static int
cache_purge_callback(mount_t mp, __unused void * arg)
{
	cache_purgevfs(mp);
	return VFS_RETURNED;
}

extern lck_rw_t rootvnode_rw_lock;
extern void set_rootvnode(vnode_t);


static int
mntonname_fixup_callback(mount_t mp, __unused void *arg)
{
	int error = 0;

	if ((strncmp(&mp->mnt_vfsstat.f_mntonname[0], "/", sizeof("/")) == 0) ||
	    (strncmp(&mp->mnt_vfsstat.f_mntonname[0], "/dev", sizeof("/dev")) == 0)) {
		return 0;
	}

	if ((error = vfs_busy(mp, LK_NOWAIT))) {
		printf("vfs_busy failed with %d for %s\n", error, mp->mnt_vfsstat.f_mntonname);
		return -1;
	}

	int pathlen = MAXPATHLEN;
	if ((error = vn_getpath_ext(mp->mnt_vnodecovered, NULL, mp->mnt_vfsstat.f_mntonname, &pathlen, VN_GETPATH_FSENTER))) {
		printf("vn_getpath_ext failed with %d for mnt_vnodecovered of %s\n", error, mp->mnt_vfsstat.f_mntonname);
	}

	vfs_unbusy(mp);

	return error;
}

static int
clear_mntk_backs_root_callback(mount_t mp, __unused void *arg)
{
	lck_rw_lock_exclusive(&mp->mnt_rwlock);
	mp->mnt_kern_flag &= ~MNTK_BACKS_ROOT;
	lck_rw_done(&mp->mnt_rwlock);
	return VFS_RETURNED;
}

static int
verify_incoming_rootfs(vnode_t *incoming_rootvnodep, vfs_context_t ctx,
    vfs_switch_root_flags_t flags)
{
	mount_t mp;
	vnode_t tdp;
	vnode_t incoming_rootvnode_with_iocount = *incoming_rootvnodep;
	vnode_t incoming_rootvnode_with_usecount = NULLVP;
	int error = 0;

	if (vnode_vtype(incoming_rootvnode_with_iocount) != VDIR) {
		printf("Incoming rootfs path not a directory\n");
		error = ENOTDIR;
		goto done;
	}

	  
                                                                     
                                                   
    
	vnode_ref_ext(incoming_rootvnode_with_iocount, 0, VNODE_REF_FORCE);
	incoming_rootvnode_with_usecount = incoming_rootvnode_with_iocount;
	vnode_lock_spin(incoming_rootvnode_with_usecount);
	if ((mp = incoming_rootvnode_with_usecount->v_mount)) {
		mp->mnt_crossref++;
		vnode_unlock(incoming_rootvnode_with_usecount);
	} else {
		vnode_unlock(incoming_rootvnode_with_usecount);
		printf("Incoming rootfs root vnode does not have associated mount\n");
		error = ENOTDIR;
		goto done;
	}

	if (vfs_busy(mp, LK_NOWAIT)) {
		printf("Incoming rootfs root vnode mount is busy\n");
		error = ENOENT;
		goto out;
	}

	vnode_put(incoming_rootvnode_with_iocount);
	incoming_rootvnode_with_iocount = NULLVP;

	error = VFS_ROOT(mp, &tdp, ctx);

	if (error) {
		printf("Could not get rootvnode of incoming rootfs\n");
	} else if (tdp != incoming_rootvnode_with_usecount) {
		vnode_put(tdp);
		tdp = NULLVP;
		printf("Incoming rootfs root vnode mount is is not a mountpoint\n");
		error = EINVAL;
		goto out_busy;
	} else {
		incoming_rootvnode_with_iocount = tdp;
		tdp = NULLVP;
	}

	if ((flags & VFSSR_VIRTUALDEV_PROHIBITED) != 0) {
		lck_rw_lock_shared(&mp->mnt_rwlock);
		if (mp->mnt_flag & MNTK_VIRTUALDEV) {
			error = ENODEV;
		}
		lck_rw_done(&mp->mnt_rwlock);
		if (error) {
			printf("Incoming rootfs is backed by a virtual device; cannot switch to it");
			goto out_busy;
		}
	}

out_busy:
	vfs_unbusy(mp);

out:
	vnode_lock(incoming_rootvnode_with_usecount);
	mp->mnt_crossref--;
	if (mp->mnt_crossref < 0) {
		panic("mount cross refs -ve");
	}
	vnode_unlock(incoming_rootvnode_with_usecount);

done:
	if (incoming_rootvnode_with_usecount) {
		vnode_rele(incoming_rootvnode_with_usecount);
		incoming_rootvnode_with_usecount = NULLVP;
	}

	if (error && incoming_rootvnode_with_iocount) {
		vnode_put(incoming_rootvnode_with_iocount);
		incoming_rootvnode_with_iocount = NULLVP;
	}

	*incoming_rootvnodep = incoming_rootvnode_with_iocount;
	return error;
}

  
                    
  
                                                                        
  
                                                                         
                                        
                                                                         
                                                          
                                    
  
                                                                             
                                                                         
                                                                           
                                   
  
                                                                        
                                                                     
                                                                          
                  
   
int
vfs_switch_root(const char *incoming_vol_old_path,
    const char *outgoing_vol_new_path,
    vfs_switch_root_flags_t flags)
{
	                  
#define countof(x) (sizeof(x) / sizeof(x[0]))

	struct preserved_mount {
		vnode_t pm_rootvnode;
		mount_t pm_mount;
		vnode_t pm_new_covered_vp;
		vnode_t pm_old_covered_vp;
		const char *pm_path;
	};

	vfs_context_t ctx = vfs_context_kernel();
	vnode_t incoming_rootvnode = NULLVP;
	vnode_t outgoing_vol_new_covered_vp = NULLVP;
	vnode_t incoming_vol_old_covered_vp = NULLVP;
	mount_t outgoing = NULL;
	mount_t incoming = NULL;

	struct preserved_mount devfs = { NULLVP, NULL, NULLVP, NULLVP, "dev" };
	struct preserved_mount preboot = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/Preboot" };
	struct preserved_mount recovery = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/Recovery" };
	struct preserved_mount vm = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/VM" };
	struct preserved_mount update = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/Update" };
	struct preserved_mount iscPreboot = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/iSCPreboot" };
	struct preserved_mount hardware = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/Hardware" };
	struct preserved_mount xarts = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/xarts" };
	struct preserved_mount factorylogs = { NULLVP, NULL, NULLVP, NULLVP, "FactoryLogs" };
	struct preserved_mount idiags = { NULLVP, NULL, NULLVP, NULLVP, "System/Volumes/Diags" };

	struct preserved_mount *preserved[10];
	preserved[0] = &devfs;
	preserved[1] = &preboot;
	preserved[2] = &recovery;
	preserved[3] = &vm;
	preserved[4] = &update;
	preserved[5] = &iscPreboot;
	preserved[6] = &hardware;
	preserved[7] = &xarts;
	preserved[8] = &factorylogs;
	preserved[9] = &idiags;

	int error;

	printf("%s : shuffling mount points : %s <-> / <-> %s\n", __FUNCTION__, incoming_vol_old_path, outgoing_vol_new_path);

	if (outgoing_vol_new_path[0] == '/') {
		                                                                                                    
		printf("Do not use a leading slash in outgoing_vol_new_path\n");
		return EINVAL;
	}

	                          
	                                                             
	                                                   
	error = vnode_lookup(incoming_vol_old_path, 0, &incoming_rootvnode, ctx);
	if (error) {
		printf("Incoming rootfs root vnode not found\n");
		error = ENOENT;
		goto done;
	}

	  
                                                                        
    
	error = verify_incoming_rootfs(&incoming_rootvnode, ctx, flags);
	if (error) {
		goto done;
	}

	  
                                    
                                                                
                                                             
                                                                     
                                                                       
    
	error = vnode_lookupat(outgoing_vol_new_path, 0, &outgoing_vol_new_covered_vp, ctx, incoming_rootvnode);
	if (error) {
		printf("Outgoing rootfs path not found, abandoning / switch, error = %d\n", error);
		error = ENOENT;
		goto done;
	}
	if (vnode_vtype(outgoing_vol_new_covered_vp) != VDIR) {
		printf("Outgoing rootfs path is not a directory, abandoning / switch\n");
		error = ENOTDIR;
		goto done;
	}

	  
                                                                       
                                                                      
                                                
   
                                              
                                                                                  
                                                   
                                                                            
    
	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];

		error = vnode_lookupat(pmi->pm_path, 0, &pmi->pm_rootvnode, ctx, rootvnode);
		if (error) {
			printf("skipping preserved mountpoint because not found or error: %d: %s\n", error, pmi->pm_path);
			                                           
			continue;
		}
		bool is_mountpoint = false;
		vnode_lock_spin(pmi->pm_rootvnode);
		if ((pmi->pm_rootvnode->v_flag & VROOT) != 0) {
			is_mountpoint = true;
		}
		vnode_unlock(pmi->pm_rootvnode);
		if (!is_mountpoint) {
			printf("skipping preserved mountpoint because not a mountpoint: %s\n", pmi->pm_path);
			vnode_put(pmi->pm_rootvnode);
			pmi->pm_rootvnode = NULLVP;
			                                           
			continue;
		}

		error = vnode_lookupat(pmi->pm_path, 0, &pmi->pm_new_covered_vp, ctx, incoming_rootvnode);
		if (error) {
			printf("preserved new mount directory not found or error: %d: %s\n", error, pmi->pm_path);
			error = ENOENT;
			goto done;
		}
		if (vnode_vtype(pmi->pm_new_covered_vp) != VDIR) {
			printf("preserved new mount directory not directory: %s\n", pmi->pm_path);
			error = ENOTDIR;
			goto done;
		}

		printf("will preserve mountpoint across pivot: /%s\n", pmi->pm_path);
	}

	  
      
                                                                        
                                                                    
                                                                      
                           
        
    

	                                                          
	vnode_ref_ext(outgoing_vol_new_covered_vp, 0, VNODE_REF_FORCE);
	                                                   
	vnode_ref_ext(incoming_rootvnode, 0, VNODE_REF_FORCE);


	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];
		if (pmi->pm_rootvnode == NULLVP) {
			continue;
		}

		                                                          
		vnode_ref_ext(pmi->pm_new_covered_vp, 0, VNODE_REF_FORCE);

		                                                     
		vnode_lock_spin(pmi->pm_new_covered_vp);
		pmi->pm_new_covered_vp->v_flag |= VMOUNT;
		vnode_unlock(pmi->pm_new_covered_vp);
	}

	                                                                  
	vnode_lock_spin(outgoing_vol_new_covered_vp);
	outgoing_vol_new_covered_vp->v_flag |= VMOUNT;
	vnode_unlock(outgoing_vol_new_covered_vp);


	  
                                                                   
                                                                    
           
    
	outgoing = rootvnode->v_mount;
	incoming = incoming_rootvnode->v_mount;
	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];
		if (pmi->pm_rootvnode == NULLVP) {
			continue;
		}

		pmi->pm_mount = pmi->pm_rootvnode->v_mount;
	}

	lck_rw_lock_exclusive(&rootvnode_rw_lock);

	                                      
	lck_rw_lock_exclusive(&incoming->mnt_rwlock);
	incoming_vol_old_covered_vp = incoming->mnt_vnodecovered;
	incoming->mnt_vnodecovered = NULLVP;
	strlcpy(incoming->mnt_vfsstat.f_mntonname, "/", MAXPATHLEN);
	incoming->mnt_flag |= MNT_ROOTFS;
	lck_rw_done(&incoming->mnt_rwlock);

	  
                                                  
                                                                   
                                                               
                                   
    
	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];
		if (pmi->pm_rootvnode == NULLVP) {
			continue;
		}

		lck_rw_lock_exclusive(&pmi->pm_mount->mnt_rwlock);
		pmi->pm_old_covered_vp = pmi->pm_mount->mnt_vnodecovered;
		pmi->pm_mount->mnt_vnodecovered = pmi->pm_new_covered_vp;
		vnode_lock_spin(pmi->pm_new_covered_vp);
		pmi->pm_new_covered_vp->v_mountedhere = pmi->pm_mount;
		vnode_unlock(pmi->pm_new_covered_vp);
		lck_rw_done(&pmi->pm_mount->mnt_rwlock);
	}

	  
                                                              
                                                     
                                                  
    
	lck_rw_lock_exclusive(&outgoing->mnt_rwlock);
	outgoing->mnt_vnodecovered = outgoing_vol_new_covered_vp;
	strlcpy(outgoing->mnt_vfsstat.f_mntonname, "/", MAXPATHLEN);
	strlcat(outgoing->mnt_vfsstat.f_mntonname, outgoing_vol_new_path, MAXPATHLEN);
	outgoing->mnt_flag &= ~MNT_ROOTFS;
	vnode_lock_spin(outgoing_vol_new_covered_vp);
	outgoing_vol_new_covered_vp->v_mountedhere = outgoing;
	vnode_unlock(outgoing_vol_new_covered_vp);
	lck_rw_done(&outgoing->mnt_rwlock);

	if (!(outgoing->mnt_kern_flag & MNTK_VIRTUALDEV) &&
	    (TAILQ_FIRST(&mountlist) == outgoing)) {
		vfs_setmntsystem(outgoing);
	}

	  
                                                                   
                                                                    
                                                                      
                                               
    
	vnode_lock_spin(incoming_vol_old_covered_vp);
	incoming_vol_old_covered_vp->v_flag &= ~VMOUNT;
	incoming_vol_old_covered_vp->v_mountedhere = NULL;
	vnode_unlock(incoming_vol_old_covered_vp);

	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];
		if (pmi->pm_rootvnode == NULLVP) {
			continue;
		}

		vnode_lock_spin(pmi->pm_old_covered_vp);
		pmi->pm_old_covered_vp->v_flag &= ~VMOUNT;
		pmi->pm_old_covered_vp->v_mountedhere = NULL;
		vnode_unlock(pmi->pm_old_covered_vp);
	}

	  
                                                                 
    
	vfs_iterate(0            , cache_purge_callback, NULL);

	  
                                                                 
                                      
    
	set_rootvnode(incoming_rootvnode);
	lck_rw_unlock_exclusive(&rootvnode_rw_lock);

	if (!(incoming->mnt_kern_flag & MNTK_VIRTUALDEV) &&
	    !(outgoing->mnt_kern_flag & MNTK_VIRTUALDEV)) {
		  
                                                                    
                                                                       
                                                                       
            
     
		mount_list_lock();
		for (size_t i = 0; i < countof(preserved); i++) {
			struct preserved_mount *pmi = preserved[i];
			if (pmi->pm_rootvnode == NULLVP) {
				continue;
			}

			TAILQ_REMOVE(&mountlist, pmi->pm_mount, mnt_list);
			TAILQ_INSERT_HEAD(&mountlist, pmi->pm_mount, mnt_list);
		}
		TAILQ_REMOVE(&mountlist, incoming, mnt_list);
		TAILQ_INSERT_HEAD(&mountlist, incoming, mnt_list);
		mount_list_unlock();
	}

	  
                             
    
	vfs_iterate(0            , mntonname_fixup_callback, NULL);
	vfs_iterate(0            , clear_mntk_backs_root_callback, NULL);

	error = 0;

done:
	for (size_t i = 0; i < countof(preserved); i++) {
		struct preserved_mount *pmi = preserved[i];

		if (pmi->pm_rootvnode) {
			vnode_put(pmi->pm_rootvnode);
		}
		if (pmi->pm_new_covered_vp) {
			vnode_put(pmi->pm_new_covered_vp);
		}
		if (pmi->pm_old_covered_vp) {
			vnode_rele(pmi->pm_old_covered_vp);
		}
	}

	if (outgoing_vol_new_covered_vp) {
		vnode_put(outgoing_vol_new_covered_vp);
	}

	if (incoming_vol_old_covered_vp) {
		vnode_rele(incoming_vol_old_covered_vp);
	}

	if (incoming_rootvnode) {
		vnode_put(incoming_rootvnode);
	}

	printf("%s : done shuffling mount points with error: %d\n", __FUNCTION__, error);
	return error;
}

  
                                           
   
int
vfs_mount_recovery(void)
{
#if CONFIG_MOUNT_PREBOOTRECOVERY
	int error = 0;

	error = vnode_get(rootvnode);
	if (error) {
		                                
		printf("vnode_get(rootvnode) failed with error %d\n", error);
		return error;
	}

	char recoverypath[] = PLATFORM_RECOVERY_VOLUME_MOUNT_POINT;                                         

	                               
	printf("attempting kernel mount for recovery volume... \n");
	error = kernel_mount(rootvnode->v_mount->mnt_vfsstat.f_fstypename, NULLVP, NULLVP,
	    recoverypath, (rootvnode->v_mount), 0, 0, (KERNEL_MOUNT_RECOVERYVOL), vfs_context_kernel());

	if (error) {
		printf("Failed to mount recovery volume (%d)\n", error);
	} else {
		printf("mounted recovery volume\n");
	}

	vnode_put(rootvnode);
	return error;
#else
	return 0;
#endif
}

  
                                                 
   

struct mount *
vfs_getvfs(fsid_t *fsid)
{
	return mount_list_lookupby_fsid(fsid, 0, 0);
}

static struct mount *
vfs_getvfs_locked(fsid_t *fsid)
{
	return mount_list_lookupby_fsid(fsid, 1, 0);
}

struct mount *
vfs_getvfs_by_mntonname(char *path)
{
	mount_t retmp = (mount_t)0;
	mount_t mp;

	mount_list_lock();
	TAILQ_FOREACH(mp, &mountlist, mnt_list) {
		if (!strncmp(mp->mnt_vfsstat.f_mntonname, path,
		    sizeof(mp->mnt_vfsstat.f_mntonname))) {
			retmp = mp;
			if (mount_iterref(retmp, 1)) {
				retmp = NULL;
			}
			goto out;
		}
	}
out:
	mount_list_unlock();
	return retmp;
}

                                                 
u_short mntid_gen = 0;
  
                        
   
void
vfs_getnewfsid(struct mount *mp)
{
	fsid_t tfsid;
	int mtype;

	mount_list_lock();

	                         
	mtype = mp->mnt_vtable->vfc_typenum;
	if (++mntid_gen == 0) {
		mntid_gen++;
	}
	tfsid.val[0] = makedev(nblkdev + mtype, mntid_gen);
	tfsid.val[1] = mtype;

	while (vfs_getvfs_locked(&tfsid)) {
		if (++mntid_gen == 0) {
			mntid_gen++;
		}
		tfsid.val[0] = makedev(nblkdev + mtype, mntid_gen);
	}

	mp->mnt_vfsstat.f_fsid.val[0] = tfsid.val[0];
	mp->mnt_vfsstat.f_fsid.val[1] = tfsid.val[1];
	mount_list_unlock();
}

  
                                                                
   
extern int(**dead_vnodeop_p)(void *);
long numvnodes, freevnodes, deadvnodes, async_work_vnodes;


int async_work_timed_out = 0;
int async_work_handled = 0;
int dead_vnode_wanted = 0;
int dead_vnode_waited = 0;

  
                                                
   
static void
insmntque(vnode_t vp, mount_t mp)
{
	mount_t lmp;
	  
                                                      
    
	if ((lmp = vp->v_mount) != NULL && lmp != dead_mountp) {
		if ((vp->v_lflag & VNAMED_MOUNT) == 0) {
			panic("insmntque: vp not in mount vnode list");
		}
		vp->v_lflag &= ~VNAMED_MOUNT;

		mount_lock_spin(lmp);

		mount_drop(lmp, 1);

		if (vp->v_mntvnodes.tqe_next == NULL) {
			if (TAILQ_LAST(&lmp->mnt_vnodelist, vnodelst) == vp) {
				TAILQ_REMOVE(&lmp->mnt_vnodelist, vp, v_mntvnodes);
			} else if (TAILQ_LAST(&lmp->mnt_newvnodes, vnodelst) == vp) {
				TAILQ_REMOVE(&lmp->mnt_newvnodes, vp, v_mntvnodes);
			} else if (TAILQ_LAST(&lmp->mnt_workerqueue, vnodelst) == vp) {
				TAILQ_REMOVE(&lmp->mnt_workerqueue, vp, v_mntvnodes);
			}
		} else {
			vp->v_mntvnodes.tqe_next->v_mntvnodes.tqe_prev = vp->v_mntvnodes.tqe_prev;
			*vp->v_mntvnodes.tqe_prev = vp->v_mntvnodes.tqe_next;
		}
		vp->v_mntvnodes.tqe_next = NULL;
		vp->v_mntvnodes.tqe_prev = NULL;
		mount_unlock(lmp);
		return;
	}

	  
                                                                     
    
	if ((vp->v_mount = mp) != NULL) {
		mount_lock_spin(mp);
		if ((vp->v_mntvnodes.tqe_next != 0) && (vp->v_mntvnodes.tqe_prev != 0)) {
			panic("vp already in mount list");
		}
		if (mp->mnt_lflag & MNT_LITER) {
			TAILQ_INSERT_HEAD(&mp->mnt_newvnodes, vp, v_mntvnodes);
		} else {
			TAILQ_INSERT_HEAD(&mp->mnt_vnodelist, vp, v_mntvnodes);
		}
		if (vp->v_lflag & VNAMED_MOUNT) {
			panic("insmntque: vp already in mount vnode list");
		}
		vp->v_lflag |= VNAMED_MOUNT;
		mount_ref(mp, 1);
		mount_unlock(mp);
	}
}


  
                                     
                                                    
                                                    
   
int
bdevvp(dev_t dev, vnode_t *vpp)
{
	vnode_t nvp;
	int     error;
	struct vnode_fsparam vfsp;
	struct vfs_context context;

	if (dev == NODEV) {
		*vpp = NULLVP;
		return ENODEV;
	}

	context.vc_thread = current_thread();
	context.vc_ucred = FSCRED;

	vfsp.vnfs_mp = (struct mount *)0;
	vfsp.vnfs_vtype = VBLK;
	vfsp.vnfs_str = "bdevvp";
	vfsp.vnfs_dvp = NULL;
	vfsp.vnfs_fsnode = NULL;
	vfsp.vnfs_cnp = NULL;
	vfsp.vnfs_vops = spec_vnodeop_p;
	vfsp.vnfs_rdev = dev;
	vfsp.vnfs_filesize = 0;

	vfsp.vnfs_flags = VNFS_NOCACHE | VNFS_CANTCACHE;

	vfsp.vnfs_marksystem = 0;
	vfsp.vnfs_markroot = 0;

	if ((error = vnode_create(VNCREATE_FLAVOR, VCREATESIZE, &vfsp, &nvp))) {
		*vpp = NULLVP;
		return error;
	}
	vnode_lock_spin(nvp);
	nvp->v_flag |= VBDEVVP;
	nvp->v_tag = VT_NON;                                                                  
	vnode_unlock(nvp);
	if ((error = vnode_ref(nvp))) {
		panic("bdevvp failed: vnode_ref");
		return error;
	}
	if ((error = VNOP_FSYNC(nvp, MNT_WAIT, &context))) {
		panic("bdevvp failed: fsync");
		return error;
	}
	if ((error = buf_invalidateblks(nvp, BUF_WRITE_DATA, 0, 0))) {
		panic("bdevvp failed: invalidateblks");
		return error;
	}

#if CONFIG_MACF
	  
                                                          
                             
    
#endif          

	if ((error = VNOP_OPEN(nvp, FREAD, &context))) {
		panic("bdevvp failed: open");
		return error;
	}
	*vpp = nvp;

	return 0;
}

  
                                                            
                                                       
                                                        
                                                              
                                                          
                                                              
   
static vnode_t
checkalias(struct vnode *nvp, dev_t nvp_rdev)
{
	struct vnode *vp;
	struct vnode **vpp;
	struct specinfo *sin = NULL;
	int vid = 0;

	vpp = &speclisth[SPECHASH(nvp_rdev)];
loop:
	SPECHASH_LOCK();

	for (vp = *vpp; vp; vp = vp->v_specnext) {
		if (nvp_rdev == vp->v_rdev && nvp->v_type == vp->v_type) {
			vid = vp->v_id;
			break;
		}
	}
	SPECHASH_UNLOCK();

	if (vp) {
found_alias:
		if (vnode_getwithvid(vp, vid)) {
			goto loop;
		}
		  
                                                     
     
		vnode_lock(vp);

		  
                                            
     
		if ((vp->v_iocount == 1) && (vp->v_usecount == 0)) {
			vnode_reclaim_internal(vp, 1, 1, 0);
			vnode_put_locked(vp);
			vnode_unlock(vp);
			goto loop;
		}
	}
	if (vp == NULL || vp->v_tag != VT_NON) {
		if (sin == NULL) {
			sin = zalloc_flags(specinfo_zone, Z_WAITOK | Z_ZERO);
		} else {
			bzero(sin, sizeof(struct specinfo));
		}

		nvp->v_specinfo = sin;
		nvp->v_rdev = nvp_rdev;
		nvp->v_specflags = 0;
		nvp->v_speclastr = -1;
		nvp->v_specinfo->si_opencount = 0;
		nvp->v_specinfo->si_initted = 0;
		nvp->v_specinfo->si_throttleable = 0;

		SPECHASH_LOCK();

		                                                   
		if (vp == NULLVP) {
			for (vp = *vpp; vp; vp = vp->v_specnext) {
				if (nvp_rdev == vp->v_rdev && nvp->v_type == vp->v_type) {
					vid = vp->v_id;
					SPECHASH_UNLOCK();
					goto found_alias;
				}
			}
		}

		nvp->v_hashchain = vpp;
		nvp->v_specnext = *vpp;
		*vpp = nvp;

		if (vp != NULLVP) {
			nvp->v_specflags |= SI_ALIASED;
			vp->v_specflags |= SI_ALIASED;
			SPECHASH_UNLOCK();
			vnode_put_locked(vp);
			vnode_unlock(vp);
		} else {
			SPECHASH_UNLOCK();
		}

		return NULLVP;
	}

	if (sin) {
		zfree(specinfo_zone, sin);
	}

	if ((vp->v_flag & (VBDEVVP | VDEVFLUSH)) != 0) {
		return vp;
	}

	panic("checkalias with VT_NON vp that shouldn't: %p", vp);

	return vp;
}


  
                                                                  
                                                                  
                                                                 
                                      
                                                                
                                                                   
                                                                
                                                                   
   
int
vget_internal(vnode_t vp, int vid, int vflags)
{
	int error = 0;

	vnode_lock_spin(vp);

	if ((vflags & VNODE_WRITEABLE) && (vp->v_writecount == 0)) {
		  
                                                       
     
		error = EINVAL;
	} else {
		error = vnode_getiocount(vp, vid, vflags);
	}

	vnode_unlock(vp);

	return error;
}

  
                       
                                                    
   
int
vnode_ref(vnode_t vp)
{
	return vnode_ref_ext(vp, 0, 0);
}

  
                       
                                                    
   
int
vnode_ref_ext(vnode_t vp, int fmode, int flags)
{
	int     error = 0;

	vnode_lock_spin(vp);

	  
                                                                       
                                                                       
                                                                         
    
	if (vp->v_iocount <= 0 && vp->v_usecount <= 0) {
		panic("vnode_ref_ext: vp %p has no valid reference %d, %d", vp, vp->v_iocount, vp->v_usecount);
	}

	  
                                                                   
    
	if ((flags & VNODE_REF_FORCE) == 0) {
		if ((vp->v_lflag & (VL_DRAIN | VL_TERMINATE | VL_DEAD))) {
			if (vp->v_owner != current_thread()) {
				error = ENOENT;
				goto out;
			}
		}
	}

	                                                            
	os_atomic_inc(&vp->v_usecount, relaxed);

	if (fmode & FWRITE) {
		if (++vp->v_writecount <= 0) {
			panic("vnode_ref_ext: v_writecount");
		}
	}
	if (fmode & O_EVTONLY) {
		if (++vp->v_kusecount <= 0) {
			panic("vnode_ref_ext: v_kusecount");
		}
	}
	if (vp->v_flag & VRAGE) {
		struct  uthread *ut;

		ut = get_bsdthread_info(current_thread());

		if (!(current_proc()->p_lflag & P_LRAGE_VNODES) &&
		    !(ut->uu_flag & UT_RAGE_VNODES)) {
			  
                                            
                                       
                                         
                                           
                                         
                                            
      
			vp->v_flag &= ~VRAGE;
			vp->v_references = 0;
			vnode_list_remove(vp);
		}
	}
	if (vp->v_usecount == 1 && vp->v_type == VREG && !(vp->v_flag & VSYSTEM)) {
		if (vp->v_ubcinfo) {
			vnode_lock_convert(vp);
			memory_object_mark_used(vp->v_ubcinfo->ui_control);
		}
	}
out:
	vnode_unlock(vp);

	return error;
}


boolean_t
vnode_on_reliable_media(vnode_t vp)
{
	mount_t mp = vp->v_mount;

	  
                                                                        
                                                                         
                                                                            
                                                                     
            
    
	if (!mp || (!(mp->mnt_kern_flag & MNTK_VIRTUALDEV) && (mp->mnt_flag & MNT_LOCAL))) {
		return TRUE;
	}

	return FALSE;
}

static void
vnode_async_list_add_locked(vnode_t vp)
{
	if (VONLIST(vp) || (vp->v_lflag & (VL_TERMINATE | VL_DEAD))) {
		panic("vnode_async_list_add: %p is in wrong state", vp);
	}

	TAILQ_INSERT_HEAD(&vnode_async_work_list, vp, v_freelist);
	vp->v_listflag |= VLIST_ASYNC_WORK;

	async_work_vnodes++;
}

static void
vnode_async_list_add(vnode_t vp)
{
	vnode_list_lock();

	vnode_async_list_add_locked(vp);

	vnode_list_unlock();

	wakeup(&vnode_async_work_list);
}


  
                                          
                           
   
static void
vnode_list_add(vnode_t vp)
{
	boolean_t need_dead_wakeup = FALSE;

#if DIAGNOSTIC
	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);
#endif

again:

	  
                                                            
    
	if (VONLIST(vp) || (vp->v_usecount != 0) || (vp->v_iocount != 0) || (vp->v_lflag & VL_TERMINATE)) {
		return;
	}

	  
                                                             
                                                              
                                      
    
	if (ISSET(vp->v_lflag, VL_DEAD)
	    && (!LIST_EMPTY(&vp->v_cleanblkhd) || !LIST_EMPTY(&vp->v_dirtyblkhd))) {
		++vp->v_iocount;                                               
#ifdef JOE_DEBUG
		record_vp(vp, 1);
#endif
		vnode_unlock(vp);
		buf_invalidateblks(vp, BUF_INVALIDATE_LOCKED, 0, 0);
		vnode_lock(vp);
		vnode_dropiocount(vp);
		goto again;
	}

	vnode_list_lock();

	if ((vp->v_flag & VRAGE) && !(vp->v_lflag & VL_DEAD)) {
		  
                                                            
     
		if ((vp->v_flag & VAGE)) {
			TAILQ_INSERT_HEAD(&vnode_rage_list, vp, v_freelist);
		} else {
			TAILQ_INSERT_TAIL(&vnode_rage_list, vp, v_freelist);
		}

		vp->v_listflag |= VLIST_RAGE;
		ragevnodes++;

		  
                                                             
                                                                  
                                                                    
                                                                      
                                                                     
                                           
     
		microuptime(&rage_tv);
	} else {
		  
                                                   
                                                              
     
		if ((vp->v_lflag & VL_DEAD)) {
			TAILQ_INSERT_HEAD(&vnode_dead_list, vp, v_freelist);
			vp->v_listflag |= VLIST_DEAD;
			deadvnodes++;

			if (dead_vnode_wanted) {
				dead_vnode_wanted--;
				need_dead_wakeup = TRUE;
			}
		} else if ((vp->v_flag & VAGE)) {
			TAILQ_INSERT_HEAD(&vnode_free_list, vp, v_freelist);
			vp->v_flag &= ~VAGE;
			freevnodes++;
		} else {
			TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
			freevnodes++;
		}
	}
	vnode_list_unlock();

	if (need_dead_wakeup == TRUE) {
		wakeup_one((caddr_t)&dead_vnode_wanted);
	}
}


  
                                               
                               
                     
   
static void
vnode_list_remove_locked(vnode_t vp)
{
	if (VONLIST(vp)) {
		  
                            
                                     
     
		if (vp->v_listflag & VLIST_RAGE) {
			VREMRAGE("vnode_list_remove", vp);
		} else if (vp->v_listflag & VLIST_DEAD) {
			VREMDEAD("vnode_list_remove", vp);
		} else if (vp->v_listflag & VLIST_ASYNC_WORK) {
			VREMASYNC_WORK("vnode_list_remove", vp);
		} else {
			VREMFREE("vnode_list_remove", vp);
		}
	}
}


  
                                               
                           
   
static void
vnode_list_remove(vnode_t vp)
{
#if DIAGNOSTIC
	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);
#endif
	  
                                         
                                           
                                      
                                              
   
                                          
                                        
                                         
                                        
                                     
                                                
                                
    
	if (VONLIST(vp)) {
		vnode_list_lock();
		  
                                       
                                       
                                      
                                     
                                          
                                          
                                        
                                        
                     
     
		vnode_list_remove_locked(vp);

		vnode_list_unlock();
	}
}


void
vnode_rele(vnode_t vp)
{
	vnode_rele_internal(vp, 0, 0, 0);
}


void
vnode_rele_ext(vnode_t vp, int fmode, int dont_reenter)
{
	vnode_rele_internal(vp, fmode, dont_reenter, 0);
}


void
vnode_rele_internal(vnode_t vp, int fmode, int dont_reenter, int locked)
{
	int32_t old_usecount;

	if (!locked) {
		vnode_lock_spin(vp);
	}
#if DIAGNOSTIC
	else {
		lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);
	}
#endif
	                                                            
	old_usecount = os_atomic_dec_orig(&vp->v_usecount, relaxed);
	if (old_usecount < 1) {
		  
                                                                   
                                                            
                                                              
                                                               
     
		panic("vnode_rele_ext: vp %p usecount -ve : %d.  v_tag = %d, v_type = %d, v_flag = %x.",
		    vp, old_usecount - 1, vp->v_tag, vp->v_type, vp->v_flag);
	}

	if (fmode & FWRITE) {
		if (--vp->v_writecount < 0) {
			panic("vnode_rele_ext: vp %p writecount -ve : %d.  v_tag = %d, v_type = %d, v_flag = %x.", vp, vp->v_writecount, vp->v_tag, vp->v_type, vp->v_flag);
		}
	}
	if (fmode & O_EVTONLY) {
		if (--vp->v_kusecount < 0) {
			panic("vnode_rele_ext: vp %p kusecount -ve : %d.  v_tag = %d, v_type = %d, v_flag = %x.", vp, vp->v_kusecount, vp->v_tag, vp->v_type, vp->v_flag);
		}
	}
	if (vp->v_kusecount > vp->v_usecount) {
		panic("vnode_rele_ext: vp %p kusecount(%d) out of balance with usecount(%d).  v_tag = %d, v_type = %d, v_flag = %x.", vp, vp->v_kusecount, vp->v_usecount, vp->v_tag, vp->v_type, vp->v_flag);
	}

	if ((vp->v_iocount > 0) || (vp->v_usecount > 0)) {
		  
                                             
                                                      
                                        
     
		if (vp->v_usecount == 0) {
			vp->v_lflag |= VL_NEEDINACTIVE;
			vp->v_flag  &= ~(VNOCACHE_DATA | VRAOFF | VOPENEVT);
		}
		goto done;
	}
	vp->v_flag  &= ~(VNOCACHE_DATA | VRAOFF | VOPENEVT);

	if (ISSET(vp->v_lflag, VL_TERMINATE | VL_DEAD) || dont_reenter) {
		  
                               
                                          
                                        
                                               
     
		if (dont_reenter) {
			if (!(vp->v_lflag & (VL_TERMINATE | VL_DEAD | VL_MARKTERM))) {
				vp->v_lflag |= VL_NEEDINACTIVE;

				if (vnode_on_reliable_media(vp) == FALSE || vp->v_flag & VISDIRTY) {
					vnode_async_list_add(vp);
					goto done;
				}
			}
			vp->v_flag |= VAGE;
		}
		vnode_list_add(vp);

		goto done;
	}
	  
                                               
            
                                          
                                            
    
	vp->v_iocount++;
#ifdef JOE_DEBUG
	record_vp(vp, 1);
#endif
	vp->v_lflag &= ~VL_NEEDINACTIVE;
	vnode_unlock(vp);

	VNOP_INACTIVE(vp, vfs_context_current());

	vnode_lock_spin(vp);
	  
                                                           
                                                          
                                                           
                                                          
                                                         
                                                            
                                                            
    
	if ((vp->v_iocount == 1) && (vp->v_usecount == 0) &&
	    ((vp->v_lflag & (VL_MARKTERM | VL_TERMINATE | VL_DEAD)) == VL_MARKTERM)) {
		struct  uthread *ut;

		ut = get_bsdthread_info(current_thread());

		if (ut->uu_defer_reclaims) {
			vp->v_defer_reclaimlist = ut->uu_vreclaims;
			ut->uu_vreclaims = vp;
			goto done;
		}
		vnode_lock_convert(vp);
		vnode_reclaim_internal(vp, 1, 1, 0);
	}
	vnode_dropiocount(vp);
	vnode_list_add(vp);
done:
	if (vp->v_usecount == 0 && vp->v_type == VREG && !(vp->v_flag & VSYSTEM)) {
		if (vp->v_ubcinfo) {
			vnode_lock_convert(vp);
			memory_object_mark_unused(vp->v_ubcinfo->ui_control, (vp->v_flag & VRAGE) == VRAGE);
		}
	}
	if (!locked) {
		vnode_unlock(vp);
	}
	return;
}

  
                                                                    
  
                                                                    
                                                                 
                                                                     
                  
   

int
vflush(struct mount *mp, struct vnode *skipvp, int flags)
{
	struct vnode *vp;
	int busy = 0;
	int reclaimed = 0;
	int retval;
	unsigned int vid;
	bool first_try = true;

	  
                                                                   
    
	mount_iterate_lock(mp);

	mount_lock(mp);
	vnode_iterate_setup(mp);
	  
                                        
                                             
                                                
                                                  
                            
    
	if (((flags & FORCECLOSE) == 0) && ((mp->mnt_kern_flag & MNTK_UNMOUNT_PREFLIGHT) != 0)) {
		if (vnode_umount_preflight(mp, skipvp, flags)) {
			vnode_iterate_clear(mp);
			mount_unlock(mp);
			mount_iterate_unlock(mp);
			return EBUSY;
		}
	}
loop:
	                                                 
	retval = vnode_iterate_prepare(mp);

	if (retval == 0) {
		vnode_iterate_clear(mp);
		mount_unlock(mp);
		mount_iterate_unlock(mp);
		return retval;
	}

	                                 
	while (!TAILQ_EMPTY(&mp->mnt_workerqueue)) {
		vp = TAILQ_FIRST(&mp->mnt_workerqueue);
		TAILQ_REMOVE(&mp->mnt_workerqueue, vp, v_mntvnodes);
		TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vp, v_mntvnodes);

		if ((vp->v_mount != mp) || (vp == skipvp)) {
			continue;
		}
		vid = vp->v_id;
		mount_unlock(mp);

		vnode_lock_spin(vp);

		                                                  
		while (vp->v_id == vid && ISSET(vp->v_lflag, VL_TERMINATE)) {
			vp->v_lflag |= VL_TERMWANT;
			msleep(&vp->v_lflag, &vp->v_lock, PVFS, "vflush", NULL);
		}

		if ((vp->v_id != vid) || ISSET(vp->v_lflag, VL_DEAD)) {
			vnode_unlock(vp);
			mount_lock(mp);
			continue;
		}

		  
                                                   
                                          
     
		if ((flags & SKIPSYSTEM) && ((vp->v_flag & VSYSTEM) ||
		    (vp->v_flag & VNOFLUSH))) {
			vnode_unlock(vp);
			mount_lock(mp);
			continue;
		}
		  
                                                 
     
		if ((flags & SKIPSWAP) && (vp->v_flag & VSWAP)) {
			vnode_unlock(vp);
			mount_lock(mp);
			continue;
		}
		  
                                                 
     
		if ((flags & SKIPROOT) && (vp->v_flag & VROOT)) {
			vnode_unlock(vp);
			mount_lock(mp);
			continue;
		}
		  
                                                      
                             
     
		if ((flags & WRITECLOSE) &&
		    (vp->v_writecount == 0 || vp->v_type != VREG)) {
			vnode_unlock(vp);
			mount_lock(mp);
			continue;
		}
		  
                                                          
                                                   
     
		if (((vp->v_usecount == 0) ||
		    ((vp->v_usecount - vp->v_kusecount) == 0))) {
			vnode_lock_convert(vp);
			vp->v_iocount++;                                                      
#ifdef JOE_DEBUG
			record_vp(vp, 1);
#endif
			vnode_reclaim_internal(vp, 1, 1, 0);
			vnode_dropiocount(vp);
			vnode_list_add(vp);
			vnode_unlock(vp);

			reclaimed++;
			mount_lock(mp);
			continue;
		}
		  
                                                    
                                                 
                                                           
     
		if (flags & FORCECLOSE) {
			vnode_lock_convert(vp);

			if (vp->v_type != VBLK && vp->v_type != VCHR) {
				vp->v_iocount++;                                                      
#ifdef JOE_DEBUG
				record_vp(vp, 1);
#endif
				vnode_abort_advlocks(vp);
				vnode_reclaim_internal(vp, 1, 1, 0);
				vnode_dropiocount(vp);
				vnode_list_add(vp);
				vnode_unlock(vp);
			} else {
				vclean(vp, 0);
				vp->v_lflag &= ~VL_DEAD;
				vp->v_op = spec_vnodeop_p;
				vp->v_flag |= VDEVFLUSH;
				vnode_unlock(vp);
			}
			mount_lock(mp);
			continue;
		}

		                                           
		if (print_busy_vnodes && first_try && ((flags & FORCECLOSE) == 0)) {
			vprint("vflush - busy vnode", vp);
		}

		vnode_unlock(vp);
		mount_lock(mp);
		busy++;
	}

	                                                 
	if (busy && ((flags & FORCECLOSE) == 0) && reclaimed) {
		busy = 0;
		reclaimed = 0;
		(void)vnode_iterate_reloadq(mp);
		first_try = false;
		                                   
		goto loop;
	}

	                                                             
	if (vnode_iterate_reloadq(mp) != 0) {
		if (!(busy && ((flags & FORCECLOSE) == 0))) {
			first_try = false;
			goto loop;
		}
	}
	vnode_iterate_clear(mp);
	mount_unlock(mp);
	mount_iterate_unlock(mp);

	if (busy && ((flags & FORCECLOSE) == 0)) {
		return EBUSY;
	}
	return 0;
}

long num_recycledvnodes = 0;
  
                                                        
                                   
   
static void
vclean(vnode_t vp, int flags)
{
	vfs_context_t ctx = vfs_context_current();
	int active;
	int need_inactive;
	int already_terminating;
	int clflags = 0;
#if NAMEDSTREAMS
	int is_namedstream;
#endif

	  
                                        
                                                        
                                                        
                                         
    
	active = vp->v_usecount;

	  
                                           
                                  
    
	need_inactive = (vp->v_lflag & VL_NEEDINACTIVE);

	vp->v_lflag &= ~VL_NEEDINACTIVE;

	  
                                            
                                           
    
	already_terminating = (vp->v_lflag & VL_TERMINATE);

	vp->v_lflag |= VL_TERMINATE;

#if NAMEDSTREAMS
	is_namedstream = vnode_isnamedstream(vp);
#endif

	vnode_unlock(vp);

	OSAddAtomicLong(1, &num_recycledvnodes);

	if (flags & DOCLOSE) {
		clflags |= IO_NDELAY;
	}
	if (flags & REVOKEALL) {
		clflags |= IO_REVOKE;
	}

#if CONFIG_MACF
	if (vp->v_mount) {
		  
                                                         
                                                       
               
     
		mac_vnode_notify_reclaim(vp);
	}
#endif

	if (active && (flags & DOCLOSE)) {
		VNOP_CLOSE(vp, clflags, ctx);
	}

	  
                                                    
    
	if (flags & DOCLOSE) {
#if CONFIG_NFS_CLIENT
		if (vp->v_tag == VT_NFS) {
			nfs_vinvalbuf(vp, V_SAVE, ctx, 0);
		} else
#endif                        
		{
			VNOP_FSYNC(vp, MNT_WAIT, ctx);

			  
                                                      
                                                         
                                                            
                                                      
                                              
      
			buf_invalidateblks(vp, BUF_WRITE_DATA | (active ? 0 : BUF_INVALIDATE_LOCKED), 0, 0);
		}
		if (UBCINFOEXISTS(vp)) {
			  
                            
      
			(void)ubc_msync(vp, (off_t)0, ubc_getsize(vp), NULL, UBC_PUSHALL | UBC_INVALIDATE | UBC_SYNC);
		}
	}
	if (active || need_inactive) {
		VNOP_INACTIVE(vp, ctx);
	}

#if NAMEDSTREAMS
	if ((is_namedstream != 0) && (vp->v_parent != NULLVP)) {
		vnode_t pvp = vp->v_parent;

		                                                               
		if (vnode_isshadow(vp)) {
			vnode_relenamedstream(pvp, vp);
		}

		  
                                                    
                                                 
                                                               
                                             
     
		if (vfs_authopaque(pvp->v_mount)) {
			vnode_lock_spin(pvp);
			pvp->v_lflag &= ~VL_HASSTREAMS;
			vnode_unlock(pvp);
		}
	}
#endif

	  
                               
                                        
                                                  
                                                          
                                                        
    
	ubc_destroy_named(vp);

#if CONFIG_TRIGGERS
	  
                                            
    
	if (vp->v_resolve) {
		vnode_resolver_detach(vp);
	}
#endif

#if CONFIG_IO_COMPRESSION_STATS
	if ((vp->io_compression_stats)) {
		vnode_iocs_record_and_free(vp);
	}
#endif                                  

	  
                      
    
	if (VNOP_RECLAIM(vp, ctx)) {
		panic("vclean: cannot reclaim");
	}

	                                                    
	vnode_update_identity(vp, NULLVP, NULL, 0, 0, VNODE_UPDATE_PARENT | VNODE_UPDATE_NAME | VNODE_UPDATE_PURGE | VNODE_UPDATE_PURGEFIRMLINK);

	vnode_lock(vp);

	  
                                                                   
                                                                 
                                                                   
         
    
	insmntque(vp, (struct mount *)0);

	vp->v_mount = dead_mountp;
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	vp->v_data = NULL;

	vp->v_lflag |= VL_DEAD;
	vp->v_flag &= ~VISDIRTY;

	if (already_terminating == 0) {
		vp->v_lflag &= ~VL_TERMINATE;
		  
                                                       
     
		if (vp->v_lflag & VL_TERMWANT) {
			vp->v_lflag &= ~VL_TERMWANT;
			wakeup(&vp->v_lflag);
		}
	}
}

  
                                                              
                                                      
   
int
#if DIAGNOSTIC
vn_revoke(vnode_t vp, int flags, __unused vfs_context_t a_context)
#else
vn_revoke(vnode_t vp, __unused int flags, __unused vfs_context_t a_context)
#endif
{
	struct vnode *vq;
	int vid;

#if DIAGNOSTIC
	if ((flags & REVOKEALL) == 0) {
		panic("vnop_revoke");
	}
#endif

	if (vnode_isaliased(vp)) {
		  
                                                   
                              
     
		if (vp->v_lflag & VL_TERMINATE) {
			return ENOENT;
		}

		  
                                                
                                 
     
		SPECHASH_LOCK();
		while ((vp->v_specflags & SI_ALIASED)) {
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_rdev != vp->v_rdev ||
				    vq->v_type != vp->v_type || vp == vq) {
					continue;
				}
				vid = vq->v_id;
				SPECHASH_UNLOCK();
				if (vnode_getwithvid(vq, vid)) {
					SPECHASH_LOCK();
					break;
				}
				vnode_lock(vq);
				if (!(vq->v_lflag & VL_TERMINATE)) {
					vnode_reclaim_internal(vq, 1, 1, 0);
				}
				vnode_put_locked(vq);
				vnode_unlock(vq);
				SPECHASH_LOCK();
				break;
			}
		}
		SPECHASH_UNLOCK();
	}
	vnode_lock(vp);
	if (vp->v_lflag & VL_TERMINATE) {
		vnode_unlock(vp);
		return ENOENT;
	}
	vnode_reclaim_internal(vp, 1, 0, REVOKEALL);
	vnode_unlock(vp);

	return 0;
}

  
                                                         
                                                              
   
int
vnode_recycle(struct vnode *vp)
{
	vnode_lock_spin(vp);

	if (vp->v_iocount || vp->v_usecount) {
		vp->v_lflag |= VL_MARKTERM;
		vnode_unlock(vp);
		return 0;
	}
	vnode_lock_convert(vp);
	vnode_reclaim_internal(vp, 1, 0, 0);

	vnode_unlock(vp);

	return 1;
}

static int
vnode_reload(vnode_t vp)
{
	vnode_lock_spin(vp);

	if ((vp->v_iocount > 1) || vp->v_usecount) {
		vnode_unlock(vp);
		return 0;
	}
	if (vp->v_iocount <= 0) {
		panic("vnode_reload with no iocount %d", vp->v_iocount);
	}

	                                             
	vp->v_lflag |= VL_MARKTERM;
	vnode_unlock(vp);

	return 1;
}


static void
vgone(vnode_t vp, int flags)
{
	struct vnode *vq;
	struct vnode *vx;

	  
                                           
                                          
                                            
    
	vclean(vp, flags | DOCLOSE);

	  
                                                               
                    
    
	if ((vp->v_type == VBLK || vp->v_type == VCHR) && vp->v_specinfo != 0) {
		SPECHASH_LOCK();
		if (*vp->v_hashchain == vp) {
			*vp->v_hashchain = vp->v_specnext;
		} else {
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_specnext != vp) {
					continue;
				}
				vq->v_specnext = vp->v_specnext;
				break;
			}
			if (vq == NULL) {
				panic("missing bdev");
			}
		}
		if (vp->v_specflags & SI_ALIASED) {
			vx = NULL;
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_rdev != vp->v_rdev ||
				    vq->v_type != vp->v_type) {
					continue;
				}
				if (vx) {
					break;
				}
				vx = vq;
			}
			if (vx == NULL) {
				panic("missing alias");
			}
			if (vq == NULL) {
				vx->v_specflags &= ~SI_ALIASED;
			}
			vp->v_specflags &= ~SI_ALIASED;
		}
		SPECHASH_UNLOCK();
		{
			struct specinfo *tmp = vp->v_specinfo;
			vp->v_specinfo = NULL;
			zfree(specinfo_zone, tmp);
		}
	}
}

  
                                   
   
int
check_mountedon(dev_t dev, enum vtype type, int  *errorp)
{
	vnode_t vp;
	int rc = 0;
	int vid;

loop:
	SPECHASH_LOCK();
	for (vp = speclisth[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (dev != vp->v_rdev || type != vp->v_type) {
			continue;
		}
		vid = vp->v_id;
		SPECHASH_UNLOCK();
		if (vnode_getwithvid(vp, vid)) {
			goto loop;
		}
		vnode_lock_spin(vp);
		if ((vp->v_usecount > 0) || (vp->v_iocount > 1)) {
			vnode_unlock(vp);
			if ((*errorp = vfs_mountedon(vp)) != 0) {
				rc = 1;
			}
		} else {
			vnode_unlock(vp);
		}
		vnode_put(vp);
		return rc;
	}
	SPECHASH_UNLOCK();
	return 0;
}

  
                                                                
   
int
vcount(vnode_t vp)
{
	vnode_t vq, vnext;
	int count;
	int vid;

	if (!vnode_isspec(vp)) {
		return vp->v_usecount - vp->v_kusecount;
	}

loop:
	if (!vnode_isaliased(vp)) {
		return vp->v_specinfo->si_opencount;
	}
	count = 0;

	SPECHASH_LOCK();
	  
                                 
    
	vq = *vp->v_hashchain;
	vid = vq ? vq->v_id : 0;

	SPECHASH_UNLOCK();

	while (vq) {
		  
                                                        
     
		if (vnode_getwithvid(vq, vid)) {
			goto loop;
		}
		vnode_lock(vq);

		if (vq->v_rdev == vp->v_rdev && vq->v_type == vp->v_type) {
			if ((vq->v_usecount == 0) && (vq->v_iocount == 1) && vq != vp) {
				  
                                              
       
				vnode_reclaim_internal(vq, 1, 1, 0);
				vnode_put_locked(vq);
				vnode_unlock(vq);
				goto loop;
			}
			count += vq->v_specinfo->si_opencount;
		}
		vnode_unlock(vq);

		SPECHASH_LOCK();
		  
                                                       
                                                     
                       
     
		vnext = vq->v_specnext;
		vid = vnext ? vnext->v_id : 0;

		SPECHASH_UNLOCK();

		vnode_put(vq);

		vq = vnext;
	}

	return count;
}

int     prtactive = 0;                                                       

  
                                      
   
static const char *typename[] =
{ "VNON", "VREG", "VDIR", "VBLK", "VCHR", "VLNK", "VSOCK", "VFIFO", "VBAD" };

void
vprint(const char *label, struct vnode *vp)
{
	char sbuf[64];

	if (label != NULL) {
		printf("%s: ", label);
	}
	printf("name %s type %s, usecount %d, writecount %d\n",
	    vp->v_name, typename[vp->v_type],
	    vp->v_usecount, vp->v_writecount);
	sbuf[0] = '\0';
	if (vp->v_flag & VROOT) {
		strlcat(sbuf, "|VROOT", sizeof(sbuf));
	}
	if (vp->v_flag & VTEXT) {
		strlcat(sbuf, "|VTEXT", sizeof(sbuf));
	}
	if (vp->v_flag & VSYSTEM) {
		strlcat(sbuf, "|VSYSTEM", sizeof(sbuf));
	}
	if (vp->v_flag & VNOFLUSH) {
		strlcat(sbuf, "|VNOFLUSH", sizeof(sbuf));
	}
	if (vp->v_flag & VBWAIT) {
		strlcat(sbuf, "|VBWAIT", sizeof(sbuf));
	}
	if (vnode_isaliased(vp)) {
		strlcat(sbuf, "|VALIASED", sizeof(sbuf));
	}
	if (sbuf[0] != '\0') {
		printf("vnode flags (%s\n", &sbuf[1]);
	}
}


int
vn_getpath(struct vnode *vp, char *pathbuf, int *len)
{
	return build_path(vp, pathbuf, *len, len, BUILDPATH_NO_FS_ENTER, vfs_context_current());
}

int
vn_getpath_fsenter(struct vnode *vp, char *pathbuf, int *len)
{
	return build_path(vp, pathbuf, *len, len, 0, vfs_context_current());
}

  
                                                                                      
                                                                                          
  
                                                                                         
                                                                                              
                                                                                        
                                                                                           
                                                                                         
                                                                                           
                                                                                   
                                                                                      
                                                                                       
                                                                                       
                                                                                          
                                                                                  
                                                                                          
                                                                                      
                                                                                        
                                                                                  
                                                                                  
  
                                                                                          
                                                                                      
                                                                                       
  
                                                                                         
                                                                     
   

int
vn_getpath_fsenter_with_parent(struct vnode *dvp, struct vnode *vp, char *pathbuf, int *len)
{
	return build_path_with_parent(vp, dvp, pathbuf, *len, len, NULL, 0, vfs_context_current());
}

int
vn_getpath_ext(struct vnode *vp, struct vnode *dvp, char *pathbuf, int *len, int flags)
{
	int bpflags = (flags & VN_GETPATH_FSENTER) ? 0 : BUILDPATH_NO_FS_ENTER;

	if (flags && (flags != VN_GETPATH_FSENTER)) {
		if (flags & VN_GETPATH_NO_FIRMLINK) {
			bpflags |= BUILDPATH_NO_FIRMLINK;;
		}
		if (flags & VN_GETPATH_VOLUME_RELATIVE) {
			bpflags |= (BUILDPATH_VOLUME_RELATIVE | BUILDPATH_NO_FIRMLINK);
		}
		if (flags & VN_GETPATH_NO_PROCROOT) {
			bpflags |= BUILDPATH_NO_PROCROOT;
		}
	}

	return build_path_with_parent(vp, dvp, pathbuf, *len, len, NULL, bpflags, vfs_context_current());
}

int
vn_getpath_no_firmlink(struct vnode *vp, char *pathbuf, int *len)
{
	return vn_getpath_ext(vp, NULLVP, pathbuf, len, VN_GETPATH_NO_FIRMLINK);
}

int
vn_getpath_ext_with_mntlen(struct vnode *vp, struct vnode *dvp, char *pathbuf, size_t *len, size_t *mntlen, int flags)
{
	int bpflags = (flags & VN_GETPATH_FSENTER) ? 0 : BUILDPATH_NO_FS_ENTER;
	int local_len;
	int error;

	if (*len > INT_MAX) {
		return EINVAL;
	}

	local_len = *len;

	if (flags && (flags != VN_GETPATH_FSENTER)) {
		if (flags & VN_GETPATH_NO_FIRMLINK) {
			bpflags |= BUILDPATH_NO_FIRMLINK;;
		}
		if (flags & VN_GETPATH_VOLUME_RELATIVE) {
			bpflags |= (BUILDPATH_VOLUME_RELATIVE | BUILDPATH_NO_FIRMLINK);
		}
		if (flags & VN_GETPATH_NO_PROCROOT) {
			bpflags |= BUILDPATH_NO_PROCROOT;
		}
	}

	error = build_path_with_parent(vp, dvp, pathbuf, local_len, &local_len, mntlen, bpflags, vfs_context_current());

	if (local_len >= 0 && local_len <= (int)*len) {
		*len = (size_t)local_len;
	}

	return error;
}

int
vn_getcdhash(struct vnode *vp, off_t offset, unsigned char *cdhash)
{
	return ubc_cs_getcdhash(vp, offset, cdhash);
}


static char *extension_table = NULL;
static int   nexts;
static int   max_ext_width;

static int
extension_cmp(const void *a, const void *b)
{
	return (int)(strlen((const char *)a) - strlen((const char *)b));
}


  
                                                           
                                            
  
                                                          
                                                         
                                                          
                                                          
                                                         
                                                      
                        
  
extern lck_mtx_t pkg_extensions_lck;

__private_extern__ int
set_package_extensions_table(user_addr_t data, int nentries, int maxwidth)
{
	char *new_exts, *old_exts;
	int old_nentries = 0, old_maxwidth = 0;
	int error;

	if (nentries <= 0 || nentries > 1024 || maxwidth <= 0 || maxwidth > 255) {
		return EINVAL;
	}


	                                                               
	new_exts = kheap_alloc(KHEAP_DATA_BUFFERS, (nentries * maxwidth) + 1,
	    Z_WAITOK);
	if (new_exts == NULL) {
		return ENOMEM;
	}

	error = copyin(data, new_exts, nentries * maxwidth);
	if (error) {
		kheap_free(KHEAP_DATA_BUFFERS, new_exts, (nentries * maxwidth) + 1);
		return error;
	}

	new_exts[(nentries * maxwidth)] = '\0';                                           

	qsort(new_exts, nentries, maxwidth, extension_cmp);

	lck_mtx_lock(&pkg_extensions_lck);

	old_exts        = extension_table;
	old_nentries    = nexts;
	old_maxwidth    = max_ext_width;
	extension_table = new_exts;
	nexts           = nentries;
	max_ext_width   = maxwidth;

	lck_mtx_unlock(&pkg_extensions_lck);

	kheap_free(KHEAP_DATA_BUFFERS, old_exts,
	    (old_nentries * old_maxwidth) + 1);

	return 0;
}


int
is_package_name(const char *name, int len)
{
	int i;
	size_t extlen;
	const char *ptr, *name_ext;

	                                                      
	                                                       
	                 
	if (len <= 3 || name[0] == '.') {
		return 0;
	}

	name_ext = NULL;
	for (ptr = name; *ptr != '\0'; ptr++) {
		if (*ptr == '.') {
			name_ext = ptr;
		}
	}

	                                               
	if (name_ext == NULL) {
		return 0;
	}

	                       
	name_ext++;

	lck_mtx_lock(&pkg_extensions_lck);

	                                                          
	ptr = &extension_table[0];
	for (i = 0; i < nexts; i++, ptr += max_ext_width) {
		extlen = strlen(ptr);
		if (strncasecmp(name_ext, ptr, extlen) == 0 && name_ext[extlen] == '\0') {
			                
			lck_mtx_unlock(&pkg_extensions_lck);
			return 1;
		}
	}

	lck_mtx_unlock(&pkg_extensions_lck);

	                                       
	return 0;
}

int
vn_path_package_check(__unused vnode_t vp, char *path, int pathlen, int *component)
{
	char *ptr, *end;
	int comp = 0;

	if (pathlen < 0) {
		return EINVAL;
	}

	*component = -1;
	if (*path != '/') {
		return EINVAL;
	}

	end = path + 1;
	while (end < path + pathlen && *end != '\0') {
		while (end < path + pathlen && *end == '/' && *end != '\0') {
			end++;
		}

		ptr = end;

		while (end < path + pathlen && *end != '/' && *end != '\0') {
			end++;
		}

		if (end > path + pathlen) {
			                                     
			return EINVAL;
		}

		*end = '\0';
		if (is_package_name(ptr, (int)(end - ptr))) {
			*component = comp;
			break;
		}

		end++;
		comp++;
	}

	return 0;
}

  
                                                             
                                           
   

int
vn_searchfs_inappropriate_name(const char *name, int len)
{
	const char *bad_names[] = { "System" };
	int   bad_len[]   = { 6 };
	int  i;

	if (len < 0) {
		return EINVAL;
	}

	for (i = 0; i < (int) (sizeof(bad_names) / sizeof(bad_names[0])); i++) {
		if (len == bad_len[i] && strncmp(name, bad_names[i], strlen(bad_names[i]) + 1) == 0) {
			return 1;
		}
	}

	                                  
	return 0;
}

  
                                                      
   
extern unsigned int vfs_nummntops;

  
                                                  
                                                        
                                                            
  
                       
                           
                             
   
SYSCTL_INT(_vfs, VFS_NUMMNTOPS, nummntops,
    CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED,
    &vfs_nummntops, 0, "");

int
vfs_sysctl(int *name __unused, u_int namelen __unused,
    user_addr_t oldp __unused, size_t *oldlenp __unused,
    user_addr_t newp __unused, size_t newlen __unused, proc_t p __unused);

int
vfs_sysctl(int *name __unused, u_int namelen __unused,
    user_addr_t oldp __unused, size_t *oldlenp __unused,
    user_addr_t newp __unused, size_t newlen __unused, proc_t p __unused)
{
	return EINVAL;
}


  
                                                                   
                                                                     
                                                                      
                                                                      
                                                                   
                                                                  
                                                                  
                                                                    
                    
  
                                                                    
                                                                  
                                  
  

#define AFPFS_VFS_CTL_GETID            0x00020001
#define AFPFS_VFS_CTL_NETCHANGE        0x00020002
#define AFPFS_VFS_CTL_VOLCHANGE        0x00020003

#define SMBFS_SYSCTL_REMOUNT           1
#define SMBFS_SYSCTL_REMOUNT_INFO      2
#define SMBFS_SYSCTL_GET_SERVER_SHARE  3


static int
is_bad_sysctl_name(struct vfstable *vfsp, int selector_name)
{
	switch (selector_name) {
	case VFS_CTL_QUERY:
	case VFS_CTL_TIMEO:
	case VFS_CTL_NOLOCKS:
	case VFS_CTL_NSTATUS:
	case VFS_CTL_SADDR:
	case VFS_CTL_DISC:
	case VFS_CTL_SERVERINFO:
		return 1;

	default:
		break;
	}

	                                                              
	if (strcmp(vfsp->vfc_name, "smbfs") == 0) {
		switch (selector_name) {
		case SMBFS_SYSCTL_REMOUNT:
		case SMBFS_SYSCTL_REMOUNT_INFO:
		case SMBFS_SYSCTL_GET_SERVER_SHARE:
			return 1;
		}
	} else if (strcmp(vfsp->vfc_name, "afpfs") == 0) {
		switch (selector_name) {
		case AFPFS_VFS_CTL_GETID:
		case AFPFS_VFS_CTL_NETCHANGE:
		case AFPFS_VFS_CTL_VOLCHANGE:
			return 1;
		}
	}

	  
	                                                                
	  
	return 0;
}


int vfs_sysctl_node SYSCTL_HANDLER_ARGS
{
	int *name, namelen;
	struct vfstable *vfsp;
	int error;
	int fstypenum;

	fstypenum = oidp->oid_number;
	name = arg1;
	namelen = arg2;

	                                                                                  
	if (namelen < 1) {
		return EISDIR;                 
	}
	mount_list_lock();
	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (vfsp->vfc_typenum == fstypenum) {
			vfsp->vfc_refcount++;
			break;
		}
	}
	mount_list_unlock();

	if (vfsp == NULL) {
		return ENOTSUP;
	}

	if (is_bad_sysctl_name(vfsp, name[0])) {
		printf("vfs: bad selector 0x%.8x for old-style sysctl().  use the sysctl-by-fsid interface instead\n", name[0]);
		error = EPERM;
	} else {
		error = (vfsp->vfc_vfsops->vfs_sysctl)(name, namelen,
		    req->oldptr, &req->oldlen, req->newptr, req->newlen,
		    vfs_context_current());
	}

	mount_list_lock();
	vfsp->vfc_refcount--;
	mount_list_unlock();

	return error;
}

  
                                                             
   
int
vfs_mountedon(struct vnode *vp)
{
	struct vnode *vq;
	int error = 0;

	SPECHASH_LOCK();
	if (vp->v_specflags & SI_MOUNTEDON) {
		error = EBUSY;
		goto out;
	}
	if (vp->v_specflags & SI_ALIASED) {
		for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
			if (vq->v_rdev != vp->v_rdev ||
			    vq->v_type != vp->v_type) {
				continue;
			}
			if (vq->v_specflags & SI_MOUNTEDON) {
				error = EBUSY;
				break;
			}
		}
	}
out:
	SPECHASH_UNLOCK();
	return error;
}

struct unmount_info {
	int     u_errs;                         
	int     u_busy;                         
	int     u_count;                          
	int     u_only_non_system;
};

static int
unmount_callback(mount_t mp, void *arg)
{
	int error;
	char *mntname;
	struct unmount_info *uip = arg;

	uip->u_count++;

	mntname = zalloc(ZV_NAMEI);
	strlcpy(mntname, mp->mnt_vfsstat.f_mntonname, MAXPATHLEN);

	if (uip->u_only_non_system
	    && ((mp->mnt_flag & MNT_ROOTFS) || (mp->mnt_kern_flag & MNTK_SYSTEM))) {                  
		printf("unmount(%d) %s skipped\n", uip->u_only_non_system, mntname);
		mount_iterdrop(mp);                              
	} else {
		printf("unmount(%d) %s\n", uip->u_only_non_system, mntname);

		mount_ref(mp, 0);
		mount_iterdrop(mp);                              
		error = dounmount(mp, MNT_FORCE, 1, vfs_context_current());
		if (error) {
			uip->u_errs++;
			printf("Unmount of %s failed (%d)\n", mntname ? mntname:"?", error);
			if (error == EBUSY) {
				uip->u_busy++;
			}
		}
	}
	if (mntname) {
		zfree(ZV_NAMEI, mntname);
	}

	return VFS_RETURNED;
}

  
                                                                  
                                     
                           
   
__private_extern__ void
vfs_unmountall(int only_non_system)
{
	int mounts, sec = 1;
	struct unmount_info ui;

	vfs_unmountall_started = 1;
	printf("vfs_unmountall(%ssystem) start\n", only_non_system ? "non" : "");

retry:
	ui.u_errs = ui.u_busy = ui.u_count = 0;
	ui.u_only_non_system = only_non_system;
	                                                                        
	vfs_iterate(VFS_ITERATE_CB_DROPREF | VFS_ITERATE_TAIL_FIRST, unmount_callback, &ui);
	mounts = mount_getvfscnt();
	if (mounts == 0) {
		return;
	}
	if (ui.u_busy > 0) {                                         
		tsleep(&nummounts, PVFS, "busy mount", sec * hz);
		sec *= 2;
		if (sec <= 32) {
			goto retry;
		}
		printf("Unmounting timed out\n");
	} else if (ui.u_count < mounts) {
		                                                            
		tsleep(&nummounts, PVFS, "missed mount", 2 * hz);
	}

	printf("vfs_unmountall(%ssystem) end\n", only_non_system ? "non" : "");
}

  
                                                                   
                                                                                
                                
   
__private_extern__ void
vnode_pager_vrele(vnode_t vp)
{
	struct ubc_info *uip;

	vnode_lock_spin(vp);

	vp->v_lflag &= ~VNAMED_UBC;
	if (vp->v_usecount != 0) {
		  
                                                     
                                                  
                                                           
                                                    
    
                                                           
                                                  
                                                    
                                                         
                   
     
		vnode_unlock(vp);
		ubc_unmap(vp);
		vnode_lock_spin(vp);
	}

	uip = vp->v_ubcinfo;
	vp->v_ubcinfo = UBC_INFO_NULL;

	vnode_unlock(vp);

	ubc_info_deallocate(uip);
}


#include <sys/disk.h>

u_int32_t rootunit = (u_int32_t)-1;

#if CONFIG_IOSCHED
extern int lowpri_throttle_enabled;
extern int iosched_enabled;
#endif

errno_t
vfs_init_io_attributes(vnode_t devvp, mount_t mp)
{
	int     error;
	off_t   readblockcnt = 0;
	off_t   writeblockcnt = 0;
	off_t   readmaxcnt = 0;
	off_t   writemaxcnt = 0;
	off_t   readsegcnt = 0;
	off_t   writesegcnt = 0;
	off_t   readsegsize = 0;
	off_t   writesegsize = 0;
	off_t   alignment = 0;
	u_int32_t minsaturationbytecount = 0;
	u_int32_t ioqueue_depth = 0;
	u_int32_t blksize;
	u_int64_t temp;
	u_int32_t features;
	u_int64_t location = 0;
	vfs_context_t ctx = vfs_context_current();
	dk_corestorage_info_t cs_info;
	boolean_t cs_present = FALSE;;
	int isssd = 0;
	int isvirtual = 0;


	VNOP_IOCTL(devvp, DKIOCGETTHROTTLEMASK, (caddr_t)&mp->mnt_throttle_mask, 0, NULL);
	  
                                                                      
                                  
    
	mp->mnt_devbsdunit = num_trailing_0(mp->mnt_throttle_mask);

	if (devvp == rootvp) {
		rootunit = mp->mnt_devbsdunit;
	}

	if (mp->mnt_devbsdunit == rootunit) {
		  
                                                           
                                                              
                                                      
     
		mp->mnt_kern_flag |= MNTK_ROOTDEV;
	}
	  
                                     
                                     
                                             
    
	set_fsblocksize(devvp);


	if ((error = VNOP_IOCTL(devvp, DKIOCGETBLOCKSIZE,
	    (caddr_t)&blksize, 0, ctx))) {
		return error;
	}

	mp->mnt_devblocksize = blksize;

	  
                                     
                                           
                                                   
                                                           
                         
    
	mp->mnt_maxreadcnt = MAX_UPL_SIZE_BYTES;
	mp->mnt_maxwritecnt = MAX_UPL_SIZE_BYTES;

	if (VNOP_IOCTL(devvp, DKIOCISVIRTUAL, (caddr_t)&isvirtual, 0, ctx) == 0) {
		if (isvirtual) {
			mp->mnt_kern_flag |= MNTK_VIRTUALDEV;
			mp->mnt_flag |= MNT_REMOVABLE;
		}
	}
	if (VNOP_IOCTL(devvp, DKIOCISSOLIDSTATE, (caddr_t)&isssd, 0, ctx) == 0) {
		if (isssd) {
			mp->mnt_kern_flag |= MNTK_SSD;
		}
	}
	if ((error = VNOP_IOCTL(devvp, DKIOCGETFEATURES,
	    (caddr_t)&features, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXBLOCKCOUNTREAD,
	    (caddr_t)&readblockcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXBLOCKCOUNTWRITE,
	    (caddr_t)&writeblockcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXBYTECOUNTREAD,
	    (caddr_t)&readmaxcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXBYTECOUNTWRITE,
	    (caddr_t)&writemaxcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXSEGMENTCOUNTREAD,
	    (caddr_t)&readsegcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXSEGMENTCOUNTWRITE,
	    (caddr_t)&writesegcnt, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXSEGMENTBYTECOUNTREAD,
	    (caddr_t)&readsegsize, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMAXSEGMENTBYTECOUNTWRITE,
	    (caddr_t)&writesegsize, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETMINSEGMENTALIGNMENTBYTECOUNT,
	    (caddr_t)&alignment, 0, ctx))) {
		return error;
	}

	if ((error = VNOP_IOCTL(devvp, DKIOCGETCOMMANDPOOLSIZE,
	    (caddr_t)&ioqueue_depth, 0, ctx))) {
		return error;
	}

	if (readmaxcnt) {
		mp->mnt_maxreadcnt = (readmaxcnt > UINT32_MAX) ? UINT32_MAX :(uint32_t) readmaxcnt;
	}

	if (readblockcnt) {
		temp = readblockcnt * blksize;
		temp = (temp > UINT32_MAX) ? UINT32_MAX : temp;

		if (temp < mp->mnt_maxreadcnt) {
			mp->mnt_maxreadcnt = (u_int32_t)temp;
		}
	}

	if (writemaxcnt) {
		mp->mnt_maxwritecnt = (writemaxcnt > UINT32_MAX) ? UINT32_MAX : (uint32_t)writemaxcnt;
	}

	if (writeblockcnt) {
		temp = writeblockcnt * blksize;
		temp = (temp > UINT32_MAX) ? UINT32_MAX : temp;

		if (temp < mp->mnt_maxwritecnt) {
			mp->mnt_maxwritecnt = (u_int32_t)temp;
		}
	}

	if (readsegcnt) {
		temp = (readsegcnt > UINT16_MAX) ? UINT16_MAX : readsegcnt;
	} else {
		temp = mp->mnt_maxreadcnt / PAGE_SIZE;

		if (temp > UINT16_MAX) {
			temp = UINT16_MAX;
		}
	}
	mp->mnt_segreadcnt = (u_int16_t)temp;

	if (writesegcnt) {
		temp = (writesegcnt > UINT16_MAX) ? UINT16_MAX : writesegcnt;
	} else {
		temp = mp->mnt_maxwritecnt / PAGE_SIZE;

		if (temp > UINT16_MAX) {
			temp = UINT16_MAX;
		}
	}
	mp->mnt_segwritecnt = (u_int16_t)temp;

	if (readsegsize) {
		temp = (readsegsize > UINT32_MAX) ? UINT32_MAX : readsegsize;
	} else {
		temp = mp->mnt_maxreadcnt;
	}
	mp->mnt_maxsegreadsize = (u_int32_t)temp;

	if (writesegsize) {
		temp = (writesegsize > UINT32_MAX) ? UINT32_MAX : writesegsize;
	} else {
		temp = mp->mnt_maxwritecnt;
	}
	mp->mnt_maxsegwritesize = (u_int32_t)temp;

	if (alignment) {
		temp = (alignment > PAGE_SIZE) ? PAGE_MASK : alignment - 1;
	} else {
		temp = 0;
	}
	mp->mnt_alignmentmask = (uint32_t)temp;


	if (ioqueue_depth > MNT_DEFAULT_IOQUEUE_DEPTH) {
		temp = ioqueue_depth;
	} else {
		temp = MNT_DEFAULT_IOQUEUE_DEPTH;
	}

	mp->mnt_ioqueue_depth = (uint32_t)temp;
	mp->mnt_ioscale = MNT_IOSCALE(mp->mnt_ioqueue_depth);

	if (mp->mnt_ioscale > 1) {
		printf("ioqueue_depth = %d,   ioscale = %d\n", (int)mp->mnt_ioqueue_depth, (int)mp->mnt_ioscale);
	}

	if (features & DK_FEATURE_FORCE_UNIT_ACCESS) {
		mp->mnt_ioflags |= MNT_IOFLAGS_FUA_SUPPORTED;
	}

	if (VNOP_IOCTL(devvp, DKIOCGETIOMINSATURATIONBYTECOUNT, (caddr_t)&minsaturationbytecount, 0, ctx) == 0) {
		mp->mnt_minsaturationbytecount = minsaturationbytecount;
	} else {
		mp->mnt_minsaturationbytecount = 0;
	}

	if (VNOP_IOCTL(devvp, DKIOCCORESTORAGE, (caddr_t)&cs_info, 0, ctx) == 0) {
		cs_present = TRUE;
	}

	if (features & DK_FEATURE_UNMAP) {
		mp->mnt_ioflags |= MNT_IOFLAGS_UNMAP_SUPPORTED;

		if (cs_present == TRUE) {
			mp->mnt_ioflags |= MNT_IOFLAGS_CSUNMAP_SUPPORTED;
		}
	}
	if (cs_present == TRUE) {
		  
                                                        
                                                
     
		if ((cs_info.flags & DK_CORESTORAGE_PIN_YOUR_METADATA)) {
			mp->mnt_ioflags |= MNT_IOFLAGS_FUSION_DRIVE;
		}
	} else {
		                           
		dk_apfs_flavour_t flavour;
		if ((VNOP_IOCTL(devvp, DKIOCGETAPFSFLAVOUR, (caddr_t)&flavour, 0, ctx) == 0) &&
		    (flavour == DK_APFS_FUSION)) {
			mp->mnt_ioflags |= MNT_IOFLAGS_FUSION_DRIVE;
		}
	}

	if (VNOP_IOCTL(devvp, DKIOCGETLOCATION, (caddr_t)&location, 0, ctx) == 0) {
		if (location & DK_LOCATION_EXTERNAL) {
			mp->mnt_ioflags |= MNT_IOFLAGS_PERIPHERAL_DRIVE;
			mp->mnt_flag |= MNT_REMOVABLE;
		}
	}

#if CONFIG_IOSCHED
	if (iosched_enabled && (features & DK_FEATURE_PRIORITY)) {
		mp->mnt_ioflags |= MNT_IOFLAGS_IOSCHED_SUPPORTED;
		throttle_info_disable_throttle(mp->mnt_devbsdunit, (mp->mnt_ioflags & MNT_IOFLAGS_FUSION_DRIVE) != 0);
	}
#endif                     
	return error;
}

static struct klist fs_klist;
static LCK_GRP_DECLARE(fs_klist_lck_grp, "fs_klist");
static LCK_MTX_DECLARE(fs_klist_lock, &fs_klist_lck_grp);

void
vfs_event_init(void)
{
	klist_init(&fs_klist);
}

void
vfs_event_signal(fsid_t *fsid, u_int32_t event, intptr_t data)
{
	if (event == VQ_DEAD || event == VQ_NOTRESP) {
		struct mount *mp = vfs_getvfs(fsid);
		if (mp) {
			mount_lock_spin(mp);
			if (data) {
				mp->mnt_kern_flag &= ~MNT_LNOTRESP;                      
			} else {
				mp->mnt_kern_flag |= MNT_LNOTRESP;                       
			}
			mount_unlock(mp);
		}
	}

	lck_mtx_lock(&fs_klist_lock);
	KNOTE(&fs_klist, event);
	lck_mtx_unlock(&fs_klist_lock);
}

  
                                            
   
static int
sysctl_vfs_getvfscnt(void)
{
	return mount_getvfscnt();
}


static int
mount_getvfscnt(void)
{
	int ret;

	mount_list_lock();
	ret = nummounts;
	mount_list_unlock();
	return ret;
}



static int
mount_fillfsids(fsid_t *fsidlst, int count)
{
	struct mount *mp;
	int actual = 0;

	actual = 0;
	mount_list_lock();
	TAILQ_FOREACH(mp, &mountlist, mnt_list) {
		if (actual < count) {
			fsidlst[actual] = mp->mnt_vfsstat.f_fsid;
			actual++;
		}
	}
	mount_list_unlock();
	return actual;
}

  
                                                                   
                                                                         
                                                                       
                         
                                                                     
   
static int
sysctl_vfs_getvfslist(fsid_t *fsidlst, unsigned long count, unsigned long *actual)
{
	struct mount *mp;

	*actual = 0;
	mount_list_lock();
	TAILQ_FOREACH(mp, &mountlist, mnt_list) {
		(*actual)++;
		if (*actual <= count) {
			fsidlst[(*actual) - 1] = mp->mnt_vfsstat.f_fsid;
		}
	}
	mount_list_unlock();
	return *actual <= count ? 0 : ENOMEM;
}

static int
sysctl_vfs_vfslist(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	unsigned long actual;
	int error;
	size_t space;
	fsid_t *fsidlst;

	                              
	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	                                                             
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = sysctl_vfs_getvfscnt() * sizeof(fsid_t);
		return 0;
	}
again:
	  
                                                                      
                                    
    
	space = req->oldlen;
	req->oldlen = sysctl_vfs_getvfscnt() * sizeof(fsid_t);

	                                       
	if (space < req->oldlen) {
		return ENOMEM;
	}

	fsidlst = kheap_alloc(KHEAP_TEMP, req->oldlen, Z_WAITOK | Z_ZERO);
	if (fsidlst == NULL) {
		return ENOMEM;
	}

	error = sysctl_vfs_getvfslist(fsidlst, req->oldlen / sizeof(fsid_t),
	    &actual);
	  
                                                                     
                                                               
    
	if (error == ENOMEM) {
		kheap_free(KHEAP_TEMP, fsidlst, req->oldlen);
		req->oldlen = space;
		goto again;
	}
	if (error == 0) {
		error = SYSCTL_OUT(req, fsidlst, actual * sizeof(fsid_t));
	}
	kheap_free(KHEAP_TEMP, fsidlst, req->oldlen);
	return error;
}

  
                       
   
static int
sysctl_vfs_ctlbyfsid(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	union union_vfsidctl vc;
	struct mount *mp;
	struct vfsstatfs *sp;
	int *name, namelen;
	int flags = 0;
	int error = 0, gotref = 0;
	vfs_context_t ctx = vfs_context_current();
	proc_t p = req->p;                                         
	boolean_t is_64_bit;
	union {
		struct statfs64 sfs64;
		struct user64_statfs osfs64;
		struct user32_statfs osfs32;
	} *sfsbuf;

	if (req->newptr == USER_ADDR_NULL) {
		error = EINVAL;
		goto out;
	}

	name = arg1;
	namelen = arg2;
	is_64_bit = proc_is64bit(p);

	error = SYSCTL_IN(req, &vc, is_64_bit? sizeof(vc.vc64):sizeof(vc.vc32));
	if (error) {
		goto out;
	}
	if (vc.vc32.vc_vers != VFS_CTL_VERS1) {                          
		error = EINVAL;
		goto out;
	}
	mp = mount_list_lookupby_fsid(&vc.vc32.vc_fsid, 0, 1);                          
	if (mp == NULL) {
		error = ENOENT;
		goto out;
	}
	gotref = 1;
	                                                      
	req->newidx = 0;
	  
                                                                
                                                              
                           
    
	if (mp->mnt_op->vfs_sysctl != NULL) {
		if (is_64_bit) {
			if (vfs_64bitready(mp)) {
				error = mp->mnt_op->vfs_sysctl(name, namelen,
				    CAST_USER_ADDR_T(req),
				    NULL, USER_ADDR_NULL, 0,
				    ctx);
			} else {
				error = ENOTSUP;
			}
		} else {
			error = mp->mnt_op->vfs_sysctl(name, namelen,
			    CAST_USER_ADDR_T(req),
			    NULL, USER_ADDR_NULL, 0,
			    ctx);
		}
		if (error != ENOTSUP) {
			goto out;
		}
	}
	switch (name[0]) {
	case VFS_CTL_UMOUNT:
#if CONFIG_MACF
		error = mac_mount_check_umount(ctx, mp);
		if (error != 0) {
			goto out;
		}
#endif
		req->newidx = 0;
		if (is_64_bit) {
			req->newptr = vc.vc64.vc_ptr;
			req->newlen = (size_t)vc.vc64.vc_len;
		} else {
			req->newptr = CAST_USER_ADDR_T(vc.vc32.vc_ptr);
			req->newlen = vc.vc32.vc_len;
		}
		error = SYSCTL_IN(req, &flags, sizeof(flags));
		if (error) {
			break;
		}

		mount_ref(mp, 0);
		mount_iterdrop(mp);
		gotref = 0;
		                                  
		error = safedounmount(mp, flags, ctx);
		break;
	case VFS_CTL_OSTATFS:
	case VFS_CTL_STATFS64:
#if CONFIG_MACF
		error = mac_mount_check_stat(ctx, mp);
		if (error != 0) {
			break;
		}
#endif
		req->newidx = 0;
		if (is_64_bit) {
			req->newptr = vc.vc64.vc_ptr;
			req->newlen = (size_t)vc.vc64.vc_len;
		} else {
			req->newptr = CAST_USER_ADDR_T(vc.vc32.vc_ptr);
			req->newlen = vc.vc32.vc_len;
		}
		error = SYSCTL_IN(req, &flags, sizeof(flags));
		if (error) {
			break;
		}
		sp = &mp->mnt_vfsstat;
		if (((flags & MNT_NOWAIT) == 0 || (flags & (MNT_WAIT | MNT_DWAIT))) &&
		    (error = vfs_update_vfsstat(mp, ctx, VFS_USER_EVENT))) {
			goto out;
		}

		sfsbuf = kheap_alloc(KHEAP_TEMP, sizeof(*sfsbuf), Z_WAITOK);

		if (name[0] == VFS_CTL_STATFS64) {
			struct statfs64 *sfs = &sfsbuf->sfs64;

			vfs_get_statfs64(mp, sfs);
			error = SYSCTL_OUT(req, sfs, sizeof(*sfs));
		} else if (is_64_bit) {
			struct user64_statfs *sfs = &sfsbuf->osfs64;

			bzero(sfs, sizeof(*sfs));
			sfs->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			sfs->f_type = (short)mp->mnt_vtable->vfc_typenum;
			sfs->f_bsize = (user64_long_t)sp->f_bsize;
			sfs->f_iosize = (user64_long_t)sp->f_iosize;
			sfs->f_blocks = (user64_long_t)sp->f_blocks;
			sfs->f_bfree = (user64_long_t)sp->f_bfree;
			sfs->f_bavail = (user64_long_t)sp->f_bavail;
			sfs->f_files = (user64_long_t)sp->f_files;
			sfs->f_ffree = (user64_long_t)sp->f_ffree;
			sfs->f_fsid = sp->f_fsid;
			sfs->f_owner = sp->f_owner;
#ifdef CONFIG_NFS_CLIENT
			if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
				strlcpy(&sfs->f_fstypename[0], &mp->fstypename_override[0], MFSNAMELEN);
			} else
#endif                        
			{
				strlcpy(sfs->f_fstypename, sp->f_fstypename, MFSNAMELEN);
			}
			strlcpy(sfs->f_mntonname, sp->f_mntonname, MNAMELEN);
			strlcpy(sfs->f_mntfromname, sp->f_mntfromname, MNAMELEN);

			error = SYSCTL_OUT(req, sfs, sizeof(*sfs));
		} else {
			struct user32_statfs *sfs = &sfsbuf->osfs32;
			long temp;

			bzero(sfs, sizeof(*sfs));
			sfs->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			sfs->f_type = (short)mp->mnt_vtable->vfc_typenum;

			  
                                                                                   
                                                                                      
                                                    
      
			if (sp->f_blocks > INT_MAX) {
				int             shift;

				  
                                                                             
                                                                         
                                                                          
                                 
      
                                                                           
                                  
       
				for (shift = 0; shift < 32; shift++) {
					if ((sp->f_blocks >> shift) <= INT_MAX) {
						break;
					}
					if ((((long long)sp->f_bsize) << (shift + 1)) > INT_MAX) {
						break;
					}
				}
#define __SHIFT_OR_CLIP(x, s)   ((((x) >> (s)) > INT_MAX) ? INT_MAX : ((x) >> (s)))
				sfs->f_blocks = (user32_long_t)__SHIFT_OR_CLIP(sp->f_blocks, shift);
				sfs->f_bfree = (user32_long_t)__SHIFT_OR_CLIP(sp->f_bfree, shift);
				sfs->f_bavail = (user32_long_t)__SHIFT_OR_CLIP(sp->f_bavail, shift);
#undef __SHIFT_OR_CLIP
				sfs->f_bsize = (user32_long_t)(sp->f_bsize << shift);
				temp = lmax(sp->f_iosize, sp->f_bsize);
				if (temp > INT32_MAX) {
					error = EINVAL;
					kheap_free(KHEAP_TEMP, sfsbuf, sizeof(*sfsbuf));
					goto out;
				}
				sfs->f_iosize = (user32_long_t)temp;
			} else {
				sfs->f_bsize = (user32_long_t)sp->f_bsize;
				sfs->f_iosize = (user32_long_t)sp->f_iosize;
				sfs->f_blocks = (user32_long_t)sp->f_blocks;
				sfs->f_bfree = (user32_long_t)sp->f_bfree;
				sfs->f_bavail = (user32_long_t)sp->f_bavail;
			}
			sfs->f_files = (user32_long_t)sp->f_files;
			sfs->f_ffree = (user32_long_t)sp->f_ffree;
			sfs->f_fsid = sp->f_fsid;
			sfs->f_owner = sp->f_owner;

#ifdef CONFIG_NFS_CLIENT
			if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
				strlcpy(&sfs->f_fstypename[0], &mp->fstypename_override[0], MFSNAMELEN);
			} else
#endif                        
			{
				strlcpy(sfs->f_fstypename, sp->f_fstypename, MFSNAMELEN);
			}
			strlcpy(sfs->f_mntonname, sp->f_mntonname, MNAMELEN);
			strlcpy(sfs->f_mntfromname, sp->f_mntfromname, MNAMELEN);

			error = SYSCTL_OUT(req, sfs, sizeof(*sfs));
		}
		kheap_free(KHEAP_TEMP, sfsbuf, sizeof(*sfsbuf));
		break;
	default:
		error = ENOTSUP;
		goto out;
	}
out:
	if (gotref != 0) {
		mount_iterdrop(mp);
	}
	return error;
}

static int      filt_fsattach(struct knote *kn, struct kevent_qos_s *kev);
static void     filt_fsdetach(struct knote *kn);
static int      filt_fsevent(struct knote *kn, long hint);
static int      filt_fstouch(struct knote *kn, struct kevent_qos_s *kev);
static int      filt_fsprocess(struct knote *kn, struct kevent_qos_s *kev);
SECURITY_READ_ONLY_EARLY(struct filterops) fs_filtops = {
	.f_attach = filt_fsattach,
	.f_detach = filt_fsdetach,
	.f_event = filt_fsevent,
	.f_touch = filt_fstouch,
	.f_process = filt_fsprocess,
};

static int
filt_fsattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	kn->kn_flags |= EV_CLEAR;                
	kn->kn_sdata = 0;                                       

	lck_mtx_lock(&fs_klist_lock);
	KNOTE_ATTACH(&fs_klist, kn);
	lck_mtx_unlock(&fs_klist_lock);

	  
                                   
                                 
    
	return 0;
}

static void
filt_fsdetach(struct knote *kn)
{
	lck_mtx_lock(&fs_klist_lock);
	KNOTE_DETACH(&fs_klist, kn);
	lck_mtx_unlock(&fs_klist_lock);
}

static int
filt_fsevent(struct knote *kn, long hint)
{
	  
                            
                                                         
    

	if ((kn->kn_sfflags == 0) || (kn->kn_sfflags & hint)) {
		kn->kn_fflags |= hint;
	}

	return kn->kn_fflags != 0;
}

static int
filt_fstouch(struct knote *kn, struct kevent_qos_s *kev)
{
	int res;

	lck_mtx_lock(&fs_klist_lock);

	kn->kn_sfflags = kev->fflags;

	  
                                                                           
                                                                   
                
   
                                                                       
                        
    
	                     
	                                   
	res = (kn->kn_fflags != 0);

	lck_mtx_unlock(&fs_klist_lock);

	return res;
}

static int
filt_fsprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	int res = 0;

	lck_mtx_lock(&fs_klist_lock);
	if (kn->kn_fflags) {
		knote_fill_kevent(kn, kev, 0);
		res = 1;
	}
	lck_mtx_unlock(&fs_klist_lock);
	return res;
}

static int
sysctl_vfs_noremotehang(__unused struct sysctl_oid *oidp,
    __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	int out, error;
	pid_t pid;
	proc_t p;

	                    
	if (req->newptr == USER_ADDR_NULL) {
		return EINVAL;
	}

	error = SYSCTL_IN(req, &pid, sizeof(pid));
	if (error) {
		return error;
	}

	p = proc_find(pid < 0 ? -pid : pid);
	if (p == NULL) {
		return ESRCH;
	}

	  
                                                          
                     
    
	if (req->oldptr != USER_ADDR_NULL) {
		out = !((p->p_flag & P_NOREMOTEHANG) == 0);
		proc_rele(p);
		error = SYSCTL_OUT(req, &out, sizeof(out));
		return error;
	}

	                                          
	if (p != req->p && proc_suser(req->p) != 0) {
		proc_rele(p);
		return EPERM;
	}

	if (pid < 0) {
		OSBitAndAtomic(~((uint32_t)P_NOREMOTEHANG), &p->p_flag);
	} else {
		OSBitOrAtomic(P_NOREMOTEHANG, &p->p_flag);
	}
	proc_rele(p);

	return 0;
}

static int
sysctl_vfs_generic_conf SYSCTL_HANDLER_ARGS
{
	int *name, namelen;
	struct vfstable *vfsp;
	struct vfsconf vfsc = {};

	(void)oidp;
	name = arg1;
	namelen = arg2;

	if (namelen < 1) {
		return EISDIR;
	} else if (namelen > 1) {
		return ENOTDIR;
	}

	mount_list_lock();
	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (vfsp->vfc_typenum == name[0]) {
			break;
		}
	}

	if (vfsp == NULL) {
		mount_list_unlock();
		return ENOTSUP;
	}

	vfsc.vfc_reserved1 = 0;
	bcopy(vfsp->vfc_name, vfsc.vfc_name, sizeof(vfsc.vfc_name));
	vfsc.vfc_typenum = vfsp->vfc_typenum;
	vfsc.vfc_refcount = vfsp->vfc_refcount;
	vfsc.vfc_flags = vfsp->vfc_flags;
	vfsc.vfc_reserved2 = 0;
	vfsc.vfc_reserved3 = 0;

	mount_list_unlock();
	return SYSCTL_OUT(req, &vfsc, sizeof(struct vfsconf));
}

                              
SYSCTL_EXTENSIBLE_NODE(_vfs, VFS_GENERIC, generic,
    CTLFLAG_RW | CTLFLAG_LOCKED, NULL, "vfs generic hinge");
                                                  
SYSCTL_PROC(_vfs_generic, OID_AUTO, vfsidlist,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    NULL, 0, sysctl_vfs_vfslist, "S,fsid", "List of mounted filesystem ids");
                                                 
SYSCTL_NODE(_vfs_generic, OID_AUTO, ctlbyfsid, CTLFLAG_RW | CTLFLAG_LOCKED,
    sysctl_vfs_ctlbyfsid, "ctlbyfsid");
SYSCTL_PROC(_vfs_generic, OID_AUTO, noremotehang, CTLFLAG_RW | CTLFLAG_ANYBODY,
    NULL, 0, sysctl_vfs_noremotehang, "I", "noremotehang");
SYSCTL_INT(_vfs_generic, VFS_MAXTYPENUM, maxtypenum,
    CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED,
    &maxvfstypenum, 0, "");
SYSCTL_INT(_vfs_generic, OID_AUTO, sync_timeout, CTLFLAG_RW | CTLFLAG_LOCKED, &sync_timeout_seconds, 0, "");
SYSCTL_NODE(_vfs_generic, VFS_CONF, conf,
    CTLFLAG_RD | CTLFLAG_LOCKED,
    sysctl_vfs_generic_conf, "");
#if DEVELOPMENT || DEBUG
SYSCTL_INT(_vfs_generic, OID_AUTO, print_busy_vnodes,
    CTLTYPE_INT | CTLFLAG_RW,
    &print_busy_vnodes, 0,
    "VFS log busy vnodes blocking unmount");
#endif

                                                          
static int vfs_root_unmounted_cleanly = 0;
SYSCTL_INT(_vfs_generic, OID_AUTO, root_unmounted_cleanly, CTLFLAG_RD, &vfs_root_unmounted_cleanly, 0, "Root filesystem was unmounted cleanly");

void
vfs_set_root_unmounted_cleanly(void)
{
	vfs_root_unmounted_cleanly = 1;
}

  
                     
   
void
vn_print_state(struct vnode *vp, const char *fmt, ...)
{
	va_list ap;
	char perm_str[] = "(VM_KERNEL_ADDRPERM pointer)";
	char fs_name[MFSNAMELEN];

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("vp 0x%0llx %s: ", (uint64_t)VM_KERNEL_ADDRPERM(vp), perm_str);
	printf("tag %d, type %d\n", vp->v_tag, vp->v_type);
	               
	printf("    iocount %d, usecount %d, kusecount %d references %d\n",
	    vp->v_iocount, vp->v_usecount, vp->v_kusecount, vp->v_references);
	printf("    writecount %d, numoutput %d\n", vp->v_writecount,
	    vp->v_numoutput);
	           
	printf("    flag 0x%x, lflag 0x%x, listflag 0x%x\n", vp->v_flag,
	    vp->v_lflag, vp->v_listflag);

	if (vp->v_mount == NULL || vp->v_mount == dead_mountp) {
		strlcpy(fs_name, "deadfs", MFSNAMELEN);
	} else {
		vfs_name(vp->v_mount, fs_name);
	}

	printf("    v_data 0x%0llx %s\n",
	    (vp->v_data ? (uint64_t)VM_KERNEL_ADDRPERM(vp->v_data) : 0),
	    perm_str);
	printf("    v_mount 0x%0llx %s vfs_name %s\n",
	    (vp->v_mount ? (uint64_t)VM_KERNEL_ADDRPERM(vp->v_mount) : 0),
	    perm_str, fs_name);
}

long num_reusedvnodes = 0;


static vnode_t
process_vp(vnode_t vp, int want_vp, bool can_defer, int *deferred)
{
	unsigned int  vpid;

	*deferred = 0;

	vpid = vp->v_id;

	vnode_list_remove_locked(vp);

	vnode_list_unlock();

	vnode_lock_spin(vp);

	  
                                                                            
                                                                             
                                                                           
    
	if ((vpid != vp->v_id) || (vp->v_usecount != 0) || (vp->v_iocount != 0) ||
	    VONLIST(vp) || (vp->v_lflag & VL_TERMINATE)) {
		  
                                                    
                                                  
                                                 
     
		vnode_unlock(vp);

		return NULLVP;
	}
	if ((vp->v_lflag & (VL_NEEDINACTIVE | VL_MARKTERM)) == VL_NEEDINACTIVE) {
		  
                                           
                                            
                                                
                                       
                        
    
                                           
                                             
                                       
                                        
                                              
                                                       
                                          
     
		vp->v_iocount++;
#ifdef JOE_DEBUG
		record_vp(vp, 1);
#endif
		vnode_put_locked(vp);
		vnode_unlock(vp);

		return NULLVP;
	}
	  
                                           
    
	if (vp->v_type != VBAD) {
		if ((want_vp || can_defer) && (vnode_on_reliable_media(vp) == FALSE || (vp->v_flag & VISDIRTY))) {
			vnode_async_list_add(vp);
			vnode_unlock(vp);

			*deferred = 1;

			return NULLVP;
		}
		if (vp->v_lflag & VL_DEAD) {
			panic("new_vnode(%p): the vnode is VL_DEAD but not VBAD", vp);
		}

		vnode_lock_convert(vp);
		(void)vnode_reclaim_internal(vp, 1, want_vp, 0);

		if (want_vp) {
			if ((VONLIST(vp))) {
				panic("new_vnode(%p): vp on list", vp);
			}
			if (vp->v_usecount || vp->v_iocount || vp->v_kusecount ||
			    (vp->v_lflag & (VNAMED_UBC | VNAMED_MOUNT | VNAMED_FSHASH))) {
				panic("new_vnode(%p): free vnode still referenced", vp);
			}
			if ((vp->v_mntvnodes.tqe_prev != 0) && (vp->v_mntvnodes.tqe_next != 0)) {
				panic("new_vnode(%p): vnode seems to be on mount list", vp);
			}
			if (!LIST_EMPTY(&vp->v_nclinks) || !TAILQ_EMPTY(&vp->v_ncchildren)) {
				panic("new_vnode(%p): vnode still hooked into the name cache", vp);
			}
		} else {
			vnode_unlock(vp);
			vp = NULLVP;
		}
	}
	return vp;
}

__attribute__((noreturn))
static void
async_work_continue(void)
{
	struct async_work_lst *q;
	int     deferred;
	vnode_t vp;

	q = &vnode_async_work_list;

	for (;;) {
		vnode_list_lock();

		if (TAILQ_EMPTY(q)) {
			assert_wait(q, (THREAD_UNINT));

			vnode_list_unlock();

			thread_block((thread_continue_t)async_work_continue);

			continue;
		}
		async_work_handled++;

		vp = TAILQ_FIRST(q);

		vp = process_vp(vp, 0, false, &deferred);

		if (vp != NULLVP) {
			panic("found VBAD vp (%p) on async queue", vp);
		}
	}
}

__attribute__((noreturn))
static void
vn_laundry_continue(void)
{
	struct freelst *free_q;
	struct ragelst *rage_q;
	int     deferred;
	vnode_t vp;
	bool rage_q_empty;
	bool free_q_empty;


	free_q = &vnode_free_list;
	rage_q = &vnode_rage_list;

	for (;;) {
		vnode_list_lock();

		free_q_empty = TAILQ_EMPTY(free_q);
		rage_q_empty = TAILQ_EMPTY(rage_q);

		if (!rage_q_empty && !free_q_empty) {
			struct timeval current_tv;

			microuptime(&current_tv);
			if (ragevnodes < rage_limit &&
			    ((current_tv.tv_sec - rage_tv.tv_sec) < RAGE_TIME_LIMIT)) {
				rage_q_empty = true;
			}
		}

		if (deadvnodes >= deadvnodes_high ||
		    (rage_q_empty && free_q_empty) ||
		    numvnodes < desiredvnodes) {
			assert_wait(free_q, (THREAD_UNINT));

			vnode_list_unlock();

			thread_block((thread_continue_t)vn_laundry_continue);

			continue;
		}

		if (!rage_q_empty) {
			vp = TAILQ_FIRST(rage_q);
		} else {
			vp = TAILQ_FIRST(free_q);
		}

		vp = process_vp(vp, 0, true, &deferred);
	}
}

static inline void
wakeup_laundry_thread()
{
	if ((deadvnodes < deadvnodes_low) &&
	                                                                
	    ((freevnodes + ragevnodes) > 10)) {
		wakeup(&vnode_free_list);
	}
}

static int
new_vnode(vnode_t *vpp)
{
	vnode_t vp;
	uint32_t retries = 0, max_retries = 100;                                               
	uint32_t bdevvp_vnodes = 0;
	int force_alloc = 0, walk_count = 0;
	boolean_t need_reliable_vp = FALSE;
	int deferred;
	struct timeval initial_tv;
	struct timeval current_tv;
	proc_t  curproc = current_proc();

	initial_tv.tv_sec = 0;
retry:
	vp = NULLVP;

	vnode_list_lock();
	newvnode++;

	if (need_reliable_vp == TRUE) {
		async_work_timed_out++;
	}

	if ((numvnodes - deadvnodes) < desiredvnodes || force_alloc) {
		struct timespec ts;

		if (!TAILQ_EMPTY(&vnode_dead_list)) {
			  
                                 
      
			vp = TAILQ_FIRST(&vnode_dead_list);
			if (numvnodes >= desiredvnodes) {
				wakeup_laundry_thread();
			}
			goto steal_this_vp;
		}
		  
                                               
                                        
     
		numvnodes++;
		if (numvnodes >= desiredvnodes) {
			wakeup_laundry_thread();
		}
		vnode_list_unlock();

		vp = zalloc_flags(vnode_zone, Z_WAITOK | Z_ZERO);
		VLISTNONE(vp);                                          
		lck_mtx_init(&vp->v_lock, &vnode_lck_grp, &vnode_lck_attr);

		TAILQ_INIT(&vp->v_ncchildren);

		klist_init(&vp->v_knotes);
		nanouptime(&ts);
		vp->v_id = (uint32_t)ts.tv_nsec;
		vp->v_flag = VSTANDARD;

#if CONFIG_MACF
		if (mac_vnode_label_init_needed(vp)) {
			mac_vnode_label_init(vp);
		}
#endif          

		vp->v_iocount = 1;
		goto done;
	}

	wakeup_laundry_thread();

	microuptime(&current_tv);

#define MAX_WALK_COUNT 1000

	if (!TAILQ_EMPTY(&vnode_rage_list) &&
	    (ragevnodes >= rage_limit ||
	    (current_tv.tv_sec - rage_tv.tv_sec) >= RAGE_TIME_LIMIT)) {
		TAILQ_FOREACH(vp, &vnode_rage_list, v_freelist) {
			if (!(vp->v_listflag & VLIST_RAGE)) {
				panic("new_vnode: vp (%p) on RAGE list not marked VLIST_RAGE", vp);
			}

			                                                              
			                                                              
			                                                          
			                                                      
			                                                          
			                                                          
			                                                    

			if ((curproc->p_flag & P_DEPENDENCY_CAPABLE) == 0 || vp->v_mount == NULL ||
			    (vp->v_mount->mnt_kern_flag & MNTK_VIRTUALDEV) == 0) {
				  
                                                                       
                                                                       
                                                                       
                                                                  
                                
       
				if (need_reliable_vp == FALSE || vnode_on_reliable_media(vp) == TRUE) {
					break;
				}
			}

			                                                   
			                                                       

			if (walk_count++ > MAX_WALK_COUNT) {
				vp = NULL;
				break;
			}
		}
	}

	if (vp == NULL && !TAILQ_EMPTY(&vnode_free_list)) {
		  
                                         
     
		walk_count = 0;
		TAILQ_FOREACH(vp, &vnode_free_list, v_freelist) {
			                                                              
			                                                              
			                                                          
			                                                      
			                                                          
			                                                          
			                                                    

			if ((curproc->p_flag & P_DEPENDENCY_CAPABLE) == 0 || vp->v_mount == NULL ||
			    (vp->v_mount->mnt_kern_flag & MNTK_VIRTUALDEV) == 0) {
				  
                                                                       
                                                                       
                                                                       
                                                                  
                                
       
				if (need_reliable_vp == FALSE || vnode_on_reliable_media(vp) == TRUE) {
					break;
				}
			}

			                                                   
			                                                       

			if (walk_count++ > MAX_WALK_COUNT) {
				vp = NULL;
				break;
			}
		}
	}

	  
	                                                                   
	                                                     
	                                                       
	                                                      
	                                                         
	                                                        
	                  
	  
	if (vp == NULL && walk_count >= MAX_WALK_COUNT) {
		force_alloc = 1;
		vnode_list_unlock();
		goto retry;
	}

	if (vp == NULL) {
		  
                                                              
                                           
                                                         
                                                             
     
		if (++retries <= max_retries) {
			vnode_list_unlock();
			delay_for_interval(1, 1000 * 1000);
			goto retry;
		}

		vnode_list_unlock();
		tablefull("vnode");
		log(LOG_EMERG, "%d desired, %ld numvnodes, "
		    "%ld free, %ld dead, %ld async, %d rage %d bdevvp\n",
		    desiredvnodes, numvnodes, freevnodes, deadvnodes, async_work_vnodes, ragevnodes, bdevvp_vnodes);
#if CONFIG_JETSAM

#if DEVELOPMENT || DEBUG
		if (bootarg_no_vnode_jetsam) {
			panic("vnode table is full\n");
		}
#endif                           

		  
                                                                         
                                              
     
		if (memorystatus_kill_on_vnode_limit() == FALSE) {
			  
                                                               
                                                                
                                          
      
			panic("vnode table is full\n");
		}

		  
                                                                   
                                                     
     
		delay_for_interval(3, 1000 * 1000);
		retries = 0;
		max_retries = 10;
		goto retry;
#endif

		*vpp = NULL;
		return ENFILE;
	}
	newvnode_nodead++;
steal_this_vp:
	if ((vp = process_vp(vp, 1, true, &deferred)) == NULLVP) {
		if (deferred) {
			int     elapsed_msecs;
			struct timeval elapsed_tv;

			if (initial_tv.tv_sec == 0) {
				microuptime(&initial_tv);
			}

			vnode_list_lock();

			dead_vnode_waited++;
			dead_vnode_wanted++;

			  
                                                        
                                                             
                                                              
                                                         
                                                                      
                                                                      
      
			assert_wait_timeout((caddr_t)&dead_vnode_wanted, (THREAD_INTERRUPTIBLE), 10000, NSEC_PER_USEC);

			vnode_list_unlock();

			thread_block(THREAD_CONTINUE_NULL);

			microuptime(&elapsed_tv);

			timevalsub(&elapsed_tv, &initial_tv);
			elapsed_msecs = (int)(elapsed_tv.tv_sec * 1000 + elapsed_tv.tv_usec / 1000);

			if (elapsed_msecs >= 100) {
				  
                                           
                                                
                                            
                                              
                    
      
                                       
                                         
                                            
                                          
                                         
       
				need_reliable_vp = TRUE;
			}
		}
		goto retry;
	}
	OSAddAtomicLong(1, &num_reusedvnodes);


#if CONFIG_MACF
	  
                                                      
                                         
    
	assert((vp->v_lflag & VL_LABELWAIT) != VL_LABELWAIT);
	assert((vp->v_lflag & VL_LABEL) != VL_LABEL);
	if (vp->v_lflag & VL_LABELED || vp->v_label != NULL) {
		vnode_lock_convert(vp);
		mac_vnode_label_recycle(vp);
	} else if (mac_vnode_label_init_needed(vp)) {
		vnode_lock_convert(vp);
		mac_vnode_label_init(vp);
	}

#endif          

	vp->v_iocount = 1;
	vp->v_lflag = 0;
	vp->v_writecount = 0;
	vp->v_references = 0;
	vp->v_iterblkflags = 0;
	vp->v_flag = VSTANDARD;
	                                          
	vp->v_mount = NULL;
	vp->v_defer_reclaimlist = (vnode_t)0;

	vnode_unlock(vp);

done:
	*vpp = vp;

	return 0;
}

void
vnode_lock(vnode_t vp)
{
	lck_mtx_lock(&vp->v_lock);
}

void
vnode_lock_spin(vnode_t vp)
{
	lck_mtx_lock_spin(&vp->v_lock);
}

void
vnode_unlock(vnode_t vp)
{
	lck_mtx_unlock(&vp->v_lock);
}



int
vnode_get(struct vnode *vp)
{
	int retval;

	vnode_lock_spin(vp);
	retval = vnode_get_locked(vp);
	vnode_unlock(vp);

	return retval;
}

int
vnode_get_locked(struct vnode *vp)
{
#if DIAGNOSTIC
	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);
#endif
	if ((vp->v_iocount == 0) && (vp->v_lflag & (VL_TERMINATE | VL_DEAD))) {
		return ENOENT;
	}

	if (os_add_overflow(vp->v_iocount, 1, &vp->v_iocount)) {
		panic("v_iocount overflow");
	}

#ifdef JOE_DEBUG
	record_vp(vp, 1);
#endif
	return 0;
}

  
                                                                      
                                                                      
                                                                     
                                                                 
   
int
vnode_getwithvid(vnode_t vp, uint32_t vid)
{
	return vget_internal(vp, vid, (VNODE_NODEAD | VNODE_WITHID | VNODE_DRAINO));
}

  
                                                                                         
                                                                                          
                                                 
   
int
vnode_getwithvid_drainok(vnode_t vp, uint32_t vid)
{
	return vget_internal(vp, vid, (VNODE_NODEAD | VNODE_WITHID));
}

int
vnode_getwithref(vnode_t vp)
{
	return vget_internal(vp, 0, 0);
}


__private_extern__ int
vnode_getalways(vnode_t vp)
{
	return vget_internal(vp, 0, VNODE_ALWAYS);
}

__private_extern__ int
vnode_getalways_from_pager(vnode_t vp)
{
	return vget_internal(vp, 0, VNODE_ALWAYS | VNODE_PAGER);
}

static inline void
vn_set_dead(vnode_t vp)
{
	vp->v_mount = NULL;
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	vp->v_data = NULL;
	vp->v_type = VBAD;
	vp->v_lflag |= VL_DEAD;
}

static int
vnode_put_internal_locked(vnode_t vp, bool from_pager)
{
	vfs_context_t ctx = vfs_context_current();                              

#if DIAGNOSTIC
	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);
#endif
retry:
	if (vp->v_iocount < 1) {
		panic("vnode_put(%p): iocount < 1", vp);
	}

	if ((vp->v_usecount > 0) || (vp->v_iocount > 1)) {
		vnode_dropiocount(vp);
		return 0;
	}

	if (((vp->v_lflag & (VL_DEAD | VL_NEEDINACTIVE)) == VL_NEEDINACTIVE)) {
		vp->v_lflag &= ~VL_NEEDINACTIVE;
		vnode_unlock(vp);

		VNOP_INACTIVE(vp, ctx);

		vnode_lock_spin(vp);
		  
                                                         
                                                               
                                              
                                                             
                                                            
                                                            
                             
     
		goto retry;
	}
	vp->v_lflag &= ~VL_NEEDINACTIVE;

	if ((vp->v_lflag & (VL_MARKTERM | VL_TERMINATE | VL_DEAD)) == VL_MARKTERM) {
		if (from_pager) {
			  
                                                          
                                                        
                                      
      
			if (VONLIST(vp)) {
				if (!(vp->v_listflag & VLIST_ASYNC_WORK)) {
					vnode_list_lock();
					vnode_list_remove_locked(vp);
					vnode_async_list_add_locked(vp);
					vnode_list_unlock();
				}
				wakeup(&vnode_async_work_list);
			} else {
				vnode_async_list_add(vp);
			}
		} else {
			vnode_lock_convert(vp);
			vnode_reclaim_internal(vp, 1, 1, 0);
		}
	}
	vnode_dropiocount(vp);
	vnode_list_add(vp);

	return 0;
}

int
vnode_put_locked(vnode_t vp)
{
	return vnode_put_internal_locked(vp, false);
}

int
vnode_put(vnode_t vp)
{
	int retval;

	vnode_lock_spin(vp);
	retval = vnode_put_internal_locked(vp, false);
	vnode_unlock(vp);

	return retval;
}

int
vnode_put_from_pager(vnode_t vp)
{
	int retval;

	vnode_lock_spin(vp);
	                                          
	retval = vnode_put_internal_locked(vp, true);
	vnode_unlock(vp);

	return retval;
}

                                   
int
vnode_isinuse(vnode_t vp, int refcnt)
{
	return vnode_isinuse_locked(vp, refcnt, 0);
}

int
vnode_usecount(vnode_t vp)
{
	return vp->v_usecount;
}

int
vnode_iocount(vnode_t vp)
{
	return vp->v_iocount;
}

int
vnode_isinuse_locked(vnode_t vp, int refcnt, int locked)
{
	int retval = 0;

	if (!locked) {
		vnode_lock_spin(vp);
	}
	if ((vp->v_type != VREG) && ((vp->v_usecount - vp->v_kusecount) > refcnt)) {
		retval = 1;
		goto out;
	}
	if (vp->v_type == VREG) {
		retval = ubc_isinuse_locked(vp, refcnt, 1);
	}

out:
	if (!locked) {
		vnode_unlock(vp);
	}
	return retval;
}


                    
errno_t
vnode_resume(vnode_t vp)
{
	if ((vp->v_lflag & VL_SUSPENDED) && vp->v_owner == current_thread()) {
		vnode_lock_spin(vp);
		vp->v_lflag &= ~VL_SUSPENDED;
		vp->v_owner = NULL;
		vnode_unlock(vp);

		wakeup(&vp->v_iocount);
	}
	return 0;
}

                  
                                                               
                   
                                                       
   

errno_t
vnode_suspend(vnode_t vp)
{
	if (vp->v_lflag & VL_SUSPENDED) {
		return EBUSY;
	}

	vnode_lock_spin(vp);

	  
                                                       
             
    

	if (vp->v_owner == NULL) {
		vp->v_lflag |= VL_SUSPENDED;
		vp->v_owner = current_thread();
	}
	vnode_unlock(vp);

	return 0;
}

  
                                                     
                            
  
                                      
   
static void
vnode_abort_advlocks(vnode_t vp)
{
	if (vp->v_flag & VLOCKLOCAL) {
		lf_abort_advlocks(vp);
	}
}


static errno_t
vnode_drain(vnode_t vp)
{
	if (vp->v_lflag & VL_DRAIN) {
		panic("vnode_drain: recursive drain");
		return ENOENT;
	}
	vp->v_lflag |= VL_DRAIN;
	vp->v_owner = current_thread();

	while (vp->v_iocount > 1) {
		if (bootarg_no_vnode_drain) {
			struct timespec ts = {.tv_sec = 10, .tv_nsec = 0};
			int error;

			if (vfs_unmountall_started) {
				ts.tv_sec = 1;
			}

			error = msleep(&vp->v_iocount, &vp->v_lock, PVFS, "vnode_drain_with_timeout", &ts);

			                                                                      
			if (vp->v_iocount > 1 && error == EWOULDBLOCK &&
			    ts.tv_sec == 1 && vp->v_numoutput == 0) {
				vp->v_iocount = 1;
				break;
			}
		} else {
			msleep(&vp->v_iocount, &vp->v_lock, PVFS, "vnode_drain", NULL);
		}
	}

	vp->v_lflag &= ~VL_DRAIN;

	return 0;
}


  
                                                                              
                                                                      
                                                                                  
                                                                                   
                                                                               
                                                                               
                                                                     
                                                                             
   
#define UNAGE_THRESHHOLD        25
#define UNAGE_DIRTYTHRESHHOLD    6

errno_t
vnode_getiocount(vnode_t vp, unsigned int vid, int vflags)
{
	int nodead = vflags & VNODE_NODEAD;
	int nosusp = vflags & VNODE_NOSUSPEND;
	int always = vflags & VNODE_ALWAYS;
	int beatdrain = vflags & VNODE_DRAINO;
	int withvid = vflags & VNODE_WITHID;
	int forpager = vflags & VNODE_PAGER;

	for (;;) {
		int sleepflg = 0;

		  
                                      
     
		if (nodead && (vp->v_lflag & VL_DEAD) && ((vp->v_type == VBAD) || (vp->v_data == 0))) {
			return ENOENT;
		}
		  
                             
     
		if ((vp->v_lflag & (VL_SUSPENDED | VL_DRAIN | VL_TERMINATE)) == 0) {
			break;
		}
		  
                                         
     
		if (nosusp && (vp->v_lflag & VL_SUSPENDED)) {
			return ENOENT;
		}
		  
                                                                            
                                                  
     
		if ((vp->v_lflag & (VL_DRAIN | VL_SUSPENDED | VL_TERMINATE)) &&
		    (vp->v_owner == current_thread())) {
			break;
		}

		if (always != 0) {
			break;
		}

		  
                                                                 
                                                         
                   
     
		if (vp->v_lflag & VL_DRAIN) {
			  
                                                   
                                                        
                                                          
                                                      
                     
      
			if (beatdrain) {
				break;
			}
			  
                                                             
                                                           
                                                                 
                                                            
                                                               
                                                               
                                               
      
			if (withvid && (vp->v_mount) && vfs_isunmount(vp->v_mount)) {
				return ENODEV;
			}

			if (vnode_istty(vp)) {
				sleepflg = PCATCH;
			}
		}

		vnode_lock_convert(vp);

		if (vp->v_lflag & VL_TERMINATE) {
			int error;

			vp->v_lflag |= VL_TERMWANT;

			error = msleep(&vp->v_lflag, &vp->v_lock,
			    (PVFS | sleepflg), "vnode getiocount", NULL);
			if (error) {
				return error;
			}
		} else {
			msleep(&vp->v_iocount, &vp->v_lock, PVFS, "vnode_getiocount", NULL);
		}
	}
	if (withvid && vid != vp->v_id) {
		return ENOENT;
	}
	if (!forpager && (++vp->v_references >= UNAGE_THRESHHOLD ||
	    (vp->v_flag & VISDIRTY && vp->v_references >= UNAGE_DIRTYTHRESHHOLD))) {
		vp->v_references = 0;
		vnode_list_remove(vp);
	}
	vp->v_iocount++;
#ifdef JOE_DEBUG
	record_vp(vp, 1);
#endif
	return 0;
}

static void
vnode_dropiocount(vnode_t vp)
{
	if (vp->v_iocount < 1) {
		panic("vnode_dropiocount(%p): v_iocount < 1", vp);
	}

	vp->v_iocount--;
#ifdef JOE_DEBUG
	record_vp(vp, -1);
#endif
	if ((vp->v_lflag & (VL_DRAIN | VL_SUSPENDED)) && (vp->v_iocount <= 1)) {
		wakeup(&vp->v_iocount);
	}
}


void
vnode_reclaim(struct vnode * vp)
{
	vnode_reclaim_internal(vp, 0, 0, 0);
}

__private_extern__
void
vnode_reclaim_internal(struct vnode * vp, int locked, int reuse, int flags)
{
	int isfifo = 0;
	bool clear_tty_revoke = false;

	if (!locked) {
		vnode_lock(vp);
	}

	if (vp->v_lflag & VL_TERMINATE) {
		panic("vnode reclaim in progress");
	}
	vp->v_lflag |= VL_TERMINATE;

	vn_clearunionwait(vp, 1);

	  
                                                                 
                                                                
                                                          
                          
    
	if (vnode_istty(vp) && (flags & REVOKEALL) && (vp->v_iocount > 1)) {
		vnode_unlock(vp);
		VNOP_IOCTL(vp, TIOCREVOKE, (caddr_t)NULL, 0, vfs_context_kernel());
		clear_tty_revoke = true;
		vnode_lock(vp);
	}

	vnode_drain(vp);

	if (clear_tty_revoke) {
		vnode_unlock(vp);
		VNOP_IOCTL(vp, TIOCREVOKECLEAR, (caddr_t)NULL, 0, vfs_context_kernel());
		vnode_lock(vp);
	}

	isfifo = (vp->v_type == VFIFO);

	if (vp->v_type != VBAD) {
		vgone(vp, flags);                                                
	}
	  
                                                                    
                                  
                                                      
                                                                          
                                                                    
                                                                        
                                 
                                                                           
                                                                           
                                                                          
                                                                              
                                                                          
                                                                           
                                           
    
	vnode_list_lock();

	vnode_list_remove_locked(vp);
	vp->v_id++;

	vnode_list_unlock();

	if (isfifo) {
		struct fifoinfo * fip;

		fip = vp->v_fifoinfo;
		vp->v_fifoinfo = NULL;
		kheap_free(KHEAP_DEFAULT, fip, sizeof(struct fifoinfo));
	}
	vp->v_type = VBAD;

	if (vp->v_data) {
		panic("vnode_reclaim_internal: cleaned vnode isn't");
	}
	if (vp->v_numoutput) {
		panic("vnode_reclaim_internal: clean vnode has pending I/O's");
	}
	if (UBCINFOEXISTS(vp)) {
		panic("vnode_reclaim_internal: ubcinfo not cleaned");
	}
	if (vp->v_parent) {
		panic("vnode_reclaim_internal: vparent not removed");
	}
	if (vp->v_name) {
		panic("vnode_reclaim_internal: vname not removed");
	}

	vp->v_socket = NULL;

	vp->v_lflag &= ~VL_TERMINATE;
	vp->v_owner = NULL;

	KNOTE(&vp->v_knotes, NOTE_REVOKE);

	                                                                 
	klist_init(&vp->v_knotes);

	if (vp->v_lflag & VL_TERMWANT) {
		vp->v_lflag &= ~VL_TERMWANT;
		wakeup(&vp->v_lflag);
	}
	if (!reuse) {
		  
                            
                             
     
		vnode_list_add(vp);
	}
	if (!locked) {
		vnode_unlock(vp);
	}
}

static int
vnode_create_internal(uint32_t flavor, uint32_t size, void *data, vnode_t *vpp,
    int init_vnode)
{
	int error;
	int insert = 1;
	int existing_vnode;
	vnode_t vp;
	vnode_t nvp;
	vnode_t dvp;
	struct  uthread *ut;
	struct componentname *cnp;
	struct vnode_fsparam *param = (struct vnode_fsparam *)data;
#if CONFIG_TRIGGERS
	struct vnode_trigger_param *tinfo = NULL;
#endif
	if (*vpp) {
		vp = *vpp;
		*vpp = NULLVP;
		existing_vnode = 1;
	} else {
		existing_vnode = 0;
	}

	if (init_vnode) {
		                                              
		if ((param == NULL) || (param->vnfs_vtype == VBAD)) {
			error = EINVAL;
			goto error_out;
		}

#if CONFIG_TRIGGERS
		if ((flavor == VNCREATE_TRIGGER) && (size == VNCREATE_TRIGGER_SIZE)) {
			tinfo = (struct vnode_trigger_param *)data;

			                                  
			if ((param->vnfs_vtype != VDIR) ||
			    (tinfo->vnt_resolve_func == NULL) ||
			    (tinfo->vnt_flags & ~VNT_VALID_MASK)) {
				error = EINVAL;
				goto error_out;
			}
			                                                            
			flavor = VNCREATE_FLAVOR;
			size = VCREATESIZE;
		}
#endif
		if ((flavor != VNCREATE_FLAVOR) || (size != VCREATESIZE)) {
			error = EINVAL;
			goto error_out;
		}
	}

	if (!existing_vnode) {
		if ((error = new_vnode(&vp))) {
			return error;
		}
		if (!init_vnode) {
			                                                        
			vn_set_dead(vp);
			*vpp = vp;
			return 0;
		}
	} else {
		  
                                                              
                                                               
                                             
    
                                                             
                                                               
                                                               
                                                            
                                                             
                                                      
     
		vp->v_lflag &= ~VL_DEAD;
	}

	dvp = param->vnfs_dvp;
	cnp = param->vnfs_cnp;

	vp->v_op = param->vnfs_vops;
	vp->v_type = (uint16_t)param->vnfs_vtype;
	vp->v_data = param->vnfs_fsnode;

	if (param->vnfs_markroot) {
		vp->v_flag |= VROOT;
	}
	if (param->vnfs_marksystem) {
		vp->v_flag |= VSYSTEM;
	}
	if (vp->v_type == VREG) {
		error = ubc_info_init_withsize(vp, param->vnfs_filesize);
		if (error) {
#ifdef JOE_DEBUG
			record_vp(vp, 1);
#endif
			vn_set_dead(vp);

			vnode_put(vp);
			return error;
		}
		if (param->vnfs_mp->mnt_ioflags & MNT_IOFLAGS_IOSCHED_SUPPORTED) {
			memory_object_mark_io_tracking(vp->v_ubcinfo->ui_control);
		}
	}
#ifdef JOE_DEBUG
	record_vp(vp, 1);
#endif

#if CONFIG_FIRMLINKS
	vp->v_fmlink = NULLVP;
#endif
	vp->v_flag &= ~VFMLINKTARGET;

#if CONFIG_TRIGGERS
	  
                                                    
    
	if ((vp->v_type == VDIR) && (tinfo != NULL)) {
		  
                                                                 
                                                          
                        
     
#ifdef JOE_DEBUG
		record_vp(vp, -1);
#endif
		error = vnode_resolver_create(param->vnfs_mp, vp, tinfo, FALSE);
		if (error) {
			printf("vnode_create: vnode_resolver_create() err %d\n", error);
			vn_set_dead(vp);
#ifdef JOE_DEBUG
			record_vp(vp, 1);
#endif
			vnode_put(vp);
			return error;
		}
	}
#endif
	if (vp->v_type == VCHR || vp->v_type == VBLK) {
		vp->v_tag = VT_DEVFS;                                                      

		if ((nvp = checkalias(vp, param->vnfs_rdev))) {
			  
                                                      
     
                                                     
      
			vp->v_data = NULL;
			vp->v_op = spec_vnodeop_p;
			vp->v_type = VBAD;
			vp->v_lflag = VL_DEAD;
			vp->v_data = NULL;
			vp->v_tag = VT_NON;
			vnode_put(vp);

			  
                                        
                  
      
			vp = nvp;

			vclean(vp, 0);
			vp->v_op = param->vnfs_vops;
			vp->v_type = (uint16_t)param->vnfs_vtype;
			vp->v_data = param->vnfs_fsnode;
			vp->v_lflag = 0;
			vp->v_mount = NULL;
			insmntque(vp, param->vnfs_mp);
			insert = 0;
			vnode_unlock(vp);
		}

		if (VCHR == vp->v_type) {
			u_int maj = major(vp->v_rdev);

			if (maj < (u_int)nchrdev && cdevsw[maj].d_type == D_TTY) {
				vp->v_flag |= VISTTY;
			}
		}
	}

	if (vp->v_type == VFIFO) {
		struct fifoinfo *fip;

		fip = kheap_alloc(KHEAP_DEFAULT, sizeof(struct fifoinfo),
		    Z_WAITOK | Z_ZERO);
		vp->v_fifoinfo = fip;
	}
	                                                               
                                                                      
                                                                         
                                                   
    
	*vpp = vp;

	                             
	if (param->vnfs_flags & VNFS_ADDFSREF) {
		vp->v_lflag |= VNAMED_FSHASH;
	}
	if (param->vnfs_mp) {
		if (param->vnfs_mp->mnt_kern_flag & MNTK_LOCK_LOCAL) {
			vp->v_flag |= VLOCKLOCAL;
		}
		if (insert) {
			if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb)) {
				panic("insmntque: vp on the free list\n");
			}

			  
                               
      
			insmntque(vp, param->vnfs_mp);
		}
	}
	if (dvp && vnode_ref(dvp) == 0) {
		vp->v_parent = dvp;
	}
	if (cnp) {
		if (dvp && ((param->vnfs_flags & (VNFS_NOCACHE | VNFS_CANTCACHE)) == 0)) {
			  
                           
                                                            
                                                           
                                            
      
			vp->v_name = cache_enter_create(dvp, vp, cnp);
		} else {
			vp->v_name = vfs_addname(cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_hash, 0);
		}

		if ((cnp->cn_flags & UNIONCREATED) == UNIONCREATED) {
			vp->v_flag |= VISUNION;
		}
	}
	if ((param->vnfs_flags & VNFS_CANTCACHE) == 0) {
		  
                                                               
                                               
     
		vp->v_flag |= VNCACHEABLE;
	}
	ut = get_bsdthread_info(current_thread());

	if ((current_proc()->p_lflag & P_LRAGE_VNODES) ||
	    (ut->uu_flag & (UT_RAGE_VNODES | UT_KERN_RAGE_VNODES))) {
		  
                                            
                                               
                                                
              
    
                                            
                                                 
                                               
         
     
		vp->v_flag |= VRAGE;
	}

#if CONFIG_SECLUDED_MEMORY
	switch (secluded_for_filecache) {
	case 0:
		  
                                 
                                        
     
		break;
	case 1:
		  
                                 
                      
                                       
                                                 
                                       
     
		if (vnode_vtype(vp) == VREG &&
		    vnode_mount(vp) != NULL &&
		    (!(vfs_flags(vnode_mount(vp)) & MNT_ROOTFS))) {
			                                                           
			memory_object_mark_eligible_for_secluded(
				ubc_getobject(vp, UBC_FLAGS_NONE),
				TRUE);
		}
		break;
	case 2:
		  
                                 
                                      
                                    
                  
                   
     
		if (vnode_vtype(vp) == VREG) {
			memory_object_mark_eligible_for_secluded(
				ubc_getobject(vp, UBC_FLAGS_NONE),
				TRUE);
		}
		break;
	default:
		break;
	}
#endif                             

	return 0;

error_out:
	if (existing_vnode) {
		vnode_put(vp);
	}
	return error;
}

         
                                                                                                
                                                                                                       
                        
   
int
vnode_create(uint32_t flavor, uint32_t size, void *data, vnode_t *vpp)
{
	*vpp = NULLVP;
	return vnode_create_internal(flavor, size, data, vpp, 1);
}

int
vnode_create_empty(vnode_t *vpp)
{
	*vpp = NULLVP;
	return vnode_create_internal(VNCREATE_FLAVOR, VCREATESIZE, NULL,
	           vpp, 0);
}

int
vnode_initialize(uint32_t flavor, uint32_t size, void *data, vnode_t *vpp)
{
	if (*vpp == NULLVP) {
		panic("NULL vnode passed to vnode_initialize");
	}
#if DEVELOPMENT || DEBUG
	  
                                                          
                          
    
	vnode_lock_spin(*vpp);
	VNASSERT(((*vpp)->v_iocount == 1), *vpp,
	    ("vnode_initialize : iocount not 1, is %d", (*vpp)->v_iocount));
	VNASSERT(((*vpp)->v_usecount == 0), *vpp,
	    ("vnode_initialize : usecount not 0, is %d", (*vpp)->v_usecount));
	VNASSERT(((*vpp)->v_lflag & VL_DEAD), *vpp,
	    ("vnode_initialize : v_lflag does not have VL_DEAD, is 0x%x",
	    (*vpp)->v_lflag));
	VNASSERT(((*vpp)->v_data == NULL), *vpp,
	    ("vnode_initialize : v_data not NULL"));
	vnode_unlock(*vpp);
#endif
	return vnode_create_internal(flavor, size, data, vpp, 1);
}

int
vnode_addfsref(vnode_t vp)
{
	vnode_lock_spin(vp);
	if (vp->v_lflag & VNAMED_FSHASH) {
		panic("add_fsref: vp already has named reference");
	}
	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb)) {
		panic("addfsref: vp on the free list\n");
	}
	vp->v_lflag |= VNAMED_FSHASH;
	vnode_unlock(vp);
	return 0;
}
int
vnode_removefsref(vnode_t vp)
{
	vnode_lock_spin(vp);
	if ((vp->v_lflag & VNAMED_FSHASH) == 0) {
		panic("remove_fsref: no named reference");
	}
	vp->v_lflag &= ~VNAMED_FSHASH;
	vnode_unlock(vp);
	return 0;
}


int
vfs_iterate(int flags, int (*callout)(mount_t, void *), void *arg)
{
	mount_t mp;
	int ret = 0;
	fsid_t * fsid_list;
	int count, actualcount, i;
	void * allocmem;
	int indx_start, indx_stop, indx_incr;
	int cb_dropref = (flags & VFS_ITERATE_CB_DROPREF);
	int noskip_unmount = (flags & VFS_ITERATE_NOSKIP_UNMOUNT);

	count = mount_getvfscnt();
	count += 10;

	fsid_list = kheap_alloc(KHEAP_TEMP, count * sizeof(fsid_t), Z_WAITOK);
	allocmem = (void *)fsid_list;

	actualcount = mount_fillfsids(fsid_list, count);

	  
                                     
                                                                            
    
	if (flags & VFS_ITERATE_TAIL_FIRST) {
		indx_start = actualcount - 1;
		indx_stop = -1;
		indx_incr = -1;
	} else {                            
		indx_start = 0;
		indx_stop = actualcount;
		indx_incr = 1;
	}

	for (i = indx_start; i != indx_stop; i += indx_incr) {
		                                                     
		mp = mount_list_lookupby_fsid(&fsid_list[i], 0, 1);

		if (mp == (struct mount *)0) {
			continue;
		}
		mount_lock(mp);
		if ((mp->mnt_lflag & MNT_LDEAD) ||
		    (!noskip_unmount && (mp->mnt_lflag & MNT_LUNMOUNT))) {
			mount_unlock(mp);
			mount_iterdrop(mp);
			continue;
		}
		mount_unlock(mp);

		                                 
		ret = callout(mp, arg);

		  
                                                        
                                                           
     
		if (!cb_dropref) {
			mount_iterdrop(mp);
		}

		switch (ret) {
		case VFS_RETURNED:
		case VFS_RETURNED_DONE:
			if (ret == VFS_RETURNED_DONE) {
				ret = 0;
				goto out;
			}
			break;

		case VFS_CLAIMED_DONE:
			ret = 0;
			goto out;
		case VFS_CLAIMED:
		default:
			break;
		}
		ret = 0;
	}

out:
	kheap_free(KHEAP_TEMP, allocmem, (count * sizeof(fsid_t)));
	return ret;
}

  
                                                    
                                                                    
                                                                
                                                                  
   
int
vfs_update_vfsstat(mount_t mp, vfs_context_t ctx, __unused int eventtype)
{
	struct vfs_attr va;
	int             error;

	  
                                                    
                                    
    
	VFSATTR_INIT(&va);
	VFSATTR_WANTED(&va, f_iosize);
	VFSATTR_WANTED(&va, f_blocks);
	VFSATTR_WANTED(&va, f_bfree);
	VFSATTR_WANTED(&va, f_bavail);
	VFSATTR_WANTED(&va, f_bused);
	VFSATTR_WANTED(&va, f_files);
	VFSATTR_WANTED(&va, f_ffree);
	VFSATTR_WANTED(&va, f_bsize);
	VFSATTR_WANTED(&va, f_fssubtype);

	if ((error = vfs_getattr(mp, &va, ctx)) != 0) {
		KAUTH_DEBUG("STAT - filesystem returned error %d", error);
		return error;
	}
#if CONFIG_MACF
	if (eventtype == VFS_USER_EVENT) {
		error = mac_mount_check_getattr(ctx, mp, &va);
		if (error != 0) {
			return error;
		}
	}
#endif
	  
                                        
   
                                                               
            
           
            
           
           
           
   
                                                              
                       
           
            
               
   
    
	if (VFSATTR_IS_SUPPORTED(&va, f_bsize)) {
		                                                      
		mp->mnt_vfsstat.f_bsize = (va.f_bsize > 0 ? va.f_bsize : 512);
	} else {
		mp->mnt_vfsstat.f_bsize = mp->mnt_devblocksize;                                         
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_iosize)) {
		mp->mnt_vfsstat.f_iosize = va.f_iosize;
	} else {
		mp->mnt_vfsstat.f_iosize = 1024 * 1024;                                    
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_blocks)) {
		mp->mnt_vfsstat.f_blocks = va.f_blocks;
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_bfree)) {
		mp->mnt_vfsstat.f_bfree = va.f_bfree;
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_bavail)) {
		mp->mnt_vfsstat.f_bavail = va.f_bavail;
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_bused)) {
		mp->mnt_vfsstat.f_bused = va.f_bused;
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_files)) {
		mp->mnt_vfsstat.f_files = va.f_files;
	}
	if (VFSATTR_IS_SUPPORTED(&va, f_ffree)) {
		mp->mnt_vfsstat.f_ffree = va.f_ffree;
	}

	                                                           
	if (VFSATTR_IS_SUPPORTED(&va, f_fssubtype)) {
		mp->mnt_vfsstat.f_fssubtype = va.f_fssubtype;
	}

	return 0;
}

int
mount_list_add(mount_t mp)
{
	int res;

	mount_list_lock();
	if (get_system_inshutdown() != 0) {
		res = -1;
	} else {
		TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);
		nummounts++;
		res = 0;
	}
	mount_list_unlock();

	return res;
}

void
mount_list_remove(mount_t mp)
{
	mount_list_lock();
	TAILQ_REMOVE(&mountlist, mp, mnt_list);
	nummounts--;
	mp->mnt_list.tqe_next = NULL;
	mp->mnt_list.tqe_prev = NULL;
	mount_list_unlock();
}

mount_t
mount_lookupby_volfsid(int volfs_id, int withref)
{
	mount_t cur_mount = (mount_t)0;
	mount_t mp;

	mount_list_lock();
	TAILQ_FOREACH(mp, &mountlist, mnt_list) {
		if (!(mp->mnt_kern_flag & MNTK_UNMOUNT) &&
		    (mp->mnt_kern_flag & MNTK_PATH_FROM_ID) &&
		    (mp->mnt_vfsstat.f_fsid.val[0] == volfs_id)) {
			cur_mount = mp;
			if (withref) {
				if (mount_iterref(cur_mount, 1)) {
					cur_mount = (mount_t)0;
					mount_list_unlock();
					goto out;
				}
			}
			break;
		}
	}
	mount_list_unlock();
	if (withref && (cur_mount != (mount_t)0)) {
		mp = cur_mount;
		if (vfs_busy(mp, LK_NOWAIT) != 0) {
			cur_mount = (mount_t)0;
		}
		mount_iterdrop(mp);
	}
out:
	return cur_mount;
}

mount_t
mount_list_lookupby_fsid(fsid_t *fsid, int locked, int withref)
{
	mount_t retmp = (mount_t)0;
	mount_t mp;

	if (!locked) {
		mount_list_lock();
	}
	TAILQ_FOREACH(mp, &mountlist, mnt_list)
	if (mp->mnt_vfsstat.f_fsid.val[0] == fsid->val[0] &&
	    mp->mnt_vfsstat.f_fsid.val[1] == fsid->val[1]) {
		retmp = mp;
		if (withref) {
			if (mount_iterref(retmp, 1)) {
				retmp = (mount_t)0;
			}
		}
		goto out;
	}
out:
	if (!locked) {
		mount_list_unlock();
	}
	return retmp;
}

errno_t
vnode_lookupat(const char *path, int flags, vnode_t *vpp, vfs_context_t ctx,
    vnode_t start_dvp)
{
	struct nameidata *ndp;
	int error = 0;
	u_int32_t ndflags = 0;

	if (ctx == NULL) {
		return EINVAL;
	}

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
	if (!ndp) {
		return ENOMEM;
	}

	if (flags & VNODE_LOOKUP_NOFOLLOW) {
		ndflags = NOFOLLOW;
	} else {
		ndflags = FOLLOW;
	}

	if (flags & VNODE_LOOKUP_NOCROSSMOUNT) {
		ndflags |= NOCROSSMOUNT;
	}

	if (flags & VNODE_LOOKUP_CROSSMOUNTNOWAIT) {
		ndflags |= CN_NBMOUNTLOOK;
	}

	                               
	NDINIT(ndp, LOOKUP, OP_LOOKUP, ndflags, UIO_SYSSPACE,
	    CAST_USER_ADDR_T(path), ctx);

	if (start_dvp && (path[0] != '/')) {
		ndp->ni_dvp = start_dvp;
		ndp->ni_cnd.cn_flags |= USEDVP;
	}

	if ((error = namei(ndp))) {
		goto out_free;
	}

	ndp->ni_cnd.cn_flags &= ~USEDVP;

	*vpp = ndp->ni_vp;
	nameidone(ndp);

out_free:
	kheap_free(KHEAP_TEMP, ndp, sizeof(struct nameidata));
	return error;
}

errno_t
vnode_lookup(const char *path, int flags, vnode_t *vpp, vfs_context_t ctx)
{
	return vnode_lookupat(path, flags, vpp, ctx, NULLVP);
}

errno_t
vnode_open(const char *path, int fmode, int cmode, int flags, vnode_t *vpp, vfs_context_t ctx)
{
	struct nameidata *ndp = NULL;
	int error;
	u_int32_t ndflags = 0;
	int lflags = flags;

	if (ctx == NULL) {                                            
		ctx = vfs_context_current();
	}

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
	if (!ndp) {
		return ENOMEM;
	}

	if (fmode & O_NOFOLLOW) {
		lflags |= VNODE_LOOKUP_NOFOLLOW;
	}

	if (lflags & VNODE_LOOKUP_NOFOLLOW) {
		ndflags = NOFOLLOW;
	} else {
		ndflags = FOLLOW;
	}

	if (lflags & VNODE_LOOKUP_NOCROSSMOUNT) {
		ndflags |= NOCROSSMOUNT;
	}

	if (lflags & VNODE_LOOKUP_CROSSMOUNTNOWAIT) {
		ndflags |= CN_NBMOUNTLOOK;
	}

	                               
	NDINIT(ndp, LOOKUP, OP_OPEN, ndflags, UIO_SYSSPACE,
	    CAST_USER_ADDR_T(path), ctx);

	if ((error = vn_open(ndp, fmode, cmode))) {
		*vpp = NULL;
	} else {
		*vpp = ndp->ni_vp;
	}

	kheap_free(KHEAP_TEMP, ndp, sizeof(struct nameidata));
	return error;
}

errno_t
vnode_close(vnode_t vp, int flags, vfs_context_t ctx)
{
	int error;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = vn_close(vp, flags, ctx);
	vnode_put(vp);
	return error;
}

errno_t
vnode_mtime(vnode_t vp, struct timespec *mtime, vfs_context_t ctx)
{
	struct vnode_attr       va;
	int                     error;

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_modify_time);
	error = vnode_getattr(vp, &va, ctx);
	if (!error) {
		*mtime = va.va_modify_time;
	}
	return error;
}

errno_t
vnode_flags(vnode_t vp, uint32_t *flags, vfs_context_t ctx)
{
	struct vnode_attr       va;
	int                     error;

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_flags);
	error = vnode_getattr(vp, &va, ctx);
	if (!error) {
		*flags = va.va_flags;
	}
	return error;
}

  
                       
                    
   
errno_t
vnode_size(vnode_t vp, off_t *sizep, vfs_context_t ctx)
{
	struct vnode_attr       va;
	int                     error;

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_data_size);
	error = vnode_getattr(vp, &va, ctx);
	if (!error) {
		*sizep = va.va_data_size;
	}
	return error;
}

errno_t
vnode_setsize(vnode_t vp, off_t size, int ioflag, vfs_context_t ctx)
{
	struct vnode_attr       va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_data_size, size);
	va.va_vaflags = ioflag & 0xffff;
	return vnode_setattr(vp, &va, ctx);
}

int
vnode_setdirty(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VISDIRTY;
	vnode_unlock(vp);
	return 0;
}

int
vnode_cleardirty(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VISDIRTY;
	vnode_unlock(vp);
	return 0;
}

int
vnode_isdirty(vnode_t vp)
{
	int dirty;

	vnode_lock_spin(vp);
	dirty = (vp->v_flag & VISDIRTY) ? 1 : 0;
	vnode_unlock(vp);

	return dirty;
}

static int
vn_create_reg(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, struct vnode_attr *vap, uint32_t flags, int fmode, uint32_t *statusp, vfs_context_t ctx)
{
	                                                   
	if (vnode_compound_open_available(dvp) && ((flags & VN_CREATE_DOOPEN) != 0)) {
		*vpp = NULLVP;
		return VNOP_COMPOUND_OPEN(dvp, vpp, ndp, O_CREAT, fmode, statusp, vap, ctx);
	} else {
		return VNOP_CREATE(dvp, vpp, &ndp->ni_cnd, vap, ctx);
	}
}

  
                                                                            
                                                 
  
                                                          
                                     
                                           
                                              
                                               
                                             
                                 
                                             
                                           
                                    
                                                  
                                             
                                     
  
                       
                    
  
                                                         
                                  
                                                 
                                                 
                                            
  
  
                                            
  
  
                                                                      
  
                                                              
                         
  
                                                                      
  
                                                                       
                                
   
errno_t
vn_create(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, struct vnode_attr *vap, uint32_t flags, int fmode, uint32_t *statusp, vfs_context_t ctx)
{
	errno_t error, old_error;
	vnode_t vp = (vnode_t)0;
	boolean_t batched;
	struct componentname *cnp;
	uint32_t defaulted;

	cnp = &ndp->ni_cnd;
	error = 0;
	batched = namei_compound_available(dvp, ndp) ? TRUE : FALSE;

	KAUTH_DEBUG("%p    CREATE - '%s'", dvp, cnp->cn_nameptr);

	if (flags & VN_CREATE_NOINHERIT) {
		vap->va_vaflags |= VA_NOINHERIT;
	}
	if (flags & VN_CREATE_NOAUTH) {
		vap->va_vaflags |= VA_NOAUTH;
	}
	  
                                           
    
	error = vn_attribute_prepare(dvp, vap, &defaulted, ctx);
	if (error) {
		return error;
	}

	if (vap->va_type != VREG && (fmode != 0 || (flags & VN_CREATE_DOOPEN) || statusp)) {
		panic("Open parameters, but not a regular file.");
	}
	if ((fmode != 0) && ((flags & VN_CREATE_DOOPEN) == 0)) {
		panic("Mode for open, but not trying to open...");
	}


	  
                              
    
	switch (vap->va_type) {
	case VREG:
		error = vn_create_reg(dvp, vpp, ndp, vap, flags, fmode, statusp, ctx);
		break;
	case VDIR:
		error = vn_mkdir(dvp, vpp, ndp, vap, ctx);
		break;
	case VSOCK:
	case VFIFO:
	case VBLK:
	case VCHR:
		error = VNOP_MKNOD(dvp, vpp, cnp, vap, ctx);
		break;
	default:
		panic("vnode_create: unknown vtype %d", vap->va_type);
	}
	if (error != 0) {
		KAUTH_DEBUG("%p    CREATE - error %d returned by filesystem", dvp, error);
		goto out;
	}

	vp = *vpp;
	old_error = error;

	  
                                                                    
                          
    
	if ((error == 0) && !VATTR_ALL_SUPPORTED(vap) && *vpp) {
		KAUTH_DEBUG("     CREATE - doing fallback with ACL %p", vap->va_acl);
		error = vnode_setattr_fallback(*vpp, vap, ctx);
	}

#if CONFIG_MACF
	if ((error == 0) && !(flags & VN_CREATE_NOLABEL)) {
		error = vnode_label(vnode_mount(vp), dvp, vp, cnp, VNODE_LABEL_CREATE, ctx);
	}
#endif

	if ((error != 0) && (vp != (vnode_t)0)) {
		                                          
		if (batched && (old_error == 0) && (vap->va_type == VREG)) {
			VNOP_CLOSE(vp, fmode, ctx);
		}

		                                                         
		if (!batched) {
			*vpp = (vnode_t) 0;
			vnode_put(vp);
			vp = NULLVP;
		}
	}

	  
                                                 
                              
    
	if (kdebug_enable && *vpp) {
		kdebug_lookup(*vpp, cnp);
	}

out:
	vn_attribute_cleanup(vap, defaulted);

	return error;
}

static kauth_scope_t    vnode_scope;
static int      vnode_authorize_callback(kauth_cred_t credential, void *idata, kauth_action_t action,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
static int vnode_authorize_callback_int(kauth_action_t action, vfs_context_t ctx,
    vnode_t vp, vnode_t dvp, int *errorp);

typedef struct _vnode_authorize_context {
	vnode_t         vp;
	struct vnode_attr *vap;
	vnode_t         dvp;
	struct vnode_attr *dvap;
	vfs_context_t   ctx;
	int             flags;
	int             flags_valid;
#define _VAC_IS_OWNER           (1<<0)
#define _VAC_IN_GROUP           (1<<1)
#define _VAC_IS_DIR_OWNER       (1<<2)
#define _VAC_IN_DIR_GROUP       (1<<3)
#define _VAC_NO_VNODE_POINTERS  (1<<4)
} *vauth_ctx;

void
vnode_authorize_init(void)
{
	vnode_scope = kauth_register_scope(KAUTH_SCOPE_VNODE, vnode_authorize_callback, NULL);
}

#define VATTR_PREPARE_DEFAULTED_UID             0x1
#define VATTR_PREPARE_DEFAULTED_GID             0x2
#define VATTR_PREPARE_DEFAULTED_MODE            0x4

int
vn_attribute_prepare(vnode_t dvp, struct vnode_attr *vap, uint32_t *defaulted_fieldsp, vfs_context_t ctx)
{
	kauth_acl_t nacl = NULL, oacl = NULL;
	int error;

	  
                           
    
	if (!(vap->va_vaflags & VA_NOINHERIT) && vfs_extendedsecurity(dvp->v_mount)) {
		                               
		if (VATTR_IS_ACTIVE(vap, va_acl)) {
			oacl = vap->va_acl;
		}

		vap->va_acl = NULL;
		if ((error = kauth_acl_inherit(dvp,
		    oacl,
		    &nacl,
		    vap->va_type == VDIR,
		    ctx)) != 0) {
			KAUTH_DEBUG("%p    CREATE - error %d processing inheritance", dvp, error);
			return error;
		}

		  
                                                                         
                                
     
		if (nacl == NULL) {
			VATTR_CLEAR_ACTIVE(vap, va_acl);
		} else {
			vap->va_base_acl = oacl;
			VATTR_SET(vap, va_acl, nacl);
		}
	}

	error = vnode_authattr_new_internal(dvp, vap, (vap->va_vaflags & VA_NOAUTH), defaulted_fieldsp, ctx);
	if (error) {
		vn_attribute_cleanup(vap, *defaulted_fieldsp);
	}

	return error;
}

void
vn_attribute_cleanup(struct vnode_attr *vap, uint32_t defaulted_fields)
{
	  
                                                                 
                                                                       
                                   
    
	kauth_acl_t nacl, oacl;

	if (VATTR_IS_ACTIVE(vap, va_acl)) {
		nacl = vap->va_acl;
		oacl = vap->va_base_acl;

		if (oacl) {
			VATTR_SET(vap, va_acl, oacl);
			vap->va_base_acl = NULL;
		} else {
			VATTR_CLEAR_ACTIVE(vap, va_acl);
		}

		if (nacl != NULL) {
			kauth_acl_free(nacl);
		}
	}

	if ((defaulted_fields & VATTR_PREPARE_DEFAULTED_MODE) != 0) {
		VATTR_CLEAR_ACTIVE(vap, va_mode);
	}
	if ((defaulted_fields & VATTR_PREPARE_DEFAULTED_GID) != 0) {
		VATTR_CLEAR_ACTIVE(vap, va_gid);
	}
	if ((defaulted_fields & VATTR_PREPARE_DEFAULTED_UID) != 0) {
		VATTR_CLEAR_ACTIVE(vap, va_uid);
	}

	return;
}

int
vn_authorize_unlink(vnode_t dvp, vnode_t vp, struct componentname *cnp, vfs_context_t ctx, __unused void *reserved)
{
#if !CONFIG_MACF
#pragma unused(cnp)
#endif
	int error = 0;

	  
                                                        
                                                        
    
	if ((vp->v_type == VDIR) &&
	    !(vp->v_mount->mnt_kern_flag & MNTK_DIR_HARDLINKS)) {
		return EPERM;            
	}

	                                    
#if CONFIG_MACF
	if (!error) {
		error = mac_vnode_check_unlink(ctx, dvp, vp, cnp);
	}
#endif          
	if (!error) {
		error = vnode_authorize(vp, dvp, KAUTH_VNODE_DELETE, ctx);
	}

	return error;
}

int
vn_authorize_open_existing(vnode_t vp, struct componentname *cnp, int fmode, vfs_context_t ctx, void *reserved)
{
	                           
	kauth_action_t action;
	int error = 0;
	if (cnp->cn_ndp == NULL) {
		panic("NULL ndp");
	}
	if (reserved != NULL) {
		panic("reserved not NULL.");
	}

#if CONFIG_MACF
	                                                                          
	if (vfs_flags(vnode_mount(vp)) & MNT_MULTILABEL) {
		error = vnode_label(vnode_mount(vp), NULL, vp, NULL, 0, ctx);
		if (error) {
			return error;
		}
	}
#endif

	if ((fmode & O_DIRECTORY) && vp->v_type != VDIR) {
		return ENOTDIR;
	}

	if (vp->v_type == VSOCK && vp->v_tag != VT_FDESC) {
		return EOPNOTSUPP;                                           
	}

	if (vp->v_type == VLNK && (fmode & O_NOFOLLOW) != 0) {
		return ELOOP;                                                                         
	}

	                                              
	if (vnode_isdir(vp) && (fmode & (FWRITE | O_TRUNC))) {
		return EISDIR;
	}

	if ((cnp->cn_ndp->ni_flag & NAMEI_TRAILINGSLASH)) {
		if (vp->v_type != VDIR) {
			return ENOTDIR;
		}
	}

#if CONFIG_MACF
	                                                     
                                                       
                                                      
               
    
	if (!(vnode_isshadow(vp) && vnode_isnamedstream(vp))) {
		error = mac_vnode_check_open(ctx, vp, fmode);
		if (error) {
			return error;
		}
	}
#endif

	                                     
	action = 0;
	if (fmode & FREAD) {
		action |= KAUTH_VNODE_READ_DATA;
	}
	if (fmode & (FWRITE | O_TRUNC)) {
		  
                                                      
                                                  
                                                        
              
     
		if ((fmode & O_APPEND) && !(fmode & O_TRUNC)) {
			action |= KAUTH_VNODE_APPEND_DATA;
		} else {
			action |= KAUTH_VNODE_WRITE_DATA;
		}
	}
	error = vnode_authorize(vp, NULL, action, ctx);
#if NAMEDSTREAMS
	if (error == EACCES) {
		  
                                                            
                                                             
                                                             
                                  
     
		if (vnode_isshadow(vp) && vnode_isnamedstream(vp)) {
			error = vnode_verifynamedstream(vp);
		}
	}
#endif

	return error;
}

int
vn_authorize_create(vnode_t dvp, struct componentname *cnp, struct vnode_attr *vap, vfs_context_t ctx, void *reserved)
{
#if !CONFIG_MACF
#pragma unused(vap)
#endif
	                   
	int error;

	if (cnp->cn_ndp == NULL) {
		panic("NULL cn_ndp");
	}
	if (reserved != NULL) {
		panic("reserved not NULL.");
	}

	                                                                       
	if (cnp->cn_ndp->ni_flag & NAMEI_UNFINISHED) {
		error = lookup_validate_creation_path(cnp->cn_ndp);
		if (error) {
			return error;
		}
	}

#if CONFIG_MACF
	error = mac_vnode_check_create(ctx, dvp, cnp, vap);
	if (error) {
		return error;
	}
#endif                  

	return vnode_authorize(dvp, NULL, KAUTH_VNODE_ADD_FILE, ctx);
}

int
vn_authorize_rename(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp,
    struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp,
    vfs_context_t ctx, void *reserved)
{
	return vn_authorize_renamex(fdvp, fvp, fcnp, tdvp, tvp, tcnp, ctx, 0, reserved);
}

int
vn_authorize_renamex(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp,
    struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp,
    vfs_context_t ctx, vfs_rename_flags_t flags, void *reserved)
{
	return vn_authorize_renamex_with_paths(fdvp, fvp, fcnp, NULL, tdvp, tvp, tcnp, NULL, ctx, flags, reserved);
}

int
vn_authorize_renamex_with_paths(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp, const char *from_path,
    struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp, const char *to_path,
    vfs_context_t ctx, vfs_rename_flags_t flags, void *reserved)
{
	int error = 0;
	int moving = 0;
	bool swap = flags & VFS_RENAME_SWAP;

	if (reserved != NULL) {
		panic("Passed something other than NULL as reserved field!");
	}

	  
                                
   
                                                                              
                        
    
	if (fvp->v_type == VDIR &&
	    ((fdvp == fvp) ||
	    (fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
	    ((fcnp->cn_flags | tcnp->cn_flags) & ISDOTDOT))) {
		error = EINVAL;
		goto out;
	}

	if (tvp == NULLVP && vnode_compound_rename_available(tdvp)) {
		error = lookup_validate_creation_path(tcnp->cn_ndp);
		if (error) {
			goto out;
		}
	}

	                    
#if CONFIG_MACF
	error = mac_vnode_check_rename(ctx, fdvp, fvp, fcnp, tdvp, tvp, tcnp);
	if (error) {
		goto out;
	}
	if (swap) {
		error = mac_vnode_check_rename(ctx, tdvp, tvp, tcnp, fdvp, fvp, fcnp);
		if (error) {
			goto out;
		}
	}
#endif
	                     

	                          
	if (tvp != NULL) {
		if (!swap) {
			if (fvp->v_type == VDIR && tvp->v_type != VDIR) {
				error = ENOTDIR;
				goto out;
			} else if (fvp->v_type != VDIR && tvp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
		}
	} else if (swap) {
		  
                                                         
                                                            
                                                         
             
     
		error = EINVAL;
		goto out;
	}

	if (fvp == tdvp) {
		error = EINVAL;
		goto out;
	}

	  
                                           
                                       
   
                
          
         
            
         
          
                
          
         
            
    
	if (tdvp->v_parent == fvp) {
		error = EINVAL;
		goto out;
	}

	if (swap && fdvp->v_parent == tvp) {
		error = EINVAL;
		goto out;
	}
	                           

	                     

	  
                                                             
                                                             
                                                              
                                                               
                                                             
                                                             
                                                               
                                                         
   
                                                          
                                                           
                                                         
                                                           
                                    
    

	if (swap) {
		kauth_action_t f = 0, t = 0;

		  
                                                           
                         
     
		if (fdvp != tdvp) {
			if (vnode_isdir(fvp)) {
				f = KAUTH_VNODE_ADD_SUBDIRECTORY;
			}
			if (vnode_isdir(tvp)) {
				t = KAUTH_VNODE_ADD_SUBDIRECTORY;
			}
		}
		if (to_path != NULL) {
			kauth_authorize_fileop(vfs_context_ucred(ctx),
			    KAUTH_FILEOP_WILL_RENAME,
			    (uintptr_t)fvp,
			    (uintptr_t)to_path);
		}
		error = vnode_authorize(fvp, fdvp, KAUTH_VNODE_DELETE | f, ctx);
		if (error) {
			goto out;
		}
		if (from_path != NULL) {
			kauth_authorize_fileop(vfs_context_ucred(ctx),
			    KAUTH_FILEOP_WILL_RENAME,
			    (uintptr_t)tvp,
			    (uintptr_t)from_path);
		}
		error = vnode_authorize(tvp, tdvp, KAUTH_VNODE_DELETE | t, ctx);
		if (error) {
			goto out;
		}
		f = vnode_isdir(fvp) ? KAUTH_VNODE_ADD_SUBDIRECTORY : KAUTH_VNODE_ADD_FILE;
		t = vnode_isdir(tvp) ? KAUTH_VNODE_ADD_SUBDIRECTORY : KAUTH_VNODE_ADD_FILE;
		if (fdvp == tdvp) {
			error = vnode_authorize(fdvp, NULL, f | t, ctx);
		} else {
			error = vnode_authorize(fdvp, NULL, t, ctx);
			if (error) {
				goto out;
			}
			error = vnode_authorize(tdvp, NULL, f, ctx);
		}
		if (error) {
			goto out;
		}
	} else {
		error = 0;
		if ((tvp != NULL) && vnode_isdir(tvp)) {
			if (tvp != fdvp) {
				moving = 1;
			}
		} else if (tdvp != fdvp) {
			moving = 1;
		}

		  
                                                           
                                     
    
                                                            
                                                                
     
		if (to_path != NULL) {
			kauth_authorize_fileop(vfs_context_ucred(ctx),
			    KAUTH_FILEOP_WILL_RENAME,
			    (uintptr_t)fvp,
			    (uintptr_t)to_path);
		}
		if (vnode_isdir(fvp)) {
			if ((error = vnode_authorize(fvp, fdvp, KAUTH_VNODE_DELETE | KAUTH_VNODE_ADD_SUBDIRECTORY, ctx)) != 0) {
				goto out;
			}
		} else {
			if ((error = vnode_authorize(fvp, fdvp, KAUTH_VNODE_DELETE, ctx)) != 0) {
				goto out;
			}
		}
		if (moving) {
			                                                      
			if ((error = vnode_authorize(((tvp != NULL) && vnode_isdir(tvp)) ? tvp : tdvp,
			    NULL,
			    vnode_isdir(fvp) ? KAUTH_VNODE_ADD_SUBDIRECTORY : KAUTH_VNODE_ADD_FILE,
			    ctx)) != 0) {
				goto out;
			}
		} else {
			                                                                     
			if ((error = vnode_authorize(fdvp, NULL,
			    vnode_isdir(fvp) ? KAUTH_VNODE_ADD_SUBDIRECTORY : KAUTH_VNODE_ADD_FILE, ctx)) != 0) {
				goto out;
			}
		}
		                     
		if ((tvp != NULL) && !vnode_isdir(tvp) &&
		    ((error = vnode_authorize(tvp, tdvp, KAUTH_VNODE_DELETE, ctx)) != 0)) {
			goto out;
		}
	}

	                      

	                      
out:
	return error;
}

int
vn_authorize_mkdir(vnode_t dvp, struct componentname *cnp, struct vnode_attr *vap, vfs_context_t ctx, void *reserved)
{
#if !CONFIG_MACF
#pragma unused(vap)
#endif
	int error;

	if (reserved != NULL) {
		panic("reserved not NULL in vn_authorize_mkdir()");
	}

	                                                   
	if (cnp->cn_ndp == NULL) {
		return 0;
	}

	if (vnode_compound_mkdir_available(dvp)) {
		error = lookup_validate_creation_path(cnp->cn_ndp);
		if (error) {
			goto out;
		}
	}

#if CONFIG_MACF
	error = mac_vnode_check_create(ctx,
	    dvp, cnp, vap);
	if (error) {
		goto out;
	}
#endif

	                                                     
	if ((error = vnode_authorize(dvp, NULL, KAUTH_VNODE_ADD_SUBDIRECTORY, ctx)) != 0) {
		goto out;
	}

out:
	return error;
}

int
vn_authorize_rmdir(vnode_t dvp, vnode_t vp, struct componentname *cnp, vfs_context_t ctx, void *reserved)
{
#if CONFIG_MACF
	int error;
#else
#pragma unused(cnp)
#endif
	if (reserved != NULL) {
		panic("Non-NULL reserved argument to vn_authorize_rmdir()");
	}

	if (vp->v_type != VDIR) {
		  
                                      
     
		return ENOTDIR;
	}

	if (dvp == vp) {
		  
                         
     
		return EINVAL;
	}

#if CONFIG_MACF
	error = mac_vnode_check_unlink(ctx, dvp,
	    vp, cnp);
	if (error) {
		return error;
	}
#endif

	return vnode_authorize(vp, dvp, KAUTH_VNODE_DELETE, ctx);
}

  
                                                                         
                                                       
  
                                                                             
                                          
   
int
vnode_attr_authorize_dir_clone(struct vnode_attr *vap, kauth_action_t action,
    struct vnode_attr *dvap, __unused vnode_t sdvp, mount_t mp,
    dir_clone_authorizer_op_t vattr_op, uint32_t flags, vfs_context_t ctx,
    __unused void *reserved)
{
	int error;
	int is_suser = vfs_context_issuser(ctx);

	if (vattr_op == OP_VATTR_SETUP) {
		VATTR_INIT(vap);

		  
                                                              
                                                       
     
		VATTR_WANTED(vap, va_type);
		VATTR_WANTED(vap, va_mode);
		VATTR_WANTED(vap, va_flags);
		VATTR_WANTED(vap, va_uid);
		VATTR_WANTED(vap, va_gid);
		if (dvap) {
			VATTR_INIT(dvap);
			VATTR_WANTED(dvap, va_flags);
		}

		if (!is_suser) {
			  
                                                    
                                                      
                            
      
			VATTR_WANTED(vap, va_acl);
			if (dvap) {
				VATTR_WANTED(dvap, va_gid);
			}
		} else if (dvap && (flags & VNODE_CLONEFILE_NOOWNERCOPY)) {
			VATTR_WANTED(dvap, va_gid);
		}
		return 0;
	} else if (vattr_op == OP_VATTR_CLEANUP) {
		return 0;                            
	}

	                                       
	error = vnode_attr_authorize(vap, NULL, mp, action, ctx);

	if (error) {
		return error;
	}

	  
                                                                       
                                         
    
	if (!is_suser || (flags & VNODE_CLONEFILE_NOOWNERCOPY)) {
		  
                                                                  
                                                               
     
		if ((mp->mnt_flag & MNT_IGNORE_OWNERSHIP) &&
		    (mp->mnt_fsowner != KAUTH_UID_NONE)) {
			VATTR_SET(vap, va_uid, mp->mnt_fsowner);
		} else {
			                                   
			VATTR_SET(vap, va_uid,
			    kauth_cred_getuid(vfs_context_ucred(ctx)));
		}

		if ((mp->mnt_flag & MNT_IGNORE_OWNERSHIP) &&
		    (mp->mnt_fsgroup != KAUTH_GID_NONE)) {
			VATTR_SET(vap, va_gid, mp->mnt_fsgroup);
		} else {
			  
                                             
                              
      
			if (VATTR_IS_SUPPORTED(dvap, va_gid)) {
				VATTR_SET(vap, va_gid, dvap->va_gid);
			} else {
				VATTR_SET(vap, va_gid,
				    kauth_cred_getgid(vfs_context_ucred(ctx)));
			}
		}
	}

	                                                               
	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		VATTR_SET(vap, va_flags,
		    ((vap->va_flags & ~(UF_DATAVAULT | SF_RESTRICTED))));                           
		if (VATTR_IS_ACTIVE(dvap, va_flags)) {
			VATTR_SET(vap, va_flags,
			    vap->va_flags | (dvap->va_flags & (UF_DATAVAULT | SF_RESTRICTED)));
		}
	} else if (VATTR_IS_ACTIVE(dvap, va_flags)) {
		VATTR_SET(vap, va_flags, (dvap->va_flags & (UF_DATAVAULT | SF_RESTRICTED)));
	}

	return 0;
}


  
                                     
  
                                                      
  
                       
                                   
                                      
                                   
                                                               
                                               
                                               
             
         
          
         
       
   
int
vnode_authorize(vnode_t vp, vnode_t dvp, kauth_action_t action, vfs_context_t ctx)
{
	int     error, result;

	  
                                                                                 
                                      
    
	if (vp->v_type == VBAD) {
		return 0;
	}

	error = 0;
	result = kauth_authorize_action(vnode_scope, vfs_context_ucred(ctx), action,
	    (uintptr_t)ctx, (uintptr_t)vp, (uintptr_t)dvp, (uintptr_t)&error);
	if (result == EPERM) {                                     
		result = EACCES;
	}
	                                                      
	if ((result != 0) && (error != 0)) {
		return error;
	}
	return result;
}

  
                               
  
                                                                         
                                                                  
  
                                                                              
                                                                           
                                    
   
static int
vnode_immutable(struct vnode_attr *vap, int append, int ignore)
{
	int     mask;

	                                                  
	mask = IMMUTABLE | APPEND;

	                                                    
	if (append) {
		mask &= ~APPEND;
	}

	                                                    
	if (ignore) {
		if (securelevel <= 0) {
			                                                     
			mask = 0;
		} else {
			                                               
			mask &= ~(UF_IMMUTABLE | UF_APPEND);
		}
	}
	KAUTH_DEBUG("IMMUTABLE - file flags 0x%x mask 0x%x append = %d ignore = %d", vap->va_flags, mask, append, ignore);
	if ((vap->va_flags & mask) != 0) {
		return EPERM;
	}
	return 0;
}

static int
vauth_node_owner(struct vnode_attr *vap, kauth_cred_t cred)
{
	int result;

	                                     
	result = 0;

	  
                                                                         
    
	if (vap && VATTR_IS_SUPPORTED(vap, va_uid)) {
		result = (vap->va_uid == kauth_cred_getuid(cred)) ? 1 : 0;
	}
	                                                                 

	return result;
}

  
                   
  
                                                                              
  
                                    
                                             
                              
                                                
                                                    
  
                       
                                    
                                                       
                                                       
   
static int
vauth_node_group(struct vnode_attr *vap, kauth_cred_t cred, int *ismember, int idontknow)
{
	int     error;
	int     result;

	error = 0;
	result = 0;

	  
                                                                   
                                                                 
                                                                
                                         
    
	if (vap && VATTR_IS_SUPPORTED(vap, va_gid)) {
		error = kauth_cred_ismember_gid(cred, vap->va_gid, &result);
		  
                                                               
                                                            
                                                            
                                                         
    
                                                            
                                                            
                                                              
                                                                
                                                            
                                                            
                              
     
		if (error == ENOENT) {
			error = idontknow;
		}
	}
	  
                                                                    
                                                                     
                                                                    
                                                                 
                                                                    
                                                                       
                                                                 
                                                                   
                                                                  
                            
    

	if (!error) {
		*ismember = result;
	}
	return error;
}

static int
vauth_file_owner(vauth_ctx vcp)
{
	int result;

	if (vcp->flags_valid & _VAC_IS_OWNER) {
		result = (vcp->flags & _VAC_IS_OWNER) ? 1 : 0;
	} else {
		result = vauth_node_owner(vcp->vap, vcp->ctx->vc_ucred);

		                      
		vcp->flags_valid |= _VAC_IS_OWNER;
		if (result) {
			vcp->flags |= _VAC_IS_OWNER;
		} else {
			vcp->flags &= ~_VAC_IS_OWNER;
		}
	}
	return result;
}


  
                     
  
                                                                           
  
                                                         
                                           
                                  
                              
                                     
                                      
                                                
                                                    
  
                       
                                                    
  
                                                               
                                   
   
static int
vauth_file_ingroup(vauth_ctx vcp, int *ismember, int idontknow)
{
	int     error;

	                                                                     
	if (vcp->flags_valid & _VAC_IN_GROUP) {
		*ismember = (vcp->flags & _VAC_IN_GROUP) ? 1 : 0;
		error = 0;
	} else {
		                               
		error = vauth_node_group(vcp->vap, vcp->ctx->vc_ucred, ismember, idontknow);

		if (!error) {
			                      
			vcp->flags_valid |= _VAC_IN_GROUP;
			if (*ismember) {
				vcp->flags |= _VAC_IN_GROUP;
			} else {
				vcp->flags &= ~_VAC_IN_GROUP;
			}
		}
	}
	return error;
}

static int
vauth_dir_owner(vauth_ctx vcp)
{
	int result;

	if (vcp->flags_valid & _VAC_IS_DIR_OWNER) {
		result = (vcp->flags & _VAC_IS_DIR_OWNER) ? 1 : 0;
	} else {
		result = vauth_node_owner(vcp->dvap, vcp->ctx->vc_ucred);

		                      
		vcp->flags_valid |= _VAC_IS_DIR_OWNER;
		if (result) {
			vcp->flags |= _VAC_IS_DIR_OWNER;
		} else {
			vcp->flags &= ~_VAC_IS_DIR_OWNER;
		}
	}
	return result;
}

  
                    
  
                                                                           
  
                                                         
                                           
                                  
                              
                                     
                                      
                                                
                                                    
  
                       
                                                    
  
                                                               
                                   
   
static int
vauth_dir_ingroup(vauth_ctx vcp, int *ismember, int idontknow)
{
	int     error;

	                                                                     
	if (vcp->flags_valid & _VAC_IN_DIR_GROUP) {
		*ismember = (vcp->flags & _VAC_IN_DIR_GROUP) ? 1 : 0;
		error = 0;
	} else {
		                               
		error = vauth_node_group(vcp->dvap, vcp->ctx->vc_ucred, ismember, idontknow);

		if (!error) {
			                      
			vcp->flags_valid |= _VAC_IN_DIR_GROUP;
			if (*ismember) {
				vcp->flags |= _VAC_IN_DIR_GROUP;
			} else {
				vcp->flags &= ~_VAC_IN_DIR_GROUP;
			}
		}
	}
	return error;
}

  
                                                                        
                       
   
static int
vnode_authorize_posix(vauth_ctx vcp, int action, int on_dir)
{
	struct vnode_attr *vap;
	int needed, error, owner_ok, group_ok, world_ok, ismember;
#ifdef KAUTH_DEBUG_ENABLE
	const char *where = "uninitialized";
# define _SETWHERE(c)   where = c;
#else
# define _SETWHERE(c)
#endif

	                                 
	if (on_dir) {
		vap = vcp->dvap;
	} else {
		vap = vcp->vap;
	}

	error = 0;

	  
                                                                     
                                                                             
                                                                           
    

	                       
	needed = 0;
	if (action & VREAD) {
		needed |= S_IRUSR;
	}
	if (action & VWRITE) {
		needed |= S_IWUSR;
	}
	if (action & VEXEC) {
		needed |= S_IXUSR;
	}
	owner_ok = (needed & vap->va_mode) == needed;

	  
                                                                       
                                                       
    
	if (!owner_ok && proc_ignores_node_permissions(vfs_context_proc(vcp->ctx))) {
		owner_ok = 1;
	}

	                       
	needed = 0;
	if (action & VREAD) {
		needed |= S_IRGRP;
	}
	if (action & VWRITE) {
		needed |= S_IWGRP;
	}
	if (action & VEXEC) {
		needed |= S_IXGRP;
	}
	group_ok = (needed & vap->va_mode) == needed;

	                       
	needed = 0;
	if (action & VREAD) {
		needed |= S_IROTH;
	}
	if (action & VWRITE) {
		needed |= S_IWOTH;
	}
	if (action & VEXEC) {
		needed |= S_IXOTH;
	}
	world_ok = (needed & vap->va_mode) == needed;

	                                                
	if (owner_ok && group_ok && world_ok) {
		_SETWHERE("all");
		goto out;
	}

	if (!owner_ok && !group_ok && !world_ok) {
		_SETWHERE("all");
		error = EACCES;
		goto out;
	}

	                                        
	if ((on_dir && vauth_dir_owner(vcp)) ||
	    (!on_dir && vauth_file_owner(vcp))) {
		_SETWHERE("user");
		if (!owner_ok) {
			error = EACCES;
		}
		goto out;
	}

	                                                            
	if (group_ok && world_ok) {
		_SETWHERE("group/world");
		goto out;
	}
	if (!group_ok && !world_ok) {
		_SETWHERE("group/world");
		error = EACCES;
		goto out;
	}

	                                             
	ismember = 0;                                                           

	  
                                                                      
                                                                   
                                                                       
                                                                     
                                                      
    
	if (on_dir) {
		error = vauth_dir_ingroup(vcp, &ismember, (!group_ok ? EACCES : 0));
	} else {
		error = vauth_file_ingroup(vcp, &ismember, (!group_ok ? EACCES : 0));
	}
	if (error) {
		if (!group_ok) {
			ismember = 1;
		}
		error = 0;
	}
	if (ismember) {
		_SETWHERE("group");
		if (!group_ok) {
			error = EACCES;
		}
		goto out;
	}

	                                               
	_SETWHERE("world");
	if (!world_ok) {
		error = EACCES;
	}

	                 

out:
	KAUTH_DEBUG("%p    %s - posix %s permissions : need %s%s%s %x have %s%s%s%s%s%s%s%s%s UID = %d file = %d,%d",
	    vcp->vp, (error == 0) ? "ALLOWED" : "DENIED", where,
	    (action & VREAD)  ? "r" : "-",
	    (action & VWRITE) ? "w" : "-",
	    (action & VEXEC)  ? "x" : "-",
	    needed,
	    (vap->va_mode & S_IRUSR) ? "r" : "-",
	    (vap->va_mode & S_IWUSR) ? "w" : "-",
	    (vap->va_mode & S_IXUSR) ? "x" : "-",
	    (vap->va_mode & S_IRGRP) ? "r" : "-",
	    (vap->va_mode & S_IWGRP) ? "w" : "-",
	    (vap->va_mode & S_IXGRP) ? "x" : "-",
	    (vap->va_mode & S_IROTH) ? "r" : "-",
	    (vap->va_mode & S_IWOTH) ? "w" : "-",
	    (vap->va_mode & S_IXOTH) ? "x" : "-",
	    kauth_cred_getuid(vcp->ctx->vc_ucred),
	    on_dir ? vcp->dvap->va_uid : vcp->vap->va_uid,
	    on_dir ? vcp->dvap->va_gid : vcp->vap->va_gid);
	return error;
}

  
                                                                
  
                  
                                                      
                                   
  
                                                                               
  
                                                      
                                                                 
  
                                                                           
               
  
                                  
                                                                            
                                                                              
                                                                               
             
  
                                        
  
                                                                        
                                                                         
                                                                                
                                                                             
                                                                           
                                                                          
                                                                            
                  
   
static int
vnode_authorize_delete(vauth_ctx vcp, boolean_t cached_delete_child)
{
	struct vnode_attr       *vap = vcp->vap;
	struct vnode_attr       *dvap = vcp->dvap;
	kauth_cred_t            cred = vcp->ctx->vc_ucred;
	struct kauth_acl_eval   eval;
	int                     error, ismember;

	                                     
	if (VATTR_IS_NOT(vap, va_acl, NULL)) {
		eval.ae_requested = KAUTH_VNODE_DELETE;
		eval.ae_acl = &vap->va_acl->acl_ace[0];
		eval.ae_count = vap->va_acl->acl_entrycount;
		eval.ae_options = 0;
		if (vauth_file_owner(vcp)) {
			eval.ae_options |= KAUTH_AEVAL_IS_OWNER;
		}
		  
                                                           
                                                            
                                                          
                                             
     
		if ((error = vauth_file_ingroup(vcp, &ismember, ENOENT)) != 0 && error != ENOENT) {
			return error;
		}
		if (error == ENOENT) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP_UNKNOWN;
		} else if (ismember) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP;
		}
		eval.ae_exp_gall = KAUTH_VNODE_GENERIC_ALL_BITS;
		eval.ae_exp_gread = KAUTH_VNODE_GENERIC_READ_BITS;
		eval.ae_exp_gwrite = KAUTH_VNODE_GENERIC_WRITE_BITS;
		eval.ae_exp_gexec = KAUTH_VNODE_GENERIC_EXECUTE_BITS;

		if ((error = kauth_acl_evaluate(cred, &eval)) != 0) {
			KAUTH_DEBUG("%p    ERROR during ACL processing - %d", vcp->vp, error);
			return error;
		}

		switch (eval.ae_result) {
		case KAUTH_RESULT_DENY:
			if (vauth_file_owner(vcp) && proc_ignores_node_permissions(vfs_context_proc(vcp->ctx))) {
				KAUTH_DEBUG("%p    Override DENY due to entitlement", vcp->vp);
				return 0;
			}
			KAUTH_DEBUG("%p    DENIED - denied by ACL", vcp->vp);
			return EACCES;
		case KAUTH_RESULT_ALLOW:
			KAUTH_DEBUG("%p    ALLOWED - granted by ACL", vcp->vp);
			return 0;
		case KAUTH_RESULT_DEFER:
		default:
			                        
			KAUTH_DEBUG("%p    DEFERRED - by file ACL", vcp->vp);
			break;
		}
	}

	  
                                                                 
                                        
   
                                                                       
                                                                        
                                                                     
                                                                      
                                                                  
    
	if (!(dvap->va_mode & S_ISTXT) && cached_delete_child) {
		KAUTH_DEBUG("%p    ALLOWED - granted by directory ACL or POSIX permissions and no sticky bit on directory", vcp->vp);
		return 0;
	}

	                                    
	if (VATTR_IS_NOT(dvap, va_acl, NULL)) {
		eval.ae_requested = KAUTH_VNODE_DELETE_CHILD;
		eval.ae_acl = &dvap->va_acl->acl_ace[0];
		eval.ae_count = dvap->va_acl->acl_entrycount;
		eval.ae_options = 0;
		if (vauth_dir_owner(vcp)) {
			eval.ae_options |= KAUTH_AEVAL_IS_OWNER;
		}
		  
                                                           
                                                            
                                                          
                                             
     
		if ((error = vauth_dir_ingroup(vcp, &ismember, ENOENT)) != 0 && error != ENOENT) {
			return error;
		}
		if (error == ENOENT) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP_UNKNOWN;
		} else if (ismember) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP;
		}
		eval.ae_exp_gall = KAUTH_VNODE_GENERIC_ALL_BITS;
		eval.ae_exp_gread = KAUTH_VNODE_GENERIC_READ_BITS;
		eval.ae_exp_gwrite = KAUTH_VNODE_GENERIC_WRITE_BITS;
		eval.ae_exp_gexec = KAUTH_VNODE_GENERIC_EXECUTE_BITS;

		  
                                                         
                              
     
		error = kauth_acl_evaluate(cred, &eval);

		if (error != 0) {
			KAUTH_DEBUG("%p    ERROR during ACL processing - %d", vcp->vp, error);
			return error;
		}
		switch (eval.ae_result) {
		case KAUTH_RESULT_DENY:
			if (vauth_dir_owner(vcp) && proc_ignores_node_permissions(vfs_context_proc(vcp->ctx))) {
				KAUTH_DEBUG("%p    Override DENY due to entitlement", vcp->vp);
				return 0;
			}
			KAUTH_DEBUG("%p    DENIED - denied by directory ACL", vcp->vp);
			return EACCES;
		case KAUTH_RESULT_ALLOW:
			KAUTH_DEBUG("%p    ALLOWED - granted by directory ACL", vcp->vp);
			if (!cached_delete_child && vcp->dvp) {
				vnode_cache_authorized_action(vcp->dvp,
				    vcp->ctx, KAUTH_VNODE_DELETE_CHILD);
			}
			return 0;
		case KAUTH_RESULT_DEFER:
		default:
			                               
			KAUTH_DEBUG("%p    DEFERRED - directory ACL", vcp->vp);
			break;
		}
	}

	  
                                                                      
                                                                    
    
	if (!cached_delete_child) {
		if (vnode_authorize_posix(vcp, VWRITE, 1             ) != 0) {
			KAUTH_DEBUG("%p    DENIED - denied by posix permisssions", vcp->vp);
			return EACCES;
		}
		  
                                                               
                                                               
                                                                 
     
		if (vcp->dvp) {
			vnode_cache_authorized_action(vcp->dvp, vcp->ctx,
			    KAUTH_VNODE_DELETE_CHILD);
		}
	}

	                                  
	if ((dvap->va_mode & S_ISTXT) && !vauth_file_owner(vcp) && !vauth_dir_owner(vcp)) {
		KAUTH_DEBUG("%p    DENIED - sticky bit rules (user %d  file %d  dir %d)",
		    vcp->vp, cred->cr_posix.cr_uid, vap->va_uid, dvap->va_uid);
		return EACCES;
	}

	                            
	return 0;
}


  
                                                         
   
static int
vnode_authorize_simple(vauth_ctx vcp, kauth_ace_rights_t acl_rights, kauth_ace_rights_t preauth_rights, boolean_t *found_deny)
{
	struct vnode_attr       *vap = vcp->vap;
	kauth_cred_t            cred = vcp->ctx->vc_ucred;
	struct kauth_acl_eval   eval;
	int                     error, ismember;
	mode_t                  posix_action;

	  
                                                                
   
                                                         
    
	if (vauth_file_owner(vcp)) {
		acl_rights &= ~(KAUTH_VNODE_WRITE_SECURITY);
	}

	  
                                                                     
                                                                           
                                                                    
                                                                   
                                                                    
              
    
	if ((acl_rights & KAUTH_VNODE_TAKE_OWNERSHIP) &&
	    (acl_rights & KAUTH_VNODE_WRITE_SECURITY)) {
		acl_rights &= ~KAUTH_VNODE_WRITE_SECURITY;
	}

	if (acl_rights == 0) {
		KAUTH_DEBUG("%p    ALLOWED - implicit or no rights required", vcp->vp);
		return 0;
	}

	                                    
	if (VATTR_IS_NOT(vap, va_acl, NULL)) {
		eval.ae_requested = acl_rights;
		eval.ae_acl = &vap->va_acl->acl_ace[0];
		eval.ae_count = vap->va_acl->acl_entrycount;
		eval.ae_options = 0;
		if (vauth_file_owner(vcp)) {
			eval.ae_options |= KAUTH_AEVAL_IS_OWNER;
		}
		  
                                                           
                                                            
                                                          
                                             
     
		if ((error = vauth_file_ingroup(vcp, &ismember, ENOENT)) != 0 && error != ENOENT) {
			return error;
		}
		if (error == ENOENT) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP_UNKNOWN;
		} else if (ismember) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP;
		}
		eval.ae_exp_gall = KAUTH_VNODE_GENERIC_ALL_BITS;
		eval.ae_exp_gread = KAUTH_VNODE_GENERIC_READ_BITS;
		eval.ae_exp_gwrite = KAUTH_VNODE_GENERIC_WRITE_BITS;
		eval.ae_exp_gexec = KAUTH_VNODE_GENERIC_EXECUTE_BITS;

		if ((error = kauth_acl_evaluate(cred, &eval)) != 0) {
			KAUTH_DEBUG("%p    ERROR during ACL processing - %d", vcp->vp, error);
			return error;
		}

		switch (eval.ae_result) {
		case KAUTH_RESULT_DENY:
			if (vauth_file_owner(vcp) && proc_ignores_node_permissions(vfs_context_proc(vcp->ctx))) {
				KAUTH_DEBUG("%p    Override DENY due to entitlement", vcp->vp);
				return 0;
			}
			KAUTH_DEBUG("%p    DENIED - by ACL", vcp->vp);
			return EACCES;                                         
		case KAUTH_RESULT_ALLOW:
			KAUTH_DEBUG("%p    ALLOWED - all rights granted by ACL", vcp->vp);
			return 0;
		case KAUTH_RESULT_DEFER:
		default:
			                                                  
			KAUTH_DEBUG("%p    DEFERRED - directory ACL", vcp->vp);
			break;
		}

		*found_deny = eval.ae_found_deny;

		                                               
	} else {
		                                    
		eval.ae_residual = acl_rights;
	}

	  
                                                        
    
	eval.ae_residual &= ~preauth_rights;

	  
                                                                    
    
	if (vauth_file_owner(vcp)) {
		eval.ae_residual &= ~KAUTH_VNODE_WRITE_ATTRIBUTES;
	}

	if (eval.ae_residual == 0) {
		KAUTH_DEBUG("%p    ALLOWED - rights already authorized", vcp->vp);
		return 0;
	}

	  
                                                                               
                                             
   
                                              
    
	if (eval.ae_residual & KAUTH_VNODE_CHANGE_OWNER) {
		KAUTH_DEBUG("%p    DENIED - CHANGE_OWNER not permitted", vcp->vp);
		return EACCES;
	}
	if (eval.ae_residual & KAUTH_VNODE_WRITE_SECURITY) {
		KAUTH_DEBUG("%p    DENIED - WRITE_SECURITY not permitted", vcp->vp);
		return EACCES;
	}

#if DIAGNOSTIC
	if (eval.ae_residual & KAUTH_VNODE_DELETE) {
		panic("vnode_authorize: can't be checking delete permission here");
	}
#endif

	  
                                                                          
           
    
	posix_action = 0;
	if (eval.ae_residual & (KAUTH_VNODE_READ_DATA |
	    KAUTH_VNODE_LIST_DIRECTORY |
	    KAUTH_VNODE_READ_EXTATTRIBUTES)) {
		posix_action |= VREAD;
	}
	if (eval.ae_residual & (KAUTH_VNODE_WRITE_DATA |
	    KAUTH_VNODE_ADD_FILE |
	    KAUTH_VNODE_ADD_SUBDIRECTORY |
	    KAUTH_VNODE_DELETE_CHILD |
	    KAUTH_VNODE_WRITE_ATTRIBUTES |
	    KAUTH_VNODE_WRITE_EXTATTRIBUTES)) {
		posix_action |= VWRITE;
	}
	if (eval.ae_residual & (KAUTH_VNODE_EXECUTE |
	    KAUTH_VNODE_SEARCH)) {
		posix_action |= VEXEC;
	}

	if (posix_action != 0) {
		return vnode_authorize_posix(vcp, posix_action, 0              );
	} else {
		KAUTH_DEBUG("%p    ALLOWED - residual rights %s%s%s%s%s%s%s%s%s%s%s%s%s%s granted due to no posix mapping",
		    vcp->vp,
		    (eval.ae_residual & KAUTH_VNODE_READ_DATA)
		    ? vnode_isdir(vcp->vp) ? " LIST_DIRECTORY" : " READ_DATA" : "",
		    (eval.ae_residual & KAUTH_VNODE_WRITE_DATA)
		    ? vnode_isdir(vcp->vp) ? " ADD_FILE" : " WRITE_DATA" : "",
		    (eval.ae_residual & KAUTH_VNODE_EXECUTE)
		    ? vnode_isdir(vcp->vp) ? " SEARCH" : " EXECUTE" : "",
		    (eval.ae_residual & KAUTH_VNODE_DELETE)
		    ? " DELETE" : "",
		    (eval.ae_residual & KAUTH_VNODE_APPEND_DATA)
		    ? vnode_isdir(vcp->vp) ? " ADD_SUBDIRECTORY" : " APPEND_DATA" : "",
		    (eval.ae_residual & KAUTH_VNODE_DELETE_CHILD)
		    ? " DELETE_CHILD" : "",
		    (eval.ae_residual & KAUTH_VNODE_READ_ATTRIBUTES)
		    ? " READ_ATTRIBUTES" : "",
		    (eval.ae_residual & KAUTH_VNODE_WRITE_ATTRIBUTES)
		    ? " WRITE_ATTRIBUTES" : "",
		    (eval.ae_residual & KAUTH_VNODE_READ_EXTATTRIBUTES)
		    ? " READ_EXTATTRIBUTES" : "",
		    (eval.ae_residual & KAUTH_VNODE_WRITE_EXTATTRIBUTES)
		    ? " WRITE_EXTATTRIBUTES" : "",
		    (eval.ae_residual & KAUTH_VNODE_READ_SECURITY)
		    ? " READ_SECURITY" : "",
		    (eval.ae_residual & KAUTH_VNODE_WRITE_SECURITY)
		    ? " WRITE_SECURITY" : "",
		    (eval.ae_residual & KAUTH_VNODE_CHECKIMMUTABLE)
		    ? " CHECKIMMUTABLE" : "",
		    (eval.ae_residual & KAUTH_VNODE_CHANGE_OWNER)
		    ? " CHANGE_OWNER" : "");
	}

	  
                                                                        
    
	return 0;
}

  
                               
   
static int
vnode_authorize_checkimmutable(mount_t mp, vauth_ctx vcp,
    struct vnode_attr *vap, int rights, int ignore)
{
	int error;
	int append;

	  
                                                                
   
                                                        
    
	switch (vap->va_type) {
	case VSOCK:
	case VFIFO:
	case VBLK:
	case VCHR:
		  
                                                                
                                     
     
		rights &= ~KAUTH_VNODE_WRITE_DATA;
		break;
	default:
		break;
	}

	error = 0;
	if (rights & KAUTH_VNODE_WRITE_RIGHTS) {
		                                              
		if (mp != NULL) {
			                                 
			if ((rights & KAUTH_VNODE_WRITE_EXTATTRIBUTES) &&
			    (vfs_flags(mp) & MNT_NOUSERXATTR)) {
				KAUTH_DEBUG("%p    DENIED - filesystem disallowed extended attributes", vap);
				error = EACCES;                                
				goto out;
			}
		}

		  
                                                                          
                                    
     
		append = 0;
		if (vap->va_type == VDIR) {
			if ((rights & (KAUTH_VNODE_ADD_FILE | KAUTH_VNODE_ADD_SUBDIRECTORY | KAUTH_VNODE_WRITE_EXTATTRIBUTES)) == rights) {
				append = 1;
			}
		} else {
			if ((rights & (KAUTH_VNODE_APPEND_DATA | KAUTH_VNODE_WRITE_EXTATTRIBUTES)) == rights) {
				append = 1;
			}
		}
		if ((error = vnode_immutable(vap, append, ignore)) != 0) {
			if (error && !ignore) {
				  
                                                                       
       
				int owner = 0;
				if (rights & KAUTH_VNODE_DELETE_CHILD && vcp->dvp != NULL) {
					owner = vauth_file_owner(vcp) && vauth_dir_owner(vcp);
				} else {
					owner = vauth_file_owner(vcp);
				}
				if (owner && proc_ignores_node_permissions(vfs_context_proc(vcp->ctx))) {
					error = vnode_immutable(vap, append, 1);
				}
			}
		}
		if (error) {
			KAUTH_DEBUG("%p    DENIED - file is immutable", vap);
			goto out;
		}
	}
out:
	return error;
}

  
                                                                       
                            
  
                                                       
                                           
  
                                                                     
                          
  
                                                                      
                         
                   
                 
   
static int
vnode_authorize_opaque(vnode_t vp, int *resultp, kauth_action_t action, vfs_context_t ctx)
{
	int     error;

	  
                                                                             
                                              
    
	switch (vp->v_type) {
	case VBLK:
	case VCHR:
	case VSOCK:
	case VFIFO:
		return 0;
	default:
		break;
	}

	  
                                                                               
                                                                               
    
	if ((action & KAUTH_VNODE_ACCESS) && !vfs_authopaqueaccess(vp->v_mount)) {
		return 0;
	}

	  
                                                                                  
                                                                       
    
	if ((error = VNOP_ACCESS(vp, action, ctx)) != ENOTSUP) {
		*resultp = error;
		KAUTH_DEBUG("%p    DENIED - opaque filesystem VNOP_ACCESS denied access", vp);
		return 1;
	}

	  
                                                                                          
                                                                                      
    
	if ((action & KAUTH_VNODE_EXECUTE) && (vp->v_type == VREG)) {
		                                         
		if ((error = VNOP_OPEN(vp, FREAD, ctx)) != 0) {
			*resultp = error;
			KAUTH_DEBUG("%p    DENIED - EXECUTE denied because file could not be opened readonly", vp);
			return 1;
		}
		VNOP_CLOSE(vp, FREAD, ctx);
	}

	  
                                                                                        
                             
    
	*resultp = 0;
	KAUTH_DEBUG("%p    ALLOWED - bypassing access check for non-local filesystem", vp);
	return 1;
}




  
                              
                     
  
                                                        
                                 
                              
                                                
                                                               
                    
                                   
                                     
                             
                             
   


static int
vnode_authorize_callback(__unused kauth_cred_t cred, __unused void *idata,
    kauth_action_t action, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3)
{
	vfs_context_t   ctx;
	vnode_t         cvp = NULLVP;
	vnode_t         vp, dvp;
	int             result = KAUTH_RESULT_DENY;
	int             parent_iocount = 0;
	int             parent_action;                                                                      

	ctx = (vfs_context_t)arg0;
	vp = (vnode_t)arg1;
	dvp = (vnode_t)arg2;

	  
                                                     
                                                   
                                                           
                                                 
                                                            
                                                         
                                                            
                                                                
                                                  
                                                             
    
	if (dvp && vp) {
		goto defer;
	}
	if (dvp) {
		cvp = dvp;
	} else {
		  
                                                                                       
                                                                                         
                                                                                 
                                                                                     
                                                                                      
                                                                                            
                                     
    
                                                                                 
                                                                                          
                                                                                           
                                                                                            
                                                                                            
                                         
     
		if (vnode_isnamedstream(vp) && (!vfs_authopaque(vp->v_mount))) {
			cvp = vnode_getparent(vp);
			if (cvp != NULLVP) {
				parent_iocount = 1;
			} else {
				cvp = NULL;
				goto defer;                                                     
			}

			                                    
			parent_action = action;
			if (parent_action & KAUTH_VNODE_READ_DATA) {
				parent_action &= ~KAUTH_VNODE_READ_DATA;
				parent_action |= KAUTH_VNODE_READ_EXTATTRIBUTES;
			}
			if (parent_action & KAUTH_VNODE_WRITE_DATA) {
				parent_action &= ~KAUTH_VNODE_WRITE_DATA;
				parent_action |= KAUTH_VNODE_WRITE_EXTATTRIBUTES;
			}
		} else {
			cvp = vp;
		}
	}

	if (vnode_cache_is_authorized(cvp, ctx, parent_iocount ? parent_action : action) == TRUE) {
		result = KAUTH_RESULT_ALLOW;
		goto out;
	}
defer:
	result = vnode_authorize_callback_int(action, ctx, vp, dvp, (int *)arg3);

	if (result == KAUTH_RESULT_ALLOW && cvp != NULLVP) {
		KAUTH_DEBUG("%p - caching action = %x", cvp, action);
		vnode_cache_authorized_action(cvp, ctx, action);
	}

out:
	if (parent_iocount) {
		vnode_put(cvp);
	}

	return result;
}

static int
vnode_attr_authorize_internal(vauth_ctx vcp, mount_t mp,
    kauth_ace_rights_t rights, int is_suser, boolean_t *found_deny,
    int noimmutable, int parent_authorized_for_delete_child)
{
	int result;

	  
                           
   
                                                                       
                
    
	if ((result = vnode_authorize_checkimmutable(mp, vcp, vcp->vap, rights,
	    noimmutable)) != 0) {
		goto out;
	}

	if ((rights & KAUTH_VNODE_DELETE) &&
	    !parent_authorized_for_delete_child) {
		result = vnode_authorize_checkimmutable(mp, vcp, vcp->dvap,
		    KAUTH_VNODE_DELETE_CHILD, 0);
		if (result) {
			goto out;
		}
	}

	  
                                                                                          
          
    
	rights &= ~(KAUTH_VNODE_LINKTARGET | KAUTH_VNODE_CHECKIMMUTABLE);
	if (rights == 0) {
		goto out;
	}

	  
                                                                   
                                                                    
                                     
    
	if (!is_suser) {
		                           
		if ((rights & KAUTH_VNODE_DELETE) &&
		    ((result = vnode_authorize_delete(vcp, parent_authorized_for_delete_child)) != 0)) {
			goto out;
		}

		                              
		if ((rights & ~KAUTH_VNODE_DELETE) &&
		    (result = vnode_authorize_simple(vcp, rights, rights & KAUTH_VNODE_DELETE, found_deny)) != 0) {
			goto out;
		}
	} else {
		  
                                                                                  
                                                               
     
		if ((rights & KAUTH_VNODE_EXECUTE) &&
		    (vcp->vap->va_type == VREG) &&
		    VATTR_IS_SUPPORTED(vcp->vap, va_mode) &&
		    !(vcp->vap->va_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
			result = EPERM;
			KAUTH_DEBUG("%p    DENIED - root execute requires at least one x bit in 0x%x", vcp, vcp->vap->va_mode);
			goto out;
		}

		                                                                                       
		*found_deny = TRUE;

		KAUTH_DEBUG("%p    ALLOWED - caller is superuser", vcp);
	}
out:
	return result;
}

static int
vnode_authorize_callback_int(kauth_action_t action, vfs_context_t ctx,
    vnode_t vp, vnode_t dvp, int *errorp)
{
	struct _vnode_authorize_context auth_context;
	vauth_ctx               vcp;
	kauth_cred_t            cred;
	kauth_ace_rights_t      rights;
	struct vnode_attr       va, dva;
	int                     result;
	int                     noimmutable;
	boolean_t               parent_authorized_for_delete_child = FALSE;
	boolean_t               found_deny = FALSE;
	boolean_t               parent_ref = FALSE;
	boolean_t               is_suser = FALSE;

	vcp = &auth_context;
	vcp->ctx = ctx;
	vcp->vp = vp;
	vcp->dvp = dvp;
	  
                                                                   
                           
    
	cred = ctx->vc_ucred;

	VATTR_INIT(&va);
	vcp->vap = &va;
	VATTR_INIT(&dva);
	vcp->dvap = &dva;

	vcp->flags = vcp->flags_valid = 0;

#if DIAGNOSTIC
	if ((ctx == NULL) || (vp == NULL) || (cred == NULL)) {
		panic("vnode_authorize: bad arguments (context %p  vp %p  cred %p)", ctx, vp, cred);
	}
#endif

	KAUTH_DEBUG("%p  AUTH - %s %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s on %s '%s' (0x%x:%p/%p)",
	    vp, vfs_context_proc(ctx)->p_comm,
	    (action & KAUTH_VNODE_ACCESS)               ? "access" : "auth",
	    (action & KAUTH_VNODE_READ_DATA)            ? vnode_isdir(vp) ? " LIST_DIRECTORY" : " READ_DATA" : "",
	    (action & KAUTH_VNODE_WRITE_DATA)           ? vnode_isdir(vp) ? " ADD_FILE" : " WRITE_DATA" : "",
	    (action & KAUTH_VNODE_EXECUTE)              ? vnode_isdir(vp) ? " SEARCH" : " EXECUTE" : "",
	    (action & KAUTH_VNODE_DELETE)               ? " DELETE" : "",
	    (action & KAUTH_VNODE_APPEND_DATA)          ? vnode_isdir(vp) ? " ADD_SUBDIRECTORY" : " APPEND_DATA" : "",
	    (action & KAUTH_VNODE_DELETE_CHILD)         ? " DELETE_CHILD" : "",
	    (action & KAUTH_VNODE_READ_ATTRIBUTES)      ? " READ_ATTRIBUTES" : "",
	    (action & KAUTH_VNODE_WRITE_ATTRIBUTES)     ? " WRITE_ATTRIBUTES" : "",
	    (action & KAUTH_VNODE_READ_EXTATTRIBUTES)   ? " READ_EXTATTRIBUTES" : "",
	    (action & KAUTH_VNODE_WRITE_EXTATTRIBUTES)  ? " WRITE_EXTATTRIBUTES" : "",
	    (action & KAUTH_VNODE_READ_SECURITY)        ? " READ_SECURITY" : "",
	    (action & KAUTH_VNODE_WRITE_SECURITY)       ? " WRITE_SECURITY" : "",
	    (action & KAUTH_VNODE_CHANGE_OWNER)         ? " CHANGE_OWNER" : "",
	    (action & KAUTH_VNODE_NOIMMUTABLE)          ? " (noimmutable)" : "",
	    vnode_isdir(vp) ? "directory" : "file",
	    vp->v_name ? vp->v_name : "<NULL>", action, vp, dvp);

	  
                                                                
                     
    
	noimmutable = (action & KAUTH_VNODE_NOIMMUTABLE) ? 1 : 0;
	rights = action & ~(KAUTH_VNODE_ACCESS | KAUTH_VNODE_NOIMMUTABLE);

	if (rights & KAUTH_VNODE_DELETE) {
#if DIAGNOSTIC
		if (dvp == NULL) {
			panic("vnode_authorize: KAUTH_VNODE_DELETE test requires a directory");
		}
#endif
		  
                                                        
                                                        
                                                            
                                                      
     
		if (vnode_cache_is_authorized(dvp, ctx, KAUTH_VNODE_DELETE_CHILD) == TRUE) {
			parent_authorized_for_delete_child = TRUE;
		}
	} else {
		vcp->dvp = NULLVP;
		vcp->dvap = NULL;
	}

	  
                                    
    
	if ((rights & KAUTH_VNODE_WRITE_RIGHTS) &&
	    (vp->v_mount->mnt_flag & MNT_RDONLY) &&
	    ((vp->v_type == VREG) || (vp->v_type == VDIR) ||
	    (vp->v_type == VLNK) || (vp->v_type == VCPLX) ||
	    (rights & KAUTH_VNODE_DELETE) || (rights & KAUTH_VNODE_DELETE_CHILD))) {
		result = EROFS;
		goto out;
	}

	  
                                 
    
	if ((rights & KAUTH_VNODE_EXECUTE) && (vp->v_type == VREG) && (vp->v_mount->mnt_flag & MNT_NOEXEC)) {
		result = EACCES;
		goto out;
	}

	  
                                                                   
                                                                           
                                                                      
                                                                    
    
	if ((vp->v_mount->mnt_kern_flag & MNTK_AUTH_OPAQUE) && vnode_authorize_opaque(vp, &result, action, ctx)) {
		goto out;
	}

	  
                                                                      
                                                     
    
	if (vnode_isnamedstream(vp)) {
		if (rights & KAUTH_VNODE_READ_DATA) {
			rights &= ~KAUTH_VNODE_READ_DATA;
			rights |= KAUTH_VNODE_READ_EXTATTRIBUTES;
		}
		if (rights & KAUTH_VNODE_WRITE_DATA) {
			rights &= ~KAUTH_VNODE_WRITE_DATA;
			rights |= KAUTH_VNODE_WRITE_EXTATTRIBUTES;
		}

		  
                                                            
     
		if ((vp->v_parent != NULL) &&
		    (vget_internal(vp->v_parent, 0, VNODE_NODEAD | VNODE_DRAINO) == 0)) {
			parent_ref = TRUE;
			vcp->vp = vp = vp->v_parent;
		}
	}

	if (vfs_context_issuser(ctx)) {
		  
                                                                  
                                                
     
		if (!(rights & (KAUTH_VNODE_EXECUTE | KAUTH_VNODE_WRITE_RIGHTS))) {
			goto success;
		}

		is_suser = TRUE;
	}

	  
                                                                        
                              
   
                                                               
                             
    
	VATTR_WANTED(&va, va_mode);
	VATTR_WANTED(&va, va_flags);
	if (!is_suser) {
		VATTR_WANTED(&va, va_uid);
		VATTR_WANTED(&va, va_gid);
		VATTR_WANTED(&va, va_acl);
	}
	if ((result = vnode_getattr(vp, &va, ctx)) != 0) {
		KAUTH_DEBUG("%p    ERROR - failed to get vnode attributes - %d", vp, result);
		goto out;
	}
	VATTR_WANTED(&va, va_type);
	VATTR_RETURN(&va, va_type, vnode_vtype(vp));

	if (vcp->dvp) {
		VATTR_WANTED(&dva, va_mode);
		VATTR_WANTED(&dva, va_flags);
		if (!is_suser) {
			VATTR_WANTED(&dva, va_uid);
			VATTR_WANTED(&dva, va_gid);
			VATTR_WANTED(&dva, va_acl);
		}
		if ((result = vnode_getattr(vcp->dvp, &dva, ctx)) != 0) {
			KAUTH_DEBUG("%p    ERROR - failed to get directory vnode attributes - %d", vp, result);
			goto out;
		}
		VATTR_WANTED(&dva, va_type);
		VATTR_RETURN(&dva, va_type, vnode_vtype(vcp->dvp));
	}

	result = vnode_attr_authorize_internal(vcp, vp->v_mount, rights, is_suser,
	    &found_deny, noimmutable, parent_authorized_for_delete_child);
out:
	if (VATTR_IS_SUPPORTED(&va, va_acl) && (va.va_acl != NULL)) {
		kauth_acl_free(va.va_acl);
	}
	if (VATTR_IS_SUPPORTED(&dva, va_acl) && (dva.va_acl != NULL)) {
		kauth_acl_free(dva.va_acl);
	}

	if (result) {
		if (parent_ref) {
			vnode_put(vp);
		}
		*errorp = result;
		KAUTH_DEBUG("%p    DENIED - auth denied", vp);
		return KAUTH_RESULT_DENY;
	}
	if ((rights & KAUTH_VNODE_SEARCH) && found_deny == FALSE && vp->v_type == VDIR) {
		  
                                                                       
                                                                                
                                                                         
                                                                       
                                                               
    
                                                                
                                                               
                                                        
     
		if (!VATTR_IS_SUPPORTED(&va, va_mode) ||
		    ((va.va_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ==
		    (S_IXUSR | S_IXGRP | S_IXOTH))) {
			vnode_cache_authorized_action(vp, ctx, KAUTH_VNODE_SEARCHBYANYONE);
		}
	}
success:
	if (parent_ref) {
		vnode_put(vp);
	}

	  
                                                                                
                                                                        
    
	KAUTH_DEBUG("%p    ALLOWED - auth granted", vp);
	return KAUTH_RESULT_ALLOW;
}

int
vnode_attr_authorize_init(struct vnode_attr *vap, struct vnode_attr *dvap,
    kauth_action_t action, vfs_context_t ctx)
{
	VATTR_INIT(vap);
	VATTR_WANTED(vap, va_type);
	VATTR_WANTED(vap, va_mode);
	VATTR_WANTED(vap, va_flags);
	if (dvap) {
		VATTR_INIT(dvap);
		if (action & KAUTH_VNODE_DELETE) {
			VATTR_WANTED(dvap, va_type);
			VATTR_WANTED(dvap, va_mode);
			VATTR_WANTED(dvap, va_flags);
		}
	} else if (action & KAUTH_VNODE_DELETE) {
		return EINVAL;
	}

	if (!vfs_context_issuser(ctx)) {
		VATTR_WANTED(vap, va_uid);
		VATTR_WANTED(vap, va_gid);
		VATTR_WANTED(vap, va_acl);
		if (dvap && (action & KAUTH_VNODE_DELETE)) {
			VATTR_WANTED(dvap, va_uid);
			VATTR_WANTED(dvap, va_gid);
			VATTR_WANTED(dvap, va_acl);
		}
	}

	return 0;
}

int
vnode_attr_authorize(struct vnode_attr *vap, struct vnode_attr *dvap, mount_t mp,
    kauth_action_t action, vfs_context_t ctx)
{
	struct _vnode_authorize_context auth_context;
	vauth_ctx vcp;
	kauth_ace_rights_t rights;
	int noimmutable;
	boolean_t found_deny;
	boolean_t is_suser = FALSE;
	int result = 0;

	vcp = &auth_context;
	vcp->ctx = ctx;
	vcp->vp = NULLVP;
	vcp->vap = vap;
	vcp->dvp = NULLVP;
	vcp->dvap = dvap;
	vcp->flags = vcp->flags_valid = 0;

	noimmutable = (action & KAUTH_VNODE_NOIMMUTABLE) ? 1 : 0;
	rights = action & ~(KAUTH_VNODE_ACCESS | KAUTH_VNODE_NOIMMUTABLE);

	  
                                    
    
	if ((rights & KAUTH_VNODE_WRITE_RIGHTS) &&
	    mp && (mp->mnt_flag & MNT_RDONLY) &&
	    ((vap->va_type == VREG) || (vap->va_type == VDIR) ||
	    (vap->va_type == VLNK) || (rights & KAUTH_VNODE_DELETE) ||
	    (rights & KAUTH_VNODE_DELETE_CHILD))) {
		result = EROFS;
		goto out;
	}

	  
                                 
    
	if ((rights & KAUTH_VNODE_EXECUTE) &&
	    (vap->va_type == VREG) && mp && (mp->mnt_flag & MNT_NOEXEC)) {
		result = EACCES;
		goto out;
	}

	if (vfs_context_issuser(ctx)) {
		  
                                                                  
                                                
     
		if (!(rights & (KAUTH_VNODE_EXECUTE | KAUTH_VNODE_WRITE_RIGHTS))) {
			goto out;
		}
		is_suser = TRUE;
	} else {
		if (!VATTR_IS_SUPPORTED(vap, va_uid) ||
		    !VATTR_IS_SUPPORTED(vap, va_gid) ||
		    (mp && vfs_extendedsecurity(mp) && !VATTR_IS_SUPPORTED(vap, va_acl))) {
			panic("vnode attrs not complete for vnode_attr_authorize\n");
		}
	}

	if (mp) {
		vnode_attr_handle_mnt_ignore_ownership(vap, mp, ctx);
	}

	result = vnode_attr_authorize_internal(vcp, mp, rights, is_suser,
	    &found_deny, noimmutable, FALSE);

	if (result == EPERM) {
		result = EACCES;
	}
out:
	return result;
}


int
vnode_authattr_new(vnode_t dvp, struct vnode_attr *vap, int noauth, vfs_context_t ctx)
{
	return vnode_authattr_new_internal(dvp, vap, noauth, NULL, ctx);
}

  
                                                                          
                             
   
static int
vnode_authattr_new_internal(vnode_t dvp, struct vnode_attr *vap, int noauth, uint32_t *defaulted_fieldsp, vfs_context_t ctx)
{
	int             error;
	int             has_priv_suser, ismember, defaulted_owner, defaulted_group, defaulted_mode;
	uint32_t        inherit_flags;
	kauth_cred_t    cred;
	guid_t          changer;
	mount_t         dmp;
	struct vnode_attr dva;

	error = 0;

	if (defaulted_fieldsp) {
		*defaulted_fieldsp = 0;
	}

	defaulted_owner = defaulted_group = defaulted_mode = 0;

	inherit_flags = 0;

	  
                                                                       
    
	if (!vfs_extendedsecurity(dvp->v_mount) &&
	    (VATTR_IS_ACTIVE(vap, va_acl) || VATTR_IS_ACTIVE(vap, va_uuuid) || VATTR_IS_ACTIVE(vap, va_guuid))) {
		error = EINVAL;
		goto out;
	}

	  
                        
    
	dmp = dvp->v_mount;

	  
                                                                                    
                                           
    
	if ((dmp->mnt_flag & MNT_IGNORE_OWNERSHIP) && (dmp->mnt_fsowner != KAUTH_UID_NONE)) {
		VATTR_SET(vap, va_uid, dmp->mnt_fsowner);
		defaulted_owner = 1;
	} else {
		if (!VATTR_IS_ACTIVE(vap, va_uid)) {
			                                   
			VATTR_SET(vap, va_uid, kauth_cred_getuid(vfs_context_ucred(ctx)));
			defaulted_owner = 1;
		}
	}

	  
                                                                       
                         
    
	VATTR_INIT(&dva);
	VATTR_WANTED(&dva, va_gid);
	VATTR_WANTED(&dva, va_flags);
	if ((error = vnode_getattr(dvp, &dva, ctx)) != 0) {
		goto out;
	}

	  
                                                                                    
                                           
    
	if ((dmp->mnt_flag & MNT_IGNORE_OWNERSHIP) && (dmp->mnt_fsgroup != KAUTH_GID_NONE)) {
		VATTR_SET(vap, va_gid, dmp->mnt_fsgroup);
		defaulted_group = 1;
	} else {
		if (!VATTR_IS_ACTIVE(vap, va_gid)) {
			                                                                      
			if (VATTR_IS_SUPPORTED(&dva, va_gid)) {
				VATTR_SET(vap, va_gid, dva.va_gid);
			} else {
				VATTR_SET(vap, va_gid, kauth_cred_getgid(vfs_context_ucred(ctx)));
			}
			defaulted_group = 1;
		}
	}

	if (!VATTR_IS_ACTIVE(vap, va_flags)) {
		VATTR_SET(vap, va_flags, 0);
	}

	                                                                 
                 
	if (VATTR_IS_SUPPORTED(&dva, va_flags)) {
		inherit_flags = dva.va_flags & (UF_DATAVAULT | SF_RESTRICTED);
	}

	                                                           
	if (!VATTR_IS_ACTIVE(vap, va_mode)) {
		VATTR_SET(vap, va_mode, ACCESSPERMS & ~vfs_context_proc(ctx)->p_fd->fd_cmask);
		KAUTH_DEBUG("ATTR - defaulting new file mode to %o from umask %o", vap->va_mode, vfs_context_proc(ctx)->p_fd->fd_cmask);
		defaulted_mode = 1;
	}
	                           
	if (!VATTR_IS_ACTIVE(vap, va_create_time)) {
		nanotime(&vap->va_create_time);
		VATTR_SET_ACTIVE(vap, va_create_time);
	}

	  
                                                 
    
	if (vap->va_active & ~VNODE_ATTR_NEWOBJ) {
		error = EINVAL;
		KAUTH_DEBUG("ATTR - ERROR - attempt to set unsupported new-file attributes %llx",
		    vap->va_active & ~VNODE_ATTR_NEWOBJ);
		goto out;
	}

	  
                                                                
                                                                   
    
	if (vfs_authopaque(dvp->v_mount)) {
		goto out;
	}

	  
                                                                     
                                 
    
	cred = vfs_context_ucred(ctx);
	if (noauth) {
		                               
		has_priv_suser = 1;
	} else {
		has_priv_suser = vfs_context_issuser(ctx);
	}


	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		vap->va_flags &= ~SF_SYNTHETIC;
		if (has_priv_suser) {
			if ((vap->va_flags & (UF_SETTABLE | SF_SETTABLE)) != vap->va_flags) {
				error = EPERM;
				KAUTH_DEBUG("  DENIED - superuser attempt to set illegal flag(s)");
				goto out;
			}
		} else {
			if ((vap->va_flags & UF_SETTABLE) != vap->va_flags) {
				error = EPERM;
				KAUTH_DEBUG("  DENIED - user attempt to set illegal flag(s)");
				goto out;
			}
		}
	}

	                                                                
	if (!has_priv_suser) {
		if (!defaulted_mode && VATTR_IS_ACTIVE(vap, va_mode)) {
			             
			if (vap->va_mode & S_ISGID) {
				if ((error = kauth_cred_ismember_gid(cred, vap->va_gid, &ismember)) != 0) {
					KAUTH_DEBUG("ATTR - ERROR: got %d checking for membership in %d", error, vap->va_gid);
					goto out;
				}
				if (!ismember) {
					KAUTH_DEBUG("  DENIED - can't set SGID bit, not a member of %d", vap->va_gid);
					error = EPERM;
					goto out;
				}
			}

			             
			if ((vap->va_mode & S_ISUID) && (vap->va_uid != kauth_cred_getuid(cred))) {
				KAUTH_DEBUG("ATTR - ERROR: illegal attempt to set the setuid bit");
				error = EPERM;
				goto out;
			}
		}
		if (!defaulted_owner && (vap->va_uid != kauth_cred_getuid(cred))) {
			KAUTH_DEBUG("  DENIED - cannot create new item owned by %d", vap->va_uid);
			error = EPERM;
			goto out;
		}
		if (!defaulted_group) {
			if ((error = kauth_cred_ismember_gid(cred, vap->va_gid, &ismember)) != 0) {
				KAUTH_DEBUG("  ERROR - got %d checking for membership in %d", error, vap->va_gid);
				goto out;
			}
			if (!ismember) {
				KAUTH_DEBUG("  DENIED - cannot create new item with group %d - not a member", vap->va_gid);
				error = EPERM;
				goto out;
			}
		}

		                                   
		if (VATTR_IS_ACTIVE(vap, va_uuuid)) {
			if ((error = kauth_cred_getguid(cred, &changer)) != 0) {
				KAUTH_DEBUG("  ERROR - got %d trying to get caller UUID", error);
				                                                             
				goto out;
			}
			if (!kauth_guid_equal(&vap->va_uuuid, &changer)) {
				KAUTH_DEBUG("  ERROR - cannot create item with supplied owner UUID - not us");
				error = EPERM;
				goto out;
			}
		}
		if (VATTR_IS_ACTIVE(vap, va_guuid)) {
			if ((error = kauth_cred_ismember_guid(cred, &vap->va_guuid, &ismember)) != 0) {
				KAUTH_DEBUG("  ERROR - got %d trying to check group membership", error);
				goto out;
			}
			if (!ismember) {
				KAUTH_DEBUG("  ERROR - cannot create item with supplied group UUID - not a member");
				error = EPERM;
				goto out;
			}
		}
	}
out:
	if (inherit_flags) {
		                                                              
                                                             
                                                             
		VATTR_SET(vap, va_flags, (vap->va_flags | inherit_flags));
	}
	if (defaulted_fieldsp) {
		if (defaulted_mode) {
			*defaulted_fieldsp |= VATTR_PREPARE_DEFAULTED_MODE;
		}
		if (defaulted_group) {
			*defaulted_fieldsp |= VATTR_PREPARE_DEFAULTED_GID;
		}
		if (defaulted_owner) {
			*defaulted_fieldsp |= VATTR_PREPARE_DEFAULTED_UID;
		}
	}
	return error;
}

  
                                                                            
           
  
                                                                           
                                                               
  
                                                                             
                                                         
   
int
vnode_authattr(vnode_t vp, struct vnode_attr *vap, kauth_action_t *actionp, vfs_context_t ctx)
{
	struct vnode_attr ova;
	kauth_action_t  required_action;
	int             error, has_priv_suser, ismember, chowner, chgroup, clear_suid, clear_sgid;
	guid_t          changer;
	gid_t           group;
	uid_t           owner;
	mode_t          newmode;
	kauth_cred_t    cred;
	uint32_t        fdelta;

	VATTR_INIT(&ova);
	required_action = 0;
	error = 0;

	  
                                                
    
	if (vfs_authopaque(vp->v_mount)) {
		goto out;
	}

	  
                                                 
    
	if (vap->va_active & VNODE_ATTR_RDONLY) {
		KAUTH_DEBUG("ATTR - ERROR: attempt to set readonly attribute(s)");
		error = EINVAL;
		goto out;
	}

	  
                                                   
    
	cred = vfs_context_ucred(ctx);
	has_priv_suser = kauth_cred_issuser(cred);

	  
                                                                                
          
          
           
            
            
    
	if (VATTR_IS_ACTIVE(vap, va_uid) ||
	    VATTR_IS_ACTIVE(vap, va_gid) ||
	    VATTR_IS_ACTIVE(vap, va_mode) ||
	    VATTR_IS_ACTIVE(vap, va_uuuid) ||
	    VATTR_IS_ACTIVE(vap, va_guuid)) {
		VATTR_WANTED(&ova, va_mode);
		VATTR_WANTED(&ova, va_uid);
		VATTR_WANTED(&ova, va_gid);
		VATTR_WANTED(&ova, va_uuuid);
		VATTR_WANTED(&ova, va_guuid);
		KAUTH_DEBUG("ATTR - security information changing, fetching existing attributes");
	}

	  
                                                                          
       
    
	if (VATTR_IS_ACTIVE(vap, va_create_time) ||
	    VATTR_IS_ACTIVE(vap, va_change_time) ||
	    VATTR_IS_ACTIVE(vap, va_modify_time) ||
	    VATTR_IS_ACTIVE(vap, va_access_time) ||
	    VATTR_IS_ACTIVE(vap, va_backup_time) ||
	    VATTR_IS_ACTIVE(vap, va_addedtime)) {
		VATTR_WANTED(&ova, va_uid);
#if 0                                                             
		VATTR_WANTED(&ova, va_uuuid);
#endif
		KAUTH_DEBUG("ATTR - timestamps changing, fetching uid and GUID");
	}

	  
                                                      
    
	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		KAUTH_DEBUG("ATTR - flags changing, fetching old flags");
		VATTR_WANTED(&ova, va_flags);
	}

	  
                                                    
    
	if (VATTR_IS_ACTIVE(vap, va_acl)) {
		KAUTH_DEBUG("ATTR - acl changing, fetching old flags");
		VATTR_WANTED(&ova, va_acl);
	}

	  
                                                             
    
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		                                                                      
		if (!vnode_isreg(vp)) {
			KAUTH_DEBUG("ATTR - ERROR: size change requested on non-file");
			error = vnode_isdir(vp) ? EISDIR : EINVAL;
			goto out;
		}
	}

	  
                 
    
	KAUTH_DEBUG("ATTR - fetching old attributes %016llx", ova.va_active);
	if ((error = vnode_getattr(vp, &ova, ctx)) != 0) {
		KAUTH_DEBUG("  ERROR - got %d trying to get attributes", error);
		goto out;
	}

	  
                                                       
    
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		                                                                       
		KAUTH_DEBUG("ATTR - size change, requiring WRITE_DATA");
		required_action |= KAUTH_VNODE_WRITE_DATA;
	}

	  
                        
   
                                                                          
                                                                       
                                
   
                                                                  
                                                 
    
	if (VATTR_IS_ACTIVE(vap, va_create_time) ||
	    VATTR_IS_ACTIVE(vap, va_change_time) ||
	    VATTR_IS_ACTIVE(vap, va_modify_time) ||
	    VATTR_IS_ACTIVE(vap, va_access_time) ||
	    VATTR_IS_ACTIVE(vap, va_backup_time) ||
	    VATTR_IS_ACTIVE(vap, va_addedtime)) {
		  
                                                         
                                                                    
                                                                
     
		if (has_priv_suser || vauth_node_owner(&ova, cred)) {
			KAUTH_DEBUG("ATTR - root or owner changing timestamps");
			required_action |= KAUTH_VNODE_CHECKIMMUTABLE | KAUTH_VNODE_WRITE_ATTRIBUTES;
		} else {
			                                    
			if (vap->va_vaflags & VA_UTIMES_NULL) {
				KAUTH_DEBUG("ATTR - non-root/owner changing timestamps, requiring WRITE_ATTRIBUTES");
				required_action |= KAUTH_VNODE_WRITE_ATTRIBUTES;
			} else {
				KAUTH_DEBUG("ATTR - ERROR: illegal timestamp modification attempted");
				error = EACCES;
				goto out;
			}
		}
	}

	  
                       
    
	if (VATTR_IS_ACTIVE(vap, va_mode) && VATTR_IS_SUPPORTED(&ova, va_mode) && (ova.va_mode != vap->va_mode)) {
		KAUTH_DEBUG("ATTR - mode change from %06o to %06o", ova.va_mode, vap->va_mode);

		  
                                                               
     
		if (has_priv_suser) {
			KAUTH_DEBUG("ATTR - superuser mode change, requiring immutability check");
			required_action |= KAUTH_VNODE_CHECKIMMUTABLE;
		} else {
			                         
			KAUTH_DEBUG("ATTR - non-superuser mode change, requiring WRITE_SECURITY");
			required_action |= KAUTH_VNODE_WRITE_SECURITY;
		}

		  
                                                                                    
                                                                           
     
		if (vap->va_mode & S_ISGID) {
			required_action |= KAUTH_VNODE_CHECKIMMUTABLE;                       
			if (!has_priv_suser) {
				if (VATTR_IS_ACTIVE(vap, va_gid)) {
					group = vap->va_gid;
				} else if (VATTR_IS_SUPPORTED(&ova, va_gid)) {
					group = ova.va_gid;
				} else {
					KAUTH_DEBUG("ATTR - ERROR: setgid but no gid available");
					error = EINVAL;
					goto out;
				}
				  
                                                                        
                                                                            
       
				if ((error = kauth_cred_ismember_gid(cred, group, &ismember)) != 0) {
					KAUTH_DEBUG("ATTR - ERROR: got %d checking for membership in %d", error, vap->va_gid);
					goto out;
				}
				if (!ismember) {
					KAUTH_DEBUG("  DENIED - can't set SGID bit, not a member of %d", group);
					error = EPERM;
					goto out;
				}
			}
		}

		  
                                                                     
     
		if (vap->va_mode & S_ISUID) {
			required_action |= KAUTH_VNODE_CHECKIMMUTABLE;                       
			if (!has_priv_suser) {
				if (VATTR_IS_ACTIVE(vap, va_uid)) {
					owner = vap->va_uid;
				} else if (VATTR_IS_SUPPORTED(&ova, va_uid)) {
					owner = ova.va_uid;
				} else {
					KAUTH_DEBUG("ATTR - ERROR: setuid but no uid available");
					error = EINVAL;
					goto out;
				}
				if (owner != kauth_cred_getuid(cred)) {
					  
                                                                    
        
					KAUTH_DEBUG("ATTR - ERROR: illegal attempt to set the setuid bit");
					error = EPERM;
					goto out;
				}
			}
		}
	}

	  
                                                                    
                                                                  
                         
   
                                                                      
                                                                     
                                                                           
   
    
	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		                                 
		vap->va_flags &= ~SF_SYNTHETIC;
		ova.va_flags &= ~SF_SYNTHETIC;
		if (VATTR_IS_SUPPORTED(&ova, va_flags)) {
			fdelta = vap->va_flags ^ ova.va_flags;
		} else {
			fdelta = vap->va_flags;
		}

		if (fdelta != 0) {
			KAUTH_DEBUG("ATTR - flags changing, requiring WRITE_SECURITY");
			required_action |= KAUTH_VNODE_WRITE_SECURITY;

			                                        
			if (has_priv_suser) {
				  
                                                                    
                                                                    
                            
       
				if (fdelta & ~(UF_SETTABLE | SF_SETTABLE)) {
					error = EPERM;
					KAUTH_DEBUG("  DENIED - superuser attempt to set illegal flag(s)");
					goto out;
				}
			} else {
				if (fdelta & ~UF_SETTABLE) {
					error = EPERM;
					KAUTH_DEBUG("  DENIED - user attempt to set illegal flag(s)");
					goto out;
				}
			}
			  
                                                             
                                                                  
     
                                                                              
                                                                          
                   
     
                                                                               
                                        
      
			required_action |= KAUTH_VNODE_NOIMMUTABLE;
		}
	}

	  
                                   
    
	chowner = 0;
	chgroup = 0;
	clear_suid = 0;
	clear_sgid = 0;

	  
                
                                                                               
                                                                         
                                                              
    
	if (VATTR_IS_ACTIVE(vap, va_uid)) {
		if (VATTR_IS_SUPPORTED(&ova, va_uid) && (vap->va_uid != ova.va_uid)) {
			if (!has_priv_suser && (kauth_cred_getuid(cred) != vap->va_uid)) {
				KAUTH_DEBUG("  DENIED - non-superuser cannot change ownershipt to a third party");
				error = EPERM;
				goto out;
			}
			chowner = 1;
		}
		clear_suid = 1;
	}

	  
                
                                                                               
                                                                         
                                                              
    
	if (VATTR_IS_ACTIVE(vap, va_gid)) {
		if (VATTR_IS_SUPPORTED(&ova, va_gid) && (vap->va_gid != ova.va_gid)) {
			if (!has_priv_suser) {
				if ((error = kauth_cred_ismember_gid(cred, vap->va_gid, &ismember)) != 0) {
					KAUTH_DEBUG("  ERROR - got %d checking for membership in %d", error, vap->va_gid);
					goto out;
				}
				if (!ismember) {
					KAUTH_DEBUG("  DENIED - group change from %d to %d but not a member of target group",
					    ova.va_gid, vap->va_gid);
					error = EPERM;
					goto out;
				}
			}
			chgroup = 1;
		}
		clear_sgid = 1;
	}

	  
                                    
    
	if (VATTR_IS_ACTIVE(vap, va_uuuid)) {
		                                                    
		if (VATTR_IS_SUPPORTED(&ova, va_uuuid)) {
			if (kauth_guid_equal(&vap->va_uuuid, &ova.va_uuuid)) {
				goto no_uuuid_change;
			}

			  
                                                     
                                                         
      
			if (kauth_guid_equal(&ova.va_uuuid, &kauth_null_guid) &&
			    VATTR_IS_SUPPORTED(&ova, va_uid)) {
				guid_t uid_guid;

				if (kauth_cred_uid2guid(ova.va_uid, &uid_guid) == 0 &&
				    kauth_guid_equal(&vap->va_uuuid, &uid_guid)) {
					goto no_uuuid_change;
				}
			}
		}

		  
                                                                           
                                                          
                                                          
                                                         
            
     
		if (!has_priv_suser) {
			if ((error = kauth_cred_getguid(cred, &changer)) != 0) {
				KAUTH_DEBUG("  ERROR - got %d trying to get caller UUID", error);
				                                                             
				goto out;
			}
			if (!kauth_guid_equal(&vap->va_uuuid, &changer) &&
			    !kauth_guid_equal(&vap->va_uuuid, &kauth_null_guid)) {
				KAUTH_DEBUG("  ERROR - cannot set supplied owner UUID - not us / null");
				error = EPERM;
				goto out;
			}
		}
		chowner = 1;
		clear_suid = 1;
	}
no_uuuid_change:
	  
                                    
    
	if (VATTR_IS_ACTIVE(vap, va_guuid)) {
		                                                    
		if (VATTR_IS_SUPPORTED(&ova, va_guuid)) {
			if (kauth_guid_equal(&vap->va_guuid, &ova.va_guuid)) {
				goto no_guuid_change;
			}

			  
                                                     
                                                         
      
			if (kauth_guid_equal(&ova.va_guuid, &kauth_null_guid) &&
			    VATTR_IS_SUPPORTED(&ova, va_gid)) {
				guid_t gid_guid;

				if (kauth_cred_gid2guid(ova.va_gid, &gid_guid) == 0 &&
				    kauth_guid_equal(&vap->va_guuid, &gid_guid)) {
					goto no_guuid_change;
				}
			}
		}

		  
                                                                           
                                                              
                     
                                                          
                                                         
            
     
		if (!has_priv_suser) {
			if (kauth_guid_equal(&vap->va_guuid, &kauth_null_guid)) {
				ismember = 1;
			} else if ((error = kauth_cred_ismember_guid(cred, &vap->va_guuid, &ismember)) != 0) {
				KAUTH_DEBUG("  ERROR - got %d trying to check group membership", error);
				goto out;
			}
			if (!ismember) {
				KAUTH_DEBUG("  ERROR - cannot set supplied group UUID - not a member / null");
				error = EPERM;
				goto out;
			}
		}
		chgroup = 1;
	}
no_guuid_change:

	  
                                                      
    
	if (chowner || chgroup || clear_suid || clear_sgid) {
		if (has_priv_suser) {
			KAUTH_DEBUG("ATTR - superuser changing file owner/group, requiring immutability check");
			required_action |= KAUTH_VNODE_CHECKIMMUTABLE;
		} else {
			if (chowner) {
				KAUTH_DEBUG("ATTR - ownership change, requiring TAKE_OWNERSHIP");
				required_action |= KAUTH_VNODE_TAKE_OWNERSHIP;
			}
			if (chgroup && !chowner) {
				KAUTH_DEBUG("ATTR - group change, requiring WRITE_SECURITY");
				required_action |= KAUTH_VNODE_WRITE_SECURITY;
			}
		}

		  
                                                                 
                                                         
     
		if (VATTR_IS_ACTIVE(vap, va_mode)) {
			newmode = vap->va_mode;
		} else if (VATTR_IS_SUPPORTED(&ova, va_mode)) {
			newmode = ova.va_mode;
		} else {
			KAUTH_DEBUG("CHOWN - trying to change owner but cannot get mode from filesystem to mask setugid bits");
			newmode = 0;
		}

		                                                                
                                                                                 
                                                                              
             
     
		if (newmode & (S_ISUID | S_ISGID)) {
			if (!VATTR_IS_ACTIVE(vap, va_mode)) {
				KAUTH_DEBUG("CHOWN - masking setugid bits from mode %o to %o",
				    newmode, newmode & ~(S_ISUID | S_ISGID));
				newmode &= ~(S_ISUID | S_ISGID);
			}
			VATTR_SET(vap, va_mode, newmode);
		}
	}

	  
                                 
    
	if (VATTR_IS_ACTIVE(vap, va_acl)) {
		                     
		if (!VATTR_IS_ACTIVE(&ova, va_acl) || (ova.va_acl == NULL)) {
			                   
			if (vap->va_acl != NULL) {
				required_action |= KAUTH_VNODE_WRITE_SECURITY;
				KAUTH_DEBUG("CHMOD - adding ACL");
			}

			                              
		} else if (vap->va_acl == NULL) {
			required_action |= KAUTH_VNODE_WRITE_SECURITY;
			KAUTH_DEBUG("CHMOD - removing ACL");

			                              
		} else {
			if (vap->va_acl->acl_entrycount != ova.va_acl->acl_entrycount) {
				                                            
				required_action |= KAUTH_VNODE_WRITE_SECURITY;
				KAUTH_DEBUG("CHMOD - adding/removing ACL entries");
			} else if (vap->va_acl->acl_entrycount > 0) {
				                                                                                      
				if (memcmp(&vap->va_acl->acl_ace[0], &ova.va_acl->acl_ace[0],
				    sizeof(struct kauth_ace) * vap->va_acl->acl_entrycount)) {
					required_action |= KAUTH_VNODE_WRITE_SECURITY;
					KAUTH_DEBUG("CHMOD - changing ACL entries");
				}
			}
		}
	}

	  
                                                
    
	if (VATTR_IS_ACTIVE(vap, va_encoding)) {
		required_action |= KAUTH_VNODE_WRITE_ATTRIBUTES;
	}

out:
	if (VATTR_IS_SUPPORTED(&ova, va_acl) && (ova.va_acl != NULL)) {
		kauth_acl_free(ova.va_acl);
	}
	if (error == 0) {
		*actionp = required_action;
	}
	return error;
}

static int
setlocklocal_callback(struct vnode *vp, __unused void *cargs)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VLOCKLOCAL;
	vnode_unlock(vp);

	return VNODE_RETURNED;
}

void
vfs_setlocklocal(mount_t mp)
{
	mount_lock_spin(mp);
	mp->mnt_kern_flag |= MNTK_LOCK_LOCAL;
	mount_unlock(mp);

	  
                                                 
                                                
    
	vnode_iterate(mp, 0, setlocklocal_callback, NULL);
}

void
vfs_setcompoundopen(mount_t mp)
{
	mount_lock_spin(mp);
	mp->mnt_compound_ops |= COMPOUND_VNOP_OPEN;
	mount_unlock(mp);
}

void
vnode_setswapmount(vnode_t vp)
{
	mount_lock(vp->v_mount);
	vp->v_mount->mnt_kern_flag |= MNTK_SWAP_MOUNT;
	mount_unlock(vp->v_mount);
}


int64_t
vnode_getswappin_avail(vnode_t vp)
{
	int64_t max_swappin_avail = 0;

	mount_lock(vp->v_mount);
	if (vp->v_mount->mnt_ioflags & MNT_IOFLAGS_SWAPPIN_SUPPORTED) {
		max_swappin_avail = vp->v_mount->mnt_max_swappin_available;
	}
	mount_unlock(vp->v_mount);

	return max_swappin_avail;
}


void
vn_setunionwait(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VISUNION;
	vnode_unlock(vp);
}


void
vn_checkunionwait(vnode_t vp)
{
	vnode_lock_spin(vp);
	while ((vp->v_flag & VISUNION) == VISUNION) {
		msleep((caddr_t)&vp->v_flag, &vp->v_lock, 0, 0, 0);
	}
	vnode_unlock(vp);
}

void
vn_clearunionwait(vnode_t vp, int locked)
{
	if (!locked) {
		vnode_lock_spin(vp);
	}
	if ((vp->v_flag & VISUNION) == VISUNION) {
		vp->v_flag &= ~VISUNION;
		wakeup((caddr_t)&vp->v_flag);
	}
	if (!locked) {
		vnode_unlock(vp);
	}
}

int
vnode_materialize_dataless_file(vnode_t vp, uint64_t op_type)
{
	int error;

	                                         
	if (vnode_isswap(vp)) {
		return 0;
	}

	error = resolve_nspace_item(vp,
	    op_type | NAMESPACE_HANDLER_NSPACE_EVENT);

	  
                                                               
                                                              
               
    
	if (error == EJUSTRETURN) {
		  
                                                       
                                                      
                                           
     
		switch (op_type) {
		case NAMESPACE_HANDLER_WRITE_OP:
		case NAMESPACE_HANDLER_TRUNCATE_OP:
		case NAMESPACE_HANDLER_RENAME_OP:
			  
                                                  
                                              
            
      
			error = 0;
			break;
		case NAMESPACE_HANDLER_READ_OP:
			  
                                                   
                                                     
                                                  
                                              
      
			error = (vnode_vtype(vp) == VDIR) ? 0 : EBADF;
			break;
		default:
			error = EBADF;
			break;
		}
	}

	return error;
}

  
                                                     
            
                      
                                                               
                                                                                                                        
                                                            
                                                                                          
                    
                                                                  
   

errno_t
rmdir_remove_orphaned_appleDouble(vnode_t vp, vfs_context_t ctx, int * restart_flag)
{
#define UIO_BUFF_SIZE 2048
	uio_t auio = NULL;
	int eofflag, siz = UIO_BUFF_SIZE, alloc_size = 0, nentries = 0;
	int open_flag = 0, full_erase_flag = 0;
	char uio_buf[UIO_SIZEOF(1)];
	char *rbuf = NULL;
	void *dir_pos;
	void *dir_end;
	struct dirent *dp;
	errno_t error;

	error = vnode_suspend(vp);

	  
                                                                   
    
	if (error == EBUSY) {
		*restart_flag = 1;
	}
	if (error != 0) {
		return error;
	}

	  
                                                        
                      
    
	uthread_t ut = get_bsdthread_info(current_thread());
	bool saved_nodatalessfaults =
	    (ut->uu_flag & UT_NSPACE_NODATALESSFAULTS) ? true : false;
	ut->uu_flag |= UT_NSPACE_NODATALESSFAULTS;

	  
              
    
	rbuf = kheap_alloc(KHEAP_DATA_BUFFERS, siz, Z_WAITOK);
	alloc_size = siz;
	if (rbuf) {
		auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
	}
	if (!rbuf || !auio) {
		error = ENOMEM;
		goto outsc;
	}

	uio_setoffset(auio, 0);

	eofflag = 0;

	if ((error = VNOP_OPEN(vp, FREAD, ctx))) {
		goto outsc;
	} else {
		open_flag = 1;
	}

	  
                                                         
    

	do {
		siz = UIO_BUFF_SIZE;
		uio_reset(auio, uio_offset(auio), UIO_SYSSPACE, UIO_READ);
		uio_addiov(auio, CAST_USER_ADDR_T(rbuf), UIO_BUFF_SIZE);

		if ((error = VNOP_READDIR(vp, auio, 0, &eofflag, &nentries, ctx))) {
			goto outsc;
		}

		if (uio_resid(auio) != 0) {
			siz -= uio_resid(auio);
		}

		  
                              
     
		dir_pos = (void*) rbuf;
		dir_end = (void*) (rbuf + siz);
		dp = (struct dirent*) (dir_pos);

		if (dir_pos == dir_end) {
			eofflag = 1;
		}

		while (dir_pos < dir_end) {
			  
                                               
      
			if (dp->d_ino != 0 &&
			    !((dp->d_namlen == 1 && dp->d_name[0] == '.') ||
			    (dp->d_namlen == 2 && dp->d_name[0] == '.' && dp->d_name[1] == '.'))) {
				  
                                             
                                           
       
				if (dp->d_namlen < 2 ||
				    strncmp(dp->d_name, "._", 2) ||
				    (dp->d_namlen >= 4 && !strncmp(&(dp->d_name[2]), "._", 2))) {
					error = ENOTEMPTY;
					goto outsc;
				}
			}
			dir_pos = (void*) ((uint8_t*)dir_pos + dp->d_reclen);
			dp = (struct dirent*)dir_pos;
		}

		  
                                                              
     
		if (vp->v_tag == VT_HFS && nentries > 2) {
			eofflag = 0;
		}

		if (vp->v_tag == VT_NFS) {
			if (eofflag && !full_erase_flag) {
				full_erase_flag = 1;
				eofflag = 0;
				uio_reset(auio, 0, UIO_SYSSPACE, UIO_READ);
			} else if (!eofflag && full_erase_flag) {
				full_erase_flag = 0;
			}
		}
	} while (!eofflag);
	  
                                                                
                                                             
                                         
    

	uio_reset(auio, 0, UIO_SYSSPACE, UIO_READ);
	eofflag = 0;
	full_erase_flag = 0;

	do {
		siz = UIO_BUFF_SIZE;
		uio_reset(auio, uio_offset(auio), UIO_SYSSPACE, UIO_READ);
		uio_addiov(auio, CAST_USER_ADDR_T(rbuf), UIO_BUFF_SIZE);

		error = VNOP_READDIR(vp, auio, 0, &eofflag, &nentries, ctx);

		if (error != 0) {
			goto outsc;
		}

		if (uio_resid(auio) != 0) {
			siz -= uio_resid(auio);
		}

		  
                              
     
		dir_pos = (void*) rbuf;
		dir_end = (void*) (rbuf + siz);
		dp = (struct dirent*) dir_pos;

		if (dir_pos == dir_end) {
			eofflag = 1;
		}

		while (dir_pos < dir_end) {
			  
                                               
      
			if (dp->d_ino != 0 &&
			    !((dp->d_namlen == 1 && dp->d_name[0] == '.') ||
			    (dp->d_namlen == 2 && dp->d_name[0] == '.' && dp->d_name[1] == '.'))
			    ) {
				error = unlink1(ctx, vp,
				    CAST_USER_ADDR_T(dp->d_name), UIO_SYSSPACE,
				    VNODE_REMOVE_SKIP_NAMESPACE_EVENT |
				    VNODE_REMOVE_NO_AUDIT_PATH);

				if (error && error != ENOENT) {
					goto outsc;
				}
			}
			dir_pos = (void*) ((uint8_t*)dir_pos + dp->d_reclen);
			dp = (struct dirent*)dir_pos;
		}

		  
                                                              
     
		if (vp->v_tag == VT_HFS && nentries > 2) {
			eofflag = 0;
		}

		if (vp->v_tag == VT_NFS) {
			if (eofflag && !full_erase_flag) {
				full_erase_flag = 1;
				eofflag = 0;
				uio_reset(auio, 0, UIO_SYSSPACE, UIO_READ);
			} else if (!eofflag && full_erase_flag) {
				full_erase_flag = 0;
			}
		}
	} while (!eofflag);


	error = 0;

outsc:
	if (open_flag) {
		VNOP_CLOSE(vp, FREAD, ctx);
	}

	if (auio) {
		uio_free(auio);
	}
	kheap_free(KHEAP_DATA_BUFFERS, rbuf, alloc_size);

	if (saved_nodatalessfaults == false) {
		ut->uu_flag &= ~UT_NSPACE_NODATALESSFAULTS;
	}

	vnode_resume(vp);

	return error;
}


void
lock_vnode_and_post(vnode_t vp, int kevent_num)
{
	                                                    
	if (vp->v_knotes.slh_first != NULL) {
		vnode_lock(vp);
		KNOTE(&vp->v_knotes, kevent_num);
		vnode_unlock(vp);
	}
}

void panic_print_vnodes(void);

                                                                   
#ifdef PANIC_PRINTS_VNODES

static const char *
__vtype(uint16_t vtype)
{
	switch (vtype) {
	case VREG:
		return "R";
	case VDIR:
		return "D";
	case VBLK:
		return "B";
	case VCHR:
		return "C";
	case VLNK:
		return "L";
	case VSOCK:
		return "S";
	case VFIFO:
		return "F";
	case VBAD:
		return "x";
	case VSTR:
		return "T";
	case VCPLX:
		return "X";
	default:
		return "?";
	}
}

  
                                  
                                                                          
   
static char *
__vpath(vnode_t vp, char *str, int len, int depth)
{
	int vnm_len;
	const char *src;
	char *dst;

	if (len <= 0) {
		return str;
	}
	                                                     
	if (!vp->v_name) {
		return str + len;
	}

	                                              
	if ((vp->v_flag & VROOT)) {
		if (vp->v_mount != NULL && vp->v_mount->mnt_vnodecovered) {
			return __vpath(vp->v_mount->mnt_vnodecovered,
			           str, len, depth + 1);
		}
		return str + len;
	}

	src = vp->v_name;
	vnm_len = strlen(src);
	if (vnm_len > len) {
		                                            
		src += (vnm_len - len);
		vnm_len = len;
	}

	                                                             

	                                               
	if (depth > 0) {
		dst = str + len - vnm_len;
		memcpy(dst, src, vnm_len);
		len -= vnm_len;
	} else {
		dst = str + len;
	}

	if (vp->v_parent && len > 1) {
		                                 
		len--;
		*(dst - 1) = '/';
		return __vpath(vp->v_parent, str, len, depth + 1);
	}

	return dst;
}

#define SANE_VNODE_PRINT_LIMIT 5000
void
panic_print_vnodes(void)
{
	mount_t mnt;
	vnode_t vp;
	int nvnodes = 0;
	const char *type;
	char *nm;
	char vname[257];

	paniclog_append_noflush("\n***** VNODES *****\n"
	    "TYPE UREF ICNT PATH\n");

	                                  
	vname[sizeof(vname) - 1] = '\0';

	  
                                                                        
    
	TAILQ_FOREACH(mnt, &mountlist, mnt_list) {
		if (!ml_validate_nofault((vm_offset_t)mnt, sizeof(mount_t))) {
			paniclog_append_noflush("Unable to iterate the mount list %p - encountered an invalid mount pointer %p \n",
			    &mountlist, mnt);
			break;
		}

		TAILQ_FOREACH(vp, &mnt->mnt_vnodelist, v_mntvnodes) {
			if (!ml_validate_nofault((vm_offset_t)vp, sizeof(vnode_t))) {
				paniclog_append_noflush("Unable to iterate the vnode list %p - encountered an invalid vnode pointer %p \n",
				    &mnt->mnt_vnodelist, vp);
				break;
			}

			if (++nvnodes > SANE_VNODE_PRINT_LIMIT) {
				return;
			}
			type = __vtype(vp->v_type);
			nm = __vpath(vp, vname, sizeof(vname) - 1, 0);
			paniclog_append_noflush("%s %0d %0d %s\n",
			    type, vp->v_usecount, vp->v_iocount, nm);
		}
	}
}

#else                           
void
panic_print_vnodes(void)
{
	return;
}
#endif


#ifdef JOE_DEBUG
static void
record_vp(vnode_t vp, int count)
{
	struct uthread *ut;

#if CONFIG_TRIGGERS
	if (vp->v_resolve) {
		return;
	}
#endif
	if ((vp->v_flag & VSYSTEM)) {
		return;
	}

	ut = get_bsdthread_info(current_thread());
	ut->uu_iocount += count;

	if (count == 1) {
		if (ut->uu_vpindex < 32) {
			OSBacktrace((void **)&ut->uu_pcs[ut->uu_vpindex][0], 10);

			ut->uu_vps[ut->uu_vpindex] = vp;
			ut->uu_vpindex++;
		}
	}
}
#endif


#if CONFIG_TRIGGERS

#define TRIG_DEBUG 0

#if TRIG_DEBUG
#define TRIG_LOG(...) do { printf("%s: ", __FUNCTION__); printf(__VA_ARGS__); } while (0)
#else
#define TRIG_LOG(...)
#endif

  
                            
   

resolver_result_t
vfs_resolver_result(uint32_t seq, enum resolver_status stat, int aux)
{
	  
                                             
                                            
    
	return (((uint64_t)seq) << 32) |
	       (((uint64_t)(aux & 0x0fffffff)) << 4) |
	       (uint64_t)(stat & 0x0000000F);
}

enum resolver_status
vfs_resolver_status(resolver_result_t result)
{
	                            
	return result & 0x0000000F;
}

uint32_t
vfs_resolver_sequence(resolver_result_t result)
{
	                               
	return (uint32_t)(result >> 32);
}

int
vfs_resolver_auxiliary(resolver_result_t result)
{
	                          
	return (int)(((uint32_t)(result & 0xFFFFFFF0)) >> 4);
}

  
      
                                                      
   
int
vnode_trigger_update(vnode_t vp, resolver_result_t result)
{
	vnode_resolve_t rp;
	uint32_t seq;
	enum resolver_status stat;

	if (vp->v_resolve == NULL) {
		return EINVAL;
	}

	stat = vfs_resolver_status(result);
	seq = vfs_resolver_sequence(result);

	if ((stat != RESOLVER_RESOLVED) && (stat != RESOLVER_UNRESOLVED)) {
		return EINVAL;
	}

	rp = vp->v_resolve;
	lck_mtx_lock(&rp->vr_lock);

	if (seq > rp->vr_lastseq) {
		if (stat == RESOLVER_RESOLVED) {
			rp->vr_flags |= VNT_RESOLVED;
		} else {
			rp->vr_flags &= ~VNT_RESOLVED;
		}

		rp->vr_lastseq = seq;
	}

	lck_mtx_unlock(&rp->vr_lock);

	return 0;
}

static int
vnode_resolver_attach(vnode_t vp, vnode_resolve_t rp, boolean_t ref)
{
	int error;

	vnode_lock_spin(vp);
	if (vp->v_resolve != NULL) {
		vnode_unlock(vp);
		return EINVAL;
	} else {
		vp->v_resolve = rp;
	}
	vnode_unlock(vp);

	if (ref) {
		error = vnode_ref_ext(vp, O_EVTONLY, VNODE_REF_FORCE);
		if (error != 0) {
			panic("VNODE_REF_FORCE didn't help...");
		}
	}

	return 0;
}

  
                                             
  
                                               
                                                
   
static int
vnode_resolver_create(mount_t mp, vnode_t vp, struct vnode_trigger_param *tinfo, boolean_t external)
{
	vnode_resolve_t rp;
	int result;
	char byte;

#if 1
	                                      
	if (tinfo->vnt_data) {
		byte = *((char *)tinfo->vnt_data);
	}
#endif
	rp = kheap_alloc(KHEAP_DEFAULT, sizeof(struct vnode_resolve), Z_WAITOK);
	if (rp == NULL) {
		return ENOMEM;
	}

	lck_mtx_init(&rp->vr_lock, &trigger_vnode_lck_grp, &trigger_vnode_lck_attr);

	rp->vr_resolve_func = tinfo->vnt_resolve_func;
	rp->vr_unresolve_func = tinfo->vnt_unresolve_func;
	rp->vr_rearm_func = tinfo->vnt_rearm_func;
	rp->vr_reclaim_func = tinfo->vnt_reclaim_func;
	rp->vr_data = tinfo->vnt_data;
	rp->vr_lastseq = 0;
	rp->vr_flags = tinfo->vnt_flags & VNT_VALID_MASK;
	if (external) {
		rp->vr_flags |= VNT_EXTERNAL;
	}

	result = vnode_resolver_attach(vp, rp, external);
	if (result != 0) {
		goto out;
	}

	if (mp) {
		OSAddAtomic(1, &mp->mnt_numtriggers);
	}

	return result;

out:
	kheap_free(KHEAP_DEFAULT, rp, sizeof(struct vnode_resolve));
	return result;
}

static void
vnode_resolver_release(vnode_resolve_t rp)
{
	  
                                               
    
	if (rp->vr_data && rp->vr_reclaim_func) {
		rp->vr_reclaim_func(NULLVP, rp->vr_data);
	}

	lck_mtx_destroy(&rp->vr_lock, &trigger_vnode_lck_grp);
	kheap_free(KHEAP_DEFAULT, rp, sizeof(struct vnode_resolve));
}

                                             
static void
vnode_resolver_detach(vnode_t vp)
{
	vnode_resolve_t rp;
	mount_t mp;

	mp = vnode_mount(vp);

	vnode_lock(vp);
	rp = vp->v_resolve;
	vp->v_resolve = NULL;
	vnode_unlock(vp);

	if ((rp->vr_flags & VNT_EXTERNAL) != 0) {
		vnode_rele_ext(vp, O_EVTONLY, 1);
	}

	vnode_resolver_release(rp);

	                                                   
	OSAddAtomic(-1, &mp->mnt_numtriggers);
}

__private_extern__
void
vnode_trigger_rearm(vnode_t vp, vfs_context_t ctx)
{
	vnode_resolve_t rp;
	resolver_result_t result;
	enum resolver_status status;
	uint32_t seq;

	if ((vp->v_resolve == NULL) ||
	    (vp->v_resolve->vr_rearm_func == NULL) ||
	    (vp->v_resolve->vr_flags & VNT_AUTO_REARM) == 0) {
		return;
	}

	rp = vp->v_resolve;
	lck_mtx_lock(&rp->vr_lock);

	  
                                                                                             
    
	if (rp->vr_flags & VNT_VFS_UNMOUNTED) {
		lck_mtx_unlock(&rp->vr_lock);
		return;
	}

	                                          
	if ((rp->vr_flags & VNT_RESOLVED) == 0) {
		lck_mtx_unlock(&rp->vr_lock);
		return;
	}

	lck_mtx_unlock(&rp->vr_lock);

	result = rp->vr_rearm_func(vp, 0, rp->vr_data, ctx);
	status = vfs_resolver_status(result);
	seq = vfs_resolver_sequence(result);

	lck_mtx_lock(&rp->vr_lock);
	if (seq > rp->vr_lastseq) {
		if (status == RESOLVER_UNRESOLVED) {
			rp->vr_flags &= ~VNT_RESOLVED;
		}
		rp->vr_lastseq = seq;
	}
	lck_mtx_unlock(&rp->vr_lock);
}

__private_extern__
int
vnode_trigger_resolve(vnode_t vp, struct nameidata *ndp, vfs_context_t ctx)
{
	vnode_resolve_t rp;
	enum path_operation op;
	resolver_result_t result;
	enum resolver_status status;
	uint32_t seq;

	  
                                                          
                                                        
                                                          
                                                             
                                     
    

	                                    
	if ((vp->v_resolve == NULL) ||
	    (vp->v_resolve->vr_resolve_func == NULL) ||
	    (vp->v_mountedhere != NULL)) {
		return 0;
	}

	rp = vp->v_resolve;
	lck_mtx_lock(&rp->vr_lock);

	                                             
	if (rp->vr_flags & VNT_RESOLVED) {
		lck_mtx_unlock(&rp->vr_lock);
		return 0;
	}

	lck_mtx_unlock(&rp->vr_lock);

#if CONFIG_MACF
	if ((rp->vr_flags & VNT_KERN_RESOLVE) == 0) {
		  
                                                              
                                                           
                                                          
     
		int rv = mac_vnode_check_trigger_resolve(ctx, vp, &ndp->ni_cnd);
		if (rv != 0) {
			return rv;
		}
	}
#endif

	  
       
                                                                                                 
                                   
                                                          
   
                                                                  
   
                                                                                   
                                                                       
    
	op = (ndp->ni_op < OP_MAXOP) ? ndp->ni_op: OP_LOOKUP;

	result = rp->vr_resolve_func(vp, &ndp->ni_cnd, op, 0, rp->vr_data, ctx);
	status = vfs_resolver_status(result);
	seq = vfs_resolver_sequence(result);

	lck_mtx_lock(&rp->vr_lock);
	if (seq > rp->vr_lastseq) {
		if (status == RESOLVER_RESOLVED) {
			rp->vr_flags |= VNT_RESOLVED;
		}
		rp->vr_lastseq = seq;
	}
	lck_mtx_unlock(&rp->vr_lock);

	                                                     
	return status == RESOLVER_ERROR ? vfs_resolver_auxiliary(result) : 0;
}

static int
vnode_trigger_unresolve(vnode_t vp, int flags, vfs_context_t ctx)
{
	vnode_resolve_t rp;
	resolver_result_t result;
	enum resolver_status status;
	uint32_t seq;

	if ((vp->v_resolve == NULL) || (vp->v_resolve->vr_unresolve_func == NULL)) {
		return 0;
	}

	rp = vp->v_resolve;
	lck_mtx_lock(&rp->vr_lock);

	                                             
	if ((rp->vr_flags & VNT_RESOLVED) == 0) {
		printf("vnode_trigger_unresolve: not currently resolved\n");
		lck_mtx_unlock(&rp->vr_lock);
		return 0;
	}

	rp->vr_flags |= VNT_VFS_UNMOUNTED;

	lck_mtx_unlock(&rp->vr_lock);

	  
       
                                                                                                 
                                                          
   
                                                                  
    

	result = rp->vr_unresolve_func(vp, flags, rp->vr_data, ctx);
	status = vfs_resolver_status(result);
	seq = vfs_resolver_sequence(result);

	lck_mtx_lock(&rp->vr_lock);
	if (seq > rp->vr_lastseq) {
		if (status == RESOLVER_UNRESOLVED) {
			rp->vr_flags &= ~VNT_RESOLVED;
		}
		rp->vr_lastseq = seq;
	}
	rp->vr_flags &= ~VNT_VFS_UNMOUNTED;
	lck_mtx_unlock(&rp->vr_lock);

	                                                     
	return status == RESOLVER_ERROR ? vfs_resolver_auxiliary(result) : 0;
}

static int
triggerisdescendant(mount_t mp, mount_t rmp)
{
	int match = FALSE;

	  
                                                   
    
	name_cache_lock_shared();

	while (1) {
		vnode_t vp;

		                            
		if (mp->mnt_flag & MNT_ROOTFS) {
			break;
		}

		vp = mp->mnt_vnodecovered;
		if (vp == NULLVP) {
			break;
		}

		mp = vp->v_mount;
		if (mp == rmp) {
			match = TRUE;
			break;
		}
	}

	name_cache_unlock();

	return match;
}

struct trigger_unmount_info {
	vfs_context_t   ctx;
	mount_t         top_mp;
	vnode_t         trigger_vp;
	mount_t         trigger_mp;
	uint32_t        trigger_vid;
	int             flags;
};

static int
trigger_unmount_callback(mount_t mp, void * arg)
{
	struct trigger_unmount_info * infop = (struct trigger_unmount_info *)arg;
	boolean_t mountedtrigger = FALSE;

	  
                                                    
    
	if (mp == infop->top_mp) {
		return VFS_RETURNED_DONE;
	}

	if ((mp->mnt_vnodecovered == NULL) ||
	    (vnode_getwithref(mp->mnt_vnodecovered) != 0)) {
		return VFS_RETURNED;
	}

	if ((mp->mnt_vnodecovered->v_mountedhere == mp) &&
	    (mp->mnt_vnodecovered->v_resolve != NULL) &&
	    (mp->mnt_vnodecovered->v_resolve->vr_flags & VNT_RESOLVED)) {
		mountedtrigger = TRUE;
	}
	vnode_put(mp->mnt_vnodecovered);

	  
                                                                               
    
	if (!mountedtrigger || !triggerisdescendant(mp, infop->top_mp)) {
		return VFS_RETURNED;
	}

	  
                                                                  
    
	if ((infop->trigger_vp != NULLVP) &&
	    (vnode_getwithvid(infop->trigger_vp, infop->trigger_vid) == 0)) {
		vnode_t vp = infop->trigger_vp;
		int error;

		infop->trigger_vp = NULLVP;

		if (mp == vp->v_mountedhere) {
			vnode_put(vp);
			printf("trigger_unmount_callback: unexpected match '%s'\n",
			    mp->mnt_vfsstat.f_mntonname);
			return VFS_RETURNED;
		}
		if (infop->trigger_mp != vp->v_mountedhere) {
			vnode_put(vp);
			printf("trigger_unmount_callback: trigger mnt changed! (%p != %p)\n",
			    infop->trigger_mp, vp->v_mountedhere);
			goto savenext;
		}

		error = vnode_trigger_unresolve(vp, infop->flags, infop->ctx);
		vnode_put(vp);
		if (error) {
			printf("unresolving: '%s', err %d\n",
			    vp->v_mountedhere ? vp->v_mountedhere->mnt_vfsstat.f_mntonname :
			    "???", error);
			return VFS_RETURNED_DONE;                               
		}
	}
savenext:
	  
                                                          
                                                         
    
	infop->trigger_vp = mp->mnt_vnodecovered;
	if ((infop->trigger_vp != NULLVP) &&
	    (vnode_getwithref(infop->trigger_vp) == 0)) {
		if (infop->trigger_vp->v_mountedhere == mp) {
			infop->trigger_vid = infop->trigger_vp->v_id;
			infop->trigger_mp = mp;
		}
		vnode_put(infop->trigger_vp);
	}

	return VFS_RETURNED;
}

  
                                                                   
                                                                   
  
                                                                     
   
__private_extern__
void
vfs_nested_trigger_unmounts(mount_t mp, int flags, vfs_context_t ctx)
{
	struct trigger_unmount_info info;

	                              
	if (mp->mnt_numtriggers == 0) {
		return;
	}
	                                                          
	if ((mp->mnt_vnodecovered != NULL) &&
	    (vnode_getwithref(mp->mnt_vnodecovered) == 0)) {
		boolean_t recursive = FALSE;

		if ((mp->mnt_vnodecovered->v_mountedhere == mp) &&
		    (mp->mnt_vnodecovered->v_resolve != NULL) &&
		    (mp->mnt_vnodecovered->v_resolve->vr_flags & VNT_VFS_UNMOUNTED)) {
			recursive = TRUE;
		}
		vnode_put(mp->mnt_vnodecovered);
		if (recursive) {
			return;
		}
	}

	  
                                                              
    
	info.ctx = ctx;
	info.top_mp = mp;
	info.trigger_vp = NULLVP;
	info.trigger_vid = 0;
	info.trigger_mp = NULL;
	info.flags = flags;

	(void) vfs_iterate(VFS_ITERATE_TAIL_FIRST, trigger_unmount_callback, &info);

	  
                                                                
    
	if ((info.trigger_vp != NULLVP) &&
	    (vnode_getwithvid(info.trigger_vp, info.trigger_vid) == 0)) {
		vnode_t vp = info.trigger_vp;

		if (info.trigger_mp == vp->v_mountedhere) {
			(void) vnode_trigger_unresolve(vp, flags, ctx);
		}
		vnode_put(vp);
	}
}

int
vfs_addtrigger(mount_t mp, const char *relpath, struct vnode_trigger_info *vtip, vfs_context_t ctx)
{
	struct nameidata *ndp;
	int res;
	vnode_t rvp, vp;
	struct vnode_trigger_param vtp;

	  
                                                               
    
	lck_rw_assert(&mp->mnt_rwlock, LCK_RW_ASSERT_HELD);

	TRIG_LOG("Adding trigger at %s\n", relpath);
	TRIG_LOG("Trying VFS_ROOT\n");

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
	if (!ndp) {
		return ENOMEM;
	}

	  
                                                                    
                                    
    
	res = VFS_ROOT(mp, &rvp, ctx);
	if (res != 0) {
		goto out;
	}

	TRIG_LOG("Trying namei\n");

	NDINIT(ndp, LOOKUP, OP_LOOKUP, USEDVP | NOCROSSMOUNT | FOLLOW, UIO_SYSSPACE,
	    CAST_USER_ADDR_T(relpath), ctx);
	ndp->ni_dvp = rvp;
	res = namei(ndp);
	if (res != 0) {
		vnode_put(rvp);
		goto out;
	}

	vp = ndp->ni_vp;
	nameidone(ndp);
	vnode_put(rvp);

	TRIG_LOG("Trying vnode_resolver_create()\n");

	  
                                                         
                                                         
                                                               
                        
    
	bzero(&vtp, sizeof(vtp));
	vtp.vnt_resolve_func = vtip->vti_resolve_func;
	vtp.vnt_unresolve_func = vtip->vti_unresolve_func;
	vtp.vnt_rearm_func = vtip->vti_rearm_func;
	vtp.vnt_reclaim_func = vtip->vti_reclaim_func;
	vtp.vnt_reclaim_func = vtip->vti_reclaim_func;
	vtp.vnt_data = vtip->vti_data;
	vtp.vnt_flags = vtip->vti_flags;

	res = vnode_resolver_create(mp, vp, &vtp, TRUE);
	vnode_put(vp);
out:
	kheap_free(KHEAP_TEMP, ndp, sizeof(struct nameidata));
	TRIG_LOG("Returning %d\n", res);
	return res;
}

#endif                      

vm_offset_t
kdebug_vnode(vnode_t vp)
{
	return VM_KERNEL_ADDRPERM(vp);
}

static int flush_cache_on_write = 0;
SYSCTL_INT(_kern, OID_AUTO, flush_cache_on_write,
    CTLFLAG_RW | CTLFLAG_LOCKED, &flush_cache_on_write, 0,
    "always flush the drive cache on writes to uncached files");

int
vnode_should_flush_after_write(vnode_t vp, int ioflag)
{
	return flush_cache_on_write
	       && (ISSET(ioflag, IO_NOCACHE) || vnode_isnocache(vp));
}

  
                                                                       
                
   

#define NPATH_WORDS (MAXPATHLEN / sizeof(unsigned long))
struct vnode_trace_paths_context {
	uint64_t count;
	  
                                                  
    
	unsigned long path[NPATH_WORDS + (4 - (NPATH_WORDS % 4)) - 1];
};

static int
vnode_trace_path_callback(struct vnode *vp, void *vctx)
{
	struct vnode_trace_paths_context *ctx = vctx;
	size_t path_len = sizeof(ctx->path);

	int getpath_len = (int)path_len;
	if (vn_getpath(vp, (char *)ctx->path, &getpath_len) == 0) {
		                                                            
		assert(getpath_len >= 0);
		path_len = (size_t)getpath_len;

		assert(path_len <= sizeof(ctx->path));
		kdebug_vfs_lookup(ctx->path, (int)path_len, vp,
		    KDBG_VFS_LOOKUP_FLAG_LOOKUP | KDBG_VFS_LOOKUP_FLAG_NOPROCFILT);

		if (++(ctx->count) == 1000) {
			thread_yield_to_preemption();
			ctx->count = 0;
		}
	}

	return VNODE_RETURNED;
}

static int
vfs_trace_paths_callback(mount_t mp, void *arg)
{
	if (mp->mnt_flag & MNT_LOCAL) {
		vnode_iterate(mp, VNODE_ITERATE_ALL, vnode_trace_path_callback, arg);
	}

	return VFS_RETURNED;
}

static int sysctl_vfs_trace_paths SYSCTL_HANDLER_ARGS {
	struct vnode_trace_paths_context ctx;

	(void)oidp;
	(void)arg1;
	(void)arg2;
	(void)req;

	if (!kauth_cred_issuser(kauth_cred_get())) {
		return EPERM;
	}

	if (!kdebug_enable || !kdebug_debugid_enabled(VFS_LOOKUP)) {
		return EINVAL;
	}

	bzero(&ctx, sizeof(struct vnode_trace_paths_context));

	vfs_iterate(0, vfs_trace_paths_callback, &ctx);

	return 0;
}

SYSCTL_PROC(_vfs_generic, OID_AUTO, trace_paths, CTLFLAG_RD | CTLFLAG_LOCKED | CTLFLAG_MASKED, NULL, 0, &sysctl_vfs_trace_paths, "-", "trace_paths");
