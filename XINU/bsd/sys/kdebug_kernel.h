/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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

#ifndef BSD_SYS_KDEBUG_KERNEL_H
#define BSD_SYS_KDEBUG_KERNEL_H

#include <mach/boolean.h>
#include <mach/clock_types.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#ifdef KERNEL

  
                               
  
                                 
  
                                                  
  
       
                
    
                                                        
          
                           
          
                                          
    
   

#pragma mark - kernel tracepoints

  
                                                                          
                      
  
                                                                              
                                   
   

  
                                                               
   
#define KDBG(x, ...) KDBG_(, x, ## __VA_ARGS__, 4, 3, 2, 1, 0)

  
                                                                            
                                                         
   
#define KDBG_FILTERED(x, ...) KDBG_(_FILTERED, x, ## __VA_ARGS__, 4, 3, 2, 1, 0)

#ifdef KERNEL_PRIVATE

  
                                                                           
                                  
   
#define KDBG_RELEASE_NOPROCFILT(x, ...) \
	        KDBG_(_RELEASE_NOPROCFILT, x, ## __VA_ARGS__, 4, 3, 2, 1, 0)

#endif                     

  
                                                     
  
                                                                           
        
   
#define KDBG_RELEASE(x, ...) KDBG_(_RELEASE, x, ## __VA_ARGS__, 4, 3, 2, 1, 0)

  
                                
   
#define KDBG_DEBUG(x, ...) KDBG_(_DEBUG, x, ## __VA_ARGS__, 4, 3, 2, 1, 0)

#pragma mark - kernel API

#ifdef KERNEL_PRIVATE

  
                                                             
                                                             
                                                             
                                                             
                                   
  
                                                   
   
int kernel_debug_string(uint32_t debugid, uint64_t *str_id, const char *str);

  
                                                                      
          
   
void kernel_debug_disable(void);

#endif                     

  
                                                                            
             
   
bool kdebug_using_continuous_time(void);

  
                                                                          
                   
   
bool kdebug_debugid_enabled(uint32_t debugid);

  
                                                                              
                                                         
   
bool kdebug_debugid_explicitly_enabled(uint32_t debugid);

uint32_t kdebug_commpage_state(void);

#pragma mark - IOP tracing

  
                                      
   

typedef enum {
	                                          
	KD_CALLBACK_KDEBUG_ENABLED,
	                                           
	KD_CALLBACK_KDEBUG_DISABLED,
	  
                                                                        
              
    
	KD_CALLBACK_SYNC_FLUSH,
	  
                                                                       
                                               
    
	KD_CALLBACK_TYPEFILTER_CHANGED,
} kd_callback_type;

typedef void (*kd_callback_fn) (void *context, kd_callback_type reason,
    void *arg);

struct kd_callback {
	kd_callback_fn func;
	void *context;
	                                 
	char iop_name[8];
};

typedef struct kd_callback kd_callback_t;

  
                                                 
  
                                                           
                                                        
                                                     
  
                                                             
                                                               
                                   
  
           
                                                             
                                                           
                  
   
int kernel_debug_register_callback(kd_callback_t callback);

void kernel_debug_enter(uint32_t coreid, uint32_t debugid, uint64_t timestamp,
    uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
    uintptr_t threadid);

#pragma mark - internals

#define KDBG_(f, x, a, b, c, d, n, ...) KDBG##n(f, x, a, b, c, d)
#define KDBG0(f, x, a, b, c, d) KERNEL_DEBUG_CONSTANT##f(x, 0, 0, 0, 0, 0)
#define KDBG1(f, x, a, b, c, d) KERNEL_DEBUG_CONSTANT##f(x, a, 0, 0, 0, 0)
#define KDBG2(f, x, a, b, c, d) KERNEL_DEBUG_CONSTANT##f(x, a, b, 0, 0, 0)
#define KDBG3(f, x, a, b, c, d) KERNEL_DEBUG_CONSTANT##f(x, a, b, c, 0, 0)
#define KDBG4(f, x, a, b, c, d) KERNEL_DEBUG_CONSTANT##f(x, a, b, c, d, 0)

#ifdef XNU_KERNEL_PRIVATE
#define KDBG_IMPROBABLE __improbable
#else
#define KDBG_IMPROBABLE
#endif

extern unsigned int kdebug_enable;

  
                                                                               
                                                    
  
                                                                        
                                                                                
        
   
#define KDEBUG_LEVEL_NONE     0
#define KDEBUG_LEVEL_IST      1
#define KDEBUG_LEVEL_STANDARD 2
#define KDEBUG_LEVEL_FULL     3

#if NO_KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_NONE
#elif IST_KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_IST
#elif KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_FULL
#else
#define KDEBUG_LEVEL KDEBUG_LEVEL_STANDARD
  
                                                                           
                         
   
#endif

  
                                                                             
                                                                         
                                
   
#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_STANDARD)
#define KERNEL_DEBUG_CONSTANT_FILTERED(x, a, b, c, d, ...)           \
	do {                                                             \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {   \
	                kernel_debug_filtered((x), (uintptr_t)(a), (uintptr_t)(b),  \
	                        (uintptr_t)(c), (uintptr_t)(d)); \
	        }                                                            \
	} while (0)
#else                                              
#define KERNEL_DEBUG_CONSTANT_FILTERED(type, x, a, b, c, d, ...) do {} while (0)
#endif                                              

#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_IST)
#define KERNEL_DEBUG_CONSTANT_RELEASE_NOPROCFILT(x, a, b, c, d, ...)   \
	do {                                                               \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {     \
	                kernel_debug_flags((x), (uintptr_t)(a), (uintptr_t)(b),    \
	                        (uintptr_t)(c), (uintptr_t)(d), KDBG_FLAG_NOPROCFILT); \
	        }                                                              \
	} while (0)
#else                                         
#define KERNEL_DEBUG_CONSTANT_RELEASE_NOPROCFILT(x, a, b, c, d, ...) \
	do { } while (0)
#endif                                         


#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_STANDARD)
#define KERNEL_DEBUG_CONSTANT(x, a, b, c, d, e)                               \
	do {                                                                      \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {            \
	                kernel_debug((x), (uintptr_t)(a), (uintptr_t)(b), (uintptr_t)(c), \
	                        (uintptr_t)(d),(uintptr_t)(e));                               \
	        }                                                                     \
	} while (0)

  
                                                                              
                                                                              
                                                   
   
#define KERNEL_DEBUG_CONSTANT1(x, a, b, c, d, e)                               \
	do {                                                                       \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {             \
	                kernel_debug1((x), (uintptr_t)(a), (uintptr_t)(b), (uintptr_t)(c), \
	                (uintptr_t)(d), (uintptr_t)(e));                                   \
	        }                                                                      \
	} while (0)

#else                                              
#define KERNEL_DEBUG_CONSTANT(x, a, b, c, d, e) do {} while (0)
#define KERNEL_DEBUG_CONSTANT1(x, a, b, c, d, e) do {} while (0)
#endif                                              

  
                                                                               
                                                                            
                                                                              
                                                                              
                                                                             
                                 
   
#define KERNEL_DEBUG_CONSTANT_RELEASE(x, a, b, c, d, e) \
	KERNEL_DEBUG_CONSTANT_IST(~KDEBUG_ENABLE_PPT, x, a, b, c, d, 0)

#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_IST)
#define KERNEL_DEBUG_CONSTANT_IST(type, x, a, b, c, d, e)                     \
	do {                                                                      \
	        if (KDBG_IMPROBABLE(kdebug_enable & (type))) {                        \
	                kernel_debug((x), (uintptr_t)(a), (uintptr_t)(b), (uintptr_t)(c), \
	                        (uintptr_t)(d), 0);                                           \
	        }                                                                     \
	} while (0)

#define KERNEL_DEBUG_CONSTANT_IST1(x, a, b, c, d, e)                     \
	do {                                                                       \
	        if (KDBG_IMPROBABLE(kdebug_enable)) {                         \
	                kernel_debug1((x), (uintptr_t)(a), (uintptr_t)(b), (uintptr_t)(c), \
	                        (uintptr_t)(d), (uintptr_t)(e));                               \
	        }                                                                      \
	} while (0)

#define KERNEL_DEBUG_EARLY(x, a, b, c, d)                                 \
	do {                                                                  \
	        kernel_debug_early((uint32_t)(x), (uintptr_t)(a), (uintptr_t)(b), \
	                (uintptr_t)(c), (uintptr_t)(d));                              \
	} while (0)

#else                                         
#define KERNEL_DEBUG_CONSTANT_IST(type, x, a, b, c, d, e) do {} while (0)
#define KERNEL_DEBUG_CONSTANT_IST1(x, a, b, c, d, e) do {} while (0)
#define KERNEL_DEBUG_EARLY(x, a, b, c, d) do {} while (0)
#endif                                         

#if NO_KDEBUG
#define __kdebug_constant_only __unused
#endif

  
                                                         
   
#define KERNEL_DEBUG_CONSTANT_DEBUG(x, a, b, c, d, e) \
	KERNEL_DEBUG(x, a, b, c, d, e)

#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_FULL)
#define __kdebug_only

#undef KERNEL_DEBUG
#define KERNEL_DEBUG(x, a, b, c, d, e)                                  \
	do {                                                                \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {      \
	                kernel_debug((uint32_t)(x), (uintptr_t)(a), (uintptr_t)(b), \
	                        (uintptr_t)(c), (uintptr_t)(d), (uintptr_t)(e));        \
	        }                                                               \
	} while (0)

  
                                                                         
   
#define KERNEL_DEBUG1(x, a, b, c, d, e)                                  \
	do {                                                                 \
	        if (KDBG_IMPROBABLE(kdebug_enable & ~KDEBUG_ENABLE_PPT)) {       \
	                kernel_debug1((uint32_t)(x), (uintptr_t)(a), (uintptr_t)(b), \
	                        (uintptr_t)(c), (uintptr_t)(d), (uintptr_t)(e));         \
	        }                                                                \
	} while (0)

#else                                          
#define __kdebug_only __unused

#undef KERNEL_DEBUG
#define KERNEL_DEBUG(x, a, b, c, d, e) do {} while (0)
#define KERNEL_DEBUG1(x, a, b, c, d, e) do {} while (0)
#endif                                          

void kernel_debug(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

void kernel_debug1(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

#define KDBG_FLAG_FILTERED 0x01
#define KDBG_FLAG_NOPROCFILT 0x02

void kernel_debug_flags(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uint64_t flags);

void kernel_debug_filtered(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4);

#pragma mark - xnu API

#ifdef XNU_KERNEL_PRIVATE

                                       
void kernel_debug_early(uint32_t  debugid, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4);
                                                                          
void kernel_debug_string_early(const char *message);
                                                                            
void kernel_debug_string_simple(uint32_t eventid, const char *str);
                                                                     
extern void kdebug_reset(void);

void kdbg_dump_trace_to_file(const char *);

enum kdebug_opts {
	KDOPT_WRAPPING = 0x1,
	KDOPT_ATBOOT = 0x2,
};

void kdebug_init(unsigned int n_events, char *filterdesc,
    enum kdebug_opts opts);
void kdebug_trace_start(unsigned int n_events, const char *filterdesc,
    enum kdebug_opts opts);
uint64_t kdebug_wake(void);
void kdebug_free_early_buf(void);
void release_storage_unit(int cpu, uint32_t storage_unit);
bool allocate_storage_unit(int cpu);

struct proc;
void kdbg_trace_data(struct proc *proc, long *arg_pid, long *arg_uniqueid);
void kdbg_trace_string(struct proc *proc, long *arg1, long *arg2, long *arg3,
    long *arg4);

#define KDBG_VFS_LOOKUP_FLAG_LOOKUP 0x01
#define KDBG_VFS_LOOKUP_FLAG_NOPROCFILT 0x02
void kdebug_vfs_lookup(unsigned long *path_words, int path_len, void *vnp,
    uint32_t flags);

#endif                         

#ifdef KERNEL_PRIVATE

#define NUMPARMS 23
void kdebug_lookup_gen_events(long *path_words, int path_len, void *vnp,
    bool lookup);

#pragma mark - EnergyTracing

#define KERNEL_DBG_IST_SANE KDBG_RELEASE
#define ENTR_KDTRACEFUNC KDBG_RELEASE

                                        
#define KERNEL_ENERGYTRACE(opcode, lifespan, id, quality, value)        \
	    ENTR_KDTRACE(kEnTrCompKernel, opcode, lifespan, id,         \
	                 quality, value)
#define KERNEL_ENTR_ASSOCIATE(par_opcode, par_act_id, sub_opcode, sub_act_id) \
	    ENTR_KDASSOCIATE(kEnTrCompKernel, par_opcode, par_act_id,   \
	                     kEnTrCompKernel, sub_opcode, sub_act_id)

#endif                     

#endif             

__END_DECLS

#endif                                        
