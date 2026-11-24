/*
 * Copyright (c) 2004-2020 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   

  
                                                                           
                            
   

#include <sys/param.h>                         
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/sdt.h>

#include <security/audit/audit.h>

#include <sys/mount.h>
#include <sys/stat.h>                                                    
#include <sys/sysproto.h>
#include <mach/message.h>
#include <mach/host_security.h>

#include <machine/atomic.h>

#include <kern/task.h>
#include <kern/locks.h>
#ifdef MACH_ASSERT
# undef MACH_ASSERT
#endif
#define MACH_ASSERT 1                     
#include <kern/assert.h>

#if CONFIG_MACF
#include <security/mac.h>
#include <security/mac_framework.h>
#include <security/_label.h>
#endif

#include <os/hash.h>
#include <IOKit/IOBSD.h>

void mach_kauth_cred_uthread_update( void );

# define NULLCRED_CHECK(_c)     do {if (!IS_VALID_CRED(_c)) panic("%s: bad credential %p", __FUNCTION__,_c);} while(0)

                                                           
#if 0
#ifdef KAUTH_DEBUG
#undef KAUTH_DEBUG
#endif

#ifdef K_UUID_FMT
#undef K_UUID_FMT
#endif

#ifdef K_UUID_ARG
#undef K_UUID_ARG
#endif

# define K_UUID_FMT "%08x:%08x:%08x:%08x"
# define K_UUID_ARG(_u) &_u.g_guid_asint[0],&_u.g_guid_asint[1],&_u.g_guid_asint[2],&_u.g_guid_asint[3]
# define KAUTH_DEBUG(fmt, args...)      do { printf("%s:%d: " fmt "\n", __PRETTY_FUNCTION__, __LINE__ , ##args); } while (0)
#endif

  
                                                                      
                                                                       
          
  
                                                                   
   
#if DEBUG_CRED
#define DEBUG_CRED_ENTER                printf
#define DEBUG_CRED_CHANGE               printf
#else                    
#define DEBUG_CRED_ENTER(fmt, ...)      do {} while (0)
#define DEBUG_CRED_CHANGE(fmt, ...)     do {} while (0)
#endif                   

#if CONFIG_EXT_RESOLVER
  
                                           
  
                                                                           
                                                                         
                                                                           
             
   

static LCK_MTX_DECLARE(kauth_resolver_mtx, &kauth_lck_grp);
#define KAUTH_RESOLVER_LOCK()   lck_mtx_lock(&kauth_resolver_mtx);
#define KAUTH_RESOLVER_UNLOCK() lck_mtx_unlock(&kauth_resolver_mtx);

static volatile pid_t   kauth_resolver_identity;
static int      kauth_identitysvc_has_registered;
static int      kauth_resolver_registered;
static uint32_t kauth_resolver_sequence = 31337;
static int      kauth_resolver_timeout = 30;                             

struct kauth_resolver_work {
	TAILQ_ENTRY(kauth_resolver_work) kr_link;
	struct kauth_identity_extlookup kr_work;
	uint64_t        kr_extend;
	uint32_t        kr_seqno;
	int             kr_refs;
	int             kr_flags;
#define KAUTH_REQUEST_UNSUBMITTED       (1<<0)
#define KAUTH_REQUEST_SUBMITTED         (1<<1)
#define KAUTH_REQUEST_DONE              (1<<2)
	int             kr_result;
};

TAILQ_HEAD(kauth_resolver_unsubmitted_head, kauth_resolver_work) kauth_resolver_unsubmitted =
    TAILQ_HEAD_INITIALIZER(kauth_resolver_unsubmitted);
TAILQ_HEAD(kauth_resolver_submitted_head, kauth_resolver_work) kauth_resolver_submitted =
    TAILQ_HEAD_INITIALIZER(kauth_resolver_submitted);
TAILQ_HEAD(kauth_resolver_done_head, kauth_resolver_work) kauth_resolver_done =
    TAILQ_HEAD_INITIALIZER(kauth_resolver_done);

                                                           
#define KAUTH_COMPLAINT_INTERVAL 1000
int kauth_resolver_timeout_cnt = 0;

#if DEVELOPMENT || DEBUG
                                                                 
#define KAUTH_RESOLVER_FAILED_ERRCODE   EOWNERDEAD
#else
                                                                       
#define KAUTH_RESOLVER_FAILED_ERRCODE   EIO
#endif                           

int kauth_resolver_failed_cnt = 0;
#define RESOLVER_FAILED_MESSAGE(fmt, args...)                           \
do {                                                                    \
	if (!(kauth_resolver_failed_cnt++ % 100)) {                     \
	        printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ##args);   \
	}                                                               \
} while (0)

static int      kauth_resolver_submit(struct kauth_identity_extlookup *lkp, uint64_t extend_data);
static int      kauth_resolver_complete(user_addr_t message);
static int      kauth_resolver_getwork(user_addr_t message);
static int      kauth_resolver_getwork2(user_addr_t message);
static __attribute__((noinline)) int __KERNEL_IS_WAITING_ON_EXTERNAL_CREDENTIAL_RESOLVER__(
	struct kauth_resolver_work *);

#define KAUTH_CACHES_MAX_SIZE 10000                                                  

struct kauth_identity {
	TAILQ_ENTRY(kauth_identity) ki_link;
	int     ki_valid;
	uid_t   ki_uid;
	gid_t   ki_gid;
	uint32_t         ki_supgrpcnt;
	gid_t   ki_supgrps[NGROUPS];
	guid_t  ki_guid;
	ntsid_t ki_ntsid;
	const char      *ki_name;                                          
	  
                                                                     
                                                                     
                                                                   
                                                                      
              
    
	time_t  ki_groups_expiry;
	time_t  ki_guid_expiry;
	time_t  ki_ntsid_expiry;
};

static TAILQ_HEAD(kauth_identity_head, kauth_identity) kauth_identities =
    TAILQ_HEAD_INITIALIZER(kauth_identities);
static LCK_MTX_DECLARE(kauth_identity_mtx, &kauth_lck_grp);
#define KAUTH_IDENTITY_LOCK()   lck_mtx_lock(&kauth_identity_mtx);
#define KAUTH_IDENTITY_UNLOCK() lck_mtx_unlock(&kauth_identity_mtx);
#define KAUTH_IDENTITY_CACHEMAX_DEFAULT 100                              
static int kauth_identity_cachemax = KAUTH_IDENTITY_CACHEMAX_DEFAULT;
static int kauth_identity_count;

static struct kauth_identity *kauth_identity_alloc(uid_t uid, gid_t gid, guid_t *guidp, time_t guid_expiry,
    ntsid_t *ntsidp, time_t ntsid_expiry, size_t supgrpcnt, gid_t *supgrps, time_t groups_expiry,
    const char *name, int nametype);
static void     kauth_identity_register_and_free(struct kauth_identity *kip);
static void     kauth_identity_updatecache(struct kauth_identity_extlookup *elp, struct kauth_identity *kip, uint64_t extend_data);
static void     kauth_identity_trimcache(int newsize);
static void     kauth_identity_lru(struct kauth_identity *kip);
static int      kauth_identity_guid_expired(struct kauth_identity *kip);
static int      kauth_identity_ntsid_expired(struct kauth_identity *kip);
static int      kauth_identity_find_uid(uid_t uid, struct kauth_identity *kir, char *getname);
static int      kauth_identity_find_gid(gid_t gid, struct kauth_identity *kir, char *getname);
static int      kauth_identity_find_guid(guid_t *guidp, struct kauth_identity *kir, char *getname);
static int      kauth_identity_find_ntsid(ntsid_t *ntsid, struct kauth_identity *kir, char *getname);
static int      kauth_identity_find_nam(char *name, int valid, struct kauth_identity *kir);

struct kauth_group_membership {
	TAILQ_ENTRY(kauth_group_membership) gm_link;
	uid_t   gm_uid;                                                            
	gid_t   gm_gid;                                               
	time_t  gm_expiry;                                                               
	int     gm_flags;
#define KAUTH_GROUP_ISMEMBER    (1<<0)
};

TAILQ_HEAD(kauth_groups_head, kauth_group_membership) kauth_groups =
    TAILQ_HEAD_INITIALIZER(kauth_groups);
static LCK_MTX_DECLARE(kauth_groups_mtx, &kauth_lck_grp);
#define KAUTH_GROUPS_LOCK()     lck_mtx_lock(&kauth_groups_mtx);
#define KAUTH_GROUPS_UNLOCK()   lck_mtx_unlock(&kauth_groups_mtx);
#define KAUTH_GROUPS_CACHEMAX_DEFAULT 100                                
static int kauth_groups_cachemax = KAUTH_GROUPS_CACHEMAX_DEFAULT;
static int kauth_groups_count;

static int      kauth_groups_expired(struct kauth_group_membership *gm);
static void     kauth_groups_lru(struct kauth_group_membership *gm);
static void     kauth_groups_updatecache(struct kauth_identity_extlookup *el);
static void     kauth_groups_trimcache(int newsize);

#endif                           

#define KAUTH_CRED_TABLE_SIZE 128

ZONE_DECLARE(ucred_zone, "cred", sizeof(struct ucred), ZC_ZFREE_CLEARMEM);

LIST_HEAD(kauth_cred_entry_head, ucred);
static struct kauth_cred_entry_head
    kauth_cred_table_anchor[KAUTH_CRED_TABLE_SIZE];

static struct kauth_cred_entry_head *kauth_cred_get_bucket(kauth_cred_t cred);
static kauth_cred_t kauth_cred_add(kauth_cred_t new_cred, struct kauth_cred_entry_head *bucket);
static void kauth_cred_remove_locked(kauth_cred_t cred);
static kauth_cred_t kauth_cred_update(kauth_cred_t old_cred, kauth_cred_t new_cred, boolean_t retain_auditinfo);
static kauth_cred_t kauth_cred_find_and_ref(kauth_cred_t cred,
    struct kauth_cred_entry_head *bucket);
static bool kauth_cred_is_equal(kauth_cred_t cred1, kauth_cred_t cred2);

#if CONFIG_EXT_RESOLVER

  
                                                         
  
                                                                          
                                                                       
                                                               
  
                                                            
  
                                                      
                                                               
                                                                       
                                                                     
                                                              
  
   
static __attribute__((noinline)) int
__KERNEL_IS_WAITING_ON_EXTERNAL_CREDENTIAL_RESOLVER__(
	struct kauth_resolver_work  *workp)
{
	int error = 0;
	struct timespec ts;
	for (;;) {
		                                            
		ts.tv_sec = kauth_resolver_timeout;
		ts.tv_nsec = 0;
		error = msleep(workp, &kauth_resolver_mtx, PCATCH, "kr_submit", &ts);
		                                 
		if ((error == 0) && (workp->kr_flags & KAUTH_REQUEST_DONE)) {
			break;
		}
		                                          
		if (kauth_resolver_identity == 0) {
			RESOLVER_FAILED_MESSAGE("kauth external resolver died while while waiting for work to complete");
			error = KAUTH_RESOLVER_FAILED_ERRCODE;
			break;
		}
		               
		if (error != 0) {
			break;
		}
	}
	return error;
}


  
                                
  
                                                                      
                                         
  
                     
  
                        
   
void
kauth_resolver_identity_reset(void)
{
	KAUTH_RESOLVER_LOCK();
	if (kauth_resolver_identity != 0) {
		printf("kauth external resolver %d failed to de-register.\n",
		    kauth_resolver_identity);
		kauth_resolver_identity = 0;
		kauth_resolver_registered = 0;
	}
	KAUTH_RESOLVER_UNLOCK();
}

  
                        
  
                                                                            
                          
  
                                              
                      
                                             
  
                        
                                        
                                           
                 
                                          
                                                 
                                                      
                                 
  
                    
                    
  
                                                                   
                                                              
                                     
  
                                                              
                                                            
                                                           
                                                              
                                                                
                                   
  
                                                             
                                                        
                                                       
   
static int
kauth_resolver_submit(struct kauth_identity_extlookup *lkp, uint64_t extend_data)
{
	struct kauth_resolver_work *workp, *killp;
	struct timespec ts;
	int     error, shouldfree;

	                                                             
	if (kauth_resolver_identity == 0) {
		  
                                                             
                            
    
                                                                
                                                            
                                                              
                                                            
     
		error = tsleep(&ts, PZERO | PCATCH, "kr_submit", hz / 2);
		if (kauth_resolver_identity == 0) {
			  
                                                     
                                                      
             
      
			return EWOULDBLOCK;
		}
	}

	workp = kheap_alloc(KM_KAUTH, sizeof(struct kauth_resolver_work),
	    Z_WAITOK);
	if (workp == NULL) {
		return ENOMEM;
	}

	workp->kr_work = *lkp;
	workp->kr_extend = extend_data;
	workp->kr_refs = 1;
	workp->kr_flags = KAUTH_REQUEST_UNSUBMITTED;
	workp->kr_result = 0;

	  
                                                                      
                                                                  
    
	KAUTH_RESOLVER_LOCK();
	workp->kr_seqno = workp->kr_work.el_seqno = kauth_resolver_sequence++;
	workp->kr_work.el_result = KAUTH_EXTLOOKUP_INPROG;

	  
                                                                     
                                                                   
                                                                        
                                                          
    
	TAILQ_INSERT_TAIL(&kauth_resolver_unsubmitted, workp, kr_link);

	  
                                                                      
                                                                      
                                                                    
         
    
	wakeup_one((caddr_t)&kauth_resolver_unsubmitted);
	error = __KERNEL_IS_WAITING_ON_EXTERNAL_CREDENTIAL_RESOLVER__(workp);

	                                                   
	if (error == 0) {
		*lkp = workp->kr_work;
	}

	if (error == EWOULDBLOCK) {
		if ((kauth_resolver_timeout_cnt++ % KAUTH_COMPLAINT_INTERVAL) == 0) {
			printf("kauth external resolver timed out (%d timeout(s) of %d seconds).\n",
			    kauth_resolver_timeout_cnt, kauth_resolver_timeout);
		}

		if (workp->kr_flags & KAUTH_REQUEST_UNSUBMITTED) {
			  
                                                                    
                                                                 
                                                                     
                                             
      
			KAUTH_DEBUG("RESOLVER - request timed out without being collected for processing, resolver dead");

			  
                                                                 
                                                               
                                                               
                                                           
      
			kauth_resolver_identity = 0;
			kauth_resolver_registered = 0;

			                                                           
			TAILQ_FOREACH(killp, &kauth_resolver_submitted, kr_link)
			wakeup(killp);
			TAILQ_FOREACH(killp, &kauth_resolver_unsubmitted, kr_link)
			wakeup(killp);
			                                                      
			wakeup((caddr_t)&kauth_resolver_unsubmitted);
		}
	}

	  
                                                                   
                  
    
	if (--workp->kr_refs <= 0) {
		                                                   
		if (workp->kr_flags & KAUTH_REQUEST_DONE) {
			TAILQ_REMOVE(&kauth_resolver_done, workp, kr_link);
		} else if (workp->kr_flags & KAUTH_REQUEST_SUBMITTED) {
			TAILQ_REMOVE(&kauth_resolver_submitted, workp, kr_link);
		} else if (workp->kr_flags & KAUTH_REQUEST_UNSUBMITTED) {
			TAILQ_REMOVE(&kauth_resolver_unsubmitted, workp, kr_link);
		} else {
			KAUTH_DEBUG("RESOLVER - completed request has no valid queue");
		}
		shouldfree = 1;
	} else {
		                                                        
		shouldfree = 0;
	}

	                            
	if (error == 0) {
		error = workp->kr_result;
	}
	KAUTH_RESOLVER_UNLOCK();

	  
                                                       
    
	if (shouldfree) {
		kheap_free(KM_KAUTH, workp, sizeof(struct kauth_resolver_work));
	}

	KAUTH_DEBUG("RESOLVER - returning %d", error);
	return error;
}


  
              
  
                                                                         
  
                                                           
  
                                             
                                     
                                               
                              
                                 
                               
                                
  
                                                                  
                                                             
                                                                 
                                                              
                                                                
                                                               
                         
   
int
identitysvc(__unused struct proc *p, struct identitysvc_args *uap, __unused int32_t *retval)
{
	int opcode = uap->opcode;
	user_addr_t message = uap->message;
	struct kauth_resolver_work *workp;
	struct kauth_cache_sizes sz_arg = {};
	int error;
	pid_t new_id;

	if (!IOTaskHasEntitlement(current_task(), IDENTITYSVC_ENTITLEMENT)) {
		KAUTH_DEBUG("RESOLVER - pid %d not entitled to call identitysvc", current_proc()->p_pid);
		return EPERM;
	}

	  
                                  
    
	if (opcode == KAUTH_EXTLOOKUP_REGISTER) {
		new_id = current_proc()->p_pid;
		if ((error = kauth_authorize_generic(kauth_cred_get(), KAUTH_GENERIC_ISSUSER)) != 0) {
			KAUTH_DEBUG("RESOLVER - pid %d refused permission to become identity resolver", new_id);
			return error;
		}
		KAUTH_RESOLVER_LOCK();
		if (kauth_resolver_identity != new_id) {
			KAUTH_DEBUG("RESOLVER - new resolver %d taking over from old %d", new_id, kauth_resolver_identity);
			  
                                                                               
      
			while ((workp = TAILQ_LAST(&kauth_resolver_submitted, kauth_resolver_submitted_head)) != NULL) {
				TAILQ_REMOVE(&kauth_resolver_submitted, workp, kr_link);
				workp->kr_flags &= ~KAUTH_REQUEST_SUBMITTED;
				workp->kr_flags |= KAUTH_REQUEST_UNSUBMITTED;
				TAILQ_INSERT_HEAD(&kauth_resolver_unsubmitted, workp, kr_link);
			}
			  
                                               
                                 
      
			if (message > 30 && message < 10000) {
				kauth_resolver_timeout = (int)message;
				KAUTH_DEBUG("RESOLVER - new resolver changes timeout to %d seconds\n", (int)message);
			}
			kauth_resolver_identity = new_id;
			kauth_resolver_registered = 1;
			kauth_identitysvc_has_registered = 1;
			wakeup(&kauth_resolver_unsubmitted);
		}
		KAUTH_RESOLVER_UNLOCK();
		return 0;
	}

	  
                                                                      
                                                  
    
	if ((kauth_cred_getuid(kauth_cred_get()) != 0) || (current_proc()->p_pid != kauth_resolver_identity)) {
		KAUTH_DEBUG("RESOLVER - call from bogus resolver %d\n", current_proc()->p_pid);
		return EPERM;
	}

	if (opcode == KAUTH_GET_CACHE_SIZES) {
		KAUTH_IDENTITY_LOCK();
		sz_arg.kcs_id_size = kauth_identity_cachemax;
		KAUTH_IDENTITY_UNLOCK();

		KAUTH_GROUPS_LOCK();
		sz_arg.kcs_group_size = kauth_groups_cachemax;
		KAUTH_GROUPS_UNLOCK();

		if ((error = copyout(&sz_arg, uap->message, sizeof(sz_arg))) != 0) {
			return error;
		}

		return 0;
	} else if (opcode == KAUTH_SET_CACHE_SIZES) {
		if ((error = copyin(uap->message, &sz_arg, sizeof(sz_arg))) != 0) {
			return error;
		}

		if ((sz_arg.kcs_group_size > KAUTH_CACHES_MAX_SIZE) ||
		    (sz_arg.kcs_id_size > KAUTH_CACHES_MAX_SIZE)) {
			return EINVAL;
		}

		KAUTH_IDENTITY_LOCK();
		kauth_identity_cachemax = sz_arg.kcs_id_size;
		kauth_identity_trimcache(kauth_identity_cachemax);
		KAUTH_IDENTITY_UNLOCK();

		KAUTH_GROUPS_LOCK();
		kauth_groups_cachemax = sz_arg.kcs_group_size;
		kauth_groups_trimcache(kauth_groups_cachemax);
		KAUTH_GROUPS_UNLOCK();

		return 0;
	} else if (opcode == KAUTH_CLEAR_CACHES) {
		KAUTH_IDENTITY_LOCK();
		kauth_identity_trimcache(0);
		KAUTH_IDENTITY_UNLOCK();

		KAUTH_GROUPS_LOCK();
		kauth_groups_trimcache(0);
		KAUTH_GROUPS_UNLOCK();
	} else if (opcode == KAUTH_EXTLOOKUP_DEREGISTER) {
		  
                                                             
                                                    
     
		struct kauth_resolver_work *killp;

		KAUTH_RESOLVER_LOCK();

		  
                                                            
                                                            
                                
     
		kauth_resolver_identity = 0;
		kauth_resolver_registered = 0;

		TAILQ_FOREACH(killp, &kauth_resolver_submitted, kr_link)
		wakeup(killp);
		TAILQ_FOREACH(killp, &kauth_resolver_unsubmitted, kr_link)
		wakeup(killp);
		                                                      
		wakeup((caddr_t)&kauth_resolver_unsubmitted);
		KAUTH_RESOLVER_UNLOCK();
	}

	  
                           
    
	if (opcode & KAUTH_EXTLOOKUP_RESULT) {
		if ((error = kauth_resolver_complete(message)) != 0) {
			return error;
		}
	}

	  
                                   
    
	if (opcode & KAUTH_EXTLOOKUP_WORKER) {
		if ((error = kauth_resolver_getwork(message)) != 0) {
			return error;
		}
	}

	return 0;
}


  
                                  
  
                                                       
  
                                                      
                                    
  
                        
                                         
                                 
  
                                                                        
                     
   
static int
kauth_resolver_getwork_continue(int result)
{
	thread_t thread;
	struct uthread *ut;
	user_addr_t message;

	if (result) {
		KAUTH_RESOLVER_UNLOCK();
		return result;
	}

	  
                                                                     
                                                                      
                                                                     
                           
    
	if (TAILQ_FIRST(&kauth_resolver_unsubmitted) == NULL) {
		int error;

		error = msleep0(&kauth_resolver_unsubmitted, &kauth_resolver_mtx, PCATCH, "GRGetWork", 0, kauth_resolver_getwork_continue);
		  
                                                            
                                                            
     
		if (!kauth_resolver_identity) {
			RESOLVER_FAILED_MESSAGE("external resolver died");
			error = KAUTH_RESOLVER_FAILED_ERRCODE;
		}
		KAUTH_RESOLVER_UNLOCK();
		return error;
	}

	thread = current_thread();
	ut = get_bsdthread_info(thread);
	message = ut->uu_save.uus_kauth.message;
	return kauth_resolver_getwork2(message);
}


  
                          
  
                                                                           
                                                                
                                      
  
                                               
  
                        
                                            
  
                                                                        
                                                                
                                                                
                                                                
                                                                
                                                                 
   
static int
kauth_resolver_getwork2(user_addr_t message)
{
	struct kauth_resolver_work *workp;
	int             error;

	  
                                                                     
                                                                      
             
    
	workp = TAILQ_FIRST(&kauth_resolver_unsubmitted);

	  
                                                               
                                                                    
                                                                   
                                           
    
	                  
	if ((error = copyout(&workp->kr_work, message, offsetof(struct kauth_identity_extlookup, el_extend))) != 0) {
		KAUTH_DEBUG("RESOLVER - error submitting work to resolve");
		goto out;
	}
	                 
	if ((error = copyout(&workp->kr_work.el_info_reserved_1,
	    message + offsetof(struct kauth_identity_extlookup, el_info_reserved_1),
	    sizeof(struct kauth_identity_extlookup) - offsetof(struct kauth_identity_extlookup, el_info_reserved_1))) != 0) {
		KAUTH_DEBUG("RESOLVER - error submitting work to resolve");
		goto out;
	}

	  
                                                                       
                                                                        
                                                                   
                                                                       
                         
    

	  
                                                          
    
	if (workp->kr_work.el_flags & (KAUTH_EXTLOOKUP_VALID_PWNAM | KAUTH_EXTLOOKUP_VALID_GRNAM)) {
		uint64_t uaddr;

		error = copyin(message + offsetof(struct kauth_identity_extlookup, el_extend), &uaddr, sizeof(uaddr));
		if (!error) {
			size_t actual;                
			  
                                                      
                                                     
                                 
      
			error = copyoutstr(CAST_DOWN(void *, workp->kr_extend), uaddr, MAXPATHLEN, &actual);
		}
		if (error) {
			KAUTH_DEBUG("RESOLVER - error submitting work to resolve");
			goto out;
		}
	}
	TAILQ_REMOVE(&kauth_resolver_unsubmitted, workp, kr_link);
	workp->kr_flags &= ~KAUTH_REQUEST_UNSUBMITTED;
	workp->kr_flags |= KAUTH_REQUEST_SUBMITTED;
	TAILQ_INSERT_TAIL(&kauth_resolver_submitted, workp, kr_link);

out:
	KAUTH_RESOLVER_UNLOCK();
	return error;
}


  
                         
  
                                                                              
                                     
  
                                               
  
                        
                                         
                                 
  
                                                                     
                                                             
                                                              
                                                               
                                                                
                               
   
static int
kauth_resolver_getwork(user_addr_t message)
{
	struct kauth_resolver_work *workp;
	int             error;

	KAUTH_RESOLVER_LOCK();
	error = 0;
	while ((workp = TAILQ_FIRST(&kauth_resolver_unsubmitted)) == NULL) {
		thread_t thread = current_thread();
		struct uthread *ut = get_bsdthread_info(thread);

		ut->uu_save.uus_kauth.message = message;
		error = msleep0(&kauth_resolver_unsubmitted, &kauth_resolver_mtx, PCATCH, "GRGetWork", 0, kauth_resolver_getwork_continue);
		KAUTH_RESOLVER_UNLOCK();
		  
                                                            
                                                            
     
		if (!kauth_resolver_identity) {
			printf("external resolver died");
			error = KAUTH_RESOLVER_FAILED_ERRCODE;
		}
		return error;
	}
	return kauth_resolver_getwork2(message);
}


  
                          
  
                                               
  
                                                 
  
                        
                               
                                               
   
static int
kauth_resolver_complete(user_addr_t message)
{
	struct kauth_identity_extlookup extl;
	struct kauth_resolver_work *workp;
	struct kauth_resolver_work *killp;
	int error, result, want_extend_data;

	  
                                                                   
                                  
    
	if ((error = copyin(message, &extl, sizeof(extl))) != 0) {
		KAUTH_DEBUG("RESOLVER - error getting completed work\n");
		return error;
	}

	KAUTH_RESOLVER_LOCK();

	error = 0;
	result = 0;
	switch (extl.el_result) {
	case KAUTH_EXTLOOKUP_INPROG:
	{
		static int once = 0;

		                                                     
		if (!once) {
			printf("kauth_resolver: memberd is not setting valid result codes (assuming always successful)\n");
			once = 1;
		}
	}
		OS_FALLTHROUGH;

	case KAUTH_EXTLOOKUP_SUCCESS:
		break;

	case KAUTH_EXTLOOKUP_FATAL:
		                                            
		KAUTH_DEBUG("RESOLVER - resolver %d died, waiting for a new one", kauth_resolver_identity);
		RESOLVER_FAILED_MESSAGE("resolver %d died, waiting for a new one", kauth_resolver_identity);
		  
                                                             
                                                             
                                                               
                                                              
                                                          
     
		kauth_resolver_identity = 0;
		kauth_resolver_registered = 0;

		TAILQ_FOREACH(killp, &kauth_resolver_submitted, kr_link)
		wakeup(killp);
		TAILQ_FOREACH(killp, &kauth_resolver_unsubmitted, kr_link)
		wakeup(killp);
		                                                      
		wakeup((caddr_t)&kauth_resolver_unsubmitted);
		                                  
		error = KAUTH_RESOLVER_FAILED_ERRCODE;
		break;

	case KAUTH_EXTLOOKUP_BADRQ:
		KAUTH_DEBUG("RESOLVER - resolver reported invalid request %d", extl.el_seqno);
		result = EINVAL;
		break;

	case KAUTH_EXTLOOKUP_FAILURE:
		KAUTH_DEBUG("RESOLVER - resolver reported transient failure for request %d", extl.el_seqno);
		RESOLVER_FAILED_MESSAGE("resolver reported transient failure for request %d", extl.el_seqno);
		result = KAUTH_RESOLVER_FAILED_ERRCODE;
		break;

	default:
		KAUTH_DEBUG("RESOLVER - resolver returned unexpected status %d", extl.el_result);
		RESOLVER_FAILED_MESSAGE("resolver returned unexpected status %d", extl.el_result);
		result = KAUTH_RESOLVER_FAILED_ERRCODE;
		break;
	}

	  
                                                                  
                                                                   
                                                                
                 
    
	if (extl.el_result != KAUTH_EXTLOOKUP_FATAL) {
		                                    
		TAILQ_FOREACH(workp, &kauth_resolver_submitted, kr_link) {
			               
			if (workp->kr_seqno == extl.el_seqno) {
				  
                              
       
				want_extend_data = (workp->kr_work.el_flags & (KAUTH_EXTLOOKUP_WANT_PWNAM | KAUTH_EXTLOOKUP_WANT_GRNAM));

				  
                                                
                                               
                       
       
				TAILQ_REMOVE(&kauth_resolver_submitted, workp, kr_link);
				workp->kr_flags &= ~KAUTH_REQUEST_SUBMITTED;
				workp->kr_flags |= KAUTH_REQUEST_DONE;
				workp->kr_result = result;

				                                               
				memcpy(&workp->kr_work, &extl, sizeof(struct kauth_identity_extlookup));

				  
                                                 
                                                  
                                               
                                                 
                                                 
                                               
                                               
                                               
                                                   
                                                  
               
      
                                                
                                                  
                                                  
                                                   
                                               
                                                
                                               
                                                  
                                                
                                         
      
                                                  
                                                    
                                                        
       
				if (want_extend_data && (extl.el_flags & (KAUTH_EXTLOOKUP_VALID_PWNAM | KAUTH_EXTLOOKUP_VALID_GRNAM))) {
					size_t actual;               

					KAUTH_RESOLVER_UNLOCK();
					error = copyinstr(extl.el_extend, CAST_DOWN(void *, workp->kr_extend), MAXPATHLEN, &actual);
					KAUTH_DEBUG("RESOLVER - resolver got name :%*s: len = %d\n", (int)actual,
					    actual ? "null" : (char *)extl.el_extend, actual);
					KAUTH_RESOLVER_LOCK();
				} else if (extl.el_flags &  (KAUTH_EXTLOOKUP_VALID_PWNAM | KAUTH_EXTLOOKUP_VALID_GRNAM)) {
					error = EFAULT;
					KAUTH_DEBUG("RESOLVER - resolver returned mismatching extension flags (%d), request contained (%d)",
					    extl.el_flags, want_extend_data);
				}

				  
                                          
                                                
       
				TAILQ_INSERT_TAIL(&kauth_resolver_done, workp, kr_link);
				wakeup(workp);
				break;
			}
		}
	}
	  
                                                                     
                                           
    
	KAUTH_RESOLVER_UNLOCK();

	return error;
}
#endif                          


  
                  
   

#define KI_VALID_UID    (1<<0)                                                  
#define KI_VALID_GID    (1<<1)
#define KI_VALID_GUID   (1<<2)
#define KI_VALID_NTSID  (1<<3)
#define KI_VALID_PWNAM  (1<<4)                            
#define KI_VALID_GRNAM  (1<<5)                            
#define KI_VALID_GROUPS (1<<6)

#if CONFIG_EXT_RESOLVER
  
                       
  
                                                                    
                                            
  
                  
  
                                                  
                                     
                                       
                            
  
                                                                        
                                                               
                                                
   
static struct kauth_identity *
kauth_identity_alloc(uid_t uid, gid_t gid, guid_t *guidp, time_t guid_expiry,
    ntsid_t *ntsidp, time_t ntsid_expiry, size_t supgrpcnt, gid_t *supgrps, time_t groups_expiry,
    const char *name, int nametype)
{
	struct kauth_identity *kip;

	                                    
	kip = kheap_alloc(KM_KAUTH, sizeof(struct kauth_identity),
	    Z_WAITOK | Z_ZERO);
	if (kip != NULL) {
		if (gid != KAUTH_GID_NONE) {
			kip->ki_gid = gid;
			kip->ki_valid = KI_VALID_GID;
		}
		if (uid != KAUTH_UID_NONE) {
			if (kip->ki_valid & KI_VALID_GID) {
				panic("can't allocate kauth identity with both uid and gid");
			}
			kip->ki_uid = uid;
			kip->ki_valid = KI_VALID_UID;
		}
		if (supgrpcnt) {
			  
                                                         
      
			assert(supgrpcnt <= NGROUPS);
			assert(supgrps != NULL);

			if ((supgrpcnt > NGROUPS) || (supgrps == NULL)) {
				return NULL;
			}
			if (kip->ki_valid & KI_VALID_GID) {
				panic("can't allocate kauth identity with both gid and supplementary groups");
			}
			kip->ki_supgrpcnt = (uint32_t)supgrpcnt;
			memcpy(kip->ki_supgrps, supgrps, sizeof(supgrps[0]) * supgrpcnt);
			kip->ki_valid |= KI_VALID_GROUPS;
		}
		kip->ki_groups_expiry = groups_expiry;
		if (guidp != NULL) {
			kip->ki_guid = *guidp;
			kip->ki_valid |= KI_VALID_GUID;
		}
		kip->ki_guid_expiry = guid_expiry;
		if (ntsidp != NULL) {
			kip->ki_ntsid = *ntsidp;
			kip->ki_valid |= KI_VALID_NTSID;
		}
		kip->ki_ntsid_expiry = ntsid_expiry;
		if (name != NULL) {
			kip->ki_name = name;
			kip->ki_valid |= nametype;
		}
	}
	return kip;
}


  
                                   
  
                                                                            
                                       
  
                                               
                             
  
                  
  
                                                                
                                                    
   
static void
kauth_identity_register_and_free(struct kauth_identity *kip)
{
	struct kauth_identity *ip;

	  
                                                                       
                                                               
    
	ip = NULL;
	KAUTH_IDENTITY_LOCK();
	if (kip->ki_valid & KI_VALID_UID) {
		if (kip->ki_valid & KI_VALID_GID) {
			panic("kauth_identity: can't insert record with both UID and GID as key");
		}
		TAILQ_FOREACH(ip, &kauth_identities, ki_link)
		if ((ip->ki_valid & KI_VALID_UID) && (ip->ki_uid == kip->ki_uid)) {
			break;
		}
	} else if (kip->ki_valid & KI_VALID_GID) {
		TAILQ_FOREACH(ip, &kauth_identities, ki_link)
		if ((ip->ki_valid & KI_VALID_GID) && (ip->ki_gid == kip->ki_gid)) {
			break;
		}
	} else {
		panic("kauth_identity: can't insert record without UID or GID as key");
	}

	if (ip != NULL) {
		                                               
		if (kip->ki_valid & KI_VALID_GUID) {
			ip->ki_guid = kip->ki_guid;
			ip->ki_valid |= KI_VALID_GUID;
		}
		ip->ki_guid_expiry = kip->ki_guid_expiry;
		if (kip->ki_valid & KI_VALID_NTSID) {
			ip->ki_ntsid = kip->ki_ntsid;
			ip->ki_valid |= KI_VALID_NTSID;
		}
		ip->ki_ntsid_expiry = kip->ki_ntsid_expiry;
		                                                              
		if (kip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM)) {
			                                       
			const char *oname = NULL;
			if (ip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM)) {
				oname = ip->ki_name;
			}
			ip->ki_name = kip->ki_name;
			kip->ki_name = oname;
		}
		                                    
		ip = kip;
	} else {
		  
                                                               
                                                            
     
		TAILQ_INSERT_HEAD(&kauth_identities, kip, ki_link);
		if (++kauth_identity_count > kauth_identity_cachemax) {
			ip = TAILQ_LAST(&kauth_identities, kauth_identity_head);
			TAILQ_REMOVE(&kauth_identities, ip, ki_link);
			kauth_identity_count--;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	                                                                       
	if (ip != NULL) {
		                                                  
		if (ip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM)) {
			vfs_removename(ip->ki_name);
		}
		                            
		kheap_free(KM_KAUTH, ip, sizeof(struct kauth_identity));
	}
}


  
                             
  
                                                                         
                                                    
  
                                                 
                                   
                                     
                         
                                          
  
                  
  
                    
                                   
  
                                                                         
                                                              
                                                               
             
  
                                                                      
                                                              
                                                          
                                                
  
                                                                    
                                                             
   
static void
kauth_identity_updatecache(struct kauth_identity_extlookup *elp, struct kauth_identity *rkip, uint64_t extend_data)
{
	struct timeval tv;
	struct kauth_identity *kip;
	const char *speculative_name = NULL;

	microuptime(&tv);

	  
                                                                     
                                                                    
                                                                      
                              
    
	if (elp->el_flags & (KAUTH_EXTLOOKUP_VALID_PWNAM | KAUTH_EXTLOOKUP_VALID_GRNAM)) {
		const char *tmp = CAST_DOWN(const char *, extend_data);
		speculative_name = vfs_addname(tmp, (uint32_t)strnlen(tmp, MAXPATHLEN - 1), 0, 0);
	}

	                    
	if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_UID) {
		KAUTH_IDENTITY_LOCK();
		TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
			                     
			if ((kip->ki_valid & KI_VALID_UID) && (kip->ki_uid == elp->el_uid)) {
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_SUPGRPS) {
					assert(elp->el_sup_grp_cnt <= NGROUPS);
					if (elp->el_sup_grp_cnt > NGROUPS) {
						KAUTH_DEBUG("CACHE - invalid sup_grp_cnt provided (%d), truncating to  %d",
						    elp->el_sup_grp_cnt, NGROUPS);
						elp->el_sup_grp_cnt = NGROUPS;
					}
					kip->ki_supgrpcnt = elp->el_sup_grp_cnt;
					memcpy(kip->ki_supgrps, elp->el_sup_groups, sizeof(elp->el_sup_groups[0]) * kip->ki_supgrpcnt);
					kip->ki_valid |= KI_VALID_GROUPS;
					kip->ki_groups_expiry = (elp->el_member_valid) ? tv.tv_sec + elp->el_member_valid : 0;
				}
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_UGUID) {
					kip->ki_guid = elp->el_uguid;
					kip->ki_valid |= KI_VALID_GUID;
				}
				kip->ki_guid_expiry = (elp->el_uguid_valid) ? tv.tv_sec + elp->el_uguid_valid : 0;
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_USID) {
					kip->ki_ntsid = elp->el_usid;
					kip->ki_valid |= KI_VALID_NTSID;
				}
				kip->ki_ntsid_expiry = (elp->el_usid_valid) ? tv.tv_sec + elp->el_usid_valid : 0;
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_PWNAM) {
					const char *oname = kip->ki_name;
					kip->ki_name = speculative_name;
					speculative_name = NULL;
					kip->ki_valid |= KI_VALID_PWNAM;
					if (oname) {
						  
                                    
                 
         
						speculative_name = oname;
					}
				}
				kauth_identity_lru(kip);
				if (rkip != NULL) {
					*rkip = *kip;
				}
				KAUTH_DEBUG("CACHE - refreshed %d is " K_UUID_FMT, kip->ki_uid, K_UUID_ARG(kip->ki_guid));
				break;
			}
		}
		KAUTH_IDENTITY_UNLOCK();
		                                        
		if (kip == NULL) {
			kip = kauth_identity_alloc(elp->el_uid, KAUTH_GID_NONE,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_UGUID) ? &elp->el_uguid : NULL,
			    (elp->el_uguid_valid) ? tv.tv_sec + elp->el_uguid_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_USID) ? &elp->el_usid : NULL,
			    (elp->el_usid_valid) ? tv.tv_sec + elp->el_usid_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_SUPGRPS) ? elp->el_sup_grp_cnt : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_SUPGRPS) ? elp->el_sup_groups : NULL,
			    (elp->el_member_valid) ? tv.tv_sec + elp->el_member_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_PWNAM) ? speculative_name : NULL,
			    KI_VALID_PWNAM);
			if (kip != NULL) {
				if (rkip != NULL) {
					*rkip = *kip;
				}
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_PWNAM) {
					speculative_name = NULL;
				}
				KAUTH_DEBUG("CACHE - learned %d is " K_UUID_FMT, kip->ki_uid, K_UUID_ARG(kip->ki_guid));
				kauth_identity_register_and_free(kip);
			}
		}
	}

	                                                                    
	if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GID && !(elp->el_flags & KAUTH_EXTLOOKUP_VALID_UID)) {
		KAUTH_IDENTITY_LOCK();
		TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
			                     
			if ((kip->ki_valid & KI_VALID_GID) && (kip->ki_gid == elp->el_gid)) {
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GGUID) {
					kip->ki_guid = elp->el_gguid;
					kip->ki_valid |= KI_VALID_GUID;
				}
				kip->ki_guid_expiry = (elp->el_gguid_valid) ? tv.tv_sec + elp->el_gguid_valid : 0;
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GSID) {
					kip->ki_ntsid = elp->el_gsid;
					kip->ki_valid |= KI_VALID_NTSID;
				}
				kip->ki_ntsid_expiry = (elp->el_gsid_valid) ? tv.tv_sec + elp->el_gsid_valid : 0;
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GRNAM) {
					const char *oname = kip->ki_name;
					kip->ki_name = speculative_name;
					speculative_name = NULL;
					kip->ki_valid |= KI_VALID_GRNAM;
					if (oname) {
						  
                                    
                 
         
						speculative_name = oname;
					}
				}
				kauth_identity_lru(kip);
				if (rkip != NULL) {
					*rkip = *kip;
				}
				KAUTH_DEBUG("CACHE - refreshed %d is " K_UUID_FMT, kip->ki_uid, K_UUID_ARG(kip->ki_guid));
				break;
			}
		}
		KAUTH_IDENTITY_UNLOCK();
		                                        
		if (kip == NULL) {
			kip = kauth_identity_alloc(KAUTH_UID_NONE, elp->el_gid,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GGUID) ? &elp->el_gguid : NULL,
			    (elp->el_gguid_valid) ? tv.tv_sec + elp->el_gguid_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GSID) ? &elp->el_gsid : NULL,
			    (elp->el_gsid_valid) ? tv.tv_sec + elp->el_gsid_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_SUPGRPS) ? elp->el_sup_grp_cnt : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_SUPGRPS) ? elp->el_sup_groups : NULL,
			    (elp->el_member_valid) ? tv.tv_sec + elp->el_member_valid : 0,
			    (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GRNAM) ? speculative_name : NULL,
			    KI_VALID_GRNAM);
			if (kip != NULL) {
				if (rkip != NULL) {
					*rkip = *kip;
				}
				if (elp->el_flags & KAUTH_EXTLOOKUP_VALID_GRNAM) {
					speculative_name = NULL;
				}
				KAUTH_DEBUG("CACHE - learned %d is " K_UUID_FMT, kip->ki_uid, K_UUID_ARG(kip->ki_guid));
				kauth_identity_register_and_free(kip);
			}
		}
	}

	                                                       
	if (speculative_name != NULL) {
		vfs_removename(speculative_name);
	}
}


  
                                              
  
                                                    
   
static void
kauth_identity_trimcache(int newsize)
{
	struct kauth_identity           *kip;

	lck_mtx_assert(&kauth_identity_mtx, LCK_MTX_ASSERT_OWNED);

	while (kauth_identity_count > newsize) {
		kip = TAILQ_LAST(&kauth_identities, kauth_identity_head);
		TAILQ_REMOVE(&kauth_identities, kip, ki_link);
		kauth_identity_count--;
		kheap_free(KM_KAUTH, kip, sizeof(struct kauth_identity));
	}
}

  
                     
  
                                                                           
              
  
                                                   
                                     
                         
  
                  
  
                                                                    
                                                                   
                                                                
                                                      
   
static void
kauth_identity_lru(struct kauth_identity *kip)
{
	if (kip != TAILQ_FIRST(&kauth_identities)) {
		TAILQ_REMOVE(&kauth_identities, kip, ki_link);
		TAILQ_INSERT_HEAD(&kauth_identities, kip, ki_link);
	}
}


  
                              
  
                                                            
  
                                                 
                       
  
                        
                    
   
static int
kauth_identity_guid_expired(struct kauth_identity *kip)
{
	struct timeval tv;

	  
                                                        
    
	if (kip->ki_guid_expiry == 0) {
		return 0;
	}

	microuptime(&tv);
	KAUTH_DEBUG("CACHE - GUID expires @ %ld now %ld", kip->ki_guid_expiry, tv.tv_sec);

	return (kip->ki_guid_expiry <= tv.tv_sec) ? 1 : 0;
}


  
                               
  
                                                             
  
                                                 
                        
  
                        
                    
   
static int
kauth_identity_ntsid_expired(struct kauth_identity *kip)
{
	struct timeval tv;

	  
                                                        
    
	if (kip->ki_ntsid_expiry == 0) {
		return 0;
	}

	microuptime(&tv);
	KAUTH_DEBUG("CACHE - NTSID expires @ %ld now %ld", kip->ki_ntsid_expiry, tv.tv_sec);

	return (kip->ki_ntsid_expiry <= tv.tv_sec) ? 1 : 0;
}

  
                                
  
                                                                          
  
                                                 
                         
  
                        
                    
   
static int
kauth_identity_groups_expired(struct kauth_identity *kip)
{
	struct timeval tv;

	  
                                                        
    
	if (kip->ki_groups_expiry == 0) {
		return 0;
	}

	microuptime(&tv);
	KAUTH_DEBUG("CACHE - GROUPS expires @ %ld now %ld\n", kip->ki_groups_expiry, tv.tv_sec);

	return (kip->ki_groups_expiry <= tv.tv_sec) ? 1 : 0;
}

  
                          
  
                                          
  
                                 
                                 
                                             
  
                      
                       
  
                    
                              
   
static int
kauth_identity_find_uid(uid_t uid, struct kauth_identity *kir, char *getname)
{
	struct kauth_identity *kip;

	KAUTH_IDENTITY_LOCK();
	TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
		if ((kip->ki_valid & KI_VALID_UID) && (uid == kip->ki_uid)) {
			kauth_identity_lru(kip);
			                                   
			*kir = *kip;
			                                                     
			if (getname != NULL && (kip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM))) {
				strlcpy(getname, kip->ki_name, MAXPATHLEN);
			}
			break;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	return (kip == NULL) ? ENOENT : 0;
}


  
                          
  
                                          
  
                                 
                                 
                                             
  
                      
                       
  
                    
                              
   
static int
kauth_identity_find_gid(uid_t gid, struct kauth_identity *kir, char *getname)
{
	struct kauth_identity *kip;

	KAUTH_IDENTITY_LOCK();
	TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
		if ((kip->ki_valid & KI_VALID_GID) && (gid == kip->ki_gid)) {
			kauth_identity_lru(kip);
			                                   
			*kir = *kip;
			                                                     
			if (getname != NULL && (kip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM))) {
				strlcpy(getname, kip->ki_name, MAXPATHLEN);
			}
			break;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	return (kip == NULL) ? ENOENT : 0;
}


  
                           
  
                                           
  
                                               
                                 
                                             
  
                      
                       
  
                    
                              
  
                                                                 
                                                    
   
static int
kauth_identity_find_guid(guid_t *guidp, struct kauth_identity *kir, char *getname)
{
	struct kauth_identity *kip;

	KAUTH_IDENTITY_LOCK();
	TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
		if ((kip->ki_valid & KI_VALID_GUID) && (kauth_guid_equal(guidp, &kip->ki_guid))) {
			kauth_identity_lru(kip);
			                                   
			*kir = *kip;
			                                                     
			if (getname != NULL && (kip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM))) {
				strlcpy(getname, kip->ki_name, MAXPATHLEN);
			}
			break;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	return (kip == NULL) ? ENOENT : 0;
}

  
                          
  
                                           
  
                                              
                                             
                                 
  
                      
                       
  
                    
                              
   
static int
kauth_identity_find_nam(char *name, int valid, struct kauth_identity *kir)
{
	struct kauth_identity *kip;

	KAUTH_IDENTITY_LOCK();
	TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
		if ((kip->ki_valid & valid) && !strcmp(name, kip->ki_name)) {
			kauth_identity_lru(kip);
			                                   
			*kir = *kip;
			break;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	return (kip == NULL) ? ENOENT : 0;
}


  
                            
  
                                            
  
                                                
                                 
                                             
  
                      
                       
  
                    
                              
  
                                                                 
                                                    
   
static int
kauth_identity_find_ntsid(ntsid_t *ntsid, struct kauth_identity *kir, char *getname)
{
	struct kauth_identity *kip;

	KAUTH_IDENTITY_LOCK();
	TAILQ_FOREACH(kip, &kauth_identities, ki_link) {
		if ((kip->ki_valid & KI_VALID_NTSID) && (kauth_ntsid_equal(ntsid, &kip->ki_ntsid))) {
			kauth_identity_lru(kip);
			                                   
			*kir = *kip;
			                                                     
			if (getname != NULL && (kip->ki_valid & (KI_VALID_PWNAM | KI_VALID_GRNAM))) {
				strlcpy(getname, kip->ki_name, MAXPATHLEN);
			}
			break;
		}
	}
	KAUTH_IDENTITY_UNLOCK();
	return (kip == NULL) ? ENOENT : 0;
}
#endif                           


  
                 
   
guid_t kauth_null_guid;


  
                   
  
                                                   
  
                                             
                                   
  
                                     
                            
   
int
kauth_guid_equal(guid_t *guid1, guid_t *guid2)
{
	return bcmp(guid1, guid2, sizeof(*guid1)) == 0;
}


  
                       
  
                                                        
  
                                               
  
                                                 
                                    
                              
                             
                             
   
int
kauth_wellknown_guid(guid_t *guid)
{
	static char     fingerprint[] = {0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef};
	uint32_t                code;
	  
                                          
    
	if (bcmp((void *)guid, fingerprint, 12) == 0) {
		  
                                                            
     
		code = OSSwapHostToBigInt32(*(uint32_t *)&guid->g_guid[12]);
		switch (code) {
		case 0x0000000c:
			return KAUTH_WKG_EVERYBODY;
		case 0xfffffffe:
			return KAUTH_WKG_NOBODY;
		case 0x0000000a:
			return KAUTH_WKG_OWNER;
		case 0x00000010:
			return KAUTH_WKG_GROUP;
		}
	}
	return KAUTH_WKG_NOT;
}


  
                    
  
                                                                              
  
                                             
                                   
  
                                     
                            
   
int
kauth_ntsid_equal(ntsid_t *sid1, ntsid_t *sid2)
{
	                                                                        
	if ((KAUTH_NTSID_SIZE(sid1) == KAUTH_NTSID_SIZE(sid2)) &&
	    (KAUTH_NTSID_SIZE(sid1) <= sizeof(*sid1)) &&
	    bcmp(sid1, sid2, KAUTH_NTSID_SIZE(sid1)) == 0) {
		return 1;
	}
	return 0;
}


  
               
  
                                                
                          
         
          
                            
  
                                                                           
                    
   



  
                         
  
                                                                           
                                                            
                                                               
                        
  
                                                         
                                 
  
                                                     
                                   
                                             
                                       
  
                                                                
  
                                                               
                                                                
                                                            
                                                         
   
static int
kauth_cred_change_egid(kauth_cred_t cred, gid_t new_egid)
{
	int     i;
	int     displaced = 1;
#if radar_4600026
	int     is_member;
#endif                     
	gid_t   old_egid = kauth_cred_getgid(cred);
	posix_cred_t pcred = posix_cred_get(cred);

	                                                                 
	for (i = 1; i < pcred->cr_ngroups; i++) {
		  
                                                           
                      
     
		if (pcred->cr_groups[i] == new_egid) {
			pcred->cr_groups[i] = old_egid;
			DEBUG_CRED_CHANGE("kauth_cred_change_egid: unset displaced\n");
			displaced = 0;
			break;
		}
	}

#if radar_4600026
#error Fix radar 4600026 first!!!

  
                                                                              
                                                                                
                                                                                
                                                                              
                                                                      
   

	  
                                                                    
                                                                     
                                                                      
                            
   
                                           
    
	if (displaced && !(pcred->cr_flags & CRF_NOMEMBERD) &&
	    kauth_cred_ismember_gid(cred, new_egid, &is_member) == 0 &&
	    is_member) {
		displaced = 0;
		DEBUG_CRED_CHANGE("kauth_cred_change_egid: reset displaced\n");
	}
#endif                     

	                                        
	pcred->cr_groups[0] = new_egid;

	return displaced;
}


  
                    
  
                                         
  
                                            
  
                                                     
   
uid_t
kauth_cred_getuid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_uid;
}


  
                     
  
                                          
  
                                            
  
                                                      
   
uid_t
kauth_cred_getruid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_ruid;
}


  
                      
  
                                           
  
                                            
  
                                                       
   
uid_t
kauth_cred_getsvuid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_svuid;
}


  
                    
  
                                         
  
                                            
  
                                                     
   
gid_t
kauth_cred_getgid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_gid;
}


  
                     
  
                                          
  
                                            
  
                                                      
   
gid_t
kauth_cred_getrgid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_rgid;
}


  
                      
  
                                           
  
                                            
  
                                                       
   
gid_t
kauth_cred_getsvgid(kauth_cred_t cred)
{
	NULLCRED_CHECK(cred);
	return posix_cred_get(cred)->cr_svgid;
}


static int      kauth_cred_cache_lookup(int from, int to, void *src, void *dst);

#if CONFIG_EXT_RESOLVER == 0
  
                                                                                 
   
static __inline int
kauth_cred_cache_lookup(int from, int to, void *src, void *dst)
{
	                                                                                        
	static const uuid_t _user_compat_prefix = {0xff, 0xff, 0xee, 0xee, 0xdd, 0xdd, 0xcc, 0xcc, 0xbb, 0xbb, 0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00};
	static const uuid_t _group_compat_prefix = {0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef, 0x00, 0x00, 0x00, 0x00};
#define COMPAT_PREFIX_LEN       (sizeof(uuid_t) - sizeof(id_t))

	assert(from != to);

	switch (from) {
	case KI_VALID_UID: {
		id_t uid = htonl(*(id_t *)src);

		if (to == KI_VALID_GUID) {
			uint8_t *uu = dst;
			memcpy(uu, _user_compat_prefix, sizeof(_user_compat_prefix));
			memcpy(&uu[COMPAT_PREFIX_LEN], &uid, sizeof(uid));
			return 0;
		}
		break;
	}
	case KI_VALID_GID: {
		id_t gid = htonl(*(id_t *)src);

		if (to == KI_VALID_GUID) {
			uint8_t *uu = dst;
			memcpy(uu, _group_compat_prefix, sizeof(_group_compat_prefix));
			memcpy(&uu[COMPAT_PREFIX_LEN], &gid, sizeof(gid));
			return 0;
		}
		break;
	}
	case KI_VALID_GUID: {
		const uint8_t *uu = src;

		if (to == KI_VALID_UID) {
			if (memcmp(uu, _user_compat_prefix, COMPAT_PREFIX_LEN) == 0) {
				id_t uid;
				memcpy(&uid, &uu[COMPAT_PREFIX_LEN], sizeof(uid));
				*(id_t *)dst = ntohl(uid);
				return 0;
			}
		} else if (to == KI_VALID_GID) {
			if (memcmp(uu, _group_compat_prefix, COMPAT_PREFIX_LEN) == 0) {
				id_t gid;
				memcpy(&gid, &uu[COMPAT_PREFIX_LEN], sizeof(gid));
				*(id_t *)dst = ntohl(gid);
				return 0;
			}
		}
		break;
	}
	default:
		                     
		break;
	}
	return ENOENT;
}
#endif

#if defined(CONFIG_EXT_RESOLVER) && (CONFIG_EXT_RESOLVER)
  
                                                                          
                                 
   
struct supgroups {
	size_t *count;
	gid_t *groups;
};

  
                        
  
                                                  
  
                                    
                                           
                                                             
  
                        
                                 
  
                    
                                      
                                      
  
   
static int
kauth_cred_uid2groups(uid_t *uid, gid_t *groups, size_t *gcount)
{
	int rv;

	struct supgroups supgroups;
	supgroups.count = gcount;
	supgroups.groups = groups;

	rv = kauth_cred_cache_lookup(KI_VALID_UID, KI_VALID_GROUPS, uid, &supgroups);

	return rv;
}
#endif

  
                        
  
                                     
  
                                                  
                                          
  
                        
                                 
  
                    
                                     
  
                                                                     
   
int
kauth_cred_guid2pwnam(guid_t *guidp, char *pwnam)
{
	return kauth_cred_cache_lookup(KI_VALID_GUID, KI_VALID_PWNAM, guidp, pwnam);
}


  
                        
  
                                     
  
                                                  
                                           
  
                        
                                 
  
                    
                                     
  
                                                                     
   
int
kauth_cred_guid2grnam(guid_t *guidp, char *grnam)
{
	return kauth_cred_cache_lookup(KI_VALID_GUID, KI_VALID_GRNAM, guidp, grnam);
}


  
                        
  
                                     
  
                                                     
                                       
  
                        
                                 
  
                    
                                     
  
                                                                    
                                                               
   
int
kauth_cred_pwnam2guid(char *pwnam, guid_t *guidp)
{
	return kauth_cred_cache_lookup(KI_VALID_PWNAM, KI_VALID_GUID, pwnam, guidp);
}


  
                        
  
                                     
  
                                                      
                                       
  
                        
                                 
  
                    
                                     
  
                                                                    
                                                               
   
int
kauth_cred_grnam2guid(char *grnam, guid_t *guidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GRNAM, KI_VALID_GUID, grnam, guidp);
}


  
                      
  
                                   
  
                                                  
                                     
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_guid2uid(guid_t *guidp, uid_t *uidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GUID, KI_VALID_UID, guidp, uidp);
}


  
                      
  
                                   
  
                                                  
                                     
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_guid2gid(guid_t *guidp, gid_t *gidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GUID, KI_VALID_GID, guidp, gidp);
}

  
                               
  
                                            
  
                                                           
                                          
  
                        
                                                   
  
                    
                                       
   
int
kauth_cred_nfs4domain2dsnode(__unused char *nfs4domain, __unused char *dsnode)
{
	return ENOENT;
}

  
                               
  
                                            
  
                                                     
                                              
  
                        
                                                   
  
                    
                                          
   
int
kauth_cred_dsnode2nfs4domain(__unused char *dsnode, __unused char *nfs4domain)
{
	return ENOENT;
}

  
                       
  
                                    
  
                                                  
                                     
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_ntsid2uid(ntsid_t *sidp, uid_t *uidp)
{
	return kauth_cred_cache_lookup(KI_VALID_NTSID, KI_VALID_UID, sidp, uidp);
}


  
                       
  
                                    
  
                                                  
                                     
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_ntsid2gid(ntsid_t *sidp, gid_t *gidp)
{
	return kauth_cred_cache_lookup(KI_VALID_NTSID, KI_VALID_GID, sidp, gidp);
}


  
                        
  
                                     
  
                                                  
                                       
  
                        
                                 
  
                    
                                     
   
int
kauth_cred_ntsid2guid(ntsid_t *sidp, guid_t *guidp)
{
	return kauth_cred_cache_lookup(KI_VALID_NTSID, KI_VALID_GUID, sidp, guidp);
}


  
                      
  
                                   
  
                                    
                                       
  
                        
                                 
  
                    
                                     
   
int
kauth_cred_uid2guid(uid_t uid, guid_t *guidp)
{
	return kauth_cred_cache_lookup(KI_VALID_UID, KI_VALID_GUID, &uid, guidp);
}


  
                     
  
                                          
  
                                            
                                       
  
                        
                                 
  
                    
                                     
   
int
kauth_cred_getguid(kauth_cred_t cred, guid_t *guidp)
{
	NULLCRED_CHECK(cred);
	return kauth_cred_uid2guid(kauth_cred_getuid(cred), guidp);
}


  
                     
  
                                   
  
                                    
                                       
  
                        
                                 
  
                    
                                     
   
int
kauth_cred_gid2guid(gid_t gid, guid_t *guidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GID, KI_VALID_GUID, &gid, guidp);
}


  
                       
  
                                    
  
                                    
                                       
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_uid2ntsid(uid_t uid, ntsid_t *sidp)
{
	return kauth_cred_cache_lookup(KI_VALID_UID, KI_VALID_NTSID, &uid, sidp);
}


  
                      
  
                                           
  
                                            
                                       
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_getntsid(kauth_cred_t cred, ntsid_t *sidp)
{
	NULLCRED_CHECK(cred);
	return kauth_cred_uid2ntsid(kauth_cred_getuid(cred), sidp);
}


  
                       
  
                                    
  
                                    
                                       
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_gid2ntsid(gid_t gid, ntsid_t *sidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GID, KI_VALID_NTSID, &gid, sidp);
}


  
                        
  
                                     
  
                                                  
                                       
  
                        
                                 
  
                    
                                    
   
int
kauth_cred_guid2ntsid(guid_t *guidp, ntsid_t *sidp)
{
	return kauth_cred_cache_lookup(KI_VALID_GUID, KI_VALID_NTSID, guidp, sidp);
}


  
                          
  
                                                                           
                                                                 
                                                       
  
                                                   
                                      
                                       
                           
                                       
                           
  
                        
                                          
   
#if CONFIG_EXT_RESOLVER
static int
kauth_cred_cache_lookup(int from, int to, void *src, void *dst)
{
	struct kauth_identity ki;
	struct kauth_identity_extlookup el;
	int error;
	uint64_t extend_data = 0ULL;
	int (* expired)(struct kauth_identity *kip);
	char *namebuf = NULL;

	KAUTH_DEBUG("CACHE - translate %d to %d", from, to);

	  
                                                          
                                                                
                                                               
                                                            
               
    
	if (to == KI_VALID_PWNAM || to == KI_VALID_GRNAM) {
		if (dst == NULL) {
			return EINVAL;
		}
		namebuf = dst;
		*namebuf = '\0';
	}
	ki.ki_valid = 0;
	switch (from) {
	case KI_VALID_UID:
		error = kauth_identity_find_uid(*(uid_t *)src, &ki, namebuf);
		break;
	case KI_VALID_GID:
		error = kauth_identity_find_gid(*(gid_t *)src, &ki, namebuf);
		break;
	case KI_VALID_GUID:
		error = kauth_identity_find_guid((guid_t *)src, &ki, namebuf);
		break;
	case KI_VALID_NTSID:
		error = kauth_identity_find_ntsid((ntsid_t *)src, &ki, namebuf);
		break;
	case KI_VALID_PWNAM:
	case KI_VALID_GRNAM:
		                                            
		error = kauth_identity_find_nam((char *)src, from, &ki);
		break;
	default:
		return EINVAL;
	}
	                                                               
	if (!error && !(to & ki.ki_valid)) {
		error = ENOENT;
	}
	                             
	if (error != 0) {
		                              
		if (error != ENOENT) {
			                                            
			KAUTH_DEBUG("CACHE - cache search error %d", error);
			return error;
		}
	} else {
		                                              
		switch (to) {
		case KI_VALID_GUID:
			expired = kauth_identity_guid_expired;
			break;
		case KI_VALID_NTSID:
			expired = kauth_identity_ntsid_expired;
			break;
		case KI_VALID_GROUPS:
			expired = kauth_identity_groups_expired;
			break;
		default:
			switch (from) {
			case KI_VALID_GUID:
				expired = kauth_identity_guid_expired;
				break;
			case KI_VALID_NTSID:
				expired = kauth_identity_ntsid_expired;
				break;
			default:
				expired = NULL;
			}
		}

		  
                                                         
           
     
		if (expired) {
			if (!expired(&ki)) {
				KAUTH_DEBUG("CACHE - entry valid, unexpired");
				expired = NULL;                                      
			} else {
				  
                                                
                                                        
                                                        
                                         
       

				KAUTH_DEBUG("CACHE - expired entry found");
			}
		} else {
			KAUTH_DEBUG("CACHE - no expiry function");
		}

		if (!expired) {
			                               
			if (ki.ki_valid & to) {
				KAUTH_DEBUG("CACHE - found matching entry with valid 0x%08x", ki.ki_valid);
				DTRACE_PROC4(kauth__identity__cache__hit, int, from, int, to, void *, src, void *, dst);
				goto found;
			} else {
				  
                                                                   
                                                                  
                                                                 
                                                           
                                                      
       
				switch (from) {
				case KI_VALID_GUID:
				case KI_VALID_NTSID:
					switch (to) {
					case KI_VALID_GID:
						if ((ki.ki_valid & KI_VALID_UID)) {
							KAUTH_DEBUG("CACHE - unexpected entry 0x%08x & %x", ki.ki_valid, KI_VALID_GID);
							return ENOENT;
						}
						break;
					case KI_VALID_UID:
						if ((ki.ki_valid & KI_VALID_GID)) {
							KAUTH_DEBUG("CACHE - unexpected entry 0x%08x & %x", ki.ki_valid, KI_VALID_UID);
							return ENOENT;
						}
						break;
					}
					break;
				}
			}
		}
	}

	  
                                                       
   
                                                             
                                                          
                                                        
                                                         
                        
    
	bzero(&el, sizeof(el));
	el.el_info_pid = current_proc()->p_pid;
	switch (from) {
	case KI_VALID_UID:
		el.el_flags = KAUTH_EXTLOOKUP_VALID_UID;
		el.el_uid = *(uid_t *)src;
		break;
	case KI_VALID_GID:
		el.el_flags = KAUTH_EXTLOOKUP_VALID_GID;
		el.el_gid = *(gid_t *)src;
		break;
	case KI_VALID_GUID:
		el.el_flags = KAUTH_EXTLOOKUP_VALID_UGUID | KAUTH_EXTLOOKUP_VALID_GGUID;
		el.el_uguid = *(guid_t *)src;
		el.el_gguid = *(guid_t *)src;
		break;
	case KI_VALID_NTSID:
		el.el_flags = KAUTH_EXTLOOKUP_VALID_USID | KAUTH_EXTLOOKUP_VALID_GSID;
		el.el_usid = *(ntsid_t *)src;
		el.el_gsid = *(ntsid_t *)src;
		break;
	case KI_VALID_PWNAM:
		                    
		el.el_flags = KAUTH_EXTLOOKUP_VALID_PWNAM;
		extend_data = CAST_USER_ADDR_T(src);
		break;
	case KI_VALID_GRNAM:
		                    
		el.el_flags = KAUTH_EXTLOOKUP_VALID_GRNAM;
		extend_data = CAST_USER_ADDR_T(src);
		break;
	default:
		return EINVAL;
	}
	  
                                                                   
                                                    
   
                                                                     
                                                                   
                                                                    
                           
    
	el.el_flags |= KAUTH_EXTLOOKUP_WANT_UID | KAUTH_EXTLOOKUP_WANT_GID |
	    KAUTH_EXTLOOKUP_WANT_UGUID | KAUTH_EXTLOOKUP_WANT_GGUID |
	    KAUTH_EXTLOOKUP_WANT_USID | KAUTH_EXTLOOKUP_WANT_GSID;
	if (to == KI_VALID_PWNAM) {
		                    
		el.el_flags |= KAUTH_EXTLOOKUP_WANT_PWNAM;
		extend_data = CAST_USER_ADDR_T(dst);
	}
	if (to == KI_VALID_GRNAM) {
		                    
		el.el_flags |= KAUTH_EXTLOOKUP_WANT_GRNAM;
		extend_data = CAST_USER_ADDR_T(dst);
	}
	if (to == KI_VALID_GROUPS) {
		                                                                    
		el.el_flags |= KAUTH_EXTLOOKUP_WANT_SUPGRPS;
		if (ki.ki_valid & KI_VALID_GROUPS) {
			  
                                                            
                                                         
                                                            
                                                            
                                                           
                 
      
			if (ki.ki_supgrpcnt > NGROUPS) {
				panic("kauth data structure corrupted. kauth identity 0x%p with %u groups, greater than max of %d",
				    &ki, ki.ki_supgrpcnt, NGROUPS);
			}

			el.el_sup_grp_cnt = (uint32_t)ki.ki_supgrpcnt;

			memcpy(el.el_sup_groups, ki.ki_supgrps, sizeof(el.el_sup_groups[0]) * ki.ki_supgrpcnt);
			                                                                
			el.el_flags |= KAUTH_EXTLOOKUP_VALID_SUPGRPS;
			KAUTH_DEBUG("GROUPS: Sending previously valid GROUPS");
		} else {
			KAUTH_DEBUG("GROUPS: no valid groups to send");
		}
	}

	                   
	KAUTH_DEBUG("CACHE - calling resolver for %x", el.el_flags);

	DTRACE_PROC3(kauth__id__resolver__submitted, int, from, int, to, uintptr_t, src);

	error = kauth_resolver_submit(&el, extend_data);

	DTRACE_PROC2(kauth__id__resolver__returned, int, error, struct kauth_identity_extlookup *, &el)

	KAUTH_DEBUG("CACHE - resolver returned %d", error);

	                                         
	if (error == 0) {
		  
                                                         
                                                     
                   
    
                                                              
     
		if (from == KI_VALID_PWNAM) {
			el.el_flags |= KAUTH_EXTLOOKUP_VALID_PWNAM;
		} else if (from == KI_VALID_GRNAM) {
			el.el_flags |= KAUTH_EXTLOOKUP_VALID_GRNAM;
		}

		kauth_identity_updatecache(&el, &ki, extend_data);

		  
                                                
                                 
     
		if (!(ki.ki_valid & to)) {
			error = ENOENT;
		}
	}
	if (error) {
		return error;
	}
found:
	  
                                                               
                                               
    
	switch (to) {
	case KI_VALID_UID:
		*(uid_t *)dst = ki.ki_uid;
		break;
	case KI_VALID_GID:
		*(gid_t *)dst = ki.ki_gid;
		break;
	case KI_VALID_GUID:
		*(guid_t *)dst = ki.ki_guid;
		break;
	case KI_VALID_NTSID:
		*(ntsid_t *)dst = ki.ki_ntsid;
		break;
	case KI_VALID_GROUPS: {
		struct supgroups *gp = (struct supgroups *)dst;
		size_t limit = ki.ki_supgrpcnt;

		if (gp->count) {
			limit = MIN(ki.ki_supgrpcnt, *gp->count);
			*gp->count = limit;
		}

		memcpy(gp->groups, ki.ki_supgrps, sizeof(gid_t) * limit);
	}
	break;
	case KI_VALID_PWNAM:
	case KI_VALID_GRNAM:
		                                          
		break;
	default:
		return EINVAL;
	}
	KAUTH_DEBUG("CACHE - returned successfully");
	return 0;
}


  
                          
  
                                                                 
   

  
                       
  
                                                                        
  
                                              
                            
  
                        
                    
   
static int
kauth_groups_expired(struct kauth_group_membership *gm)
{
	struct timeval tv;

	  
                                                        
    
	if (gm->gm_expiry == 0) {
		return 0;
	}

	microuptime(&tv);

	return (gm->gm_expiry <= tv.tv_sec) ? 1 : 0;
}


  
                   
  
                                                                           
              
  
                                                    
                                    
                                 
  
                  
  
                                                                    
                                                                   
                                                                
                                                      
   
static void
kauth_groups_lru(struct kauth_group_membership *gm)
{
	if (gm != TAILQ_FIRST(&kauth_groups)) {
		TAILQ_REMOVE(&kauth_groups, gm, gm_link);
		TAILQ_INSERT_HEAD(&kauth_groups, gm, gm_link);
	}
}


  
                           
  
                                                                            
                            
  
                                                 
                                   
                                     
                         
  
                  
   
static void
kauth_groups_updatecache(struct kauth_identity_extlookup *el)
{
	struct kauth_group_membership *gm;
	struct timeval tv;

	                                                       
	if ((el->el_flags &
	    (KAUTH_EXTLOOKUP_VALID_UID | KAUTH_EXTLOOKUP_VALID_GID | KAUTH_EXTLOOKUP_VALID_MEMBERSHIP)) !=
	    (KAUTH_EXTLOOKUP_VALID_UID | KAUTH_EXTLOOKUP_VALID_GID | KAUTH_EXTLOOKUP_VALID_MEMBERSHIP)) {
		return;
	}

	microuptime(&tv);

	  
                                                                       
                                                                      
    
	KAUTH_GROUPS_LOCK();
	TAILQ_FOREACH(gm, &kauth_groups, gm_link) {
		if ((el->el_uid == gm->gm_uid) &&
		    (el->el_gid == gm->gm_gid)) {
			if (el->el_flags & KAUTH_EXTLOOKUP_ISMEMBER) {
				gm->gm_flags |= KAUTH_GROUP_ISMEMBER;
			} else {
				gm->gm_flags &= ~KAUTH_GROUP_ISMEMBER;
			}
			gm->gm_expiry = (el->el_member_valid) ? el->el_member_valid + tv.tv_sec : 0;
			kauth_groups_lru(gm);
			break;
		}
	}
	KAUTH_GROUPS_UNLOCK();

	                                               
	if (gm != NULL) {
		return;
	}

	                           
	gm = kheap_alloc(KM_KAUTH, sizeof(struct kauth_group_membership),
	    Z_WAITOK);
	if (gm != NULL) {
		gm->gm_uid = el->el_uid;
		gm->gm_gid = el->el_gid;
		if (el->el_flags & KAUTH_EXTLOOKUP_ISMEMBER) {
			gm->gm_flags |= KAUTH_GROUP_ISMEMBER;
		} else {
			gm->gm_flags &= ~KAUTH_GROUP_ISMEMBER;
		}
		gm->gm_expiry = (el->el_member_valid) ? el->el_member_valid + tv.tv_sec : 0;
	}

	  
                                                                    
                                                                      
                                                                   
                                                   
    
	KAUTH_GROUPS_LOCK();
	TAILQ_INSERT_HEAD(&kauth_groups, gm, gm_link);
	if (++kauth_groups_count > kauth_groups_cachemax) {
		gm = TAILQ_LAST(&kauth_groups, kauth_groups_head);
		TAILQ_REMOVE(&kauth_groups, gm, gm_link);
		kauth_groups_count--;
	} else {
		gm = NULL;
	}
	KAUTH_GROUPS_UNLOCK();

	                              
	kheap_free(KM_KAUTH, gm, sizeof(struct kauth_group_membership));
}

  
                                                      
  
                                                 
   
static void
kauth_groups_trimcache(int new_size)
{
	struct kauth_group_membership *gm;

	lck_mtx_assert(&kauth_groups_mtx, LCK_MTX_ASSERT_OWNED);

	while (kauth_groups_count > new_size) {
		gm = TAILQ_LAST(&kauth_groups, kauth_groups_head);
		TAILQ_REMOVE(&kauth_groups, gm, gm_link);
		kauth_groups_count--;
		kheap_free(KM_KAUTH, gm, sizeof(struct kauth_group_membership));
	}
}
#endif                           

  
                       
   

  
                          
  
                                                                              
                                                                
              
  
                                             
                                      
                                            
                          
  
                        
                                      
                                    
                              
                               
                                                               
                        
                                                            
  
                    
                                   
                      
  
                                                                       
             
  
                                                                
                                                     
   
int
kauth_cred_ismember_gid(kauth_cred_t cred, gid_t gid, int *resultp)
{
	posix_cred_t pcred = posix_cred_get(cred);
	int i;

	  
                                                     
   
                                                                        
                                           
    
	for (i = 0; i < pcred->cr_ngroups; i++) {
		if (gid == pcred->cr_groups[i]) {
			*resultp = 1;
			return 0;
		}
	}

	  
                                                                        
                                           
    
	if (pcred->cr_gmuid == KAUTH_UID_NONE) {
		*resultp = 0;
		return 0;
	}

#if CONFIG_EXT_RESOLVER
	struct kauth_group_membership *gm;
	struct kauth_identity_extlookup el;
	int error;

	  
                                                                   
                                                                 
    
	if (!kauth_resolver_registered) {
		*resultp = 0;
		return 0;
	}

	           
	                                    
	                               
	                                                   

	  
                          
    
	KAUTH_GROUPS_LOCK();
	TAILQ_FOREACH(gm, &kauth_groups, gm_link) {
		if ((gm->gm_uid == pcred->cr_gmuid) && (gm->gm_gid == gid) && !kauth_groups_expired(gm)) {
			kauth_groups_lru(gm);
			break;
		}
	}

	                                     
	if (gm != NULL) {
		*resultp = (gm->gm_flags & KAUTH_GROUP_ISMEMBER) ? 1 : 0;
	}
	KAUTH_GROUPS_UNLOCK();

	                                  
	if (gm != NULL) {
		DTRACE_PROC2(kauth__group__cache__hit, int, pcred->cr_gmuid, int, gid);
		return 0;
	}

	                                                  
	bzero(&el, sizeof(el));
	el.el_info_pid = current_proc()->p_pid;
	el.el_flags = KAUTH_EXTLOOKUP_VALID_UID | KAUTH_EXTLOOKUP_VALID_GID | KAUTH_EXTLOOKUP_WANT_MEMBERSHIP;
	el.el_uid = pcred->cr_gmuid;
	el.el_gid = gid;
	el.el_member_valid = 0;                                   

	DTRACE_PROC2(kauth__group__resolver__submitted, int, el.el_uid, int, el.el_gid);

	error = kauth_resolver_submit(&el, 0ULL);

	DTRACE_PROC2(kauth__group__resolver__returned, int, error, int, el.el_flags);

	if (error != 0) {
		return error;
	}
	                                      
	kauth_groups_updatecache(&el);

	                                                       
	if (el.el_flags & KAUTH_EXTLOOKUP_VALID_MEMBERSHIP) {
		*resultp = (el.el_flags & KAUTH_EXTLOOKUP_ISMEMBER) ? 1 : 0;
		return 0;
	}

	return ENOENT;
#else
	*resultp = 0;
	return 0;
#endif
}

  
                           
  
                                                                            
                            
  
                                             
                                        
                                     
                                            
                          
  
                        
                             
                                 
                                                               
                        
                                      
                                
                                 
                                                              
  
                    
                                   
                      
   
int
kauth_cred_ismember_guid(__unused kauth_cred_t cred, guid_t *guidp, int *resultp)
{
	int error = 0;

	switch (kauth_wellknown_guid(guidp)) {
	case KAUTH_WKG_NOBODY:
		*resultp = 0;
		break;
	case KAUTH_WKG_EVERYBODY:
		*resultp = 1;
		break;
	default:
	{
		gid_t gid;
#if CONFIG_EXT_RESOLVER
		struct kauth_identity ki;

		  
                                                     
                                                       
                                    
    
                                                        
                                                      
                                                    
    
                                                       
                                                         
                                          
     
		ki.ki_valid = 0;
		if ((error = kauth_identity_find_guid(guidp, &ki, NULL)) == 0 &&
		    !kauth_identity_guid_expired(&ki)) {
			if (ki.ki_valid & KI_VALID_GID) {
				                               
				gid = ki.ki_gid;
				goto do_check;
			}
			if (ki.ki_valid & KI_VALID_UID) {
				*resultp = 0;
				return 0;
			}
		}
#endif                          
		  
                                                     
                                                       
                                                       
                 
     
		if ((error = kauth_cred_guid2gid(guidp, &gid)) != 0) {
			  
                                                                 
                                      
      
			if (error == ENOENT) {
				*resultp = 0;
				error = 0;
			}
		} else {
#if CONFIG_EXT_RESOLVER
do_check:
#endif                          
			error = kauth_cred_ismember_gid(cred, gid, resultp);
		}
	}
	break;
	}
	return error;
}

  
                        
  
                                                                            
                                                             
  
                                               
                                               
                                            
                          
  
                        
                                              
                   
  
                    
                                   
                      
  
                                                                       
             
   
int
kauth_cred_gid_subset(kauth_cred_t cred1, kauth_cred_t cred2, int *resultp)
{
	int i, err, res = 1;
	gid_t gid;
	posix_cred_t pcred1 = posix_cred_get(cred1);
	posix_cred_t pcred2 = posix_cred_get(cred2);

	                                           
	for (i = 0; i < pcred1->cr_ngroups; i++) {
		gid = pcred1->cr_groups[i];
		if ((err = kauth_cred_ismember_gid(cred2, gid, &res)) != 0) {
			return err;
		}

		if (!res && gid != pcred2->cr_rgid && gid != pcred2->cr_svgid) {
			*resultp = 0;
			return 0;
		}
	}

	                    
	if ((err = kauth_cred_ismember_gid(cred2, pcred1->cr_rgid, &res)) != 0) {
		return err;
	}

	if (!res && pcred1->cr_rgid != pcred2->cr_rgid &&
	    pcred1->cr_rgid != pcred2->cr_svgid) {
		*resultp = 0;
		return 0;
	}

	                              
	if ((err = kauth_cred_ismember_gid(cred2, pcred1->cr_svgid, &res)) != 0) {
		return err;
	}

	if (!res && pcred1->cr_svgid != pcred2->cr_rgid &&
	    pcred1->cr_svgid != pcred2->cr_svgid) {
		*resultp = 0;
		return 0;
	}

	*resultp = 1;
	return 0;
}


  
                     
  
                                              
  
                                                    
                       
  
                               
                       
  
                                                                   
                                   
   
int
kauth_cred_issuser(kauth_cred_t cred)
{
	return kauth_cred_getuid(cred) == 0;
}


  
                 
   

                                           
static LCK_MTX_DECLARE(kauth_cred_hash_mtx, &kauth_lck_grp);
#define KAUTH_CRED_HASH_LOCK()          lck_mtx_lock(&kauth_cred_hash_mtx);
#define KAUTH_CRED_HASH_UNLOCK()        lck_mtx_unlock(&kauth_cred_hash_mtx);
#define KAUTH_CRED_HASH_LOCK_ASSERT()   LCK_MTX_ASSERT(&kauth_cred_hash_mtx, LCK_MTX_ASSERT_OWNED)


  
                  
  
                                                    
  
                     
  
                  
  
                                                                     
                                                           
                                                              
                                                               
                                                              
                                                               
                                                             
                                              
  
                                                               
                                                          
                                                          
                                                           
                                                                
                                                               
  
                                                           
                                           
  
                                                         
                         
   
void
kauth_cred_init(void)
{
	for (int i = 0; i < KAUTH_CRED_TABLE_SIZE; i++) {
		LIST_INIT(&kauth_cred_table_anchor[i]);
	}
}


  
               
  
                                                       
  
                     
  
                                               
                      
   
uid_t
kauth_getuid(void)
{
	return kauth_cred_getuid(kauth_cred_get());
}


  
                
  
                                                  
  
                     
  
                                                  
              
   
uid_t
kauth_getruid(void)
{
	return kauth_cred_getruid(kauth_cred_get());
}


  
               
  
                                                       
  
                     
  
                                               
                      
   
gid_t
kauth_getgid(void)
{
	return kauth_cred_getgid(kauth_cred_get());
}


  
               
  
                                                  
  
                     
  
                                                  
              
   
gid_t
kauth_getrgid(void)
{
	return kauth_cred_getrgid(kauth_cred_get());
}


  
                 
  
                                                                    
  
                     
  
                                                            
                  
  
                                                                       
                                                           
                                                             
                                                
  
                                                            
                                                                 
                                                             
                                                              
                                                               
                                               
   
kauth_cred_t
kauth_cred_get(void)
{
	struct proc *p;
	struct uthread *uthread;

	uthread = get_bsdthread_info(current_thread());
	            
	if (uthread == NULL) {
		panic("thread wants credential but has no BSD thread info");
	}
	  
                                                                       
              
   
                                                                
                                                    
    
	if (uthread->uu_ucred == NOCRED) {
		if ((p = (proc_t) get_bsdtask_info(get_threadtask(current_thread()))) == NULL) {
			panic("thread wants credential but has no BSD process");
		}
		uthread->uu_ucred = kauth_cred_proc_ref(p);
	}
	return uthread->uu_ucred;
}

void
mach_kauth_cred_uthread_update(void)
{
	uthread_t uthread;
	proc_t proc;

	uthread = get_bsdthread_info(current_thread());
	proc = current_proc();

	kauth_cred_uthread_update(uthread, proc);
}

  
                            
  
                                                                               
                                                
  
                                                
                                      
  
                  
  
                                                                        
                                                             
                                                             
                                                                 
               
   
void
kauth_cred_uthread_update(uthread_t uthread, proc_t proc)
{
	if (uthread->uu_ucred != proc->p_ucred &&
	    (uthread->uu_flag & UT_SETUID) == 0) {
		kauth_cred_t old = uthread->uu_ucred;
		uthread->uu_ucred = kauth_cred_proc_ref(proc);
		if (IS_VALID_CRED(old)) {
			kauth_cred_unref(&old);
		}
	}
}


  
                          
  
                                                                              
                                          
  
                     
  
                                                            
                                   
  
                                                                  
                               
  
                                                                
                                                      
  
                                                              
                                                              
                                                                  
                                                                   
                                                                   
                                    
   
kauth_cred_t
kauth_cred_get_with_ref(void)
{
	struct uthread *uthread = current_uthread();

	  
                                                                       
              
   
                                                                
                                                    
    
	if (uthread->uu_ucred == NOCRED) {
		                                           
		uthread->uu_ucred = kauth_cred_proc_ref(current_proc());
	}
	                                     
	kauth_cred_ref(uthread->uu_ucred);
	return uthread->uu_ucred;
}


  
                      
  
                                                                          
                                               
  
                                                   
                                     
  
                                                     
                                   
  
                                                                          
                                                               
                                     
  
                                                                  
                               
  
                                                                
                                                      
  
                                                              
                                                              
                                                                  
                                                                   
                                                                   
                                    
   
kauth_cred_t
kauth_cred_proc_ref(proc_t procp)
{
	kauth_cred_t    cred;

	proc_ucred_lock(procp);
	cred = proc_ucred(procp);
	kauth_cred_ref(cred);
	proc_ucred_unlock(procp);
	return cred;
}

  
                   
  
                                         
  
                     
  
                                               
                               
  
                                                                  
                                                           
                                                             
                             
  
                                                               
                                                                   
                                                               
                                                                 
                                                    
  
                                                            
                                                               
                                                              
            
  
                                                                 
                                                               
                                                   
  
                                                                   
                                                                 
                                                                
                                                                
                                                 
  
                                                                    
                                                           
                                                            
   
static kauth_cred_t
kauth_cred_alloc(void)
{
	kauth_cred_t newcred;

	newcred = zalloc_flags(ucred_zone, Z_WAITOK | Z_ZERO);
	posix_cred_get(newcred)->cr_gmuid = KAUTH_UID_NONE;
	newcred->cr_audit.as_aia_p = audit_default_aia_p;
	                                                              
#if CONFIG_MACF
	mac_cred_label_init(newcred);
#endif
	return newcred;
}


  
                  
  
                                    
  
                                             
   
static void
kauth_cred_free(kauth_cred_t cred)
{
	assert(os_atomic_load(&cred->cr_ref, relaxed) == 0);
#if CONFIG_MACF
	mac_cred_label_destroy(cred);
#endif
	AUDIT_SESSION_UNREF(cred);
	zfree(ucred_zone, cred);
}

  
                    
  
                                                                             
                                                               
                                                         
                                               
  
                                                 
                  
  
                                                          
                              
                                       
                                       
  
                                                                     
                                                               
                                                               
                      
   
kauth_cred_t
kauth_cred_create(kauth_cred_t cred)
{
	kauth_cred_t    found_cred, new_cred = NULL;
	posix_cred_t    pcred = posix_cred_get(cred);
	int is_member = 0;

	if (pcred->cr_flags & CRF_NOMEMBERD) {
		pcred->cr_gmuid = KAUTH_UID_NONE;
	} else {
		  
                                                             
                                                            
                                                           
                                                             
                                                            
                                                             
                                                           
                                                              
                                                  
     
		if (!kauth_cred_ismember_gid(cred, 0, &is_member)) {
			  
                                                         
                                                        
                                 
      
			pcred->cr_gmuid = pcred->cr_uid;
		} else {
			  
                                                    
                                                  
                                                        
                                                     
                                                      
                                                     
                                                         
      
			pcred->cr_gmuid = KAUTH_UID_NONE;
			pcred->cr_flags |= CRF_NOMEMBERD;
		}
	}

	                                                             
	if (pcred->cr_ngroups < 1) {
		return NULL;
	}

	struct kauth_cred_entry_head *bucket = kauth_cred_get_bucket(cred);

	KAUTH_CRED_HASH_LOCK();
	found_cred = kauth_cred_find_and_ref(cred, bucket);
	KAUTH_CRED_HASH_UNLOCK();
	if (found_cred != NULL) {
		return found_cred;
	}

	  
                                                           
                   
    
	new_cred = kauth_cred_alloc();
	if (new_cred != NULL) {
		*posix_cred_get(new_cred) = *pcred;
#if CONFIG_AUDIT
		new_cred->cr_audit = cred->cr_audit;
#endif
		new_cred = kauth_cred_add(new_cred, bucket);
	}

	return new_cred;
}


  
                       
  
                                                                               
                                                                
                                                   
  
                                              
                            
                                 
                              
                                        
                            
  
                                                   
  
                                                               
                                                           
                               
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_setresuid(kauth_cred_t cred, uid_t ruid, uid_t euid, uid_t svuid, uid_t gmuid)
{
	struct ucred temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred = posix_cred_get(cred);

	NULLCRED_CHECK(cred);

	  
                                                                
                                          
    
	if ((euid == KAUTH_UID_NONE || pcred->cr_uid == euid) &&
	    (ruid == KAUTH_UID_NONE || pcred->cr_ruid == ruid) &&
	    (svuid == KAUTH_UID_NONE || pcred->cr_svuid == svuid) &&
	    (pcred->cr_gmuid == gmuid)) {
		                      
		return cred;
	}

	  
                                                                      
                                                                     
    
	temp_cred = *cred;
	if (euid != KAUTH_UID_NONE) {
		temp_pcred->cr_uid = euid;
	}
	if (ruid != KAUTH_UID_NONE) {
		temp_pcred->cr_ruid = ruid;
	}
	if (svuid != KAUTH_UID_NONE) {
		temp_pcred->cr_svuid = svuid;
	}

	  
                                                                  
                                                                    
                                           
    
	if ((temp_pcred->cr_gmuid = gmuid) == KAUTH_UID_NONE) {
		temp_pcred->cr_flags |= CRF_NOMEMBERD;
	}

	return kauth_cred_update(cred, &temp_cred, TRUE);
}


  
                       
  
                                                                               
                                                               
        
  
                                              
                            
                                 
                              
  
                                                   
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_setresgid(kauth_cred_t cred, gid_t rgid, gid_t egid, gid_t svgid)
{
	struct ucred    temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred = posix_cred_get(cred);

	NULLCRED_CHECK(cred);
	DEBUG_CRED_ENTER("kauth_cred_setresgid %p %d %d %d\n", cred, rgid, egid, svgid);

	  
                                                                 
                                       
    
	if (pcred->cr_groups[0] == egid &&
	    pcred->cr_rgid == rgid &&
	    pcred->cr_svgid == svgid) {
		                      
		return cred;
	}

	  
                                                                      
                                                                     
    
	temp_cred = *cred;
	if (egid != KAUTH_GID_NONE) {
		                                                             
		if (kauth_cred_change_egid(&temp_cred, egid)) {
			DEBUG_CRED_CHANGE("displaced!\n");
			temp_pcred->cr_flags |= CRF_NOMEMBERD;
			temp_pcred->cr_gmuid = KAUTH_UID_NONE;
		} else {
			DEBUG_CRED_CHANGE("not displaced\n");
		}
	}
	if (rgid != KAUTH_GID_NONE) {
		temp_pcred->cr_rgid = rgid;
	}
	if (svgid != KAUTH_GID_NONE) {
		temp_pcred->cr_svgid = svgid;
	}

	return kauth_cred_update(cred, &temp_cred, TRUE);
}


  
                                                                             
                                                                            
              
                                                                       
                                                    
   
  
                       
  
                                                                           
                                       
  
                                              
                                          
                                   
                                                
                                 
                                        
                            
  
                                                   
  
                                                               
                                                           
                               
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
  
                                                                       
                                                          
                                                                 
                                                                
                                                             
                          
  
                                                                 
                               
   
kauth_cred_t
kauth_cred_setgroups(kauth_cred_t cred, gid_t *groups, size_t groupcount, uid_t gmuid)
{
	size_t i;
	struct ucred temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred;

	NULLCRED_CHECK(cred);
	assert(groupcount <= NGROUPS);
	groupcount = MIN(groupcount, NGROUPS);

	pcred = posix_cred_get(cred);

	  
                                                                     
           
    
	if ((pcred->cr_gmuid == gmuid) && (pcred->cr_ngroups == groupcount)) {
		for (i = 0; i < groupcount; i++) {
			if (pcred->cr_groups[i] != groups[i]) {
				break;
			}
		}
		if (i == groupcount) {
			                      
			return cred;
		}
	}

	  
                                                                      
                                                                   
                                                                      
                                                                  
                                                                
    
	temp_cred = *cred;
	temp_pcred->cr_ngroups = (short)groupcount;
	bcopy(groups, temp_pcred->cr_groups, groupcount * sizeof(temp_pcred->cr_groups[0]));
	temp_pcred->cr_gmuid = gmuid;
	if (gmuid == KAUTH_UID_NONE) {
		temp_pcred->cr_flags |= CRF_NOMEMBERD;
	} else {
		temp_pcred->cr_flags &= ~CRF_NOMEMBERD;
	}

	return kauth_cred_update(cred, &temp_cred, TRUE);
}

  
                                                                     
                                                                 
                                                              
   
#if CONFIG_EXT_RESOLVER
int kauth_external_supplementary_groups_supported = 1;

SYSCTL_INT(_kern, OID_AUTO, ds_supgroups_supported, CTLFLAG_RW | CTLFLAG_LOCKED, &kauth_external_supplementary_groups_supported, 0, "");
#endif

int
kauth_cred_getgroups(kauth_cred_t cred, gid_t *grouplist, size_t *countp)
{
	size_t limit = NGROUPS;
	posix_cred_t pcred;

	pcred = posix_cred_get(cred);

#if CONFIG_EXT_RESOLVER
	  
                                                                                 
                                                                                    
                                                                                
    
	if (kauth_identitysvc_has_registered && kauth_external_supplementary_groups_supported && (pcred->cr_flags & CRF_NOMEMBERD) == 0) {
		uid_t uid = kauth_cred_getuid(cred);
		int err;

		err = kauth_cred_uid2groups(&uid, grouplist, countp);
		if (!err) {
			return 0;
		}

		                                
		KAUTH_DEBUG("kauth_cred_getgroups failed %d\n", err);
	}
#endif                          

	  
                                                                  
                                                                     
                                                                       
                                             
    
	if (countp) {
		limit = MIN(*countp, pcred->cr_ngroups);
		*countp = limit;
	}

	memcpy(grouplist, pcred->cr_groups, sizeof(gid_t) * limit);

	return 0;
}


  
                       
  
                                                                            
                                                                 
                                                               
                            
  
                                              
                             
                             
  
                                                   
  
                                                                     
                                                              
                               
  
                                                                
                                                             
                                                           
                
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_setuidgid(kauth_cred_t cred, uid_t uid, gid_t gid)
{
	struct ucred temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred;

	NULLCRED_CHECK(cred);

	pcred = posix_cred_get(cred);

	  
                                                                 
                                                                
    
	if (pcred->cr_uid == uid && pcred->cr_ruid == uid && pcred->cr_svuid == uid &&
	    pcred->cr_gid == gid && pcred->cr_rgid == gid && pcred->cr_svgid == gid) {
		                      
		return cred;
	}

	  
                                                                      
                        
    
	bzero(&temp_cred, sizeof(temp_cred));
	temp_pcred->cr_uid = uid;
	temp_pcred->cr_ruid = uid;
	temp_pcred->cr_svuid = uid;
	temp_pcred->cr_flags = pcred->cr_flags;
	                                    
	if (pcred->cr_flags & CRF_NOMEMBERD) {
		temp_pcred->cr_gmuid = KAUTH_UID_NONE;
		temp_pcred->cr_flags |= CRF_NOMEMBERD;
	} else {
		temp_pcred->cr_gmuid = uid;
		temp_pcred->cr_flags &= ~CRF_NOMEMBERD;
	}
	temp_pcred->cr_ngroups = 1;
	                                                             
	if (kauth_cred_change_egid(&temp_cred, gid)) {
		temp_pcred->cr_gmuid = KAUTH_UID_NONE;
		temp_pcred->cr_flags |= CRF_NOMEMBERD;
	}
	temp_pcred->cr_rgid = gid;
	temp_pcred->cr_svgid = gid;
#if CONFIG_MACF
	temp_cred.cr_label = cred->cr_label;
#endif

	return kauth_cred_update(cred, &temp_cred, TRUE);
}


  
                         
  
                                                                           
                          
  
                                               
                               
                               
  
                                                   
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_setsvuidgid(kauth_cred_t cred, uid_t uid, gid_t gid)
{
	struct ucred temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred;

	NULLCRED_CHECK(cred);

	pcred = posix_cred_get(cred);

	DEBUG_CRED_ENTER("kauth_cred_setsvuidgid: %p u%d->%d g%d->%d\n", cred, cred->cr_svuid, uid, cred->cr_svgid, gid);

	  
                                                                 
                                                                 
                        
    
	if (pcred->cr_svuid == uid && pcred->cr_svgid == gid) {
		                      
		return cred;
	}
	DEBUG_CRED_CHANGE("kauth_cred_setsvuidgid: cred change\n");

	                                                                     
                    
    
	temp_cred = *cred;
	temp_pcred->cr_svuid = uid;
	temp_pcred->cr_svgid = gid;

	return kauth_cred_update(cred, &temp_cred, TRUE);
}


  
                          
  
                                                                         
  
                                              
                                                 
  
                                                   
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_setauditinfo(kauth_cred_t cred, au_session_t *auditinfo_p)
{
	struct ucred temp_cred;

	NULLCRED_CHECK(cred);

	  
                                                                 
                                                      
    
	if (bcmp(&cred->cr_audit, auditinfo_p, sizeof(cred->cr_audit)) == 0) {
		                      
		return cred;
	}

	temp_cred = *cred;
	bcopy(auditinfo_p, &temp_cred.cr_audit, sizeof(temp_cred.cr_audit));

	return kauth_cred_update(cred, &temp_cred, FALSE);
}

#if CONFIG_MACF
  
                          
  
                                                                 
  
                                              
                                 
  
                                                   
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   
kauth_cred_t
kauth_cred_label_update(kauth_cred_t cred, struct label *label)
{
	kauth_cred_t newcred;
	struct ucred temp_cred;

	temp_cred = *cred;

	mac_cred_label_init(&temp_cred);
	mac_cred_label_associate(cred, &temp_cred);
	mac_cred_label_update(&temp_cred, label);

	newcred = kauth_cred_update(cred, &temp_cred, TRUE);
	mac_cred_label_destroy(&temp_cred);
	return newcred;
}

  
                                 
  
                                                                    
                
  
                                              
                        
                                   
                                     
                                             
                                    
                
  
                                                   
  
                    
                                            
  
                                                                          
                                                               
                                                                  
                                                                  
                                                      
  
                                                           
                                                              
                                                              
                                                           
                         
   

static
kauth_cred_t
kauth_cred_label_update_execve(kauth_cred_t cred, vfs_context_t ctx,
    struct vnode *vp, off_t offset, struct vnode *scriptvp, struct label *scriptl,
    struct label *execl, unsigned int *csflags, void *macextensions, int *disjointp, int *labelupdateerror)
{
	kauth_cred_t newcred;
	struct ucred temp_cred;

	temp_cred = *cred;

	mac_cred_label_init(&temp_cred);
	mac_cred_label_associate(cred, &temp_cred);
	mac_cred_label_update_execve(ctx, &temp_cred,
	    vp, offset, scriptvp, scriptl, execl, csflags,
	    macextensions, disjointp, labelupdateerror);

	newcred = kauth_cred_update(cred, &temp_cred, TRUE);
	mac_cred_label_destroy(&temp_cred);
	return newcred;
}

  
                           
  
                                                                                    
  
                                        
                                                         
  
                                                                            
                                                                          
                                      
   
int
kauth_proc_label_update(struct proc *p, struct label *label)
{
	kauth_cred_t my_cred, my_new_cred;

	my_cred = kauth_cred_proc_ref(p);

	DEBUG_CRED_ENTER("kauth_proc_label_update: %p\n", my_cred);

	                                                                       
	for (;;) {
		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_label_update(my_cred, label);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("kauth_proc_setlabel_unlocked CH(%d): %p/0x%08x -> %p/0x%08x\n", p->p_pid, my_cred, my_cred->cr_flags, my_new_cred, my_new_cred->cr_flags);

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                       
                                           
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				my_cred = kauth_cred_proc_ref(p);
				               
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);

			proc_ucred_unlock(p);
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	return 0;
}

  
                                  
  
                                                                          
                                                              
                                                              
                      
  
                                        
                                 
                                
                                  
                                    
                                                               
                                            
  
                                                    
               
                                            
               
  
                                                                     
                                                                  
                                                 
   

void
kauth_proc_label_update_execve(struct proc *p, vfs_context_t ctx,
    struct vnode *vp, off_t offset, struct vnode *scriptvp, struct label *scriptl,
    struct label *execl, unsigned int *csflags, void *macextensions, int *disjoint, int *update_return)
{
	kauth_cred_t my_cred, my_new_cred;
	my_cred = kauth_cred_proc_ref(p);

	DEBUG_CRED_ENTER("kauth_proc_label_update_execve: %p\n", my_cred);

	                                                                       
	for (;;) {
		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_label_update_execve(my_cred, ctx, vp, offset, scriptvp, scriptl, execl, csflags, macextensions, disjoint, update_return);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("kauth_proc_label_update_execve_unlocked CH(%d): %p/0x%08x -> %p/0x%08x\n", p->p_pid, my_cred, my_cred->cr_flags, my_new_cred, my_new_cred->cr_flags);

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                       
                                           
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				my_cred = kauth_cred_proc_ref(p);
				               
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			proc_ucred_unlock(p);
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);
}

#if 1
  
                                     
   
kauth_cred_t    kauth_cred_setlabel(kauth_cred_t cred, struct label *label);
kauth_cred_t
kauth_cred_setlabel(kauth_cred_t cred, struct label *label)
{
	return kauth_cred_label_update(cred, label);
}

int kauth_proc_setlabel(struct proc *p, struct label *label);
int
kauth_proc_setlabel(struct proc *p, struct label *label)
{
	return kauth_proc_label_update(p, label);
}
#endif

#else

                                                                                    
                                                                                   
                    
   
kauth_cred_t
kauth_cred_label_update(__unused kauth_cred_t cred, __unused void *label)
{
	return NULL;
}

int
kauth_proc_label_update(__unused struct proc *p, __unused void *label)
{
	return 0;
}

#if 1
  
                                     
   
kauth_cred_t    kauth_cred_setlabel(kauth_cred_t cred, void *label);
kauth_cred_t
kauth_cred_setlabel(__unused kauth_cred_t cred, __unused void *label)
{
	return NULL;
}

int kauth_proc_setlabel(struct proc *p, void *label);
int
kauth_proc_setlabel(__unused struct proc *p, __unused void *label)
{
	return 0;
}
#endif
#endif

                                                         

#define KAUTH_CRED_REF_MAX 0x0ffffffful

__attribute__((noinline, cold, noreturn))
static void
kauth_cred_panic_resurrection(kauth_cred_t cred)
{
	panic("kauth_cred_unref: cred %p resurrected", cred);
	__builtin_unreachable();
}

__attribute__((noinline, cold, noreturn))
static void
kauth_cred_panic_over_released(kauth_cred_t cred)
{
	panic("kauth_cred_unref: cred %p over-released", cred);
	__builtin_unreachable();
}

__attribute__((noinline, cold, noreturn))
static void
kauth_cred_panic_over_retain(kauth_cred_t cred)
{
	panic("kauth_cred_ref: cred %p over-retained", cred);
	__builtin_unreachable();
}

  
                    
  
                                                                            
                                               
  
                                                  
  
                                                     
   
static inline bool
kauth_cred_tryref(kauth_cred_t cred)
{
	u_long old_ref, new_ref;
	os_atomic_rmw_loop(&cred->cr_ref, old_ref, new_ref, relaxed, {
		if (old_ref == 0) {
		        os_atomic_rmw_loop_give_up(return false);
		}
		new_ref = old_ref + 1;
	});
	if (__improbable(old_ref >= KAUTH_CRED_REF_MAX)) {
		kauth_cred_panic_over_retain(cred);
	}

	return true;
}

  
                 
  
                                                        
  
                                                  
  
                  
   
void
kauth_cred_ref(kauth_cred_t cred)
{
	u_long old_ref = os_atomic_inc_orig(&cred->cr_ref, relaxed);

	if (__improbable(old_ref < 1)) {
		kauth_cred_panic_resurrection(cred);
	}
	if (__improbable(old_ref >= KAUTH_CRED_REF_MAX)) {
		kauth_cred_panic_over_retain(cred);
	}
}

  
                        
  
                                               
  
                                                     
                              
  
                                                
                                      
  
   
static inline bool
kauth_cred_unref_fast(kauth_cred_t cred)
{
	u_long old_ref = os_atomic_dec_orig(&cred->cr_ref, relaxed);

	if (__improbable(old_ref <= 0)) {
		kauth_cred_panic_over_released(cred);
	}
	return old_ref == 1;
}

  
                   
  
                                               
                                               
  
                                                     
                              
  
                  
  
                    
                           
  
   
void
kauth_cred_unref(kauth_cred_t *credp)
{
	if (kauth_cred_unref_fast(*credp)) {
		KAUTH_CRED_HASH_LOCK();
		kauth_cred_remove_locked(*credp);
		KAUTH_CRED_HASH_UNLOCK();
		kauth_cred_free(*credp);
	}

	*credp = NOCRED;
}


#ifndef __LP64__
  
                  
  
                                                                          
                                          
  
                                            
  
                  
  
                                                                           
                                                                 
                                                                
                       
   
void
kauth_cred_rele(kauth_cred_t cred)
{
	kauth_cred_unref(&cred);
}
#endif                


  
                 
  
                                                                             
                     
  
                                                  
  
                                                     
  
                                                                       
                                                                  
                        
  
                                                              
                                                               
                            
  
                                                           
                                                           
                                                             
                             
  
                                                               
                                                                   
                                                               
                                                                 
                                                   
  
                                                            
                                                               
                                                            
            
  
                                                                 
                                                               
                                                   
  
   
static kauth_cred_t
kauth_cred_dup(kauth_cred_t cred)
{
	kauth_cred_t newcred;

	assert(cred != NOCRED && cred != FSCRED);
	newcred = kauth_cred_alloc();
	if (newcred != NULL) {
		newcred->cr_posix = cred->cr_posix;
#if CONFIG_AUDIT
		newcred->cr_audit = cred->cr_audit;
#endif
#if CONFIG_MACF
		mac_cred_label_associate(cred, newcred);
#endif
		AUDIT_SESSION_REF(cred);
	}
	return newcred;
}

  
                       
  
                                                                             
                                                        
  
                                                   
                                 
  
                                                  
  
                                                                      
                                                           
                                                             
                                                             
                                   
   
kauth_cred_t
kauth_cred_copy_real(kauth_cred_t cred)
{
	kauth_cred_t newcred = NULL, found_cred;
	struct ucred temp_cred;
	posix_cred_t temp_pcred = posix_cred_get(&temp_cred);
	posix_cred_t pcred = posix_cred_get(cred);

	                                                                
	if ((pcred->cr_ruid == pcred->cr_uid) &&
	    (pcred->cr_rgid == pcred->cr_gid)) {
		kauth_cred_ref(cred);
		return cred;
	}

	  
                                                                      
                        
    
	temp_cred = *cred;
	temp_pcred->cr_uid = pcred->cr_ruid;
	                                                             
	if (kauth_cred_change_egid(&temp_cred, pcred->cr_rgid)) {
		temp_pcred->cr_flags |= CRF_NOMEMBERD;
		temp_pcred->cr_gmuid = KAUTH_UID_NONE;
	}
	  
                                                                   
                    
    
	if (temp_pcred->cr_gmuid != KAUTH_UID_NONE) {
		temp_pcred->cr_gmuid = pcred->cr_ruid;
	}

	struct kauth_cred_entry_head *bucket = kauth_cred_get_bucket(cred);

	KAUTH_CRED_HASH_LOCK();
	found_cred = kauth_cred_find_and_ref(&temp_cred, bucket);
	KAUTH_CRED_HASH_UNLOCK();

	if (found_cred) {
		return found_cred;
	}

	  
                                                          
                                                
    
	newcred = kauth_cred_dup(&temp_cred);
	return kauth_cred_add(newcred, bucket);
}


  
                    
  
                                                  
  
                                                      
                 
                                            
                                  
                                
                                                   
                                   
                                     
                      
  
                                                   
  
                                                                           
                                                             
                                                              
                                                             
                                         
  
                                                           
                                                              
                                                              
                                                           
                         
   
static kauth_cred_t
kauth_cred_update(kauth_cred_t old_cred, kauth_cred_t model_cred,
    boolean_t retain_auditinfo)
{
	kauth_cred_t cred;

	  
                                                                  
                                                  
    
	if (retain_auditinfo) {
		model_cred->cr_audit = old_cred->cr_audit;
	}

	if (kauth_cred_is_equal(old_cred, model_cred)) {
		return old_cred;
	}

	struct kauth_cred_entry_head *bucket = kauth_cred_get_bucket(model_cred);

	KAUTH_CRED_HASH_LOCK();
	cred = kauth_cred_find_and_ref(model_cred, bucket);
	if (cred != NULL) {
		  
                                                       
                                                            
                                                                   
                                                    
     
		bool needs_free = kauth_cred_unref_fast(old_cred);
		if (needs_free) {
			kauth_cred_remove_locked(old_cred);
		}
		KAUTH_CRED_HASH_UNLOCK();

		DEBUG_CRED_CHANGE("kauth_cred_update(cache hit): %p -> %p\n",
		    old_cred, cred);
		if (needs_free) {
			kauth_cred_free(old_cred);
		}
		return cred;
	}

	KAUTH_CRED_HASH_UNLOCK();

	  
                                                         
                                                         
    
	cred = kauth_cred_dup(model_cred);
	cred = kauth_cred_add(cred, bucket);
	DEBUG_CRED_CHANGE("kauth_cred_update(cache miss): %p -> %p\n",
	    old_cred, cred);


	  
                                                                    
                                                   
    
	kauth_cred_unref(&old_cred);
	return cred;
}


  
                 
  
                                                                         
                                                             
              
  
                                                        
                                      
                                
  
                                                       
                                 
  
                                                                   
   
static kauth_cred_t
kauth_cred_add(kauth_cred_t new_cred, struct kauth_cred_entry_head *bucket)
{
	kauth_cred_t found_cred;
	u_long old_ref;

	KAUTH_CRED_HASH_LOCK();
	found_cred = kauth_cred_find_and_ref(new_cred, bucket);
	if (found_cred) {
		KAUTH_CRED_HASH_UNLOCK();
		kauth_cred_free(new_cred);
		return found_cred;
	}

	old_ref = os_atomic_xchg(&new_cred->cr_ref, 1, relaxed);
	if (old_ref != 0) {
		panic("kauth_cred_add: invalid cred %p", new_cred);
	}

	                                               
	LIST_INSERT_HEAD(bucket, new_cred, cr_link);

	KAUTH_CRED_HASH_UNLOCK();
	return new_cred;
}

  
                           
  
                                                                           
  
                                            
  
                                                         
   
static void
kauth_cred_remove_locked(kauth_cred_t cred)
{
	KAUTH_CRED_HASH_LOCK_ASSERT();

	if (cred->cr_link.le_prev == NULL) {
		panic("kauth_cred_unref: cred %p never added", cred);
	}

	LIST_REMOVE(cred, cr_link);
}

  
                      
  
                                                              
  
                                             
                                              
  
                                         
                                      
   
static bool
kauth_cred_is_equal(kauth_cred_t cred1, kauth_cred_t cred2)
{
	posix_cred_t pcred1 = posix_cred_get(cred1);
	posix_cred_t pcred2 = posix_cred_get(cred2);

	  
                                                   
                                 
    
	if (memcmp(pcred1, pcred2, sizeof(*pcred1))) {
		return false;
	}
	if (memcmp(&cred1->cr_audit, &cred2->cr_audit, sizeof(cred1->cr_audit))) {
		return false;
	}
#if CONFIG_MACF
	                                                                    
	if (pcred1->cr_flags & CRF_MAC_ENFORCE) {
		if (!mac_cred_label_is_equal(cred1->cr_label, cred2->cr_label)) {
			return false;
		}
	}
#endif
	return true;
}

  
                          
  
                                                                        
                         
  
                                                   
                  
  
                             
                                           
                                      
  
                                                         
   
static kauth_cred_t
kauth_cred_find_and_ref(kauth_cred_t cred, struct kauth_cred_entry_head *bucket)
{
	kauth_cred_t found_cred;

	KAUTH_CRED_HASH_LOCK_ASSERT();

	                                            
	LIST_FOREACH(found_cred, bucket, cr_link) {
		if (kauth_cred_is_equal(found_cred, cred)) {
			  
                                             
                                              
                               
      
			if (!kauth_cred_tryref(found_cred)) {
				found_cred = NULL;
			}
			break;
		}
	}

	return found_cred;
}

  
                  
  
                                                                             
                                                   
  
                                                   
                  
  
                             
                                           
                       
  
                                                         
   
kauth_cred_t
kauth_cred_find(kauth_cred_t cred)
{
	struct kauth_cred_entry_head *bucket = kauth_cred_get_bucket(cred);
	kauth_cred_t found_cred;

	KAUTH_CRED_HASH_LOCK_ASSERT();

	                                            
	LIST_FOREACH(found_cred, bucket, cr_link) {
		if (kauth_cred_is_equal(found_cred, cred)) {
			break;
		}
	}

	return found_cred;
}


  
                        
  
                                                                          
                                                             
                                                                
                          
  
                                                      
  
                                                   
               
  
                                                      
  
                                                                      
                                             
   
static struct kauth_cred_entry_head *
kauth_cred_get_bucket(kauth_cred_t cred)
{
#if CONFIG_MACF
	posix_cred_t pcred = posix_cred_get(cred);
#endif
	uint32_t hash_key = 0;

	hash_key = os_hash_jenkins_update(&cred->cr_posix,
	    sizeof(struct posix_cred), hash_key);

	hash_key = os_hash_jenkins_update(&cred->cr_audit,
	    sizeof(struct au_session), hash_key);
#if CONFIG_MACF
	if (pcred->cr_flags & CRF_MAC_ENFORCE) {
		hash_key = mac_cred_label_hash_update(cred->cr_label, hash_key);
	}
#endif                  

	hash_key = os_hash_jenkins_finish(hash_key);
	hash_key %= KAUTH_CRED_TABLE_SIZE;
	return &kauth_cred_table_anchor[hash_key];
}


  
                                                                       
                                                                     
                     
                                                                       
   

  
                    
  
                                                                           
                                                            
  
                                                             
                  
  
                                                                
                      
                                                
                                         
                                           
  
                                                                       
                                                                    
                                                               
                                                               
                                                              
                                            
  
                                                   
  
                                                              
                                                          
                                                              
  
                                                            
                                                               
                                                              
                                                           
                                                                  
                                                                 
                                                             
   
kauth_cred_t
posix_cred_create(posix_cred_t pcred)
{
	struct ucred temp_cred;

	bzero(&temp_cred, sizeof(temp_cred));
	temp_cred.cr_posix = *pcred;

	return kauth_cred_create(&temp_cred);
}


  
                 
  
                                                                           
                                     
  
                                                             
  
                                                    
  
                                                                       
                                                               
                                                                  
                                                             
                                                               
                                                                  
                                                                
                                                             
  
                                                                 
                                                                
                                                                 
                                                             
                                                               
                                                                
                                                                
           
  
                                                               
                                                              
                                                                
         
   
posix_cred_t
posix_cred_get(kauth_cred_t cred)
{
	return &cred->cr_posix;
}


  
                   
  
                                                                  
  
                                             
                                                
  
                  
  
                                                                      
                                                                
                                                           
                                                             
                                                             
                                                     
  
                                                                
                                                               
                                                             
   
void
posix_cred_label(kauth_cred_t cred, posix_cred_t pcred)
{
	cred->cr_posix = *pcred;                                      
}


  
                    
  
                                                                   
  
                                             
                                                     
                                                     
                                                       
                                               
  
                                
                             
  
                                                                       
                                                                 
                                                                
                       
   
int
posix_cred_access(kauth_cred_t cred, id_t object_uid, id_t object_gid, mode_t object_mode, mode_t mode_req)
{
	int is_member;
	mode_t mode_owner = (object_mode & S_IRWXU);
	mode_t mode_group = (mode_t)((object_mode & S_IRWXG) << 3);
	mode_t mode_world = (mode_t)((object_mode & S_IRWXO) << 6);

	  
                                
    
	if (kauth_cred_getuid(cred) == object_uid && (mode_req & mode_owner) == mode_req) {
		return 0;
	}

	  
                                                                        
   
                                                                       
                                                                       
                                                                        
    
	if ((mode_req & mode_group & mode_world) == mode_req) {
		return 0;
	} else {
		  
                                                    
     
		if ((mode_req & mode_group) != mode_req) {
			  
                                                     
     
                                                        
      
			if (!kauth_cred_ismember_gid(cred, object_gid, &is_member) && is_member) {
				  
                          
       
				return EACCES;
			} else {
				if ((mode_req & mode_world) != mode_req) {
					  
                                            
        
					return EACCES;
				} else {
					  
                                           
        
					return 0;
				}
			}
		} else {
			  
                                                     
     
                                                      
                 
      
			if (!kauth_cred_ismember_gid(cred, object_gid, &is_member) && is_member) {
				  
                               
       
				return 0;
			} else {
				if ((mode_req & mode_world) != mode_req) {
					  
                                            
        
					return EACCES;
				} else {
					  
                                           
        
					return 0;
				}
			}
		}
	}
}
