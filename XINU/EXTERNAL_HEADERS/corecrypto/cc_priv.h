/* Copyright (c) (2010,2011,2012,2014,2015,2016,2017,2018,2019,2020) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CC_PRIV_H_
#define _CORECRYPTO_CC_PRIV_H_

#include <corecrypto/cc.h>
#include <stdbool.h>
#include <stdint.h>

                                                           
void cc_atfork_prepare(void);
void cc_atfork_parent(void);
void cc_atfork_child(void);

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef __DECONST
#define __DECONST(type, var) ((type)(uintptr_t)(const void *)(var))
#endif

                                 

                                                         

                                                                      
                                                                         
                                                                      
                                                                         

                                                                      
                                                                         
                                                                      
                                                                         

                                                                
                                                               
                                                                 
                                                                

                                                                  
                                                                 
                                                                   
                                                                  

                                          

                                                                       
                                                                          

                                           

                                                  

                                                    
                                                    

                                                                      
                                                                         

  

                                                       
#define CC_MEMCPY(D,S,L) cc_memcpy((D),(S),(L))
#define CC_MEMMOVE(D,S,L) cc_memmove((D),(S),(L))
#define CC_MEMSET(D,V,L) cc_memset((D),(V),(L))

#if __has_builtin(__builtin___memcpy_chk) && !defined(_MSC_VER)
#define cc_memcpy(dst, src, len) __builtin___memcpy_chk((dst), (src), (len), __builtin_object_size((dst), 1))
#define cc_memcpy_nochk(dst, src, len) __builtin___memcpy_chk((dst), (src), (len), __builtin_object_size((dst), 0))
#else
#define cc_memcpy(dst, src, len) memcpy((dst), (src), (len))
#define cc_memcpy_nochk(dst, src, len) memcpy((dst), (src), (len))
#endif

#if __has_builtin(__builtin___memmove_chk) && !defined(_MSC_VER)
#define cc_memmove(dst, src, len) __builtin___memmove_chk((dst), (src), (len), __builtin_object_size((dst), 1))
#else
#define cc_memmove(dst, src, len) memmove((dst), (src), (len))
#endif

#if __has_builtin(__builtin___memset_chk) && !defined(_MSC_VER)
#define cc_memset(dst, val, len) __builtin___memset_chk((dst), (val), (len), __builtin_object_size((dst), 1))
#else
#define cc_memset(dst, val, len) memset((dst), (val), (len))
#endif

#define CC_ARRAY_LEN(x) (sizeof((x))/sizeof((x)[0]))

                          

                                   

                            

#define	CC_STORE32_LE(x, y) do {                                    \
    ((unsigned char *)(y))[3] = (unsigned char)(((x)>>24)&255);		\
    ((unsigned char *)(y))[2] = (unsigned char)(((x)>>16)&255);		\
    ((unsigned char *)(y))[1] = (unsigned char)(((x)>>8)&255);		\
    ((unsigned char *)(y))[0] = (unsigned char)((x)&255);			\
} while(0)

#define	CC_LOAD32_LE(x, y) do {                                     \
x = ((uint32_t)(((const unsigned char *)(y))[3] & 255)<<24) |			    \
    ((uint32_t)(((const unsigned char *)(y))[2] & 255)<<16) |			    \
    ((uint32_t)(((const unsigned char *)(y))[1] & 255)<<8)  |			    \
    ((uint32_t)(((const unsigned char *)(y))[0] & 255));				    \
} while(0)

                                   

#define	CC_STORE64_LE(x, y) do {                                    \
    ((unsigned char *)(y))[7] = (unsigned char)(((x)>>56)&255);     \
    ((unsigned char *)(y))[6] = (unsigned char)(((x)>>48)&255);		\
    ((unsigned char *)(y))[5] = (unsigned char)(((x)>>40)&255);		\
    ((unsigned char *)(y))[4] = (unsigned char)(((x)>>32)&255);		\
    ((unsigned char *)(y))[3] = (unsigned char)(((x)>>24)&255);		\
    ((unsigned char *)(y))[2] = (unsigned char)(((x)>>16)&255);		\
    ((unsigned char *)(y))[1] = (unsigned char)(((x)>>8)&255);		\
    ((unsigned char *)(y))[0] = (unsigned char)((x)&255);			\
} while(0)

#define	CC_LOAD64_LE(x, y) do {                                     \
x = (((uint64_t)(((const unsigned char *)(y))[7] & 255))<<56) |           \
    (((uint64_t)(((const unsigned char *)(y))[6] & 255))<<48) |           \
    (((uint64_t)(((const unsigned char *)(y))[5] & 255))<<40) |           \
    (((uint64_t)(((const unsigned char *)(y))[4] & 255))<<32) |           \
    (((uint64_t)(((const unsigned char *)(y))[3] & 255))<<24) |           \
    (((uint64_t)(((const unsigned char *)(y))[2] & 255))<<16) |           \
    (((uint64_t)(((const unsigned char *)(y))[1] & 255))<<8)  |           \
    (((uint64_t)(((const unsigned char *)(y))[0] & 255)));                \
} while(0)

                                
                          

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)

#define CC_STORE32_BE(x, y)     \
    __asm__ __volatile__ (      \
    "bswapl %0     \n\t"        \
    "movl   %0,(%1)\n\t"        \
    "bswapl %0     \n\t"        \
    ::"r"(x), "r"(y))

#define CC_LOAD32_BE(x, y)      \
    __asm__ __volatile__ (      \
    "movl (%1),%0\n\t"          \
    "bswapl %0\n\t"             \
    :"=r"(x): "r"(y))

#else
                            
#define	CC_STORE32_BE(x, y) do {                                \
    ((unsigned char *)(y))[0] = (unsigned char)(((x)>>24)&255);	\
    ((unsigned char *)(y))[1] = (unsigned char)(((x)>>16)&255);	\
    ((unsigned char *)(y))[2] = (unsigned char)(((x)>>8)&255);	\
    ((unsigned char *)(y))[3] = (unsigned char)((x)&255);       \
} while(0)

#define	CC_LOAD32_BE(x, y) do {                             \
x = ((uint32_t)(((const unsigned char *)(y))[0] & 255)<<24) |	    \
    ((uint32_t)(((const unsigned char *)(y))[1] & 255)<<16) |		\
    ((uint32_t)(((const unsigned char *)(y))[2] & 255)<<8)  |		\
    ((uint32_t)(((const unsigned char *)(y))[3] & 255));          \
} while(0)

#endif

                                

                                  

#if defined(__x86_64__) && !defined (_MSC_VER)

#define	CC_STORE64_BE(x, y)   \
__asm__ __volatile__ (        \
"bswapq %0     \n\t"          \
"movq   %0,(%1)\n\t"          \
"bswapq %0     \n\t"          \
::"r"(x), "r"(y))

#define	CC_LOAD64_BE(x, y)    \
__asm__ __volatile__ (        \
"movq (%1),%0\n\t"            \
"bswapq %0\n\t"               \
:"=r"(x): "r"(y))

#else

                            

#define CC_STORE64_BE(x, y) do {                                    \
    ((unsigned char *)(y))[0] = (unsigned char)(((x)>>56)&255);		\
    ((unsigned char *)(y))[1] = (unsigned char)(((x)>>48)&255);		\
    ((unsigned char *)(y))[2] = (unsigned char)(((x)>>40)&255);		\
    ((unsigned char *)(y))[3] = (unsigned char)(((x)>>32)&255);		\
    ((unsigned char *)(y))[4] = (unsigned char)(((x)>>24)&255);		\
    ((unsigned char *)(y))[5] = (unsigned char)(((x)>>16)&255);		\
    ((unsigned char *)(y))[6] = (unsigned char)(((x)>>8)&255);		\
    ((unsigned char *)(y))[7] = (unsigned char)((x)&255);			\
} while(0)

#define	CC_LOAD64_BE(x, y) do {                                     \
x = (((uint64_t)(((const unsigned char *)(y))[0] & 255))<<56) |           \
    (((uint64_t)(((const unsigned char *)(y))[1] & 255))<<48) |           \
    (((uint64_t)(((const unsigned char *)(y))[2] & 255))<<40) |           \
    (((uint64_t)(((const unsigned char *)(y))[3] & 255))<<32) |           \
    (((uint64_t)(((const unsigned char *)(y))[4] & 255))<<24) |           \
    (((uint64_t)(((const unsigned char *)(y))[5] & 255))<<16) |           \
    (((uint64_t)(((const unsigned char *)(y))[6] & 255))<<8)  |          	\
    (((uint64_t)(((const unsigned char *)(y))[7] & 255)));	            \
} while(0)

#endif

                         

#if defined(_MSC_VER)
                        

#include <stdlib.h>
#if !defined(__clang__)
 #pragma intrinsic(_lrotr,_lrotl)
#endif
#define	CC_ROR(x,n) _lrotr(x,n)
#define	CC_ROL(x,n) _lrotl(x,n)
#define	CC_RORc(x,n) _lrotr(x,n)
#define	CC_ROLc(x,n) _lrotl(x,n)

#elif (defined(__i386__) || defined(__x86_64__))
                             

CC_INLINE uint32_t CC_ROL(uint32_t word, int i)
{
    __asm__ ("roll %%cl,%0"
         :"=r" (word)
         :"0" (word),"c" (i));
    return word;
}

CC_INLINE uint32_t CC_ROR(uint32_t word, int i)
{
    __asm__ ("rorl %%cl,%0"
         :"=r" (word)
         :"0" (word),"c" (i));
    return word;
}

                                                                     
#define CC_ROLc(word, i)                \
({  uint32_t _word=(word);              \
    __asm__ __volatile__ ("roll %2,%0"  \
        :"=r" (_word)                   \
        :"0" (_word),"I" (i));          \
    _word;                              \
})


#define CC_RORc(word, i)                \
({  uint32_t _word=(word);              \
    __asm__ __volatile__ ("rorl %2,%0"  \
        :"=r" (_word)                   \
        :"0" (_word),"I" (i));          \
    _word;                              \
})

#else

                           

CC_INLINE uint32_t CC_ROL(uint32_t word, int i)
{
    return ( (word<<(i&31)) | (word>>(32-(i&31))) );
}

CC_INLINE uint32_t CC_ROR(uint32_t word, int i)
{
    return ( (word>>(i&31)) | (word<<(32-(i&31))) );
}

#define	CC_ROLc(x, y) CC_ROL(x, y)
#define	CC_RORc(x, y) CC_ROR(x, y)

#endif

                          

#if defined(__x86_64__) && !defined(_MSC_VER)                                                        
                                

CC_INLINE uint64_t CC_ROL64(uint64_t word, int i)
{
    __asm__("rolq %%cl,%0"
        :"=r" (word)
        :"0" (word),"c" (i));
    return word;
}

CC_INLINE uint64_t CC_ROR64(uint64_t word, int i)
{
    __asm__("rorq %%cl,%0"
        :"=r" (word)
        :"0" (word),"c" (i));
    return word;
}

                                                                     
#define CC_ROL64c(word, i)      \
({                              \
    uint64_t _word=(word);      \
    __asm__("rolq %2,%0"        \
        :"=r" (_word)           \
        :"0" (_word),"J" (i));  \
    _word;                      \
})

#define CC_ROR64c(word, i)      \
({                              \
    uint64_t _word=(word);      \
    __asm__("rorq %2,%0"        \
        :"=r" (_word)           \
        :"0" (_word),"J" (i));  \
    _word;                      \
})


#else                  

                             

CC_INLINE uint64_t CC_ROL64(uint64_t word, int i)
{
    return ( (word<<(i&63)) | (word>>(64-(i&63))) );
}

CC_INLINE uint64_t CC_ROR64(uint64_t word, int i)
{
    return ( (word>>(i&63)) | (word<<(64-(i&63))) );
}

#define	CC_ROL64c(x, y) CC_ROL64(x, y)
#define	CC_ROR64c(x, y) CC_ROR64(x, y)

#endif


                     

#if __has_builtin(__builtin_bswap32)
#define CC_BSWAP32(x) __builtin_bswap32(x)
#else
CC_INLINE uint32_t CC_BSWAP32(uint32_t x)
{
    return
        ((x & 0xff000000) >> 24) |
        ((x & 0x00ff0000) >>  8) |
        ((x & 0x0000ff00) <<  8) |
        ((x & 0x000000ff) << 24);
}
#endif

#if __has_builtin(__builtin_bswap64)
#define CC_BSWAP64(x) __builtin_bswap64(x)
#else
CC_INLINE uint64_t CC_BSWAP64(uint64_t x)
{
    return
        ((x & 0xff00000000000000ULL) >> 56) |
        ((x & 0x00ff000000000000ULL) >> 40) |
        ((x & 0x0000ff0000000000ULL) >> 24) |
        ((x & 0x000000ff00000000ULL) >>  8) |
        ((x & 0x00000000ff000000ULL) <<  8) |
        ((x & 0x0000000000ff0000ULL) << 24) |
        ((x & 0x000000000000ff00ULL) << 40) |
        ((x & 0x00000000000000ffULL) << 56);
}
#endif

#ifdef __LITTLE_ENDIAN__
#define CC_H2BE32(x) CC_BSWAP32(x)
#define CC_H2LE32(x) (x)
#define CC_H2BE64(x) CC_BSWAP64(x)
#define CC_H2LE64(x) (x)
#else
#define CC_H2BE32(x) (x)
#define CC_H2LE32(x) CC_BSWAP32(x)
#define CC_H2BE64(x) (x)
#define CC_H2LE64(x) CC_BSWAP64(x)
#endif

#define	CC_READ_LE32(ptr) \
( (uint32_t)( \
((uint32_t)((const uint8_t *)(ptr))[0]) | \
(((uint32_t)((const uint8_t *)(ptr))[1]) <<  8) | \
(((uint32_t)((const uint8_t *)(ptr))[2]) << 16) | \
(((uint32_t)((const uint8_t *)(ptr))[3]) << 24)))

#define	CC_WRITE_LE32(ptr, x) \
do { \
((uint8_t *)(ptr))[0] = (uint8_t)( (x)        & 0xFF); \
((uint8_t *)(ptr))[1] = (uint8_t)(((x) >>  8) & 0xFF); \
((uint8_t *)(ptr))[2] = (uint8_t)(((x) >> 16) & 0xFF); \
((uint8_t *)(ptr))[3] = (uint8_t)(((x) >> 24) & 0xFF); \
} while(0)

#define	CC_WRITE_LE64(ptr, x) \
do { \
((uint8_t *)(ptr))[0] = (uint8_t)( (x)        & 0xFF); \
((uint8_t *)(ptr))[1] = (uint8_t)(((x) >>  8) & 0xFF); \
((uint8_t *)(ptr))[2] = (uint8_t)(((x) >> 16) & 0xFF); \
((uint8_t *)(ptr))[3] = (uint8_t)(((x) >> 24) & 0xFF); \
((uint8_t *)(ptr))[4] = (uint8_t)(((x) >> 32) & 0xFF); \
((uint8_t *)(ptr))[5] = (uint8_t)(((x) >> 40) & 0xFF); \
((uint8_t *)(ptr))[6] = (uint8_t)(((x) >> 48) & 0xFF); \
((uint8_t *)(ptr))[7] = (uint8_t)(((x) >> 56) & 0xFF); \
} while(0)

                             
#ifdef _MSC_VER
#define cc_byte(x, n) ((unsigned char)((x) >> (8 * (n))))
#else
#define cc_byte(x, n) (((x) >> (8 * (n))) & 255)
#endif

                                              

  
                                                                   
                                                                        
                                                                            
                                                                           
  
                                                                               
                 
  
                                                                               
                                       
   

#if defined(_WIN32)
                                               
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm64__) || defined(__arm__)
                                                           
#elif defined __GNUC__
#warning Using __builtin_clz() on an unknown architecture; it may not be constant-time.
                                                                        
                                                                 
                                                                            
                                                                               
#endif

CC_INLINE CC_CONST unsigned cc_clz32_fallback(uint32_t data)
{
    unsigned int b = 0;
    unsigned int bit = 0;
                           
    for (int i = 0; i < 32; i++) {
        bit = (data >> i) & 1;
                                                                           
        b += (1 - bit);
                                                                           
                                                                   
           
        b &= (bit - 1);
    }
    return b;
}

CC_INLINE CC_CONST unsigned cc_clz64_fallback(uint64_t data)
{
    unsigned int b = 0;
    unsigned int bit = 0;
                           
    for (int i = 0; i < 64; i++) {
        bit = (data >> i) & 1;
                                                                       
        b += (1 - bit);
                                                                           
                                                                   
           
        b &= (bit - 1);
    }
    return b;
}

CC_INLINE CC_CONST unsigned cc_ctz32_fallback(uint32_t data)
{
    unsigned int b = 0;
    unsigned int bit = 0;
                           
    for (int i = 31; i >= 0; i--) {
        bit = (data >> i) & 1;
                                                                    
        b += (1 - bit);
                                                                           
                                                                   
           
        b &= (bit - 1);
    }
    return b;
}

CC_INLINE CC_CONST unsigned cc_ctz64_fallback(uint64_t data)
{
    unsigned int b = 0;
    unsigned int bit = 0;
                           
    for (int i = 63; i >= 0; i--) {
        bit = (data >> i) & 1;
                                                                    
        b += (1 - bit);
                                                                           
                                                                   
           
        b &= (bit - 1);
    }
    return b;
}

   
                    
                                                         

                                    

                                           

                                               
  
CC_INLINE CC_CONST unsigned cc_clz32(uint32_t data) {
    cc_assert(data != 0);
#if defined(_WIN32)
    return cc_clz32_fallback(data);
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm64__) || defined(__arm__) || defined(__GNUC__)
    cc_static_assert(sizeof(unsigned) == 4, "clz relies on an unsigned int being 4 bytes");
    return (unsigned)__builtin_clz(data);
#else
    return cc_clz32_fallback(data);
#endif
}

   
                    
                                                         

                                    

                                           

                                               
  
CC_INLINE CC_CONST unsigned cc_clz64(uint64_t data) {
    cc_assert(data != 0);
#if defined(_WIN32)
    return cc_clz64_fallback(data);
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm64__) || defined(__arm__) || defined(__GNUC__)
    return (unsigned)__builtin_clzll(data);
#else
    return cc_clz64_fallback(data);
#endif
}

   
                    
                                                          

                                    

                                            

                                               
  
CC_INLINE CC_CONST unsigned cc_ctz32(uint32_t data) {
    cc_assert(data != 0);
#if defined(_WIN32)
    return cc_ctz32_fallback(data);
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm64__) || defined(__arm__) || defined(__GNUC__)
    cc_static_assert(sizeof(unsigned) == 4, "ctz relies on an unsigned int being 4 bytes");
    return (unsigned)__builtin_ctz(data);
#else
    return cc_ctz32_fallback(data);
#endif
}

   
                    
                                                          

                                    

                                            

                                               
  
CC_INLINE CC_CONST unsigned cc_ctz64(uint64_t data) {
    cc_assert(data != 0);
#if defined(_WIN32)
    return cc_ctz64_fallback(data);
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm64__) || defined(__arm__) || defined(__GNUC__)
    return (unsigned)__builtin_ctzll(data);
#else
    return cc_ctz64_fallback(data);
#endif
}

   
                             
                                                

                            

                                                                                                     
   
CC_INLINE CC_CONST unsigned cc_ffs32_fallback(int32_t data)
{
    unsigned b = 0;
    unsigned bit = 0;
    unsigned seen = 0;

                           
    for (int i = 0; i < 32; i++) {
        bit = ((uint32_t)data >> i) & 1;

                                            
        seen |= bit;

                                                                    
        b += (1 - bit) & (seen - 1);
    }

                                           
    return (~(seen - 1)) & (b + 1);
}

   
                             
                                                

                            

                                                                                                     
   
CC_INLINE CC_CONST unsigned cc_ffs64_fallback(int64_t data)
{
    unsigned b = 0;
    unsigned bit = 0;
    unsigned seen = 0;

                           
    for (int i = 0; i < 64; i++) {
        bit = ((uint64_t)data >> i) & 1;

                                            
        seen |= bit;

                                                                    
        b += (1 - bit) & (seen - 1);
    }

                                           
    return (~(seen - 1)) & (b + 1);
}

   
                    
                                                

                            

                                                                                                     
   
CC_INLINE CC_CONST unsigned cc_ffs32(int32_t data)
{
    cc_static_assert(sizeof(int) == 4, "ffs relies on an int being 4 bytes");
#ifdef _WIN32
    return cc_ffs32_fallback(data);
#else
    return (unsigned)__builtin_ffs(data);
#endif
}

   
                    
                                                

                            

                                                                                                     
   
CC_INLINE CC_CONST unsigned cc_ffs64(int64_t data)
{
#ifdef _WIN32
    return cc_ffs64_fallback(data);
#else
    return (unsigned)__builtin_ffsll(data);
#endif
}

#define cc_add_overflow __builtin_add_overflow
#define cc_mul_overflow __builtin_mul_overflow

                                  
                                
                                
                                           
                  
                                           
                          
                                              
                                     
  
#define CC_HEAVISIDE_STEP(r, s) {                       \
    const uint64_t _s = (uint64_t)s;                    \
    const uint64_t _t = (_s & 0xffffffff) | (_s >> 32); \
    r = (__typeof__(r))((0xffffffff + _t) >> 32);       \
}

                                             
#define CC_CARRY_2BITS(x) (((x>>1) | x) & 0x1)
#define CC_CARRY_3BITS(x) (((x>>2) | (x>>1) | x) & 0x1)

#define cc_ceiling(a,b)  (((a)+((b)-1))/(b))
#define CC_BITLEN_TO_BYTELEN(x) cc_ceiling((x), 8)

   
                                                                                             
                       
                       
                                                                                                                                                          
                                                
   
void *cc_muxp(int s, const void *a, const void *b);

   
                                                                                             
                   
                   
                                                  
                                                 
   
#define CC_MUXU(r, s, a, b)                           \
    {                                                 \
        __typeof__(r) _cond = (__typeof__(r))((s)-1); \
        r = (~_cond & (a)) | (_cond & (b));           \
    }

#define CC_PROVIDES_ABORT (!(CC_USE_SEPROM || CC_USE_S3 || CC_BASEBAND || CC_EFI || CC_IBOOT || CC_RTKITROM))

   
                   
                                          
   
CC_NORETURN
void cc_abort(const char *msg);

   
                        
                                                                                              

                                                                                                                                                                                  
  
#if CC_PROVIDES_ABORT

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

CC_INLINE
void cc_try_abort(const char *msg)
{
    cc_abort(msg);
}

#pragma clang diagnostic pop

#else

CC_INLINE
void cc_try_abort(CC_UNUSED const char *msg)
{

}

#endif

#if __has_builtin(__builtin_expect)
 #define CC_UNLIKELY(cond) __builtin_expect(cond, 0)
#else
 #define CC_UNLIKELY(cond) cond
#endif

CC_INLINE
void cc_try_abort_if(bool condition, const char *msg)
{
    if (CC_UNLIKELY(condition)) {
        cc_try_abort(msg);
    }
}

  
                                                                    
                                           

                                             
  
extern const void *fipspost_trace_vtable;

#endif                             
