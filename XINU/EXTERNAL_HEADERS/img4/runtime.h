   
          
                             
   
#ifndef __IMG4_RUNTIME_H
#define __IMG4_RUNTIME_H

#ifndef __IMG4_INDIRECT
#error "Please #include <img4/firmware.h> instead of this file directly"
#endif                   

#if IMG4_TAPI
#include "tapi.h"
#endif

OS_ASSUME_NONNULL_BEGIN

   
                             
                                                                     
  
                              
                                                                                
                                                                        
  
                           
  
                              
                                                                            
                                                                 
  
                           
  
                              
                                                                           
                                                                
  
                           
  
                              
                                                                           
                                                                     
  
                           
  
                              
                                                                             
                                                                            
  
                           
  
                              
                                                                          
                                                                                
               
  
           
  
                              
                                                                                
                                                                               
  
           
  
                              
                                                                           
                                                                 
                                                                           
                                                                              
                      
  
           
  
                              
                                                                       
                                                                 
                                                                           
                                                                              
                      
  
           
  
                              
                                                                               
                                                                                
                                                                                
                                                                  
  
                                                                           
                                                                               
                                                                                
                                                 
  
           
  
                              
                                                                         
                                                                       
                                                                               
        
  
           
  
                              
                                                                                
                                                   
  
                                                                              
                              
                                                                           
                                     
  
                                                                        
                                                    
  
                                                                           
                                                
  
          
  
                              
                                                                              
                                                                                
                                                                            
                                                                              
           
  
           
  
                              
                                                                               
                                                                               
                                                                
  
           
  
                              
                                                                    
   
IMG4_API_AVAILABLE_20200508
OS_CLOSED_ENUM(img4_identifier, uint64_t,
	IMG4_IDENTIFIER_CEPO,
	IMG4_IDENTIFIER_BORD,
	IMG4_IDENTIFIER_CHIP,
	IMG4_IDENTIFIER_SDOM,
	IMG4_IDENTIFIER_ECID,
	IMG4_IDENTIFIER_CPRO,
	IMG4_IDENTIFIER_CSEC,
	IMG4_IDENTIFIER_EPRO,
	IMG4_IDENTIFIER_ESEC,
	IMG4_IDENTIFIER_IUOU,
	IMG4_IDENTIFIER_RSCH,
	IMG4_IDENTIFIER_CHMH,
	IMG4_IDENTIFIER_AMNM,
	IMG4_IDENTIFIER_EUOU,
	_IMG4_IDENTIFIER_CNT,
);

   
                                  
                                                                    
                  
   
#define IMG4_DGST_STRUCT_VERSION (0u)

   
                           
                                                                          
   
#define IMG4_DGST_MAX_LEN (48u)

   
                       
                                             
  
                 
                                                                                
  
                 
                            
  
                   
                    
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_dgst {
	img4_struct_version_t i4d_version;
	size_t i4d_len;
	uint8_t i4d_bytes[IMG4_DGST_MAX_LEN];
} img4_dgst_t;

   
                        
                                                                  
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IMG4_DGST_INIT (img4_dgst_t){ \
	.i4d_version = IMG4_DGST_STRUCT_VERSION, \
	.i4d_len = 0, \
	.i4d_bytes = {0}, \
}
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define IMG4_DGST_INIT (img4_nonce_t{ \
	IMG4_DGST_STRUCT_VERSION, \
	0, \
	{0}, \
})
#elif defined(__cplusplus)
#define IMG4_DGST_INIT (img4_nonce_t((img4_nonce_t){ \
	IMG4_DGST_STRUCT_VERSION, \
	0, \
	{0}, \
}))
#else
#define IMG4_DGST_INIT {IMG4_DGST_STRUCT_VERSION}
#endif

   
                               
                                            
  
            
                                                       
  
              
                                                                           
                                                                               
                                                                                
                                       
   
IMG4_API_AVAILABLE_20200508
typedef void (*img4_runtime_init_t)(
	const img4_runtime_t *rt
);

   
                                
                          
  
            
                                                       
  
           
                                   
  
          
                                                                               
   
IMG4_API_AVAILABLE_20200508
typedef void *_Nullable (*img4_runtime_alloc_t)(
	const img4_runtime_t *rt,
	size_t n
);

   
                                  
                           
  
            
                                                       
  
           
                                                                        
                                 
  
           
                                                                      
                               
   
IMG4_API_AVAILABLE_20200508
typedef void (*img4_runtime_dealloc_t)(
	const img4_runtime_t *rt,
	void *_Nullable p,
	size_t n
);

   
                            
                                                                      
  
                              
                                                                             
             
  
                             
                                                              
  
                              
                                                         
  
                             
                                                              
   
IMG4_API_AVAILABLE_20200508
OS_CLOSED_ENUM(img4_log_level, uint64_t,
	IMG4_LOG_LEVEL_ERROR,
	IMG4_LOG_LEVEL_INFO,
	IMG4_LOG_LEVEL_DEBUG,
	_IMG4_LOG_LEVEL_CNT,
);

   
                              
                                        
  
            
                                                       
  
                
                                                         
  
               
                                                                                
                                       
  
             
                                   
  
             
                                                                    
                               
   
OS_FORMAT_PRINTF(4, 5)
IMG4_API_AVAILABLE_20200508
typedef void (*img4_runtime_log_t)(
	const img4_runtime_t *rt,
	void *_Nullable handle,
	img4_log_level_t level,
	const char *fmt,
	...
);

   
                                     
                                         
  
            
                                                       
  
          
                                                                             
   
IMG4_API_AVAILABLE_20200508
typedef void *_Nullable (*img4_runtime_log_handle_t)(
	const img4_runtime_t *rt
);

   
                                              
                                                          
  
            
                                                       
  
              
                                                 
  
                    
                              
  
               
                                                                               
  
          
                                                                             
                                               
  
                                                                    
                                                                           
                           
                                                                              
   
IMG4_API_AVAILABLE_20200508
typedef errno_t (*img4_runtime_get_identifier_bool_t)(
	const img4_runtime_t *rt,
	const img4_chip_t *chip,
	img4_identifier_t identifier,
	bool *value
);

   
                                                
                                                                           
  
            
                                                       
  
              
                                                 
  
                    
                              
  
               
                                                                               
  
          
                                                                             
                                               
  
                                                                    
                                                                           
                           
                                                                              
   
IMG4_API_AVAILABLE_20200508
typedef errno_t (*img4_runtime_get_identifier_uint32_t)(
	const img4_runtime_t *rt,
	const img4_chip_t *chip,
	img4_identifier_t identifier,
	uint32_t *value
);

   
                                                
                                                                           
  
            
                                                       
  
              
                                                 
  
                    
                              
  
               
                                                                               
  
          
                                                                             
                                               
  
                                                                    
                                                                           
                           
                                                                              
   
IMG4_API_AVAILABLE_20200508
typedef errno_t (*img4_runtime_get_identifier_uint64_t)(
	const img4_runtime_t *rt,
	const img4_chip_t *chip,
	img4_identifier_t identifier,
	uint64_t *value
);

   
                                                
                                                         
  
            
                                                       
  
              
                                                 
  
                    
                              
  
               
                                                                               
  
          
                                                                             
                                               
  
                                                                    
                                                                           
                           
                                                                              
   
IMG4_API_AVAILABLE_20200508
typedef errno_t (*img4_runtime_get_identifier_digest_t)(
	const img4_runtime_t *rt,
	const img4_chip_t *chip,
	img4_identifier_t identifier,
	img4_dgst_t *value
);

   
                                   
                                                                    
                  
   
#define IMG4_BUFF_STRUCT_VERSION (0u)

   
                     
                                   
  
                     
                                                                                
  
                   
                           
  
                 
                            
  
                     
                                                                          
                                                                        
                                                                             
                                                    
   
struct _img4_buff {
	img4_struct_version_t i4b_version;
	uint8_t *i4b_bytes;
	size_t i4b_len;
	img4_runtime_dealloc_t _Nullable i4b_dealloc;
} IMG4_API_AVAILABLE_20200508;

   
                        
                                                                   
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IMG4_BUFF_INIT (img4_buff_t){ \
	.i4b_version = IMG4_BUFF_STRUCT_VERSION, \
	.i4b_len = 0, \
	.i4b_bytes = NULL, \
	.i4b_dealloc = NULL, \
}
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define IMG4_BUFF_INIT (img4_buff_t{ \
	IMG4_BUFF_STRUCT_VERSION, \
	NULL, \
	0, \
	NULL, \
})
#elif defined(__cplusplus)
#define IMG4_BUFF_INIT (img4_buff_t((img4_buff_t){ \
	IMG4_BUFF_STRUCT_VERSION, \
	NULL, \
	0, \
	NULL, \
}))
#else
#define IMG4_BUFF_INIT {IMG4_BUFF_STRUCT_VERSION}
#endif

   
                                      
                                                                       
                  
   
#define IMG4_RUNTIME_STRUCT_VERSION (1u)

   
                        
                                                                            
           
  
                      
                                                                            
                                                                        
  
                   
                                       
  
                   
                                                         
                               
  
                    
                                                                              
                                    
  
                      
                                                                                
                                  
  
                  
                                                            
  
                         
                                                                              
  
                                  
                                                  
  
                                    
                                                                  
  
                                    
                                                                  
  
                                    
                                                 
  
                      
                                  
   
struct _img4_runtime {
	img4_struct_version_t i4rt_version;
	const char *i4rt_name;
	img4_runtime_init_t _Nullable i4rt_init;
	img4_runtime_alloc_t i4rt_alloc;
	img4_runtime_dealloc_t i4rt_dealloc;
	img4_runtime_log_t i4rt_log;
	img4_runtime_log_handle_t i4rt_log_handle;
	img4_runtime_get_identifier_bool_t i4rt_get_identifier_bool;
	img4_runtime_get_identifier_uint32_t i4rt_get_identifier_uint32;
	img4_runtime_get_identifier_uint64_t i4rt_get_identifier_uint64;
	img4_runtime_get_identifier_digest_t i4rt_get_identifier_digest;
	void *_Nullable i4rt_context;
} IMG4_API_AVAILABLE_20200508;

   
                                  
                                                                   
                                                                               
                                                                               
                                       
  
             
                                            
   
#define IMG4_RUNTIME_REGISTER(_rt) LINKER_SET_ENTRY(__img4_rt, _rt);

   
                              
                                                             
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_runtime_t _img4_runtime_default;
#define IMG4_RUNTIME_DEFAULT (&_img4_runtime_default)
#else
#define IMG4_RUNTIME_DEFAULT (img4if->i4if_v7.runtime_default)
#endif

   
                              
                                                                              
                                                                             
                                                     
   
#if XNU_KERNEL_PRIVATE
#define IMG4_RUNTIME_PMAP_CS (img4if->i4if_v7.runtime_pmap_cs)
#endif

   
                              
                                                                             
                                   
   
#if !KERNEL
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_runtime_t _img4_runtime_restore;
#define IMG4_RUNTIME_RESTORE (&_img4_runtime_restore)
#endif

   
                              
                                                               
  
              
                                                                           
                                                         
  
              
                                                                   
                                                                             
                  
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
void
img4_buff_dealloc(img4_buff_t *_Nullable buff);
#else
#define img4_buff_dealloc(...) (img4if->i4if_v7.buff_dealloc(__VA_ARGS__))
#endif

OS_ASSUME_NONNULL_END

#endif                    
