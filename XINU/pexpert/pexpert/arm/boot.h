/*
 * Copyright (c) 2007-2009 Apple Inc. All rights reserved.
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 */
/*
 * @OSF_COPYRIGHT@
 */

#ifndef _PEXPERT_ARM_BOOT_H_
#define _PEXPERT_ARM_BOOT_H_

#include <pexpert/arm/consistent_debug.h>

#define BOOT_LINE_LENGTH        256

  
                      
   

struct Boot_Video {
	unsigned long   v_baseAddr;                                       
	unsigned long   v_display;                                       
	unsigned long   v_rowBytes;                                        
	unsigned long   v_width;                   
	unsigned long   v_height;                   
	unsigned long   v_depth;                                              
};

#define kBootVideoDepthMask             (0xFF)
#define kBootVideoDepthDepthShift       (0)
#define kBootVideoDepthRotateShift      (8)
#define kBootVideoDepthScaleShift       (16)
#define kBootVideoDepthBootRotateShift  (24)

#define kBootFlagsDarkBoot              (1 << 0)

typedef struct Boot_Video       Boot_Video;

                                                                  
   
#define kBootArgsRevision               1
#define kBootArgsRevision2              2                                       
#define kBootArgsVersion1               1
#define kBootArgsVersion2               2

typedef struct boot_args {
	uint16_t                Revision;                                                            
	uint16_t                Version;                                                            
	uint32_t                virtBase;                                                   
	uint32_t                physBase;                                                    
	uint32_t                memSize;                                            
	uint32_t                topOfKernelData;                                                               
	Boot_Video              Video;                                                 
	uint32_t                machineType;                              
	void                    *deviceTreeP;                                              
	uint32_t                deviceTreeLength;                                     
	char                    CommandLine[BOOT_LINE_LENGTH];                              
	uint32_t                bootFlags;                                                                
	uint32_t                memSizeActual;                                     
} boot_args;

#define SOC_DEVICE_TYPE_BUFFER_SIZE     32

#define PC_TRACE_BUF_SIZE               1024

#define CDBG_MEM ((sizeof(dbg_registry_t) + PAGE_SIZE - 1) & ~PAGE_MASK)

#endif                           
