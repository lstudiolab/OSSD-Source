/* Copyright (c) (2016,2017,2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCCHACHA20POLY1305_H_
#define _CORECRYPTO_CCCHACHA20POLY1305_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CCCHACHA20_KEY_NBYTES 32
#define CCCHACHA20_BLOCK_NBYTES 64
#define CCCHACHA20_BLOCK_NBITS (CCCHACHA20_BLOCK_NBYTES * 8)
#define CCCHACHA20_NONCE_NBYTES 12

typedef struct {
	uint32_t state[16];
	uint8_t	buffer[CCCHACHA20_BLOCK_NBYTES];
	size_t leftover;
} ccchacha20_ctx;

#define CCPOLY1305_TAG_NBYTES 16

typedef struct {
	uint32_t r0, r1, r2, r3, r4;
	uint32_t s1, s2, s3, s4;
	uint32_t h0, h1, h2, h3, h4;
	uint8_t	buf[16];
	size_t buf_used;
	uint8_t	key[16];
} ccpoly1305_ctx;


   
                           
                                                                    
                                      

                                                           

                                                                      

                                             

                                   
                                 
                                                                     
                                                                     
                                 

                                                                       

                                    
                                 
                                                                     
                                                                     
                                 

                                       

                                   
                                 
                                                                     
                                                                     
                                                                          

                                                                       

                                    
                                 
                                                                     
                                                                     
                                                                          
  

#define CCCHACHA20POLY1305_KEY_NBYTES (CCCHACHA20_KEY_NBYTES)
#define CCCHACHA20POLY1305_NONCE_NBYTES (CCCHACHA20_NONCE_NBYTES)
#define CCCHACHA20POLY1305_TAG_NBYTES (CCPOLY1305_TAG_NBYTES)

                       
                       
                       
                                                                    
#define CCCHACHA20POLY1305_TEXT_MAX_NBYTES ((1ULL << 38) - 64ULL)

#define CCCHACHA20POLY1305_STATE_SETNONCE 1
#define CCCHACHA20POLY1305_STATE_AAD 2
#define CCCHACHA20POLY1305_STATE_ENCRYPT 3
#define CCCHACHA20POLY1305_STATE_DECRYPT 4
#define CCCHACHA20POLY1305_STATE_FINAL 5

typedef struct {
	ccchacha20_ctx chacha20_ctx;
	ccpoly1305_ctx poly1305_ctx;
	uint64_t aad_nbytes;
	uint64_t text_nbytes;
    uint8_t state;
} ccchacha20poly1305_ctx;

                                 
                                           
struct ccchacha20poly1305_info {

};

const struct ccchacha20poly1305_info *ccchacha20poly1305_info(void);

   
                                    
                                                   

                                                 
                                                 
                                           

                              

                                           

                                                           
   
int	ccchacha20poly1305_init(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, const uint8_t *key);

   
                                     
                                                        

                                                 
                                                 

                              
   
int ccchacha20poly1305_reset(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx);

   
                                        
                                                        

                                                 
                                                 
                                                   

                              

                                             

                                                           
   
int ccchacha20poly1305_setnonce(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, const uint8_t *nonce);
int ccchacha20poly1305_incnonce(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, uint8_t *nonce);

   
                                   
                                          

                                               
                                                 
                                                              
                                                       

                              

                                                                                                        

                                                
   
int	ccchacha20poly1305_aad(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, size_t nbytes, const void *aad);

   
                                       
                          

                                               
                                                 
                                                        
                                       
                                         

                              

                                              

                                                
   
int	ccchacha20poly1305_encrypt(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, size_t nbytes, const void *ptext, void *ctext);

   
                                        
                                 

                                               
                                                 
                                                    

                              

                                                     
   
int	ccchacha20poly1305_finalize(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, uint8_t *tag);

   
                                       
                          

                                               
                                                 
                                                         
                                        
                                        

                              

                                              

                                                
   
int	ccchacha20poly1305_decrypt(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, size_t nbytes, const void *ctext, void *ptext);

   
                                      
                                 

                                               
                                                 
                                                   

                                                      

                                                    
   
int	ccchacha20poly1305_verify(const struct ccchacha20poly1305_info *info, ccchacha20poly1305_ctx *ctx, const uint8_t *tag);

   
                                                  
                                              

                                                   
                                               
                                                       
                                                                  
                                                           
                                                            
                                           
                                             
                                                        

                                      

                               

                                 

                                         

                                  

                                                           

                                                                      
   
int ccchacha20poly1305_encrypt_oneshot(const struct ccchacha20poly1305_info *info, const uint8_t *key, const uint8_t *nonce, size_t aad_nbytes, const void *aad, size_t ptext_nbytes, const void *ptext, void *ctext, uint8_t *tag);

   
                                                  
                                              

                                                   
                                               
                                                       
                                                                  
                                                           
                                                             
                                            
                                            
                                                       

                                      

                               

                                 

                                         

                                  
   
int ccchacha20poly1305_decrypt_oneshot(const struct ccchacha20poly1305_info *info, const uint8_t *key, const uint8_t *nonce, size_t aad_nbytes, const void *aad, size_t ctext_nbytes, const void *ctext, void *ptext, const uint8_t *tag);

#endif
