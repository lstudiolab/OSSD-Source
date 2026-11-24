/*
 * Copyright (c) 2000-2013 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1983, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)fcntl.h	8.3 (Berkeley) 1/21/94
 */


#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

  
                                                        
                                                     
                              
   
#include <sys/_types.h>
#include <sys/cdefs.h>
#ifndef KERNEL
#include <Availability.h>
#endif

                                                                               
#include <sys/_types/_size_t.h>
#include <sys/_types/_mode_t.h>
#include <sys/_types/_off_t.h>
#include <sys/_types/_pid_t.h>

  
                                                          
                                                                        
                                                                       
                                                                 
                                                                      
   
                     
#define O_RDONLY        0x0000                                     
#define O_WRONLY        0x0001                                     
#define O_RDWR          0x0002                                            
#define O_ACCMODE       0x0003                                    

  
                                                                      
                                                    
  
      
                                                                          
                                                               
   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FREAD           0x00000001
#define FWRITE          0x00000002
#endif
#define O_NONBLOCK      0x00000004                    
#define O_APPEND        0x00000008                           

#include <sys/_types/_o_sync.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_SHLOCK        0x00000010                                      
#define O_EXLOCK        0x00000020                                         
#define O_ASYNC         0x00000040                                       
#define O_FSYNC         O_SYNC                                                
#define O_NOFOLLOW      0x00000100                                 
#endif                                             
#define O_CREAT         0x00000200                                 
#define O_TRUNC         0x00000400                                   
#define O_EXCL          0x00000800                                   
#ifdef KERNEL
#define FMARK           0x00001000                            
#define FDEFER          0x00002000                                  
#define FWASLOCKED      0x00004000                                                 
#define FHASLOCK        FWASLOCKED                                       
#endif

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_EVTONLY       0x00008000                                                             
#endif

#ifdef KERNEL
#define FWASWRITTEN     0x00010000                                  
#endif

#define O_NOCTTY        0x00020000                                             

#ifdef KERNEL
#define FNOCACHE        0x00040000                               
#define FNORDAHEAD      0x00080000                               
#endif

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_DIRECTORY     0x00100000
#define O_SYMLINK       0x00200000                                   
#endif

                                                                      
#include <sys/_types/_o_dsync.h>

#ifdef KERNEL
#define FNODIRECT       0x00800000                                
#endif

#if __DARWIN_C_LEVEL >= 200809L
#define O_CLOEXEC       0x01000000                                     
#endif

#ifdef KERNEL
#define FENCRYPTED      0x02000000
#define FSINGLE_WRITER  0x04000000                                     
#define O_CLOFORK       0x08000000                                     
#define FUNENCRYPTED    0x10000000
#endif

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define O_NOFOLLOW_ANY  0x20000000                                       
#endif

#ifdef KERNEL
                                                  
#endif

#if __DARWIN_C_LEVEL >= 200809L
  
                                                     
   
#define AT_FDCWD        -2

  
                             
   
#define AT_EACCESS              0x0010                                         
#define AT_SYMLINK_NOFOLLOW     0x0020                                                
#define AT_SYMLINK_FOLLOW       0x0040                                
#define AT_REMOVEDIR            0x0080                                
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#ifdef PRIVATE
#define AT_REMOVEDIR_DATALESS   0x0100                                                               
#endif
#define AT_REALDEV              0x0200                                                           
#define AT_FDONLY               0x0400                                                          
#endif
#endif

                           
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_DP_GETRAWENCRYPTED    0x0001
#define O_DP_GETRAWUNENCRYPTED  0x0002
#endif


#ifdef KERNEL
                                                                               
#define FFLAGS(oflags)  ((oflags) + 1)
#define OFLAGS(fflags)  ((fflags) - 1)

                             
#define FMASK           (FREAD|FWRITE|FAPPEND|FASYNC|FFSYNC|FFDSYNC|FNONBLOCK)
                                          
#define FCNTLFLAGS      (FAPPEND|FASYNC|FFSYNC|FFDSYNC|FNONBLOCK)
#endif

  
                                                                          
                                                                     
                                            
   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FAPPEND         O_APPEND                           
#define FASYNC          O_ASYNC                            
#define FFSYNC          O_FSYNC                     
#define FFDSYNC         O_DSYNC                     
#define FNONBLOCK       O_NONBLOCK                  
#define FNDELAY         O_NONBLOCK                  
#define O_NDELAY        O_NONBLOCK                  
#endif

  
                             
   

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define CPF_OVERWRITE    0x0001
#define CPF_IGNORE_MODE  0x0002
#define CPF_MASK (CPF_OVERWRITE|CPF_IGNORE_MODE)
#endif

  
                              
   

                    
#define F_DUPFD         0                                              
#define F_GETFD         1                                              
#define F_SETFD         2                                              
#define F_GETFL         3                                          
#define F_SETFL         4                                          
#define F_GETOWN        5                                               
#define F_SETOWN        6                                               
#define F_GETLK         7                                                   
#define F_SETLK         8                                                   
#define F_SETLKW        9                                             
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define F_SETLKWTIMEOUT 10                                                               
#endif                                          
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define F_FLUSH_DATA    40
#define F_CHKCLEAN      41                                            
#define F_PREALLOCATE   42                                       
#define F_SETSIZE       43                                                                      
#define F_RDADVISE      44                                                                     
#define F_RDAHEAD       45                                                      
  
                                                        
   
#define F_NOCACHE       48                                                        
#define F_LOG2PHYS      49                                                
#define F_GETPATH       50                                                  
#define F_FULLFSYNC     51                                                               
#define F_PATHPKG_CHECK 52                                                              
#define F_FREEZE_FS     53                                              
#define F_THAW_FS       54                                            
#define F_GLOBAL_NOCACHE 55                                                                    

#ifdef PRIVATE
#define F_OPENFROM      56                                                                   
#define F_UNLINKFROM    57                                                                   
#define F_CHECK_OPENEVT 58                                                                                               
#endif              

#define F_ADDSIGS       59                                           

#ifdef PRIVATE
                                
#define F_MARKDEPENDENCY 60                                                                                  
#endif

#define F_ADDFILESIGS   61                                                                               

#define F_NODIRECT      62                                                                                                 
                                                                                                                

#define F_GETPROTECTIONCLASS    63                                                                               
#define F_SETPROTECTIONCLASS    64                                                                               

#define F_LOG2PHYS_EXT  65                                                          

#define F_GETLKPID              66                                                               

                                      

#ifdef PRIVATE
#define F_SETSTATICCONTENT              68                
                                                                                                                    
                                                                                                                          
                                                    
#define F_MOVEDATAEXTENTS       69                                                                
#endif

#define F_SETBACKINGSTORE       70                                                                           
#define F_GETPATH_MTMINFO       71                                                                                    

#define F_GETCODEDIR            72                                                                             

#define F_SETNOSIGPIPE          73                                         
#define F_GETNOSIGPIPE          74                                         

#define F_TRANSCODEKEY          75                                                                 

#define F_SINGLE_WRITER         76                                                                                     
                                                                                                          

#define F_GETPROTECTIONLEVEL    77                                                                 

#define F_FINDSIGS              78                                                                       

#ifdef PRIVATE
#define F_GETDEFAULTPROTLEVEL   79                                                          
#define F_MAKECOMPRESSED                80                                                           
#define F_SET_GREEDY_MODE               81        
                                                                                                            
                                                                                                            
                                                                                                                      
                                            

#define F_SETIOTYPE             82    
                                                                                               
                                                                         
                                
#endif

#define F_ADDFILESIGS_FOR_DYLD_SIM 83                                                                                                 

#ifdef PRIVATE
#define F_RECYCLE                       84                                                        
#endif

#define F_BARRIERFSYNC          85                                          

#ifdef PRIVATE
#define F_OFD_SETLK             90                                                         
#define F_OFD_SETLKW            91                                                             
#define F_OFD_GETLK             92                            

#define F_OFD_SETLKWTIMEOUT     93                                                   
#define F_OFD_GETLKPID          94                                          

#define F_SETCONFINED           95                                    
#define F_GETCONFINED           96                           
#endif

#define F_ADDFILESIGS_RETURN    97                                                                                   
#define F_CHECK_LV              98                                                                                                     

#define F_PUNCHHOLE     99                                                  

#define F_TRIM_ACTIVE_FILE      100                              

#define F_SPECULATIVE_READ     101                                                                           

#define F_GETPATH_NOFIRMLINK       102                                                                    

#define F_ADDFILESIGS_INFO      103                                                           
#define F_ADDFILESUPPL          104                                                                                  
#define F_GETSIGSINFO           105                                                                         

                                                              
#define FCNTL_FS_SPECIFIC_BASE  0x00010000

#endif                                             

#if __DARWIN_C_LEVEL >= 200809L
#define F_DUPFD_CLOEXEC         67                                        
#endif

                                              
#define FD_CLOEXEC      1                                       
#if PRIVATE
#define FD_CLOFORK      2                                       
#endif

                                                       
#define F_RDLCK         1                                        
#define F_UNLCK         2                           
#define F_WRLCK         3                                            
#ifdef KERNEL
#define F_WAIT          0x010                                           
#define F_FLOCK         0x020                                                
#define F_POSIX         0x040                                             
#define F_PROV          0x080                                               
#define F_WAKE1_SAFE    0x100                                                 
#define F_ABORT         0x200                                                    
#define F_OFD_LOCK      0x400                                             
#endif

#if PRIVATE
  
                                                                          
                                                       
   
#define F_IOTYPE_ISOCHRONOUS 0x0001
#endif

  
                                                                   
                
   
#include <sys/_types/_seek_set.h>

  
                                                                      
                                                
   
#include <sys/_types/_s_ifmt.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
                                    

#define F_ALLOCATECONTIG  0x00000002                                   
#define F_ALLOCATEALL     0x00000004                                                         

                                                    

#define F_PEOFPOSMODE 3                                                                     
                                                                                       
#define F_VOLPOSMODE    4                                                    
#endif                                             

  
                                            
                                       
   
struct flock {
	off_t   l_start;                             
	off_t   l_len;                                               
	pid_t   l_pid;                          
	short   l_type;                                          
	short   l_whence;                            
};

#include <sys/_types/_timespec.h>

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
  
                                                
                                                           
   
struct flocktimeout {
	struct flock    fl;                                                
	struct timespec timeout;                                         
};
#endif                                          

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                 
                                       
   

#ifdef KERNEL
#pragma pack(4)                                          
#endif             

struct radvisory {
	off_t   ra_offset;
	int     ra_count;
};

#ifdef KERNEL
#pragma pack()
#endif             

  
                                       
                                                                            
                                                                             
                                                                      
   
#define USER_FSIGNATURES_CDHASH_LEN 20
typedef struct fsignatures {
	off_t           fs_file_start;
	void            *fs_blob_start;
	size_t          fs_blob_size;

	                                                                                  
	                                                                        
	size_t          fs_fsignatures_size;                                                 
	char            fs_cdhash[USER_FSIGNATURES_CDHASH_LEN];                      
	int             fs_hash_type;                                         
} fsignatures_t;
#ifdef KERNEL
                                             
                                                 
                                          
   

typedef struct user32_fsignatures {
	off_t           fs_file_start;
	user32_addr_t   fs_blob_start;
	user32_size_t   fs_blob_size;
} user32_fsignatures_t;

typedef struct user_fsignatures {
	off_t           fs_file_start;                                          
	user_addr_t     fs_blob_start;                                          
	                                                                        
	                                                                        
	user_size_t     fs_blob_size;                                           

	                                                                     
	                                                                        
	user_size_t     fs_fsignatures_size;                                                 
	char            fs_cdhash[USER_FSIGNATURES_CDHASH_LEN];                      
	int             fs_hash_type;                                        
} user_fsignatures_t;
#endif             

typedef struct fsupplement {
	off_t           fs_file_start;                                            
	off_t           fs_blob_start;                                            
	size_t          fs_blob_size;                                             
	int             fs_orig_fd;                                               
} fsupplement_t;

#ifdef KERNEL
                               
                                            
                                           
   

typedef struct user_fsupplement {
	off_t           fs_file_start;                                            
	off_t           fs_blob_start;                                            
	size_t          fs_blob_size;                                             
	int             fs_orig_fd;                                               
} user_fsupplement_t;
#endif             


  
                                                              
                                                                
                                                                         
  
                                                                   
                                                                    
                                                                    
                                             
   
typedef struct fchecklv {
	off_t           lv_file_start;
	size_t          lv_error_message_size;
	void            *lv_error_message;
} fchecklv_t;

#ifdef KERNEL
                                          
                                                 
                                          
   

typedef struct user32_fchecklv {
	user32_off_t    lv_file_start;
	user32_size_t   lv_error_message_size;
	user32_addr_t   lv_error_message;
} user32_fchecklv_t;

typedef struct user_fchecklv {
	off_t           lv_file_start;
	user_size_t     lv_error_message_size;
	user_addr_t     lv_error_message;
} user_fchecklv_t;

#endif             

                                                             
                                                                                    
                                                                                           
   
#define GETSIGSINFO_PLATFORM_BINARY 1

                                                  
typedef struct fgetsigsinfo {
	off_t fg_file_start;                                                                           
	int   fg_info_request;                                            
	int   fg_sig_is_platform;                                                              
} fgetsigsinfo_t;


                                  
#define LOCK_SH         0x01                                  
#define LOCK_EX         0x02                                     
#define LOCK_NB         0x04                                          
#define LOCK_UN         0x08                             

                                                 

typedef struct fstore {
	unsigned int fst_flags;                     
	int     fst_posmode;                                           
	off_t   fst_offset;                                  
	off_t   fst_length;                                 
	off_t   fst_bytesalloc;                                     
} fstore_t;

                                      
typedef struct fpunchhole {
	unsigned int fp_flags;             
	unsigned int reserved;                                     
	off_t fp_offset;                              
	off_t fp_length;                             
} fpunchhole_t;

                                                    
typedef struct ftrimactivefile {
	off_t fta_offset;                               
	off_t fta_length;                             
} ftrimactivefile_t;

                                            
typedef struct fspecread {
	unsigned int fsr_flags;                      
	unsigned int reserved;                                     
	off_t fsr_offset;                                     
	off_t fsr_length;                                    
} fspecread_t;

                                                                                

typedef struct fbootstraptransfer {
	off_t fbt_offset;                                           
	size_t fbt_length;                                         
	void *fbt_buffer;                                          
} fbootstraptransfer_t;

#ifdef KERNEL
                                                    
                                                 
                                                 
   

typedef struct user32_fbootstraptransfer {
	off_t fbt_offset;                                           
	user32_size_t fbt_length;                                         
	user32_addr_t fbt_buffer;                                          
} user32_fbootstraptransfer_t;

typedef struct user_fbootstraptransfer {
	off_t fbt_offset;                                           
	user_size_t fbt_length;                                              
	user_addr_t fbt_buffer;                                            
} user_fbootstraptransfer_t;

#endif          

  
                                                         
                                                              
                                                        
                                                  
  
                                                         
                                                            
                                                                
                                                               
                                                             
                                                                    
                                                                 
                                                                 
                                           
  
                                                                  
                                                             
                                                                   
                          
   
#pragma pack(4)

struct log2phys {
	unsigned int    l2p_flags;                          
	off_t           l2p_contigbytes;                                    
	                                                                                         
	                                                                                                       
	off_t           l2p_devoffset;                                               
	                                                                           
	                                                                             
};

#pragma pack()

#define O_POPUP    0x80000000                                      
#define O_ALERT    0x20000000                                  

#ifdef PRIVATE
  
                                    
   
struct fopenfrom {
	unsigned int    o_flags;                             
	mode_t          o_mode;                              
	char *          o_pathname;                            
};

#ifdef KERNEL
  
                                              
                                                 
  
                                                
   
struct user32_fopenfrom {
	unsigned int    o_flags;
	mode_t          o_mode;
	user32_addr_t   o_pathname;
};

struct user_fopenfrom {
	unsigned int    o_flags;
	mode_t          o_mode;
	user_addr_t     o_pathname;
};
#endif             

#endif              

#endif                                             

#ifndef KERNEL

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

#include <sys/_types/_filesec_t.h>

typedef enum {
	FILESEC_OWNER = 1,
	FILESEC_GROUP = 2,
	FILESEC_UUID = 3,
	FILESEC_MODE = 4,
	FILESEC_ACL = 5,
	FILESEC_GRPUUID = 6,

                                                 
	FILESEC_ACL_RAW = 100,
	FILESEC_ACL_ALLOCSIZE = 101
} filesec_property_t;

                                 
#define FILESEC_GUID FILESEC_UUID
#endif                                             

__BEGIN_DECLS
int     open(const char *, int, ...) __DARWIN_ALIAS_C(open);
#if __DARWIN_C_LEVEL >= 200809L
int     openat(int, const char *, int, ...) __DARWIN_NOCANCEL(openat) __OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0);
#endif
int     creat(const char *, mode_t) __DARWIN_ALIAS_C(creat);
int     fcntl(int, int, ...) __DARWIN_ALIAS_C(fcntl);
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

#ifdef PRIVATE
  
                                                                
                                           
                            
     
                                   
   
#ifndef _FILEPORT_T
#define _FILEPORT_T
typedef __darwin_mach_port_t fileport_t;
#define FILEPORT_NULL ((fileport_t)0)
#endif                  

int     fileport_makeport(int, fileport_t*);
int     fileport_makefd(fileport_t);
#endif              
int     openx_np(const char *, int, filesec_t);
  
                                        
                                                                                         
   
int open_dprotected_np( const char *, int, int, int, ...);
int     flock(int, int);
filesec_t filesec_init(void);
filesec_t filesec_dup(filesec_t);
void    filesec_free(filesec_t);
int     filesec_get_property(filesec_t, filesec_property_t, void *);
int     filesec_query_property(filesec_t, filesec_property_t, int *);
int     filesec_set_property(filesec_t, filesec_property_t, const void *);
int     filesec_unset_property(filesec_t, filesec_property_t) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
#define _FILESEC_UNSET_PROPERTY ((void *)0)
#define _FILESEC_REMOVE_ACL     ((void *)1)
#endif                                             
__END_DECLS
#endif

#endif                     
