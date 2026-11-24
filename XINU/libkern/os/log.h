/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef __os_log_h
#define __os_log_h

#include <os/object.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_attribute(not_tail_called)
#define OS_LOG_NOTAILCALL __attribute__((not_tail_called))
#define OS_LOG_NOTAILCALL_MARKER
#else
#define OS_LOG_NOTAILCALL
#define OS_LOG_NOTAILCALL_MARKER __asm__("")
#endif

__BEGIN_DECLS

extern void *__dso_handle;

#ifdef XNU_KERNEL_PRIVATE
extern bool startup_serial_logging_active;
extern uint64_t startup_serial_num_procs;
#endif                         

#ifdef KERNEL
#define OS_LOG_BUFFER_MAX_SIZE 256
#else
#define OS_LOG_BUFFER_MAX_SIZE 1024
#endif

                                                              
                                                 
#define OS_LOG_DATA_MAX_SIZE (OS_LOG_BUFFER_MAX_SIZE - 16)

OS_ALWAYS_INLINE static inline void _os_log_verify_format_str(__unused const char *msg, ...) __attribute__((format(os_log, 1, 2)));
OS_ALWAYS_INLINE static inline void
_os_log_verify_format_str(__unused const char *msg, ...)                                                        
{
}

#if OS_OBJECT_USE_OBJC
OS_OBJECT_DECL(os_log);
#else
typedef struct os_log_s *os_log_t;
#endif                         

   
                         
  
              
                                              
   
#define OS_LOG_DISABLED NULL

   
                        
  
              
                                                                        
   
#define OS_LOG_DEFAULT OS_OBJECT_GLOBAL_OBJECT(os_log_t, _os_log_default)
__OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_10_0)
OS_EXPORT
struct os_log_s _os_log_default;

   
                      
  
              
                               
  
                                
                                                                                  
                              
  
                             
                                                                                         
  
                              
                                                                       
  
                              
                                                                                     
  
                              
                                                                                    
                                                                           
   
OS_ENUM(os_log_type, uint8_t,
    OS_LOG_TYPE_DEFAULT = 0x00,
    OS_LOG_TYPE_INFO    = 0x01,
    OS_LOG_TYPE_DEBUG   = 0x02,
    OS_LOG_TYPE_ERROR   = 0x10,
    OS_LOG_TYPE_FAULT   = 0x11);

   
                          
  
            
                                                                    
  
              
                                                                         
                                                                         
                                                                       
                         
  
                                                                         
                                                                       
                                                                        
                                                              
  
           
  
                                                                
  
                                                                                              
  
         
  
                                                                                                       
                                      
                  
                                                                                              
                                        
                                                                                                                  
                                   
                                                                                                                 
                                                                                                                         
                   
  
                                                 
                                  
                  
  
                                                                                                    
                                  
                      
                                                                               
                                        
                                                                                                  
                                         
                       
  
                                                                                       
                               
                      
                                                                                   
                                        
                                                                                       
                                       
                       
  
                                                                                        
                                
                      
                                                                                                  
                                       
                       
                   
               
  
                                                                                     
  
                   
                                                                      
                                                                           
                             
  
                  
                                                                          
                                                                          
             
  
          
                                                                          
                                                                           
                                                                           
                                       
  
                                                                   
   
__OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_10_0)
OS_EXPORT OS_NOTHROW OS_WARN_RESULT OS_OBJECT_RETURNS_RETAINED
os_log_t
os_log_create(const char *subsystem, const char *category);

   
                                
  
            
                                                                               
  
              
                                                                               
  
             
                                                                             
  
          
                                                    
   
__WATCHOS_AVAILABLE(3.0) __OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_AVAILABLE(10.0)
OS_EXPORT OS_NOTHROW OS_WARN_RESULT
bool
os_log_info_enabled(os_log_t log);

   
                                 
  
            
                                                                         
  
              
                                                                         
  
             
                                                                             
  
          
                                                    
   
__WATCHOS_AVAILABLE(3.0) __OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_AVAILABLE(10.0)
OS_EXPORT OS_NOTHROW OS_WARN_RESULT
bool
os_log_debug_enabled(os_log_t log);

   
                   
  
            
                                                                    
  
              
                                                                      
                                                                        
                                                                         
                     
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log(log, format, ...) __extension__({                                                \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");             \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;       \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                           \
    _os_log_internal(&__dso_handle, log, OS_LOG_TYPE_DEFAULT, _os_log_fmt, ##__VA_ARGS__);      \
    __asm__("");                                                                                \
})

   
                        
  
            
                                                                                
  
              
                                                                      
                                                                        
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_info(log, format, ...) __extension__({                                       \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");         \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;   \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                       \
    _os_log_internal(&__dso_handle, log, OS_LOG_TYPE_INFO, _os_log_fmt, ##__VA_ARGS__);     \
    __asm__("");                                                                            \
})

   
                         
  
            
                                                                          
  
              
                                                                            
                                                                        
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_debug(log, format, ...) __extension__({                                          \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");             \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;       \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                           \
    _os_log_internal(&__dso_handle, log, OS_LOG_TYPE_DEBUG, _os_log_fmt, ##__VA_ARGS__);        \
    __asm__("");                                                                                \
})

   
                         
  
            
                                                                           
  
              
                                                                           
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_error(log, format, ...) __extension__({                                          \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");             \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;       \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                           \
    _os_log_internal(&__dso_handle, log, OS_LOG_TYPE_ERROR, _os_log_fmt, ##__VA_ARGS__);        \
    __asm__("");                                                                                \
})

   
                         
  
            
                                                                          
  
              
                                                                        
                                                                        
                                                                            
                                                                    
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_fault(log, format, ...) __extension__({                                          \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");             \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;       \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                           \
    _os_log_internal(&__dso_handle, log, OS_LOG_TYPE_FAULT, _os_log_fmt, ##__VA_ARGS__);        \
    __asm__("");                                                                                \
})

   
                             
  
            
                                       
  
              
                                                      
  
             
                                                                             
  
              
                                        
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_with_type(log, type, format, ...) __extension__({                            \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");         \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;   \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                       \
    _os_log_internal(&__dso_handle, log, type, _os_log_fmt, ##__VA_ARGS__);                 \
    __asm__("");                                                                            \
})

   
                             
  
            
                                                                                      
  
              
                                                      
  
             
                                                                             
  
              
                                        
  
                
                                                                        
                                                                           
                                                                   
  
          
                                                                                
   
#define os_log_driverKit(out, log, type, format, ...) __extension__({                            \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");         \
    __attribute__((section("__TEXT,__os_log"))) static const char _os_log_fmt[] = format;   \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                       \
    (*(out)) = _os_log_internal_driverKit(&__dso_handle, log, type, _os_log_fmt, ##__VA_ARGS__);                 \
    __asm__("");                                                                            \
})

   
                               
  
            
                                                        
                                         
   
bool
os_log_coprocessor(void *buff, uint64_t buff_len, os_log_type_t type,
    const char *uuid, uint64_t timestamp, uint32_t offset, bool stream_log);

   
                                        
  
            
                                                             
                                         
   
void
os_log_coprocessor_register(const char *uuid, const char *file_path, bool copy);

   
                                   
  
            
                                                                          
                            
  
              
                                                                          
                                                                           
                           
  
                                                                     
                                                                      
                                                             
  
                                                                              
                                                                          
                                
  
                                                                      
                                                                           
                                                                            
                                                       
  
             
                                                                             
  
                
                                                                        
                                                                           
                                                                   
   
#define os_log_sensitive_debug(log, format, ...) __extension__({                                                      \
    _Static_assert(__builtin_constant_p(format), "format string must be constant");                                   \
    __attribute__((section("__TEXT,__os_log_sens"))) static const char _os_log_fmt[] = format;                        \
    _os_log_verify_format_str(format, ##__VA_ARGS__);                                                                 \
    _os_log_sensitive(&__dso_handle, log, OS_LOG_TYPE_DEBUG, _os_log_fmt, ##__VA_ARGS__);                             \
    __asm__("");                                                                                                      \
})

#ifdef XNU_KERNEL_PRIVATE
#define os_log_with_startup_serial(log, format, ...) __extension__({                                                  \
    if (startup_serial_logging_active) { printf(format, ##__VA_ARGS__); }                                             \
    else { os_log(log, format, ##__VA_ARGS__); }                                                                      \
})
#endif                         

   
                             
  
            
                                    
   
__WATCHOS_AVAILABLE(3.0) __OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_AVAILABLE(10.0)
OS_EXPORT OS_NOTHROW
void
_os_log_internal(void *dso, os_log_t log, os_log_type_t type, const char *message, ...);

   
                                       
  
            
                                    
   
__WATCHOS_AVAILABLE(6.0) __OSX_AVAILABLE(10.15) __IOS_AVAILABLE(13.0) __TVOS_AVAILABLE(13.0)
OS_EXPORT OS_NOTHROW
int
_os_log_internal_driverKit(void *dso, os_log_t log, os_log_type_t type, const char *message, ...);
__END_DECLS

#endif                 
