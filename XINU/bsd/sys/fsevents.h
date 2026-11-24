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
#ifndef FSEVENT_H
#define FSEVENT_H 1

                                             
#define FSE_INVALID             -1
#define FSE_CREATE_FILE          0
#define FSE_DELETE               1
#define FSE_STAT_CHANGED         2
#define FSE_RENAME               3
#define FSE_CONTENT_MODIFIED     4
#define FSE_EXCHANGE             5
#define FSE_FINDER_INFO_CHANGED  6
#define FSE_CREATE_DIR           7
#define FSE_CHOWN                8
#define FSE_XATTR_MODIFIED       9
#define FSE_XATTR_REMOVED       10
#define FSE_DOCID_CREATED       11
#define FSE_DOCID_CHANGED       12
#define FSE_UNMOUNT_PENDING     13                                                                  
#define FSE_CLONE               14

#define FSE_MAX_EVENTS          15
#define FSE_ALL_EVENTS         998

#define FSE_EVENTS_DROPPED     999

  
                                                        
                                                        
                                                      
                                                         
                                                          
                                    
  
#define FSE_TYPE_MASK          0x0fff
#define FSE_FLAG_MASK          0xf000
#define FSE_FLAG_SHIFT         12
#define FSE_GET_FLAGS(type)    (((type) >> 12) & 0x000f)

#define FSE_COMBINED_EVENTS          0x0001
#define FSE_CONTAINS_DROPPED_EVENTS  0x0002


                              
#define FSE_IGNORE    0
#define FSE_REPORT    1
#define FSE_ASK       2                          

                                                  
                                                 
                                             
#define FSE_ARG_VNODE    0x0001                                 
#define FSE_ARG_STRING   0x0002                                               
#define FSE_ARG_PATH     0x0003                             
#define FSE_ARG_INT32    0x0004                              
#define FSE_ARG_INT64    0x0005                              
#define FSE_ARG_RAW      0x0006                                                 
#define FSE_ARG_INO      0x0007                                          
#define FSE_ARG_UID      0x0008                                        
#define FSE_ARG_DEV      0x0009                                  
#define FSE_ARG_MODE     0x000a                                                               
#define FSE_ARG_GID      0x000b                                        
#define FSE_ARG_FINFO    0x000c                                                           
#define FSE_ARG_DONE     0xb33f                       

#define FSE_MAX_ARGS     12

  
                                                        
                                     
  
#define FSE_MODE_HLINK         (1U << 31)                                      
#define FSE_MODE_LAST_HLINK    (1U << 30)                                            
#define FSE_REMOTE_DIR_EVENT   (1U << 29)                                                                     
#define FSE_TRUNCATED_PATH     (1U << 28)                                                 
#define FSE_MODE_CLONE         (1U << 27)                                  

                           
typedef struct fsevent_clone_args {
	int8_t  *event_list;
	int32_t  num_events;
	int32_t  event_queue_depth;
	int32_t *fd;
} fsevent_clone_args;

#define FSEVENTS_CLONE          _IOW('s', 1, fsevent_clone_args)


                           
#pragma pack(push, 4)
typedef struct fsevent_dev_filter_args {
	uint32_t  num_devices;
	dev_t    *devices;
} fsevent_dev_filter_args;
#pragma pack(pop)

#define FSEVENTS_DEVICE_FILTER          _IOW('s', 100, fsevent_dev_filter_args)
#define FSEVENTS_WANT_COMPACT_EVENTS    _IO('s', 101)
#define FSEVENTS_WANT_EXTENDED_INFO     _IO('s', 102)
#define FSEVENTS_GET_CURRENT_ID         _IOR('s', 103, uint64_t)
#define FSEVENTS_UNMOUNT_PENDING_ACK    _IOW('s', 104, dev_t)


#ifdef BSD_KERNEL_PRIVATE

void fsevents_init(void);
void fsevent_unmount(struct mount *mp, vfs_context_t ctx);
struct vnode_attr;
void create_fsevent_from_kevent(vnode_t vp, uint32_t kevents, struct vnode_attr *vap);

                                                             
typedef struct fse_info {
	ino64_t    ino;
	dev_t      dev;
	int32_t    mode;                                                    
	uid_t      uid;
	gid_t      gid;
	uint64_t   nlink;                                                      
} fse_info;

int   get_fse_info(struct vnode *vp, fse_info *fse, vfs_context_t ctx);
int   vnode_get_fse_info_from_vap(vnode_t vp, fse_info *fse, struct vnode_attr *vap);

char *get_pathbuff(void);
void  release_pathbuff(char *path);

#endif                         

#ifdef KERNEL_PRIVATE

int  need_fsevent(int type, vnode_t vp);
int  add_fsevent(int type, vfs_context_t, ...);

#endif                     

#endif                
