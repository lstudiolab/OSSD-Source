   
          
                               
   
#ifndef __IMG4_CHIP_H
#define __IMG4_CHIP_H

#ifndef __IMG4_INDIRECT
#error "Please #include <img4/firmware.h> instead of this file directly"
#endif                   

#if IMG4_TAPI
#include "tapi.h"
#endif

OS_ASSUME_NONNULL_BEGIN

   
                       
                                                                            
         
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_chip img4_chip_t;

   
                                    
                                                                                
   
IMG4_API_AVAILABLE_20200724
typedef const img4_chip_t *_Nullable const *img4_chip_select_array_t;

   
                                           
                                                                   
                  
   
#define IMG4_CHIP_INSTANCE_STRUCT_VERSION (1u)

   
                                     
                                                                  
  
                                      
                                  
  
                                      
                                             
  
                                      
                                            
  
                                      
                                            
  
                                      
                                                   
  
                                      
                                                          
  
                                      
                                                      
  
                                      
                                                        
  
                                      
                                                    
  
                                      
                                                            
  
                                      
                                                  
  
                                      
                                                               
   
OS_CLOSED_OPTIONS(img4_chip_instance_omit, uint64_t,
	IMG4_CHIP_INSTANCE_OMIT_CEPO = (1 << 0),
	IMG4_CHIP_INSTANCE_OMIT_BORD = (1 << 1),
	IMG4_CHIP_INSTANCE_OMIT_CHIP = (1 << 2),
	IMG4_CHIP_INSTANCE_OMIT_SDOM = (1 << 3),
	IMG4_CHIP_INSTANCE_OMIT_ECID = (1 << 4),
	IMG4_CHIP_INSTANCE_OMIT_CPRO = (1 << 5),
	IMG4_CHIP_INSTANCE_OMIT_CSEC = (1 << 6),
	IMG4_CHIP_INSTANCE_OMIT_EPRO = (1 << 7),
	IMG4_CHIP_INSTANCE_OMIT_ESEC = (1 << 8),
	IMG4_CHIP_INSTANCE_OMIT_IUOU = (1 << 9),
	IMG4_CHIP_INSTANCE_OMIT_RSCH = (1 << 10),
	IMG4_CHIP_INSTANCE_OMIT_EUOU = (1 << 11),
);

   
                                
                                                 
  
                      
                                              
                                             
  
                          
                                                
  
                   
                                                           
  
                   
                                              
  
                   
                                             
  
                   
                                            
  
                   
                                            
  
                   
                                                   
  
                   
                                                          
  
                   
                                                      
  
                   
                                                        
  
                   
                                                    
  
                   
                                                          
  
                   
                                          
  
                   
                                                             
  
                                       
   
IMG4_API_AVAILABLE_20200508
typedef struct _img4_chip_instance {
	img4_struct_version_t chid_version;
	const img4_chip_t *chid_chip_family;
	img4_chip_instance_omit_t chid_omit;
	uint32_t chid_cepo;
	uint32_t chid_bord;
	uint32_t chid_chip;
	uint32_t chid_sdom;
	uint64_t chid_ecid;
	bool chid_cpro;
	bool chid_csec;
	bool chid_epro;
	bool chid_esec;
	bool chid_iuou;
	bool chid_rsch;
	bool chid_euou;
} img4_chip_instance_t;

   
                           
                                                                      
                    
  
                                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_sha1;
#define IMG4_CHIP_AP_SHA1 (&_img4_chip_ap_sha1)
#else
#define IMG4_CHIP_AP_SHA1 (img4if->i4if_v7.chip_ap_sha1)
#endif

   
                               
                                                                          
                    
  
                                                                       
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_sha2_384;
#define IMG4_CHIP_AP_SHA2_384 (&_img4_chip_ap_sha2_384)
#else
#define IMG4_CHIP_AP_SHA2_384 (img4if->i4if_v7.chip_ap_sha2_384)
#endif

   
                             
                                                              
                                                                             
                                                                             
               
  
                                                                            
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_hybrid;
#define IMG4_CHIP_AP_HYBRID (&_img4_chip_ap_hybrid)
#else
#define IMG4_CHIP_AP_HYBRID (img4if->i4if_v7.chip_ap_hybrid)
#endif

   
                              
                                                                               
                 
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_reduced;
#define IMG4_CHIP_AP_REDUCED (&_img4_chip_ap_reduced)
#else
#define IMG4_CHIP_AP_REDUCED (img4if->i4if_v7.chip_ap_reduced)
#endif

   
                                 
                                                                             
               
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_permissive;
#define IMG4_CHIP_AP_PERMISSIVE (&_img4_chip_ap_permissive)
#else
#define IMG4_CHIP_AP_PERMISSIVE (img4if->i4if_v8.chip_ap_permissive)
#endif

   
                                    
                                                              
                                                                          
                                                              
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_hybrid_medium;
#define IMG4_CHIP_AP_HYBRID_MEDIUM (&_img4_chip_ap_hybrid_medium)
#else
#define IMG4_CHIP_AP_HYBRID_MEDIUM (img4if->i4if_v8.chip_ap_hybrid_medium)
#endif

   
                                     
                                                              
                                                                            
                                                                
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_hybrid_relaxed;
#define IMG4_CHIP_AP_HYBRID_RELAXED (&_img4_chip_ap_hybrid_relaxed)
#else
#define IMG4_CHIP_AP_HYBRID_RELAXED (img4if->i4if_v8.chip_ap_hybrid_relaxed)
#endif

   
                                    
                                                                          
                                                                              
                                         
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_software_ff00;
#define IMG4_CHIP_AP_SOFTWARE_FF00 (&_img4_chip_ap_software_ff00)
#else
#define IMG4_CHIP_AP_SOFTWARE_FF00 (img4if->i4if_v7.chip_ap_software_ff00)
#endif

   
                                    
                                                                          
                                                                              
                                                                     
                           
  
                                           
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_ap_software_ff01;
#define IMG4_CHIP_AP_SOFTWARE_FF01 (&_img4_chip_ap_software_ff01)
#else
#define IMG4_CHIP_AP_SOFTWARE_FF01 (img4if->i4if_v7.chip_ap_software_ff01)
#endif

   
                       
                                                              
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_x86;
#define IMG4_CHIP_X86 (&_img4_chip_x86)
#else
#define IMG4_CHIP_X86 (img4if->i4if_v7.chip_x86)
#endif

   
                                     
                                                                              
                                                                               
                                                                             
                                                    
                                                                               
                                                                            
  
                                                                       
                                                                    
  
              
                                        
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT
const img4_chip_t _img4_chip_x86_software_8012;
#define IMG4_CHIP_X86_SOFTWARE_8012 (&_img4_chip_x86_software_8012)
#else
#define IMG4_CHIP_X86_SOFTWARE_8012 (img4if->i4if_v7.chip_x86_software_8012)
#endif

   
                                     
                                         
  
              
                                                       
  
             
                          
  
              
                                                                                
                                                                 
  
           
  
                                                 
                                
  
                                                             
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1
img4_chip_t *
img4_chip_init_from_buff(void *buff, size_t len);
#else
#define img4_chip_init_from_buff (img4if->i4if_v7.chip_init_from_buff)
#endif

   
                                             
                                                                              
      
  
          
                                                                          
                    
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT
const img4_chip_t *
img4_chip_select_personalized_ap(void);
#else
#define img4_chip_select_personalized_ap(...) \
		(img4if->i4if_v7.chip_select_personalized_ap(__VA_ARGS__))
#endif

   
                                          
                                                                     
  
          
                                                                          
                                   
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT
const img4_chip_t *
img4_chip_select_effective_ap(void);
#else
#define img4_chip_select_effective_ap(...) \
		(img4if->i4if_v7.chip_select_effective_ap(__VA_ARGS__))
#endif

   
                                  
                                                                             
             
  
              
                           
  
                       
                                                                              
                                                            
  
          
                                                                              
                    
  
                                                                              
                                                                               
                                          
                                                                               
                                                                        
                                  
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1 OS_NONNULL2
errno_t
img4_chip_instantiate(const img4_chip_t *chip,
		img4_chip_instance_t *chip_instance);
#else
#define img4_chip_instantiate(...) \
		(img4if->i4if_v7.chip_instantiate(__VA_ARGS__))
#endif

   
                             
                                                                  
                                                                         
                                              
  
                       
                                   
  
                                                                               
                                                                       
  
              
                                                
  
                                                                               
                                                                       
  
                                                             
                                    
  
          
                     
   
#if !XNU_KERNEL_PRIVATE
IMG4_API_AVAILABLE_20200508
OS_EXPORT OS_WARN_RESULT OS_NONNULL1
const img4_chip_t *
img4_chip_custom(const img4_chip_instance_t *chip_instance, img4_chip_t *chip);
#else
#define img4_chip_custom(...) (img4if->i4if_v7.chip_custom(__VA_ARGS__))
#endif

OS_ASSUME_NONNULL_END

#endif                 
