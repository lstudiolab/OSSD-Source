/* Copyright (c) (2012,2015,2016,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCDRBG_IMPL_H_
#define _CORECRYPTO_CCDRBG_IMPL_H_

                           
struct ccdrbg_state;

struct ccdrbg_info {
                                           
    size_t size;

                            
                                     
                                        
                                    
                                                 
                                             
                            
       
    int (*CC_SPTR(ccdrbg_info, init))(const struct ccdrbg_info *info, struct ccdrbg_state *drbg,
                size_t entropyLength, const void* entropy,
                size_t nonceLength, const void* nonce,
                size_t psLength, const void* ps);

                               
                                     
                                        
                                    
                                                 
                                             
                            
       
    int (*CC_SPTR(ccdrbg_info, reseed))(struct ccdrbg_state *prng,
                  size_t entropylen, const void *entropy,
                  size_t inlen, const void *in);

                                                            
                                               
                                                 
                                                   
                                              
                                          
                             
       
    int (*CC_SPTR(ccdrbg_info, generate))(struct ccdrbg_state *prng,
                    size_t outlen, void *out,
                    size_t inlen, const void *in);

                              
                                              
       
    void (*CC_SPTR(ccdrbg_info, done))(struct ccdrbg_state *prng);

                             
    const void *custom;
};



#endif                              
