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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1992,7 NeXT Computer, Inc.
 *
 * Unix data structure initialization.
 *
 */

#include <mach/mach_types.h>

#include <kern/startup.h>
#include <vm/vm_kern.h>
#include <mach/vm_prot.h>

#include <sys/param.h>
#include <sys/buf_internal.h>
#include <sys/file_internal.h>
#include <sys/proc_internal.h>
#include <sys/mcache.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/sysctl.h>
#include <machine/cons.h>
#include <pexpert/pexpert.h>
#include <sys/socketvar.h>
#include <pexpert/pexpert.h>
#include <netinet/tcp_var.h>

extern uint32_t kern_maxvnodes;
extern vm_map_t mb_map;

#if INET
extern uint32_t   tcp_sendspace;
extern uint32_t   tcp_recvspace;
#endif

void            bsd_bufferinit(void);

unsigned int    bsd_mbuf_cluster_reserve(boolean_t *);
void bsd_scale_setup(int);
void bsd_exec_setup(int);

  
                                                          
   

#ifdef  NBUF
int             max_nbuf_headers = NBUF;
int             niobuf_headers = (NBUF / 2) + 2048;
int             nbuf_hashelements = NBUF;
int             nbuf_headers = NBUF;
#else
int             max_nbuf_headers = 0;
int             niobuf_headers = 0;
int             nbuf_hashelements = 0;
int             nbuf_headers = 0;
#endif

SYSCTL_INT(_kern, OID_AUTO, nbuf, CTLFLAG_RD | CTLFLAG_LOCKED, &nbuf_headers, 0, "");
SYSCTL_INT(_kern, OID_AUTO, maxnbuf, CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_KERN, &max_nbuf_headers, 0, "");

__private_extern__ int customnbuf = 0;
int             ncl = 0;

                                      
TUNABLE(int, serverperfmode, "serverperfmode", 0);

#if SOCKETS
static unsigned int mbuf_poolsz;
#endif

vm_map_t        buffer_map;
vm_map_t        bufferhdr_map;
static int vnodes_sized = 0;

extern void     bsd_startupearly(void);

void
bsd_startupearly(void)
{
	vm_offset_t     firstaddr;
	vm_size_t       size;
	kern_return_t   ret;

	                                             
	if (max_nbuf_headers == 0) {
		max_nbuf_headers = (int)atop_kernel(sane_size / 50);                                                 
	}
	if ((customnbuf == 0) && ((unsigned int)max_nbuf_headers > 16384)) {
		max_nbuf_headers = 16384;
	}
	if (max_nbuf_headers < CONFIG_MIN_NBUF) {
		max_nbuf_headers = CONFIG_MIN_NBUF;
	}

	                                                 
	if ((customnbuf == 0) && nbuf_hashelements == 0) {
		nbuf_hashelements = (int)atop_kernel(sane_size / 50);
		if ((unsigned int)nbuf_hashelements > 200000) {
			nbuf_hashelements = 200000;
		}
	} else {
		nbuf_hashelements = max_nbuf_headers;
	}

	if (niobuf_headers == 0) {
		if (max_nbuf_headers < 4096) {
			niobuf_headers = max_nbuf_headers;
		} else {
			niobuf_headers = (max_nbuf_headers / 2) + 2048;
		}
	}
	if (niobuf_headers < CONFIG_MIN_NIOBUF) {
		niobuf_headers = CONFIG_MIN_NIOBUF;
	}

	size = (max_nbuf_headers + niobuf_headers) * sizeof(struct buf);
	size = round_page(size);

	ret = kmem_suballoc(kernel_map,
	    &firstaddr,
	    size,
	    FALSE,
	    VM_FLAGS_ANYWHERE,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_FILE,
	    &bufferhdr_map);

	if (ret != KERN_SUCCESS) {
		panic("Failed to create bufferhdr_map");
	}

	ret = kernel_memory_allocate(bufferhdr_map,
	    &firstaddr,
	    size,
	    0,
	    KMA_HERE | KMA_KOBJECT,
	    VM_KERN_MEMORY_FILE);

	if (ret != KERN_SUCCESS) {
		panic("Failed to allocate bufferhdr_map");
	}

	buf_headers = (struct buf *) firstaddr;
	bzero(buf_headers, size);

#if SOCKETS
	{
		static const unsigned int       maxspace = 128 * 1024;
		int             scale;

		nmbclusters = bsd_mbuf_cluster_reserve(NULL) / MCLBYTES;

#if INET
		if ((scale = nmbclusters / NMBCLUSTERS) > 1) {
			tcp_sendspace *= scale;
			tcp_recvspace *= scale;

			if (tcp_sendspace > maxspace) {
				tcp_sendspace = maxspace;
			}
			if (tcp_recvspace > maxspace) {
				tcp_recvspace = maxspace;
			}
		}
#endif           
	}
#endif              

	if (vnodes_sized == 0) {
		if (!PE_get_default("kern.maxvnodes", &desiredvnodes, sizeof(desiredvnodes))) {
			  
                                 
                                            
                                                              
                                          
                                                          
                                                                              
                                   
      
			desiredvnodes  = (int)(sane_size / 65536) + 1024;
#ifdef CONFIG_VNODES
			if (desiredvnodes > CONFIG_VNODES) {
				desiredvnodes = CONFIG_VNODES;
			}
#endif
		}
		vnodes_sized = 1;
	}
}

void
bsd_bufferinit(void)
{
#if SOCKETS
	kern_return_t   ret;
#endif
	  
                                                                    
                                      
    

	bsd_startupearly();

#if SOCKETS
	ret = kmem_suballoc(kernel_map,
	    (vm_offset_t *) &mbutl,
	    (vm_size_t) (nmbclusters * MCLBYTES),
	    FALSE,
	    VM_FLAGS_ANYWHERE,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_MBUF,
	    &mb_map);

	if (ret != KERN_SUCCESS) {
		panic("Failed to allocate mb_map\n");
	}
#endif              

	  
                                                            
    
	bufinit();
}

                                                                    
#if !defined(__LP64__)
#define MAX_MBUF_POOL   (512 << MBSHIFT)
#else
#define MAX_MBUF_POOL   (2ULL << GBSHIFT)
#endif                
#define MAX_NCL         (MAX_MBUF_POOL >> MCLSHIFT)

#if SOCKETS
  
                                                        
                                                        
                                                          
                                                            
                          
   
unsigned int
bsd_mbuf_cluster_reserve(boolean_t *overridden)
{
	int mbuf_pool = 0;
	static boolean_t was_overridden = FALSE;

	                                                                     
	if (mbuf_poolsz != 0) {
		goto done;
	}

	  
                                                                    
                                                                     
                                                                   
                                  
    
	(void) PE_parse_boot_argn("ncl", &ncl, sizeof(ncl));
	(void) PE_parse_boot_argn("mbuf_pool", &mbuf_pool, sizeof(mbuf_pool));

	  
                                                           
                                                            
    
	if (mbuf_pool != 0) {
		ncl = (mbuf_pool << MBSHIFT) >> MCLSHIFT;
	}

	if (sane_size > (64 * 1024 * 1024) || ncl != 0) {
		if (ncl || serverperfmode) {
			was_overridden = TRUE;
		}

		if ((nmbclusters = ncl) == 0) {
			                                       
			nmbclusters = mbuf_default_ncl(mem_actual);
		} else {
			                                                        
			if (nmbclusters & 0x1) {
				--nmbclusters;
			}

			                              
			if (nmbclusters > MAX_NCL) {
				nmbclusters = MAX_NCL;
			}
		}

		                                                    
		nmbclusters = (unsigned int)P2ROUNDDOWN(nmbclusters, NCLPG);
	}
	mbuf_poolsz = nmbclusters << MCLSHIFT;
done:
	if (overridden) {
		*overridden = was_overridden;
	}

	return mbuf_poolsz;
}
#endif

#if defined(__LP64__)
extern int tcp_tcbhashsize;
extern int max_cached_sock_count;
#endif


void
bsd_scale_setup(int scale)
{
#if defined(__LP64__)
	if ((scale > 0) && (serverperfmode == 0)) {
		maxproc *= scale;
		maxprocperuid = (maxproc * 2) / 3;
		if (scale > 2) {
			maxfiles *= scale;
			maxfilesperproc = maxfiles / 2;
		}
	}
	                                
	if ((scale > 0) && (serverperfmode != 0)) {
		maxproc = 2500 * scale;
		hard_maxproc = maxproc;
		                 
		maxprocperuid = (maxproc * 3) / 4;
		maxfiles = (150000 * scale);
		maxfilesperproc = maxfiles / 2;
		desiredvnodes = maxfiles;
		vnodes_sized = 1;
		tcp_tfo_backlog = 100 * scale;
		if (scale > 4) {
			                                 
			somaxconn = 2048;
			  
                                 
                            
      
			tcp_tcbhashsize = 32 * 1024;

			if (scale > 7) {
				                       
				max_cached_sock_count = 165000;
			} else {
				max_cached_sock_count = 60000 + ((scale - 1) * 15000);
			}
		} else {
			somaxconn = 512 * scale;
			tcp_tcbhashsize = 4 * 1024 * scale;
			max_cached_sock_count = 60000 + ((scale - 1) * 15000);
		}
	}

	if (maxproc > hard_maxproc) {
		hard_maxproc = maxproc;
	}
#endif
	bsd_exec_setup(scale);
}
