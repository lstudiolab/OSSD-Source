/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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

#ifndef _KERN_DEBUG_H_
#define _KERN_DEBUG_H_

#include <kern/kcdata.h>

#include <sys/cdefs.h>
#include <stdint.h>
#include <stdarg.h>
#include <uuid/uuid.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>

#ifndef XNU_KERNEL_PRIVATE
#include <TargetConditionals.h>
#endif

#ifdef __APPLE_API_PRIVATE
#ifdef __APPLE_API_UNSTABLE

struct thread_snapshot {
	uint32_t                snapshot_magic;
	uint32_t                nkern_frames;
	uint32_t                nuser_frames;
	uint64_t                wait_event;
	uint64_t                continuation;
	uint64_t                thread_id;
	uint64_t                user_time;
	uint64_t                system_time;
	int32_t                 state;
	int32_t                 priority;                         
	int32_t                 sched_pri;                                     
	int32_t                 sched_flags;                      
	char                    ss_flags;
	char                    ts_qos;                         
	char                    ts_rqos;                        
	char                    ts_rqos_override;                             
	char                    io_tier;
	char                    _reserved[3];                                        

	  
                  
                                      
    
	uint64_t                disk_reads_count;
	uint64_t                disk_reads_size;
	uint64_t                disk_writes_count;
	uint64_t                disk_writes_size;
	uint64_t                io_priority_count[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t                io_priority_size[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t                paging_count;
	uint64_t                paging_size;
	uint64_t                non_paging_count;
	uint64_t                non_paging_size;
	uint64_t                data_count;
	uint64_t                data_size;
	uint64_t                metadata_count;
	uint64_t                metadata_size;
	                             

	uint64_t                voucher_identifier;                                    
	uint64_t                total_syscalls;
	char                    pth_name[STACKSHOT_MAX_THREAD_NAME_SIZE];
} __attribute__((packed));

                            
struct task_snapshot {
	uint32_t snapshot_magic;
	int32_t pid;
	uint64_t                uniqueid;
	uint64_t                user_time_in_terminated_threads;
	uint64_t                system_time_in_terminated_threads;
	uint8_t                 shared_cache_identifier[16];
	uint64_t                shared_cache_slide;
	uint32_t                nloadinfos;
	int                     suspend_count;
	int                     task_size;                 
	int                     faults;                                    
	int                     pageins;                                      
	int                     cow_faults;                                         
	uint32_t                ss_flags;
	uint64_t                p_start_sec;                                  
	uint64_t                p_start_usec;                                 

	  
                                                 
                                       
                                                                   
                          
    
	char                    p_comm[17];
	uint32_t                was_throttled;
	uint32_t                did_throttle;
	uint32_t                latency_qos;
	  
                  
                                       
    
	uint64_t                disk_reads_count;
	uint64_t                disk_reads_size;
	uint64_t                disk_writes_count;
	uint64_t                disk_writes_size;
	uint64_t                io_priority_count[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t                io_priority_size[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t                paging_count;
	uint64_t                paging_size;
	uint64_t                non_paging_count;
	uint64_t                non_paging_size;
	uint64_t                data_count;
	uint64_t                data_size;
	uint64_t                metadata_count;
	uint64_t                metadata_size;
	                             

	uint32_t                donating_pid_count;
} __attribute__ ((packed));



struct micro_snapshot {
	uint32_t                snapshot_magic;
	uint32_t                ms_cpu;                                                
	uint64_t                ms_time;                               
	uint64_t                ms_time_microsecs;
	uint8_t                 ms_flags;
	uint16_t                ms_opaque_flags;                                                        
} __attribute__ ((packed));


  
                                                                                            
   
struct _dyld_cache_header {
	char        magic[16];                                                   
	uint32_t    mappingOffset;                                                     
	uint32_t    mappingCount;                                                   
	uint32_t    imagesOffset;                                                    
	uint32_t    imagesCount;                                                  
	uint64_t    dyldBaseAddress;                                                
	uint64_t    codeSignatureOffset;                                     
	uint64_t    codeSignatureSize;                                                            
	uint64_t    slideInfoOffset;                                      
	uint64_t    slideInfoSize;                                 
	uint64_t    localSymbolsOffset;                                                 
	uint64_t    localSymbolsSize;                                       
	uint8_t     uuid[16];                                                     
	uint64_t    cacheType;                                                
	uint32_t    branchPoolsOffset;                                                    
	uint32_t    branchPoolsCount;                                
	uint64_t    accelerateInfoAddr;                                         
	uint64_t    accelerateInfoSize;                             
	uint64_t    imagesTextOffset;                                                     
	uint64_t    imagesTextCount;                                                   
	uint64_t    dylibsImageGroupAddr;                                                          
	uint64_t    dylibsImageGroupSize;                                              
	uint64_t    otherImageGroupAddr;                                                     
	uint64_t    otherImageGroupSize;                                          
	uint64_t    progClosuresAddr;                                                         
	uint64_t    progClosuresSize;                                             
	uint64_t    progClosuresTrieAddr;                                                                   
	uint64_t    progClosuresTrieSize;                                                       
	uint32_t    platform;                                            
	uint32_t    formatVersion        : 8,                                 
	    dylibsExpectedOnDisk : 1,                                                                                                        
	    simulator            : 1,                                            
	    locallyBuiltCache    : 1,                                                         
	    padding              : 21;           
};

  
                                                                                                     
   
struct _dyld_cache_image_text_info {
	uuid_t      uuid;
	uint64_t    loadAddress;                                            
	uint32_t    textSegmentSize;
	uint32_t    pathOffset;                                           
};


enum micro_snapshot_flags {
	kInterruptRecord        = 0x1,
	kTimerArmingRecord      = 0x2,
	kUserMode               = 0x4,                                                 
	kIORecord               = 0x8,
	kPMIRecord              = 0x10,
};

  
                                                       
   
enum generic_snapshot_flags {
	kUser64_p               = 0x1,                                     
	kKernel64_p             = 0x2                                       
};

#define VM_PRESSURE_TIME_WINDOW 5              

__options_decl(stackshot_flags_t, uint64_t, {
	STACKSHOT_GET_DQ                           = 0x01,
	STACKSHOT_SAVE_LOADINFO                    = 0x02,
	STACKSHOT_GET_GLOBAL_MEM_STATS             = 0x04,
	STACKSHOT_SAVE_KEXT_LOADINFO               = 0x08,
	  
                                           
   
                                                                    
                
    
	STACKSHOT_ACTIVE_KERNEL_THREADS_ONLY       = 0x100,
	STACKSHOT_GET_BOOT_PROFILE                 = 0x200,
	STACKSHOT_DO_COMPRESS                      = 0x400,
	STACKSHOT_SAVE_IMP_DONATION_PIDS           = 0x2000,
	STACKSHOT_SAVE_IN_KERNEL_BUFFER            = 0x4000,
	STACKSHOT_RETRIEVE_EXISTING_BUFFER         = 0x8000,
	STACKSHOT_KCDATA_FORMAT                    = 0x10000,
	STACKSHOT_ENABLE_BT_FAULTING               = 0x20000,
	STACKSHOT_COLLECT_DELTA_SNAPSHOT           = 0x40000,
	                                                   
	STACKSHOT_COLLECT_SHAREDCACHE_LAYOUT       = 0x80000,
	  
                                                                          
                                                                        
    
	STACKSHOT_TRYLOCK                          = 0x100000,
	STACKSHOT_ENABLE_UUID_FAULTING             = 0x200000,
	STACKSHOT_FROM_PANIC                       = 0x400000,
	STACKSHOT_NO_IO_STATS                      = 0x800000,
	                                                                                 
	STACKSHOT_THREAD_WAITINFO                  = 0x1000000,
	STACKSHOT_THREAD_GROUP                     = 0x2000000,
	STACKSHOT_SAVE_JETSAM_COALITIONS           = 0x4000000,
	STACKSHOT_INSTRS_CYCLES                    = 0x8000000,
	STACKSHOT_ASID                             = 0x10000000,
	STACKSHOT_PAGE_TABLES                      = 0x20000000,
	STACKSHOT_DISABLE_LATENCY_INFO             = 0x40000000,
});

__options_decl(microstackshot_flags_t, uint32_t, {
	STACKSHOT_GET_MICROSTACKSHOT               = 0x10,
	STACKSHOT_GLOBAL_MICROSTACKSHOT_ENABLE     = 0x20,
	STACKSHOT_GLOBAL_MICROSTACKSHOT_DISABLE    = 0x40,
	STACKSHOT_SET_MICROSTACKSHOT_MARK          = 0x80,
});

#define STACKSHOT_THREAD_SNAPSHOT_MAGIC     0xfeedface
#define STACKSHOT_TASK_SNAPSHOT_MAGIC       0xdecafbad
#define STACKSHOT_MEM_AND_IO_SNAPSHOT_MAGIC 0xbfcabcde
#define STACKSHOT_MICRO_SNAPSHOT_MAGIC      0x31c54011

#define STACKSHOT_PAGETABLES_MASK_ALL           ~0

#define KF_INITIALIZED (0x1)
#define KF_SERIAL_OVRD (0x2)
#define KF_PMAPV_OVRD (0x4)
#define KF_MATV_OVRD (0x8)
#define KF_STACKSHOT_OVRD (0x10)
#define KF_COMPRSV_OVRD (0x20)
#define KF_INTERRUPT_MASKED_DEBUG_OVRD (0x40)
#define KF_TRAPTRACE_OVRD (0x80)
#define KF_IOTRACE_OVRD (0x100)
#define KF_INTERRUPT_MASKED_DEBUG_STACKSHOT_OVRD (0x200)

boolean_t kern_feature_override(uint32_t fmask);

#define EMBEDDED_PANIC_HEADER_OSVERSION_LEN 32

  
                                                                            
                                 
  
                                                                               
                                                                                    
                                                            
   
struct embedded_panic_header {
	uint32_t eph_magic;                                                   
	uint32_t eph_crc;                                                                                             
	uint32_t eph_version;                                                 
	uint64_t eph_panic_flags;                                                              
	uint32_t eph_panic_log_offset;                                                                                    
	uint32_t eph_panic_log_len;                                     
	uint32_t eph_stackshot_offset;                                                                                          
	uint32_t eph_stackshot_len;                                                             
	uint32_t eph_other_log_offset;                                                                                                             
	uint32_t eph_other_log_len;                                     
	union {
		struct {
			uint64_t eph_x86_power_state:8,
			    eph_x86_efi_boot_state:8,
			    eph_x86_system_state:8,
			    eph_x86_unused_bits:40;
		};                                                     
		uint64_t eph_x86_do_not_use;                                                   
	};
	char eph_os_version[EMBEDDED_PANIC_HEADER_OSVERSION_LEN];
	char eph_macos_version[EMBEDDED_PANIC_HEADER_OSVERSION_LEN];
} __attribute__((packed));

#define EMBEDDED_PANIC_HEADER_FLAG_COREDUMP_COMPLETE             0x01
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_SUCCEEDED           0x02
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_FAILED_DEBUGGERSYNC 0x04
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_FAILED_ERROR        0x08
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_FAILED_INCOMPLETE   0x10
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_FAILED_NESTED       0x20
#define EMBEDDED_PANIC_HEADER_FLAG_NESTED_PANIC                  0x40
#define EMBEDDED_PANIC_HEADER_FLAG_BUTTON_RESET_PANIC            0x80
#define EMBEDDED_PANIC_HEADER_FLAG_COPROC_INITIATED_PANIC        0x100
#define EMBEDDED_PANIC_HEADER_FLAG_COREDUMP_FAILED               0x200
#define EMBEDDED_PANIC_HEADER_FLAG_COMPRESS_FAILED               0x400
#define EMBEDDED_PANIC_HEADER_FLAG_STACKSHOT_DATA_COMPRESSED     0x800

#define EMBEDDED_PANIC_HEADER_CURRENT_VERSION 2
#define EMBEDDED_PANIC_MAGIC 0x46554E4B           

struct macos_panic_header {
	uint32_t mph_magic;                                                   
	uint32_t mph_crc;                                                                                             
	uint32_t mph_version;                                                 
	uint32_t mph_padding;                             
	uint64_t mph_panic_flags;                                                                 
	uint32_t mph_panic_log_offset;                                                                      
	uint32_t mph_panic_log_len;                                        
	uint32_t mph_stackshot_offset;                                                                      
	uint32_t mph_stackshot_len;                                        
	uint32_t mph_other_log_offset;                                                                                                                
	uint32_t mph_other_log_len;                                        
	char     mph_data[];                                                                                                                                      
} __attribute__((packed));

#define MACOS_PANIC_HEADER_CURRENT_VERSION 2
#define MACOS_PANIC_MAGIC 0x44454544           

#define MACOS_PANIC_HEADER_FLAG_NESTED_PANIC                  0x01
#define MACOS_PANIC_HEADER_FLAG_COPROC_INITIATED_PANIC        0x02
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_SUCCEEDED           0x04
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_DATA_COMPRESSED     0x08
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_FAILED_DEBUGGERSYNC 0x10
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_FAILED_ERROR        0x20
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_FAILED_INCOMPLETE   0x40
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_FAILED_NESTED       0x80
#define MACOS_PANIC_HEADER_FLAG_COREDUMP_COMPLETE             0x100
#define MACOS_PANIC_HEADER_FLAG_COREDUMP_FAILED               0x200
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_KERNEL_ONLY         0x400
#define MACOS_PANIC_HEADER_FLAG_STACKSHOT_FAILED_COMPRESS     0x800

  
                                                                                          
                   
   

struct efi_aurr_panic_header {
	uint32_t efi_aurr_magic;
	uint32_t efi_aurr_crc;
	uint32_t efi_aurr_version;
	uint32_t efi_aurr_reset_cause;
	uint32_t efi_aurr_reset_log_offset;
	uint32_t efi_aurr_reset_log_len;
	char efi_aurr_panic_data[];
} __attribute__((packed));

  
                                                                                                                          
   
#define EXTENDED_DEBUG_BUF_SIZE 0x0013ff80

#define EFI_AURR_PANIC_STRING_MAX_LEN 112
#define EFI_AURR_EXTENDED_LOG_SIZE (EXTENDED_DEBUG_BUF_SIZE - sizeof(struct efi_aurr_panic_header) - EFI_AURR_PANIC_STRING_MAX_LEN)

struct efi_aurr_extended_panic_log {
	char efi_aurr_extended_log_buf[EFI_AURR_EXTENDED_LOG_SIZE];
	uint32_t efi_aurr_log_tail;                              
	uint32_t efi_aurr_log_head;              
} __attribute__((packed));

#endif                           
#endif                          

#ifdef KERNEL

__BEGIN_DECLS

__abortlike __printflike(1, 2)
extern void panic(const char *string, ...);

__END_DECLS

#endif             

#ifdef KERNEL_PRIVATE
#if DEBUG
#ifndef DKPR
#define DKPR 1
#endif
#endif

#if DKPR
  
                                               
                                                
                                                  
                                                    
                                                   
                                                    
                                                  
                                          
   
extern int debug_kprint_syscall;
extern int debug_kprint_current_process(const char **namep);
#define DEBUG_KPRINT_SYSCALL_PREDICATE_INTERNAL(mask, namep)                    \
	( (debug_kprint_syscall & (mask)) && debug_kprint_current_process(namep) )
#define DEBUG_KPRINT_SYSCALL_MASK(mask, fmt, args...)   do {                    \
	        const char *dks_name = NULL;                                                                    \
	        if (DEBUG_KPRINT_SYSCALL_PREDICATE_INTERNAL(mask, &dks_name)) { \
	                kprintf("[%s%s%p]" fmt, dks_name ? dks_name : "",                       \
	                                dks_name ? "@" : "", current_thread(), args);                   \
	        }                                                                                                                               \
	} while (0)
#else             
#define DEBUG_KPRINT_SYSCALL_PREDICATE_INTERNAL(mask, namep) (0)
#define DEBUG_KPRINT_SYSCALL_MASK(mask, fmt, args...) do { } while (0)                         
#endif             

enum {
	DEBUG_KPRINT_SYSCALL_UNIX_MASK = 1 << 0,
	DEBUG_KPRINT_SYSCALL_MACH_MASK = 1 << 1,
	DEBUG_KPRINT_SYSCALL_MDEP_MASK = 1 << 2,
	DEBUG_KPRINT_SYSCALL_IPC_MASK  = 1 << 3
};

#define DEBUG_KPRINT_SYSCALL_PREDICATE(mask)                            \
	DEBUG_KPRINT_SYSCALL_PREDICATE_INTERNAL(mask, NULL)
#define DEBUG_KPRINT_SYSCALL_UNIX(fmt, args...)                         \
	DEBUG_KPRINT_SYSCALL_MASK(DEBUG_KPRINT_SYSCALL_UNIX_MASK,fmt,args)
#define DEBUG_KPRINT_SYSCALL_MACH(fmt, args...)                         \
	DEBUG_KPRINT_SYSCALL_MASK(DEBUG_KPRINT_SYSCALL_MACH_MASK,fmt,args)
#define DEBUG_KPRINT_SYSCALL_MDEP(fmt, args...)                         \
	DEBUG_KPRINT_SYSCALL_MASK(DEBUG_KPRINT_SYSCALL_MDEP_MASK,fmt,args)
#define DEBUG_KPRINT_SYSCALL_IPC(fmt, args...)                          \
	DEBUG_KPRINT_SYSCALL_MASK(DEBUG_KPRINT_SYSCALL_IPC_MASK,fmt,args)

                     
#define DB_HALT         0x1
                                         
#define DB_NMI          0x4
#define DB_KPRT         0x8
#define DB_KDB          0x10
#define DB_ARP          0x40
#define DB_KDP_BP_DIS   0x80
                                           
#define DB_KDP_GETC_ENA 0x200

#define DB_KERN_DUMP_ON_PANIC           0x400                                
#define DB_KERN_DUMP_ON_NMI             0x800                               
#define DB_DBG_POST_CORE                0x1000                                     
#define DB_PANICLOG_DUMP                0x2000                                     
#define DB_REBOOT_POST_CORE             0x4000                           
                                                                        
                                                
                                           
#define DB_NMI_BTN_ENA          0x8000                                             
                                                                        
#define DB_DISABLE_LOCAL_CORE   0x20000                                            
#define DB_DISABLE_GZIP_CORE    0x40000                                   
#define DB_DISABLE_CROSS_PANIC  0x80000                                               
                                                                               
                                                                                 
                                                     
                                    
#define DB_REBOOT_ALWAYS        0x100000                                         
#define DB_DISABLE_STACKSHOT_TO_DISK 0x200000                                              

  
                                                          
   
#define DEBUGGER_OPTION_NONE                        0x0ULL
#define DEBUGGER_OPTION_PANICLOGANDREBOOT           0x1ULL                                                      
#define DEBUGGER_OPTION_RECURPANIC_ENTRY            0x2ULL
#define DEBUGGER_OPTION_RECURPANIC_PRELOG           0x4ULL
#define DEBUGGER_OPTION_RECURPANIC_POSTLOG          0x8ULL
#define DEBUGGER_OPTION_RECURPANIC_POSTCORE         0x10ULL
#define DEBUGGER_OPTION_INITPROC_PANIC              0x20ULL
#define DEBUGGER_OPTION_COPROC_INITIATED_PANIC      0x40ULL                                        
#define DEBUGGER_OPTION_SKIP_LOCAL_COREDUMP         0x80ULL                                                       
#define DEBUGGER_OPTION_ATTEMPTCOREDUMPANDREBOOT    0x100ULL                                              
#define DEBUGGER_INTERNAL_OPTION_THREAD_BACKTRACE   0x200ULL                                                                
#define DEBUGGER_OPTION_PRINT_CPU_USAGE_PANICLOG    0x400ULL                                                  
#define DEBUGGER_OPTION_SKIP_PANICEND_CALLOUTS      0x800ULL                                                                                                  

#define DEBUGGER_INTERNAL_OPTIONS_MASK              (DEBUGGER_INTERNAL_OPTION_THREAD_BACKTRACE)

__BEGIN_DECLS

#define panic_plain(ex, ...)  (panic)(ex, ## __VA_ARGS__)

#define __STRINGIFY(x) #x
#define LINE_NUMBER(x) __STRINGIFY(x)
#define PANIC_LOCATION __FILE__ ":" LINE_NUMBER(__LINE__)

#if defined(__arm__) || defined(__arm64__)
#define panic(ex, ...)  ({ \
	        __asm__("" ::: "memory"); \
	        (panic)(# ex, ## __VA_ARGS__); \
	})
#else
#define panic(ex, ...)  ({ \
	        __asm__("" ::: "memory"); \
	        (panic)(# ex "@" PANIC_LOCATION, ## __VA_ARGS__); \
	})
#endif

__abortlike __printflike(4, 5)
void panic_with_options(unsigned int reason, void *ctx,
    uint64_t debugger_options_mask, const char *str, ...);
void Debugger(const char * message);
void populate_model_name(char *);

#if defined(__arm__) || defined(__arm64__)
                                                                                                     
void register_additional_panic_data_buffer(const char *producer_name, void *buf, int len);
#endif

unsigned panic_active(void);

__END_DECLS

#endif                      

#if XNU_KERNEL_PRIVATE

#if defined (__x86_64__)
struct thread;

__abortlike __printflike(5, 6)
void panic_with_thread_context(unsigned int reason, void *ctx,
    uint64_t debugger_options_mask, struct thread* th, const char *str, ...);
#endif

                                               
#define ADDITIONAL_PANIC_DATA_BUFFER_MAX_LEN 64

struct additional_panic_data_buffer {
	const char *producer_name;
	void *buf;
	int len;
};

extern struct additional_panic_data_buffer *panic_data_buffers;

boolean_t oslog_is_safe(void);
boolean_t debug_mode_active(void);
boolean_t stackshot_active(void);
void panic_stackshot_reset_state(void);

  
                                       
  
                                                                               
  
                                                                 
                                                                               
                                        
                                                     
                                                           
                                                                                                    
                                                                         
  
   
#ifdef __cplusplus
extern "C" {
#endif
kern_return_t
stack_snapshot_from_kernel(int pid, void *buf, uint32_t size, uint64_t flags,
    uint64_t delta_since_timestamp, uint32_t pagetable_mask, unsigned *bytes_traced);

  
                                                                     
                          
   
boolean_t on_device_corefile_enabled(void);

  
                                                                        
                          
   
boolean_t panic_stackshot_to_disk_enabled(void);

#ifdef __cplusplus
}
#endif

#if defined(__x86_64__)
extern char debug_buf[];
extern boolean_t coprocessor_paniclog_flush;
extern boolean_t extended_debug_log_enabled;
#endif                          

extern char     *debug_buf_base;

#if defined(XNU_TARGET_OS_BRIDGE)
extern uint64_t macos_panic_base;
extern unsigned int macos_panic_size;
#endif                                    

extern char     kernel_uuid_string[];
extern char     panic_disk_error_description[];
extern size_t   panic_disk_error_description_size;

extern unsigned char    *kernel_uuid;
extern unsigned int     debug_boot_arg;

extern boolean_t kernelcache_uuid_valid;
extern uuid_t kernelcache_uuid;
extern uuid_string_t kernelcache_uuid_string;

extern boolean_t pageablekc_uuid_valid;
extern uuid_t pageablekc_uuid;
extern uuid_string_t pageablekc_uuid_string;

extern boolean_t auxkc_uuid_valid;
extern uuid_t auxkc_uuid;
extern uuid_string_t auxkc_uuid_string;

#ifdef __cplusplus
extern "C" {
#endif

extern boolean_t        doprnt_hide_pointers;

#ifdef __cplusplus
}
#endif

extern unsigned int     halt_in_debugger;                                          
extern unsigned int     current_debugger;
#define NO_CUR_DB       0x0
#define KDP_CUR_DB      0x1

extern unsigned int     active_debugger;
extern unsigned int     kernel_debugger_entry_count;

extern unsigned int     panicDebugging;

extern const char       *debugger_panic_str;

extern char *debug_buf_ptr;
extern unsigned int debug_buf_size;

extern void debug_log_init(void);
extern void debug_putc(char);
extern boolean_t debug_is_current_cpu_in_panic_state(void);

  
                                                                         
                                                                           
                                  
   
extern void phys_carveout_init(void);

extern uintptr_t phys_carveout_pa;
extern size_t phys_carveout_size;

extern boolean_t kernel_debugging_allowed(void);

#if defined (__x86_64__)
extern void extended_debug_log_init(void);

int     packA(char *inbuf, uint32_t length, uint32_t buflen);
void    unpackA(char *inbuf, uint32_t length);

#define PANIC_STACKSHOT_BUFSIZE (1024 * 1024)

extern uintptr_t panic_stackshot_buf;
extern size_t panic_stackshot_buf_len;

extern size_t panic_stackshot_len;
#endif                           

void    SavePanicInfo(const char *message, void *panic_data, uint64_t panic_options);
void    paniclog_flush(void);
void    panic_display_zprint(void);
void    panic_display_kernel_aslr(void);
void    panic_display_hibb(void);
void    panic_display_model_name(void);
void    panic_display_kernel_uuid(void);
void    panic_display_process_name(void);
#if CONFIG_ZLEAKS
void    panic_display_ztrace(void);
#endif                    
#if CONFIG_ECC_LOGGING
void    panic_display_ecc_errors(void);
#endif                         

  
                  
  
                                                                          
                                
   
extern unsigned int not_in_kdp;

#define DEBUGGER_NO_CPU -1

typedef enum {
	DBOP_NONE,
	DBOP_STACKSHOT,
	DBOP_RESET_PGO_COUNTERS,
	DBOP_PANIC,
	DBOP_DEBUGGER,
	DBOP_BREAKPOINT,
} debugger_op;

kern_return_t DebuggerTrapWithState(debugger_op db_op, const char *db_message, const char *db_panic_str, va_list *db_panic_args,
    uint64_t db_panic_options, void *db_panic_data_ptr, boolean_t db_proceed_on_sync_failure, unsigned long db_panic_caller);
void handle_debugger_trap(unsigned int exception, unsigned int code, unsigned int subcode, void *state);

void DebuggerWithContext(unsigned int reason, void *ctx, const char *message, uint64_t debugger_options_mask);

#if DEBUG || DEVELOPMENT
                                   

enum{
	kInstanceFlagAddress    = 0x01UL,
	kInstanceFlagReferenced = 0x02UL,
	kInstanceFlags          = 0x03UL
};

#define INSTANCE_GET(x) ((x) & ~kInstanceFlags)
#define INSTANCE_PUT(x) ((x) ^ ~kInstanceFlags)

typedef void (*leak_site_proc)(void * refCon, uint32_t siteCount, uint32_t zoneSize,
    uintptr_t * backtrace, uint32_t btCount);

#ifdef __cplusplus
extern "C" {
#endif

extern kern_return_t
zone_leaks(const char * zoneName, uint32_t nameLen, leak_site_proc proc, void * refCon);

extern void
zone_leaks_scan(uintptr_t * instances, uint32_t count, uint32_t zoneSize, uint32_t * found);

#ifdef __cplusplus
}
#endif

const char *sysctl_debug_get_preoslog(size_t *size);

#endif                            
#endif                          

#endif                      
