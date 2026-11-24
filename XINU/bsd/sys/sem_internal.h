/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   
                                                        

  
                                      
  
                         
   
  
                                                                
   

#ifndef _SYS_SEM__INTERNALH_
#define _SYS_SEM__INTERNALH_

#include <sys/sem.h>
#include <sys/cdefs.h>


  
                                                                            
   

#if __DARWIN_ALIGN_NATURAL
#pragma options align=natural
#endif

struct user_semid_ds {
	struct ipc_perm sem_perm;                                              
	struct sem      *sem_base;                                             
	unsigned short  sem_nsems;                                       
	user_time_t     sem_otime;                                     
	__int32_t       sem_pad1;                                  
	user_time_t     sem_ctime;                                  
	                                                                  
	                                                                
	__int32_t       sem_pad2;                                  
	__int32_t       sem_pad3[4];                               
};

#pragma pack(4)
struct user64_semid_ds {
	struct ipc_perm sem_perm;                                              
	int32_t sem_base;                                              
	unsigned short  sem_nsems;                                       
	user64_time_t   sem_otime;                                     
	int32_t sem_pad1;                                  
	user64_time_t   sem_ctime;                                  
	                                                                  
	                                                                
	int32_t sem_pad2;                                  
	int32_t sem_pad3[4];                               
};

struct user32_semid_ds {
	struct ipc_perm sem_perm;                                              
	int32_t sem_base;                                              
	unsigned short  sem_nsems;                                       
	user32_time_t   sem_otime;                                     
	int32_t sem_pad1;                                  
	user32_time_t   sem_ctime;                                  
	                                                                  
	                                                                
	int32_t sem_pad2;                                  
	int32_t sem_pad3[4];                               
};
#pragma pack()

union user_semun {
	user_addr_t     buf;                                               
	user_addr_t     array;                                         
};
typedef union user_semun user_semun_t;

#if __DARWIN_ALIGN_NATURAL
#pragma options align=reset
#endif


  
                              
   
#define SEMVMX  32767                                        
#define SEMAEM  16384                                         

  
                                                                         
                                                                      
                                                                       
                                                                       
                                                   
   
  
                           
   
#ifndef SEMMNS                                                 
#define SEMMNS  (1048576/sizeof(struct sem))
#endif                                                                 
#ifndef SEMMNI                                                  
#define SEMMNI  SEMMNS                                           
#endif
#ifndef SEMUME
#define SEMUME  10                                                     
#endif
#ifndef SEMMNU                                                      
#define SEMMNU  SEMMNS                                                          
#endif                                                                     

                           
#ifndef SEMMAP
#define SEMMAP  30                                                 
#endif
#ifndef SEMMSL
#define SEMMSL  SEMMNS                                          
#endif
#ifndef SEMOPM
#define SEMOPM  5                                                       
#endif


  
                                             
   
struct sem_undo {
	int     un_next_idx;                                             
	struct  proc *un_proc;                                       
	short   un_cnt;                                          
	struct undo {
		short   une_adjval;                                
		short   une_num;                         
		int     une_id;                    
		struct undo *une_next;                       
	} *un_ent;                                        
};

  
                                                                       
   
struct seminfo {
	int     semmap,                                            
	    semmni,                                             
	    semmns,                                            
	    semmnu,                                                 
	    semmsl,                                             
	    semopm,                                                     
	    semume,                                                    
	    semusz,                                                  
	    semvmx,                                          
	    semaem;                                           
};
extern struct seminfo   seminfo;

  
                                              
   
struct semid_kernel {
	struct  user_semid_ds u;
	struct  label *label;                            
};


                          
#define SEM_ALLOC       01000                               
#define SEM_DEST        02000                                                   

#define SEMMNI_INC      8                                                      
#define SEMMNS_INC      64                                          
#define SEMMNU_INC      32                                               

  
                                                                       
                              
  
                                                                   
                                     
   

  
                                                                                  
   

                                      
  
                                                                      
   
#define SEMUSZ          sizeof(struct sem_undo)

extern struct semid_kernel      *sema;                                 
extern struct sem               *sem_pool;                          
                                                               
                                            
   
extern struct sem_undo  *semu;                                   

  
                                             
   
                                                                                
                                                                           
  
                                                                                   
   
  
                                                                          
                
                                                                              
   
#define SEMU(ix)        (&semu[ix])


  
                                         
   
void    semexit(struct proc *p);

#endif                            
