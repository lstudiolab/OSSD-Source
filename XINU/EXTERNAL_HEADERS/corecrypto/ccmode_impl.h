/* Copyright (c) (2010,2011,2012,2015,2016,2017,2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCMODE_IMPL_H_
#define _CORECRYPTO_CCMODE_IMPL_H_

#include <corecrypto/cc.h>

               
cc_aligned_struct(16) ccecb_ctx;

                                                                                
struct ccmode_ecb {
    size_t size;                                          
    size_t block_size;
    int (*CC_SPTR(ccmode_ecb, init))(const struct ccmode_ecb *ecb, ccecb_ctx *ctx, size_t key_nbytes, const void *key);
    int (*CC_SPTR(ccmode_ecb, ecb))(const ccecb_ctx *ctx, size_t nblocks, const void *in, void *out);
    void (*CC_SPTR(ccmode_ecb, roundkey))(const ccecb_ctx *ctx, unsigned r, void *key);
};

   
                                                                
  
                                                                                                         
                                                                                                                    
                        
                           
                           
  
                                                                                                                              
                        
                         
                       
                           
                           
                            
                         
  
                                   
  
                                   
                                                 
                                   
  
                       
                       
                       
                       
   

                         
cc_aligned_struct(16) cccbc_ctx;
cc_aligned_struct(16) cccbc_iv;

struct ccmode_cbc {
    size_t size;                                          
    size_t block_size;
    int (*CC_SPTR(ccmode_cbc, init))(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, size_t key_len, const void *key);
                                                                                     
    int (*CC_SPTR(ccmode_cbc, cbc))(const cccbc_ctx *ctx, cccbc_iv *iv, size_t nblocks, const void *in, void *out);
    const void *custom;
};

                         
cc_aligned_struct(16) cccfb_ctx;

struct ccmode_cfb {
    size_t size;                                          
    size_t block_size;
    int (*CC_SPTR(ccmode_cfb,
                  init))(const struct ccmode_cfb *cfb, cccfb_ctx *ctx, size_t key_len, const void *key, const void *iv);
    int (*CC_SPTR(ccmode_cfb, cfb))(cccfb_ctx *ctx, size_t nbytes, const void *in, void *out);
    const void *custom;
};

                          
cc_aligned_struct(16) cccfb8_ctx;

struct ccmode_cfb8 {
    size_t size;                                           
    size_t block_size;
    int (*CC_SPTR(ccmode_cfb8,
                  init))(const struct ccmode_cfb8 *cfb8, cccfb8_ctx *ctx, size_t key_len, const void *key, const void *iv);
    int (*CC_SPTR(ccmode_cfb8, cfb8))(cccfb8_ctx *ctx, size_t nbytes, const void *in, void *out);
    const void *custom;
};

                         
cc_aligned_struct(16) ccctr_ctx;

struct ccmode_ctr {
    size_t size;                                                    
    size_t block_size;                                                   
    size_t ecb_block_size;                                                     
    int (*CC_SPTR(ccmode_ctr,
                  init))(const struct ccmode_ctr *mode, ccctr_ctx *ctx, size_t key_len, const void *key, const void *iv);
    int (*CC_SPTR(ccmode_ctr, setctr))(const struct ccmode_ctr *mode, ccctr_ctx *ctx, const void *ctr);
    int (*CC_SPTR(ccmode_ctr, ctr))(ccctr_ctx *ctx, size_t nbytes, const void *in, void *out);
    const void *custom;
};

                         
cc_aligned_struct(16) ccofb_ctx;

struct ccmode_ofb {
    size_t size;                                          
    size_t block_size;
    int (*CC_SPTR(ccmode_ofb,
                  init))(const struct ccmode_ofb *ofb, ccofb_ctx *ctx, size_t key_len, const void *key, const void *iv);
    int (*CC_SPTR(ccmode_ofb, ofb))(ccofb_ctx *ctx, size_t nbytes, const void *in, void *out);
    const void *custom;
};

                         
cc_aligned_struct(16) ccxts_ctx;
cc_aligned_struct(16) ccxts_tweak;

struct ccmode_xts {
    size_t size;                                                                               
    size_t tweak_size;                                                                                                     
    size_t block_size;

                                               
                                                             
                                                                         
                                   
                                
       
    int (*CC_SPTR(ccmode_xts, init))(const struct ccmode_xts *xts,
                                     ccxts_ctx *ctx,
                                     size_t key_nbytes,
                                     const void *data_key,
                                     const void *tweak_key);

    void (*CC_SPTR(ccmode_xts, key_sched))(const struct ccmode_xts *xts,
                                           ccxts_ctx *ctx,
                                           size_t key_nbytes,
                                           const void *data_key,
                                           const void *tweak_key);

                                                                          
    int (*CC_SPTR(ccmode_xts, set_tweak))(const ccxts_ctx *ctx, ccxts_tweak *tweak, const void *iv);

                                                                              
                                                             
    void *(*CC_SPTR(ccmode_xts, xts))(const ccxts_ctx *ctx, ccxts_tweak *tweak, size_t nblocks, const void *in, void *out);

    const void *custom;
    const void *custom1;
};

                       
cc_aligned_struct(16) ccgcm_ctx;
#define CCMODE_GCM_DECRYPTOR 78647
#define CCMODE_GCM_ENCRYPTOR 4073947

struct ccmode_gcm {
    size_t size;                                          
    int encdec;                                    
    size_t block_size;
    int (*CC_SPTR(ccmode_gcm, init))(const struct ccmode_gcm *gcm, ccgcm_ctx *ctx, size_t key_nbytes, const void *key);
    int (*CC_SPTR(ccmode_gcm, set_iv))(ccgcm_ctx *ctx, size_t iv_nbytes, const void *iv);
    int (*CC_SPTR(ccmode_gcm, gmac))(ccgcm_ctx *ctx, size_t nbytes, const void *in);                                   
    int (*CC_SPTR(ccmode_gcm, gcm))(ccgcm_ctx *ctx, size_t nbytes, const void *in, void *out);
    int (*CC_SPTR(ccmode_gcm, finalize))(ccgcm_ctx *key, size_t tag_nbytes, void *tag);
    int (*CC_SPTR(ccmode_gcm, reset))(ccgcm_ctx *ctx);
    const void *custom;
};

                        
cc_aligned_struct(16) ccccm_ctx;
cc_aligned_struct(16) ccccm_nonce;

struct ccmode_ccm {
    size_t size;                                                
    size_t nonce_size;                                            
    size_t block_size;
    int (*CC_SPTR(ccmode_ccm, init))(const struct ccmode_ccm *ccm, ccccm_ctx *ctx, size_t key_len, const void *key);
    int (*CC_SPTR(ccmode_ccm, set_iv))(ccccm_ctx *ctx,
                                       ccccm_nonce *nonce_ctx,
                                       size_t nonce_len,
                                       const void *nonce,
                                       size_t mac_size,
                                       size_t auth_len,
                                       size_t data_len);
    int (*CC_SPTR(ccmode_ccm, cbcmac))(ccccm_ctx *ctx,
                                       ccccm_nonce *nonce_ctx,
                                       size_t nbytes,
                                       const void *in);                                   
    int (*CC_SPTR(ccmode_ccm, ccm))(ccccm_ctx *ctx, ccccm_nonce *nonce_ctx, size_t nbytes, const void *in, void *out);
    int (*CC_SPTR(ccmode_ccm, finalize))(ccccm_ctx *key, ccccm_nonce *nonce_ctx, void *mac);
    int (*CC_SPTR(ccmode_ccm, reset))(ccccm_ctx *key, ccccm_nonce *nonce_ctx);
    const void *custom;
};

                                                                   
struct _ccmode_ccm_nonce {
    unsigned char A_i[16];                      
    unsigned char B_i[16];                   
    unsigned char MAC[16];                  
    unsigned char buf[16];                   

    uint32_t mode;                                
    uint32_t buflen;                             
    uint32_t b_i_len;                                   

    size_t nonce_size;
    size_t mac_size;
};

                
cc_aligned_struct(16) ccomac_ctx;

struct ccmode_omac {
    size_t size;                                           
    size_t block_size;
    int (*CC_SPTR(ccmode_omac,
                  init))(const struct ccmode_omac *omac, ccomac_ctx *ctx, size_t tweak_len, size_t key_len, const void *key);
    int (*CC_SPTR(ccmode_omac, omac))(ccomac_ctx *ctx, size_t nblocks, const void *tweak, const void *in, void *out);
    const void *custom;
};

#endif                                 
