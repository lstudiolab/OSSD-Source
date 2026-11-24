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

#ifndef _CORECRYPTO_CCRNG_H_
#define _CORECRYPTO_CCRNG_H_

#include <corecrypto/cc.h>

#define CCRNG_STATE_COMMON \
    int (*CC_SPTR(ccrng_state, generate))(struct ccrng_state *rng, size_t outlen, void *out);

   
                              
                                                                                                      
   
struct ccrng_state {
    CCRNG_STATE_COMMON
};

   
                  
                                                                                                                               
                                                                 
                                                                                                                    
                                                                                                      

                                                                             

            
                                                               
                                              
                                  
                                                                                  
                       
                         
                                                       
   

struct ccrng_state *ccrng(int *error);

   
                           
                                                                                         

                                                                    
                                                   
                                                                                        

                                             
   
#define ccrng_generate(rng, outlen, out) \
    ((rng)->generate((struct ccrng_state *)(rng), (outlen), (out)))

   
                         
                                                     

                                    
                                                       
                                                                     

                                                       
   
int ccrng_uniform(struct ccrng_state *rng, uint64_t bound, uint64_t *rand);

#endif                           
