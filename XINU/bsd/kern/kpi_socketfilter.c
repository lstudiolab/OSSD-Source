/*
 * Copyright (c) 2003-2017 Apple Inc. All rights reserved.
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

#include <sys/kpi_socketfilter.h>

#include <sys/socket.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/proc.h>
#include <kern/locks.h>
#include <kern/thread.h>
#include <kern/debug.h>
#include <net/kext_net.h>
#include <net/if.h>
#include <net/net_api_stats.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <libkern/libkern.h>
#include <libkern/OSAtomic.h>
#include <os/refcnt.h>

#include <stdbool.h>
#include <string.h>

#define SFEF_ATTACHED           0x1                                
#define SFEF_NODETACH           0x2                                      
#define SFEF_NOSOCKET           0x4                         

  
                                                      
                                       
   
#define KM_IFADDR       KHEAP_DEFAULT

struct socket_filter_entry {
	struct socket_filter_entry      *sfe_next_onsocket;
	struct socket_filter_entry      *sfe_next_onfilter;
	struct socket_filter_entry      *sfe_next_oncleanup;

	struct socket_filter            *sfe_filter;
	struct socket                   *sfe_socket;
	void                            *sfe_cookie;

	uint32_t                        sfe_flags;
	int32_t                         sfe_refcount;
};

struct socket_filter {
	TAILQ_ENTRY(socket_filter)      sf_protosw_next;
	TAILQ_ENTRY(socket_filter)      sf_global_next;
	struct socket_filter_entry      *sf_entry_head;

	struct protosw                  *sf_proto;
	struct sflt_filter              sf_filter;
	struct os_refcnt                sf_refcount;
};

TAILQ_HEAD(socket_filter_list, socket_filter);

static LCK_GRP_DECLARE(sock_filter_lock_grp, "socket filter lock");
static LCK_RW_DECLARE(sock_filter_lock, &sock_filter_lock_grp);
static LCK_MTX_DECLARE(sock_filter_cleanup_lock, &sock_filter_lock_grp);

static struct socket_filter_list        sock_filter_head =
    TAILQ_HEAD_INITIALIZER(sock_filter_head);
static struct socket_filter_entry       *sock_filter_cleanup_entries = NULL;
static thread_t                         sock_filter_cleanup_thread = NULL;

static void sflt_cleanup_thread(void *, wait_result_t);
static void sflt_detach_locked(struct socket_filter_entry *entry);

#undef sflt_register
static errno_t sflt_register_common(const struct sflt_filter *filter, int domain,
    int type, int protocol, bool is_internal);
errno_t sflt_register(const struct sflt_filter *filter, int domain,
    int type, int protocol);


#pragma mark -- Internal State Management --

__private_extern__ int
sflt_permission_check(struct inpcb *inp)
{
	                                                  
	if (!(inp->inp_vflag & INP_IPV4) &&
	    !(inp->inp_vflag & INP_IPV6)) {
		return 0;
	}
	                                                               
	if (INP_INTCOPROC_ALLOWED(inp)) {
		return 1;
	}
	                                                                    
	if ((inp->inp_flags & INP_BOUND_IF) &&
	    IFNET_IS_INTCOPROC(inp->inp_boundifp)) {
		return 1;
	}
#if NECP
	  
                                                
                                                     
                                                       
                                                    
                                   
                                                         
                                      
    
	if (inp->inp_policyresult.policy_id == NECP_KERNEL_POLICY_ID_NONE) {
		necp_socket_find_policy_match(inp, NULL, NULL, 0);
	}

	                                                                    
	if (inp->inp_policyresult.results.filter_control_unit ==
	    NECP_FILTER_UNIT_NO_FILTER) {
		return 1;
	}
#endif           
	return 0;
}

static void
sflt_retain_locked(struct socket_filter *filter)
{
	os_ref_retain_locked(&filter->sf_refcount);
}

static void
sflt_release_locked(struct socket_filter *filter)
{
	if (os_ref_release_locked(&filter->sf_refcount) == 0) {
		                                    
		if (filter->sf_filter.sf_unregistered) {
			lck_rw_unlock_exclusive(&sock_filter_lock);
			filter->sf_filter.sf_unregistered(
				filter->sf_filter.sf_handle);
			lck_rw_lock_exclusive(&sock_filter_lock);
		}

		                    
		kheap_free(KM_IFADDR, filter, sizeof(struct socket_filter));
	}
}

static void
sflt_entry_retain(struct socket_filter_entry *entry)
{
	if (OSIncrementAtomic(&entry->sfe_refcount) <= 0) {
		panic("sflt_entry_retain - sfe_refcount <= 0\n");
		                
	}
}

static void
sflt_entry_release(struct socket_filter_entry *entry)
{
	SInt32 old = OSDecrementAtomic(&entry->sfe_refcount);
	if (old == 1) {
		                                 

		                           
		lck_mtx_lock(&sock_filter_cleanup_lock);

		                                       
		entry->sfe_next_oncleanup = sock_filter_cleanup_entries;
		sock_filter_cleanup_entries = entry;

		                                               
		if (entry->sfe_next_oncleanup == NULL) {
			if (sock_filter_cleanup_thread == NULL) {
				                     
				kernel_thread_start(sflt_cleanup_thread,
				    NULL, &sock_filter_cleanup_thread);
			} else {
				                       
				wakeup(&sock_filter_cleanup_entries);
			}
		}

		                           
		lck_mtx_unlock(&sock_filter_cleanup_lock);
	} else if (old <= 0) {
		panic("sflt_entry_release - sfe_refcount (%d) <= 0\n",
		    (int)old);
		                
	}
}

__attribute__((noreturn))
static void
sflt_cleanup_thread(void *blah, wait_result_t blah2)
{
#pragma unused(blah, blah2)
	while (1) {
		lck_mtx_lock(&sock_filter_cleanup_lock);
		while (sock_filter_cleanup_entries == NULL) {
			                                                  
			msleep(&sock_filter_cleanup_entries,
			    &sock_filter_cleanup_lock, PWAIT,
			    "sflt_cleanup", NULL);
		}

		                                         
		struct socket_filter_entry *dead = sock_filter_cleanup_entries;
		sock_filter_cleanup_entries = NULL;

		                   
		lck_mtx_unlock(&sock_filter_cleanup_lock);

		                                 
		lck_rw_lock_exclusive(&sock_filter_lock);

		                             
		struct socket_filter_entry      *entry;
		for (entry = dead; entry; entry = dead) {
			struct socket_filter_entry      **nextpp;

			dead = entry->sfe_next_oncleanup;

			                                                       
			if ((entry->sfe_flags & SFEF_NODETACH) == 0 &&
			    entry->sfe_filter->sf_filter.sf_detach) {
				entry->sfe_flags |= SFEF_NODETACH;
				lck_rw_unlock_exclusive(&sock_filter_lock);

				  
                                      
                             
       
				entry->sfe_filter->sf_filter.sf_detach(
					entry->sfe_cookie, entry->sfe_socket);

				lck_rw_lock_exclusive(&sock_filter_lock);
			}

			  
                                       
                                
      
			if ((entry->sfe_flags & SFEF_NOSOCKET) == 0) {
				for (nextpp = &entry->sfe_socket->so_filt;
				    *nextpp;
				    nextpp = &(*nextpp)->sfe_next_onsocket) {
					if (*nextpp == entry) {
						*nextpp =
						    entry->sfe_next_onsocket;
						break;
					}
				}
			}

			                                    
			for (nextpp = &entry->sfe_filter->sf_entry_head;
			    *nextpp; nextpp = &(*nextpp)->sfe_next_onfilter) {
				if (*nextpp == entry) {
					*nextpp = entry->sfe_next_onfilter;
					break;
				}
			}

			  
                                                          
      
			sflt_release_locked(entry->sfe_filter);
			entry->sfe_socket = NULL;
			entry->sfe_filter = NULL;
			kheap_free(KM_IFADDR, entry, sizeof(struct socket_filter_entry));
		}

		                                 
		lck_rw_unlock_exclusive(&sock_filter_lock);
	}
	                
}

static int
sflt_attach_locked(struct socket *so, struct socket_filter *filter,
    int socklocked)
{
	int error = 0;
	struct socket_filter_entry *entry = NULL;

	if (sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	if (filter == NULL) {
		return ENOENT;
	}

	for (entry = so->so_filt; entry; entry = entry->sfe_next_onfilter) {
		if (entry->sfe_filter->sf_filter.sf_handle ==
		    filter->sf_filter.sf_handle) {
			return EEXIST;
		}
	}
	                                      
	entry = kheap_alloc(KM_IFADDR, sizeof(struct socket_filter_entry),
	    Z_WAITOK);
	if (entry == NULL) {
		return ENOMEM;
	}

	                                        
	entry->sfe_cookie = NULL;
	entry->sfe_flags = SFEF_ATTACHED;
	entry->sfe_refcount = 1;                                            

	                                      
	sflt_retain_locked(filter);
	entry->sfe_filter = filter;
	entry->sfe_next_onfilter = filter->sf_entry_head;
	filter->sf_entry_head = entry;

	                                             
	entry->sfe_socket = so;
	entry->sfe_next_onsocket = so->so_filt;
	so->so_filt = entry;

	if (entry->sfe_filter->sf_filter.sf_attach) {
		                                           
		sflt_entry_retain(entry);

		  
                               
                                          
     
		lck_rw_unlock_exclusive(&sock_filter_lock);

		                       
		if (socklocked) {
			socket_unlock(so, 0);
		}

		                                                   
		error = entry->sfe_filter->sf_filter.sf_attach(
			&entry->sfe_cookie, so);

		                           
		if (socklocked) {
			socket_lock(so, 0);
		}

		                            
		lck_rw_lock_exclusive(&sock_filter_lock);

		  
                                             
                                 
     
		if (error) {
			                          
			entry->sfe_flags |= SFEF_NODETACH;
			sflt_detach_locked(entry);
		}

		                                                        
		sflt_entry_release(entry);
	}

	return error;
}

errno_t
sflt_attach_internal(socket_t socket, sflt_handle handle)
{
	if (socket == NULL || handle == 0) {
		return EINVAL;
	}

	int result = EINVAL;

	lck_rw_lock_exclusive(&sock_filter_lock);

	struct socket_filter *filter = NULL;
	TAILQ_FOREACH(filter, &sock_filter_head, sf_global_next) {
		if (filter->sf_filter.sf_handle == handle) {
			break;
		}
	}

	if (filter) {
		result = sflt_attach_locked(socket, filter, 1);
	}

	lck_rw_unlock_exclusive(&sock_filter_lock);

	return result;
}

static void
sflt_detach_locked(struct socket_filter_entry *entry)
{
	if ((entry->sfe_flags & SFEF_ATTACHED) != 0) {
		entry->sfe_flags &= ~SFEF_ATTACHED;
		sflt_entry_release(entry);
	}
}

#pragma mark -- Socket Layer Hooks --

__private_extern__ void
sflt_initsock(struct socket *so)
{
	  
                                                          
                                  
    
	struct protosw *proto = so->so_proto->pr_protosw;

	lck_rw_lock_shared(&sock_filter_lock);
	if (TAILQ_FIRST(&proto->pr_filter_head) != NULL) {
		                               
		if (!lck_rw_lock_shared_to_exclusive(&sock_filter_lock)) {
			lck_rw_lock_exclusive(&sock_filter_lock);
		}

		  
                                                          
                                                           
                                                         
                                                                
                                                                 
                                                              
                                                            
                                    
     
		struct socket_filter *filter =
		    TAILQ_FIRST(&proto->pr_filter_head);

		sflt_retain_locked(filter);

		while (filter) {
			struct socket_filter *filter_next;
			  
                                         
                        
      
			sflt_attach_locked(so, filter, 0);

			filter_next = TAILQ_NEXT(filter, sf_protosw_next);
			if (filter_next) {
				sflt_retain_locked(filter_next);
			}

			  
                                             
                               
      
			sflt_release_locked(filter);
			filter = filter_next;
		}
	}
	lck_rw_done(&sock_filter_lock);
}

  
                
  
                                        
   
__private_extern__ void
sflt_termsock(struct socket *so)
{
	lck_rw_lock_exclusive(&sock_filter_lock);

	struct socket_filter_entry *entry;

	while ((entry = so->so_filt) != NULL) {
		                                
		so->so_filt = entry->sfe_next_onsocket;
		entry->sfe_flags |= SFEF_NOSOCKET;

		                 
		sflt_detach_locked(entry);

		  
                                                                
                                                               
                                                                
                                                         
                                                 
     
		if ((entry->sfe_flags & SFEF_NODETACH) == 0 &&
		    entry->sfe_filter->sf_filter.sf_detach) {
			void *sfe_cookie = entry->sfe_cookie;
			struct socket_filter *sfe_filter = entry->sfe_filter;

			                              
			sflt_retain_locked(sfe_filter);

			                                                
			entry->sfe_flags |= SFEF_NODETACH;

			                                                      
			lck_rw_unlock_exclusive(&sock_filter_lock);
			sfe_filter->sf_filter.sf_detach(sfe_cookie, so);
			lck_rw_lock_exclusive(&sock_filter_lock);

			                        
			sflt_release_locked(sfe_filter);
		}
	}

	lck_rw_unlock_exclusive(&sock_filter_lock);
}


static void
sflt_notify_internal(struct socket *so, sflt_event_t event, void *param,
    sflt_handle handle)
{
	if (so->so_filt == NULL) {
		return;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry; entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_notify &&
		    ((handle && entry->sfe_filter->sf_filter.sf_handle !=
		    handle) || !handle)) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				unlocked = 1;
				socket_unlock(so, 0);
			}

			                             
			entry->sfe_filter->sf_filter.sf_notify(
				entry->sfe_cookie, so, event, param);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked != 0) {
		socket_lock(so, 0);
	}
}

__private_extern__ void
sflt_notify(struct socket *so, sflt_event_t event, void  *param)
{
	sflt_notify_internal(so, event, param, 0);
}

static void
sflt_notify_after_register(struct socket *so, sflt_event_t event,
    sflt_handle handle)
{
	sflt_notify_internal(so, event, NULL, handle);
}

__private_extern__ int
sflt_ioctl(struct socket *so, u_long cmd, caddr_t data)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_ioctl) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_ioctl(
				entry->sfe_cookie, so, cmd, data);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_bind(struct socket *so, const struct sockaddr *nam)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_bind) {
			  
                                 
                                    
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_bind(
				entry->sfe_cookie, so, nam);

			  
                                           
                       
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_listen(struct socket *so)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_listen) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_listen(
				entry->sfe_cookie, so);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_accept(struct socket *head, struct socket *so,
    const struct sockaddr *local, const struct sockaddr *remote)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_accept) {
			  
                                 
                                    
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_accept(
				entry->sfe_cookie, head, so, local, remote);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_getsockname(struct socket *so, struct sockaddr **local)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_getsockname) {
			  
                                 
                                    
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_getsockname(
				entry->sfe_cookie, so, local);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_getpeername(struct socket *so, struct sockaddr **remote)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_getpeername) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_getpeername(
				entry->sfe_cookie, so, remote);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_connectin(struct socket *so, const struct sockaddr *remote)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_connect_in) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_connect_in(
				entry->sfe_cookie, so, remote);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

static int
sflt_connectout_common(struct socket *so, const struct sockaddr *nam)
{
	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_connect_out) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_connect_out(
				entry->sfe_cookie, so, nam);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_connectout(struct socket *so, const struct sockaddr *nam)
{
	char buf[SOCK_MAXADDRLEN];
	struct sockaddr *sa;
	int error;

	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	  
                                  
                                                             
    
	bzero(buf, sizeof(buf));
	bcopy(nam, buf, nam->sa_len);
	sa = (struct sockaddr *)buf;

	error = sflt_connectout_common(so, sa);
	if (error != 0) {
		return error;
	}

	  
                                             
    
	if (bcmp(sa, nam, nam->sa_len) != 0) {
		bcopy(sa, (struct sockaddr *)(uintptr_t)nam, nam->sa_len);
	}

	return 0;
}

__private_extern__ int
sflt_setsockopt(struct socket *so, struct sockopt *sopt)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_setoption) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_setoption(
				entry->sfe_cookie, so, sopt);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_getsockopt(struct socket *so, struct sockopt *sopt)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_getoption) {
			  
                                         
                            
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_getoption(
				entry->sfe_cookie, so, sopt);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

__private_extern__ int
sflt_data_out(struct socket *so, const struct sockaddr *to, mbuf_t *data,
    mbuf_t *control, sflt_data_flag_t flags)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int unlocked = 0;
	int setsendthread = 0;
	int error = 0;

	lck_rw_lock_shared(&sock_filter_lock);
	for (entry = so->so_filt; entry && error == 0;
	    entry = entry->sfe_next_onsocket) {
		                                      
		if (so->so_flags & SOF_MP_SUBFLOW) {
			continue;
		}
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_data_out) {
			  
                                 
                                    
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				if (so->so_send_filt_thread == NULL) {
					setsendthread = 1;
					so->so_send_filt_thread =
					    current_thread();
				}
				socket_unlock(so, 0);
				unlocked = 1;
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_data_out(
				entry->sfe_cookie, so, to, data, control, flags);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
		if (setsendthread) {
			so->so_send_filt_thread = NULL;
		}
	}

	return error;
}

__private_extern__ int
sflt_data_in(struct socket *so, const struct sockaddr *from, mbuf_t *data,
    mbuf_t *control, sflt_data_flag_t flags)
{
	if (so->so_filt == NULL || sflt_permission_check(sotoinpcb(so))) {
		return 0;
	}

	struct socket_filter_entry *entry;
	int error = 0;
	int unlocked = 0;

	lck_rw_lock_shared(&sock_filter_lock);

	for (entry = so->so_filt; entry && (error == 0);
	    entry = entry->sfe_next_onsocket) {
		                                      
		if (so->so_flags & SOF_MP_SUBFLOW) {
			continue;
		}
		if ((entry->sfe_flags & SFEF_ATTACHED) &&
		    entry->sfe_filter->sf_filter.sf_data_in) {
			  
                                 
                                    
      
			sflt_entry_retain(entry);
			lck_rw_unlock_shared(&sock_filter_lock);

			                                                     
			if (unlocked == 0) {
				unlocked = 1;
				socket_unlock(so, 0);
			}

			                     
			error = entry->sfe_filter->sf_filter.sf_data_in(
				entry->sfe_cookie, so, from, data, control, flags);

			  
                                       
                           
      
			lck_rw_lock_shared(&sock_filter_lock);
			sflt_entry_release(entry);
		}
	}
	lck_rw_unlock_shared(&sock_filter_lock);

	if (unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

#pragma mark -- KPI --

errno_t
sflt_attach(socket_t socket, sflt_handle handle)
{
	socket_lock(socket, 1);
	errno_t result = sflt_attach_internal(socket, handle);
	socket_unlock(socket, 1);
	return result;
}

errno_t
sflt_detach(socket_t socket, sflt_handle handle)
{
	struct socket_filter_entry *entry;
	errno_t result = 0;

	if (socket == NULL || handle == 0) {
		return EINVAL;
	}

	lck_rw_lock_exclusive(&sock_filter_lock);
	for (entry = socket->so_filt; entry; entry = entry->sfe_next_onsocket) {
		if (entry->sfe_filter->sf_filter.sf_handle == handle &&
		    (entry->sfe_flags & SFEF_ATTACHED) != 0) {
			break;
		}
	}

	if (entry != NULL) {
		sflt_detach_locked(entry);
	}
	lck_rw_unlock_exclusive(&sock_filter_lock);

	return result;
}

struct solist {
	struct solist *next;
	struct socket *so;
};

static errno_t
sflt_register_common(const struct sflt_filter *filter, int domain, int type,
    int  protocol, bool is_internal)
{
	struct socket_filter *sock_filt = NULL;
	struct socket_filter *match = NULL;
	int error = 0;
	struct protosw *pr;
	unsigned int len;
	struct socket *so;
	struct inpcb *inp;
	struct solist *solisthead = NULL, *solist = NULL;

	if ((domain != PF_INET) && (domain != PF_INET6)) {
		return ENOTSUP;
	}

	pr = pffindproto(domain, protocol, type);
	if (pr == NULL) {
		return ENOENT;
	}

	if (filter->sf_attach == NULL || filter->sf_detach == NULL ||
	    filter->sf_handle == 0 || filter->sf_name == NULL) {
		return EINVAL;
	}

	                                
	sock_filt = kheap_alloc(KM_IFADDR,
	    sizeof(struct socket_filter), Z_WAITOK | Z_ZERO);
	if (sock_filt == NULL) {
		return ENOBUFS;
	}

	                                                                 
	len = sizeof(*filter) - sizeof(struct sflt_filter_ext);
	  
                                                            
                                                          
                                                        
    
	if (filter->sf_flags & SFLT_EXTENDED) {
		unsigned int ext_len = filter->sf_len;

		if (ext_len > sizeof(struct sflt_filter_ext)) {
			ext_len = sizeof(struct sflt_filter_ext);
		}

		len += ext_len;
	}
	bcopy(filter, &sock_filt->sf_filter, len);

	lck_rw_lock_exclusive(&sock_filter_lock);
	                                
	TAILQ_FOREACH(match, &sock_filter_head, sf_global_next) {
		if (match->sf_filter.sf_handle ==
		    sock_filt->sf_filter.sf_handle) {
			break;
		}
	}

	                                                       
	if (match == NULL) {
		TAILQ_INSERT_TAIL(&sock_filter_head, sock_filt, sf_global_next);
		if ((sock_filt->sf_filter.sf_flags & SFLT_GLOBAL) != 0) {
			TAILQ_INSERT_TAIL(&pr->pr_filter_head, sock_filt,
			    sf_protosw_next);
			sock_filt->sf_proto = pr;
		}
		os_ref_init(&sock_filt->sf_refcount, NULL);

		OSIncrementAtomic64(&net_api_stats.nas_sfltr_register_count);
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_sfltr_register_total);
		if (is_internal) {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_sfltr_register_os_total);
		}
	}
	lck_rw_unlock_exclusive(&sock_filter_lock);

	if (match != NULL) {
		kheap_free(KM_IFADDR, sock_filt, sizeof(struct socket_filter));
		return EEXIST;
	}

	if (!(filter->sf_flags & SFLT_EXTENDED_REGISTRY)) {
		return error;
	}

	  
                                                                
    
#define SOLIST_ADD(_so) do {                                            \
	solist->next = solisthead;                                      \
	sock_retain((_so));                                             \
	solist->so = (_so);                                             \
	solisthead = solist;                                            \
} while (0)
	if (protocol == IPPROTO_TCP) {
		lck_rw_lock_shared(tcbinfo.ipi_lock);
		LIST_FOREACH(inp, tcbinfo.ipi_listhead, inp_list) {
			so = inp->inp_socket;
			if (so == NULL || (so->so_state & SS_DEFUNCT) ||
			    (!(so->so_flags & SOF_MP_SUBFLOW) &&
			    (so->so_state & SS_NOFDREF)) ||
			    !SOCK_CHECK_DOM(so, domain) ||
			    !SOCK_CHECK_TYPE(so, type)) {
				continue;
			}
			solist = kheap_alloc(KHEAP_TEMP, sizeof(struct solist), Z_NOWAIT);
			if (!solist) {
				continue;
			}
			SOLIST_ADD(so);
		}
		lck_rw_done(tcbinfo.ipi_lock);
	} else if (protocol == IPPROTO_UDP) {
		lck_rw_lock_shared(udbinfo.ipi_lock);
		LIST_FOREACH(inp, udbinfo.ipi_listhead, inp_list) {
			so = inp->inp_socket;
			if (so == NULL || (so->so_state & SS_DEFUNCT) ||
			    (!(so->so_flags & SOF_MP_SUBFLOW) &&
			    (so->so_state & SS_NOFDREF)) ||
			    !SOCK_CHECK_DOM(so, domain) ||
			    !SOCK_CHECK_TYPE(so, type)) {
				continue;
			}
			solist = kheap_alloc(KHEAP_TEMP, sizeof(struct solist), Z_NOWAIT);
			if (!solist) {
				continue;
			}
			SOLIST_ADD(so);
		}
		lck_rw_done(udbinfo.ipi_lock);
	}
	                                                           
#undef SOLIST_ADD

	while (solisthead) {
		sflt_handle handle = filter->sf_handle;

		so = solisthead->so;
		socket_lock(so, 0);
		sflt_initsock(so);
		if (so->so_state & SS_ISCONNECTING) {
			sflt_notify_after_register(so, sock_evt_connecting,
			    handle);
		} else if (so->so_state & SS_ISCONNECTED) {
			sflt_notify_after_register(so, sock_evt_connected,
			    handle);
		} else if ((so->so_state &
		    (SS_ISDISCONNECTING | SS_CANTRCVMORE | SS_CANTSENDMORE)) ==
		    (SS_ISDISCONNECTING | SS_CANTRCVMORE | SS_CANTSENDMORE)) {
			sflt_notify_after_register(so, sock_evt_disconnecting,
			    handle);
		} else if ((so->so_state &
		    (SS_CANTRCVMORE | SS_CANTSENDMORE | SS_ISDISCONNECTED)) ==
		    (SS_CANTRCVMORE | SS_CANTSENDMORE | SS_ISDISCONNECTED)) {
			sflt_notify_after_register(so, sock_evt_disconnected,
			    handle);
		} else if (so->so_state & SS_CANTSENDMORE) {
			sflt_notify_after_register(so, sock_evt_cantsendmore,
			    handle);
		} else if (so->so_state & SS_CANTRCVMORE) {
			sflt_notify_after_register(so, sock_evt_cantrecvmore,
			    handle);
		}
		socket_unlock(so, 0);
		                                                        
		sock_release(so);
		solist = solisthead;
		solisthead = solisthead->next;
		kheap_free(KHEAP_TEMP, solist, sizeof(struct solist));
	}

	return error;
}

errno_t
sflt_register_internal(const struct sflt_filter *filter, int domain, int type,
    int  protocol)
{
	return sflt_register_common(filter, domain, type, protocol, true);
}

errno_t
sflt_register(const struct sflt_filter *filter, int domain, int type,
    int  protocol)
{
	return sflt_register_common(filter, domain, type, protocol, false);
}

errno_t
sflt_unregister(sflt_handle handle)
{
	struct socket_filter *filter;
	lck_rw_lock_exclusive(&sock_filter_lock);

	                                  
	TAILQ_FOREACH(filter, &sock_filter_head, sf_global_next) {
		if (filter->sf_filter.sf_handle == handle) {
			break;
		}
	}

	if (filter) {
		VERIFY(OSDecrementAtomic64(&net_api_stats.nas_sfltr_register_count) > 0);

		                                    
		TAILQ_REMOVE(&sock_filter_head, filter, sf_global_next);

		                                     
		if ((filter->sf_filter.sf_flags & SFLT_GLOBAL) != 0) {
			TAILQ_REMOVE(&filter->sf_proto->pr_filter_head,
			    filter, sf_protosw_next);
		}

		                             
		struct socket_filter_entry *entry = NULL;

		for (entry = filter->sf_entry_head; entry;
		    entry = entry->sfe_next_onfilter) {
			sflt_detach_locked(entry);
		}

		                        
		sflt_release_locked(filter);
	}

	lck_rw_unlock_exclusive(&sock_filter_lock);

	if (filter == NULL) {
		return ENOENT;
	}

	return 0;
}

errno_t
sock_inject_data_in(socket_t so, const struct sockaddr *from, mbuf_t data,
    mbuf_t control, sflt_data_flag_t flags)
{
	int error = 0;

	if (so == NULL || data == NULL) {
		return EINVAL;
	}

	if (flags & sock_data_filt_flag_oob) {
		return ENOTSUP;
	}

	socket_lock(so, 1);

	                                        
	if (so->so_flags & SOF_MP_SUBFLOW) {
		error = ENOTSUP;
		goto done;
	}

	if (from) {
		if (sbappendaddr(&so->so_rcv,
		    (struct sockaddr *)(uintptr_t)from, data, control, NULL)) {
			sorwakeup(so);
		}
		goto done;
	}

	if (control) {
		if (sbappendcontrol(&so->so_rcv, data, control, NULL)) {
			sorwakeup(so);
		}
		goto done;
	}

	if (flags & sock_data_filt_flag_record) {
		if (control || from) {
			error = EINVAL;
			goto done;
		}
		if (sbappendrecord(&so->so_rcv, (struct mbuf *)data)) {
			sorwakeup(so);
		}
		goto done;
	}

	if (sbappend(&so->so_rcv, data)) {
		sorwakeup(so);
	}
done:
	socket_unlock(so, 1);
	return error;
}

errno_t
sock_inject_data_out(socket_t so, const struct sockaddr *to, mbuf_t data,
    mbuf_t control, sflt_data_flag_t flags)
{
	int sosendflags = 0;

	                                        
	if (so->so_flags & SOF_MP_SUBFLOW) {
		return ENOTSUP;
	}

	if (flags & sock_data_filt_flag_oob) {
		sosendflags = MSG_OOB;
	}
	return sosend(so, (struct sockaddr *)(uintptr_t)to, NULL,
	           data, control, sosendflags);
}

sockopt_dir
sockopt_direction(sockopt_t sopt)
{
	return (sopt->sopt_dir == SOPT_GET) ? sockopt_get : sockopt_set;
}

int
sockopt_level(sockopt_t sopt)
{
	return sopt->sopt_level;
}

int
sockopt_name(sockopt_t sopt)
{
	return sopt->sopt_name;
}

size_t
sockopt_valsize(sockopt_t sopt)
{
	return sopt->sopt_valsize;
}

errno_t
sockopt_copyin(sockopt_t sopt, void *data, size_t len)
{
	return sooptcopyin(sopt, data, len, len);
}

errno_t
sockopt_copyout(sockopt_t sopt, void *data, size_t len)
{
	return sooptcopyout(sopt, data, len);
}
