/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#ifndef _REASON_H_
#define _REASON_H_

#include <stdint.h>

__BEGIN_DECLS

#ifdef KERNEL_PRIVATE

#include <kern/kern_cdata.h>
#include <os/refcnt.h>

#ifdef XNU_KERNEL_PRIVATE
#include <kern/locks.h>

typedef struct os_reason {
	decl_lck_mtx_data(, osr_lock);
	os_refcnt_t                     osr_refcount;
	uint32_t                        osr_namespace;
	uint64_t                        osr_code;
	uint64_t                        osr_flags;
	uint32_t                        osr_bufsize;
	struct kcdata_descriptor        osr_kcd_descriptor;
	char                            *osr_kcd_buf;
} *os_reason_t;

#define OS_REASON_NULL ((os_reason_t) 0)

                                                                                                   
#define LAUNCHD_PANIC_REASON_STRING_MAXLEN "800"

void os_reason_init(void);

os_reason_t build_userspace_exit_reason(uint32_t reason_namespace, uint64_t reason_code, user_addr_t payload, uint32_t payload_size,
    user_addr_t reason_string, uint64_t reason_flags);
char *launchd_exit_reason_get_string_desc(os_reason_t exit_reason);

                                                                
int os_reason_alloc_buffer(os_reason_t cur_reason, uint32_t osr_bufsize);

#else                         

typedef void * os_reason_t;

#endif                         

os_reason_t os_reason_create(uint32_t osr_namespace, uint64_t osr_code);
int os_reason_alloc_buffer_noblock(os_reason_t cur_reason, uint32_t osr_bufsize);
struct kcdata_descriptor * os_reason_get_kcdata_descriptor(os_reason_t cur_reason);
void os_reason_ref(os_reason_t cur_reason);
void os_reason_free(os_reason_t cur_reason);
void os_reason_set_flags(os_reason_t cur_reason, uint64_t flags);
void os_reason_set_description_data(os_reason_t cur_reason, uint32_t type, void *reason_data, uint32_t reason_data_len);
#endif                     

  
                     
   
#define OS_REASON_INVALID       0
#define OS_REASON_JETSAM        1
#define OS_REASON_SIGNAL        2
#define OS_REASON_CODESIGNING   3
#define OS_REASON_HANGTRACER    4
#define OS_REASON_TEST          5
#define OS_REASON_DYLD          6
#define OS_REASON_LIBXPC        7
#define OS_REASON_OBJC          8
#define OS_REASON_EXEC          9
#define OS_REASON_SPRINGBOARD   10
#define OS_REASON_TCC           11
#define OS_REASON_REPORTCRASH   12
#define OS_REASON_COREANIMATION 13
#define OS_REASON_AGGREGATED    14
#define OS_REASON_RUNNINGBOARD  15
#define OS_REASON_ASSERTIOND    OS_REASON_RUNNINGBOARD                
#define OS_REASON_SKYWALK       16
#define OS_REASON_SETTINGS      17
#define OS_REASON_LIBSYSTEM     18
#define OS_REASON_FOUNDATION    19
#define OS_REASON_WATCHDOG      20
#define OS_REASON_METAL         21
#define OS_REASON_WATCHKIT      22
#define OS_REASON_GUARD         23
#define OS_REASON_ANALYTICS     24
#define OS_REASON_SANDBOX       25
#define OS_REASON_SECURITY      26
#define OS_REASON_ENDPOINTSECURITY      27
#define OS_REASON_PAC_EXCEPTION 28
#define OS_REASON_BLUETOOTH_CHIP 29

  
                                                      
   
#define OS_REASON_MAX_VALID_NAMESPACE OS_REASON_BLUETOOTH_CHIP

#define OS_REASON_BUFFER_MAX_SIZE 5120

#define OS_REASON_FLAG_NO_CRASH_REPORT          0x1                                    
#define OS_REASON_FLAG_GENERATE_CRASH_REPORT    0x2                                                                   
#define OS_REASON_FLAG_FROM_USERSPACE           0x4                                                
#define OS_REASON_FLAG_FAILED_DATA_COPYIN       0x8                                                
#define OS_REASON_FLAG_PAYLOAD_TRUNCATED        0x10                                                                    
#define OS_REASON_FLAG_BAD_PARAMS               0x20                                                                         
#define OS_REASON_FLAG_CONSISTENT_FAILURE       0x40                                                                   
#define OS_REASON_FLAG_ONE_TIME_FAILURE         0x80                                                                      
#define OS_REASON_FLAG_NO_CRASHED_TID           0x100                                                                        
#define OS_REASON_FLAG_ABORT                    0x200                                                          
#define OS_REASON_FLAG_SHAREDREGION_FAULT       0x400                                                    

  
                                                            
   
#define OS_REASON_FLAG_MASK_ALLOWED_FROM_USER (OS_REASON_FLAG_CONSISTENT_FAILURE | OS_REASON_FLAG_ONE_TIME_FAILURE | OS_REASON_FLAG_NO_CRASH_REPORT | OS_REASON_FLAG_ABORT)

  
                                                                                         
                                                                                     
                                             
   
#define ENCODE_OSR_NAMESPACE_TO_MACH_EXCEPTION_CODE(code, osr_namespace) \
	(code) = (code) | (((osr_namespace) & ((uint64_t)UINT32_MAX)) << 32)
#define ENCODE_OSR_CODE_TO_MACH_EXCEPTION_CODE(code, osr_code) \
	(code) = (code) | ((osr_code) & ((uint64_t)UINT32_MAX))

#ifndef KERNEL
  
                                                                     
                                                                            
  
                                                                                             
                                                                                          
                                                                                                       
                                                                                                                                     
  
                                        
   
void abort_with_reason(uint32_t reason_namespace, uint64_t reason_code, const char *reason_string, uint64_t reason_flags)
__attribute__((noreturn, cold));

  
                                                                      
                                                                                                 
                                                                                                 
                                               
  
                                                                                             
                                                                                          
                                                                                  
                                                                                                                             
                                                                                                       
                                                                                                                                     
  
                                        
   
void abort_with_payload(uint32_t reason_namespace, uint64_t reason_code, void *payload, uint32_t payload_size, const char *reason_string,
    uint64_t reason_flags) __attribute__((noreturn, cold));

  
                                                                             
                                                                                
  
                                                                           
                                                                                             
                                                                                          
                                                                                                       
                                                                                                                                    
  
                                                                                                                                             
                                                                                       
                                                                                                               
                                           
   
int terminate_with_reason(int pid, uint32_t reason_namespace, uint64_t reason_code, const char *reason_string, uint64_t reason_flags);

  
                                                                              
                                                                                                     
                                                                                                     
                                                   
  
                                                                            
                                                                                             
                                                                                          
                                                                                  
                                                                                                                             
                                                                                                       
                                                                                                                                    
  
                                                                                                                                                
                                                                                       
                                                                                                               
                                           
   
int terminate_with_payload(int pid, uint32_t reason_namespace, uint64_t reason_code, void *payload, uint32_t payload_size,
    const char *reason_string, uint64_t reason_flags);
#endif             

  
                           
   
#define CODESIGNING_EXIT_REASON_TASKGATED_INVALID_SIG 1
#define CODESIGNING_EXIT_REASON_INVALID_PAGE          2
#define CODESIGNING_EXIT_REASON_TASK_ACCESS_PORT      3

  
                                  
   
#define EXEC_EXIT_REASON_BAD_MACHO          1
#define EXEC_EXIT_REASON_SUGID_FAILURE      2
#define EXEC_EXIT_REASON_ACTV_THREADSTATE   3
#define EXEC_EXIT_REASON_STACK_ALLOC        4
#define EXEC_EXIT_REASON_APPLE_STRING_INIT  5
#define EXEC_EXIT_REASON_COPYOUT_STRINGS    6
#define EXEC_EXIT_REASON_COPYOUT_DYNLINKER  7
#define EXEC_EXIT_REASON_SECURITY_POLICY    8
#define EXEC_EXIT_REASON_TASKGATED_OTHER    9
#define EXEC_EXIT_REASON_FAIRPLAY_DECRYPT   10
#define EXEC_EXIT_REASON_DECRYPT            11
#define EXEC_EXIT_REASON_UPX                12
#define EXEC_EXIT_REASON_NO32EXEC           13
#define EXEC_EXIT_REASON_WRONG_PLATFORM     14
#define EXEC_EXIT_REASON_MAIN_FD_ALLOC      15
#define EXEC_EXIT_REASON_COPYOUT_ROSETTA    16
  
                
   
#define GUARD_REASON_VNODE       1
#define GUARD_REASON_VIRT_MEMORY 2
#define GUARD_REASON_MACH_PORT   3

__END_DECLS

#endif                 
