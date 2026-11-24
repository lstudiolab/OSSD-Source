/* Copyright (c) (2010,2011,2012,2014,2015,2016,2017,2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CC_H_
#define _CORECRYPTO_CC_H_

#include <corecrypto/cc_config.h>
#include <corecrypto/cc_error.h>
#include <string.h>
#include <stdint.h>

#if __has_feature(attribute_availability_with_replacement)
#if __has_feature(attribute_availability_bridgeos)
  #ifndef __CC_BRIDGE_OS_DEPRECATED
    #define __CC_BRIDGEOS_DEPRECATED(_dep, _msg) __attribute__((availability(bridgeos,deprecated=_dep, replacement=_msg)))
  #endif
#endif

#ifndef __CC_BRIDGEOS_DEPRECATED
  #define __CC_BRIDGEOS_DEPRECATED(_dep, _msg)
#endif

#define cc_deprecate_with_replacement(replacement_message, ios_version, macos_version, tvos_version, watchos_version, bridgeos_version) \
__attribute__((availability(macos,deprecated=macos_version,       replacement=replacement_message)))\
__attribute__((availability(ios,deprecated=ios_version,           replacement=replacement_message)))\
__attribute__((availability(watchos,deprecated=watchos_version,   replacement=replacement_message)))\
__attribute__((availability(tvos,deprecated=tvos_version,         replacement=replacement_message)))\
__CC_BRIDGEOS_DEPRECATED(bridgeos_version, replacement_message)

#else                                                              

#define cc_deprecate_with_replacement(replacement_message, ios_version, macos_version, tvos_version, watchos_version, bridgeos_version)

#endif                                                             

                                                    
#define cc_concat_(a, b) a##b
#define cc_concat(a, b) cc_concat_(a, b)

#if defined(_MSC_VER)
#define __asm__(x)
#endif

                                                                                       
#if CORECRYPTO_DEBUG
#define cc_assert(x) assert(x)
#else
#define cc_assert(x)
#endif

#if CC_KERNEL
#include <kern/assert.h>
#elif CC_USE_S3
#define assert(args)                    
#else
#include <assert.h>
#endif

                                                                               
#define cc_static_assert(e,m)                                               \
    enum { cc_concat(static_assert_, __COUNTER__) = 1/(int)(!!(e)) }

                                                                       
                                                                        
                        
#define cc_aligned_struct(_alignment_)  \
typedef struct { \
uint8_t b[_alignment_]; \
} CC_ALIGNED(_alignment_)

#if defined(__BIGGEST_ALIGNMENT__)
#define CC_MAX_ALIGNMENT ((size_t)__BIGGEST_ALIGNMENT__)
#else
#define CC_MAX_ALIGNMENT ((size_t)16)
#endif

                                                                              
#define cc_pad_align(_size_) ((_size_ + CC_MAX_ALIGNMENT - 1) & (~(CC_MAX_ALIGNMENT - 1)))

                                                    
#define cc_ctx_n(_type_, _size_) ((_size_ + sizeof(_type_) - 1) / sizeof(_type_))

                                                   
#define cc_ctx_sizeof(_type_, _size_) sizeof(_type_[cc_ctx_n(_type_, _size_)])

  
                                                                                                    
                                                                                                                                                                                                                                                                    
                                                                                                                                         
                                                                                                                                                           
   
#if defined(_MSC_VER)
#include <malloc.h>
#define cc_ctx_decl(_type_, _size_, _name_)  _type_ * _name_ = (_type_ *) _alloca(sizeof(_type_) * cc_ctx_n(_type_, _size_) )
#define cc_ctx_decl_field(_type_, _size_, _name_)  _type_ _name_ [cc_ctx_n(_type_, _size_)]
#else
                                                                                                                                                                                                                                                                                                                                                            
#define cc_ctx_decl(_type_, _size_, _name_)     \
  _Pragma("GCC diagnostic push")                \
  _Pragma("GCC diagnostic ignored \"-Wvla\"")   \
  _type_ _name_ [cc_ctx_n(_type_, _size_)]      \
  _Pragma("GCC diagnostic pop")
#define cc_ctx_decl_field cc_ctx_decl
#endif

   
                                                                               
                                                                                               
                                                
                       
   
CC_NONNULL((2))
void cc_clear(size_t len, void *dst);

                                                      
cc_deprecate_with_replacement("cc_clear", 13.0, 10.15, 13.0, 6.0, 4.0)
CC_NONNULL_ALL CC_INLINE
void cc_zero(size_t len, void *dst)
{
    cc_clear(len, dst);
}

#define cc_copy(_size_, _dst_, _src_) memcpy(_dst_, _src_, _size_)

CC_INLINE CC_NONNULL((2, 3, 4))
void cc_xor(size_t size, void *r, const void *s, const void *t) {
    uint8_t *_r=(uint8_t *)r;
    const uint8_t *_s=(const uint8_t *)s;
    const uint8_t *_t=(const uint8_t *)t;
    while (size--) {
        _r[size] = _s[size] ^ _t[size];
    }
}

   
                                                                                 
                                                                                                                                                              
                                          
                        
                        
                                                                                                                                                                
   
CC_NONNULL((2, 3))
int cc_cmp_safe (size_t num, const void * ptr1, const void * ptr2);

                                                                      
                                                 
#define CC_SWAP(S,T)  do { \
    volatile __typeof__(S) _cc_swap_tmp = S; S = T; T = _cc_swap_tmp; \
    _cc_swap_tmp = 0;\
} while(0)

                                               
#define CC_MAX(S, T) ({__typeof__(S) _cc_max_s = S; __typeof__(T) _cc_max_t = T; _cc_max_s > _cc_max_t ? _cc_max_s : _cc_max_t;})

                                                                              
#define CC_MAX_EVAL(S, T) ((S) > (T) ? (S) : (T))

                                               
#define CC_MIN(S, T) ({__typeof__(S) _cc_min_s = S; __typeof__(T) _cc_min_t = T; _cc_min_s <= _cc_min_t ? _cc_min_s : _cc_min_t;})

  
                                                                                      
                                                                       
 
                                                                        
                    
  
#if __has_feature(ptrauth_calls) && (CC_KERNEL || CC_USE_L4 || CC_USE_SEPROM)
#include <ptrauth.h>
#define CC_SPTR(_sn_, _n_) \
    __ptrauth(ptrauth_key_process_independent_code, 1, ptrauth_string_discriminator("cc_" #_sn_ #_n_)) _n_
#else
#define CC_SPTR(_sn_, _n_) _n_
#endif

#endif                        
