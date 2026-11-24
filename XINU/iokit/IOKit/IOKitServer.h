/*
 * Copyright (c) 1998-2011 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 */

  
                                                               
                   
   

#ifndef _IOKIT_IOKITSERVER_H
#define _IOKIT_IOKITSERVER_H

#include <IOKit/IOTypes.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/OSMessageNotification.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <mach/kmod.h>
#ifdef __cplusplus
}
#endif

                      
   
                                                 
                                                                                              
                                                                                                                                          
                                                                                                    
                                                                                                                                                
                                                                                                      
                                                                      
                                                                               
                                                                                                                                   
                                                                                              
                                                                                                                                          
   
enum {
	kIOCatalogAddDrivers        = 1,
	kIOCatalogAddDriversNoMatch,
	kIOCatalogRemoveDrivers,
	kIOCatalogRemoveDriversNoMatch,
	kIOCatalogStartMatching__Removed,
	kIOCatalogRemoveKernelLinker__Removed,
	kIOCatalogKextdActive,
	kIOCatalogKextdFinishedLaunching,
	kIOCatalogResetDrivers,
	kIOCatalogResetDriversNoMatch
};

                     
   
                                               
                                                                                       
   
enum {
	kIOCatalogGetContents         = 1,
	kIOCatalogGetModuleDemandList = 2,
	kIOCatalogGetCacheMissList    = 3,
	kIOCatalogGetROMMkextList     = 4
};

                   
   
                                            
                                                                                                                     
   
enum {
	kIOCatalogResetDefault      = 1
};

                       
   
                                                  
                                                                                                                         
                                                                                                                                    
                                                                                  
   
enum {
	kIOCatalogModuleUnload      = 1,
	kIOCatalogModuleTerminate,
	kIOCatalogServiceTerminate
};


#ifdef XNU_KERNEL_PRIVATE

#ifdef __cplusplus
extern "C" {
#endif                  

#include <kern/ipc_kobject.h>

  
                                      
   

extern void iokit_add_reference( io_object_t obj, ipc_kobject_type_t type );

extern ipc_port_t iokit_port_for_object( io_object_t obj,
    ipc_kobject_type_t type );

extern kern_return_t iokit_client_died( io_object_t obj,
    ipc_port_t port, ipc_kobject_type_t type, mach_port_mscount_t * mscount );

extern kern_return_t
iokit_client_memory_for_type(
	io_object_t     connect,
	unsigned int    type,
	unsigned int *  flags,
	vm_address_t *  address,
	vm_size_t    *  size );

  
                                 
   

extern ipc_port_t iokit_alloc_object_port( io_object_t obj,
    ipc_kobject_type_t type );

extern kern_return_t iokit_destroy_object_port( ipc_port_t port );

extern mach_port_name_t iokit_make_send_right( task_t task,
    io_object_t obj, ipc_kobject_type_t type );

extern mach_port_t ipc_port_make_send(mach_port_t);
extern mach_port_t ipc_port_copy_send(mach_port_t);
extern void ipc_port_release_send(ipc_port_t port);

extern io_object_t iokit_lookup_io_object(ipc_port_t port, ipc_kobject_type_t type);

extern kern_return_t iokit_mod_send_right( task_t task, mach_port_name_t name, mach_port_delta_t delta );

extern io_object_t iokit_lookup_object_with_port_name(mach_port_name_t name, ipc_kobject_type_t type, task_t task);

extern io_object_t iokit_lookup_connect_ref_current_task(mach_port_name_t name);
extern io_object_t iokit_lookup_uext_ref_current_task(mach_port_name_t name);

extern void iokit_retain_port( ipc_port_t port );
extern void iokit_release_port( ipc_port_t port );
extern void iokit_make_port_send( ipc_port_t port );
extern void iokit_release_port_send( ipc_port_t port );

extern void iokit_lock_port(ipc_port_t port);
extern void iokit_unlock_port(ipc_port_t port);

extern kern_return_t iokit_switch_object_port( ipc_port_t port, io_object_t obj, ipc_kobject_type_t type );

#ifndef MACH_KERNEL_PRIVATE
typedef struct ipc_kmsg * ipc_kmsg_t;
extern ipc_kmsg_t ipc_kmsg_alloc(size_t);
extern void ipc_kmsg_destroy(ipc_kmsg_t);
extern mach_msg_header_t * ipc_kmsg_msg_header(ipc_kmsg_t);
#endif                          

extern kern_return_t
uext_server(ipc_kmsg_t request, ipc_kmsg_t * preply);

extern kern_return_t
iokit_label_dext_task(task_t task);

  
                                                     
   

extern ppnum_t IOGetLastPageNumber(void);

extern kern_return_t IOMapPages(vm_map_t map, mach_vm_address_t va, mach_vm_address_t pa,
    mach_vm_size_t length, unsigned int mapFlags);

extern kern_return_t IOUnmapPages(vm_map_t map, mach_vm_address_t va, mach_vm_size_t length);

extern kern_return_t IOProtectCacheMode(vm_map_t map, mach_vm_address_t va,
    mach_vm_size_t length, unsigned int options);

extern unsigned int IODefaultCacheBits(addr64_t pa);

#ifdef __cplusplus
}                 
#endif                  

#endif                          

#endif                             
