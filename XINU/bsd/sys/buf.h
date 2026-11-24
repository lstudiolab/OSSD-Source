/*
 * Copyright (c) 2000-2016 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)buf.h	8.9 (Berkeley) 3/30/95
 */

#ifndef _SYS_BUF_H_
#define _SYS_BUF_H_

#include <sys/cdefs.h>
#include <sys/kernel_types.h>
#include <sys/ucred.h>
#include <mach/memory_object_types.h>


#define B_WRITE         0x00000000                                       
#define B_READ          0x00000001                        
#define B_ASYNC         0x00000002                                   
#define B_NOCACHE       0x00000004                                         
#define B_DELWRI        0x00000008                                          
#define B_LOCKED        0x00000010                                          
#define B_PHYS          0x00000020                               
#define B_CLUSTER       0x00000040                                                    
#define B_PAGEIO        0x00000080                       
#define B_META          0x00000100                                      
#define B_RAW           0x00000200                                            
#define B_FUA           0x00000400                                                  
#define B_PASSIVE       0x00000800                                                    
#define B_IOSTREAMING   0x00001000                                              
#define B_THROTTLED_IO  0x00002000                                         
#define B_ENCRYPTED_IO  0x00004000                         
#define B_STATICCONTENT 0x00008000                                                

  
                                            
                                                   
                    
   

__BEGIN_DECLS

   
                          
                                                                                                              
                             
   
void    buf_markaged(buf_t bp);

   
                             
                                                                                                            
                             
   
void    buf_markinvalid(buf_t bp);

   
                             
                                                                                              
                                                                                                    
                                                                                           
                             
   
void    buf_markdelayed(buf_t bp);

void    buf_markclean(buf_t);

   
                           
                                                                  
                                                                                                 
                                              
                             
   
void    buf_markeintr(buf_t bp);

   
                         
                                                                              
                             
   
void    buf_markfua(buf_t bp);

   
                     
                                                                        
                             
                                                                    
   
int     buf_fua(buf_t bp);

   
                       
                                                    
                             
                                                       
   
int     buf_valid(buf_t bp);

   
                           
                                                         
                                                                                                           
                                       
                             
                                                                             
   
int     buf_fromcache(buf_t bp);

   
                     
                                                                         
                                                                                   
                                                                    
                                       
                                                  
   
void *  buf_upl(buf_t bp);

   
                           
                                                                    
                                                                                                       
                                             
                                                                                              
   
uint32_t buf_uploffset(buf_t bp);

   
                       
                                                                      
                                                                                                        
                                                
                                              
                                               
   
kauth_cred_t buf_rcred(buf_t bp);

   
                       
                                                                      
                                                                                                        
                                                
                                              
                                               
   
kauth_cred_t buf_wcred(buf_t bp);

   
                      
                                                          
                                                                                                  
                                             
                                                      
                                              
   
proc_t  buf_proc(buf_t bp);

   
                          
                                                                                   
                                                                                                                       
                                               
                                                       
   
uint32_t buf_dirtyoff(buf_t bp);

   
                          
                                                                                 
                                                                                                                          
                                                                                               
                                            
                                                                                                                                
   
uint32_t buf_dirtyend(buf_t bp);

   
                             
                                                                                   
                                                                    
                                            
   
void    buf_setdirtyoff(buf_t bp, uint32_t);

   
                             
                                                                                 
                                                                                                                          
                                                                                               
                                            
   
void    buf_setdirtyend(buf_t bp, uint32_t);

   
                       
                                                           
                                                   
                                                   
                                  
   
errno_t buf_error(buf_t bp);

   
                          
                                             
                                              
   
void    buf_seterror(buf_t bp, errno_t);

   
                          
                                    
                                     
                                        
                                                                                                        
   
void    buf_setflags(buf_t bp, int32_t flags);

   
                            
                                      
                                      
                                          
                                                                                                             
   
void    buf_clearflags(buf_t bp, int32_t flags);

   
                       
                                        
                                                                                         
                                         
                  
   
int32_t buf_flags(buf_t bp);

   
                       
                                               
                                                                                                             
                                               
                                         
                                                                             
   
void    buf_reset(buf_t bp, int32_t flags);

   
                     
                                                   
                                                                                               
                                                                                                                       
                                                                                                
                                                                                                        
                                                                                                          
                                                                                                      
                                                                                                           
                                                                                                                             
                                                     
                                                   
                                                              
   
errno_t buf_map(buf_t bp, caddr_t *io_addr);

   
                       
                                               
                                                                                               
                                                                                                              
                                                                                                
                                                                                                        
                                                                                                        
                                                                                                        
                     
                                                     
                                                            
   
errno_t buf_unmap(buf_t bp);

   
                            
                                                   
                                              
                                      
   
void    buf_setdrvdata(buf_t bp, void *drvdata);

   
                            
                                                     
                                              
                               
   
void *  buf_drvdata(buf_t bp);

   
                              
                                                       
                                                  
                                            
   
void    buf_setfsprivate(buf_t bp, void *fsprivate);

   
                           
                                                         
                                                  
                                   
   
void *  buf_fsprivate(buf_t bp);

   
                       
                                                                                                
                                                                                                                
                                                                                                             
                                                                                                               
                                                                         
                                                        
                         
   
daddr64_t buf_blkno(buf_t bp);

   
                        
                                                                
                                                                                                                
                               
                                                       
                         
   
daddr64_t buf_lblkno(buf_t bp);

   
                          
                                                                 
                                                                                               
                                                        
                                     
   
void    buf_setblkno(buf_t bp, daddr64_t blkno);

   
                           
                                                                
                                                                                                                
                               
                                                       
                                      
   
void    buf_setlblkno(buf_t bp, daddr64_t lblkno);

   
                       
                                                                                                             
                                             
                       
   
uint32_t buf_count(buf_t bp);

   
                      
                                                            
                                                                  
                                       
                 
   
uint32_t buf_size(buf_t bp);

   
                       
                                                                                 
                                                     
                                                    
                                            
   
uint32_t buf_resid(buf_t bp);

   
                          
                                                                                                             
                                             
                               
   
void    buf_setcount(buf_t bp, uint32_t bcount);

   
                         
                                                            
                                                                                     
                                                                                          
                                       
   
void    buf_setsize(buf_t bp, uint32_t);

   
                          
                                                                   
                                                                                                   
                                                                                                  
                           
                                                    
   
void    buf_setresid(buf_t bp, uint32_t resid);

   
                            
                                                                      
                                                                                                      
                                                    
                                               
                                       
   
void    buf_setdataptr(buf_t bp, uintptr_t data);

   
                         
                                                                                       
                                                 
                                                    
                                        
   
uintptr_t buf_dataptr(buf_t bp);

   
                       
                                                     
                                                                                           
                                                                                               
                       
                                             
                           
   
vnode_t buf_vnode(buf_t bp);

   
                          
                                                     
                                                                                             
                                        
                                                
   
void    buf_setvnode(buf_t bp, vnode_t vp);

   
                        
                                                         
                                                                                                    
                                                                                                    
                                          
                                                 
                      
   
dev_t   buf_device(buf_t bp);

   
                           
                                                      
                                                                                                        
                                                                       
                                            
                                          
                                                             
   
errno_t buf_setdevice(buf_t bp, vnode_t vp);

   
                          
                                                                        
                                                                                                           
                                                                                                     
                                                                                                       
                                                                                                        
                                                                                                          
                                              
                                               
                                                                        
                                                                      
   
errno_t buf_strategy(vnode_t devvp, void *ap);

  
                             
   
#define BUF_WAIT        0x01

   
                            
                                                              
                                                                                   
                                                                                      
                                                                                             
                                                   
                                                                                        
                                                                                          
                                                                            
                                                                                
                                                                                               
                               
                                              
                                       
                                                                                                       
                                      
                                                             
   
errno_t buf_invalblkno(vnode_t vp, daddr64_t lblkno, int flags);

   
                          
                                                                               
                                                                                              
                                           
                                                                      
   
void * buf_callback(buf_t bp);

   
                             
                                                                              
                                                                                               
                                                                                           
                                                       
                                           
                                                
                                                                
                               
   
errno_t buf_setcallback(buf_t bp, void (*callback)(buf_t, void *), void *transaction);

   
                        
                                                                                 
                                                                                         
                                                                                           
                                                                                              
                                                                                           
                                                                                                   
           
                                      
                                        
                                                                 
                                                                                   
   
errno_t buf_setupl(buf_t bp, upl_t upl, uint32_t offset);

   
                       
                                                                              
                                                                                      
                                                                                  
                                                                            
                                                                                             
                                                                                                                    
                                                                                                   
                                                                          
                              
                                                                                                                 
                                                                                    
                                                                                                                 
                                                     
                                                                                                                    
   
buf_t   buf_clone(buf_t bp, int io_offset, int io_size, void (*iodone)(buf_t, void *), void *arg);


   
                               
                                                                                            
                               
                                                                                                 
                                                  
                                                                                                       
                                           
                                                                                                                 
                                                     
                                                                                                                                      
   

buf_t   buf_create_shadow(buf_t bp, boolean_t force_copy, uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg);


   
                        
                                                                 
                              
                                               
   
int     buf_shadow(buf_t bp);


   
                       
                                               
                                                                                                                        
                                                            
                                                                                                
                                                                                                       
                       
   
buf_t   buf_alloc(vnode_t vp);

   
                      
                                                                
                                                                                                
                                 
   
void    buf_free(buf_t bp);

  
                               
   
#define BUF_WRITE_DATA  0x0001                                       
#define BUF_SKIP_META   0x0002                                         
#define BUF_INVALIDATE_LOCKED   0x0004                                               

   
                                
                                                                
                                                                                                                 
                                                                                                               
                                                                                                                 
                                 
                                                 
                                                                                                                     
                                                     
                                                                                   
                                                                                                                 
                             
                                                      
   
int     buf_invalidateblks(vnode_t vp, int flags, int slpflag, int slptimeo);

  
                                               
   
#define BUF_SKIP_NONLOCKED      0x01
#define BUF_SKIP_LOCKED         0x02
#define BUF_SCAN_CLEAN          0x04                                
#define BUF_SCAN_DIRTY          0x08                                
#define BUF_NOTIFY_BUSY         0x10                                                                


#define BUF_RETURNED            0
#define BUF_RETURNED_DONE       1
#define BUF_CLAIMED             2
#define BUF_CLAIMED_DONE        3
   
                                
                                              
                                              
                                                             
                                                                  
                                                                               
                                                                        
                                          
   
void    buf_flushdirtyblks(vnode_t vp, int wait, int flags, const char *msg);

   
                         
                                                                            
                                              
                                                                          
                                                                       
                                                                                                    
                                                                                     
                                                                                      
                
                                                                                                                                                    
                                              
                                              
                                                                                              
                                         
                                                                 
   
void    buf_iterate(vnode_t vp, int (*callout)(buf_t, void *), int flags, void *arg);

   
                       
                                                            
                                                                                 
                                                                                      
                                                                                                
                      
                                     
   
void    buf_clear(buf_t bp);

   
                         
                                                     
                                                                                                                     
                                                                                                                   
                                                                      
                                                  
                                                                                                          
   
errno_t buf_bawrite(buf_t bp);

   
                         
                                              
                                                                                                                                  
                                                                                                                
                                                                                                        
                                                                                                               
                                                                                                                                              
                                          
                                                   
                                                                                       
   
errno_t buf_bdwrite(buf_t bp);

   
                        
                                                     
                                                                                                              
                                                                                                              
                                                                     
                                          
                                                     
   
errno_t buf_bwrite(buf_t bp);

   
                         
                                       
                                                                                                                     
                                                                                                              
                                                                                                                    
                                                                                               
                                                                                                        
                                                                                                                      
                                                           
                                                  
   
void    buf_biodone(buf_t bp);

   
                         
                                                   
                                                                                       
                                    
                                                                                                           
   
errno_t buf_biowait(buf_t bp);

   
                        
                                                                           
                                                                                                           
                                                                                                            
                                                                                     
                                                                                     
                                                                                                                  
                                                                                                               
                                                                                                     
                                                                                    
                                    
   
void    buf_brelse(buf_t bp);

   
                       
                                                   
                                                                                                                     
                                                                                                                     
                                                                                                                      
                                                                                                                         
                                                                       
                                          
                                                               
                                                         
                                                                                                  
                                              
                                                          
   
errno_t buf_bread(vnode_t vp, daddr64_t blkno, int size, kauth_cred_t cred, buf_t *bpp);

   
                        
                                                       
                                                                                                
                                                                                             
                                                                                              
                                                                                                    
                                                                       
                                          
                                                                             
                                                         
                                                                              
                                                                                                                         
                                                          
                                                                                                  
                                              
                                                          
   
errno_t buf_breadn(vnode_t vp, daddr64_t blkno, int size, daddr64_t *rablks, int *rasizes, int nrablks, kauth_cred_t cred, buf_t *bpp);

   
                            
                                                            
                                                                                                                          
                                                                                                                     
                                                                                                                      
                                                                                                                  
                                                                                                                            
                                          
                                                               
                                                         
                                                                                                  
                                              
                                                          
   
errno_t buf_meta_bread(vnode_t vp, daddr64_t blkno, int size, kauth_cred_t cred, buf_t *bpp);

   
                             
                                                                
                                                                                                          
                                                                                             
                                                                                              
                             
                                          
                                                                             
                                                         
                                                                              
                                                                                                                         
                                                          
                                                                                                  
                                              
                                                          
   
errno_t buf_meta_breadn(vnode_t vp, daddr64_t blkno, int size, daddr64_t *rablks, int *rasizes, int nrablks, kauth_cred_t cred, buf_t *bpp);

   
                     
                                                                                                                      
                                                                                                                          
                                                                                                                   
                                                   
                                                    
                           
   
u_int   minphys(buf_t bp);

   
                    
                                                                               
                                                                                          
                                             
                                                               
                                                                            
                                              
                                   
                                                                                              
                                                                                                         
                                                
                                                       
                                                                                
   
int     physio(void (*f_strategy)(buf_t), buf_t bp, dev_t dev, int flags, u_int (*f_minphys)(buf_t), struct uio *uio, int blocksize);


  
                                       
   
#define BLK_READ        0x01                         
#define BLK_WRITE       0x02                          
#define BLK_META        0x10                             
  
                                                               
                                                        
   
#define BLK_ONLYVALID   0x80000000

   
                        
                                                                                                          
                                                                                                                  
                                                                                                                   
                                                                                                                             
                                                                                                                                       
                                                                                                                     
                                                                                                                
                                                                                                               
                                                                                                           
                                                                                                    
                                              
                                          
                                      
                              
                                                                                      
                                                                                                              
                                                                                                                                      
                                                                                                                             
                                                                                                                                      
                                                                                                   
   
buf_t   buf_getblk(vnode_t vp, daddr64_t blkno, int size, int slpflag, int slptimeo, int operation);

   
                         
                                                                                              
                                                                                                                  
                                                          
                               
                                        
   
buf_t   buf_geteblk(int size);

   
                                        
                                      
                                                 
                                          
                                                   
   
void    buf_clear_redundancy_flags(buf_t bp, uint32_t flags);

   
                                 
                                                   
                                                    
                  
   
uint32_t        buf_redundancy_flags(buf_t bp);

   
                                    
                                               
                                                
                                        
                                                          
   
void    buf_set_redundancy_flags(buf_t bp, uint32_t flags);

   
                      
                                               
                                             
                      
   
bufattr_t buf_attr(buf_t bp);

   
                            
                                                                   
                             
   
void buf_markstatic(buf_t bp);

   
                        
                                                     
                             
                                                           
   
int     buf_static(buf_t bp);

   
                                 
                                                                       
                                         
                                                                                                                 
   
void bufattr_markioscheduled(bufattr_t bap);

   
                             
                                                         
                                         
                                                                      
   
int bufattr_ioscheduled(bufattr_t bap);

   
                                       
                                                                                      
                                         
                                                                                              
   
void bufattr_markexpeditedmeta(bufattr_t bap);

   
                                   
                                                                    
                                         
                                                                                
   
int bufattr_expeditedmeta(bufattr_t bap);

#ifdef KERNEL_PRIVATE
void    buf_setfilter(buf_t, void (*)(buf_t, void *), void *, void(**)(buf_t, void *), void **);

                                                           
bufattr_t bufattr_alloc(void);
bufattr_t bufattr_dup(bufattr_t bap);
void bufattr_free(bufattr_t bap);

   
                         
                                                     
                                                                      
                                                           
   
struct cpx *bufattr_cpx(bufattr_t bap);

   
                            
                                                   
                                                       
   
void bufattr_setcpx(bufattr_t bap, struct cpx *cpx);

   
                           
                                                 
                                                               
   
uint64_t bufattr_cpoff(bufattr_t bap);

   
                              
                                                                
                       
                                                                  
   
void bufattr_setcpoff(bufattr_t bap, uint64_t);

   
                                  
                                                            
                                        
                                                                  
   
int bufattr_rawencrypted(bufattr_t bap);

   
                                    
                                                               
                                         
                                                                                                                               
   
void bufattr_markgreedymode(bufattr_t bap);

   
                                
                                                                
                                         
                                                                                                                               
                                                            
   
int     bufattr_greedymode(bufattr_t bap);

   
                                     
                                                                               
                                         
                                                                                                                                    
   
void bufattr_markisochronous(bufattr_t bap);

   
                                 
                                                                
                                         
                                                                                                                                    
                                                                 
   
int     bufattr_isochronous(bufattr_t bap);


   
                               
                                             
                                        
                                                            
   
int bufattr_throttled(bufattr_t bap);

   
                             
                                                  
                                        
                                                                 
   
int bufattr_passive(bufattr_t bap);

   
                             
                                                      
                                        
                                                             
   
int bufattr_nocache(bufattr_t bap);

   
                          
                                                               
                                        
                                                                  
   

int bufattr_meta(bufattr_t bap);

   
                              
                                             
                                              
   
void bufattr_markmeta(bufattr_t bap);


   
                                    
                                                                         
                                        
                                                                                        
   
int bufattr_delayidlesleep(bufattr_t bap);

   
                                      
                                         
                              
                                                                
   
vm_offset_t buf_kernel_addrperm_addr(void * addr);

   
                                       
                                                                   
                                                                                                        
                                                                                                    
                                         
   
void bufattr_markquickcomplete(bufattr_t bap);

   
                                   
                                                              
                                                                                                        
                                                                                                    
                                        
                                                                              
   
int bufattr_quickcomplete(bufattr_t bap);

int     count_lock_queue(void);

  
                        
   
#define BAC_NOWAIT              0x01                                      
#define BAC_REMOVE              0x02                                                       
#define BAC_SKIP_NONLOCKED      0x04                                     
#define BAC_SKIP_LOCKED         0x08                                    

errno_t buf_acquire(buf_t, int, int, int);

buf_t   buf_create_shadow_priv(buf_t bp, boolean_t force_copy, uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg);

void    buf_drop(buf_t);

#endif                     

__END_DECLS


                                     
#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) &= ~(f)
#define ISSET(t, f)     ((t) & (f))


#endif                   
