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

#ifndef _CORECRYPTO_CCN_H_
#define _CORECRYPTO_CCN_H_

#include <corecrypto/cc.h>
#include <stdint.h>
#include <stdarg.h>

typedef uint8_t cc_byte;
typedef size_t  cc_size;

#if  CCN_UNIT_SIZE == 8
typedef uint64_t cc_unit;                        
typedef int64_t  cc_int;
#define CCN_LOG2_BITS_PER_UNIT  6                  
#define CC_UNIT_C(x) UINT64_C(x)
 #if  CCN_UINT128_SUPPORT_FOR_64BIT_ARCH
   typedef unsigned cc_dunit __attribute__((mode(TI)));                                     
   typedef   signed cc_dint  __attribute__((mode(TI)));
 #else
   typedef struct cc_dunit {
    uint64_t l;                                                                           
    uint64_t h;
   } cc_dunit;

   typedef struct cc_dint {
    uint64_t l;
    uint64_t h;
   } cc_dint;
 #endif

#elif  CCN_UNIT_SIZE == 4
typedef uint32_t cc_unit;                        
typedef uint64_t cc_dunit;                                    
typedef int64_t cc_dint;
typedef int32_t cc_int;
#define CCN_LOG2_BITS_PER_UNIT  5                  
#define CC_UNIT_C(x) UINT32_C(x)

#elif CCN_UNIT_SIZE == 2
typedef uint16_t cc_unit;                        
typedef uint32_t cc_dunit;                                    
#define CCN_LOG2_BITS_PER_UNIT  4                  
#define CC_UNIT_C(x) UINT16_C(x)

#elif CCN_UNIT_SIZE == 1
typedef uint8_t cc_unit;                        
typedef uint16_t cc_dunit;                                    
#define CCN_LOG2_BITS_PER_UNIT  3                 
#define CC_UNIT_C(x) UINT8_C(x)

#else
#error invalid CCN_UNIT_SIZE
#endif

#define CCN_UNIT_BITS  (sizeof(cc_unit) * 8)
#define CCN_UNIT_MASK  ((cc_unit)~0)
#define CCN_UNIT_LOWER_HALF_MASK  ((CCN_UNIT_MASK) >> (CCN_UNIT_BITS/2))
#define CCN_UNIT_UPPER_HALF_MASK  (~CCN_UNIT_LOWER_HALF_MASK)
#define CCN_UNIT_HALF_BITS (CCN_UNIT_BITS / 2)

typedef struct {
    cc_unit *start;                                       
    cc_unit *end;                                               
} cc_ws,*cc_ws_t;

                                           

                                                          
#define ccn_sizeof_n(_n_)  (sizeof(cc_unit) * (_n_))

                                                                      
#define ccn_nof(_bits_)  (((_bits_) + CCN_UNIT_BITS - 1) >> CCN_LOG2_BITS_PER_UNIT)

                                                                
#define ccn_sizeof(_bits_)  (ccn_sizeof_n(ccn_nof(_bits_)))

                                                                            
#define ccn_nof_size(_size_)  (((_size_) + CCN_UNIT_SIZE - 1) / CCN_UNIT_SIZE)

#define ccn_nof_sizeof(_expr_) ccn_nof_size(sizeof (_expr_))

                                                                       
#define ccn_bitsof_n(_n_)  ((_n_) * CCN_UNIT_BITS)

                                                                          
#define ccn_bitsof_size(_size_)  ((_size_) * 8)

                                                      
#define ccn_sizeof_size(_size_)  ccn_sizeof_n(ccn_nof_size(_size_))

                                                                           
#define ccn_bit(_ccn_, _k_) ({size_t __k = (size_t)(_k_); \
    1 & ((_ccn_)[ __k >> CCN_LOG2_BITS_PER_UNIT] >> (__k & (CCN_UNIT_BITS - 1)));})

                                                         
#define ccn_set_bit(_ccn_, _k_, _v_) ({size_t __k = (size_t)(_k_);          \
    if (_v_)                                                                \
        (_ccn_)[ __k >> CCN_LOG2_BITS_PER_UNIT] |= CC_UNIT_C(1) << (__k & (CCN_UNIT_BITS - 1));     \
    else                                                                    \
        (_ccn_)[ __k >> CCN_LOG2_BITS_PER_UNIT] &= ~(CC_UNIT_C(1) << (__k & (CCN_UNIT_BITS - 1)));  \
    })

                                                                             
                                                                         
                                                                        
                           
#define CCN8_C(a0) CC_UNIT_C(0x##a0)

#if CCN_UNIT_SIZE >= 2
#define CCN16_C(a1,a0) CC_UNIT_C(0x##a1##a0)
#define ccn16_v(a0)  (a0)
#elif CCN_UNIT_SIZE == 1
#define CCN16_C(a1,a0) CCN8_C(a0),CCN8_C(a1)
#define ccn16_v(a0)  (a0 & UINT8_C(0xff)),(a0 >> 8)
#endif

#if CCN_UNIT_SIZE >= 4
#define CCN32_C(a3,a2,a1,a0) CC_UNIT_C(0x##a3##a2##a1##a0)
#define ccn32_v(a0)  (a0)
#else
#define CCN32_C(a3,a2,a1,a0) CCN16_C(a1,a0),CCN16_C(a3,a2)
#define ccn32_v(a0)  ccn16_v(a0 & UINT16_C(0xffff)),ccn16_v(a0 >> 16)
#endif

#if CCN_UNIT_SIZE == 8
#define CCN64_C(a7,a6,a5,a4,a3,a2,a1,a0) CC_UNIT_C(0x##a7##a6##a5##a4##a3##a2##a1##a0)
#define CCN40_C(a4,a3,a2,a1,a0) CC_UNIT_C(0x##a4##a3##a2##a1##a0)
#define ccn64_v(a0)  (a0)
                                            
                                
#else
#define CCN64_C(a7,a6,a5,a4,a3,a2,a1,a0) CCN32_C(a3,a2,a1,a0),CCN32_C(a7,a6,a5,a4)
#define CCN40_C(a4,a3,a2,a1,a0) CCN32_C(a3,a2,a1,a0),CCN8_C(a4)
#define ccn64_v(a0)  ccn32_v((uint64_t)a0 & UINT32_C(0xffffffff)),ccn32_v((uint64_t)a0 >> 32)
                                                  
                                                                                                       
#endif

                                                                             
                                
#if CCN_UNIT_SIZE == 8
                                                                            
                                                                        
                                                                        
                                                
  
                                                                                                                  
#elif CCN_UNIT_SIZE == 4
                           
                           
                                                                                                              
                                
#elif CCN_UNIT_SIZE == 2
                           
                                 
                                
                                                                                  
#elif CCN_UNIT_SIZE == 1
                                                     
                                                                        
                                                                                                       
#endif

                                                                             
                              
#if CCN_UNIT_SIZE == 8

#define ccn64_32(a1,a0) (((const cc_unit)a1) << 32 | ((const cc_unit)a0))
#define ccn32_32(a0) a0
#if __LITTLE_ENDIAN__
#define ccn32_32_parse(p,i) (((const uint32_t *)p)[i])
#else
#define ccn32_32_parse(p,i) (((const uint32_t *)p)[i^1])
#endif
#define ccn32_32_null 0

#define ccn64_64(a0) a0
#define ccn64_64_parse(p,i) p[i]
#define ccn64_64_null 0

#elif CCN_UNIT_SIZE == 4

#define ccn32_32(a0) a0
#define ccn32_32_parse(p,i) p[i]
#define ccn32_32_null 0
#define ccn64_32(a1,a0) ccn32_32(a0),ccn32_32(a1)

#define ccn64_64(a1,a0) a0,a1
#define ccn64_64_parse(p,i) p[1+(i<<1)],p[i<<1]
#define ccn64_64_null 0,0

#elif CCN_UNIT_SIZE == 2

#define ccn32_32(a1,a0) a0,a1
#define ccn32_32_parse(p,i) p[1+(i<<1)],p[i<<1]
#define ccn32_32_null 0,0
#define ccn64_32(a3,a2,a1,a0) ccn32_32(a1,a0),ccn32_32(a3,a2)

#define ccn64_64(a3,a2,a1,a0) a0,a1,a2,a3
#define ccn64_64_parse(p,i) p[3+(i<<2)],p[2+(i<<2)],p[1+(i<<2)],p[i<<2]
#define ccn64_64_null 0,0,0,0

#elif CCN_UNIT_SIZE == 1

#define ccn32_32(a3,a2,a1,a0) a0,a1,a2,a3
#define ccn32_32_parse(p,i) p[3+(i<<2)],p[2+(i<<2)],p[1+(i<<2)],p[i<<2]
#define ccn32_32_null 0,0,0,0
#define ccn64_32(a7,a6,a5,a4,a3,a2,a1,a0) ccn32_32(a3,a2,a1,a0),ccn32_32(a7,a6,a5,a4)

#define ccn64_64(a7,a6,a5,a4,a3,a2,a1,a0) a0,a1,a2,a3,a4,a5,a6,a7
#define ccn64_64_parse(p,i)  p[7+(i<<3)],p[6+(i<<3)],p[5+(i<<3)],p[4+(i<<3)],p[3+(i<<3)],p[2+(i<<3)],p[1+(i<<3)],p[i<<3]
#define ccn64_64_null  0,0,0,0,0,0,0,0

#endif


                                                                             
#define ccn192_64(a2,a1,a0) ccn64_64(a0),ccn64_64(a1),ccn64_64(a2)
#define ccn192_32(a5,a4,a3,a2,a1,a0) ccn64_32(a1,a0),ccn64_32(a3,a2),ccn64_32(a5,a4)
#define ccn224_32(a6,a5,a4,a3,a2,a1,a0) ccn64_32(a1,a0),ccn64_32(a3,a2),ccn64_32(a5,a4),ccn32_32(a6)
#define ccn256_32(a7,a6,a5,a4,a3,a2,a1,a0) ccn64_32(a1,a0),ccn64_32(a3,a2),ccn64_32(a5,a4),ccn64_32(a7,a6)
#define ccn384_32(a11,a10,a9,a8,a7,a6,a5,a4,a3,a2,a1,a0) ccn64_32(a1,a0),ccn64_32(a3,a2),ccn64_32(a5,a4),ccn64_32(a7,a6),ccn64_32(a9,a8),ccn64_32(a11,a10)


#define CCN192_C(c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN64_C(a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN64_C(b7,b6,b5,b4,b3,b2,b1,b0),\
    CCN64_C(c7,c6,c5,c4,c3,c2,c1,c0)

#define CCN200_C(d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN192_C(c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN8_C(d0)

#define CCN224_C(d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN192_C(c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN32_C(d3,d2,d1,d0)

#define CCN232_C(d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN192_C(c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN40_C(d4,d3,d2,d1,d0)

#define CCN256_C(d7,d6,d5,d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN192_C(c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN64_C(d7,d6,d5,d4,d3,d2,d1,d0)

#define CCN384_C(f7,f6,f5,f4,f3,f2,f1,f0,e7,e6,e5,e4,e3,e2,e1,e0,d7,d6,d5,d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN256_C(d7,d6,d5,d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN64_C(e7,e6,e5,e4,e3,e2,e1,e0),\
    CCN64_C(f7,f6,f5,f4,f3,f2,f1,f0)

#define CCN528_C(i1,i0,h7,h6,h5,h4,h3,h2,h1,h0,g7,g6,g5,g4,g3,g2,g1,g0,f7,f6,f5,f4,f3,f2,f1,f0,e7,e6,e5,e4,e3,e2,e1,e0,d7,d6,d5,d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0) \
    CCN256_C(d7,d6,d5,d4,d3,d2,d1,d0,c7,c6,c5,c4,c3,c2,c1,c0,b7,b6,b5,b4,b3,b2,b1,b0,a7,a6,a5,a4,a3,a2,a1,a0),\
    CCN256_C(h7,h6,h5,h4,h3,h2,h1,h0,g7,g6,g5,g4,g3,g2,g1,g0,f7,f6,f5,f4,f3,f2,f1,f0,e7,e6,e5,e4,e3,e2,e1,e0),\
    CCN16_C(i1,i0)

#define CCN192_N  ccn_nof(192)
#define CCN224_N  ccn_nof(224)
#define CCN256_N  ccn_nof(256)
#define CCN384_N  ccn_nof(384)
#define CCN512_N  ccn_nof(512)
#define CCN521_N  ccn_nof(521)

                                                                       
CC_PURE CC_NONNULL((2))
cc_size ccn_n(cc_size n, const cc_unit *s) __asm__("_ccn_n");

                                                                                            
                                                                                                  
                           
                                                       
                                                                              
                 
CC_NONNULL((2, 3))
cc_unit ccn_shift_right(cc_size n, cc_unit *r, const cc_unit *s, size_t k) __asm__("_ccn_shift_right");

                                                                     
                             
                                        
  
                                                          
   
CC_NONNULL((2))
size_t ccn_bitlen(cc_size n, const cc_unit *s);

                                                 
                                         
#define ccn_is_zero(_n_, _s_) (!ccn_n(_n_, _s_))

                                                 
                                         
#define ccn_is_one(_n_, _s_) (ccn_n(_n_, _s_) == 1 && _s_[0] == 1)

#define ccn_is_zero_or_one(_n_, _s_) (((_n_)==0) || ((ccn_n(_n_, _s_) <= 1) && (_s_[0] <= 1)))

                                                               
                                                       
CC_PURE CC_NONNULL((2, 3))
int ccn_cmp(cc_size n, const cc_unit *s, const cc_unit *t) __asm__("_ccn_cmp");

                      
                                                                    

                                                                               
                                                                   

                       
                         
                       
                          

                                              
   
CC_NONNULL_ALL
int ccn_cmpn(cc_size ns, const cc_unit *s, cc_size nt, const cc_unit *t);

                                
                                                         
CC_NONNULL((2, 3, 4))
cc_unit ccn_sub(cc_size n, cc_unit *r, const cc_unit *s, const cc_unit *t) __asm__("_ccn_sub");

                                                    
                                                                           
CC_NONNULL((2, 3))
cc_unit ccn_sub1(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v);

                                
                                                                  
CC_INLINE
CC_NONNULL((2, 3, 5))
cc_unit ccn_subn(cc_size n, cc_unit *r, const cc_unit *s,
             cc_size nt, const cc_unit *t) {
    assert(n >= nt);
    return ccn_sub1(n - nt, r + nt, s + nt, ccn_sub(nt, r, s, t));
}


                                                           
                                                         
CC_NONNULL((2, 3, 4))
cc_unit ccn_add(cc_size n, cc_unit *r, const cc_unit *s, const cc_unit *t) __asm__("_ccn_add");

                                                           
                                                                           
CC_NONNULL((2, 3))
cc_unit ccn_add1(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v);

                                                          
                                                                  
CC_INLINE
CC_NONNULL((2, 3, 5))
cc_unit ccn_addn(cc_size n, cc_unit *r, const cc_unit *s,
                 cc_size nt, const cc_unit *t) {
    assert(n >= nt);
    return ccn_add1(n - nt, r + nt, s + nt, ccn_add(nt, r, s, t));
}


                                                                     
                                                              
                                                
CC_NONNULL((2, 3, 4))
void ccn_mul(cc_size n, cc_unit *r_2n, const cc_unit *s, const cc_unit *t) __asm__("_ccn_mul");

                                      
                                                                                                 
CC_NONNULL((2, 3))
cc_unit ccn_mul1(cc_size n, cc_unit *r, const cc_unit *s, const cc_unit v);

                                                
                                                                                                 
CC_NONNULL((2, 3))
cc_unit ccn_addmul1(cc_size n, cc_unit *r, const cc_unit *s, const cc_unit v);


   
                          
                                                                 
 
                                                                      
                                                            
                                           
                                                                    
 
                                                      
 
                                                                                                   
 
                                                                                                                                
                   
   

CC_NONNULL((2, 4))
int ccn_read_uint(cc_size n, cc_unit *r, size_t data_nbytes, const uint8_t *data);

                                                                        
                                      
                                                                    
#define ccn_read_int(n, r, data_size, data) ccn_read_uint(n, r, data_size, data)

   
                                
                                                                      
 
                                                                          
                                       
 
                                                               
 
            
                                                                   
                                                                                          
   
CC_PURE CC_NONNULL((2)) size_t ccn_write_uint_size(cc_size n, const cc_unit *s);

   
                           
                                                                    
 
                                                                          
                                       
                                             
                                                                

                                         
                                                                    
                                                                   
                                                                
                                         

   

CC_NONNULL((2, 4))
void ccn_write_uint(cc_size n, const cc_unit *s, size_t out_size, void *out);

   
                                     
                                                                    
 
                                                                          
                                       
                                             
                                                                
 
                                                                                                   
 
                                                                                         
                                                                                                 
 
                                                                                          
                                                                                     
                          
   
CC_NONNULL((2, 4))
int ccn_write_uint_padded_ct(cc_size n, const cc_unit *s, size_t out_size, uint8_t *out);

   
                                  
                                                                    
                                                                             
                                                                                           
 
                                                                          
                                       
                                             
                                                                
 
                                     
 
                                                                                         
                                                                          
 
                                                                                                        
                             
   

CC_INLINE CC_NONNULL((2, 4)) size_t ccn_write_uint_padded(cc_size n, const cc_unit *s, size_t out_size, uint8_t *out)
{
    size_t offset = 0;
                                        
    int offset_int = ccn_write_uint_padded_ct(n, s, out_size, out);
    if (offset_int >= 0) {
                    
        offset = (size_t)offset_int;
    } else {
                                                                           
        ccn_write_uint(n, s, out_size, out);
    }
    return offset;
}


                                                            
                                                
CC_PURE CC_NONNULL((2))
size_t ccn_write_int_size(cc_size n, const cc_unit *s);

                        
                                                    
                                      

                                                     

                                                                               
                                                                          
                                                                               
                                                                            
CC_NONNULL((2, 4))
void ccn_write_int(cc_size n, const cc_unit *s, size_t out_size, void *out);

         
                      
CC_NONNULL((2, 3))
void ccn_set(cc_size n, cc_unit *r, const cc_unit *s);

CC_INLINE CC_NONNULL((2))
void ccn_zero(cc_size n, cc_unit *r) {
    cc_clear(ccn_sizeof_n(n),r);
}

CC_INLINE CC_NONNULL((2))
void ccn_clear(cc_size n, cc_unit *r) {
    cc_clear(ccn_sizeof_n(n),r);
}

CC_NONNULL((2))
void ccn_zero_multi(cc_size n, cc_unit *r, ...);

CC_INLINE CC_NONNULL((2))
void ccn_seti(cc_size n, cc_unit *r, cc_unit v) {
    assert(n > 0);
    r[0] = v;
    ccn_zero(n - 1, r + 1);
}

CC_INLINE CC_NONNULL((2, 4))
void ccn_setn(cc_size n, cc_unit *r, const cc_size s_size, const cc_unit *s) {
    assert(n > 0);
    assert(s_size > 0);
    assert(s_size <= n);
    ccn_set(s_size, r, s);
    ccn_zero(n - s_size, r + s_size);
}

#define CC_SWAP_HOST_BIG_64(x) \
    ((uint64_t)((((uint64_t)(x) & 0xff00000000000000ULL) >> 56) | \
    (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
    (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
    (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
    (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
    (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
    (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
    (((uint64_t)(x) & 0x00000000000000ffULL) << 56)))
#define CC_SWAP_HOST_BIG_32(x) \
    ((((x) & 0xff000000) >> 24) | \
    (((x) & 0x00ff0000) >>  8) | \
    (((x) & 0x0000ff00) <<  8) | \
    (((x) & 0x000000ff) <<  24))
#define CC_SWAP_HOST_BIG_16(x) \
    ((((x) & 0xff00) >>  8) | \
    (((x) & 0x00ff) <<  8))

                                                                
#if CCN_UNIT_SIZE == 8
#define CC_UNIT_TO_BIG(x) CC_SWAP_HOST_BIG_64(x)
#elif CCN_UNIT_SIZE == 4
#define CC_UNIT_TO_BIG(x) CC_SWAP_HOST_BIG_32(x)
#elif CCN_UNIT_SIZE == 2
#define CC_UNIT_TO_BIG(x) CC_SWAP_HOST_BIG_16(x)
#elif CCN_UNIT_SIZE == 1
#define CC_UNIT_TO_BIG(x) (x)
#else
#error unsupported CCN_UNIT_SIZE
#endif

                                                                                                 
CC_INLINE CC_NONNULL((2))
void ccn_swap(cc_size n, cc_unit *r) {
    cc_unit *e;
    for (e = r + n - 1; r < e; ++r, --e) {
        cc_unit t = CC_UNIT_TO_BIG(*r);
        *r = CC_UNIT_TO_BIG(*e);
        *e = t;
    }
    if (n & 1)
        *r = CC_UNIT_TO_BIG(*r);
}

CC_INLINE CC_NONNULL((2, 3, 4))
void ccn_xor(cc_size n, cc_unit *r, const cc_unit *s, const cc_unit *t) {
    while (n--) {
        r[n] = s[n] ^ t[n];
    }
}

               
CC_NONNULL((2))
void ccn_print(cc_size n, const cc_unit *s);
CC_NONNULL((3))
void ccn_lprint(cc_size n, const char *label, const cc_unit *s);

                                                        
struct ccrng_state;

#if 0
CC_INLINE CC_NONNULL((2, 3))
int ccn_random(cc_size n, cc_unit *r, struct ccrng_state *rng) {
    return (RNG)->generate((RNG), ccn_sizeof_n(n), (unsigned char *)r);
}
#else
#define ccn_random(_n_,_r_,_ccrng_ctx_) \
    ccrng_generate(_ccrng_ctx_, ccn_sizeof_n(_n_), (unsigned char *)_r_)
#endif

                                                                                  
CC_NONNULL((2, 3))
int ccn_random_bits(cc_size nbits, cc_unit *r, struct ccrng_state *rng);

CC_NONNULL((6, 8))
int ccn_div_euclid(cc_size nq, cc_unit *q, cc_size nr, cc_unit *r, cc_size na, const cc_unit *a, cc_size nd, const cc_unit *d);

#define ccn_div(nq, q, na, a, nd, d) ccn_div_euclid(nq, q, 0, NULL, na, a, nd, d)
#define ccn_mod(nr, r, na, a, nd, d) ccn_div_euclid(0 , NULL, nr, r, na, a, nd, d)

#endif                         
