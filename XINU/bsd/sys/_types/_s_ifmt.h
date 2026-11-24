/*
 * Copyright (c) 2003-2012 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                                      
                                                
   
#ifndef S_IFMT
               
#define S_IFMT          0170000                                      
#define S_IFIFO         0010000                                      
#define S_IFCHR         0020000                                      
#define S_IFDIR         0040000                              
#define S_IFBLK         0060000                                  
#define S_IFREG         0100000                            
#define S_IFLNK         0120000                                  
#define S_IFSOCK        0140000                           
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define S_IFWHT         0160000                                 
#endif

               
                                          
#define S_IRWXU         0000700                                       
#define S_IRUSR         0000400                                
#define S_IWUSR         0000200                                
#define S_IXUSR         0000100                                
                                          
#define S_IRWXG         0000070                                       
#define S_IRGRP         0000040                                
#define S_IWGRP         0000020                                
#define S_IXGRP         0000010                                
                                           
#define S_IRWXO         0000007                                       
#define S_IROTH         0000004                                
#define S_IWOTH         0000002                                
#define S_IXOTH         0000001                                

#define S_ISUID         0004000                                             
#define S_ISGID         0002000                                              
#define S_ISVTX         0001000                                               

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define S_ISTXT         S_ISVTX                                        
#define S_IREAD         S_IRUSR                                     
#define S_IWRITE        S_IWUSR                                     
#define S_IEXEC         S_IXUSR                                     
#endif
#endif               
