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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Quite extensively rewritten by Poul-Henning Kamp of the FreeBSD
 * project, to make these variables more userfriendly.
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
 *	@(#)kern_sysctl.c	8.4 (Berkeley) 4/14/94
 */


#include <kern/counter.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/sysproto.h>

#include <os/atomic_private.h>

#include <security/audit/audit.h>
#include <pexpert/pexpert.h>

#include <IOKit/IOBSD.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#if defined(HAS_APPLE_PAC)
#include <os/hash.h>
#include <ptrauth.h>
#endif                             

static LCK_GRP_DECLARE(sysctl_lock_group, "sysctl");
static LCK_RW_DECLARE(sysctl_geometry_lock, &sysctl_lock_group);
static LCK_MTX_DECLARE(sysctl_unlocked_node_lock, &sysctl_lock_group);

  
                                                                            
   
#ifdef STATIC
#undef STATIC
#endif
#if 0
#define STATIC
#else
#define STATIC static
#endif

                                               
STATIC void sysctl_sysctl_debug_dump_node(struct sysctl_oid_list *l, int i);
STATIC int sysctl_sysctl_debug(struct sysctl_oid *oidp, void *arg1,
    int arg2, struct sysctl_req *req);
STATIC int sysctl_sysctl_name(struct sysctl_oid *oidp, void *arg1,
    int arg2, struct sysctl_req *req);
STATIC int sysctl_sysctl_next_ls(struct sysctl_oid_list *lsp,
    int *name, u_int namelen, int *next, int *len, int level,
    struct sysctl_oid **oidpp);
STATIC int sysctl_old_kernel(struct sysctl_req *req, const void *p, size_t l);
STATIC int sysctl_new_kernel(struct sysctl_req *req, void *p, size_t l);
STATIC int name2oid(char *name, int *oid, size_t *len);
STATIC int sysctl_sysctl_name2oid(struct sysctl_oid *oidp, void *arg1, int arg2, struct sysctl_req *req);
STATIC int sysctl_sysctl_next(struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req);
STATIC int sysctl_sysctl_oidfmt(struct sysctl_oid *oidp, void *arg1, int arg2, struct sysctl_req *req);
STATIC int sysctl_old_user(struct sysctl_req *req, const void *p, size_t l);
STATIC int sysctl_new_user(struct sysctl_req *req, void *p, size_t l);

STATIC void sysctl_create_user_req(struct sysctl_req *req, struct proc *p, user_addr_t oldp,
    size_t oldlen, user_addr_t newp, size_t newlen);
STATIC int sysctl_root(boolean_t from_kernel, boolean_t string_is_canonical, char *namestring, size_t namestringlen, int *name, size_t namelen, struct sysctl_req *req);

int     kernel_sysctl(struct proc *p, int *name, size_t namelen, void *old, size_t *oldlenp, void *new, size_t newlen);
int     kernel_sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
int     userland_sysctl(boolean_t string_is_canonical,
    char *namestring, size_t namestringlen,
    int *name, u_int namelen, struct sysctl_req *req,
    size_t *retval);

SECURITY_READ_ONLY_LATE(struct sysctl_oid_list) sysctl__children;                
__SYSCTL_EXTENSION_NODE();

  
                                  
  
                                
   

static void
sysctl_register_oid_locked(struct sysctl_oid *new_oidp,
    struct sysctl_oid *oidp)
{
	struct sysctl_oid_list *parent = new_oidp->oid_parent;
	struct sysctl_oid_list *parent_rw = NULL;
	struct sysctl_oid *p, **prevp;

	p = SLIST_FIRST(parent);
	if (p && p->oid_number == OID_MUTABLE_ANCHOR) {
		parent_rw = p->oid_arg1;
	}

	if (oidp->oid_number == OID_AUTO) {
		int n = OID_AUTO_START;

		  
                                                              
                                                               
                                                                
     

		SLIST_FOREACH_PREVPTR(p, prevp, parent, oid_link) {
			if (p->oid_number >= n) {
				n = p->oid_number + 1;
			}
		}

		if (parent_rw) {
			SLIST_FOREACH_PREVPTR(p, prevp, parent_rw, oid_link) {
				if (p->oid_number >= n) {
					n = p->oid_number + 1;
				}
			}
		}

		  
                                                             
                                                        
     
		oidp->oid_number = new_oidp->oid_number = n;
	} else {
		  
                                                    
     
		SLIST_FOREACH_PREVPTR(p, prevp, parent, oid_link) {
			if (oidp->oid_number == p->oid_number) {
				panic("attempting to register a sysctl at previously registered slot : %d",
				    oidp->oid_number);
			} else if (oidp->oid_number < p->oid_number) {
				break;
			}
		}

		if (parent_rw) {
			SLIST_FOREACH_PREVPTR(p, prevp, parent_rw, oid_link) {
				if (oidp->oid_number == p->oid_number) {
					panic("attempting to register a sysctl at previously registered slot : %d",
					    oidp->oid_number);
				} else if (oidp->oid_number < p->oid_number) {
					break;
				}
			}
		}
	}

#if defined(HAS_APPLE_PAC)
	if (oidp->oid_handler) {
		  
                                                                        
                                                                      
                                                                 
     
		oidp->oid_handler = ptrauth_auth_and_resign(oidp->oid_handler,
		    ptrauth_key_function_pointer,
		    ptrauth_function_pointer_type_discriminator(typeof(oidp->oid_handler)),
		    ptrauth_key_function_pointer,
		    ptrauth_blend_discriminator(&oidp->oid_handler,
		    os_hash_kernel_pointer(oidp->oid_arg1)));
	}
#endif                             

	SLIST_NEXT(oidp, oid_link) = *prevp;
	*prevp = oidp;
}

void
sysctl_register_oid(struct sysctl_oid *new_oidp)
{
	struct sysctl_oid *oidp;

	if (new_oidp->oid_number < OID_AUTO) {
		panic("trying to register a node %p with an invalid oid_number: %d",
		    new_oidp, new_oidp->oid_number);
	}
	if (new_oidp->oid_kind & CTLFLAG_PERMANENT) {
		panic("Use sysctl_register_oid_early to register permanent nodes");
	}

	  
                                                                       
                                                                       
                                                                   
                                                  
    
	if (!(new_oidp->oid_kind & CTLFLAG_OID2)) {
#if __x86_64__
		  
                                                                      
                                                               
     
		oidp = kheap_alloc(KHEAP_DEFAULT, sizeof(struct sysctl_oid),
		    Z_WAITOK | Z_ZERO);
		if (oidp == NULL) {
			return;                                
		}
		  
                                                             
                                                   
    
                                                  
                                           
     
		memcpy(oidp, new_oidp, offsetof(struct sysctl_oid, oid_descr));
#else
		panic("Old style sysctl without a version number isn't supported");
#endif
	} else {
		                                                             
		switch (new_oidp->oid_version) {
		case SYSCTL_OID_VERSION:
			                     
			oidp = new_oidp;
			break;
		default:
			return;                                              
		}
	}

	lck_rw_lock_exclusive(&sysctl_geometry_lock);
	sysctl_register_oid_locked(new_oidp, oidp);
	lck_rw_unlock_exclusive(&sysctl_geometry_lock);
}

__startup_func
void
sysctl_register_oid_early(struct sysctl_oid *oidp)
{
	assert((oidp->oid_kind & CTLFLAG_OID2) &&
	    (oidp->oid_kind & CTLFLAG_PERMANENT) &&
	    oidp->oid_version == SYSCTL_OID_VERSION);
	assert(startup_phase < STARTUP_SUB_SYSCTL);

	  
                                                                    
                                              
    
	if (oidp->oid_kind & CTLFLAG_NOAUTO) {
		oidp->oid_kind &= ~CTLFLAG_NOAUTO;
		return;
	}

	sysctl_register_oid_locked(oidp, oidp);
}

void
sysctl_unregister_oid(struct sysctl_oid *oidp)
{
	struct sysctl_oid *removed_oidp = NULL;                            
#if __x86_64__
	struct sysctl_oid *old_oidp = NULL;                                 
#endif
	struct sysctl_oid_list *lsp;

	                                               
	lck_rw_lock_exclusive(&sysctl_geometry_lock);

	lsp = oidp->oid_parent;
	if (SLIST_FIRST(lsp) && SLIST_FIRST(lsp)->oid_number == OID_MUTABLE_ANCHOR) {
		lsp = SLIST_FIRST(lsp)->oid_arg1;
	}

	if (oidp->oid_kind & CTLFLAG_PERMANENT) {
		panic("Trying to unregister permanent sysctl %p", oidp);
	}

	if (!(oidp->oid_kind & CTLFLAG_OID2)) {
#if __x86_64__
		  
                                                          
                                                        
                                                          
                                  
     
		SLIST_FOREACH(old_oidp, lsp, oid_link) {
			if (!memcmp(&oidp->oid_number, &old_oidp->oid_number, (offsetof(struct sysctl_oid, oid_descr) - offsetof(struct sysctl_oid, oid_number)))) {
				break;
			}
		}
		if (old_oidp != NULL) {
			SLIST_REMOVE(lsp, old_oidp, sysctl_oid, oid_link);
			removed_oidp = old_oidp;
		}
#else
		panic("Old style sysctl without a version number isn't supported");
#endif
	} else {
		                                                             
		switch (oidp->oid_version) {
		case SYSCTL_OID_VERSION:
			                                            
			SLIST_REMOVE(lsp, oidp, sysctl_oid, oid_link);
			removed_oidp = oidp;
			break;
		default:
			                                                
			break;                                               
		}
	}

#if defined(HAS_APPLE_PAC)
	if (removed_oidp && removed_oidp->oid_handler) {
		  
                                                      
                                                                  
     
		removed_oidp->oid_handler = ptrauth_auth_and_resign(removed_oidp->oid_handler,
		    ptrauth_key_function_pointer,
		    ptrauth_blend_discriminator(&removed_oidp->oid_handler,
		    os_hash_kernel_pointer(removed_oidp->oid_arg1)),
		    ptrauth_key_function_pointer,
		    ptrauth_function_pointer_type_discriminator(typeof(removed_oidp->oid_handler)));
	}
#endif                             

	  
                                                                   
                                                                     
                                                                   
                                                                     
   
                                                
    
	while (removed_oidp && removed_oidp->oid_refcnt) {
		lck_rw_sleep(&sysctl_geometry_lock, LCK_SLEEP_EXCLUSIVE,
		    &removed_oidp->oid_refcnt, THREAD_UNINT);
	}

	                            
	lck_rw_unlock_exclusive(&sysctl_geometry_lock);

#if __x86_64__
	                                                          
	kheap_free(KHEAP_DEFAULT, old_oidp, sizeof(struct sysctl_oid));
#endif
}

  
                                                               
   
#if defined(__x86_64__)
void
sysctl_register_fixed(void)
{
}
#endif

  
                        
                                                                         
                                                                       
                                                                
                                
   

int
sysctl_io_number(struct sysctl_req *req, long long bigValue, size_t valueSize, void *pValue, int *changed)
{
	int             smallValue;
	int             error;

	if (changed) {
		*changed = 0;
	}

	  
                                                             
                                                           
                                                                
          
    

	                                                    
	if (((valueSize == sizeof(int)) ||
	    ((req->oldlen == sizeof(int)) && (valueSize == sizeof(long long))))
	    && (req->oldptr)) {
		smallValue = (int)bigValue;
		if ((long long)smallValue != bigValue) {
			return ERANGE;
		}
		error = SYSCTL_OUT(req, &smallValue, sizeof(smallValue));
	} else {
		                                                  
		error = SYSCTL_OUT(req, &bigValue, valueSize);
	}
	                             
	if (error || !req->newptr) {
		return error;
	}

	                              
	if (pValue == NULL) {
		return EPERM;
	}

	                                   
	if ((req->newlen == sizeof(int)) && (valueSize == sizeof(long long))) {
		                                                
		error = SYSCTL_IN(req, &smallValue, sizeof(smallValue));
		if (!error) {
			*(long long *)pValue = (long long)smallValue;
		}
	} else if ((req->newlen == sizeof(long long)) && (valueSize == sizeof(int))) {
		                                                                  
		error = SYSCTL_IN(req, &bigValue, sizeof(bigValue));
		if (!error) {
			smallValue = (int)bigValue;
			if ((long long)smallValue != bigValue) {
				return ERANGE;
			}
			*(int *)pValue = smallValue;
		}
	} else {
		                               
		error = SYSCTL_IN(req, pValue, valueSize);
	}
	if (!error && changed) {
		*changed = 1;
	}
	return error;
}

int
sysctl_io_string(struct sysctl_req *req, char *pValue, size_t valueSize, int trunc, int *changed)
{
	int error;
	size_t len = strlen(pValue) + 1;

	if (changed) {
		*changed = 0;
	}

	if (trunc && req->oldptr && req->oldlen && (req->oldlen < len)) {
		                                                             
                                                                   
                                                                      
                                                                    
                                                                    
                                                              
		error = SYSCTL_OUT(req, pValue, req->oldlen - 1);
		if (!error) {
			char c = '\0';
			error = SYSCTL_OUT(req, &c, 1);
		}
	} else {
		                     
		error = SYSCTL_OUT(req, pValue, len);
	}

	                           
	if (error || !req->newptr) {
		return error;
	}

	                                    
	if (valueSize == 0) {
		return EPERM;
	}

	                                               
	if (req->newlen >= valueSize) {
		return EINVAL;
	}

	                                                  
	error = SYSCTL_IN(req, pValue, req->newlen);
	pValue[req->newlen] = '\0';

	if (!error && changed) {
		*changed = 1;
	}
	return error;
}

int
sysctl_io_opaque(struct sysctl_req *req, void *pValue, size_t valueSize, int *changed)
{
	int error;

	if (changed) {
		*changed = 0;
	}

	                   
	error = SYSCTL_OUT(req, pValue, valueSize);

	                             
	if (error || !req->newptr) {
		return error;
	}

	error = SYSCTL_IN(req, pValue, valueSize);

	if (!error && changed) {
		*changed = 1;
	}
	return error;
}

  
                         
  
                                                                               
                                                                                
                                                                           
  
                                                                             
                      
                                                               
                                                           
                                 
   
struct sysctl_oid_iterator {
	struct sysctl_oid *a;
	struct sysctl_oid *b;
};

static struct sysctl_oid_iterator
sysctl_oid_iterator_begin(struct sysctl_oid_list *l)
{
	struct sysctl_oid_iterator it = { };
	struct sysctl_oid *a = SLIST_FIRST(l);

	if (a == NULL) {
		return it;
	}

	if (a->oid_number == OID_MUTABLE_ANCHOR) {
		it.a = SLIST_NEXT(a, oid_link);
		it.b = SLIST_FIRST((struct sysctl_oid_list *)a->oid_arg1);
	} else {
		it.a = a;
	}
	return it;
}

static struct sysctl_oid *
sysctl_oid_iterator_next_num_order(struct sysctl_oid_iterator *it)
{
	struct sysctl_oid *a = it->a;
	struct sysctl_oid *b = it->b;

	if (a == NULL && b == NULL) {
		return NULL;
	}

	if (a == NULL) {
		it->b = SLIST_NEXT(b, oid_link);
		return b;
	}

	if (b == NULL || a->oid_number <= b->oid_number) {
		it->a = SLIST_NEXT(a, oid_link);
		return a;
	}

	it->b = SLIST_NEXT(b, oid_link);
	return b;
}

#define SYSCTL_OID_FOREACH_NUM_ORDER(oidp, l) \
	for (struct sysctl_oid_iterator it = sysctl_oid_iterator_begin(l); \
	        ((oidp) = sysctl_oid_iterator_next_num_order(&it)); )

static struct sysctl_oid *
sysctl_oid_iterator_next_system_order(struct sysctl_oid_iterator *it)
{
	struct sysctl_oid *a = it->a;
	struct sysctl_oid *b = it->b;

	if (a) {
		it->a = SLIST_NEXT(a, oid_link);
		return a;
	}

	if (b) {
		it->b = SLIST_NEXT(b, oid_link);
		return b;
	}

	return NULL;
}

#define SYSCTL_OID_FOREACH_SYS_ORDER(oidp, l) \
	for (struct sysctl_oid_iterator it = sysctl_oid_iterator_begin(l); \
	        ((oidp) = sysctl_oid_iterator_next_system_order(&it)); )

  
                    
  
                                                               
                                                                
                             
                                                                      
                                                                      
                                                                     
                                                        
  
                                    
                                              
                                 
                                            
                                                             
   

  
                                
  
                                                                         
                                                     
  
                                           
                           
  
                  
  
                                 
  
                                                               
  
                                                                   
                                                               
  
                                                              
                         
  
                                                                 
                                                              
                                 
   
STATIC void
sysctl_sysctl_debug_dump_node(struct sysctl_oid_list *l, int i)
{
	struct sysctl_oid *oidp;
	struct sysctl_oid_list *lp;
	const char *what;

	SYSCTL_OID_FOREACH_SYS_ORDER(oidp, l) {
		switch (oidp->oid_kind & CTLTYPE) {
		case CTLTYPE_NODE:
			lp = oidp->oid_arg1;
			what = "Node   ";
			if (lp && SLIST_FIRST(lp) &&
			    SLIST_FIRST(lp)->oid_number == OID_MUTABLE_ANCHOR) {
				what = "NodeExt";
			} else {
			}
			break;
		case CTLTYPE_INT:
			what = "Int    ";
			break;
		case CTLTYPE_STRING:
			what = "String ";
			break;
		case CTLTYPE_QUAD:
			what = "Quad   ";
			break;
		case CTLTYPE_OPAQUE:
			what = "Opaque ";
			break;
		default:
			what = "Unknown";
			break;
		}

		printf("%*s%-3d[%c%c%c%c%c] %s %s\n", i, "", oidp->oid_number,
		    oidp->oid_kind & CTLFLAG_LOCKED ? 'L':' ',
		    oidp->oid_kind & CTLFLAG_RD ? 'R':' ',
		    oidp->oid_kind & CTLFLAG_WR ? 'W':' ',
		    oidp->oid_kind & CTLFLAG_PERMANENT ? ' ':'*',
		    oidp->oid_handler ? 'h' : ' ',
		    what, oidp->oid_name);

		if ((oidp->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
			if (!oidp->oid_handler) {
				sysctl_sysctl_debug_dump_node(lp, i + 2);
			}
		}
	}
}

  
                      
  
                                                                          
                         
  
             
  
                       
  
                  
  
                                 
  
                                                       
                        
   
STATIC int
sysctl_sysctl_debug(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, __unused struct sysctl_req *req)
{
	lck_rw_lock_shared(&sysctl_geometry_lock);
	sysctl_sysctl_debug_dump_node(&sysctl__children, 0);
	lck_rw_done(&sysctl_geometry_lock);
	return ENOENT;
}

SYSCTL_PROC(_sysctl, 0, debug, CTLTYPE_STRING | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_sysctl_debug, "-", "");

  
                     
  
                                                                           
                                                                 
                                                                 
            
  
             
  
                               
                                          
                                   
                              
                                        
                                
  
                        
                                       
                                               
                                                    
                                 
  
                                                     
  
                                                       
                        
  
                                                                       
                                                               
                                                              
                                                                
                                            
  
                                                           
                                                               
                                                           
                                                               
         
  
                                                             
                                                                 
                                                                  
                                                                
                                      
   
STATIC int
sysctl_sysctl_name(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	int *name = (int *) arg1;
	u_int namelen = arg2;
	int error = 0;
	struct sysctl_oid *oid;
	struct sysctl_oid_list *lsp = &sysctl__children, *lsp2;
	char tempbuf[10] = {};

	lck_rw_lock_shared(&sysctl_geometry_lock);
	while (namelen) {
		if (!lsp) {
			snprintf(tempbuf, sizeof(tempbuf), "%d", *name);
			if (req->oldidx) {
				error = SYSCTL_OUT(req, ".", 1);
			}
			if (!error) {
				error = SYSCTL_OUT(req, tempbuf, strlen(tempbuf));
			}
			if (error) {
				lck_rw_done(&sysctl_geometry_lock);
				return error;
			}
			namelen--;
			name++;
			continue;
		}
		lsp2 = 0;
		SYSCTL_OID_FOREACH_NUM_ORDER(oid, lsp) {
			if (oid->oid_number != *name) {
				continue;
			}

			if (req->oldidx) {
				error = SYSCTL_OUT(req, ".", 1);
			}
			if (!error) {
				error = SYSCTL_OUT(req, oid->oid_name,
				    strlen(oid->oid_name));
			}
			if (error) {
				lck_rw_done(&sysctl_geometry_lock);
				return error;
			}

			namelen--;
			name++;

			if ((oid->oid_kind & CTLTYPE) != CTLTYPE_NODE) {
				break;
			}

			if (oid->oid_handler) {
				break;
			}

			lsp2 = (struct sysctl_oid_list *)oid->oid_arg1;
			break;
		}
		lsp = lsp2;
	}
	lck_rw_done(&sysctl_geometry_lock);
	return SYSCTL_OUT(req, "", 1);
}

SYSCTL_NODE(_sysctl, 1, name, CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_sysctl_name, "");

  
                        
  
                                                                           
                                       
  
                                             
                                      
                                         
                                         
               
                                     
                                       
                                            
                  
                                      
                                      
                                
  
                                      
                                     
  
                                                     
                                          
  
                                                               
  
                                                                    
                                                                 
                                                                
                                                  
   
STATIC int
sysctl_sysctl_next_ls(struct sysctl_oid_list *lsp, int *name, u_int namelen,
    int *next, int *len, int level, struct sysctl_oid **oidpp)
{
	struct sysctl_oid *oidp;

	*len = level;
	SYSCTL_OID_FOREACH_NUM_ORDER(oidp, lsp) {
		*next = oidp->oid_number;
		*oidpp = oidp;

		if (!namelen) {
			if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE) {
				return 0;
			}
			if (oidp->oid_handler) {
				                                              
				return 0;
			}
			lsp = (struct sysctl_oid_list *)oidp->oid_arg1;

			if (!SLIST_FIRST(lsp)) {
				                                          
				continue;
			}

			if (!sysctl_sysctl_next_ls(lsp, 0, 0, next + 1,
			    len, level + 1, oidpp)) {
				return 0;
			}
			goto next;
		}

		if (oidp->oid_number < *name) {
			continue;
		}

		if (oidp->oid_number > *name) {
			if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE) {
				return 0;
			}
			if (oidp->oid_handler) {
				return 0;
			}
			lsp = (struct sysctl_oid_list *)oidp->oid_arg1;
			if (!sysctl_sysctl_next_ls(lsp, name + 1, namelen - 1,
			    next + 1, len, level + 1, oidpp)) {
				return 0;
			}
			goto next;
		}
		if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE) {
			continue;
		}

		if (oidp->oid_handler) {
			continue;
		}

		lsp = (struct sysctl_oid_list *)oidp->oid_arg1;
		if (!sysctl_sysctl_next_ls(lsp, name + 1, namelen - 1, next + 1,
		    len, level + 1, oidpp)) {
			return 0;
		}
next:
		namelen = 1;
		*len = level;
	}
	return 1;
}

  
                     
  
                                                                             
                                                                 
                     
  
             
  
                               
                                     
                                 
                                         
  
                        
                                      
                                       
                                               
                                                    
                                 
  
                                                     
  
                                                       
                        
  
                                                                       
                                                               
                                                              
                                                                
                                            
  
                                                             
                                                                 
                                                                  
                                                                
                                      
   
STATIC int
sysctl_sysctl_next(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	int *name = (int *) arg1;
	u_int namelen = arg2;
	int i, j, error;
	struct sysctl_oid *oid;
	struct sysctl_oid_list *lsp = &sysctl__children;
	int newoid[CTL_MAXNAME] = {};

	lck_rw_lock_shared(&sysctl_geometry_lock);
	i = sysctl_sysctl_next_ls(lsp, name, namelen, newoid, &j, 1, &oid);
	lck_rw_done(&sysctl_geometry_lock);
	if (i) {
		return ENOENT;
	}
	error = SYSCTL_OUT(req, newoid, j * sizeof(int));
	return error;
}

SYSCTL_NODE(_sysctl, 2, next, CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_sysctl_next, "");

  
           
  
                                                                           
                                       
  
                                                  
                                      
                                        
                                   
  
                        
                             
  
                                                  
                                          
  
                                                               
   
STATIC int
name2oid(char *name, int *oid, size_t *len)
{
	struct sysctl_oid_iterator it;
	struct sysctl_oid *oidp;
	char *p;
	char i;

	if (!*name) {
		return ENOENT;
	}

	p = name + strlen(name) - 1;
	if (*p == '.') {
		*p = '\0';
	}

	*len = 0;

	for (p = name; *p && *p != '.'; p++) {
		;
	}
	i = *p;
	if (i == '.') {
		*p = '\0';
	}

	it = sysctl_oid_iterator_begin(&sysctl__children);
	oidp = sysctl_oid_iterator_next_system_order(&it);

	while (oidp && *len < CTL_MAXNAME) {
		if (strcmp(name, oidp->oid_name)) {
			oidp = sysctl_oid_iterator_next_system_order(&it);
			continue;
		}
		*oid++ = oidp->oid_number;
		(*len)++;

		if (i == '\0') {
			return 0;
		}

		if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE) {
			break;
		}

		if (oidp->oid_handler) {
			break;
		}

		it = sysctl_oid_iterator_begin(oidp->oid_arg1);
		oidp = sysctl_oid_iterator_next_system_order(&it);

		*p = i;              
		name = p + 1;
		for (p = name; *p && *p != '.'; p++) {
			;
		}
		i = *p;
		if (i == '.') {
			*p = '\0';
		}
	}
	return ENOENT;
}

  
                         
  
                                                                             
                                                                  
                 
  
             
  
                               
                    
                    
                            
  
                                            
                                        
                                          
                                          
                                                  
                                                      
                               
  
                                                  
  
                                                       
                        
  
                                                                       
                                                               
                                                              
                                                                
                                            
  
                                                             
                                                                 
                                                                  
                                                                
                                      
   
STATIC int
sysctl_sysctl_name2oid(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	char *p;
	int error, oid[CTL_MAXNAME] = {};
	size_t len = 0;                                 

	if (req->newlen < 1) {
		return ENOENT;
	}
	if (req->newlen >= MAXPATHLEN) {                                  
		return ENAMETOOLONG;
	}

	p = kheap_alloc(KHEAP_TEMP, req->newlen + 1, Z_WAITOK);
	if (!p) {
		return ENOMEM;
	}

	error = SYSCTL_IN(req, p, req->newlen);
	if (error) {
		kheap_free(KHEAP_TEMP, p, req->newlen + 1);
		return error;
	}

	p[req->newlen] = '\0';

	  
                                                          
                                              
    
	lck_rw_lock_shared(&sysctl_geometry_lock);
	error = name2oid(p, oid, &len);
	lck_rw_done(&sysctl_geometry_lock);

	kheap_free(KHEAP_TEMP, p, req->newlen + 1);

	if (error) {
		return error;
	}

	error = SYSCTL_OUT(req, oid, len * sizeof *oid);
	return error;
}

SYSCTL_PROC(_sysctl, 3, name2oid, CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_KERN | CTLFLAG_LOCKED, 0, 0,
    sysctl_sysctl_name2oid, "I", "");

  
                       
  
                                                                            
                                                                
                 
  
             
  
                               
                                   
                                      
                                         
  
                        
                               
                              
                                       
                                               
                                                    
  
                                                     
  
                                                       
                        
  
                                                                       
                                                               
                                                              
                                                                
  
                                                             
                                                                 
                                                                  
                                                                
                                      
   
STATIC int
sysctl_sysctl_oidfmt(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	int *name = (int *) arg1;
	int error = ENOENT;                                           
	u_int namelen = arg2;
	u_int indx;
	struct sysctl_oid_iterator it;
	struct sysctl_oid *oid;

	lck_rw_lock_shared(&sysctl_geometry_lock);

	it = sysctl_oid_iterator_begin(&sysctl__children);
	oid = sysctl_oid_iterator_next_system_order(&it);

	indx = 0;
	while (oid && indx < CTL_MAXNAME) {
		if (oid->oid_number == name[indx]) {
			indx++;
			if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
				if (oid->oid_handler) {
					goto found;
				}
				if (indx == namelen) {
					goto found;
				}
				it = sysctl_oid_iterator_begin(oid->oid_arg1);
				oid = sysctl_oid_iterator_next_system_order(&it);
			} else {
				if (indx != namelen) {
					error =  EISDIR;
					goto err;
				}
				goto found;
			}
		} else {
			oid = sysctl_oid_iterator_next_system_order(&it);
		}
	}
	               
	goto err;

found:
	if (!oid->oid_fmt) {
		goto err;
	}
	error = SYSCTL_OUT(req,
	    &oid->oid_kind, sizeof(oid->oid_kind));
	if (!error) {
		error = SYSCTL_OUT(req, oid->oid_fmt,
		    strlen(oid->oid_fmt) + 1);
	}
err:
	lck_rw_done(&sysctl_geometry_lock);
	return error;
}

SYSCTL_NODE(_sysctl, 4, oidfmt, CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_sysctl_oidfmt, "");


  
                               
   

  
                                     
             
                                     
                                    
   

int
sysctl_handle_int(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	return sysctl_io_number(req, arg1? *(int*)arg1: arg2, sizeof(int), arg1, NULL);
}

  
                                                         
   

int
sysctl_handle_long(__unused struct sysctl_oid *oidp, void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	if (!arg1) {
		return EINVAL;
	}
	return sysctl_io_number(req, *(long*)arg1, sizeof(long), arg1, NULL);
}

  
                                                         
   

int
sysctl_handle_quad(__unused struct sysctl_oid *oidp, void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	if (!arg1) {
		return EINVAL;
	}
	return sysctl_io_number(req, *(long long*)arg1, sizeof(long long), arg1, NULL);
}

  
                                 
  
                                                         
                                                         
              
   
int
sysctl_handle_int2quad(__unused struct sysctl_oid *oidp, void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	int error = 0;
	long long val;
	int newval;

	if (!arg1) {
		return EINVAL;
	}
	val = (long long)*(int *)arg1;
	error = SYSCTL_OUT(req, &val, sizeof(long long));

	if (error || !req->newptr) {
		return error;
	}

	error = SYSCTL_IN(req, &val, sizeof(long long));
	if (!error) {
		  
                                          
                                   
     
		newval = (int)val;
		if ((long long)newval != val) {
			error = ERANGE;
		} else {
			*(int *)arg1 = newval;
		}
	}
	return error;
}

  
                                                 
             
                                                                 
                                                           
   

int
sysctl_handle_string( __unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	return sysctl_io_string(req, arg1, arg2, 0, NULL);
}

  
                                  
                                       
   

int
sysctl_handle_opaque(__unused struct sysctl_oid *oidp, void *arg1, int arg2,
    struct sysctl_req *req)
{
	return sysctl_io_opaque(req, arg1, arg2, NULL);
}

  
                                           
   
STATIC int
sysctl_old_kernel(struct sysctl_req *req, const void *p, size_t l)
{
	size_t i = 0;

	if (req->oldptr) {
		i = l;
		if (i > req->oldlen - req->oldidx) {
			i = req->oldlen - req->oldidx;
		}
		if (i > 0) {
			bcopy((const void*)p, CAST_DOWN(char *, (req->oldptr + req->oldidx)), i);
		}
	}
	req->oldidx += l;
	if (req->oldptr && i != l) {
		return ENOMEM;
	}
	return 0;
}

STATIC int
sysctl_new_kernel(struct sysctl_req *req, void *p, size_t l)
{
	if (!req->newptr) {
		return 0;
	}
	if (req->newlen - req->newidx < l) {
		return EINVAL;
	}
	bcopy(CAST_DOWN(char *, (req->newptr + req->newidx)), p, l);
	req->newidx += l;
	return 0;
}

int
kernel_sysctl(struct proc *p, int *name, size_t namelen, void *old, size_t *oldlenp, void *new, size_t newlen)
{
	int error = 0;
	struct sysctl_req req;

	  
                      
    
	bzero(&req, sizeof req);
	req.p = p;
	if (oldlenp) {
		req.oldlen = *oldlenp;
	}
	if (old) {
		req.oldptr = CAST_USER_ADDR_T(old);
	}
	if (newlen) {
		req.newlen = newlen;
		req.newptr = CAST_USER_ADDR_T(new);
	}
	req.oldfunc = sysctl_old_kernel;
	req.newfunc = sysctl_new_kernel;
	req.lock = 1;

	                      
	error = sysctl_root(TRUE, FALSE, NULL, 0, name, namelen, &req);

	if (error && error != ENOMEM) {
		return error;
	}

	if (oldlenp) {
		*oldlenp = req.oldidx;
	}

	return error;
}

  
                                        
   
STATIC int
sysctl_old_user(struct sysctl_req *req, const void *p, size_t l)
{
	int error = 0;
	size_t i = 0;

	if (req->oldptr) {
		if (req->oldlen - req->oldidx < l) {
			return ENOMEM;
		}
		i = l;
		if (i > req->oldlen - req->oldidx) {
			i = req->oldlen - req->oldidx;
		}
		if (i > 0) {
			error = copyout((const void*)p, (req->oldptr + req->oldidx), i);
		}
	}
	req->oldidx += l;
	if (error) {
		return error;
	}
	if (req->oldptr && i < l) {
		return ENOMEM;
	}
	return 0;
}

STATIC int
sysctl_new_user(struct sysctl_req *req, void *p, size_t l)
{
	int error;

	if (!req->newptr) {
		return 0;
	}
	if (req->newlen - req->newidx < l) {
		return EINVAL;
	}
	error = copyin((req->newptr + req->newidx), p, l);
	req->newidx += l;
	return error;
}

#define WRITE_EXPERIMENT_FACTORS_ENTITLEMENT "com.apple.private.write-kr-experiment-factors"
  
                                                              
                                                                                         
                                                                      
   
STATIC bool
can_write_experiment_factors(__unused struct sysctl_req *req)
{
	if (IOTaskHasEntitlement(current_task(), WRITE_EXPERIMENT_FACTORS_ENTITLEMENT)) {
		return true;
	}
#if DEBUG || DEVELOPMENT
	return !proc_suser(req->p);
#else
	return false;
#endif                           
}

  
                                                                         
                                           
   

int
sysctl_root(boolean_t from_kernel, boolean_t string_is_canonical,
    char *namestring, size_t namestringlen,
    int *name, size_t namelen, struct sysctl_req *req)
{
	u_int indx;
	int i;
	struct sysctl_oid_iterator it;
	struct sysctl_oid *oid;
	sysctl_handler_t oid_handler = NULL;
	int error;
	boolean_t unlocked_node_found = FALSE;
	boolean_t namestring_started = FALSE;

	                                       
	lck_rw_lock_shared(&sysctl_geometry_lock);

	if (string_is_canonical) {
		                                                                          
		error = name2oid(namestring, name, &namelen);
		if (error) {
			goto err;
		}
	}

	it = sysctl_oid_iterator_begin(&sysctl__children);
	oid = sysctl_oid_iterator_next_system_order(&it);

	indx = 0;
	while (oid && indx < CTL_MAXNAME) {
		if (oid->oid_number == name[indx]) {
			if (!from_kernel && !string_is_canonical) {
				if (namestring_started) {
					if (strlcat(namestring, ".", namestringlen) >= namestringlen) {
						error = ENAMETOOLONG;
						goto err;
					}
				}

				if (strlcat(namestring, oid->oid_name, namestringlen) >= namestringlen) {
					error = ENAMETOOLONG;
					goto err;
				}
				namestring_started = TRUE;
			}

			indx++;
			if (!(oid->oid_kind & CTLFLAG_LOCKED)) {
				unlocked_node_found = TRUE;
			}
			if (oid->oid_kind & CTLFLAG_NOLOCK) {
				req->lock = 0;
			}
			  
                                                        
                                                   
                                                   
                   
     
                                                      
                                                  
                                                 
                                                       
                              
      
			if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
				if (oid->oid_handler) {
					goto found;
				}
				if (indx == namelen) {
					error = ENOENT;
					goto err;
				}

				it = sysctl_oid_iterator_begin(oid->oid_arg1);
				oid = sysctl_oid_iterator_next_system_order(&it);
			} else {
				if (indx != namelen) {
					error = EISDIR;
					goto err;
				}
				goto found;
			}
		} else {
			oid = sysctl_oid_iterator_next_system_order(&it);
		}
	}
	error = ENOENT;
	goto err;
found:

	  
                                                      
                                           
    
	if (!from_kernel && !string_is_canonical && (indx < namelen)) {
		char tempbuf[10];
		u_int indx2;

		for (indx2 = indx; indx2 < namelen; indx2++) {
			snprintf(tempbuf, sizeof(tempbuf), "%d", name[indx2]);

			if (namestring_started) {
				if (strlcat(namestring, ".", namestringlen) >= namestringlen) {
					error = ENAMETOOLONG;
					goto err;
				}
			}

			if (strlcat(namestring, tempbuf, namestringlen) >= namestringlen) {
				error = ENAMETOOLONG;
				goto err;
			}
			namestring_started = TRUE;
		}
	}

	                              
	if (req->newptr && (!(oid->oid_kind & CTLFLAG_WR) ||
	    ((oid->oid_kind & CTLFLAG_SECURE) && securelevel > 0))) {
		error = (EPERM);
		goto err;
	}

	  
                                                                       
    
	if (from_kernel && !(oid->oid_kind & CTLFLAG_KERN)) {
		error = (EPERM);
		goto err;
	}

	if (req->newptr && req->p) {
		if (oid->oid_kind & CTLFLAG_EXPERIMENT) {
			  
                                                                         
                                                                  
      
			if (!can_write_experiment_factors(req)) {
				error = (EPERM);
				goto err;
			}
		} else {
			  
                                                                     
                                                                    
                                                                      
                                                                       
                                                                     
                                                             
      
			if (!(oid->oid_kind & CTLFLAG_ANYBODY) &&
			    (error = proc_suser(req->p))) {
				goto err;
			}
		}
	}

	  
                                                                    
                   
    
	oid_handler = oid->oid_handler;
	if (!oid_handler) {
		error = EINVAL;
		goto err;
	}

	  
                                                                   
                                                                    
                                                                
                                       
    
	if ((oid->oid_kind & CTLFLAG_PERMANENT) == 0) {
		OSAddAtomic(1, &oid->oid_refcnt);
	}

	lck_rw_done(&sysctl_geometry_lock);

#if CONFIG_MACF
	if (!from_kernel) {
		error = mac_system_check_sysctlbyname(kauth_cred_get(),
		    namestring,
		    name,
		    namelen,
		    req->oldptr,
		    req->oldlen,
		    req->newptr,
		    req->newlen);
		if (error) {
			goto dropref;
		}
	}
#endif

	  
                                                                     
                                                         
    
	if (unlocked_node_found) {
		lck_mtx_lock(&sysctl_unlocked_node_lock);
	}

#if defined(HAS_APPLE_PAC)
	  
                                                                         
    
	oid_handler = ptrauth_auth_and_resign(oid_handler,
	    ptrauth_key_function_pointer,
	    ptrauth_blend_discriminator(&oid->oid_handler,
	    os_hash_kernel_pointer(oid->oid_arg1)),
	    ptrauth_key_function_pointer,
	    ptrauth_function_pointer_type_discriminator(typeof(oid_handler)));
#endif                             

	if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
		i = oid_handler(oid, name + indx, (int)(namelen - indx), req);
	} else {
		i = oid_handler(oid, oid->oid_arg1, oid->oid_arg2, req);
	}
	error = i;

	if (unlocked_node_found) {
		lck_mtx_unlock(&sysctl_unlocked_node_lock);
	}

#if CONFIG_MACF
	                                              
dropref:
#endif

	  
                                                                   
                                                             
                                                                
                                                                
                                                                     
                                                                  
                             
   
                                                                
                                                       
                                                        
          
    
	lck_rw_lock_shared(&sysctl_geometry_lock);

	if ((oid->oid_kind & CTLFLAG_PERMANENT) == 0) {
		if (OSAddAtomic(-1, &oid->oid_refcnt) == 1) {
			wakeup(&oid->oid_refcnt);
		}
	}

err:
	lck_rw_done(&sysctl_geometry_lock);
	return error;
}

void
sysctl_create_user_req(struct sysctl_req *req, struct proc *p, user_addr_t oldp,
    size_t oldlen, user_addr_t newp, size_t newlen)
{
	bzero(req, sizeof(*req));

	req->p = p;

	req->oldlen = oldlen;
	req->oldptr = oldp;

	if (newlen) {
		req->newlen = newlen;
		req->newptr = newp;
	}

	req->oldfunc = sysctl_old_user;
	req->newfunc = sysctl_new_user;
	req->lock = 1;

	return;
}

int
sysctl(proc_t p, struct sysctl_args *uap, __unused int32_t *retval)
{
	int error, new_error;
	size_t oldlen = 0, newlen;
	int name[CTL_MAXNAME];
	struct sysctl_req req;
	char *namestring;
	size_t namestringlen = MAXPATHLEN;

	  
                                               
    
	if (uap->namelen > CTL_MAXNAME || uap->namelen < 2) {
		return EINVAL;
	}
	error = copyin(uap->name, &name[0], uap->namelen * sizeof(int));
	if (error) {
		return error;
	}

	AUDIT_ARG(ctlname, name, uap->namelen);

	if (uap->newlen > SIZE_T_MAX) {
		return EINVAL;
	}
	newlen = (size_t)uap->newlen;

	if (uap->oldlenp != USER_ADDR_NULL) {
		uint64_t        oldlen64 = fuulong(uap->oldlenp);

		  
                                 
     
		if (oldlen64 > SIZE_T_MAX) {
			oldlen = SIZE_T_MAX;
		} else {
			oldlen = (size_t)oldlen64;
		}
	}

	sysctl_create_user_req(&req, p, uap->old, oldlen, uap->new, newlen);

	                                                                                                   
	if (uap->namelen == 2) {
		if (name[0] == CTL_KERN && name[1] < KERN_MAXID) {
			namestringlen = 32;                                        
		} else if (name[0] == CTL_HW && name[1] < HW_MAXID) {
			namestringlen = 32;                                
		}
	}

	namestring = kheap_alloc(KHEAP_TEMP, namestringlen, Z_WAITOK);
	if (!namestring) {
		oldlen = 0;
		goto err;
	}

	error = userland_sysctl(FALSE, namestring, namestringlen, name, uap->namelen, &req, &oldlen);

	kheap_free(KHEAP_TEMP, namestring, namestringlen);

	if ((error) && (error != ENOMEM)) {
		return error;
	}

err:
	if (uap->oldlenp != USER_ADDR_NULL) {
		  
                                                      
     
		new_error = suulong(uap->oldlenp, oldlen);

		if (new_error) {
			error = new_error;
		}
	}

	return error;
}

                                                
                                               
int
sys_sysctlbyname(proc_t p, struct sysctlbyname_args *uap, __unused int32_t *retval)
{
	int error, new_error;
	size_t oldlen = 0, newlen;
	char *name;
	size_t namelen = 0;
	struct sysctl_req req;
	int oid[CTL_MAXNAME];

	if (uap->namelen >= MAXPATHLEN) {                                  
		return ENAMETOOLONG;
	}
	namelen = (size_t)uap->namelen;

	name = kheap_alloc(KHEAP_TEMP, namelen + 1, Z_WAITOK);
	if (!name) {
		return ENOMEM;
	}

	error = copyin(uap->name, name, namelen);
	if (error) {
		kheap_free(KHEAP_TEMP, name, namelen + 1);
		return error;
	}
	name[namelen] = '\0';

	      
                                           
    

	if (uap->newlen > SIZE_T_MAX) {
		kheap_free(KHEAP_TEMP, name, namelen + 1);
		return EINVAL;
	}
	newlen = (size_t)uap->newlen;

	if (uap->oldlenp != USER_ADDR_NULL) {
		uint64_t        oldlen64 = fuulong(uap->oldlenp);

		  
                                 
     
		if (oldlen64 > SIZE_T_MAX) {
			oldlen = SIZE_T_MAX;
		} else {
			oldlen = (size_t)oldlen64;
		}
	}

	sysctl_create_user_req(&req, p, uap->old, oldlen, uap->new, newlen);

	error = userland_sysctl(TRUE, name, namelen + 1, oid, CTL_MAXNAME, &req, &oldlen);

	kheap_free(KHEAP_TEMP, name, namelen + 1);

	if ((error) && (error != ENOMEM)) {
		return error;
	}

	if (uap->oldlenp != USER_ADDR_NULL) {
		  
                                                      
     
		new_error = suulong(uap->oldlenp, oldlen);

		if (new_error) {
			error = new_error;
		}
	}

	return error;
}

  
                                                                         
                           
   
int
userland_sysctl(boolean_t string_is_canonical,
    char *namestring, size_t namestringlen,
    int *name, u_int namelen, struct sysctl_req *req,
    size_t *retval)
{
	int error = 0;
	struct sysctl_req req2;

	do {
		                                    
		req2 = *req;
		if (!string_is_canonical) {
			namestring[0] = '\0';
		}

		error = sysctl_root(FALSE, string_is_canonical, namestring, namestringlen, name, namelen, &req2);
	} while (error == EAGAIN);

	if (error && error != ENOMEM) {
		return error;
	}

	if (retval) {
		if (req2.oldptr && req2.oldidx > req2.oldlen) {
			*retval = req2.oldlen;
		} else {
			*retval = req2.oldidx;
		}
	}
	return error;
}

  
                                                           
  
                                                            
                           
  
                                                           
                          
  
                                                                       
   

int
kernel_sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	int oid[CTL_MAXNAME];
	int name2mib_oid[2];
	int error;
	size_t oidlen;

	                                             
	name2mib_oid[0] = 0;
	name2mib_oid[1] = 3;

	oidlen = sizeof(oid);
	error = kernel_sysctl(current_proc(), name2mib_oid, 2, oid, &oidlen, __DECONST(void *, name), strlen(name));
	oidlen /= sizeof(int);
	if (oidlen > UINT_MAX) {
		error = EDOM;
	}

	                     
	if (error == 0) {
		error = kernel_sysctl(current_proc(), oid, (u_int)oidlen, oldp, oldlenp, newp, newlen);
	}
	return error;
}

int
scalable_counter_sysctl_handler SYSCTL_HANDLER_ARGS
{
#pragma unused(arg2, oidp)
	scalable_counter_t counter = *(scalable_counter_t*) arg1;
	uint64_t value = counter_load(&counter);
	return SYSCTL_OUT(req, &value, sizeof(value));
}

#define X(name, T) \
int \
experiment_factor_##name##_handler SYSCTL_HANDLER_ARGS \
{ \
	int error, changed = 0; \
	T *ptr; \
	T new_value, current_value; \
	struct experiment_spec *spec = (struct experiment_spec *) arg1; \
	if (!arg1) { \
	        return EINVAL; \
	} \
	ptr = (T *)(spec->ptr); \
	current_value = *ptr; \
	error = sysctl_io_number(req, current_value, sizeof(T), &new_value, &changed); \
	if (error != 0) { \
	        return error; \
	} \
	if (changed) { \
	        if (new_value < (T) spec->min_value || new_value > (T) spec->max_value) { \
	                return EINVAL; \
	        } \
	        if (os_atomic_cmpxchg(&spec->modified, false, true, acq_rel)) { \
	                spec->original_value = current_value; \
	        } \
	        os_atomic_store_wide(ptr, new_value, relaxed); \
	} \
	return 0; \
}

experiment_factor_numeric_types
#undef X
