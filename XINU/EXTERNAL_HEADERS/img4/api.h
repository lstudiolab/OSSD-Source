   
          
                   
   
#ifndef __IMG4_API_H
#define __IMG4_API_H

#ifndef __IMG4_INDIRECT
#error "Please #include <img4/firmware.h> instead of this file directly"
#endif                   

#include <stdint.h>
#include <stdbool.h>
#include <os/base.h>

#ifndef KERNEL
#include <os/availability.h>
#include <unistd.h>
#endif

#if !XNU_KERNEL_PRIVATE
#include <TargetConditionals.h>
#endif

   
                          
                                                                             
                                                                               
                                                                             
  
                                       
                      
             
  
                                                                          
                                                            
                                                                           
                                                      
  
                                                                            
                                                                             
            
   
#define IMG4_API_VERSION (20200724u)

#if IMG4_TAPI || (!defined(KERNEL) && !IMG4_PROJECT_BUILD)
#define IMG4_API_AVAILABLE_20180112 \
		API_AVAILABLE( \
			macos(10.15), \
			ios(12.0), \
			tvos(12.0), \
			watchos(5.0))

#define IMG4_API_AVAILABLE_20180112_DEPRECATED \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(10.15, 11.0), \
			ios(12.0, 14.0), \
			tvos(12.0, 14.0), \
			watchos(5.0, 7.0))
#define IMG4_API_AVAILABLE_20181004 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(10.15, 11.0), \
			ios(12.2, 14.0), \
			tvos(12.2, 14.0), \
			watchos(5.2, 7.0))

                                                                              
                              
#define IMG4_API_AVAILABLE_20181106 \
		API_AVAILABLE( \
			macos(10.15), \
			ios(12.2), \
			tvos(12.2), \
			watchos(5.2))
#define IMG4_API_AVAILABLE_20181106_DEPRECATED \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(10.15, 11.0), \
			ios(12.2, 14.0), \
			tvos(12.2, 14.0), \
			watchos(5.2, 7.0))
#define IMG4_API_AVAILABLE_20190125 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(10.15, 11.0), \
			ios(13.0, 14.0), \
			tvos(13.0, 14.0), \
			watchos(6.0, 7.0))
#define IMG4_API_AVAILABLE_20191001 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(10.15.2, 11.0), \
			ios(13.3, 14.0), \
			tvos(13.3, 14.0), \
			watchos(6.1.1, 7.0))
#define IMG4_API_AVAILABLE_20191108 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(11.0, 11.0), \
			ios(14.0, 14.0), \
			tvos(14.0, 14.0), \
			watchos(7.0, 7.0))
#define IMG4_API_AVAILABLE_20200221 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(11.0, 11.0), \
			ios(14.0, 14.0), \
			tvos(14.0, 14.0), \
			watchos(7.0, 7.0))
#define IMG4_API_AVAILABLE_20200310 \
		API_DEPRECATED_WITH_REPLACEMENT( \
			"img4_firmware_t", \
			macos(11.0, 11.0), \
			ios(14.0, 14.0), \
			tvos(14.0, 14.0), \
			watchos(7.0, 7.0))
#define IMG4_API_AVAILABLE_20200508 \
		API_AVAILABLE( \
			macos(11.0), \
			ios(14.0), \
			tvos(14.0), \
			watchos(7.0), \
			bridgeos(5.0))
#define IMG4_API_AVAILABLE_20200608 \
		API_AVAILABLE( \
			macos(11.0), \
			ios(14.0), \
			tvos(14.0), \
			watchos(7.0), \
			bridgeos(5.0))
#define IMG4_API_AVAILABLE_20200724 \
		API_AVAILABLE( \
			macos(11.0), \
			ios(14.0), \
			tvos(14.0), \
			watchos(7.0), \
			bridgeos(5.0))
#else
#define IMG4_API_AVAILABLE_20180112
#define IMG4_API_AVAILABLE_20180112_DEPRECATED
#define IMG4_API_AVAILABLE_20181004
#define IMG4_API_AVAILABLE_20181106
#define IMG4_API_AVAILABLE_20181106_DEPRECATED
#define IMG4_API_AVAILABLE_20190125
#define IMG4_API_AVAILABLE_20191001
#define IMG4_API_AVAILABLE_20191108
#define IMG4_API_AVAILABLE_20200221
#define IMG4_API_AVAILABLE_20200310
#define IMG4_API_AVAILABLE_20200508
#define IMG4_API_AVAILABLE_20200608
#define IMG4_API_AVAILABLE_20200724
#endif

   
                                 
                                                               
   
IMG4_API_AVAILABLE_20180112
typedef uint16_t img4_struct_version_t;

#endif                
