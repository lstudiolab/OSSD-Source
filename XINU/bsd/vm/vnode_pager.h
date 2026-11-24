/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef _VNODE_PAGER_
#define _VNODE_PAGER_   1

#include <mach/kern_return.h>
#include <sys/types.h>
#include <kern/queue.h>

#ifdef  KERNEL
#include <mach/boolean.h>
#include <mach/memory_object_types.h>
#include <mach/vm_types.h>
#include <vm/vm_protos.h>
#include <vm/vm_pager.h>

  
                                                                             
                                                                                 
                                                                             
               
  
                                                                               
                                                                               
                                                                    
  
                                                                               
                                                                                
                                                                               
                                                                                  
                                                                                
                                           
   

  
                                            
   
typedef struct pager_file {
	queue_chain_t   pf_chain;                                       
	struct  vnode   *pf_vp;                                   
	u_int           pf_count;                                              
	u_char          *pf_bmap;                               
	long            pf_npgs;                                   
	long            pf_pfree;                                   
	long            pf_lowat;                           
	long            pf_hipage;                                  
	long            pf_hint;                                     
	char            *pf_name;                                  
	boolean_t       pf_prefer;
	int             pf_index;                                            
	void *          pf_lock;                                        
} *pager_file_t;

#define PAGER_FILE_NULL (pager_file_t) 0

#define MAXPAGERFILES 16

#define MAX_BACKING_STORE 100

struct bs_map {
	struct vnode    *vp;
	void            *bs;
};
extern struct bs_map  bs_port_table[];



  
                              
   
#define INDEX_NULL      0
typedef struct {
	unsigned int index:8;                                     
	unsigned int offset:24;                                      
} pf_entry;

typedef enum {
	IS_INODE,                               
	IS_RNODE                         
} vpager_fstype;

  
                                                                     
   
typedef struct vstruct {
	boolean_t       is_device;                                          
	pager_file_t    vs_pf;                                    
	pf_entry        **vs_pmap;                                         
	unsigned int
	                vs_swapfile:1;                           
	short           vs_count;                      
	int             vs_size;                                        
	struct vnode    *vs_vp;                               
} *vnode_pager_t;

#define VNODE_PAGER_NULL        ((vnode_pager_t) 0)

#endif              

#endif                     
