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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright 1995 NeXT Computer, Inc. All rights reserved. */
/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
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
 *	@(#)cdefs.h	8.8 (Berkeley) 1/9/95
 */

#ifndef _CDEFS_H_
#define _CDEFS_H_

#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

                                                                   
                                        
#if !defined(__GNUC__) || __GNUC__ < 4
#warning "Unsupported compiler detected"
#endif

  
                                                                            
                                         
   
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#ifndef __has_include
#define __has_include(x) 0
#endif
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif

  
                                                                        
                                                                       
                                                                       
                                                                         
                                                                           
   
#if defined(__STDC__) || defined(__cplusplus)
#define __P(protos)     protos                                 
#define __CONCAT(x, y)   x ## y
#define __STRING(x)     #x

#define __const         const                                                  
#define __signed        signed
#define __volatile      volatile
#if defined(__cplusplus)
#define __inline        inline                                      
#else
#ifndef __GNUC__
#define __inline                                                
#endif                
#endif                   

#else                                   
#define __P(protos)     ()                                              
#define __CONCAT(x, y)   x      y
#define __STRING(x)     "x"

#ifndef __GNUC__
#define __const                                                            
#define __inline
#define __signed
#define __volatile
#endif                 

  
                                                                          
                                                                       
                                                                           
                                                                             
                                                                       
                                                                            
   
#ifndef NO_ANSI_KEYWORDS
#define const           __const                                              
#define inline          __inline
#define signed          __signed
#define volatile        __volatile
#endif                        
#endif                                 

#define __dead2         __attribute__((__noreturn__))
#define __pure2         __attribute__((__const__))

                                                                            
                                                  
   
#define __unused        __attribute__((__unused__))

                                                                          
                                                                         
   
#define __used          __attribute__((__used__))

                                                               
                                                                
   
#if __has_attribute(cold)
#define __cold          __attribute__((__cold__))
#else
#define __cold
#endif

                                                                       
                         
                                                                       
                                                                            
   
#define __exported                      __attribute__((__visibility__("default")))
#define __exported_push         _Pragma("GCC visibility push(default)")
#define __exported_pop          _Pragma("GCC visibility pop")

                                                                          
                                           
                                                                             
                                                
                                                                       
                                                                              
                                         
                                                                              
                                                
   
#define __deprecated    __attribute__((__deprecated__))

#if __has_extension(attribute_deprecated_with_message) || \
        (defined(__GNUC__) && ((__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))))
	#define __deprecated_msg(_msg) __attribute__((__deprecated__(_msg)))
#else
	#define __deprecated_msg(_msg) __attribute__((__deprecated__))
#endif

#if __has_extension(enumerator_attributes)
	#define __deprecated_enum_msg(_msg) __deprecated_msg(_msg)
#else
	#define __deprecated_enum_msg(_msg)
#endif

#if defined(KERNEL) && !defined(KERNEL_PRIVATE)
#define __kpi_deprecated(_msg) __deprecated_msg(_msg)
#else                                                  
#define __kpi_deprecated(_msg)
#endif                                                  

                                                                   
                                 
   
#if __has_attribute(unavailable)
#define __unavailable __attribute__((__unavailable__))
#else
#define __unavailable
#endif

#if defined(KERNEL) && !defined(KERNEL_PRIVATE)
#define __kpi_unavailable __unavailable
#else                                                  
#define __kpi_unavailable
#endif                                                  

#if defined(KERNEL)
#if defined(XNU_KERNEL_PRIVATE)
                                                                       
                                                             
                                                                
                                            
                                            
                                             
   
#define __kpi_deprecated_arm64_macos_unavailable
#elif defined(KERNEL_PRIVATE)
#define __kpi_deprecated_arm64_macos_unavailable __deprecated
#else                                   
#if TARGET_OS_OSX && defined(__arm64__)
#define __kpi_deprecated_arm64_macos_unavailable __unavailable
#else                                            
#define __kpi_deprecated_arm64_macos_unavailable __deprecated
#endif                                            
#endif                                   
#else                       
#define __kpi_deprecated_arm64_macos_unavailable
#endif                       

                                                                       
#ifndef __dead
#define __dead
#define __pure
#endif

  
                                                                       
                                                                     
   
#if __STDC_VERSION__ < 199901
#define __restrict
#else
#define __restrict      restrict
#endif

                                                                       
                       
   

#if !__has_feature(nullability)
#ifndef __nullable
#define __nullable
#endif
#ifndef __nonnull
#define __nonnull
#endif
#ifndef __null_unspecified
#define __null_unspecified
#endif
#ifndef _Nullable
#define _Nullable
#endif
#ifndef _Nonnull
#define _Nonnull
#endif
#ifndef _Null_unspecified
#define _Null_unspecified
#endif
#endif

  
                                                                    
                                           
   
#if __has_attribute(disable_tail_calls)
#define __disable_tail_calls    __attribute__((__disable_tail_calls__))
#else
#define __disable_tail_calls
#endif

  
                                                                          
                                                                           
                                                                          
                                                         
   
#if __has_attribute(not_tail_called)
#define __not_tail_called       __attribute__((__not_tail_called__))
#else
#define __not_tail_called
#endif

  
                                                                             
                                                                            
               
   
#if __has_attribute(warn_unused_result)
#define __result_use_check __attribute__((__warn_unused_result__))
#else
#define __result_use_check
#endif

  
                                                                           
                                                                   
   
#if __has_feature(attribute_availability_swift)
#define __swift_unavailable(_msg)       __attribute__((__availability__(swift, unavailable, message=_msg)))
#else
#define __swift_unavailable(_msg)
#endif

  
                                                                         
                                                                
                                                      
   
#ifndef __abortlike
#define __abortlike __dead2 __cold __not_tail_called
#endif

                                                                              
                                                                             
                                                                           
                                                                        
                                                    
  
                                                                            
                                                                             
                                                                            
           
  
                                                                        
                                    
                                                    
   

#if defined(__cplusplus) || \
        (__STDC_VERSION__ >= 199901L && \
        !defined(__GNUC_GNU_INLINE__) && \
        (!defined(__GNUC__) || defined(__clang__)))
# define __header_inline           inline
#elif defined(__GNUC__) && defined(__GNUC_STDC_INLINE__)
# define __header_inline           extern __inline __attribute__((__gnu_inline__))
#elif defined(__GNUC__)
# define __header_inline           extern __inline
#else
                                                              
                                                             
   
# define __header_inline           static __inline
#endif

#ifdef __GNUC__
# define __header_always_inline    __header_inline __attribute__ ((__always_inline__))
#else
                                                                           
                    
   
# define __header_always_inline    __header_inline
#endif

  
                                                                    
                                                                        
   
#if defined(__clang__)
# define __unreachable_ok_push \
	 _Pragma("clang diagnostic push") \
	 _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
# define __unreachable_ok_pop \
	 _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# define __unreachable_ok_push \
	 _Pragma("GCC diagnostic push") \
	 _Pragma("GCC diagnostic ignored \"-Wunreachable-code\"")
# define __unreachable_ok_pop \
	 _Pragma("GCC diagnostic pop")
#else
# define __unreachable_ok_push
# define __unreachable_ok_pop
#endif

  
                                                                       
                                                                     
                                                                       
                                                                      
                                                                       
         
   
#define __printflike(fmtarg, firstvararg) \
	        __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#define __printf0like(fmtarg, firstvararg) \
	        __attribute__((__format__ (__printf0__, fmtarg, firstvararg)))
#define __scanflike(fmtarg, firstvararg) \
	        __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))

#define __IDSTRING(name, string) static const char name[] __used = string

#ifndef __COPYRIGHT
#define __COPYRIGHT(s) __IDSTRING(copyright,s)
#endif

#ifndef __RCSID
#define __RCSID(s) __IDSTRING(rcsid,s)
#endif

#ifndef __SCCSID
#define __SCCSID(s) __IDSTRING(sccsid,s)
#endif

#ifndef __PROJECT_VERSION
#define __PROJECT_VERSION(s) __IDSTRING(project_version,s)
#endif

                                                                     
#ifndef __FBSDID
#define __FBSDID(s)
#endif

#ifndef __DECONST
#define __DECONST(type, var)    __CAST_AWAY_QUALIFIER(var, const, type)
#endif

#ifndef __DEVOLATILE
#define __DEVOLATILE(type, var) __CAST_AWAY_QUALIFIER(var, volatile, type)
#endif

#ifndef __DEQUALIFY
#define __DEQUALIFY(type, var)  __CAST_AWAY_QUALIFIER(var, const volatile, type)
#endif

  
                                                                          
                                                                           
                                                                          
                                  
  
                                               
  
                                                                             
                            
  
                                                                 
   
#ifndef __alloc_size
#if __has_attribute(alloc_size)
#define __alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
#else
#define __alloc_size(...)
#endif
#endif                

  
                                                                  
  
                                                                  
                                  
  
                                                             
                                                      
  
                                                                    
                            
  
                                                                 
                                                                 
                           
  
                                                              
  
                                                                 
                                                            
                                                       
  
                                                                  
                                                             
  
                                                             
                                                           
                                                           
                                                                 
  
                                                                          
                                                                          
                                                                          
                                                                         
                                     
   

                                                    
#ifdef KERNEL
#define __DARWIN_ONLY_64_BIT_INO_T      0
#define __DARWIN_ONLY_UNIX_CONFORMANCE  0
#define __DARWIN_ONLY_VERS_1050         0
#if defined(__x86_64__)
#define __DARWIN_SUF_DARWIN14   "_darwin14"
#define __DARWIN14_ALIAS(sym)   __asm("_" __STRING(sym) __DARWIN_SUF_DARWIN14)
#else
#define __DARWIN14_ALIAS(sym)
#endif
#else              
#ifdef PLATFORM_iPhoneOS
                        
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                        
#ifdef PLATFORM_iPhoneSimulator
                               
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                               
#ifdef PLATFORM_tvOS
                    
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                    
#ifdef PLATFORM_AppleTVOS
                         
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                         
#ifdef PLATFORM_tvSimulator
                           
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                           
#ifdef PLATFORM_AppleTVSimulator
                                
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                                
#ifdef PLATFORM_iPhoneOSNano
                            
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                            
#ifdef PLATFORM_iPhoneNanoSimulator
                                   
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                                   
#ifdef PLATFORM_WatchOS
                       
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                       
#ifdef PLATFORM_WatchSimulator
                              
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                              
#ifdef PLATFORM_BridgeOS
                        
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                        
#ifdef PLATFORM_DriverKit
                         
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif                         
#ifdef PLATFORM_MacOSX
                      
#if defined(__i386__)
#define __DARWIN_ONLY_64_BIT_INO_T      0
#define __DARWIN_ONLY_UNIX_CONFORMANCE  0
#define __DARWIN_ONLY_VERS_1050         0
#elif defined(__x86_64__)
#define __DARWIN_ONLY_64_BIT_INO_T      0
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         0
#else
#define __DARWIN_ONLY_64_BIT_INO_T      1
#define __DARWIN_ONLY_UNIX_CONFORMANCE  1
#define __DARWIN_ONLY_VERS_1050         1
#endif
#endif                      
#endif             

  
                                                                       
                                                                           
                                                                             
  
                                                                     
                                                                    
                                                                  
                                                                 
                                                               
  
                                                                            
                                                                          
                                                                              
                                                                              
                                                                      
                                                  
   
#if !defined(__DARWIN_UNIX03)
#  if defined(KERNEL)
#    define __DARWIN_UNIX03     0
#  elif __DARWIN_ONLY_UNIX_CONFORMANCE
#    if defined(_NONSTD_SOURCE)
#      error "Can't define _NONSTD_SOURCE when only UNIX conformance is available."
#    endif                     
#    define __DARWIN_UNIX03     1
#  elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && ((__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ - 0) < 1040)
#    define __DARWIN_UNIX03     0
#  elif defined(_DARWIN_C_SOURCE) || defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
#    if defined(_NONSTD_SOURCE)
#      error "Can't define both _NONSTD_SOURCE and any of _DARWIN_C_SOURCE, _XOPEN_SOURCE or _POSIX_C_SOURCE."
#    endif                     
#    define __DARWIN_UNIX03     1
#  elif defined(_NONSTD_SOURCE)
#    define __DARWIN_UNIX03     0
#  else              
#    if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && ((__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ - 0) < 1050)
#      define __DARWIN_UNIX03   0
#    else                                                            
#      define __DARWIN_UNIX03   1
#    endif                                                            
#  endif                                                                       
#endif                       

#if !defined(__DARWIN_64_BIT_INO_T)
#  if defined(KERNEL)
#    define __DARWIN_64_BIT_INO_T 0
#  elif defined(_DARWIN_USE_64_BIT_INODE)
#    if defined(_DARWIN_NO_64_BIT_INODE)
#      error "Can't define both _DARWIN_USE_64_BIT_INODE and _DARWIN_NO_64_BIT_INODE."
#    endif                              
#    define __DARWIN_64_BIT_INO_T 1
#  elif defined(_DARWIN_NO_64_BIT_INODE)
#    if __DARWIN_ONLY_64_BIT_INO_T
#      error "Can't define _DARWIN_NO_64_BIT_INODE when only 64-bit inodes are available."
#    endif                                 
#    define __DARWIN_64_BIT_INO_T 0
#  else              
#    if __DARWIN_ONLY_64_BIT_INO_T
#      define __DARWIN_64_BIT_INO_T 1
#    elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && ((__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ - 0) < 1060) || __DARWIN_UNIX03 == 0
#      define __DARWIN_64_BIT_INO_T 0
#    else              
#      define __DARWIN_64_BIT_INO_T 1
#    endif                                 
#  endif
#endif                             

#if !defined(__DARWIN_VERS_1050)
#  if defined(KERNEL)
#    define __DARWIN_VERS_1050 0
#  elif __DARWIN_ONLY_VERS_1050
#    define __DARWIN_VERS_1050 1
#  elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && ((__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ - 0) < 1050) || __DARWIN_UNIX03 == 0
#    define __DARWIN_VERS_1050 0
#  else              
#    define __DARWIN_VERS_1050 1
#  endif
#endif                          

#if !defined(__DARWIN_NON_CANCELABLE)
#  if defined(KERNEL)
#    define __DARWIN_NON_CANCELABLE 0
#  else              
#    define __DARWIN_NON_CANCELABLE 0
#  endif
#endif                               

  
                                             
   
#if __DARWIN_UNIX03
#  if __DARWIN_ONLY_UNIX_CONFORMANCE
#    define __DARWIN_SUF_UNIX03                      
#  else                                      
#    define __DARWIN_SUF_UNIX03         "$UNIX2003"
#  endif                                     

#  if __DARWIN_64_BIT_INO_T
#    if __DARWIN_ONLY_64_BIT_INO_T
#      define __DARWIN_SUF_64_BIT_INO_T              
#    else                                  
#      define __DARWIN_SUF_64_BIT_INO_T "$INODE64"
#    endif                                 
#  else                             
#    define __DARWIN_SUF_64_BIT_INO_T                
#  endif                            

#  if __DARWIN_VERS_1050
#    if __DARWIN_ONLY_VERS_1050
#      define __DARWIN_SUF_1050                      
#    else                               
#      define __DARWIN_SUF_1050         "$1050"
#    endif                              
#  else                          
#    define __DARWIN_SUF_1050                        
#  endif                         

#  if __DARWIN_NON_CANCELABLE
#    define __DARWIN_SUF_NON_CANCELABLE "$NOCANCEL"
#  else                               
#    define __DARWIN_SUF_NON_CANCELABLE              
#  endif                              

#else                       
#  define __DARWIN_SUF_UNIX03                        
#  define __DARWIN_SUF_64_BIT_INO_T                  
#  define __DARWIN_SUF_NON_CANCELABLE                
#  define __DARWIN_SUF_1050                          
#endif                      

#define __DARWIN_SUF_EXTSN              "$DARWIN_EXTSN"

  
                           
   
#define __DARWIN_ALIAS(sym)             __asm("_" __STRING(sym) __DARWIN_SUF_UNIX03)
#define __DARWIN_ALIAS_C(sym)           __asm("_" __STRING(sym) __DARWIN_SUF_NON_CANCELABLE __DARWIN_SUF_UNIX03)
#define __DARWIN_ALIAS_I(sym)           __asm("_" __STRING(sym) __DARWIN_SUF_64_BIT_INO_T __DARWIN_SUF_UNIX03)
#define __DARWIN_NOCANCEL(sym)          __asm("_" __STRING(sym) __DARWIN_SUF_NON_CANCELABLE)
#define __DARWIN_INODE64(sym)           __asm("_" __STRING(sym) __DARWIN_SUF_64_BIT_INO_T)

#define __DARWIN_1050(sym)              __asm("_" __STRING(sym) __DARWIN_SUF_1050)
#define __DARWIN_1050ALIAS(sym)         __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_UNIX03)
#define __DARWIN_1050ALIAS_C(sym)       __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_NON_CANCELABLE __DARWIN_SUF_UNIX03)
#define __DARWIN_1050ALIAS_I(sym)       __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_64_BIT_INO_T __DARWIN_SUF_UNIX03)
#define __DARWIN_1050INODE64(sym)       __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_64_BIT_INO_T)

#define __DARWIN_EXTSN(sym)             __asm("_" __STRING(sym) __DARWIN_SUF_EXTSN)
#define __DARWIN_EXTSN_C(sym)           __asm("_" __STRING(sym) __DARWIN_SUF_EXTSN __DARWIN_SUF_NON_CANCELABLE)
#if XNU_KERNEL_PRIVATE
#define __XNU_INTERNAL(sym)             __asm("_" __STRING(sym) "$XNU_INTERNAL")
#endif

  
                        
   
#ifdef KERNEL
#define __DARWIN_ALIAS_STARTING(_mac, _iphone, x)
#else
#include <sys/_symbol_aliasing.h>

#if defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#define __DARWIN_ALIAS_STARTING(_mac, _iphone, x)   __DARWIN_ALIAS_STARTING_IPHONE_##_iphone(x)
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#define __DARWIN_ALIAS_STARTING(_mac, _iphone, x)   __DARWIN_ALIAS_STARTING_MAC_##_mac(x)
#else
#define __DARWIN_ALIAS_STARTING(_mac, _iphone, x)   x
#endif
#endif             


  
                                                                          
                                                                          
                                                     
  
                                           
                                       
                                      
                                                                
                                            
                                                           
                                           
                                           
                                           
  
                                                                          
                                                                             
                                                          
   

                                                                      
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE == 1L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         199009L
#endif

                                                                      
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE == 2L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         199209L
#endif

                                                                       
#ifdef _XOPEN_SOURCE
#if _XOPEN_SOURCE - 0L >= 700L && (!defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE - 0L < 200809L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         200809L
#elif _XOPEN_SOURCE - 0L >= 600L && (!defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE - 0L < 200112L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         200112L
#elif _XOPEN_SOURCE - 0L >= 500L && (!defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE - 0L < 199506L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         199506L
#endif
#endif

  
                                                                                
             
   
#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE         198808L
#endif

                                
#ifdef KERNEL
#define __POSIX_C_DEPRECATED(ver)
#else
#include <sys/_posix_availability.h>

#define __POSIX_C_DEPRECATED(ver) ___POSIX_C_DEPRECATED_STARTING_##ver
#endif

  
                                                                               
                                                                         
                                                                                  
                                                                  
   
#define __DARWIN_C_ANSI         010000L
#define __DARWIN_C_FULL         900000L

#if   defined(_ANSI_SOURCE)
#define __DARWIN_C_LEVEL        __DARWIN_C_ANSI
#elif defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE) && !defined(_NONSTD_SOURCE)
#define __DARWIN_C_LEVEL        _POSIX_C_SOURCE
#else
#define __DARWIN_C_LEVEL        __DARWIN_C_FULL
#endif

                                                                              
                                                                             
                      
   
#if !defined(__STDC_WANT_LIB_EXT1__) && !defined(__STRICT_ANSI__) && __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define __STDC_WANT_LIB_EXT1__ 1
#endif

  
                                                                         
                                                                         
       
   
#if (defined(__STRICT_ANSI__) && (__STDC_VERSION__ - 0 < 199901L) && !defined(__GNUG__))
#define __DARWIN_NO_LONG_LONG 1
#else
#define __DARWIN_NO_LONG_LONG 0
#endif

                                          
                                        
                                          

  
                                                                            
                                                                         
   
#if __DARWIN_64_BIT_INO_T
#define _DARWIN_FEATURE_64_BIT_INODE            1
#endif

  
                                                                           
                                                                             
                                                                    
                                                  
   
#if __DARWIN_ONLY_64_BIT_INO_T
#define _DARWIN_FEATURE_ONLY_64_BIT_INODE       1
#endif

  
                                                                        
                                                      
   
#if __DARWIN_ONLY_VERS_1050
#define _DARWIN_FEATURE_ONLY_VERS_1050          1
#endif

  
                                                                           
                                                        
   
#if __DARWIN_ONLY_UNIX_CONFORMANCE
#define _DARWIN_FEATURE_ONLY_UNIX_CONFORMANCE   1
#endif

  
                                                                             
                                                   
   
#if __DARWIN_UNIX03
#define _DARWIN_FEATURE_UNIX_CONFORMANCE        3
#endif

#if defined(DRIVERKIT) && !defined(KERNEL)
  
                                                                       
                                                                        
                                                                           
                 
   
#define __DRIVERKIT_LIBC__                      1
#endif                                             

  
                                                        
  
                                                                
                                  
   
#ifndef __CAST_AWAY_QUALIFIER
#define __CAST_AWAY_QUALIFIER(variable, qualifier, type)  (type) (long)(variable)
#endif

  
                                                                               
                                                                             
   
#ifndef __XNU_PRIVATE_EXTERN
#define __XNU_PRIVATE_EXTERN __attribute__((visibility("hidden")))
#endif

  
                                          
   
#if !defined(__sys_cdefs_arch_unknown__) && defined(__i386__)
#elif !defined(__sys_cdefs_arch_unknown__) && defined(__x86_64__)
#elif !defined(__sys_cdefs_arch_unknown__) && defined(__arm__)
#elif !defined(__sys_cdefs_arch_unknown__) && defined(__arm64__)
#else
#error Unsupported architecture
#endif

#ifdef XNU_KERNEL_PRIVATE
  
                                             
   
#define __IGNORE_WCASTALIGN(x) _Pragma("clang diagnostic push")                     \
	                       _Pragma("clang diagnostic ignored \"-Wcast-align\"") \
	                       x;                                                   \
	                       _Pragma("clang diagnostic pop")
#endif

#if defined(PRIVATE) || defined(KERNEL)
  
                                                                            
                                                                                
               
   
#if !defined(__probable) && !defined(__improbable)
#define __probable(x)   __builtin_expect(!!(x), 1)
#define __improbable(x) __builtin_expect(!!(x), 0)
#endif                                                     

#if defined(__cplusplus)
#define __container_of(ptr, type, field) __extension__({ \
	        const __typeof__(((type *)nullptr)->field) *__ptr = (ptr); \
	        (type *)((uintptr_t)__ptr - offsetof(type, field)); \
	})
#else
#define __container_of(ptr, type, field) __extension__({ \
	        const __typeof__(((type *)NULL)->field) *__ptr = (ptr); \
	        (type *)((uintptr_t)__ptr - offsetof(type, field)); \
	})
#endif

#endif                        

#define __compiler_barrier() __asm__ __volatile__("" ::: "memory")

#if __has_attribute(enum_extensibility)
#define __enum_open __attribute__((__enum_extensibility__(open)))
#define __enum_closed __attribute__((__enum_extensibility__(closed)))
#else
#define __enum_open
#define __enum_closed
#endif                                       

#if __has_attribute(flag_enum)
#define __enum_options __attribute__((__flag_enum__))
#else
#define __enum_options
#endif

  
                                                                 
  
                                                                    
                                    
   
#if __has_feature(objc_fixed_enum) || __has_extension(cxx_fixed_enum) || \
        __has_extension(cxx_strong_enums)
#define __enum_decl(_name, _type, ...) \
	        typedef enum : _type __VA_ARGS__ __enum_open _name
#define __enum_closed_decl(_name, _type, ...) \
	        typedef enum : _type __VA_ARGS__ __enum_closed _name
#define __options_decl(_name, _type, ...) \
	        typedef enum : _type __VA_ARGS__ __enum_open __enum_options _name
#define __options_closed_decl(_name, _type, ...) \
	        typedef enum : _type __VA_ARGS__ __enum_closed __enum_options _name
#else
#define __enum_decl(_name, _type, ...) \
	        typedef _type _name; enum __VA_ARGS__ __enum_open
#define __enum_closed_decl(_name, _type, ...) \
	        typedef _type _name; enum __VA_ARGS__ __enum_closed
#define __options_decl(_name, _type, ...) \
	        typedef _type _name; enum __VA_ARGS__ __enum_open __enum_options
#define __options_closed_decl(_name, _type, ...) \
	        typedef _type _name; enum __VA_ARGS__ __enum_closed __enum_options
#endif

#endif                 
