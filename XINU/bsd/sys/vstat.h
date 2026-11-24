/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1998 Apple Computer, Inc. All Rights Reserved */
   
              
   

#ifndef _SYS_VSTAT_H_
#define _SYS_VSTAT_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#warning obsolete header! delete the include from your sources

#ifdef __APPLE_API_OBSOLETE

#include <sys/time.h>
#include <sys/attr.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

struct vstat {
	fsid_t                  vst_volid;                                     
	fsobj_id_t              vst_nodeid;                              
	fsobj_type_t            vst_vnodetype;                                     
	fsobj_tag_t             vst_vnodetag;                                   
	mode_t                  vst_mode;                                          
	nlink_t                 vst_nlink;                                        
	uid_t                   vst_uid;                                                 
	gid_t                   vst_gid;                                                  
	dev_t                   vst_dev;                                    
	dev_t                   vst_rdev;                                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	struct  timespec vst_atimespec;                          
	struct  timespec vst_mtimespec;                                     
	struct  timespec vst_ctimespec;                                      
#else
	time_t                  vst_atime;                                       
	long                    vst_atimensec;                           
	time_t                  vst_mtime;                                                  
	long                    vst_mtimensec;                                      
	time_t                  vst_ctime;                                                   
	long                    vst_ctimensec;                                       
#endif
	off_t                   vst_filesize;                            
	quad_t                  vst_blocks;                                           
	u_int32_t               vst_blksize;                                   
	u_int32_t               vst_flags;                                               
};

#endif                                             
#endif                           

#endif                     
