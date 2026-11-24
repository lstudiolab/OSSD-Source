/*
 * Copyright (c) 2004-2016 Apple Inc. All rights reserved.
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

  
                                       
   

#include <sys/appleapiopts.h>
#include <sys/param.h>                         
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/malloc.h>
#include <sys/vnode_internal.h>
#include <sys/kauth.h>
#include <sys/stat.h>

#include <security/audit/audit.h>

#include <sys/mount.h>
#include <sys/sysproto.h>
#include <mach/message.h>
#include <mach/host_security.h>

#include <kern/locks.h>


  
                        
   

LCK_GRP_DECLARE(kauth_lck_grp, "kauth");
static LCK_MTX_DECLARE(kauth_scope_mtx, &kauth_lck_grp);
#define KAUTH_SCOPELOCK()       lck_mtx_lock(&kauth_scope_mtx);
#define KAUTH_SCOPEUNLOCK()     lck_mtx_unlock(&kauth_scope_mtx);

  
                                                                     
                                                                             
                                                                             
                                                                          
   
struct kauth_listener {
	TAILQ_ENTRY(kauth_listener)     kl_link;
	const char *                            kl_identifier;
	kauth_scope_callback_t          kl_callback;
	void *                                          kl_idata;
};

                                                                             
                                                                                
                                                                                
              
                                                                          
                                                                              
                                                                               
                                                                             
                               
   
struct kauth_local_listener {
	kauth_listener_t                        kll_listenerp;
	kauth_scope_callback_t          kll_callback;
	void *                                          kll_idata;
};
typedef struct kauth_local_listener *kauth_local_listener_t;

static TAILQ_HEAD(, kauth_listener) kauth_dangling_listeners =
    TAILQ_HEAD_INITIALIZER(kauth_dangling_listeners);

  
                                                     
                                                                           
                                                   
                   
   
#define KAUTH_SCOPE_MAX_LISTENERS  15

struct kauth_scope {
	TAILQ_ENTRY(kauth_scope)        ks_link;
	volatile struct kauth_local_listener  ks_listeners[KAUTH_SCOPE_MAX_LISTENERS];
	const char *                            ks_identifier;
	kauth_scope_callback_t          ks_callback;
	void *                                          ks_idata;
	u_int                                           ks_flags;
};

                                     
#define KS_F_HAS_LISTENERS              (1 << 0)

static TAILQ_HEAD(, kauth_scope) kauth_scopes = TAILQ_HEAD_INITIALIZER(kauth_scopes);

static int kauth_add_callback_to_scope(kauth_scope_t sp, kauth_listener_t klp);
static void     kauth_scope_init(void);
static kauth_scope_t kauth_alloc_scope(const char *identifier, kauth_scope_callback_t callback, void *idata);
static kauth_listener_t kauth_alloc_listener(const char *identifier, kauth_scope_callback_t callback, void *idata);
#if 0
static int      kauth_scope_valid(kauth_scope_t scope);
#endif

kauth_scope_t   kauth_scope_process;
static int      kauth_authorize_process_callback(kauth_cred_t _credential, void *_idata, kauth_action_t _action,
    uintptr_t arg0, uintptr_t arg1, __unused uintptr_t arg2, __unused uintptr_t arg3);
kauth_scope_t   kauth_scope_generic;
static int      kauth_authorize_generic_callback(kauth_cred_t _credential, void *_idata, kauth_action_t _action,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
kauth_scope_t   kauth_scope_fileop;

extern int              cansignal(struct proc *, kauth_cred_t, struct proc *, int);
extern char *   get_pathbuff(void);
extern void             release_pathbuff(char *path);

  
                  
   
void
kauth_init(void)
{
	                                         
	kauth_cred_init();
	kauth_scope_init();
}

static void
kauth_scope_init(void)
{
	kauth_scope_process = kauth_register_scope(KAUTH_SCOPE_PROCESS, kauth_authorize_process_callback, NULL);
	kauth_scope_generic = kauth_register_scope(KAUTH_SCOPE_GENERIC, kauth_authorize_generic_callback, NULL);
	kauth_scope_fileop = kauth_register_scope(KAUTH_SCOPE_FILEOP, NULL, NULL);
}

  
                      
   

static kauth_scope_t
kauth_alloc_scope(const char *identifier, kauth_scope_callback_t callback, void *idata)
{
	kauth_scope_t   sp;

	  
                                              
    
	sp = kheap_alloc(KM_KAUTH, sizeof(*sp), Z_WAITOK | Z_ZERO);
	if (sp == NULL) {
		return NULL;
	}
	sp->ks_flags = 0;
	sp->ks_identifier = identifier;
	sp->ks_idata = idata;
	sp->ks_callback = callback;
	return sp;
}

static kauth_listener_t
kauth_alloc_listener(const char *identifier, kauth_scope_callback_t callback, void *idata)
{
	kauth_listener_t lsp;

	  
                                                 
    
	lsp = kheap_alloc(KM_KAUTH, sizeof(*lsp), Z_WAITOK);
	if (lsp == NULL) {
		return NULL;
	}
	lsp->kl_identifier = identifier;
	lsp->kl_idata = idata;
	lsp->kl_callback = callback;
	return lsp;
}

kauth_scope_t
kauth_register_scope(const char *identifier, kauth_scope_callback_t callback, void *idata)
{
	kauth_scope_t           sp, tsp;
	kauth_listener_t        klp;

	if ((sp = kauth_alloc_scope(identifier, callback, idata)) == NULL) {
		return NULL;
	}

	  
                             
    
	KAUTH_SCOPELOCK();
	TAILQ_FOREACH(tsp, &kauth_scopes, ks_link) {
		                
		if (strncmp(tsp->ks_identifier, identifier,
		    strlen(tsp->ks_identifier) + 1) == 0) {
			KAUTH_SCOPEUNLOCK();
			kheap_free(KM_KAUTH, sp, sizeof(struct kauth_scope));
			return NULL;
		}
	}
	TAILQ_INSERT_TAIL(&kauth_scopes, sp, ks_link);

	  
                                                                            
                   
                                                                       
                                                                        
    
restart:
	TAILQ_FOREACH(klp, &kauth_dangling_listeners, kl_link) {
		if (strncmp(klp->kl_identifier, sp->ks_identifier,
		    strlen(klp->kl_identifier) + 1) == 0) {
			                                                              
                       
      
			if (kauth_add_callback_to_scope(sp, klp) == 0) {
				TAILQ_REMOVE(&kauth_dangling_listeners, klp, kl_link);
			} else {
#if 0
				printf("%s - failed to add listener to scope \"%s\" \n", __FUNCTION__, sp->ks_identifier);
#endif
				break;
			}
			goto restart;
		}
	}

	KAUTH_SCOPEUNLOCK();
	return sp;
}



void
kauth_deregister_scope(kauth_scope_t scope)
{
	int             i;

	KAUTH_SCOPELOCK();

	TAILQ_REMOVE(&kauth_scopes, scope, ks_link);

	                                                 
	for (i = 0; i < KAUTH_SCOPE_MAX_LISTENERS; i++) {
		if (scope->ks_listeners[i].kll_listenerp != NULL) {
			TAILQ_INSERT_TAIL(&kauth_dangling_listeners, scope->ks_listeners[i].kll_listenerp, kl_link);
			scope->ks_listeners[i].kll_listenerp = NULL;
			  
                                                              
                                                                    
      
		}
	}
	KAUTH_SCOPEUNLOCK();
	kheap_free(KM_KAUTH, scope, sizeof(struct kauth_scope));

	return;
}

kauth_listener_t
kauth_listen_scope(const char *identifier, kauth_scope_callback_t callback, void *idata)
{
	kauth_listener_t klp;
	kauth_scope_t   sp;

	if ((klp = kauth_alloc_listener(identifier, callback, idata)) == NULL) {
		return NULL;
	}

	  
                                                                           
    
	KAUTH_SCOPELOCK();
	TAILQ_FOREACH(sp, &kauth_scopes, ks_link) {
		if (strncmp(sp->ks_identifier, identifier,
		    strlen(sp->ks_identifier) + 1) == 0) {
			                                                  
			if (kauth_add_callback_to_scope(sp, klp) == 0) {
				KAUTH_SCOPEUNLOCK();
				return klp;
			}
			                        
			KAUTH_SCOPEUNLOCK();
			kheap_free(KM_KAUTH, klp, sizeof(struct kauth_listener));
			return NULL;
		}
	}

	                                               
	TAILQ_INSERT_TAIL(&kauth_dangling_listeners, klp, kl_link);

	KAUTH_SCOPEUNLOCK();

	return klp;
}

void
kauth_unlisten_scope(kauth_listener_t listener)
{
	kauth_scope_t           sp;
	kauth_listener_t        klp;
	int                                     i, listener_count, do_free;

	KAUTH_SCOPELOCK();

	                                               
	TAILQ_FOREACH(sp, &kauth_scopes, ks_link) {
		do_free = 0;
		if ((sp->ks_flags & KS_F_HAS_LISTENERS) != 0) {
			listener_count = 0;
			for (i = 0; i < KAUTH_SCOPE_MAX_LISTENERS; i++) {
				if (sp->ks_listeners[i].kll_listenerp == listener) {
					sp->ks_listeners[i].kll_listenerp = NULL;
					do_free = 1;
					  
                                                                
                                                                      
        
				} else if (sp->ks_listeners[i].kll_listenerp != NULL) {
					listener_count++;
				}
			}
			if (do_free) {
				if (listener_count == 0) {
					sp->ks_flags &= ~KS_F_HAS_LISTENERS;
				}
				KAUTH_SCOPEUNLOCK();
				kheap_free(KM_KAUTH, listener, sizeof(struct kauth_listener));
				return;
			}
		}
	}

	                                            
	TAILQ_FOREACH(klp, &kauth_dangling_listeners, kl_link) {
		if (klp == listener) {
			TAILQ_REMOVE(&kauth_dangling_listeners, klp, kl_link);
			KAUTH_SCOPEUNLOCK();
			kheap_free(KM_KAUTH, listener, sizeof(struct kauth_listener));
			return;
		}
	}

	KAUTH_SCOPEUNLOCK();
	return;
}

  
                          
  
                       
                                   
  
                                                                  
                                    
   
int
kauth_authorize_action(kauth_scope_t scope, kauth_cred_t credential, kauth_action_t action,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
	int result, ret, i;

	                   
	if (scope->ks_callback != NULL) {
		result = scope->ks_callback(credential, scope->ks_idata, action, arg0, arg1, arg2, arg3);
	} else {
		result = KAUTH_RESULT_DEFER;
	}

	                          
	if ((scope->ks_flags & KS_F_HAS_LISTENERS) != 0) {
		for (i = 0; i < KAUTH_SCOPE_MAX_LISTENERS; i++) {
			                                                                                                
                                                                                               
                                                                                                      
                                                                                              
                    
      
			if (scope->ks_listeners[i].kll_listenerp == NULL ||
			    scope->ks_listeners[i].kll_callback == NULL) {
				continue;
			}

			ret = scope->ks_listeners[i].kll_callback(
				credential, scope->ks_listeners[i].kll_idata,
				action, arg0, arg1, arg2, arg3);
			if ((ret == KAUTH_RESULT_DENY) ||
			    (result == KAUTH_RESULT_DEFER)) {
				result = ret;
			}
		}
	}

	                                                  
	                                                                      
	return result == KAUTH_RESULT_ALLOW ? 0 : EPERM;
}

  
                                  
   
int
kauth_authorize_allow(__unused kauth_cred_t credential, __unused void *idata, __unused kauth_action_t action,
    __unused uintptr_t arg0, __unused uintptr_t arg1, __unused uintptr_t arg2, __unused uintptr_t arg3)
{
	return KAUTH_RESULT_ALLOW;
}

#if 0
  
                     
   
static int
kauth_scope_valid(kauth_scope_t scope)
{
	kauth_scope_t   sp;

	KAUTH_SCOPELOCK();
	TAILQ_FOREACH(sp, &kauth_scopes, ks_link) {
		if (sp == scope) {
			break;
		}
	}
	KAUTH_SCOPEUNLOCK();
	return (sp == NULL) ? 0 : 1;
}
#endif

  
                               
   

int
kauth_authorize_process(kauth_cred_t credential, kauth_action_t action, struct proc *process, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
	return kauth_authorize_action(kauth_scope_process, credential, action, (uintptr_t)process, arg1, arg2, arg3);
}

static int
kauth_authorize_process_callback(kauth_cred_t credential, __unused void *idata, kauth_action_t action,
    uintptr_t arg0, uintptr_t arg1, __unused uintptr_t arg2, __unused uintptr_t arg3)
{
	switch (action) {
	case KAUTH_PROCESS_CANSIGNAL:
		panic("KAUTH_PROCESS_CANSIGNAL not implemented");
		                               
		                           
                                      
     
		if (cansignal(current_proc(), credential, (struct proc *)arg0, (int)arg1)) {
			return KAUTH_RESULT_ALLOW;
		}
		break;
	case KAUTH_PROCESS_CANTRACE:
		                                                    
                                
                                                      
     
		if (cantrace(current_proc(), credential, (proc_t)arg0, (int *)arg1)) {
			return KAUTH_RESULT_ALLOW;
		}
		break;
	}

	                                                         
	return KAUTH_RESULT_DEFER;
}

  
                                                                                 
                                                                                
                
                                                  
                                                           
                                                       
                                                   
                                                             
                                                          
                        
                                                         
                                                                
                                               
                                             
                                                    
                                            
                                          
                                                      
                                            
                                            
                                                  
                                                      
                                                   
   

int
kauth_authorize_fileop_has_listeners(void)
{
	  
                                                          
                      
    
	if ((kauth_scope_fileop->ks_flags & KS_F_HAS_LISTENERS) != 0) {
		return 1;
	}
	return 0;
}

int
kauth_authorize_fileop(kauth_cred_t credential, kauth_action_t action, uintptr_t arg0, uintptr_t arg1)
{
	char            *namep = NULL;
	int                     name_len;
	uintptr_t       arg2 = 0;

	                                                                       
                           
    
	if ((kauth_scope_fileop->ks_flags & KS_F_HAS_LISTENERS) == 0) {
		return 0;
	}

	if (action == KAUTH_FILEOP_OPEN ||
	    action == KAUTH_FILEOP_CLOSE ||
	    action == KAUTH_FILEOP_EXEC ||
	    action == KAUTH_FILEOP_WILL_RENAME) {
		                                                                 
     
		namep = get_pathbuff();
		name_len = MAXPATHLEN;
		if (vn_getpath((vnode_t)arg0, namep, &name_len) != 0) {
			release_pathbuff(namep);
			return 0;
		}
		if (action == KAUTH_FILEOP_CLOSE ||
		    action == KAUTH_FILEOP_WILL_RENAME) {
			  
                                                   
                                               
                                              
                                       
      
			arg2 = arg1;
		}
		arg1 = (uintptr_t)namep;
	}
	kauth_authorize_action(kauth_scope_fileop, credential, action, arg0, arg1, arg2, 0);

	if (namep != NULL) {
		release_pathbuff(namep);
	}

	return 0;
}

  
                               
   

int
kauth_authorize_generic(kauth_cred_t credential, kauth_action_t action)
{
	if (credential == NULL) {
		panic("auth against NULL credential");
	}

	return kauth_authorize_action(kauth_scope_generic, credential, action, 0, 0, 0, 0);
}

static int
kauth_authorize_generic_callback(kauth_cred_t credential, __unused void *idata, kauth_action_t action,
    __unused uintptr_t arg0, __unused uintptr_t arg1, __unused uintptr_t arg2, __unused uintptr_t arg3)
{
	switch (action) {
	case KAUTH_GENERIC_ISSUSER:
		                
		return (kauth_cred_getuid(credential) == 0) ?
		       KAUTH_RESULT_ALLOW : KAUTH_RESULT_DENY;
	}

	                                                         
	return KAUTH_RESULT_DEFER;
}

  
                 
  
                                                                                                   
       
  
                                                                                                   
                                                                                      
   
int
kauth_acl_evaluate(kauth_cred_t cred, kauth_acl_eval_t eval)
{
	int applies, error, i, gotguid;
	kauth_ace_t ace;
	guid_t guid;
	uint32_t rights;
	int wkguid;

	                                  
	if (eval->ae_requested == 0) {
		eval->ae_result = KAUTH_RESULT_ALLOW;
		return 0;
	}

	eval->ae_residual = eval->ae_requested;
	eval->ae_found_deny = FALSE;

	  
                                         
    
	if ((error = kauth_cred_getguid(cred, &guid)) != 0) {
		KAUTH_DEBUG("    ACL - can't get credential GUID (%d)", error);
		error = 0;
		gotguid = 0;
	} else {
		gotguid = 1;
	}

	KAUTH_DEBUG("    ACL - %d entries, initial residual %x", eval->ae_count, eval->ae_residual);
	for (i = 0, ace = eval->ae_acl; i < eval->ae_count; i++, ace++) {
		  
                               
     
		if (ace->ace_flags & KAUTH_ACE_ONLY_INHERIT) {
			continue;
		}

		  
                                           
     
		rights = ace->ace_rights;
		if (rights & KAUTH_ACE_GENERIC_ALL) {
			rights |= eval->ae_exp_gall;
		}
		if (rights & KAUTH_ACE_GENERIC_READ) {
			rights |= eval->ae_exp_gread;
		}
		if (rights & KAUTH_ACE_GENERIC_WRITE) {
			rights |= eval->ae_exp_gwrite;
		}
		if (rights & KAUTH_ACE_GENERIC_EXECUTE) {
			rights |= eval->ae_exp_gexec;
		}

		  
                                                                       
                                                                        
                           
     
		switch (ace->ace_flags & KAUTH_ACE_KINDMASK) {
		case KAUTH_ACE_PERMIT:
			if (!(eval->ae_residual & rights)) {
				continue;
			}
			break;
		case KAUTH_ACE_DENY:
			if (!(eval->ae_requested & rights)) {
				continue;
			}
			eval->ae_found_deny = TRUE;
			break;
		default:
			                                          
			continue;
		}

		  
                                                         
     
		wkguid = kauth_wellknown_guid(&ace->ace_applicable);
		switch (wkguid) {
		case KAUTH_WKG_OWNER:
			applies = eval->ae_options & KAUTH_AEVAL_IS_OWNER;
			break;
		case KAUTH_WKG_GROUP:
			if (!gotguid || (eval->ae_options & KAUTH_AEVAL_IN_GROUP_UNKNOWN)) {
				applies = ((ace->ace_flags & KAUTH_ACE_KINDMASK) == KAUTH_ACE_DENY);
			} else {
				applies = eval->ae_options & KAUTH_AEVAL_IN_GROUP;
			}
			break;
		                                                                                            
		case KAUTH_WKG_EVERYBODY:
			applies = 1;
			break;
		case KAUTH_WKG_NOBODY:
			applies = 0;
			break;

		default:
			                                                                         
			applies = !gotguid ? 0 : kauth_guid_equal(&guid, &ace->ace_applicable);
			KAUTH_DEBUG("    ACL - ACE applicable " K_UUID_FMT " caller " K_UUID_FMT " %smatched",
			    K_UUID_ARG(ace->ace_applicable), K_UUID_ARG(guid), applies ? "" : "not ");

			if (!applies) {
				error = !gotguid ? ENOENT : kauth_cred_ismember_guid(cred, &ace->ace_applicable, &applies);
				  
                                                                           
                                                                           
                                                                            
                                           
       
				if (error != 0) {
					KAUTH_DEBUG("    ACL[%d] - can't get membership, making pessimistic assumption", i);
					switch (ace->ace_flags & KAUTH_ACE_KINDMASK) {
					case KAUTH_ACE_PERMIT:
						applies = 0;
						break;
					case KAUTH_ACE_DENY:
						applies = 1;
						break;
					}
				} else {
					KAUTH_DEBUG("    ACL - %s group member", applies ? "is" : "not");
				}
			} else {
				KAUTH_DEBUG("    ACL - entry matches caller");
			}
		}
		if (!applies) {
			continue;
		}

		  
                                     
     
		switch (ace->ace_flags & KAUTH_ACE_KINDMASK) {
		case KAUTH_ACE_PERMIT:
			                                             
			eval->ae_residual = eval->ae_residual & ~rights;
			KAUTH_DEBUG("    ACL[%d] - rights %x leave residual %x", i, rights, eval->ae_residual);
			                           
			if (eval->ae_residual == 0) {
				eval->ae_result = KAUTH_RESULT_ALLOW;
				return 0;
			}
			break;
		case KAUTH_ACE_DENY:
			                                                               
			if (eval->ae_requested & rights) {
				KAUTH_DEBUG("    ACL[%d] - denying based on %x", i, rights);
				eval->ae_result = KAUTH_RESULT_DENY;
				return 0;
			}
			break;
		default:
			KAUTH_DEBUG("    ACL - unknown entry kind %d", ace->ace_flags & KAUTH_ACE_KINDMASK);
			break;
		}
	}
	                                                             
	eval->ae_result = KAUTH_RESULT_DEFER;
	return 0;
}

  
                                                  
  
                                                                
                                                            
                                                                 
                 
                                                                                     
   
int
kauth_acl_inherit(vnode_t dvp, kauth_acl_t initial, kauth_acl_t *product, int isdir, vfs_context_t ctx)
{
	int     entries, error, index;
	unsigned int i;
	struct vnode_attr dva;
	kauth_acl_t inherit, result;

	  
                                                              
                                                                   
                                                               
                                                              
                                                                
   
                                                                    
    
	inherit = NULL;
	  
                                                                 
                                                                        
                                                                    
                                                                  
                                                                       
                                
    
	if ((initial == NULL || !(initial->acl_flags & KAUTH_ACL_NO_INHERIT)) &&
	    (dvp != NULL) && !vfs_authopaque(vnode_mount(dvp))) {
		VATTR_INIT(&dva);
		VATTR_WANTED(&dva, va_acl);
		if ((error = vnode_getattr(dvp, &dva, ctx)) != 0) {
			KAUTH_DEBUG("    ERROR - could not get parent directory ACL for inheritance");
			return error;
		}
		if (VATTR_IS_SUPPORTED(&dva, va_acl)) {
			inherit = dva.va_acl;
		}
	}

	  
                                                                   
                
    
	entries = 0;
	if (inherit != NULL) {
		for (i = 0; i < inherit->acl_entrycount; i++) {
			if (inherit->acl_ace[i].ace_flags & (isdir ? KAUTH_ACE_DIRECTORY_INHERIT : KAUTH_ACE_FILE_INHERIT)) {
				entries++;
			}
		}
	}

	if (initial == NULL) {
		  
                                                            
                                                          
                                      
     
	}

	if (initial != NULL) {
		if (initial->acl_entrycount != KAUTH_FILESEC_NOACL) {
			entries += initial->acl_entrycount;
		} else {
			initial = NULL;
		}
	}

	  
                                                               
                                                
                                                              
                                                                   
    
	if ((entries == 0) && (initial == NULL)) {
		*product = NULL;
		error = 0;
		goto out;
	}

	  
                               
    
	if ((result = kauth_acl_alloc(entries)) == NULL) {
		KAUTH_DEBUG("    ERROR - could not allocate %d-entry result buffer for inherited ACL", entries);
		error = ENOMEM;
		goto out;
	}

	  
                          
                          
                                     
    
	index = 0;
	if (initial != NULL) {
		for (i = 0; i < initial->acl_entrycount; i++) {
			if (!(initial->acl_ace[i].ace_flags & KAUTH_ACE_INHERITED)) {
				result->acl_ace[index++] = initial->acl_ace[i];
			}
		}
		KAUTH_DEBUG("    INHERIT - applied %d of %d initial entries", index, initial->acl_entrycount);
	}
	if (inherit != NULL) {
		for (i = 0; i < inherit->acl_entrycount; i++) {
			  
                                                   
                                                     
                                                   
                                                  
                                            
      
			if (inherit->acl_ace[i].ace_flags & (isdir ? KAUTH_ACE_DIRECTORY_INHERIT : KAUTH_ACE_FILE_INHERIT)) {
				result->acl_ace[index] = inherit->acl_ace[i];
				result->acl_ace[index].ace_flags |= KAUTH_ACE_INHERITED;
				result->acl_ace[index].ace_flags &= ~KAUTH_ACE_ONLY_INHERIT;
				  
                                             
                                          
                                             
                                              
                                              
       
				if ((result->acl_ace[index].ace_flags & KAUTH_ACE_LIMIT_INHERIT) || !isdir) {
					result->acl_ace[index].ace_flags &=
					    ~(KAUTH_ACE_INHERIT_CONTROL_FLAGS);
				}
				index++;
			}
		}
	}
	result->acl_entrycount = index;
	*product = result;
	KAUTH_DEBUG("    INHERIT - product ACL has %d entries", index);
	error = 0;
out:
	if (inherit != NULL) {
		kauth_acl_free(inherit);
	}
	return error;
}

  
                                                   
  
                                                    
                                               
                                        
                                          
                          
  
                       
                                              
                                                
                                       
                                                
                                         
                                          
  
                                                               
  
                                                            
  
                                                      
                                                          
                        
  
                                                          
                                         
   
int
kauth_copyinfilesec(user_addr_t xsecurity, kauth_filesec_t *xsecdestpp)
{
	int error;
	kauth_filesec_t fsec;
	size_t count;
	size_t copysize;

	error = 0;
	fsec = NULL;

	  
                                                                    
                                                                   
                                                                  
                                                                    
   
                                                                 
                                                                  
   
                                                                   
                                                                    
                                                                 
                                                                   
                                    
    
	{
		user_addr_t known_bound = (xsecurity & PAGE_MASK) + KAUTH_FILESEC_SIZE(0);
		user_addr_t uaddr = (user_addr_t)mach_vm_round_page(known_bound);
		count = (uaddr - known_bound) / sizeof(struct kauth_ace);
	}
	if (count > 32) {
		count = 32;
	}
restart:
	if ((fsec = kauth_filesec_alloc((int)count)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	copysize = KAUTH_FILESEC_SIZE(count);
	if ((error = copyin(xsecurity, (caddr_t)fsec, copysize)) != 0) {
		goto out;
	}

	                                 
	if (fsec->fsec_magic != KAUTH_FILESEC_MAGIC) {
		error = EINVAL;
		goto out;
	}

	  
                                               
    
	if ((fsec->fsec_entrycount != KAUTH_FILESEC_NOACL) &&
	    (fsec->fsec_entrycount > count)) {
		if (fsec->fsec_entrycount > KAUTH_ACL_MAX_ENTRIES) {
			                              
			error = EINVAL;
			goto out;
		}
		count = fsec->fsec_entrycount;
		kauth_filesec_free(fsec);
		goto restart;
	}

out:
	if (error) {
		if (fsec) {
			kauth_filesec_free(fsec);
		}
	} else {
		*xsecdestpp = fsec;
		AUDIT_ARG(opaque, fsec, copysize);
	}
	return error;
}

  
                                                                         
                                            
  
                                                            
  
                                                    
                                                 
  
                                                                     
                                                                
                                            
  
                                                                
                                                              
                                                                 
                                                                   
                      
   
kauth_filesec_t
kauth_filesec_alloc(int count)
{
	kauth_filesec_t fsp;

	                                                                       
	if ((count < 0) || (count > KAUTH_ACL_MAX_ENTRIES)) {
		return NULL;
	}

	fsp = kheap_alloc(KM_KAUTH, KAUTH_FILESEC_SIZE(count), Z_WAITOK);
	if (fsp != NULL) {
		fsp->fsec_magic = KAUTH_FILESEC_MAGIC;
		fsp->fsec_owner = kauth_null_guid;
		fsp->fsec_group = kauth_null_guid;
		fsp->fsec_entrycount = KAUTH_FILESEC_NOACL;
		fsp->fsec_flags = 0;
	}
	return fsp;
}

  
                                                                         
                                                                        
  
                                            
  
                  
  
                                                                  
                                                             
                                                
   
void
kauth_filesec_free(kauth_filesec_t fsp)
{
#ifdef KAUTH_DEBUG_ENABLE
	if (fsp == KAUTH_FILESEC_NONE) {
		panic("freeing KAUTH_FILESEC_NONE");
	}
	if (fsp == KAUTH_FILESEC_WANTED) {
		panic("freeing KAUTH_FILESEC_WANTED");
	}
#endif
	kheap_free_addr(KM_KAUTH, fsp);
}

  
                                                                        
                                                                          
                                     
  
                                                              
                                              
                                  
                                       
  
                  
  
                                                                      
                                                               
                                                            
   
void
kauth_filesec_acl_setendian(int kendian, kauth_filesec_t fsec, kauth_acl_t acl)
{
	uint32_t        compare_magic = KAUTH_FILESEC_MAGIC;
	uint32_t        invert_magic = ntohl(KAUTH_FILESEC_MAGIC);
	uint32_t        compare_acl_entrycount;
	uint32_t        i;

	if (compare_magic == invert_magic) {
		return;
	}

	                                                   
	if (acl == NULL) {
		acl = &fsec->fsec_acl;
	}

	compare_acl_entrycount = acl->acl_entrycount;

	  
                                                                      
                                                                 
                                                                    
    
	switch (kendian) {
	case KAUTH_ENDIAN_HOST:                                           
		if (fsec->fsec_magic != invert_magic) {
			return;
		}
		                                   
		compare_acl_entrycount = ntohl(acl->acl_entrycount);
		break;
	case KAUTH_ENDIAN_DISK:                                           
		if (fsec->fsec_magic != compare_magic) {
			return;
		}
		break;
	default:                                          
		return;
	}

	                              
	fsec->fsec_magic = ntohl(fsec->fsec_magic);
	acl->acl_entrycount = ntohl(acl->acl_entrycount);
	if (compare_acl_entrycount != KAUTH_FILESEC_NOACL) {
		acl->acl_flags = ntohl(acl->acl_flags);

		                               
		for (i = 0; i < compare_acl_entrycount; i++) {
			acl->acl_ace[i].ace_flags = ntohl(acl->acl_ace[i].ace_flags);
			acl->acl_ace[i].ace_rights = ntohl(acl->acl_ace[i].ace_rights);
		}
	}
}


  
                          
   
kauth_acl_t
kauth_acl_alloc(int count)
{
	kauth_acl_t     aclp;

	                                                                       
	if ((count < 0) || (count > KAUTH_ACL_MAX_ENTRIES)) {
		return NULL;
	}

	aclp = kheap_alloc(KM_KAUTH, KAUTH_ACL_SIZE(count), Z_WAITOK);
	if (aclp != NULL) {
		aclp->acl_entrycount = 0;
		aclp->acl_flags = 0;
	}
	return aclp;
}

void
kauth_acl_free(kauth_acl_t aclp)
{
	  
                                                                   
                                                          
    
	kheap_free_addr(KHEAP_ANY, aclp);
}


  
                                             
   
static int
kauth_add_callback_to_scope(kauth_scope_t sp, kauth_listener_t klp)
{
	int             i;

	for (i = 0; i < KAUTH_SCOPE_MAX_LISTENERS; i++) {
		if (sp->ks_listeners[i].kll_listenerp == NULL) {
			sp->ks_listeners[i].kll_callback = klp->kl_callback;
			sp->ks_listeners[i].kll_idata = klp->kl_idata;
			sp->ks_listeners[i].kll_listenerp = klp;
			sp->ks_flags |= KS_F_HAS_LISTENERS;
			return 0;
		}
	}
	return ENOSPC;
}
