   
          
                                                      
   
#ifndef __IMG4_IMAGE_H
#define __IMG4_IMAGE_H

#ifndef __IMG4_INDIRECT
#error "Please #include <img4/firmware.h> instead of this file directly"
#endif                   

#if IMG4_TAPI
#include "tapi.h"
#endif

OS_ASSUME_NONNULL_BEGIN

   
                                 
                                                          
  
               
                      
  
          
                                                      
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1
const img4_buff_t *
img4_image_get_bytes(img4_image_t image);
#else
#define img4_image_get_bytes(...) (img4if->i4if_v7.image_get_bytes(__VA_ARGS__))
#endif

   
                                         
                                                                
  
               
                      
  
              
                                         
  
                 
                                            
  
          
                                                                              
                                                                               
                                      
  
              
                                                                              
                                                                               
                                      
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL3
const bool *
img4_image_get_property_bool(img4_image_t image,
		img4_4cc_t _4cc,
		bool *storage);
#else
#define img4_image_get_property_bool(...) \
		(img4if->i4if_v7.image_get_property_bool(__VA_ARGS__))
#endif

   
                                           
                                                                                
  
               
                      
  
              
                                         
  
                 
                                                            
  
          
                                                                              
                                                                               
                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL3
const uint32_t *
img4_image_get_property_uint32(img4_image_t image,
		img4_4cc_t _4cc,
		uint32_t *storage);
#else
#define img4_image_get_property_uint32(...) \
		(img4if->i4if_v7.image_get_property_uint32(__VA_ARGS__))
#endif

   
                                           
                                                                                
  
               
                      
  
              
                                         
  
                 
                                                            
  
          
                                                                              
                                                                               
                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL3
const uint64_t *
img4_image_get_property_uint64(img4_image_t image,
		img4_4cc_t _4cc,
		uint64_t *storage);
#else
#define img4_image_get_property_uint64(...) \
		(img4if->i4if_v7.image_get_property_uint64(__VA_ARGS__))
#endif

   
                                         
                                                               
  
               
                      
  
              
                                         
  
                 
                                           
  
          
                                                                              
                                                                               
                                   
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL3
const img4_buff_t *
img4_image_get_property_data(img4_image_t image,
		img4_4cc_t _4cc,
		img4_buff_t *storage);
#else
#define img4_image_get_property_data(...) \
		(img4if->i4if_v7.image_get_property_data(__VA_ARGS__))
#endif

OS_ASSUME_NONNULL_END

#endif                  
