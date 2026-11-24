/*
 * Copyright (c) 2000-2018 Apple Computer, Inc. All rights reserved.
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

/*
 * attr.h - attribute data structures and interfaces
 *
 * Copyright (c) 1998, Apple Computer, Inc.  All Rights Reserved.
 */

#ifndef _SYS_ATTR_H_
#define _SYS_ATTR_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_UNSTABLE
#include <sys/types.h>
#include <sys/ucred.h>
#include <sys/time.h>
#include <sys/cdefs.h>

#define FSOPT_NOFOLLOW          0x00000001
#define FSOPT_NOINMEMUPDATE     0x00000002
#define FSOPT_REPORT_FULLSIZE   0x00000004
                                                                             
#define FSOPT_PACK_INVAL_ATTRS  0x00000008

#ifdef PRIVATE
#define FSOPT_EXCHANGE_DATA_ONLY 0x0000010
#endif

#define FSOPT_ATTR_CMN_EXTENDED 0x00000020
#ifdef PRIVATE
#define FSOPT_LIST_SNAPSHOT     0x00000040
#ifndef FSOPT_NOFIRMLINKPATH                              
#define FSOPT_NOFIRMLINKPATH     0x00000080
#endif                           
#define FSOPT_FOLLOW_FIRMLINK    0x00000100
#endif              
#define FSOPT_RETURN_REALDEV     0x00000200
#ifdef PRIVATE
#ifndef FSOPT_ISREALFSID                               
#define FSOPT_ISREALFSID         FSOPT_RETURN_REALDEV
#endif
#endif              

                                                            
                                                                         
                                                                          
             
   
#define SEARCHFS_MAX_SEARCHPARMS  4096

typedef u_int32_t text_encoding_t;

typedef u_int32_t fsobj_type_t;

typedef u_int32_t fsobj_tag_t;

typedef u_int32_t fsfile_type_t;

typedef u_int32_t fsvolid_t;

#include <sys/_types/_fsobj_id_t.h>                          

typedef u_int32_t attrgroup_t;

struct attrlist {
	u_short bitmapcount;                                                                        
	u_int16_t reserved;                                                         
	attrgroup_t commonattr;                                             
	attrgroup_t volattr;                                                
	attrgroup_t dirattr;                                                   
	attrgroup_t fileattr;                                             
	attrgroup_t forkattr;                                             
};
#define ATTR_BIT_MAP_COUNT 5

typedef struct attribute_set {
	attrgroup_t commonattr;                                             
	attrgroup_t volattr;                                                
	attrgroup_t dirattr;                                                   
	attrgroup_t fileattr;                                             
	attrgroup_t forkattr;                                             
} attribute_set_t;

typedef struct attrreference {
	int32_t     attr_dataoffset;
	u_int32_t   attr_length;
} attrreference_t;

                                                                                               

struct diskextent {
	u_int32_t                                       startblock;                                                        
	u_int32_t                                       blockcount;                                                             
};

typedef struct diskextent extentrecord[8];

typedef u_int32_t vol_capabilities_set_t[4];

#define VOL_CAPABILITIES_FORMAT 0
#define VOL_CAPABILITIES_INTERFACES 1
#define VOL_CAPABILITIES_RESERVED1 2
#define VOL_CAPABILITIES_RESERVED2 3

typedef struct vol_capabilities_attr {
	vol_capabilities_set_t capabilities;
	vol_capabilities_set_t valid;
} vol_capabilities_attr_t;

  
                                              
   
#define ATTR_MAX_BUFFER         8192

  
                                                                       
                                                                   
                                                                     
                                                                    
                                  
  
                                                                    
                                                                    
                 
  
                                                                   
                                      
  
                                                                     
                                                                 
                                                                    
                                                            
  
                                                                      
                                                                      
                                                               
  
                                                                  
                                                                 
                                         
  
                                                                        
                                                                     
                                                               
                                                              
  
                                                                      
                                                                       
                                                                     
                                                                   
                                                                     
                                           
  
                                                                     
                                                                    
                                                                   
                                                                 
                  
  
                                                                  
                                                                   
                                                               
                                                                
                                                                          
  
                                                                      
                                                                    
                                                                  
                                                                          
                                                                       
                                       
  
                                                                          
                                                                     
                                                                   
  
                                                                     
                                                                        
                                     
  
                                                                        
                                                                      
                                                                       
                                   
  
                                                                       
                                                                     
                                                                   
                                                                      
                                                                       
                                                                   
  
                                                                     
                                                                           
                                                                            
                                                                             
  
                                                                             
                                                   
  
                                                                          
                                                                            
                                                                               
                                                                        
                                                           
  
                                                                     
              
  
                                                                      
                                                                          
  
                                                                          
                                                                            
  
                                                                        
                                 
  
                                                                            
               
  
                                                                             
                                                                            
                                                                        
                                                                     
  
                                                                          
                                                                              
                                                                          
                        
  
                                                                         
                                                                        
                              
   
#define VOL_CAP_FMT_PERSISTENTOBJECTIDS         0x00000001
#define VOL_CAP_FMT_SYMBOLICLINKS               0x00000002
#define VOL_CAP_FMT_HARDLINKS                   0x00000004
#define VOL_CAP_FMT_JOURNAL                     0x00000008
#define VOL_CAP_FMT_JOURNAL_ACTIVE              0x00000010
#define VOL_CAP_FMT_NO_ROOT_TIMES               0x00000020
#define VOL_CAP_FMT_SPARSE_FILES                0x00000040
#define VOL_CAP_FMT_ZERO_RUNS                   0x00000080
#define VOL_CAP_FMT_CASE_SENSITIVE              0x00000100
#define VOL_CAP_FMT_CASE_PRESERVING             0x00000200
#define VOL_CAP_FMT_FAST_STATFS                 0x00000400
#define VOL_CAP_FMT_2TB_FILESIZE                0x00000800
#define VOL_CAP_FMT_OPENDENYMODES               0x00001000
#define VOL_CAP_FMT_HIDDEN_FILES                0x00002000
#define VOL_CAP_FMT_PATH_FROM_ID                0x00004000
#define VOL_CAP_FMT_NO_VOLUME_SIZES             0x00008000
#define VOL_CAP_FMT_DECMPFS_COMPRESSION         0x00010000
#define VOL_CAP_FMT_64BIT_OBJECT_IDS            0x00020000
#define VOL_CAP_FMT_DIR_HARDLINKS               0x00040000
#define VOL_CAP_FMT_DOCUMENT_ID                 0x00080000
#define VOL_CAP_FMT_WRITE_GENERATION_COUNT      0x00100000
#define VOL_CAP_FMT_NO_IMMUTABLE_FILES          0x00200000
#define VOL_CAP_FMT_NO_PERMISSIONS              0x00400000
#define VOL_CAP_FMT_SHARED_SPACE                0x00800000
#define VOL_CAP_FMT_VOL_GROUPS                  0x01000000
#define VOL_CAP_FMT_SEALED                      0x02000000

  
                                                            
                                                              
  
                                                            
                                                                 
                                                                
                                                            
                                                           
                                                            
                                                           
                           
  
                                                                   
                  
  
                                                               
                                                                
  
                                                                
                                                              
  
                                                            
                                                                   
                              
  
                                                            
                                                               
                                      
  
                                                              
                                                                    
                                                                         
  
                                                                   
                                                                
  
                                                                         
                                                                         
                             
  
                                                                 
                            
  
                                                             
                                                               
                     
  
                                                               
                                             
  
                                                             
                             
  
                                                          
                        
  
                                                           
  
                                                                 
  
                                                                  
                       
  
                                                            
                              
  
                                                                    
                                     
   
#define VOL_CAP_INT_SEARCHFS                    0x00000001
#define VOL_CAP_INT_ATTRLIST                    0x00000002
#define VOL_CAP_INT_NFSEXPORT                   0x00000004
#define VOL_CAP_INT_READDIRATTR                 0x00000008
#define VOL_CAP_INT_EXCHANGEDATA                0x00000010
#define VOL_CAP_INT_COPYFILE                    0x00000020
#define VOL_CAP_INT_ALLOCATE                    0x00000040
#define VOL_CAP_INT_VOL_RENAME                  0x00000080
#define VOL_CAP_INT_ADVLOCK                     0x00000100
#define VOL_CAP_INT_FLOCK                       0x00000200
#define VOL_CAP_INT_EXTENDED_SECURITY           0x00000400
#define VOL_CAP_INT_USERACCESS                  0x00000800
#define VOL_CAP_INT_MANLOCK                     0x00001000
#define VOL_CAP_INT_NAMEDSTREAMS                0x00002000
#define VOL_CAP_INT_EXTENDED_ATTR               0x00004000
#ifdef PRIVATE
                                                            
#define VOL_CAP_INT_REMOTE_EVENT                0x00008000
#endif              
#define VOL_CAP_INT_CLONE                       0x00010000
#define VOL_CAP_INT_SNAPSHOT                    0x00020000
#define VOL_CAP_INT_RENAME_SWAP                 0x00040000
#define VOL_CAP_INT_RENAME_EXCL                 0x00080000
#define VOL_CAP_INT_RENAME_OPENFAIL             0x00100000

typedef struct vol_attributes_attr {
	attribute_set_t validattr;
	attribute_set_t nativeattr;
} vol_attributes_attr_t;

#define ATTR_CMN_NAME                           0x00000001
#define ATTR_CMN_DEVID                          0x00000002
#define ATTR_CMN_FSID                           0x00000004
#define ATTR_CMN_OBJTYPE                        0x00000008
#define ATTR_CMN_OBJTAG                         0x00000010
#define ATTR_CMN_OBJID                          0x00000020
#define ATTR_CMN_OBJPERMANENTID                 0x00000040
#define ATTR_CMN_PAROBJID                       0x00000080
#define ATTR_CMN_SCRIPT                         0x00000100
#define ATTR_CMN_CRTIME                         0x00000200
#define ATTR_CMN_MODTIME                        0x00000400
#define ATTR_CMN_CHGTIME                        0x00000800
#define ATTR_CMN_ACCTIME                        0x00001000
#define ATTR_CMN_BKUPTIME                       0x00002000
#define ATTR_CMN_FNDRINFO                       0x00004000
#define ATTR_CMN_OWNERID                        0x00008000
#define ATTR_CMN_GRPID                          0x00010000
#define ATTR_CMN_ACCESSMASK                     0x00020000
#define ATTR_CMN_FLAGS                          0x00040000

                                       
                                                      
                                                     
                                                 
                                            
                                              
                                                             
                                                                  

#define ATTR_CMN_GEN_COUNT                      0x00080000
#define ATTR_CMN_DOCUMENT_ID                    0x00100000

#define ATTR_CMN_USERACCESS                     0x00200000
#define ATTR_CMN_EXTENDED_SECURITY              0x00400000
#define ATTR_CMN_UUID                           0x00800000
#define ATTR_CMN_GRPUUID                        0x01000000
#define ATTR_CMN_FILEID                         0x02000000
#define ATTR_CMN_PARENTID                       0x04000000
#define ATTR_CMN_FULLPATH                       0x08000000
#define ATTR_CMN_ADDEDTIME                      0x10000000
#define ATTR_CMN_ERROR                          0x20000000
#define ATTR_CMN_DATA_PROTECT_FLAGS             0x40000000

  
                                                                
                                                                             
   
#define ATTR_CMN_RETURNED_ATTRS                 0x80000000

#define ATTR_CMN_VALIDMASK                      0xFFFFFFFF
  
                                                            
                  
                  
                   
                   
  
                   
                    
                    
                   
  
                 
                      
                 
  
                             
                
  
                   
  
                              
   
#define ATTR_CMN_SETMASK                        0x51C7FF00
#define ATTR_CMN_VOLSETMASK                     0x00006700

#define ATTR_VOL_FSTYPE                         0x00000001
#define ATTR_VOL_SIGNATURE                      0x00000002
#define ATTR_VOL_SIZE                           0x00000004
#define ATTR_VOL_SPACEFREE                      0x00000008
#define ATTR_VOL_SPACEAVAIL                     0x00000010
#define ATTR_VOL_MINALLOCATION                  0x00000020
#define ATTR_VOL_ALLOCATIONCLUMP                0x00000040
#define ATTR_VOL_IOBLOCKSIZE                    0x00000080
#define ATTR_VOL_OBJCOUNT                       0x00000100
#define ATTR_VOL_FILECOUNT                      0x00000200
#define ATTR_VOL_DIRCOUNT                       0x00000400
#define ATTR_VOL_MAXOBJCOUNT                    0x00000800
#define ATTR_VOL_MOUNTPOINT                     0x00001000
#define ATTR_VOL_NAME                           0x00002000
#define ATTR_VOL_MOUNTFLAGS                     0x00004000
#define ATTR_VOL_MOUNTEDDEVICE                  0x00008000
#define ATTR_VOL_ENCODINGSUSED                  0x00010000
#define ATTR_VOL_CAPABILITIES                   0x00020000
#define ATTR_VOL_UUID                           0x00040000
#define ATTR_VOL_QUOTA_SIZE                     0x10000000
#define ATTR_VOL_RESERVED_SIZE          0x20000000
#define ATTR_VOL_ATTRIBUTES                     0x40000000
#define ATTR_VOL_INFO                           0x80000000

#define ATTR_VOL_VALIDMASK                      0xF007FFFF

  
                                                                    
                
                
   
#define ATTR_VOL_SETMASK                        0x80002000


                                
#define ATTR_DIR_LINKCOUNT                      0x00000001
#define ATTR_DIR_ENTRYCOUNT                     0x00000002
#define ATTR_DIR_MOUNTSTATUS                    0x00000004
#define ATTR_DIR_ALLOCSIZE                      0x00000008
#define ATTR_DIR_IOBLOCKSIZE                    0x00000010
#define ATTR_DIR_DATALENGTH                     0x00000020

                                 
#define   DIR_MNTSTATUS_MNTPOINT                0x00000001
#define   DIR_MNTSTATUS_TRIGGER                 0x00000002

#define ATTR_DIR_VALIDMASK                      0x0000003f
#define ATTR_DIR_SETMASK                        0x00000000

#define ATTR_FILE_LINKCOUNT                     0x00000001
#define ATTR_FILE_TOTALSIZE                     0x00000002
#define ATTR_FILE_ALLOCSIZE                     0x00000004
#define ATTR_FILE_IOBLOCKSIZE                   0x00000008
#define ATTR_FILE_DEVTYPE                       0x00000020
#define ATTR_FILE_FORKCOUNT                     0x00000080
#define ATTR_FILE_FORKLIST                      0x00000100
#define ATTR_FILE_DATALENGTH                    0x00000200
#define ATTR_FILE_DATAALLOCSIZE                 0x00000400
#define ATTR_FILE_RSRCLENGTH                    0x00001000
#define ATTR_FILE_RSRCALLOCSIZE                 0x00002000

#define ATTR_FILE_VALIDMASK                     0x000037FF
  
                                           
                    
   
#define ATTR_FILE_SETMASK                       0x00000020

                                                                               
#define ATTR_CMNEXT_RELPATH     0x00000004
#define ATTR_CMNEXT_PRIVATESIZE 0x00000008
#define ATTR_CMNEXT_LINKID      0x00000010
#define ATTR_CMNEXT_NOFIRMLINKPATH     0x00000020
#define ATTR_CMNEXT_REALDEVID   0x00000040
#define ATTR_CMNEXT_REALFSID    0x00000080
#define ATTR_CMNEXT_CLONEID     0x00000100
#define ATTR_CMNEXT_EXT_FLAGS   0x00000200
#define ATTR_CMNEXT_RECURSIVE_GENCOUNT 0x00000400

#define ATTR_CMNEXT_VALIDMASK   0x000007fc
#define ATTR_CMNEXT_SETMASK             0x00000000

                                
#define ATTR_FORK_TOTALSIZE                     0x00000001
#define ATTR_FORK_ALLOCSIZE                     0x00000002
#define ATTR_FORK_RESERVED                      0xffffffff

#define ATTR_FORK_VALIDMASK                     0x00000003
#define ATTR_FORK_SETMASK                       0x00000000

                                          
#define ATTR_CMN_NAMEDATTRCOUNT                 0x00080000
#define ATTR_CMN_NAMEDATTRLIST                  0x00100000
#define ATTR_FILE_CLUMPSIZE                     0x00000010                    
#define ATTR_FILE_FILETYPE                      0x00000040                       
#define ATTR_FILE_DATAEXTENTS                   0x00000800                                  
#define ATTR_FILE_RSRCEXTENTS                   0x00004000                                  

                                                
#define ATTR_BULK_REQUIRED (ATTR_CMN_NAME | ATTR_CMN_RETURNED_ATTRS)

  
           
   
#define SRCHFS_START                            0x00000001
#define SRCHFS_MATCHPARTIALNAMES                0x00000002
#define SRCHFS_MATCHDIRS                        0x00000004
#define SRCHFS_MATCHFILES                       0x00000008
#define SRCHFS_SKIPLINKS                        0x00000010
#define SRCHFS_SKIPINVISIBLE                    0x00000020
#define SRCHFS_SKIPPACKAGES                     0x00000040
#define SRCHFS_SKIPINAPPROPRIATE                0x00000080

#define SRCHFS_NEGATEPARAMS                     0x80000000
#define SRCHFS_VALIDOPTIONSMASK                 0x800000FF

struct fssearchblock {
	struct attrlist         *returnattrs;
	void                            *returnbuffer;
	size_t                          returnbuffersize;
	u_long                          maxmatches;
	struct timeval          timelimit;
	void                            *searchparams1;
	size_t                          sizeofsearchparams1;
	void                            *searchparams2;
	size_t                          sizeofsearchparams2;
	struct attrlist         searchattrs;
};

#ifdef KERNEL
                                                         
                                                 
                                            
   

struct user64_fssearchblock {
	user64_addr_t         returnattrs;
	user64_addr_t         returnbuffer;
	user64_size_t         returnbuffersize;
	user64_ulong_t        maxmatches;
	struct user64_timeval   timelimit;
	user64_addr_t         searchparams1;
	user64_size_t         sizeofsearchparams1;
	user64_addr_t         searchparams2;
	user64_size_t         sizeofsearchparams2;
	struct attrlist         searchattrs;
};

struct user32_fssearchblock {
	user32_addr_t         returnattrs;
	user32_addr_t         returnbuffer;
	user32_size_t         returnbuffersize;
	user32_ulong_t        maxmatches;
	struct user32_timeval   timelimit;
	user32_addr_t         searchparams1;
	user32_size_t         sizeofsearchparams1;
	user32_addr_t         searchparams2;
	user32_size_t         sizeofsearchparams2;
	struct attrlist         searchattrs;
};

#endif             

struct searchstate {
	uint32_t                        ss_union_flags;                            
	uint32_t                        ss_union_layer;                   
	u_char                          ss_fsstate[548];                     
} __attribute__((packed));

#define FST_EOF (-1)                                                    

#endif                           
#endif                    
