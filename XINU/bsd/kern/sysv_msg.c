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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Implementation of SVID messages
 *
 * Author:  Daniel Boulet
 *
 * Copyright 1993 Daniel Boulet and RTMX Inc.
 *
 * This system call was implemented by Daniel Boulet under contract from RTMX.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/msg.h>
#include <sys/malloc.h>
#include <mach/mach_types.h>

#include <security/audit/audit.h>

#include <sys/filedesc.h>
#include <sys/file_internal.h>
#include <sys/sysctl.h>
#include <sys/sysproto.h>
#include <sys/ipcs.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#if SYSV_MSG

static int msginit(void *);

#define MSG_DEBUG
#undef MSG_DEBUG_OK

                                                      
                       
#if CONFIG_MACF_DEBUG
#define MPRINTF(a)      printf(a)
#else
#define MPRINTF(a)
#endif
static void msg_freehdr(struct msg *msghdr);

typedef int     sy_call_t(struct proc *, void *, int *);

                                                      
static sy_call_t* const msgcalls[] = {
	(sy_call_t *)msgctl, (sy_call_t *)msgget,
	(sy_call_t *)msgsnd, (sy_call_t *)msgrcv
};

static int              nfree_msgmaps;                             
static short            free_msgmaps;                                   
static struct msg       *free_msghdrs;                                
char                    *msgpool;                                             
struct msgmap           *msgmaps;                                     
struct msg              *msghdrs;                               
struct msqid_kernel     *msqids;                                                                          

static LCK_GRP_DECLARE(sysv_msg_subsys_lck_grp, "sysv_msg_subsys_lock");
static LCK_MTX_DECLARE(sysv_msg_subsys_mutex, &sysv_msg_subsys_lck_grp);

#define SYSV_MSG_SUBSYS_LOCK() lck_mtx_lock(&sysv_msg_subsys_mutex)
#define SYSV_MSG_SUBSYS_UNLOCK() lck_mtx_unlock(&sysv_msg_subsys_mutex)

#ifdef __APPLE_API_PRIVATE
int     msgmax,                                             
    msgmni,                                                        
    msgmnb,                                               
    msgtql,                                                 
    msgssz,                                                                      
    msgseg;                                                     
struct msginfo msginfo = {
	.msgmax = MSGMAX,                                                               
	.msgmni = MSGMNI,                                                         
	.msgmnb = MSGMNB,                                                  
	.msgtql = MSGTQL,                                                  
	.msgssz = MSGSSZ,                                                               
	.msgseg = MSGSEG                                                         
};
#endif                          

static __inline__ user_time_t
sysv_msgtime(void)
{
	struct timeval  tv;
	microtime(&tv);
	return tv.tv_sec;
}

  
                                                                 
   
static void
msqid_ds_kerneltouser32(struct user_msqid_ds *in, struct user32_msqid_ds *out)
{
	out->msg_perm   = in->msg_perm;
	out->msg_qnum   = in->msg_qnum;
	out->msg_cbytes = in->msg_cbytes;                     
	out->msg_qbytes = in->msg_qbytes;
	out->msg_lspid  = in->msg_lspid;
	out->msg_lrpid  = in->msg_lrpid;
	out->msg_stime  = in->msg_stime;                               
	out->msg_rtime  = in->msg_rtime;                               
	out->msg_ctime  = in->msg_ctime;                               
}

static void
msqid_ds_kerneltouser64(struct user_msqid_ds *in, struct user64_msqid_ds *out)
{
	out->msg_perm   = in->msg_perm;
	out->msg_qnum   = in->msg_qnum;
	out->msg_cbytes = in->msg_cbytes;                     
	out->msg_qbytes = in->msg_qbytes;
	out->msg_lspid  = in->msg_lspid;
	out->msg_lrpid  = in->msg_lrpid;
	out->msg_stime  = in->msg_stime;                               
	out->msg_rtime  = in->msg_rtime;                               
	out->msg_ctime  = in->msg_ctime;                               
}

  
                                                                             
                                                                           
                 
   
static void
msqid_ds_user32tokernel(struct user32_msqid_ds *in, struct user_msqid_ds *out)
{
	out->msg_ctime  = in->msg_ctime;
	out->msg_rtime  = in->msg_rtime;
	out->msg_stime  = in->msg_stime;
	out->msg_lrpid  = in->msg_lrpid;
	out->msg_lspid  = in->msg_lspid;
	out->msg_qbytes = in->msg_qbytes;
	out->msg_cbytes = in->msg_cbytes;                     
	out->msg_qnum   = in->msg_qnum;
	out->msg_perm   = in->msg_perm;
}

static void
msqid_ds_user64tokernel(struct user64_msqid_ds *in, struct user_msqid_ds *out)
{
	out->msg_ctime  = in->msg_ctime;
	out->msg_rtime  = in->msg_rtime;
	out->msg_stime  = in->msg_stime;
	out->msg_lrpid  = in->msg_lrpid;
	out->msg_lspid  = in->msg_lspid;
	out->msg_qbytes = in->msg_qbytes;
	out->msg_cbytes = in->msg_cbytes;                     
	out->msg_qnum   = in->msg_qnum;
	out->msg_perm   = in->msg_perm;
}

                                                                             
static int
msginit(__unused void *dummy)
{
	static int initted = 0;
	int i;

	                                                                        
	if (initted) {
		return initted;
	}

	  
                                                                   
                                                            
                                    
    
	i = 8;
	while (i < 1024 && i != msginfo.msgssz) {
		i <<= 1;
	}
	if (i != msginfo.msgssz) {
		printf("msginfo.msgssz=%d (0x%x) not a small power of 2; resetting to %d\n", msginfo.msgssz, msginfo.msgssz, MSGSSZ);
		msginfo.msgssz = MSGSSZ;
	}

	if (msginfo.msgseg > 32767) {
		printf("msginfo.msgseg=%d (> 32767); resetting to %d\n", msginfo.msgseg, MSGSEG);
		msginfo.msgseg = MSGSEG;
	}


	  
                                                                   
                                                                  
                            
    
	msgpool = kheap_alloc(KHEAP_DATA_BUFFERS, msginfo.msgmax, Z_WAITOK);
	if (msgpool == NULL) {
		printf("msginit: can't allocate msgpool");
		goto bad;
	}
	msgmaps = kheap_alloc(KM_SHM, sizeof(struct msgmap) * msginfo.msgseg,
	    Z_WAITOK);
	if (msgmaps == NULL) {
		printf("msginit: can't allocate msgmaps");
		goto bad;
	}

	msghdrs = kheap_alloc(KM_SHM, sizeof(struct msg) * msginfo.msgtql,
	    Z_WAITOK);
	if (msghdrs == NULL) {
		printf("msginit: can't allocate msghdrs");
		goto bad;
	}

	msqids = kheap_alloc(KM_SHM,
	    sizeof(struct msqid_kernel) * msginfo.msgmni, Z_WAITOK);
	if (msqids == NULL) {
		printf("msginit: can't allocate msqids");
		goto bad;
	}


	                  
	for (i = 0; i < msginfo.msgseg; i++) {
		if (i > 0) {
			msgmaps[i - 1].next = i;
		}
		msgmaps[i].next = -1;                                   
	}
	free_msgmaps = 0;
	nfree_msgmaps = msginfo.msgseg;


	                  
	for (i = 0; i < msginfo.msgtql; i++) {
		msghdrs[i].msg_type = 0;
		if (i > 0) {
			msghdrs[i - 1].msg_next = &msghdrs[i];
		}
		msghdrs[i].msg_next = NULL;
#if CONFIG_MACF
		mac_sysvmsg_label_init(&msghdrs[i]);
#endif
	}
	free_msghdrs = &msghdrs[0];

	                 
	for (i = 0; i < msginfo.msgmni; i++) {
		msqids[i].u.msg_qbytes = 0;                                     
		msqids[i].u.msg_perm._seq = 0;                              
		msqids[i].u.msg_perm.mode = 0;
#if CONFIG_MACF
		mac_sysvmsq_label_init(&msqids[i]);
#endif
	}

	initted = 1;
bad:
	if (!initted) {
		kheap_free(KHEAP_DATA_BUFFERS, msgpool,
		    sizeof(struct msgmap) * msginfo.msgseg);
		kheap_free(KM_SHM, msgmaps,
		    sizeof(struct msgmap) * msginfo.msgseg);
		kheap_free(KM_SHM, msghdrs,
		    sizeof(struct msg) * msginfo.msgtql);
		kheap_free(KM_SHM, msqids,
		    sizeof(struct msqid_kernel) * msginfo.msgmni);
	}
	return initted;
}

  
         
  
                                                                
  
                                            
                                                        
                                                            
  
                                                                                   
                                                         
  
                     
                              
  
                                                                 
  
                                                                       
                                                                       
                                                                  
  
   
int
msgsys(struct proc *p, struct msgsys_args *uap, int32_t *retval)
{
	if (uap->which >= sizeof(msgcalls) / sizeof(msgcalls[0])) {
		return EINVAL;
	}
	return (*msgcalls[uap->which])(p, &uap->a2, retval);
}

static void
msg_freehdr(struct msg *msghdr)
{
	while (msghdr->msg_ts > 0) {
		short next;
		if (msghdr->msg_spot < 0 || msghdr->msg_spot >= msginfo.msgseg) {
			panic("msghdr->msg_spot out of range");
		}
		next = msgmaps[msghdr->msg_spot].next;
		msgmaps[msghdr->msg_spot].next = free_msgmaps;
		free_msgmaps = msghdr->msg_spot;
		nfree_msgmaps++;
		msghdr->msg_spot = next;
		if (msghdr->msg_ts >= msginfo.msgssz) {
			msghdr->msg_ts -= msginfo.msgssz;
		} else {
			msghdr->msg_ts = 0;
		}
	}
	if (msghdr->msg_spot != -1) {
		panic("msghdr->msg_spot != -1");
	}
	msghdr->msg_next = free_msghdrs;
	free_msghdrs = msghdr;
#if CONFIG_MACF
	mac_sysvmsg_label_recycle(msghdr);
#endif
	  
                                                                   
                  
    
	wakeup((caddr_t)&free_msghdrs);
}

int
msgctl(struct proc *p, struct msgctl_args *uap, int32_t *retval)
{
	int msqid = uap->msqid;
	int cmd = uap->cmd;
	kauth_cred_t cred = kauth_cred_get();
	int rval, eval;
	struct user_msqid_ds msqbuf;
	struct msqid_kernel *msqptr;

	SYSV_MSG_SUBSYS_LOCK();

	if (!msginit(0)) {
		eval =  ENOMEM;
		goto msgctlout;
	}

#ifdef MSG_DEBUG_OK
	printf("call to msgctl(%d, %d, 0x%qx)\n", msqid, cmd, uap->buf);
#endif

	AUDIT_ARG(svipc_cmd, cmd);
	AUDIT_ARG(svipc_id, msqid);
	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
#ifdef MSG_DEBUG_OK
		printf("msqid (%d) out of range (0<=msqid<%d)\n", msqid,
		    msginfo.msgmni);
#endif
		eval = EINVAL;
		goto msgctlout;
	}

	msqptr = &msqids[msqid];

	if (msqptr->u.msg_qbytes == 0) {
#ifdef MSG_DEBUG_OK
		printf("no such msqid\n");
#endif
		eval = EINVAL;
		goto msgctlout;
	}
	if (msqptr->u.msg_perm._seq != IPCID_TO_SEQ(uap->msqid)) {
#ifdef MSG_DEBUG_OK
		printf("wrong sequence number\n");
#endif
		eval = EINVAL;
		goto msgctlout;
	}
#if CONFIG_MACF
	eval = mac_sysvmsq_check_msqctl(kauth_cred_get(), msqptr, cmd);
	if (eval) {
		goto msgctlout;
	}
#endif

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
	{
		struct msg *msghdr;
		if ((eval = ipcperm(cred, &msqptr->u.msg_perm, IPC_M))) {
			goto msgctlout;
		}
#if CONFIG_MACF
		  
                                                        
                                                       
                                                     
                                                            
                                                      
                                                           
                                
     
		for (msghdr = msqptr->u.msg_first; msghdr != NULL;
		    msghdr = msghdr->msg_next) {
			eval = mac_sysvmsq_check_msgrmid(kauth_cred_get(), msghdr);
			if (eval) {
				goto msgctlout;
			}
		}
#endif
		                              
		msghdr = msqptr->u.msg_first;
		while (msghdr != NULL) {
			struct msg *msghdr_tmp;

			                                       
			msqptr->u.msg_cbytes -= msghdr->msg_ts;
			msqptr->u.msg_qnum--;
			msghdr_tmp = msghdr;
			msghdr = msghdr->msg_next;
			msg_freehdr(msghdr_tmp);
		}

		if (msqptr->u.msg_cbytes != 0) {
			panic("msg_cbytes is messed up");
		}
		if (msqptr->u.msg_qnum != 0) {
			panic("msg_qnum is messed up");
		}

		msqptr->u.msg_qbytes = 0;                            
#if CONFIG_MACF
		mac_sysvmsq_label_recycle(msqptr);
#endif

		wakeup((caddr_t)msqptr);
	}

	break;

	case IPC_SET:
		if ((eval = ipcperm(cred, &msqptr->u.msg_perm, IPC_M))) {
			goto msgctlout;
		}

		SYSV_MSG_SUBSYS_UNLOCK();

		if (IS_64BIT_PROCESS(p)) {
			struct user64_msqid_ds tmpds;
			eval = copyin(uap->buf, &tmpds, sizeof(tmpds));

			msqid_ds_user64tokernel(&tmpds, &msqbuf);
		} else {
			struct user32_msqid_ds tmpds;

			eval = copyin(uap->buf, &tmpds, sizeof(tmpds));

			msqid_ds_user32tokernel(&tmpds, &msqbuf);
		}
		if (eval) {
			return eval;
		}

		SYSV_MSG_SUBSYS_LOCK();

		if (msqbuf.msg_qbytes > msqptr->u.msg_qbytes) {
			eval = suser(cred, &p->p_acflag);
			if (eval) {
				goto msgctlout;
			}
		}


		                                                           
		if (msqbuf.msg_qbytes > (user_msglen_t)msginfo.msgmnb) {
#ifdef MSG_DEBUG_OK
			printf("can't increase msg_qbytes beyond %d (truncating)\n",
			    msginfo.msgmnb);
#endif
			msqbuf.msg_qbytes = msginfo.msgmnb;                                                   
		}
		if (msqbuf.msg_qbytes == 0) {
#ifdef MSG_DEBUG_OK
			printf("can't reduce msg_qbytes to 0\n");
#endif
			eval = EINVAL;
			goto msgctlout;
		}
		msqptr->u.msg_perm.uid = msqbuf.msg_perm.uid;                         
		msqptr->u.msg_perm.gid = msqbuf.msg_perm.gid;                         
		msqptr->u.msg_perm.mode = (msqptr->u.msg_perm.mode & ~0777) |
		    (msqbuf.msg_perm.mode & 0777);
		msqptr->u.msg_qbytes = msqbuf.msg_qbytes;
		msqptr->u.msg_ctime = sysv_msgtime();
		break;

	case IPC_STAT:
		if ((eval = ipcperm(cred, &msqptr->u.msg_perm, IPC_R))) {
#ifdef MSG_DEBUG_OK
			printf("requester doesn't have read access\n");
#endif
			goto msgctlout;
		}

		SYSV_MSG_SUBSYS_UNLOCK();
		if (IS_64BIT_PROCESS(p)) {
			struct user64_msqid_ds msqid_ds64 = {};
			msqid_ds_kerneltouser64(&msqptr->u, &msqid_ds64);
			eval = copyout(&msqid_ds64, uap->buf, sizeof(msqid_ds64));
		} else {
			struct user32_msqid_ds msqid_ds32 = {};
			msqid_ds_kerneltouser32(&msqptr->u, &msqid_ds32);
			eval = copyout(&msqid_ds32, uap->buf, sizeof(msqid_ds32));
		}
		SYSV_MSG_SUBSYS_LOCK();
		break;

	default:
#ifdef MSG_DEBUG_OK
		printf("invalid command %d\n", cmd);
#endif
		eval = EINVAL;
		goto msgctlout;
	}

	if (eval == 0) {
		*retval = rval;
	}
msgctlout:
	SYSV_MSG_SUBSYS_UNLOCK();
	return eval;
}

int
msgget(__unused struct proc *p, struct msgget_args *uap, int32_t *retval)
{
	int msqid, eval;
	int key = uap->key;
	int msgflg = uap->msgflg;
	kauth_cred_t cred = kauth_cred_get();
	struct msqid_kernel *msqptr = NULL;

	SYSV_MSG_SUBSYS_LOCK();

	if (!msginit(0)) {
		eval =  ENOMEM;
		goto msggetout;
	}

#ifdef MSG_DEBUG_OK
	printf("msgget(0x%x, 0%o)\n", key, msgflg);
#endif

	if (key != IPC_PRIVATE) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			msqptr = &msqids[msqid];
			if (msqptr->u.msg_qbytes != 0 &&
			    msqptr->u.msg_perm._key == key) {
				break;
			}
		}
		if (msqid < msginfo.msgmni) {
#ifdef MSG_DEBUG_OK
			printf("found public key\n");
#endif
			if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
#ifdef MSG_DEBUG_OK
				printf("not exclusive\n");
#endif
				eval = EEXIST;
				goto msggetout;
			}
			if ((eval = ipcperm(cred, &msqptr->u.msg_perm, msgflg & 0700 ))) {
#ifdef MSG_DEBUG_OK
				printf("requester doesn't have 0%o access\n",
				    msgflg & 0700);
#endif
				goto msggetout;
			}
#if CONFIG_MACF
			eval = mac_sysvmsq_check_msqget(cred, msqptr);
			if (eval) {
				goto msggetout;
			}
#endif
			goto found;
		}
	}

#ifdef MSG_DEBUG_OK
	printf("need to allocate the user_msqid_ds\n");
#endif
	if (key == IPC_PRIVATE || (msgflg & IPC_CREAT)) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			  
                                                         
                                                       
                                                    
                                                   
      
			msqptr = &msqids[msqid];
			if (msqptr->u.msg_qbytes == 0 &&
			    (msqptr->u.msg_perm.mode & MSG_LOCKED) == 0) {
				break;
			}
		}
		if (msqid == msginfo.msgmni) {
#ifdef MSG_DEBUG_OK
			printf("no more user_msqid_ds's available\n");
#endif
			eval = ENOSPC;
			goto msggetout;
		}
#ifdef MSG_DEBUG_OK
		printf("msqid %d is available\n", msqid);
#endif
		msqptr->u.msg_perm._key = key;
		msqptr->u.msg_perm.cuid = kauth_cred_getuid(cred);
		msqptr->u.msg_perm.uid = kauth_cred_getuid(cred);
		msqptr->u.msg_perm.cgid = kauth_cred_getgid(cred);
		msqptr->u.msg_perm.gid = kauth_cred_getgid(cred);
		msqptr->u.msg_perm.mode = (msgflg & 0777);
		                                                 
		msqptr->u.msg_perm._seq++;
		msqptr->u.msg_first = NULL;
		msqptr->u.msg_last = NULL;
		msqptr->u.msg_cbytes = 0;
		msqptr->u.msg_qnum = 0;
		msqptr->u.msg_qbytes = msginfo.msgmnb;
		msqptr->u.msg_lspid = 0;
		msqptr->u.msg_lrpid = 0;
		msqptr->u.msg_stime = 0;
		msqptr->u.msg_rtime = 0;
		msqptr->u.msg_ctime = sysv_msgtime();
#if CONFIG_MACF
		mac_sysvmsq_label_associate(cred, msqptr);
#endif
	} else {
#ifdef MSG_DEBUG_OK
		printf("didn't find it and wasn't asked to create it\n");
#endif
		eval = ENOENT;
		goto msggetout;
	}

found:
	                                
	*retval = IXSEQ_TO_IPCID(msqid, msqptr->u.msg_perm);
	AUDIT_ARG(svipc_id, *retval);
	eval = 0;
msggetout:
	SYSV_MSG_SUBSYS_UNLOCK();
	return eval;
}


int
msgsnd(struct proc *p, struct msgsnd_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return msgsnd_nocancel(p, (struct msgsnd_nocancel_args *)uap, retval);
}

int
msgsnd_nocancel(struct proc *p, struct msgsnd_nocancel_args *uap, int32_t *retval)
{
	int msqid = uap->msqid;
	user_addr_t user_msgp = uap->msgp;
	size_t msgsz = (size_t)uap->msgsz;                       
	int msgflg = uap->msgflg;
	int segs_needed, eval;
	struct msqid_kernel *msqptr;
	struct msg *msghdr;
	short next;
	user_long_t msgtype;


	SYSV_MSG_SUBSYS_LOCK();

	if (!msginit(0)) {
		eval =  ENOMEM;
		goto msgsndout;
	}

#ifdef MSG_DEBUG_OK
	printf("call to msgsnd(%d, 0x%qx, %ld, %d)\n", msqid, user_msgp, msgsz,
	    msgflg);
#endif

	AUDIT_ARG(svipc_id, msqid);
	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
#ifdef MSG_DEBUG_OK
		printf("msqid (%d) out of range (0<=msqid<%d)\n", msqid,
		    msginfo.msgmni);
#endif
		eval = EINVAL;
		goto msgsndout;
	}

	msqptr = &msqids[msqid];
	if (msqptr->u.msg_qbytes == 0) {
#ifdef MSG_DEBUG_OK
		printf("no such message queue id\n");
#endif
		eval = EINVAL;
		goto msgsndout;
	}
	if (msqptr->u.msg_perm._seq != IPCID_TO_SEQ(uap->msqid)) {
#ifdef MSG_DEBUG_OK
		printf("wrong sequence number\n");
#endif
		eval = EINVAL;
		goto msgsndout;
	}

	if ((eval = ipcperm(kauth_cred_get(), &msqptr->u.msg_perm, IPC_W))) {
#ifdef MSG_DEBUG_OK
		printf("requester doesn't have write access\n");
#endif
		goto msgsndout;
	}

#if CONFIG_MACF
	eval = mac_sysvmsq_check_msqsnd(kauth_cred_get(), msqptr);
	if (eval) {
		goto msgsndout;
	}
#endif
	segs_needed = (msgsz + msginfo.msgssz - 1) / msginfo.msgssz;
#ifdef MSG_DEBUG_OK
	printf("msgsz=%ld, msgssz=%d, segs_needed=%d\n", msgsz, msginfo.msgssz,
	    segs_needed);
#endif

	  
                                                                    
                                                                   
                                                                      
                                            
    
	for (;;) {
		void *blocking_resource = NULL;

		  
                                                               
                                                              
                                     
     
		if (msgsz > msqptr->u.msg_qbytes) {
#ifdef MSG_DEBUG_OK
			printf("msgsz > msqptr->msg_qbytes\n");
#endif
			eval = EINVAL;
			goto msgsndout;
		}

		  
                                                                
                                   
     
		if (msqptr->u.msg_perm.mode & MSG_LOCKED) {
#ifdef MSG_DEBUG_OK
			printf("msqid is locked\n");
#endif
			blocking_resource = msqptr;
		}

		  
                                                                
                                                           
                                                                 
     
		if (msgsz + msqptr->u.msg_cbytes > msqptr->u.msg_qbytes) {
#ifdef MSG_DEBUG_OK
			printf("msgsz + msg_cbytes > msg_qbytes\n");
#endif
			blocking_resource = msqptr;
		}

		  
                                                           
                                                               
                                                           
            
     
		if (segs_needed > nfree_msgmaps) {
#ifdef MSG_DEBUG_OK
			printf("segs_needed > nfree_msgmaps\n");
#endif
			blocking_resource = &free_msghdrs;
		}
		if (free_msghdrs == NULL) {
#ifdef MSG_DEBUG_OK
			printf("no more msghdrs\n");
#endif
			blocking_resource = &free_msghdrs;
		}

		if (blocking_resource != NULL) {
			int we_own_it;

			if ((msgflg & IPC_NOWAIT) != 0) {
#ifdef MSG_DEBUG_OK
				printf("need more resources but caller doesn't want to wait\n");
#endif
				eval = EAGAIN;
				goto msgsndout;
			}

			if ((msqptr->u.msg_perm.mode & MSG_LOCKED) != 0) {
#ifdef MSG_DEBUG_OK
				printf("we don't own the user_msqid_ds\n");
#endif
				we_own_it = 0;
			} else {
				                                       
                  
#ifdef MSG_DEBUG_OK
				printf("we own the user_msqid_ds\n");
#endif
				msqptr->u.msg_perm.mode |= MSG_LOCKED;
				we_own_it = 1;
			}
#ifdef MSG_DEBUG_OK
			printf("goodnight\n");
#endif
			eval = msleep(blocking_resource, &sysv_msg_subsys_mutex, (PZERO - 4) | PCATCH,
			    "msgwait", 0);
#ifdef MSG_DEBUG_OK
			printf("good morning, eval=%d\n", eval);
#endif
			if (we_own_it) {
				msqptr->u.msg_perm.mode &= ~MSG_LOCKED;
			}
			if (eval != 0) {
#ifdef MSG_DEBUG_OK
				printf("msgsnd:  interrupted system call\n");
#endif
				eval = EINTR;
				goto msgsndout;
			}

			  
                                               
      

			if (msqptr->u.msg_qbytes == 0) {
#ifdef MSG_DEBUG_OK
				printf("msqid deleted\n");
#endif
				eval = EIDRM;
				goto msgsndout;
			}
		} else {
#ifdef MSG_DEBUG_OK
			printf("got all the resources that we need\n");
#endif
			break;
		}
	}

	  
                                       
              
    

	if (msqptr->u.msg_perm.mode & MSG_LOCKED) {
		panic("msg_perm.mode & MSG_LOCKED");
	}
	if (segs_needed > nfree_msgmaps) {
		panic("segs_needed > nfree_msgmaps");
	}
	if (msgsz + msqptr->u.msg_cbytes > msqptr->u.msg_qbytes) {
		panic("msgsz + msg_cbytes > msg_qbytes");
	}
	if (free_msghdrs == NULL) {
		panic("no more msghdrs");
	}

	  
                                                                   
               
    
	if ((msqptr->u.msg_perm.mode & MSG_LOCKED) != 0) {
		panic("user_msqid_ds is already locked");
	}
	msqptr->u.msg_perm.mode |= MSG_LOCKED;

	  
                             
    
	msghdr = free_msghdrs;
	free_msghdrs = msghdr->msg_next;
	msghdr->msg_spot = -1;
	msghdr->msg_ts = msgsz;

#if CONFIG_MACF
	mac_sysvmsg_label_associate(kauth_cred_get(), msqptr, msghdr);
#endif
	  
                                  
    

	while (segs_needed > 0) {
		if (nfree_msgmaps <= 0) {
			panic("not enough msgmaps");
		}
		if (free_msgmaps == -1) {
			panic("nil free_msgmaps");
		}
		next = free_msgmaps;
		if (next <= -1) {
			panic("next too low #1");
		}
		if (next >= msginfo.msgseg) {
			panic("next out of range #1");
		}
#ifdef MSG_DEBUG_OK
		printf("allocating segment %d to message\n", next);
#endif
		free_msgmaps = msgmaps[next].next;
		nfree_msgmaps--;
		msgmaps[next].next = msghdr->msg_spot;
		msghdr->msg_spot = next;
		segs_needed--;
	}

	  
                                                                     
                                                            
    
	if (IS_64BIT_PROCESS(p)) {
		SYSV_MSG_SUBSYS_UNLOCK();
		eval = copyin(user_msgp, &msgtype, sizeof(msgtype));
		SYSV_MSG_SUBSYS_LOCK();
		msghdr->msg_type = CAST_DOWN(long, msgtype);
		user_msgp = user_msgp + sizeof(msgtype);                      
	} else {
		SYSV_MSG_SUBSYS_UNLOCK();
		int32_t msg_type32;
		eval = copyin(user_msgp, &msg_type32, sizeof(msg_type32));
		msghdr->msg_type = msg_type32;
		SYSV_MSG_SUBSYS_LOCK();
		user_msgp = user_msgp + sizeof(msg_type32);                           
	}

	if (eval != 0) {
#ifdef MSG_DEBUG_OK
		printf("error %d copying the message type\n", eval);
#endif
		msg_freehdr(msghdr);
		msqptr->u.msg_perm.mode &= ~MSG_LOCKED;
		wakeup((caddr_t)msqptr);
		goto msgsndout;
	}


	  
                             
    
	if (msghdr->msg_type < 1) {
		msg_freehdr(msghdr);
		msqptr->u.msg_perm.mode &= ~MSG_LOCKED;
		wakeup((caddr_t)msqptr);
#ifdef MSG_DEBUG_OK
		printf("mtype (%ld) < 1\n", msghdr->msg_type);
#endif
		eval = EINVAL;
		goto msgsndout;
	}

	  
                            
    
	next = msghdr->msg_spot;
	while (msgsz > 0) {
		size_t tlen;
		                                                               
		if (msgsz > (size_t)msginfo.msgssz) {
			tlen = msginfo.msgssz;
		} else {
			tlen = msgsz;
		}
		if (next <= -1) {
			panic("next too low #2");
		}
		if (next >= msginfo.msgseg) {
			panic("next out of range #2");
		}

		SYSV_MSG_SUBSYS_UNLOCK();
		eval = copyin(user_msgp, &msgpool[next * msginfo.msgssz], tlen);
		SYSV_MSG_SUBSYS_LOCK();

		if (eval != 0) {
#ifdef MSG_DEBUG_OK
			printf("error %d copying in message segment\n", eval);
#endif
			msg_freehdr(msghdr);
			msqptr->u.msg_perm.mode &= ~MSG_LOCKED;
			wakeup((caddr_t)msqptr);

			goto msgsndout;
		}
		msgsz -= tlen;
		user_msgp = user_msgp + tlen;                 
		next = msgmaps[next].next;
	}
	if (next != -1) {
		panic("didn't use all the msg segments");
	}

	  
                                                     
    

	msqptr->u.msg_perm.mode &= ~MSG_LOCKED;

	  
                                                        
    

	if (msqptr->u.msg_qbytes == 0) {
		msg_freehdr(msghdr);
		wakeup((caddr_t)msqptr);
		                                    
#ifdef EIDRM
		eval = EIDRM;
#else
		                                                      
		eval = EINVAL;
#endif
		goto msgsndout;
	}

#if CONFIG_MACF
	  
                                                                
                                                                    
                                                             
                                                                      
                                                                   
                                                                   
                                                                  
                                                                 
         
    
	eval = mac_sysvmsq_check_enqueue(kauth_cred_get(), msghdr, msqptr);
	if (eval) {
		msg_freehdr(msghdr);
		wakeup((caddr_t) msqptr);
		goto msgsndout;
	}
#endif
	  
                                  
    

	if (msqptr->u.msg_first == NULL) {
		msqptr->u.msg_first = msghdr;
		msqptr->u.msg_last = msghdr;
	} else {
		msqptr->u.msg_last->msg_next = msghdr;
		msqptr->u.msg_last = msghdr;
	}
	msqptr->u.msg_last->msg_next = NULL;

	msqptr->u.msg_cbytes += msghdr->msg_ts;
	msqptr->u.msg_qnum++;
	msqptr->u.msg_lspid = p->p_pid;
	msqptr->u.msg_stime = sysv_msgtime();

	wakeup((caddr_t)msqptr);
	*retval = 0;
	eval = 0;

msgsndout:
	SYSV_MSG_SUBSYS_UNLOCK();
	return eval;
}


int
msgrcv(struct proc *p, struct msgrcv_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return msgrcv_nocancel(p, (struct msgrcv_nocancel_args *)uap, retval);
}

int
msgrcv_nocancel(struct proc *p, struct msgrcv_nocancel_args *uap, user_ssize_t *retval)
{
	int msqid = uap->msqid;
	user_addr_t user_msgp = uap->msgp;
	size_t msgsz = (size_t)uap->msgsz;                       
	long msgtyp = (long)uap->msgtyp;                              
	int msgflg = uap->msgflg;
	size_t len;
	struct msqid_kernel *msqptr;
	struct msg *msghdr;
	int eval;
	short next;
	user_long_t msgtype;
	int32_t msg_type32;

	SYSV_MSG_SUBSYS_LOCK();

	if (!msginit(0)) {
		eval =  ENOMEM;
		goto msgrcvout;
	}

#ifdef MSG_DEBUG_OK
	printf("call to msgrcv(%d, 0x%qx, %ld, %ld, %d)\n", msqid, user_msgp,
	    msgsz, msgtyp, msgflg);
#endif

	AUDIT_ARG(svipc_id, msqid);
	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
#ifdef MSG_DEBUG_OK
		printf("msqid (%d) out of range (0<=msqid<%d)\n", msqid,
		    msginfo.msgmni);
#endif
		eval = EINVAL;
		goto msgrcvout;
	}

	msqptr = &msqids[msqid];
	if (msqptr->u.msg_qbytes == 0) {
#ifdef MSG_DEBUG_OK
		printf("no such message queue id\n");
#endif
		eval = EINVAL;
		goto msgrcvout;
	}
	if (msqptr->u.msg_perm._seq != IPCID_TO_SEQ(uap->msqid)) {
#ifdef MSG_DEBUG_OK
		printf("wrong sequence number\n");
#endif
		eval = EINVAL;
		goto msgrcvout;
	}

	if ((eval = ipcperm(kauth_cred_get(), &msqptr->u.msg_perm, IPC_R))) {
#ifdef MSG_DEBUG_OK
		printf("requester doesn't have read access\n");
#endif
		goto msgrcvout;
	}

#if CONFIG_MACF
	eval = mac_sysvmsq_check_msqrcv(kauth_cred_get(), msqptr);
	if (eval) {
		goto msgrcvout;
	}
#endif
	msghdr = NULL;
	while (msghdr == NULL) {
		if (msgtyp == 0) {
			msghdr = msqptr->u.msg_first;
			if (msghdr != NULL) {
				if (msgsz < msghdr->msg_ts &&
				    (msgflg & MSG_NOERROR) == 0) {
#ifdef MSG_DEBUG_OK
					printf("first message on the queue is too big (want %ld, got %d)\n",
					    msgsz, msghdr->msg_ts);
#endif
					eval = E2BIG;
					goto msgrcvout;
				}
#if CONFIG_MACF
				eval = mac_sysvmsq_check_msgrcv(kauth_cred_get(),
				    msghdr);
				if (eval) {
					goto msgrcvout;
				}
#endif
				if (msqptr->u.msg_first == msqptr->u.msg_last) {
					msqptr->u.msg_first = NULL;
					msqptr->u.msg_last = NULL;
				} else {
					msqptr->u.msg_first = msghdr->msg_next;
					if (msqptr->u.msg_first == NULL) {
						panic("msg_first/last messed up #1");
					}
				}
			}
		} else {
			struct msg *previous;
			struct msg **prev;

			previous = NULL;
			prev = &(msqptr->u.msg_first);
			while ((msghdr = *prev) != NULL) {
				  
                                                  
                                                
                                               
                                                 
                                                
                                   
       

				if (msgtyp == msghdr->msg_type ||
				    msghdr->msg_type <= -msgtyp) {
#ifdef MSG_DEBUG_OK
					printf("found message type %ld, requested %ld\n",
					    msghdr->msg_type, msgtyp);
#endif
					if (msgsz < msghdr->msg_ts &&
					    (msgflg & MSG_NOERROR) == 0) {
#ifdef MSG_DEBUG_OK
						printf("requested message on the queue is too big (want %ld, got %d)\n",
						    msgsz, msghdr->msg_ts);
#endif
						eval = E2BIG;
						goto msgrcvout;
					}
#if CONFIG_MACF
					eval = mac_sysvmsq_check_msgrcv(
						kauth_cred_get(), msghdr);
					if (eval) {
						goto msgrcvout;
					}
#endif
					*prev = msghdr->msg_next;
					if (msghdr == msqptr->u.msg_last) {
						if (previous == NULL) {
							if (prev !=
							    &msqptr->u.msg_first) {
								panic("msg_first/last messed up #2");
							}
							msqptr->u.msg_first =
							    NULL;
							msqptr->u.msg_last =
							    NULL;
						} else {
							if (prev ==
							    &msqptr->u.msg_first) {
								panic("msg_first/last messed up #3");
							}
							msqptr->u.msg_last =
							    previous;
						}
					}
					break;
				}
				previous = msghdr;
				prev = &(msghdr->msg_next);
			}
		}

		  
                                                          
                                
                                                
     

		if (msghdr != NULL) {
			break;
		}

		  
                                                          
     

		if ((msgflg & IPC_NOWAIT) != 0) {
#ifdef MSG_DEBUG_OK
			printf("no appropriate message found (msgtyp=%ld)\n",
			    msgtyp);
#endif
			                                     
#ifdef ENOMSG
			eval = ENOMSG;
#else
			                                                      
			eval = EAGAIN;
#endif
			goto msgrcvout;
		}

		  
                                 
     

#ifdef MSG_DEBUG_OK
		printf("msgrcv:  goodnight\n");
#endif
		eval = msleep((caddr_t)msqptr, &sysv_msg_subsys_mutex, (PZERO - 4) | PCATCH, "msgwait",
		    0);
#ifdef MSG_DEBUG_OK
		printf("msgrcv:  good morning (eval=%d)\n", eval);
#endif

		if (eval != 0) {
#ifdef MSG_DEBUG_OK
			printf("msgsnd:  interrupted system call\n");
#endif
			eval = EINTR;
			goto msgrcvout;
		}

		  
                                              
     

		if (msqptr->u.msg_qbytes == 0 ||
		    msqptr->u.msg_perm._seq != IPCID_TO_SEQ(uap->msqid)) {
#ifdef MSG_DEBUG_OK
			printf("msqid deleted\n");
#endif
			                                    
#ifdef EIDRM
			eval = EIDRM;
#else
			                                                      
			eval = EINVAL;
#endif
			goto msgrcvout;
		}
	}

	  
                                   
   
                                                                 
    

	msqptr->u.msg_cbytes -= msghdr->msg_ts;
	msqptr->u.msg_qnum--;
	msqptr->u.msg_lrpid = p->p_pid;
	msqptr->u.msg_rtime = sysv_msgtime();

	  
                                                         
                                                                      
                                     
    

#ifdef MSG_DEBUG_OK
	printf("found a message, msgsz=%ld, msg_ts=%d\n", msgsz,
	    msghdr->msg_ts);
#endif
	if (msgsz > msghdr->msg_ts) {
		msgsz = msghdr->msg_ts;
	}

	  
                                
    

	  
                                                                      
                                                            
    
	if (IS_64BIT_PROCESS(p)) {
		msgtype = msghdr->msg_type;
		SYSV_MSG_SUBSYS_UNLOCK();
		eval = copyout(&msgtype, user_msgp, sizeof(msgtype));
		SYSV_MSG_SUBSYS_LOCK();
		user_msgp = user_msgp + sizeof(msgtype);                      
	} else {
		msg_type32 = msghdr->msg_type;
		SYSV_MSG_SUBSYS_UNLOCK();
		eval = copyout(&msg_type32, user_msgp, sizeof(msg_type32));
		SYSV_MSG_SUBSYS_LOCK();
		user_msgp = user_msgp + sizeof(msg_type32);                           
	}

	if (eval != 0) {
#ifdef MSG_DEBUG_OK
		printf("error (%d) copying out message type\n", eval);
#endif
		msg_freehdr(msghdr);
		wakeup((caddr_t)msqptr);

		goto msgrcvout;
	}


	  
                                   
    

	next = msghdr->msg_spot;
	for (len = 0; len < msgsz; len += msginfo.msgssz) {
		size_t tlen;

		  
                                                       
                   
     
		tlen = MIN(msgsz - len, (size_t)msginfo.msgssz);
		if (next <= -1) {
			panic("next too low #3");
		}
		if (next >= msginfo.msgseg) {
			panic("next out of range #3");
		}
		SYSV_MSG_SUBSYS_UNLOCK();
		eval = copyout(&msgpool[next * msginfo.msgssz],
		    user_msgp, tlen);
		SYSV_MSG_SUBSYS_LOCK();
		if (eval != 0) {
#ifdef MSG_DEBUG_OK
			printf("error (%d) copying out message segment\n",
			    eval);
#endif
			msg_freehdr(msghdr);
			wakeup((caddr_t)msqptr);
			goto msgrcvout;
		}
		user_msgp = user_msgp + tlen;                 
		next = msgmaps[next].next;
	}

	  
                                                       
    

	msg_freehdr(msghdr);
	wakeup((caddr_t)msqptr);
	*retval = msgsz;
	eval = 0;
msgrcvout:
	SYSV_MSG_SUBSYS_UNLOCK();
	return eval;
}

static int
IPCS_msg_sysctl(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	int error;
	int cursor;
	union {
		struct user32_IPCS_command u32;
		struct user_IPCS_command u64;
	} ipcs = { };
	struct user32_msqid_ds msqid_ds32 = {};                                      
	struct user64_msqid_ds msqid_ds64 = {};                                      
	void *msqid_dsp;
	size_t ipcs_sz;
	size_t msqid_ds_sz;
	struct proc *p = current_proc();

	if (IS_64BIT_PROCESS(p)) {
		ipcs_sz = sizeof(struct user_IPCS_command);
		msqid_ds_sz = sizeof(struct user64_msqid_ds);
	} else {
		ipcs_sz = sizeof(struct user32_IPCS_command);
		msqid_ds_sz = sizeof(struct user32_msqid_ds);
	}

	                                   
	if ((error = SYSCTL_IN(req, &ipcs, ipcs_sz)) != 0) {
		return error;
	}

	if (!IS_64BIT_PROCESS(p)) {                           
		ipcs.u64.ipcs_data = CAST_USER_ADDR_T(ipcs.u32.ipcs_data);
	}

	                                      
	if (ipcs.u64.ipcs_magic != IPCS_MAGIC) {
		return EINVAL;
	}

	SYSV_MSG_SUBSYS_LOCK();

	switch (ipcs.u64.ipcs_op) {
	case IPCS_MSG_CONF:                                           
		if (ipcs.u64.ipcs_datalen != sizeof(struct msginfo)) {
			error = ERANGE;
			break;
		}
		if (ipcs.u64.ipcs_cursor != 0) {                          
			error = EINVAL;
			break;
		}
		SYSV_MSG_SUBSYS_UNLOCK();
		error = copyout(&msginfo, ipcs.u64.ipcs_data, ipcs.u64.ipcs_datalen);
		SYSV_MSG_SUBSYS_LOCK();
		break;

	case IPCS_MSG_ITER:                                         
		                                                             
		if (!msginit(0)) {
			error =  ENOMEM;
			break;
		}

		cursor = ipcs.u64.ipcs_cursor;
		if (cursor < 0 || cursor >= msginfo.msgmni) {
			error = ERANGE;
			break;
		}
		if (ipcs.u64.ipcs_datalen != (int)msqid_ds_sz) {
			error = EINVAL;
			break;
		}
		for (; cursor < msginfo.msgmni; cursor++) {
			if (msqids[cursor].u.msg_qbytes != 0) {                
				break;
			}
			continue;
		}
		if (cursor == msginfo.msgmni) {
			error = ENOENT;
			break;
		}

		msqid_dsp = &msqids[cursor];                         

		  
                                                    
                                     
     
		if (IS_64BIT_PROCESS(p)) {
			msqid_ds_kerneltouser64(msqid_dsp, &msqid_ds64);
			msqid_dsp = &msqid_ds64;
		} else {
			msqid_ds_kerneltouser32(msqid_dsp, &msqid_ds32);
			msqid_dsp = &msqid_ds32;
		}

		SYSV_MSG_SUBSYS_UNLOCK();
		error = copyout(msqid_dsp, ipcs.u64.ipcs_data, ipcs.u64.ipcs_datalen);
		if (!error) {
			                   
			ipcs.u64.ipcs_cursor = cursor + 1;

			if (!IS_64BIT_PROCESS(p)) {                           
				ipcs.u32.ipcs_data = CAST_DOWN_EXPLICIT(user32_addr_t, ipcs.u64.ipcs_data);
			}
			error = SYSCTL_OUT(req, &ipcs, ipcs_sz);
		}
		SYSV_MSG_SUBSYS_LOCK();
		break;

	default:
		error = EINVAL;
		break;
	}

	SYSV_MSG_SUBSYS_UNLOCK();
	return error;
}

SYSCTL_DECL(_kern_sysv_ipcs);
SYSCTL_PROC(_kern_sysv_ipcs, OID_AUTO, msg, CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, IPCS_msg_sysctl,
    "S,IPCS_msg_command",
    "ipcs msg command interface");

#endif               
