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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)disklabel.h	8.2 (Berkeley) 7/10/94
 */
#ifndef _SYS_DISKLABEL_H_
#define _SYS_DISKLABEL_H_

#include <sys/appleapiopts.h>
#include <sys/types.h>                           

#ifdef __APPLE_API_OBSOLETE

  
                                         
   
#define _PATH_DISKTAB   "/etc/disktab"
#define DISKTAB         "/etc/disktab"                          

  
                                                                      
                                                                        
                                                                     
                                                              
                                    
   
#include <machine/disklabel.h>

  
                                                          
                                                                          
                                                                              
   
#define MAXMAXPARTITIONS        22
#if MAXPARTITIONS > MAXMAXPARTITIONS
#warning beware: MAXPARTITIONS bigger than MAXMAXPARTITIONS
#endif

  
                                                                       
   
#define DISKUNIT(dev)   (minor(dev) / MAXPARTITIONS)
#define DISKPART(dev)   (minor(dev) % MAXPARTITIONS)
#define MAKEDISKDEV(maj, unit, part) \
    (makedev((maj), ((unit) * MAXPARTITIONS) + (part)))

#define DISKMAGIC       ((u_int32_t)0x82564557)                            

#ifndef LOCORE
struct disklabel {
	u_int32_t d_magic;                                    
	u_int16_t d_type;                               
	u_int16_t d_subtype;                                            
	char      d_typename[16];                                    

	  
                                                                
                                                       
                                                       
                                                           
                                                         
                                                              
    
	union {
		char    un_d_packname[16];                           
		struct {
			char *un_d_boot0;                                   
			char *un_d_boot1;                                     
		} un_b;
	} d_un;
#define d_packname      d_un.un_d_packname
#define d_boot0         d_un.un_b.un_d_boot0
#define d_boot1         d_un.un_b.un_d_boot1

	                    
	u_int32_t d_secsize;                                       
	u_int32_t d_nsectors;                                            
	u_int32_t d_ntracks;                                          
	u_int32_t d_ncylinders;                                           
	u_int32_t d_secpercyl;                                              
	u_int32_t d_secperunit;                                         

	  
                                                             
                                                            
                                                             
                          
    
	u_int16_t d_sparespertrack;                                       
	u_int16_t d_sparespercyl;                                            
	  
                                                                       
                           
    
	u_int32_t d_acylinders;                                           

	                               
	  
                                                                  
                                                                 
                                                           
                                                                  
                                                         
                                                           
                                                              
                                                            
                                                             
                                                              
                                                                
                                                                   
                                                                
                    
    
	u_int16_t d_rpm;                                      
	u_int16_t d_interleave;                                         
	u_int16_t d_trackskew;                                        
	u_int16_t d_cylskew;                                             
	u_int32_t d_headswitch;                                     
	u_int32_t d_trkseek;                                           
	u_int32_t d_flags;                                 
#define NDDATA 5
	u_int32_t d_drivedata[NDDATA];                                       
#define NSPARE 5
	u_int32_t d_spare[NSPARE];                                   
	u_int32_t d_magic2;                                           
	u_int16_t d_checksum;                                             

	                                           
	u_int16_t d_npartitions;                                               
	u_int32_t d_bbsize;                                                  
	u_int32_t d_sbsize;                                                   
	struct  partition {                                      
		u_int32_t p_size;                                           
		u_int32_t p_offset;                          
		u_int32_t p_fsize;                                          
		u_int8_t p_fstype;                                      
		u_int8_t p_frag;                                            
		union {
			u_int16_t cpg;                                   
			u_int16_t sgs;                             
		} __partition_u1;
#define p_cpg   __partition_u1.cpg
#define p_sgs   __partition_u1.sgs
	} d_partitions[MAXPARTITIONS];                            
};
#else             
  
                              
   
.set    d_secsize, 40
.set    d_nsectors, 44
.set    d_ntracks, 48
.set    d_ncylinders, 52
.set    d_secpercyl, 56
.set    d_secperunit, 60
.set    d_end_, 276                                             
#endif             

                    
#define DTYPE_SMD               1                                         
#define DTYPE_MSCP              2                         
#define DTYPE_DEC               3                                       
#define DTYPE_SCSI              4                         
#define DTYPE_ESDI              5                                   
#define DTYPE_ST506             6                               
#define DTYPE_HPIB              7                                   
#define DTYPE_HPFL              8                                  
#define DTYPE_FLOPPY            10                          

#ifdef DKTYPENAMES
static const char *dktypenames[] = {
	"unknown",
	"SMD",
	"MSCP",
	"old DEC",
	"SCSI",
	"ESDI",
	"ST506",
	"HP-IB",
	"HP-FL",
	"type 9",
	"floppy",
	NULL
};
#define DKMAXTYPES      (sizeof(dktypenames) / sizeof(dktypenames[0]) - 1)
#endif

  
                               
                                              
                             
   
#define FS_UNUSED       0                           
#define FS_SWAP         1                         
#define FS_V6           2                                  
#define FS_V7           3                                    
#define FS_SYSV         4                             
#define FS_V71K         5                                                 
#define FS_V8           6                                              
#define FS_BSDFFS       7                                            
#define FS_MSDOS        8                                      
#define FS_BSDLFS       9                                                      
#define FS_OTHER        10                                                   
#define FS_HPFS         11                                                     
#define FS_ISO9660      12                                             
#define FS_BOOT         13                                                
#define FS_ADOS         14                                             
#define FS_HFS          15                                 

#ifdef  DKTYPENAMES
static const char *fstypenames[] = {
	"unused",
	"swap",
	"Version 6",
	"Version 7",
	"System V",
	"4.1BSD",
	"Eighth Edition",
	"4.2BSD",
	"MSDOS",
	"4.4LFS",
	"unknown",
	"HPFS",
	"ISO9660",
	"boot",
	"ADOS",
	"HFS",
	NULL
};
#define FSMAXTYPES      (sizeof(fstypenames) / sizeof(fstypenames[0]) - 1)
#endif

  
                                  
   
#define         D_REMOVABLE     0x01                                 
#define         D_ECC           0x02                              
#define         D_BADSECT       0x04                                           
#define         D_RAMDISK       0x08                               
#define         D_CHAIN         0x10                                            

  
                      
   
#define d_smdflags      d_drivedata[0]
#define         D_SSE           0x1                                          
#define d_mindist       d_drivedata[1]
#define d_maxdist       d_drivedata[2]
#define d_sdist         d_drivedata[3]

  
                        
   
#define d_precompcyl    d_drivedata[0]
#define d_gap3          d_drivedata[1]                                         

  
                       
   
#define d_blind         d_drivedata[0]

#ifndef LOCORE
  
                                                                       
                                                                   
                                    
   
struct format_op {
	char    *df_buf;
	int      df_count;                                
	daddr_t  df_startblk;
	int      df_reg[8];                         
};

  
                                                                      
             
   
struct partinfo {
	struct disklabel *disklab;
	struct partition *part;
};

  
                        
   
                                                      
#define DIOCGDINFO      _IOR('d', 101, struct disklabel)         
#define DIOCSDINFO      _IOW('d', 102, struct disklabel)         
#define DIOCWDINFO      _IOW('d', 103, struct disklabel)                      
#define DIOCGPART       _IOW('d', 104, struct partinfo)                    

                                        
#define DIOCRFORMAT     _IOWR('d', 105, struct format_op)
#define DIOCWFORMAT     _IOWR('d', 106, struct format_op)

#define DIOCSSTEP       _IOW('d', 107, int)                        
#define DIOCSRETRIES    _IOW('d', 108, int)                           
#define DIOCWLABEL      _IOW('d', 109, int)                                 

#define DIOCSBAD        _IOW('d', 110, struct dkbad)                          

#endif             

#if !defined(KERNEL) && !defined(LOCORE)

#include <sys/cdefs.h>

__BEGIN_DECLS
struct disklabel *getdiskbyname(const char *);
__END_DECLS

#endif

#endif                           

#endif                          
