/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#ifndef _MACH_O_FAT_H_
#define _MACH_O_FAT_H_
  
                                                                         
                                                                            
                                                                     
                                                                         
                                                                         
                                                                      
                                                                               
                                                                                
                                                  
  
                                                                       
                       
   

  
                                                                             
                                                                     
   
#include <stdint.h>
#include <mach/machine.h>
#include <architecture/byte_order.h>

#define FAT_MAGIC	0xcafebabe
#define FAT_CIGAM	0xbebafeca	                           

struct fat_header {
	uint32_t	magic;		               
	uint32_t	nfat_arch;	                                   
};

struct fat_arch {
	cpu_type_t	cputype;	                         
	cpu_subtype_t	cpusubtype;	                             
	uint32_t	offset;		                                     
	uint32_t	size;		                              
	uint32_t	align;		                               
};

#endif                     
