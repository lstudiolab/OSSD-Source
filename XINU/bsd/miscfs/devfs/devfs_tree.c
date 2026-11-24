/*
 * Copyright (c) 2000-2014 Apple Inc. All rights reserved.
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

/*
 * Copyright 1997,1998 Julian Elischer.  All rights reserved.
 * julian@freebsd.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * devfs_tree.c
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
          
                                                                   
                                             
                                                           
                                                       
                                                  
                                                
                                                                         
                                                                   
                                                                     
                                                                        
                                                                       
                                                                 
                                                                     
                                      
                                                                      
               
   

                                                                              
                                                                         
                                                                           
                                                                                
                     

#define SPLIT_DEVS 1                                
                        

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mount_internal.h>
#include <sys/proc.h>
#include <sys/vnode_internal.h>
#include <stdarg.h>
#include <libkern/OSAtomic.h>
#include <os/refcnt.h>
#define BSD_KERNEL_PRIVATE      1                                        
#include "devfs.h"
#include "devfsdefs.h"

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#if FDESC
#include "fdesc.h"
#endif

typedef struct devfs_vnode_event {
	vnode_t                 dve_vp;
	uint32_t                dve_vid;
	uint32_t                dve_events;
} *devfs_vnode_event_t;

  
                                                          
                                                             
   
#define NUM_STACK_ENTRIES 5

typedef struct devfs_event_log {
	size_t                  del_max;
	size_t                  del_used;
	devfs_vnode_event_t     del_entries;
} *devfs_event_log_t;


static void     dev_free_hier(devdirent_t *);
static int      devfs_propogate(devdirent_t *, devdirent_t *, devfs_event_log_t);
static int      dev_finddir(const char *, devnode_t *, int, devnode_t **, devfs_event_log_t);
static int      dev_dup_entry(devnode_t *, devdirent_t *, devdirent_t **, struct devfsmount *);
void            devfs_ref_node(devnode_t *);
void            devfs_rele_node(devnode_t *);
static void     devfs_record_event(devfs_event_log_t, devnode_t*, uint32_t);
static int      devfs_init_event_log(devfs_event_log_t, uint32_t, devfs_vnode_event_t);
static void     devfs_release_event_log(devfs_event_log_t, int);
static void     devfs_bulk_notify(devfs_event_log_t);
static devdirent_t *devfs_make_node_internal(dev_t, devfstype_t type, uid_t, gid_t, int,
    int (*clone)(dev_t dev, int action), const char *fmt, va_list ap);


static LCK_GRP_DECLARE(devfs_lck_grp, "devfs_lock");
LCK_MTX_DECLARE(devfs_mutex, &devfs_lck_grp);
LCK_MTX_DECLARE(devfs_attr_mutex, &devfs_lck_grp);

os_refgrp_decl(static, devfs_refgrp, "devfs", NULL);

devdirent_t *           dev_root = NULL;                                  
struct devfs_stats      devfs_stats;                            

static ino_t            devfs_unique_fileno = 0;

#ifdef HIDDEN_MOUNTPOINT
static struct mount *devfs_hidden_mount;
#endif                        

static int devfs_ready = 0;
static uint32_t devfs_nmountplanes = 0;                                              

#define DEVFS_NOCREATE  FALSE
#define DEVFS_CREATE    TRUE

  
                                                      
                                                    
                                                      
                                                           
                   
                                                                 
                                                                   
   
         
int
devfs_sinit(void)
{
	int error;

	DEVFS_LOCK();
	error = dev_add_entry("root", NULL, DEV_DIR, NULL, NULL, NULL, &dev_root);
	DEVFS_UNLOCK();

	if (error) {
		printf("devfs_sinit: dev_add_entry failed ");
		return ENOTSUP;
	}
#ifdef HIDDEN_MOUNTPOINT
	devfs_hidden_mount = zalloc_flags(mount_zone, Z_WAITOK | Z_ZERO);
	mount_lock_init(devfs_hidden_mount);
	TAILQ_INIT(&devfs_hidden_mount->mnt_vnodelist);
	TAILQ_INIT(&devfs_hidden_mount->mnt_workerqueue);
	TAILQ_INIT(&devfs_hidden_mount->mnt_newvnodes);
#if CONFIG_MACF
	mac_mount_label_init(devfs_hidden_mount);
	mac_mount_label_associate(vfs_context_kernel(), devfs_hidden_mount);
#endif

	                                           
	mp->mnt_maxreadcnt = mp->mnt_maxwritecnt = MAXPHYS;
	mp->mnt_segreadcnt = mp->mnt_segwritecnt = 32;
	mp->mnt_ioflags = 0;
	mp->mnt_realrootvp = NULLVP;
	mp->mnt_authcache_ttl = CACHED_LOOKUP_RIGHT_TTL;

	devfs_mount(devfs_hidden_mount, "dummy", NULL, NULL, NULL);
	dev_root->de_dnp->dn_dvm
	        = (struct devfsmount *)devfs_hidden_mount->mnt_data;
#endif                        
#if CONFIG_MACF
	mac_devfs_label_associate_directory("/", (int) strlen("/"),
	    dev_root->de_dnp, "/");
#endif
	devfs_ready = 1;
	return 0;
}

                                                                         
                                                                         
                                                         
                                                                         
                                                                         



                                                                
                                                      
                                       
 
                             
                                                                
devdirent_t *
dev_findname(devnode_t * dir, const char *name)
{
	devdirent_t * newfp;
	if (dir->dn_type != DEV_DIR) {
		return 0;                                         
	}
	if (name[0] == '.') {
		if (name[1] == 0) {
			return dir->dn_typeinfo.Dir.myname;
		}
		if ((name[1] == '.') && (name[2] == 0)) {
			                       
			return dir->dn_typeinfo.Dir.parent->dn_typeinfo.Dir.myname;
		}
	}
	newfp = dir->dn_typeinfo.Dir.dirlist;

	while (newfp) {
		if (!(strncmp(name, newfp->de_name, sizeof(newfp->de_name)))) {
			return newfp;
		}
		newfp = newfp->de_next;
	}
	return NULL;
}

                                                                        
                                                                    
                                                                             
                                                                      
                                    
                                                                 
 
                             
                                                                        
static int
dev_finddir(const char * path,
    devnode_t * dirnode,
    int create,
    devnode_t * * dn_pp,
    devfs_event_log_t delp)
{
	devnode_t *     dnp = NULL;
	int             error = 0;
	const char *            scan;
#if CONFIG_MACF
	char            fullpath[DEVMAXPATHSIZE];
#endif


	if (!dirnode) {                                          
		dirnode = dev_root->de_dnp;
	}

	if (dirnode->dn_type != DEV_DIR) {
		return ENOTDIR;
	}

	if (strlen(path) > (DEVMAXPATHSIZE - 1)) {
		return ENAMETOOLONG;
	}

#if CONFIG_MACF
	strlcpy(fullpath, path, DEVMAXPATHSIZE);
#endif
	scan = path;

	while (*scan == '/') {
		scan++;
	}

	*dn_pp = NULL;

	while (1) {
		char                component[DEVMAXPATHSIZE];
		devdirent_t *       dirent_p;
		const char *        start;

		if (*scan == 0) {
			                                              
			*dn_pp = dirnode;
			break;
		}
		start = scan;
		while (*scan != '/' && *scan) {
			scan++;
		}

		strlcpy(component, start, (scan - start) + 1);
		if (*scan == '/') {
			scan++;
		}

		dirent_p = dev_findname(dirnode, component);
		if (dirent_p) {
			dnp = dirent_p->de_dnp;
			if (dnp->dn_type != DEV_DIR) {
				error = ENOTDIR;
				break;
			}
		} else {
			if (!create) {
				error = ENOENT;
				break;
			}
			error = dev_add_entry(component, dirnode,
			    DEV_DIR, NULL, NULL, NULL, &dirent_p);
			if (error) {
				break;
			}
			dnp = dirent_p->de_dnp;
#if CONFIG_MACF
			mac_devfs_label_associate_directory(
				dirnode->dn_typeinfo.Dir.myname->de_name,
				(int) strlen(dirnode->dn_typeinfo.Dir.myname->de_name),
				dnp, fullpath);
#endif
			devfs_propogate(dirnode->dn_typeinfo.Dir.myname, dirent_p, delp);
		}
		dirnode = dnp;                                          
	}
	return error;
}


                                                                        
                                     
                                                     
                                                                  
 
                                                        
 
                             
                                                                        
int
dev_add_name(const char * name, devnode_t * dirnode, __unused devdirent_t * back,
    devnode_t * dnp, devdirent_t * *dirent_pp)
{
	devdirent_t *   dirent_p = NULL;

	if (dirnode != NULL) {
		if (dirnode->dn_type != DEV_DIR) {
			return ENOTDIR;
		}

		if (dev_findname(dirnode, name)) {
			return EEXIST;
		}
	}
	  
                               
                                           
    
	if (!name || (strlen(name) > (DEVMAXNAMESIZE - 1))) {
		return ENAMETOOLONG;
	}

	  
                                               
    
	MALLOC(dirent_p, devdirent_t *, sizeof(devdirent_t),
	    M_DEVFSNAME, M_WAITOK);
	if (!dirent_p) {
		return ENOMEM;
	}
	bzero(dirent_p, sizeof(devdirent_t));

	                                              
	                      
	if (dirnode && (dnp->dn_dvm == NULL)) {
		dnp->dn_dvm = dirnode->dn_dvm;
		                                                      
	}

	  
                         
                                                                    
                                                      
    
	dirent_p->de_dnp = dnp;
	dnp->dn_links++;                                       

	  
                                                                  
                                                           
    
	if (dnp->dn_linklist) {
		dirent_p->de_nextlink = dnp->dn_linklist;
		dirent_p->de_prevlinkp = dirent_p->de_nextlink->de_prevlinkp;
		dirent_p->de_nextlink->de_prevlinkp = &(dirent_p->de_nextlink);
		*dirent_p->de_prevlinkp = dirent_p;
	} else {
		dirent_p->de_nextlink = dirent_p;
		dirent_p->de_prevlinkp = &(dirent_p->de_nextlink);
	}
	dnp->dn_linklist = dirent_p;

	  
                                                          
                            
                                                              
    
	if (dnp->dn_type == DEV_DIR) {
		dnp->dn_typeinfo.Dir.myname = dirent_p;
		  
                                                             
                                   
                                                    
                                          
     
		if (dnp->dn_typeinfo.Dir.parent) {
			dnp->dn_typeinfo.Dir.parent->dn_links--;
		}
		if (dirnode) {
			dnp->dn_typeinfo.Dir.parent = dirnode;
		} else {
			dnp->dn_typeinfo.Dir.parent = dnp;
		}
		dnp->dn_typeinfo.Dir.parent->dn_links++;                               
	}

	  
                                          
    
	strlcpy(dirent_p->de_name, name, DEVMAXNAMESIZE);


	  
                                            
                      
    
	if (dirnode) {
		  
                                                              
     
		dirent_p->de_parent = dirnode;                    
		dirent_p->de_prevp = dirnode->dn_typeinfo.Dir.dirlast;
		dirent_p->de_next = *(dirent_p->de_prevp);                     
		                                                     
		*(dirent_p->de_prevp) = dirent_p;
		dirnode->dn_typeinfo.Dir.dirlast = &(dirent_p->de_next);
		dirnode->dn_typeinfo.Dir.entrycount++;
		dirnode->dn_len += strlen(name) + 8;           
	}

	*dirent_pp = dirent_p;
	DEVFS_INCR_ENTRIES();
	return 0;
}


                                                                        
                                       
 
                                                                     
                                                             
                                                                       
                                                                  
                                                    
                                               
                                        
                                            
                          
                                                                
 
                             
                                                                        
int
dev_add_node(int entrytype, devnode_type_t * typeinfo, devnode_t * proto,
    devnode_t * *dn_pp, struct devfsmount *dvm)
{
	devnode_t *     dnp = NULL;
	int     error = 0;

#if defined SPLIT_DEVS
	  
                                                                    
                                                                
    
	if (proto) {
		dnp = proto->dn_nextsibling;
		while (dnp != proto) {
			if (dnp->dn_dvm == dvm) {
				*dn_pp = dnp;
				return 0;
			}
			dnp = dnp->dn_nextsibling;
		}
		if (typeinfo == NULL) {
			typeinfo = &(proto->dn_typeinfo);
		}
	}
#else                   
	if (proto) {
		switch (proto->type) {
		case DEV_BDEV:
		case DEV_CDEV:
			*dn_pp = proto;
			return 0;
		}
	}
#endif                  
	MALLOC(dnp, devnode_t *, sizeof(devnode_t), M_DEVFSNODE, M_WAITOK);
	if (!dnp) {
		return ENOMEM;
	}

	  
                                                               
                                                                       
    
	if (proto) {
		bcopy(proto, dnp, sizeof(devnode_t));
		dnp->dn_links = 0;
		dnp->dn_linklist = NULL;
		dnp->dn_vn = NULL;
		dnp->dn_len = 0;
		                                 
		dnp->dn_prevsiblingp = proto->dn_prevsiblingp;
		*(dnp->dn_prevsiblingp) = dnp;
		dnp->dn_nextsibling = proto;
		proto->dn_prevsiblingp = &(dnp->dn_nextsibling);
#if CONFIG_MACF
		mac_devfs_label_init(dnp);
		mac_devfs_label_copy(proto->dn_label, dnp->dn_label);
#endif
	} else {
		struct timeval tv;

		  
                                                          
     
		microtime(&tv);
		bzero(dnp, sizeof(devnode_t));
		dnp->dn_type = entrytype;
		dnp->dn_nextsibling = dnp;
		dnp->dn_prevsiblingp = &(dnp->dn_nextsibling);
		dnp->dn_atime.tv_sec = tv.tv_sec;
		dnp->dn_mtime.tv_sec = tv.tv_sec;
		dnp->dn_ctime.tv_sec = tv.tv_sec;
#if CONFIG_MACF
		mac_devfs_label_init(dnp);
#endif
	}
	dnp->dn_dvm = dvm;

	                                                                                
	os_ref_init_raw(&dnp->dn_refcount, &devfs_refgrp);
	dnp->dn_ino = devfs_unique_fileno;
	devfs_unique_fileno++;

	  
                                           
    
	switch (entrytype) {
	case DEV_DIR:
		  
                                   
                               
     
		dnp->dn_typeinfo.Dir.dirlast = &(dnp->dn_typeinfo.Dir.dirlist);
		dnp->dn_typeinfo.Dir.dirlist = (devdirent_t *)0;
		dnp->dn_typeinfo.Dir.entrycount = 0;
		                                                        
		dnp->dn_typeinfo.Dir.parent = NULL;
		dnp->dn_links++;           
		dnp->dn_typeinfo.Dir.myname = NULL;
		  
                                                          
                                             
     
		dnp->dn_ops = &devfs_vnodeop_p;
		dnp->dn_mode |= 0555;                      
		break;
	case DEV_SLNK:
		  
                                                       
                                                      
                                                        
                                                              
                                                       
                                                      
     
		MALLOC(dnp->dn_typeinfo.Slnk.name, char *,
		    typeinfo->Slnk.namelen + 1,
		    M_DEVFSNODE, M_WAITOK);
		if (!dnp->dn_typeinfo.Slnk.name) {
			error = ENOMEM;
			break;
		}
		strlcpy(dnp->dn_typeinfo.Slnk.name, typeinfo->Slnk.name,
		    typeinfo->Slnk.namelen + 1);
		dnp->dn_typeinfo.Slnk.namelen = typeinfo->Slnk.namelen;
		DEVFS_INCR_STRINGSPACE(dnp->dn_typeinfo.Slnk.namelen + 1);
		dnp->dn_ops = &devfs_vnodeop_p;
		dnp->dn_mode |= 0555;                      
		break;
	case DEV_CDEV:
	case DEV_BDEV:
		  
                                     
                                           
     
		dnp->dn_ops = &devfs_spec_vnodeop_p;
		dnp->dn_typeinfo.dev = typeinfo->dev;
		break;

	#if FDESC
	                        
	case DEV_DEVFD:
		dnp->dn_ops = &devfs_devfd_vnodeop_p;
		dnp->dn_mode |= 0555;                      
		break;

	#endif            
	default:
		error = EINVAL;
	}

	if (error) {
		FREE(dnp, M_DEVFSNODE);
	} else {
		*dn_pp = dnp;
		DEVFS_INCR_NODES();
	}

	return error;
}


                                                                        
                              
                                                                        
void
devnode_free(devnode_t * dnp)
{
#if CONFIG_MACF
	mac_devfs_label_destroy(dnp);
#endif
	if (dnp->dn_type == DEV_SLNK) {
		DEVFS_DECR_STRINGSPACE(dnp->dn_typeinfo.Slnk.namelen + 1);
		FREE(dnp->dn_typeinfo.Slnk.name, M_DEVFSNODE);
	}
	DEVFS_DECR_NODES();
	FREE(dnp, M_DEVFSNODE);
}


                                                                        
                              
                                                                        
static void
devfs_dn_free(devnode_t * dnp)
{
	if (--dnp->dn_links <= 0) {                                           
		                                           
		if (dnp->dn_nextsibling != dnp) {
			devnode_t * *   prevp = dnp->dn_prevsiblingp;
			*prevp = dnp->dn_nextsibling;
			dnp->dn_nextsibling->dn_prevsiblingp = prevp;
		}

		                                                                                              
		os_ref_count_t rc = os_ref_get_count_raw(&dnp->dn_refcount);
		if (rc == 1) {
			                                               
			(void) os_ref_release_locked_raw(&dnp->dn_refcount, &devfs_refgrp);
			devnode_free(dnp);
		} else {
			dnp->dn_lflags |= DN_DELETE;
		}
	}
}

                                                                         
                              
                                           
                                                                         


                                                                        
                                                                   
                                                                
                                      
 
                                                                      
                                                                     
                                            
 
                                                                    
 
                             
                                                                        
static int
devfs_propogate(devdirent_t * parent, devdirent_t * child, devfs_event_log_t delp)
{
	int     error;
	devdirent_t * newnmp;
	devnode_t *     dnp = child->de_dnp;
	devnode_t *     pdnp = parent->de_dnp;
	devnode_t *     adnp = parent->de_dnp;
	int type = child->de_dnp->dn_type;
	uint32_t events;

	events = (dnp->dn_type == DEV_DIR ? VNODE_EVENT_DIR_CREATED : VNODE_EVENT_FILE_CREATED);
	if (delp != NULL) {
		devfs_record_event(delp, pdnp, events);
	}

	                                                
                                              
                                                 
	for (adnp = pdnp->dn_nextsibling;
	    adnp != pdnp;
	    adnp = adnp->dn_nextsibling) {
		  
                                                      
                                                         
              
     
		if ((error = dev_add_entry(child->de_name, adnp, type,
		    NULL, dnp, adnp->dn_dvm,
		    &newnmp)) != 0) {
			printf("duplicating %s failed\n", child->de_name);
		} else {
			if (delp != NULL) {
				devfs_record_event(delp, adnp, events);

				  
                                                         
                                                                 
                                                                    
                                                                    
                                                            
                                                            
                  
       
				devfs_record_event(delp, newnmp->de_dnp, VNODE_EVENT_LINK);
			}
		}
	}
	return 0;                                   
}

static uint32_t
remove_notify_count(devnode_t *dnp)
{
	uint32_t notify_count = 0;
	devnode_t *dnp2;

	  
                                                               
                                  
    
	notify_count = devfs_nmountplanes;
	notify_count += dnp->dn_links;
	for (dnp2 = dnp->dn_nextsibling; dnp2 != dnp; dnp2 = dnp2->dn_nextsibling) {
		notify_count += dnp2->dn_links;
	}

	return notify_count;
}

                                                                        
                                                               
                                                                  
                                                                     
                                                                         
 
                                                                    
                                                                         
                                         
                                                                        
void
devfs_remove(void *dirent_p)
{
	devnode_t * dnp = ((devdirent_t *)dirent_p)->de_dnp;
	devnode_t * dnp2;
	boolean_t   lastlink;
	struct devfs_event_log event_log;
	uint32_t    log_count = 0;
	int         do_notify = 0;
	int         need_free = 0;
	struct devfs_vnode_event stackbuf[NUM_STACK_ENTRIES];

	DEVFS_LOCK();

	if (!devfs_ready) {
		printf("devfs_remove: not ready for devices!\n");
		goto out;
	}

	log_count = remove_notify_count(dnp);

	if (log_count > NUM_STACK_ENTRIES) {
		uint32_t new_count;
wrongsize:
		DEVFS_UNLOCK();
		if (devfs_init_event_log(&event_log, log_count, NULL) == 0) {
			do_notify = 1;
			need_free = 1;
		}
		DEVFS_LOCK();

		new_count = remove_notify_count(dnp);
		if (need_free && (new_count > log_count)) {
			devfs_release_event_log(&event_log, 1);
			need_free = 0;
			do_notify = 0;
			log_count = log_count * 2;
			goto wrongsize;
		}
	} else {
		if (devfs_init_event_log(&event_log, NUM_STACK_ENTRIES, &stackbuf[0]) == 0) {
			do_notify = 1;
		}
	}

	                                
	if (do_notify != 0) {
		devfs_record_event(&event_log, dnp, VNODE_EVENT_DELETE);
	}

	                                                        
	while ((dnp2 = dnp->dn_nextsibling) != dnp) {
		  
                                                         
     
		dnp->dn_nextsibling = dnp2->dn_nextsibling;
		dnp->dn_nextsibling->dn_prevsiblingp = &(dnp->dn_nextsibling);
		dnp2->dn_nextsibling = dnp2;
		dnp2->dn_prevsiblingp = &(dnp2->dn_nextsibling);

		                                              
		if (do_notify != 0) {
			devfs_record_event(&event_log, dnp2, VNODE_EVENT_DELETE);
		}

		if (dnp2->dn_linklist) {
			do {
				lastlink = (1 == dnp2->dn_links);
				                                                                       
				if (do_notify != 0) {
					devfs_record_event(&event_log, dnp2->dn_linklist->de_parent, VNODE_EVENT_FILE_REMOVED);
				}
				dev_free_name(dnp2->dn_linklist);
			} while (!lastlink);
		}
	}

	  
                           
                                                  
                                                  
    
	if (dnp->dn_linklist) {
		do {
			lastlink = (1 == dnp->dn_links);
			                                                         
			if (do_notify != 0) {
				devfs_record_event(&event_log, dnp->dn_linklist->de_parent, VNODE_EVENT_FILE_REMOVED);
			}
			dev_free_name(dnp->dn_linklist);
		} while (!lastlink);
	}
out:
	DEVFS_UNLOCK();
	if (do_notify != 0) {
		devfs_bulk_notify(&event_log);
		devfs_release_event_log(&event_log, need_free);
	}

	return;
}



                                                                
                                                               
                                                
                                                   
                
  
                              
                                                                
int
dev_dup_plane(struct devfsmount *devfs_mp_p)
{
	devdirent_t *   new;
	int             error = 0;

	if ((error = dev_dup_entry(NULL, dev_root, &new, devfs_mp_p))) {
		return error;
	}
	devfs_mp_p->plane_root = new;
	devfs_nmountplanes++;
	return error;
}



                                                                
                    
 
                             
                                                                
void
devfs_free_plane(struct devfsmount *devfs_mp_p)
{
	devdirent_t * dirent_p;

	dirent_p = devfs_mp_p->plane_root;
	if (dirent_p) {
		dev_free_hier(dirent_p);
		dev_free_name(dirent_p);
	}
	devfs_mp_p->plane_root = NULL;
	devfs_nmountplanes--;

	if (devfs_nmountplanes > (devfs_nmountplanes + 1)) {
		panic("plane count wrapped around.\n");
	}
}


                                                                
                                          
                                 
                                            
                                
                                                              
                               
 
                             
                                                                
static int
dev_dup_entry(devnode_t * parent, devdirent_t * back, devdirent_t * *dnm_pp,
    struct devfsmount *dvm)
{
	devdirent_t *   entry_p = NULL;
	devdirent_t *   newback;
	devdirent_t *   newfront;
	int     error;
	devnode_t *     dnp = back->de_dnp;
	int type = dnp->dn_type;

	  
                                        
                                   
    
	error = dev_add_entry(back->de_name, parent, type, NULL, dnp,
	    parent?parent->dn_dvm:dvm, &entry_p);
	if (!error && (entry_p == NULL)) {
		error = ENOMEM;                                                          
	}
	if (error != 0) {
		printf("duplicating %s failed\n", back->de_name);
		goto out;
	}

	  
                                                                 
                     
    
	if (dvm) {
		entry_p->de_dnp->dn_dvm = dvm;
	}

	  
                                                         
                      
                                                         
    
	if (type == DEV_DIR) {
		for (newback = back->de_dnp->dn_typeinfo.Dir.dirlist;
		    newback; newback = newback->de_next) {
			if ((error = dev_dup_entry(entry_p->de_dnp,
			    newback, &newfront, NULL)) != 0) {
				break;                             
			}
		}
	}
out:
	*dnm_pp = entry_p;
	return error;
}


                                                                
                  
                                                        
                                        
                               
 
                             
                                                                

int
dev_free_name(devdirent_t * dirent_p)
{
	devnode_t *     parent = dirent_p->de_parent;
	devnode_t *     dnp = dirent_p->de_dnp;

	if (dnp) {
		if (dnp->dn_type == DEV_DIR) {
			devnode_t * p;

			if (dnp->dn_typeinfo.Dir.dirlist) {
				return ENOTEMPTY;
			}
			p = dnp->dn_typeinfo.Dir.parent;
			devfs_dn_free(dnp);                          
			devfs_dn_free(p);                 
		}
		  
                                                   
                                        
                                                      
                 
     
		if (dirent_p->de_nextlink == dirent_p) {
			dnp->dn_linklist = NULL;
		} else {
			if (dnp->dn_linklist == dirent_p) {
				dnp->dn_linklist = dirent_p->de_nextlink;
			}
		}
		devfs_dn_free(dnp);
	}

	dirent_p->de_nextlink->de_prevlinkp = dirent_p->de_prevlinkp;
	*(dirent_p->de_prevlinkp) = dirent_p->de_nextlink;

	  
                                                     
    
	if (parent) {                     
		if ((*dirent_p->de_prevp = dirent_p->de_next)) {                 
			dirent_p->de_next->de_prevp = dirent_p->de_prevp;
		} else {
			parent->dn_typeinfo.Dir.dirlast
			        = dirent_p->de_prevp;
		}
		parent->dn_typeinfo.Dir.entrycount--;
		parent->dn_len -= strlen(dirent_p->de_name) + 8;
	}

	DEVFS_DECR_ENTRIES();
	FREE(dirent_p, M_DEVFSNAME);
	return 0;
}


                                                                
                                                    
                                                        
                                        
                               
                                  
 
                             
                                                                

static void
dev_free_hier(devdirent_t * dirent_p)
{
	devnode_t *     dnp = dirent_p->de_dnp;

	if (dnp) {
		if (dnp->dn_type == DEV_DIR) {
			while (dnp->dn_typeinfo.Dir.dirlist) {
				dev_free_hier(dnp->dn_typeinfo.Dir.dirlist);
				dev_free_name(dnp->dn_typeinfo.Dir.dirlist);
			}
		}
	}
}


                                                                
                                                                 
                                                                  
  
                              
  
                                                                
                                                                   
                              
                                                                  
int
devfs_dntovn(devnode_t * dnp, struct vnode **vn_pp, __unused struct proc * p)
{
	struct vnode *vn_p;
	int error = 0;
	struct vnode_fsparam vfsp;
	enum vtype vtype = 0;
	int markroot = 0;
	int nretries = 0;
	int n_minor = DEVFS_CLONE_ALLOC;                                        

	  
                                                                                 
                                                                                 
                                                                                
                                                                              
                            
    
	if (dnp->dn_lflags & DN_DELETE) {
		panic("devfs_dntovn: DN_DELETE set on a devnode upon entry.");
	}

	devfs_ref_node(dnp);

retry:
	*vn_pp = NULL;
	vn_p = dnp->dn_vn;

	if (vn_p) {                          
		uint32_t vid;

		vid = vnode_vid(vn_p);

		DEVFS_UNLOCK();

		  
                                                           
                                                              
                                                       
                                                             
                                                               
                                                              
                                   
     
		error = vnode_getwithvid_drainok(vn_p, vid);

		DEVFS_LOCK();

		if (dnp->dn_lflags & DN_DELETE) {
			  
                                  
                                   
                    
      
			if (error == 0) {
				  
                                            
                            
       
				vnode_put(vn_p);
			}

			  
                                                             
                                                                   
                                                  
      
			error = ENOENT;
		} else if (error == ENODEV) {
			  
                                          
      
			error = ENOENT;
		} else if (error && (nretries < DEV_MAX_VNODE_RETRY)) {
			  
                                                        
                                                        
                                                        
                                                         
                                                           
                                                     
                                                           
                                                         
                                                        
                                                       
                                                         
                                   
      
			error = 0;
			nretries++;
			goto retry;
		}
		if (!error) {
			*vn_pp = vn_p;
		}

		goto out;
	}

	  
                                                  
                                                      
                                                
    

	if (dnp->dn_lflags & DN_CREATE) {
		dnp->dn_lflags |= DN_CREATEWAIT;
		msleep(&dnp->dn_lflags, &devfs_mutex, PRIBIO, 0, 0);
		goto retry;
	}

	dnp->dn_lflags |= DN_CREATE;

	switch (dnp->dn_type) {
	case    DEV_SLNK:
		vtype = VLNK;
		break;
	case    DEV_DIR:
		if (dnp->dn_typeinfo.Dir.parent == dnp) {
			markroot = 1;
		}
		vtype = VDIR;
		break;
	case    DEV_BDEV:
	case    DEV_CDEV:
		vtype = (dnp->dn_type == DEV_BDEV) ? VBLK : VCHR;
		break;
#if FDESC
	case    DEV_DEVFD:
		vtype = VDIR;
		break;
#endif            
	}
	vfsp.vnfs_mp = dnp->dn_dvm->mount;
	vfsp.vnfs_vtype = vtype;
	vfsp.vnfs_str = "devfs";
	vfsp.vnfs_dvp = 0;
	vfsp.vnfs_fsnode = dnp;
	vfsp.vnfs_cnp = 0;
	vfsp.vnfs_vops = *(dnp->dn_ops);

	if (vtype == VBLK || vtype == VCHR) {
		  
                                                               
                                                              
                                                          
     
		if (dnp->dn_clone != NULL) {
			int     n_major = major(dnp->dn_typeinfo.dev);

			n_minor = (*dnp->dn_clone)(dnp->dn_typeinfo.dev, DEVFS_CLONE_ALLOC);
			if (n_minor == -1) {
				error = ENOMEM;
				goto out;
			}

			vfsp.vnfs_rdev = makedev(n_major, n_minor);;
		} else {
			vfsp.vnfs_rdev = dnp->dn_typeinfo.dev;
		}
	} else {
		vfsp.vnfs_rdev = 0;
	}
	vfsp.vnfs_filesize = 0;
	vfsp.vnfs_flags = VNFS_NOCACHE | VNFS_CANTCACHE;
	                      
	vfsp.vnfs_marksystem = 0;
	vfsp.vnfs_markroot = markroot;

	DEVFS_UNLOCK();

	error = vnode_create(VNCREATE_FLAVOR, VCREATESIZE, &vfsp, &vn_p);

	                                      
	if (error == 0) {
		vnode_setneedinactive(vn_p);
	}

	DEVFS_LOCK();

	if (error == 0) {
		vnode_settag(vn_p, VT_DEVFS);

		if ((dnp->dn_clone != NULL) && (dnp->dn_vn != NULLVP)) {
			panic("devfs_dntovn: cloning device with a vnode?\n");
		}

		*vn_pp = vn_p;

		  
                                                       
                                                      
                                                   
                                  
     
		devfs_ref_node(dnp);

		  
                                                                
                      
     
		if (dnp->dn_clone == NULL) {
			dnp->dn_vn = vn_p;
		}
	} else if (n_minor != DEVFS_CLONE_ALLOC) {
		  
                                                                 
                                                               
                                                               
                                                               
                                                          
                                                                
                                   
     
		(void)(*dnp->dn_clone)(dnp->dn_typeinfo.dev, DEVFS_CLONE_FREE);
	}

	dnp->dn_lflags &= ~DN_CREATE;
	if (dnp->dn_lflags & DN_CREATEWAIT) {
		dnp->dn_lflags &= ~DN_CREATEWAIT;
		wakeup(&dnp->dn_lflags);
	}

out:
	  
                                                                                        
                                                                                    
                                                                      
    
	devfs_rele_node(dnp);

	return error;
}

  
                                                             
                                    
   
void
devfs_ref_node(devnode_t *dnp)
{
	os_ref_retain_locked_raw(&dnp->dn_refcount, &devfs_refgrp);
}

  
                                                                  
                                                        
   
void
devfs_rele_node(devnode_t *dnp)
{
	os_ref_count_t rc = os_ref_release_locked_raw(&dnp->dn_refcount, &devfs_refgrp);
	if (rc < 1) {
		panic("devfs_rele_node: devnode without a refcount!\n");
	} else if ((rc == 1) && (dnp->dn_lflags & DN_DELETE)) {
		                                               
		(void) os_ref_release_locked_raw(&dnp->dn_refcount, &devfs_refgrp);
		devnode_free(dnp);
	}
}

                                                                        
                                                                    
                                             
 
                             
                                                                        
int
dev_add_entry(const char *name, devnode_t * parent, int type, devnode_type_t * typeinfo,
    devnode_t * proto, struct devfsmount *dvm, devdirent_t * *nm_pp)
{
	devnode_t *     dnp;
	int     error = 0;

	if ((error = dev_add_node(type, typeinfo, proto, &dnp,
	    (parent?parent->dn_dvm:dvm))) != 0) {
		printf("devfs: %s: base node allocation failed (Errno=%d)\n",
		    name, error);
		return error;
	}
	if ((error = dev_add_name(name, parent, NULL, dnp, nm_pp)) != 0) {
		devfs_dn_free(dnp);                                      
		printf("devfs: %s: name slot allocation failed (Errno=%d)\n",
		    name, error);
	}
	return error;
}

static void
devfs_bulk_notify(devfs_event_log_t delp)
{
	uint32_t i;
	for (i = 0; i < delp->del_used; i++) {
		devfs_vnode_event_t dvep = &delp->del_entries[i];
		if (vnode_getwithvid(dvep->dve_vp, dvep->dve_vid) == 0) {
			vnode_notify(dvep->dve_vp, dvep->dve_events, NULL);
			vnode_put(dvep->dve_vp);
		}
	}
}

static void
devfs_record_event(devfs_event_log_t delp, devnode_t *dnp, uint32_t events)
{
	if (delp->del_used >= delp->del_max) {
		panic("devfs event log overflowed.\n");
	}

	                                                             
	if (dnp->dn_vn != NULLVP && vnode_ismonitored(dnp->dn_vn)) {
		devfs_vnode_event_t dvep = &delp->del_entries[delp->del_used];
		dvep->dve_vp = dnp->dn_vn;
		dvep->dve_vid = vnode_vid(dnp->dn_vn);
		dvep->dve_events = events;
		delp->del_used++;
	}
}

static int
devfs_init_event_log(devfs_event_log_t delp, uint32_t count, devfs_vnode_event_t buf)
{
	devfs_vnode_event_t dvearr;

	if (buf == NULL) {
		MALLOC(dvearr, devfs_vnode_event_t, count * sizeof(struct devfs_vnode_event), M_TEMP, M_WAITOK | M_ZERO);
		if (dvearr == NULL) {
			return ENOMEM;
		}
	} else {
		dvearr = buf;
	}

	delp->del_max = count;
	delp->del_used = 0;
	delp->del_entries = dvearr;
	return 0;
}

static void
devfs_release_event_log(devfs_event_log_t delp, int need_free)
{
	if (delp->del_entries == NULL) {
		panic("Free of devfs notify info that has not been intialized.\n");
	}

	if (need_free) {
		FREE(delp->del_entries, M_TEMP);
	}

	delp->del_entries = NULL;
}

  
                            
  
          
                                                                         
  
              
                                              
                                                                   
                         
                        
                                          
                                                                           
           
                                                             
   
void *
devfs_make_node_clone(dev_t dev, int chrblk, uid_t uid,
    gid_t gid, int perms, int (*clone)(dev_t dev, int action),
    const char *fmt, ...)
{
	devdirent_t *   new_dev = NULL;
	devfstype_t     type;
	va_list ap;

	switch (chrblk) {
	case DEVFS_CHAR:
		type = DEV_CDEV;
		break;
	case DEVFS_BLOCK:
		type = DEV_BDEV;
		break;
	default:
		goto out;
	}

	va_start(ap, fmt);
	new_dev = devfs_make_node_internal(dev, type, uid, gid, perms, clone, fmt, ap);
	va_end(ap);
out:
	return new_dev;
}


  
                            
  
          
                                                                         
  
              
                                              
                                                                   
                         
                        
                                                                           
           
                                                             
   
void *
devfs_make_node(dev_t dev, int chrblk, uid_t uid,
    gid_t gid, int perms, const char *fmt, ...)
{
	devdirent_t *   new_dev = NULL;
	devfstype_t type;
	va_list ap;

	if (chrblk != DEVFS_CHAR && chrblk != DEVFS_BLOCK) {
		goto out;
	}

	type = (chrblk == DEVFS_BLOCK ? DEV_BDEV : DEV_CDEV);

	va_start(ap, fmt);
	new_dev = devfs_make_node_internal(dev, type, uid, gid, perms, NULL, fmt, ap);
	va_end(ap);

out:
	return new_dev;
}

static devdirent_t *
devfs_make_node_internal(dev_t dev, devfstype_t type, uid_t uid,
    gid_t gid, int perms, int (*clone)(dev_t dev, int action), const char *fmt, va_list ap)
{
	devdirent_t *   new_dev = NULL;
	devnode_t * dnp;
	devnode_type_t  typeinfo;

	char            *name, buf[256];          
	const char      *path;
#if CONFIG_MACF
	char buff[sizeof(buf)];
#endif
	size_t          i;
	uint32_t        log_count;
	struct devfs_event_log event_log;
	struct devfs_vnode_event stackbuf[NUM_STACK_ENTRIES];
	int             need_free = 0;

	vsnprintf(buf, sizeof(buf), fmt, ap);

#if CONFIG_MACF
	bcopy(buf, buff, sizeof(buff));
	buff[sizeof(buff) - 1] = 0;
#endif
	name = NULL;

	for (i = strlen(buf); i > 0; i--) {
		if (buf[i] == '/') {
			name = &buf[i];
			buf[i] = 0;
			break;
		}
	}

	if (name) {
		*name++ = '\0';
		path = buf;
	} else {
		name = buf;
		path = "/";
	}

	log_count = devfs_nmountplanes;
	if (log_count > NUM_STACK_ENTRIES) {
wrongsize:
		need_free = 1;
		if (devfs_init_event_log(&event_log, log_count, NULL) != 0) {
			return NULL;
		}
	} else {
		need_free = 0;
		log_count = NUM_STACK_ENTRIES;
		if (devfs_init_event_log(&event_log, log_count, &stackbuf[0]) != 0) {
			return NULL;
		}
	}

	DEVFS_LOCK();
	if (log_count < devfs_nmountplanes) {
		DEVFS_UNLOCK();
		devfs_release_event_log(&event_log, need_free);
		log_count = log_count * 2;
		goto wrongsize;
	}

	if (!devfs_ready) {
		printf("devfs_make_node: not ready for devices!\n");
		goto out;
	}

	                                             
	if (dev_finddir(path, NULL, DEVFS_CREATE, &dnp, &event_log) == 0) {
		typeinfo.dev = dev;
		if (dev_add_entry(name, dnp, type, &typeinfo, NULL, NULL, &new_dev) == 0) {
			new_dev->de_dnp->dn_gid = gid;
			new_dev->de_dnp->dn_uid = uid;
			new_dev->de_dnp->dn_mode |= perms;
			new_dev->de_dnp->dn_clone = clone;
#if CONFIG_MACF
			mac_devfs_label_associate_device(dev, new_dev->de_dnp, buff);
#endif
			devfs_propogate(dnp->dn_typeinfo.Dir.myname, new_dev, &event_log);
		}
	}

out:
	DEVFS_UNLOCK();

	devfs_bulk_notify(&event_log);
	devfs_release_event_log(&event_log, need_free);
	return new_dev;
}

  
                            
  
           
                                                       
  
           
                                  
   
int
devfs_make_link(void *original, char *fmt, ...)
{
	devdirent_t *   new_dev = NULL;
	devdirent_t *   orig = (devdirent_t *) original;
	devnode_t *     dirnode;                                          
	struct devfs_event_log event_log;
	uint32_t        log_count;

	va_list ap;
	char *p, buf[256];          
	size_t i;

	DEVFS_LOCK();

	if (!devfs_ready) {
		DEVFS_UNLOCK();
		printf("devfs_make_link: not ready for devices!\n");
		return -1;
	}
	DEVFS_UNLOCK();

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	p = NULL;

	for (i = strlen(buf); i > 0; i--) {
		if (buf[i] == '/') {
			p = &buf[i];
			buf[i] = 0;
			break;
		}
	}

	  
                                                     
                            
    
	log_count = devfs_nmountplanes * 2;
wrongsize:
	if (devfs_init_event_log(&event_log, log_count, NULL) != 0) {
		                                                        
		return -1;
	}

	DEVFS_LOCK();

	if (log_count < devfs_nmountplanes) {
		DEVFS_UNLOCK();
		devfs_release_event_log(&event_log, 1);
		log_count = log_count * 2;
		goto wrongsize;
	}

	if (p) {
		*p++ = '\0';

		if (dev_finddir(buf, NULL, DEVFS_CREATE, &dirnode, &event_log)
		    || dev_add_name(p, dirnode, NULL, orig->de_dnp, &new_dev)) {
			goto fail;
		}
	} else {
		if (dev_finddir("", NULL, DEVFS_CREATE, &dirnode, &event_log)
		    || dev_add_name(buf, dirnode, NULL, orig->de_dnp, &new_dev)) {
			goto fail;
		}
	}
	devfs_propogate(dirnode->dn_typeinfo.Dir.myname, new_dev, &event_log);
fail:
	DEVFS_UNLOCK();
	devfs_bulk_notify(&event_log);
	devfs_release_event_log(&event_log, 1);

	return (new_dev != NULL) ? 0 : -1;
}
