/*
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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
 */

#include "exc_helpers.h"

#include <darwintest.h>
#include <ptrauth.h>

#if __arm64__
#define EXCEPTION_THREAD_STATE          ARM_THREAD_STATE64
#define EXCEPTION_THREAD_STATE_COUNT    ARM_THREAD_STATE64_COUNT
#elif __arm__
#define EXCEPTION_THREAD_STATE          ARM_THREAD_STATE
#define EXCEPTION_THREAD_STATE_COUNT    ARM_THREAD_STATE_COUNT
#elif __x86_64__
#define EXCEPTION_THREAD_STATE          x86_THREAD_STATE
#define EXCEPTION_THREAD_STATE_COUNT    x86_THREAD_STATE_COUNT
#else
#error Unsupported architecture
#endif

   
                                                                          
                                                              
                                                              
   
extern boolean_t mach_exc_server(mach_msg_header_t *, mach_msg_header_t *);

extern kern_return_t
catch_mach_exception_raise(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count);

extern kern_return_t
catch_mach_exception_raise_state(
	mach_port_t exception_port,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count,
	int *flavor,
	thread_state_t in_state,
	mach_msg_type_number_t in_state_count,
	thread_state_t out_state,
	mach_msg_type_number_t *out_state_count);

extern kern_return_t
catch_mach_exception_raise_state_identity(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count,
	int *flavor,
	thread_state_t in_state,
	mach_msg_type_number_t in_state_count,
	thread_state_t out_state,
	mach_msg_type_number_t *out_state_count);

static exc_handler_callback_t exc_handler_callback;

   
                                                                
   
kern_return_t
catch_mach_exception_raise(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count)
{
#pragma unused(exception_port, thread, task, type, codes, code_count)
	T_FAIL("Triggered catch_mach_exception_raise() which shouldn't happen...");
	__builtin_unreachable();
}

   
                                                                
   
kern_return_t
catch_mach_exception_raise_state(
	mach_port_t exception_port,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count,
	int *flavor,
	thread_state_t in_state,
	mach_msg_type_number_t in_state_count,
	thread_state_t out_state,
	mach_msg_type_number_t *out_state_count)
{
#pragma unused(exception_port, type, codes, code_count, flavor, in_state, in_state_count, out_state, out_state_count)
	T_FAIL("Triggered catch_mach_exception_raise_state() which shouldn't happen...");
	__builtin_unreachable();
}

   
                                                                          
                                                         
                                                    
   
kern_return_t
catch_mach_exception_raise_state_identity(
	mach_port_t exception_port __unused,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t type,
	exception_data_t codes,
	mach_msg_type_number_t code_count,
	int *flavor,
	thread_state_t in_state,
	mach_msg_type_number_t in_state_count,
	thread_state_t out_state,
	mach_msg_type_number_t *out_state_count)
{
	T_LOG("Caught a mach exception!\n");

	                                           
	T_ASSERT_EQ(code_count, 2, "Two code values were provided with the mach exception");

	   
                                                                             
                                    
    
	mach_exception_data_t codes_64 = (mach_exception_data_t)(void *)codes;
	T_LOG("Mach exception codes[0]: %#llx, codes[1]: %#llx\n", codes_64[0], codes_64[1]);

	                                                                   
	T_ASSERT_EQ(*flavor, EXCEPTION_THREAD_STATE, "The thread state flavor is EXCEPTION_THREAD_STATE");
	T_ASSERT_EQ(in_state_count, EXCEPTION_THREAD_STATE_COUNT, "The thread state count is EXCEPTION_THREAD_STATE_COUNT");

	size_t advance_pc = exc_handler_callback(task, thread, type, codes_64);

	   
                                                                        
                                      
    
	*out_state_count = in_state_count;                                           
	memcpy((void*)out_state, (void*)in_state, in_state_count * 4);

#if __arm64__
	arm_thread_state64_t *state = (arm_thread_state64_t*)(void *)out_state;

	void *pc = (void*)(arm_thread_state64_get_pc(*state) + advance_pc);
	                                                                           
	pc = ptrauth_sign_unauthenticated(pc, ptrauth_key_function_pointer, 0);
	arm_thread_state64_set_pc_fptr(*state, pc);
#else
	(void)advance_pc;
	T_FAIL("catch_mach_exception_raise_state() not fully implemented on this architecture");
	__builtin_unreachable();
#endif

	                                                                               
	return KERN_SUCCESS;
}

mach_port_t
create_exception_port(exception_mask_t exception_mask)
{
	mach_port_t exc_port = MACH_PORT_NULL;
	mach_port_t task = mach_task_self();
	mach_port_t thread = mach_thread_self();
	kern_return_t kr = KERN_SUCCESS;

	                                                                  
	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &exc_port);
	T_ASSERT_MACH_SUCCESS(kr, "Allocated mach exception port");

	   
                                                                           
                                                     
    
	kr = mach_port_insert_right(task, exc_port, exc_port, MACH_MSG_TYPE_MAKE_SEND);
	T_ASSERT_MACH_SUCCESS(kr, "Inserted a SEND right into the exception port");

	                                                      
	kr = thread_set_exception_ports(
		thread,
		exception_mask,
		exc_port,
		(exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES),
		EXCEPTION_THREAD_STATE);
	T_ASSERT_MACH_SUCCESS(kr, "Set the exception port to my custom handler");

	return exc_port;
}

   
                                       
  
                                                          
   
static void *
exc_server_thread(void *arg)
{
	mach_port_t exc_port = (mach_port_t)arg;

	   
                                                                         
                                                                       
                                                                        
                                                                 
    
#define MACH_MSG_REPLY_SIZE 4096
	kern_return_t kr = mach_msg_server_once(mach_exc_server, MACH_MSG_REPLY_SIZE, exc_port, 0);
	T_ASSERT_MACH_SUCCESS(kr, "Received mach exception message");

	pthread_exit((void*)0);
	__builtin_unreachable();
}

void
run_exception_handler(mach_port_t exc_port, exc_handler_callback_t callback)
{
	exc_handler_callback = callback;

	pthread_t exc_thread;

	                                          
	int err = pthread_create(&exc_thread, (pthread_attr_t*)0, exc_server_thread, (void*)(uintptr_t)exc_port);
	T_ASSERT_POSIX_ZERO(err, "Spawned exception server thread");

	                                                                          
	pthread_detach(exc_thread);
}
