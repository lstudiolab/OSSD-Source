/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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

#ifndef __OS_ATOMIC_PRIVATE_H__
#define __OS_ATOMIC_PRIVATE_H__

   
                              
  
         
                                                                               
  
              
                                                                           
                     
  
                                                                         
                                 
  
                                                                                
                                                                           
                                                                        
                                
  
                                                                        
                                                             
                                             
  
                                                                             
                                          
   

#include <os/atomic.h>

   
                                         
  
     
  
         
                                                             
   

   
                              
  
         
                                                        
   
#ifndef OS_ATOMIC_CONFIG_SMP
#define OS_ATOMIC_CONFIG_SMP 1
#endif                        

   
                                               
  
         
                                                                        
                  
  
              
                                                           
                                   
                                          
                                          
                                           
  
                                                                            
                                                                              
                                                                    
  
                                                                               
                                                                              
  
            
                                        
                                                            
                                                            
  
                                                                           
                                                                  
  
                                                                           
   
#ifndef OS_ATOMIC_CONFIG_STARVATION_FREE_ONLY
#if XNU_KERNEL_PRIVATE
#define OS_ATOMIC_CONFIG_STARVATION_FREE_ONLY 0
#elif KERNEL
#define OS_ATOMIC_CONFIG_STARVATION_FREE_ONLY 1
#else
#define OS_ATOMIC_CONFIG_STARVATION_FREE_ONLY 0
#endif
#endif                                         

   
                                                  
  
         
                                                                   
  
              
                                                                                
                                                                                  
  
                                                                          
                                                                                
  
             
   
#ifndef OS_ATOMIC_CONFIG_MEMORY_ORDER_DEPENDENCY
#if XNU_KERNEL_PRIVATE
#define OS_ATOMIC_CONFIG_MEMORY_ORDER_DEPENDENCY 1
#else
#define OS_ATOMIC_CONFIG_MEMORY_ORDER_DEPENDENCY 0
#endif
#endif                                            

         

   
                                                         
  
     
  
         
                                                                    
                                                                             
                                                          
   

   
                            
  
         
                                           
  
              
                                                            
   
#if defined(__i386__) || defined(__x86_64__)
#define OS_ATOMIC_HAS_LLSC  0
#elif defined(__arm__) || defined(__arm64__)
#define OS_ATOMIC_HAS_LLSC  1
#else
#error unsupported architecture
#endif

   
                            
  
         
                                           
  
              
                                                 
   
#if defined(__arm64__) && defined(__ARM_ARCH_8_2__)
#define OS_ATOMIC_USE_LLSC  0
#else
#define OS_ATOMIC_USE_LLSC  OS_ATOMIC_HAS_LLSC
#endif

   
                                           
  
         
                                                                      
                                
   
#define OS_ATOMIC_HAS_STARVATION_FREE_RMW (!OS_ATOMIC_USE_LLSC)

         

#include "atomic_private_impl.h"                                   

   
                                
  
         
                                                                  
  
           
                                                                              
                                   
                                                                         
                                                                 
   
#undef os_compiler_barrier
#define os_compiler_barrier(b...) \
	os_atomic_std(atomic_signal_fence)(_os_compiler_barrier_##b)

   
                                   
  
         
                                                                       
                                  
  
           
                               
   
#define os_atomic_thread_fence(m)  ({ \
	os_atomic_std(atomic_thread_fence)(_os_atomic_mo_##m##_smp); \
	os_atomic_std(atomic_signal_fence)(_os_atomic_mo_##m); \
})

   
                           
  
         
                                
  
              
                                                                               
                                                                           
                 
  
           
                                   
  
           
                                             
  
           
                              
   
#define os_atomic_init(p, v) \
	os_atomic_std(atomic_init)(os_cast_to_atomic_pointer(p), v)

   
                                                              
  
         
                                                                           
                                                                   
  
              
                                                                            
                                       
  
                                                                 
  
                                                                 
                    
  
           
                                   
  
           
                                                                        
                                              
   
#define os_atomic_load_is_plain(p)  (sizeof(*(p)) <= sizeof(void *))
#define os_atomic_store_is_plain(p) os_atomic_load_is_plain(p)

   
                           
  
         
                                                                            
                                                   
  
           
                                   
  
           
                       
  
           
                              
   
#define os_atomic_load(p, m)  ({ \
	_Static_assert(os_atomic_load_is_plain(p), "Load is wide"); \
	_os_compiler_barrier_before_atomic(m); \
	__auto_type _r = os_atomic_std(atomic_load_explicit)( \
	    os_cast_to_atomic_pointer(p), _os_atomic_mo_##m##_smp); \
	_os_compiler_barrier_after_atomic(m); \
	_r; \
})

   
                            
  
         
                                                                             
                                                    
  
           
                                   
  
           
                      
  
           
                       
  
           
                            
   
#define os_atomic_store(p, v, m)  ({ \
	_Static_assert(os_atomic_store_is_plain(p), "Store is wide"); \
	__auto_type _v = (v); \
	_os_compiler_barrier_before_atomic(m); \
	os_atomic_std(atomic_store_explicit)(os_cast_to_atomic_pointer(p), _v, \
	    _os_atomic_mo_##m##_smp); \
	_os_compiler_barrier_after_atomic(m); \
	_v; \
})

#if OS_ATOMIC_HAS_STARVATION_FREE_RMW || !OS_ATOMIC_CONFIG_STARVATION_FREE_ONLY

   
                                
  
         
                                                                        
                                                   
  
           
                                   
  
           
                       
  
           
                              
   
#define os_atomic_load_wide(p, m)  ({ \
	_os_compiler_barrier_before_atomic(m); \
	__auto_type _r = os_atomic_std(atomic_load_explicit)( \
	    os_cast_to_atomic_pointer(p), _os_atomic_mo_##m##_smp); \
	_os_compiler_barrier_after_atomic(m); \
	_r; \
})

   
                                 
  
         
                                                                         
                                                   
  
           
                                   
  
           
                      
  
           
                       
  
           
                            
   
#define os_atomic_store_wide(p, v, m)  ({ \
	__auto_type _v = (v); \
	_os_compiler_barrier_before_atomic(m); \
	os_atomic_std(atomic_store_explicit)(os_cast_to_atomic_pointer(p), _v, \
	    _os_atomic_mo_##m##_smp); \
	_os_compiler_barrier_after_atomic(m); \
	_v; \
})

   
                                              
  
         
                                                
  
           
                                   
  
           
                    
  
           
                       
  
           
                                                                              
                                                                        
   
#define os_atomic_add_orig(p, v, m) _os_atomic_c11_op_orig(p, v, m, fetch_add)
#define os_atomic_add(p, v, m)      _os_atomic_c11_op(p, v, m, fetch_add, +)

   
                                              
  
         
                               
  
           
                                   
  
           
                       
  
           
                                                                                    
                                                                              
   
#define os_atomic_inc_orig(p, m)    _os_atomic_c11_op_orig(p, 1, m, fetch_add)
#define os_atomic_inc(p, m)         _os_atomic_c11_op(p, 1, m, fetch_add, +)

   
                                              
  
         
                                                
  
           
                                   
  
           
                         
  
           
                       
  
           
                                                                                   
                                                                             
   
#define os_atomic_sub_orig(p, v, m) _os_atomic_c11_op_orig(p, v, m, fetch_sub)
#define os_atomic_sub(p, v, m)      _os_atomic_c11_op(p, v, m, fetch_sub, -)

   
                                              
  
         
                               
  
           
                                   
  
           
                       
  
           
                                                                                    
                                                                              
   
#define os_atomic_dec_orig(p, m)    _os_atomic_c11_op_orig(p, 1, m, fetch_sub)
#define os_atomic_dec(p, m)         _os_atomic_c11_op(p, 1, m, fetch_sub, -)

   
                                              
  
         
                                                
  
           
                                   
  
           
                    
  
           
                       
  
           
                                                                              
                                                                        
   
#define os_atomic_and_orig(p, v, m) _os_atomic_c11_op_orig(p, v, m, fetch_and)
#define os_atomic_and(p, v, m)      _os_atomic_c11_op(p, v, m, fetch_and, &)

   
                                                    
  
         
                                                         
  
           
                                   
  
           
                                     
  
           
                       
  
           
                                                                                    
                                                                              
   
#define os_atomic_andnot_orig(p, v, m) _os_atomic_c11_op_orig(p, (typeof(v))~(v), m, fetch_and)
#define os_atomic_andnot(p, v, m)      _os_atomic_c11_op(p, (typeof(v))~(v), m, fetch_and, &)

   
                                            
  
         
                                               
  
           
                                   
  
           
                   
  
           
                       
  
           
                                                                            
                                                                      
   
#define os_atomic_or_orig(p, v, m)  _os_atomic_c11_op_orig(p, v, m, fetch_or)
#define os_atomic_or(p, v, m)       _os_atomic_c11_op(p, v, m, fetch_or, |)

   
                                              
  
         
                                                
  
           
                                   
  
           
                    
  
           
                       
  
           
                                                                              
                                                                        
   
#define os_atomic_xor_orig(p, v, m) _os_atomic_c11_op_orig(p, v, m, fetch_xor)
#define os_atomic_xor(p, v, m)      _os_atomic_c11_op(p, v, m, fetch_xor, ^)

   
                                              
  
         
                                            
  
           
                                   
  
           
                         
  
           
                       
  
           
                                                                              
                                                                        
   
#define os_atomic_min_orig(p, v, m) _os_atomic_clang_op_orig(p, v, m, fetch_min)
#define os_atomic_min(p, v, m)      _os_atomic_clang_op(p, v, m, fetch_min, MIN)

   
                                              
  
         
                                            
  
           
                                   
  
           
                         
  
           
                       
  
           
                                                                              
                                                                        
   
#define os_atomic_max_orig(p, v, m) _os_atomic_clang_op_orig(p, v, m, fetch_max)
#define os_atomic_max(p, v, m)      _os_atomic_clang_op(p, v, m, fetch_max, MAX)

   
                           
  
         
                                              
  
           
                                   
  
           
                              
  
           
                       
  
           
                                                 
   
#define os_atomic_xchg(p, v, m)     _os_atomic_c11_op_orig(p, v, m, exchange)

   
                              
  
         
                                                             
  
              
                                                                  
                                                                           
                              
  
           
                                   
  
           
                                             
  
           
                                                                         
  
           
                                          
                                                       
  
           
                                    
                                       
   
#define os_atomic_cmpxchg(p, e, v, m)  ({ \
	os_atomic_basetypeof(p) _r = (e); int _b; \
	_os_compiler_barrier_before_atomic(m); \
	_b = os_atomic_std(atomic_compare_exchange_strong_explicit)( \
	    os_cast_to_atomic_pointer(p), &_r, \
	    _os_atomic_value_cast(p, v), \
	    _os_atomic_mo_##m##_smp, _os_atomic_mo_relaxed); \
	_os_compiler_barrier_after_atomic(m); \
	_b; \
})

   
                               
  
         
                                                             
  
              
                                                                   
                                                                           
                              
  
           
                                   
  
           
                                             
  
           
                                                                         
  
           
                                                                            
                                                                               
                                                      
  
           
                                          
                                                       
  
           
                                    
                                       
   
#define os_atomic_cmpxchgv(p, e, v, g, m)  ({ \
	os_atomic_basetypeof(p) _r = (e); int _b; \
	_os_compiler_barrier_before_atomic(m); \
	_b = os_atomic_std(atomic_compare_exchange_strong_explicit)( \
	    os_cast_to_atomic_pointer(p), &_r, \
	    _os_atomic_value_cast(p, v), \
	    _os_atomic_mo_##m##_smp, _os_atomic_mo_relaxed); \
	_os_compiler_barrier_after_atomic(m); \
	*(g) = _r; _b; \
})

   
                               
  
         
                                                                       
  
           
                                                  
  
            
                                                                              
                                                   
  
            
                                                                               
                                                                               
         
  
           
                                          
                                                       
  
             
                                                                             
                                                                              
               
  
                                                                            
                                                                     
                                             
  
                                                                                
                                                                              
  
           
                                                               
                           
   
#define os_atomic_rmw_loop(p, ov, nv, m, ...)  ({ \
	int _result = 0; \
	__auto_type _p = os_cast_to_nonatomic_pointer(p); \
	_os_compiler_barrier_before_atomic(m); \
	ov = *_p; \
	do { \
	    __VA_ARGS__; \
	    _result = os_atomic_std(atomic_compare_exchange_weak_explicit)( \
	        os_cast_to_atomic_pointer(_p), &ov, nv, \
	        _os_atomic_mo_##m##_smp, _os_atomic_mo_relaxed); \
	} while (__builtin_expect(!_result, 0)); \
	_os_compiler_barrier_after_atomic(m); \
	_result; \
})

   
                                       
  
         
                                      
  
             
                                                                         
                                                                     
   
#define os_atomic_rmw_loop_give_up(...) ({ __VA_ARGS__; break; })

#else                                                                               

#define _os_atomic_error_is_starvable(name) \
	_Static_assert(0, #name " is not starvation-free and isn't available in this configuration")
#define os_atomic_load_wide(p, m)               _os_atomic_error_is_starvable(os_atomic_load_wide)
#define os_atomic_store_wide(p, v, m)           _os_atomic_error_is_starvable(os_atomic_store_wide)
#define os_atomic_add_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_add_orig)
#define os_atomic_add(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_add)
#define os_atomic_inc_orig(p, m)                _os_atomic_error_is_starvable(os_atomic_inc_orig)
#define os_atomic_inc(p, m)                     _os_atomic_error_is_starvable(os_atomic_inc)
#define os_atomic_sub_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_sub_orig)
#define os_atomic_sub(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_sub)
#define os_atomic_dec_orig(p, m)                _os_atomic_error_is_starvable(os_atomic_dec_orig)
#define os_atomic_dec(p, m)                     _os_atomic_error_is_starvable(os_atomic_dec)
#define os_atomic_and_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_and_orig)
#define os_atomic_and(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_and)
#define os_atomic_andnot_orig(p, v, m)          _os_atomic_error_is_starvable(os_atomic_andnot_orig)
#define os_atomic_andnot(p, v, m)               _os_atomic_error_is_starvable(os_atomic_andnot)
#define os_atomic_or_orig(p, v, m)              _os_atomic_error_is_starvable(os_atomic_or_orig)
#define os_atomic_or(p, v, m)                   _os_atomic_error_is_starvable(os_atomic_or)
#define os_atomic_xor_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_xor_orig)
#define os_atomic_xor(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_xor)
#define os_atomic_min_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_min_orig)
#define os_atomic_min(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_min)
#define os_atomic_max_orig(p, v, m)             _os_atomic_error_is_starvable(os_atomic_max_orig)
#define os_atomic_max(p, v, m)                  _os_atomic_error_is_starvable(os_atomic_max)
#define os_atomic_xchg(p, v, m)                 _os_atomic_error_is_starvable(os_atomic_xchg)
#define os_atomic_cmpxchg(p, e, v, m)           _os_atomic_error_is_starvable(os_atomic_cmpxchg)
#define os_atomic_cmpxchgv(p, e, v, g, m)       _os_atomic_error_is_starvable(os_atomic_cmpxchgv)
#define os_atomic_rmw_loop(p, ov, nv, m, ...)   _os_atomic_error_is_starvable(os_atomic_rmw_loop)
#define os_atomic_rmw_loop_give_up(...)         _os_atomic_error_is_starvable(os_atomic_rmw_loop_give_up)

#endif                                                                               

#if OS_ATOMIC_CONFIG_MEMORY_ORDER_DEPENDENCY

   
                                  
  
         
                                                                            
                                         
  
           
                                                                              
                                                                                
  
                                                                        
                                                                              
                                                                              
                     
   
typedef struct { unsigned long __opaque_zero; } os_atomic_dependency_t;

   
                                   
  
         
                                                                             
                                   
   
#define OS_ATOMIC_DEPENDENCY_NONE \
	((os_atomic_dependency_t){ 0UL })

   
                                      
  
         
                                                                             
                       
  
              
                                                                               
                 
  
                                                                            
                                                                 
  
           
                 
                                        
                                      
                                            
  
                                                                        
                                                             
  
           
                                                                            
          
  
                                                                             
                                                
   
#define os_atomic_make_dependency(v) \
	((void)(v), OS_ATOMIC_DEPENDENCY_NONE)

   
                                        
  
         
                                                                         
                     
  
           
                                           
  
           
                                                                  
  
                                                                        
  
                                                       
  
           
                                                                            
        
   
#define os_atomic_inject_dependency(p, e) \
	((typeof(*(p)) *)((p) + _os_atomic_auto_dependency(e).__opaque_zero))

   
                                              
  
         
                                                            
  
              
                         
  
         
                                                                  
          
  
           
                                   
  
           
                                                                  
  
                                                                        
  
                                                       
  
           
                              
   
#define os_atomic_load_with_dependency_on(p, e) \
	os_atomic_load(os_atomic_inject_dependency(p, e), dependency)

#endif                                            

#include "atomic_private_arch.h"                              

#endif                              
