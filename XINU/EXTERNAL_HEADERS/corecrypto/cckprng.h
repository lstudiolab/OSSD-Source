/* Copyright (c) (2018,2019) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to 
 * people who accept that license. IMPORTANT:  Any license rights granted to you by 
 * Apple Inc. (if any) are limited to internal use within your organization only on 
 * devices and computers you own or control, for the sole purpose of verifying the 
 * security characteristics and correct functioning of the Apple Software.  You may 
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCKPRNG_H_
#define _CORECRYPTO_CCKPRNG_H_

#include <stdbool.h>

#include <corecrypto/cc.h>

                                                                    
                                                                
                                                                      
                                      
  
                                              
  
                                                                      
                                                    
                                                                
                             
  
                                                              
                                                                
                                                          
                                                                     
                                                                  
          
  
                                                                     
                                                                    
                                                                     
                                                              
  
                                                                     
                                                                     
                                                        
                   
  
                                                                      
                                                   
  
                                                               
                                                                    
                                     
  
                                                                   
                                                                  
                                                           
                                                                 
                                                           
  
                                                                      
                                                                  
              
  
                                                                      
                               
  
                                                                
                                                                  
                                              

#define CCKPRNG_NPOOLS 32
#define CCKPRNG_SEED_NBYTES 32
#define CCKPRNG_POOL_NBYTES 32
#define CCKPRNG_KEY_NBYTES 32

struct cckprng_gen_diag {
                                                                               
    uint64_t nrekeys;

                                                          
    uint64_t out_nreqs;

                                                                               
    uint64_t out_nbytes;

                                                                                  
    uint64_t out_nbytes_req_max;

                                                                                  
    uint64_t out_nbytes_key;

                                                                                        
    uint64_t out_nbytes_key_max;
};

struct cckprng_pool_diag {
                                                           
    uint64_t nsamples;

                                                                 
    uint64_t ndrains;

                                                                       
    uint64_t nsamples_max;
};

struct cckprng_diag {
                                                                            
    uint64_t userreseed_nreseeds;

                                              
    uint64_t schedreseed_nreseeds;

                                                                         
    uint64_t schedreseed_nsamples_max;

                                                                      
    uint64_t addentropy_nsamples_max;

                                                                
    unsigned ngens;
    CC_ALIGNED(8) struct cckprng_gen_diag *gens;

                                                          
    struct cckprng_pool_diag pools[CCKPRNG_NPOOLS];
};

#if CC_KERNEL

#include <kern/locks.h>

typedef lck_grp_t *cckprng_lock_group;
typedef lck_mtx_t *cckprng_lock_mutex;

struct cckprng_lock_ctx {
    cckprng_lock_group group;
    cckprng_lock_mutex mutex;
};

#elif CC_ANDROID || CC_LINUX

#include <pthread.h>

typedef pthread_mutex_t cckprng_lock_mutex;

struct cckprng_lock_ctx {
    cckprng_lock_mutex mutex;
};

#else

#include <os/lock.h>

typedef os_unfair_lock cckprng_lock_mutex;

struct cckprng_lock_ctx {
    cckprng_lock_mutex mutex;
};

#endif

struct cckprng_key_ctx {
    uint8_t data[CCKPRNG_KEY_NBYTES];
};

struct cckprng_gen_ctx {
                                                            
                                          
    struct cckprng_key_ctx keys[2];
    _Atomic unsigned swap;
    unsigned key_live_idx;
    unsigned key_idle_idx;

                                 
    uint8_t ctr[16];

                                                 
    bool init;

                                                                      
                                                           
    struct {
        cckprng_lock_mutex mutex;
    } lock;
};

struct cckprng_pool_ctx {
    uint8_t data[CCKPRNG_POOL_NBYTES];
};

                                                                   
                                             
                                                                 
                                                                   
                                                                   
                                                                     
                                                 

struct cckprng_entropybuf {

                                                                                                           
    const void *buf;

                                     
    size_t nbytes;

                                                                 
    const uint32_t *nsamples;

                                                                  
    uint32_t nsamples_last;
};

struct cckprng_sched_ctx {
                                                            
    uint64_t reseed_sched;

                                                                   
    unsigned pool_idx;
};

                                                                      
                                                                     
                                                                   
                                                                     
                                                     
  
                                                                  
                                                                      
                                                                      
                                                          
typedef int32_t (*cckprng_getentropy)(size_t *entropy_nbytes,
                                      void *entropy,
                                      void *arg);

struct cckprng_ctx {
                                    
    struct cckprng_key_ctx key;

                                                          
    uint8_t ctr[16];

                                                             
    struct cckprng_sched_ctx sched;

                                               
    struct cckprng_lock_ctx lock;

                                                             
    unsigned max_ngens;

                                                                 
    unsigned ngens;

                                                                                
    struct cckprng_gen_ctx *gens;

                             
    struct cckprng_pool_ctx pools[CCKPRNG_NPOOLS];

                                                       
    struct cckprng_entropybuf entropybuf;

                               
    struct cckprng_diag diag;

                                        
    cckprng_getentropy getentropy;

                                                                         
    void *getentropy_arg;
};

                                                                 
                                
struct cckprng_funcs {
    void (*CC_SPTR(cckprng_funcs, init))(struct cckprng_ctx *ctx,
										 unsigned max_ngens,
										 size_t entropybuf_nbytes,
										 const void *entropybuf,
										 const uint32_t *entropybuf_nsamples,
										 size_t seed_nbytes,
										 const void *seed,
										 size_t nonce_nbytes,
										 const void *nonce);
    void (*CC_SPTR(cckprng_funcs, initgen))(struct cckprng_ctx *ctx, unsigned gen_idx);
    void (*CC_SPTR(cckprng_funcs, reseed))(struct cckprng_ctx *ctx, size_t nbytes, const void *seed);
    void (*CC_SPTR(cckprng_funcs, refresh))(struct cckprng_ctx *ctx);
    void (*CC_SPTR(cckprng_funcs, generate))(struct cckprng_ctx *ctx, unsigned gen_idx, size_t nbytes, void *out);
	void (*CC_SPTR(cckprng_funcs, init_with_getentropy))(struct cckprng_ctx *ctx,
														 unsigned max_ngens,
														 size_t seed_nbytes,
														 const void *seed,
														 size_t nonce_nbytes,
														 const void *nonce,
														 cckprng_getentropy getentropy,
														 void *getentropy_arg);
};

  
                        
                                             

                                      
                                                                    
                                                                
                                                                    
                                                                                            
                                                
                                            
                                                  

                                                                                                                                                                                                                                                                                                    
  
void cckprng_init(struct cckprng_ctx *ctx,
                  unsigned max_ngens,
                  size_t entropybuf_nbytes,
                  const void *entropybuf,
                  const uint32_t *entropybuf_nsamples,
                  size_t seed_nbytes,
                  const void *seed,
                  size_t nonce_nbytes,
                  const void *nonce);

  
                                        
                                             

                                      
                                                                    
                                                
                                            
                                                  
                                           
                                                                
                                                              

                                                                                                                                                                                
  
void cckprng_init_with_getentropy(struct cckprng_ctx *ctx,
                                  unsigned max_ngens,
                                  size_t seed_nbytes,
                                  const void *seed,
                                  size_t nonce_nbytes,
                                  const void *nonce,
                                  cckprng_getentropy getentropy,
                                  void *getentropy_arg);

  
                           
                                           

                                      
                                       

                                                                                                                                                                                                     
  
void cckprng_initgen(struct cckprng_ctx *ctx, unsigned gen_idx);

  
                          
                                                                   

                                      
                                           
                                            

                                                                                                              
  
void cckprng_reseed(struct cckprng_ctx *ctx, size_t nbytes, const void *seed);

  
                           
                                                                         

                                      

                                                                                                                                                                                                                                                                                                                         
  
void cckprng_refresh(struct cckprng_ctx *ctx);

#define CCKPRNG_GENERATE_MAX_NBYTES 256

  
                            
                                                           

                                      
                                              
                                                     
                                         

                                                                                                                                                                                                                                                                                      
  
void cckprng_generate(struct cckprng_ctx *ctx, unsigned gen_idx, size_t nbytes, void *out);

#endif                             
