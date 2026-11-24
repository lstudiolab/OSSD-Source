/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
 *
 * @Apple_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/proc_internal.h>
#include <sys/stackshot.h>
#include <sys/sysproto.h>

  
                         
   

#if CONFIG_TELEMETRY
extern kern_return_t stack_microstackshot(user_addr_t tracebuf, uint32_t tracebuf_size, uint32_t flags, int32_t *retval);
#endif                       
extern kern_return_t kern_stack_snapshot_with_reason(char* reason);
extern kern_return_t kern_stack_snapshot_internal(int stackshot_config_version, void *stackshot_config, size_t stackshot_config_size, boolean_t stackshot_from_user);

static int
stackshot_kern_return_to_bsd_error(kern_return_t kr)
{
	switch (kr) {
	case KERN_SUCCESS:
		return 0;
	case KERN_RESOURCE_SHORTAGE:
		                                                                  
                          
		return ENOMEM;
	case KERN_INSUFFICIENT_BUFFER_SIZE:
	case KERN_NO_SPACE:
		                                                                 
                                                                  
                                   
		return ENOSPC;
	case KERN_NO_ACCESS:
		return EPERM;
	case KERN_MEMORY_PRESENT:
		return EEXIST;
	case KERN_NOT_SUPPORTED:
		return ENOTSUP;
	case KERN_NOT_IN_SET:
		                                                     
		return ENOENT;
	case KERN_ABORTED:
		                                                                    
		return EINTR;
	case KERN_FAILURE:
		                                                                    
		return EBUSY;
	case KERN_OPERATION_TIMED_OUT:
		                                        
		return ETIMEDOUT;
	default:
		return EINVAL;
	}
}

  
                                                                                                         
                                                                                      
                                                                       
  
                                                                                                                    
                                                             
                                                                           
                                                             
                                                              
                                           
                                                                                                 
                                                                                             
                                                                                        
                               
                                                                                   
                                                                                          
                                         
                                     
   
int
stack_snapshot_with_config(struct proc *p, struct stack_snapshot_with_config_args *uap, __unused int *retval)
{
	int error = 0;
	kern_return_t kr;

	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
		return error;
	}

	if ((void*)uap->stackshot_config == NULL) {
		return EINVAL;
	}

	switch (uap->stackshot_config_version) {
	case STACKSHOT_CONFIG_TYPE:
		if (uap->stackshot_config_size != sizeof(stackshot_config_t)) {
			return EINVAL;
		}
		stackshot_config_t config;
		error = copyin(uap->stackshot_config, &config, sizeof(stackshot_config_t));
		if (error != KERN_SUCCESS) {
			return EFAULT;
		}
		kr = kern_stack_snapshot_internal(uap->stackshot_config_version, &config, sizeof(stackshot_config_t), TRUE);
		return stackshot_kern_return_to_bsd_error(kr);
	default:
		return ENOTSUP;
	}
}

#if CONFIG_TELEMETRY
  
                                                                                         
                                                                        
                                                                      
                                                                             
           
                                                             
                               
                                                  
                                                                                                             
                                                                 
                                                               
                      
   
int
microstackshot(struct proc *p, struct microstackshot_args *uap, int32_t *retval)
{
	int error = 0;
	kern_return_t kr;

	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
		return error;
	}

	kr = stack_microstackshot(uap->tracebuf, uap->tracebuf_size, uap->flags, retval);
	return stackshot_kern_return_to_bsd_error(kr);
}
#endif                       

  
                                                                                                              
                                                                                       
                                                    
  
                                                                                                                        
                                              
                                                                      
                                                                          
                                                                                    
                                          
                                      
   
int
kern_stack_snapshot_with_reason(__unused char *reason)
{
	stackshot_config_t config;
	kern_return_t kr;

	config.sc_pid = -1;
	config.sc_flags = (STACKSHOT_SAVE_LOADINFO | STACKSHOT_GET_GLOBAL_MEM_STATS | STACKSHOT_SAVE_IN_KERNEL_BUFFER |
	    STACKSHOT_KCDATA_FORMAT | STACKSHOT_ENABLE_UUID_FAULTING | STACKSHOT_ENABLE_BT_FAULTING | STACKSHOT_THREAD_WAITINFO |
	    STACKSHOT_NO_IO_STATS | STACKSHOT_COLLECT_SHAREDCACHE_LAYOUT);
	config.sc_delta_timestamp = 0;
	config.sc_out_buffer_addr = 0;
	config.sc_out_size_addr = 0;

	kr = kern_stack_snapshot_internal(STACKSHOT_CONFIG_TYPE, &config, sizeof(stackshot_config_t), FALSE);
	return stackshot_kern_return_to_bsd_error(kr);
}
