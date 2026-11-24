/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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
#include <mach/mach_types.h>
#include <mach/notify.h>
#include <ipc/ipc_port.h>
#include <kern/ipc_kobject.h>
#include <kern/audit_sessionport.h>
#include <libkern/OSAtomic.h>

#if CONFIG_AUDIT
  
                       
  
                                                            
  
                                                                  
                                             
                                                                           
                                                                 
  
                                        
                                                                    
                                           
  
                                                                        
                                                              
                                                            
                                                             
                                                             
                                                       
   
ipc_port_t
audit_session_mksend(struct auditinfo_addr *aia_p, ipc_port_t *sessionport)
{
	audit_session_aiaref(aia_p);
	if (!ipc_kobject_make_send_lazy_alloc_port(sessionport,
	    (ipc_kobject_t)aia_p, IKOT_AU_SESSIONPORT, IPC_KOBJECT_ALLOC_NONE, false, 0)) {
		audit_session_aiaunref(aia_p);
	}

	return *sessionport;
}


  
                          
  
                                                                             
  
                                 
  
                                                                
                                             
                                                               
                    
  
                                                              
   
struct auditinfo_addr *
audit_session_porttoaia(ipc_port_t port)
{
	struct auditinfo_addr *aia_p = NULL;

	if (IP_VALID(port)) {
		ip_lock(port);
		if (IKOT_AU_SESSIONPORT == ip_kotype(port)) {
			require_ip_active(port);
			aia_p = (struct auditinfo_addr *)ip_get_kobject(port);
		}
		ip_unlock(port);
	}

	return aia_p;
}


  
                          
  
                                                                   
  
                                                           
  
                                                                 
                                                                      
                              
   
void
audit_session_nosenders(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;
	struct auditinfo_addr *port_aia_p = NULL;

	require_ip_active(port);
	assert(IKOT_AU_SESSIONPORT == ip_kotype(port));
	port_aia_p = (struct auditinfo_addr *)ip_get_kobject(port);
	assert(NULL != port_aia_p);

	audit_session_aiaunref(port_aia_p);
}

void
audit_session_portdestroy(ipc_port_t *sessionport)
{
	ipc_port_t port = *sessionport;

	if (IP_VALID(port)) {
		require_ip_active(port);
		assert(IKOT_AU_SESSIONPORT == ip_kotype(port));
		ipc_kobject_set_atomically(port, IKO_NULL, IKOT_NONE);
		ipc_port_dealloc_kernel(port);
		*sessionport = IP_NULL;
	}
}
#endif                   
