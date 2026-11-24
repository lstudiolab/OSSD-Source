/*
 * Copyright (c) 2000-2014 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)stat.h	8.9 (Berkeley) 8/17/94
 */


#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#include <sys/_types.h>
#include <sys/cdefs.h>
#ifdef KERNEL
#include <machine/types.h>
#else              
#include <Availability.h>
#endif             

                                                                          
#include <sys/_types/_timespec.h>
#ifdef KERNEL
#include <sys/_types/_user64_timespec.h>
#include <sys/_types/_user32_timespec.h>
#endif             

  
                                                                       
                                                                  
                 
   
#include <sys/_types/_blkcnt_t.h>
#include <sys/_types/_blksize_t.h>
#include <sys/_types/_dev_t.h>                                     
#include <sys/_types/_ino_t.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#include <sys/_types/_ino64_t.h>
#endif                                                             

#include <sys/_types/_mode_t.h>
#include <sys/_types/_nlink_t.h>
#include <sys/_types/_uid_t.h>
#include <sys/_types/_gid_t.h>
#include <sys/_types/_off_t.h>
#include <sys/_types/_time_t.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                                  
  
                                                                           
                                                                        
                                                                     
                     
   
struct ostat {
	__uint16_t      st_dev;                             
	ino_t           st_ino;                             
	mode_t          st_mode;                                   
	nlink_t         st_nlink;                                 
	__uint16_t      st_uid;                                          
	__uint16_t      st_gid;                                           
	__uint16_t      st_rdev;                         
	__int32_t       st_size;                                 
	struct  timespec st_atimespec;                           
	struct  timespec st_mtimespec;                                      
	struct  timespec st_ctimespec;                                       
	__int32_t       st_blksize;                                    
	__int32_t       st_blocks;                                     
	__uint32_t      st_flags;                                        
	__uint32_t      st_gen;                                     
};

#define __DARWIN_STRUCT_STAT64_TIMES \
	struct timespec st_atimespec;                                     \
	struct timespec st_mtimespec;                                                \
	struct timespec st_ctimespec;                                            \
	struct timespec st_birthtimespec;                                         

#else                                             

#define __DARWIN_STRUCT_STAT64_TIMES \
	time_t		st_atime;                                               \
	long		st_atimensec;                                     \
	time_t		st_mtime;                                                       \
	long		st_mtimensec;                                             \
	time_t		st_ctime;                                                      \
	long		st_ctimensec;                                            \
	time_t		st_birthtime;                                             \
	long		st_birthtimensec;                                       

#endif                                             

  
                                                                   
                                                              
                                                                  
                                                                   
              
  
                                                         
                                                                        
   
#define __DARWIN_STRUCT_STAT64 { \
	dev_t		st_dev;                                                          \
	mode_t		st_mode;                                                     \
	nlink_t		st_nlink;                                                \
	__darwin_ino64_t st_ino;                                               \
	uid_t		st_uid;                                                 \
	gid_t		st_gid;                                                  \
	dev_t		st_rdev;                                      \
	__DARWIN_STRUCT_STAT64_TIMES \
	off_t		st_size;                                                \
	blkcnt_t	st_blocks;                                                    \
	blksize_t	st_blksize;                                                   \
	__uint32_t	st_flags;                                                 \
	__uint32_t	st_gen;                                              \
	__int32_t	st_lspare;                                          \
	__int64_t	st_qspare[2];                                       \
}

  
                                                                       
                                 
   
#if __DARWIN_64_BIT_INO_T

struct stat __DARWIN_STRUCT_STAT64;

#else                             

struct stat {
	dev_t           st_dev;                                                 
	ino_t           st_ino;                                       
	mode_t          st_mode;                                            
	nlink_t         st_nlink;                                       
	uid_t           st_uid;                                        
	gid_t           st_gid;                                         
	dev_t           st_rdev;                             
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	struct  timespec st_atimespec;                           
	struct  timespec st_mtimespec;                                      
	struct  timespec st_ctimespec;                                  
#else
	time_t          st_atime;                                      
	long            st_atimensec;                            
	time_t          st_mtime;                                              
	long            st_mtimensec;                                    
	time_t          st_ctime;                                             
	long            st_ctimensec;                                   
#endif
	off_t           st_size;                                       
	blkcnt_t        st_blocks;                                           
	blksize_t       st_blksize;                                          
	__uint32_t      st_flags;                                        
	__uint32_t      st_gen;                                     
	__int32_t       st_lspare;                                 
	__int64_t       st_qspare[2];                              
};

#endif                            

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

#if !__DARWIN_ONLY_64_BIT_INO_T

struct stat64 __DARWIN_STRUCT_STAT64;

#endif                                  

#endif                                             


#ifdef KERNEL
#ifdef BSD_KERNEL_PRIVATE
                                                                         
                                                 
                                          
   

struct user64_stat {
	dev_t           st_dev;                                                 
	ino_t           st_ino;                                       
	mode_t          st_mode;                                            
	nlink_t         st_nlink;                                       
	uid_t           st_uid;                                        
	gid_t           st_gid;                                         
	dev_t           st_rdev;                             
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	struct  user64_timespec st_atimespec;                          
	struct  user64_timespec st_mtimespec;                                     
	struct  user64_timespec st_ctimespec;                                 
#else
	user64_time_t   st_atime;                                      
	user64_long_t   st_atimensec;                            
	user64_time_t   st_mtime;                                         
	user64_long_t   st_mtimensec;                                    
	user64_time_t   st_ctime;                                             
	user64_long_t   st_ctimensec;                                   
#endif
	off_t           st_size;                                       
	blkcnt_t        st_blocks;                                           
	blksize_t       st_blksize;                                          
	__uint32_t      st_flags;                                        
	__uint32_t      st_gen;                                     
	__int32_t       st_lspare;                                 
	__int64_t       st_qspare[2];                              
};

                                
                                          
   

struct user32_stat {
	dev_t           st_dev;                                                 
	ino_t           st_ino;                                       
	mode_t          st_mode;                                            
	nlink_t         st_nlink;                                       
	uid_t           st_uid;                                        
	gid_t           st_gid;                                         
	dev_t           st_rdev;                             
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	struct  user32_timespec st_atimespec;                          
	struct  user32_timespec st_mtimespec;                                     
	struct  user32_timespec st_ctimespec;                                 
#else
	user32_time_t   st_atime;                                      
	user32_long_t   st_atimensec;                            
	user32_time_t   st_mtime;                                         
	user32_long_t   st_mtimensec;                                    
	user32_time_t   st_ctime;                                             
	user32_long_t   st_ctimensec;                                   
#endif
	off_t           st_size;                                       
	blkcnt_t        st_blocks;                                           
	blksize_t       st_blksize;                                          
	__uint32_t      st_flags;                                        
	__uint32_t      st_gen;                                     
	__int32_t       st_lspare;                                 
	__int64_t       st_qspare[2];                              
};

extern void munge_user64_stat(struct stat *sbp, struct user64_stat *usbp);
extern void munge_user32_stat(struct stat *sbp, struct user32_stat *usbp);


struct user64_stat64 {
	dev_t           st_dev;                                                                         
	mode_t          st_mode;                                                                    
	nlink_t         st_nlink;                                                               
	ino64_t         st_ino;                                                               
	uid_t           st_uid;                                                                
	gid_t           st_gid;                                                                 
	dev_t           st_rdev;                                                     
#ifndef _POSIX_C_SOURCE
	struct user64_timespec st_atimespec;                                     
	struct user64_timespec st_mtimespec;                                                
	struct user64_timespec st_ctimespec;                                            
	struct user64_timespec st_birthtimespec;                                          
#else
	user64_time_t   st_atime;                                                              
	user64_long_t   st_atimensec;                                            
	user64_time_t   st_mtime;                                                                      
	user64_long_t   st_mtimensec;                                                    
	user64_time_t   st_ctime;                                                                     
	user64_long_t   st_ctimensec;                                                   
	user64_time_t   st_birthtime;                                                    
	user64_long_t   st_birthtimensec;                                               
#endif
	off_t           st_size;                                                               
	blkcnt_t        st_blocks;                                                                   
	blksize_t       st_blksize;                                                                  
	__uint32_t      st_flags;                                                                
	__uint32_t      st_gen;                                                             
	__uint32_t      st_lspare;                                                         
	__int64_t       st_qspare[2];                                              
};

struct user32_stat64 {
	dev_t           st_dev;                                                                         
	mode_t          st_mode;                                                                    
	nlink_t         st_nlink;                                                               
	ino64_t         st_ino;                                                               
	uid_t           st_uid;                                                                
	gid_t           st_gid;                                                                 
	dev_t           st_rdev;                                                     
#ifndef _POSIX_C_SOURCE
	struct user32_timespec st_atimespec;                                     
	struct user32_timespec st_mtimespec;                                                
	struct user32_timespec st_ctimespec;                                            
	struct user32_timespec st_birthtimespec;                                          
#else
	user32_time_t   st_atime;                                                              
	user32_long_t   st_atimensec;                                            
	user32_time_t   st_mtime;                                                                      
	user32_long_t   st_mtimensec;                                                    
	user32_time_t   st_ctime;                                                                     
	user32_long_t   st_ctimensec;                                                   
	user32_time_t   st_birthtime;                                                    
	user32_long_t   st_birthtimensec;                                               
#endif
	off_t           st_size;                                                               
	blkcnt_t        st_blocks;                                                                   
	blksize_t       st_blksize;                                                                  
	__uint32_t      st_flags;                                                                
	__uint32_t      st_gen;                                                             
	__uint32_t      st_lspare;                                                         
	__int64_t       st_qspare[2];                                              
#if defined(__x86_64__)
  
                                                                                
                                                                              
                                                                       
                                                      
   
} __attribute__((packed, aligned(4)));
#else
};
#endif

extern void munge_user64_stat64(struct stat64 *sbp, struct user64_stat64 *usbp);
extern void munge_user32_stat64(struct stat64 *sbp, struct user32_stat64 *usbp);

#endif                         

#endif             


#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#define st_birthtime st_birthtimespec.tv_sec
#endif                                             

  
                                                                              
                     
   
#include <sys/_types/_s_ifmt.h>

  
                                                                         
                                                                          
                                                                            
                                                     
   
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)                        
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)                       
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)                    
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)                         
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)                       
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)                        
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define S_ISWHT(m)      (((m) & S_IFMT) == S_IFWHT)                             
#endif

  
                                                                        
                                                                      
                                                                     
                                                                       
                                                                     
                                                                      
                                                                       
                                                     
  
                                                              
                                                              
                                                             
                             
   
#define S_TYPEISMQ(buf)         (0)                                   
#define S_TYPEISSEM(buf)        (0)                               
#define S_TYPEISSHM(buf)        (0)                                          

  
                                                                          
                                                                          
                                                                           
                                                                         
                                                                           
                                                                           
                                                     
  
                                                              
                                                              
                                                           
                             
   
#define S_TYPEISTMO(buf)        (0)                                         


#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define ACCESSPERMS     (S_IRWXU|S_IRWXG|S_IRWXO)                 
                                                                  
#define ALLPERMS        (S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
          
#define DEFFILEMODE     (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define S_BLKSIZE       512                                                     

  
                                                  
  
                                         
   
#define UF_SETTABLE     0x0000ffff                                          
#define UF_NODUMP       0x00000001                            
#define UF_IMMUTABLE    0x00000002                                   
#define UF_APPEND       0x00000004                                          
#define UF_OPAQUE       0x00000008                                          
  
                                                                    
               
   
                                    	                                        
#define UF_COMPRESSED   0x00000020                                                  

                                                                        
                                                                                
#define UF_TRACKED              0x00000040

#define UF_DATAVAULT    0x00000080                                            
                                                         

                                                         
#define UF_HIDDEN       0x00008000                                             
                                                                
  
                               
   
#define SF_SUPPORTED    0x009f0000                                             
#define SF_SETTABLE     0x3fff0000                                              
#define SF_SYNTHETIC    0xc0000000                                                    
#define SF_ARCHIVED     0x00010000                            
#define SF_IMMUTABLE    0x00020000                                   
#define SF_APPEND       0x00040000                                          
#define SF_RESTRICTED   0x00080000                                            
#define SF_NOUNLINK     0x00100000                                                          

  
                                                                 
                           
   
                                    	                    
                                      

#define SF_FIRMLINK     0x00800000                              

  
                   
  
                                                                 
                                  
   
#define SF_DATALESS     0x40000000                                  

#ifdef PRIVATE
  
                   
  
                                                                     
                                                                     
                                                            
   
#define UF_SF_PROTECTED (UF_COMPRESSED)
#endif

#ifdef KERNEL
  
                                    
   
#define OPAQUE          (UF_OPAQUE)
#define APPEND          (UF_APPEND | SF_APPEND)
#define IMMUTABLE       (UF_IMMUTABLE | SF_IMMUTABLE)
#endif
#endif

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
  
                                                                                           
   
#define EF_MAY_SHARE_BLOCKS     0x00000001                                                   
#define EF_NO_XATTRS            0x00000002                                     
#define EF_IS_SYNC_ROOT         0x00000004                                          
#define EF_IS_PURGEABLE         0x00000008                             
#define EF_IS_SPARSE            0x00000010                                               
#define EF_IS_SYNTHETIC         0x00000020                                         
#endif


#ifndef KERNEL

__BEGIN_DECLS
           
int     chmod(const char *, mode_t) __DARWIN_ALIAS(chmod);
int     fchmod(int, mode_t) __DARWIN_ALIAS(fchmod);
int     fstat(int, struct stat *) __DARWIN_INODE64(fstat);
int     lstat(const char *, struct stat *) __DARWIN_INODE64(lstat);
int     mkdir(const char *, mode_t);
int     mkfifo(const char *, mode_t);
int     stat(const char *, struct stat *) __DARWIN_INODE64(stat);
int     mknod(const char *, mode_t, dev_t);
mode_t  umask(mode_t);

#if __DARWIN_C_LEVEL >= 200809L
int     fchmodat(int, const char *, mode_t, int) __OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0);
int     fstatat(int, const char *, struct stat *, int) __DARWIN_INODE64(fstatat) __OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0);
int     mkdirat(int, const char *, mode_t) __OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0);

#define UTIME_NOW       -1
#define UTIME_OMIT      -2

int     futimens(int __fd, const struct timespec __times[2]) __API_AVAILABLE(macosx(10.13), ios(11.0), tvos(11.0), watchos(4.0));
int     utimensat(int __fd, const char *__path, const struct timespec __times[2],
    int __flag) __API_AVAILABLE(macosx(10.13), ios(11.0), tvos(11.0), watchos(4.0));
#endif

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

#include <sys/_types/_filesec_t.h>

int     chflags(const char *, __uint32_t);
int     chmodx_np(const char *, filesec_t);
int     fchflags(int, __uint32_t);
int     fchmodx_np(int, filesec_t);
int     fstatx_np(int, struct stat *, filesec_t) __DARWIN_INODE64(fstatx_np);
int     lchflags(const char *, __uint32_t) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
int     lchmod(const char *, mode_t) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
int     lstatx_np(const char *, struct stat *, filesec_t) __DARWIN_INODE64(lstatx_np);
int     mkdirx_np(const char *, filesec_t);
int     mkfifox_np(const char *, filesec_t);
int     statx_np(const char *, struct stat *, filesec_t) __DARWIN_INODE64(statx_np);
int     umaskx_np(filesec_t) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);

#if !__DARWIN_ONLY_64_BIT_INO_T
                                                                                                                             
int     fstatx64_np(int, struct stat64 *, filesec_t) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
int     lstatx64_np(const char *, struct stat64 *, filesec_t) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
int     statx64_np(const char *, struct stat64 *, filesec_t) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
int     fstat64(int, struct stat64 *) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
int     lstat64(const char *, struct stat64 *) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
int     stat64(const char *, struct stat64 *) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
#endif                                  
#endif                                             

__END_DECLS
#endif              
#endif                    
