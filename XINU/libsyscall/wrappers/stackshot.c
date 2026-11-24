/*
 * Copyright (c) 2014 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */
#include <sys/stackshot.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

  
                          
   
int __stack_snapshot_with_config(int stackshot_config_version, user_addr_t stackshot_config, size_t stackshot_config_size);

  
                                                                               
  
                                  
                                                      
   
stackshot_config_t *
stackshot_config_create(void)
{
	stackshot_config_t *s_config;

	s_config = malloc(sizeof(stackshot_config_t));
	if (s_config == NULL) {
		return NULL;
	}

	s_config->sc_pid = -1;
	s_config->sc_flags = 0;
	s_config->sc_delta_timestamp = 0;
	s_config->sc_buffer = 0;
	s_config->sc_size = 0;
	s_config->sc_pagetable_mask = 0;

	return s_config;
}

  
                                                     
  
                                                                                     
                                                                           
  
                                                                   
                  
   
int
stackshot_config_set_pid(stackshot_config_t *stackshot_config, int pid)
{
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return EINVAL;
	}

	s_config = (stackshot_config_t *) stackshot_config;
	s_config->sc_pid = pid;

	return 0;
}

  
                                                                           
  
                                                                                     
                                        
  
                                                                   
                  
   
int
stackshot_config_set_flags(stackshot_config_t *stackshot_config, uint64_t flags)
{
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return EINVAL;
	}

	s_config = (stackshot_config_t *) stackshot_config;
	s_config->sc_flags = flags;

	return 0;
}

  
                                                                           
  
                                                                                   
  
                                                                                        
                                                                          
                                  
                                                                                 
                                            
                                                                              
                                                                       
                                                        
                                                        
                                                                                
                                      
                                                                               
                                     
                   
   
int
stackshot_capture_with_config(stackshot_config_t *stackshot_config)
{
	int ret;
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return EINVAL;
	}

	s_config = (stackshot_config_t *) stackshot_config;
	if (s_config->sc_buffer != 0) {
		return EINVAL;
	}

	s_config->sc_out_buffer_addr = (uintptr_t)&s_config->sc_buffer;
	s_config->sc_out_size_addr = (uintptr_t)&s_config->sc_size;
	ret = __stack_snapshot_with_config(STACKSHOT_CONFIG_TYPE, (uintptr_t)s_config, sizeof(stackshot_config_t));

	if (ret != 0) {
		ret = errno;
		s_config->sc_buffer = 0;
		s_config->sc_size = 0;
	}

	return ret;
}

  
                                                                                              
  
                                                                   
  
                                                                                    
                                                                   
   
void *
stackshot_config_get_stackshot_buffer(stackshot_config_t *stackshot_config)
{
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return NULL;
	}
	s_config = (stackshot_config_t *) stackshot_config;

	return (void *)s_config->sc_buffer;
}

  
                                                                            
  
                                                                
  
                                                                           
                                                            
   
uint32_t
stackshot_config_get_stackshot_size(stackshot_config_t * stackshot_config)
{
	if (stackshot_config == NULL || (void *)stackshot_config->sc_buffer == NULL) {
		return -1;
	}

	return stackshot_config->sc_size;
}

  
                                                                       
  
                                                                
                                                                  
  
                                                                                                  
                                                              
   
int
stackshot_config_set_size_hint(stackshot_config_t *stackshot_config, uint32_t suggested_size)
{
	if (stackshot_config == NULL || (void *)stackshot_config->sc_buffer != NULL) {
		return -1;
	}

	stackshot_config->sc_size = suggested_size;

	return 0;
}

  
                                                                                                      
  
                                                                                                           
  
                                                                
                                                                                                               
  
                                                                                                  
                         
   
int
stackshot_config_set_delta_timestamp(stackshot_config_t *stackshot_config, uint64_t delta_timestamp)
{
	if (stackshot_config == NULL || (void *)stackshot_config->sc_buffer != NULL) {
		return -1;
	}

	stackshot_config->sc_delta_timestamp = delta_timestamp;

	return 0;
}

  
                                                                                
  
                                                                             
                                                                            
                                                                            
                        
  
                                                                             
                                                                      
  
                                                                
                                                                     
  
                                                                                                  
                         
   
int
stackshot_config_set_pagetable_mask(stackshot_config_t *stackshot_config, uint32_t pagetable_mask)
{
	if (stackshot_config == NULL || (void *)stackshot_config->sc_buffer != NULL) {
		return -1;
	}

	stackshot_config->sc_pagetable_mask = pagetable_mask;

	return 0;
}


  
                                                                                              
                                         
  
                                                                 
  
                                                                                   
                        
   
int
stackshot_config_dealloc_buffer(stackshot_config_t *stackshot_config)
{
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return EINVAL;
	}
	s_config = (stackshot_config_t *) stackshot_config;

	if (s_config->sc_size && s_config->sc_buffer) {
		mach_vm_deallocate(mach_task_self(), (mach_vm_offset_t)s_config->sc_buffer, (mach_vm_size_t)s_config->sc_size);
	}

	s_config->sc_buffer = 0;
	s_config->sc_size = 0;

	return 0;
}

  
                                                                                  
  
                                                                 
  
                                                           
                 
   
int
stackshot_config_dealloc(stackshot_config_t *stackshot_config)
{
	stackshot_config_t *s_config;

	if (stackshot_config == NULL) {
		return EINVAL;
	}
	s_config = (stackshot_config_t *) stackshot_config;

	if (s_config->sc_size && s_config->sc_buffer) {
		mach_vm_deallocate(mach_task_self(), (mach_vm_offset_t)s_config->sc_buffer, (mach_vm_size_t)s_config->sc_size);
	}

	s_config->sc_buffer = 0;
	s_config->sc_size = 0;

	free(s_config);
	return 0;
}
