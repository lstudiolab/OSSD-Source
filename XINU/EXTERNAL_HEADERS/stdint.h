/*
 * Copyright (c) 2000-2010 Apple Inc.
 * All rights reserved.
 */

#ifndef _KERNEL_STDINT_H_
#define _KERNEL_STDINT_H_

#ifndef KERNEL
                                                      
#include_next <stdint.h>
#else             

#include <machine/types.h>

#if __LP64__
#define __WORDSIZE 64
#else
#define __WORDSIZE 32
#endif

                                

                                        
                                                                                     
                                                                                      
                                                                                      
                                                                                      
typedef u_int8_t              uint8_t;                                                 
typedef u_int16_t            uint16_t;                                                  
typedef u_int32_t            uint32_t;                                                  
typedef u_int64_t            uint64_t;                                                  


                                          
typedef int8_t           int_least8_t;
typedef int16_t         int_least16_t;
typedef int32_t         int_least32_t;
typedef int64_t         int_least64_t;
typedef uint8_t         uint_least8_t;
typedef uint16_t       uint_least16_t;
typedef uint32_t       uint_least32_t;
typedef uint64_t       uint_least64_t;


                                          
typedef int8_t            int_fast8_t;
typedef int16_t          int_fast16_t;
typedef int32_t          int_fast32_t;
typedef int64_t          int_fast64_t;
typedef uint8_t          uint_fast8_t;
typedef uint16_t        uint_fast16_t;
typedef uint32_t        uint_fast32_t;
typedef uint64_t        uint_fast64_t;


                                                               
                                                                                      
                                                                                       


                                           
#ifdef __INTMAX_TYPE__
typedef __INTMAX_TYPE__ intmax_t;
#else
#ifdef __LP64__
typedef long int intmax_t;
#else
typedef long long int intmax_t;
#endif               
#endif                      
#ifdef __UINTMAX_TYPE__
typedef __UINTMAX_TYPE__ uintmax_t;
#else
#ifdef __LP64__
typedef long unsigned int uintmax_t;
#else
typedef long long unsigned int uintmax_t;
#endif               
#endif                       

                                         
#define INT8_C(v)    (v)
#define INT16_C(v)   (v)
#define INT32_C(v)   (v)
#define INT64_C(v)   (v ## LL)

#define UINT8_C(v)   (v)
#define UINT16_C(v)  (v)
#define UINT32_C(v)  (v ## U)
#define UINT64_C(v)  (v ## ULL)

#ifdef __LP64__
#define INTMAX_C(v)  (v ## L)
#define UINTMAX_C(v) (v ## UL)
#else
#define INTMAX_C(v)  (v ## LL)
#define UINTMAX_C(v) (v ## ULL)
#endif

                                                  
                                                          
                                         
  
                                                                    
                                                                        
                
   


                                                  
#define INT8_MAX         127
#define INT16_MAX        32767
#define INT32_MAX        2147483647
#define INT64_MAX        9223372036854775807LL

#define INT8_MIN          -128
#define INT16_MIN         -32768
     
                                                                      
                                                                     
                                                                     
                                                                        
     
#define INT32_MIN        (-INT32_MAX-1)
#define INT64_MIN        (-INT64_MAX-1)

#define UINT8_MAX         255
#define UINT16_MAX        65535
#define UINT32_MAX        4294967295U
#define UINT64_MAX        18446744073709551615ULL

                                                    
#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST64_MIN   INT64_MIN

#define INT_LEAST8_MAX    INT8_MAX
#define INT_LEAST16_MAX   INT16_MAX
#define INT_LEAST32_MAX   INT32_MAX
#define INT_LEAST64_MAX   INT64_MAX

#define UINT_LEAST8_MAX   UINT8_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define UINT_LEAST32_MAX  UINT32_MAX
#define UINT_LEAST64_MAX  UINT64_MAX

                                                            
#define INT_FAST8_MIN     INT8_MIN
#define INT_FAST16_MIN    INT16_MIN
#define INT_FAST32_MIN    INT32_MIN
#define INT_FAST64_MIN    INT64_MIN

#define INT_FAST8_MAX     INT8_MAX
#define INT_FAST16_MAX    INT16_MAX
#define INT_FAST32_MAX    INT32_MAX
#define INT_FAST64_MAX    INT64_MAX

#define UINT_FAST8_MAX    UINT8_MAX
#define UINT_FAST16_MAX   UINT16_MAX
#define UINT_FAST32_MAX   UINT32_MAX
#define UINT_FAST64_MAX   UINT64_MAX

                                                                         

#if __WORDSIZE == 64
#define INTPTR_MAX        9223372036854775807L
#else
#define INTPTR_MAX        2147483647L
#endif
#define INTPTR_MIN        (-INTPTR_MAX-1)

#if __WORDSIZE == 64
#define UINTPTR_MAX       18446744073709551615UL
#else
#define UINTPTR_MAX       4294967295UL
#endif

                                                     
#define INTMAX_MAX        INTMAX_C(9223372036854775807)
#define UINTMAX_MAX       UINTMAX_C(18446744073709551615)
#define INTMAX_MIN        (-INTMAX_MAX-1)

                    
#if __WORDSIZE == 64
#define PTRDIFF_MIN	  INTMAX_MIN
#define PTRDIFF_MAX	  INTMAX_MAX
#else
#define PTRDIFF_MIN       INT32_MIN
#define PTRDIFF_MAX       INT32_MAX
#endif

#define SIZE_MAX          UINTPTR_MAX

#if defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ >= 1
#define RSIZE_MAX         (SIZE_MAX >> 1)
#endif

#ifndef WCHAR_MAX
#  ifdef __WCHAR_MAX__
#    define WCHAR_MAX     __WCHAR_MAX__
#  else
#    define WCHAR_MAX     0x7fffffff
#  endif
#endif

                                                           
                                                              
                                                            
               
#ifndef WCHAR_MIN
#  if WCHAR_MAX == 0xffff
#    define WCHAR_MIN       0
#  else
#    define WCHAR_MIN       (-WCHAR_MAX-1)
#  endif
#endif

#define WINT_MIN	  INT32_MIN
#define WINT_MAX	  INT32_MAX

#define SIG_ATOMIC_MIN	  INT32_MIN
#define SIG_ATOMIC_MAX	  INT32_MAX

#endif             

#endif                        
