/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                  

                                                                       
  
                                                           
                                        
  
                                                                          


  
                                                                       
                    
   

#ifndef _SYS_LOADABLE_FS_
#define _SYS_LOADABLE_FS_


  
                                                                         
  
                                          
  
                                                           
                     
                                                            
                                                                
                                                             
                                                            
   
#define FS_DIR_LOCATION         "/System/Library/Filesystems"
#define FS_DIR_SUFFIX           ".fs"
#define FS_UTIL_SUFFIX          ".util"

  
                                                                             
   
#define FSUC_PROBE              'p'                                     
                                                       

#define FSUC_PROBEFORINIT       'P'                                 
                                              

#define FSUC_MOUNT              'm'                   
                                                            

#define FSUC_REPAIR             'r'                             
                                              

#define FSUC_UNMOUNT            'u'                     
                                         

                                                    
#define FSUC_MOUNT_FORCE        'M'                                            
                                             
  
                                  
   
#define FSUR_RECOGNIZED         (-1)                                           
                                                          
#define FSUR_UNRECOGNIZED       (-2)                                         
#define FSUR_IO_SUCCESS         (-3)                                          
#define FSUR_IO_FAIL            (-4)                                 
#define FSUR_IO_UNCLEAN         (-5)                                          
                                    
#define FSUR_INVAL              (-6)                          
#define FSUR_LOADERR            (-7)                           
#define FSUR_INITRECOGNIZED     (-8)                                
                                                                   
                                                                 

  
                                                         
   
#define DEVICE_READONLY         "readonly"
#define DEVICE_WRITABLE         "writable"

#define DEVICE_REMOVABLE        "removable"
#define DEVICE_FIXED            "fixed"

#endif                         
