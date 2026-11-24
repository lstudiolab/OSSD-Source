/*
 * Copyright (c) 2004-2012 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>

#include <sys/fcntl.h>
#include <sys/fsevents.h>
#include <sys/kernel.h>
#include <sys/kauth.h>
#include <kern/kalloc.h>
#include <sys/mount_internal.h>
#include <sys/namei.h>
#include <sys/proc_internal.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/utfconv.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>
#include <sys/xattr.h>

#include <libkern/OSByteOrder.h>
#include <vm/vm_kern.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif


#if NAMEDSTREAMS

static int shadow_sequence;

  
                                                                                
   

#define SHADOW_NAME_FMT         ".vfs_rsrc_stream_%p%08x%p"
#define SHADOW_DIR_FMT          ".vfs_rsrc_streams_%p%x"
#define SHADOW_DIR_CONTAINER "/var/run"

#define MAKE_SHADOW_NAME(VP, NAME)  \
	snprintf((NAME), sizeof((NAME)), (SHADOW_NAME_FMT), \
	                ((void*)(VM_KERNEL_ADDRPERM(VP))), \
	                (VP)->v_id, \
	                ((void*)(VM_KERNEL_ADDRPERM((VP)->v_data))))

                                           
#define MAKE_SHADOW_DIRNAME(VP, NAME)   \
	snprintf((NAME), sizeof((NAME)), (SHADOW_DIR_CONTAINER "/" SHADOW_DIR_FMT), \
	                ((void*)(VM_KERNEL_ADDRPERM(VP))), shadow_sequence)

                                            
#define MAKE_SHADOW_DIR_LEAF(VP, NAME)  \
	snprintf((NAME), sizeof((NAME)), (SHADOW_DIR_FMT), \
	                ((void*)(VM_KERNEL_ADDRPERM(VP))), shadow_sequence)

static int  default_getnamedstream(vnode_t vp, vnode_t *svpp, const char *name, enum nsoperation op, vfs_context_t context);

static int  default_makenamedstream(vnode_t vp, vnode_t *svpp, const char *name, vfs_context_t context);

static int  default_removenamedstream(vnode_t vp, const char *name, vfs_context_t context);

static int  getshadowfile(vnode_t vp, vnode_t *svpp, int makestream, size_t *rsrcsize, int *creator, vfs_context_t context);

static int  get_shadow_dir(vnode_t *sdvpp);

#endif                   

  
                                  
   

static int default_getxattr(vnode_t vp, const char *name, uio_t uio, size_t *size, int options,
    vfs_context_t context);
static int default_setxattr(vnode_t vp, const char *name, uio_t uio, int options,
    vfs_context_t context);
static int default_listxattr(vnode_t vp, uio_t uio, size_t *size, int options,
    vfs_context_t context);
static int default_removexattr(vnode_t vp, const char *name, int options,
    vfs_context_t context);

  
                                               
   
int
vn_getxattr(vnode_t vp, const char *name, uio_t uio, size_t *size,
    int options, vfs_context_t context)
{
	int error;

	if (!XATTR_VNODE_SUPPORTED(vp)) {
		return EPERM;
	}
#if NAMEDSTREAMS
	                                                 
	if (vp->v_flag & VISNAMEDSTREAM) {
		error = EPERM;
		goto out;
	}
#endif
	  
                                                   
   
                                                       
    
	if (!(options & XATTR_NOSECURITY)) {
#if CONFIG_MACF
		error = mac_vnode_check_getextattr(context, vp, name, uio);
		if (error) {
			goto out;
		}
#endif          
		if ((error = xattr_validatename(name))) {
			goto out;
		}
		if ((error = vnode_authorize(vp, NULL, KAUTH_VNODE_READ_EXTATTRIBUTES, context))) {
			goto out;
		}
		                                                         
		if (uio != NULL && uio_offset(uio) != 0 &&
		    strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
			error = EINVAL;
			goto out;
		}
	}

	                                                         
	if (uio != NULL && uio_offset(uio) != 0 &&
	    strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
		error = EINVAL;
		goto out;
	}

	error = VNOP_GETXATTR(vp, name, uio, size, options, context);
	if (error == ENOTSUP && !(options & XATTR_NODEFAULT)) {
		  
                                                                           
     
		error = default_getxattr(vp, name, uio, size, options, context);
	}
out:
	return error;
}

  
                                         
   
int
vn_setxattr(vnode_t vp, const char *name, uio_t uio, int options, vfs_context_t context)
{
	int error;

	if (!XATTR_VNODE_SUPPORTED(vp)) {
		return EPERM;
	}
#if NAMEDSTREAMS
	                                                 
	if (vp->v_flag & VISNAMEDSTREAM) {
		error = EPERM;
		goto out;
	}
#endif
	if ((options & (XATTR_REPLACE | XATTR_CREATE)) == (XATTR_REPLACE | XATTR_CREATE)) {
		return EINVAL;
	}
	if ((error = xattr_validatename(name))) {
		return error;
	}
	if (!(options & XATTR_NOSECURITY)) {
#if CONFIG_MACF
		error = mac_vnode_check_setextattr(context, vp, name, uio);
		if (error) {
			goto out;
		}
#endif          
		error = vnode_authorize(vp, NULL, KAUTH_VNODE_WRITE_EXTATTRIBUTES, context);
		if (error) {
			goto out;
		}
	}
	                                                         
	if (uio_offset(uio) != 0 &&
	    strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
		error = EINVAL;
		goto out;
	}

	error = VNOP_SETXATTR(vp, name, uio, options, context);
#ifdef DUAL_EAS
	  
                                                              
                                                                   
                                                                        
                                                                   
                                                                      
                                                                      
                                                     
    
	if (error == EJUSTRETURN) {
		int native = 0, dufile = 0;
		size_t sz;                    

		native = VNOP_GETXATTR(vp, name, NULL, &sz, 0, context) ? 0 : 1;
		dufile = default_getxattr(vp, name, NULL, &sz, 0, context) ? 0 : 1;
		if (options & XATTR_CREATE && (native || dufile)) {
			error = EEXIST;
			goto out;
		}
		if (options & XATTR_REPLACE && !(native || dufile)) {
			error = ENOATTR;
			goto out;
		}
		  
                                                                
                                                            
     
		options &= ~(XATTR_CREATE | XATTR_REPLACE);
		error = VNOP_SETXATTR(vp, name, uio, options, context);
		                                                          
	}
#endif               
	if (error == ENOTSUP && !(options & XATTR_NODEFAULT)) {
		  
                                                                           
     
		error = default_setxattr(vp, name, uio, options, context);
	}
#if CONFIG_MACF
	if ((error == 0) && !(options & XATTR_NOSECURITY)) {
		mac_vnode_notify_setextattr(context, vp, name, uio);
		if (vfs_flags(vnode_mount(vp)) & MNT_MULTILABEL) {
			mac_vnode_label_update_extattr(vnode_mount(vp), vp, name);
		}
	}
#endif
out:
	return error;
}

  
                                
   
int
vn_removexattr(vnode_t vp, const char * name, int options, vfs_context_t context)
{
	int error;

	if (!XATTR_VNODE_SUPPORTED(vp)) {
		return EPERM;
	}
#if NAMEDSTREAMS
	                                                    
	if (vp->v_flag & VISNAMEDSTREAM) {
		error = EPERM;
		goto out;
	}
#endif
	if ((error = xattr_validatename(name))) {
		return error;
	}
	if (!(options & XATTR_NOSECURITY)) {
#if CONFIG_MACF
		error = mac_vnode_check_deleteextattr(context, vp, name);
		if (error) {
			goto out;
		}
#endif          
		error = vnode_authorize(vp, NULL, KAUTH_VNODE_WRITE_EXTATTRIBUTES, context);
		if (error) {
			goto out;
		}
	}
	error = VNOP_REMOVEXATTR(vp, name, options, context);
	if (error == ENOTSUP && !(options & XATTR_NODEFAULT)) {
		  
                                                                           
     
		error = default_removexattr(vp, name, options, context);
#ifdef DUAL_EAS
	} else if (error == EJUSTRETURN) {
		  
                                                                             
                                                                             
                                                              
                                                           
     
		error = default_removexattr(vp, name, options, context);
		if (error == ENOATTR) {
			error = 0;
		}
#endif               
	}
#if CONFIG_MACF
	if ((error == 0) && !(options & XATTR_NOSECURITY)) {
		mac_vnode_notify_deleteextattr(context, vp, name);
		if (vfs_flags(vnode_mount(vp)) & MNT_MULTILABEL) {
			mac_vnode_label_update_extattr(vnode_mount(vp), vp, name);
		}
	}
#endif
out:
	return error;
}

  
                                                 
   
int
vn_listxattr(vnode_t vp, uio_t uio, size_t *size, int options, vfs_context_t context)
{
	int error;

	if (!XATTR_VNODE_SUPPORTED(vp)) {
		return EPERM;
	}
#if NAMEDSTREAMS
	                                                  
	if (vp->v_flag & VISNAMEDSTREAM) {
		return EPERM;
	}
#endif

	if (!(options & XATTR_NOSECURITY)) {
#if CONFIG_MACF
		error = mac_vnode_check_listextattr(context, vp);
		if (error) {
			goto out;
		}
#endif          

		error = vnode_authorize(vp, NULL, KAUTH_VNODE_READ_EXTATTRIBUTES, context);
		if (error) {
			goto out;
		}
	}

	error = VNOP_LISTXATTR(vp, uio, size, options, context);
	if (error == ENOTSUP && !(options & XATTR_NODEFAULT)) {
		  
                                                                       
                                                                        
                                                        
     
		error = default_listxattr(vp, uio, size, options, context);
	}
out:
	return error;
}

int
xattr_validatename(const char *name)
{
	size_t namelen;

	if (name == NULL || name[0] == '\0') {
		return EINVAL;
	}
	namelen = strlen(name);

	if (utf8_validatestr((const unsigned char *)name, namelen) != 0) {
		return EINVAL;
	}

	return 0;
}


  
                                                           
   
int
xattr_protected(const char *attrname)
{
	return !strncmp(attrname, "com.apple.system.", 17);
}


static void
vnode_setasnamedstream_internal(vnode_t vp, vnode_t svp)
{
	uint32_t streamflags = VISNAMEDSTREAM;

	if ((vp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) == 0) {
		streamflags |= VISSHADOW;
	}

	                    
	vnode_lock_spin(svp);
	svp->v_flag |= streamflags;
	vnode_unlock(svp);

	                                                                           
	vnode_lock_spin(vp);
	vp->v_lflag |= VL_HASSTREAMS;
	vnode_unlock(vp);

	                             
                                                           
                                                             
                                                        
                                                             
                                                             
         
    
	vnode_update_identity(svp, vp, NULL, 0, 0, VNODE_UPDATE_NAMEDSTREAM_PARENT);

	if (vnode_isdyldsharedcache(vp)) {
		vnode_lock_spin(svp);
		svp->v_flag |= VSHARED_DYLD;
		vnode_unlock(svp);
	}

	return;
}

errno_t
vnode_setasnamedstream(vnode_t vp, vnode_t svp)
{
	if ((vp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) == 0) {
		return EINVAL;
	}

	vnode_setasnamedstream_internal(vp, svp);
	return 0;
}

#if NAMEDSTREAMS

  
                                       
   
errno_t
vnode_getnamedstream(vnode_t vp, vnode_t *svpp, const char *name, enum nsoperation op, int flags, vfs_context_t context)
{
	int error;

	if (vp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) {
		error = VNOP_GETNAMEDSTREAM(vp, svpp, name, op, flags, context);
	} else {
		if (flags) {
			error = ENOTSUP;
		} else {
			error = default_getnamedstream(vp, svpp, name, op, context);
		}
	}

	if (error == 0) {
		vnode_setasnamedstream_internal(vp, *svpp);
	}

	return error;
}

  
                                    
   
errno_t
vnode_makenamedstream(vnode_t vp, vnode_t *svpp, const char *name, int flags, vfs_context_t context)
{
	int error;

	if (vp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) {
		error = VNOP_MAKENAMEDSTREAM(vp, svpp, name, flags, context);
	} else {
		error = default_makenamedstream(vp, svpp, name, context);
	}

	if (error == 0) {
		vnode_setasnamedstream_internal(vp, *svpp);
	}

	return error;
}

  
                                       
   
errno_t
vnode_removenamedstream(vnode_t vp, vnode_t svp, const char *name, int flags, vfs_context_t context)
{
	int error;

	if (vp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) {
		error = VNOP_REMOVENAMEDSTREAM(vp, svp, name, flags, context);
	} else {
		error = default_removenamedstream(vp, name, context);
	}

	return error;
}

#define NS_IOBUFSIZE  (128 * 1024)

  
                                      
  
                                                                     
                                                                    
                                                                     
                                                                    
                                                                     
                                                                      
                                                                        
                                           
   
errno_t
vnode_relenamedstream(vnode_t vp, vnode_t svp)
{
	vnode_t dvp;
	struct componentname cn;
	char tmpname[80];
	errno_t err;

	  
                                                                    
                                                                          
                                                                           
                                                                          
                                  
    
	vfs_context_t kernelctx = vfs_context_kernel();

	cache_purge(svp);

	vnode_lock(svp);
	MAKE_SHADOW_NAME(vp, tmpname);
	vnode_unlock(svp);

	cn.cn_nameiop = DELETE;
	cn.cn_flags = ISLASTCN;
	cn.cn_context = kernelctx;
	cn.cn_pnbuf = tmpname;
	cn.cn_pnlen = sizeof(tmpname);
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_namelen = (int)strlen(tmpname);

	  
                                                                  
                                          
    
	err = get_shadow_dir(&dvp);
	if (err != 0) {
		return err;
	}

	(void) VNOP_REMOVE(dvp, svp, &cn, 0, kernelctx);
	vnode_put(dvp);

	return 0;
}

  
                                    
  
                                        
                                    
   
errno_t
vnode_flushnamedstream(vnode_t vp, vnode_t svp, vfs_context_t context)
{
	struct vnode_attr va;
	uio_t auio = NULL;
	caddr_t  bufptr = NULL;
	size_t  bufsize = 0;
	size_t  offset;
	size_t  iosize;
	size_t datasize;
	int error;
	  
                                                                  
                                
    
	vfs_context_t kernelctx = vfs_context_kernel();

	                                                                   

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_data_size);
	if (VNOP_GETATTR(svp, &va, context) != 0 ||
	    !VATTR_IS_SUPPORTED(&va, va_data_size)) {
		return 0;
	}
	if (va.va_data_size > UINT32_MAX) {
		return EINVAL;
	}
	datasize = (size_t)va.va_data_size;
	if (datasize == 0) {
		(void) default_removexattr(vp, XATTR_RESOURCEFORK_NAME, 0, context);
		return 0;
	}

	iosize = bufsize = MIN(datasize, NS_IOBUFSIZE);
	if (kmem_alloc(kernel_map, (vm_offset_t *)&bufptr, bufsize, VM_KERN_MEMORY_FILE)) {
		return ENOMEM;
	}
	auio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ);
	offset = 0;

	  
                                                            
    
	error = VNOP_OPEN(svp, 0, kernelctx);
	if (error) {
		printf("vnode_flushnamedstream: err %d opening file\n", error);
		goto out;
	}
	while (offset < datasize) {
		iosize = MIN(datasize - offset, iosize);

		uio_reset(auio, offset, UIO_SYSSPACE, UIO_READ);
		uio_addiov(auio, (uintptr_t)bufptr, iosize);
		error = VNOP_READ(svp, auio, 0, kernelctx);
		if (error) {
			break;
		}
		                                                                       
		if (offset == 0) {
			error = default_removexattr(vp, XATTR_RESOURCEFORK_NAME, 0, context);
			if ((error != 0) && (error != ENOATTR)) {
				break;
			}
		}
		uio_reset(auio, offset, UIO_SYSSPACE, UIO_WRITE);
		uio_addiov(auio, (uintptr_t)bufptr, iosize);
		error = vn_setxattr(vp, XATTR_RESOURCEFORK_NAME, auio, XATTR_NOSECURITY, context);
		if (error) {
			break;
		}
		offset += iosize;
	}

	                      
	(void) VNOP_CLOSE(svp, 0, kernelctx);
out:
	if (bufptr) {
		kmem_free(kernel_map, (vm_offset_t)bufptr, bufsize);
	}
	if (auio) {
		uio_free(auio);
	}
	return error;
}


  
                                                                 
                                                                    
                                                                    
   
errno_t
vnode_verifynamedstream(vnode_t vp)
{
	int error;
	struct vnode *shadow_dvp = NULL;
	struct vnode *shadowfile = NULL;
	struct componentname cn;

	  
                                                                    
                                                                          
                                                                           
                                                                          
                                  
    
	vfs_context_t kernelctx = vfs_context_kernel();
	char tmpname[80];


	                                    
	error = get_shadow_dir(&shadow_dvp);
	if (error) {
		return error;
	}

	                                               
	MAKE_SHADOW_NAME(vp, tmpname);

	                                
	bzero(&cn, sizeof(cn));
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = ISLASTCN | CN_ALLOWRSRCFORK;
	cn.cn_context = kernelctx;
	cn.cn_pnbuf = tmpname;
	cn.cn_pnlen = sizeof(tmpname);
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_namelen = (int)strlen(tmpname);

	if (VNOP_LOOKUP(shadow_dvp, &shadowfile, &cn, kernelctx) == 0) {
		                              
		if (shadowfile == vp) {
			error = 0;
		} else {
			error = EPERM;
		}
		                               
		vnode_put(shadowfile);
	}

	                                
	vnode_put(shadow_dvp);
	return error;
}

  
                                              
  
                                                                               
                              
  
                                                                             
                                   
  
                                                                       
   
static int
getshadowfile(vnode_t vp, vnode_t *svpp, int makestream, size_t *rsrcsize,
    int *creator, vfs_context_t context)
{
	vnode_t  dvp = NULLVP;
	vnode_t  svp = NULLVP;
	struct componentname cn;
	struct vnode_attr va;
	char tmpname[80];
	size_t datasize = 0;
	int  error = 0;
	int retries = 0;
	vfs_context_t kernelctx = vfs_context_kernel();

retry_create:
	*creator = 0;
	                                   
	MAKE_SHADOW_NAME(vp, tmpname);
	bzero(&cn, sizeof(cn));
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = ISLASTCN;
	cn.cn_context = context;
	cn.cn_pnbuf = tmpname;
	cn.cn_pnlen = sizeof(tmpname);
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_namelen = (int)strlen(tmpname);

	                                                         
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_uid);
	VATTR_WANTED(&va, va_gid);
	VATTR_WANTED(&va, va_mode);
	VATTR_WANTED(&va, va_create_time);
	VATTR_WANTED(&va, va_modify_time);
	if (VNOP_GETATTR(vp, &va, context) != 0 ||
	    !VATTR_IS_SUPPORTED(&va, va_uid) ||
	    !VATTR_IS_SUPPORTED(&va, va_gid) ||
	    !VATTR_IS_SUPPORTED(&va, va_mode)) {
		va.va_uid = KAUTH_UID_NONE;
		va.va_gid = KAUTH_GID_NONE;
		va.va_mode = S_IRUSR | S_IWUSR;
	}
	va.va_vaflags = VA_EXCLUSIVE;
	VATTR_SET(&va, va_type, VREG);
	                                                           
	VATTR_SET(&va, va_flags, UF_HIDDEN);

	                                                      
	if (get_shadow_dir(&dvp) != 0) {
		error = ENOTDIR;
		goto out;
	}
	if (!makestream) {
		                                              
		if (VNOP_LOOKUP(dvp, &svp, &cn, kernelctx) == 0) {
			                                                
			VATTR_INIT(&va);
			VATTR_WANTED(&va, va_data_size);
			if (VNOP_GETATTR(svp, &va, context) == 0 &&
			    VATTR_IS_SUPPORTED(&va, va_data_size)) {
				goto out;                  
			}
		}

		  
                                                       
                                                        
     
		error = vn_getxattr(vp, XATTR_RESOURCEFORK_NAME, NULL, &datasize,
		    XATTR_NOSECURITY, context);
		  
                                                        
                                                         
                                                         
                                                        
     
		if ((error == ENOATTR) &&
		    (vn_getxattr(vp, XATTR_FINDERINFO_NAME, NULL, &datasize,
		    XATTR_NOSECURITY, context) == 0)) {
			datasize = 0;
			error = 0;
		} else {
			if (error) {
				goto out;
			}

			                                                                       
			if (datasize == 0) {
				error = ENOATTR;
				goto out;
			}
		}
	}
	                                    
	error = VNOP_CREATE(dvp, &svp, &cn, &va, kernelctx);
	if (error == 0) {
		vnode_recycle(svp);
		*creator = 1;
	} else if ((error == EEXIST) && !makestream) {
		error = VNOP_LOOKUP(dvp, &svp, &cn, kernelctx);
	} else if ((error == ENOENT) && !makestream) {
		  
                                                             
                                                        
                                                             
                       
     
		if (retries == 0) {
			retries++;
			if (dvp) {
				vnode_put(dvp);
				dvp = NULLVP;
			}
			if (svp) {
				vnode_put(svp);
				svp = NULLVP;
			}
			goto retry_create;
		}
		                                              
	}

out:
	if (dvp) {
		vnode_put(dvp);
	}
	if (error) {
		                                             
		if (svp) {
			vnode_put(svp);
			svp = NULLVP;
		}
	}
	*svpp = svp;
	if (rsrcsize) {
		*rsrcsize = datasize;
	}
	return error;
}


static int
default_getnamedstream(vnode_t vp, vnode_t *svpp, const char *name, enum nsoperation op, vfs_context_t context)
{
	vnode_t  svp = NULLVP;
	uio_t auio = NULL;
	caddr_t  bufptr = NULL;
	size_t  bufsize = 0;
	size_t  datasize = 0;
	int  creator;
	int  error;

	                                                          
	vfs_context_t kernelctx = vfs_context_kernel();

	  
                                                               
    
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
		*svpp = NULLVP;
		return ENOATTR;
	}
retry:
	  
                                                   
   
                                                                 
                                               
    
	error = getshadowfile(vp, &svp, 0, &datasize, &creator, context);
	if (error) {
		*svpp = NULLVP;
		return error;
	}

	  
                                                          
                                                               
                                                                      
                                                                             
                                                                    
                                                                       
                                                                     
                                                                             
              
    
	if (!creator) {
		vnode_lock(svp);
		if (svp->v_flag & VISNAMEDSTREAM) {
			                              
			vnode_unlock(svp);
			goto out;
		} else {
			                                                                   
			if ((svp->v_flag & VISSHADOW)) {
				  
                                                                             
                                                                          
       
				vnode_unlock(svp);
			} else {
				                                                            
				msleep((caddr_t)&svp->v_parent, &svp->v_lock, PINOD | PDROP,
				    "getnamedstream", NULL);
			}
			vnode_put(svp);
			svp = NULLVP;
			goto retry;
		}
	}

	  
                                                             
    
	if (op == NS_OPEN && datasize != 0) {
		size_t  offset;
		size_t  iosize;

		iosize = bufsize = MIN(datasize, NS_IOBUFSIZE);
		if (kmem_alloc(kernel_map, (vm_offset_t *)&bufptr, bufsize, VM_KERN_MEMORY_FILE)) {
			error = ENOMEM;
			goto out;
		}

		auio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ);
		offset = 0;

		                          
		error = VNOP_OPEN(svp, 0, kernelctx);
		if (error) {
			goto out;
		}
		while (offset < datasize) {
			size_t  tmpsize;

			iosize = MIN(datasize - offset, iosize);

			uio_reset(auio, offset, UIO_SYSSPACE, UIO_READ);
			uio_addiov(auio, (uintptr_t)bufptr, iosize);
			                                  
			error = vn_getxattr(vp, XATTR_RESOURCEFORK_NAME, auio, &tmpsize,
			    XATTR_NOSECURITY, context);
			if (error) {
				break;
			}

			uio_reset(auio, offset, UIO_SYSSPACE, UIO_WRITE);
			uio_addiov(auio, (uintptr_t)bufptr, iosize);
			                                           
			error = VNOP_WRITE(svp, auio, 0, kernelctx);
			if (error) {
				break;
			}
			offset += iosize;
		}

		                       
		(void) VNOP_CLOSE(svp, 0, kernelctx);
	}
out:
	                                                 
	if (creator) {
		if (error == 0) {
			vnode_lock(svp);
			                                                              
			svp->v_flag |= (VISNAMEDSTREAM | VISSHADOW);
			wakeup((caddr_t)&svp->v_parent);
			vnode_unlock(svp);
		} else {
			                                                           
                                                                
                                                               
                                                               
                                                                      
                    
      
			(void)vnode_relenamedstream(vp, svp);
			vnode_lock(svp);
			svp->v_flag |= VISSHADOW;
			wakeup((caddr_t)&svp->v_parent);
			vnode_unlock(svp);
		}
	}

	if (bufptr) {
		kmem_free(kernel_map, (vm_offset_t)bufptr, bufsize);
	}
	if (auio) {
		uio_free(auio);
	}
	if (error) {
		                                             
		if (svp) {
			vnode_put(svp);
			svp = NULLVP;
		}
	}
	*svpp = svp;
	return error;
}

static int
default_makenamedstream(vnode_t vp, vnode_t *svpp, const char *name, vfs_context_t context)
{
	int creator;
	int error;

	  
                                                               
    
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
		*svpp = NULLVP;
		return ENOATTR;
	}

	                                                                          
	error = getshadowfile(vp, svpp, 1, NULL, &creator, context);

	  
                                                         
    
	if ((error == 0) && (*svpp != NULL) && creator) {
		vnode_t svp = *svpp;

		vnode_lock(svp);
		                                                     
		svp->v_flag |= (VISNAMEDSTREAM | VISSHADOW);
		                                                
		wakeup((caddr_t)&svp->v_parent);
		vnode_unlock(svp);
	}

	return error;
}

static int
default_removenamedstream(vnode_t vp, const char *name, vfs_context_t context)
{
	  
                                                               
    
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) != 0) {
		return ENOATTR;
	}
	  
                                            
    
	return default_removexattr(vp, XATTR_RESOURCEFORK_NAME, 0, context);
}

static int
get_shadow_dir(vnode_t *sdvpp)
{
	vnode_t  dvp = NULLVP;
	vnode_t  sdvp = NULLVP;
	struct componentname  cn;
	struct vnode_attr  va;
	char tmpname[80];
	uint32_t  tmp_fsid;
	int  error;
	vfs_context_t kernelctx = vfs_context_kernel();

	bzero(tmpname, sizeof(tmpname));
	MAKE_SHADOW_DIRNAME(rootvnode, tmpname);
	  
                                                                
                                                                                     
                                                                                 
   
                                                                    
                                                    
    
	error = vnode_lookup(tmpname, 0, &sdvp, kernelctx);
	if (error == 0) {
		  
                                                                        
                                                                                 
     
		*sdvpp = sdvp;
		return 0;
	}
	                                                 
	sdvp = NULLVP;
	bzero(tmpname, sizeof(tmpname));

	  
                                                              
            
   
                                                     
    
	if (vnode_lookup(SHADOW_DIR_CONTAINER, 0, &dvp, kernelctx) != 0) {
		error = ENOTSUP;
		goto out;
	}

	  
                                       
                                                
                                               
    
	MAKE_SHADOW_DIR_LEAF(rootvnode, tmpname);
	bzero(&cn, sizeof(cn));
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = ISLASTCN;
	cn.cn_context = kernelctx;
	cn.cn_pnbuf = tmpname;
	cn.cn_pnlen = sizeof(tmpname);
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_namelen = (int)strlen(tmpname);

	  
                                                
    
	VATTR_INIT(&va);
	VATTR_SET(&va, va_uid, 0);
	VATTR_SET(&va, va_gid, 0);
	VATTR_SET(&va, va_mode, S_IRUSR | S_IXUSR);
	VATTR_SET(&va, va_type, VDIR);
	VATTR_SET(&va, va_flags, UF_HIDDEN);
	va.va_vaflags = VA_EXCLUSIVE;

	error = VNOP_MKDIR(dvp, &sdvp, &cn, &va, kernelctx);

	  
                                                         
    
	if (error == EEXIST) {
		                                    
		error = VNOP_LOOKUP(dvp, &sdvp, &cn, kernelctx);
		if (error == 0) {
			                                       
			if (sdvp->v_type != VDIR) {
				goto baddir;
			}
			                                            
			VATTR_INIT(&va);
			VATTR_WANTED(&va, va_fsid);
			if (VNOP_GETATTR(dvp, &va, kernelctx) != 0 ||
			    !VATTR_IS_SUPPORTED(&va, va_fsid)) {
				goto baddir;
			}
			tmp_fsid = va.va_fsid;

			VATTR_INIT(&va);
			VATTR_WANTED(&va, va_uid);
			VATTR_WANTED(&va, va_gid);
			VATTR_WANTED(&va, va_mode);
			VATTR_WANTED(&va, va_fsid);
			VATTR_WANTED(&va, va_dirlinkcount);
			VATTR_WANTED(&va, va_acl);
			                                                          
			va.va_dirlinkcount = 1;
			va.va_acl = (kauth_acl_t) KAUTH_FILESEC_NONE;

			if (VNOP_GETATTR(sdvp, &va, kernelctx) != 0 ||
			    !VATTR_IS_SUPPORTED(&va, va_uid) ||
			    !VATTR_IS_SUPPORTED(&va, va_gid) ||
			    !VATTR_IS_SUPPORTED(&va, va_mode) ||
			    !VATTR_IS_SUPPORTED(&va, va_fsid)) {
				goto baddir;
			}
			  
                                 
                          
                              
                                       
                                   
                                               
      
			if ((va.va_uid != 0) || (va.va_gid != 0) ||
			    (va.va_mode & (S_IWUSR | S_IRWXG | S_IRWXO)) ||
			    (va.va_fsid != tmp_fsid) ||
			    (va.va_dirlinkcount != 1) ||
			    (va.va_acl != (kauth_acl_t) KAUTH_FILESEC_NONE)) {
				goto baddir;
			}
		}
	}
out:
	if (dvp) {
		vnode_put(dvp);
	}
	if (error) {
		                                                  
		if (sdvp) {
			vnode_put(sdvp);
			sdvp = NULLVP;
		}
	}
	*sdvpp = sdvp;
	return error;

baddir:
	                                                       
	++shadow_sequence;                                    
	error = ENOTDIR;
	goto out;
}
#endif                   


#if CONFIG_APPLEDOUBLE
  
                                         
   


  
                                                
                                                               
                                   
                                   
                          
                          
                                                          
                                                           
                     
                                                   
                      
                       
                           
                              
                                 
                                 
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                       
                                              
    
                       
                                             
                       
                       
                       
                       
                       
                
                       
  
                                                               
  
                                                            
                                                                
                                                              
                                                            
   

  
                          
  
                                               
  
                                                                             
  
   

#define ADH_MAGIC     0x00051607
#define ADH_VERSION   0x00020000
#define ADH_MACOSX    "Mac OS X        "

  
                         
   
#define AD_DATA          1                  
#define AD_RESOURCE      2                      
#define AD_REALNAME      3                                        
#define AD_COMMENT       4                             
#define AD_ICONBW        5                               
#define AD_ICONCOLOR     6                       
#define AD_UNUSED        7                 
#define AD_FILEDATES     8                                        
#define AD_FINDERINFO    9                                        
#define AD_MACINFO      10                                       
#define AD_PRODOSINFO   11                                        
#define AD_MSDOSINFO    12                                          
#define AD_AFPNAME      13                                 
#define AD_AFPINFO      14                                    
#define AD_AFPDIRID     15                         
#define AD_ATTRIBUTES   AD_FINDERINFO


#define ATTR_FILE_PREFIX   "._"
#define ATTR_HDR_MAGIC     0x41545452               

#define ATTR_BUF_SIZE      4096                                                                      

                           
#define ATTR_MAX_SIZE      AD_XATTR_MAXSIZE
#define ATTR_MAX_HDR_SIZE  65536
  
                                                          
                                                           
                                                           
                                                          
                                                           
                                            
  
                                                           
                                                             
                                                           
                                                             
                                                             
                                                              
                                                 
   


#define FINDERINFOSIZE  32

typedef struct apple_double_entry {
	u_int32_t   type;                                          
	u_int32_t   offset;                                                          
	u_int32_t   length;                                    
} __attribute__((aligned(2), packed)) apple_double_entry_t;


typedef struct apple_double_header {
	u_int32_t   magic;                           
	u_int32_t   version;                                           
	u_int32_t   filler[4];
	u_int16_t   numEntries;                                        
	apple_double_entry_t   entries[2];                                     
	u_int8_t    finfo[FINDERINFOSIZE];                                              
	u_int8_t    pad[2];                                                     
} __attribute__((aligned(2), packed)) apple_double_header_t;

#define ADHDRSIZE  (4+4+16+2)

                                              
typedef struct attr_entry {
	u_int32_t   offset;                              
	u_int32_t   length;                                 
	u_int16_t   flags;
	u_int8_t    namelen;
	u_int8_t    name[1];                                                          
} __attribute__((aligned(2), packed)) attr_entry_t;


                                                                      
typedef struct attr_header {
	apple_double_header_t  appledouble;
	u_int32_t   magic;                               
	u_int32_t   debug_tag;                                                 
	u_int32_t   total_size;                                                                
	u_int32_t   data_start;                                           
	u_int32_t   data_length;                                     
	u_int32_t   reserved[3];
	u_int16_t   flags;
	u_int16_t   num_attrs;
} __attribute__((aligned(2), packed)) attr_header_t;


                                
typedef struct rsrcfork_header {
	u_int32_t    fh_DataOffset;
	u_int32_t    fh_MapOffset;
	u_int32_t    fh_DataLength;
	u_int32_t    fh_MapLength;
	u_int8_t     systemData[112];
	u_int8_t     appData[128];
	u_int32_t    mh_DataOffset;
	u_int32_t    mh_MapOffset;
	u_int32_t    mh_DataLength;
	u_int32_t    mh_MapLength;
	u_int32_t    mh_Next;
	u_int16_t    mh_RefNum;
	u_int8_t     mh_Attr;
	u_int8_t     mh_InMemoryAttr;
	u_int16_t    mh_Types;
	u_int16_t    mh_Names;
	u_int16_t    typeCount;
} __attribute__((aligned(2), packed)) rsrcfork_header_t;

#define RF_FIRST_RESOURCE    256
#define RF_NULL_MAP_LENGTH    30
#define RF_EMPTY_TAG  "This resource fork intentionally left blank   "

                                                   
typedef struct attr_info {
	vfs_context_t          context;
	vnode_t                filevp;
	size_t                 filesize;
	size_t                 iosize;
	u_int8_t               *rawdata;
	size_t                 rawsize;                                                
	apple_double_header_t  *filehdr;
	apple_double_entry_t   *finderinfo;
	apple_double_entry_t   *rsrcfork;
	attr_header_t          *attrhdr;
	attr_entry_t           *attr_entry;
	u_int8_t               readonly;
	u_int8_t               emptyfinderinfo;
} attr_info_t;


#define ATTR_SETTING  1

#define ATTR_ALIGN 3L                               

#define ATTR_ENTRY_LENGTH(namelen)  \
	((sizeof(attr_entry_t) - 1 + (namelen) + ATTR_ALIGN) & (~ATTR_ALIGN))

#define ATTR_NEXT(ae)  \
	 (attr_entry_t *)((u_int8_t *)(ae) + ATTR_ENTRY_LENGTH((ae)->namelen))

#define ATTR_VALID(ae, ai)  \
	((u_int8_t *)ATTR_NEXT(ae) <= ((ai).rawdata + (ai).rawsize))

#define SWAP16(x)  OSSwapBigToHostInt16((x))
#define SWAP32(x)  OSSwapBigToHostInt32((x))
#define SWAP64(x)  OSSwapBigToHostInt64((x))


static u_int32_t emptyfinfo[8] = {0};


  
                         
   
static void  close_xattrfile(vnode_t xvp, int fileflags, vfs_context_t context);

static int  open_xattrfile(vnode_t vp, int fileflags, vnode_t *xvpp, vfs_context_t context);

static int  create_xattrfile(vnode_t xvp, u_int32_t fileid, vfs_context_t context);

static int  remove_xattrfile(vnode_t xvp, vfs_context_t context);

static int  get_xattrinfo(vnode_t xvp, int setting, attr_info_t *ainfop, vfs_context_t context);

static void  rel_xattrinfo(attr_info_t *ainfop);

static int  write_xattrinfo(attr_info_t *ainfop);

static void  init_empty_resource_fork(rsrcfork_header_t * rsrcforkhdr);

static int  lock_xattrfile(vnode_t xvp, short locktype, vfs_context_t context);

static int  unlock_xattrfile(vnode_t xvp, vfs_context_t context);


#if BYTE_ORDER == LITTLE_ENDIAN
static void  swap_adhdr(apple_double_header_t *adh);
static void  swap_attrhdr(attr_header_t *ah, attr_info_t* info);

#else
#define swap_adhdr(x)
#define swap_attrhdr(x, y)
#endif

static int  check_and_swap_attrhdr(attr_header_t *ah, attr_info_t* ainfop);
static int  shift_data_down(vnode_t xvp, off_t start, size_t len, off_t delta, vfs_context_t context);
static int  shift_data_up(vnode_t xvp, off_t start, size_t len, off_t delta, vfs_context_t context);


  
                                                                               
                                                                         
                
                  
                      
                                              
  
                                                 
  
                                                                         
                                            
   
static int
check_and_swap_apple_double_header(attr_info_t *ainfop)
{
	int i, j;
	u_int32_t header_end;
	u_int32_t entry_end;
	size_t rawsize;
	apple_double_header_t *header;

	rawsize = ainfop->rawsize;
	header = (apple_double_header_t *) ainfop->rawdata;

	                                                              
	if (rawsize < offsetof(apple_double_header_t, entries)) {
		return ENOATTR;
	}

	                                                        
	header->magic = SWAP32(header->magic);
	header->version = SWAP32(header->version);
	header->numEntries = SWAP16(header->numEntries);

	                                                
	if (header->magic != ADH_MAGIC ||
	    header->version != ADH_VERSION ||
	    header->numEntries < 1 ||
	    header->numEntries > 15) {
		return ENOATTR;
	}

	                                              
	header_end = offsetof(apple_double_header_t, entries) +
	    header->numEntries * sizeof(apple_double_entry_t);

	                                                                
	if (rawsize < header_end) {
		return ENOATTR;
	}

	                                                  
	for (i = 0; i < header->numEntries; i++) {
		                                               
		header->entries[i].type   = SWAP32(header->entries[i].type);
		header->entries[i].offset = SWAP32(header->entries[i].offset);
		header->entries[i].length = SWAP32(header->entries[i].length);

		entry_end = header->entries[i].offset + header->entries[i].length;

		  
                                                             
                                                           
                                     
     
		if (header->entries[i].offset < header_end ||
		    entry_end < header->entries[i].offset ||
		    entry_end > ainfop->filesize) {
			return ENOATTR;
		}

		  
                                                             
                     
    
                                                                  
                                                              
                                                                  
                                                           
     

		for (j = 0; j < i; j++) {
			if (entry_end > header->entries[j].offset &&
			    header->entries[j].offset + header->entries[j].length > header->entries[i].offset) {
				return ENOATTR;
			}
		}
	}

	return 0;
}



  
                                              
   
static int
default_getxattr(vnode_t vp, const char *name, uio_t uio, size_t *size,
    __unused int options, vfs_context_t context)
{
	vnode_t xvp = NULL;
	attr_info_t ainfo;
	attr_header_t *header;
	attr_entry_t *entry;
	u_int8_t *attrdata;
	u_int32_t datalen;
	size_t namelen;
	int isrsrcfork;
	int fileflags;
	int i;
	int error;

	fileflags = FREAD;
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) == 0) {
		isrsrcfork = 1;
		  
                                                   
                                                     
                                             
     
		fileflags |= O_SHLOCK;
	} else {
		isrsrcfork = 0;
	}

	if ((error = open_xattrfile(vp, fileflags, &xvp, context))) {
		return error;
	}
	if ((error = get_xattrinfo(xvp, 0, &ainfo, context))) {
		close_xattrfile(xvp, fileflags, context);
		return error;
	}

	                          
	if (strncmp(name, XATTR_FINDERINFO_NAME, sizeof(XATTR_FINDERINFO_NAME)) == 0) {
		if (ainfo.finderinfo == NULL || ainfo.emptyfinderinfo) {
			error = ENOATTR;
		} else if (uio == NULL) {
			*size = FINDERINFOSIZE;
			error = 0;
		} else if (uio_offset(uio) != 0) {
			error = EINVAL;
		} else if (uio_resid(uio) < FINDERINFOSIZE) {
			error = ERANGE;
		} else {
			attrdata = (u_int8_t*)ainfo.filehdr + ainfo.finderinfo->offset;
			error = uiomove((caddr_t)attrdata, FINDERINFOSIZE, uio);
		}
		goto out;
	}

	                             
	if (isrsrcfork) {
		if (!vnode_isreg(vp)) {
			error = EPERM;
		} else if (ainfo.rsrcfork == NULL) {
			error = ENOATTR;
		} else if (uio == NULL) {
			*size = (size_t)ainfo.rsrcfork->length;
		} else {
			uio_setoffset(uio, uio_offset(uio) + ainfo.rsrcfork->offset);
			error = VNOP_READ(xvp, uio, 0, context);
			if (error == 0) {
				uio_setoffset(uio, uio_offset(uio) - ainfo.rsrcfork->offset);
			}
		}
		goto out;
	}

	if (ainfo.attrhdr == NULL || ainfo.attr_entry == NULL) {
		error = ENOATTR;
		goto out;
	}
	if (uio_offset(uio) != 0) {
		error = EINVAL;
		goto out;
	}
	error = ENOATTR;
	namelen = strlen(name) + 1;
	header = ainfo.attrhdr;
	entry = ainfo.attr_entry;
	  
                                            
    
	for (i = 0; i < header->num_attrs && ATTR_VALID(entry, ainfo); i++) {
		if (strncmp((const char *)entry->name, name, namelen) == 0) {
			datalen = entry->length;
			if (uio == NULL) {
				*size = datalen;
				error = 0;
				break;
			}
			if (uio_resid(uio) < (user_ssize_t)datalen) {
				error = ERANGE;
				break;
			}
			if (entry->offset + datalen < ATTR_MAX_HDR_SIZE) {
				attrdata = ((u_int8_t *)header + entry->offset);
				error = uiomove((caddr_t)attrdata, datalen, uio);
			} else {
				uio_setoffset(uio, entry->offset);
				error = VNOP_READ(xvp, uio, 0, context);
				uio_setoffset(uio, 0);
			}
			break;
		}
		entry = ATTR_NEXT(entry);
	}
out:
	rel_xattrinfo(&ainfo);
	close_xattrfile(xvp, fileflags, context);

	return error;
}

  
                                         
   
static int
default_setxattr(vnode_t vp, const char *name, uio_t uio, int options, vfs_context_t context)
{
	vnode_t xvp = NULL;
	attr_info_t ainfo;
	attr_header_t *header;
	attr_entry_t *entry;
	attr_entry_t *lastentry;
	u_int8_t *attrdata;
	size_t datalen;
	size_t entrylen;
	size_t datafreespace;
	int namelen;
	int found = 0;
	int i;
	int splitdata;
	int fileflags;
	int error;
	char finfo[FINDERINFOSIZE];

	datalen = uio_resid(uio);
	if (datalen > XATTR_MAXSIZE) {
		return EINVAL;
	}
	namelen = (int)strlen(name) + 1;
	if (namelen > UINT8_MAX) {
		return EINVAL;
	}
	entrylen = ATTR_ENTRY_LENGTH(namelen);

	  
                                                                      
                                                                       
                                                                    
                                                                       
                           
   
                                                                      
                                                                   
                                                                      
                                                                        
                                                                       
   
                                                                  
    
	if (strncmp(name, XATTR_FINDERINFO_NAME, sizeof(XATTR_FINDERINFO_NAME)) == 0) {
		  
                                                          
                                                                     
     
		if (uio_offset(uio) != 0 || datalen != FINDERINFOSIZE) {
			return EINVAL;
		}
		error = uiomove(finfo, (int)datalen, uio);
		if (error) {
			return error;
		}
		if ((options & (XATTR_CREATE | XATTR_REPLACE)) == 0 &&
		    bcmp(finfo, emptyfinfo, FINDERINFOSIZE) == 0) {
			error = default_removexattr(vp, name, 0, context);
			if (error == ENOATTR) {
				error = 0;
			}
			return error;
		}
	}

start:
	  
                                                   
                                                   
    
	fileflags = FREAD | FWRITE | O_EXLOCK;
	if ((error = open_xattrfile(vp, O_CREAT | fileflags, &xvp, context))) {
		return error;
	}
	if ((error = get_xattrinfo(xvp, ATTR_SETTING, &ainfo, context))) {
		close_xattrfile(xvp, fileflags, context);
		return error;
	}

	                          
	if (strncmp(name, XATTR_FINDERINFO_NAME, sizeof(XATTR_FINDERINFO_NAME)) == 0) {
		if (ainfo.finderinfo && !ainfo.emptyfinderinfo) {
			                                             
			if (options & XATTR_CREATE) {
				error = EEXIST;
				goto out;
			}
		} else {
			                                                      
			if (options & XATTR_REPLACE) {
				error = ENOATTR;
				goto out;
			}
		}
		if (options != 0 && bcmp(finfo, emptyfinfo, FINDERINFOSIZE) == 0) {
			  
                                                            
                                                
                                                         
                                                   
     
                                                    
                                                       
              
     
                                                           
      
			rel_xattrinfo(&ainfo);
			close_xattrfile(xvp, fileflags, context);
			error = default_removexattr(vp, name, 0, context);
			if (error == ENOATTR) {
				error = 0;
			}
			return error;
		}
		if (ainfo.finderinfo) {
			attrdata = (u_int8_t *)ainfo.filehdr + ainfo.finderinfo->offset;
			bcopy(finfo, attrdata, datalen);
			ainfo.iosize = sizeof(attr_header_t);
			error = write_xattrinfo(&ainfo);
			goto out;
		}
		error = ENOATTR;
		goto out;
	}

	                              
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) == 0) {
		off_t endoffset;

		if (!vnode_isreg(vp)) {
			error = EPERM;
			goto out;
		}
		                                             
		if (ainfo.rsrcfork == NULL) {
			error = ENOATTR;
			goto out;
		}
		if (ainfo.rsrcfork) {
			if (ainfo.rsrcfork->length != 0) {
				if (options & XATTR_CREATE) {
					                                         
					error = EEXIST;
					goto out;
				}
			} else {
				                              
				if (options & XATTR_REPLACE) {
					                                                            
					error = ENOATTR;
					goto out;
				}
			}
		} else {
			                                                                  
			error = ENOATTR;
			goto out;
		}

		endoffset = uio_resid(uio) + uio_offset(uio);               
		if (endoffset > UINT32_MAX || endoffset < 0) {
			error = EINVAL;
			goto out;
		}
		uio_setoffset(uio, uio_offset(uio) + ainfo.rsrcfork->offset);
		error = VNOP_WRITE(xvp, uio, 0, context);
		if (error) {
			goto out;
		}
		uio_setoffset(uio, uio_offset(uio) - ainfo.rsrcfork->offset);
		if (endoffset > ainfo.rsrcfork->length) {
			ainfo.rsrcfork->length = (u_int32_t)endoffset;
			ainfo.iosize = sizeof(attr_header_t);
			error = write_xattrinfo(&ainfo);
			goto out;
		}
		goto out;
	}

	if (datalen > ATTR_MAX_SIZE) {
		return E2BIG;                        
	}

	if (ainfo.attrhdr == NULL) {
		error = ENOATTR;
		goto out;
	}
	header = ainfo.attrhdr;
	entry = ainfo.attr_entry;

	                                                         
	if ((header->data_start + header->data_length + entrylen + datalen) > ATTR_MAX_HDR_SIZE) {
		splitdata = 1;                              
	} else {
		splitdata = 0;
	}

	  
                                    
    
	for (i = 0; i < header->num_attrs && ATTR_VALID(entry, ainfo); i++) {
		if (strncmp((const char *)entry->name, name, namelen) == 0) {
			found = 1;
			break;
		}
		entry = ATTR_NEXT(entry);
	}

	if (found) {
		if (options & XATTR_CREATE) {
			error = EEXIST;
			goto out;
		}
		if (datalen == entry->length) {
			if (splitdata) {
				uio_setoffset(uio, entry->offset);
				error = VNOP_WRITE(xvp, uio, 0, context);
				uio_setoffset(uio, 0);
				if (error) {
					printf("setxattr: VNOP_WRITE error %d\n", error);
				}
			} else {
				attrdata = (u_int8_t *)header + entry->offset;
				error = uiomove((caddr_t)attrdata, (int)datalen, uio);
				if (error) {
					goto out;
				}
				ainfo.iosize = ainfo.attrhdr->data_start + ainfo.attrhdr->data_length;
				error = write_xattrinfo(&ainfo);
				if (error) {
					printf("setxattr: write_xattrinfo error %d\n", error);
				}
			}
			goto out;
		} else {
			  
                                                                     
      
			found = 0;
			rel_xattrinfo(&ainfo);
			close_xattrfile(xvp, fileflags, context);
			error = default_removexattr(vp, name, options, context);
			if (error) {
				return error;
			}
			                                                                     
			options &= ~XATTR_REPLACE;
			goto start;                 
		}
	}

	if (options & XATTR_REPLACE) {
		error = ENOATTR;                                
		goto out;
	}
	                                                  
	if ((header->data_start + entrylen) > ATTR_MAX_HDR_SIZE) {
		error = ENOSPC;
		goto out;
	}

	datafreespace = header->total_size - (header->data_start + header->data_length);

	                                  
	if ((datalen + entrylen) > datafreespace) {
		size_t growsize;

		growsize = roundup((datalen + entrylen) - datafreespace, ATTR_BUF_SIZE);

		                                                                   
		if (!splitdata && (header->total_size + growsize) > ATTR_MAX_HDR_SIZE) {
			growsize = ATTR_MAX_HDR_SIZE - header->total_size;
		}

		ainfo.filesize += growsize;
		error = vnode_setsize(xvp, ainfo.filesize, 0, context);
		if (error) {
			printf("setxattr: VNOP_TRUNCATE error %d\n", error);
		}
		if (error) {
			goto out;
		}

		  
                                           
     
		if (ainfo.rsrcfork) {
			if (ainfo.rsrcfork->length != 0) {
				shift_data_down(xvp,
				    ainfo.rsrcfork->offset,
				    ainfo.rsrcfork->length,
				    growsize, context);
			}
			ainfo.rsrcfork->offset += growsize;
		}
		ainfo.finderinfo->length += growsize;
		header->total_size += growsize;
	}

	                                 
	if (splitdata) {
		shift_data_down(xvp,
		    header->data_start,
		    header->data_length,
		    entrylen, context);
	} else {
		bcopy((u_int8_t *)header + header->data_start,
		    (u_int8_t *)header + header->data_start + entrylen,
		    header->data_length);
	}
	header->data_start += entrylen;

	                                
	lastentry = entry;
	for (entry = ainfo.attr_entry; entry != lastentry && ATTR_VALID(entry, ainfo); entry = ATTR_NEXT(entry)) {
		entry->offset += entrylen;
	}

	  
                                                 
                                                   
                                                    
    
	if (splitdata) {
		off_t offset;

		                                                              
		offset = header->data_start + header->data_length;
		uio_setoffset(uio, offset);
		error = VNOP_WRITE(xvp, uio, 0, context);
		uio_setoffset(uio, 0);
		if (error) {
			printf("setxattr: VNOP_WRITE error %d\n", error);
			goto out;
		}
	} else {
		attrdata = (u_int8_t *)header + header->data_start + header->data_length;

		error = uiomove((caddr_t)attrdata, (int)datalen, uio);
		if (error) {
			printf("setxattr: uiomove error %d\n", error);
			goto out;
		}
	}

	                                 
	lastentry->length = (u_int32_t)datalen;
	lastentry->offset = header->data_start + header->data_length;
	lastentry->namelen = (u_int8_t)namelen;
	lastentry->flags = 0;
	bcopy(name, &lastentry->name[0], namelen);

	                                   
	header->num_attrs++;
	header->data_length += datalen;

	if (splitdata) {
		                                                                    
		ainfo.iosize = ainfo.attrhdr->data_start;
	} else {
		                                                                     
		ainfo.iosize = ainfo.attrhdr->data_start + ainfo.attrhdr->data_length;
	}
	error = write_xattrinfo(&ainfo);
	if (error) {
		printf("setxattr: write_xattrinfo error %d\n", error);
	}

out:
	rel_xattrinfo(&ainfo);
	close_xattrfile(xvp, fileflags, context);

	                                                       
	if (error == 0) {
		struct vnode_attr va;

		                                                 
		VATTR_INIT(&va);
		VATTR_WANTED(&va, va_modify_time);
		if (vnode_getattr(vp, &va, context) == 0) {
			VATTR_INIT(&va);
			VATTR_SET(&va, va_modify_time, va.va_modify_time);
			(void) vnode_setattr(vp, &va, context);
		}
	}

	post_event_if_success(vp, error, NOTE_ATTRIB);

	return error;
}


  
                                
   
static int
default_removexattr(vnode_t vp, const char *name, __unused int options, vfs_context_t context)
{
	vnode_t xvp = NULL;
	attr_info_t ainfo;
	attr_header_t *header;
	attr_entry_t *entry;
	attr_entry_t *oldslot;
	u_int8_t *attrdata;
	u_int32_t dataoff;
	size_t datalen;
	size_t entrylen;
	int namelen;
	int found = 0, lastone = 0;
	int i;
	int splitdata;
	int attrcount = 0;
	int isrsrcfork;
	int fileflags;
	int error;

	fileflags = FREAD | FWRITE;
	if (strncmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) == 0) {
		isrsrcfork = 1;
		  
                                                      
                                                     
                                             
     
		fileflags |= O_EXLOCK;
	} else {
		isrsrcfork = 0;
	}

	if ((error = open_xattrfile(vp, fileflags, &xvp, context))) {
		return error;
	}
	if ((error = get_xattrinfo(xvp, 0, &ainfo, context))) {
		close_xattrfile(xvp, fileflags, context);
		return error;
	}
	if (ainfo.attrhdr) {
		attrcount += ainfo.attrhdr->num_attrs;
	}
	if (ainfo.rsrcfork) {
		++attrcount;
	}
	if (ainfo.finderinfo && !ainfo.emptyfinderinfo) {
		++attrcount;
	}

	                            
	if (strncmp(name, XATTR_FINDERINFO_NAME, sizeof(XATTR_FINDERINFO_NAME)) == 0) {
		if (ainfo.finderinfo == NULL || ainfo.emptyfinderinfo) {
			error = ENOATTR;
			goto out;
		}
		                                                          
		if (--attrcount == 0) {
			goto out;
		}
		attrdata = (u_int8_t *)ainfo.filehdr + ainfo.finderinfo->offset;
		bzero((caddr_t)attrdata, FINDERINFOSIZE);
		ainfo.iosize = sizeof(attr_header_t);
		error = write_xattrinfo(&ainfo);
		goto out;
	}

	                              
	if (isrsrcfork) {
		if (!vnode_isreg(vp)) {
			error = EPERM;
			goto out;
		}
		if (ainfo.rsrcfork == NULL || ainfo.rsrcfork->length == 0) {
			error = ENOATTR;
			goto out;
		}
		                                                          
		if (--attrcount == 0) {
			goto out;
		}
		  
        
                                                    
                                                  
                                                  
     
		if ((ainfo.rsrcfork->offset + ainfo.rsrcfork->length) == ainfo.filesize) {
			ainfo.filesize -= ainfo.rsrcfork->length;
			error = vnode_setsize(xvp, ainfo.filesize, 0, context);
		}
		if (error == 0) {
			ainfo.rsrcfork->length = 0;
			ainfo.iosize = sizeof(attr_header_t);
			error = write_xattrinfo(&ainfo);
		}
		goto out;
	}

	if (ainfo.attrhdr == NULL) {
		error = ENOATTR;
		goto out;
	}
	namelen = (int)strlen(name) + 1;
	header = ainfo.attrhdr;
	entry = ainfo.attr_entry;

	  
                                 
    
	for (i = 0; i < header->num_attrs && ATTR_VALID(entry, ainfo); i++) {
		if (strncmp((const char *)entry->name, name, namelen) == 0) {
			found = 1;
			if ((i + 1) == header->num_attrs) {
				lastone = 1;
			}
			break;
		}
		entry = ATTR_NEXT(entry);
	}
	if (!found) {
		error = ENOATTR;
		goto out;
	}
	                                                          
	if (--attrcount == 0) {
		goto out;
	}

	datalen = entry->length;
	dataoff = entry->offset;
	entrylen = ATTR_ENTRY_LENGTH(namelen);
	if ((header->data_start + header->data_length) > ATTR_MAX_HDR_SIZE) {
		splitdata = 1;
	} else {
		splitdata = 0;
	}

	                                 
	if (!lastone) {
		bcopy((u_int8_t *)entry + entrylen, (u_int8_t *)entry,
		    ((size_t)header + header->data_start) - ((size_t)entry + entrylen));
	}

	                                
	if (splitdata) {
		shift_data_up(xvp,
		    header->data_start,
		    dataoff - header->data_start,
		    entrylen,
		    context);
		if (!lastone) {
			shift_data_up(xvp,
			    dataoff + datalen,
			    (header->data_start + header->data_length) - (dataoff + datalen),
			    datalen + entrylen,
			    context);
		}
		                                      
		ainfo.iosize = ainfo.attrhdr->data_start - entrylen;
	} else {
		bcopy((u_int8_t *)header + header->data_start,
		    (u_int8_t *)header + header->data_start - entrylen,
		    dataoff - header->data_start);
		if (!lastone) {
			bcopy((u_int8_t *)header + dataoff + datalen,
			    (u_int8_t *)header + dataoff - entrylen,
			    (header->data_start + header->data_length) - (dataoff + datalen));
		}
		bzero(((u_int8_t *)header + header->data_start + header->data_length) - (datalen + entrylen), (datalen + entrylen));
		ainfo.iosize = ainfo.attrhdr->data_start + ainfo.attrhdr->data_length;
	}

	                                                 
	header->num_attrs--;
	header->data_start -= entrylen;
	header->data_length -= datalen;

	oldslot = entry;
	entry = ainfo.attr_entry;
	for (i = 0; i < header->num_attrs && ATTR_VALID(entry, ainfo); i++) {
		entry->offset -= entrylen;
		if (entry >= oldslot) {
			entry->offset -= datalen;
		}
		entry = ATTR_NEXT(entry);
	}
	error = write_xattrinfo(&ainfo);
	if (error) {
		printf("removexattr: write_xattrinfo error %d\n", error);
	}
out:
	rel_xattrinfo(&ainfo);

	                                                           
	if (attrcount == 0) {
		if (fileflags & O_EXLOCK) {
			(void) unlock_xattrfile(xvp, context);
		}
		VNOP_CLOSE(xvp, fileflags, context);
		vnode_rele(xvp);
		error = remove_xattrfile(xvp, context);
		vnode_put(xvp);
	} else {
		close_xattrfile(xvp, fileflags, context);
	}
	                                                       
	if (error == 0) {
		struct vnode_attr va;

		                                                 
		VATTR_INIT(&va);
		VATTR_WANTED(&va, va_modify_time);
		if (vnode_getattr(vp, &va, context) == 0) {
			VATTR_INIT(&va);
			VATTR_SET(&va, va_modify_time, va.va_modify_time);
			(void) vnode_setattr(vp, &va, context);
		}
	}

	post_event_if_success(vp, error, NOTE_ATTRIB);

	return error;
}


  
                                                 
   
static int
default_listxattr(vnode_t vp, uio_t uio, size_t *size, __unused int options, vfs_context_t context)
{
	vnode_t xvp = NULL;
	attr_info_t ainfo;
	attr_entry_t *entry;
	int i, count;
	int error;

	  
                                                                         
                                                                                  
                                                     
    

	if ((error = open_xattrfile(vp, FREAD, &xvp, context))) {
		if (error == ENOATTR) {
			error = 0;
		}
		return error;
	}
	if ((error = get_xattrinfo(xvp, 0, &ainfo, context))) {
		if (error == ENOATTR) {
			error = 0;
		}
		close_xattrfile(xvp, FREAD, context);
		return error;
	}

	                            
	if (ainfo.finderinfo && !ainfo.emptyfinderinfo) {
		if (uio == NULL) {
			*size += sizeof(XATTR_FINDERINFO_NAME);
		} else if (uio_resid(uio) < (user_ssize_t)sizeof(XATTR_FINDERINFO_NAME)) {
			error = ERANGE;
			goto out;
		} else {
			error = uiomove(XATTR_FINDERINFO_NAME,
			    sizeof(XATTR_FINDERINFO_NAME), uio);
			if (error) {
				error = ERANGE;
				goto out;
			}
		}
	}

	                              
	if (vnode_isreg(vp) && ainfo.rsrcfork) {
		if (uio == NULL) {
			*size += sizeof(XATTR_RESOURCEFORK_NAME);
		} else if (uio_resid(uio) < (user_ssize_t)sizeof(XATTR_RESOURCEFORK_NAME)) {
			error = ERANGE;
			goto out;
		} else {
			error = uiomove(XATTR_RESOURCEFORK_NAME,
			    sizeof(XATTR_RESOURCEFORK_NAME), uio);
			if (error) {
				error = ERANGE;
				goto out;
			}
		}
	}

	                           
	if (ainfo.attrhdr) {
		count = ainfo.attrhdr->num_attrs;
		for (i = 0, entry = ainfo.attr_entry; i < count && ATTR_VALID(entry, ainfo); i++) {
			if (xattr_protected((const char *)entry->name) ||
			    ((entry->namelen < XATTR_MAXNAMELEN) &&
			    (entry->name[entry->namelen] == '\0') &&
			    (xattr_validatename((const char *)entry->name) != 0))) {
				entry = ATTR_NEXT(entry);
				continue;
			}
			if (uio == NULL) {
				*size += entry->namelen;
				entry = ATTR_NEXT(entry);
				continue;
			}
			if (uio_resid(uio) < entry->namelen) {
				error = ERANGE;
				break;
			}
			error = uiomove((caddr_t) entry->name, entry->namelen, uio);
			if (error) {
				if (error != EFAULT) {
					error = ERANGE;
				}
				break;
			}
			entry = ATTR_NEXT(entry);
		}
	}
out:
	rel_xattrinfo(&ainfo);
	close_xattrfile(xvp, FREAD, context);

	return error;
}

static int
open_xattrfile(vnode_t vp, int fileflags, vnode_t *xvpp, vfs_context_t context)
{
	vnode_t xvp = NULLVP;
	vnode_t dvp = NULLVP;
	struct vnode_attr *va = NULL;
	struct nameidata *nd = NULL;
	char smallname[64];
	char *filename = NULL;
	const char *basename = NULL;
	size_t alloc_len;
	size_t copy_len;
	errno_t error;
	int opened = 0;
	int referenced = 0;

	if (vnode_isvroot(vp) && vnode_isdir(vp)) {
		  
                                                             
     
		filename = &smallname[0];
		snprintf(filename, sizeof(smallname), "%s%s", ATTR_FILE_PREFIX, ".");
		dvp = vp;                                              
		goto lookup;
	}
	if ((dvp = vnode_getparent(vp)) == NULLVP) {
		error = ENOATTR;
		goto out;
	}
	if ((basename = vnode_getname(vp)) == NULL) {
		error = ENOATTR;
		goto out;
	}

	                                                 
	if (vp->v_type == VREG && strlen(basename) > 2 &&
	    basename[0] == '.' && basename[1] == '_') {
		error = EPERM;
		goto out;
	}
	filename = &smallname[0];
	alloc_len = snprintf(filename, sizeof(smallname), "%s%s", ATTR_FILE_PREFIX, basename);
	if (alloc_len >= sizeof(smallname)) {
		alloc_len++;                                            
		filename = kheap_alloc(KHEAP_TEMP, alloc_len, Z_WAITOK);
		copy_len = snprintf(filename, alloc_len, "%s%s", ATTR_FILE_PREFIX, basename);
	}
	  
                                                                       
                                                                    
                                                                    
                                                                     
                                                                 
                                                    
    
lookup:
	nd = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
	NDINIT(nd, LOOKUP, OP_OPEN, LOCKLEAF | NOFOLLOW | USEDVP | DONOTAUTH,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(filename), context);
	nd->ni_dvp = dvp;

	va = kheap_alloc(KHEAP_TEMP, sizeof(struct vnode_attr), Z_WAITOK);

	if (fileflags & O_CREAT) {
		nd->ni_cnd.cn_nameiop = CREATE;
#if CONFIG_TRIGGERS
		nd->ni_op = OP_LINK;
#endif
		if (dvp != vp) {
			nd->ni_cnd.cn_flags |= LOCKPARENT;
		}
		if ((error = namei(nd))) {
			nd->ni_dvp = NULLVP;
			error = ENOATTR;
			goto out;
		}
		if ((xvp = nd->ni_vp) == NULLVP) {
			uid_t uid;
			gid_t gid;
			mode_t umode;

			  
                                            
      
			VATTR_INIT(va);
			VATTR_WANTED(va, va_uid);
			VATTR_WANTED(va, va_gid);
			VATTR_WANTED(va, va_mode);
			if (VNOP_GETATTR(vp, va, context) == 0 &&
			    VATTR_IS_SUPPORTED(va, va_uid) &&
			    VATTR_IS_SUPPORTED(va, va_gid) &&
			    VATTR_IS_SUPPORTED(va, va_mode)) {
				uid = va->va_uid;
				gid = va->va_gid;
				umode = va->va_mode & (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			} else {                      
				uid = KAUTH_UID_NONE;
				gid = KAUTH_GID_NONE;
				umode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
			}

			VATTR_INIT(va);
			VATTR_SET(va, va_type, VREG);
			VATTR_SET(va, va_mode, umode);
			if (uid != KAUTH_UID_NONE) {
				VATTR_SET(va, va_uid, uid);
			}
			if (gid != KAUTH_GID_NONE) {
				VATTR_SET(va, va_gid, gid);
			}

			error = vn_create(dvp, &nd->ni_vp, nd, va,
			    VN_CREATE_NOAUTH | VN_CREATE_NOINHERIT | VN_CREATE_NOLABEL,
			    0, NULL,
			    context);
			if (error) {
				error = ENOATTR;
			} else {
				xvp = nd->ni_vp;
			}
		}
		nameidone(nd);
		if (dvp != vp) {
			vnode_put(dvp);                                                  
		}
		if (error) {
			goto out;
		}
	} else {
		if ((error = namei(nd))) {
			nd->ni_dvp = NULLVP;
			error = ENOATTR;
			goto out;
		}
		xvp = nd->ni_vp;
		nameidone(nd);
	}
	nd->ni_dvp = NULLVP;

	if (xvp->v_type != VREG) {
		error = ENOATTR;
		goto out;
	}
	  
                      
    
	VATTR_INIT(va);
	VATTR_WANTED(va, va_uid);
	if (VNOP_GETATTR(vp, va, context) == 0 && VATTR_IS_SUPPORTED(va, va_uid)) {
		uid_t owner = va->va_uid;

		VATTR_INIT(va);
		VATTR_WANTED(va, va_uid);
		if (VNOP_GETATTR(xvp, va, context) == 0 && (owner != va->va_uid)) {
			error = ENOATTR;                                
			goto out;
		}
	}

	if ((error = VNOP_OPEN(xvp, fileflags & ~(O_EXLOCK | O_SHLOCK), context))) {
		error = ENOATTR;
		goto out;
	}
	opened = 1;

	if ((error = vnode_ref(xvp))) {
		goto out;
	}
	referenced = 1;

	                                                            
	if (fileflags & O_CREAT) {
		VATTR_INIT(va);
		VATTR_WANTED(va, va_data_size);
		VATTR_WANTED(va, va_fileid);
		VATTR_WANTED(va, va_nlink);
		if ((error = vnode_getattr(xvp, va, context)) != 0) {
			error = EPERM;
			goto out;
		}

		                                                     
		if (va->va_data_size == 0) {
			                                         
			if (VATTR_IS_SUPPORTED(va, va_nlink) && va->va_nlink > 1) {
				error = EPERM;
				goto out;
			}
			if ((error = create_xattrfile(xvp, (u_int32_t)va->va_fileid, context))) {
				goto out;
			}
		}
	}
	                                      
	if (fileflags & (O_EXLOCK | O_SHLOCK)) {
		short locktype;

		locktype = (fileflags & O_EXLOCK) ? F_WRLCK : F_RDLCK;
		error = lock_xattrfile(xvp, locktype, context);
		if (error) {
			error = ENOATTR;
		}
	}
out:
	if (error) {
		if (xvp != NULLVP) {
			if (opened) {
				(void) VNOP_CLOSE(xvp, fileflags, context);
			}

			if (fileflags & O_CREAT) {
				                                                        
				(void) remove_xattrfile(xvp, context);
			}

			if (referenced) {
				(void) vnode_rele(xvp);
			}
			(void) vnode_put(xvp);
			xvp = NULLVP;
		}
		if ((error == ENOATTR) && (fileflags & O_CREAT)) {
			error = EPERM;
		}
	}
	                                            
	kheap_free(KHEAP_TEMP, nd, sizeof(struct nameidata));
	kheap_free(KHEAP_TEMP, va, sizeof(struct vnode_attr));
	if (dvp && (dvp != vp)) {
		vnode_put(dvp);
	}
	if (basename) {
		vnode_putname(basename);
	}
	if (filename && filename != &smallname[0]) {
		kheap_free(KHEAP_TEMP, filename, alloc_len);
	}

	*xvpp = xvp;                                 
	return error;
}

static void
close_xattrfile(vnode_t xvp, int fileflags, vfs_context_t context)
{
                          
                                              

	if (fileflags & (O_EXLOCK | O_SHLOCK)) {
		(void) unlock_xattrfile(xvp, context);
	}

	(void) VNOP_CLOSE(xvp, fileflags, context);
	(void) vnode_rele(xvp);
	(void) vnode_put(xvp);
}

static int
remove_xattrfile(vnode_t xvp, vfs_context_t context)
{
	vnode_t dvp;
	struct nameidata nd;
	char *path = NULL;
	int pathlen;
	int error = 0;

	path = zalloc(ZV_NAMEI);
	pathlen = MAXPATHLEN;
	error = vn_getpath(xvp, path, &pathlen);
	if (error) {
		zfree(ZV_NAMEI, path);
		return error;
	}

	NDINIT(&nd, DELETE, OP_UNLINK, LOCKPARENT | NOFOLLOW | DONOTAUTH,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(path), context);
	error = namei(&nd);
	zfree(ZV_NAMEI, path);
	if (error) {
		return error;
	}
	dvp = nd.ni_dvp;
	xvp = nd.ni_vp;

	error = VNOP_REMOVE(dvp, xvp, &nd.ni_cnd, 0, context);
	nameidone(&nd);
	vnode_put(dvp);
	vnode_put(xvp);

	return error;
}

  
                                                                         
                                                                       
                                          
  
                        
                                                                         
                                                                     
                                                                         
                                                                      
                                                                
                                                                            
              
                                                            
                                                                             
                                                                              
                
                                                                              
   
static int
get_xattrinfo(vnode_t xvp, int setting, attr_info_t *ainfop, vfs_context_t context)
{
	uio_t auio = NULL;
	void * buffer = NULL;
	apple_double_header_t  *filehdr;
	struct vnode_attr va;
	size_t iosize = 0;
	int i;
	int error;

	bzero(ainfop, sizeof(attr_info_t));
	ainfop->filevp = xvp;
	ainfop->context = context;
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_data_size);
	VATTR_WANTED(&va, va_fileid);
	if ((error = vnode_getattr(xvp, &va, context))) {
		goto bail;
	}
	ainfop->filesize = va.va_data_size;

	                                                                 
	if (setting) {
		iosize = ATTR_MAX_HDR_SIZE;
	} else {
		iosize = MIN(ATTR_MAX_HDR_SIZE, ainfop->filesize);
	}

	if (iosize == 0) {
		error = ENOATTR;
		goto bail;
	}
	ainfop->iosize = iosize;
	buffer = kheap_alloc(KHEAP_DATA_BUFFERS, iosize, Z_WAITOK);
	if (buffer == NULL) {
		error = ENOMEM;
		goto bail;
	}

	auio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ);
	uio_addiov(auio, (uintptr_t)buffer, iosize);

	                           
	error = VNOP_READ(xvp, auio, 0, context);
	if (error) {
		goto bail;
	}
	ainfop->rawsize = iosize - uio_resid(auio);
	ainfop->rawdata = (u_int8_t *)buffer;

	filehdr = (apple_double_header_t *)buffer;

	error = check_and_swap_apple_double_header(ainfop);
	if (error) {
		goto bail;
	}

	ainfop->filehdr = filehdr;                                

	                                                                
	buffer = NULL;

	                                                            
	for (i = 0; i < filehdr->numEntries; ++i) {
		if (filehdr->entries[i].type == AD_FINDERINFO &&
		    filehdr->entries[i].length >= FINDERINFOSIZE) {
			                                     
			ainfop->finderinfo = &filehdr->entries[i];

			                                                                
                                                       
                                           
     
                                                         
                                      
      
			if (((ainfop->finderinfo->offset + FINDERINFOSIZE) > ainfop->finderinfo->offset) &&
			    ((ainfop->finderinfo->offset + FINDERINFOSIZE) <= ainfop->rawsize)) {
				  
                                                       
                                                            
                      
       
				if (bcmp((u_int8_t*)ainfop->filehdr + ainfop->finderinfo->offset, emptyfinfo, sizeof(emptyfinfo)) == 0) {
					ainfop->emptyfinderinfo = 1;
				}
			} else {
				error = ENOATTR;
				goto bail;
			}
		}
		if (filehdr->entries[i].type == AD_RESOURCE) {
			  
                                                                  
                                                              
                                                    
      
			if (filehdr->entries[i].length == 0 && !setting) {
				continue;
			}

			  
                                                                            
     
                                                                        
                                                      
      
			if (filehdr->entries[i].length == sizeof(rsrcfork_header_t) && !setting) {
				uio_t  rf_uio;
				u_int8_t  systemData[64];
				int  rf_err;


				                                                  
				rf_uio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ);
				uio_addiov(rf_uio, (uintptr_t)systemData, sizeof(systemData));
				uio_setoffset(rf_uio, filehdr->entries[i].offset + 16);
				rf_err = VNOP_READ(xvp, rf_uio, 0, context);
				uio_free(rf_uio);

				if (rf_err != 0 ||
				    bcmp(systemData, RF_EMPTY_TAG, sizeof(RF_EMPTY_TAG)) == 0) {
					continue;                               
				}
			}
			ainfop->rsrcfork = &filehdr->entries[i];
			if (i != (filehdr->numEntries - 1)) {
				printf("get_xattrinfo: resource fork not last entry\n");
				ainfop->readonly = 1;
			}
			continue;
		}
	}

	  
                                                                   
                                                       
   
                                                                    
                                                                      
                                    
   
                                                                     
               
    
	if (filehdr->numEntries == 2 &&
	    ainfop->finderinfo == &filehdr->entries[0] &&
	    ainfop->rsrcfork == &filehdr->entries[1] &&
	    ainfop->finderinfo->offset == offsetof(apple_double_header_t, finfo)) {
		attr_header_t *attrhdr;
		attrhdr = (attr_header_t *)filehdr;
		  
                                                              
                                                                
                                             
     
		if (setting && ainfop->finderinfo->length == FINDERINFOSIZE) {
			size_t delta;
			size_t writesize;

			delta = ATTR_BUF_SIZE - (filehdr->entries[0].offset + FINDERINFOSIZE);
			if (ainfop->rsrcfork && filehdr->entries[1].length) {
				                                                   
				shift_data_down(xvp,
				    filehdr->entries[1].offset,
				    filehdr->entries[1].length,
				    delta, context);
				writesize = sizeof(attr_header_t);
			} else {
				                                        
				rsrcfork_header_t *rsrcforkhdr;

				vnode_setsize(xvp, filehdr->entries[1].offset + delta, 0, context);

				                                              
				delta -= sizeof(rsrcfork_header_t);

				bzero(&attrhdr->appledouble.pad[0], delta);
				rsrcforkhdr = (rsrcfork_header_t *)((char *)filehdr + filehdr->entries[1].offset + delta);

				                                         
				init_empty_resource_fork(rsrcforkhdr);

				filehdr->entries[1].length = sizeof(rsrcfork_header_t);
				writesize = ATTR_BUF_SIZE;
			}
			filehdr->entries[0].length += delta;
			filehdr->entries[1].offset += delta;

			                               
			attrhdr->magic       = ATTR_HDR_MAGIC;
			attrhdr->debug_tag   = (u_int32_t)va.va_fileid;
			attrhdr->total_size  = filehdr->entries[1].offset;
			attrhdr->data_start  = sizeof(attr_header_t);
			attrhdr->data_length = 0;
			attrhdr->reserved[0] = 0;
			attrhdr->reserved[1] = 0;
			attrhdr->reserved[2] = 0;
			attrhdr->flags       = 0;
			attrhdr->num_attrs   = 0;

			                         
			uio_reset(auio, 0, UIO_SYSSPACE, UIO_WRITE);
			uio_addiov(auio, (uintptr_t)filehdr, writesize);

			swap_adhdr(filehdr);                       
			swap_attrhdr(attrhdr, ainfop);                     
			error = VNOP_WRITE(xvp, auio, 0, context);
			swap_adhdr(filehdr);                        
			                                              
		}
	}
	  
                                                           
                                                                 
                                                             
              
   
                                                              
                                                              
                                                            
                                                           
   
                                                          
                     
    
	if (ainfop->finderinfo &&
	    ainfop->finderinfo == &filehdr->entries[0] &&
	    ainfop->finderinfo->length >= (sizeof(attr_header_t) - sizeof(apple_double_header_t))) {
		attr_header_t *attrhdr = (attr_header_t*)filehdr;

		if (ainfop->finderinfo->offset != offsetof(apple_double_header_t, finfo)) {
			error = ENOATTR;
			goto bail;
		}

		if ((error = check_and_swap_attrhdr(attrhdr, ainfop)) == 0) {
			ainfop->attrhdr = attrhdr;                              
			                                                                    
			ainfop->attr_entry = (attr_entry_t *)&attrhdr[1];
		}
	}

	error = 0;
bail:
	if (auio != NULL) {
		uio_free(auio);
	}
	kheap_free(KHEAP_DATA_BUFFERS, buffer, iosize);
	return error;
}


static int
create_xattrfile(vnode_t xvp, u_int32_t fileid, vfs_context_t context)
{
	attr_header_t *xah;
	rsrcfork_header_t *rsrcforkhdr;
	void * buffer;
	uio_t auio;
	int rsrcforksize;
	int error;

	buffer = kheap_alloc(KHEAP_TEMP, ATTR_BUF_SIZE, Z_WAITOK);
	bzero(buffer, ATTR_BUF_SIZE);

	xah = (attr_header_t *)buffer;
	auio = uio_create(1, 0, UIO_SYSSPACE, UIO_WRITE);
	uio_addiov(auio, (uintptr_t)buffer, ATTR_BUF_SIZE);
	rsrcforksize = sizeof(rsrcfork_header_t);
	rsrcforkhdr = (rsrcfork_header_t *) ((char *)buffer + ATTR_BUF_SIZE - rsrcforksize);

	                                  
	xah->appledouble.magic             = SWAP32(ADH_MAGIC);
	xah->appledouble.version           = SWAP32(ADH_VERSION);
	xah->appledouble.numEntries        = SWAP16(2);
	xah->appledouble.entries[0].type   = SWAP32(AD_FINDERINFO);
	xah->appledouble.entries[0].offset = SWAP32(offsetof(apple_double_header_t, finfo));
	xah->appledouble.entries[0].length = SWAP32(ATTR_BUF_SIZE - offsetof(apple_double_header_t, finfo) - rsrcforksize);
	xah->appledouble.entries[1].type   = SWAP32(AD_RESOURCE);
	xah->appledouble.entries[1].offset = SWAP32(ATTR_BUF_SIZE - rsrcforksize);
	xah->appledouble.entries[1].length = SWAP32(rsrcforksize);
	bcopy(ADH_MACOSX, xah->appledouble.filler, sizeof(xah->appledouble.filler));

	                               
	xah->magic       = SWAP32(ATTR_HDR_MAGIC);
	xah->debug_tag   = SWAP32(fileid);
	xah->total_size  = SWAP32(ATTR_BUF_SIZE - rsrcforksize);
	xah->data_start  = SWAP32(sizeof(attr_header_t));

	                                         
	init_empty_resource_fork(rsrcforkhdr);

	                  
	error = VNOP_WRITE(xvp, auio, IO_UNIT, context);

	                                    
	if (uio_resid(auio) > 0) {
		error = ENOSPC;
	}

	uio_free(auio);
	kheap_free(KHEAP_TEMP, buffer, ATTR_BUF_SIZE);

	return error;
}

static void
init_empty_resource_fork(rsrcfork_header_t * rsrcforkhdr)
{
	bzero(rsrcforkhdr, sizeof(rsrcfork_header_t));
	rsrcforkhdr->fh_DataOffset = SWAP32(RF_FIRST_RESOURCE);
	rsrcforkhdr->fh_MapOffset  = SWAP32(RF_FIRST_RESOURCE);
	rsrcforkhdr->fh_MapLength  = SWAP32(RF_NULL_MAP_LENGTH);
	rsrcforkhdr->mh_DataOffset = SWAP32(RF_FIRST_RESOURCE);
	rsrcforkhdr->mh_MapOffset  = SWAP32(RF_FIRST_RESOURCE);
	rsrcforkhdr->mh_MapLength  = SWAP32(RF_NULL_MAP_LENGTH);
	rsrcforkhdr->mh_Types      = SWAP16(RF_NULL_MAP_LENGTH - 2 );
	rsrcforkhdr->mh_Names      = SWAP16(RF_NULL_MAP_LENGTH);
	rsrcforkhdr->typeCount     = SWAP16(-1);
	bcopy(RF_EMPTY_TAG, rsrcforkhdr->systemData, sizeof(RF_EMPTY_TAG));
}

static void
rel_xattrinfo(attr_info_t *ainfop)
{
	kheap_free_addr(KHEAP_DATA_BUFFERS, ainfop->filehdr);
	bzero(ainfop, sizeof(attr_info_t));
}

static int
write_xattrinfo(attr_info_t *ainfop)
{
	uio_t auio;
	int error;

	auio = uio_create(1, 0, UIO_SYSSPACE, UIO_WRITE);
	uio_addiov(auio, (uintptr_t)ainfop->filehdr, ainfop->iosize);

	swap_adhdr(ainfop->filehdr);
	if (ainfop->attrhdr != NULL) {
		swap_attrhdr(ainfop->attrhdr, ainfop);
	}

	error = VNOP_WRITE(ainfop->filevp, auio, 0, ainfop->context);

	swap_adhdr(ainfop->filehdr);
	if (ainfop->attrhdr != NULL) {
		swap_attrhdr(ainfop->attrhdr, ainfop);
	}
	uio_free(auio);

	return error;
}

#if BYTE_ORDER == LITTLE_ENDIAN
  
                                  
   
static void
swap_adhdr(apple_double_header_t *adh)
{
	int count;
	int i;

	count = (adh->magic == ADH_MAGIC) ? adh->numEntries : SWAP16(adh->numEntries);

	adh->magic      = SWAP32(adh->magic);
	adh->version    = SWAP32(adh->version);
	adh->numEntries = SWAP16(adh->numEntries);

	for (i = 0; i < count; i++) {
		adh->entries[i].type   = SWAP32(adh->entries[i].type);
		adh->entries[i].offset = SWAP32(adh->entries[i].offset);
		adh->entries[i].length = SWAP32(adh->entries[i].length);
	}
}

  
                                         
   
static void
swap_attrhdr(attr_header_t *ah, attr_info_t* info)
{
	attr_entry_t *ae;
	int count;
	int i;

	count = (ah->magic == ATTR_HDR_MAGIC) ? ah->num_attrs : SWAP16(ah->num_attrs);

	ah->magic       = SWAP32(ah->magic);
	ah->debug_tag   = SWAP32(ah->debug_tag);
	ah->total_size  = SWAP32(ah->total_size);
	ah->data_start  = SWAP32(ah->data_start);
	ah->data_length = SWAP32(ah->data_length);
	ah->flags       = SWAP16(ah->flags);
	ah->num_attrs   = SWAP16(ah->num_attrs);

	ae = (attr_entry_t *)(&ah[1]);
	for (i = 0; i < count && ATTR_VALID(ae, *info); i++, ae = ATTR_NEXT(ae)) {
		ae->offset = SWAP32(ae->offset);
		ae->length = SWAP32(ae->length);
		ae->flags  = SWAP16(ae->flags);
	}
}
#endif

  
                                                                         
                
  
                                                                              
                                                                         
                                                      
   
static int
check_and_swap_attrhdr(attr_header_t *ah, attr_info_t *ainfop)
{
	attr_entry_t *ae;
	u_int8_t *buf_end;
	u_int32_t end;
	int count;
	int i;

	if (ah == NULL) {
		return EINVAL;
	}

	if (SWAP32(ah->magic) != ATTR_HDR_MAGIC) {
		return EINVAL;
	}

	                                  
	ah->magic       = SWAP32(ah->magic);
	ah->debug_tag   = SWAP32(ah->debug_tag);
	ah->total_size  = SWAP32(ah->total_size);
	ah->data_start  = SWAP32(ah->data_start);
	ah->data_length = SWAP32(ah->data_length);
	ah->flags       = SWAP16(ah->flags);
	ah->num_attrs   = SWAP16(ah->num_attrs);

	  
                                                                      
                                                        
    
	end = ah->data_start + ah->data_length;
	if (ah->total_size > ainfop->finderinfo->offset + ainfop->finderinfo->length ||
	    ah->data_start < sizeof(attr_header_t) ||
	    end < ah->data_start ||
	    end > ah->total_size) {
		return EINVAL;
	}

	  
                                                                
    
	buf_end = ainfop->rawdata + ah->total_size;
	count = ah->num_attrs;
	ae = (attr_entry_t *)(&ah[1]);

	for (i = 0; i < count; i++) {
		                                                              
		if ((u_int8_t *) &ae[1] > buf_end) {
			return EINVAL;
		}

		                                                                        
		if (&ae->name[ae->namelen + 1] > buf_end) {
			return EINVAL;
		}

		                                                                                 
		if (strnlen((const char *)ae->name, ae->namelen) != ae->namelen - 1) {
			return EINVAL;
		}


		                                     
		ae->offset      = SWAP32(ae->offset);
		ae->length      = SWAP32(ae->length);
		ae->flags       = SWAP16(ae->flags);

		                                                                      
		end = ae->offset + ae->length;
		if (end < ae->offset || end > ah->total_size) {
			return EINVAL;
		}

		                                                           
		if (ae->offset < ah->data_start) {
			return EINVAL;
		}

		ae = ATTR_NEXT(ae);
	}

	return 0;
}

  
                                                
                                              
                                      
  
                                                
                      
  
static int
shift_data_down(vnode_t xvp, off_t start, size_t len, off_t delta, vfs_context_t context)
{
	int ret, iolen;
	size_t chunk, orig_chunk;
	char *buff;
	off_t pos;
	kauth_cred_t ucred = vfs_context_ucred(context);
	proc_t p = vfs_context_proc(context);

	if (delta == 0 || len == 0) {
		return 0;
	}

	chunk = 4096;
	if (len < chunk) {
		chunk = len;
	}
	orig_chunk = chunk;

	if (kmem_alloc(kernel_map, (vm_offset_t *)&buff, chunk, VM_KERN_MEMORY_FILE)) {
		return ENOMEM;
	}

	for (pos = start + len - chunk; pos >= start; pos -= chunk) {
		ret = vn_rdwr(UIO_READ, xvp, buff, (int)chunk, pos, UIO_SYSSPACE, IO_NODELOCKED | IO_NOAUTH, ucred, &iolen, p);
		if (iolen != 0) {
			printf("xattr:shift_data: error reading data @ %lld (read %d of %lu) (%d)\n",
			    pos, ret, chunk, ret);
			break;
		}

		ret = vn_rdwr(UIO_WRITE, xvp, buff, (int)chunk, pos + delta, UIO_SYSSPACE, IO_NODELOCKED | IO_NOAUTH, ucred, &iolen, p);
		if (iolen != 0) {
			printf("xattr:shift_data: error writing data @ %lld (wrote %d of %lu) (%d)\n",
			    pos + delta, ret, chunk, ret);
			break;
		}

		if ((pos - (off_t)chunk) < start) {
			chunk = pos - start;

			if (chunk == 0) {                    
				break;
			}
		}
	}
	kmem_free(kernel_map, (vm_offset_t)buff, orig_chunk);

	return 0;
}


static int
shift_data_up(vnode_t xvp, off_t start, size_t len, off_t delta, vfs_context_t context)
{
	int ret, iolen;
	size_t chunk, orig_chunk;
	char *buff;
	off_t pos;
	off_t end;
	kauth_cred_t ucred = vfs_context_ucred(context);
	proc_t p = vfs_context_proc(context);

	if (delta == 0 || len == 0) {
		return 0;
	}

	chunk = 4096;
	if (len < chunk) {
		chunk = len;
	}
	orig_chunk = chunk;
	end = start + len;

	if (kmem_alloc(kernel_map, (vm_offset_t *)&buff, chunk, VM_KERN_MEMORY_FILE)) {
		return ENOMEM;
	}

	for (pos = start; pos < end; pos += chunk) {
		ret = vn_rdwr(UIO_READ, xvp, buff, (int)chunk, pos, UIO_SYSSPACE, IO_NODELOCKED | IO_NOAUTH, ucred, &iolen, p);
		if (iolen != 0) {
			printf("xattr:shift_data: error reading data @ %lld (read %d of %lu) (%d)\n",
			    pos, ret, chunk, ret);
			break;
		}

		ret = vn_rdwr(UIO_WRITE, xvp, buff, (int)chunk, pos - delta, UIO_SYSSPACE, IO_NODELOCKED | IO_NOAUTH, ucred, &iolen, p);
		if (iolen != 0) {
			printf("xattr:shift_data: error writing data @ %lld (wrote %d of %lu) (%d)\n",
			    pos + delta, ret, chunk, ret);
			break;
		}

		if ((pos + (off_t)chunk) > end) {
			chunk = end - pos;

			if (chunk == 0) {                    
				break;
			}
		}
	}
	kmem_free(kernel_map, (vm_offset_t)buff, orig_chunk);

	return 0;
}

static int
lock_xattrfile(vnode_t xvp, short locktype, vfs_context_t context)
{
	struct flock lf;
	int error;

	lf.l_whence = SEEK_SET;
	lf.l_start = 0;
	lf.l_len = 0;
	lf.l_type = locktype;                         
	                                                         
	error = VNOP_ADVLOCK(xvp, (caddr_t)xvp, F_SETLK, &lf, F_FLOCK | F_WAIT, context, NULL);
	return error == ENOTSUP ? 0 : error;
}

int
unlock_xattrfile(vnode_t xvp, vfs_context_t context)
{
	struct flock lf;
	int error;

	lf.l_whence = SEEK_SET;
	lf.l_start = 0;
	lf.l_len = 0;
	lf.l_type = F_UNLCK;
	                                                         
	error = VNOP_ADVLOCK(xvp, (caddr_t)xvp, F_UNLCK, &lf, F_FLOCK, context, NULL);
	return error == ENOTSUP ? 0 : error;
}

#else                         


static int
default_getxattr(__unused vnode_t vp, __unused const char *name,
    __unused uio_t uio, __unused size_t *size, __unused int options,
    __unused vfs_context_t context)
{
	return ENOTSUP;
}

static int
default_setxattr(__unused vnode_t vp, __unused const char *name,
    __unused uio_t uio, __unused int options, __unused vfs_context_t context)
{
	return ENOTSUP;
}

static int
default_listxattr(__unused vnode_t vp,
    __unused uio_t uio, __unused size_t *size, __unused int options,
    __unused vfs_context_t context)
{
	return ENOTSUP;
}

static int
default_removexattr(__unused vnode_t vp, __unused const char *name,
    __unused int options, __unused vfs_context_t context)
{
	return ENOTSUP;
}

#endif                         
