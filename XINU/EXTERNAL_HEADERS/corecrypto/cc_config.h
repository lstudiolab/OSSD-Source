/* Copyright (c) (2010,2011,2012,2013,2014,2015,2016,2017,2018,2019,2020) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CC_CONFIG_H_
#define _CORECRYPTO_CC_CONFIG_H_

                                     

                                                                                          
                                                                               
                                                                                      

                                                                                     

                                                                        

                                                                                        
                                                           

                                                                                    
                                                                                         
                                                                                   

                                      
                                   
              
                                   
               


                                                                                     
                                 

                                                                                     

                                                  
                                                                        

  

                                                                                      
#define CORECRYPTO_SIMULATE_POSIX_ENVIRONMENT    0

                                                                                         
#define CORECRYPTO_SIMULATE_WINDOWS_ENVIRONMENT 0
#define CORECRYPTO_HACK_FOR_WINDOWS_DEVELOPMENT 0

#if (defined(DEBUG) && (DEBUG)) || defined(_DEBUG)                      
                                              
 #define CORECRYPTO_DEBUG 1
#else
 #define CORECRYPTO_DEBUG 0
#endif

                                                                                     
                                                                             
                           
#define CORECRYPTO_DEBUG_ENABLE_CC_REQUIRE_PRINTS 0

#if defined(KERNEL) && (KERNEL)
 #define CC_KERNEL 1                                                             
#else
 #define CC_KERNEL 0
#endif

#if defined(LINUX_SGX) && (LINUX_SGX)
 #define CC_SGX 1
#else
 #define CC_SGX 0
#endif

#if (defined(__linux__) && !(CC_SGX)) || CORECRYPTO_SIMULATE_POSIX_ENVIRONMENT
 #define CC_LINUX 1
#else
 #define CC_LINUX 0
#endif

#if defined(__ANDROID__) && (__ANDROID__)
 #define CC_ANDROID 1
#else
 #define CC_ANDROID 0
#endif

#if defined(USE_L4) && (USE_L4)
 #define CC_USE_L4 1
#else
 #define CC_USE_L4 0
#endif

#if defined(RTKIT) && (RTKIT)
 #define CC_RTKIT 1
#else
 #define CC_RTKIT 0
#endif

#if defined(RTKITROM) && (RTKITROM)
#define CC_RTKITROM 1
#else
#define CC_RTKITROM 0
#endif

#if defined(USE_SEPROM) && (USE_SEPROM)
 #define CC_USE_SEPROM 1
#else
 #define CC_USE_SEPROM 0
#endif

#if defined(USE_S3) && (USE_S3)
 #define CC_USE_S3 1
#else
 #define CC_USE_S3 0
#endif

#if (defined(ICE_FEATURES_ENABLED)) || (defined(MAVERICK) && (MAVERICK))
 #define CC_BASEBAND 1
#else
 #define CC_BASEBAND 0
#endif

#if defined(EFI) && (EFI)
 #define CC_EFI 1
#else
 #define CC_EFI 0
#endif

#if defined(IBOOT) && (IBOOT)
 #define CC_IBOOT 1
#else
 #define CC_IBOOT 0
#endif

#if defined(TARGET_OS_BRIDGE)
 #define CC_BRIDGE TARGET_OS_BRIDGE
#else
 #define CC_BRIDGE 0
#endif

                                                                                              
#ifndef CC_GENERIC_PLATFORM
  #define CC_GENERIC_PLATFORM \
            (!CC_RTKIT && !CC_KERNEL && !CC_USE_L4 && \
             !CC_RTKITROM && !CC_EFI && !CC_IBOOT &&  \
             !CC_USE_SEPROM && !CC_ANDROID && !CC_LINUX && \
             !CC_BRIDGE)
#endif

                                                  
#ifndef CC_DARWIN_SPIS_AVAILABLE
  #if defined(__has_include)
    #define CC_DARWIN_SPIS_AVAILABLE __has_include(<os/log_private.h>)
  #else
    #define CC_DARWIN_SPIS_AVAILABLE 0
  #endif
#endif

                               

                            
                                                                                                                    
#ifndef CC_RINGBUFFER_AVAILABLE
  #define CC_RINGBUFFER_AVAILABLE (CC_GENERIC_PLATFORM && CC_DARWIN_SPIS_AVAILABLE && !CC_OPEN_SOURCE)
#endif

                     
                                                                                         
#ifndef CC_LOGGING_AVAILABLE
  #define CC_LOGGING_AVAILABLE (CC_GENERIC_PLATFORM && CC_DARWIN_SPIS_AVAILABLE && !CC_OPEN_SOURCE)
#endif

                          
                                                                                              
                                                                              
#ifndef CC_FEATURE_FLAGS_AVAILABLE
  #if defined(__has_include)
    #define CC_FEATURE_FLAGS_AVAILABLE __has_include(<os/feature_private.h>)
  #else
    #define CC_FEATURE_FLAGS_AVAILABLE 0
  #endif
#endif

                                                         
                                                                                                
                                                                                                          
#ifndef CC_FEATURE_ENABLED
  #if (CC_RINGBUFFER_AVAILABLE && CC_FEATURE_FLAGS_AVAILABLE && !defined(__i386__))
    #define CC_FEATURE_ENABLED(FEATURE) os_feature_enabled(Cryptography, FEATURE)
  #else
    #define CC_FEATURE_ENABLED(FEATURE) 0
  #endif
#endif

                                                                   
                       
#ifndef CC_LOG_TRACE
  #define CC_LOG_TRACE 0
#endif

                                   
                                                      
#if defined(XNU_KERNEL_PRIVATE)
 #define CC_XNU_KERNEL_PRIVATE 1
#else
 #define CC_XNU_KERNEL_PRIVATE 0
#endif

                                                                                                      
#define CC_HANDLE_UNALIGNED_DATA CC_BASEBAND

                         
#if CC_BASEBAND

               
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
 #if defined(ENDIAN_LITTLE) || (defined(__arm__) && !defined(__BIG_ENDIAN))
  #define __LITTLE_ENDIAN__
 #elif !defined(ENDIAN_BIG) && !defined(__BIG_ENDIAN)
  #error Baseband endianess not defined.
 #endif
 #define AESOPT_ENDIAN_NO_FILE
#endif

                  
 #define CCN_UNIT_SIZE  4           

                       
 #define assert ASSERT            

              
                                                
                                                          
                                                             
                                                         
 #ifdef __arm__
  #pragma diag_suppress 186, 1254,546
 #elif defined(__GNUC__)
                                                       
  #pragma GCC diagnostic ignored "-Wpointer-arith"
 #endif           
#define CC_SMALL_CODE 1

#endif               

#if CC_RTKIT || CC_RTKITROM
#define CC_SMALL_CODE 1
#endif


#ifndef CC_SMALL_CODE
#define CC_SMALL_CODE 0
#endif

                                                                           
                                               
#if defined(__APPLE__) && defined(__MACH__)
 #define CC_XNU_KERNEL_AVAILABLE 1
#else
 #define CC_XNU_KERNEL_AVAILABLE 0
#endif

                               
#if defined(__GNUC__) && defined(__aarch64__) && !defined(__arm64__)
    #define __arm64__
#endif

#if !defined(CCN_UNIT_SIZE)
 #if defined(__arm64__) || defined(__x86_64__)  || defined(_WIN64)
  #define CCN_UNIT_SIZE  8
 #elif defined(__arm__) || defined(__i386__) || defined(_WIN32)
  #define CCN_UNIT_SIZE  4
 #else
  #error undefined architecture
 #endif
#endif                              


                                                        
#if defined(CORECRYPTO_SIMULATE_WINDOWS_ENVIRONMENT)
 #if CORECRYPTO_SIMULATE_WINDOWS_ENVIRONMENT && CC_XNU_KERNEL_AVAILABLE && CORECRYPTO_DEBUG
  #define CC_USE_ASM 0
  #define CC_USE_HEAP_FOR_WORKSPACE 1
   #if (CCN_UNIT_SIZE==8)
    #define CCN_UINT128_SUPPORT_FOR_64BIT_ARCH 0
   #else
    #define CCN_UINT128_SUPPORT_FOR_64BIT_ARCH 1
   #endif
 #endif
#endif

#if !defined(CCN_UINT128_SUPPORT_FOR_64BIT_ARCH)
 #if defined(_WIN64) && defined(_WIN32) && (CCN_UNIT_SIZE==8)
  #define CCN_UINT128_SUPPORT_FOR_64BIT_ARCH 0
 #elif defined(_WIN32)
  #define CCN_UINT128_SUPPORT_FOR_64BIT_ARCH 1                         
 #else
  #define CCN_UINT128_SUPPORT_FOR_64BIT_ARCH 1
 #endif
#endif

#if defined(_MSC_VER)
    #if defined(__clang__)
        #define CC_ALIGNED(x) __attribute__ ((aligned(x)))                 
    #else
        #define CC_ALIGNED(x) __declspec(align(x))              
    #endif
#else
    #if __clang__ || CCN_UNIT_SIZE==8
        #define CC_ALIGNED(x) __attribute__ ((aligned(x)))
    #else
        #define CC_ALIGNED(x) __attribute__ ((aligned((x)>8?8:(x))))
    #endif
#endif

#if defined(__arm__)
                                                                                               
#if defined (__ARM_ARCH_7A__) || defined (__ARM_ARCH_7S__) || defined (__ARM_ARCH_7F__) || defined (__ARM_ARCH_7K__) || defined(__ARM_ARCH_7EM__)
  #define _ARM_ARCH_7
 #endif

 #if defined(__ARM_ARCH_6M__) || defined(__TARGET_ARCH_6S_M) || defined (__armv6m__)
  #define _ARM_ARCH_6M
 #endif
#endif

#if defined(__arm64__) || defined(__arm__)
 #define CCN_IOS				   1
 #define CCN_OSX				   0
#elif defined(__x86_64__) || defined(__i386__)
 #define CCN_IOS				   0
 #define CCN_OSX				   1
#endif

#if CC_USE_S3
                                                        
 #define CC_STATIC              1
#endif

#if !defined(CC_USE_HEAP_FOR_WORKSPACE)
 #if CC_USE_S3 || CC_USE_SEPROM || CC_RTKITROM
  #define CC_USE_HEAP_FOR_WORKSPACE 0
 #else
  #define CC_USE_HEAP_FOR_WORKSPACE 1
 #endif
#endif

                                              
#if CC_USE_SEPROM || defined(__CC_ARM) \
    || defined(__hexagon__) || CC_EFI
 #define CC_HAS_MEMSET_S 0
#else
 #define CC_HAS_MEMSET_S 1
#endif

                                            
#if defined(__has_include)                      
#if __has_include(<TargetConditionals.h>)
#include <TargetConditionals.h>
#endif                                            
#endif                             

                                
#if defined(TARGET_OS_BRIDGE) && TARGET_OS_BRIDGE && CC_KERNEL
#define CC_DISABLE_RSAKEYGEN 1                  
#else
#define CC_DISABLE_RSAKEYGEN 0              
#endif

#if (CCN_UNIT_SIZE == 8) && !( defined(_MSC_VER) && defined(__clang__))
#define CCEC25519_CURVE25519_64BIT 1
#else
#define CCEC25519_CURVE25519_64BIT 0
#endif

                                                                                
                                                                              
#if !(CC_XNU_KERNEL_AVAILABLE || CC_KERNEL || CC_USE_L4 || CC_IBOOT || CC_RTKIT || CC_RTKITROM || CC_USE_SEPROM || CC_USE_S3) && !defined(_WIN32) && CORECRYPTO_DEBUG
 #warning "You are using the default corecrypto configuration, assembly optimizations may not be available for your platform"
#endif

                                                       
#if (CC_LINUX || CC_SGX) && defined(CC_LINUX_ASM) && CC_LINUX_ASM
#define CC_USE_ASM 1
#endif

                                                                                 
                                                                                     
#if !defined(CC_USE_ASM)
 #if defined(_WIN32) || CC_EFI || CC_BASEBAND || CC_XNU_KERNEL_PRIVATE || (defined(__GNUC__) && !defined(__clang__)) || defined(__ANDROID_API__) || CC_LINUX
  #define CC_USE_ASM 0
 #else
  #define CC_USE_ASM 1
 #endif
#endif

#define CC_CACHE_DESCRIPTORS CC_KERNEL

             
#if defined(_ARM_ARCH_7) && __clang__ && CC_USE_ASM
 #define CCN_DEDICATED_SQR      CC_SMALL_CODE
 #define CCN_MUL_KARATSUBA      0                              
 #define CCN_ADD_ASM            1
 #define CCN_SUB_ASM            1
 #define CCN_MUL_ASM            0
 #define CCN_ADDMUL1_ASM        1
 #define CCN_MUL1_ASM           1
 #define CCN_CMP_ASM            1
 #define CCN_ADD1_ASM           1
 #define CCN_SUB1_ASM           1
 #define CCN_N_ASM              1
 #define CCN_SET_ASM            1
 #define CCN_SHIFT_RIGHT_ASM    1
 #if defined(__ARM_NEON__) 
 #define CCN_SHIFT_LEFT_ASM     1
 #else
 #define CCN_SHIFT_LEFT_ASM     0
 #endif
 #define CCN_MULMOD_224_ASM     1
 #define CCN_MULMOD_256_ASM     1
 #define CCAES_ARM_ASM          1
 #define CCAES_INTEL_ASM        0
 #if CC_KERNEL || CC_USE_L4 || CC_IBOOT || CC_RTKIT || CC_RTKITROM || CC_USE_SEPROM || CC_USE_S3
  #define CCAES_MUX             0
 #else
  #define CCAES_MUX             1
 #endif
 #define CCN_USE_BUILTIN_CLZ    1
 #define CCSHA1_VNG_INTEL       0
 #define CCSHA2_VNG_INTEL       0

 #if defined(__ARM_NEON__) || CC_KERNEL
  #define CCSHA1_VNG_ARM        1
  #define CCSHA2_VNG_ARM        1
 #else                             
  #define CCSHA1_VNG_ARM        0
  #define CCSHA2_VNG_ARM        0
 #endif                             
 #define CCSHA256_ARMV6M_ASM 0

 #define CC_ACCELERATECRYPTO    1

             
#elif defined(__arm64__) && __clang__ && CC_USE_ASM
 #define CCN_DEDICATED_SQR      CC_SMALL_CODE
 #define CCN_MUL_KARATSUBA      0                                       
 #define CCN_ADD_ASM            1
 #define CCN_SUB_ASM            1
 #define CCN_MUL_ASM            1
 #define CCN_ADDMUL1_ASM        0
 #define CCN_MUL1_ASM           0
 #define CCN_CMP_ASM            1
 #define CCN_ADD1_ASM           0
 #define CCN_SUB1_ASM           0
 #define CCN_N_ASM              1
 #define CCN_SET_ASM            0
 #define CCN_SHIFT_RIGHT_ASM    1
 #define CCN_SHIFT_LEFT_ASM     1
 #define CCN_MULMOD_224_ASM     1
 #define CCN_MULMOD_256_ASM     1
 #define CCAES_ARM_ASM          1
 #define CCAES_INTEL_ASM        0
 #define CCAES_MUX              0                                                   
 #define CCN_USE_BUILTIN_CLZ    1
 #define CCSHA1_VNG_INTEL       0
 #define CCSHA2_VNG_INTEL       0
 #define CCSHA1_VNG_ARM         1
 #define CCSHA2_VNG_ARM         1
 #define CCSHA256_ARMV6M_ASM    0

 #define CC_ACCELERATECRYPTO    1

                  
#elif (defined(__x86_64__) || defined(__i386__)) && __clang__ && CC_USE_ASM
 #define CCN_DEDICATED_SQR      1
 #define CCN_MUL_KARATSUBA      0                                       
                                                                    
 #if (defined(__x86_64__) && CCN_UNIT_SIZE == 8) || (defined(__i386__) && CCN_UNIT_SIZE == 4)
  #define CCN_ADD_ASM            1
  #define CCN_SUB_ASM            1
  #define CCN_MUL_ASM            1
 #else
  #define CCN_ADD_ASM            0
  #define CCN_SUB_ASM            0
  #define CCN_MUL_ASM            0
 #endif

 #if (defined(__x86_64__) && CCN_UNIT_SIZE == 8)
  #define CCN_CMP_ASM            1
  #define CCN_N_ASM              1
  #define CCN_SHIFT_RIGHT_ASM    1
  #define CCN_SHIFT_LEFT_ASM     1
 #else
  #define CCN_CMP_ASM            0
  #define CCN_N_ASM              0
  #define CCN_SHIFT_RIGHT_ASM    0
  #define CCN_SHIFT_LEFT_ASM     0
 #endif

 #define CCN_MULMOD_224_ASM     0
 #if defined(__x86_64__) && CCN_UNIT_SIZE == 8 && !CC_SGX
  #define CCN_MULMOD_256_ASM    1
  #define CCN_ADDMUL1_ASM       1
  #define CCN_MUL1_ASM          1
 #else
  #define CCN_MULMOD_256_ASM    0
  #define CCN_ADDMUL1_ASM       0
  #define CCN_MUL1_ASM          0
 #endif
 #define CCN_ADD1_ASM           0
 #define CCN_SUB1_ASM           0
 #define CCN_SET_ASM            0
 #define CCAES_ARM_ASM          0
 #define CCAES_INTEL_ASM        1
 #define CCAES_MUX              0
 #define CCN_USE_BUILTIN_CLZ    0
 #define CCSHA1_VNG_INTEL       1
 #define CCSHA2_VNG_INTEL       1
 #define CCSHA1_VNG_ARM         0
 #define CCSHA2_VNG_ARM         0
 #define CCSHA256_ARMV6M_ASM    0

 #define CC_ACCELERATECRYPTO    1

                       
#else
 #if CCN_UINT128_SUPPORT_FOR_64BIT_ARCH
  #define CCN_DEDICATED_SQR     1
 #else
  #define CCN_DEDICATED_SQR     0                                                                                                                    
 #endif
 #define CCN_MUL_KARATSUBA      0                                       
 #define CCN_ADD_ASM            0
 #define CCN_SUB_ASM            0
 #define CCN_MUL_ASM            0
 #define CCN_ADDMUL1_ASM        0
 #define CCN_MUL1_ASM           0
 #define CCN_CMP_ASM            0
 #define CCN_ADD1_ASM           0
 #define CCN_SUB1_ASM           0
 #define CCN_N_ASM              0
 #define CCN_SET_ASM            0
 #define CCN_SHIFT_RIGHT_ASM    0
 #define CCN_SHIFT_LEFT_ASM     0
 #define CCN_MULMOD_224_ASM     0
 #define CCN_MULMOD_256_ASM     0
 #define CCAES_ARM_ASM          0
 #define CCAES_INTEL_ASM        0
 #define CCAES_MUX              0
 #define CCN_USE_BUILTIN_CLZ    0
 #define CCSHA1_VNG_INTEL       0
 #define CCSHA2_VNG_INTEL       0
 #define CCSHA1_VNG_ARM         0
 #define CCSHA2_VNG_ARM         0
 #define CCSHA256_ARMV6M_ASM    0

 #define CC_ACCELERATECRYPTO    0

#endif

#define CC_INLINE static inline

#ifdef __GNUC__
 #define CC_NORETURN __attribute__((__noreturn__))
 #define CC_NOTHROW __attribute__((__nothrow__))
 #define CC_NONNULL(N) __attribute__((__nonnull__ N))
 #define CC_NONNULL4 CC_NONNULL((4))
 #define CC_NONNULL_ALL __attribute__((__nonnull__))
 #define CC_SENTINEL __attribute__((__sentinel__))
                                                                                                                                                                          
                                                                                                                     
 #define CC_CONST __attribute__((__const__))
 #define CC_PURE __attribute__((__pure__))
 #define CC_WARN_RESULT __attribute__((__warn_unused_result__))
 #define CC_MALLOC_CLEAR __attribute__((__malloc__))
 #define CC_UNUSED __attribute__((unused))
#else                
                 
 #define CC_UNUSED
                 
 #define CC_NONNULL(N)
                 
 #define CC_NONNULL4
                 
 #define CC_NORETURN
                 
 #define CC_NOTHROW
                 
 #define CC_NONNULL_ALL
                 
 #define CC_SENTINEL
                 
 #define CC_CONST
                 
 #define CC_PURE
                 
 #define CC_WARN_RESULT
                 
 #define CC_MALLOC_CLEAR
#endif                


                                                                   
#if CC_LINUX || CC_SGX
#define CC_ASM_SECTION_CONST .rodata
#define CC_ASM_PRIVATE_EXTERN .hidden
#if CC_LINUX
                                                           
                                                                      
                                          
#define CC_C_LABEL(_sym) _sym@PLT
#else             
#define CC_C_LABEL(_sym) _sym
#endif
#define _IMM(x) $(x)
#else                           
#define CC_ASM_SECTION_CONST .const
#define CC_ASM_PRIVATE_EXTERN .private_extern
#define CC_C_LABEL(_sym) _##_sym
#define _IMM(x) $$(x)
#endif                           

                                                           
#ifdef CORECRYPTO_POST_TRACE
#define CC_FIPSPOST_TRACE 1
#else
#define CC_FIPSPOST_TRACE 0
#endif

#ifndef CC_INTERNAL_SDK
#if __has_include(<System/i386/cpu_capabilities.h>)
#define CC_INTERNAL_SDK 1
#elif __has_include(<System/arm/cpu_capabilities.h>)
#define CC_INTERNAL_SDK 1
#else
#define CC_INTERNAL_SDK 0
#endif
#endif

#endif                               
