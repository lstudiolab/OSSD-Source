   
          
                                                       
   
#ifndef __IMG4_FIRMWARE_H
#define __IMG4_FIRMWARE_H

#include <os/base.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>

__BEGIN_DECLS;

#if !KERNEL
#include <os/stdio.h>
#endif

#if !_DARWIN_BUILDING_PROJECT_APPLEIMAGE4
#if __has_include(<os/linker_set.h>) && !KERNEL
#include <os/linker_set.h>
#elif XNU_KERNEL_PRIVATE
                                                                            
                         
  
                            
#include <sys/linker_set.h>
#else
#define LINKER_SET_ENTRY(...)
#endif
#endif                                         

   
              
                                                                            
                                                                              
                                                                           
                                                  
  
                                                                           
                                                 
                                                                               
                                                                                
                                 
   
#if MACH_KERNEL_PRIVATE
#define _SYS_TYPES_H_ 1
#include <sys/kernel_types.h>
#include <sys/_types/_errno_t.h>
#else
#include <sys/kernel_types.h>
#include <sys/types.h>
#endif

#define __IMG4_INDIRECT 1
#include <img4/api.h>

#if IMG4_TAPI
#include "tapi.h"
#endif

OS_ASSUME_NONNULL_BEGIN

   
                      
                                                                          
                                                                                
                                                                     
   
IMG4_API_AVAILABLE_20200508
typedef uint32_t img4_4cc_t;

   
                       
                                                           
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_buff img4_buff_t;

   
                           
                                                       
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_firmware *img4_firmware_t;

   
                        
                                                                    
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_image *img4_image_t;

   
                          
                                                                            
                                      
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_runtime img4_runtime_t;

OS_ASSUME_NONNULL_END

#if !_DARWIN_BUILDING_PROJECT_APPLEIMAGE4 || IMG4_TAPI
#define __IMG4_INDIRECT 1
#include <img4/nonce.h>
#include <img4/object.h>
#include <img4/chip.h>
#include <img4/image.h>
#include <img4/runtime.h>
#endif

OS_ASSUME_NONNULL_BEGIN

   
                                                 
                                                                               
                                                              
  
            
                                             
  
               
                                                                             
                                    
  
                                                                        
  
               
                                                                               
                                                                           
                                          
  
                                                                               
                                                        
                                                   
                                                                        
                                                                            
                            
                                                                               
                                   
                                                                          
                                     
                                                                                
                                     
                                                                               
                                                                              
                                     
  
              
                                     
   
IMG4_API_AVAILABLE_20200508
typedef void (*img4_firmware_authenticated_execute_t)(
	const img4_firmware_t fw,
	img4_image_t _Nullable image,
	errno_t error,
	void *_ctx
);

   
                                                         
                                                                         
                                   
   
#define IMG4_FIRMWARE_EXECUTION_CONTEXT_STRUCT_VERSION (0u)

   
                                             
                                                                            
  
                       
                                                                              
                                                          
  
                       
                                                
  
                       
                                                                             
                              
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_firmware_execution_context {
	img4_struct_version_t i4fex_version;
	img4_firmware_authenticated_execute_t i4fex_execute;
	void *i4fex_context;
} img4_firmware_execution_context_t;

   
                                 
                                                                          
  
                                 
                                                                   
  
                                              
                                                                              
                            
  
                                 
                                                                                
                                                                               
                                                                   
  
                                             
                                                                     
                                                                          
                                           
  
                                                                               
                                                          
  
                                         
                                                                                
                                                                            
                                                                          
                                                  
  
                                                                            
               
   
IMG4_API_AVAILABLE_20200508
OS_CLOSED_OPTIONS(img4_firmware_flags, uint64_t,
	IMG4_FIRMWARE_FLAG_INIT,
	IMG4_FIRMWARE_FLAG_ATTACHED_MANIFEST = (1 << 0),
	IMG4_FIRMWARE_FLAG_BARE = (1 << 1),
	IMG4_FIRMWARE_FLAG_SUBSEQUENT_STAGE = (1 << 2),
	IMG4_FIRMWARE_FLAG_RESPECT_AMNM = (1 << 3),
);

   
                              
                                                 
  
            
                                                 
  
              
                                            
  
              
                                                                               
                        
  
                                                                           
                                                                                
  
               
                                              
  
          
                                                                    
  
              
                                                              
  
                                                              
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_MALLOC OS_NONNULL1 OS_NONNULL2 OS_NONNULL4
img4_firmware_t _Nullable
img4_firmware_new(const img4_runtime_t *rt,
		const img4_firmware_execution_context_t *exec,
		img4_4cc_t _4cc,
		img4_buff_t *buff,
		img4_firmware_flags_t flags);
#else
#define img4_firmware_new(...) (img4if->i4if_v7.firmware_new(__VA_ARGS__))
#endif

   
                                              
                                                              
  
            
                                                                        
                                                                              
                                              
  
              
                                            
  
            
                                                                                
                        
  
               
                                              
  
          
                                                                    
  
              
                                                                              
                                                                               
                                                                               
                                                                              
                                                                       
  
                                                            
                                                                          
                   
  
                                                                   
                                                     
   
#if KERNEL
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_MALLOC OS_NONNULL1 OS_NONNULL2 OS_NONNULL4
img4_firmware_t _Nullable
img4_firmware_new_from_vnode_4xnu(const img4_runtime_t *rt,
		const img4_firmware_execution_context_t *exec,
		img4_4cc_t _4cc,
		vnode_t vn,
		img4_firmware_flags_t flags);
#else
#define img4_firmware_new_from_vnode_4xnu(...) \
		(img4if->i4if_v7.firmware_new_from_vnode_4xnu(__VA_ARGS__))
#endif                       
#endif           

   
                                           
                                                                        
  
            
                                                                        
                                                                       
                                                        
  
              
                                            
  
            
                                                                              
                                                                              
                                                                               
                
  
               
                                              
  
          
                                                                  
  
              
                                                
                                                                             
   
#if !KERNEL
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_MALLOC OS_NONNULL1 OS_NONNULL2
img4_firmware_t
img4_firmware_new_from_fd_4MSM(const img4_runtime_t *rt,
		const img4_firmware_execution_context_t *exec,
		img4_4cc_t _4cc,
		os_fd_t *fd,
		img4_firmware_flags_t flags);
#endif

   
                                         
                                                                          
                                                               
  
                 
                                                           
  
             
                          
  
              
                                                                                
                                                                 
  
           
  
                                                     
                                 
  
                                                               
                                                                  
                                          
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1
img4_firmware_t
img4_firmware_init_from_buff(void *buff, size_t len);
#else
#define img4_firmware_init_from_buff(...) \
		(img4if->i4if_v7.firmware_init_from_buff(__VA_ARGS__))
#endif

   
                               
                                
  
            
                                                                              
                                                                             
                                                                        
                           
  
            
                                                 
  
              
                                            
  
              
                                                                               
                        
  
                                                                           
                                                                                
  
               
                                              
  
              
                                                                           
                                                                             
                                                     
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_NONNULL1 OS_NONNULL2 OS_NONNULL3 OS_NONNULL5
void
img4_firmware_init(img4_firmware_t fw,
		const img4_runtime_t *rt,
		const img4_firmware_execution_context_t *exec,
		img4_4cc_t _4cc,
		img4_buff_t *buff,
		img4_firmware_flags_t flags);
#else
#define img4_firmware_init(...) (img4if->i4if_v7.firmware_init(__VA_ARGS__))
#endif

   
                                          
                                              
  
            
                              
  
              
                                                                                
                        
  
                                                                           
                                                                                
  
              
                                                             
                                                                          
                         
  
                                                                 
                                        
  
                                                    
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_NONNULL1 OS_NONNULL2
void
img4_firmware_attach_manifest(img4_firmware_t fw,
		img4_buff_t *buff);
#else
#define img4_firmware_attach_manifest(...) \
		(img4if->i4if_v7.firmware_attach_manifest(__VA_ARGS__))
#endif

   
                                      
                                                                             
                
  
            
                         
  
                          
                                                                        
  
                              
                                                      
  
          
                                                                                
                                                                          
                                                                             
                           
  
              
                                                                          
                                  
  
                                                                  
                                                                        
  
                                          
                                                                              
                                                                            
                          
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200724
OS_EXPORT OS_WARN_RESULT
const img4_chip_t *_Nullable
img4_firmware_select_chip(const img4_firmware_t fw,
		const img4_chip_select_array_t _Nonnull acceptable_chips,
		size_t acceptable_chips_cnt);
#else
#define img4_firmware_select_chip(...) \
		(img4if->i4if_v10.firmware_select_chip(__VA_ARGS__))
#endif

   
                                  
                                                               
  
            
                           
  
              
                                             
  
               
                                                                                
                                                                        
             
  
              
                                            
                                                                          
                                                                                
                              
  
                                                                         
                          
  
                                                                             
                                                                             
                                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_NONNULL1 OS_NONNULL2
void
img4_firmware_execute(img4_firmware_t fw,
		const img4_chip_t *chip,
		const img4_nonce_t *_Nullable nonce);
#else
#define img4_firmware_execute(...) \
		(img4if->i4if_v7.firmware_execute(__VA_ARGS__))
#endif

   
                                   
                                          
  
            
                            
  
              
                                              
  
               
                                                                                
                                                                        
             
  
          
                                                                               
                                                                                
                    
  
                                                                               
                                                          
                                                   
                                                                        
                                                                            
                            
                                                                               
                                   
                                                                          
                                     
                                                                                
                                     
                                                                               
                                                                              
                                     
  
              
                                                                           
                                                                          
              
  
                                                                              
                                                                             
                                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200608
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL2
errno_t
img4_firmware_evaluate(img4_firmware_t fw,
		const img4_chip_t *chip,
		const img4_nonce_t *_Nullable nonce);
#else
#define img4_firmware_evaluate(...) \
		(img4if->i4if_v9.firmware_evaluate(__VA_ARGS__))
#endif

   
                                  
                                                                                
                               
  
            
                                    
  
                                                                              
                                                                     
               
  
              
                                                                           
                                         
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
void
img4_firmware_destroy(img4_firmware_t _Nonnull *_Nullable fw);
#else
#define img4_firmware_destroy(...) \
		(img4if->i4if_v7.firmware_destroy(__VA_ARGS__))
#endif

OS_ASSUME_NONNULL_END

__END_DECLS;

#endif                     
