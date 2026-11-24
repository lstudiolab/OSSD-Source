/* Copyright (c) (2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */
                                 
  

#ifndef ccmode_siv_hmac_h
#define ccmode_siv_hmac_h

#include <corecrypto/cc.h>
#include <corecrypto/ccmode.h>
#include <corecrypto/ccmode_impl.h>
#include <corecrypto/ccdigest.h>
#include <corecrypto/cchmac.h>
#include <corecrypto/ccsha2.h>

                                                                                  
                                                                                              
                                                          
                                                                                       
                                        
   

cc_aligned_struct(16) ccsiv_hmac_ctx;

struct ccmode_siv_hmac {
    size_t size;                                               
    size_t block_size;
    
    int (*CC_SPTR(ccmode_siv_hmac, init))(const struct ccmode_siv_hmac *sivhmac,
                ccsiv_hmac_ctx *ctx,
                size_t key_len,
                const uint8_t *key,
                const size_t tag_size);
    int (*CC_SPTR(ccmode_siv_hmac, set_nonce))(ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in);
    int (*CC_SPTR(ccmode_siv_hmac, auth))(ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in);
    int (*CC_SPTR(ccmode_siv_hmac, crypt))(ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in, uint8_t *out);
    int (*CC_SPTR(ccmode_siv_hmac, reset))(ccsiv_hmac_ctx *ctx);
    const struct ccdigest_info *hmac_digest;                              
    const struct ccmode_ctr *ctr;
};

#define ccsiv_hmac_ctx_decl(_size_, _name_) cc_ctx_decl(ccsiv_hmac_ctx, _size_, _name_)
#define ccsiv_hmac_ctx_clear(_size_, _name_) cc_clear(_size_, _name_)

   
                                    
                                   
 
                                               
   
CC_INLINE size_t ccsiv_hmac_context_size(const struct ccmode_siv_hmac *mode)
{
    return mode->size;
}

   
                                  
                                   
 
                                               
   
CC_INLINE size_t ccsiv_hmac_block_size(const struct ccmode_siv_hmac *mode)
{
    return mode->block_size;
}

   
                                       
                                                                                                                          
 
                                                                                            
                                                    
 
                                                                                                                                 
         
   
size_t ccsiv_hmac_ciphertext_size(ccsiv_hmac_ctx *ctx, size_t plaintext_size);

   
                                      
                                                                         

                                                                                            
                                                                               

                                                                                                                                                                     
   
size_t ccsiv_hmac_plaintext_size(ccsiv_hmac_ctx *ctx, size_t ciphertext_size);

   
                            
                                                                                                                 
 
                                                       
                                                                 
                                                                                            
                                                
                                                                                                                              
                           
 
                                                                                                                                  
                                                                                                                                
                                                                                                                                  
                                                                                                                               
                 
   
int ccsiv_hmac_init(const struct ccmode_siv_hmac *mode, ccsiv_hmac_ctx *ctx, size_t key_byte_len, const uint8_t *key, size_t tag_size);

   
                           
                                                                                                                              
                                                                                                                   
                                                                                                                   
                                                                                                                     
                
 
                                                       
                                              
                                                                                 
                                                                    
 
                                                                                                       
   
int ccsiv_hmac_aad(const struct ccmode_siv_hmac *mode, ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in);

   
                             
                                                                                                                  
                                                                
                                                       
                                              
                                                                            
                                                               
 
                                                                                                                         
                                                                                                                              
                                                                                                                             
   
int ccsiv_hmac_set_nonce(const struct ccmode_siv_hmac *mode, ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in);

   
                             
                                                                                          
                                                                                                              
                                                                                                                               
                                                                                                        
                                                       
                                              
                                                                       
                                                                         
                                                 
                   
                                                                                                                             
                                                                                                                          
          
                                                                                                                           
                                                                                                               

                                                                                                                                                                       
                                                                                     
                                                                                                   
   
int ccsiv_hmac_crypt(const struct ccmode_siv_hmac *mode, ccsiv_hmac_ctx *ctx, size_t nbytes, const uint8_t *in, uint8_t *out);

   
                             
                                                                                        
                                                                      
                                                       
                                              
   
int ccsiv_hmac_reset(const struct ccmode_siv_hmac *mode, ccsiv_hmac_ctx *ctx);

   
                                
                                                                                                                               
                                              
                                                       
                                                                                            
                                                
                                                                                                        
                                                         
                                                                            
                                                               
                                                              
                                                                    
                                                                                                              
                                                                                                                                      
                                                                                                                                                                                   
   

                                                            
int ccsiv_hmac_one_shot(const struct ccmode_siv_hmac *mode,
                                  size_t key_len,
                                  const uint8_t *key,
                                  size_t tag_length,
                                  unsigned nonce_nbytes,
                                  const uint8_t *nonce,
                                  unsigned adata_nbytes,
                                  const uint8_t *adata,
                                  size_t in_nbytes,
                                  const uint8_t *in,
                        uint8_t *out);

#endif                        
