/*
 * Copyright (c) 1995-2019 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/proc_internal.h>
#include <sys/file_internal.h>
#include <sys/kauth.h>
#include <sys/uio_internal.h>
#include <kern/kalloc.h>
#include <sys/attr.h>
#include <sys/sysproto.h>
#include <sys/xattr.h>
#include <sys/fsevents.h>
#include <kern/zalloc.h>
#include <miscfs/specfs/specdev.h>
#include <security/audit/audit.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#define ATTR_TIME_SIZE  -1

static int readdirattr(vnode_t, struct fd_vn_data *, uio_t, struct attrlist *,
    uint64_t, int *, int *, vfs_context_t ctx) __attribute__((noinline));

static void
vattr_get_alt_data(vnode_t, struct attrlist *, struct vnode_attr *, int, int,
    int, vfs_context_t) __attribute__((noinline));

static void get_error_attributes(vnode_t, struct attrlist *, uint64_t, user_addr_t,
    size_t, int, caddr_t, vfs_context_t) __attribute__((noinline));

static int getvolattrlist(vfs_context_t, vnode_t, struct attrlist *, user_addr_t,
    size_t, uint64_t, enum uio_seg, int) __attribute__((noinline));

static int get_direntry(vfs_context_t, vnode_t, struct fd_vn_data *, int *,
    struct direntry **) __attribute__((noinline));

  
                                                                       
   
struct _attrlist_buf {
	char    *base;
	char    *fixedcursor;
	char    *varcursor;
	ssize_t allocated;
	ssize_t needed;
	attribute_set_t actual;
	attribute_set_t valid;
};


  
                                                                     
                                 
   
static void
attrlist_pack_fixed(struct _attrlist_buf *ab, void *source, ssize_t count)
{
	  
                                          
                                    
    
	ssize_t fit;

	  
                                                                
                                                               
                            
   
                                                                  
                    
   
                                                              
                                                              
                                                                  
                                           
    
	fit = lmin(count, ab->allocated - (ab->fixedcursor - ab->base));
	if (fit > 0) {
		                               
		bcopy(source, ab->fixedcursor, fit);
	}

	                                                                          
	ab->fixedcursor += roundup(count, 4);
}

  
                                                                           
                                                                                    
                                                                               
  
                                                                             
                                                                                     
   
static void
attrlist_pack_variable2(struct _attrlist_buf *ab, const void *source, ssize_t count,
    const void *ext, ssize_t extcount)
{
	                                         
	struct attrreference ar;
	ssize_t fit;

	  
                                                          
                                                                    
                                      
    
	ar.attr_dataoffset = (int32_t)(ab->varcursor - ab->fixedcursor);
	ar.attr_length = (u_int32_t)(count + extcount);
	attrlist_pack_fixed(ab, &ar, sizeof(ar));

	  
                                                                       
                                                                      
                            
   
                                                                                 
                                                               
    
	fit = lmin(count, ab->allocated - (ab->varcursor - ab->base));
	if (fit > 0) {
		if (source != NULL) {
			bcopy(source, ab->varcursor, fit);
		}
		ab->varcursor += fit;
	}

	                                                          
	fit = lmin(extcount, ab->allocated - (ab->varcursor - ab->base));
	if (fit > 0) {
		                                                                        
		if (ext != NULL) {
			bcopy(ext, ab->varcursor, fit);
		}
		ab->varcursor += fit;
	}
	                                    
	ab->varcursor = (char *)roundup((uintptr_t)ab->varcursor, 4);
}

  
                                                                                     
                            
   
static void
attrlist_pack_variable(struct _attrlist_buf *ab, const void *source, ssize_t count)
{
	attrlist_pack_variable2(ab, source, count, NULL, 0);
}

  
                                                                                  
  
                                                                                 
                                                                                 
                                                                                  
                                                                                
  
                                                                                       
                                                                                     
                                                                                 
                                                                                
                          
   
static void
attrlist_pack_string(struct _attrlist_buf *ab, const char *source, size_t count)
{
	struct attrreference ar;
	ssize_t fit, space;

	  
                                                                            
                                
    
	if (source == NULL) {
		count = 0;
	} else if (count == 0) {
		count = strlen(source);
	}

	  
                                                                   
    
	ar.attr_dataoffset = (int32_t)(ab->varcursor - ab->fixedcursor);
	ar.attr_length = (u_int32_t)count + 1;
	attrlist_pack_fixed(ab, &ar, sizeof(ar));

	  
                                                                          
   
                                                                             
                           
   
                                                                           
                                                                               
                                                                                 
                                   
    
	space = ab->allocated - (ab->varcursor - ab->base);
	fit = lmin(count, space);
	if (space > 0) {
		long bytes_to_zero;

		  
                                                   
                                             
    
                                                                 
                                                                         
                                       
    
                                                                   
                                                       
     
		bcopy(source, ab->varcursor, fit);
		                                         
		if (space > fit) {
			ab->varcursor[fit++] = '\0';
			                                                           
			  
                                                      
                            
      
			bytes_to_zero = lmin((roundup(fit, 4) - fit),
			    space - fit);
			if (bytes_to_zero) {
				bzero(&(ab->varcursor[fit]), bytes_to_zero);
			}
		}
	}
	  
                                                               
    
	ab->varcursor += roundup((count + 1), 4);
}

#define ATTR_PACK4(AB, V)                                                 \
	do {                                                              \
	        if ((AB.allocated - (AB.fixedcursor - AB.base)) >= 4) {   \
	                *(uint32_t *)AB.fixedcursor = V;                  \
	                AB.fixedcursor += 4;                              \
	        }                                                         \
	} while (0)

#define ATTR_PACK8(AB, V)                                                 \
	do {                                                              \
	        if ((AB.allocated - (AB.fixedcursor - AB.base)) >= 8) {   \
	                memcpy(AB.fixedcursor, &V, 8);                    \
	                AB.fixedcursor += 8;                              \
	        }                                                         \
	} while (0)

#define ATTR_PACK(b, v) attrlist_pack_fixed(b, &v, sizeof(v))
#define ATTR_PACK_CAST(b, t, v)                                         \
	do {                                                            \
	        t _f = (t)v;                                            \
	        ATTR_PACK(b, _f);                                       \
	} while (0)

#define ATTR_PACK_TIME(b, v, is64)                                                      \
	do {                                                                            \
	        if (is64) {                                                             \
	                struct user64_timespec us = {.tv_sec = v.tv_sec, .tv_nsec = v.tv_nsec};         \
	                ATTR_PACK(&b, us);                                              \
	        } else {                                                                \
	                struct user32_timespec us = {.tv_sec = (user32_time_t)v.tv_sec, .tv_nsec = (user32_long_t)v.tv_nsec};         \
	                ATTR_PACK(&b, us);                                              \
	        }                                                                       \
	} while(0)


  
                                                             
   
struct getvolattrlist_attrtab {
	attrgroup_t     attr;
	uint64_t        bits;
#define VFSATTR_BIT(b)  (VFSATTR_ ## b)
	ssize_t         size;
};
static struct getvolattrlist_attrtab getvolattrlist_common_tab[] = {
	{.attr = ATTR_CMN_NAME, .bits = 0, .size = sizeof(struct attrreference)},
	{.attr = ATTR_CMN_DEVID, .bits = 0, .size = sizeof(dev_t)},
	{.attr = ATTR_CMN_FSID, .bits = 0, .size = sizeof(fsid_t)},
	{.attr = ATTR_CMN_OBJTYPE, .bits = 0, .size = sizeof(fsobj_type_t)},
	{.attr = ATTR_CMN_OBJTAG, .bits = 0, .size = sizeof(fsobj_tag_t)},
	{.attr = ATTR_CMN_OBJID, .bits = 0, .size = sizeof(fsobj_id_t)},
	{.attr = ATTR_CMN_OBJPERMANENTID, .bits = 0, .size = sizeof(fsobj_id_t)},
	{.attr = ATTR_CMN_PAROBJID, .bits = 0, .size = sizeof(fsobj_id_t)},
	{.attr = ATTR_CMN_SCRIPT, .bits = 0, .size = sizeof(text_encoding_t)},
	{.attr = ATTR_CMN_CRTIME, .bits = VFSATTR_BIT(f_create_time), .size = ATTR_TIME_SIZE},
	{.attr = ATTR_CMN_MODTIME, .bits = VFSATTR_BIT(f_modify_time), .size = ATTR_TIME_SIZE},
	{.attr = ATTR_CMN_CHGTIME, .bits = VFSATTR_BIT(f_modify_time), .size = ATTR_TIME_SIZE},
	{.attr = ATTR_CMN_ACCTIME, .bits = VFSATTR_BIT(f_access_time), .size = ATTR_TIME_SIZE},
	{.attr = ATTR_CMN_BKUPTIME, .bits = VFSATTR_BIT(f_backup_time), .size = ATTR_TIME_SIZE},
	{.attr = ATTR_CMN_FNDRINFO, .bits = 0, .size = 32},
	{.attr = ATTR_CMN_OWNERID, .bits = 0, .size = sizeof(uid_t)},
	{.attr = ATTR_CMN_GRPID, .bits = 0, .size = sizeof(gid_t)},
	{.attr = ATTR_CMN_ACCESSMASK, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = ATTR_CMN_FLAGS, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = ATTR_CMN_USERACCESS, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = ATTR_CMN_EXTENDED_SECURITY, .bits = 0, .size = sizeof(struct attrreference)},
	{.attr = ATTR_CMN_UUID, .bits = 0, .size = sizeof(guid_t)},
	{.attr = ATTR_CMN_GRPUUID, .bits = 0, .size = sizeof(guid_t)},
	{.attr = ATTR_CMN_FILEID, .bits = 0, .size = sizeof(uint64_t)},
	{.attr = ATTR_CMN_PARENTID, .bits = 0, .size = sizeof(uint64_t)},
	{.attr = ATTR_CMN_RETURNED_ATTRS, .bits = 0, .size = sizeof(attribute_set_t)},
	{.attr = ATTR_CMN_ERROR, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = 0, .bits = 0, .size = 0}
};
#define ATTR_CMN_VOL_INVALID \
	(ATTR_CMN_EXTENDED_SECURITY | ATTR_CMN_UUID | ATTR_CMN_GRPUUID | \
	 ATTR_CMN_FILEID | ATTR_CMN_PARENTID)

static struct getvolattrlist_attrtab getvolattrlist_vol_tab[] = {
	{.attr = ATTR_VOL_FSTYPE, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_SIGNATURE, .bits = VFSATTR_BIT(f_signature), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_SIZE, .bits = VFSATTR_BIT(f_blocks)  |  VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_SPACEFREE, .bits = VFSATTR_BIT(f_bfree) | VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_SPACEAVAIL, .bits = VFSATTR_BIT(f_bavail) | VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_MINALLOCATION, .bits = VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_ALLOCATIONCLUMP, .bits = VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_IOBLOCKSIZE, .bits = VFSATTR_BIT(f_iosize), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_OBJCOUNT, .bits = VFSATTR_BIT(f_objcount), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_FILECOUNT, .bits = VFSATTR_BIT(f_filecount), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_DIRCOUNT, .bits = VFSATTR_BIT(f_dircount), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_MAXOBJCOUNT, .bits = VFSATTR_BIT(f_maxobjcount), .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_MOUNTPOINT, .bits = 0, .size = sizeof(struct attrreference)},
	{.attr = ATTR_VOL_NAME, .bits = VFSATTR_BIT(f_vol_name), .size = sizeof(struct attrreference)},
	{.attr = ATTR_VOL_MOUNTFLAGS, .bits = 0, .size = sizeof(uint32_t)},
	{.attr = ATTR_VOL_MOUNTEDDEVICE, .bits = 0, .size = sizeof(struct attrreference)},
	{.attr = ATTR_VOL_ENCODINGSUSED, .bits = 0, .size = sizeof(uint64_t)},
	{.attr = ATTR_VOL_CAPABILITIES, .bits = VFSATTR_BIT(f_capabilities), .size = sizeof(vol_capabilities_attr_t)},
	{.attr = ATTR_VOL_UUID, .bits = VFSATTR_BIT(f_uuid), .size = sizeof(uuid_t)},
	{.attr = ATTR_VOL_QUOTA_SIZE, .bits = VFSATTR_BIT(f_quota) | VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_RESERVED_SIZE, .bits = VFSATTR_BIT(f_reserved) | VFSATTR_BIT(f_bsize), .size = sizeof(off_t)},
	{.attr = ATTR_VOL_ATTRIBUTES, .bits = VFSATTR_BIT(f_attributes), .size = sizeof(vol_attributes_attr_t)},
	{.attr = ATTR_VOL_INFO, .bits = 0, .size = 0},
	{.attr = 0, .bits = 0, .size = 0}
};

static int
getvolattrlist_parsetab(struct getvolattrlist_attrtab *tab, attrgroup_t attrs, struct vfs_attr *vsp,
    ssize_t *sizep, int is_64bit, unsigned int maxiter)
{
	attrgroup_t     recognised;

	recognised = 0;
	do {
		                            
		if (tab->attr & attrs) {
			recognised |= tab->attr;
			vsp->f_active |= tab->bits;
			if (tab->size == ATTR_TIME_SIZE) {
				if (is_64bit) {
					*sizep += sizeof(struct user64_timespec);
				} else {
					*sizep += sizeof(struct user32_timespec);
				}
			} else {
				*sizep += tab->size;
			}
		}
	} while (((++tab)->attr != 0) && (--maxiter > 0));

	                                                                           
	if (attrs & ~recognised) {
		return EINVAL;
	}
	return 0;
}

  
                                                               
                              
   
static int
getvolattrlist_setupvfsattr(struct attrlist *alp, struct vfs_attr *vsp, ssize_t *sizep, int is_64bit)
{
	int     error;
	if (!alp) {
		return EINVAL;
	}

	  
                           
    
	*sizep = sizeof(uint32_t);                        
	if (alp->commonattr) {
		if ((alp->commonattr & ATTR_CMN_VOL_INVALID) &&
		    (alp->commonattr & ATTR_CMN_RETURNED_ATTRS) == 0) {
			return EINVAL;
		}
		if ((error = getvolattrlist_parsetab(getvolattrlist_common_tab,
		    alp->commonattr, vsp, sizep,
		    is_64bit,
		    sizeof(getvolattrlist_common_tab) / sizeof(getvolattrlist_common_tab[0]))) != 0) {
			return error;
		}
	}
	if (alp->volattr &&
	    (error = getvolattrlist_parsetab(getvolattrlist_vol_tab, alp->volattr, vsp, sizep, is_64bit, sizeof(getvolattrlist_vol_tab) / sizeof(getvolattrlist_vol_tab[0]))) != 0) {
		return error;
	}

	return 0;
}

  
                                                         
                                                      
                                          
   
static void
getvolattrlist_fixupattrs(attribute_set_t *asp, struct vfs_attr *vsp)
{
	struct getvolattrlist_attrtab *tab;

	if (asp->commonattr) {
		tab = getvolattrlist_common_tab;
		do {
			if ((tab->attr & asp->commonattr) &&
			    (tab->bits != 0) &&
			    ((tab->bits & vsp->f_supported) == 0)) {
				asp->commonattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
	if (asp->volattr) {
		tab = getvolattrlist_vol_tab;
		do {
			if ((tab->attr & asp->volattr) &&
			    (tab->bits != 0) &&
			    ((tab->bits & vsp->f_supported) == 0)) {
				asp->volattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
}

  
                                                                                  
   
struct getattrlist_attrtab {
	attrgroup_t     attr;
	uint64_t        bits;
#define VATTR_BIT(b)    (VNODE_ATTR_ ## b)
	ssize_t         size;
	kauth_action_t  action;
};

  
                                                                                                       
                                                           
   
static struct getattrlist_attrtab getattrlist_common_tab[] = {
	{.attr = ATTR_CMN_NAME, .bits = VATTR_BIT(va_name), .size = sizeof(struct attrreference), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_DEVID, .bits = VATTR_BIT(va_fsid), .size = sizeof(dev_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FSID, .bits = VATTR_BIT(va_fsid64), .size = sizeof(fsid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJTYPE, .bits = 0, .size = sizeof(fsobj_type_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJTAG, .bits = 0, .size = sizeof(fsobj_tag_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJID, .bits = VATTR_BIT(va_fileid) | VATTR_BIT(va_linkid), .size = sizeof(fsobj_id_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJPERMANENTID, .bits = VATTR_BIT(va_fileid) | VATTR_BIT(va_linkid), .size = sizeof(fsobj_id_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_PAROBJID, .bits = VATTR_BIT(va_parentid), .size = sizeof(fsobj_id_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_SCRIPT, .bits = VATTR_BIT(va_encoding), .size = sizeof(text_encoding_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_CRTIME, .bits = VATTR_BIT(va_create_time), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_MODTIME, .bits = VATTR_BIT(va_modify_time), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_CHGTIME, .bits = VATTR_BIT(va_change_time), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_ACCTIME, .bits = VATTR_BIT(va_access_time), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_BKUPTIME, .bits = VATTR_BIT(va_backup_time), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FNDRINFO, .bits = 0, .size = 32, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OWNERID, .bits = VATTR_BIT(va_uid), .size = sizeof(uid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_GRPID, .bits = VATTR_BIT(va_gid), .size = sizeof(gid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_ACCESSMASK, .bits = VATTR_BIT(va_mode), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FLAGS, .bits = VATTR_BIT(va_flags), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_GEN_COUNT, .bits = VATTR_BIT(va_write_gencount), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_DOCUMENT_ID, .bits = VATTR_BIT(va_document_id), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_USERACCESS, .bits = 0, .size = sizeof(uint32_t), .action = 0},
	{.attr = ATTR_CMN_EXTENDED_SECURITY, .bits = VATTR_BIT(va_acl), .size = sizeof(struct attrreference), .action = KAUTH_VNODE_READ_SECURITY},
	{.attr = ATTR_CMN_UUID, .bits = VATTR_BIT(va_uuuid), .size = sizeof(guid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_GRPUUID, .bits = VATTR_BIT(va_guuid), .size = sizeof(guid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FILEID, .bits = VATTR_BIT(va_fileid), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_PARENTID, .bits = VATTR_BIT(va_parentid), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FULLPATH, .bits = 0, .size = sizeof(struct attrreference), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_ADDEDTIME, .bits = VATTR_BIT(va_addedtime), .size = ATTR_TIME_SIZE, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_RETURNED_ATTRS, .bits = 0, .size = sizeof(attribute_set_t), .action = 0},
	{.attr = ATTR_CMN_ERROR, .bits = 0, .size = sizeof(uint32_t), .action = 0},
	{.attr = ATTR_CMN_DATA_PROTECT_FLAGS, .bits = VATTR_BIT(va_dataprotect_class), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

static struct getattrlist_attrtab getattrlist_dir_tab[] = {
	{.attr = ATTR_DIR_LINKCOUNT, .bits = VATTR_BIT(va_dirlinkcount), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_DIR_ENTRYCOUNT, .bits = VATTR_BIT(va_nchildren), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_DIR_MOUNTSTATUS, .bits = 0, .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_DIR_ALLOCSIZE, .bits = VATTR_BIT(va_total_alloc) | VATTR_BIT(va_total_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_DIR_IOBLOCKSIZE, .bits = VATTR_BIT(va_iosize), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_DIR_DATALENGTH, .bits = VATTR_BIT(va_total_size) | VATTR_BIT(va_data_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};
static struct getattrlist_attrtab getattrlist_file_tab[] = {
	{.attr = ATTR_FILE_LINKCOUNT, .bits = VATTR_BIT(va_nlink), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_TOTALSIZE, .bits = VATTR_BIT(va_total_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_ALLOCSIZE, .bits = VATTR_BIT(va_total_alloc) | VATTR_BIT(va_total_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_IOBLOCKSIZE, .bits = VATTR_BIT(va_iosize), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_CLUMPSIZE, .bits = 0, .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_DEVTYPE, .bits = VATTR_BIT(va_rdev), .size = sizeof(dev_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_DATALENGTH, .bits = VATTR_BIT(va_total_size) | VATTR_BIT(va_data_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_DATAALLOCSIZE, .bits = VATTR_BIT(va_total_alloc) | VATTR_BIT(va_data_alloc), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_RSRCLENGTH, .bits = 0, .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_RSRCALLOCSIZE, .bits = 0, .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

                                                       
static struct getattrlist_attrtab getattrlist_common_extended_tab[] = {
	{.attr = ATTR_CMNEXT_RELPATH, .bits = 0, .size = sizeof(struct attrreference), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_PRIVATESIZE, .bits = VATTR_BIT(va_private_size), .size = sizeof(off_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_LINKID, .bits = VATTR_BIT(va_fileid) | VATTR_BIT(va_linkid), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_NOFIRMLINKPATH, .bits = 0, .size = sizeof(struct attrreference), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_REALDEVID, .bits = VATTR_BIT(va_devid), .size = sizeof(uint32_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_REALFSID, .bits = VATTR_BIT(va_fsid64), .size = sizeof(fsid_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_CLONEID, .bits = VATTR_BIT(va_clone_id), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_EXT_FLAGS, .bits = VATTR_BIT(va_extflags), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMNEXT_RECURSIVE_GENCOUNT, .bits = VATTR_BIT(va_recursive_gencount), .size = sizeof(uint64_t), .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

  
                                                                              
                                                                         
                                                                          
                                                                            
         
                                                                         
                                                        
   
static struct getattrlist_attrtab getattrlistbulk_common_tab[] = {
	{.attr = ATTR_CMN_DEVID, .bits = VATTR_BIT(va_devid), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_FSID, .bits = VATTR_BIT(va_fsid64), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJTYPE, .bits = VATTR_BIT(va_objtype), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_OBJTAG, .bits = VATTR_BIT(va_objtag), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_CMN_USERACCESS, .bits = VATTR_BIT(va_user_access), .size = 0, .action = 0},
	{.attr = ATTR_CMN_FNDRINFO, .bits = VATTR_BIT(va_finderinfo), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

static struct getattrlist_attrtab getattrlistbulk_file_tab[] = {
	{.attr = ATTR_FILE_RSRCLENGTH, .bits = VATTR_BIT(va_rsrc_length), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = ATTR_FILE_RSRCALLOCSIZE, .bits = VATTR_BIT(va_rsrc_alloc), .size = 0, .action = KAUTH_VNODE_READ_ATTRIBUTES},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

static struct getattrlist_attrtab getattrlistbulk_common_extended_tab[] = {
	                                                
	{.attr = 0, .bits = 0, .size = 0, .action = 0},
	{.attr = 0, .bits = 0, .size = 0, .action = 0}
};

  
                                                    
  
                                                                                          
   
#define VFS_DFLT_ATTR_VOL       (ATTR_VOL_FSTYPE | ATTR_VOL_SIGNATURE |  \
	                         ATTR_VOL_SIZE | ATTR_VOL_SPACEFREE |  ATTR_VOL_QUOTA_SIZE | ATTR_VOL_RESERVED_SIZE | \
	                         ATTR_VOL_SPACEAVAIL | ATTR_VOL_MINALLOCATION |  \
	                         ATTR_VOL_ALLOCATIONCLUMP |  ATTR_VOL_IOBLOCKSIZE |  \
	                         ATTR_VOL_MOUNTPOINT | ATTR_VOL_MOUNTFLAGS |  \
	                         ATTR_VOL_MOUNTEDDEVICE | ATTR_VOL_CAPABILITIES |  \
	                         ATTR_VOL_ATTRIBUTES | ATTR_VOL_ENCODINGSUSED)

#define VFS_DFLT_ATTR_CMN       (ATTR_CMN_NAME | ATTR_CMN_DEVID |  \
	                         ATTR_CMN_FSID | ATTR_CMN_OBJTYPE |  \
	                         ATTR_CMN_OBJTAG | ATTR_CMN_OBJID |  \
	                         ATTR_CMN_PAROBJID | ATTR_CMN_SCRIPT |  \
	                         ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME |  \
	                         ATTR_CMN_FNDRINFO |  \
	                         ATTR_CMN_OWNERID  | ATTR_CMN_GRPID |  \
	                         ATTR_CMN_ACCESSMASK | ATTR_CMN_FLAGS |  \
	                         ATTR_CMN_USERACCESS | ATTR_CMN_FILEID | \
	                         ATTR_CMN_PARENTID | ATTR_CMN_RETURNED_ATTRS | \
	                         ATTR_CMN_DOCUMENT_ID | ATTR_CMN_GEN_COUNT | \
	                         ATTR_CMN_DATA_PROTECT_FLAGS)

#define VFS_DFLT_ATTR_CMN_EXT   (ATTR_CMNEXT_PRIVATESIZE | ATTR_CMNEXT_LINKID |  \
	                         ATTR_CMNEXT_NOFIRMLINKPATH | ATTR_CMNEXT_REALDEVID |  \
	                         ATTR_CMNEXT_REALFSID | ATTR_CMNEXT_CLONEID | \
	                         ATTR_CMNEXT_EXT_FLAGS)

#define VFS_DFLT_ATTR_DIR       (ATTR_DIR_LINKCOUNT | ATTR_DIR_MOUNTSTATUS)

#define VFS_DFLT_ATTR_FILE      (ATTR_FILE_LINKCOUNT | ATTR_FILE_TOTALSIZE |  \
	                         ATTR_FILE_ALLOCSIZE  | ATTR_FILE_IOBLOCKSIZE |  \
	                         ATTR_FILE_DEVTYPE | ATTR_FILE_DATALENGTH |  \
	                         ATTR_FILE_DATAALLOCSIZE | ATTR_FILE_RSRCLENGTH |  \
	                         ATTR_FILE_RSRCALLOCSIZE)

static int
getattrlist_parsetab(struct getattrlist_attrtab *tab, attrgroup_t attrs,
    struct vnode_attr *vap, ssize_t *sizep, kauth_action_t *actionp,
    int is_64bit, unsigned int maxiter)
{
	attrgroup_t     recognised;
	recognised = 0;
	if (!tab) {
		return EINVAL;
	}

	do {
		                            
		if (tab->attr & attrs) {
			recognised |= tab->attr;
			if (vap) {
				vap->va_active |= tab->bits;
			}
			if (sizep) {
				if (tab->size == ATTR_TIME_SIZE) {
					if (is_64bit) {
						*sizep += sizeof(
							struct user64_timespec);
					} else {
						*sizep += sizeof(
							struct user32_timespec);
					}
				} else {
					*sizep += tab->size;
				}
			}
			if (actionp) {
				*actionp |= tab->action;
			}
			if (attrs == recognised) {
				break;                         
			}
		}
	} while (((++tab)->attr != 0) && (--maxiter > 0));

	                                                                           
	if (attrs & ~recognised) {
		return EINVAL;
	}
	return 0;
}

  
                                                               
                              
   
static int
getattrlist_setupvattr(struct attrlist *alp, struct vnode_attr *vap, ssize_t *sizep, kauth_action_t *actionp, int is_64bit, int isdir, int use_fork)
{
	int     error;

	  
                           
    
	*sizep = sizeof(uint32_t);                        
	*actionp = 0;
	if (alp->commonattr &&
	    (error = getattrlist_parsetab(getattrlist_common_tab, alp->commonattr, vap, sizep, actionp, is_64bit, sizeof(getattrlist_common_tab) / sizeof(getattrlist_common_tab[0]))) != 0) {
		return error;
	}
	if (isdir && alp->dirattr &&
	    (error = getattrlist_parsetab(getattrlist_dir_tab, alp->dirattr, vap, sizep, actionp, is_64bit, sizeof(getattrlist_dir_tab) / sizeof(getattrlist_dir_tab[0]))) != 0) {
		return error;
	}
	if (!isdir && alp->fileattr &&
	    (error = getattrlist_parsetab(getattrlist_file_tab, alp->fileattr, vap, sizep, actionp, is_64bit, sizeof(getattrlist_file_tab) / sizeof(getattrlist_file_tab[0]))) != 0) {
		return error;
	}
	if (use_fork && alp->forkattr &&
	    (error = getattrlist_parsetab(getattrlist_common_extended_tab, alp->forkattr, vap, sizep, actionp, is_64bit, sizeof(getattrlist_common_extended_tab) / sizeof(getattrlist_common_extended_tab[0]))) != 0) {
		return error;
	}

	return 0;
}

  
                                                               
                              
   
static int
getattrlist_setupvattr_all(struct attrlist *alp, struct vnode_attr *vap,
    enum vtype obj_type, ssize_t *fixedsize, int is_64bit, int use_fork)
{
	int     error = 0;

	  
                           
    
	if (fixedsize) {
		*fixedsize = sizeof(uint32_t);
	}
	if (alp->commonattr) {
		error = getattrlist_parsetab(getattrlist_common_tab,
		    alp->commonattr, vap, fixedsize, NULL, is_64bit,
		    sizeof(getattrlist_common_tab) / sizeof(getattrlist_common_tab[0]));

		if (!error) {
			                                            
			(void)getattrlist_parsetab(getattrlistbulk_common_tab,
			    alp->commonattr, vap, fixedsize, NULL, is_64bit,
			    sizeof(getattrlistbulk_common_tab) / sizeof(getattrlistbulk_common_tab[0]));
		}
	}

	if (!error && (obj_type == VNON || obj_type == VDIR) && alp->dirattr) {
		error = getattrlist_parsetab(getattrlist_dir_tab, alp->dirattr,
		    vap, fixedsize, NULL, is_64bit,
		    sizeof(getattrlist_dir_tab) / sizeof(getattrlist_dir_tab[0]));
	}

	if (!error && (obj_type != VDIR) && alp->fileattr) {
		error = getattrlist_parsetab(getattrlist_file_tab,
		    alp->fileattr, vap, fixedsize, NULL, is_64bit,
		    sizeof(getattrlist_file_tab) / sizeof(getattrlist_file_tab[0]));

		if (!error) {
			                                          
			(void)getattrlist_parsetab(getattrlistbulk_file_tab,
			    alp->fileattr, vap, fixedsize, NULL, is_64bit,
			    sizeof(getattrlistbulk_file_tab) / sizeof(getattrlistbulk_file_tab[0]));
		}
	}

	                                                                   
	if (!error && use_fork && alp->forkattr) {
		error = getattrlist_parsetab(getattrlist_common_extended_tab,
		    alp->forkattr, vap, fixedsize, NULL, is_64bit,
		    sizeof(getattrlist_common_extended_tab) / sizeof(getattrlist_common_extended_tab[0]));

		if (!error) {
			(void)getattrlist_parsetab(getattrlistbulk_common_extended_tab,
			    alp->forkattr, vap, fixedsize, NULL, is_64bit,
			    sizeof(getattrlistbulk_common_extended_tab) / sizeof(getattrlistbulk_common_extended_tab[0]));
		}
	}

	return error;
}

int
vfs_setup_vattr_from_attrlist(struct attrlist *alp, struct vnode_attr *vap,
    enum vtype obj_vtype, ssize_t *attrs_fixed_sizep, vfs_context_t ctx)
{
	VATTR_INIT(vap);

	                                                                           
	                                       
	return getattrlist_setupvattr_all(alp, vap, obj_vtype,
	           attrs_fixed_sizep, IS_64BIT_PROCESS(vfs_context_proc(ctx)), 1);
}




  
                                                         
                                                      
                                          
   
static void
getattrlist_fixupattrs(attribute_set_t *asp, struct vnode_attr *vap, int use_fork)
{
	struct getattrlist_attrtab *tab;

	if (asp->commonattr) {
		tab = getattrlist_common_tab;
		do {
			  
                                                                
                                                         
                                                                
                                                                          
     
                                                                                  
                                                                            
                                                                                
                                                                              
                                                                                
                                                         
     
                                                                               
                                                                                 
                                                                            
                                                                                  
                                                                        
                                                
      
			if ((tab->attr & asp->commonattr) &&
			    (tab->bits & vap->va_active) &&
			    (tab->bits & vap->va_supported) == 0) {
				asp->commonattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
	if (asp->dirattr) {
		tab = getattrlist_dir_tab;
		do {
			if ((tab->attr & asp->dirattr) &&
			    (tab->bits & vap->va_active) &&
			    (vap->va_supported & tab->bits) == 0) {
				asp->dirattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
	if (asp->fileattr) {
		tab = getattrlist_file_tab;
		do {
			if ((tab->attr & asp->fileattr) &&
			    (tab->bits & vap->va_active) &&
			    (vap->va_supported & tab->bits) == 0) {
				asp->fileattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
	if (use_fork && asp->forkattr) {
		tab = getattrlist_common_extended_tab;
		do {
			if ((tab->attr & asp->forkattr) &&
			    (tab->bits & vap->va_active) &&
			    (vap->va_supported & tab->bits) == 0) {
				asp->forkattr &= ~tab->attr;
			}
		} while ((++tab)->attr != 0);
	}
}

static int
setattrlist_setfinderinfo(vnode_t vp, char *fndrinfo, struct vfs_context *ctx)
{
	uio_t   auio;
	char    uio_buf[UIO_SIZEOF(1)];
	int     error;

	if ((auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_WRITE, uio_buf, sizeof(uio_buf))) == NULL) {
		error = ENOMEM;
	} else {
		uio_addiov(auio, CAST_USER_ADDR_T(fndrinfo), 32);
		error = vn_setxattr(vp, XATTR_FINDERINFO_NAME, auio, XATTR_NOSECURITY, ctx);
		uio_free(auio);
	}

#if CONFIG_FSE
	if (error == 0 && need_fsevent(FSE_FINDER_INFO_CHANGED, vp)) {
		add_fsevent(FSE_FINDER_INFO_CHANGED, ctx, FSE_ARG_VNODE, vp, FSE_ARG_DONE);
	}
#endif
	return error;
}


  
                                                                                  
  
   
static void
getattrlist_findnamecomp(const char *mn, const char **np, ssize_t *nl)
{
	int             counting;
	const char      *cp;

	  
                                                                
                                            
    
	*np = NULL;
	*nl = 0;
	counting = 0;
	for (cp = mn; *cp != 0; cp++) {
		if (!counting) {
			                           
			if (*cp != '/') {
				*np = cp;
				counting = 1;
			}
		} else {
			                         
			if (*cp == '/') {
				*nl = cp - *np;
				counting = 0;
			}
		}
	}
	                        
	if (counting) {
		*nl = cp - *np;
	}
}


static int
getvolattrlist(vfs_context_t ctx, vnode_t vp, struct attrlist *alp,
    user_addr_t attributeBuffer, size_t bufferSize, uint64_t options,
    enum uio_seg segflg, int is_64bit)
{
	struct vfs_attr vs;
	struct vnode_attr va;
	struct _attrlist_buf ab;
	int             error;
	ssize_t         fixedsize, varsize;
	const char      *cnp = NULL;                                    
	ssize_t         cnl = 0;                                        
	int             release_str = 0;
	mount_t         mnt;
	int             return_valid;
	int             pack_invalid;
	vnode_t         root_vp = NULL;

	ab.base = NULL;
	VATTR_INIT(&va);
	VFSATTR_INIT(&vs);
	vs.f_vol_name = NULL;
	mnt = vp->v_mount;


	                                         
	return_valid = (alp->commonattr & ATTR_CMN_RETURNED_ATTRS);
	pack_invalid = (options & FSOPT_PACK_INVAL_ATTRS);
	if (pack_invalid) {
		                                                             
		if (!return_valid) {
			error = EINVAL;
			goto out;
		}
		                                                 
		bzero(&vs, sizeof(vs));
		                                               
		bcopy(&alp->commonattr, &ab.valid, sizeof(attribute_set_t));
	}

	                                                                   
	if (!vnode_isvroot(vp)) {
		if (mnt != NULL) {
			error = VFS_ROOT(mnt, &root_vp, ctx);
			if (error) {
				VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: volume attributes requested on non-root vnode, but got an error getting root.");
				goto out;
			}
			vp = root_vp;
		} else {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: volume attributes requested on non-root vnode, but no backpointer to mount.");
			goto out;
		}
	}

	  
                                                          
    
	if ((error = getvolattrlist_setupvfsattr(alp, &vs, &fixedsize, is_64bit)) != 0) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: setup for request failed");
		goto out;
	}
	if (vs.f_active != 0) {
		                                                                            
		if (VFSATTR_IS_ACTIVE(&vs, f_vol_name)) {
			vs.f_vol_name = (char *) zalloc(ZV_NAMEI);
			vs.f_vol_name[0] = '\0';
		}

		VFS_DEBUG(ctx, vp, "ATTRLIST -       calling to get %016llx with supported %016llx", vs.f_active, vs.f_supported);
		if ((error = vfs_getattr(mnt, &vs, ctx)) != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: filesystem returned %d", error);
			goto out;
		}
#if CONFIG_MACF
		error = mac_mount_check_getattr(ctx, mnt, &vs);
		if (error != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: MAC framework returned %d", error);
			goto out;
		}
#endif
		  
                                                             
     
		if (!VFSATTR_ALL_SUPPORTED(&vs)) {
			                                      
			if (VFSATTR_IS_ACTIVE(&vs, f_fssubtype)
			    && !VFSATTR_IS_SUPPORTED(&vs, f_fssubtype)) {
				VFSATTR_RETURN(&vs, f_fssubtype, 0);
			}

			  
                                                        
                                                        
                                                  
      
			if (VFSATTR_IS_ACTIVE(&vs, f_signature)
			    && !VFSATTR_IS_SUPPORTED(&vs, f_signature)) {
				VFSATTR_RETURN(&vs, f_signature, 0x4244);
			}

			                            
			if (VFSATTR_IS_ACTIVE(&vs, f_bsize)
			    && !VFSATTR_IS_SUPPORTED(&vs, f_bsize)) {
				VFSATTR_RETURN(&vs, f_bsize, mnt->mnt_devblocksize);
			}

			                                           
			if (VFSATTR_IS_ACTIVE(&vs, f_attributes)
			    && !VFSATTR_IS_SUPPORTED(&vs, f_attributes)) {
				vol_attributes_attr_t *attrp = &vs.f_attributes;

				attrp->validattr.commonattr = VFS_DFLT_ATTR_CMN;
				attrp->validattr.volattr = VFS_DFLT_ATTR_VOL;
				attrp->validattr.dirattr = VFS_DFLT_ATTR_DIR;
				attrp->validattr.fileattr = VFS_DFLT_ATTR_FILE;
				attrp->validattr.forkattr = VFS_DFLT_ATTR_CMN_EXT;

				attrp->nativeattr.commonattr =  0;
				attrp->nativeattr.volattr = 0;
				attrp->nativeattr.dirattr = 0;
				attrp->nativeattr.fileattr = 0;
				attrp->nativeattr.forkattr = 0;
				VFSATTR_SET_SUPPORTED(&vs, f_attributes);
			}

			                                             
			if (VFSATTR_IS_ACTIVE(&vs, f_capabilities)) {
				                                          
				if (!VFSATTR_IS_SUPPORTED(&vs, f_capabilities)) {
					vs.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] = 0;
					vs.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] = VOL_CAP_INT_ATTRLIST;
					vs.f_capabilities.capabilities[VOL_CAPABILITIES_RESERVED1] = 0;
					vs.f_capabilities.capabilities[VOL_CAPABILITIES_RESERVED2] = 0;

					vs.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] = 0;
					vs.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] = VOL_CAP_INT_ATTRLIST;
					vs.f_capabilities.valid[VOL_CAPABILITIES_RESERVED1] = 0;
					vs.f_capabilities.valid[VOL_CAPABILITIES_RESERVED2] = 0;
					VFSATTR_SET_SUPPORTED(&vs, f_capabilities);
				} else {
					                                                               
					vs.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_ATTRLIST;
					vs.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_ATTRLIST;
				}
			}

			                                            
			if (!VFSATTR_ALL_SUPPORTED(&vs)) {
				if (return_valid) {
					if (pack_invalid) {
						                                           
						getvolattrlist_fixupattrs(&ab.valid, &vs);

						                                           
						vs.f_supported = vs.f_active;
					} else {
						                                     
						getvolattrlist_fixupattrs((attribute_set_t *)&alp->commonattr, &vs);
					}
				} else {
					error = EINVAL;
					goto out;
				}
			}
		}
	}

	  
                                             
    
	if (alp->commonattr & (ATTR_CMN_OWNERID | ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK | ATTR_CMN_FLAGS | ATTR_CMN_SCRIPT)) {
		VATTR_WANTED(&va, va_uid);
		VATTR_WANTED(&va, va_gid);
		VATTR_WANTED(&va, va_mode);
		VATTR_WANTED(&va, va_flags);
		VATTR_WANTED(&va, va_encoding);

		if ((error = vnode_getattr(vp, &va, ctx)) != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: could not fetch attributes from root vnode", vp);
			goto out;
		}
#if CONFIG_MACF
		error = mac_vnode_check_getattr(ctx, NOCRED, vp, &va);
		if (error != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: MAC framework returned %d for root vnode", error);
			goto out;
		}
#endif
		if (VATTR_IS_ACTIVE(&va, va_encoding) &&
		    !VATTR_IS_SUPPORTED(&va, va_encoding)) {
			if (!return_valid || pack_invalid) {
				                                 
				VATTR_RETURN(&va, va_encoding, 0x7e);
			} else {
				                         
				alp->commonattr &= ~ATTR_CMN_SCRIPT;
			}
		}
	}

	  
                                              
    
	varsize = 0;
	if (alp->commonattr & ATTR_CMN_NAME) {
		if (vp->v_mount->mnt_vfsstat.f_mntonname[1] == 0x00 &&
		    vp->v_mount->mnt_vfsstat.f_mntonname[0] == '/') {
			                                                         
                                                               
                                                                 
                                                                 
                                 
      
			cnp = vnode_getname(vp);
			if (cnp == NULL) {
				                          
				cnp = &vp->v_mount->mnt_vfsstat.f_mntonname[0];
			} else {
				release_str = 1;
			}
			cnl = strlen(cnp);
		} else {
			getattrlist_findnamecomp(vp->v_mount->mnt_vfsstat.f_mntonname, &cnp, &cnl);
		}
		if (alp->commonattr & ATTR_CMN_NAME) {
			varsize += roundup(cnl + 1, 4);
		}
	}
	if (alp->volattr & ATTR_VOL_MOUNTPOINT) {
		varsize += roundup(strlen(mnt->mnt_vfsstat.f_mntonname) + 1, 4);
	}
	if (alp->volattr & ATTR_VOL_NAME) {
		vs.f_vol_name[MAXPATHLEN - 1] = '\0';                             
		varsize += roundup(strlen(vs.f_vol_name) + 1, 4);
	}
	if (alp->volattr & ATTR_VOL_MOUNTEDDEVICE) {
		varsize += roundup(strlen(mnt->mnt_vfsstat.f_mntfromname) + 1, 4);
	}

	  
                                                   
                                                                          
                                                   
    
	ab.allocated = fixedsize + varsize;
	if (((size_t)ab.allocated) > ATTR_MAX_BUFFER) {
		error = ENOMEM;
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: buffer size too large (%d limit %d)", ab.allocated, ATTR_MAX_BUFFER);
		goto out;
	}

	if (return_valid &&
	    (ab.allocated < (ssize_t)(sizeof(uint32_t) + sizeof(attribute_set_t))) &&
	    !(options & FSOPT_REPORT_FULLSIZE)) {
		uint32_t num_bytes_valid = sizeof(uint32_t);
		  
                                                              
                                           
                                                               
                                                  
     
		if (UIO_SEG_IS_USER_SPACE(segflg)) {
			error = copyout(&num_bytes_valid,
			    CAST_USER_ADDR_T(attributeBuffer), num_bytes_valid);
		} else {
			bcopy(&num_bytes_valid, (void *)attributeBuffer,
			    (size_t)num_bytes_valid);
		}
		goto out;
	}

	ab.base = kheap_alloc(KHEAP_TEMP, ab.allocated, Z_ZERO | Z_WAITOK);
	if (ab.base == NULL) {
		error = ENOMEM;
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: could not allocate %d for copy buffer", ab.allocated);
		goto out;
	}

	  
                                             
    
	ab.fixedcursor = ab.base + sizeof(uint32_t);
	if (return_valid) {
		ab.fixedcursor += sizeof(attribute_set_t);
		bzero(&ab.actual, sizeof(ab.actual));
	}
	ab.varcursor = ab.base + fixedsize;
	ab.needed = fixedsize + varsize;

	                                                                        
	if (alp->commonattr & ATTR_CMN_ERROR) {
		ATTR_PACK4(ab, 0);
		ab.actual.commonattr |= ATTR_CMN_ERROR;
	}
	if (alp->commonattr & ATTR_CMN_NAME) {
		attrlist_pack_string(&ab, cnp, cnl);
		ab.actual.commonattr |= ATTR_CMN_NAME;
	}
	if (alp->commonattr & ATTR_CMN_DEVID) {
		ATTR_PACK4(ab, mnt->mnt_vfsstat.f_fsid.val[0]);
		ab.actual.commonattr |= ATTR_CMN_DEVID;
	}
	if (alp->commonattr & ATTR_CMN_FSID) {
		ATTR_PACK8(ab, mnt->mnt_vfsstat.f_fsid);
		ab.actual.commonattr |= ATTR_CMN_FSID;
	}
	if (alp->commonattr & ATTR_CMN_OBJTYPE) {
		if (!return_valid || pack_invalid) {
			ATTR_PACK4(ab, 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_OBJTAG) {
		ATTR_PACK4(ab, vp->v_tag);
		ab.actual.commonattr |= ATTR_CMN_OBJTAG;
	}
	if (alp->commonattr & ATTR_CMN_OBJID) {
		if (!return_valid || pack_invalid) {
			fsobj_id_t f = {0, 0};
			ATTR_PACK8(ab, f);
		}
	}
	if (alp->commonattr & ATTR_CMN_OBJPERMANENTID) {
		if (!return_valid || pack_invalid) {
			fsobj_id_t f = {0, 0};
			ATTR_PACK8(ab, f);
		}
	}
	if (alp->commonattr & ATTR_CMN_PAROBJID) {
		if (!return_valid || pack_invalid) {
			fsobj_id_t f = {0, 0};
			ATTR_PACK8(ab, f);
		}
	}
	                                                                                
	if (alp->commonattr & ATTR_CMN_SCRIPT) {
		ATTR_PACK4(ab, va.va_encoding);
		ab.actual.commonattr |= ATTR_CMN_SCRIPT;
	}
	if (alp->commonattr & ATTR_CMN_CRTIME) {
		ATTR_PACK_TIME(ab, vs.f_create_time, is_64bit);
		ab.actual.commonattr |= ATTR_CMN_CRTIME;
	}
	if (alp->commonattr & ATTR_CMN_MODTIME) {
		ATTR_PACK_TIME(ab, vs.f_modify_time, is_64bit);
		ab.actual.commonattr |= ATTR_CMN_MODTIME;
	}
	if (alp->commonattr & ATTR_CMN_CHGTIME) {
		if (!return_valid || pack_invalid) {
			ATTR_PACK_TIME(ab, vs.f_modify_time, is_64bit);
		}
	}
	if (alp->commonattr & ATTR_CMN_ACCTIME) {
		ATTR_PACK_TIME(ab, vs.f_access_time, is_64bit);
		ab.actual.commonattr |= ATTR_CMN_ACCTIME;
	}
	if (alp->commonattr & ATTR_CMN_BKUPTIME) {
		ATTR_PACK_TIME(ab, vs.f_backup_time, is_64bit);
		ab.actual.commonattr |= ATTR_CMN_BKUPTIME;
	}
	if (alp->commonattr & ATTR_CMN_FNDRINFO) {
		char f[32];
		  
                                                               
     
		if (vp->v_tag == VT_HFS) {
#define HFS_GET_BOOT_INFO   (FCNTL_FS_SPECIFIC_BASE + 0x00004)
			error = VNOP_IOCTL(vp, HFS_GET_BOOT_INFO, (caddr_t)&f, 0, ctx);
			if (error == 0) {
				attrlist_pack_fixed(&ab, f, sizeof(f));
				ab.actual.commonattr |= ATTR_CMN_FNDRINFO;
			} else if (!return_valid) {
				goto out;
			}
		} else if (!return_valid || pack_invalid) {
			                                                         
			bzero(&f, sizeof(f));
			attrlist_pack_fixed(&ab, f, sizeof(f));
		}
	}
	if (alp->commonattr & ATTR_CMN_OWNERID) {
		ATTR_PACK4(ab, va.va_uid);
		ab.actual.commonattr |= ATTR_CMN_OWNERID;
	}
	if (alp->commonattr & ATTR_CMN_GRPID) {
		ATTR_PACK4(ab, va.va_gid);
		ab.actual.commonattr |= ATTR_CMN_GRPID;
	}
	if (alp->commonattr & ATTR_CMN_ACCESSMASK) {
		ATTR_PACK_CAST(&ab, uint32_t, va.va_mode);
		ab.actual.commonattr |= ATTR_CMN_ACCESSMASK;
	}
	if (alp->commonattr & ATTR_CMN_FLAGS) {
		ATTR_PACK4(ab, va.va_flags);
		ab.actual.commonattr |= ATTR_CMN_FLAGS;
	}
	if (alp->commonattr & ATTR_CMN_USERACCESS) {                                                       
		uint32_t        perms = 0;
		if (vnode_isdir(vp)) {
			if (vnode_authorize(vp, NULL,
			    KAUTH_VNODE_ACCESS | KAUTH_VNODE_ADD_FILE | KAUTH_VNODE_ADD_SUBDIRECTORY | KAUTH_VNODE_DELETE_CHILD, ctx) == 0) {
				perms |= W_OK;
			}
			if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_LIST_DIRECTORY, ctx) == 0) {
				perms |= R_OK;
			}
			if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_SEARCH, ctx) == 0) {
				perms |= X_OK;
			}
		} else {
			if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_WRITE_DATA, ctx) == 0) {
				perms |= W_OK;
			}
			if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_READ_DATA, ctx) == 0) {
				perms |= R_OK;
			}
			if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_EXECUTE, ctx) == 0) {
				perms |= X_OK;
			}
		}
#if CONFIG_MACF
		  
                                                        
                                                              
                                                
     
		if (perms & W_OK) {
			if (mac_vnode_check_access(ctx, vp, W_OK) != 0) {
				perms &= ~W_OK;
			}
		}
		if (perms & R_OK) {
			if (mac_vnode_check_access(ctx, vp, R_OK) != 0) {
				perms &= ~R_OK;
			}
		}
		if (perms & X_OK) {
			if (mac_vnode_check_access(ctx, vp, X_OK) != 0) {
				perms &= ~X_OK;
			}
		}
#endif          
		KAUTH_DEBUG("ATTRLIST - returning user access %x", perms);
		ATTR_PACK4(ab, perms);
		ab.actual.commonattr |= ATTR_CMN_USERACCESS;
	}
	  
                                                   
                                                 
    
	if (pack_invalid) {
		uint64_t fid = 0;

		if (alp->commonattr & ATTR_CMN_EXTENDED_SECURITY) {
			attrlist_pack_variable(&ab, NULL, 0);
		}
		if (alp->commonattr & ATTR_CMN_UUID) {
			ATTR_PACK(&ab, kauth_null_guid);
		}
		if (alp->commonattr & ATTR_CMN_GRPUUID) {
			ATTR_PACK(&ab, kauth_null_guid);
		}
		if (alp->commonattr & ATTR_CMN_FILEID) {
			ATTR_PACK8(ab, fid);
		}
		if (alp->commonattr & ATTR_CMN_PARENTID) {
			ATTR_PACK8(ab, fid);
		}
	}

	                                                                        

	if (alp->volattr & ATTR_VOL_FSTYPE) {
		ATTR_PACK_CAST(&ab, uint32_t, vfs_typenum(mnt));
		ab.actual.volattr |= ATTR_VOL_FSTYPE;
	}
	if (alp->volattr & ATTR_VOL_SIGNATURE) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_signature);
		ab.actual.volattr |= ATTR_VOL_SIGNATURE;
	}
	if (alp->volattr & ATTR_VOL_SIZE) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize * vs.f_blocks);
		ab.actual.volattr |= ATTR_VOL_SIZE;
	}
	if (alp->volattr & ATTR_VOL_SPACEFREE) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize * vs.f_bfree);
		ab.actual.volattr |= ATTR_VOL_SPACEFREE;
	}
	if (alp->volattr & ATTR_VOL_SPACEAVAIL) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize * vs.f_bavail);
		ab.actual.volattr |= ATTR_VOL_SPACEAVAIL;
	}
	if (alp->volattr & ATTR_VOL_MINALLOCATION) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize);
		ab.actual.volattr |= ATTR_VOL_MINALLOCATION;
	}
	if (alp->volattr & ATTR_VOL_ALLOCATIONCLUMP) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize);                                        
		ab.actual.volattr |= ATTR_VOL_ALLOCATIONCLUMP;
	}
	if (alp->volattr & ATTR_VOL_IOBLOCKSIZE) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_iosize);
		ab.actual.volattr |= ATTR_VOL_IOBLOCKSIZE;
	}
	if (alp->volattr & ATTR_VOL_OBJCOUNT) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_objcount);
		ab.actual.volattr |= ATTR_VOL_OBJCOUNT;
	}
	if (alp->volattr & ATTR_VOL_FILECOUNT) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_filecount);
		ab.actual.volattr |= ATTR_VOL_FILECOUNT;
	}
	if (alp->volattr & ATTR_VOL_DIRCOUNT) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_dircount);
		ab.actual.volattr |= ATTR_VOL_DIRCOUNT;
	}
	if (alp->volattr & ATTR_VOL_MAXOBJCOUNT) {
		ATTR_PACK_CAST(&ab, uint32_t, vs.f_maxobjcount);
		ab.actual.volattr |= ATTR_VOL_MAXOBJCOUNT;
	}
	if (alp->volattr & ATTR_VOL_MOUNTPOINT) {
		attrlist_pack_string(&ab, mnt->mnt_vfsstat.f_mntonname, 0);
		ab.actual.volattr |= ATTR_VOL_MOUNTPOINT;
	}
	if (alp->volattr & ATTR_VOL_NAME) {
		attrlist_pack_string(&ab, vs.f_vol_name, 0);
		ab.actual.volattr |= ATTR_VOL_NAME;
	}
	if (alp->volattr & ATTR_VOL_MOUNTFLAGS) {
		ATTR_PACK_CAST(&ab, uint32_t, mnt->mnt_flag);
		ab.actual.volattr |= ATTR_VOL_MOUNTFLAGS;
	}
	if (alp->volattr & ATTR_VOL_MOUNTEDDEVICE) {
		attrlist_pack_string(&ab, mnt->mnt_vfsstat.f_mntfromname, 0);
		ab.actual.volattr |= ATTR_VOL_MOUNTEDDEVICE;
	}
	if (alp->volattr & ATTR_VOL_ENCODINGSUSED) {
		if (!return_valid || pack_invalid) {
			ATTR_PACK_CAST(&ab, uint64_t, ~0LL);                            
		}
	}
	if (alp->volattr & ATTR_VOL_CAPABILITIES) {
		                                
		if (vfs_extendedsecurity(mnt)) {
			vs.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_EXTENDED_SECURITY;
		} else {
			vs.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] &= ~VOL_CAP_INT_EXTENDED_SECURITY;
		}
		vs.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_EXTENDED_SECURITY;

		  
                                                                         
                                                                         
     
		if (!(vs.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_NO_IMMUTABLE_FILES)) {
			vs.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] &= ~VOL_CAP_FMT_NO_IMMUTABLE_FILES;
			vs.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] |= VOL_CAP_FMT_NO_IMMUTABLE_FILES;
		}

		if (!(vs.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_NO_PERMISSIONS)) {
			vs.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] &= ~VOL_CAP_FMT_NO_PERMISSIONS;
			vs.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] |= VOL_CAP_FMT_NO_PERMISSIONS;
		}

		  
                                                                                                     
                                                                                                 
                                                                                    
     
		vs.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_USERACCESS;
		vs.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] |= VOL_CAP_INT_USERACCESS;

		ATTR_PACK(&ab, vs.f_capabilities);
		ab.actual.volattr |= ATTR_VOL_CAPABILITIES;
	}
	if (alp->volattr & ATTR_VOL_UUID) {
		ATTR_PACK(&ab, vs.f_uuid);
		ab.actual.volattr |= ATTR_VOL_UUID;
	}
	if (alp->volattr & ATTR_VOL_QUOTA_SIZE) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize * vs.f_quota);
		ab.actual.volattr |= ATTR_VOL_QUOTA_SIZE;
	}
	if (alp->volattr & ATTR_VOL_RESERVED_SIZE) {
		ATTR_PACK_CAST(&ab, off_t, vs.f_bsize * vs.f_reserved);
		ab.actual.volattr |= ATTR_VOL_RESERVED_SIZE;
	}
	if (alp->volattr & ATTR_VOL_ATTRIBUTES) {
		                                         

		vs.f_attributes.validattr.commonattr |= VFS_DFLT_ATTR_CMN;
		vs.f_attributes.validattr.volattr |= VFS_DFLT_ATTR_VOL;
		vs.f_attributes.validattr.dirattr |= VFS_DFLT_ATTR_DIR;
		vs.f_attributes.validattr.fileattr |= VFS_DFLT_ATTR_FILE;

		if (vfs_extendedsecurity(mnt)) {
			vs.f_attributes.validattr.commonattr |= (ATTR_CMN_EXTENDED_SECURITY | ATTR_CMN_UUID | ATTR_CMN_GRPUUID);
		} else {
			vs.f_attributes.validattr.commonattr &= ~(ATTR_CMN_EXTENDED_SECURITY | ATTR_CMN_UUID | ATTR_CMN_GRPUUID);
			vs.f_attributes.nativeattr.commonattr &= ~(ATTR_CMN_EXTENDED_SECURITY | ATTR_CMN_UUID | ATTR_CMN_GRPUUID);
		}
		ATTR_PACK(&ab, vs.f_attributes);
		ab.actual.volattr |= ATTR_VOL_ATTRIBUTES;
	}

	                
	if (!return_valid && (ab.fixedcursor - ab.base) != fixedsize) {
		panic("packed field size mismatch; allocated %ld but packed %ld for common %08x vol %08x",
		    fixedsize, (long) (ab.fixedcursor - ab.base), alp->commonattr, alp->volattr);
	}
	if (!return_valid && ab.varcursor != (ab.base + ab.needed)) {
		panic("packed variable field size mismatch; used %ld but expected %ld", (long) (ab.varcursor - ab.base), ab.needed);
	}

	  
                                                                                     
                                                                                       
                                                                                        
                                                                     
    
	*(uint32_t *)ab.base = (options & FSOPT_REPORT_FULLSIZE) ? (uint32_t)ab.needed : (uint32_t)lmin(bufferSize, ab.needed);

	                                               
	if (return_valid &&
	    (ab.allocated >= (ssize_t)(sizeof(uint32_t) + sizeof(ab.actual)))) {
		ab.actual.commonattr |= ATTR_CMN_RETURNED_ATTRS;
		if (pack_invalid) {
			                                               
			ab.actual.commonattr &= ab.valid.commonattr;
			ab.actual.volattr &= ab.valid.volattr;
		}
		bcopy(&ab.actual, ab.base + sizeof(uint32_t), sizeof(ab.actual));
	}

	if (UIO_SEG_IS_USER_SPACE(segflg)) {
		error = copyout(ab.base, CAST_USER_ADDR_T(attributeBuffer),
		    ulmin((uint32_t)bufferSize, (uint32_t)ab.needed));
	} else {
		bcopy(ab.base, (void *)attributeBuffer, (size_t)ulmin((uint32_t)bufferSize, (uint32_t)ab.needed));
	}

out:
	if (vs.f_vol_name != NULL) {
		zfree(ZV_NAMEI, vs.f_vol_name);
	}
	if (release_str) {
		vnode_putname(cnp);
	}
	kheap_free(KHEAP_TEMP, ab.base, ab.allocated);
	VFS_DEBUG(ctx, vp, "ATTRLIST - returning %d", error);

	if (root_vp != NULL) {
		vnode_put(root_vp);
	}
	return error;
}

  
                                                  
                                                         
                                                       
                                                                     
             
   
static errno_t
attr_pack_common(vfs_context_t ctx, mount_t mp, vnode_t vp, struct attrlist *alp,
    struct _attrlist_buf *abp, struct vnode_attr *vap, int proc_is64,
    const char *cnp, ssize_t cnl, const char *fullpathptr,
    ssize_t fullpathlen, int return_valid, int pack_invalid, int vtype,
    int is_bulk)
{
	uint32_t        perms = 0;
	int             error = 0;

	if ((alp->commonattr & ATTR_CMN_ERROR) &&
	    (!return_valid || pack_invalid)) {
		ATTR_PACK4((*abp), 0);
		abp->actual.commonattr |= ATTR_CMN_ERROR;
	}
	if (alp->commonattr & ATTR_CMN_NAME) {
		attrlist_pack_string(abp, cnp, cnl);
		abp->actual.commonattr |= ATTR_CMN_NAME;
	}
	if (alp->commonattr & ATTR_CMN_DEVID) {
		if (mp) {                              
			ATTR_PACK4((*abp),
			    mp->mnt_vfsstat.f_fsid.val[0]);
			abp->actual.commonattr |= ATTR_CMN_DEVID;
		} else if (VATTR_IS_ACTIVE(vap, va_fsid) && VATTR_IS_SUPPORTED(vap, va_fsid)) {
			ATTR_PACK4((*abp), vap->va_fsid);
			abp->actual.commonattr |= ATTR_CMN_DEVID;
		} else if (vp) {
			ATTR_PACK4((*abp),
			    vp->v_mount->mnt_vfsstat.f_fsid.val[0]);
			abp->actual.commonattr |= ATTR_CMN_DEVID;
		} else if (VATTR_IS_SUPPORTED(vap, va_devid)) {
			ATTR_PACK4((*abp), vap->va_devid);
			abp->actual.commonattr |= ATTR_CMN_DEVID;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_FSID) {
		if (mp) {                             
			ATTR_PACK8((*abp),
			    mp->mnt_vfsstat.f_fsid);
			abp->actual.commonattr |= ATTR_CMN_FSID;
		} else if (VATTR_IS_SUPPORTED(vap, va_fsid64)) {
			ATTR_PACK8((*abp), vap->va_fsid64);
			abp->actual.commonattr |= ATTR_CMN_FSID;
		} else if (vp) {
			ATTR_PACK8((*abp),
			    vp->v_mount->mnt_vfsstat.f_fsid);
			abp->actual.commonattr |= ATTR_CMN_FSID;
		} else if (!return_valid || pack_invalid) {
			fsid_t fsid = {{0}};
			ATTR_PACK8((*abp), fsid);
		}
	}
	if (alp->commonattr & ATTR_CMN_OBJTYPE) {
		if (vp) {
			ATTR_PACK4((*abp), vtype);
			abp->actual.commonattr |= ATTR_CMN_OBJTYPE;
		} else if (VATTR_IS_SUPPORTED(vap, va_objtype)) {
			ATTR_PACK4((*abp), vap->va_objtype);
			abp->actual.commonattr |= ATTR_CMN_OBJTYPE;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_OBJTAG) {
		if (vp) {
			ATTR_PACK4((*abp), vp->v_tag);
			abp->actual.commonattr |= ATTR_CMN_OBJTAG;
		} else if (VATTR_IS_SUPPORTED(vap, va_objtag)) {
			ATTR_PACK4((*abp), vap->va_objtag);
			abp->actual.commonattr |= ATTR_CMN_OBJTAG;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_OBJID) {
		  
                                                      
                                                        
                                                      
                
     
		if (vap->va_vaflags & VA_64BITOBJIDS) {
			if (VATTR_IS_SUPPORTED(vap, va_linkid)) {
				ATTR_PACK8((*abp), vap->va_linkid);
			} else {
				ATTR_PACK8((*abp), vap->va_fileid);
			}
		} else {
			fsobj_id_t f;
			if (VATTR_IS_SUPPORTED(vap, va_linkid)) {
				f.fid_objno = (uint32_t)vap->va_linkid;
			} else {
				f.fid_objno = (uint32_t)vap->va_fileid;
			}
			f.fid_generation = 0;
			ATTR_PACK8((*abp), f);
		}
		abp->actual.commonattr |= ATTR_CMN_OBJID;
	}
	if (alp->commonattr & ATTR_CMN_OBJPERMANENTID) {
		  
                                                      
                                                        
                                                      
                
     
		if (vap->va_vaflags & VA_64BITOBJIDS) {
			if (VATTR_IS_SUPPORTED(vap, va_linkid)) {
				ATTR_PACK8((*abp), vap->va_linkid);
			} else {
				ATTR_PACK8((*abp), vap->va_fileid);
			}
		} else {
			fsobj_id_t f;
			if (VATTR_IS_SUPPORTED(vap, va_linkid)) {
				f.fid_objno = (uint32_t)vap->va_linkid;
			} else {
				f.fid_objno = (uint32_t)vap->va_fileid;
			}
			f.fid_generation = 0;
			ATTR_PACK8((*abp), f);
		}
		abp->actual.commonattr |= ATTR_CMN_OBJPERMANENTID;
	}
	if (alp->commonattr & ATTR_CMN_PAROBJID) {
		if (vap->va_vaflags & VA_64BITOBJIDS) {
			ATTR_PACK8((*abp), vap->va_parentid);
		} else {
			fsobj_id_t f;
			f.fid_objno = (uint32_t)vap->va_parentid;
			f.fid_generation = 0;
			ATTR_PACK8((*abp), f);
		}
		abp->actual.commonattr |= ATTR_CMN_PAROBJID;
	}
	if (alp->commonattr & ATTR_CMN_SCRIPT) {
		if (VATTR_IS_SUPPORTED(vap, va_encoding)) {
			ATTR_PACK4((*abp), vap->va_encoding);
			abp->actual.commonattr |= ATTR_CMN_SCRIPT;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0x7e);
		}
	}
	if (alp->commonattr & ATTR_CMN_CRTIME) {
		ATTR_PACK_TIME((*abp), vap->va_create_time, proc_is64);
		abp->actual.commonattr |= ATTR_CMN_CRTIME;
	}
	if (alp->commonattr & ATTR_CMN_MODTIME) {
		ATTR_PACK_TIME((*abp), vap->va_modify_time, proc_is64);
		abp->actual.commonattr |= ATTR_CMN_MODTIME;
	}
	if (alp->commonattr & ATTR_CMN_CHGTIME) {
		ATTR_PACK_TIME((*abp), vap->va_change_time, proc_is64);
		abp->actual.commonattr |= ATTR_CMN_CHGTIME;
	}
	if (alp->commonattr & ATTR_CMN_ACCTIME) {
		ATTR_PACK_TIME((*abp), vap->va_access_time, proc_is64);
		abp->actual.commonattr |= ATTR_CMN_ACCTIME;
	}
	if (alp->commonattr & ATTR_CMN_BKUPTIME) {
		ATTR_PACK_TIME((*abp), vap->va_backup_time, proc_is64);
		abp->actual.commonattr |= ATTR_CMN_BKUPTIME;
	}
	  
                                                                         
                                                                        
                
    
	if (alp->commonattr & ATTR_CMN_USERACCESS) {                           
		if (vp && !is_bulk) {
			if (vtype == VDIR) {
				if (vnode_authorize(vp, NULL,
				    KAUTH_VNODE_ACCESS | KAUTH_VNODE_ADD_FILE |
				    KAUTH_VNODE_ADD_SUBDIRECTORY |
				    KAUTH_VNODE_DELETE_CHILD, ctx) == 0) {
					perms |= W_OK;
				}

				if (vnode_authorize(vp, NULL,
				    KAUTH_VNODE_ACCESS |
				    KAUTH_VNODE_LIST_DIRECTORY, ctx) == 0) {
					perms |= R_OK;
				}

				if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS |
				    KAUTH_VNODE_SEARCH, ctx) == 0) {
					perms |= X_OK;
				}
			} else {
				if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS |
				    KAUTH_VNODE_WRITE_DATA, ctx) == 0) {
					perms |= W_OK;
				}

				if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_READ_DATA, ctx) == 0) {
					perms |= R_OK;
				}
				if (vnode_authorize(vp, NULL, KAUTH_VNODE_ACCESS | KAUTH_VNODE_EXECUTE, ctx) == 0) {
					perms |= X_OK;
				}
			}
		} else if (is_bulk &&
		    VATTR_IS_SUPPORTED(vap, va_user_access)) {
			perms = vap->va_user_access;
		}
	}
	if (alp->commonattr & ATTR_CMN_FNDRINFO) {
		size_t  fisize = 32;

		error = 0;
		if (vp && !is_bulk) {
			uio_t   auio;
			char    uio_buf[UIO_SIZEOF(1)];

			if ((auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE,
			    UIO_READ, uio_buf, sizeof(uio_buf))) == NULL) {
				error = ENOMEM;
				goto out;
			}
			uio_addiov(auio, CAST_USER_ADDR_T(abp->fixedcursor),
			    fisize);
			                                              
			error = vn_getxattr(vp, XATTR_FINDERINFO_NAME, auio,
			    &fisize, XATTR_NOSECURITY, ctx);
			uio_free(auio);

			  
                                            
                                                   
      
			if (error &&
			    (!return_valid || pack_invalid) &&
			    ((error == ENOATTR) || (error == ENOENT) ||
			    (error == ENOTSUP) || (error == EPERM))) {
				VFS_DEBUG(ctx, vp, "ATTRLIST - No system.finderinfo attribute, returning zeroes");
				bzero(abp->fixedcursor, 32);
				error = 0;
			}

			if (error == 0) {
				abp->fixedcursor += 32;
				abp->actual.commonattr |= ATTR_CMN_FNDRINFO;
			} else if (!return_valid) {
				goto out;
			} else {
				  
                                                
                                             
                                                
       
				error = 0;
			}
		} else if (VATTR_IS_SUPPORTED(vap, va_finderinfo)) {
			bcopy(&vap->va_finderinfo[0], abp->fixedcursor, fisize);
			abp->fixedcursor += fisize;
			abp->actual.commonattr |= ATTR_CMN_FNDRINFO;
		} else if (!return_valid || pack_invalid) {
			bzero(abp->fixedcursor, fisize);
			abp->fixedcursor += fisize;
		}
	}
	if (alp->commonattr & ATTR_CMN_OWNERID) {
		ATTR_PACK4((*abp), vap->va_uid);
		abp->actual.commonattr |= ATTR_CMN_OWNERID;
	}
	if (alp->commonattr & ATTR_CMN_GRPID) {
		ATTR_PACK4((*abp), vap->va_gid);
		abp->actual.commonattr |= ATTR_CMN_GRPID;
	}
	if (alp->commonattr & ATTR_CMN_ACCESSMASK) {
		ATTR_PACK4((*abp), vap->va_mode);
		abp->actual.commonattr |= ATTR_CMN_ACCESSMASK;
	}
	if (alp->commonattr & ATTR_CMN_FLAGS) {
		ATTR_PACK4((*abp), vap->va_flags);
		abp->actual.commonattr |= ATTR_CMN_FLAGS;
	}
	if (alp->commonattr & ATTR_CMN_GEN_COUNT) {
		if (VATTR_IS_SUPPORTED(vap, va_write_gencount)) {
			ATTR_PACK4((*abp), vap->va_write_gencount);
			abp->actual.commonattr |= ATTR_CMN_GEN_COUNT;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}

	if (alp->commonattr & ATTR_CMN_DOCUMENT_ID) {
		if (VATTR_IS_SUPPORTED(vap, va_document_id)) {
			ATTR_PACK4((*abp), vap->va_document_id);
			abp->actual.commonattr |= ATTR_CMN_DOCUMENT_ID;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	                                                                        
	if (alp->commonattr & ATTR_CMN_USERACCESS) {
#if CONFIG_MACF
		if (!is_bulk && vp) {
			  
                                                         
                                                           
                                                     
      
			if (perms & W_OK) {
				if (mac_vnode_check_access(ctx, vp, W_OK) != 0) {
					perms &= ~W_OK;
				}
			}
			if (perms & R_OK) {
				if (mac_vnode_check_access(ctx, vp, R_OK) != 0) {
					perms &= ~R_OK;
				}
			}
			if (perms & X_OK) {
				if (mac_vnode_check_access(ctx, vp, X_OK) != 0) {
					perms &= ~X_OK;
				}
			}
		}
#endif          
		VFS_DEBUG(ctx, vp, "ATTRLIST - granting perms %d", perms);
		if (!is_bulk && vp) {
			ATTR_PACK4((*abp), perms);
			abp->actual.commonattr |= ATTR_CMN_USERACCESS;
		} else if (is_bulk && VATTR_IS_SUPPORTED(vap, va_user_access)) {
			ATTR_PACK4((*abp), perms);
			abp->actual.commonattr |= ATTR_CMN_USERACCESS;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_EXTENDED_SECURITY) {
		if (VATTR_IS_SUPPORTED(vap, va_acl) && (vap->va_acl != NULL)) {
			struct kauth_filesec fsec;
			  
                                                                                  
      
			fsec.fsec_magic = KAUTH_FILESEC_MAGIC;
			fsec.fsec_owner = kauth_null_guid;
			fsec.fsec_group = kauth_null_guid;
			attrlist_pack_variable2(abp, &fsec, __offsetof(struct kauth_filesec, fsec_acl), vap->va_acl, KAUTH_ACL_COPYSIZE(vap->va_acl));
			abp->actual.commonattr |= ATTR_CMN_EXTENDED_SECURITY;
		} else if (!return_valid || pack_invalid) {
			attrlist_pack_variable(abp, NULL, 0);
		}
	}
	if (alp->commonattr & ATTR_CMN_UUID) {
		if (VATTR_IS_SUPPORTED(vap, va_uuuid)) {
			ATTR_PACK(abp, vap->va_uuuid);
			abp->actual.commonattr |= ATTR_CMN_UUID;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK(abp, kauth_null_guid);
		}
	}
	if (alp->commonattr & ATTR_CMN_GRPUUID) {
		if (VATTR_IS_SUPPORTED(vap, va_guuid)) {
			ATTR_PACK(abp, vap->va_guuid);
			abp->actual.commonattr |= ATTR_CMN_GRPUUID;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK(abp, kauth_null_guid);
		}
	}
	if (alp->commonattr & ATTR_CMN_FILEID) {
		ATTR_PACK8((*abp), vap->va_fileid);
		abp->actual.commonattr |= ATTR_CMN_FILEID;
	}
	if (alp->commonattr & ATTR_CMN_PARENTID) {
		ATTR_PACK8((*abp), vap->va_parentid);
		abp->actual.commonattr |= ATTR_CMN_PARENTID;
	}

	if (alp->commonattr & ATTR_CMN_FULLPATH) {
		if (vp) {
			attrlist_pack_string(abp, fullpathptr, fullpathlen);
			abp->actual.commonattr |= ATTR_CMN_FULLPATH;
		}
	}

	if (alp->commonattr & ATTR_CMN_ADDEDTIME) {
		if (VATTR_IS_SUPPORTED(vap, va_addedtime)) {
			ATTR_PACK_TIME((*abp), vap->va_addedtime, proc_is64);
			abp->actual.commonattr |= ATTR_CMN_ADDEDTIME;
		} else if (!return_valid || pack_invalid) {
			struct timespec zerotime = {.tv_sec = 0, .tv_nsec = 0};

			ATTR_PACK_TIME((*abp), zerotime, proc_is64);
		}
	}
	if (alp->commonattr & ATTR_CMN_DATA_PROTECT_FLAGS) {
		if (VATTR_IS_SUPPORTED(vap, va_dataprotect_class)) {
			ATTR_PACK4((*abp), vap->va_dataprotect_class);
			abp->actual.commonattr |= ATTR_CMN_DATA_PROTECT_FLAGS;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
out:
	return error;
}

static errno_t
attr_pack_dir(struct vnode *vp, struct attrlist *alp, struct _attrlist_buf *abp,
    struct vnode_attr *vap, int return_valid, int pack_invalid)
{
	if (alp->dirattr & ATTR_DIR_LINKCOUNT) {                             
		ATTR_PACK4((*abp), (uint32_t)vap->va_dirlinkcount);
		abp->actual.dirattr |= ATTR_DIR_LINKCOUNT;
	}
	if (alp->dirattr & ATTR_DIR_ENTRYCOUNT) {
		ATTR_PACK4((*abp), (uint32_t)vap->va_nchildren);
		abp->actual.dirattr |= ATTR_DIR_ENTRYCOUNT;
	}
	if (alp->dirattr & ATTR_DIR_MOUNTSTATUS) {
		uint32_t mntstat;

		if (vp) {
			  
                                                   
                                                   
                                                    
                                   
      
			if ((vp->v_flag & VROOT) || vnode_mountedhere(vp)) {
				mntstat = DIR_MNTSTATUS_MNTPOINT;
			} else {
				mntstat = 0;
			}
#if CONFIG_TRIGGERS
			  
                                          
                                       
      
			if (vp->v_resolve &&
			    !(vp->v_resolve->vr_flags & VNT_NO_DIRECT_MOUNT)) {
				mntstat |= DIR_MNTSTATUS_TRIGGER;
			}
#endif
		} else {
			mntstat = 0;
		}

		ATTR_PACK4((*abp), mntstat);
		abp->actual.dirattr |= ATTR_DIR_MOUNTSTATUS;
	}
	if (alp->dirattr & ATTR_DIR_ALLOCSIZE) {
		if (VATTR_IS_SUPPORTED(vap, va_data_alloc)) {
			ATTR_PACK8((*abp), vap->va_data_alloc);
			abp->actual.dirattr |= ATTR_DIR_ALLOCSIZE;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_alloc)) {
			ATTR_PACK8((*abp), vap->va_total_alloc);
			abp->actual.dirattr |= ATTR_DIR_ALLOCSIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}
	if (alp->dirattr & ATTR_DIR_IOBLOCKSIZE) {
		if (VATTR_IS_SUPPORTED(vap, va_iosize)) {
			ATTR_PACK4((*abp), vap->va_iosize);
			abp->actual.dirattr |= ATTR_DIR_IOBLOCKSIZE;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	  
                                                 
                                                      
                               
    
	if (alp->dirattr & ATTR_DIR_DATALENGTH) {
		if (VATTR_IS_SUPPORTED(vap, va_data_size)) {
			ATTR_PACK8((*abp), vap->va_data_size);
			abp->actual.dirattr |= ATTR_DIR_DATALENGTH;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_size)) {
			ATTR_PACK8((*abp), vap->va_total_size);
			abp->actual.dirattr |= ATTR_DIR_DATALENGTH;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}

	return 0;
}

  
                                                                           
                                                                       
                                                                            
                                                                        
   
static errno_t
attr_pack_file(vfs_context_t ctx, struct vnode *vp, struct attrlist *alp,
    struct _attrlist_buf *abp, struct vnode_attr *vap, int return_valid,
    int pack_invalid, int is_bulk)
{
	size_t  rsize = 0;
	uint64_t rlength = 0;
	uint64_t ralloc = 0;
	int error = 0;

	  
                                                               
                                                               
    
	if (vp && !is_bulk &&
	    (alp->fileattr & (ATTR_FILE_TOTALSIZE | ATTR_FILE_ALLOCSIZE |
	    ATTR_FILE_RSRCLENGTH | ATTR_FILE_RSRCALLOCSIZE))) {
		error = vn_getxattr(vp, XATTR_RESOURCEFORK_NAME, NULL,
		    &rsize, XATTR_NOSECURITY, ctx);
		if (error) {
			if ((error == ENOENT) || (error == ENOATTR) ||
			    (error == ENOTSUP) || (error == EPERM) ||
			    (error == EACCES)) {
				rsize = 0;
				error = 0;
			} else {
				goto out;
			}
		}
		rlength = rsize;

		if (alp->fileattr & (ATTR_FILE_RSRCALLOCSIZE |
		    ATTR_FILE_ALLOCSIZE)) {
			uint32_t  blksize;

			blksize = vp->v_mount->mnt_vfsstat.f_bsize;

			if (blksize == 0) {
				blksize = 512;
			}
			ralloc = roundup(rsize, blksize);
		}
	}

	if (alp->fileattr & ATTR_FILE_LINKCOUNT) {
		ATTR_PACK4((*abp), (uint32_t)vap->va_nlink);
		abp->actual.fileattr |= ATTR_FILE_LINKCOUNT;
	}
	  
                                                                          
                                                                                  
                                                                      
                                                                           
                                                                                    
                                                     
   
                                                                      
                                                                                  
                                                                                   
                                                                             
                                                                              
                                                               
    
	if (alp->fileattr & ATTR_FILE_TOTALSIZE) {
		if (!is_bulk) {
			uint64_t totalsize = rlength;

			if (VATTR_IS_SUPPORTED(vap, va_data_size)) {
				totalsize += vap->va_data_size;
			} else if (VATTR_IS_SUPPORTED(vap, va_total_size)) {
				totalsize += vap->va_total_size;
			}

			ATTR_PACK8((*abp), totalsize);
			abp->actual.fileattr |= ATTR_FILE_TOTALSIZE;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_size)) {
			ATTR_PACK8((*abp), vap->va_total_size);
			abp->actual.fileattr |= ATTR_FILE_TOTALSIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;

			ATTR_PACK8((*abp), zero_val);
		}
	}
	if (alp->fileattr & ATTR_FILE_ALLOCSIZE) {
		if (!is_bulk) {
			uint64_t totalalloc = ralloc;

			  
                                                            
                     
      
			if (VATTR_IS_SUPPORTED(vap, va_data_alloc)) {
				totalalloc += vap->va_data_alloc;
			} else if (VATTR_IS_SUPPORTED(vap, va_total_alloc)) {
				totalalloc += vap->va_total_alloc;
			}

			ATTR_PACK8((*abp), totalalloc);
			abp->actual.fileattr |= ATTR_FILE_ALLOCSIZE;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_alloc)) {
			ATTR_PACK8((*abp), vap->va_total_alloc);
			abp->actual.fileattr |= ATTR_FILE_ALLOCSIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;

			ATTR_PACK8((*abp), zero_val);
		}
	}
	if (alp->fileattr & ATTR_FILE_IOBLOCKSIZE) {
		if (VATTR_IS_SUPPORTED(vap, va_iosize)) {
			ATTR_PACK4((*abp), vap->va_iosize);
			abp->actual.fileattr |= ATTR_FILE_IOBLOCKSIZE;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	if (alp->fileattr & ATTR_FILE_CLUMPSIZE) {
		if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);                                   
			abp->actual.fileattr |= ATTR_FILE_CLUMPSIZE;
		}
	}
	if (alp->fileattr & ATTR_FILE_DEVTYPE) {
		if (vp && (vp->v_type == VCHR || vp->v_type == VBLK)) {
			uint32_t dev;

			if (vp->v_specinfo != NULL) {
				dev = vp->v_specinfo->si_rdev;
			} else if (VATTR_IS_SUPPORTED(vap, va_rdev)) {
				dev = vap->va_rdev;
			} else {
				dev = 0;
			}
			ATTR_PACK4((*abp), dev);
			abp->actual.fileattr |= ATTR_FILE_DEVTYPE;
		} else if (vp) {
			ATTR_PACK4((*abp), 0);
			abp->actual.fileattr |= ATTR_FILE_DEVTYPE;
		} else if (VATTR_IS_SUPPORTED(vap, va_rdev)) {
			ATTR_PACK4((*abp), vap->va_rdev);
			abp->actual.fileattr |= ATTR_FILE_DEVTYPE;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}
	  
                                                 
                                                      
                               
    
	if (alp->fileattr & ATTR_FILE_DATALENGTH) {
		if (VATTR_IS_SUPPORTED(vap, va_data_size)) {
			ATTR_PACK8((*abp), vap->va_data_size);
			abp->actual.fileattr |= ATTR_FILE_DATALENGTH;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_size)) {
			ATTR_PACK8((*abp), vap->va_total_size);
			abp->actual.fileattr |= ATTR_FILE_DATALENGTH;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}
	if (alp->fileattr & ATTR_FILE_DATAALLOCSIZE) {
		if (VATTR_IS_SUPPORTED(vap, va_data_alloc)) {
			ATTR_PACK8((*abp), vap->va_data_alloc);
			abp->actual.fileattr |= ATTR_FILE_DATAALLOCSIZE;
		} else if (VATTR_IS_SUPPORTED(vap, va_total_alloc)) {
			ATTR_PACK8((*abp), vap->va_total_alloc);
			abp->actual.fileattr |= ATTR_FILE_DATAALLOCSIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}
	                                                         
	if (alp->fileattr & ATTR_FILE_RSRCLENGTH) {
		if (!is_bulk) {
			ATTR_PACK8((*abp), rlength);
			abp->actual.fileattr |= ATTR_FILE_RSRCLENGTH;
		} else if (VATTR_IS_SUPPORTED(vap, va_rsrc_length)) {
			ATTR_PACK8((*abp), vap->va_rsrc_length);
			abp->actual.fileattr |= ATTR_FILE_RSRCLENGTH;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;

			ATTR_PACK8((*abp), zero_val);
		}
	}
	if (alp->fileattr & ATTR_FILE_RSRCALLOCSIZE) {
		if (!is_bulk) {
			ATTR_PACK8((*abp), ralloc);
			abp->actual.fileattr |= ATTR_FILE_RSRCALLOCSIZE;
		} else if (VATTR_IS_SUPPORTED(vap, va_rsrc_alloc)) {
			ATTR_PACK8((*abp), vap->va_rsrc_alloc);
			abp->actual.fileattr |= ATTR_FILE_RSRCALLOCSIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;

			ATTR_PACK8((*abp), zero_val);
		}
	}
out:
	return error;
}

  
                                               
                                                         
                                                       
                                                                     
             
   
static errno_t
attr_pack_common_extended(mount_t mp, struct vnode *vp, struct attrlist *alp,
    struct _attrlist_buf *abp, const char *relpathptr, ssize_t relpathlen,
    const char *REALpathptr, ssize_t REALpathlen,
    struct vnode_attr *vap, int return_valid, int pack_invalid)
{
	if (vp && (alp->forkattr & ATTR_CMNEXT_RELPATH)) {
		attrlist_pack_string(abp, relpathptr, relpathlen);
		abp->actual.forkattr |= ATTR_CMNEXT_RELPATH;
	}

	if (alp->forkattr & ATTR_CMNEXT_PRIVATESIZE) {
		if (VATTR_IS_SUPPORTED(vap, va_private_size)) {
			ATTR_PACK8((*abp), vap->va_private_size);
			abp->actual.forkattr |= ATTR_CMNEXT_PRIVATESIZE;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}

	if (alp->forkattr & ATTR_CMNEXT_LINKID) {
		uint64_t linkid;

		if (VATTR_IS_SUPPORTED(vap, va_linkid)) {
			linkid = vap->va_linkid;
		} else {
			linkid = vap->va_fileid;
		}

		ATTR_PACK8((*abp), linkid);
		abp->actual.forkattr |= ATTR_CMNEXT_LINKID;
	}

	if (vp && (alp->forkattr & ATTR_CMNEXT_NOFIRMLINKPATH)) {
		attrlist_pack_string(abp, REALpathptr, REALpathlen);
		abp->actual.forkattr |= ATTR_CMNEXT_NOFIRMLINKPATH;
	}

	if (alp->forkattr & ATTR_CMNEXT_REALDEVID) {
		if (mp) {
			ATTR_PACK4((*abp),
			    mp->mnt_vfsstat.f_fsid.val[0]);
			abp->actual.forkattr |= ATTR_CMNEXT_REALDEVID;
		} else if (vp) {
			ATTR_PACK4((*abp),
			    vp->v_mount->mnt_vfsstat.f_fsid.val[0]);
			abp->actual.forkattr |= ATTR_CMNEXT_REALDEVID;
		} else if (VATTR_IS_SUPPORTED(vap, va_fsid)) {
			ATTR_PACK4((*abp), vap->va_fsid);
			abp->actual.forkattr |= ATTR_CMN_DEVID;
		} else if (!return_valid || pack_invalid) {
			ATTR_PACK4((*abp), 0);
		}
	}

	if (alp->forkattr & ATTR_CMNEXT_REALFSID) {
		if (mp) {
			ATTR_PACK8((*abp),
			    mp->mnt_vfsstat.f_fsid);
			abp->actual.forkattr |= ATTR_CMNEXT_REALFSID;
		} else if (vp) {
			ATTR_PACK8((*abp),
			    vp->v_mount->mnt_vfsstat.f_fsid);
			abp->actual.forkattr |= ATTR_CMNEXT_REALFSID;
		} else if (VATTR_IS_SUPPORTED(vap, va_fsid64)) {
			ATTR_PACK8((*abp), vap->va_fsid64);
			abp->actual.forkattr |= ATTR_CMN_FSID;
		} else if (!return_valid || pack_invalid) {
			fsid_t fsid = {{0}};

			ATTR_PACK8((*abp), fsid);
		}
	}

	if (alp->forkattr & ATTR_CMNEXT_CLONEID) {
		if (VATTR_IS_SUPPORTED(vap, va_clone_id)) {
			ATTR_PACK8((*abp), vap->va_clone_id);
			abp->actual.forkattr |= ATTR_CMNEXT_CLONEID;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}

	if (alp->forkattr & ATTR_CMNEXT_EXT_FLAGS) {
		if (VATTR_IS_SUPPORTED(vap, va_extflags)) {
			ATTR_PACK8((*abp), vap->va_extflags);
			abp->actual.forkattr |= ATTR_CMNEXT_EXT_FLAGS;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}

	if (alp->forkattr & ATTR_CMNEXT_RECURSIVE_GENCOUNT) {
		if (VATTR_IS_SUPPORTED(vap, va_recursive_gencount)) {
			ATTR_PACK8((*abp), vap->va_recursive_gencount);
			abp->actual.forkattr |= ATTR_CMNEXT_RECURSIVE_GENCOUNT;
		} else if (!return_valid || pack_invalid) {
			uint64_t zero_val = 0;
			ATTR_PACK8((*abp), zero_val);
		}
	}

	return 0;
}

static void
vattr_get_alt_data(vnode_t vp, struct attrlist *alp, struct vnode_attr *vap,
    int return_valid, int is_bulk,
#if !CONFIG_FIRMLINKS
    __unused
#endif
    int is_realdev, vfs_context_t ctx)
{
	  
                                        
                                                              
    
	if ((alp->commonattr &
	    (ATTR_CMN_OBJID | ATTR_CMN_OBJPERMANENTID | ATTR_CMN_FILEID)) &&
	    !VATTR_IS_SUPPORTED(vap, va_linkid)) {
		                           
		VATTR_CLEAR_ACTIVE(vap, va_linkid);
	}

	  
                                                                               
                                 
    
	if ((alp->commonattr & ATTR_CMN_FSID) && !VATTR_IS_SUPPORTED(vap, va_fsid64)) {
		VATTR_CLEAR_ACTIVE(vap, va_fsid64);
	}

	                   
	if ((alp->commonattr & ATTR_CMN_FSID) && !VATTR_IS_SUPPORTED(vap, va_fsid)) {
		VATTR_CLEAR_ACTIVE(vap, va_fsid);
	}

	                                         
	if ((alp->commonattr & ATTR_CMN_DEVID) && !VATTR_IS_SUPPORTED(vap, va_fsid)) {
		VATTR_CLEAR_ACTIVE(vap, va_fsid);
	}


	  
                                                       
                                                      
    
	if ((alp->commonattr & (ATTR_CMN_PAROBJID | ATTR_CMN_PARENTID)) && vp) {
		vnode_t dvp;

#if CONFIG_FIRMLINKS
		                                                                             
		if (!is_realdev && (vp->v_flag & VFMLINKTARGET) && ((dvp = vp->v_fmlink) != NULL) && (vnode_get(dvp) == 0)) {
			struct vnode_attr lva;

			VATTR_INIT(&lva);
			VATTR_WANTED(&lva, va_parentid);
			VATTR_WANTED(&lva, va_fsid);
			if (vnode_getattr(dvp, &lva, ctx) == 0 &&
			    VATTR_IS_SUPPORTED(&lva, va_parentid) &&
			    VATTR_IS_SUPPORTED(&lva, va_fsid) &&
			    (lva.va_fsid == (uint32_t)vp->v_mount->mnt_vfsstat.f_fsid.val[0])) {
				vap->va_parentid = lva.va_parentid;
				VATTR_SET_SUPPORTED(vap, va_parentid);
			}
			vnode_put(dvp);
		} else
#endif                       
		if (!VATTR_IS_SUPPORTED(vap, va_parentid) && !is_bulk) {
			if ((dvp = vnode_getparent(vp)) != NULLVP) {
				struct vnode_attr lva;

				VATTR_INIT(&lva);
				VATTR_WANTED(&lva, va_fileid);
				if (vnode_getattr(dvp, &lva, ctx) == 0 &&
				    VATTR_IS_SUPPORTED(vap, va_fileid)) {
					vap->va_parentid = lva.va_fileid;
					VATTR_SET_SUPPORTED(vap, va_parentid);
				}
				vnode_put(dvp);
			}
		}
	}

	  
                                                
    
	if ((alp->fileattr & ATTR_FILE_DATALENGTH) &&
	    !VATTR_IS_SUPPORTED(vap, va_data_size)) {
		VATTR_CLEAR_ACTIVE(vap, va_data_size);
	}

	if ((alp->fileattr & ATTR_FILE_DATAALLOCSIZE) &&
	    !VATTR_IS_SUPPORTED(vap, va_data_alloc)) {
		VATTR_CLEAR_ACTIVE(vap, va_data_alloc);
	}

	  
                                               
    
	if ((alp->commonattr & ATTR_CMN_SCRIPT) &&
	    !VATTR_IS_SUPPORTED(vap, va_encoding) && !return_valid) {
		VATTR_RETURN(vap, va_encoding,
		    0x7e                              );
	}

	  
                                                            
                
    
	if ((alp->commonattr & ATTR_CMN_NAME) &&
	    !VATTR_IS_SUPPORTED(vap, va_name)) {
		VATTR_CLEAR_ACTIVE(vap, va_name);
	}

	                                                            
	if ((alp->dirattr & ATTR_DIR_LINKCOUNT) &&
	    !VATTR_IS_SUPPORTED(vap, va_dirlinkcount)) {
		VATTR_RETURN(vap, va_dirlinkcount, 1);
	}
}

struct _attrlist_paths {
	char *fullpathptr;
	ssize_t *fullpathlenp;
	char *relpathptr;
	ssize_t *relpathlenp;
	char *REALpathptr;
	ssize_t *REALpathlenp;
};

static errno_t
calc_varsize(vnode_t vp, struct attrlist *alp, struct vnode_attr *vap,
    ssize_t *varsizep, struct _attrlist_paths *pathsp, const char **vnamep,
    const char **cnpp, ssize_t *cnlp)
{
	int error = 0;

	*varsizep = 0;                   
	                                                           
	if (alp->commonattr & ATTR_CMN_NAME) {
		if (VATTR_IS_SUPPORTED(vap, va_name)) {
			vap->va_name[MAXPATHLEN - 1] = '\0';                                  
			*cnpp = vap->va_name;
			*cnlp = strlen(*cnpp);
		} else if (vp) {
			                                                 
			if (vnode_isvroot(vp)) {
				if (vp->v_mount->mnt_vfsstat.f_mntonname[1] == 0x00 &&
				    vp->v_mount->mnt_vfsstat.f_mntonname[0] == '/') {
					                                                         
                                                                 
                                                                   
                                                                   
                                   
        
					*cnpp = *vnamep = vnode_getname(vp);
					if (*cnpp == NULL) {
						                          
						*cnpp = &vp->v_mount->mnt_vfsstat.f_mntonname[0];
					}
					*cnlp = strlen(*cnpp);
				} else {
					getattrlist_findnamecomp(vp->v_mount->mnt_vfsstat.f_mntonname, cnpp, cnlp);
				}
			} else {
				*cnpp = *vnamep = vnode_getname(vp);
				*cnlp = 0;
				if (*cnpp != NULL) {
					*cnlp = strlen(*cnpp);
				}
			}
		} else {
			*cnlp = 0;
		}
		*varsizep += roundup(*cnlp + 1, 4);
	}

	  
                                                                                         
                                                                                  
    
	if (vp && (alp->commonattr & ATTR_CMN_FULLPATH)) {
		int len = MAXPATHLEN;
		int err;

		                                                                    
		err = build_path(vp, pathsp->fullpathptr, len, &len, 0, vfs_context_current());
		if (err) {
			error = err;
			goto out;
		}
		if (pathsp->fullpathptr) {
			*(pathsp->fullpathlenp) = strlen(pathsp->fullpathptr);
		} else {
			*(pathsp->fullpathlenp) = 0;
		}
		*varsizep += roundup(((*(pathsp->fullpathlenp)) + 1), 4);
	}

	  
                                              
    
	if (vp && (alp->forkattr & ATTR_CMNEXT_RELPATH)) {
		int len;
		int err;

		                                                                    
		err = build_path(vp, pathsp->relpathptr, MAXPATHLEN, &len, BUILDPATH_VOLUME_RELATIVE, vfs_context_current());
		if (err) {
			error = err;
			goto out;
		}

		                              
		*(pathsp->relpathlenp) = len - 1;
		*varsizep += roundup(len, 4);
	}

	  
                                                   
    
	if (vp && (alp->forkattr & ATTR_CMNEXT_NOFIRMLINKPATH)) {
		int len;
		int err;

		                                                                    
		err = build_path(vp, pathsp->REALpathptr, MAXPATHLEN, &len, BUILDPATH_NO_FIRMLINK, vfs_context_current());
		if (err) {
			error = err;
			goto out;
		}

		                              
		*(pathsp->REALpathlenp) = len - 1;
		*varsizep += roundup(len, 4);
	}

	  
                                                                     
   
                                                                       
                          
    
	if ((alp->commonattr & ATTR_CMN_EXTENDED_SECURITY) &&
	    VATTR_IS_SUPPORTED(vap, va_acl) &&
	    (vap->va_acl != NULL)) {
		  
                                                                                  
                                  
     
		if (vap->va_acl->acl_entrycount == KAUTH_FILESEC_NOACL) {
			*varsizep += roundup((KAUTH_FILESEC_SIZE(0)), 4);
		} else {
			*varsizep += roundup((KAUTH_FILESEC_SIZE(vap->va_acl->acl_entrycount)), 4);
		}
	}

out:
	return error;
}

static errno_t
vfs_attr_pack_internal(mount_t mp, vnode_t vp, uio_t auio, struct attrlist *alp,
    uint64_t options, struct vnode_attr *vap, __unused void *fndesc,
    vfs_context_t ctx, int is_bulk, enum vtype vtype, ssize_t fixedsize)
{
	struct _attrlist_buf ab;
	struct _attrlist_paths apaths = {.fullpathptr = NULL, .fullpathlenp = NULL,
		                         .relpathptr = NULL, .relpathlenp = NULL,
		                         .REALpathptr = NULL, .REALpathlenp = NULL};
	ssize_t buf_size;
	size_t copy_size;
	ssize_t varsize;
	const char *vname = NULL;
	const char *cnp;
	ssize_t cnl;
	char *fullpathptr;
	ssize_t fullpathlen;
	char *relpathptr;
	ssize_t relpathlen;
	char *REALpathptr;
	ssize_t REALpathlen;
	int error;
	int proc_is64;
	int return_valid;
	int pack_invalid;
	int is_realdev;
	int alloc_local_buf;
	const int use_fork = options & FSOPT_ATTR_CMN_EXTENDED;

	proc_is64 = proc_is64bit(vfs_context_proc(ctx));
	ab.base = NULL;
	cnp = "unknown";
	cnl = 0;
	fullpathptr = NULL;
	fullpathlen = 0;
	relpathptr = NULL;
	relpathlen = 0;
	REALpathptr = NULL;
	REALpathlen = 0;
	error = 0;
	alloc_local_buf = 0;

	buf_size = (ssize_t)uio_resid(auio);
	if ((buf_size <= 0) || (uio_iovcnt(auio) > 1)) {
		return EINVAL;
	}

	copy_size = 0;
	                                         
	return_valid = (alp->commonattr & ATTR_CMN_RETURNED_ATTRS) ? 1 : 0;
	pack_invalid = (options & FSOPT_PACK_INVAL_ATTRS) ? 1 : 0;
	is_realdev = options & FSOPT_RETURN_REALDEV ? 1 : 0;

	if (pack_invalid) {
		                                               
		bcopy(&(alp->commonattr), &ab.valid, sizeof(attribute_set_t));
	}

	                                                              
	if (vap->va_active &&
	    (!VATTR_ALL_SUPPORTED(vap)
#if CONFIG_FIRMLINKS
	                                                                                   
	    ||
	    (!is_realdev && vp && (vp->v_flag & VFMLINKTARGET) && vp->v_fmlink &&
	    (alp->commonattr & (ATTR_CMN_PAROBJID | ATTR_CMN_PARENTID)))
#endif
	    )) {
		                                                                        
		vattr_get_alt_data(vp, alp, vap, return_valid, is_bulk, is_realdev,
		    ctx);

		                 
		if (!VATTR_ALL_SUPPORTED(vap)) {
			if (return_valid && pack_invalid) {
				                                           
				getattrlist_fixupattrs(&ab.valid, vap, use_fork);

				                                           
				vap->va_supported = vap->va_active;
			} else if (return_valid) {
				                                     
				getattrlist_fixupattrs(
					(attribute_set_t *)&(alp->commonattr), vap, use_fork);
			} else {
				error = EINVAL;
			}
		}

		if (error) {
			goto out;
		}
	}

	                                                                 
	if (vp && (alp->commonattr & (ATTR_CMN_FULLPATH))) {
		fullpathptr = (char*) zalloc_flags(ZV_NAMEI, Z_WAITOK | Z_ZERO);
		apaths.fullpathptr = fullpathptr;
		apaths.fullpathlenp = &fullpathlen;
	}

	                                                                          
	if (vp && use_fork) {
		if (alp->forkattr & (ATTR_CMNEXT_RELPATH)) {
			relpathptr = (char*) zalloc_flags(ZV_NAMEI, Z_WAITOK | Z_ZERO);
			apaths.relpathptr = relpathptr;
			apaths.relpathlenp = &relpathlen;
		}

		if (alp->forkattr & (ATTR_CMNEXT_NOFIRMLINKPATH)) {
			REALpathptr = (char*) zalloc_flags(ZV_NAMEI, Z_WAITOK | Z_ZERO);
			apaths.REALpathptr = REALpathptr;
			apaths.REALpathlenp = &REALpathlen;
		}
	}

	  
                                        
    
	error = calc_varsize(vp, alp, vap, &varsize, &apaths, &vname, &cnp, &cnl);
	if (error) {
		goto out;
	}

	  
                                                   
   
                                                                                
                                                                                
                                                                
    
	ab.allocated = fixedsize + varsize;
	                                                               
	if (((size_t)ab.allocated) > ATTR_MAX_BUFFER) {
		error = ENOMEM;
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: buffer size too large (%d limit %d)", ab.allocated, ATTR_MAX_BUFFER);
		goto out;
	}

	  
                                                                   
               
    
	if (is_bulk) {
		if (buf_size < ab.allocated) {
			goto out;
		} else {
			uint32_t newlen;

			newlen = (ab.allocated + 7) & ~0x07;
			                                              
			if (newlen <= (uint32_t)buf_size) {
				ab.allocated = newlen;
			}
		}
	}

	  
                                                                   
                   
    
	if (uio_isuserspace(auio) || (buf_size < ab.allocated)) {
		ab.base = kheap_alloc(KHEAP_TEMP, ab.allocated, Z_ZERO | Z_WAITOK);
		alloc_local_buf = 1;
	} else {
		  
                                                     
                                                   
                                                       
             
    
                                                     
                                                         
                                                         
                   
    
                                                    
                                                      
                                               
                   
     
		ab.base = (char *)uio_curriovbase(auio) +
		    ((ssize_t)uio_curriovlen(auio) - buf_size);
		bzero(ab.base, ab.allocated);
	}

	if (ab.base == NULL) {
		error = ENOMEM;
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: could not allocate %d for copy buffer", ab.allocated);
		goto out;
	}


	                                      
	if (alp->commonattr & ATTR_CMN_ACCESSMASK) {
		if (vp) {
			switch (vp->v_type) {
			case VREG:
				vap->va_mode |= S_IFREG;
				break;
			case VDIR:
				vap->va_mode |= S_IFDIR;
				break;
			case VBLK:
				vap->va_mode |= S_IFBLK;
				break;
			case VCHR:
				vap->va_mode |= S_IFCHR;
				break;
			case VLNK:
				vap->va_mode |= S_IFLNK;
				break;
			case VSOCK:
				vap->va_mode |= S_IFSOCK;
				break;
			case VFIFO:
				vap->va_mode |= S_IFIFO;
				break;
			default:
				error = EBADF;
				goto out;
			}
		}
	}

	  
                                             
    
	ab.fixedcursor = ab.base + sizeof(uint32_t);
	if (return_valid) {
		ab.fixedcursor += sizeof(attribute_set_t);
		bzero(&ab.actual, sizeof(ab.actual));
	}
	ab.varcursor = ab.base + fixedsize;
	ab.needed = ab.allocated;

	                                                                      
	error = attr_pack_common(ctx, (options & FSOPT_RETURN_REALDEV ? mp : NULL),
	    vp, alp, &ab, vap, proc_is64, cnp, cnl, fullpathptr, fullpathlen,
	    return_valid, pack_invalid, vtype, is_bulk);

	                                                                      
	if (!error && alp->dirattr && (vtype == VDIR)) {
		error = attr_pack_dir(vp, alp, &ab, vap, return_valid, pack_invalid);
	}

	                                                                      
	if (!error && alp->fileattr && (vtype != VDIR)) {
		error = attr_pack_file(ctx, vp, alp, &ab, vap, return_valid,
		    pack_invalid, is_bulk);
	}

	                                                                        
	if (!error && use_fork) {
		error = attr_pack_common_extended(mp, vp, alp, &ab, relpathptr, relpathlen,
		    REALpathptr, REALpathlen, vap, return_valid, pack_invalid);
	}

	if (error) {
		goto out;
	}

	                
	if (!return_valid && (ab.fixedcursor - ab.base) != fixedsize) {
		panic("packed field size mismatch; allocated %ld but packed %ld for common %08x vol %08x",
		    fixedsize, (long) (ab.fixedcursor - ab.base), alp->commonattr, alp->volattr);
	}
	if (!return_valid && ab.varcursor != (ab.base + ab.needed)) {
		panic("packed variable field size mismatch; used %ld but expected %ld", (long) (ab.varcursor - ab.base), ab.needed);
	}

	  
                                                                                     
                                                                                       
                                                                                        
                                                                     
    
	*(uint32_t *)ab.base = (options & FSOPT_REPORT_FULLSIZE) ? (uint32_t)ab.needed : (uint32_t)lmin(ab.allocated, ab.needed);

	                                               
	if (return_valid) {
		ab.actual.commonattr |= ATTR_CMN_RETURNED_ATTRS;
		if (pack_invalid) {
			                                               
			ab.actual.commonattr &= ab.valid.commonattr;
			ab.actual.dirattr &= ab.valid.dirattr;
			ab.actual.fileattr &= ab.valid.fileattr;
		}
		bcopy(&ab.actual, ab.base + sizeof(uint32_t), sizeof(ab.actual));
	}

	copy_size = lmin(buf_size, ab.allocated);

	                                                                   
	if (alloc_local_buf) {
		error = uiomove(ab.base, (int)copy_size, auio);
	} else {
		off_t orig_offset = uio_offset(auio);

		  
                                                   
                                                
                                                 
                                     
    
                                                     
                                                      
                                                  
                                                 
                          
     
		uio_setresid(auio, buf_size - copy_size);
		uio_setoffset(auio, orig_offset + (off_t)copy_size);
	}

out:
	if (vname) {
		vnode_putname(vname);
	}
	if (fullpathptr) {
		zfree(ZV_NAMEI, fullpathptr);
	}
	if (relpathptr) {
		zfree(ZV_NAMEI, relpathptr);
	}
	if (REALpathptr) {
		zfree(ZV_NAMEI, REALpathptr);
	}
	if (alloc_local_buf) {
		kheap_free(KHEAP_TEMP, ab.base, ab.allocated);
	}
	return error;
}

errno_t
vfs_attr_pack_ext(mount_t mp, vnode_t vp, uio_t uio, struct attrlist *alp, uint64_t options,
    struct vnode_attr *vap, __unused void *fndesc, vfs_context_t ctx)
{
	int error;
	ssize_t fixedsize;
	uint64_t orig_active;
	struct attrlist orig_al;
	enum vtype v_type;

	if (vp) {
		v_type = vnode_vtype(vp);
	} else {
		v_type = vap->va_objtype;
	}

	orig_al = *alp;
	orig_active = vap->va_active;
	vap->va_active = 0;

	error = getattrlist_setupvattr_all(alp, vap, v_type, &fixedsize,
	    proc_is64bit(vfs_context_proc(ctx)), options & FSOPT_ATTR_CMN_EXTENDED);

	if (error) {
		VFS_DEBUG(ctx, vp,
		    "ATTRLIST - ERROR: setup for request failed");
		goto out;
	}

	error = vfs_attr_pack_internal(mp, vp, uio, alp,
	    options | FSOPT_REPORT_FULLSIZE, vap, NULL, ctx, 1, v_type,
	    fixedsize);

	VATTR_CLEAR_SUPPORTED_ALL(vap);
	vap->va_active = orig_active;
	*alp = orig_al;
out:
	return error;
}

errno_t
vfs_attr_pack(vnode_t vp, uio_t uio, struct attrlist *alp, uint64_t options,
    struct vnode_attr *vap, __unused void *fndesc, vfs_context_t ctx)
{
	return vfs_attr_pack_ext(NULL, vp, uio, alp, options, vap, fndesc, ctx);
}

  
                                                          
  
                                                                              
                                                                         
                                                                                
                                                                      
  
   
static int
getattrlist_internal(vfs_context_t ctx, vnode_t vp, struct attrlist  *alp,
    user_addr_t attributeBuffer, size_t bufferSize, uint64_t options,
    enum uio_seg segflg, char* authoritative_name, struct ucred *file_cred)
{
	struct vnode_attr *va;
	kauth_action_t  action;
	ssize_t         fixedsize;
	char            *va_name;
	int             proc_is64;
	int             error;
	int             return_valid;
	int             pack_invalid;
	int             vtype = 0;
	uio_t           auio;
	char uio_buf[UIO_SIZEOF(1)];
	                                                                       
	const int use_fork = (options & FSOPT_ATTR_CMN_EXTENDED) != 0;

	if (bufferSize < sizeof(uint32_t)) {
		return ERANGE;
	}

	proc_is64 = proc_is64bit(vfs_context_proc(ctx));

	if (segflg == UIO_USERSPACE) {
		if (proc_is64) {
			segflg = UIO_USERSPACE64;
		} else {
			segflg = UIO_USERSPACE32;
		}
	}
	auio = uio_createwithbuffer(1, 0, segflg, UIO_READ,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, attributeBuffer, bufferSize);

	va = kheap_alloc(KHEAP_TEMP, sizeof(struct vnode_attr), Z_WAITOK);
	VATTR_INIT(va);
	va_name = NULL;

	if (alp->bitmapcount != ATTR_BIT_MAP_COUNT) {
		error = EINVAL;
		goto out;
	}

	VFS_DEBUG(ctx, vp, "%p  ATTRLIST - %s request common %08x vol %08x file %08x dir %08x fork %08x %sfollow on '%s'",
	    vp, vfs_context_proc(ctx)->p_comm, alp->commonattr, alp->volattr, alp->fileattr, alp->dirattr, alp->forkattr,
	    (options & FSOPT_NOFOLLOW) ? "no":"", vp->v_name);

#if CONFIG_MACF
	error = mac_vnode_check_getattrlist(ctx, vp, alp, options);
	if (error) {
		goto out;
	}
#endif          

	  
                                                                   
   
                                                                       
                                         
    
	if (alp->volattr) {
		if (alp->fileattr || alp->dirattr || (alp->forkattr && !use_fork)) {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: mixed volume/file/directory attributes");
			goto out;
		}
		                                     
		error = getvolattrlist(ctx, vp, alp, attributeBuffer,
		    bufferSize, options, segflg, proc_is64);
		goto out;
	}

	  
                                                              
                                                       
                           
    
	if ((alp->commonattr & (ATTR_CMN_GEN_COUNT | ATTR_CMN_DOCUMENT_ID)) &&
	    !(options & FSOPT_ATTR_CMN_EXTENDED)) {
		error = EINVAL;
		goto out;
	}

	                                                                       
	if (!(use_fork) && (alp->forkattr & ATTR_CMNEXT_VALIDMASK)) {
		error = EINVAL;
		goto out;
	}

	                                                                   
	if ((options & FSOPT_ATTR_CMN_EXTENDED) && (alp->forkattr & (ATTR_FORK_VALIDMASK))) {
		error = EINVAL;
		goto out;
	}

	                                         
	return_valid = (alp->commonattr & ATTR_CMN_RETURNED_ATTRS) ? 1 : 0;
	pack_invalid = (options & FSOPT_PACK_INVAL_ATTRS) ? 1 : 0;
	if (pack_invalid) {
		                                                             
		if (!return_valid || (alp->forkattr && !use_fork)) {
			error = EINVAL;
			goto out;
		}
		                                                 
		bzero(va, sizeof(*va));
	}

	                                                                              
                                                          
    
	vtype = vp->v_type;

	  
                                                  
    
	if ((error = getattrlist_setupvattr(alp, va, &fixedsize, &action, proc_is64, (vtype == VDIR), use_fork)) != 0) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: setup for request failed");
		goto out;
	}
	if ((error = vnode_authorize(vp, NULL, action, ctx)) != 0) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: authorisation failed/denied");
		goto out;
	}


	if (va->va_active != 0) {
		uint64_t va_active = va->va_active;

		  
                                                                        
     
		if (VATTR_IS_ACTIVE(va, va_name)) {
			va_name = zalloc(ZV_NAMEI);
			  
                                                         
     
                                                                                
                                                                             
                                                                                
                             
      
			if (authoritative_name) {
				                               
				VATTR_CLEAR_ACTIVE(va, va_name);
				strlcpy(va_name, authoritative_name, MAXPATHLEN);
			}
		}

		va->va_name = authoritative_name ? NULL : va_name;

		if (options & FSOPT_RETURN_REALDEV) {
			va->va_vaflags |= VA_REALFSID;
		}

		  
                         
     
		if ((error = vnode_getattr(vp, va, ctx)) != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: filesystem returned %d", error);
			goto out;
		}
#if CONFIG_MACF
		  
                                                      
                                                          
                                                          
                                                        
                                                         
                                                          
                                                   
               
     
		error = mac_vnode_check_getattr(ctx, file_cred, vp, va);
		if (error) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: MAC framework returned %d", error);
			goto out;
		}
#else
		(void)file_cred;
#endif

		  
                                                        
                                                         
                                                   
    
                                                        
                                                  
     
		va->va_active = va_active;
		if (authoritative_name && va_name) {
			VATTR_SET_ACTIVE(va, va_name);
			if (!(VATTR_IS_SUPPORTED(va, va_name))) {
				VATTR_SET_SUPPORTED(va, va_name);
			}
		}
		va->va_name = va_name;
	}

	error = vfs_attr_pack_internal(vp->v_mount, vp, auio, alp, options, va, NULL, ctx,
	    0, vtype, fixedsize);

out:
	if (va_name) {
		zfree(ZV_NAMEI, va_name);
	}
	if (VATTR_IS_SUPPORTED(va, va_acl) && (va->va_acl != NULL)) {
		kauth_acl_free(va->va_acl);
	}
	kheap_free(KHEAP_TEMP, va, sizeof(struct vnode_attr));

	VFS_DEBUG(ctx, vp, "ATTRLIST - returning %d", error);
	return error;
}

int
fgetattrlist(proc_t p, struct fgetattrlist_args *uap, __unused int32_t *retval)
{
	vfs_context_t ctx;
	vnode_t vp;
	int error;
	struct attrlist al;
	struct fileproc *fp;

	ctx = vfs_context_current();
	vp = NULL;
	fp = NULL;
	error = 0;

	if ((error = fp_get_ftype(p, uap->fd, DTYPE_VNODE, EINVAL, &fp)) != 0) {
		return error;
	}
	vp = (struct vnode *)fp->fp_glob->fg_data;

	if ((error = vnode_getwithref(vp)) != 0) {
		goto out;
	}

	  
                                
    
	error = copyin(uap->alist, &al, sizeof(al));
	if (error) {
		goto out_vnode_put;
	}

	                                        
	error = getattrlist_internal(ctx, vp, &al, uap->attributeBuffer,
	    uap->bufferSize, uap->options,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : \
	    UIO_USERSPACE32), NULL,
	    fp->fp_glob->fg_cred);

out_vnode_put:
	vnode_put(vp);
out:
	fp_drop(p, uap->fd, fp, 0);

	return error;
}

static int
getattrlistat_internal(vfs_context_t ctx, user_addr_t path,
    struct attrlist *alp, user_addr_t attributeBuffer, size_t bufferSize,
    uint64_t options, enum uio_seg segflg, enum uio_seg pathsegflg, int fd)
{
	struct nameidata nd;
	vnode_t vp;
	int32_t nameiflags;
	int error;

	nameiflags = 0;
	  
                     
    
	if (!(options & FSOPT_NOFOLLOW)) {
		nameiflags |= FOLLOW;
	}

	nameiflags |= AUDITVNPATH1;
	NDINIT(&nd, LOOKUP, OP_GETATTR, nameiflags, pathsegflg,
	    path, ctx);

	error = nameiat(&nd, fd);

	if (error) {
		return error;
	}

	vp = nd.ni_vp;

	error = getattrlist_internal(ctx, vp, alp, attributeBuffer,
	    bufferSize, options, segflg, NULL, NOCRED);

	                                                                 
	nameidone(&nd);
	vnode_put(vp);
	return error;
}

int
getattrlist(proc_t p, struct getattrlist_args *uap, __unused int32_t *retval)
{
	enum uio_seg segflg;
	struct attrlist al;
	int error;

	segflg = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;

	  
                                
    
	error = copyin(uap->alist, &al, sizeof(al));
	if (error) {
		return error;
	}

	return getattrlistat_internal(vfs_context_current(),
	           CAST_USER_ADDR_T(uap->path), &al,
	           CAST_USER_ADDR_T(uap->attributeBuffer), uap->bufferSize,
	           (uint64_t)uap->options, segflg, segflg, AT_FDCWD);
}

int
getattrlistat(proc_t p, struct getattrlistat_args *uap, __unused int32_t *retval)
{
	enum uio_seg segflg;
	struct attrlist al;
	int error;

	segflg = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;

	  
                                
    
	error = copyin(uap->alist, &al, sizeof(al));
	if (error) {
		return error;
	}

	return getattrlistat_internal(vfs_context_current(),
	           CAST_USER_ADDR_T(uap->path), &al,
	           CAST_USER_ADDR_T(uap->attributeBuffer), uap->bufferSize,
	           (uint64_t)uap->options, segflg, segflg, uap->fd);
}

  
                                                                      
                                                                    
                                                                    
                                                                        
                                                                           
       
  
                                                                      
                                                                          
          
   
static int
refill_fd_direntries(vfs_context_t ctx, vnode_t dvp, struct fd_vn_data *fvd,
    int *eofflagp)
{
	uio_t rdir_uio;
	char uio_buf[UIO_SIZEOF(1)];
	size_t rdirbufsiz;
	size_t rdirbufused;
	int eofflag;
	int nentries;
	int error;

	  
                                                      
    
	if (fvd->fv_eofflag) {
		*eofflagp = 1;
		if (fvd->fv_buf) {
			kheap_free(KHEAP_DATA_BUFFERS, fvd->fv_buf, fvd->fv_bufallocsiz);
			fvd->fv_buf = NULL;
		}
		return 0;
	}

	error = 0;

	  
                                                                     
                                                                  
                                                                        
                                                   
    

	if (fvd->fv_offset && fvd->fv_bufallocsiz) {
		rdirbufsiz = fvd->fv_bufallocsiz;
	} else {
		rdirbufsiz = FV_DIRBUF_START_SIZ;
	}

	*eofflagp = 0;

	rdir_uio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_READ,
	    &uio_buf[0], sizeof(uio_buf));

retry_alloc:
	  
                                                       
                                 
    
	if (!fvd->fv_buf) {
		fvd->fv_buf = kheap_alloc(KHEAP_DATA_BUFFERS, rdirbufsiz, Z_WAITOK);
		fvd->fv_bufallocsiz = rdirbufsiz;
		fvd->fv_bufdone = 0;
	}

	uio_reset(rdir_uio, fvd->fv_eoff, UIO_SYSSPACE, UIO_READ);
	uio_addiov(rdir_uio, CAST_USER_ADDR_T(fvd->fv_buf), rdirbufsiz);

	  
                                                      
    
	eofflag = 0;
	nentries = 0;
	error = vnode_readdir64(dvp, rdir_uio, VNODE_READDIR_EXTENDED,
	    &eofflag, &nentries, ctx);

	rdirbufused = rdirbufsiz - (size_t)uio_resid(rdir_uio);

	if (!error && (rdirbufused > 0) && (rdirbufused <= rdirbufsiz)) {
		                  
		fvd->fv_soff = fvd->fv_eoff;
		fvd->fv_eoff = uio_offset(rdir_uio);
		                                                           
		fvd->fv_eofflag = eofflag;
		eofflag = 0;
		                             
		fvd->fv_bufsiz = rdirbufused;
		fvd->fv_bufdone = 0;
		bzero(fvd->fv_buf + rdirbufused, rdirbufsiz - rdirbufused);
	} else if (!eofflag && (rdirbufsiz < FV_DIRBUF_MAX_SIZ)) {
		  
                                                      
                                                    
                                                             
                                                        
                                                                
                                                              
                                                                  
                        
     
		kheap_free(KHEAP_DATA_BUFFERS, fvd->fv_buf, fvd->fv_bufallocsiz);
		rdirbufsiz = 2 * rdirbufsiz;
		fvd->fv_bufallocsiz = 0;
		goto retry_alloc;
	} else if (!error) {
		  
                                                        
                                                          
                          
     
		fvd->fv_eofflag = eofflag = 1;
	}

	  
                                                                      
                                      
    
	if (error && fvd->fv_eofflag) {
		eofflag = 1;
		error = 0;
	}

	  
                                                                         
                                           
    
	if ((error || eofflag) && fvd->fv_buf) {
		kheap_free(KHEAP_DATA_BUFFERS, fvd->fv_buf, fvd->fv_bufallocsiz);
		if (error) {
			fvd->fv_bufallocsiz = 0;
		}
	}

	*eofflagp = eofflag;

	return error;
}

  
                                                                            
                               
  
                                                                     
                                                                         
                                                
   
static int
get_direntry(vfs_context_t ctx, vnode_t dvp, struct fd_vn_data *fvd,
    int *eofflagp, struct direntry **dpp)
{
	int eofflag;
	int error;

	*eofflagp = 0;
	*dpp = NULL;
	error = 0;
	if (!fvd->fv_bufsiz) {
		error = refill_fd_direntries(ctx, dvp, fvd, &eofflag);
		if (error) {
			return error;
		}
		if (eofflag) {
			*eofflagp = eofflag;
			return error;
		}
	}

	*dpp = (struct direntry *)(fvd->fv_buf + fvd->fv_bufdone);
	return error;
}

  
                                 
   
static void
direntry_done(struct fd_vn_data *fvd)
{
	struct direntry *dp;

	dp = (struct direntry *)(fvd->fv_buf + fvd->fv_bufdone);
	if (dp->d_reclen) {
		fvd->fv_bufdone += dp->d_reclen;
		if (fvd->fv_bufdone > fvd->fv_bufsiz) {
			fvd->fv_bufdone = fvd->fv_bufsiz;
		}
	} else {
		fvd->fv_bufdone = fvd->fv_bufsiz;
	}

	  
                                                          
                   
    
	if (fvd->fv_bufdone == fvd->fv_bufsiz) {
		fvd->fv_bufdone = 0;
		fvd->fv_bufsiz = 0;
	}
}

  
                                                                          
                                                             
  
                                                                             
                                                                       
  
                                                           
  
                                                                       
                                                                        
                                                               
  
         
                           
                                          
                                                       
                                                                        
                    
                                            
                                                                       
                                                                            
                                                           
                                                    
                                     
   
static void
get_error_attributes(vnode_t vp, struct attrlist *alp, uint64_t options,
    user_addr_t kern_attr_buf, size_t kern_attr_buf_siz, int error_attr,
    caddr_t namebuf, vfs_context_t ctx)
{
	size_t fsiz, vsiz;
	struct _attrlist_buf ab;
	size_t namelen;
	kauth_action_t action;
	struct attrlist al;
	int needs_error_attr = (alp->commonattr & ATTR_CMN_ERROR);

	  
                                                                         
                                                                      
                                                                         
                                                                 
   
                                                             
                                                               
                                                
    
	al = *alp;
	if (!(options & FSOPT_PACK_INVAL_ATTRS)) {
		  
                                                             
                                        
     
		al.commonattr = ATTR_BULK_REQUIRED;
		if (needs_error_attr) {
			al.commonattr |= ATTR_CMN_ERROR;
		}
		al.fileattr = 0;
		al.dirattr = 0;
	}

	  
                                                        
                                                             
    
	fsiz = 0;
	(void)getattrlist_setupvattr(&al, NULL, (ssize_t *)&fsiz,
	    &action, proc_is64bit(vfs_context_proc(ctx)),
	    (vnode_vtype(vp) == VDIR), (options & FSOPT_ATTR_CMN_EXTENDED));

	namelen = strlen(namebuf);
	vsiz = namelen + 1;
	vsiz = ((vsiz + 3) & ~0x03);

	bzero(&ab, sizeof(ab));
	ab.base = (char *)kern_attr_buf;
	ab.needed = fsiz + vsiz;

	                             
	*((uint32_t *)ab.base) = (uint32_t)ab.needed;
	if (ab.needed > (ssize_t)kern_attr_buf_siz) {
		goto out;
	}

	  
                                                      
    
	ab.fixedcursor = ab.base + sizeof(uint32_t);
	  
                                                                   
                                                                 
                                                     
    
	bzero(ab.fixedcursor, fsiz);
	  
                                                       
              
    
	ab.varcursor = ab.base + fsiz;
	  
                                                                    
                                                         
    
	bzero(&ab.actual, sizeof(ab.actual));
	ab.fixedcursor += sizeof(attribute_set_t);

	ab.allocated = ab.needed;

	                                        
	if (needs_error_attr) {
		ATTR_PACK4(ab, error_attr);
		ab.actual.commonattr |= ATTR_CMN_ERROR;
	}

	  
                                                                   
                                                                    
                                                                
               
    
	attrlist_pack_string(&ab, namebuf, namelen);

	  
                                                        
                                                           
                  
    
	ab.actual.commonattr |= ATTR_CMN_NAME | ATTR_CMN_RETURNED_ATTRS;
	bcopy(&ab.actual, ab.base + sizeof(uint32_t), sizeof(ab.actual));
out:
	return;
}

  
                                                                             
                                                                              
                                            
   

#define MIN_BUF_SIZE_REQUIRED  (sizeof(uint32_t) + sizeof(attribute_set_t) +\
    sizeof(attrreference_t))

  
                                                                         
   
static int
readdirattr(vnode_t dvp, struct fd_vn_data *fvd, uio_t auio,
    struct attrlist *alp, uint64_t options, int *count, int *eofflagp,
    vfs_context_t ctx)
{
	caddr_t kern_attr_buf;
	size_t kern_attr_buf_siz;
	caddr_t max_path_name_buf = NULL;
	int error = 0;

	*count = 0;
	*eofflagp = 0;

	if (uio_iovcnt(auio) > 1) {
		return EINVAL;
	}

	  
                                                                  
                                                            
    
	kern_attr_buf_siz = uio_resid(auio);
	if (kern_attr_buf_siz > ATTR_MAX_BUFFER) {
		kern_attr_buf_siz = ATTR_MAX_BUFFER;
	} else if (kern_attr_buf_siz == 0) {
		                   
		return error;
	}

	kern_attr_buf = kheap_alloc(KHEAP_TEMP, kern_attr_buf_siz, Z_WAITOK);

	while (uio_resid(auio) > (user_ssize_t)MIN_BUF_SIZE_REQUIRED) {
		struct direntry *dp;
		user_addr_t name_buffer;
		struct nameidata nd;
		vnode_t vp;
		struct attrlist al;
		size_t entlen;
		size_t bytes_left;
		size_t pad_bytes;
		ssize_t new_resid;

		  
                                                           
                                                         
                             
     
		error = get_direntry(ctx, dvp, fvd, eofflagp, &dp);
		if (error || (*eofflagp) || !dp) {
			break;
		}

		  
                                                                 
     
		if (!dp->d_reclen || dp->d_ino == 0 || dp->d_namlen == 0 ||
		    (dp->d_namlen == 1 && dp->d_name[0] == '.') ||
		    (dp->d_namlen == 2 && dp->d_name[0] == '.' &&
		    dp->d_name[1] == '.')) {
			direntry_done(fvd);
			continue;
		}

		  
                                                    
     
		if (dp->d_name[dp->d_namlen] != '\0') {
			if (!max_path_name_buf) {
				max_path_name_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
			}
			bcopy(dp->d_name, max_path_name_buf, dp->d_namlen);
			max_path_name_buf[dp->d_namlen] = '\0';
			name_buffer = CAST_USER_ADDR_T(max_path_name_buf);
		} else {
			name_buffer = CAST_USER_ADDR_T(&(dp->d_name));
		}

		  
                                                 
    
                                                                                
                                                                             
                                                                                    
                                                                               
                                                                                     
                                                                           
                                                                                  
                            
     
		NDINIT(&nd, LOOKUP, OP_GETATTR, (AUDITVNPATH1 | USEDVP | NOCROSSMOUNT),
		    UIO_SYSSPACE, CAST_USER_ADDR_T(name_buffer), ctx);

		nd.ni_dvp = dvp;
		error = namei(&nd);

		if (error) {
			direntry_done(fvd);
			error = 0;
			continue;
		}

		vp = nd.ni_vp;

		  
                                                      
                                                         
                              
     
		al = *alp;

		error = getattrlist_internal(ctx, vp, &al,
		    CAST_USER_ADDR_T(kern_attr_buf), kern_attr_buf_siz,
		    options | FSOPT_REPORT_FULLSIZE, UIO_SYSSPACE,
		    CAST_DOWN_EXPLICIT(char *, name_buffer),
		    NOCRED);

		nameidone(&nd);

		if (error) {
			get_error_attributes(vp, alp, options,
			    CAST_USER_ADDR_T(kern_attr_buf),
			    kern_attr_buf_siz, error, (caddr_t)name_buffer,
			    ctx);
			error = 0;
		}

		                         
		vnode_put(vp);

		  
                                                             
                                                            
                                                              
                                                    
    
                                                           
                                                         
                                                            
                                                             
                                                           
                             
     
		bytes_left = (size_t)((user_size_t)uio_resid(auio));
		entlen = (size_t)(*((uint32_t *)(kern_attr_buf)));
		if (!entlen || (entlen > bytes_left)) {
			break;
		}

		  
                                                                  
                                                         
     
		pad_bytes = ((entlen + 7) & ~0x07) - entlen;
		new_resid = 0;
		if (pad_bytes && (entlen + pad_bytes <= bytes_left)) {
			  
                                                  
                                                  
                                              
                                                  
                                 
      
			if (entlen + pad_bytes <= kern_attr_buf_siz) {
				                              
				bzero(kern_attr_buf + entlen, pad_bytes);
			} else {
				bzero(kern_attr_buf + entlen,
				    kern_attr_buf_siz - entlen);
				  
                                                  
                                            
                                          
                                            
                       
       
				new_resid = (ssize_t)(bytes_left -
				    (entlen + pad_bytes));
			}
			entlen += pad_bytes;
		}
		*((uint32_t *)kern_attr_buf) = (uint32_t)entlen;
		error = uiomove(kern_attr_buf, min((int)entlen, (int)kern_attr_buf_siz),
		    auio);

		if (error) {
			break;
		}

		if (new_resid) {
			uio_setresid(auio, (user_ssize_t)new_resid);
		}

		  
                                                                  
                     
     
		(*count)++;
		direntry_done(fvd);
	}

	if (max_path_name_buf) {
		zfree(ZV_NAMEI, max_path_name_buf);
	}

	  
                                                    
    
	kheap_free(KHEAP_TEMP, kern_attr_buf, kern_attr_buf_siz);

	  
                                                      
                             
    
	uio_setoffset(auio, fvd->fv_eoff);

	return error;
}

                                                                    
#define LIST_DIR_ATTRS    (ATTR_CMN_NAME | ATTR_CMN_OBJTYPE |  \
	                   ATTR_CMN_FILEID | ATTR_CMN_RETURNED_ATTRS |  \
	                   ATTR_CMN_ERROR)

  
                                                                                
                                          
  
                                                                    
                                                    
  
                                                                        
   
int
getattrlistbulk(proc_t p, struct getattrlistbulk_args *uap, int32_t *retval)
{
	struct attrlist al;
	vnode_t dvp = NULLVP;
	struct fileproc *fp;
	struct fd_vn_data *fvdata;
	vfs_context_t ctx;
	uthread_t ut;
	enum uio_seg segflg;
	int count;
	uio_t auio = NULL;
	char uio_buf[UIO_SIZEOF(1)];
	kauth_action_t action;
	int eofflag;
	uint64_t options;
	int error;

	*retval = 0;

	error = fp_getfvp(p, uap->dirfd, &fp, &dvp);
	if (error) {
		return error;
	}

	count = 0;
	fvdata = NULL;
	eofflag = 0;
	ctx = vfs_context_current();
	ut = get_bsdthread_info(current_thread());
	segflg = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;

	if ((fp->fp_glob->fg_flag & FREAD) == 0) {
		  
                                                 
     
		error = EBADF;
		dvp = NULLVP;
		goto out;
	}

	if ((error = vnode_getwithref(dvp))) {
		dvp = NULLVP;
		goto out;
	}

	if (uap->options & FSOPT_LIST_SNAPSHOT) {
		vnode_t snapdvp;

		if (!vnode_isvroot(dvp)) {
			error = EINVAL;
			goto out;
		}

		                                            
		error = vnode_get_snapdir(dvp, &snapdvp, ctx);
		if (error) {
			goto out;
		}
		vnode_put(dvp);
		dvp = snapdvp;
	}

	if (dvp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}

#if CONFIG_MACF
	error = mac_file_check_change_offset(vfs_context_ucred(ctx),
	    fp->fp_glob);
	if (error) {
		goto out;
	}
#endif
	  
                       
                                       
    

	options = uap->options | FSOPT_ATTR_CMN_EXTENDED;

	if ((error = copyin(CAST_USER_ADDR_T(uap->alist), &al,
	    sizeof(struct attrlist)))) {
		goto out;
	}

	if (al.volattr ||
	    ((al.commonattr & ATTR_BULK_REQUIRED) != ATTR_BULK_REQUIRED)) {
		error = EINVAL;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_readdir(ctx, dvp);
	if (error != 0) {
		goto out;
	}
#endif          

	  
                                                                                     
                                                                                            
                                                          
    
	action = KAUTH_VNODE_LIST_DIRECTORY;
	if ((al.commonattr & ~LIST_DIR_ATTRS) || al.fileattr || al.dirattr) {
		action |= KAUTH_VNODE_SEARCH;
	}

	error = vnode_authorize(dvp, NULL, action, ctx);
	if (error) {
		goto out;
	}

	fvdata = (struct fd_vn_data *)fp->fp_glob->fg_vn_data;
	if (!fvdata) {
		panic("Directory expected to have fg_vn_data");
	}

	FV_LOCK(fvdata);

	  
                                                                 
                                                                        
                                                              
                                         
    
	if (!fp->fp_glob->fg_offset) {
		fvdata->fv_offset = 0;
		kheap_free(KHEAP_DATA_BUFFERS, fvdata->fv_buf,
		    fvdata->fv_bufallocsiz);
		fvdata->fv_bufsiz = 0;
		fvdata->fv_bufdone = 0;
		fvdata->fv_soff = 0;
		fvdata->fv_eoff = 0;
		fvdata->fv_eofflag = 0;
	}

	auio = uio_createwithbuffer(1, fvdata->fv_offset, segflg, UIO_READ,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, uap->attributeBuffer, (user_size_t)uap->bufferSize);

	  
                                                                       
                                                                        
                                                            
                                                                    
                                                                       
                                       
    
	if ((al.commonattr &
	    (ATTR_CMN_UUID | ATTR_CMN_GRPUUID | ATTR_CMN_EXTENDED_SECURITY)) ||
	    !(al.commonattr & ATTR_CMN_OBJTYPE)) {
		error = ENOTSUP;
	} else {
		struct vnode_attr *va;
		char *va_name;

		if (fvdata->fv_eofflag && !fvdata->fv_buf) {
			  
                                                    
                                                 
      
			eofflag = 1;
			count = 0;
			error = 0;
		} else {
			eofflag = 0;
			count = 0;

			va = kheap_alloc(KHEAP_TEMP, sizeof(struct vnode_attr), Z_WAITOK);

			VATTR_INIT(va);
			va_name = zalloc_flags(ZV_NAMEI, Z_WAITOK | Z_ZERO);
			va->va_name = va_name;

			(void)getattrlist_setupvattr_all(&al, va, VNON, NULL,
			    IS_64BIT_PROCESS(p), (uap->options & FSOPT_ATTR_CMN_EXTENDED));

			  
                                                                
                                    
      
			ut->uu_flag |= UT_KERN_RAGE_VNODES;
			error = VNOP_GETATTRLISTBULK(dvp, &al, va, auio, NULL,
			    options, &eofflag, &count, ctx);
			ut->uu_flag &= ~UT_KERN_RAGE_VNODES;

			zfree(ZV_NAMEI, va_name);
			kheap_free(KHEAP_TEMP, va, sizeof(struct vnode_attr));

			  
                             
      
			if (!error) {
				fvdata->fv_eofflag = eofflag;
			}
		}
	}

	  
                                                                
                                  
    
	if (error == ENOTSUP) {
		eofflag = 0;
		count = 0;

		ut->uu_flag |= UT_KERN_RAGE_VNODES;
		error = readdirattr(dvp, fvdata, auio, &al, options,
		    &count, &eofflag, ctx);
		ut->uu_flag &= ~UT_KERN_RAGE_VNODES;
	}

	if (count) {
		fvdata->fv_offset = uio_offset(auio);
		fp->fp_glob->fg_offset = fvdata->fv_offset;
		*retval = count;
		error = 0;
	} else if (!error && !eofflag) {
		  
                                                           
                  
     
		error = ERANGE;
	}

	FV_UNLOCK(fvdata);
out:
	if (dvp) {
		vnode_put(dvp);
	}

	file_drop(uap->dirfd);

	return error;
}

static int
attrlist_unpack_fixed(char **cursor, char *end, void *buf, ssize_t size)
{
	                                          
	if ((*cursor) + size > end) {
		return EINVAL;
	}

	bcopy(*cursor, buf, size);
	*cursor += size;
	return 0;
}

#define ATTR_UNPACK(v)          do {if ((error = attrlist_unpack_fixed(&cursor, bufend, &v, sizeof(v))) != 0) goto out;} while(0);
#define ATTR_UNPACK_CAST(t, v)  do { t _f; ATTR_UNPACK(_f); v = (typeof(v))_f;} while(0)
#define ATTR_UNPACK_TIME(v, is64)                               \
	do {                                                    \
	        if (is64) {                                     \
	                struct user64_timespec us;              \
	                ATTR_UNPACK(us);                        \
	                v.tv_sec = (unsigned long)us.tv_sec;                   \
	                v.tv_nsec = (long)us.tv_nsec;                 \
	        } else {                                        \
	                struct user32_timespec us;              \
	                ATTR_UNPACK(us);                        \
	                v.tv_sec = us.tv_sec;                   \
	                v.tv_nsec = us.tv_nsec;                 \
	        }                                               \
	} while(0)


  
                    
   
static int
setattrlist_internal(vnode_t vp, struct setattrlist_args *uap, proc_t p, vfs_context_t ctx)
{
	struct attrlist al;
	struct vnode_attr va;
	struct attrreference ar;
	kauth_action_t  action;
	char            *user_buf, *cursor, *bufend, *fndrinfo, *cp, *volname;
	int             proc_is64, error;
	uint32_t        nace;
	kauth_filesec_t rfsec;

	user_buf = NULL;
	fndrinfo = NULL;
	volname = NULL;
	error = 0;
	proc_is64 = proc_is64bit(p);
	VATTR_INIT(&va);

	  
                                         
    
	if ((error = copyin(uap->alist, (caddr_t) &al, sizeof(al)))) {
		goto out;
	}
	if (al.bitmapcount != ATTR_BIT_MAP_COUNT) {
		error = EINVAL;
		goto out;
	}

#if DEVELOPMENT || DEBUG
	  
                                                          
                                            
    
#else                           

	if (vnode_isswap(vp) && (ctx != vfs_context_kernel())) {
		error = EPERM;
		goto out;
	}
#endif                           

	VFS_DEBUG(ctx, vp, "%p  ATTRLIST - %s set common %08x vol %08x file %08x dir %08x fork %08x %sfollow on '%s'",
	    vp, p->p_comm, al.commonattr, al.volattr, al.fileattr, al.dirattr, al.forkattr,
	    (uap->options & FSOPT_NOFOLLOW) ? "no":"", vp->v_name);

	if (al.volattr) {
		if ((al.volattr & ~ATTR_VOL_SETMASK) ||
		    (al.commonattr & ~ATTR_CMN_VOLSETMASK) ||
		    al.fileattr ||
		    al.forkattr) {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: attempt to set invalid volume attributes");
			goto out;
		}
	} else {
		if ((al.commonattr & ~ATTR_CMN_SETMASK) ||
		    (al.fileattr & ~ATTR_FILE_SETMASK) ||
		    (al.dirattr & ~ATTR_DIR_SETMASK) ||
		    (al.forkattr & ~ATTR_FORK_SETMASK)) {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: attempt to set invalid file/folder attributes");
			goto out;
		}
	}

	  
                                                                         
                    
    
	if (al.commonattr == 0 &&
	    (al.volattr & ~ATTR_VOL_INFO) == 0 &&
	    al.dirattr == 0 &&
	    al.fileattr == 0 &&
	    al.forkattr == 0) {
		error = 0;
		goto out;
	}

	  
                                                                              
                                                                                 
                                                                 
                                                                                  
                                                                                   
   
                                                                                 
    
	if (uap->bufferSize > ATTR_MAX_BUFFER) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: buffer size %d too large", uap->bufferSize);
		error = ENOMEM;
		goto out;
	}
	user_buf = kheap_alloc(KHEAP_DATA_BUFFERS, uap->bufferSize, Z_WAITOK);
	if (user_buf == NULL) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: could not allocate %d bytes for buffer", uap->bufferSize);
		error = ENOMEM;
		goto out;
	}
	if ((error = copyin(uap->attributeBuffer, user_buf, uap->bufferSize)) != 0) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: buffer copyin failed");
		goto out;
	}
	VFS_DEBUG(ctx, vp, "ATTRLIST - copied in %d bytes of user attributes to %p", uap->bufferSize, user_buf);

#if CONFIG_MACF
	error = mac_vnode_check_setattrlist(ctx, vp, &al);
	if (error) {
		goto out;
	}
#endif          

	  
                               
    
	cursor = user_buf;
	bufend = cursor + uap->bufferSize;

	            
	if (al.commonattr & ATTR_CMN_SCRIPT) {
		ATTR_UNPACK(va.va_encoding);
		VATTR_SET_ACTIVE(&va, va_encoding);
	}
	if (al.commonattr & ATTR_CMN_CRTIME) {
		ATTR_UNPACK_TIME(va.va_create_time, proc_is64);
		VATTR_SET_ACTIVE(&va, va_create_time);
	}
	if (al.commonattr & ATTR_CMN_MODTIME) {
		ATTR_UNPACK_TIME(va.va_modify_time, proc_is64);
		VATTR_SET_ACTIVE(&va, va_modify_time);
	}
	if (al.commonattr & ATTR_CMN_CHGTIME) {
		ATTR_UNPACK_TIME(va.va_change_time, proc_is64);
		al.commonattr &= ~ATTR_CMN_CHGTIME;
		                                                    
	}
	if (al.commonattr & ATTR_CMN_ACCTIME) {
		ATTR_UNPACK_TIME(va.va_access_time, proc_is64);
		VATTR_SET_ACTIVE(&va, va_access_time);
	}
	if (al.commonattr & ATTR_CMN_BKUPTIME) {
		ATTR_UNPACK_TIME(va.va_backup_time, proc_is64);
		VATTR_SET_ACTIVE(&va, va_backup_time);
	}
	if (al.commonattr & ATTR_CMN_FNDRINFO) {
		if ((cursor + 32) > bufend) {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - not enough data supplied for FINDERINFO");
			goto out;
		}
		fndrinfo = cursor;
		cursor += 32;
	}
	if (al.commonattr & ATTR_CMN_OWNERID) {
		ATTR_UNPACK(va.va_uid);
		VATTR_SET_ACTIVE(&va, va_uid);
	}
	if (al.commonattr & ATTR_CMN_GRPID) {
		ATTR_UNPACK(va.va_gid);
		VATTR_SET_ACTIVE(&va, va_gid);
	}
	if (al.commonattr & ATTR_CMN_ACCESSMASK) {
		ATTR_UNPACK_CAST(uint32_t, va.va_mode);
		VATTR_SET_ACTIVE(&va, va_mode);
	}
	if (al.commonattr & ATTR_CMN_FLAGS) {
		ATTR_UNPACK(va.va_flags);
		VATTR_SET_ACTIVE(&va, va_flags);
#if CONFIG_MACF
		if ((error = mac_vnode_check_setflags(ctx, vp, va.va_flags)) != 0) {
			goto out;
		}
#endif
	}
	if (al.commonattr & ATTR_CMN_EXTENDED_SECURITY) {
		  
                                                                    
                   
     
		cp = cursor;
		ATTR_UNPACK(ar);
		if (ar.attr_dataoffset < 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: bad offset supplied", ar.attr_dataoffset);
			error = EINVAL;
			goto out;
		}

		cp += ar.attr_dataoffset;
		rfsec = (kauth_filesec_t)cp;
		if (((((char *)rfsec) + KAUTH_FILESEC_SIZE(0)) > bufend) ||                                           
		    (rfsec->fsec_magic != KAUTH_FILESEC_MAGIC) ||                      
		    (KAUTH_FILESEC_COPYSIZE(rfsec) != ar.attr_length) ||                          
		    ((cp + KAUTH_FILESEC_COPYSIZE(rfsec)) > bufend)) {                           
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: bad ACL supplied", ar.attr_length);
			goto out;
		}
		nace = rfsec->fsec_entrycount;
		if (nace == KAUTH_FILESEC_NOACL) {
			nace = 0;
		}
		if (nace > KAUTH_ACL_MAX_ENTRIES) {                                           
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: bad ACL supplied");
			goto out;
		}
		nace = rfsec->fsec_acl.acl_entrycount;
		if (nace == KAUTH_FILESEC_NOACL) {
			                  
			VATTR_SET(&va, va_acl, NULL);
		} else {
			if (nace > KAUTH_ACL_MAX_ENTRIES) {                                           
				error = EINVAL;
				VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: supplied ACL is too large");
				goto out;
			}
			VATTR_SET(&va, va_acl, &rfsec->fsec_acl);
		}
	}
	if (al.commonattr & ATTR_CMN_UUID) {
		ATTR_UNPACK(va.va_uuuid);
		VATTR_SET_ACTIVE(&va, va_uuuid);
	}
	if (al.commonattr & ATTR_CMN_GRPUUID) {
		ATTR_UNPACK(va.va_guuid);
		VATTR_SET_ACTIVE(&va, va_guuid);
	}
	if (al.commonattr & ATTR_CMN_ADDEDTIME) {
		ATTR_UNPACK_TIME(va.va_addedtime, proc_is64);
		VATTR_SET_ACTIVE(&va, va_addedtime);
	}
	                                                  
	if (al.commonattr & ATTR_CMN_DATA_PROTECT_FLAGS) {
		ATTR_UNPACK(va.va_dataprotect_class);
		VATTR_SET_ACTIVE(&va, va_dataprotect_class);
	}

	            
	if (al.volattr & ATTR_VOL_INFO) {
		if (al.volattr & ATTR_VOL_NAME) {
			volname = cursor;
			ATTR_UNPACK(ar);
			                              
			if ((ar.attr_dataoffset < 0) || (ar.attr_length == 0) ||
			    (ar.attr_length > uap->bufferSize) ||
			    (uap->bufferSize - ar.attr_length < (unsigned)ar.attr_dataoffset)) {
				VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: bad offset supplied (2) ", ar.attr_dataoffset);
				error = EINVAL;
				goto out;
			}

			if (volname >= bufend - ar.attr_dataoffset - ar.attr_length) {
				error = EINVAL;
				VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: volume name too big for caller buffer");
				goto out;
			}
			volname += ar.attr_dataoffset;
			                               
			volname[ar.attr_length - 1] = 0;
		}
	}

	          
	if (al.fileattr & ATTR_FILE_DEVTYPE) {
		                                                         
		error = EINVAL;
		VFS_DEBUG(ctx, vp, "ATTRLIST - XXX device type change not implemented");
		goto out;
	}

	  
                           
    
	action = 0;
	if ((va.va_active != 0LL) && ((error = vnode_authattr(vp, &va, &action, ctx)) != 0)) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: attribute changes refused: %d", error);
		goto out;
	}
	  
                                                                                  
                                  
    
	if (fndrinfo != NULL) {
		if (al.volattr & ATTR_VOL_INFO) {
			if (vp->v_tag != VT_HFS) {
				error = EINVAL;
				goto out;
			}
		} else {
			action |= KAUTH_VNODE_WRITE_EXTATTRIBUTES;
		}
	}

	if ((action != 0) && ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: authorization failed");
		goto out;
	}

	  
                                                                     
                                                                    
                                                                    
                               
   
                                                                    
                                                                  
                                                                  
    
	if ((fndrinfo != NULL) && !(al.volattr & ATTR_VOL_INFO) &&
	    (al.commonattr & ATTR_CMN_ACCESSMASK) && !(va.va_mode & S_IWUSR)) {
		if ((error = setattrlist_setfinderinfo(vp, fndrinfo, ctx)) != 0) {
			goto out;
		}
		fndrinfo = NULL;                                             
	}

	  
                                        
    
	if ((va.va_active != 0LL) && ((error = vnode_setattr(vp, &va, ctx)) != 0)) {
		VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: filesystem returned %d", error);
		goto out;
	}

#if CONFIG_MACF
	mac_vnode_notify_setattrlist(ctx, vp, &al);
	if (VATTR_IS_ACTIVE(&va, va_flags)) {
		mac_vnode_notify_setflags(ctx, vp, va.va_flags);
	}
#endif

	  
                                         
    
	if (fndrinfo != NULL) {
		if (al.volattr & ATTR_VOL_INFO) {
			if (vp->v_tag == VT_HFS) {
#define HFS_SET_BOOT_INFO   (FCNTL_FS_SPECIFIC_BASE + 0x00005)
				error = VNOP_IOCTL(vp, HFS_SET_BOOT_INFO, (caddr_t)fndrinfo, 0, ctx);
				if (error != 0) {
					goto out;
				}
			} else {
				                               
			}
		} else if ((error = setattrlist_setfinderinfo(vp, fndrinfo, ctx)) != 0) {
			goto out;
		}
	}

	  
                                       
    
	if (volname != NULL) {
		struct vfs_attr vs;

		VFSATTR_INIT(&vs);

		vs.f_vol_name = volname;                                                        
		VFSATTR_WANTED(&vs, f_vol_name);

#if CONFIG_MACF
		error = mac_mount_check_setattr(ctx, vp->v_mount, &vs);
		if (error != 0) {
			goto out;
		}
#endif

		if ((error = vfs_setattr(vp->v_mount, &vs, ctx)) != 0) {
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: setting volume name failed");
			goto out;
		}

		if (!VFSATTR_ALL_SUPPORTED(&vs)) {
			error = EINVAL;
			VFS_DEBUG(ctx, vp, "ATTRLIST - ERROR: could not set volume name");
			goto out;
		}
	}

	                             

out:
	kheap_free(KHEAP_DATA_BUFFERS, user_buf, uap->bufferSize);
	VFS_DEBUG(ctx, vp, "ATTRLIST - set returning %d", error);
	return error;
}

int
setattrlist(proc_t p, struct setattrlist_args *uap, __unused int32_t *retval)
{
	struct vfs_context *ctx;
	struct nameidata nd;
	vnode_t         vp = NULL;
	uint32_t          nameiflags;
	int error = 0;

	ctx = vfs_context_current();

	  
                     
    
	nameiflags = AUDITVNPATH1;
	if ((uap->options & FSOPT_NOFOLLOW) == 0) {
		nameiflags |= FOLLOW;
	}
	NDINIT(&nd, LOOKUP, OP_SETATTR, nameiflags, UIO_USERSPACE, uap->path, ctx);
	if ((error = namei(&nd)) != 0) {
		goto out;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	error = setattrlist_internal(vp, uap, p, ctx);
out:
	if (vp != NULL) {
		vnode_put(vp);
	}
	return error;
}

int
setattrlistat(proc_t p, struct setattrlistat_args *uap, __unused int32_t *retval)
{
	struct setattrlist_args ap;
	struct vfs_context *ctx;
	struct nameidata nd;
	vnode_t vp = NULLVP;
	uint32_t nameiflags;
	int error;

	ctx = vfs_context_current();

	AUDIT_ARG(fd, uap->fd);
	  
                     
    
	nameiflags = AUDITVNPATH1;
	if (!(uap->options & FSOPT_NOFOLLOW)) {
		nameiflags |= FOLLOW;
	}
	NDINIT(&nd, LOOKUP, OP_SETATTR, nameiflags, UIO_USERSPACE, uap->path, ctx);
	if ((error = nameiat(&nd, uap->fd)) != 0) {
		goto out;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	ap.path = 0;
	ap.alist = uap->alist;
	ap.attributeBuffer = uap->attributeBuffer;
	ap.bufferSize = uap->bufferSize;
	ap.options = uap->options;

	error = setattrlist_internal(vp, &ap, p, ctx);
out:
	if (vp) {
		vnode_put(vp);
	}
	return error;
}

int
fsetattrlist(proc_t p, struct fsetattrlist_args *uap, __unused int32_t *retval)
{
	struct vfs_context *ctx;
	vnode_t         vp = NULL;
	int             error;
	struct setattrlist_args ap;

	ctx = vfs_context_current();

	if ((error = file_vnode(uap->fd, &vp)) != 0) {
		return error;
	}

	if ((error = vnode_getwithref(vp)) != 0) {
		file_drop(uap->fd);
		return error;
	}

	ap.path = 0;
	ap.alist = uap->alist;
	ap.attributeBuffer = uap->attributeBuffer;
	ap.bufferSize = uap->bufferSize;
	ap.options = uap->options;

	error = setattrlist_internal(vp, &ap, p, ctx);
	file_drop(uap->fd);
	if (vp != NULL) {
		vnode_put(vp);
	}

	return error;
}
