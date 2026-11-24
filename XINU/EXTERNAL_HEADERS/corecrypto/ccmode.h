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

#ifndef _CORECRYPTO_CCMODE_H_
#define _CORECRYPTO_CCMODE_H_

#include <corecrypto/cc.h>
#include <corecrypto/ccmode_impl.h>
#include <corecrypto/ccmode_siv.h>
#include <corecrypto/ccmode_siv_hmac.h>

               

                                                                              
                 
#define ccecb_ctx_decl(_size_, _name_) cc_ctx_decl(ccecb_ctx, _size_, _name_)
#define ccecb_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t ccecb_context_size(const struct ccmode_ecb *mode)
{
    return mode->size;
}

CC_INLINE size_t ccecb_block_size(const struct ccmode_ecb *mode)
{
    return mode->block_size;
}

CC_INLINE int ccecb_init(const struct ccmode_ecb *mode, ccecb_ctx *ctx, size_t key_len, const void *key)
{
    return mode->init(mode, ctx, key_len, key);
}

CC_INLINE int ccecb_update(const struct ccmode_ecb *mode, const ccecb_ctx *ctx, size_t nblocks, const void *in, void *out)
{
    return mode->ecb(ctx, nblocks, in, out);
}

CC_INLINE int
ccecb_one_shot(const struct ccmode_ecb *mode, size_t key_len, const void *key, size_t nblocks, const void *in, void *out)
{
    int rc;
    ccecb_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, key_len, key);
    if (rc == 0) {
        rc = mode->ecb(ctx, nblocks, in, out);
    }
    ccecb_ctx_clear(mode->size, ctx);
    return rc;
}

               

#define __CC_HAS_FIX_FOR_11468135__ 1

                                                                              
                 
#define cccbc_ctx_decl(_size_, _name_) cc_ctx_decl(cccbc_ctx, _size_, _name_)
#define cccbc_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                      
                                   
#define cccbc_iv_decl(_size_, _name_) cc_ctx_decl(cccbc_iv, _size_, _name_)
#define cccbc_iv_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                          

                                                                         
                                                                         
                                                                   
                                                                   
                                                                   

CC_INLINE size_t cccbc_context_size(const struct ccmode_cbc *mode)
{
    return mode->size;
}

CC_INLINE size_t cccbc_block_size(const struct ccmode_cbc *mode)
{
    return mode->block_size;
}

CC_INLINE int cccbc_init(const struct ccmode_cbc *mode, cccbc_ctx *ctx, size_t key_len, const void *key)
{
    return mode->init(mode, ctx, key_len, key);
}

CC_INLINE int cccbc_set_iv(const struct ccmode_cbc *mode, cccbc_iv *iv_ctx, const void *iv)
{
    if (iv) {
        cc_copy(mode->block_size, iv_ctx, iv);
    } else {
        cc_clear(mode->block_size, iv_ctx);
    }
    return 0;
}

CC_INLINE int cccbc_update(const struct ccmode_cbc *mode, cccbc_ctx *ctx, cccbc_iv *iv, size_t nblocks, const void *in, void *out)
{
    return mode->cbc(ctx, iv, nblocks, in, out);
}

int cccbc_one_shot(const struct ccmode_cbc *mode,
                   size_t key_len,
                   const void *key,
                   const void *iv,
                   size_t nblocks,
                   const void *in,
                   void *out);

               

                                                                              
                 
#define cccfb_ctx_decl(_size_, _name_) cc_ctx_decl(cccfb_ctx, _size_, _name_)
#define cccfb_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t cccfb_context_size(const struct ccmode_cfb *mode)
{
    return mode->size;
}

CC_INLINE size_t cccfb_block_size(const struct ccmode_cfb *mode)
{
    return mode->block_size;
}

CC_INLINE int cccfb_init(const struct ccmode_cfb *mode, cccfb_ctx *ctx, size_t key_len, const void *key, const void *iv)
{
    return mode->init(mode, ctx, key_len, key, iv);
}

CC_INLINE int cccfb_update(const struct ccmode_cfb *mode, cccfb_ctx *ctx, size_t nbytes, const void *in, void *out)
{
    return mode->cfb(ctx, nbytes, in, out);
}

CC_INLINE int cccfb_one_shot(const struct ccmode_cfb *mode,
                             size_t key_len,
                             const void *key,
                             const void *iv,
                             size_t nbytes,
                             const void *in,
                             void *out)
{
    int rc;
    cccfb_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, key_len, key, iv);
    if (rc == 0) {
        rc = mode->cfb(ctx, nbytes, in, out);
    }
    cccfb_ctx_clear(mode->size, ctx);
    return rc;
}

                

                                                                                
               
#define cccfb8_ctx_decl(_size_, _name_) cc_ctx_decl(cccfb8_ctx, _size_, _name_)
#define cccfb8_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t cccfb8_context_size(const struct ccmode_cfb8 *mode)
{
    return mode->size;
}

CC_INLINE size_t cccfb8_block_size(const struct ccmode_cfb8 *mode)
{
    return mode->block_size;
}

CC_INLINE int cccfb8_init(const struct ccmode_cfb8 *mode, cccfb8_ctx *ctx, size_t key_len, const void *key, const void *iv)
{
    return mode->init(mode, ctx, key_len, key, iv);
}

CC_INLINE int cccfb8_update(const struct ccmode_cfb8 *mode, cccfb8_ctx *ctx, size_t nbytes, const void *in, void *out)
{
    return mode->cfb8(ctx, nbytes, in, out);
}

CC_INLINE int cccfb8_one_shot(const struct ccmode_cfb8 *mode,
                              size_t key_len,
                              const void *key,
                              const void *iv,
                              size_t nbytes,
                              const void *in,
                              void *out)
{
    int rc;
    cccfb8_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, key_len, key, iv);
    if (rc == 0) {
        rc = mode->cfb8(ctx, nbytes, in, out);
    }
    cccfb8_ctx_clear(mode->size, ctx);
    return rc;
}

               

                                                                              
               
#define ccctr_ctx_decl(_size_, _name_) cc_ctx_decl(ccctr_ctx, _size_, _name_)
#define ccctr_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                           
                                                                          
                                                        

CC_INLINE size_t ccctr_context_size(const struct ccmode_ctr *mode)
{
    return mode->size;
}

CC_INLINE size_t ccctr_block_size(const struct ccmode_ctr *mode)
{
    return mode->block_size;
}

CC_INLINE int ccctr_init(const struct ccmode_ctr *mode, ccctr_ctx *ctx, size_t key_len, const void *key, const void *iv)
{
    return mode->init(mode, ctx, key_len, key, iv);
}

CC_INLINE int ccctr_update(const struct ccmode_ctr *mode, ccctr_ctx *ctx, size_t nbytes, const void *in, void *out)
{
    return mode->ctr(ctx, nbytes, in, out);
}

CC_INLINE int ccctr_one_shot(const struct ccmode_ctr *mode,
                             size_t key_len,
                             const void *key,
                             const void *iv,
                             size_t nbytes,
                             const void *in,
                             void *out)
{
    int rc;
    ccctr_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, key_len, key, iv);
    if (rc == 0) {
        rc = mode->ctr(ctx, nbytes, in, out);
    }
    ccctr_ctx_clear(mode->size, ctx);
    return rc;
}

               

                                                                              
               
#define ccofb_ctx_decl(_size_, _name_) cc_ctx_decl(ccofb_ctx, _size_, _name_)
#define ccofb_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t ccofb_context_size(const struct ccmode_ofb *mode)
{
    return mode->size;
}

CC_INLINE size_t ccofb_block_size(const struct ccmode_ofb *mode)
{
    return mode->block_size;
}

CC_INLINE int ccofb_init(const struct ccmode_ofb *mode, ccofb_ctx *ctx, size_t key_len, const void *key, const void *iv)
{
    return mode->init(mode, ctx, key_len, key, iv);
}

CC_INLINE int ccofb_update(const struct ccmode_ofb *mode, ccofb_ctx *ctx, size_t nbytes, const void *in, void *out)
{
    return mode->ofb(ctx, nbytes, in, out);
}

CC_INLINE int ccofb_one_shot(const struct ccmode_ofb *mode,
                             size_t key_len,
                             const void *key,
                             const void *iv,
                             size_t nbytes,
                             const void *in,
                             void *out)
{
    int rc;
    ccofb_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, key_len, key, iv);
    if (rc == 0) {
        rc = mode->ofb(ctx, nbytes, in, out);
    }
    ccofb_ctx_clear(mode->size, ctx);
    return rc;
}

               

                                                                              
               
#define ccxts_ctx_decl(_size_, _name_) cc_ctx_decl(ccxts_ctx, _size_, _name_)
#define ccxts_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                    
                                   
#define ccxts_tweak_decl(_size_, _name_) cc_ctx_decl(ccxts_tweak, _size_, _name_)
#define ccxts_tweak_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                          

                                                                       
                                                                       
                                                                               
                                                   
                                                                 

                                                                        
                                                                          
                                                                             
             

CC_INLINE size_t ccxts_context_size(const struct ccmode_xts *mode)
{
    return mode->size;
}

CC_INLINE size_t ccxts_block_size(const struct ccmode_xts *mode)
{
    return mode->block_size;
}

   
                       
                                       

                                               
                                                 
                                                            
                                               
                                                

                              

                                                                  
   
CC_INLINE int
ccxts_init(const struct ccmode_xts *mode, ccxts_ctx *ctx, size_t key_nbytes, const void *data_key, const void *tweak_key)
{
    return mode->init(mode, ctx, key_nbytes, data_key, tweak_key);
}

   
                            
                                               

                                               
                                                 
                                                             
                                                       

                                                        
   
CC_INLINE int ccxts_set_tweak(const struct ccmode_xts *mode, ccxts_ctx *ctx, ccxts_tweak *tweak, const void *iv)
{
    return mode->set_tweak(ctx, tweak, iv);
}

   
                         
                                     

                                               
                                               
                                                             
                                                    
                                  
                                     

                                                                              
    
CC_INLINE void *
ccxts_update(const struct ccmode_xts *mode, ccxts_ctx *ctx, ccxts_tweak *tweak, size_t nblocks, const void *in, void *out)
{
    return mode->xts(ctx, tweak, nblocks, in, out);
}

   
                           
                                                 

                                               
                                                            
                                               
                                                
                                                       
                                                    
                                  
                                     

                              

                                                                  
   
int ccxts_one_shot(const struct ccmode_xts *mode,
                   size_t key_nbytes,
                   const void *data_key,
                   const void *tweak_key,
                   const void *iv,
                   size_t nblocks,
                   const void *in,
                   void *out);

                                 

               

                                                                              
               
#define ccgcm_ctx_decl(_size_, _name_) cc_ctx_decl(ccgcm_ctx, _size_, _name_)
#define ccgcm_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

#define CCGCM_IV_NBYTES 12
#define CCGCM_BLOCK_NBYTES 16

                       
                       
                       
                                                                    
#define CCGCM_TEXT_MAX_NBYTES ((1ULL << 36) - 32ULL)

CC_INLINE size_t ccgcm_context_size(const struct ccmode_gcm *mode)
{
    return mode->size;
}

CC_INLINE size_t ccgcm_block_size(const struct ccmode_gcm *mode)
{
    return mode->block_size;
}

   
                       
                                      

                                               
                                                 
                                                  
                                                                

                              

                                              

                      
                  
                                                        
                                                        
                    

                                                                       

                       
                  
                                                        
                                                        
                    

                                                                                          

                                                                                                                               
                                                    

                                                                                 
   
CC_INLINE int ccgcm_init(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t key_nbytes, const void *key)
{
    return mode->init(mode, ctx, key_nbytes, key);
}

   
                               
                                                               

                                               
                                                 
                                                  
                                                                
                                                   

                              

                                              

                              
                                                        
                                                        
                    

                                                                       

                       
                  
                                                        
                                                        
                    

                                           

                                                                                                                              
                                                                                                                                
                          

                                                                                                                            
                                                               

                                                                                                                                  
                                                                                      
   
int ccgcm_init_with_iv(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t key_nbytes, const void *key, const void *iv);

   
                         
                                       

                                               
                                                 
                                                 
                                             

                              

                                                          

                                                                                          

                                                                                                                                 
               

                                                                                                                         
                           

                                                                                                                                 
                              

                                                                                 
   
CC_INLINE int ccgcm_set_iv(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t iv_nbytes, const void *iv)
{
    return mode->set_iv(ctx, iv_nbytes, iv);
}

   
                                
                                       

                                               
                                                 
                                                 
                                             

                              

                                                                                

                                                                     

                                                       
   
int ccgcm_set_iv_legacy(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t iv_nbytes, const void *iv);

   
                         
                                                     

                                               
                                                 
                                                     

                              

                                                              

                                                                                                                              
                                                                                                                     

                                                                                                                               
                   

                                                                                                 
   
int ccgcm_inc_iv(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, void *iv);

   
                      
                                          

                                                       
                                                         
                                                                      
                                                               

                              

                                                                                                        

                                                
   
CC_INLINE int ccgcm_aad(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t nbytes, const void *additional_data)
{
    return mode->gmac(ctx, nbytes, additional_data);
}

   
                       

                                                                                       
   
CC_INLINE int ccgcm_gmac (const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t nbytes, const void *in)
cc_deprecate_with_replacement("ccgcm_aad", 13.0, 10.15, 13.0, 6.0, 4.0)
{
    return mode->gmac(ctx, nbytes, in);
}

   
                         
                                     

                                               
                                                 
                                                   
                                                     
                                                      

                              

                                              

                                                
   
CC_INLINE int ccgcm_update(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t nbytes, const void *in, void *out)
{
    return mode->gcm(ctx, nbytes, in, out);
}

   
                           
                                                

                                               
                                                 
                                                  
                                          

                              

                                                                            

                                                                                             

                                                                                                                           
                                                                                                                               
                                                                                     

                                                                                                                            
                                                                                                         
   
CC_INLINE int ccgcm_finalize(const struct ccmode_gcm *mode, ccgcm_ctx *ctx, size_t tag_nbytes, void *tag)
{
    return mode->finalize(ctx, tag_nbytes, tag);
}

   
                        
                                                      

                                               
                                                 

                              

                                                      
   
CC_INLINE int ccgcm_reset(const struct ccmode_gcm *mode, ccgcm_ctx *ctx)
{
    return mode->reset(ctx);
}

   
                           
                                         

                                                   
                                                      
                                                                    
                                                     
                                                 
                                                                  
                                                           
                                                       
                                                         
                                                          
                                                      
                                              

                              

                                                  

                                                                                          

                                                                                                                                 
               

                                                                                                                         
                           

                                  

                                                                                             

                                                                                                                         
                                                                                                

                                                                                                                                  
                                                                                                                               
              
   
int ccgcm_one_shot(const struct ccmode_gcm *mode,
                   size_t key_nbytes,
                   const void *key,
                   size_t iv_nbytes,
                   const void *iv,
                   size_t adata_nbytes,
                   const void *adata,
                   size_t nbytes,
                   const void *in,
                   void *out,
                   size_t tag_nbytes,
                   void *tag);

   
                                  
                                         

                                                   
                                                      
                                                                    
                                                     
                                                 
                                                                  
                                                           
                                                       
                                                         
                                                          
                                                      
                                              

                              

                                                                                  

                                                                     

                                                       
   
int ccgcm_one_shot_legacy(const struct ccmode_gcm *mode,
                          size_t key_nbytes,
                          const void *key,
                          size_t iv_nbytes,
                          const void *iv,
                          size_t adata_nbytes,
                          const void *adata,
                          size_t nbytes,
                          const void *in,
                          void *out,
                          size_t tag_nbytes,
                          void *tag);

         

#define ccccm_ctx_decl(_size_, _name_) cc_ctx_decl(ccccm_ctx, _size_, _name_)
#define ccccm_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

                                                                                  
#define ccccm_nonce_decl(_size_, _name_) cc_ctx_decl(ccccm_nonce, _size_, _name_)
#define ccccm_nonce_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t ccccm_context_size(const struct ccmode_ccm *mode)
{
    return mode->size;
}

CC_INLINE size_t ccccm_block_size(const struct ccmode_ccm *mode)
{
    return mode->block_size;
}

CC_INLINE int ccccm_init(const struct ccmode_ccm *mode, ccccm_ctx *ctx, size_t key_len, const void *key)
{
    return mode->init(mode, ctx, key_len, key);
}

CC_INLINE int ccccm_set_iv(const struct ccmode_ccm *mode,
                           ccccm_ctx *ctx,
                           ccccm_nonce *nonce_ctx,
                           size_t nonce_len,
                           const void *nonce,
                           size_t mac_size,
                           size_t auth_len,
                           size_t data_len)
{
    return mode->set_iv(ctx, nonce_ctx, nonce_len, nonce, mac_size, auth_len, data_len);
}

CC_INLINE int ccccm_cbcmac(const struct ccmode_ccm *mode, ccccm_ctx *ctx, ccccm_nonce *nonce_ctx, size_t nbytes, const void *in)
{
    return mode->cbcmac(ctx, nonce_ctx, nbytes, in);
}

CC_INLINE int
ccccm_update(const struct ccmode_ccm *mode, ccccm_ctx *ctx, ccccm_nonce *nonce_ctx, size_t nbytes, const void *in, void *out)
{
    return mode->ccm(ctx, nonce_ctx, nbytes, in, out);
}

CC_INLINE int ccccm_finalize(const struct ccmode_ccm *mode, ccccm_ctx *ctx, ccccm_nonce *nonce_ctx, void *mac)
{
    return mode->finalize(ctx, nonce_ctx, mac);
}

CC_INLINE int ccccm_reset(const struct ccmode_ccm *mode, ccccm_ctx *ctx, ccccm_nonce *nonce_ctx)
{
    return mode->reset(ctx, nonce_ctx);
}

CC_INLINE int ccccm_one_shot(const struct ccmode_ccm *mode,
                             size_t key_len,
                             const void *key,
                             size_t nonce_len,
                             const void *nonce,
                             size_t nbytes,
                             const void *in,
                             void *out,
                             size_t adata_len,
                             const void *adata,
                             size_t mac_size,
                             void *mac)
{
    int rc;
    ccccm_ctx_decl(mode->size, ctx);
    ccccm_nonce_decl(mode->nonce_size, nonce_ctx);
    rc = mode->init(mode, ctx, key_len, key);
    if (rc == 0) {
        rc = mode->set_iv(ctx, nonce_ctx, nonce_len, nonce, mac_size, adata_len, nbytes);
    }
    if (rc == 0) {
        rc = mode->cbcmac(ctx, nonce_ctx, adata_len, adata);
    }
    if (rc == 0) {
        rc = mode->ccm(ctx, nonce_ctx, nbytes, in, out);
    }
    if (rc == 0) {
        rc = mode->finalize(ctx, nonce_ctx, mac);
    }
    ccccm_ctx_clear(mode->size, ctx);
    ccccm_nonce_clear(mode->nonce_size, nonce_ctx);

    return rc;
}

                

                                                                                
               
#define ccomac_ctx_decl(_size_, _name_) cc_ctx_decl(ccomac_ctx, _size_, _name_)
#define ccomac_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

CC_INLINE size_t ccomac_context_size(const struct ccmode_omac *mode)
{
    return mode->size;
}

CC_INLINE size_t ccomac_block_size(const struct ccmode_omac *mode)
{
    return mode->block_size;
}

CC_INLINE int ccomac_init(const struct ccmode_omac *mode, ccomac_ctx *ctx, size_t tweak_len, size_t key_len, const void *key)
{
    return mode->init(mode, ctx, tweak_len, key_len, key);
}

CC_INLINE int
ccomac_update(const struct ccmode_omac *mode, ccomac_ctx *ctx, size_t nblocks, const void *tweak, const void *in, void *out)
{
    return mode->omac(ctx, nblocks, tweak, in, out);
}

CC_INLINE int ccomac_one_shot(const struct ccmode_omac *mode,
                              size_t tweak_len,
                              size_t key_len,
                              const void *key,
                              const void *tweak,
                              size_t nblocks,
                              const void *in,
                              void *out)
{
    int rc;
    ccomac_ctx_decl(mode->size, ctx);
    rc = mode->init(mode, ctx, tweak_len, key_len, key);
    if (rc == 0) {
        rc = mode->omac(ctx, nblocks, tweak, in, out);
    }
    ccomac_ctx_clear(mode->size, ctx);
    return rc;
}

#endif                            
