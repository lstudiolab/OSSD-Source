   
          
                                                                        
                                                                            
                                                                  
  
                                                     
  
                                                         
  
               
  
                        
                                         
  
                                                                              
                                                 
                                   
                                                                            
  
                                                                               
                                                                            
                                            
  
                                                                               
                    
                                                                             
                                                                          
                                                               
                                                                              
                                                           
                                                                              
                                                                    
                                           
                                                                            
                                                             
                                                                               
                                                
  
                                                                            
                                                         
   


#ifndef __IMG4_NONCE_H
#define __IMG4_NONCE_H

#ifndef __IMG4_INDIRECT
#error "Please #include <img4/firmware.h> instead of this file directly"
#endif                   

#if IMG4_TAPI
#include "tapi.h"
#endif

   
                               
                                            
   
IMG4_API_AVAILABLE_20181106
typedef struct _img4_nonce_domain img4_nonce_domain_t;

   
                                   
                                                                     
                  
   
#define IMG4_NONCE_STRUCT_VERSION ((img4_struct_version_t)0)
#define IMG4_NONCE_VERSION IMG4_NONCE_STRUCT_VERSION

   
                               
                                                                             
        
   
#define IMG4_NONCE_MAX_LENGTH (48u)

   
                        
                                  
  
                     
                                                                        
                                                       
  
                   
                                  
  
                    
                                                                          
   
IMG4_API_AVAILABLE_20181106
typedef struct _img4_nonce {
	img4_struct_version_t i4n_version;
	const uint8_t i4n_nonce[IMG4_NONCE_MAX_LENGTH];
	uint32_t i4n_length;
} img4_nonce_t;

   
                         
                                                                            
                                                     
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IMG4_NONCE_INIT (img4_nonce_t){.i4n_version = IMG4_NONCE_STRUCT_VERSION}
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define IMG4_NONCE_INIT (img4_nonce_t{IMG4_NONCE_STRUCT_VERSION})
#elif defined(__cplusplus)
#define IMG4_NONCE_INIT \
		(img4_nonce_t((img4_nonce_t){IMG4_NONCE_STRUCT_VERSION}))
#else
#define IMG4_NONCE_INIT {IMG4_NONCE_STRUCT_VERSION}
#endif

   
                         
                                                                             
                            
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IMG4_NONCE_ZERO (img4_nonce_t){ \
	.i4n_version = IMG4_NONCE_STRUCT_VERSION, \
	.i4n_nonce = {0}, \
	.i4n_length = IMG4_NONCE_MAX_LENGTH, \
}
#endif

   
                                       
                                                                              
               
  
                                               
  
               
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20181106
OS_EXPORT
const struct _img4_nonce_domain _img4_nonce_domain_trust_cache;
#define IMG4_NONCE_DOMAIN_TRUST_CACHE (&_img4_nonce_domain_trust_cache)
#else
#define IMG4_NONCE_DOMAIN_TRUST_CACHE (img4if->i4if_v1.nonce_domain_trust_cache)
#endif

   
                               
                                                                             
               
  
                                       
  
                                                                         
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20181106
OS_EXPORT
const struct _img4_nonce_domain _img4_nonce_domain_pdi;
#define IMG4_NONCE_DOMAIN_PDI (&_img4_nonce_domain_pdi)
#else
#define IMG4_NONCE_DOMAIN_PDI (img4if->i4if_v3.nonce_domain_pdi)
#endif

   
                                   
                                                                          
               
  
                                           
  
               
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20181106
OS_EXPORT
const struct _img4_nonce_domain _img4_nonce_domain_cryptex;
#define IMG4_NONCE_DOMAIN_CRYPTEX (&_img4_nonce_domain_cryptex)
#else
#define IMG4_NONCE_DOMAIN_CRYPTEX (img4if->i4if_v3.nonce_domain_cryptex)
#endif

   
                                         
                                                             
  
            
                    
  
           
                                                                           
                                                                  
                              
  
          
                                                                                
                                  
  
                                                                                
                                                                   
                                                                           
                               
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20181106
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL2
errno_t
img4_nonce_domain_copy_nonce(const img4_nonce_domain_t *nd, img4_nonce_t *n);
#else
#define img4_nonce_domain_copy_nonce(...) \
		(img4if->i4if_v1.nonce_domain_copy_nonce(__VA_ARGS__))
#endif

   
                                         
                                                                                
                                               
  
            
                    
  
          
                                                                              
                                                                                
                        
  
                                                                           
                               
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20181106
OS_EXPORT OS_NONNULL1
errno_t
img4_nonce_domain_roll_nonce(const img4_nonce_domain_t *nd);
#else
#define img4_nonce_domain_roll_nonce(...) \
		(img4if->i4if_v1.nonce_domain_roll_nonce(__VA_ARGS__))
#endif

#endif                  
