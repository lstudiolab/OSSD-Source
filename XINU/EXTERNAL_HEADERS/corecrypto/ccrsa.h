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

#ifndef _CORECRYPTO_CCRSA_H_
#define _CORECRYPTO_CCRSA_H_

#include <corecrypto/cc.h>
#include <corecrypto/ccdigest.h>
#include <corecrypto/ccrng.h>
#include <corecrypto/cczp.h>
#include <corecrypto/cc_fault_canary.h>
#include <stdbool.h>

                                                         
                                                                       
#define CCRSA_KEYGEN_MAX_NBITS 8192

struct ccrsa_full_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pb_)
} CC_ALIGNED(CCN_UNIT_SIZE);

struct ccrsa_pub_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pb_)
} CC_ALIGNED(CCN_UNIT_SIZE);

struct ccrsa_priv_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pv_)
} CC_ALIGNED(CCN_UNIT_SIZE);

typedef struct ccrsa_full_ctx* ccrsa_full_ctx_t;
typedef struct ccrsa_pub_ctx* ccrsa_pub_ctx_t;
typedef struct ccrsa_priv_ctx* ccrsa_priv_ctx_t;

  
                                                                                                        
                                                                                             
                                                                                             
                                                                                                                       
                                                                                                                       
                                                                                                                       
   

                                                                                                               

                                                                         

#define ccrsa_pub_ctx_size(_size_)   (sizeof(struct cczp) + CCN_UNIT_SIZE + 3 * (_size_))
#define ccrsa_priv_ctx_size(_size_)  ((sizeof(struct cczp) + CCN_UNIT_SIZE) * 2 + 7 * ccn_sizeof(ccn_bitsof_size(_size_)/2 + 1))
#define ccrsa_full_ctx_size(_size_)  (ccrsa_pub_ctx_size(_size_) + _size_ + ccrsa_priv_ctx_size(_size_))

                                                                            
                                                                            
                                                                           
   
#define ccrsa_full_ctx_decl(_size_, _name_)   cc_ctx_decl(struct ccrsa_full_ctx, ccrsa_full_ctx_size(_size_), _name_)
#define ccrsa_full_ctx_clear(_size_, _name_)  cc_clear(ccrsa_full_ctx_size(_size_), _name_)
#define ccrsa_pub_ctx_decl(_size_, _name_)    cc_ctx_decl(struct ccrsa_pub_ctx, ccrsa_pub_ctx_size(_size_), _name_)
#define ccrsa_pub_ctx_clear(_size_, _name_)   cc_clear(ccrsa_pub_ctx_size(_size_), _name_)

                                                    
                                                                                   
                                                                           

#define ccrsa_ctx_zm(_ctx_)        ((cczp_t)(_ctx_))
#define ccrsa_ctx_n(_ctx_)         (ccrsa_ctx_zm(_ctx_)->n)
#define ccrsa_ctx_m(_ctx_)         ((_ctx_)->pb_ccn)

#define ccrsa_ctx_e(_ctx_)         (ccrsa_ctx_m(_ctx_) + 2 * ccrsa_ctx_n(_ctx_) + 1)
#define ccrsa_ctx_d(_ctx_)         (ccrsa_ctx_m(_ctx_) + 3 * ccrsa_ctx_n(_ctx_) + 1)

                                        
                                                                                   
                                                        
#define ccrsa_ctx_private_zp(FK)   ((cczp_t)ccrsa_get_private_ctx_ptr(FK))
#define ccrsa_ctx_private_zq(FK)   ((cczp_t)((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 2 * ccrsa_ctx_private_zp(FK)->n + 1))
#define ccrsa_ctx_private_dp(FK)   ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 4 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_dq(FK)   ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 5 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_qinv(FK) ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 6 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))

                                          
CC_INLINE
ccrsa_priv_ctx_t ccrsa_get_private_ctx_ptr(ccrsa_full_ctx_t fk) {
    ccrsa_priv_ctx_t priv = (ccrsa_priv_ctx_t)(ccrsa_ctx_d(fk)+ccrsa_ctx_n(fk));
    return priv;
}

   
                             
                                              
                                 
                                   
   
CC_INLINE
ccrsa_pub_ctx_t ccrsa_ctx_public(ccrsa_full_ctx_t fk) {
    return (ccrsa_pub_ctx_t) fk;
}

   
                              
                                                                                    
                                               
                                     
  
CC_NONNULL_ALL
size_t ccrsa_pubkeylength(ccrsa_pub_ctx_t pubk);

                       
#define CCRSA_PKCS1_PAD_SIGN     1
#define CCRSA_PKCS1_PAD_ENCRYPT  2

   
                          
                                                                                                                       
                                                                          
                                                 
                                                   
                                
 
                                                                                                                             
                                                                                                                 
  
CC_NONNULL_ALL
int ccrsa_init_pub(ccrsa_pub_ctx_t pubk, const cc_unit *modulus,
                    const cc_unit *exponent);

                             
                                                                        
                                                   

                                                                                                       
                                                              
                                                         
                                                           
                                                      
                                                           
                                                      

                                        

                                                                                                                                  
                                                      
                                                                                                                                                 
                       
  
int ccrsa_make_priv(ccrsa_full_ctx_t full_ctx,
                    size_t e_nbytes, const uint8_t *e_bytes,
                    size_t p_nbytes, const uint8_t *p_bytes,
                    size_t q_nbytes, const uint8_t *q_bytes);

                                
                                                                            
                                                               

                                                
                                                                    

                                                                                                     
                                                      
                                                 
                                                              
                                                         
                                                               
                                                          
                                      

                                        
  
int ccrsa_recover_priv(ccrsa_full_ctx_t full_ctx,
                       size_t m_nbytes, const uint8_t *m_bytes,
                       size_t e_nbytes, const uint8_t *e_bytes,
                       size_t d_nbytes, const uint8_t *d_bytes,
                       struct ccrng_state *rng);

   
                          
                                                                                                  

                                                                          
                                                              
                                                                            
                                                              
                                                            
                                

                                                                                                                             
                                                                                                                    
  

CC_NONNULL((1, 3, 5))
int ccrsa_make_pub(ccrsa_pub_ctx_t pubk,
                   size_t exp_nbytes, const uint8_t *exp,
                   size_t mod_nbytes, const uint8_t *mod);

   
                           
                                                             
                                                         
                                                                                                      
                                                                       
                                
 
                                                                                                    
                                                                                         
  
CC_NONNULL((1, 2, 3))
int ccrsa_pub_crypt(ccrsa_pub_ctx_t key, cc_unit *out, const cc_unit *in);

   
                              
                                         

                                                     
                                                                               
                                                             
                                                                                                 
                                                    
                                

           
                                                                            
                                                                                                                                                                  
  
CC_NONNULL_ALL
int ccrsa_generate_key(size_t nbits, ccrsa_full_ctx_t fk,
                       size_t e_nbytes, const void *e_bytes, struct ccrng_state *rng) CC_WARN_RESULT;

   
                                      
                                                                                

                                                     
                                                                               
                                                             
                                                                                                 
                                                               
                                                                                
                                

           
                                                                           
                                                                                              
  
CC_NONNULL_ALL int
ccrsa_generate_fips186_key(size_t nbits, ccrsa_full_ctx_t fk,
                           size_t e_nbytes, const void *e_bytes,
                           struct ccrng_state *rng, struct ccrng_state *rng_mr) CC_WARN_RESULT;



                                                                             

   
                                  
                                                                                                             

                                                      
                                                      
                                                           
                                                                                 
                                                    
                                                                                  
                                                     
                                                                                
                                                   
                                                                                 
                                                    
                                                                                  
                                                     
                                                                                
                                                   
                                                                               
                                                                                                                        
                                                   
                                                                                                                        
                                                   
                                                                                                                       
                                                  
                                                                                                                            
                                                       
                                      
 
            
                                                                            
  

CC_NONNULL((3, 5, 7, 9, 11, 13, 15, 16))
int
ccrsa_make_fips186_key(size_t nbits,
                       const cc_size e_n, const cc_unit *e,
                       const cc_size xp1_nbytes, const cc_unit *xp1, const cc_size xp2_nbytes, const cc_unit *xp2,
                       const cc_size xp_nbytes, const cc_unit *xp,
                       const cc_size xq1_nbytes, const cc_unit *xq1, const cc_size xq2_nbytes, const cc_unit *xq2,
                       const cc_size xq_nbytes, const cc_unit *xq,
                       ccrsa_full_ctx_t fk,
                       cc_size *np, cc_unit *r_p,
                       cc_size *nq, cc_unit *r_q,
                       cc_size *nm, cc_unit *r_m,
                       cc_size *nd, cc_unit *r_d);

  
                                    
  

   
                        

                                                                                                

                                     
                                                                                                                                               
                                                                            
                                                                                 
                                                   
                                                                                            
                                                                                                             
                                                                                                              
                                              
                            
 
           
                                                                                    
                                  
                                                         
                                                             
                                                       
                                                                  
 
                                                                                                                                                                                                                          
                                                                                                                                                             
   
CC_NONNULL((1, 2, 3, 5, 7, 8, 9))
int ccrsa_sign_pss(ccrsa_full_ctx_t key,
                   const struct ccdigest_info* hashAlgorithm, const struct ccdigest_info* MgfHashAlgorithm,
                   size_t salt_nbytes, struct ccrng_state *rng,
                   size_t digest_nbytes, const uint8_t *digest,
                   size_t *sig_nbytes, uint8_t *sig);

   
                            

                                                                                                       

                                     
                                                                                                                                             
                                                                             
                                                                                  
                                                     
                                            
                                                                                                      
                                                                                                               
                                              
                            
 
           
                                                                                    
                                  
                                                         
                                                             
                                                       
                                                                  
 
                                                                                                                                                                                                                          
                                                                                                                                                             
   
CC_NONNULL((1, 2, 3, 5, 7, 8, 9))
int ccrsa_sign_pss_msg(ccrsa_full_ctx_t key,
                   const struct ccdigest_info* hashAlgorithm, const struct ccdigest_info* MgfHashAlgorithm,
                   size_t salt_nbytes, struct ccrng_state *rng,
                   size_t msg_nbytes, const uint8_t *msg,
                   size_t *sig_nbytes, uint8_t *sig);

   
                            
 
                                                                          

                                              
                                                                                                                                                  
                                                                               
                                                                                                
                                                   
                                                                                                                 
                                                   
                                                                                                                                                   
                                                                             

                                                                                                                    
  

CC_NONNULL((2, 3, 5, 7, 9))
int ccrsa_verify_pss(ccrsa_pub_ctx_t key,
                     const struct ccdigest_info* hashAlgorithm,
                     const struct ccdigest_info* MgfHashAlgorithm,
                     size_t digest_nbytes, const uint8_t *digest,
                     size_t sig_nbytes, const uint8_t *sig,
                     size_t salt_nbytes, bool *valid)
cc_deprecate_with_replacement("ccrsa_verify_pss_digest", 13.0, 10.15, 13.0, 6.0, 4.0);

   
                                   
 
                                                                                                   

                                             
                                                                                       
                                                                              
                                                                             
                                                  
                                                                                                                
                                                  
                                                                                  
                                                                      

                                                     
                                                      
                                                          
 
                                                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
  

CC_NONNULL((1, 2, 3, 5, 7))
int ccrsa_verify_pss_digest(ccrsa_pub_ctx_t key,
                            const struct ccdigest_info* di,
                            const struct ccdigest_info* mgfdi,
                            size_t digest_nbytes, const uint8_t *digest,
                            size_t sig_nbytes, const uint8_t *sig,
                            size_t salt_nbytes, cc_fault_canary_t fault_canary_out);

   
                                
 
                                                                                                 

                                             
                                                                                       
                                                                              
                                            
                                             
                                                                                                                
                                                  
                                                                                  
                                                                      

                                                    
                                                     
                                                         
 
                                                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
  

CC_NONNULL((1, 2, 3, 5, 7))
int ccrsa_verify_pss_msg(ccrsa_pub_ctx_t key,
                         const struct ccdigest_info* di,
                         const struct ccdigest_info* mgfdi,
                         size_t msg_nbytes, const uint8_t *msg,
                         size_t sig_nbytes, const uint8_t *sig,
                         size_t salt_nbytes, cc_fault_canary_t fault_canary_out);


   
                                
                                                                  

                                
                                                                   
                                                 
                                                                            
                                                                         
                                                               
                                                                        
                                                

                                     
 
                                                                                        
                                                                          
                                                   

   
CC_NONNULL((1, 4, 5, 6))
int ccrsa_sign_pkcs1v15(ccrsa_full_ctx_t key, const uint8_t *oid,
                        size_t digest_len, const uint8_t *digest,
                        size_t *sig_len, uint8_t *sig);

   
                                    
                                                                  

                               
                                     
                                                         
                                                                                                  
                                                                        
                                                              
                                                                       
                                               

                                     
 
                                                   

   
CC_NONNULL((1, 2, 4, 5, 6))
int ccrsa_sign_pkcs1v15_msg(ccrsa_full_ctx_t key, const struct ccdigest_info* di,
                            size_t msg_len, const uint8_t *msg,
                            size_t *sig_len, uint8_t *sig);


   
                                   
                                                                   

                                   
                                                                    
                                                  
                                                                             
                                                              
                                                                   
                                                                        

                                                                               
                                                                                      

                                                                      
                                                              
                                                                  
                   
  
CC_NONNULL((1, 4, 6, 7))
int ccrsa_verify_pkcs1v15(ccrsa_pub_ctx_t key, const uint8_t *oid,
                          size_t digest_len, const uint8_t *digest,
                          size_t sig_len, const uint8_t *sig,
                          bool *valid);

   
                                          
                                                                                   

                                            
                                                                             
                                                           
                                                                                      
                                                                       
                                                                            
                                                            

                                                          
                                                              
                                                         
 
                                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
  
CC_NONNULL((1, 4, 6))
int ccrsa_verify_pkcs1v15_digest(ccrsa_pub_ctx_t key, const uint8_t *oid,
                          size_t digest_len, const uint8_t *digest,
                          size_t sig_len, const uint8_t *sig,
                          cc_fault_canary_t fault_canary_out);

   
                                       
                                                                   

                                           
                                              
                                                          
                                                                                     
                                                                      
                                                                           
                                                           

                                                         
                                                             
                                                        

                                                    
                                                                                                    
                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                           
  
CC_NONNULL((1, 2, 4, 6))
int ccrsa_verify_pkcs1v15_msg(ccrsa_pub_ctx_t key, const struct ccdigest_info* di,
                          size_t msg_len, const uint8_t *msg,
                          size_t sig_len, const uint8_t *sig,
                          cc_fault_canary_t fault_canary_out);

   
                                      
                                                                     
 
                                  
 
                                                
   

CC_NONNULL((1))
size_t ccder_encode_rsa_pub_size(const ccrsa_pub_ctx_t key);

   
                                    
                                 
 
                                  
                                                      
                                                
   

CC_NONNULL((1, 2, 3))
uint8_t *ccder_encode_rsa_pub(const ccrsa_pub_ctx_t key, uint8_t *der, uint8_t *der_end);


   
                                       
                                                                  
 
                                
 
                                                
   

CC_NONNULL((1))
size_t ccder_encode_rsa_priv_size(const ccrsa_full_ctx_t key);

   
                                  
                                                
 
                                
                                                      
                                                
   

CC_NONNULL((1, 2, 3))
uint8_t *ccder_encode_rsa_priv(const ccrsa_full_ctx_t key, const uint8_t *der, uint8_t *der_end);

   
                                   
                                                                         
                      
 
                                                     
                                               
 
                                                                                    
                           
   

CC_NONNULL((1, 2))
cc_size ccder_decode_rsa_pub_n(const uint8_t *der, const uint8_t *der_end);

   
                                 
                                                                         
                      
 
                                                    
                                                     
                                               
 
                                                                         
   

CC_NONNULL((1, 2, 3))
const uint8_t *ccder_decode_rsa_pub(const ccrsa_pub_ctx_t key, const uint8_t *der, const uint8_t *der_end);

   
                                        
                                                                                       

                                                     
                                               

                                                                                    
                           
   

CC_NONNULL((1, 2))
cc_size ccder_decode_rsa_pub_x509_n(const uint8_t *der, const uint8_t *der_end);

   
                                      
                                                                   

                                                    
                                                     
                                               

                                                                         
   

CC_NONNULL((1, 2, 3))
const uint8_t *ccder_decode_rsa_pub_x509(const ccrsa_pub_ctx_t key, const uint8_t *der, const uint8_t *der_end);


   
                                    
                                                                          
 
                                                     
                                               
 
                                                                                    
                           
   

CC_NONNULL((1, 2))
cc_size ccder_decode_rsa_priv_n(const uint8_t *der, const uint8_t *der_end);

   
                                  
                                                                      
 
                                                  
                                                     
                                               
 
                                                                         
   

CC_NONNULL((1, 2, 3))
const uint8_t *ccder_decode_rsa_priv(const ccrsa_full_ctx_t key, const uint8_t *der, const uint8_t *der_end);

   
                                  
                                                                
 
                                  
 
                                                
   

CC_INLINE CC_NONNULL((1))
size_t ccrsa_export_pub_size(const ccrsa_pub_ctx_t key) {
    return ccder_encode_rsa_pub_size(key);
}

   
                             
                                                      
 
                                  
                                      
                                     
   

CC_NONNULL((1, 3))
int ccrsa_export_pub(const ccrsa_pub_ctx_t key, size_t out_len, uint8_t *out);
   
                               
                                                                         
 
                                                           
                                                            
 
                                                                                    
                           
   

CC_INLINE CC_NONNULL((2))
cc_size ccrsa_import_pub_n(size_t inlen, const uint8_t *der) {
    cc_size size = ccder_decode_rsa_pub_x509_n(der, der + inlen);
    if(size == 0) {
        size = ccder_decode_rsa_pub_n(der, der + inlen);
    }
    return size;
}

   
                             
                                                                         
 
                                                    
                                                           
                                                             
 
                                                                         
   

CC_NONNULL((1, 3))
int ccrsa_import_pub(ccrsa_pub_ctx_t key, size_t inlen, const uint8_t *der);

   
                                   
                                                                  
 
                                
 
                                                
   

CC_INLINE CC_NONNULL((1))
size_t ccrsa_export_priv_size(const ccrsa_full_ctx_t key) {
    return ccder_encode_rsa_priv_size(key);
}

   
                              
                                                
 
                                
                                      
                                     
   

CC_INLINE CC_NONNULL((1, 3))
int ccrsa_export_priv(const ccrsa_full_ctx_t key, size_t out_len, uint8_t *out) {
    return (ccder_encode_rsa_priv(key, out, out+out_len) != out);
}

   
                                
                                                                  
 
                                                       
                                                         
 
                                                                                    
                           
   

CC_INLINE CC_NONNULL((2))
cc_size ccrsa_import_priv_n(size_t inlen, const uint8_t *der) {
    return ccder_decode_rsa_priv_n(der, der + inlen);
}

   
                              
                                                                   
 
                                                  
                                                       
                                                         
 
                                                                     
   

CC_INLINE CC_NONNULL((1, 3))
int ccrsa_import_priv(ccrsa_full_ctx_t key, size_t inlen, const uint8_t *der) {
    return (ccder_decode_rsa_priv(key, der, der+inlen) == NULL);
}

   
                                       
                                                                      

                                                   
                                                                                   
                                                                                                                    
                                                                                  
                                                                                                                   
 
                                                
 
                                                                                            
  
CC_NONNULL((1, 2))
int ccrsa_get_pubkey_components(const ccrsa_pub_ctx_t pubkey, uint8_t *modulus, size_t *modulusLength, uint8_t *exponent, size_t *exponentLength);

   
                                        
                                                                      

                                                     
                                                                                   
                                                                                                                    
                                                                                  
                                                                                                                   
                                                                                                                    
                                                                                                                                
                                                                                                                     
                                                                                                                                

                                                
 
                                                                                            
  
CC_NONNULL((1, 2))
int ccrsa_get_fullkey_components(const ccrsa_full_ctx_t key, uint8_t *modulus, size_t *modulusLength, uint8_t *exponent, size_t *exponentLength,
                                 uint8_t *p, size_t *pLength, uint8_t *q, size_t *qLength);


   
                                  
                                                           

                                    
   
void ccrsa_dump_public_key(ccrsa_pub_ctx_t key);

   
                                
                                                            

                                    
   
void ccrsa_dump_full_key(ccrsa_full_ctx_t key);

#endif                           
