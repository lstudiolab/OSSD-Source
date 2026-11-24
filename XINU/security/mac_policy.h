/*
 * Copyright (c) 2007-2016 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 1999-2002 Robert N. M. Watson
 * Copyright (c) 2001-2005 Networks Associates Technology, Inc.
 * Copyright (c) 2005-2007 SPARTA, Inc.
 * All rights reserved.
 *
 * This software was developed by Robert Watson for the TrustedBSD Project.
 *
 * This software was developed for the FreeBSD Project in part by Network
 * Associates Laboratories, the Security Research Division of Network
 * Associates, Inc. under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"),
 * as part of the DARPA CHATS research program.
 *
 * This software was enhanced by SPARTA ISSO under SPAWAR contract
 * N66001-04-C-6019 ("SEFOS").
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/mac_policy.h,v 1.39 2003/04/18 19:57:37 rwatson Exp $
 */

   
                      
                                                   
  
                                                                      
                                                                   
                                                               
                                                                    
                                                                      
          
   

#ifndef _SECURITY_MAC_POLICY_H_
#define _SECURITY_MAC_POLICY_H_

#ifndef PRIVATE
#warning "MAC policy is not KPI, see Technical Q&A QA1574, this header will be removed in next version"
#endif

#include <security/_label.h>

struct attrlist;
struct auditinfo;
struct bpf_d;
struct cs_blob;
struct devnode;
struct exception_action;
struct fileglob;
struct ifnet;
struct inpcb;
struct ipq;
struct label;
struct mac_policy_conf;
struct mbuf;
struct mount;
struct msg;
struct msqid_kernel;
struct pipe;
struct pseminfo;
struct pshminfo;
struct sbuf;
struct semid_kernel;
struct shmid_kernel;
struct socket;
struct sockopt;
struct task;
struct thread;
struct tty;
struct ucred;
struct vfs_attr;
struct vnode;
                    


  
                                                     
                                                     
                
   
#define MAC_PROC_IDENT_SUPPORT

#ifndef _KAUTH_CRED_T
#define _KAUTH_CRED_T
typedef struct ucred *kauth_cred_t;
#endif                      

#ifndef __IOKIT_PORTS_DEFINED__
#define __IOKIT_PORTS_DEFINED__
#ifdef __cplusplus
class OSObject;
typedef OSObject *io_object_t;
#else
struct OSObject;
typedef struct OSObject *io_object_t;
#endif
#endif                              

   
                                                                     
  
                               
  
      
  
                                     
  
                                          
  
                                                                          
                         
   

   
                                           
  
                                                                         
                                                                      
  
                                                                        
                                                                      
                                                                       
                        
  
                                                                        
                                                                       
                                                                           
                                                                         
                                                                             
                                             
  
                                                                         
                                                               
                                                                  
                                                                    
                                                                         
                                       
   


   
                                    
                                  
                                 
                                 
                              
                                      
  
                                                                      
                                                                         
                                                                            
                                                                       
  
                                                                      
                                               
  
                                                                         
                                                                
  
   
typedef int mpo_audit_check_postselect_t(
	kauth_cred_t cred,
	unsigned short syscode,
	void *args,
	int error,
	int retval
	);
   
                                   
                                  
                                 
                                 
  
                                                                       
                                                                        
                                                                               
                                                                       
                                                                               
                                                             
  
                                                                                 
                                                                                
                                                                              
                                                                         
                                    
  
                                                                   
                                                                            
                                                                     
                                              
  
                                                                  
                                                                       
                                                                        
  
   
typedef int mpo_audit_check_preselect_t(
	kauth_cred_t cred,
	unsigned short syscode,
	void *args
	);
   
                                                                   
                                          
                                 
                                                             
                                                                 
                                               
                                              
                                                       
                           
                                                                 
                                                                          
                   
                                       
                               
  
                                                                       
                                                                        
                                                                      
                                                                           
                                                                      
                                                                      
                                                                        
                             
  
                                                                       
                                                                 
                                                                     
                                       
  
                                                                    
                                                                         
  
                                                                       
                                         
  
                                                                      
                                                                          
                                                                          
  
                                                              
   
typedef int mpo_cred_check_label_update_execve_t(
	kauth_cred_t old,
	struct vnode *vp,
	off_t offset,
	struct vnode *scriptvp,
	struct label *vnodelabel,
	struct label *scriptvnodelabel,
	struct label *execlabel,
	struct proc *p,
	void *macpolicyattr,
	size_t macpolicyattrlen
	);
   
                                                         
                                  
                                                             
                                
                     
  
                                                                          
                                                                           
                                                                         
                                                                       
                             
  
                                                                             
                             
   
typedef int mpo_cred_check_label_update_t(
	kauth_cred_t cred,
	struct label *newlabel
	);
   
                                                                
                                
                               
  
                                                                     
                                                                         
                                                                         
                                              
  
                                                                             
                                                                           
                                                             
   
typedef int mpo_cred_check_visible_t(
	kauth_cred_t u1,
	kauth_cred_t u2
	);
   
                                                            
                                                      
                               
  
                                                          
                                           
                                                            
                                                             
                                      
   
typedef void mpo_cred_label_associate_fork_t(
	kauth_cred_t cred,
	proc_t proc
	);
   
                                   
                                                
  
                                                                     
                                                              
                                                
   
typedef void mpo_cred_label_associate_kernel_t(
	kauth_cred_t cred
	);
   
                                    
                                        
                                      
  
                                                                      
                                                  
  
                                                                   
                                                                 
                                   
   
typedef void mpo_cred_label_associate_t(
	kauth_cred_t parent_cred,
	kauth_cred_t child_cred
	);
   
                                   
                                                
  
                                                                     
                                                                       
                                                                  
   
typedef void mpo_cred_label_associate_user_t(
	kauth_cred_t cred
	);
   
                                   
                                          
  
                                                               
                                                                  
                                                                  
   
typedef void mpo_cred_label_destroy_t(
	struct label *label
	);
   
                                                           
                                         
                                                                              
                
                                                                                
  
                                                                            
                                                                           
                                                                               
                                                                           
                              
  
                                                                  
                                 
  
   
typedef int mpo_cred_label_externalize_audit_t(
	struct label *label,
	char *element_name,
	struct sbuf *sb
	);
   
                                              
                                         
                                                                              
                
                                                                                
  
                                                             
                                                                        
                                                                  
                                                                   
            
  
                                                                  
                                 
  
   
typedef int mpo_cred_label_externalize_t(
	struct label *label,
	char *element_name,
	struct sbuf *sb
	);
   
                                           
                                        
  
                                                                  
                          
   
typedef void mpo_cred_label_init_t(
	struct label *label
	);
   
                                              
                                         
                                                                              
                   
                                                    
  
                                                                        
                                                                     
                                                                      
                                                                
                                            
  
                                                                    
                                                          
  
                                                                       
                                       
  
   
typedef int mpo_cred_label_internalize_t(
	struct label *label,
	char *element_name,
	char *element_data
	);
   
                                         
                                               
                                                        
                            
                                 
                                                             
                                                                 
                                               
                                              
                                                       
                                                          
                                                                  
                                                                           
                   
                                             
                               
  
                                                                 
                                                                       
                                                                      
                                                                             
                                                                      
                                                                    
  
                                                                       
                                                                 
                                                                     
                                       
  
                                                                    
                                                                         
  
                                                                      
                                                                       
                           
  
                                                                       
                                         
                                                                         
                         
   
typedef int mpo_cred_label_update_execve_t(
	kauth_cred_t old_cred,
	kauth_cred_t new_cred,
	struct proc *p,
	struct vnode *vp,
	off_t offset,
	struct vnode *scriptvp,
	struct label *vnodelabel,
	struct label *scriptvnodelabel,
	struct label *execlabel,
	u_int *csflags,
	void *macpolicyattr,
	size_t macpolicyattrlen,
	int *disjointp
	);
   
                                    
                                       
                                                          
                                      
                     
  
                                                                        
                                                                      
                                                                 
   
typedef void mpo_cred_label_update_t(
	kauth_cred_t cred,
	struct label *newlabel
	);
   
                                    
                                                      
                                        
                                  
                                                                         
  
                                                                              
                                                              
                                                              
   
typedef void mpo_devfs_label_associate_device_t(
	dev_t dev,
	struct devnode *de,
	struct label *label,
	const char *fullpath
	);
   
                                       
                                        
                                         
                                      
                                  
                                                                       
  
                                                                           
                                                                                  
                                                                         
   
typedef void mpo_devfs_label_associate_directory_t(
	const char *dirname,
	int dirnamelen,
	struct devnode *de,
	struct label *label,
	const char *fullpath
	);
   
                             
                                 
                                       
  
                                                                       
                                                                        
             
   
typedef void mpo_devfs_label_copy_t(
	struct label *src,
	struct label *dest
	);
   
                              
                                          
  
                                                               
                                                                        
                                               
   
typedef void mpo_devfs_label_destroy_t(
	struct label *label
	);
   
                                 
                                        
  
                                                                        
                 
   
typedef void mpo_devfs_label_init_t(
	struct label *label
	);
   
                                                           
                               
                                            
                                                 
                                      
                                        
  
                                                               
                                                                 
                                         
   
typedef void mpo_devfs_label_update_t(
	struct mount *mp,
	struct devnode *de,
	struct label *delabel,
	struct vnode *vp,
	struct label *vnodelabel
	);
   
                                                                         
                                                  
                                  
                                                     
  
                                                                    
                                                                       
                                                                        
                                                                        
                                                                      
                                                                     
                                                                    
                                     
  
                                                             
                                                   
   
typedef int mpo_exc_action_check_exception_send_t(
	struct label *crashlabel,
	struct exception_action *action,
	struct label *exclabel
	);
   
                                              
                                           
                                                                     
  
                                         
   
typedef void mpo_exc_action_label_associate_t(
	struct exception_action *action,
	struct label *exclabel
	);
   
                                         
                                          
  
                                                                       
                                                                 
                                                                      
              
   
typedef void mpo_exc_action_label_destroy_t(
	struct label *label
	);
   
                                                                      
                                          
                                                  
  
                                                                
                                                                         
                                                                   
                                                                   
                                                                    
                                                                
   
typedef void mpo_exc_action_label_populate_t(
	struct label *label,
	struct proc *proc
	);
   
                                            
                                        
  
                                                                
                                                       
   
typedef int mpo_exc_action_label_init_t(
	struct label *label
	);
   
                                                  
                                                                    
                              
                                       
                                        
  
                                                                
                                                                   
                                                                    
                                                           
   
typedef int mpo_exc_action_label_update_t(
	struct exception_action *action,
	struct label *label,
	struct label *newlabel
	);
   
                                                                      
                                  
                                
                                    
  
                                                                  
                                                    
  
                                                                   
                                       
   
typedef int mpo_file_check_change_offset_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label
	);
   
                                                        
                                  
  
                                                                  
                                   
  
                                                                   
                                       
   
typedef int mpo_file_check_create_t(
	kauth_cred_t cred
	);
   
                                                           
                                  
                                
                                    
                                           
  
                                                                  
                                                                  
                            
  
                                                                   
                                       
   
typedef int mpo_file_check_dup_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int newfd
	);
   
                                         
                                  
                                
                                    
                                                              
                                           
  
                                                                          
                                                
  
                                                                             
                             
   
typedef int mpo_file_check_fcntl_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int cmd,
	user_long_t arg
	);
   
                                              
                                  
                                
                                  
                               
  
                                                                                
                                                                              
  
                                                                             
                             
   
typedef int mpo_file_check_get_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	char *elements,
	size_t len
	);
   
                                                                     
                                  
                                
                                    
  
                                                                  
                                                 
  
                                                                   
                                       
   
typedef int mpo_file_check_get_offset_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label
	);
   
                                                          
                                  
                                
                                    
  
                                                                  
                                                     
  
                                                                   
                                       
   
typedef int mpo_file_check_inherit_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label
	);
   
                                              
                                  
                                
                                    
                                              
  
                                                                          
                                         
  
                                                                      
                                                                    
                          
  
                                                                             
                             
  
   
typedef int mpo_file_check_ioctl_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	unsigned long cmd
	);
   
                                                
                                  
                                
                                    
                                                           
                                 
  
                                                                          
                                                                            
  
                                                                             
                             
  
   
typedef int mpo_file_check_lock_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int op,
	struct flock *fl
	);
   
                                                                                                
                            
                                
                                                
                                                                                                  
                                                
  
                                                                       
                                                                      
                                                                  
                                                                       
                                                                      
                                
  
                                                                             
                             
   
typedef int mpo_file_check_library_validation_t(
	struct proc *p,
	struct fileglob *fg,
	off_t slice_offset,
	user_long_t error_message,
	size_t error_message_size
	);
   
                                                  
                                  
                                               
                                                
                                             
                                                   
                                 
  
                                                                        
                                                                            
                                                                        
                                                                               
                                                                         
                              
  
                                                                             
                                                                             
                                
   
typedef int mpo_file_check_mmap_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int prot,
	int flags,
	uint64_t file_pos,
	int *maxprot
	);
   
                                         
                                  
                         
                                                
                                                 
  
                                                                          
   
typedef void mpo_file_check_mmap_downgrade_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int *prot
	);
   
                                                         
                                  
                                
                                    
  
                                                                  
                                                     
  
                                                                   
                                       
   
typedef int mpo_file_check_receive_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label
	);
   
                                              
                                  
                                
                                   
                                        
  
                                                                  
                                                                       
                                         
  
                                                                             
                             
   
typedef int mpo_file_check_set_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	char *elements,
	size_t len
	);
   
                                                        
                                  
                                
                                    
                                                                
  
                                                                      
                                                              
   
typedef void mpo_file_notify_close_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label,
	int modified
	);
   
                            
                                  
                                
                                    
   
typedef void mpo_file_label_associate_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	struct label *label
	);
   
                             
                                          
  
                                                                   
                                                                  
                                      
   
typedef void mpo_file_label_destroy_t(
	struct label *label
	);
   
                                
                                        
   
typedef void mpo_file_label_init_t(
	struct label *label
	);
   
                                                             
                                  
                                           
                                            
  
                                                                          
                                                                         
                                                                       
                                            
  
                                                                      
                             
   
typedef int mpo_iokit_check_open_t(
	kauth_cred_t cred,
	io_object_t user_client,
	unsigned int user_client_type
	);
   
                                                             
                                  
                                   
                                            
  
                                                                         
                                                                   
                                                                           
                           
  
                                                                      
                             
   
typedef int mpo_iokit_check_open_service_t(
	kauth_cred_t cred,
	io_object_t service,
	unsigned int user_client_type
	);
   
                                                                     
                                  
                              
                                   
  
                                                                      
                                    
  
                                                                      
                             
   
typedef int mpo_iokit_check_set_properties_t(
	kauth_cred_t cred,
	io_object_t entry,
	io_object_t properties
	);
   
                                                               
                                  
                              
                                       
  
                                                                    
                                                 
                                                          
                                                                 
                                                                    
                      
  
                                                                  
                                                                     
                                                             
  
                                                                      
                                                                          
                                                                          
  
                                                              
   
typedef int mpo_iokit_check_filter_properties_t(
	kauth_cred_t cred,
	io_object_t entry
	);
   
                                                                     
                                  
                              
                             
  
                                                                      
                                    
  
                                                                      
          
   
typedef int mpo_iokit_check_get_property_t(
	kauth_cred_t cred,
	io_object_t entry,
	const char *name
	);
   
                                                        
                                  
  
                                                                  
                                                                  
                                                           
  
                                                                      
          
   
typedef int mpo_iokit_check_hid_control_t(
	kauth_cred_t cred
	);
   
                                         
                                  
                             
                                                      
                                                              
  
                                                                          
                                          
  
                                                                            
                                                                          
                                                                           
                                                                                
  
                                                                             
                             
   
typedef int mpo_mount_check_fsctl_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *label,
	unsigned long cmd
	);
   
                                                                           
                                  
                                                    
                                       
  
                                                                         
                                                                              
                                                                               
  
                                                                             
                             
  
                                                                      
                                    
   

typedef int mpo_mount_check_getattr_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mp_label,
	struct vfs_attr *vfa
	);
   
                                                          
                                  
                                            
                                                           
  
                                                                          
                                                                           
  
                                                                             
                                                                          
                                   
   
typedef int mpo_mount_check_label_update_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mntlabel
	);
   
                                                          
                                  
                                                 
                                                 
                                    
                                        
  
                                                                          
                                            
  
                                                                             
                             
   
typedef int mpo_mount_check_mount_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	struct componentname *cnp,
	const char *vfc_name
	);
   
                                                                 
                                  
                         
  
                                                                         
                                                                            
           
  
                                                                             
                             
   
typedef int mpo_mount_check_mount_late_t(
	kauth_cred_t cred,
	struct mount *mp
	);

   
                                            
                                  
                                                                   
                                                            
  
                                                                          
                                            
  
                                                                             
                             
   
typedef int mpo_mount_check_quotactl_t(
	kauth_cred_t cred,
	struct mount *mp,
	int cmd,
	int id
	);
   
                                                      
                                  
                                                    
                                    
  
                                                                  
                                                                 
  
                                                                         
                                 
   
typedef int mpo_mount_check_snapshot_create_t(
	kauth_cred_t cred,
	struct mount *mp,
	const char *name
	);
   
                                                      
                                  
                                                    
                                    
  
                                                                  
                                                              
                
  
                                                                         
                                 
   
typedef int mpo_mount_check_snapshot_delete_t(
	kauth_cred_t cred,
	struct mount *mp,
	const char *name
	);
   
                                                     
                                  
                                                        
                                                       
                                
                                                 
                                    
                                         
                                        
  
                                                                  
                                                             
              
  
                                                                         
                                 
   
typedef int mpo_mount_check_snapshot_mount_t(
	kauth_cred_t cred,
	struct vnode *rvp,
	struct vnode *vp,
	struct componentname *cnp,
	const char *name,
	const char *vfc_name
	);
   
                                                      
                                  
                                                    
                                       
  
                                                                  
                                                                         
  
                                                                         
                                 
   
typedef int mpo_mount_check_snapshot_revert_t(
	kauth_cred_t cred,
	struct mount *mp,
	const char *name
	);
   
                                                       
                                  
                             
                                                                 
  
                                                                          
                                              
  
                                                                             
                             
   
typedef int mpo_mount_check_remount_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mlabel
	);
   
                                                                          
                                  
                                                    
                                       
  
                                                                         
                                                             
  
                                                                             
                             
   

typedef int mpo_mount_check_setattr_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mp_label,
	struct vfs_attr *vfa
	);
   
                                                          
                                  
                                      
                                       
  
                                                                      
                                                                       
                                                                      
                                                                          
                                                                       
               
  
                                                                             
                                                                          
                                   
   
typedef int mpo_mount_check_stat_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mntlabel
	);
   
                                                           
                                  
                             
                                                       
  
                                                                          
                                              
  
                                                                             
                             
   
typedef int mpo_mount_check_umount_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mlabel
	);
   
                              
                                  
                                                      
                                                               
                               
  
                                                                        
                                                                            
   
typedef void mpo_mount_label_associate_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mntlabel
	);
   
                              
                                          
  
                                                 
                                                                
                                                                
              
   
typedef void mpo_mount_label_destroy_t(
	struct label *label
	);
   
                                          
                                         
                                                                              
                
                                                                                
  
                                                                    
                                                                     
                                                                      
                                                            
  
                                                                    
                                                          
  
                                                                  
                                 
  
   
typedef int mpo_mount_label_externalize_t(
	struct label *label,
	char *element_name,
	struct sbuf *sb
	);
   
                                       
                                        
  
                                                                  
                                                                     
                                                                  
                                                                       
                                                                      
                                                                
                                   
   
typedef void mpo_mount_label_init_t(
	struct label *label
	);
   
                                          
                                         
                                                                              
                   
                                                    
  
                                                                            
                                                                        
                                                                      
                                                                
                                            
  
                                                                    
                                                          
  
                                                                       
                                       
  
   
typedef int mpo_mount_label_internalize_t(
	struct label *label,
	char *element_name,
	char *element_data
	);
   
                                                                   
                                  
                           
  
                                                                       
                            
  
                                                                             
                             
  
   
typedef int mpo_necp_check_open_t(
	kauth_cred_t cred,
	int flags
	);
   
                                                         
                                  
                           
                                    
  
                                                                       
                   
  
                                                                             
                             
  
   
typedef int mpo_necp_check_client_action_t(
	kauth_cred_t cred,
	struct fileglob *fg,
	uint32_t action
	);
   
                                              
                                  
                                      
                                          
                                              
  
                                                                          
                                         
  
                                                                      
                                                                    
                          
  
                                                                             
                             
  
   
typedef int mpo_pipe_check_ioctl_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel,
	unsigned long cmd
	);
   
                                                 
                                  
                          
                                      
                                              
  
                                                                  
                                         
  
                                                                   
                                       
   
typedef int mpo_pipe_check_kqfilter_t(
	kauth_cred_t cred,
	struct knote *kn,
	struct pipe *cpipe,
	struct label *pipelabel
	);
   
                                             
                                  
                                      
                                          
  
                                                                  
                                                                       
                                                            
  
                                                                             
                             
  
   
typedef int mpo_pipe_check_read_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel
	);
   
                                               
                                  
                                      
                                          
                                                           
  
                                                                  
                                                                         
                                                            
  
                                                                             
                             
  
   
typedef int mpo_pipe_check_select_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel,
	int which
	);
   
                                             
                                  
                                      
                                          
  
                                                                  
                                                                       
                                                            
  
                                                                             
                             
  
   
typedef int mpo_pipe_check_stat_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel
	);
   
                                              
                                  
                                      
                                          
  
                                                                  
                                                                        
                                                            
  
                                                                             
                             
  
   
typedef int mpo_pipe_check_write_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel
	);
   
                              
                                  
                                     
                                              
  
                                                                    
                                                                         
                                                 
   
typedef void mpo_pipe_label_associate_t(
	kauth_cred_t cred,
	struct pipe *cpipe,
	struct label *pipelabel
	);
   
                             
                                          
  
                                                                  
                                                                       
                                      
   
typedef void mpo_pipe_label_destroy_t(
	struct label *label
	);
   
                                
                                        
  
                                                                           
                          
   
typedef void mpo_pipe_label_init_t(
	struct label *label
	);
   
                              
                                       
  
                                                                         
                                                                         
                                                                         
                                                                    
                                  
  
                                                                    
                                                                       
                                         
  
                                   
   
typedef void mpo_policy_destroy_t(
	struct mac_policy_conf *mpc
	);
   
                                      
                                       
                            
                             
  
                                                                      
                                                                      
                                                                        
                                                                       
                                                                  
                                                                       
                                                              
                                                                           
                                                                   
                                                                       
  
                                                                    
                                                                       
                                         
   
typedef void mpo_policy_init_t(
	struct mac_policy_conf *mpc
	);
   
                                          
                                       
                          
  
                                                                      
                                                                     
                                                                      
                                                                        
                                                                        
                                                                       
                                                                     
                                                                           
                                                                     
  
                                                                    
                                                                       
                                         
   
typedef void mpo_policy_initbsd_t(
	struct mac_policy_conf *mpc
	);
   
                                   
                            
                                              
                                           
  
                                                                      
                                                                      
                                                                      
                                                                        
                                                                 
                                                                   
                                                                
                                                                      
                                                                       
                                                    
  
                                                                 
                                                                        
                                                                  
                                                                  
                                                                       
                                                                     
  
                                                                    
                                                        
   
typedef int mpo_policy_syscall_t(
	struct proc *p,
	int call,
	user_addr_t arg
	);
   
                                                          
                                  
                                            
  
                                                                         
                                        
  
                                                                             
                             
   
typedef int mpo_posixsem_check_create_t(
	kauth_cred_t cred,
	const char *name
	);
   
                                                        
                                  
                                                        
                                                       
  
                                                                       
                                                  
  
                                                                             
                             
   
typedef int mpo_posixsem_check_open_t(
	kauth_cred_t cred,
	struct pseminfo *ps,
	struct label *semlabel
	);
   
                                                        
                                  
                                                        
                                                       
  
                                                                         
                                                  
  
                                                                             
                             
   
typedef int mpo_posixsem_check_post_t(
	kauth_cred_t cred,
	struct pseminfo *ps,
	struct label *semlabel
	);
   
                                                          
                                  
                                                        
                                                       
                                            
  
                                                                         
                                                  
  
                                                                             
                             
   
typedef int mpo_posixsem_check_unlink_t(
	kauth_cred_t cred,
	struct pseminfo *ps,
	struct label *semlabel,
	const char *name
	);
   
                                                        
                                  
                                                        
                                                       
  
                                                                       
                                                  
  
                                                                             
                             
   
typedef int mpo_posixsem_check_wait_t(
	kauth_cred_t cred,
	struct pseminfo *ps,
	struct label *semlabel
	);
   
                                         
                                  
                                                        
                                                             
                                            
  
                                                          
                                                                    
                                                                        
                        
   
typedef void mpo_posixsem_label_associate_t(
	kauth_cred_t cred,
	struct pseminfo *ps,
	struct label *semlabel,
	const char *name
	);
   
                                        
                                          
  
                                                         
                                                                       
                                                          
   
typedef void mpo_posixsem_label_destroy_t(
	struct label *label
	);
   
                                           
                                        
  
                                                                           
                 
   
typedef void mpo_posixsem_label_init_t(
	struct label *label
	);
   
                                                                     
                                  
                                                       
  
                                                                         
                                                      
  
                                                                             
                             
   
typedef int mpo_posixshm_check_create_t(
	kauth_cred_t cred,
	const char *name
	);
   
                                                               
                                  
                                                            
                                                                  
                                             
                                          
  
                                                                      
                                                             
  
                                                                             
                             
   
typedef int mpo_posixshm_check_mmap_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel,
	int prot,
	int flags
	);
   
                                                                   
                                  
                                                            
                                                                  
                                                           
  
                                                                       
                                   
  
                                                                             
                             
   
typedef int mpo_posixshm_check_open_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel,
	int fflags
	);
   
                                                            
                                  
                                                            
                                                                  
  
                                                                         
                                                                        
  
                                                                             
                             
   
typedef int mpo_posixshm_check_stat_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel
	);
   
                                                                
                                  
                                                            
                                                                  
                                                                 
  
                                                                           
                                                                                
  
                                                                             
                             
   
typedef int mpo_posixshm_check_truncate_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel,
	off_t len
	);
   
                                                              
                                  
                                                            
                                                                  
                                                       
  
                                                                         
                                                             
  
                                                                             
                             
   
typedef int mpo_posixshm_check_unlink_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel,
	const char *name
	);
   
                                                    
                                  
                                                            
                                                                        
                                                       
  
                                                                     
                                                                      
                                                                      
                                  
   
typedef void mpo_posixshm_label_associate_t(
	kauth_cred_t cred,
	struct pshminfo *ps,
	struct label *shmlabel,
	const char *name
	);
   
                                            
                                          
  
                                                          
                                                                
                                                                
              
   
typedef void mpo_posixshm_label_destroy_t(
	struct label *label
	);
   
                                                      
                                        
  
                                                                     
                                  
   
typedef void mpo_posixshm_label_init_t(
	struct label *label
	);
   
                                                         
                                  
                                                    
  
                                                                          
                                                                          
                                                                          
                                                  
  
                                                                              
   
typedef int mpo_priv_check_t(
	kauth_cred_t cred,
	int priv
	);
   
                                                                           
                                  
                                                    
  
                                                                        
                                                                        
                                                                          
                                                                       
                                                                         
  
                                                                   
                                                                      
                                                                         
                                                 
  
                                                                        
                       
   
typedef int mpo_priv_grant_t(
	kauth_cred_t cred,
	int priv
	);
   
                                                 
                               
  
                                                                        
               
  
                                                                             
                             
   
typedef int mpo_proc_check_dump_core_t(
	struct proc *proc
	);
   
                                                     
                                  
                              
                                                                   
                                                                          
                                                                         
  
                                                                           
                                                                         
                                                                       
                                                                           
                                                    
  
                                                                             
                             
   
typedef int mpo_proc_check_remote_thread_create_t(
	kauth_cred_t cred,
	struct proc *proc,
	int flavor,
	thread_state_t new_state,
	mach_msg_type_number_t new_state_count
	);
   
                                                     
                                  
                                                  
  
                                                                        
                                                                        
                                                                          
                               
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_debug_t(
	kauth_cred_t cred,
	struct proc_ident *pident
	);
   
                                   
                                  
                                              
  
                                                                
  
                                                                             
                             
   
typedef int mpo_proc_check_fork_t(
	kauth_cred_t cred,
	struct proc *proc
	);
   
                                                               
                                  
                                          
                                                         
  
                                                                             
                             
   
typedef int mpo_proc_check_set_host_special_port_t(
	kauth_cred_t cred,
	int id,
	struct ipc_port *port
	);
   
                                                                 
                                  
                                          
  
                                                                             
                             
   
typedef int mpo_proc_check_set_host_exception_port_t(
	kauth_cred_t cred,
	unsigned int exception
	);
   
                                                                                              
                                  
  
                                                                             
                             
   
typedef int mpo_proc_check_get_movable_control_port_t(
	kauth_cred_t cred
	);
   
                                                                                  
                                                 
                                  
  
                                                                             
                             
   
typedef int mpo_proc_check_dyld_process_info_notify_register_t(
	kauth_cred_t cred
	);
   
                                                                 
                                  
                              
                                                          
                                                    
                                                              
  
                                                                            
                                                    
                                                                               
                                                                             
                       
  
                                                                             
                             
   
typedef int mpo_proc_check_suspend_resume_t(
	kauth_cred_t cred,
	struct proc *proc,
	int sr
	);
   
                                                                
                                  
  
                                                                      
                                                                       
                                                                               
  
                                                                             
                             
   
typedef int mpo_proc_check_getaudit_t(
	kauth_cred_t cred
	);
   
                                                            
                                  
  
                                                                      
                                                                            
                
  
                                                                             
                             
   
typedef int mpo_proc_check_getauid_t(
	kauth_cred_t cred
	);
   
                                                               
                             
                             
                                   
  
                                                     
  
                                                                            
                         
  
                                                                             
                             
   
typedef int mpo_proc_check_getlcid_t(
	struct proc *p0,
	struct proc *p,
	pid_t pid
	);
   
                                                                 
                                  
                                
                              
  
                                                    
  
                                                                            
                         
  
                                                                             
                             
   
typedef int mpo_proc_check_ledger_t(
	kauth_cred_t cred,
	struct proc *target,
	int op
	);
   
                                                                   
                                  
                                                             
  
                                                                                     
                                               
  
                                                                             
                             
   
typedef int mpo_proc_check_proc_info_t(
	kauth_cred_t cred,
	struct proc *target,
	int callnum,
	int flavor
	);
   
                                                                        
                                  
                                
                                                    
  
                                                                        
                                                                     
  
                                                                             
                             
   
typedef int mpo_proc_check_get_cs_info_t(
	kauth_cred_t cred,
	struct proc *target,
	unsigned int op
	);
   
                                                                     
                                  
                                
                                                     
  
                                                                        
                                                                     
  
                                                                       
                                      
   
typedef int mpo_proc_check_set_cs_info_t(
	kauth_cred_t cred,
	struct proc *target,
	unsigned int op
	);
   
                                                 
                                                  
                                  
                                                   
                                                    
                                             
                                                   
                                 
  
                                                                        
                                                                    
                                                                          
                                                                 
                                                                                
                                                                      
                                                                              
                                                                          
  
                                                                             
                                                                             
   
typedef int mpo_proc_check_map_anon_t(
	struct proc *proc,
	kauth_cred_t cred,
	user_addr_t u_addr,
	user_size_t u_size,
	int prot,
	int flags,
	int *maxprot
	);
   
                                                           
                                  
                                                
                                      
  
                                                                     
                                                                   
  
                                                                             
                             
   
typedef int mpo_proc_check_memorystatus_control_t(
	kauth_cred_t cred,
	int32_t command,
	pid_t pid
	);
   
                                                              
                                  
                                                  
                                                 
                                                  
                                               
  
                                                                     
                                                                       
                        
  
                                                                             
                             
   
typedef int mpo_proc_check_mprotect_t(
	kauth_cred_t cred,
	struct proc *proc,
	user_addr_t addr,
	user_size_t size,
	int prot
	);
   
                                                                  
                                  
                              
  
                                                                         
                                                    
  
                                                                             
                                                                           
                                                              
   
typedef int mpo_proc_check_sched_t(
	kauth_cred_t cred,
	struct proc *proc
	);
   
                                                             
                                  
                               
  
                                                                      
                                                                        
                                                               
  
                                                                             
                             
   
typedef int mpo_proc_check_setaudit_t(
	kauth_cred_t cred,
	struct auditinfo_addr *ai
	);
   
                                                         
                                  
                             
  
                                                                      
                                                                      
                
  
                                                                             
                             
   
typedef int mpo_proc_check_setauid_t(
	kauth_cred_t cred,
	uid_t auid
	);
   
                                                             
                             
                             
                                   
                                     
  
                                                     
  
                                                                        
                                                                     
  
                                                                      
  
                                                                             
                             
   
typedef int mpo_proc_check_setlcid_t(
	struct proc *p0,
	struct proc *p,
	pid_t pid,
	pid_t lcid
	);
   
                                                     
                                  
                              
                                            
  
                                                                          
                                            
  
                                                                     
                                                                        
                                                                   
  
                                                                             
                                                                           
                                                              
   
typedef int mpo_proc_check_signal_t(
	kauth_cred_t cred,
	struct proc *proc,
	int signum
	);
   
                                                  
                               
                                                              
  
                                                                                  
  
                                                                             
                                                                                
                                         
  
                                                                             
                                                                             
   
typedef int mpo_proc_check_syscall_unix_t(
	struct proc *proc,
	int scnum
	);
   
                                        
                                  
                              
  
                                                                       
                            
  
                                                                 
                                                                       
                                                                    
                                                                      
  
                                                                             
                             
   
typedef int mpo_proc_check_wait_t(
	kauth_cred_t cred,
	struct proc *proc
	);
   
                                                        
                                  
                                            
  
                                                                          
                                          
  
                                                                             
                             
   
typedef int mpo_proc_check_work_interval_ctl_t(
	kauth_cred_t cred,
	uint32_t operation
	);
   
                                                         
                              
  
                                                                 
                                                                   
                                                              
                                                                  
                                                                  
   
typedef void mpo_proc_notify_exit_t(
	struct proc *proc
	);
   
                                                        
                                  
                           
                                                       
                                                          
                                                       
  
                                                                  
                                                                  
         
  
                                                                   
                                       
   
typedef int mpo_skywalk_flow_check_connect_t(
	kauth_cred_t cred,
	void *flow,
	const struct sockaddr *addr,
	int type,
	int protocol
	);
   
                                                       
                                  
                           
                                                   
                                                          
                                                       
  
                                                                  
                                                                   
            
  
                                                                   
                                       
   
typedef int mpo_skywalk_flow_check_listen_t(
	kauth_cred_t cred,
	void *flow,
	const struct sockaddr *addr,
	int type,
	int protocol
	);
   
                                                 
                                  
                           
                                            
  
                                                                           
                                                                   
  
                                                                   
                                       
   
typedef int mpo_socket_check_accept_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel
	);
   
                                                           
                                  
                           
                                            
                                                             
  
                                                                           
                                                                       
  
                                                                   
                                       
   
typedef int mpo_socket_check_accepted_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockaddr *addr
	);
   
                                               
                                  
                           
                                            
                                            
  
                                                                         
                                  
  
                                                                   
                                       
   
typedef int mpo_socket_check_bind_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockaddr *addr
	);
   
                                                  
                                  
                           
                                            
                                            
  
                                                                  
                                                                     
  
                                                                   
                                       
   
typedef int mpo_socket_check_connect_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockaddr *addr
	);
   
                                                         
                                  
                                      
                           
                                   
  
                                                                  
                           
  
                                                                   
                                       
   
typedef int mpo_socket_check_create_t(
	kauth_cred_t cred,
	int domain,
	int type,
	int protocol
	);
   
                                                 
                                  
                           
                                              
                                            
  
                                                                          
                                                             
  
                                                                      
                                                                 
                                                                    
                          
  
                                                                             
                             
   
typedef int mpo_socket_check_ioctl_t(
	kauth_cred_t cred,
	socket_t so,
	unsigned long cmd,
	struct label *socklabel
	);
   
                                                 
                                  
                           
                                            
  
                                                                  
                                  
  
                                                                   
                                       
   
typedef int mpo_socket_check_listen_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel
	);
   
                                                  
                                  
                           
                                            
  
                                                                  
                                 
  
                                                                   
                                       
   
typedef int mpo_socket_check_receive_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel
	);

   
                                                  
                                  
                             
                                            
                                          
  
                                                                  
                                                        
  
                                                                   
                                       
   
typedef int mpo_socket_check_received_t(
	kauth_cred_t cred,
	struct socket *sock,
	struct label *socklabel,
	struct sockaddr *saddr
	);

   
                                               
                                  
                           
                                            
                                     
  
                                                                       
                       
  
                                                                   
                                       
   
typedef int mpo_socket_check_send_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockaddr *addr
	);
   
                                                            
                                  
                           
                                        
  
                                                                  
                                                       
  
                                                                   
                                       
   
typedef int mpo_socket_check_stat_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel
	);
   
                                                          
                                  
                           
                                        
                                     
  
                                                                  
                                                           
  
                                                                   
                                       
   
typedef int mpo_socket_check_setsockopt_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockopt *sopt
	);
   
                                                          
                                  
                           
                                        
                                  
  
                                                                  
                                                           
  
                                                                   
                                       
   
typedef int mpo_socket_check_getsockopt_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *socklabel,
	struct sockopt *sopt
	);
   
                                                       
                                  
                             
                                          
  
                                                                         
                                                                     
                                 
  
                                                                         
                                                                        
  
                                                                             
                             
   
typedef int mpo_system_check_acct_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                         
                                  
                              
                                     
  
                                                                         
                                                                               
  
                                                                             
                             
   
typedef int mpo_system_check_audit_t(
	kauth_cred_t cred,
	void *record,
	int length
	);
   
                                                     
                                  
                        
                                      
  
                                                                            
                                                                                
               
  
                                                                             
                             
   
typedef int mpo_system_check_auditctl_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vl
	);
   
                                                         
                                  
                                    
  
                                                                          
                                                                            
  
                                                                             
                             
   
typedef int mpo_system_check_auditon_t(
	kauth_cred_t cred,
	int cmd
	);
   
                                                                   
                                  
  
                                                                  
                                 
  
                                                                 
   
typedef int mpo_system_check_host_priv_t(
	kauth_cred_t cred
	);
   
                                                                
                                  
                                                               
  
                                                                        
                                                   
  
                                                                            
                                                                     
                                                                             
                                                                          
  
                                                                             
                             
   
typedef int mpo_system_check_info_t(
	kauth_cred_t cred,
	const char *info_type
	);
   
                                                        
                                  
  
                                                                        
                              
  
                                                                             
                             
   
typedef int mpo_system_check_nfsd_t(
	kauth_cred_t cred
	);
   
                                          
                                  
                                               
  
                                                                        
                                                         
  
                                                                             
                             
   
typedef int mpo_system_check_reboot_t(
	kauth_cred_t cred,
	int howto
	);
   
                                                        
                                  
  
                                                                        
                                    
  
                                                                             
                             
   
typedef int mpo_system_check_settime_t(
	kauth_cred_t cred
	);
   
                                                         
                                  
                         
                                         
  
                                                                        
                                          
  
                                                                             
                             
   
typedef int mpo_system_check_swapoff_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label
	);
   
                                                       
                                  
                         
                                         
  
                                                                        
                                       
  
                                                                             
                             
   
typedef int mpo_system_check_swapon_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label
	);
   
                                          
                                  
                                                           
                                           
                                                                  
                                                                   
                                                     
                                                            
                                                     
  
                                                                        
                                                        
  
                                                                      
                                                                        
                                                                  
  
                                                                             
                             
   
typedef int mpo_system_check_sysctlbyname_t(
	kauth_cred_t cred,
	const char *namestring,
	int *name,
	size_t namelen,
	user_addr_t old,                    
	size_t oldlen,
	user_addr_t newvalue,               
	size_t newlen
	);
   
                                            
                                  
                                                                     
  
                                                                          
                                                        
                                   
  
                                                                             
                             
   
typedef int mpo_system_check_kas_info_t(
	kauth_cred_t cred,
	int selector
	);
   
                                          
                                  
                                                                 
                                                  
                             
                                            
  
                                                         
   
typedef void mpo_sysvmsg_label_associate_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel,
	struct msg *msgptr,
	struct label *msglabel
	);
   
                                         
                                          
  
                                                          
                                                                       
                                                          
   
typedef void mpo_sysvmsg_label_destroy_t(
	struct label *label
	);
   
                                            
                                        
  
                                                                   
   
typedef void mpo_sysvmsg_label_init_t(
	struct label *label
	);
   
                                            
                                          
  
                                                            
                                                             
                                                                   
                                                                       
              
   
typedef void mpo_sysvmsg_label_recycle_t(
	struct label *label
	);
   
                                                              
                                  
                             
                                       
                                   
                                             
  
                                                                          
                                             
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_enqueue_t(
	kauth_cred_t cred,
	struct msg *msgptr,
	struct label *msglabel,
	struct msqid_kernel *msqptr,
	struct label *msqlabel
	);
   
                                                              
                                                        
                             
                                       
  
                                                                          
                      
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msgrcv_t(
	kauth_cred_t cred,
	struct msg *msgptr,
	struct label *msglabel
	);
   
                                                                  
                                            
                             
                                       
  
                                                                       
                                                                         
                                                                          
                
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msgrmid_t(
	kauth_cred_t cred,
	struct msg *msgptr,
	struct label *msglabel
	);
   
                                            
                                            
                                   
                                             
  
                                                                 
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msqctl_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel,
	int cmd
	);
   
                                                               
                                            
                                             
                                             
  
                                                                 
                                                                     
                                       
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msqget_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel
	);
   
                                                                                  
                                            
                                                   
                                             
  
                                                                           
                                                 
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msqrcv_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel
	);
   
                                                                             
                                            
                                              
                                             
  
                                                                           
                                              
  
                                                                             
                             
   
typedef int mpo_sysvmsq_check_msqsnd_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel
	);
   
                                                
                                  
                                   
                                                  
  
   
typedef void mpo_sysvmsq_label_associate_t(
	kauth_cred_t cred,
	struct msqid_kernel *msqptr,
	struct label *msqlabel
	);
   
                                               
                                          
  
                                                                
                                                                       
                                                          
   
typedef void mpo_sysvmsq_label_destroy_t(
	struct label *label
	);
   
                                                  
                                        
  
                                                                         
   
typedef void mpo_sysvmsq_label_init_t(
	struct label *label
	);
   
                                                  
                                          
  
                                                                  
                                                                   
                                                                         
                                                                       
              
   
typedef void mpo_sysvmsq_label_recycle_t(
	struct label *label
	);
   
                                                                        
                                  
                                                   
                                                     
                                                               
  
                                                                          
                                                                      
  
                                                                             
                             
   
typedef int mpo_sysvsem_check_semctl_t(
	kauth_cred_t cred,
	struct semid_kernel *semakptr,
	struct label *semaklabel,
	int cmd
	);
   
                                                                  
                                  
                                                   
                                                           
  
                                                                  
                                
  
                                                                             
                             
   
typedef int mpo_sysvsem_check_semget_t(
	kauth_cred_t cred,
	struct semid_kernel *semakptr,
	struct label *semaklabel
	);
   
                                                                 
                                  
                                                   
                                                         
                                                                  
  
                                                                  
                                                                 
                                                                       
                                                                  
                                                                  
               
  
                                                                             
                             
   
typedef int mpo_sysvsem_check_semop_t(
	kauth_cred_t cred,
	struct semid_kernel *semakptr,
	struct label *semaklabel,
	size_t accesstype
	);
   
                                            
                                  
                                               
                                                              
  
                                                             
                                                                    
                                                                        
                        
   
typedef void mpo_sysvsem_label_associate_t(
	kauth_cred_t cred,
	struct semid_kernel *semakptr,
	struct label *semalabel
	);
   
                                           
                                          
  
                                                            
                                                                       
                                                          
   
typedef void mpo_sysvsem_label_destroy_t(
	struct label *label
	);
   
                                              
                                        
  
                                                                               
                 
   
typedef void mpo_sysvsem_label_init_t(
	struct label *label
	);
   
                                              
                                        
  
                                                              
                                                               
                                                                     
                                                                       
              
   
typedef void mpo_sysvsem_label_recycle_t(
	struct label *label
	);
   
                                                                  
                                  
                                                                
                                                                      
                                           
  
                                                                      
                                                                 
  
                                                                             
                             
   
typedef int mpo_sysvshm_check_shmat_t(
	kauth_cred_t cred,
	struct shmid_kernel *shmsegptr,
	struct label *shmseglabel,
	int shmflg
	);
   
                                                                            
                                  
                                                                
                                                                      
                                                               
  
                                                                          
                                                                        
              
  
                                                                             
                             
   
typedef int mpo_sysvshm_check_shmctl_t(
	kauth_cred_t cred,
	struct shmid_kernel *shmsegptr,
	struct label *shmseglabel,
	int cmd
	);
   
                                                                    
                                  
                                                                
                                                                      
  
                                                                        
                                                                 
  
                                                                             
                             
   
typedef int mpo_sysvshm_check_shmdt_t(
	kauth_cred_t cred,
	struct shmid_kernel *shmsegptr,
	struct label *shmseglabel
	);
   
                                                                           
                                  
                                                                
                                                                      
                                             
  
                                                                      
                                               
  
                                                                             
                             
   
typedef int mpo_sysvshm_check_shmget_t(
	kauth_cred_t cred,
	struct shmid_kernel *shmsegptr,
	struct label *shmseglabel,
	int shmflg
	);
   
                                                       
                                  
                                                           
                                                                        
  
                                                                        
                                                                      
                                                                      
                                  
   
typedef void mpo_sysvshm_label_associate_t(
	kauth_cred_t cred,
	struct shmid_kernel *shmsegptr,
	struct label *shmlabel
	);
   
                                               
                                          
  
                                                             
                                                                
                                                                
              
   
typedef void mpo_sysvshm_label_destroy_t(
	struct label *label
	);
   
                                                         
                                        
  
                                                                        
                                   
   
typedef void mpo_sysvshm_label_init_t(
	struct label *label
	);
   
                                                        
                                           
  
                                                           
                                                                    
                                                                   
                                                                      
                                     
   
typedef void mpo_sysvshm_label_recycle_t(
	struct label *shmlabel
	);
   
                                                                 
                                  
                                                  
  
                                                                      
                                        
                                                      
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_get_task_name_t(
	kauth_cred_t cred,
	struct proc_ident *pident
	);
   
                                                                 
                                  
                                                  
  
                                                                      
                                           
                                                 
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_get_task_t(
	kauth_cred_t cred,
	struct proc_ident *pident
	);

   
                                                                                       
                                  
                                                  
                                            
  
                                                                      
                                                   
                                                                      
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_get_task_with_flavor_t(
	kauth_cred_t cred,
	struct proc_ident *pident,
	mach_task_flavor_t flavor
	);

   
                                                                  
                                  
                                                  
  
                                                                         
                                           
                                                                         
                            
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_expose_task_t(
	kauth_cred_t cred,
	struct proc_ident *pident
	);

   
                                                                                        
                                  
                                                  
                                            
  
                                                                         
                                                   
                                                                         
                            
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_expose_task_with_flavor_t(
	kauth_cred_t cred,
	struct proc_ident *pident,
	mach_task_flavor_t flavor
	);

   
                                                                                     
                                  
                                                  
                                            
  
                                                                                       
                                                                     
                                                             
  
                                                                             
                                                                           
                                                                           
   
typedef int mpo_proc_check_task_id_token_get_task_t(
	kauth_cred_t cred,
	struct proc_ident *pident,
	mach_task_flavor_t flavor
	);

   
                                                                   
                                     
                                                   
                                                                   
                                                      
                                                                      
                                                        
                                          
                                                            
                                              
   
typedef int mpo_proc_check_inherit_ipc_ports_t(
	struct proc *p,
	struct vnode *cur_vp,
	off_t cur_offset,
	struct vnode *img_vp,
	off_t img_offset,
	struct vnode *scriptvp
	);

   
                                                       
                           
  
                                                                               
                                                           
  
                                                                             
                             
   
typedef int mpo_proc_check_run_cs_invalid_t(
	struct proc *p
	);

   
                                                
                          
  
                                                                            
                                                                         
                                                                           
                                                          
  
                                                                              
                                                                                 
                                                                               
                                                                                
                                                                            
                             
   
typedef void mpo_proc_notify_cs_invalidated_t(
	struct proc *p
	);

   
                                                                                  
                           
  
                                                                                  
                                                                                   
                                                                                    
                      
   
typedef void mpo_proc_notify_exec_complete_t(
	struct proc *p
	);

   
                                                                         
                                                         
  
                                                                  
                                                                 
                          
   
typedef void mpo_thread_userret_t(
	struct thread *thread
	);

   
                             
                                  
                          
                             
                                   
  
                                                                   
                                                                     
                                                                  
                                                             
                         
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_access_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	int acc_mode
	);
   
                                                              
                                  
                                             
                                      
  
                                                                         
                                                      
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_chdir_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel
	);
   
                                                           
                                  
                              
                                                  
                                     
  
                                                                        
                                                            
  
                                                                    
                                                        
   
typedef int mpo_vnode_check_chroot_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp
	);
   
                                                  
                                  
                                                        
                                                  
                                             
                                                
                                                         
  
                                                                        
                                                                             
  
                                                                             
                             
   
typedef int mpo_vnode_check_clone_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *label,
	struct componentname *cnp
	);
   
                                                  
                                  
                              
                                      
                                     
                                       
  
                                                                         
                                                                      
                                                                          
                                                                       
                                    
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_create_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp,
	struct vnode_attr *vap
	);
   
                                                               
                                  
                          
                                          
                                       
  
                                                                         
                                                 
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_deleteextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	const char *name
	);
   
                                                        
                                  
                             
                                  
                             
                                  
  
                                                                                
                               
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_exchangedata_t(
	kauth_cred_t cred,
	struct vnode *v1,
	struct label *vl1,
	struct vnode *v2,
	struct label *vl2
	);
   
                                                       
                                  
                                     
                                                                 
                                               
                                         
                                                       
                                                     
                                                                  
                                                                           
  
                                                                          
                                                                           
                                                               
  
                                                                    
                                                                         
                                                                       
                                                                  
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_exec_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct vnode *scriptvp,
	struct label *vnodelabel,
	struct label *scriptlabel,
	struct label *execlabel,                    
	struct componentname *cnp,
	u_int *csflags,
	void *macpolicyattr,
	size_t macpolicyattrlen
	);
   
                                             
                                  
                                                     
                                                
  
                                                                               
                                      
  
                                                                             
                             
   
typedef int mpo_vnode_check_fsgetpath_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label
	);
   
                                                              
                                         
                                                                   
                          
                                     
                                          
  
                                                                  
                                                                 
                                                                
                                                                  
                                                               
                                                               
                                  
  
                                                                             
                             
  
                                                                     
                             
   
typedef int mpo_vnode_check_getattr_t(
	kauth_cred_t active_cred,
	kauth_cred_t file_cred,             
	struct vnode *vp,
	struct label *vlabel,
	struct vnode_attr *va
	);
   
                                                              
                                  
                          
                                     
                                               
                                         
  
                                                                       
                                                                             
                                                                     
                         
  
                                                                             
                                                                             
                                                                               
                                                                             
                         
   
typedef int mpo_vnode_check_getattrlist_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	struct attrlist *alist,
	uint64_t options
	);
   
                                                                                             
                                  
                              
                                               
                                         
  
                                                                       
                                                                             
                                                                     
                         
  
                                                                             
                                                                             
                                                                               
                                                                             
                         
   
typedef int mpo_vnode_check_getattrlistbulk_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct attrlist *alist,
	uint64_t options
	);
   
                                                                    
                                  
                          
                                    
                                       
                                    
  
                                                                           
                                                                    
                                                                         
                                                             
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_getextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,                        
	const char *name,
	struct uio *uio                             
	);
   
                                         
                                  
                          
                                    
                                                          
  
                                                                          
                                         
  
                                                                      
                                                                 
                                                                    
                          
  
                                                                             
                             
   
typedef int mpo_vnode_check_ioctl_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	unsigned long cmd
	);
   
                                                  
                                         
                          
                          
                                    
  
                                                                  
                                          
  
                                                                   
                                       
   
typedef int mpo_vnode_check_kqfilter_t(
	kauth_cred_t active_cred,
	kauth_cred_t file_cred,                     
	struct knote *kn,
	struct vnode *vp,
	struct label *label
	);
   
                                           
                                  
                          
                                                  
                                                                 
                            
  
                                                                          
                                                                        
                                                                             
                
  
                                                                             
                             
   
typedef int mpo_vnode_check_label_update_t(
	struct ucred *cred,
	struct vnode *vp,
	struct label *vnodelabel,
	struct label *newlabel
	);
   
                                                 
                                  
                              
                                                  
                                    
                                                
                                                        
  
                                                                        
                                                                            
  
                                                                             
                             
   
typedef int mpo_vnode_check_link_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *label,
	struct componentname *cnp
	);
   
                                                               
                                  
                          
                                                 
  
                                                                           
                                                     
  
                                                                             
                             
   
typedef int mpo_vnode_check_listextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                          
                                  
                              
                                      
                                    
                                          
  
                                                                          
                                                                       
  
                                                                             
                                                                             
                                
  
                                                                             
                                                                          
                                       
   
typedef int mpo_vnode_check_lookup_preflight_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	const char *path,
	size_t pathlen
	);
   
                                          
                                  
                           
                                      
                                             
  
                                                                          
                                                                     
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_lookup_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp
	);
   
                                        
                                  
                          
                                                
                                       
  
                                                                          
                                                                      
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_open_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	int acc_mode
	);
   
                                        
                                         
                                                                   
                          
                                    
  
                                                                          
                                                                               
                                                                    
                                                               
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_read_t(
	kauth_cred_t active_cred,                    
	kauth_cred_t file_cred,             
	struct vnode *vp,                           
	struct label *label                        
	);
   
                                                  
                                  
                                     
                                      
  
                                                                  
                                                              
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_readdir_t(
	kauth_cred_t cred,                           
	struct vnode *dvp,                          
	struct label *dlabel                       
	);
   
                                             
                                  
                          
                                    
  
                                                                          
                                                                            
                                                                         
                                                                      
                          
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_readlink_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label
	);
   
                                          
                                  
                              
                                                  
                                 
                                                
                                    
                                           
                                                    
                                
                                                  
                                          
  
                                                                                
                                             
  
                                                                             
                             
   
typedef int mpo_vnode_check_rename_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *label,
	struct componentname *cnp,
	struct vnode *tdvp,
	struct label *tdlabel,
	struct vnode *tvp,
	struct label *tlabel,
	struct componentname *tcnp
	);
   
                                               
                                  
                              
                                                  
                                 
                                                
                                    
                                 
                                    
  
                                                                        
                                                     
  
                                                                       
                                                                     
                                                      
  
                                                                             
  
                                                                             
                             
   
typedef int mpo_vnode_check_rename_from_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *label,
	struct componentname *cnp
	);
   
                                             
                                  
                              
                                                  
                               
                                                
                                                                                    
                                         
                                 
                                      
  
                                                                        
                                                                        
                                                                       
                              
  
                                                                       
                                                                     
                                                      
  
                                                                             
  
                                                                             
                             
   
typedef int mpo_vnode_check_rename_to_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,                                   
	struct label *label,                                
	int samedir,
	struct componentname *cnp
	);
   
                                          
                                  
                          
                                    
  
                                                                         
                               
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_revoke_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label
	);
   
                                            
                                  
                          
                                     
                                                        
                                                                 
  
                                                                             
                                         
  
                                                                             
                             
   
typedef int mpo_vnode_check_searchfs_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	struct attrlist *returnattrs,
	struct attrlist *searchattrs
	);
   
                                          
                                  
                          
                                    
                                                           
  
                                                                         
              
  
                                                                             
                             
   
typedef int mpo_vnode_check_select_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	int which
	);
   
                                               
                                  
                         
                                    
                                    
  
                                                                             
                                                                               
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setacl_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct kauth_acl *acl
	);
   
                                                           
                                  
                          
                                     
                                          
  
                                                                      
                                                                             
                                                                     
               
  
                                                                             
                                                                             
                                                                               
                   
   
typedef int mpo_vnode_check_setattrlist_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	struct attrlist *alist
	);
   
                                                              
                                  
                          
                                    
                                       
                                    
  
                                                                          
                                                                        
                                                                     
                                                                   
                                                                          
                                                                           
                                                                         
                                             
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	const char *name,
	struct uio *uio
	);
   
                                                 
                                  
                          
                                    
                                           
  
                                                                      
                                         
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setflags_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	u_long flags
	);
   
                                                
                                  
                          
                                    
                                       
  
                                                                      
                                        
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setmode_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	mode_t mode
	);
   
                                                       
                                  
                          
                                    
                      
                       
  
                                                                      
                                                                        
                                                           
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setowner_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	uid_t uid,
	gid_t gid
	);
   
                                                      
                                  
                          
                                    
                                           
                                                 
  
                                                                      
                                                     
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_setutimes_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct timespec atime,
	struct timespec mtime
	);
   
                                                                         
                                              
                                                
                                                           
                                              
                                                       
                                                                            
                                                              
                                                           
                                                          
                                                                                    
  
                                                                             
                             
   
typedef int mpo_vnode_check_signature_t(
	struct vnode *vp,
	struct label *label,
	cpu_type_t cpu_type,
	struct cs_blob *cs_blob,
	unsigned int *cs_flags,
	unsigned int *signer_type,
	int flags,
	unsigned int platform,
	char **fatal_failure_desc, size_t *fatal_failure_desc_len
	);

   
                                                                      
                                                               
                                                
                                              
                                                          
                                                                
                                                                                                
  
                                                                             
                             
   
typedef int mpo_vnode_check_supplemental_signature_t(
	struct vnode *vp,
	struct label *label,
	struct cs_blob *cs_blob,
	struct vnode *linked_vp,
	struct cs_blob *linked_cs_blob,
	unsigned int *signer_type
	);

   
                                        
                                         
                                                                   
                          
                                    
  
                                                                       
                                                                        
                                                                     
                                                                  
                    
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_stat_t(
	struct ucred *active_cred,
	struct ucred *file_cred,                    
	struct vnode *vp,
	struct label *label
	);
   
                                                            
                                  
                           
                                      
                                                       
  
                                                                          
                                                                     
                                     
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_trigger_resolve_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp
	);
   
                                                      
                                         
                                                                   
                          
                                    
  
                                                                  
                                                                           
                                                                
                                                                  
                    
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_truncate_t(
	kauth_cred_t active_cred,
	kauth_cred_t file_cred,             
	struct vnode *vp,
	struct label *label
	);
   
                                                              
                                  
                              
                                      
                                     
                                       
  
                                                                            
                                                                            
                                                              
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_uipc_bind_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp,
	struct vnode_attr *vap
	);
   
                                                                 
                                  
                          
                                                
                    
  
                                                                            
                                                             
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_uipc_connect_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	socket_t so
	);
   
                                                  
                                  
                                     
                                      
                                    
                                    
                                    
                              
  
                                                                         
                                                                         
                                                                   
                                                                     
                                                                 
                                                                              
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_unlink_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *label,
	struct componentname *cnp
	);
   
                                         
                                         
                                                                   
                          
                                    
  
                                                                  
                                                                        
                                                                
                                                                  
                    
  
                                                                             
                                                                             
                                
   
typedef int mpo_vnode_check_write_t(
	kauth_cred_t active_cred,
	kauth_cred_t file_cred,             
	struct vnode *vp,
	struct label *label
	);
   
                                               
                               
                                           
                                   
                                           
                                      
                                          
  
                                                                    
                                                                    
                                                                 
   
typedef void mpo_vnode_label_associate_devfs_t(
	struct mount *mp,
	struct label *mntlabel,
	struct devnode *de,
	struct label *delabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                         
                                     
                                                 
                            
                                          
  
                                                                     
                                                                        
                                                                  
                                                                       
                 
  
                                                                    
                                                                        
                                                                    
                                                                       
                                                                  
                       
  
                                                                    
                                                        
   
typedef int mpo_vnode_label_associate_extattr_t(
	struct mount *mp,
	struct label *mntlabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                              
                               
                               
                                           
                                
                                    
                            
                                          
  
                                                                    
                                             
                                                                
   
typedef void mpo_vnode_label_associate_file_t(
	struct ucred *cred,
	struct mount *mp,
	struct label *mntlabel,
	struct fileglob *fg,
	struct label *label,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                              
                                                                    
                               
                                               
                            
                                          
  
                                                                    
                                                   
                                                                
   
typedef void mpo_vnode_label_associate_pipe_t(
	struct ucred *cred,
	struct pipe *cpipe,
	struct label *pipelabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                                         
                                                                
                                         
                                               
                            
                                          
  
                                                                    
                                          
                                                                
   
typedef void mpo_vnode_label_associate_posixsem_t(
	struct ucred *cred,
	struct pseminfo *psem,
	struct label *psemlabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                                             
                                                                 
                                             
                                               
                            
                                          
  
                                                                    
                                                     
                                                                
   
typedef void mpo_vnode_label_associate_posixshm_t(
	struct ucred *cred,
	struct pshminfo *pshm,
	struct label *pshmlabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                         
                                     
                                                 
                            
                                          
  
                                                                   
                                                             
   
typedef void mpo_vnode_label_associate_singlelabel_t(
	struct mount *mp,
	struct label *mntlabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                                                
                                                                      
                              
                                           
                            
                                          
  
                                                                    
                                                         
                                                                
   
typedef void mpo_vnode_label_associate_socket_t(
	kauth_cred_t cred,
	socket_t so,
	struct label *solabel,
	struct vnode *vp,
	struct label *vlabel
	);
   
                             
                                 
                                       
  
                                                                       
                                                                       
                                                                  
                     
   
typedef void mpo_vnode_label_copy_t(
	struct label *src,
	struct label *dest
	);
   
                              
                                          
  
                                                                   
                                                                       
                                      
   
typedef void mpo_vnode_label_destroy_t(
	struct label *label
	);
   
                                                 
                                         
                                                                              
                
                                                                                
  
                                                                           
                                                                           
                                                                               
                                                                           
                              
  
                                                                  
                                 
  
   
typedef int mpo_vnode_label_externalize_audit_t(
	struct label *label,
	char *element_name,
	struct sbuf *sb
	);
   
                                    
                                         
                                                                              
                
                                                                                
  
                                                                   
                                                                     
                                                                      
                                                            
  
                                                                  
                                 
  
   
typedef int mpo_vnode_label_externalize_t(
	struct label *label,
	char *element_name,
	struct sbuf *sb
	);
   
                                 
                                        
  
                                                                        
                                                                    
                                                               
                                                                           
                                                                          
                          
   
typedef void mpo_vnode_label_init_t(
	struct label *label
	);
   
                                    
                                         
                                                                              
                   
                                                    
  
                                                              
                                                                     
                                                                      
                                                                
                                            
  
                                                                    
                                                          
  
                                                                       
                                       
   
typedef int mpo_vnode_label_internalize_t(
	struct label *label,
	char *element_name,
	char *element_data
	);
   
                                 
                                                  
  
                                                                                
                                                                                
                                                                                   
                                                      
   
typedef void mpo_vnode_label_recycle_t(
	struct label *label
	);
   
                                                
                                  
                                                           
                                          
                                          
  
                                                                    
                                                                     
                                                                
              
  
                                                                    
                                                        
  
                                                                         
                                                            
   
typedef int mpo_vnode_label_store_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vlabel,
	struct label *intlabel
	);
   
                                                      
                                     
                                     
                            
                                          
                                 
                                     
  
                                                                             
                                                                
                                                                       
                                                                       
                                                                    
                                                                           
                                                                           
                                                                      
                                                     
  
                                                                      
                                                                      
                                                          
  
                                                                   
                                                                        
                     
   
typedef int mpo_vnode_label_update_extattr_t(
	struct mount *mp,
	struct label *mntlabel,
	struct vnode *vp,
	struct label *vlabel,
	const char *name
	);
   
                               
                                  
                                  
                                          
                                                    
                                       
  
                                                                     
                                                                    
                                                                      
                                                                    
   
typedef void mpo_vnode_label_update_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *vnodelabel,
	struct label *label
	);
   
                                                        
                                              
                                  
                                                                                            
                                     
  
   
typedef int mpo_vnode_find_sigs_t(
	struct proc *p,
	struct vnode *vp,
	off_t offset,
	struct label *label
	);
   
                                                            
                                                        
                                     
                                                 
                                     
                                              
                                 
                                                       
                                    
  
                                                                        
                                                                        
                                                                   
                                                                       
                                                             
  
                                                                        
                                                            
   
typedef int mpo_vnode_notify_create_t(
	kauth_cred_t cred,
	struct mount *mp,
	struct label *mntlabel,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *vlabel,
	struct componentname *cnp
	);

   
                                                           
                                                        
                          
                                        
                                            
  
                                                                  
                                      
   
typedef void mpo_vnode_notify_open_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	int acc_mode
	);

   
                                                            
                                                        
                                        
                                    
                                                   
                                      
                                                 
  
                                                      
   
typedef void mpo_vnode_notify_rename_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct vnode *dvp,
	struct label *dlabel,
	struct componentname *cnp
	);

   
                                                           
                                                        
                                                   
                                      
                                       
                                     
                                                 
  
                                                     
   
typedef void mpo_vnode_notify_link_t(
	kauth_cred_t cred,
	struct vnode *dvp,
	struct label *dlabel,
	struct vnode *vp,
	struct label *vlabel,
	struct componentname *cnp
	);

   
                                                                                       
                                  
                         
                                    
                                       
  
                                                                                 
   
typedef void mpo_vnode_notify_deleteextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	const char *name
	);


   
                                                                  
                                  
                         
                                    
                                    
  
                                                            
   
typedef void mpo_vnode_notify_setacl_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct kauth_acl *acl
	);

   
                                                                          
                                  
                          
                                    
                                          
  
                                                                    
   
typedef void mpo_vnode_notify_setattrlist_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct attrlist *alist
	);

   
                                                                                 
                                  
                          
                                    
                                       
                                    
  
                                                                           
   
typedef void mpo_vnode_notify_setextattr_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	const char *name,
	struct uio *uio
	);

   
                                                                  
                                  
                          
                                    
                                           
  
                                                            
   
typedef void mpo_vnode_notify_setflags_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	u_long flags
	);

   
                                                                      
                                  
                          
                                    
                                       
  
                                                                
   
typedef void mpo_vnode_notify_setmode_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	mode_t mode
	);

   
                                                                        
                                  
                          
                                    
                      
                       
  
                                                                  
   
typedef void mpo_vnode_notify_setowner_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	uid_t uid,
	gid_t gid
	);

   
                                                                           
                                  
                          
                                    
                                           
                                                 
  
                                                                     
   
typedef void mpo_vnode_notify_setutimes_t(
	kauth_cred_t cred,
	struct vnode *vp,
	struct label *label,
	struct timespec atime,
	struct timespec mtime
	);

   
                                                              
                                  
                                                                   
                          
                                    
  
                                                        
   
typedef void mpo_vnode_notify_truncate_t(
	kauth_cred_t cred,
	kauth_cred_t file_cred,
	struct vnode *vp,
	struct label *label
	);


   
                                                                
                                
                                
                                                
                                    
  
                                                          
   
typedef void mpo_pty_notify_grant_t(
	proc_t p,
	struct tty *tp,
	dev_t dev,
	struct label *label
	);

   
                                                                
                                
                                
                                                
                                    
  
                                                          
   
typedef void mpo_pty_notify_close_t(
	proc_t p,
	struct tty *tp,
	dev_t dev,
	struct label *label
	);

   
                                                
                                  
                                     
  
                                                                           
                   
  
                                                                             
                                                                             
   
typedef int mpo_kext_check_load_t(
	kauth_cred_t cred,
	const char *identifier
	);

   
                                                  
                                  
                                     
  
                                                                             
                   
  
                                                                             
                                                                             
   
typedef int mpo_kext_check_unload_t(
	kauth_cred_t cred,
	const char *identifier
	);

   
                                                                           
                                  
  
                                                                        
                                   
  
                                                                             
                                                                              
   
typedef int mpo_kext_check_query_t(
	kauth_cred_t cred
	);

   
                                                              
                          
  
                                                                                       
   
typedef void mpo_vnode_notify_reclaim_t(
	struct vnode *vp
	);

  
                                                         
   
typedef void mpo_reserved_hook_t(void);

  
                            
  
                                                                          
                                                         
   
#define MAC_POLICY_OPS_VERSION 75                                            
struct mac_policy_ops {
	mpo_audit_check_postselect_t            *mpo_audit_check_postselect;
	mpo_audit_check_preselect_t             *mpo_audit_check_preselect;

	mpo_reserved_hook_t                     *mpo_reserved01;
	mpo_reserved_hook_t                     *mpo_reserved02;
	mpo_reserved_hook_t                     *mpo_reserved03;
	mpo_reserved_hook_t                     *mpo_reserved04;

	mpo_cred_check_label_update_execve_t    *mpo_cred_check_label_update_execve;
	mpo_cred_check_label_update_t           *mpo_cred_check_label_update;
	mpo_cred_check_visible_t                *mpo_cred_check_visible;
	mpo_cred_label_associate_fork_t         *mpo_cred_label_associate_fork;
	mpo_cred_label_associate_kernel_t       *mpo_cred_label_associate_kernel;
	mpo_cred_label_associate_t              *mpo_cred_label_associate;
	mpo_cred_label_associate_user_t         *mpo_cred_label_associate_user;
	mpo_cred_label_destroy_t                *mpo_cred_label_destroy;
	mpo_cred_label_externalize_audit_t      *mpo_cred_label_externalize_audit;
	mpo_cred_label_externalize_t            *mpo_cred_label_externalize;
	mpo_cred_label_init_t                   *mpo_cred_label_init;
	mpo_cred_label_internalize_t            *mpo_cred_label_internalize;
	mpo_cred_label_update_execve_t          *mpo_cred_label_update_execve;
	mpo_cred_label_update_t                 *mpo_cred_label_update;

	mpo_devfs_label_associate_device_t      *mpo_devfs_label_associate_device;
	mpo_devfs_label_associate_directory_t   *mpo_devfs_label_associate_directory;
	mpo_devfs_label_copy_t                  *mpo_devfs_label_copy;
	mpo_devfs_label_destroy_t               *mpo_devfs_label_destroy;
	mpo_devfs_label_init_t                  *mpo_devfs_label_init;
	mpo_devfs_label_update_t                *mpo_devfs_label_update;

	mpo_file_check_change_offset_t          *mpo_file_check_change_offset;
	mpo_file_check_create_t                 *mpo_file_check_create;
	mpo_file_check_dup_t                    *mpo_file_check_dup;
	mpo_file_check_fcntl_t                  *mpo_file_check_fcntl;
	mpo_file_check_get_offset_t             *mpo_file_check_get_offset;
	mpo_file_check_get_t                    *mpo_file_check_get;
	mpo_file_check_inherit_t                *mpo_file_check_inherit;
	mpo_file_check_ioctl_t                  *mpo_file_check_ioctl;
	mpo_file_check_lock_t                   *mpo_file_check_lock;
	mpo_file_check_mmap_downgrade_t         *mpo_file_check_mmap_downgrade;
	mpo_file_check_mmap_t                   *mpo_file_check_mmap;
	mpo_file_check_receive_t                *mpo_file_check_receive;
	mpo_file_check_set_t                    *mpo_file_check_set;
	mpo_file_label_init_t                   *mpo_file_label_init;
	mpo_file_label_destroy_t                *mpo_file_label_destroy;
	mpo_file_label_associate_t              *mpo_file_label_associate;
	mpo_file_notify_close_t                 *mpo_file_notify_close;

	mpo_reserved_hook_t                     *mpo_reserved06;
	mpo_reserved_hook_t                     *mpo_reserved07;
	mpo_reserved_hook_t                     *mpo_reserved08;
	mpo_reserved_hook_t                     *mpo_reserved09;
	mpo_reserved_hook_t                     *mpo_reserved10;
	mpo_reserved_hook_t                     *mpo_reserved11;
	mpo_reserved_hook_t                     *mpo_reserved12;
	mpo_reserved_hook_t                     *mpo_reserved13;
	mpo_reserved_hook_t                     *mpo_reserved14;
	mpo_reserved_hook_t                     *mpo_reserved15;
	mpo_reserved_hook_t                     *mpo_reserved16;
	mpo_reserved_hook_t                     *mpo_reserved17;
	mpo_reserved_hook_t                     *mpo_reserved18;
	mpo_reserved_hook_t                     *mpo_reserved19;
	mpo_reserved_hook_t                     *mpo_reserved20;
	mpo_reserved_hook_t                     *mpo_reserved21;
	mpo_reserved_hook_t                     *mpo_reserved22;
	mpo_reserved_hook_t                     *mpo_reserved23;
	mpo_reserved_hook_t                     *mpo_reserved24;

	mpo_necp_check_open_t                   *mpo_necp_check_open;
	mpo_necp_check_client_action_t          *mpo_necp_check_client_action;

	mpo_file_check_library_validation_t     *mpo_file_check_library_validation;

	mpo_vnode_notify_setacl_t               *mpo_vnode_notify_setacl;
	mpo_vnode_notify_setattrlist_t          *mpo_vnode_notify_setattrlist;
	mpo_vnode_notify_setextattr_t           *mpo_vnode_notify_setextattr;
	mpo_vnode_notify_setflags_t             *mpo_vnode_notify_setflags;
	mpo_vnode_notify_setmode_t              *mpo_vnode_notify_setmode;
	mpo_vnode_notify_setowner_t             *mpo_vnode_notify_setowner;
	mpo_vnode_notify_setutimes_t            *mpo_vnode_notify_setutimes;
	mpo_vnode_notify_truncate_t             *mpo_vnode_notify_truncate;
	mpo_vnode_check_getattrlistbulk_t       *mpo_vnode_check_getattrlistbulk;

	mpo_reserved_hook_t                     *mpo_reserved28;
	mpo_reserved_hook_t                     *mpo_reserved29;
	mpo_reserved_hook_t                     *mpo_reserved30;
	mpo_reserved_hook_t                     *mpo_reserved31;
	mpo_reserved_hook_t                     *mpo_reserved32;
	mpo_reserved_hook_t                     *mpo_reserved33;
	mpo_reserved_hook_t                     *mpo_reserved34;
	mpo_reserved_hook_t                     *mpo_reserved35;
	mpo_reserved_hook_t                     *mpo_reserved36;

	mpo_mount_check_quotactl_t              *mpo_mount_check_quotactl;
	mpo_mount_check_fsctl_t                 *mpo_mount_check_fsctl;
	mpo_mount_check_getattr_t               *mpo_mount_check_getattr;
	mpo_mount_check_label_update_t          *mpo_mount_check_label_update;
	mpo_mount_check_mount_t                 *mpo_mount_check_mount;
	mpo_mount_check_remount_t               *mpo_mount_check_remount;
	mpo_mount_check_setattr_t               *mpo_mount_check_setattr;
	mpo_mount_check_stat_t                  *mpo_mount_check_stat;
	mpo_mount_check_umount_t                *mpo_mount_check_umount;
	mpo_mount_label_associate_t             *mpo_mount_label_associate;
	mpo_mount_label_destroy_t               *mpo_mount_label_destroy;
	mpo_mount_label_externalize_t           *mpo_mount_label_externalize;
	mpo_mount_label_init_t                  *mpo_mount_label_init;
	mpo_mount_label_internalize_t           *mpo_mount_label_internalize;

	mpo_proc_check_expose_task_with_flavor_t *mpo_proc_check_expose_task_with_flavor;
	mpo_proc_check_get_task_with_flavor_t   *mpo_proc_check_get_task_with_flavor;
	mpo_proc_check_task_id_token_get_task_t *mpo_proc_check_task_id_token_get_task;

	mpo_pipe_check_ioctl_t                  *mpo_pipe_check_ioctl;
	mpo_pipe_check_kqfilter_t               *mpo_pipe_check_kqfilter;
	mpo_reserved_hook_t                     *mpo_reserved41;
	mpo_pipe_check_read_t                   *mpo_pipe_check_read;
	mpo_pipe_check_select_t                 *mpo_pipe_check_select;
	mpo_pipe_check_stat_t                   *mpo_pipe_check_stat;
	mpo_pipe_check_write_t                  *mpo_pipe_check_write;
	mpo_pipe_label_associate_t              *mpo_pipe_label_associate;
	mpo_reserved_hook_t                     *mpo_reserved42;
	mpo_pipe_label_destroy_t                *mpo_pipe_label_destroy;
	mpo_reserved_hook_t                     *mpo_reserved43;
	mpo_pipe_label_init_t                   *mpo_pipe_label_init;
	mpo_reserved_hook_t                     *mpo_reserved44;
	mpo_reserved_hook_t                     *mpo_reserved45;

	mpo_policy_destroy_t                    *mpo_policy_destroy;
	mpo_policy_init_t                       *mpo_policy_init;
	mpo_policy_initbsd_t                    *mpo_policy_initbsd;
	mpo_policy_syscall_t                    *mpo_policy_syscall;

	mpo_system_check_sysctlbyname_t         *mpo_system_check_sysctlbyname;
	mpo_proc_check_inherit_ipc_ports_t      *mpo_proc_check_inherit_ipc_ports;
	mpo_vnode_check_rename_t                *mpo_vnode_check_rename;
	mpo_kext_check_query_t                  *mpo_kext_check_query;
	mpo_proc_notify_exec_complete_t         *mpo_proc_notify_exec_complete;
	mpo_proc_notify_cs_invalidated_t        *mpo_proc_notify_cs_invalidated;
	mpo_proc_check_syscall_unix_t           *mpo_proc_check_syscall_unix;
	mpo_proc_check_expose_task_t            *mpo_proc_check_expose_task;                                                                                 
	mpo_proc_check_set_host_special_port_t  *mpo_proc_check_set_host_special_port;
	mpo_proc_check_set_host_exception_port_t *mpo_proc_check_set_host_exception_port;
	mpo_exc_action_check_exception_send_t   *mpo_exc_action_check_exception_send;
	mpo_exc_action_label_associate_t        *mpo_exc_action_label_associate;
	mpo_exc_action_label_populate_t         *mpo_exc_action_label_populate;
	mpo_exc_action_label_destroy_t          *mpo_exc_action_label_destroy;
	mpo_exc_action_label_init_t             *mpo_exc_action_label_init;
	mpo_exc_action_label_update_t           *mpo_exc_action_label_update;

	mpo_vnode_check_trigger_resolve_t       *mpo_vnode_check_trigger_resolve;
	mpo_mount_check_mount_late_t            *mpo_mount_check_mount_late;
	mpo_mount_check_snapshot_mount_t        *mpo_mount_check_snapshot_mount;
	mpo_vnode_notify_reclaim_t              *mpo_vnode_notify_reclaim;
	mpo_skywalk_flow_check_connect_t        *mpo_skywalk_flow_check_connect;
	mpo_skywalk_flow_check_listen_t         *mpo_skywalk_flow_check_listen;

	mpo_posixsem_check_create_t             *mpo_posixsem_check_create;
	mpo_posixsem_check_open_t               *mpo_posixsem_check_open;
	mpo_posixsem_check_post_t               *mpo_posixsem_check_post;
	mpo_posixsem_check_unlink_t             *mpo_posixsem_check_unlink;
	mpo_posixsem_check_wait_t               *mpo_posixsem_check_wait;
	mpo_posixsem_label_associate_t          *mpo_posixsem_label_associate;
	mpo_posixsem_label_destroy_t            *mpo_posixsem_label_destroy;
	mpo_posixsem_label_init_t               *mpo_posixsem_label_init;
	mpo_posixshm_check_create_t             *mpo_posixshm_check_create;
	mpo_posixshm_check_mmap_t               *mpo_posixshm_check_mmap;
	mpo_posixshm_check_open_t               *mpo_posixshm_check_open;
	mpo_posixshm_check_stat_t               *mpo_posixshm_check_stat;
	mpo_posixshm_check_truncate_t           *mpo_posixshm_check_truncate;
	mpo_posixshm_check_unlink_t             *mpo_posixshm_check_unlink;
	mpo_posixshm_label_associate_t          *mpo_posixshm_label_associate;
	mpo_posixshm_label_destroy_t            *mpo_posixshm_label_destroy;
	mpo_posixshm_label_init_t               *mpo_posixshm_label_init;

	mpo_proc_check_debug_t                  *mpo_proc_check_debug;
	mpo_proc_check_fork_t                   *mpo_proc_check_fork;
	mpo_proc_check_get_task_name_t          *mpo_proc_check_get_task_name;                                                                   
	mpo_proc_check_get_task_t               *mpo_proc_check_get_task;                                                                        
	mpo_proc_check_getaudit_t               *mpo_proc_check_getaudit;
	mpo_proc_check_getauid_t                *mpo_proc_check_getauid;
	mpo_proc_check_getlcid_t                *mpo_proc_check_getlcid;
	mpo_proc_check_mprotect_t               *mpo_proc_check_mprotect;
	mpo_proc_check_sched_t                  *mpo_proc_check_sched;
	mpo_proc_check_setaudit_t               *mpo_proc_check_setaudit;
	mpo_proc_check_setauid_t                *mpo_proc_check_setauid;
	mpo_proc_check_setlcid_t                *mpo_proc_check_setlcid;
	mpo_proc_check_signal_t                 *mpo_proc_check_signal;
	mpo_proc_check_wait_t                   *mpo_proc_check_wait;
	mpo_proc_check_dump_core_t              *mpo_proc_check_dump_core;
	mpo_proc_check_remote_thread_create_t   *mpo_proc_check_remote_thread_create;

	mpo_socket_check_accept_t               *mpo_socket_check_accept;
	mpo_socket_check_accepted_t             *mpo_socket_check_accepted;
	mpo_socket_check_bind_t                 *mpo_socket_check_bind;
	mpo_socket_check_connect_t              *mpo_socket_check_connect;
	mpo_socket_check_create_t               *mpo_socket_check_create;
	mpo_reserved_hook_t                     *mpo_reserved46;
	mpo_reserved_hook_t                     *mpo_reserved47;
	mpo_reserved_hook_t                     *mpo_reserved48;
	mpo_socket_check_listen_t               *mpo_socket_check_listen;
	mpo_socket_check_receive_t              *mpo_socket_check_receive;
	mpo_socket_check_received_t             *mpo_socket_check_received;
	mpo_reserved_hook_t                     *mpo_reserved49;
	mpo_socket_check_send_t                 *mpo_socket_check_send;
	mpo_socket_check_stat_t                 *mpo_socket_check_stat;
	mpo_socket_check_setsockopt_t           *mpo_socket_check_setsockopt;
	mpo_socket_check_getsockopt_t           *mpo_socket_check_getsockopt;

	mpo_proc_check_get_movable_control_port_t *mpo_proc_check_get_movable_control_port;
	mpo_proc_check_dyld_process_info_notify_register_t *mpo_proc_check_dyld_process_info_notify_register;
	mpo_reserved_hook_t                     *mpo_reserved52;
	mpo_reserved_hook_t                     *mpo_reserved53;
	mpo_reserved_hook_t                     *mpo_reserved54;
	mpo_reserved_hook_t                     *mpo_reserved55;
	mpo_reserved_hook_t                     *mpo_reserved56;
	mpo_reserved_hook_t                     *mpo_reserved57;
	mpo_reserved_hook_t                     *mpo_reserved58;

	mpo_proc_check_memorystatus_control_t   *mpo_proc_check_memorystatus_control;
	mpo_proc_check_work_interval_ctl_t      *mpo_proc_check_work_interval_ctl;

	mpo_reserved_hook_t                     *mpo_reserved61;

	mpo_iokit_check_open_service_t          *mpo_iokit_check_open_service;

	mpo_system_check_acct_t                 *mpo_system_check_acct;
	mpo_system_check_audit_t                *mpo_system_check_audit;
	mpo_system_check_auditctl_t             *mpo_system_check_auditctl;
	mpo_system_check_auditon_t              *mpo_system_check_auditon;
	mpo_system_check_host_priv_t            *mpo_system_check_host_priv;
	mpo_system_check_nfsd_t                 *mpo_system_check_nfsd;
	mpo_system_check_reboot_t               *mpo_system_check_reboot;
	mpo_system_check_settime_t              *mpo_system_check_settime;
	mpo_system_check_swapoff_t              *mpo_system_check_swapoff;
	mpo_system_check_swapon_t               *mpo_system_check_swapon;
	mpo_socket_check_ioctl_t                *mpo_socket_check_ioctl;

	mpo_sysvmsg_label_associate_t           *mpo_sysvmsg_label_associate;
	mpo_sysvmsg_label_destroy_t             *mpo_sysvmsg_label_destroy;
	mpo_sysvmsg_label_init_t                *mpo_sysvmsg_label_init;
	mpo_sysvmsg_label_recycle_t             *mpo_sysvmsg_label_recycle;
	mpo_sysvmsq_check_enqueue_t             *mpo_sysvmsq_check_enqueue;
	mpo_sysvmsq_check_msgrcv_t              *mpo_sysvmsq_check_msgrcv;
	mpo_sysvmsq_check_msgrmid_t             *mpo_sysvmsq_check_msgrmid;
	mpo_sysvmsq_check_msqctl_t              *mpo_sysvmsq_check_msqctl;
	mpo_sysvmsq_check_msqget_t              *mpo_sysvmsq_check_msqget;
	mpo_sysvmsq_check_msqrcv_t              *mpo_sysvmsq_check_msqrcv;
	mpo_sysvmsq_check_msqsnd_t              *mpo_sysvmsq_check_msqsnd;
	mpo_sysvmsq_label_associate_t           *mpo_sysvmsq_label_associate;
	mpo_sysvmsq_label_destroy_t             *mpo_sysvmsq_label_destroy;
	mpo_sysvmsq_label_init_t                *mpo_sysvmsq_label_init;
	mpo_sysvmsq_label_recycle_t             *mpo_sysvmsq_label_recycle;
	mpo_sysvsem_check_semctl_t              *mpo_sysvsem_check_semctl;
	mpo_sysvsem_check_semget_t              *mpo_sysvsem_check_semget;
	mpo_sysvsem_check_semop_t               *mpo_sysvsem_check_semop;
	mpo_sysvsem_label_associate_t           *mpo_sysvsem_label_associate;
	mpo_sysvsem_label_destroy_t             *mpo_sysvsem_label_destroy;
	mpo_sysvsem_label_init_t                *mpo_sysvsem_label_init;
	mpo_sysvsem_label_recycle_t             *mpo_sysvsem_label_recycle;
	mpo_sysvshm_check_shmat_t               *mpo_sysvshm_check_shmat;
	mpo_sysvshm_check_shmctl_t              *mpo_sysvshm_check_shmctl;
	mpo_sysvshm_check_shmdt_t               *mpo_sysvshm_check_shmdt;
	mpo_sysvshm_check_shmget_t              *mpo_sysvshm_check_shmget;
	mpo_sysvshm_label_associate_t           *mpo_sysvshm_label_associate;
	mpo_sysvshm_label_destroy_t             *mpo_sysvshm_label_destroy;
	mpo_sysvshm_label_init_t                *mpo_sysvshm_label_init;
	mpo_sysvshm_label_recycle_t             *mpo_sysvshm_label_recycle;

	mpo_proc_notify_exit_t                  *mpo_proc_notify_exit;
	mpo_mount_check_snapshot_revert_t       *mpo_mount_check_snapshot_revert;
	mpo_vnode_check_getattr_t               *mpo_vnode_check_getattr;
	mpo_mount_check_snapshot_create_t       *mpo_mount_check_snapshot_create;
	mpo_mount_check_snapshot_delete_t       *mpo_mount_check_snapshot_delete;
	mpo_vnode_check_clone_t                 *mpo_vnode_check_clone;
	mpo_proc_check_get_cs_info_t            *mpo_proc_check_get_cs_info;
	mpo_proc_check_set_cs_info_t            *mpo_proc_check_set_cs_info;

	mpo_iokit_check_hid_control_t           *mpo_iokit_check_hid_control;

	mpo_vnode_check_access_t                *mpo_vnode_check_access;
	mpo_vnode_check_chdir_t                 *mpo_vnode_check_chdir;
	mpo_vnode_check_chroot_t                *mpo_vnode_check_chroot;
	mpo_vnode_check_create_t                *mpo_vnode_check_create;
	mpo_vnode_check_deleteextattr_t         *mpo_vnode_check_deleteextattr;
	mpo_vnode_check_exchangedata_t          *mpo_vnode_check_exchangedata;
	mpo_vnode_check_exec_t                  *mpo_vnode_check_exec;
	mpo_vnode_check_getattrlist_t           *mpo_vnode_check_getattrlist;
	mpo_vnode_check_getextattr_t            *mpo_vnode_check_getextattr;
	mpo_vnode_check_ioctl_t                 *mpo_vnode_check_ioctl;
	mpo_vnode_check_kqfilter_t              *mpo_vnode_check_kqfilter;
	mpo_vnode_check_label_update_t          *mpo_vnode_check_label_update;
	mpo_vnode_check_link_t                  *mpo_vnode_check_link;
	mpo_vnode_check_listextattr_t           *mpo_vnode_check_listextattr;
	mpo_vnode_check_lookup_t                *mpo_vnode_check_lookup;
	mpo_vnode_check_open_t                  *mpo_vnode_check_open;
	mpo_vnode_check_read_t                  *mpo_vnode_check_read;
	mpo_vnode_check_readdir_t               *mpo_vnode_check_readdir;
	mpo_vnode_check_readlink_t              *mpo_vnode_check_readlink;
	mpo_vnode_check_rename_from_t           *mpo_vnode_check_rename_from;
	mpo_vnode_check_rename_to_t             *mpo_vnode_check_rename_to;
	mpo_vnode_check_revoke_t                *mpo_vnode_check_revoke;
	mpo_vnode_check_select_t                *mpo_vnode_check_select;
	mpo_vnode_check_setattrlist_t           *mpo_vnode_check_setattrlist;
	mpo_vnode_check_setextattr_t            *mpo_vnode_check_setextattr;
	mpo_vnode_check_setflags_t              *mpo_vnode_check_setflags;
	mpo_vnode_check_setmode_t               *mpo_vnode_check_setmode;
	mpo_vnode_check_setowner_t              *mpo_vnode_check_setowner;
	mpo_vnode_check_setutimes_t             *mpo_vnode_check_setutimes;
	mpo_vnode_check_stat_t                  *mpo_vnode_check_stat;
	mpo_vnode_check_truncate_t              *mpo_vnode_check_truncate;
	mpo_vnode_check_unlink_t                *mpo_vnode_check_unlink;
	mpo_vnode_check_write_t                 *mpo_vnode_check_write;
	mpo_vnode_label_associate_devfs_t       *mpo_vnode_label_associate_devfs;
	mpo_vnode_label_associate_extattr_t     *mpo_vnode_label_associate_extattr;
	mpo_vnode_label_associate_file_t        *mpo_vnode_label_associate_file;
	mpo_vnode_label_associate_pipe_t        *mpo_vnode_label_associate_pipe;
	mpo_vnode_label_associate_posixsem_t    *mpo_vnode_label_associate_posixsem;
	mpo_vnode_label_associate_posixshm_t    *mpo_vnode_label_associate_posixshm;
	mpo_vnode_label_associate_singlelabel_t *mpo_vnode_label_associate_singlelabel;
	mpo_vnode_label_associate_socket_t      *mpo_vnode_label_associate_socket;
	mpo_vnode_label_copy_t                  *mpo_vnode_label_copy;
	mpo_vnode_label_destroy_t               *mpo_vnode_label_destroy;
	mpo_vnode_label_externalize_audit_t     *mpo_vnode_label_externalize_audit;
	mpo_vnode_label_externalize_t           *mpo_vnode_label_externalize;
	mpo_vnode_label_init_t                  *mpo_vnode_label_init;
	mpo_vnode_label_internalize_t           *mpo_vnode_label_internalize;
	mpo_vnode_label_recycle_t               *mpo_vnode_label_recycle;
	mpo_vnode_label_store_t                 *mpo_vnode_label_store;
	mpo_vnode_label_update_extattr_t        *mpo_vnode_label_update_extattr;
	mpo_vnode_label_update_t                *mpo_vnode_label_update;
	mpo_vnode_notify_create_t               *mpo_vnode_notify_create;
	mpo_vnode_check_signature_t             *mpo_vnode_check_signature;
	mpo_vnode_check_uipc_bind_t             *mpo_vnode_check_uipc_bind;
	mpo_vnode_check_uipc_connect_t          *mpo_vnode_check_uipc_connect;

	mpo_proc_check_run_cs_invalid_t         *mpo_proc_check_run_cs_invalid;
	mpo_proc_check_suspend_resume_t         *mpo_proc_check_suspend_resume;

	mpo_thread_userret_t                    *mpo_thread_userret;

	mpo_iokit_check_set_properties_t        *mpo_iokit_check_set_properties;

	mpo_vnode_check_supplemental_signature_t *mpo_vnode_check_supplemental_signature;

	mpo_vnode_check_searchfs_t              *mpo_vnode_check_searchfs;

	mpo_priv_check_t                        *mpo_priv_check;
	mpo_priv_grant_t                        *mpo_priv_grant;

	mpo_proc_check_map_anon_t               *mpo_proc_check_map_anon;

	mpo_vnode_check_fsgetpath_t             *mpo_vnode_check_fsgetpath;

	mpo_iokit_check_open_t                  *mpo_iokit_check_open;

	mpo_proc_check_ledger_t                 *mpo_proc_check_ledger;

	mpo_vnode_notify_rename_t               *mpo_vnode_notify_rename;

	mpo_vnode_check_setacl_t                *mpo_vnode_check_setacl;

	mpo_vnode_notify_deleteextattr_t        *mpo_vnode_notify_deleteextattr;

	mpo_system_check_kas_info_t             *mpo_system_check_kas_info;

	mpo_vnode_check_lookup_preflight_t      *mpo_vnode_check_lookup_preflight;

	mpo_vnode_notify_open_t                 *mpo_vnode_notify_open;

	mpo_system_check_info_t                 *mpo_system_check_info;

	mpo_pty_notify_grant_t                  *mpo_pty_notify_grant;
	mpo_pty_notify_close_t                  *mpo_pty_notify_close;

	mpo_vnode_find_sigs_t                   *mpo_vnode_find_sigs;

	mpo_kext_check_load_t                   *mpo_kext_check_load;
	mpo_kext_check_unload_t                 *mpo_kext_check_unload;

	mpo_proc_check_proc_info_t              *mpo_proc_check_proc_info;
	mpo_vnode_notify_link_t                 *mpo_vnode_notify_link;
	mpo_iokit_check_filter_properties_t     *mpo_iokit_check_filter_properties;
	mpo_iokit_check_get_property_t          *mpo_iokit_check_get_property;
};

   
                                 
  
                                                                      
                      
  
                                                            
   
typedef unsigned int mac_policy_handle_t;

#define mpc_t   struct mac_policy_conf *

   
                                   
  
                                                                
                                                             
                                                                             
                                                                            
                                                                              
  
                                                                      
                                                 
  
                                                                       
                                                                  
                                                                 
  
                                                                 
                         
   
                                                                
struct mac_policy_conf {
	const char              *mpc_name;                                
	const char              *mpc_fullname;                          
	char const * const *mpc_labelnames;                                    
	unsigned int             mpc_labelname_count;                                            
	const struct mac_policy_ops     *mpc_ops;                                      
	int                      mpc_loadtime_flags;                          
	int                     *mpc_field_off;                          
	int                      mpc_runtime_flags;                          
	mpc_t                    mpc_list;                                   
	void                    *mpc_data;                                
};

   
                                                 
  
                                                         
                                                                     
                                     
   
int     mac_policy_register(struct mac_policy_conf *mpc,
    mac_policy_handle_t *handlep, void *xd);

   
                                                    
  
                                                             
                                                                     
                                        
   
int     mac_policy_unregister(mac_policy_handle_t handle);

  
                                                             
   
int     mac_audit_text(char *text, mac_policy_handle_t handle);

  
                                                                  
   
int     mac_vnop_setxattr(struct vnode *, const char *, char *, size_t);
int     mac_vnop_getxattr(struct vnode *, const char *, char *, size_t,
    size_t *);
int     mac_vnop_removexattr(struct vnode *, const char *);

   
                                                               
                                                                         
                                       
                                                                    
                             
  
                                                      
  
                                                                         
   
#ifdef KERNEL_PRIVATE
int     mac_file_setxattr(struct fileglob *fg, const char *name, char *buf, size_t len);
#endif

   
                                                                      
                                                                            
                                            
                                                                       
                                  
                                                             
  
                                                           
  
                                                                              
   
#ifdef KERNEL_PRIVATE
int     mac_file_getxattr(struct fileglob *fg, const char *name, char *buf, size_t len,
    size_t *attrlen);
#endif

   
                                                                         
                                                                              
                                            
  
                                                            
  
                                                                              
   
#ifdef KERNEL_PRIVATE
int     mac_file_removexattr(struct fileglob *fg, const char *name);
#endif

  
                                                               
                      
   
#define MAC_AUDIT_DATA_LIMIT    1024

  
                                                                          
                                                   
                                                                   
   
#define MAC_AUDIT_DEFAULT       0                                
#define MAC_AUDIT_NO            1                                          
#define MAC_AUDIT_YES           2                                      

                                                                       

   
                                                
                        
  
                                                                
                                                                     
                                                                
            
   

      

   
                                                   
  
                                                                 
                                                                    
                                                                        
                                                                       
                                                                
                                       
   
#define MPC_LOADTIME_FLAG_NOTLATE       0x00000001

   
                                             
  
                                                                       
                                                                      
                                                                       
                                                                    
                                                                
   
#define MPC_LOADTIME_FLAG_UNLOADOK      0x00000002

   
                      
  
                                       
   
#define MPC_LOADTIME_FLAG_LABELMBUFS    0x00000004

   
                                         
  
                                                                     
                                                                     
                   
   
#define MPC_LOADTIME_BASE_POLICY        0x00000008

      

   
                                   
                        
  
                                                                    
                                                                     
                                                                
                                                           
   
#define MPC_RUNTIME_FLAG_REGISTERED     0x00000001

  
                        
   

#ifndef POLICY_VER
#define POLICY_VER      1.0
#endif

#define MAC_POLICY_SET(handle, mpops, mpname, mpfullname, lnames, lcount, slot, lflags, rflags) \
	static struct mac_policy_conf mpname##_mac_policy_conf = {      \
	        .mpc_name		= #mpname,                      \
	        .mpc_fullname		= mpfullname,                   \
	        .mpc_labelnames		= lnames,                       \
	        .mpc_labelname_count	= lcount,                       \
	        .mpc_ops		= mpops,                        \
	        .mpc_loadtime_flags	= lflags,                       \
	        .mpc_field_off		= slot,                         \
	        .mpc_runtime_flags	= rflags                        \
	};                                                              \
                                                                        \
	static kern_return_t                                            \
	kmod_start(kmod_info_t *ki, void *xd)                           \
	{                                                               \
	        return mac_policy_register(&mpname##_mac_policy_conf,   \
	            &handle, xd);                                       \
	}                                                               \
                                                                        \
	static kern_return_t                                            \
	kmod_stop(kmod_info_t *ki, void *xd)                            \
	{                                                               \
	        return mac_policy_unregister(handle);                   \
	}                                                               \
                                                                        \
	extern kern_return_t _start(kmod_info_t *ki, void *data);       \
	extern kern_return_t _stop(kmod_info_t *ki, void *data);        \
                                                                        \
	KMOD_EXPLICIT_DECL(security.mpname, POLICY_VER, _start, _stop)  \
	kmod_start_func_t *_realmain = kmod_start;                      \
	kmod_stop_func_t *_antimain = kmod_stop;                        \
	int _kext_apple_cc = __APPLE_CC__


#define LABEL_TO_SLOT(l, s)     (l)->l_perpolicy[s]

  
                                                                     
                                                                          
                                           
   
#ifdef KERNEL_PRIVATE
intptr_t        mac_label_get(struct label *l, int slot);
void            mac_label_set(struct label *l, int slot, intptr_t v);
intptr_t        mac_vnode_label_get(struct vnode *vp, int slot, intptr_t sentinel);
void            mac_vnode_label_set(struct vnode *vp, int slot, intptr_t v);
#endif

#define mac_get_mpc(h)          (mac_policy_list.entries[h].mpc)

   
                                            
  
                                                                     
                                                                 
                                                                        
                                                         
   

      

   
                                           
  
                                                                   
                                                               
  
                                                                     
   
#define MAC_WAITOK  0

   
                                               
  
                                                                      
                                                                    
                                         
   
#define MAC_NOWAIT  1

      

#endif                               
