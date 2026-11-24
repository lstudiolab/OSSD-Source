/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 *	Copyright (c) 1990, 1996-1998 Apple Computer, Inc.
 *	All Rights Reserved.
 */
  
                                                     
  
                    
                                
  
          
                      
                     
  
   
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <sys/filedesc.h>
#include <sys/stat.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysproto.h>
#include <sys/proc_info.h>
#include <sys/posix_shm.h>
#include <security/audit/audit.h>
#include <stdbool.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <mach/mach_types.h>
#include <mach/mach_vm.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>
#include <mach/vm_inherit.h>
#include <mach/kern_return.h>

#include <vm/vm_map.h>
#include <vm/vm_protos.h>

#define f_flag fp_glob->fg_flag
#define f_ops fp_glob->fg_ops
#define f_data fp_glob->fg_data

  
                                               
                                                 
   
typedef struct pshm_mobj {
	void                  *pshmo_memobject;
	memory_object_size_t  pshmo_size;
	SLIST_ENTRY(pshm_mobj) pshmo_next;
} pshm_mobj_t;

  
                                                          
  
                                                         
                                                           
                                                              
  
                                                                       
                                                                
                                                             
                                                               
                                                      
   
typedef struct internal_pshminfo {
	struct pshminfo pshm_hdr;
	SLIST_HEAD(pshm_mobjhead, pshm_mobj) pshm_mobjs;
	RB_ENTRY(internal_pshminfo) pshm_links;                                      
} pshm_info_t;
#define pshm_flags    pshm_hdr.pshm_flags
#define pshm_usecount pshm_hdr.pshm_usecount
#define pshm_length   pshm_hdr.pshm_length
#define pshm_mode     pshm_hdr.pshm_mode
#define pshm_uid      pshm_hdr.pshm_uid
#define pshm_gid      pshm_hdr.pshm_gid
#define pshm_label    pshm_hdr.pshm_label

                                               
#define PSHM_ALLOCATED  0x004                                     
#define PSHM_MAPPED     0x008                             
#define PSHM_INUSE      0x010                             
#define PSHM_REMOVED    0x020                                                        
#define PSHM_ALLOCATING 0x100                                   

  
                                                                           
   
static int pshm_ref(pshm_info_t *pinfo);
static void pshm_deref(pshm_info_t *pinfo);
#define PSHM_MAXCOUNT UINT_MAX

  
                                                 
                                                               
                                                              
                                                              
                                                             
               
   
typedef struct pshmnode {
	off_t       mapp_addr;
	pshm_info_t *pinfo;
} pshmnode_t;


                                             
static int
pshm_compare(pshm_info_t *a, pshm_info_t *b)
{
	int cmp = strncmp(a->pshm_hdr.pshm_name, b->pshm_hdr.pshm_name, PSHMNAMLEN + 1);

	if (cmp < 0) {
		return -1;
	}
	if (cmp > 0) {
		return 1;
	}
	return 0;
}


  
                                                                  
   
u_long pshmnument;                                                          
RB_HEAD(pshmhead, internal_pshminfo) pshm_head;
RB_PROTOTYPE(pshmhead, internal_pshminfo, pshm_links, pshm_compare)
RB_GENERATE(pshmhead, internal_pshminfo, pshm_links, pshm_compare)

                                   
static pshm_info_t *pshm_cache_search(pshm_info_t * look);
static void pshm_cache_add(pshm_info_t *entry);
static void pshm_cache_delete(pshm_info_t *entry);

static int pshm_closefile(struct fileglob *fg, vfs_context_t ctx);

static int pshm_access(pshm_info_t *pinfo, int mode, kauth_cred_t cred, proc_t p);
int pshm_cache_purge_all(proc_t p);

static int pshm_unlink_internal(pshm_info_t *pinfo);

static const struct fileops pshmops = {
	.fo_type     = DTYPE_PSXSHM,
	.fo_read     = fo_no_read,
	.fo_write    = fo_no_write,
	.fo_ioctl    = fo_no_ioctl,
	.fo_select   = fo_no_select,
	.fo_close    = pshm_closefile,
	.fo_drain    = fo_no_drain,
	.fo_kqfilter = fo_no_kqfilter,
};

  
                                                  
   
static LCK_GRP_DECLARE(psx_shm_subsys_lck_grp, "posix shared memory");
static LCK_MTX_DECLARE(psx_shm_subsys_mutex, &psx_shm_subsys_lck_grp);

#define PSHM_SUBSYS_LOCK() lck_mtx_lock(& psx_shm_subsys_mutex)
#define PSHM_SUBSYS_UNLOCK() lck_mtx_unlock(& psx_shm_subsys_mutex)
#define PSHM_SUBSYS_ASSERT_HELD()  LCK_MTX_ASSERT(&psx_shm_subsys_mutex, LCK_MTX_ASSERT_OWNED)

  
                                                                   
   
static pshm_info_t *
pshm_cache_search(pshm_info_t *look)
{
	PSHM_SUBSYS_ASSERT_HELD();
	return RB_FIND(pshmhead, &pshm_head, look);
}

  
                                
   
static void
pshm_cache_add(pshm_info_t *entry)
{
	pshm_info_t *conflict;

	PSHM_SUBSYS_ASSERT_HELD();
	conflict = RB_INSERT(pshmhead, &pshm_head, entry);
	if (conflict != NULL) {
		panic("pshm_cache_add() found %p", conflict);
	}
	pshmnument++;
}

  
                                                  
   
static void
pshm_cache_delete(pshm_info_t *entry)
{
	PSHM_SUBSYS_ASSERT_HELD();
	assert(!(entry->pshm_flags & PSHM_REMOVED));
	RB_REMOVE(pshmhead, &pshm_head, entry);
	pshmnument--;
}

  
                                 
   
void
pshm_cache_init(void)
{
	RB_INIT(&pshm_head);
}

  
                                                                     
                                                                       
                                                                         
                                                          
   
int
pshm_cache_purge_all(__unused proc_t proc)
{
	pshm_info_t *p;
	pshm_info_t *tmp;
	int error = 0;

	if (kauth_cred_issuser(kauth_cred_get()) == 0) {
		return EPERM;
	}

	PSHM_SUBSYS_LOCK();
	RB_FOREACH_SAFE(p, pshmhead, &pshm_head, tmp) {
		error = pshm_unlink_internal(p);
		if (error) {                                                      
			goto out;
		}
	}
	assert(pshmnument == 0);

out:
	PSHM_SUBSYS_UNLOCK();

	if (error) {
		printf("%s: Error %d removing posix shm cache: %ld remain!\n",
		    __func__, error, pshmnument);
	}
	return error;
}

  
                                                           
                                                       
                                                                 
   
static int
pshm_get_name(pshm_info_t *pinfo, const user_addr_t user_addr)
{
	size_t bytes_copied = 0;
	int error;


	error = copyinstr(user_addr, &pinfo->pshm_hdr.pshm_name[0], PSHMNAMLEN + 1, &bytes_copied);
	if (error != 0) {
		return error;
	}
	assert(bytes_copied <= PSHMNAMLEN + 1);
	assert(pinfo->pshm_hdr.pshm_name[bytes_copied - 1] == 0);
	if (bytes_copied < 2) {                                                            
		return EINVAL;
	}
	AUDIT_ARG(text, &pinfo->pshm_hdr.pshm_name[0]);
	return 0;
}

  
                                    
   
int
shm_open(proc_t p, struct shm_open_args *uap, int32_t *retval)
{
	int             indx;
	int             error = 0;
	pshm_info_t     *pinfo = NULL;
	pshm_info_t     *new_pinfo = NULL;
	pshmnode_t      *new_pnode = NULL;
	struct fileproc *fp = NULL;
	int             fmode;
	mode_t          cmode = (mode_t)uap->mode;
	bool            incache = false;
	bool            have_label = false;

	AUDIT_ARG(fflags, uap->oflag);
	AUDIT_ARG(mode, cmode);

	  
                                                                      
                                                      
    
	new_pinfo = kheap_alloc(KM_SHM, sizeof(pshm_info_t), Z_WAITOK | Z_ZERO);
	if (new_pinfo == NULL) {
		error = ENOSPC;
		goto bad;
	}

	  
                           
    
	error = pshm_get_name(new_pinfo, uap->name);
	if (error != 0) {
		goto bad;
	}

	  
                                                                 
                           
    
	error = falloc(p, &fp, &indx, vfs_context_current());
	if (error) {
		goto bad;
	}

	cmode &= ALLPERMS;

	fmode = FFLAGS(uap->oflag);
	if ((fmode & (FREAD | FWRITE)) == 0) {
		error = EINVAL;
		goto bad;
	}

	  
                                              
    
	new_pnode = kheap_alloc(KM_SHM, sizeof(pshmnode_t),
	    Z_WAITOK | Z_ZERO);
	if (new_pnode == NULL) {
		error = ENOSPC;
		goto bad;
	}

	  
                                                       
                                                                                 
    
	if (fmode & O_CREAT) {
		new_pinfo->pshm_usecount = 2;                                             
		new_pinfo->pshm_length = 0;
		new_pinfo->pshm_mode = cmode;
		new_pinfo->pshm_uid = kauth_getuid();
		new_pinfo->pshm_gid = kauth_getgid();
		SLIST_INIT(&new_pinfo->pshm_mobjs);
#if CONFIG_MACF
		mac_posixshm_label_init(&new_pinfo->pshm_hdr);
		have_label = true;
		error = mac_posixshm_check_create(kauth_cred_get(), new_pinfo->pshm_hdr.pshm_name);
		if (error) {
			goto bad;
		}
#endif
	}

	  
                                                                         
                   
    
	PSHM_SUBSYS_LOCK();

	pinfo = pshm_cache_search(new_pinfo);
	if (pinfo != NULL) {
		incache = true;

		                                                     
		error = pshm_ref(pinfo);
		if (error) {
			pinfo = NULL;                                         
			goto bad_locked;
		}

		                                       
		if ((fmode & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
			error = EEXIST;
			goto bad_locked;
		}

		                                                       
		if ((fmode & O_TRUNC) &&
		    (pinfo->pshm_flags & (PSHM_ALLOCATING | PSHM_ALLOCATED))) {
			error = EINVAL;
			goto bad_locked;
		}
	} else {
		incache = false;

		                                           
		if (!(fmode & O_CREAT)) {
			error = ENOENT;
			goto bad_locked;
		}

		                                      
		pinfo = new_pinfo;
		pshm_cache_add(pinfo);
		new_pinfo = NULL;                                          
	}

	PSHM_SUBSYS_UNLOCK();

	  
                                                               
    
	if (incache) {
		if (fmode & O_CREAT) {
			AUDIT_ARG(posix_ipc_perm, pinfo->pshm_uid,
			    pinfo->pshm_gid, pinfo->pshm_mode);
		}
#if CONFIG_MACF
		if ((error = mac_posixshm_check_open(kauth_cred_get(), &pinfo->pshm_hdr, fmode))) {
			goto bad;
		}
#endif
		if ((error = pshm_access(pinfo, fmode, kauth_cred_get(), p))) {
			goto bad;
		}
	} else {
#if CONFIG_MACF
		mac_posixshm_label_associate(kauth_cred_get(), &pinfo->pshm_hdr, pinfo->pshm_hdr.pshm_name);
#endif
	}

	proc_fdlock(p);
	fp->f_flag = fmode & FMASK;
	fp->f_ops = &pshmops;
	new_pnode->pinfo = pinfo;
	fp->f_data = (caddr_t)new_pnode;
	*fdflags(p, indx) |= UF_EXCLOSE;
	procfdtbl_releasefd(p, indx, NULL);
	fp_drop(p, indx, fp, 1);
	proc_fdunlock(p);

	*retval = indx;
	error = 0;
	goto done;

bad_locked:
	PSHM_SUBSYS_UNLOCK();
bad:
	  
                                                                  
    
	if (incache && pinfo != NULL) {
		PSHM_SUBSYS_LOCK();
		pshm_deref(pinfo);
		PSHM_SUBSYS_UNLOCK();
	}

	  
                                                
    
	kheap_free(KM_SHM, new_pnode, sizeof(pshmnode_t));

	if (fp != NULL) {
		fp_free(p, indx, fp);
	}

done:
	if (new_pinfo != NULL) {
#if CONFIG_MACF
		if (have_label) {
			mac_posixshm_label_destroy(&new_pinfo->pshm_hdr);
		}
#endif
		kheap_free(KM_SHM, new_pinfo, sizeof(pshm_info_t));
	}
	return error;
}


  
                                                                        
                                                                                 
   
int
pshm_truncate(
	__unused proc_t       p,
	struct fileproc       *fp,
	__unused int          fd,
	off_t                 length,
	__unused int32_t      *retval)
{
	pshm_info_t           *pinfo;
	pshmnode_t            *pnode;
	kern_return_t         kret;
	mem_entry_name_port_t mem_object;
	mach_vm_size_t        total_size, alloc_size;
	memory_object_size_t  mosize;
	pshm_mobj_t           *pshmobj, *pshmobj_last;
	vm_map_t              user_map;
	int                   error;

	user_map = current_map();

	if (FILEGLOB_DTYPE(fp->fp_glob) != DTYPE_PSXSHM) {
		return EINVAL;
	}

#if 0
	  
                                                        
                                                         
    
	                                           
	if (!(fp->f_flag & FWRITE)) {
		return EINVAL;
	}
#endif

	PSHM_SUBSYS_LOCK();
	if (((pnode = (pshmnode_t *)fp->f_data)) == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	if ((pinfo = pnode->pinfo) == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	                                                                   
	if (pinfo->pshm_flags & (PSHM_ALLOCATING | PSHM_ALLOCATED)) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

#if CONFIG_MACF
	error = mac_posixshm_check_truncate(kauth_cred_get(), &pinfo->pshm_hdr, length);
	if (error) {
		PSHM_SUBSYS_UNLOCK();
		return error;
	}
#endif
	  
                                                                         
                                       
    
	error = pshm_ref(pinfo);
	if (error) {
		PSHM_SUBSYS_UNLOCK();
		return error;
	}

	                                                     
	pinfo->pshm_flags |= PSHM_ALLOCATING;
	total_size = vm_map_round_page(length, vm_map_page_mask(user_map));

	pshmobj_last = NULL;
	for (alloc_size = 0; alloc_size < total_size; alloc_size += mosize) {
		PSHM_SUBSYS_UNLOCK();

		                                                        
		mosize = MIN(total_size - alloc_size, ANON_MAX_SIZE);
		kret = mach_make_memory_entry_64(VM_MAP_NULL, &mosize, 0,
		    MAP_MEM_NAMED_CREATE | VM_PROT_DEFAULT, &mem_object, 0);

		if (kret != KERN_SUCCESS) {
			goto out;
		}

		                                                 
		pshmobj = kheap_alloc(KM_SHM, sizeof(pshm_mobj_t), Z_WAITOK);
		if (pshmobj == NULL) {
			kret = KERN_NO_SPACE;
			mach_memory_entry_port_release(mem_object);
			mem_object = NULL;
			goto out;
		}

		PSHM_SUBSYS_LOCK();

		                           
		pshmobj->pshmo_memobject = (void *)mem_object;
		pshmobj->pshmo_size = mosize;
		SLIST_NEXT(pshmobj, pshmo_next) = NULL;

		if (pshmobj_last == NULL) {
			SLIST_FIRST(&pinfo->pshm_mobjs) = pshmobj;
		} else {
			SLIST_INSERT_AFTER(pshmobj_last, pshmobj, pshmo_next);
		}
		pshmobj_last = pshmobj;
	}

	                                                            
	pinfo->pshm_flags |= PSHM_ALLOCATED;
	pinfo->pshm_flags &= ~(PSHM_ALLOCATING);
	pinfo->pshm_length = total_size;
	pshm_deref(pinfo);                                                   
	PSHM_SUBSYS_UNLOCK();
	return 0;

out:
	                                              
	PSHM_SUBSYS_LOCK();
	while ((pshmobj = SLIST_FIRST(&pinfo->pshm_mobjs)) != NULL) {
		SLIST_REMOVE_HEAD(&pinfo->pshm_mobjs, pshmo_next);
		PSHM_SUBSYS_UNLOCK();
		mach_memory_entry_port_release(pshmobj->pshmo_memobject);
		kheap_free(KM_SHM, pshmobj, sizeof(pshm_mobj_t));
		PSHM_SUBSYS_LOCK();
	}
	pinfo->pshm_flags &= ~PSHM_ALLOCATING;
	pshm_deref(pinfo);                                                   
	PSHM_SUBSYS_UNLOCK();

	switch (kret) {
	case KERN_INVALID_ADDRESS:
	case KERN_NO_SPACE:
		return ENOMEM;
	case KERN_PROTECTION_FAILURE:
		return EACCES;
	default:
		return EINVAL;
	}
}

int
pshm_stat(pshmnode_t *pnode, void *ub, int isstat64)
{
	struct stat *sb = (struct stat *)0;                                                    
	struct stat64 * sb64 = (struct stat64 *)0;                                                 
	pshm_info_t *pinfo;
#if CONFIG_MACF
	int error;
#endif

	PSHM_SUBSYS_LOCK();
	if ((pinfo = pnode->pinfo) == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

#if CONFIG_MACF
	error = mac_posixshm_check_stat(kauth_cred_get(), &pinfo->pshm_hdr);
	if (error) {
		PSHM_SUBSYS_UNLOCK();
		return error;
	}
#endif

	if (isstat64 != 0) {
		sb64 = (struct stat64 *)ub;
		bzero(sb64, sizeof(struct stat64));
		sb64->st_mode = pinfo->pshm_mode;
		sb64->st_uid = pinfo->pshm_uid;
		sb64->st_gid = pinfo->pshm_gid;
		sb64->st_size = pinfo->pshm_length;
	} else {
		sb = (struct stat *)ub;
		bzero(sb, sizeof(struct stat));
		sb->st_mode = pinfo->pshm_mode;
		sb->st_uid = pinfo->pshm_uid;
		sb->st_gid = pinfo->pshm_gid;
		sb->st_size = pinfo->pshm_length;
	}
	PSHM_SUBSYS_UNLOCK();

	return 0;
}

  
                                           
   
static int
pshm_access(pshm_info_t *pinfo, int mode, kauth_cred_t cred, __unused proc_t p)
{
	mode_t mode_req = ((mode & FREAD) ? S_IRUSR : 0) |
	    ((mode & FWRITE) ? S_IWUSR : 0);

	                                              
	if (!suser(cred, NULL)) {
		return 0;
	}

	return posix_cred_access(cred, pinfo->pshm_uid, pinfo->pshm_gid, pinfo->pshm_mode, mode_req);
}

int
pshm_mmap(
	__unused proc_t    p,
	struct mmap_args   *uap,
	user_addr_t        *retval,
	struct fileproc    *fp,
	off_t              pageoff)
{
	vm_map_offset_t    user_addr = (vm_map_offset_t)uap->addr;
	vm_map_size_t      user_size = (vm_map_size_t)uap->len;
	vm_map_offset_t    user_start_addr;
	vm_map_size_t      map_size, mapped_size;
	int                prot = uap->prot;
	int                max_prot = VM_PROT_DEFAULT;
	int                flags = uap->flags;
	vm_object_offset_t file_pos = (vm_object_offset_t)uap->pos;
	vm_object_offset_t map_pos;
	vm_map_t           user_map;
	int                alloc_flags;
	vm_map_kernel_flags_t vmk_flags;
	bool               docow;
	kern_return_t      kret = KERN_SUCCESS;
	pshm_info_t        *pinfo;
	pshmnode_t         *pnode;
	pshm_mobj_t        *pshmobj;
	int                error;

	if (user_size == 0) {
		return 0;
	}

	if (!(flags & MAP_SHARED)) {
		return EINVAL;
	}

	                                                                       
	if (!(fp->f_flag & FWRITE)) {
		if (prot & VM_PROT_WRITE) {
			return EPERM;
		}
		max_prot &= ~VM_PROT_WRITE;
	}

	PSHM_SUBSYS_LOCK();
	pnode = (pshmnode_t *)fp->f_data;
	if (pnode == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	pinfo = pnode->pinfo;
	if (pinfo == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	if (!(pinfo->pshm_flags & PSHM_ALLOCATED)) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	if (user_size > (vm_map_size_t)pinfo->pshm_length) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	vm_map_size_t end_pos = 0;
	if (os_add_overflow(user_size, file_pos, &end_pos)) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}
	if (end_pos > (vm_map_size_t)pinfo->pshm_length) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	pshmobj = SLIST_FIRST(&pinfo->pshm_mobjs);
	if (pshmobj == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

#if CONFIG_MACF
	error = mac_posixshm_check_mmap(kauth_cred_get(), &pinfo->pshm_hdr, prot, flags);
	if (error) {
		PSHM_SUBSYS_UNLOCK();
		return error;
	}
#endif
	                                                                     
	error = pshm_ref(pinfo);
	if (error) {
		PSHM_SUBSYS_UNLOCK();
		return error;
	}

	PSHM_SUBSYS_UNLOCK();
	user_map = current_map();

	if (!(flags & MAP_FIXED)) {
		alloc_flags = VM_FLAGS_ANYWHERE;
		user_addr = vm_map_round_page(user_addr,
		    vm_map_page_mask(user_map));
	} else {
		if (user_addr != vm_map_round_page(user_addr,
		    vm_map_page_mask(user_map))) {
			error = EINVAL;
			goto out_deref;
		}

		  
                                                            
                                                           
                                                         
                              
     
		alloc_flags = VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE;
	}
	docow = false;

	mapped_size = 0;
	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	                                       
	kret = vm_map_enter_mem_object(user_map,
	    &user_addr,
	    user_size,
	    0,
	    alloc_flags,
	    vmk_flags,
	    VM_KERN_MEMORY_NONE,
	    IPC_PORT_NULL,
	    0,
	    false,
	    VM_PROT_NONE,
	    VM_PROT_NONE,
	    VM_INHERIT_NONE);
	user_start_addr = user_addr;
	if (kret != KERN_SUCCESS) {
		goto out_deref;
	}

	                                           
	for (map_pos = 0, pshmobj = SLIST_FIRST(&pinfo->pshm_mobjs);
	    user_size != 0;
	    map_pos += pshmobj->pshmo_size, pshmobj = SLIST_NEXT(pshmobj, pshmo_next)) {
		if (pshmobj == NULL) {
			                             
			goto out_deref;
		}
		if (file_pos >= map_pos + pshmobj->pshmo_size) {
			continue;
		}
		map_size = (vm_map_size_t)(pshmobj->pshmo_size - (file_pos - map_pos));
		if (map_size > user_size) {
			map_size = user_size;
		}
		vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
		kret = vm_map_enter_mem_object(
			user_map,
			&user_addr,
			map_size,
			0,
			VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
			vmk_flags,
			VM_KERN_MEMORY_NONE,
			pshmobj->pshmo_memobject,
			file_pos - map_pos,
			docow,
			prot,
			max_prot,
			VM_INHERIT_SHARE);
		if (kret != KERN_SUCCESS) {
			goto out_deref;
		}

		user_addr += map_size;
		user_size -= map_size;
		mapped_size += map_size;
		file_pos += map_size;
	}

	PSHM_SUBSYS_LOCK();
	pnode->mapp_addr = user_start_addr;
	pinfo->pshm_flags |= (PSHM_MAPPED | PSHM_INUSE);
	PSHM_SUBSYS_UNLOCK();
out_deref:
	PSHM_SUBSYS_LOCK();
	pshm_deref(pinfo);                                                          
	PSHM_SUBSYS_UNLOCK();
	if (kret != KERN_SUCCESS) {
		if (mapped_size != 0) {
			(void) mach_vm_deallocate(current_map(),
			    user_start_addr,
			    mapped_size);
		}
	}

	switch (kret) {
	case KERN_SUCCESS:
		*retval = (user_addr_t)(user_start_addr + pageoff);
		return 0;
	case KERN_INVALID_ADDRESS:
	case KERN_NO_SPACE:
		return ENOMEM;
	case KERN_PROTECTION_FAILURE:
		return EACCES;
	default:
		return EINVAL;
	}
}

  
                                                                 
   
static int
pshm_unlink_internal(pshm_info_t *pinfo)
{
	PSHM_SUBSYS_ASSERT_HELD();

	if (pinfo == NULL) {
		return EINVAL;
	}

	pshm_cache_delete(pinfo);
	pinfo->pshm_flags |= PSHM_REMOVED;

	                                    
	pshm_deref(pinfo);

	return 0;
}

int
shm_unlink(proc_t p, struct shm_unlink_args *uap, __unused int32_t *retval)
{
	int         error = 0;
	pshm_info_t *pinfo = NULL;
	pshm_info_t *name_pinfo = NULL;

	  
                                
    
	name_pinfo = kheap_alloc(KHEAP_TEMP, sizeof(pshm_info_t),
	    Z_WAITOK | Z_ZERO);
	if (name_pinfo == NULL) {
		error = ENOSPC;
		goto bad;
	}
	error = pshm_get_name(name_pinfo, uap->name);
	if (error != 0) {
		error = EINVAL;
		goto bad;
	}

	PSHM_SUBSYS_LOCK();
	pinfo = pshm_cache_search(name_pinfo);

	if (pinfo == NULL) {
		error = ENOENT;
		goto bad_unlock;
	}

#if CONFIG_MACF
	error = mac_posixshm_check_unlink(kauth_cred_get(), &pinfo->pshm_hdr, name_pinfo->pshm_hdr.pshm_name);
	if (error) {
		goto bad_unlock;
	}
#endif

	AUDIT_ARG(posix_ipc_perm, pinfo->pshm_uid, pinfo->pshm_gid, pinfo->pshm_mode);

	  
                                                               
                                                                   
                                                                 
                                                                    
                                
    
	error = pshm_access(pinfo, FWRITE, kauth_cred_get(), p);
	if (error != 0 && pinfo->pshm_uid != kauth_getuid()) {
		goto bad_unlock;
	}

	error = pshm_unlink_internal(pinfo);
bad_unlock:
	PSHM_SUBSYS_UNLOCK();
bad:
	kheap_free(KHEAP_TEMP, name_pinfo, sizeof(pshm_info_t));
	return error;
}

  
                                                 
                                                   
   
static int
pshm_ref(pshm_info_t *pinfo)
{
	PSHM_SUBSYS_ASSERT_HELD();

	if (pinfo->pshm_usecount == PSHM_MAXCOUNT) {
		return EMFILE;
	}
	pinfo->pshm_usecount++;
	return 0;
}

  
                                                  
                                      
   
static void
pshm_deref(pshm_info_t *pinfo)
{
	pshm_mobj_t *pshmobj;

	PSHM_SUBSYS_ASSERT_HELD();
	if (pinfo->pshm_usecount == 0) {
		panic("negative usecount in pshm_close\n");
	}
	pinfo->pshm_usecount--;                                  

	if (pinfo->pshm_usecount == 0) {
#if CONFIG_MACF
		mac_posixshm_label_destroy(&pinfo->pshm_hdr);
#endif
		PSHM_SUBSYS_UNLOCK();

		  
                                               
     
		while ((pshmobj = SLIST_FIRST(&pinfo->pshm_mobjs)) != NULL) {
			SLIST_REMOVE_HEAD(&pinfo->pshm_mobjs, pshmo_next);
			mach_memory_entry_port_release(pshmobj->pshmo_memobject);
			kheap_free(KM_SHM, pshmobj, sizeof(pshm_mobj_t));
		}

		                           
		kheap_free(KM_SHM, pinfo, sizeof(pshm_info_t));

		PSHM_SUBSYS_LOCK();
	}
}

                                                                
static int
pshm_closefile(struct fileglob *fg, __unused vfs_context_t ctx)
{
	int        error = EINVAL;
	pshmnode_t *pnode;

	PSHM_SUBSYS_LOCK();

	pnode = (pshmnode_t *)fg->fg_data;
	if (pnode != NULL) {
		error = 0;
		fg->fg_data = NULL;                                                    
		if (pnode->pinfo != NULL) {
			pshm_deref(pnode->pinfo);
			pnode->pinfo = NULL;
		}
	}

	PSHM_SUBSYS_UNLOCK();
	kheap_free(KM_SHM, pnode, sizeof(pshmnode_t));

	return error;
}

int
fill_pshminfo(pshmnode_t * pshm, struct pshm_info * info)
{
	pshm_info_t *pinfo;
	struct vinfo_stat *sb;

	PSHM_SUBSYS_LOCK();
	if ((pinfo = pshm->pinfo) == NULL) {
		PSHM_SUBSYS_UNLOCK();
		return EINVAL;
	}

	sb = &info->pshm_stat;

	bzero(sb, sizeof(struct vinfo_stat));
	sb->vst_mode = pinfo->pshm_mode;
	sb->vst_uid = pinfo->pshm_uid;
	sb->vst_gid = pinfo->pshm_gid;
	sb->vst_size = pinfo->pshm_length;

	info->pshm_mappaddr = pshm->mapp_addr;
	bcopy(&pinfo->pshm_hdr.pshm_name[0], &info->pshm_name[0], PSHMNAMLEN + 1);

	PSHM_SUBSYS_UNLOCK();
	return 0;
}

#if CONFIG_MACF
void
pshm_label_associate(struct fileproc *fp, struct vnode *vp, vfs_context_t ctx)
{
	pshmnode_t *pnode;
	pshm_info_t *pshm;

	PSHM_SUBSYS_LOCK();
	pnode = (pshmnode_t *)fp->f_data;
	if (pnode != NULL) {
		pshm = pnode->pinfo;
		if (pshm != NULL) {
			mac_posixshm_vnode_label_associate(
				vfs_context_ucred(ctx), &pshm->pshm_hdr, pshm->pshm_label,
				vp, vp->v_label);
		}
	}
	PSHM_SUBSYS_UNLOCK();
}
#endif
