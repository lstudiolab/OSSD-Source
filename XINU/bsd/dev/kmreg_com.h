/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*      Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 * kmreg_com.h - machine independent km ioctl interface.
 *
 * HISTORY
 * 16-Jan-92    Doug Mitchell at NeXT
 *      Created.
 */

#ifdef  KERNEL_PRIVATE

#ifndef _BSD_DEV_KMREG_COM_
#define _BSD_DEV_KMREG_COM_

#include <sys/types.h>
#include <sys/ioctl.h>

  
                                          
   
#define KM_COLOR_WHITE          0
#define KM_COLOR_LTGRAY         1
#define KM_COLOR_DKGRAY         2
#define KM_COLOR_BLACK          3

  
                                                                  
                                                                       
                                                       
  
                                                                     
                                                  
  
                                                                      
                                                        
                                                                   
                       
   
struct km_drawrect {
	unsigned short x;                                                    
	unsigned short y;
	unsigned short width;                                            
                                        
	unsigned short height;
	union {
		void *bits;                                                  
		int   fill;                                           
	} data;
};

  
                            
   
typedef enum {
	KM_ANIM_STOP,                                 
	KM_ANIM_SUSPEND,                     
	KM_ANIM_RESUME                      
} km_anim_ctl_t;

#define KMIOCPOPUP      _IO('k', 1)                                   
#define KMIOCRESTORE    _IO('k', 2)                                     
#define KMIOCDUMPLOG    _IO('k', 3)                                   
#define KMIOCDRAWRECT   _IOW('k', 5, struct km_drawrect)                   
                                                             
#define KMIOCERASERECT  _IOW('k', 6, struct km_drawrect)                    

#ifdef  KERNEL_PRIVATE
#define KMIOCDISABLCONS _IO('k', 8)                                           
#endif                      

#define KMIOCANIMCTL    _IOW('k',9, km_anim_ctl_t)
                    
#define KMIOCSTATUS     _IOR('k',10, int)                            
#define KMIOCSIZE       _IOR('k',11, struct winsize)                      

  
                                        
   
#define KMS_SEE_MSGS    0x00000001

#endif                           

#endif                      
