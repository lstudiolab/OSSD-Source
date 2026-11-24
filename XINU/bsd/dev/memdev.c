/*
 * Copyright (c) 2004-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah Hdr: vn.c 1.13 94/04/02
 *
 *	from: @(#)vn.c	8.6 (Berkeley) 4/1/94
 * $FreeBSD: src/sys/dev/vn/vn.c,v 1.105.2.4 2001/11/18 07:11:00 dillon Exp $
 */

  
                   
  
                                
  
   

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/vm.h>
#include <sys/uio_internal.h>
#include <libkern/libkern.h>

#include <vm/pmap.h>
#include <vm/vm_pager.h>
#include <mach/memory_object_types.h>
#include <kern/debug.h>

#include <miscfs/devfs/devfs.h>


void            mdevinit(int the_cnt);

static open_close_fcn_t mdevopen;
static open_close_fcn_t mdevclose;
static psize_fcn_t              mdevsize;
static strategy_fcn_t   mdevstrategy;
static int                              mdevbioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p);
static int                              mdevcioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p);
static int                              mdevrw(dev_t dev, struct uio *uio, int ioflag);

#ifdef CONFIG_MEMDEV_INSECURE
static char *                   nonspace(char *pos, char *end);
static char *                   getspace(char *pos, char *end);
static char *                   cvtnum(char *pos, char *end, uint64_t *num);
#endif                             

extern void             bcopy_phys(addr64_t from, addr64_t to, vm_size_t bytes);
extern void             mapping_set_mod(ppnum_t pn);
extern ppnum_t  pmap_find_phys(pmap_t pmap, addr64_t va);

  
                                    
   
#define NB_MAX_MDEVICES (16)

  
         
                                      
                                 
   

static const struct bdevsw mdevbdevsw = {
	.d_open     = mdevopen,
	.d_close    = mdevclose,
	.d_strategy = mdevstrategy,
	.d_ioctl    = mdevbioctl,
	.d_dump     = eno_dump,
	.d_psize    = mdevsize,
	.d_type     = D_DISK,
};

static const struct cdevsw mdevcdevsw = {
	.d_open       = mdevopen,
	.d_close      = mdevclose,
	.d_read       = mdevrw,
	.d_write      = mdevrw,
	.d_ioctl      = mdevcioctl,
	.d_stop       = eno_stop,
	.d_reset      = eno_reset,
	.d_ttys       = NULL,
	.d_select     = eno_select,
	.d_mmap       = eno_mmap,
	.d_strategy   = eno_strat,
	.d_reserved_1 = eno_getc,
	.d_reserved_2 = eno_putc,
	.d_type       = D_DISK,
};

struct mdev {
	uint64_t        mdBase;                                 
	uint32_t        mdSize;                                 
	int                     mdFlags;                   
	int                     mdSecsize;                       
	int                     mdBDev;                                  
	int                     mdCDev;                                      
	void *          mdbdevb;
	void *          mdcdevb;
} mdev[NB_MAX_MDEVICES];

             
#define mdInited        0x01                             
#define mdRO            0x02                                  
#define mdPhys          0x04                                           

int mdevBMajor = -1;
int mdevCMajor = -1;

static int      mdevioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p, int is_char);
dev_t           mdevadd(int devid, uint64_t base, unsigned int size, int phys);
dev_t           mdevlookup(int devid);
void            mdevremoveall(void);
int             mdevgetrange(int devid, uint64_t *base, uint64_t *size);

static  int
mdevclose(__unused dev_t dev, __unused int flags,
    __unused int devtype, __unused struct proc *p)
{
	return 0;
}

static  int
mdevopen(dev_t dev, int flags, __unused int devtype, __unused struct proc *p)
{
	int devid;

	devid = minor(dev);                                                                                                  

	if (devid >= NB_MAX_MDEVICES) {
		return ENXIO;                                                                                
	}
	if ((flags & FWRITE) && (mdev[devid].mdFlags & mdRO)) {
		return EACCES;                                                                          
	}
	return 0;
}

static int
mdevrw(dev_t dev, struct uio *uio, __unused int ioflag)
{
	int                     status;
	addr64_t                mdata;
	int                     devid;
	enum uio_seg    saveflag;

	devid = minor(dev);                                                                                                  

	if (devid >= NB_MAX_MDEVICES) {
		return ENXIO;                                                                                
	}
	if (!(mdev[devid].mdFlags & mdInited)) {
		return ENXIO;                                                                         
	}
	mdata = ((addr64_t)mdev[devid].mdBase << 12) + uio->uio_offset;                                  

	saveflag = uio->uio_segflg;                                                                                       
#if LP64_DEBUG
	if (UIO_IS_USER_SPACE(uio) == 0 && UIO_IS_SYS_SPACE(uio) == 0) {
		panic("mdevrw - invalid uio_segflg\n");
	}
#endif                 
	                                                                  
	if (mdev[devid].mdFlags & mdPhys) {
		if (uio->uio_segflg == UIO_USERSPACE64) {
			uio->uio_segflg = UIO_PHYS_USERSPACE64;
		} else if (uio->uio_segflg == UIO_USERSPACE32) {
			uio->uio_segflg = UIO_PHYS_USERSPACE32;
		} else {
			uio->uio_segflg = UIO_PHYS_USERSPACE;
		}
	}
	status = uiomove64(mdata, (int)uio_resid(uio), uio);                       
	uio->uio_segflg = saveflag;                                                                           

	return status;
}

static void
mdevstrategy(struct buf *bp)
{
	unsigned int left, lop, csize;
	vm_offset_t vaddr, blkoff;
	int devid;
	addr64_t paddr, fvaddr;
	ppnum_t pp;

	devid = minor(buf_device(bp));                                                                               

	if ((mdev[devid].mdFlags & mdInited) == 0) {                                                    
		buf_seterror(bp, ENXIO);
		buf_biodone(bp);
		return;
	}

	buf_setresid(bp, buf_count(bp));                                                                    

	blkoff = buf_blkno(bp) * mdev[devid].mdSecsize;                                   

  
                                                                                   
                                      
   

	if (blkoff >= (mdev[devid].mdSize << 12)) {                                                                      
		if (blkoff != (mdev[devid].mdSize << 12)) {                                                    
			buf_seterror(bp, EINVAL);                                                                           
		}
		buf_biodone(bp);                                                                            
		return;
	}

	if ((blkoff + buf_count(bp)) > (mdev[devid].mdSize << 12)) {                                             
		buf_setcount(bp, (uint32_t)((mdev[devid].mdSize << 12) - blkoff));                        
	}
	  
                                       
              
    
	if (buf_map(bp, (caddr_t *)&vaddr)) {
		panic("ramstrategy: buf_map failed\n");
	}

	fvaddr = (mdev[devid].mdBase << 12) + blkoff;                                              

	if (buf_flags(bp) & B_READ) {                                                        
		if (!(mdev[devid].mdFlags & mdPhys)) {                                              
			bcopy((void *)((uintptr_t)fvaddr),
			    (void *)vaddr, (size_t)buf_count(bp));                                              
		} else {
			left = buf_count(bp);                                                                             
			while (left) {                                                                                          
				lop = min((4096 - (vaddr & 4095)), (4096 - (fvaddr & 4095)));                                                    
				csize = min(lop, left);                                                                      

				pp = pmap_find_phys(kernel_pmap, (addr64_t)((uintptr_t)vaddr));                                    
				if (!pp) {                                                                                           
					panic("mdevstrategy: sink address %016llX not mapped\n", (addr64_t)((uintptr_t)vaddr));
				}
				paddr = (addr64_t)(((addr64_t)pp << 12) | (addr64_t)(vaddr & 4095));                            
				bcopy_phys(fvaddr, paddr, csize);                                    
				mapping_set_mod((ppnum_t)(paddr >> 12));                                                   

				left = left - csize;                                                                
				vaddr = vaddr + csize;                                                                 
				fvaddr = fvaddr + csize;                                                                   
			}
		}
	} else {                                                                                                             
		if (!(mdev[devid].mdFlags & mdPhys)) {                                              
			bcopy((void *)vaddr, (void *)((uintptr_t)fvaddr),
			    (size_t)buf_count(bp));                                                     
		} else {
			left = buf_count(bp);                                                                             
			while (left) {                                                                                          
				lop = min((4096 - (vaddr & 4095)), (4096 - (fvaddr & 4095)));                                                    
				csize = min(lop, left);                                                                      

				pp = pmap_find_phys(kernel_pmap, (addr64_t)((uintptr_t)vaddr));                                      
				if (!pp) {                                                                                           
					panic("mdevstrategy: source address %016llX not mapped\n", (addr64_t)((uintptr_t)vaddr));
				}
				paddr = (addr64_t)(((addr64_t)pp << 12) | (addr64_t)(vaddr & 4095));                            

				bcopy_phys(paddr, fvaddr, csize);                                     

				left = left - csize;                                                                
				vaddr = vaddr + csize;                                                                 
				fvaddr = fvaddr + csize;                                                                   
			}
		}
	}
	  
                                         
                                        
                                       
                      
    
	buf_unmap(bp);

	buf_setresid(bp, 0);                                                                                            
	buf_biodone(bp);                                                                                                
}

static int
mdevbioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	return mdevioctl(dev, cmd, data, flag, p, 0);
}

static int
mdevcioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	return mdevioctl(dev, cmd, data, flag, p, 1);
}

static int
mdevioctl(dev_t dev, u_long cmd, caddr_t data, __unused int flag,
    struct proc *p, int is_char)
{
	int error;
	u_int32_t *f;
	u_int64_t *o;
	int devid;
	dk_memdev_info_t * memdev_info;

	devid = minor(dev);                                                                                                  

	if (devid >= NB_MAX_MDEVICES) {
		return ENXIO;                                                                                
	}
	error = proc_suser(p);                                        
	if (error) {
		return error;                                                                      
	}
	f = (u_int32_t*)data;
	o = (u_int64_t *)data;
	memdev_info = (dk_memdev_info_t *) data;

	switch (cmd) {
	case DKIOCGETMAXBLOCKCOUNTREAD:
		*o = 32;
		break;

	case DKIOCGETMAXBLOCKCOUNTWRITE:
		*o = 32;
		break;

	case DKIOCGETMAXSEGMENTCOUNTREAD:
		*o = 32;
		break;

	case DKIOCGETMAXSEGMENTCOUNTWRITE:
		*o = 32;
		break;

	case DKIOCGETBLOCKSIZE:
		*f = mdev[devid].mdSecsize;
		break;

	case DKIOCSETBLOCKSIZE:
		if (is_char) {
			return ENODEV;                                                                             
		}
		if (*f < DEV_BSIZE) {
			return EINVAL;                                                
		}
		mdev[devid].mdSecsize = *f;                                                                         
		break;

	case DKIOCISWRITABLE:
		*f = 1;
		break;

	case DKIOCGETBLOCKCOUNT:
		if (!(mdev[devid].mdFlags & mdInited)) {
			return ENXIO;
		}
		*o = ((mdev[devid].mdSize << 12) + mdev[devid].mdSecsize - 1) / mdev[devid].mdSecsize;
		break;

	  
                                                          
                                                                
                               
                             
                        
    
	case DKIOCGETMEMDEVINFO:
		if (!(mdev[devid].mdFlags & mdInited)) {
			return ENXIO;
		}
		memdev_info->mi_mdev = TRUE;
		memdev_info->mi_phys = (mdev[devid].mdFlags & mdPhys) ? TRUE : FALSE;
		memdev_info->mi_base = (uint32_t)mdev[devid].mdBase;
		memdev_info->mi_size = mdev[devid].mdSize;
		break;

	default:
		error = ENOTTY;
		break;
	}
	return error;
}


static  int
mdevsize(dev_t dev)
{
	int devid;

	devid = minor(dev);                                                                                                  
	if (devid >= NB_MAX_MDEVICES) {
		return ENXIO;                                                                                
	}
	if ((mdev[devid].mdFlags & mdInited) == 0) {
		return -1;                                                                
	}
	return mdev[devid].mdSecsize;
}

#include <pexpert/pexpert.h>

void
mdevinit(__unused int the_cnt)
{
#ifdef CONFIG_MEMDEV_INSECURE

	int devid, phys;
	uint64_t base;
	uint64_t size;
	char *ba, *lp;
	dev_t dev;


	ba = PE_boot_args();                                                                                        
	lp = ba + 256;                                                                                                

	while (1) {                                                                                                                                  
		phys = 0;                                                                                                                 
		ba = nonspace(ba, lp);                                                                      
		if (ba >= lp) {
			return;                                                                                        
		}
		if (((ba[0] != 'v') && (ba[0] != 'p'))
		    || (ba[1] != 'm') || (ba[2] != 'd') || (ba[4] != '=')
		    || (ba[3] < '0') || (ba[3] > 'f')
		    || ((ba[3] > '9') && (ba[3] < 'a'))) {                                                                            
			ba = getspace(ba, lp);                                                                            
			continue;                                                                                                           
		}

		if (ba[0] == 'p') {
			phys = 1;                                                                                     
		}
		devid = ba[3] & 0xF;                                                                      
		if (ba[3] > '9') {
			devid += 9;                                                                                
		}
		ba = &ba[5];                                                                                           
		ba = cvtnum(ba, lp, &base);                                                                              
		if (ba >= lp) {
			return;                                                                                              
		}
		if (ba[0] != '.') {
			continue;                                                                                                 
		}
		if (base & 0xFFF) {
			continue;                                                                                          
		}
		ba++;                                                                                                      
		ba = cvtnum(ba, lp, &size);                                                                    
		if (!size || (size & 0xFFF)) {
			continue;                                                                                   
		}
		if (ba < lp) {                                                                                                                  
			if ((ba[0] != ' ') && (ba[0] != 0)) {
				continue;                                                              
			}
		}

		dev = mdevadd(devid, base >> 12, (unsigned)size >> 12, phys);                          
	}

#endif                             

	return;
}

#ifdef CONFIG_MEMDEV_INSECURE

char *
nonspace(char *pos, char *end)                                                                             
{
	if (pos >= end) {
		return end;                                                                                    
	}
	if (pos[0] == 0) {
		return end;                                                                                       
	}
	while (1) {                                                                                                      
		if (pos[0] != ' ') {
			return pos;                                                                        
		}
		pos++;                                                                                            
		if (pos >= end) {
			return end;                                                                                 
		}
	}
}

char *
getspace(char *pos, char *end)                                                                             
{
	while (1) {                                                                                                      
		if (pos >= end) {
			return end;                                                                            
		}
		if (pos[0] == 0) {
			return end;                                                                               
		}
		if (pos[0] == ' ') {
			return pos;                                                                        
		}
		pos++;                                                                                            
	}
}

char *
cvtnum(char *pos, char *end, uint64_t *num)                                              
{
	int rad, dig;

	*num = 0;                                                                                                                     
	rad = 10;

	if (pos >= end) {
		return end;                                                                                    
	}
	if (pos[0] == 0) {
		return end;                                                                                       
	}
	if (pos[0] == '0' && ((pos[1] == 'x') || (pos[1] == 'x'))) {                          
		rad = 16;
		pos += 2;                                                                                                        
	}

	while (1) {                                                                                                      
		if (pos >= end) {
			return end;                                                                            
		}
		if (pos[0] == 0) {
			return end;                                                                               
		}
		if (pos[0] < '0') {
			return pos;                                                                     
		}
		dig = pos[0] & 0xF;                                                                                
		if (pos[0] > '9') {                                                                                        
			if (rad == 10) {
				return pos;                                                                       
			}
			if (!(((pos[0] >= 'A') && (pos[0] <= 'F'))
			    || ((pos[0] >= 'a') && (pos[0] <= 'f')))) {
				return pos;                                                              
			}
			dig = dig + 9;                                                                                    
		}
		*num = (*num * rad) + dig;                                                                         
		pos++;                                                                                               
	}
}

#endif                             

dev_t
mdevadd(int devid, uint64_t base, unsigned int size, int phys)
{
	int i;

	if (devid < 0) {
		devid = -1;
		for (i = 0; i < NB_MAX_MDEVICES; i++) {                                                                               
			if (!(mdev[i].mdFlags & mdInited)) {                                              
				if (devid < 0) {
					devid = i;                                                                   
				}
				continue;                                                                               
			}
			if (!(((base + size - 1) < mdev[i].mdBase) || ((mdev[i].mdBase + mdev[i].mdSize - 1) < base))) {                            
				panic("mdevadd: attempt to add overlapping memory device at %016llX-%016llX\n", mdev[i].mdBase, mdev[i].mdBase + mdev[i].mdSize - 1);
			}
		}
		if (devid < 0) {                                                                                             
			panic("mdevadd: attempt to add more than %d memory devices\n", NB_MAX_MDEVICES);
		}
	} else {
		if (devid >= NB_MAX_MDEVICES) {                                                                                          
			panic("mdevadd: attempt to explicitly add a bogus memory device: %08X\n", devid);
		}
		if (mdev[devid].mdFlags & mdInited) {                                        
			panic("mdevadd: attempt to explicitly add a previously defined memory device: %08X\n", devid);
		}
	}

	if (mdevBMajor < 0) {                                                                                                    
		mdevBMajor = bdevsw_add(-1, &mdevbdevsw);                                                                   
		if (mdevBMajor < 0) {
			printf("mdevadd: error - bdevsw_add() returned %d\n", mdevBMajor);
			return -1;
		}
	}

	if (mdevCMajor < 0) {                                                                                                    
		mdevCMajor = cdevsw_add_with_bdev(-1, &mdevcdevsw, mdevBMajor);                                                             
		if (mdevCMajor < 0) {
			printf("ramdevice_init: error - cdevsw_add() returned %d\n", mdevCMajor);
			return -1;
		}
	}

	mdev[devid].mdBDev = makedev(mdevBMajor, devid);                                   
	mdev[devid].mdbdevb = devfs_make_node(mdev[devid].mdBDev, DEVFS_BLOCK,                     
	    UID_ROOT, GID_OPERATOR,
	    0600, "md%d", devid);
	if (mdev[devid].mdbdevb == NULL) {                                                            
		printf("mdevadd: devfs_make_node for block failed!\n");
		return -1;                                                                                           
	}

	mdev[devid].mdCDev = makedev(mdevCMajor, devid);                                   
	mdev[devid].mdcdevb = devfs_make_node(mdev[devid].mdCDev, DEVFS_CHAR,                              
	    UID_ROOT, GID_OPERATOR,
	    0600, "rmd%d", devid);
	if (mdev[devid].mdcdevb == NULL) {                                                            
		printf("mdevadd: devfs_make_node for character failed!\n");
		return -1;                                                                                           
	}

	mdev[devid].mdBase = base;                                                                                            
	mdev[devid].mdSize = size;                                                                                          
	mdev[devid].mdSecsize = DEV_BSIZE;                                                                   
	if (phys) {
		mdev[devid].mdFlags |= mdPhys;                                                                   
	}
	mdev[devid].mdFlags |= mdInited;                                                                    
	printf("Added memory device md%x/rmd%x (%08X/%08X) at %016llX for %016llX\n",
	    devid, devid, mdev[devid].mdBDev, mdev[devid].mdCDev, base << 12, (uint64_t)size << 12);
	return mdev[devid].mdBDev;
}


dev_t
mdevlookup(int devid)
{
	if ((devid < 0) || (devid >= NB_MAX_MDEVICES)) {
		return -1;                                                                                             
	}
	if (!(mdev[devid].mdFlags & mdInited)) {
		return -1;                                                                        
	}
	return mdev[devid].mdBDev;                                                                                    
}

void
mdevremoveall(void)
{
	int i;

	for (i = 0; i < NB_MAX_MDEVICES; i++) {
		if (!(mdev[i].mdFlags & mdInited)) {
			continue;                                                        
		}
		devfs_remove(mdev[i].mdbdevb);                                               
		devfs_remove(mdev[i].mdcdevb);                                                   

		mdev[i].mdBase = 0;                                                           
		mdev[i].mdSize = 0;
		mdev[i].mdSecsize = 0;
		mdev[i].mdFlags = 0;
		mdev[i].mdBDev = 0;
		mdev[i].mdCDev = 0;
		mdev[i].mdbdevb = 0;
		mdev[i].mdcdevb = 0;
	}
}

int
mdevgetrange(int devid, uint64_t *base, uint64_t *size)
{
	assert(base);
	assert(size);

	                            
	if ((devid < 0) || (devid >= NB_MAX_MDEVICES)) {
		return -1;
	}

	                                           
	if ((mdev[devid].mdFlags & mdInited) == 0) {
		return -1;
	}

	*base = mdev[devid].mdBase << 12;
	*size = mdev[devid].mdSize << 12;

	                                                                   
	assert(*size < (UINT64_MAX - *base));

	return 0;
}
