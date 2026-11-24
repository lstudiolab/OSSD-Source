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
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005-2006 SPARTA, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/sem_internal.h>
#include <sys/malloc.h>
#include <mach/mach_types.h>

#include <sys/filedesc.h>
#include <sys/file_internal.h>
#include <sys/sysctl.h>
#include <sys/ipcs.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <security/audit/audit.h>

#if SYSV_SEM


                                                     
                       

                                                      
                       
#if CONFIG_MACF_DEBUG
#define MPRINTF(a)      printf(a)
#else
#define MPRINTF(a)
#endif

#define KM_SYSVSEM       KHEAP_DEFAULT


                                                                 
                                                                     
   
static struct seminfo limitseminfo = {
	.semmap = SEMMAP,                                           
	.semmni = SEMMNI,                                        
	.semmns = SEMMNS,                                       
	.semmnu = SEMMNU,                                            
	.semmsl = SEMMSL,                                        
	.semopm = SEMOPM,                                                
	.semume = SEMUME,                                               
	.semusz = SEMUSZ,                                             
	.semvmx = SEMVMX,                                     
	.semaem = SEMAEM                                       
};

                                                                       
                                                                             
                                                 
   
struct seminfo seminfo = {
	.semmap = SEMMAP,                                                  
	.semmni = 0,                                            
	.semmns = 0,                                           
	.semmnu = 0,                                             
	.semmsl = SEMMSL,                                       
	.semopm = SEMOPM,                                               
	.semume = SEMUME,                                              
	.semusz = SEMUSZ,                                            
	.semvmx = SEMVMX,                                    
	.semaem = SEMAEM                                      
};


static int semu_alloc(struct proc *p);
static int semundo_adjust(struct proc *p, int *supidx,
    int semid, int semnum, int adjval);
static void semundo_clear(int semid, int semnum);

                                                      
static sy_call_t* const semcalls[] = {
	(sy_call_t *)semctl, (sy_call_t *)semget,
	(sy_call_t *)semop
};

static int              semtot = 0;                                       
struct semid_kernel     *sema = NULL;                                  
struct sem              *sem_pool =  NULL;                          
static int              semu_list_idx = -1;                                 
struct sem_undo         *semu = NULL;                                    


static LCK_GRP_DECLARE(sysv_sem_subsys_lck_grp, "sysv_sem_subsys_lock");
static LCK_MTX_DECLARE(sysv_sem_subsys_mutex, &sysv_sem_subsys_lck_grp);

#define SYSV_SEM_SUBSYS_LOCK() lck_mtx_lock(&sysv_sem_subsys_mutex)
#define SYSV_SEM_SUBSYS_UNLOCK() lck_mtx_unlock(&sysv_sem_subsys_mutex)

static __inline__ user_time_t
sysv_semtime(void)
{
	struct timeval  tv;
	microtime(&tv);
	return tv.tv_sec;
}

  
                                                                  
                                                                 
                                             
  
                                                                          
  
                                                                 
   
static void
semid_ds_kernelto32(struct user_semid_ds *in, struct user32_semid_ds *out)
{
	out->sem_perm = in->sem_perm;
	out->sem_base = CAST_DOWN_EXPLICIT(__int32_t, in->sem_base);
	out->sem_nsems = in->sem_nsems;
	out->sem_otime = in->sem_otime;                                  
	out->sem_ctime = in->sem_ctime;                                  
}

static void
semid_ds_kernelto64(struct user_semid_ds *in, struct user64_semid_ds *out)
{
	out->sem_perm = in->sem_perm;
	out->sem_base = CAST_DOWN_EXPLICIT(__int32_t, in->sem_base);
	out->sem_nsems = in->sem_nsems;
	out->sem_otime = in->sem_otime;                                  
	out->sem_ctime = in->sem_ctime;                                  
}

  
                                                                          
  
                                                                             
                                                                           
                 
  
                                                                          
                   
   
static void
semid_ds_32tokernel(struct user32_semid_ds *in, struct user_semid_ds *out)
{
	out->sem_ctime = in->sem_ctime;
	out->sem_otime = in->sem_otime;
	out->sem_nsems = in->sem_nsems;
	out->sem_base = (void *)(uintptr_t)in->sem_base;
	out->sem_perm = in->sem_perm;
}

static void
semid_ds_64tokernel(struct user64_semid_ds *in, struct user_semid_ds *out)
{
	out->sem_ctime = in->sem_ctime;
	out->sem_otime = in->sem_otime;
	out->sem_nsems = in->sem_nsems;
	out->sem_base = (void *)(uintptr_t)in->sem_base;
	out->sem_perm = in->sem_perm;
}


  
         
  
                                                       
  
                                            
                                                        
                                                            
  
                                                                                   
                                                         
  
                     
                  
  
                                                                 
  
                                                                       
                                                                       
                                                         
  
   
int
semsys(struct proc *p, struct semsys_args *uap, int32_t *retval)
{
	                                                   

	if (uap->which >= sizeof(semcalls) / sizeof(semcalls[0])) {
		return EINVAL;
	}
	return (*semcalls[uap->which])(p, &uap->a2, retval);
}

  
                                                                       
                                
  
                                              
   
static int
grow_semu_array(int newSize)
{
	int i;
	struct sem_undo *newSemu;

	if (newSize <= seminfo.semmnu) {
		return 1;
	}
	if (newSize > limitseminfo.semmnu) {                         
#ifdef SEM_DEBUG
		printf("undo structure hard limit of %d reached, requested %d\n",
		    limitseminfo.semmnu, newSize);
#endif
		return 0;
	}
	newSize = (newSize / SEMMNU_INC + 1) * SEMMNU_INC;
	newSize = newSize > limitseminfo.semmnu ? limitseminfo.semmnu : newSize;

#ifdef SEM_DEBUG
	printf("growing semu[] from %d to %d\n", seminfo.semmnu, newSize);
#endif
	newSemu = kheap_alloc(KM_SYSVSEM, sizeof(struct sem_undo) * newSize,
	    Z_WAITOK | Z_ZERO);
	if (NULL == newSemu) {
#ifdef SEM_DEBUG
		printf("allocation failed.  no changes made.\n");
#endif
		return 0;
	}

	                                        
	for (i = 0; i < seminfo.semmnu; i++) {
		newSemu[i] = semu[i];
	}
	  
                                                                       
                                                                    
                                                     
    

	                            
	kheap_free(KM_SYSVSEM, semu, sizeof(struct sem_undo) * seminfo.semmnu);

	semu = newSemu;
	seminfo.semmnu = newSize;
#ifdef SEM_DEBUG
	printf("expansion successful\n");
#endif
	return 1;
}

  
                                                                       
                                      
  
                                              
   
static int
grow_sema_array(int newSize)
{
	struct semid_kernel *newSema;
	int i;

	if (newSize <= seminfo.semmni) {
		return 0;
	}
	if (newSize > limitseminfo.semmni) {                         
#ifdef SEM_DEBUG
		printf("identifier hard limit of %d reached, requested %d\n",
		    limitseminfo.semmni, newSize);
#endif
		return 0;
	}
	newSize = (newSize / SEMMNI_INC + 1) * SEMMNI_INC;
	newSize = newSize > limitseminfo.semmni ? limitseminfo.semmni : newSize;

#ifdef SEM_DEBUG
	printf("growing sema[] from %d to %d\n", seminfo.semmni, newSize);
#endif
	newSema = kheap_alloc(KM_SYSVSEM, sizeof(struct semid_kernel) * newSize,
	    Z_WAITOK | Z_ZERO);
	if (NULL == newSema) {
#ifdef SEM_DEBUG
		printf("allocation failed.  no changes made.\n");
#endif
		return 0;
	}

	                           
	for (i = 0; i < seminfo.semmni; i++) {
		newSema[i] = sema[i];
		                                                     
                                                   
                                                     
                                                   
                                                         
                                                        
                                                            
     
		if (sema[i].u.sem_perm.mode & SEM_ALLOC) {
			wakeup((caddr_t)&sema[i]);
		}
	}

#if CONFIG_MACF
	for (i = seminfo.semmni; i < newSize; i++) {
		mac_sysvsem_label_init(&newSema[i]);
	}
#endif

	  
                                                                       
                                                                     
                                                                         
    

	                            
	kheap_free(KM_SYSVSEM, sema,
	    sizeof(struct semid_kernel) * seminfo.semmni);

	sema = newSema;
	seminfo.semmni = newSize;
#ifdef SEM_DEBUG
	printf("expansion successful\n");
#endif
	return 1;
}

  
                                                                           
                                                       
  
                                              
   
static int
grow_sem_pool(int new_pool_size)
{
	struct sem *new_sem_pool = NULL;
	struct sem *sem_free;
	int i;

	if (new_pool_size < semtot) {
		return 0;
	}
	                        
	if (new_pool_size > limitseminfo.semmns) {
#ifdef SEM_DEBUG
		printf("semaphore hard limit of %d reached, requested %d\n",
		    limitseminfo.semmns, new_pool_size);
#endif
		return 0;
	}

	new_pool_size = (new_pool_size / SEMMNS_INC + 1) * SEMMNS_INC;
	new_pool_size = new_pool_size > limitseminfo.semmns ? limitseminfo.semmns : new_pool_size;

#ifdef SEM_DEBUG
	printf("growing sem_pool array from %d to %d\n", seminfo.semmns, new_pool_size);
#endif
	new_sem_pool = kheap_alloc(KM_SYSVSEM, sizeof(struct sem) * new_pool_size,
	    Z_WAITOK | Z_ZERO);
	if (NULL == new_sem_pool) {
#ifdef SEM_DEBUG
		printf("allocation failed.  no changes made.\n");
#endif
		return 0;
	}

	                                                            
	if (sem_pool) {
		for (i = 0; i < seminfo.semmns; i++) {
			new_sem_pool[i] = sem_pool[i];
		}
	}

	                                                             
	for (i = 0; i < seminfo.semmni; i++) {
		if (sema[i].u.sem_perm.mode & SEM_ALLOC) {                
			sema[i].u.sem_base = new_sem_pool +
			    (sema[i].u.sem_base - sem_pool);
		}
	}

	sem_free = sem_pool;
	sem_pool = new_sem_pool;

	                            
	kheap_free(KM_SYSVSEM, sem_free, sizeof(struct sem) * seminfo.semmns);

	seminfo.semmns = new_pool_size;
#ifdef SEM_DEBUG
	printf("expansion complete\n");
#endif
	return 1;
}

  
                                                  
                                                     
  
                                              
   

static int
semu_alloc(struct proc *p)
{
	int i;
	struct sem_undo *suptr;
	int *supidx;
	int attempt;

	  
                                    
                                                             
                                 
    

	for (attempt = 0; attempt < 2; attempt++) {
		  
                               
                                             
     

		for (i = 0; i < seminfo.semmnu; i++) {
			suptr = SEMU(i);
			if (suptr->un_proc == NULL) {
				suptr->un_next_idx = semu_list_idx;
				semu_list_idx = i;
				suptr->un_cnt = 0;
				suptr->un_ent = NULL;
				suptr->un_proc = p;
				return i;
			}
		}

		  
                                                            
                                      
     

		if (attempt == 0) {
			                                                      
			int did_something = 0;

			supidx = &semu_list_idx;
			while (*supidx != -1) {
				suptr = SEMU(*supidx);
				if (suptr->un_cnt == 0) {
					suptr->un_proc = NULL;
					*supidx = suptr->un_next_idx;
					did_something = 1;
				} else {
					supidx = &(suptr->un_next_idx);
				}
			}

			                                            
                                             
                                           
                                           
      
			if (!did_something) {
				if (!grow_semu_array(seminfo.semmnu + 1)) {
					return -1;
				}
			}
		} else {
			  
                                                 
                                     
                         
      
			panic("semu_alloc - second attempt failed");
		}
	}
	return -1;
}

  
                                                  
  
                                              
   
static int
semundo_adjust(struct proc *p, int *supidx, int semid,
    int semnum, int adjval)
{
	struct sem_undo *suptr;
	int suidx;
	struct undo *sueptr, **suepptr, *new_sueptr;
	int i;

	  
                                                                       
    

	suidx = *supidx;
	if (suidx == -1) {
		for (suidx = semu_list_idx; suidx != -1;
		    suidx = suptr->un_next_idx) {
			suptr = SEMU(suidx);
			if (suptr->un_proc == p) {
				*supidx = suidx;
				break;
			}
		}
		if (suidx == -1) {
			if (adjval == 0) {
				return 0;
			}
			suidx = semu_alloc(p);
			if (suidx == -1) {
				return ENOSPC;
			}
			*supidx = suidx;
		}
	}

	  
                                                                        
       
    
	suptr = SEMU(suidx);
	new_sueptr = NULL;
	for (i = 0, suepptr = &suptr->un_ent, sueptr = suptr->un_ent;
	    i < suptr->un_cnt;
	    i++, suepptr = &sueptr->une_next, sueptr = sueptr->une_next) {
		if (sueptr->une_id != semid || sueptr->une_num != semnum) {
			continue;
		}
		if (adjval == 0) {
			sueptr->une_adjval = 0;
		} else {
			sueptr->une_adjval += adjval;
		}
		if (sueptr->une_adjval == 0) {
			suptr->un_cnt--;
			*suepptr = sueptr->une_next;
			kheap_free(KM_SYSVSEM, sueptr, sizeof(struct undo));
		}
		return 0;
	}

	                                             
	if (adjval == 0) {
		                                            
		return 0;
	}

	if (suptr->un_cnt == limitseminfo.semume) {
		                                                        
		return EINVAL;
	}

	                                         
	new_sueptr = kheap_alloc(KM_SYSVSEM, sizeof(struct undo), Z_WAITOK);
	if (new_sueptr == NULL) {
		return ENOMEM;
	}

	                                          
	new_sueptr->une_next = suptr->un_ent;
	suptr->un_ent = new_sueptr;
	suptr->un_cnt++;
	new_sueptr->une_adjval = adjval;
	new_sueptr->une_id = semid;
	new_sueptr->une_num = semnum;

	return 0;
}

                                              
   
static void
semundo_clear(int semid, int semnum)
{
	struct sem_undo *suptr;
	int suidx;

	for (suidx = semu_list_idx; suidx != -1; suidx = suptr->un_next_idx) {
		struct undo *sueptr;
		struct undo **suepptr;
		int i = 0;

		suptr = SEMU(suidx);
		sueptr = suptr->un_ent;
		suepptr = &suptr->un_ent;
		while (i < suptr->un_cnt) {
			if (sueptr->une_id == semid) {
				if (semnum == -1 || sueptr->une_num == semnum) {
					suptr->un_cnt--;
					*suepptr = sueptr->une_next;
					kheap_free(KM_SYSVSEM, sueptr, sizeof(struct undo));
					sueptr = *suepptr;
					continue;
				}
				if (semnum != -1) {
					break;
				}
			}
			i++;
			suepptr = &sueptr->une_next;
			sueptr = sueptr->une_next;
		}
	}
}

  
                                                                   
                                                                   
                                                               
                                                                      
                                                              
   
int
semctl(struct proc *p, struct semctl_args *uap, int32_t *retval)
{
	int semid = uap->semid;
	int semnum = uap->semnum;
	int cmd = uap->cmd;
	user_semun_t user_arg = (user_semun_t)uap->arg;
	kauth_cred_t cred = kauth_cred_get();
	int i, rval, eval;
	struct user_semid_ds sbuf;
	struct semid_kernel *semakptr;


	AUDIT_ARG(svipc_cmd, cmd);
	AUDIT_ARG(svipc_id, semid);

	SYSV_SEM_SUBSYS_LOCK();

#ifdef SEM_DEBUG
	printf("call to semctl(%d, %d, %d, 0x%qx)\n", semid, semnum, cmd, user_arg);
#endif

	semid = IPCID_TO_IX(semid);

	if (semid < 0 || semid >= seminfo.semmni) {
#ifdef SEM_DEBUG
		printf("Invalid semid\n");
#endif
		eval = EINVAL;
		goto semctlout;
	}

	semakptr = &sema[semid];
	if ((semakptr->u.sem_perm.mode & SEM_ALLOC) == 0 ||
	    semakptr->u.sem_perm._seq != IPCID_TO_SEQ(uap->semid)) {
		eval = EINVAL;
		goto semctlout;
	}
#if CONFIG_MACF
	eval = mac_sysvsem_check_semctl(cred, semakptr, cmd);
	if (eval) {
		goto semctlout;
	}
#endif

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_M))) {
			goto semctlout;
		}

		semakptr->u.sem_perm.cuid = kauth_cred_getuid(cred);
		semakptr->u.sem_perm.uid = kauth_cred_getuid(cred);
		semtot -= semakptr->u.sem_nsems;
		for (i = semakptr->u.sem_base - sem_pool; i < semtot; i++) {
			sem_pool[i] = sem_pool[i + semakptr->u.sem_nsems];
		}
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].u.sem_perm.mode & SEM_ALLOC) &&
			    sema[i].u.sem_base > semakptr->u.sem_base) {
				sema[i].u.sem_base -= semakptr->u.sem_nsems;
			}
		}
		semakptr->u.sem_perm.mode = 0;
#if CONFIG_MACF
		mac_sysvsem_label_recycle(semakptr);
#endif
		semundo_clear(semid, -1);
		wakeup((caddr_t)semakptr);
		break;

	case IPC_SET:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_M))) {
			goto semctlout;
		}

		if (IS_64BIT_PROCESS(p)) {
			struct user64_semid_ds ds64;
			eval = copyin(user_arg.buf, &ds64, sizeof(ds64));
			semid_ds_64tokernel(&ds64, &sbuf);
		} else {
			struct user32_semid_ds ds32;
			eval = copyin(user_arg.buf, &ds32, sizeof(ds32));
			semid_ds_32tokernel(&ds32, &sbuf);
		}

		if (eval != 0) {
			goto semctlout;
		}

		semakptr->u.sem_perm.uid = sbuf.sem_perm.uid;
		semakptr->u.sem_perm.gid = sbuf.sem_perm.gid;
		semakptr->u.sem_perm.mode = (semakptr->u.sem_perm.mode &
		    ~0777) | (sbuf.sem_perm.mode & 0777);
		semakptr->u.sem_ctime = sysv_semtime();
		break;

	case IPC_STAT:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}

		if (IS_64BIT_PROCESS(p)) {
			struct user64_semid_ds semid_ds64;
			bzero(&semid_ds64, sizeof(semid_ds64));
			semid_ds_kernelto64(&semakptr->u, &semid_ds64);
			eval = copyout(&semid_ds64, user_arg.buf, sizeof(semid_ds64));
		} else {
			struct user32_semid_ds semid_ds32;
			bzero(&semid_ds32, sizeof(semid_ds32));
			semid_ds_kernelto32(&semakptr->u, &semid_ds32);
			eval = copyout(&semid_ds32, user_arg.buf, sizeof(semid_ds32));
		}
		break;

	case GETNCNT:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}
		if (semnum < 0 || semnum >= semakptr->u.sem_nsems) {
			eval = EINVAL;
			goto semctlout;
		}
		rval = semakptr->u.sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}
		if (semnum < 0 || semnum >= semakptr->u.sem_nsems) {
			eval = EINVAL;
			goto semctlout;
		}
		rval = semakptr->u.sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}
		if (semnum < 0 || semnum >= semakptr->u.sem_nsems) {
			eval = EINVAL;
			goto semctlout;
		}
		rval = semakptr->u.sem_base[semnum].semval;
		break;

	case GETALL:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}
                                           
		for (i = 0; i < semakptr->u.sem_nsems; i++) {
			                                    
			eval = copyout((caddr_t)&semakptr->u.sem_base[i].semval,
			    user_arg.array + (i * sizeof(unsigned short)),
			    sizeof(unsigned short));
			if (eval != 0) {
				break;
			}
		}
		break;

	case GETZCNT:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_R))) {
			goto semctlout;
		}
		if (semnum < 0 || semnum >= semakptr->u.sem_nsems) {
			eval = EINVAL;
			goto semctlout;
		}
		rval = semakptr->u.sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_W))) {
#ifdef SEM_DEBUG
			printf("Invalid credentials for write\n");
#endif
			goto semctlout;
		}
		if (semnum < 0 || semnum >= semakptr->u.sem_nsems) {
#ifdef SEM_DEBUG
			printf("Invalid number out of range for set\n");
#endif
			eval = EINVAL;
			goto semctlout;
		}

		  
                                                               
                                                            
                                      
     
		u_int newsemval = CAST_DOWN_EXPLICIT(u_int, user_arg.buf);

		  
                                                             
                         
     
		if (newsemval > (u_int)seminfo.semvmx) {
#ifdef SEM_DEBUG
			printf("Out of range sem value for set\n");
#endif
			eval = ERANGE;
			goto semctlout;
		}
		semakptr->u.sem_base[semnum].semval = newsemval;
		semakptr->u.sem_base[semnum].sempid = p->p_pid;
		                                                 
		semundo_clear(semid, semnum);
		wakeup((caddr_t)semakptr);
		break;

	case SETALL:
		if ((eval = ipcperm(cred, &semakptr->u.sem_perm, IPC_W))) {
			goto semctlout;
		}
                               
		for (i = 0; i < semakptr->u.sem_nsems; i++) {
			                                    
			eval = copyin(user_arg.array + (i * sizeof(unsigned short)),
			    (caddr_t)&semakptr->u.sem_base[i].semval,
			    sizeof(unsigned short));
			if (eval != 0) {
				break;
			}
			semakptr->u.sem_base[i].sempid = p->p_pid;
		}
		                                                 
		semundo_clear(semid, -1);
		wakeup((caddr_t)semakptr);
		break;

	default:
		eval = EINVAL;
		goto semctlout;
	}

	if (eval == 0) {
		*retval = rval;
	}
semctlout:
	SYSV_SEM_SUBSYS_UNLOCK();
	return eval;
}

int
semget(__unused struct proc *p, struct semget_args *uap, int32_t *retval)
{
	int semid, eval;
	int key = uap->key;
	int nsems = uap->nsems;
	int semflg = uap->semflg;
	kauth_cred_t cred = kauth_cred_get();

#ifdef SEM_DEBUG
	if (key != IPC_PRIVATE) {
		printf("semget(0x%x, %d, 0%o)\n", key, nsems, semflg);
	} else {
		printf("semget(IPC_PRIVATE, %d, 0%o)\n", nsems, semflg);
	}
#endif


	SYSV_SEM_SUBSYS_LOCK();


	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].u.sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].u.sem_perm._key == key) {
				break;
			}
		}
		if (semid < seminfo.semmni) {
#ifdef SEM_DEBUG
			printf("found public key\n");
#endif
			if ((eval = ipcperm(cred, &sema[semid].u.sem_perm,
			    semflg & 0700))) {
				goto semgetout;
			}
			if (nsems < 0 || sema[semid].u.sem_nsems < nsems) {
#ifdef SEM_DEBUG
				printf("too small\n");
#endif
				eval = EINVAL;
				goto semgetout;
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
#ifdef SEM_DEBUG
				printf("not exclusive\n");
#endif
				eval = EEXIST;
				goto semgetout;
			}
#if CONFIG_MACF
			eval = mac_sysvsem_check_semget(cred, &sema[semid]);
			if (eval) {
				goto semgetout;
			}
#endif
			goto found;
		}
	}

#ifdef SEM_DEBUG
	printf("need to allocate an id for the request\n");
#endif
	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > limitseminfo.semmsl) {
#ifdef SEM_DEBUG
			printf("nsems out of range (0<%d<=%d)\n", nsems,
			    seminfo.semmsl);
#endif
			eval = EINVAL;
			goto semgetout;
		}
		if (nsems > seminfo.semmns - semtot) {
#ifdef SEM_DEBUG
			printf("not enough semaphores left (need %d, got %d)\n",
			    nsems, seminfo.semmns - semtot);
#endif
			if (!grow_sem_pool(semtot + nsems)) {
#ifdef SEM_DEBUG
				printf("failed to grow the sem array\n");
#endif
				eval = ENOSPC;
				goto semgetout;
			}
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].u.sem_perm.mode & SEM_ALLOC) == 0) {
				break;
			}
		}
		if (semid == seminfo.semmni) {
#ifdef SEM_DEBUG
			printf("no more id's available\n");
#endif
			if (!grow_sema_array(seminfo.semmni + 1)) {
#ifdef SEM_DEBUG
				printf("failed to grow sema array\n");
#endif
				eval = ENOSPC;
				goto semgetout;
			}
		}
#ifdef SEM_DEBUG
		printf("semid %d is available\n", semid);
#endif
		sema[semid].u.sem_perm._key = key;
		sema[semid].u.sem_perm.cuid = kauth_cred_getuid(cred);
		sema[semid].u.sem_perm.uid = kauth_cred_getuid(cred);
		sema[semid].u.sem_perm.cgid = kauth_cred_getgid(cred);
		sema[semid].u.sem_perm.gid = kauth_cred_getgid(cred);
		sema[semid].u.sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].u.sem_perm._seq =
		    (sema[semid].u.sem_perm._seq + 1) & 0x7fff;
		sema[semid].u.sem_nsems = nsems;
		sema[semid].u.sem_otime = 0;
		sema[semid].u.sem_ctime = sysv_semtime();
		sema[semid].u.sem_base = &sem_pool[semtot];
		semtot += nsems;
		bzero(sema[semid].u.sem_base,
		    sizeof(sema[semid].u.sem_base[0]) * nsems);
#if CONFIG_MACF
		mac_sysvsem_label_associate(cred, &sema[semid]);
#endif
#ifdef SEM_DEBUG
		printf("sembase = 0x%x, next = 0x%x\n", sema[semid].u.sem_base,
		    &sem_pool[semtot]);
#endif
	} else {
#ifdef SEM_DEBUG
		printf("didn't find it and wasn't asked to create it\n");
#endif
		eval = ENOENT;
		goto semgetout;
	}

found:
	*retval = IXSEQ_TO_IPCID(semid, sema[semid].u.sem_perm);
	AUDIT_ARG(svipc_id, *retval);
#ifdef SEM_DEBUG
	printf("semget is done, returning %d\n", *retval);
#endif
	eval = 0;

semgetout:
	SYSV_SEM_SUBSYS_UNLOCK();
	return eval;
}

int
semop(struct proc *p, struct semop_args *uap, int32_t *retval)
{
	int semid = uap->semid;
	int nsops = uap->nsops;
	struct sembuf sops[seminfo.semopm];
	struct semid_kernel *semakptr;
	struct sembuf *sopptr = NULL;                              
	struct sem *semptr = NULL;                             
	int supidx = -1;
	int i, j, eval;
	int do_wakeup, do_undos;

	AUDIT_ARG(svipc_id, uap->semid);

	SYSV_SEM_SUBSYS_LOCK();

#ifdef SEM_DEBUG
	printf("call to semop(%d, 0x%x, %d)\n", semid, sops, nsops);
#endif

	semid = IPCID_TO_IX(semid);                                      

	if (semid < 0 || semid >= seminfo.semmni) {
		eval = EINVAL;
		goto semopout;
	}

	semakptr = &sema[semid];
	if ((semakptr->u.sem_perm.mode & SEM_ALLOC) == 0) {
		eval = EINVAL;
		goto semopout;
	}
	if (semakptr->u.sem_perm._seq != IPCID_TO_SEQ(uap->semid)) {
		eval = EINVAL;
		goto semopout;
	}

	if ((eval = ipcperm(kauth_cred_get(), &semakptr->u.sem_perm, IPC_W))) {
#ifdef SEM_DEBUG
		printf("eval = %d from ipaccess\n", eval);
#endif
		goto semopout;
	}

	if (nsops < 0 || nsops > seminfo.semopm) {
#ifdef SEM_DEBUG
		printf("too many sops (max=%d, nsops=%d)\n",
		    seminfo.semopm, nsops);
#endif
		eval = E2BIG;
		goto semopout;
	}

	                                                                      
	if ((eval = copyin(uap->sops, &sops, nsops * sizeof(struct sembuf))) != 0) {
#ifdef SEM_DEBUG
		printf("eval = %d from copyin(%08x, %08x, %ld)\n", eval,
		    uap->sops, &sops, nsops * sizeof(struct sembuf));
#endif
		goto semopout;
	}

#if CONFIG_MACF
	  
                                                              
    
	j = 0;                                 
	for (i = 0; i < nsops; i++) {
		j |= (sops[i].sem_op == 0) ? SEM_R : SEM_A;
	}

	  
                                                                 
                                                          
                         
    
	eval = mac_sysvsem_check_semop(kauth_cred_get(), semakptr, j);
	if (eval) {
		goto semopout;
	}
#endif

	  
                                                  
                                                                
                                                                 
                                                                 
   
                                                                
                                                      
    
	do_undos = 0;

	for (;;) {
		do_wakeup = 0;

		for (i = 0; i < nsops; i++) {
			sopptr = &sops[i];

			if (sopptr->sem_num >= semakptr->u.sem_nsems) {
				eval = EFBIG;
				goto semopout;
			}

			semptr = &semakptr->u.sem_base[sopptr->sem_num];

#ifdef SEM_DEBUG
			printf("semop:  semakptr=%x, sem_base=%x, semptr=%x, sem[%d]=%d : op=%d, flag=%s\n",
			    semakptr, semakptr->u.sem_base, semptr,
			    sopptr->sem_num, semptr->semval, sopptr->sem_op,
			    (sopptr->sem_flg & IPC_NOWAIT) ? "nowait" : "wait");
#endif

			if (sopptr->sem_op < 0) {
				if (semptr->semval + sopptr->sem_op < 0) {
#ifdef SEM_DEBUG
					printf("semop:  can't do it now\n");
#endif
					break;
				} else {
					semptr->semval += sopptr->sem_op;
					if (semptr->semval == 0 &&
					    semptr->semzcnt > 0) {
						do_wakeup = 1;
					}
				}
				if (sopptr->sem_flg & SEM_UNDO) {
					do_undos = 1;
				}
			} else if (sopptr->sem_op == 0) {
				if (semptr->semval > 0) {
#ifdef SEM_DEBUG
					printf("semop:  not zero now\n");
#endif
					break;
				}
			} else {
				if (semptr->semncnt > 0) {
					do_wakeup = 1;
				}
				semptr->semval += sopptr->sem_op;
				if (sopptr->sem_flg & SEM_UNDO) {
					do_undos = 1;
				}
			}
		}

		  
                                          
     
		if (i >= nsops) {
			goto done;
		}

		  
                                                     
     
#ifdef SEM_DEBUG
		printf("semop:  rollback 0 through %d\n", i - 1);
#endif
		for (j = 0; j < i; j++) {
			semakptr->u.sem_base[sops[j].sem_num].semval -=
			    sops[j].sem_op;
		}

		  
                                                    
                                             
     
		if (sopptr->sem_flg & IPC_NOWAIT) {
			eval = EAGAIN;
			goto semopout;
		}

		if (sopptr->sem_op == 0) {
			semptr->semzcnt++;
		} else {
			semptr->semncnt++;
		}

#ifdef SEM_DEBUG
		printf("semop:  good night!\n");
#endif
		                                                 
                                                   
                                                    
             
     
		eval = msleep((caddr_t)semakptr, &sysv_sem_subsys_mutex, (PZERO - 4) | PCATCH,
		    "semwait", 0);

#ifdef SEM_DEBUG
		printf("semop:  good morning (eval=%d)!\n", eval);
#endif
		if (eval != 0) {
			eval = EINTR;
		}

		  
                                                               
                                                                  
                                                        
                                     
     

		semptr = NULL;

		  
                                              
    
                                                               
                                                           
     
		semakptr = &sema[semid];                                       
		if ((semakptr->u.sem_perm.mode & SEM_ALLOC) == 0 ||
		    semakptr->u.sem_perm._seq != IPCID_TO_SEQ(uap->semid) ||
		    sopptr->sem_num >= semakptr->u.sem_nsems) {
			                                        
			                                                  
			if (eval == EINTR) {
				  
                                                
                                              
                  
       
			} else {
#ifdef EIDRM
				eval = EIDRM;
#else
				eval = EINVAL;                            
#endif
			}
			goto semopout;
		}

		  
                                                         
                                                     
                                                      
                                                     
     
		semptr = &semakptr->u.sem_base[sopptr->sem_num];
		if (sopptr->sem_op == 0) {
			semptr->semzcnt--;
		} else {
			semptr->semncnt--;
		}

		if (eval != 0) {            
			goto semopout;
		}
	}

done:
	  
                                  
    
	if (do_undos) {
		for (i = 0; i < nsops; i++) {
			  
                                                       
           
      
			int adjval;

			if ((sops[i].sem_flg & SEM_UNDO) == 0) {
				continue;
			}
			adjval = sops[i].sem_op;
			if (adjval == 0) {
				continue;
			}
			eval = semundo_adjust(p, &supidx, semid,
			    sops[i].sem_num, -adjval);
			if (eval == 0) {
				continue;
			}

			  
                                                        
                                                     
                                                          
                                                     
                                                         
                                                         
                                              
      
			for (j = i - 1; j >= 0; j--) {
				if ((sops[j].sem_flg & SEM_UNDO) == 0) {
					continue;
				}
				adjval = sops[j].sem_op;
				if (adjval == 0) {
					continue;
				}
				if (semundo_adjust(p, &supidx, semid,
				    sops[j].sem_num, adjval) != 0) {
					panic("semop - can't undo undos");
				}
			}

			for (j = 0; j < nsops; j++) {
				semakptr->u.sem_base[sops[j].sem_num].semval -=
				    sops[j].sem_op;
			}

#ifdef SEM_DEBUG
			printf("eval = %d from semundo_adjust\n", eval);
#endif
			goto semopout;
		}                            
	}                    

	                                              
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semakptr->u.sem_base[sopptr->sem_num];
		semptr->sempid = p->p_pid;
	}
	semakptr->u.sem_otime = sysv_semtime();

	if (do_wakeup) {
#ifdef SEM_DEBUG
		printf("semop:  doing wakeup\n");
#ifdef SEM_WAKEUP
		sem_wakeup((caddr_t)semakptr);
#else
		wakeup((caddr_t)semakptr);
#endif
		printf("semop:  back from wakeup\n");
#else
		wakeup((caddr_t)semakptr);
#endif
	}
#ifdef SEM_DEBUG
	printf("semop:  done\n");
#endif
	*retval = 0;
	eval = 0;
semopout:
	SYSV_SEM_SUBSYS_UNLOCK();
	return eval;
}

  
                                                                               
              
   
void
semexit(struct proc *p)
{
	struct sem_undo *suptr = NULL;
	int suidx;
	int *supidx;
	int did_something;

	                                                             
                                                     
                                   
    
	SYSV_SEM_SUBSYS_LOCK();

	if (!sem_pool) {
		SYSV_SEM_SUBSYS_UNLOCK();
		return;
	}
	did_something = 0;

	  
                                                        
                                 
    

	for (supidx = &semu_list_idx; (suidx = *supidx) != -1;
	    supidx = &suptr->un_next_idx) {
		suptr = SEMU(suidx);
		if (suptr->un_proc == p) {
			break;
		}
	}

	if (suidx == -1) {
		goto unlock;
	}

#ifdef SEM_DEBUG
	printf("proc @%08x has undo structure with %d entries\n", p,
	    suptr->un_cnt);
#endif

	  
                                                            
    
	if (suptr->un_cnt > 0) {
		while (suptr->un_ent != NULL) {
			struct undo *sueptr;
			int semid;
			int semnum;
			int adjval;
			struct semid_kernel *semakptr;

			sueptr = suptr->un_ent;
			semid = sueptr->une_id;
			semnum = sueptr->une_num;
			adjval = sueptr->une_adjval;

			semakptr = &sema[semid];
			if ((semakptr->u.sem_perm.mode & SEM_ALLOC) == 0) {
				panic("semexit - semid not allocated");
			}
			if (semnum >= semakptr->u.sem_nsems) {
				panic("semexit - semnum out of range");
			}

#ifdef SEM_DEBUG
			printf("semexit:  %08x id=%d num=%d(adj=%d) ; sem=%d\n",
			    suptr->un_proc,
			    semid,
			    semnum,
			    adjval,
			    semakptr->u.sem_base[semnum].semval);
#endif

			if (adjval < 0) {
				if (semakptr->u.sem_base[semnum].semval < -adjval) {
					semakptr->u.sem_base[semnum].semval = 0;
				} else {
					semakptr->u.sem_base[semnum].semval +=
					    adjval;
				}
			} else {
				semakptr->u.sem_base[semnum].semval += adjval;
			}

			                                                     
                                                           
                                                            
                     
      
#ifdef SEM_WAKEUP
			sem_wakeup((caddr_t)semakptr);
#else
			wakeup((caddr_t)semakptr);
#endif
#ifdef SEM_DEBUG
			printf("semexit:  back from wakeup\n");
#endif
			suptr->un_cnt--;
			suptr->un_ent = sueptr->une_next;
			kheap_free(KM_SYSVSEM, sueptr, sizeof(struct undo));
		}
	}

	  
                               
    
#ifdef SEM_DEBUG
	printf("removing vector\n");
#endif
	suptr->un_proc = NULL;
	*supidx = suptr->un_next_idx;

unlock:
	  
                                                              
                                                                  
                                                                         
                                                                    
                                                                     
                                                                   
                                                                      
                                                                    
                                                                     
                                                                
                                                                         
                                                                  
                                                           
   
                                                                        
                                 
    

	SYSV_SEM_SUBSYS_UNLOCK();
}


                                                    
                                   
static int
sysctl_seminfo(__unused struct sysctl_oid *oidp, void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	int error = 0;

	error = SYSCTL_OUT(req, arg1, sizeof(int));
	if (error || req->newptr == USER_ADDR_NULL) {
		return error;
	}

	SYSV_SEM_SUBSYS_LOCK();

	                                                             
	if ((sem_pool == NULL) &&
	    (sema == NULL) &&
	    (semu == NULL) &&
	    (semu_list_idx == -1)) {
		if ((error = SYSCTL_IN(req, arg1, sizeof(int)))) {
			goto out;
		}
	} else {
		error = EINVAL;
	}
out:
	SYSV_SEM_SUBSYS_UNLOCK();
	return error;
}

                                                                 
extern struct sysctl_oid_list sysctl__kern_sysv_children;
SYSCTL_PROC(_kern_sysv, OID_AUTO, semmni, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &limitseminfo.semmni, 0, &sysctl_seminfo, "I", "semmni");

SYSCTL_PROC(_kern_sysv, OID_AUTO, semmns, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &limitseminfo.semmns, 0, &sysctl_seminfo, "I", "semmns");

SYSCTL_PROC(_kern_sysv, OID_AUTO, semmnu, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &limitseminfo.semmnu, 0, &sysctl_seminfo, "I", "semmnu");

SYSCTL_PROC(_kern_sysv, OID_AUTO, semmsl, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &limitseminfo.semmsl, 0, &sysctl_seminfo, "I", "semmsl");

SYSCTL_PROC(_kern_sysv, OID_AUTO, semume, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &limitseminfo.semume, 0, &sysctl_seminfo, "I", "semume");


static int
IPCS_sem_sysctl(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	int error;
	int cursor;
	union {
		struct user32_IPCS_command u32;
		struct user_IPCS_command u64;
	} ipcs = { };
	struct user32_semid_ds semid_ds32 = { };                                      
	struct user64_semid_ds semid_ds64 = { };                                      
	void *semid_dsp;
	size_t ipcs_sz;
	size_t semid_ds_sz;
	struct proc *p = current_proc();

	if (IS_64BIT_PROCESS(p)) {
		ipcs_sz = sizeof(struct user_IPCS_command);
		semid_ds_sz = sizeof(struct user64_semid_ds);
	} else {
		ipcs_sz = sizeof(struct user32_IPCS_command);
		semid_ds_sz = sizeof(struct user32_semid_ds);
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

	SYSV_SEM_SUBSYS_LOCK();
	switch (ipcs.u64.ipcs_op) {
	case IPCS_SEM_CONF:                                           
		if (ipcs.u64.ipcs_datalen != sizeof(struct seminfo)) {
			error = ERANGE;
			break;
		}
		if (ipcs.u64.ipcs_cursor != 0) {                          
			error = EINVAL;
			break;
		}
		error = copyout(&seminfo, ipcs.u64.ipcs_data, ipcs.u64.ipcs_datalen);
		break;

	case IPCS_SEM_ITER:                                         
		cursor = ipcs.u64.ipcs_cursor;
		if (cursor < 0 || cursor >= seminfo.semmni) {
			error = ERANGE;
			break;
		}
		if (ipcs.u64.ipcs_datalen != (int)semid_ds_sz) {
			error = EINVAL;
			break;
		}
		for (; cursor < seminfo.semmni; cursor++) {
			if (sema[cursor].u.sem_perm.mode & SEM_ALLOC) {
				break;
			}
			continue;
		}
		if (cursor == seminfo.semmni) {
			error = ENOENT;
			break;
		}

		semid_dsp = &sema[cursor].u;                         

		  
                                                    
                                     
     
		if (!IS_64BIT_PROCESS(p)) {
			bzero(&semid_ds32, sizeof(semid_ds32));
			semid_ds_kernelto32(semid_dsp, &semid_ds32);
			semid_dsp = &semid_ds32;
		} else {
			bzero(&semid_ds64, sizeof(semid_ds64));
			semid_ds_kernelto64(semid_dsp, &semid_ds64);
			semid_dsp = &semid_ds64;
		}

		error = copyout(semid_dsp, ipcs.u64.ipcs_data, ipcs.u64.ipcs_datalen);
		if (!error) {
			                   
			ipcs.u64.ipcs_cursor = cursor + 1;

			if (!IS_64BIT_PROCESS(p)) {                           
				ipcs.u32.ipcs_data = CAST_DOWN_EXPLICIT(user32_addr_t, ipcs.u64.ipcs_data);
			}

			error = SYSCTL_OUT(req, &ipcs, ipcs_sz);
		}
		break;

	default:
		error = EINVAL;
		break;
	}
	SYSV_SEM_SUBSYS_UNLOCK();
	return error;
}

SYSCTL_DECL(_kern_sysv_ipcs);
SYSCTL_PROC(_kern_sysv_ipcs, OID_AUTO, sem, CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, IPCS_sem_sysctl,
    "S,IPCS_sem_command",
    "ipcs sem command interface");

#endif               
