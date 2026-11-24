/*
 * Copyright (c) 2008, 2010 Apple Inc. All rights reserved.
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
#include <kern/ipc_misc.h>

#include <mach/mach_port.h>
#include <mach/vm_map.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>

extern void fileport_releasefg(struct fileglob *);

  
                 
  
                                                                         
               
  
                               
  
                                                                            
                                                   
   
ipc_port_t
fileport_alloc(struct fileglob *fg)
{
	return ipc_kobject_alloc_port((ipc_kobject_t)fg, IKOT_FILEPORT,
	           IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST);
}


  
                        
  
                                                                 
  
                                                       
  
                                                            
               
                                                         
                
  
                                                           
   
struct fileglob *
fileport_port_to_fileglob(ipc_port_t port)
{
	struct fileglob *fg = NULL;

	if (!IP_VALID(port)) {
		return NULL;
	}

	ip_lock(port);
	if (ip_active(port) && IKOT_FILEPORT == ip_kotype(port)) {
		fg = (void *) ip_get_kobject(port);
	}
	ip_unlock(port);

	return fg;
}


  
                  
  
                                                                        
                                                                      
                                              
  
                                                           
   
void
fileport_notify(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;
	struct fileglob *fg = NULL;

	if (!IP_VALID(port)) {
		panic("Invalid port passed to fileport_notify()\n");
	}

	ip_lock(port);

	fg = (struct fileglob *) ip_get_kobject(port);

	if (!ip_active(port)) {
		panic("Inactive port passed to fileport_notify()\n");
	}
	if (ip_kotype(port) != IKOT_FILEPORT) {
		panic("Port of type other than IKOT_FILEPORT passed to fileport_notify()\n");
	}
	if (fg == NULL) {
		panic("fileport without an assocated fileglob\n");
	}

	if (port->ip_srights == 0) {
		ip_unlock(port);

		fileport_releasefg(fg);
		ipc_port_dealloc_kernel(port);
	} else {
		ip_unlock(port);
	}
}

  
                  
  
                                                                            
                                                          
  
                                    
                                                    
                                          
                                                  
   
kern_return_t
fileport_invoke(task_t task, mach_port_name_t name,
    int (*action)(mach_port_name_t, struct fileglob *, void *),
    void *arg, int *rval)
{
	kern_return_t kr;
	ipc_port_t fileport;
	struct fileglob *fg;

	kr = ipc_object_copyin(task->itk_space, name,
	    MACH_MSG_TYPE_COPY_SEND, (ipc_object_t *)&fileport, 0, NULL,
	    IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	if ((fg = fileport_port_to_fileglob(fileport)) != NULL) {
		*rval = (*action)(name, fg, arg);
	} else {
		kr = KERN_FAILURE;
	}
	ipc_port_release_send(fileport);
	return kr;
}

  
                
  
                                                                         
  
                                                                   
                                                                   
                                                                
                                                               
                                                      
  
                                     
                                                   
                                           
   
kern_return_t
fileport_walk(task_t task,
    int (*action)(mach_port_name_t, struct fileglob *, void *arg),
    void *arg)
{
	mach_port_name_t *names;
	mach_msg_type_number_t ncnt, tcnt;
	vm_map_copy_t map_copy_names, map_copy_types;
	vm_map_address_t map_names;
	kern_return_t kr;
	uint_t i;
	int rval;

	  
                                                               
                                                            
                                                  
    

	kr = mach_port_names(task->itk_space,
	    (mach_port_name_t **)&map_copy_names, &ncnt,
	    (mach_port_type_t **)&map_copy_types, &tcnt);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	vm_map_copy_discard(map_copy_types);

	kr = vm_map_copyout(ipc_kernel_map, &map_names, map_copy_names);
	if (kr != KERN_SUCCESS) {
		vm_map_copy_discard(map_copy_names);
		return kr;
	}
	names = (mach_port_name_t *)(uintptr_t)map_names;

	for (rval = 0, i = 0; i < ncnt; i++) {
		if (fileport_invoke(task, names[i], action, arg,
		    &rval) == KERN_SUCCESS && -1 == rval) {
			break;                                        
		}
	}
	vm_deallocate(ipc_kernel_map,
	    (vm_address_t)names, ncnt * sizeof(*names));
	return KERN_SUCCESS;
}
