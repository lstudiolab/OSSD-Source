/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
                                                                         
                                                                          
                                                                        
               
   
  
   
  
                    
  
                            
  
                                                                  
                                                                    
               
  
                                                                
                                                                 
                                                                  
                                               
  
                                                                  
                                                                   
  
                                                                    
                                                                      
  
   

#ifndef _MACH_PORT_H_
#define _MACH_PORT_H_

#include <sys/cdefs.h>
#include <stdint.h>
#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

  
                                                        
  
                                                           
                                                           
                                                   
  
                                                            
                                                           
  
   

typedef natural_t mach_port_name_t;
typedef mach_port_name_t *mach_port_name_array_t;

#ifdef  KERNEL

  
                                   
  
                                                                 
                                                                      
                          
  
                                                                     
                                                                     
                                                                      
                                                                       
                                                              
  
                                                                    
                                                                  
                                                                     
                                                                  
                                                 
  
   

#ifndef MACH_KERNEL_PRIVATE
  
                                                                     
                             
   
struct ipc_port;

#endif                           

typedef struct ipc_port         *ipc_port_t;

#define IPC_PORT_NULL           ((ipc_port_t) NULL)
#define IPC_PORT_DEAD           ((ipc_port_t)~0UL)
#define IPC_PORT_VALID(port)    ipc_port_valid(port)

static inline boolean_t
ipc_port_valid(ipc_port_t port)
{
	return port != IPC_PORT_DEAD && port;
}

typedef ipc_port_t              mach_port_t;

  
                                                                   
                                   
   

#define CAST_MACH_PORT_TO_NAME(x) ((mach_port_name_t)(uintptr_t)(x))
#define CAST_MACH_NAME_TO_PORT(x) ((x) == MACH_PORT_DEAD ? (mach_port_t)IPC_PORT_DEAD : (mach_port_t)(uintptr_t)(x))

#else               

  
                                   
  
                                                             
                                                           
                                                                
                                         
  
                                                               
                                                                   
                                                           
                                                           
               
  
   

#include <sys/_types.h>
#include <sys/_types/_mach_port_t.h>

#endif              

typedef mach_port_t                     *mach_port_array_t;

  
                                                                    
                                                                 
                                                               
                                                                       
                                                        
                                               
   

#if defined(XNU_KERNEL_PRIVATE) && defined(__cplusplus)
#define MACH_PORT_NULL          NULL
#else
#define MACH_PORT_NULL          0                                
#endif
#define MACH_PORT_DEAD          ((mach_port_name_t) ~0)
#define MACH_PORT_VALID(name)                           \
	        (((name) != MACH_PORT_NULL) &&          \
	         ((name) != MACH_PORT_DEAD))


  
                                                         
                                                            
                                                               
                                                                   
                                                                
                                                              
                                                             
                                                               
                                                               
                                                     
  
   

#ifndef NO_PORT_GEN

#define MACH_PORT_INDEX(name)           ((name) >> 8)
#define MACH_PORT_GEN(name)             (((name) & 0xff) << 24)
#define MACH_PORT_MAKE(index, gen)      \
	        (((index) << 8) | (gen) >> 24)

#else                    

#define MACH_PORT_INDEX(name)           (name)
#define MACH_PORT_GEN(name)             (0)
#define MACH_PORT_MAKE(index, gen)      (index)

#endif                   


  
                                                              
                                                           
                                                              
                                                            
                                                                   
                                                          
                    
   

typedef natural_t mach_port_right_t;

#define MACH_PORT_RIGHT_SEND            ((mach_port_right_t) 0)
#define MACH_PORT_RIGHT_RECEIVE         ((mach_port_right_t) 1)
#define MACH_PORT_RIGHT_SEND_ONCE       ((mach_port_right_t) 2)
#define MACH_PORT_RIGHT_PORT_SET        ((mach_port_right_t) 3)
#define MACH_PORT_RIGHT_DEAD_NAME       ((mach_port_right_t) 4)
#define MACH_PORT_RIGHT_LABELH          ((mach_port_right_t) 5)                     
#define MACH_PORT_RIGHT_NUMBER          ((mach_port_right_t) 6)                            

#ifdef MACH_KERNEL_PRIVATE
#define MACH_PORT_RIGHT_VALID_TRANSLATE(right) \
	((right) >= MACH_PORT_RIGHT_SEND && (right) <= MACH_PORT_RIGHT_DEAD_NAME)
#endif

typedef natural_t mach_port_type_t;
typedef mach_port_type_t *mach_port_type_array_t;

#define MACH_PORT_TYPE(right)                                           \
	        ((mach_port_type_t)(((mach_port_type_t) 1)              \
	        << ((right) + ((mach_port_right_t) 16))))
#define MACH_PORT_TYPE_NONE         ((mach_port_type_t) 0L)
#define MACH_PORT_TYPE_SEND         MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND)
#define MACH_PORT_TYPE_RECEIVE      MACH_PORT_TYPE(MACH_PORT_RIGHT_RECEIVE)
#define MACH_PORT_TYPE_SEND_ONCE    MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_SET     MACH_PORT_TYPE(MACH_PORT_RIGHT_PORT_SET)
#define MACH_PORT_TYPE_DEAD_NAME    MACH_PORT_TYPE(MACH_PORT_RIGHT_DEAD_NAME)
#define MACH_PORT_TYPE_LABELH       MACH_PORT_TYPE(MACH_PORT_RIGHT_LABELH)               

#ifdef MACH_KERNEL_PRIVATE
                                                                           
#define MACH_PORT_TYPE_EX_RECEIVE   MACH_PORT_TYPE_LABELH
#endif

                              

#define MACH_PORT_TYPE_SEND_RECEIVE                                     \
	        (MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_RECEIVE)
#define MACH_PORT_TYPE_SEND_RIGHTS                                      \
	        (MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_RIGHTS                                      \
	        (MACH_PORT_TYPE_SEND_RIGHTS|MACH_PORT_TYPE_RECEIVE)
#define MACH_PORT_TYPE_PORT_OR_DEAD                                     \
	        (MACH_PORT_TYPE_PORT_RIGHTS|MACH_PORT_TYPE_DEAD_NAME)
#define MACH_PORT_TYPE_ALL_RIGHTS                                       \
	        (MACH_PORT_TYPE_PORT_OR_DEAD|MACH_PORT_TYPE_PORT_SET)

                                                                     

#define MACH_PORT_TYPE_DNREQUEST                0x80000000
#define MACH_PORT_TYPE_SPREQUEST                0x40000000
#define MACH_PORT_TYPE_SPREQUEST_DELAYED        0x20000000

                                       

typedef natural_t mach_port_urefs_t;
typedef integer_t mach_port_delta_t;                                         

                                                               

typedef natural_t mach_port_seqno_t;                                 
typedef natural_t mach_port_mscount_t;                               
typedef natural_t mach_port_msgcount_t;                             
typedef natural_t mach_port_rights_t;                                 

  
                                                      
   
#define MACH_PORT_SRIGHTS_NONE          0                               
#define MACH_PORT_SRIGHTS_PRESENT       1                            
typedef unsigned int mach_port_srights_t;                                  

typedef struct mach_port_status {
	mach_port_rights_t      mps_pset;                                          
	mach_port_seqno_t       mps_seqno;                           
	mach_port_mscount_t     mps_mscount;                         
	mach_port_msgcount_t    mps_qlimit;                      
	mach_port_msgcount_t    mps_msgcount;                            
	mach_port_rights_t      mps_sorights;                                  
	boolean_t               mps_srights;                               
	boolean_t               mps_pdrequest;                               
	boolean_t               mps_nsrequest;                             
	natural_t               mps_flags;                              
} mach_port_status_t;

                                                           
#define MACH_PORT_QLIMIT_ZERO           (0)
#define MACH_PORT_QLIMIT_BASIC          (5)
#define MACH_PORT_QLIMIT_SMALL          (16)
#define MACH_PORT_QLIMIT_LARGE          (1024)
#define MACH_PORT_QLIMIT_KERNEL         (65534)
#define MACH_PORT_QLIMIT_MIN            MACH_PORT_QLIMIT_ZERO
#define MACH_PORT_QLIMIT_DEFAULT        MACH_PORT_QLIMIT_BASIC
#define MACH_PORT_QLIMIT_MAX            MACH_PORT_QLIMIT_LARGE

typedef struct mach_port_limits {
	mach_port_msgcount_t    mpl_qlimit;                         
} mach_port_limits_t;

                                                                
#define MACH_PORT_STATUS_FLAG_TEMPOWNER         0x01
#define MACH_PORT_STATUS_FLAG_GUARDED           0x02
#define MACH_PORT_STATUS_FLAG_STRICT_GUARD      0x04
#define MACH_PORT_STATUS_FLAG_IMP_DONATION      0x08
#define MACH_PORT_STATUS_FLAG_REVIVE            0x10
#define MACH_PORT_STATUS_FLAG_TASKPTR           0x20
#define MACH_PORT_STATUS_FLAG_GUARD_IMMOVABLE_RECEIVE 0x40
#define MACH_PORT_STATUS_FLAG_NO_GRANT          0x80

typedef struct mach_port_info_ext {
	mach_port_status_t      mpie_status;
	mach_port_msgcount_t    mpie_boost_cnt;
	uint32_t                reserved[6];
} mach_port_info_ext_t;

typedef integer_t *mach_port_info_t;                                            

                                                
typedef int     mach_port_flavor_t;
#define MACH_PORT_LIMITS_INFO           1                                    
#define MACH_PORT_RECEIVE_STATUS        2                                    
#define MACH_PORT_DNREQUESTS_SIZE       3                        
#define MACH_PORT_TEMPOWNER             4                                                                       
#define MACH_PORT_IMPORTANCE_RECEIVER   5                                                              
#define MACH_PORT_DENAP_RECEIVER        6                                                            
#define MACH_PORT_INFO_EXT              7                                      

#define MACH_PORT_LIMITS_INFO_COUNT     ((natural_t) \
	(sizeof(mach_port_limits_t)/sizeof(natural_t)))
#define MACH_PORT_RECEIVE_STATUS_COUNT  ((natural_t) \
	(sizeof(mach_port_status_t)/sizeof(natural_t)))
#define MACH_PORT_DNREQUESTS_SIZE_COUNT 1
#define MACH_PORT_INFO_EXT_COUNT        ((natural_t) \
	(sizeof(mach_port_info_ext_t)/sizeof(natural_t)))
  
                                                                     
                                          
   
typedef struct mach_port_qos {
	unsigned int            name:1;                         
	unsigned int            prealloc:1;                             
	boolean_t               pad1:30;
	natural_t               len;
} mach_port_qos_t;

                                    

  
                                        
                                      
                                               
                  
   
#define MPO_CONTEXT_AS_GUARD    0x01                               
#define MPO_QLIMIT              0x02                                           
#define MPO_TEMPOWNER           0x04                                           
#define MPO_IMPORTANCE_RECEIVER 0x08                                              
#define MPO_INSERT_SEND_RIGHT   0x10                                          
#define MPO_STRICT              0x20                                        
#define MPO_DENAP_RECEIVER      0x40                                              
#define MPO_IMMOVABLE_RECEIVE   0x80                                                                    
#define MPO_FILTER_MSG          0x100                                
#define MPO_TG_BLOCK_TRACKING   0x200                                                                     

  
                                                      
                    
   
typedef struct mach_port_options {
	uint32_t                flags;                                                  
	mach_port_limits_t      mpl;                                              
	union {
		uint64_t                reserved[2];                         
		mach_port_name_t        work_interval_port;                            
	};
}mach_port_options_t;

typedef mach_port_options_t *mach_port_options_ptr_t;

  
                                                  
                                                       
                               
   
#define GUARD_TYPE_MACH_PORT    0x1

                                                   
enum mach_port_guard_exception_codes {
	kGUARD_EXC_DESTROY                       = 1u << 0,
	kGUARD_EXC_MOD_REFS                      = 1u << 1,
	kGUARD_EXC_SET_CONTEXT               = 1u << 2,
	kGUARD_EXC_UNGUARDED                 = 1u << 3,
	kGUARD_EXC_INCORRECT_GUARD           = 1u << 4,
	kGUARD_EXC_IMMOVABLE                 = 1u << 5,
	kGUARD_EXC_STRICT_REPLY              = 1u << 6,
	kGUARD_EXC_MSG_FILTERED              = 1u << 7,
	                                            
	kGUARD_EXC_INVALID_RIGHT         = 1u << 8,
	kGUARD_EXC_INVALID_NAME          = 1u << 9,
	kGUARD_EXC_INVALID_VALUE         = 1u << 10,
	kGUARD_EXC_INVALID_ARGUMENT      = 1u << 11,
	kGUARD_EXC_RIGHT_EXISTS          = 1u << 12,
	kGUARD_EXC_KERN_NO_SPACE         = 1u << 13,
	kGUARD_EXC_KERN_FAILURE          = 1u << 14,
	kGUARD_EXC_KERN_RESOURCE         = 1u << 15,
	kGUARD_EXC_SEND_INVALID_REPLY    = 1u << 16,
	kGUARD_EXC_SEND_INVALID_VOUCHER  = 1u << 17,
	kGUARD_EXC_SEND_INVALID_RIGHT    = 1u << 18,
	kGUARD_EXC_RCV_INVALID_NAME      = 1u << 19,
	kGUARD_EXC_RCV_GUARDED_DESC      = 1u << 20,                                                  
	kGUARD_EXC_MOD_REFS_NON_FATAL    = 1u << 21,
	kGUARD_EXC_IMMOVABLE_NON_FATAL   = 1u << 22,
};

#define MAX_FATAL_kGUARD_EXC_CODE (1u << 7)

  
                         
   
#define MPG_FLAGS_NONE                             (0x00ull)

  
                                                                                     
   
#define MPG_FLAGS_STRICT_REPLY_INVALID_REPLY_DISP  (0x01ull << 56)
#define MPG_FLAGS_STRICT_REPLY_INVALID_REPLY_PORT  (0x02ull << 56)
#define MPG_FLAGS_STRICT_REPLY_INVALID_VOUCHER     (0x04ull << 56)
#define MPG_FLAGS_STRICT_REPLY_NO_BANK_ATTR        (0x08ull << 56)
#define MPG_FLAGS_STRICT_REPLY_MISMATCHED_PERSONA  (0x10ull << 56)
#define MPG_FLAGS_STRICT_REPLY_MASK                (0xffull << 56)

  
                                                                                 
   
#define MPG_FLAGS_MOD_REFS_PINNED_DEALLOC          (0x01ull << 56)

  
                                                                                  
   
#define MPG_FLAGS_IMMOVABLE_PINNED                 (0x01ull << 56)

  
                                                           
                                                 
   
#define MPG_STRICT              0x01                                          
#define MPG_IMMOVABLE_RECEIVE   0x02                                                        

#if     !__DARWIN_UNIX03 && !defined(_NO_PORT_T_FROM_MACH)
  
                                                             
                                                                  
                      
   
typedef mach_port_t             port_t;
typedef mach_port_name_t        port_name_t;
typedef mach_port_name_t        *port_name_array_t;

#define PORT_NULL               ((port_t) 0)
#define PORT_DEAD               ((port_t) ~0)
#define PORT_VALID(name) \
	        ((port_t)(name) != PORT_NULL && (port_t)(name) != PORT_DEAD)

#endif                                                 

#endif                     
