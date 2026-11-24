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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
  
           
                                        
                                                         
  
                                         
                                                       
                                                          
                                                            
                                        
                                                               
                                                            
                       
   

#ifndef _SYS_DISKTAB_
#define _SYS_DISKTAB_

#include <sys/appleapiopts.h>

#ifdef  __APPLE_API_OBSOLETE

  
                                         
   
#ifndef KERNEL
#define DISKTAB         "/etc/disktab"
#endif               

#define MAXDNMLEN       24                          
#define MAXMPTLEN       16                           
#define MAXFSTLEN       8                                 
#define MAXTYPLEN       24                          
#define NBOOTS          2                          
#define MAXBFLEN        24                             
#define MAXHNLEN        32                         
#define NPART           8                         

typedef struct partition {
	int     p_base;                                        
	int     p_size;                                    
	short   p_bsize;                                 
	short   p_fsize;                                
	char    p_opt;                                                
	short   p_cpg;                                   
	short   p_density;                                   
	char    p_minfree;                       
	char    p_newfs;                                   
	char    p_mountpt[MAXMPTLEN];                 
	char    p_automnt;                                    
	char    p_type[MAXFSTLEN];                      
} partition_t;

typedef struct disktab {
	char    d_name[MAXDNMLEN];                      
	char    d_type[MAXTYPLEN];                      
	int     d_secsize;                                        
	int     d_ntracks;                                     
	int     d_nsectors;                                  
	int     d_ncylinders;                            
	int     d_rpm;                                          
	short   d_front;                                                   
	short   d_back;                                                   
	short   d_ngroups;                                        
	short   d_ag_size;                                            
	short   d_ag_alts;                                                 
	short   d_ag_off;                                                     
	int     d_boot0_blkno[NBOOTS];                              
	char    d_bootfile[MAXBFLEN];                         
	char    d_hostname[MAXHNLEN];                  
	char    d_rootpartition;                                     
	char    d_rwpartition;                                      
	partition_t d_partitions[NPART];
} disktab_t;

#ifndef KERNEL
struct  disktab *getdiskbyname(), *getdiskbydev();
#endif               

#endif                            

#endif                     
