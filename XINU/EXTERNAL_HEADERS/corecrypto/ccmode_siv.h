/* Copyright (c) (2015,2016,2017,2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCMODE_SIV_H_
#define _CORECRYPTO_CCMODE_SIV_H_

#include <corecrypto/cc.h>
#include <corecrypto/ccmode.h>
#include <corecrypto/ccmode_impl.h>

#include <corecrypto/cccmac.h>

                                        
                                                    
                                                                                       
                                       
   

cc_aligned_struct(16) ccsiv_ctx;

struct ccmode_siv {
    size_t size;                                                 
    size_t block_size;
    int (*CC_SPTR(ccmode_siv, init))(const struct ccmode_siv *siv, ccsiv_ctx *ctx,
                 size_t key_len, const uint8_t *key);
    int (*CC_SPTR(ccmode_siv, set_nonce))(ccsiv_ctx *ctx,  size_t nbytes, const uint8_t *in);                                    
    int (*CC_SPTR(ccmode_siv, auth))(ccsiv_ctx *ctx,  size_t nbytes, const uint8_t *in);                                    
    int (*CC_SPTR(ccmode_siv, crypt))(ccsiv_ctx *ctx, size_t nbytes, const uint8_t *in, uint8_t *out);
    int (*CC_SPTR(ccmode_siv, reset))(ccsiv_ctx *ctx);
    const struct ccmode_cbc *cbc;
    const struct ccmode_ctr *ctr;
};

#define ccsiv_ctx_decl(_size_, _name_)  cc_ctx_decl(ccsiv_ctx, _size_, _name_)
#define ccsiv_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

            

CC_INLINE size_t ccsiv_context_size(const struct ccmode_siv *mode)
{
    return mode->size;
}

   
                          
                                                                             

                                                    

                                                                                                                                                                                                            
  
CC_INLINE size_t ccsiv_block_size(const struct ccmode_siv *mode)
{
    return mode->block_size;
}

   
                                  
                                                                                                                          

                                                     
                                                    

                                                                                                                                          
   
CC_INLINE size_t ccsiv_ciphertext_size(const struct ccmode_siv *mode,
                                       size_t plaintext_size)
{
    return plaintext_size + mode->cbc->block_size;
}

   
                                 
                                                                         

                                                       
                                                       

                                                                                                                                                                     
   

CC_INLINE size_t ccsiv_plaintext_size(const struct ccmode_siv *mode,
                                       size_t ciphertext_size)
{
    if (ciphertext_size<mode->cbc->block_size) {
        return 0;         
    }
    return ciphertext_size - mode->cbc->block_size;
}

   
                       
                                                                                   

                                                       
                                                                 
                                                                                             
                                                                                                    

                                                                                                                                                                                       


      
                
                                                        
                     
                 
         

                                                                       

      
                 
                                                        
                     
                 
         

                                                                                                                                                           
   
CC_INLINE int ccsiv_init(const struct ccmode_siv *mode, ccsiv_ctx *ctx,
                          size_t key_byte_len, const uint8_t *key)
{
    return mode->init(mode, ctx, key_byte_len, key);
}

   
                            
                                                                                                             
                                                                

                                                       
                                              
                                                                            
                                                               

                                                                                                                             
                                                                                                                          
                                                                                                                     
   
CC_INLINE int ccsiv_set_nonce(const struct ccmode_siv *mode, ccsiv_ctx *ctx,
                         size_t nbytes, const uint8_t *in)
{
    return mode->set_nonce(ctx, nbytes, in);
}

   
                      
                                                                                       
                                                       
                                              
                                                                                 
                                                                    

                                                                                                                                                                                                                                                                                                                                                                                            
   
CC_INLINE int ccsiv_aad(const struct ccmode_siv *mode, ccsiv_ctx *ctx,
                            size_t nbytes, const uint8_t *in)
{
    return mode->auth(ctx, nbytes, in);
}

   
                        
                                                                                   

                                                       
                                              
                                                                       
                                                                                                           
                                                 
                                                  
                                                                                                                                        
                                                                                                                                           

                                                                                          
                                                                                                                  
                                                                                                                                                                                                                                            

                                                                                                                 
                                                                                                                     
          

                                                                                                                                                                       
                                                                                     
                                                                                                   

                                                                                                                                                                                                                                                                                                    
   
CC_INLINE int ccsiv_crypt(const struct ccmode_siv *mode, ccsiv_ctx *ctx,
                            size_t nbytes, const uint8_t *in, uint8_t *out)
{
    return mode->crypt(ctx, nbytes, in, out);
}

   
                        
                                                                                        
                                                                      
                                                       
                                              
   
CC_INLINE int ccsiv_reset(const struct ccmode_siv *mode, ccsiv_ctx *ctx)
{
    return mode->reset(ctx);
}

   
                           
                                                                                                                        
                                              

                                                       
                                                                                            
                                           
                                                                            
                                                               
                                                              
                                                                    
                                                                                                                                                                             
                                                                                                                                      
                                                                                                                                                                                                                              
                                                                                                            

                                                                                                                                                                
                                                                                                                                                

                                                                                                                                                                       
                            
                                                                          
                                                                  
                                                                  
                                                                           

                                                                                         


   

CC_INLINE int ccsiv_one_shot(const struct ccmode_siv *mode,
                              size_t key_len, const uint8_t *key,
                              unsigned nonce_nbytes, const uint8_t* nonce,
                              unsigned adata_nbytes, const uint8_t* adata,
                              size_t in_nbytes, const uint8_t *in, uint8_t *out)
{
    int rc;
    ccsiv_ctx_decl(mode->size, ctx);
    rc=mode->init(mode, ctx, key_len, key);
    if (rc) {return rc;}
    rc=mode->set_nonce(ctx, nonce_nbytes, nonce);
    if (rc) {return rc;}
    rc=mode->auth(ctx, adata_nbytes, adata);
    if (rc) {return rc;}
    rc=mode->crypt(ctx, in_nbytes, in, out);
    if (rc) {return rc;}
    ccsiv_ctx_clear(mode->size, ctx);
    return rc;
}

#endif                            
