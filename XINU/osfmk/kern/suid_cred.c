/*
 * Copyright (c) 2019-2020 Apple Inc. All rights reserved.
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

  
  
                                                                           
                                                                               
                                                         
  
                                                                               
                                                                                
                                                                              
                                                                        
                                                                     
   

#include <kern/ipc_kobject.h>
#include <kern/queue.h>
#include <kern/suid_cred.h>

#include <mach/mach_types.h>
#include <mach/task.h>

#include <IOKit/IOBSD.h>

                                                                
struct vnode;
struct vfs_context;
extern int vnode_lookup(const char *, int, struct vnode **,
    struct vfs_context *);
extern struct vfs_context * vfs_context_current(void);
extern int vnode_put(struct vnode *);

                                                          
struct ucred;
extern int kauth_cred_issuser(struct ucred *);
extern struct ucred *kauth_cred_get(void);

                                                                             
struct suid_cred {
	ipc_port_t port;
	struct vnode *vnode;
	uint32_t uid;
};

static ZONE_DECLARE(suid_cred_zone, "suid_cred",
    sizeof(struct suid_cred), ZC_NONE);

                                                                               
                          
static suid_cred_t
suid_cred_alloc(struct vnode *vnode, uint32_t uid)
{
	suid_cred_t sc = SUID_CRED_NULL;

	assert(vnode != NULL);

	sc = zalloc(suid_cred_zone);
	if (sc != NULL) {
		                                                   
		sc->port = IP_NULL;
		sc->vnode = vnode;
		sc->uid = uid;
	}

	return sc;
}

static void
suid_cred_free(suid_cred_t sc)
{
	assert(sc != NULL);
	assert(sc->vnode != NULL);

	vnode_put(sc->vnode);

	sc->uid = UINT32_MAX;
	sc->vnode = NULL;
	sc->port = IP_NULL;

	zfree(suid_cred_zone, sc);
}

void
suid_cred_destroy(ipc_port_t port)
{
	suid_cred_t sc = NULL;

	ip_lock(port);
	assert(ip_kotype(port) == IKOT_SUID_CRED);
	sc = (suid_cred_t)ipc_kobject_get(port);
	ipc_kobject_set_atomically(port, IKO_NULL, IKOT_NONE);
	ip_unlock(port);

	assert(sc->port == port);

	suid_cred_free(sc);
}

void
suid_cred_notify(mach_msg_header_t *msg)
{
	assert(msg->msgh_id == MACH_NOTIFY_NO_SENDERS);

	mach_no_senders_notification_t *not = (mach_no_senders_notification_t *)msg;
	ipc_port_t port = not->not_header.msgh_remote_port;

	if (IP_VALID(port)) {
		ipc_port_dealloc_kernel(port);
	}
}

ipc_port_t
convert_suid_cred_to_port(suid_cred_t sc)
{
	if (sc == NULL) {
		return IP_NULL;
	}

	if (!ipc_kobject_make_send_lazy_alloc_port(&sc->port,
	    (ipc_kobject_t) sc, IKOT_SUID_CRED, IPC_KOBJECT_ALLOC_NONE, false, 0)) {
		suid_cred_free(sc);
		return IP_NULL;
	}

	return sc->port;
}

  
                                                                             
                                                                    
   
int
suid_cred_verify(ipc_port_t port, struct vnode *vnode, uint32_t *uid)
{
	suid_cred_t sc = NULL;
	int ret = -1;

	if (!IP_VALID(port)) {
		return -1;
	}

	ip_lock(port);

	if (ip_kotype(port) != IKOT_SUID_CRED) {
		ip_unlock(port);
		return -1;
	}

	if (!ip_active(port)) {
		ip_unlock(port);
		return -1;
	}

	sc = (suid_cred_t)ipc_kobject_get(port);

	if (vnode != sc->vnode) {
		ip_unlock(port);
		return -1;
	}

	*uid = sc->uid;
	ret = 0;

	ipc_port_destroy(port);
	return ret;
}

kern_return_t
task_create_suid_cred(
	task_t task,
	suid_cred_path_t path,
	suid_cred_uid_t uid,
	suid_cred_t *sc_p)
{
	suid_cred_t sc = NULL;
	struct vnode *vnode;
	int  err = -1;

	if (task == TASK_NULL || task != current_task()) {
		return KERN_INVALID_ARGUMENT;
	}

	                              
	if (!IOTaskHasEntitlement(task, "com.apple.private.suid_cred")) {
		return KERN_NO_ACCESS;
	}

	                             
	if (!kauth_cred_issuser(kauth_cred_get())) {
		return KERN_NO_ACCESS;
	}

	                               
	err = vnode_lookup(path, 0, &vnode, vfs_context_current());
	if (err != 0) {
		return KERN_INVALID_ARGUMENT;
	}

	sc = suid_cred_alloc(vnode, uid);
	if (sc == NULL) {
		(void) vnode_put(vnode);
		return KERN_RESOURCE_SHORTAGE;
	}

	*sc_p = sc;

	return KERN_SUCCESS;
}
