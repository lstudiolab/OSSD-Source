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

   
                            
                                                                            
                                            

   

#ifndef _CORECRYPTO_CCDRBG_H_
#define _CORECRYPTO_CCDRBG_H_

#include <corecrypto/cc.h>
#include <corecrypto/ccdrbg_impl.h>

  
                                                                                                                    
                                                                                                       
                                                                                                          
                                                     
   

#define CCDRBG_MAX_ENTROPY_SIZE         ((uint32_t)1<<16)
#define CCDRBG_MAX_ADDITIONALINPUT_SIZE ((uint32_t)1<<16)
#define CCDRBG_MAX_PSINPUT_SIZE         ((uint32_t)1<<16)
#define CCDRBG_MAX_REQUEST_SIZE         ((uint32_t)1<<16)                                               
#define CCDRBG_RESEED_INTERVAL          ((uint64_t)1<<48)                                                


  
                                                                                 
                                                                                                          
                                                       
                                                                     
   

CC_INLINE int ccdrbg_init(const struct ccdrbg_info *info,
			struct ccdrbg_state *drbg,
            size_t entropyLength, const void* entropy,
            size_t nonceLength, const void* nonce,
            size_t psLength, const void* ps)
{
	return info->init(info, drbg, entropyLength, entropy, nonceLength, nonce, psLength, ps);
}

  
                                                                                  
   
CC_INLINE int ccdrbg_reseed(const struct ccdrbg_info *info,
       struct ccdrbg_state *drbg,
       size_t entropyLength, const void *entropy,
       size_t additionalLength, const void *additional)
{
    return info->reseed(drbg, entropyLength, entropy, additionalLength, additional);
}


CC_INLINE int ccdrbg_generate(const struct ccdrbg_info *info,
         struct ccdrbg_state *drbg,
         size_t dataOutLength, void *dataOut,
         size_t additionalLength, const void *additional)
{
    return info->generate(drbg, dataOutLength, dataOut, additionalLength, additional);
}

CC_INLINE void ccdrbg_done(const struct ccdrbg_info *info,
		struct ccdrbg_state *drbg)
{
	info->done(drbg);
}

CC_INLINE size_t ccdrbg_context_size(const struct ccdrbg_info *info)
{
    return info->size;
}


  
                          
                                                                                          
   
struct ccdrbg_nistctr_custom {
    const struct ccmode_ctr *ctr_info;
    size_t keylen;
    int strictFIPS;
    int use_df;
};

void ccdrbg_factory_nistctr(struct ccdrbg_info *info, const struct ccdrbg_nistctr_custom *custom);

  
                           
                                                                                                                                  
   
struct ccdrbg_nisthmac_custom {
    const struct ccdigest_info *di;
    int strictFIPS;
};

void ccdrbg_factory_nisthmac(struct ccdrbg_info *info, const struct ccdrbg_nisthmac_custom *custom);

#endif                            
