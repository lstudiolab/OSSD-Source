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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                        

  
                             
  
                         
                                                                
   

#ifndef _SYS_SEM_H_
#define _SYS_SEM_H_


#include <sys/cdefs.h>
#include <sys/_types.h>
#include <machine/types.h>                

  
                                                             
                                
   
#include <sys/ipc.h>


  
                                                                       
                              
  
                                                              
                            
   
#include <sys/_types/_pid_t.h>
#include <sys/_types/_time_t.h>
#include <sys/_types/_size_t.h>

  
                                                                        
                                                                        
                                                                       
                                                                       
   
#if __DARWIN_UNIX03 || defined(KERNEL)
#pragma pack(4)
  
                             
  
                                                                         
                                                                          
                                         
  
                                                                      
                                
   
#if (defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE))
struct semid_ds
#else
#define semid_ds        __semid_ds_new
struct __semid_ds_new
#endif
{
	struct __ipc_perm_new sem_perm;                                        
	__int32_t       sem_base;                                              
	unsigned short  sem_nsems;                                       
	time_t          sem_otime;                                     
	__int32_t       sem_pad1;                                  
	time_t          sem_ctime;                                  
	                                                                  
	                                                                
	__int32_t       sem_pad2;                                  
	__int32_t       sem_pad3[4];                               
};
#pragma pack()
#else                         
#define semid_ds        __semid_ds_old
#endif                       

#if !__DARWIN_UNIX03
struct __semid_ds_old {
	struct __ipc_perm_old sem_perm;                                        
	__int32_t       sem_base;                                              
	unsigned short  sem_nsems;                                       
	time_t          sem_otime;                                     
	__int32_t       sem_pad1;                                  
	time_t          sem_ctime;                                  
	                                                                  
	                                                                
	__int32_t       sem_pad2;                                  
	__int32_t       sem_pad3[4];                               
};
#endif                        

  
                                                     
   
#define GETNCNT 3                                                     
#define GETPID  4                                                    
#define GETVAL  5                                                    
#define GETALL  6                                                       
#define GETZCNT 7                                                     
#define SETVAL  8                                                             
#define SETALL  9                                                     


                                                                       
struct sem {
	unsigned short  semval;                              
	pid_t           sempid;                                    
	unsigned short  semncnt;                                      
	unsigned short  semzcnt;                                    
};


  
                                                            
   
struct sembuf {
	unsigned short  sem_num;                               
	short           sem_op;                                        
	short           sem_flg;                                   
};

  
                                   
   
#define SEM_UNDO        010000                                                 


#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

  
                                                                       
                                                                      
  
                     
                                                     
                          
                 
                           
  
                                                                         
                                                                   
  
                                                                     
                                                                  
  
                                                                       
                                                                 
                         
   
union semun {
	int             val;                                  
	struct semid_ds *buf;                                              
	unsigned short  *array;                                        
};
typedef union semun semun_t;


  
              
   
#define SEM_A           0200                          
#define SEM_R           0400                         

#endif                                              



#ifndef KERNEL

__BEGIN_DECLS
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
int     semsys(int, ...);
#endif                                             
int     semctl(int, int, int, ...) __DARWIN_ALIAS(semctl);
int     semget(key_t, int, int);
int     semop(int, struct sembuf *, size_t);
__END_DECLS

#endif              

#endif               
