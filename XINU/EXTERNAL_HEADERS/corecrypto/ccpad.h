/* Copyright (c) (2010,2011,2012,2014,2015,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCPAD_H_
#define _CORECRYPTO_CCPAD_H_

#include <corecrypto/ccmode.h>

                                               
                                                                                  
                                         

                                                                                                  
size_t ccpad_cts1_decrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);

                                                                                                  
size_t ccpad_cts1_encrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);
                                                                                                  
size_t ccpad_cts2_decrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);

                                                                                                  
size_t ccpad_cts2_encrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);
                                                                                                  
size_t ccpad_cts3_decrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);

                                                                                                  
size_t ccpad_cts3_encrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                       size_t nbytes, const void *in, void *out);

                                                                                                                                                                                                                                                                                          

                                                                                                                                                                                                                                                                                                                            
   
size_t ccpad_pkcs7_decrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                           size_t nbytes, const void *in, void *out);

                                                                                                                                                                                                             
size_t ccpad_pkcs7_encrypt(const struct ccmode_cbc *cbc, cccbc_ctx *ctx, cccbc_iv *iv,
                         size_t nbytes, const void *in, void *out);

                                                                                                                                                                           
                                                

size_t ccpad_pkcs7_ecb_decrypt(const struct ccmode_ecb *ecb, ccecb_ctx *ecb_key,
                               size_t nbytes, const void *in, void *out);

size_t ccpad_pkcs7_ecb_encrypt(const struct ccmode_ecb *ecb, ccecb_ctx *ctx,
                             size_t nbytes, const void *in, void *out);

                                                                        
size_t ccpad_pkcs7_decode(const size_t block_size, const uint8_t* last_block);

                                                                                                  
size_t ccpad_xts_decrypt(const struct ccmode_xts *xts, ccxts_ctx *ctx, ccxts_tweak *tweak,
                       size_t nbytes, const void *in, void *out);

                                                                                                  
void ccpad_xts_encrypt(const struct ccmode_xts *xts, ccxts_ctx *ctx, ccxts_tweak *tweak,
                       size_t nbytes, const void *in, void *out);

#endif                           
