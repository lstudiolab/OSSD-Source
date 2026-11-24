/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
 *	@(#)quota.h
 *	derived from @(#)ufs/ufs/quota.h	8.3 (Berkeley) 8/19/94
 */

#ifndef _SYS_QUOTA_H
#define _SYS_QUOTA_H

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/types.h>                
#ifdef KERNEL_PRIVATE
#include <kern/locks.h>
#endif

#include <mach/boolean.h>

#ifdef __APPLE_API_UNSTABLE
  
                                                          
                                   
  
                                                                            
                                                                             
                                                                            
                                                
   
#define MAX_IQ_TIME     (7*24*60*60)                           
#define MAX_DQ_TIME     (7*24*60*60)                           

  
                                                                          
                                                                                     
                                                                          
                                                                          
                                                 
   
#define MAXQUOTAS       2
#define USRQUOTA        0                                         
#define GRPQUOTA        1                                          

  
                                                         
   
#define INITQFNAMES { \
	"user",                        \
	"group",                       \
	"undefined", \
};
#define QUOTAFILENAME   ".quota"
#define QUOTAOPSNAME    ".quota.ops"
#define QUOTAGROUP      "operator"

  
                                                                        
                                                                         
                                                                     
   
#define SUBCMDMASK      0x00ff
#define SUBCMDSHIFT     8
#define QCMD(cmd, type) (((cmd) << SUBCMDSHIFT) | ((type) & SUBCMDMASK))

#define Q_QUOTAON       0x0100                     
#define Q_QUOTAOFF      0x0200                      
#define Q_GETQUOTA      0x0300                            
#define Q_SETQUOTA      0x0400                            
#define Q_SETUSE        0x0500                 
#define Q_SYNC          0x0600                                              
#define Q_QUOTASTAT     0x0700                               

  
                                                             
                                                           
                                                             
                                                             
                                                             
                                                           
                                                            
                                                             
                                                             
                                                              
                                                               
                                         
  
                                                                
                                                                   
   
struct dqfilehdr {
	u_int32_t dqh_magic;
	u_int32_t dqh_version;                             
	u_int32_t dqh_maxentries;                                 
	u_int32_t dqh_entrycnt;                                      
	u_int32_t dqh_flags;                                      
	u_int32_t dqh_chktime;                                        
	u_int32_t dqh_btime;                                                   
	u_int32_t dqh_itime;                                                
	char      dqh_string[16];                       
	u_int32_t dqh_spare[4];                                       
};

struct dqblk {
	u_int64_t dqb_bhardlimit;                                               
	u_int64_t dqb_bsoftlimit;                                          
	u_int64_t dqb_curbytes;                                 
	u_int32_t dqb_ihardlimit;                                           
	u_int32_t dqb_isoftlimit;                                  
	u_int32_t dqb_curinodes;                                        
	u_int32_t dqb_btime;                                                   
	u_int32_t dqb_itime;                                                
	u_int32_t dqb_id;                                                     
	u_int32_t dqb_spare[4];                                       
};

#ifdef KERNEL_PRIVATE
#include <machine/types.h>                       
                                                                     
                                       
                                           
   

struct user_dqblk {
	u_int64_t dqb_bhardlimit;                                               
	u_int64_t dqb_bsoftlimit;                                          
	u_int64_t dqb_curbytes;                                 
	u_int32_t dqb_ihardlimit;                                           
	u_int32_t dqb_isoftlimit;                                  
	u_int32_t dqb_curinodes;                                        
	u_int32_t dqb_btime;                                                   
	u_int32_t dqb_itime;                                                
	u_int32_t dqb_id;                                                     
	u_int32_t dqb_spare[4];                                       
};
#endif                      

#define INITQMAGICS { \
	0xff31ff35,                    \
	0xff31ff27,                    \
}

#define QF_VERSION          1
#define QF_STRING_TAG       "QUOTA HASH FILE"

#define QF_USERS_PER_GB     256
#define QF_MIN_USERS        2048
#define QF_MAX_USERS       (2048*1024)

#define QF_GROUPS_PER_GB    32
#define QF_MIN_GROUPS       2048
#define QF_MAX_GROUPS       (256*1024)


  
                                                              
                                                           
                                           
   
#define dqhash1(id, shift, mask)  \
	((((id) * 2654435761U) >> (shift)) & (mask))

#define dqhash2(id, mask)  \
	(dqhash1((id), 11, (mask)>>1) | 1)

  
                                           
   
#define dqoffset(index)  \
	(sizeof (struct dqfilehdr) + ((index) * sizeof (struct dqblk)))
  
                                
                                                                    
   
static __inline int dqhashshift(u_int32_t);

static __inline int
dqhashshift(u_int32_t size)
{
	int shift;

	for (shift = 32; size > 1; size >>= 1, --shift) {
		continue;
	}
	return shift;
}


#ifndef KERNEL
__BEGIN_DECLS
int quotactl(const char *, int, int, caddr_t);
__END_DECLS
#endif              

#ifdef KERNEL_PRIVATE
#include <sys/queue.h>



                  
   
struct quotafile {
	lck_mtx_t     qf_lock;                             
	struct vnode *qf_vp;                               
	kauth_cred_t  qf_cred;                                   
	int           qf_shift;                              
	int           qf_maxentries;                                      
	int           qf_entrycnt;                                
	u_int32_t     qf_btime;                                  
	u_int32_t     qf_itime;                                  

	                                          
	                             
	char          qf_qflags;                               
	int           qf_refcnt;                                           
};

  
                                                
                 
   
#define QTF_OPENING     0x01                                
#define QTF_CLOSING     0x02                                
#define QTF_WANTED      0x04                                     


  
                                                                      
                                                                       
                                                                        
                
   
struct dquot {
	LIST_ENTRY(dquot) dq_hash;                     
	TAILQ_ENTRY(dquot) dq_freelist;                
	u_int16_t dq_flags;                                   
	u_int16_t dq_cnt_unused;                                      
	u_int16_t dq_lflags;                                                  
	u_int16_t dq_type;                                            
	u_int32_t dq_id;                                                
	u_int32_t dq_index;                                        
	struct  quotafile *dq_qfile;                                            
	struct  dqblk dq_dqb;                                      
	uint32_t  dq_cnt;                                               
};

  
                   
   
#define DQ_LLOCK        0x01                                             
#define DQ_LWANT        0x02                                  

  
                  
   
#define DQ_MOD          0x01                                                
#define DQ_FAKE         0x02                                            
#define DQ_BLKS         0x04                                                 
#define DQ_INODS        0x08                                                   

  
                      
   
#define dq_bhardlimit   dq_dqb.dqb_bhardlimit
#define dq_bsoftlimit   dq_dqb.dqb_bsoftlimit
#define dq_curbytes     dq_dqb.dqb_curbytes
#define dq_ihardlimit   dq_dqb.dqb_ihardlimit
#define dq_isoftlimit   dq_dqb.dqb_isoftlimit
#define dq_curinodes    dq_dqb.dqb_curinodes
#define dq_btime        dq_dqb.dqb_btime
#define dq_itime        dq_dqb.dqb_itime

  
                                                                        
                                                                         
                                  
   
#define NODQUOT         NULL

  
                               
   
#define FORCE   0x01                                                   
#define CHOWN   0x02                                              


  
                                                  
                                 
   
__BEGIN_DECLS
void    dqfileinit(struct quotafile *);
int     dqfileopen(struct quotafile *, int);
void    dqfileclose(struct quotafile *, int);
void    dqflush(struct vnode *);
int     dqget(u_int32_t, struct quotafile *, int, struct dquot **);
void    dqhashinit(void);
int     dqisinitialized(void);
void    dqref(struct dquot *);
void    dqrele(struct dquot *);
void    dqreclaim(struct dquot *);
int     dqsync(struct dquot *);
void    dqsync_orphans(struct quotafile *);
void    dqlock(struct dquot *);
void    dqunlock(struct dquot *);

int     qf_get(struct quotafile *, int type);
void    qf_put(struct quotafile *, int type);

__private_extern__ void  munge_dqblk(struct dqblk *dqblkp, struct user_dqblk *user_dqblkp, boolean_t to64);
__END_DECLS

#endif                     

#endif                           

#endif                     
