/* Copyright (c) (2010,2011,2012,2013,2014,2015,2016,2017,2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCZP_H_
#define _CORECRYPTO_CCZP_H_

#include <corecrypto/ccn.h>
#include <corecrypto/ccrng.h>

  
                                                                                           
             

                                                                                                
              
  

struct cczp;

typedef struct cczp *cczp_t;
typedef const struct cczp *cczp_const_t;

struct cczp_funcs;
typedef const struct cczp_funcs *cczp_funcs_t;

                                              
                                                        
                                                         
#define __CCZP_HEADER_ELEMENTS_DEFINITIONS(pre) \
    cc_size pre##n;                             \
    cc_unit pre##bitlen;                        \
    cczp_funcs_t pre##funcs;

#define __CCZP_ELEMENTS_DEFINITIONS(pre)    \
    __CCZP_HEADER_ELEMENTS_DEFINITIONS(pre) \
    cc_unit pre##ccn[];

                                                                                               
                                   
struct cczp_hd {
    __CCZP_HEADER_ELEMENTS_DEFINITIONS()
} CC_ALIGNED(CCN_UNIT_SIZE);

struct cczp {
    __CCZP_ELEMENTS_DEFINITIONS()
} CC_ALIGNED(CCN_UNIT_SIZE);

                                                                
#define cczp_size(_size_) (sizeof(struct cczp) + ccn_sizeof_n(1) + 2 * (_size_))

                                                                             
                                                                 
#define cczp_nof_n(_n_) (ccn_nof_size(sizeof(struct cczp)) + 1 + 2 * (_n_))

                                                                             
                        
#define cczp_decl_n(_n_, _name_) cc_ctx_decl(struct cczp, ccn_sizeof_n(cczp_nof_n(_n_)), _name_)
#define cczp_clear_n(_n_, _name_) cc_clear(ccn_sizeof_n(cczp_nof_n(_n_)), _name_)

#define CCZP_N(ZP) ((ZP)->n)
#define CCZP_PRIME(ZP) ((ZP)->ccn)
#define CCZP_BITLEN(ZP) ((ZP)->bitlen)
#define CCZP_RECIP(ZP) ((ZP)->ccn + CCZP_N(ZP))
CC_NONNULL((1)) CC_INLINE cc_size cczp_n(cczp_const_t zp)
{
    return zp->n;
}

CC_NONNULL((1)) CC_INLINE const cc_unit *cczp_prime(cczp_const_t zp)
{
    return zp->ccn;
}

CC_NONNULL((1)) CC_INLINE size_t cczp_bitlen(cczp_const_t zp)
{
    cc_assert(ccn_bitlen(cczp_n(zp), cczp_prime(zp)) == CCZP_BITLEN(zp));
    return (size_t)CCZP_BITLEN(zp);
}

                                                                            
                                        
CC_NONNULL((1)) CC_INLINE const cc_unit *cczp_recip(cczp_const_t zp)
{
    return zp->ccn + zp->n;
}

                                                                          
                                                                        
                          
CC_NONNULL((1))
int cczp_init(cczp_t zp);

                                  
                                                             

                                       
                                        
   
CC_NONNULL((1, 2))
void cczp_init_with_recip(cczp_t zp, const cc_unit *recip);

                                                                 
                                 
                                               
                                                        
                                               
                                                                            
                                                                           
                                 
   
CC_NONNULL((1, 2, 3, 4))
int cczp_power(cczp_const_t zp, cc_unit *r, const cc_unit *m, const cc_unit *e);

   
                                                                              
                                                                                               
                                                                                                  
                                                                                            
                                                                                 
                                                                                                   
                                      
                           
                            
                                                     
   
CC_NONNULL((1, 2, 3))
int cczp_inv(cczp_const_t zp, cc_unit *r, const cc_unit *x);

#endif                          
