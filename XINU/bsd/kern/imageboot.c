/*
 * Copyright (c) 2006-2020 Apple Computer, Inc. All rights reserved.
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
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/systm.h>
#include <sys/systm.h>
#include <sys/mount_internal.h>
#include <sys/fsctl.h>
#include <sys/filedesc.h>
#include <sys/vnode_internal.h>
#include <sys/imageboot.h>
#include <kern/assert.h>

#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/sysproto.h>
#include <sys/csr.h>
#include <miscfs/devfs/devfsdefs.h>
#include <libkern/crypto/sha2.h>
#include <libkern/crypto/rsa.h>
#include <libkern/OSKextLibPrivate.h>
#include <sys/ubc_internal.h>

#if CONFIG_IMAGEBOOT_IMG4
#include <libkern/img4/interface.h>
#include <img4/firmware.h>
#endif

#include <kern/kalloc.h>

#include <pexpert/pexpert.h>
#include <kern/chunklist.h>

extern struct filedesc filedesc0;

extern int (*mountroot)(void);
extern char rootdevice[DEVMAXNAMESIZE];

#if CONFIG_LOCKERBOOT
typedef struct _locker_mount_args {
	char lmnt_path[PATH_MAX];
	uint16_t lmnt_preferred_hash;
} locker_mount_args_t;
#endif

#define DEBUG_IMAGEBOOT 0

#if DEBUG_IMAGEBOOT
#define DBG_TRACE(...) printf("imageboot: " __VA_ARGS__)
#else
#define DBG_TRACE(...) do {} while(0)
#endif

#define AUTHDBG(fmt, args...) do { printf("%s: " fmt "\n", __func__, ##args); } while (0)
#define AUTHPRNT(fmt, args...) do { printf("%s: " fmt "\n", __func__, ##args); } while (0)
#define kheap_free_safe(h, x, l) do { if ((x)) { kheap_free(h, x, l); (x) = NULL; } } while (0)

extern int di_root_image_ext(const char *path, char *devname, size_t devsz, dev_t *dev_p, bool removable);
extern int di_root_image(const char *path, char *devname, size_t devsz, dev_t *dev_p);
extern int di_root_ramfile_buf(void *buf, size_t bufsz, char *devname, size_t devsz, dev_t *dev_p);

static boolean_t imageboot_setup_new(imageboot_type_t type);

void *ubc_getobject_from_filename(const char *filename, struct vnode **vpp, off_t *file_size);

extern lck_rw_t rootvnode_rw_lock;

#define kIBFilePrefix "file://"

__private_extern__ int
imageboot_format_is_valid(const char *root_path)
{
	return strncmp(root_path, kIBFilePrefix,
	           strlen(kIBFilePrefix)) == 0;
}

static void
vnode_get_and_drop_always(vnode_t vp)
{
	vnode_getalways(vp);
	vnode_rele(vp);
	vnode_put(vp);
}

__private_extern__ bool
imageboot_desired(void)
{
	bool do_imageboot = false;

	char *root_path = NULL;
	root_path = zalloc(ZV_NAMEI);
	  
                                      
   
                                                                      
                                                                           
   
                                                                        
                     
   
                                                                                          
                                                               
    
	if (!(PE_parse_boot_argn("rp0", root_path, MAXPATHLEN) ||
#if CONFIG_IMAGEBOOT_IMG4
	    PE_parse_boot_argn("arp0", root_path, MAXPATHLEN) ||
#endif
	    PE_parse_boot_argn("rp", root_path, MAXPATHLEN) ||
	    PE_parse_boot_argn(IMAGEBOOT_ROOT_ARG, root_path, MAXPATHLEN) ||
	    PE_parse_boot_argn(IMAGEBOOT_AUTHROOT_ARG, root_path, MAXPATHLEN))) {
		                             
		do_imageboot = false;
	} else {
		                                           
		if (imageboot_format_is_valid(root_path)) {
			DBG_TRACE("%s: Found %s\n", __FUNCTION__, root_path);
			                                                                         
			do_imageboot = true;
		} else {
			                             
			do_imageboot = false;
		}
	}

	zfree(ZV_NAMEI, root_path);
	return do_imageboot;
}

__private_extern__ imageboot_type_t
imageboot_needed(void)
{
	imageboot_type_t result = IMAGEBOOT_NONE;
	char *root_path = NULL;

	DBG_TRACE("%s: checking for presence of root path\n", __FUNCTION__);

	if (!imageboot_desired()) {
		goto out;
	}

	root_path = zalloc(ZV_NAMEI);
	result = IMAGEBOOT_DMG;

	                            
	if (!(PE_parse_boot_argn("rp1", root_path, MAXPATHLEN) ||
	    PE_parse_boot_argn(IMAGEBOOT_CONTAINER_ARG, root_path, MAXPATHLEN))) {
		goto out;
	}

	                               
	if (imageboot_format_is_valid(root_path)) {
		DBG_TRACE("%s: Found %s\n", __FUNCTION__, root_path);
	} else {
		panic("%s: Invalid URL scheme for %s\n",
		    __FUNCTION__, root_path);
	}

out:
	if (root_path != NULL) {
		zfree(ZV_NAMEI, root_path);
	}
	return result;
}

extern bool IOBaseSystemARVRootHashAvailable(void);


  
                                                                        
                                                    
                                    
                                                   
                                                                             
                                                                                           
                                                               
   
__private_extern__ int
imageboot_pivot_image(const char *image_path, imageboot_type_t type, const char *mount_path,
    const char *outgoing_root_path, const bool rooted_dmg, const bool skip_signature_check)
{
	int error;
	boolean_t authenticated_dmg_chunklist = false;
	vnode_t mount_vp = NULLVP;
	errno_t rootauth;


	if (type != IMAGEBOOT_DMG) {
		panic("not supported");
	}

	  
                                              
                                                                     
         
    
	vnode_t imagevp = NULLVP;
	error = vnode_lookup(image_path, 0, &imagevp, vfs_context_kernel());
	if (error) {
		printf("%s: image file not found or couldn't be read: %d\n", __FUNCTION__, error);
		  
                                                             
                                                                             
                                                                                        
     
		return error;
	}

	  
                                              
                                                                      
                                                              
                                           
    

	dev_t dev;
	char devname[DEVMAXNAMESIZE];

	error = di_root_image_ext(image_path, devname, DEVMAXNAMESIZE, &dev, true);
	if (error) {
		panic("%s: di_root_image failed: %d\n", __FUNCTION__, error);
	}

	printf("%s: attached disk image %s as %s\n", __FUNCTION__, image_path, devname);


#if CONFIG_IMAGEBOOT_CHUNKLIST
	if ((rooted_dmg == false) && !IOBaseSystemARVRootHashAvailable()) {
		error = authenticate_root_with_chunklist(image_path, NULL);
		if (error == 0) {
			printf("authenticated root-dmg via chunklist...\n");
			authenticated_dmg_chunklist = true;
		} else {
			                                                                
			printf("failed to chunklist-authenticate root-dmg @ %s\n", image_path);
		}
	}
#endif

	char fulldevname[DEVMAXNAMESIZE + 5];           
	strlcpy(fulldevname, "/dev/", sizeof(fulldevname));
	strlcat(fulldevname, devname, sizeof(fulldevname));

	  
                                                                     
                                         
                                                             
                                                    
    
	char *fulldevnamep = &(fulldevname[0]);
	char **fulldevnamepp = &fulldevnamep;

#define PIVOTMNT "/System/Volumes/BaseSystem"


	                                                            
	printf("%s: attempting to mount as hfs...\n", __FUNCTION__);
	error = kernel_mount("hfs", NULLVP, NULLVP, PIVOTMNT, fulldevnamepp, 0, (MNT_RDONLY | MNT_DONTBROWSE), (KERNEL_MOUNT_NOAUTH | KERNEL_MOUNT_BASESYSTEMROOT), vfs_context_kernel());
	if (error) {
		printf("mount failed: %d\n", error);
		printf("%s: attempting to mount as apfs...\n", __FUNCTION__);
		error = kernel_mount("apfs", NULLVP, NULLVP, PIVOTMNT, fulldevnamepp, 0, (MNT_RDONLY | MNT_DONTBROWSE), (KERNEL_MOUNT_NOAUTH | KERNEL_MOUNT_BASESYSTEMROOT), vfs_context_kernel());
	}

	                                                             
	if (error) {
		  
                                                                                  
                                                                                       
                                                                                     
                                                                                        
                                                                                     
                                                                             
                        
     
#if defined(__arm64__) && XNU_TARGET_OS_OSX
		panic("%s: failed to mount pivot image(%d)!", __FUNCTION__, error);
#endif
		printf("%s: failed to mount pivot image(%d) !", __FUNCTION__, error);
		goto done;
	}

	                                                                                                                 
	error = vnode_lookup(PIVOTMNT, 0, &mount_vp, vfs_context_kernel());
	if (error) {
#if defined(__arm64__) && XNU_TARGET_OS_OSX
		panic("%s: failed to lookup pivot root (%d) !", __FUNCTION__, error);
#endif
		printf("%s: failed to lookup pivot root (%d)!", __FUNCTION__, error);
		goto done;
	}

	                                 
	rootauth = VNOP_IOCTL(mount_vp, FSIOC_KERNEL_ROOTAUTH, (caddr_t)0x1, 0, vfs_context_kernel());
	if (rootauth) {
		printf("BS-DMG failed to authenticate intra-FS \n");
		  
                                                                             
                                                          
                                                                                   
     
		if (rooted_dmg || authenticated_dmg_chunklist || skip_signature_check) {
			rootauth = 0;
		}
		error = rootauth;
	}
	vnode_put(mount_vp);
	mount_vp = NULLVP;

	if (error) {
		  
                                                                          
                                                                               
                                                                            
                     
     
#if defined(__arm64__) && XNU_TARGET_OS_OSX
		panic("%s: could not authenticate the pivot image: %d. giving up.\n", __FUNCTION__, error);
#endif
		printf("%s: could not authenticate the pivot image: %d. giving up.\n", __FUNCTION__, error);
		goto done;
	}

	if (rootvnode) {
		mount_t root_mp = vnode_mount(rootvnode);
		if (root_mp && (root_mp->mnt_kern_flag & MNTK_SSD)) {
			rootvp_is_ssd = true;
		}
	}
	  
                                               
    
	error = vfs_switch_root(mount_path, outgoing_root_path, 0);
	if (error) {
		panic("%s: vfs_switch_root failed: %d\n", __FUNCTION__, error);
	}

	  
                                                                   
                         
   
                                                                  
                   
                                                                
                                                                    
    
	mount_t imagemp = imagevp->v_mount;
	lck_rw_lock_exclusive(&imagemp->mnt_rwlock);
	imagemp->mnt_kern_flag |= MNTK_BACKS_ROOT;
	lck_rw_done(&imagemp->mnt_rwlock);

	error = 0;

	  
                                                   
                                                                      
                                                                 
                                                                    
                                                              
                                     
    

done:
	if (imagevp != NULLVP) {
		vnode_put(imagevp);
	}
	return error;
}

                   
extern uuid_string_t fake_bootuuid;

static void
set_fake_bootuuid(mount_t mp)
{
	struct vfs_attr va;
	VFSATTR_INIT(&va);
	VFSATTR_WANTED(&va, f_uuid);

	if (vfs_getattr(mp, &va, vfs_context_current()) != 0) {
		return;
	}

	if (!VFSATTR_IS_SUPPORTED(&va, f_uuid)) {
		return;
	}

	uuid_unparse(va.f_uuid, fake_bootuuid);
}

  
                                                    
                                                         
                                                           
                                                              
                                          
                                                                         
                                                                         
   
__private_extern__ int
imageboot_mount_image(const char *root_path, int height, imageboot_type_t type)
{
	dev_t           dev;
	int             error;
	  
                                                                             
                                                                             
                                                 
    
	vnode_t         old_rootvnode = rootvnode;
	vnode_t         newdp;
	mount_t         new_rootfs;
	boolean_t update_rootvnode = FALSE;

	if (type == IMAGEBOOT_DMG) {
		error = di_root_image(root_path, rootdevice, DEVMAXNAMESIZE, &dev);
		if (error) {
			panic("%s: di_root_image failed: %d\n", __FUNCTION__, error);
		}

		rootdev = dev;
		mountroot = NULL;
		printf("%s: root device 0x%x\n", __FUNCTION__, rootdev);
		error = vfs_mountroot();
		if (error != 0) {
			panic("vfs_mountroot() failed.\n");
		}

		update_rootvnode = TRUE;
	}
#if CONFIG_LOCKERBOOT
	else if (type == IMAGEBOOT_LOCKER) {
		locker_mount_args_t *mntargs = kheap_alloc(KHEAP_TEMP,
		    sizeof(*mntargs), Z_WAITOK);
		if (!mntargs) {
			panic("could not alloc mount args");
		}

		strlcpy(mntargs->lmnt_path, root_path, sizeof(mntargs->lmnt_path));
		mntargs->lmnt_preferred_hash = 0;

		DBG_TRACE("%s: mounting locker: %s\n", __FUNCTION__, root_path);
		error = kernel_mount(LOCKERFS_NAME, NULLVP, NULLVP, "/",
		    mntargs, sizeof(*mntargs), 0, 0, vfs_context_kernel());
		if (error) {
			panic("failed to mount locker: %d", error);
		}
		kheap_free(KHEAP_TEMP, mntargs, sizeof(*mntargs));

		                                      
		old_rootvnode->v_mountedhere = NULL;
		rootvnode->v_mount->mnt_vnodecovered = NULL;
	}
#endif
	else {
		panic("invalid imageboot type: %d", type);
	}

	  
                          
                                           
    
	if (VFS_ROOT(TAILQ_LAST(&mountlist, mntlist), &newdp, vfs_context_kernel())) {
		panic("%s: cannot find root vnode", __FUNCTION__);
	}
	DBG_TRACE("%s: old root fsname: %s\n", __FUNCTION__, old_rootvnode->v_mount->mnt_vtable->vfc_name);

	if (old_rootvnode != NULL) {
		                                                              
		mount_t old_rootfs = old_rootvnode->v_mount;

		mount_list_remove(old_rootfs);
		mount_lock(old_rootfs);
		old_rootfs->mnt_kern_flag |= MNTK_BACKS_ROOT;
		old_rootfs->mnt_flag &= ~MNT_ROOTFS;
		mount_unlock(old_rootfs);
	}

	vnode_ref(newdp);
	vnode_put(newdp);

	lck_rw_lock_exclusive(&rootvnode_rw_lock);
	                                 
	if (update_rootvnode) {
		rootvnode = newdp;
		set_fake_bootuuid(rootvnode->v_mount);
	}

	new_rootfs = rootvnode->v_mount;
	mount_lock(new_rootfs);
	new_rootfs->mnt_flag |= MNT_ROOTFS;
	mount_unlock(new_rootfs);

	filedesc0.fd_cdir = newdp;
	lck_rw_unlock_exclusive(&rootvnode_rw_lock);

	DBG_TRACE("%s: root switched\n", __FUNCTION__);

	if (old_rootvnode != NULL) {
#ifdef CONFIG_IMGSRC_ACCESS
		if (height >= 0 && PE_imgsrc_mount_supported()) {
			imgsrc_rootvnodes[height] = old_rootvnode;
		} else {
			vnode_get_and_drop_always(old_rootvnode);
		}
#else
#pragma unused(height)
		vnode_get_and_drop_always(old_rootvnode);
#endif                           
	}
	return 0;
}

  
                                              
                                                          
   
void *
ubc_getobject_from_filename(const char *filename, struct vnode **vpp, off_t *file_size)
{
	int err = 0;
	struct nameidata ndp = {};
	struct vnode *vp = NULL;
	off_t fsize = 0;
	vfs_context_t ctx = vfs_context_kernel();
	void *control = NULL;

	NDINIT(&ndp, LOOKUP, OP_OPEN, LOCKLEAF, UIO_SYSSPACE, CAST_USER_ADDR_T(filename), ctx);
	if ((err = namei(&ndp)) != 0) {
		goto errorout;
	}
	nameidone(&ndp);
	vp = ndp.ni_vp;

	if ((err = vnode_size(vp, &fsize, ctx)) != 0) {
		goto errorout;
	}

	if (fsize < 0) {
		goto errorout;
	}

	control = ubc_getobject(vp, UBC_FLAGS_NONE);
	if (control == NULL) {
		goto errorout;
	}

	*file_size = fsize;
	*vpp = vp;
	vp = NULL;

errorout:
	if (vp) {
		vnode_put(vp);
	}
	return control;
}

int
imageboot_read_file_from_offset(kalloc_heap_t kheap, const char *path, off_t offset, void **bufp, size_t *bufszp)
{
	int err = 0;
	struct nameidata ndp = {};
	struct vnode *vp = NULL;
	off_t fsize = 0;
	int resid = 0;
	char *buf = NULL;
	bool doclose = false;

	vfs_context_t ctx = vfs_context_kernel();
	proc_t p = vfs_context_proc(ctx);
	kauth_cred_t kerncred = vfs_context_ucred(ctx);

	NDINIT(&ndp, LOOKUP, OP_OPEN, LOCKLEAF, UIO_SYSSPACE, CAST_USER_ADDR_T(path), ctx);
	if ((err = namei(&ndp)) != 0) {
		AUTHPRNT("namei failed (%s) - %d", path, err);
		goto out;
	}
	nameidone(&ndp);
	vp = ndp.ni_vp;

	if ((err = vnode_size(vp, &fsize, ctx)) != 0) {
		AUTHPRNT("failed to get vnode size of %s - %d", path, err);
		goto out;
	}
	if (fsize < 0) {
		panic("negative file size");
	}

	if ((err = VNOP_OPEN(vp, FREAD, ctx)) != 0) {
		AUTHPRNT("failed to open %s - %d", path, err);
		goto out;
	}
	doclose = true;

	                                                       
	if (*bufszp && *bufszp < (size_t)fsize) {
		fsize = *bufszp;
	}

	fsize = (off_t)MIN((size_t)fsize, INT_MAX);

	buf = kheap_alloc(kheap, (size_t)fsize, Z_WAITOK);
	if (buf == NULL) {
		err = ENOMEM;
		goto out;
	}

	if ((err = vn_rdwr(UIO_READ, vp, (caddr_t)buf, (int)fsize, offset, UIO_SYSSPACE, IO_NODELOCKED, kerncred, &resid, p)) != 0) {
		AUTHPRNT("Cannot read %d bytes at offset %d from %s - %d", (int)fsize, (int)offset, path, err);
		goto out;
	}

	if (resid) {
		                                    
		AUTHPRNT("Short read of %d bytes at offset %d from %s - %d", (int)fsize, (int)offset, path, resid);
		err = EINVAL;
		goto out;
	}

out:
	if (doclose) {
		VNOP_CLOSE(vp, FREAD, ctx);
	}
	if (vp) {
		vnode_put(vp);
		vp = NULL;
	}

	if (err) {
		kheap_free_safe(kheap, buf, (size_t)fsize);
	} else {
		*bufp = buf;
		*bufszp = (size_t)fsize;
	}

	return err;
}

int
imageboot_read_file(kalloc_heap_t kheap, const char *path, void **bufp, size_t *bufszp)
{
	return imageboot_read_file_from_offset(kheap, path, 0, bufp, bufszp);
}

#if CONFIG_IMAGEBOOT_IMG4 || CONFIG_IMAGEBOOT_CHUNKLIST
vnode_t
imgboot_get_image_file(const char *path, off_t *fsize, int *errp)
{
	struct nameidata ndp = {};
	vnode_t vp = NULL;
	vfs_context_t ctx = vfs_context_kernel();
	int err;

	NDINIT(&ndp, LOOKUP, OP_OPEN, LOCKLEAF, UIO_SYSSPACE, CAST_USER_ADDR_T(path), ctx);
	if ((err = namei(&ndp)) != 0) {
		AUTHPRNT("Cannot find %s - error %d", path, err);
	} else {
		nameidone(&ndp);
		vp = ndp.ni_vp;

		if (vp->v_type != VREG) {
			err = EINVAL;
			AUTHPRNT("%s it not a regular file", path);
		} else if (fsize) {
			if ((err = vnode_size(vp, fsize, ctx)) != 0) {
				AUTHPRNT("Cannot get file size of %s - error %d", path, err);
			}
		}
	}

	if (err) {
		if (vp) {
			vnode_put(vp);
		}
		*errp = err;
		vp = NULL;
	}
	return vp;
}
#endif                                                               

#if CONFIG_IMAGEBOOT_IMG4

#define APTICKET_NAME "apticket.der"

static char *
imgboot_get_apticket_path(const char *rootpath, size_t *sz)
{
	size_t plen = strlen(rootpath) + sizeof(APTICKET_NAME) + 1;
	char *path = kheap_alloc(KHEAP_TEMP, plen, Z_WAITOK);

	if (path) {
		char *slash;

		strlcpy(path, rootpath, plen);
		slash = strrchr(path, '/');
		if (slash == NULL) {
			slash = path;
		} else {
			slash++;
		}
		strlcpy(slash, APTICKET_NAME, sizeof(APTICKET_NAME) + 1);
	}

	*sz = plen;
	return path;
}

static int
authenticate_root_with_img4(const char *rootpath)
{
	errno_t rv;
	vnode_t vp;
	size_t ticket_pathsz = 0;
	char *ticket_path;
	img4_buff_t tck = IMG4_BUFF_INIT;
	img4_firmware_execution_context_t exec = {
		.i4fex_version = IMG4_FIRMWARE_EXECUTION_CONTEXT_STRUCT_VERSION,
		.i4fex_execute = NULL,
		.i4fex_context = NULL,
	};
	img4_firmware_t fw = NULL;
	img4_firmware_flags_t fw_flags = IMG4_FIRMWARE_FLAG_BARE |
	    IMG4_FIRMWARE_FLAG_SUBSEQUENT_STAGE;

	DBG_TRACE("Check %s\n", rootpath);

	if (img4if == NULL) {
		AUTHPRNT("AppleImage4 is not ready");
		return EAGAIN;
	}

	ticket_path = imgboot_get_apticket_path(rootpath, &ticket_pathsz);
	if (ticket_path == NULL) {
		AUTHPRNT("Cannot construct ticket path - out of memory");
		return ENOMEM;
	}

	rv = imageboot_read_file(KHEAP_TEMP, ticket_path, (void **)&tck.i4b_bytes, &tck.i4b_len);
	if (rv) {
		AUTHPRNT("Cannot get a ticket from %s - %d\n", ticket_path, rv);
		goto out_with_ticket_path;
	}

	DBG_TRACE("Got %lu bytes of manifest from %s\n", tck.i4b_len, ticket_path);

	vp = imgboot_get_image_file(rootpath, NULL, &rv);
	if (vp == NULL) {
		                                            
		rv = EIO;
		goto out_with_ticket_bytes;
	}

	fw = img4_firmware_new_from_vnode_4xnu(IMG4_RUNTIME_DEFAULT, &exec, 'rosi',
	    vp, fw_flags);
	if (!fw) {
		AUTHPRNT("Could not allocate new firmware");
		rv = ENOMEM;
		goto out_with_ticket_bytes;
	}

	img4_firmware_attach_manifest(fw, &tck);
	rv = img4_firmware_evaluate(fw, img4_chip_select_personalized_ap(), NULL);

out_with_ticket_bytes:
	kheap_free_safe(KHEAP_TEMP, tck.i4b_bytes, tck.i4b_len);
out_with_ticket_path:
	kheap_free_safe(KHEAP_TEMP, ticket_path, ticket_pathsz);

	img4_firmware_destroy(&fw);
	return rv;
}
#endif                            


  
                                                                          
                                          
   
static int
imageboot_mount_ramdisk(const char *path)
{
	int err = 0;
	size_t bufsz = 0;
	void *buf = NULL;
	dev_t dev;
	vnode_t newdp;
	vnode_t tvp;
	mount_t new_rootfs;

	                                     
	err = imageboot_read_file(KHEAP_DATA_BUFFERS, path, &buf, &bufsz);
	if (err) {
		printf("%s: failed: imageboot_read_file() = %d\n", __func__, err);
		goto out;
	}
	DBG_TRACE("%s: read '%s' sz = %lu\n", __func__, path, bufsz);

#if CONFIG_IMGSRC_ACCESS
	                                                                      
	mount_list_remove(rootvnode->v_mount);
	for (int i = 0; i < MAX_IMAGEBOOT_NESTING; i++) {
		struct vnode *vn = imgsrc_rootvnodes[i];
		if (vn) {
			vnode_getalways(vn);
			imgsrc_rootvnodes[i] = NULLVP;

			mount_t mnt = vn->v_mount;
			mount_lock(mnt);
			mnt->mnt_flag |= MNT_ROOTFS;
			mount_list_add(mnt);
			mount_unlock(mnt);

			vnode_rele(vn);
			vnode_put(vn);
		}
	}
	mount_list_add(rootvnode->v_mount);
#endif

	                                
	vfs_unmountall(FALSE);

	lck_rw_lock_exclusive(&rootvnode_rw_lock);
	filedesc0.fd_cdir = NULL;
	tvp = rootvnode;
	rootvnode = NULL;
	rootvp = NULLVP;
	rootdev = NODEV;
	lck_rw_unlock_exclusive(&rootvnode_rw_lock);
	vnode_get_and_drop_always(tvp);

	                                
	err = di_root_ramfile_buf(buf, bufsz, rootdevice, DEVMAXNAMESIZE, &dev);
	if (err) {
		printf("%s: failed: di_root_ramfile_buf() = %d\n", __func__, err);
		goto out;
	}

	                      
	rootdev = dev;
	mountroot = NULL;
	err = vfs_mountroot();
	if (err) {
		printf("%s: failed: vfs_mountroot() = %d\n", __func__, err);
		goto out;
	}

	                              
	if (VFS_ROOT(TAILQ_LAST(&mountlist, mntlist), &newdp, vfs_context_kernel())) {
		panic("%s: cannot find root vnode", __func__);
	}
	vnode_ref(newdp);

	lck_rw_lock_exclusive(&rootvnode_rw_lock);
	rootvnode = newdp;
	rootvnode->v_flag |= VROOT;
	new_rootfs = rootvnode->v_mount;
	mount_lock(new_rootfs);
	new_rootfs->mnt_flag |= MNT_ROOTFS;
	mount_unlock(new_rootfs);

	set_fake_bootuuid(new_rootfs);

	filedesc0.fd_cdir = newdp;
	lck_rw_unlock_exclusive(&rootvnode_rw_lock);

	vnode_put(newdp);

	DBG_TRACE("%s: root switched\n", __func__);

out:
	if (err) {
		kheap_free_safe(KHEAP_DATA_BUFFERS, buf, bufsz);
	}
	return err;
}

  
                                                                                
                                     
  
                                                             
   
static char *
url_to_path(char *url_path, size_t *sz)
{
	char *path = url_path;
	size_t len = strlen(kIBFilePrefix);

	if (strncmp(kIBFilePrefix, url_path, len) == 0) {
		                                                              
		url_path += len;

		len = strlen(url_path);
		if (len) {
			                                           
			path = kheap_alloc(KHEAP_TEMP, len + 1, Z_WAITOK);
			if (path == NULL) {
				panic("imageboot path allocation failed - cannot allocate %d bytes\n", (int)len);
			}

			strlcpy(path, url_path, len + 1);
			*sz = len + 1;
			url_decode(path);
		} else {
			panic("Bogus imageboot path URL - missing path\n");
		}

		DBG_TRACE("%s: root image URL <%s> becomes %s\n", __func__, url_path, path);
	}

	return path;
}

static boolean_t
imageboot_setup_new(imageboot_type_t type)
{
	int error;
	char *root_path = NULL;
	int height = 0;
	boolean_t done = FALSE;
	boolean_t auth_root = TRUE;
	boolean_t ramdisk_root = FALSE;

	root_path = zalloc(ZV_NAMEI);
	assert(root_path != NULL);

#if CONFIG_LOCKERBOOT
	if (type == IMAGEBOOT_LOCKER) {
		if (!PE_parse_boot_argn(IMAGEBOOT_LOCKER_ARG, root_path, MAXPATHLEN)) {
			panic("locker boot with no locker given");
		}

		DBG_TRACE("%s: root fsname: %s\n", __FUNCTION__, rootvnode->v_mount->mnt_vtable->vfc_name);

		  
                                                                      
                             
     
		error = imageboot_mount_image(root_path, 0, type);
		if (error) {
			panic("failed to mount system locker: %d", error);
		}

		done = TRUE;
		goto out;
	}
#endif                        

	unsigned imgboot_arg;
	if (PE_parse_boot_argn("-rootdmg-ramdisk", &imgboot_arg, sizeof(imgboot_arg))) {
		ramdisk_root = TRUE;
	}

	if (PE_parse_boot_argn(IMAGEBOOT_CONTAINER_ARG, root_path, MAXPATHLEN) == TRUE) {
		printf("%s: container image url is %s\n", __FUNCTION__, root_path);
		error = imageboot_mount_image(root_path, height, type);
		if (error != 0) {
			panic("Failed to mount container image.");
		}

		height++;
	}

	if (PE_parse_boot_argn(IMAGEBOOT_AUTHROOT_ARG, root_path, MAXPATHLEN) == FALSE &&
	    PE_parse_boot_argn(IMAGEBOOT_ROOT_ARG, root_path, MAXPATHLEN) == FALSE) {
		if (height > 0) {
			panic("%s specified without %s or %s?\n", IMAGEBOOT_CONTAINER_ARG, IMAGEBOOT_AUTHROOT_ARG, IMAGEBOOT_ROOT_ARG);
		}
		goto out;
	}

	printf("%s: root image URL is '%s'\n", __func__, root_path);

	                                           
	size_t pathsz;
	char *path = url_to_path(root_path, &pathsz);
	assert(path);

#if CONFIG_IMAGEBOOT_CHUNKLIST
	if (auth_root) {
		  
                                                            
                                                           
                                                        
     
		AUTHDBG("authenticating root image at %s", path);
		error = authenticate_root_with_chunklist(path, &auth_root);
		if (error) {
			panic("root image authentication failed (err = %d)\n", error);
		}
		AUTHDBG("successfully authenticated %s", path);
	}
#endif

	if (ramdisk_root) {
		error = imageboot_mount_ramdisk(path);
	} else {
		error = imageboot_mount_image(root_path, height, type);
	}

	if (path != root_path) {
		kheap_free_safe(KHEAP_TEMP, path, pathsz);
	}

	if (error) {
		panic("Failed to mount root image (err=%d, auth=%d, ramdisk=%d)\n",
		    error, auth_root, ramdisk_root);
	}

#if CONFIG_IMAGEBOOT_CHUNKLIST
	if (auth_root) {
		                                                             
		AUTHDBG("checking root image version");
		error = authenticate_root_version_check();
		if (error) {
			panic("root image version check failed");
		} else {
			AUTHDBG("root image version matches kernel");
		}
	}
#endif

	done = TRUE;

out:
	zfree(ZV_NAMEI, root_path);
	return done;
}

__private_extern__ void
imageboot_setup(imageboot_type_t type)
{
	int         error = 0;
	char *root_path = NULL;

	DBG_TRACE("%s: entry\n", __FUNCTION__);

	if (rootvnode == NULL) {
		panic("imageboot_setup: rootvnode is NULL.");
	}

	  
                        
                                                                                       
                                          
                                                                    
                                                                     
                                                
    
	if (imageboot_setup_new(type)) {
		return;
	}

	root_path = zalloc(ZV_NAMEI);
	assert(root_path != NULL);

	  
                                                                              
                                                                                      
                                                                                   
                                                                                    
                                                        
    
#if CONFIG_IMAGEBOOT_IMG4
	if (PE_parse_boot_argn("arp0", root_path, MAXPATHLEN)) {
		size_t pathsz;
		char *path = url_to_path(root_path, &pathsz);

		assert(path);

		if (authenticate_root_with_img4(path)) {
			panic("Root image %s does not match the manifest\n", root_path);
		}
		if (path != root_path) {
			kheap_free_safe(KHEAP_TEMP, path, pathsz);
		}
	} else
#endif                            
	if ((PE_parse_boot_argn("rp", root_path, MAXPATHLEN) == FALSE) &&
	    (PE_parse_boot_argn("rp0", root_path, MAXPATHLEN) == FALSE)) {
		panic("%s: no valid path to image.\n", __FUNCTION__);
	}

	DBG_TRACE("%s: root image url is %s\n", __FUNCTION__, root_path);

	error = imageboot_mount_image(root_path, 0, type);
	if (error) {
		panic("Failed on first stage of imageboot.");
	}

	  
                                             
    
	if (PE_parse_boot_argn("rp1", root_path, MAXPATHLEN) == FALSE) {
		goto done;
	}

	printf("%s: second level root image url is %s\n", __FUNCTION__, root_path);

	  
                                                               
                                  
    
	error = imageboot_mount_image(root_path, 1, type);
	if (error) {
		panic("Failed on second stage of imageboot.");
	}

done:
	zfree(ZV_NAMEI, root_path);

	DBG_TRACE("%s: exit\n", __FUNCTION__);

	return;
}
