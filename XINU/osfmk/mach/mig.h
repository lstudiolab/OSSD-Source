/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */

  
                                
   

#ifndef _MACH_MIG_H_
#define _MACH_MIG_H_

#include <stdint.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/vm_types.h>

#include <sys/cdefs.h>

#if defined(MACH_KERNEL)

#if !defined(__MigTypeCheck)
                                                     
#define __MigTypeCheck 1
#endif

#define __MigKernelSpecificCode 1
#define _MIG_KERNEL_SPECIFIC_CODE_ 1

#elif !defined(__MigTypeCheck)

#if defined(TypeCheck)
                                    
#define __MigTypeCheck TypeCheck
#else
                                  
#define __MigTypeCheck 1
#endif

#endif                                                        

  
                            
                                                               
                                                                       
   
#define __MigPackStructs 1

  
                                                                     
                                                                      
                 
   
typedef void    (*mig_stub_routine_t) (mach_msg_header_t *InHeadP,
    mach_msg_header_t *OutHeadP);

typedef mig_stub_routine_t mig_routine_t;

  
                                                                     
                                                                       
           
   
typedef mig_routine_t (*mig_server_routine_t) (mach_msg_header_t *InHeadP);

  
                                                                     
                                                                    
                                                        
   
typedef kern_return_t   (*mig_impl_routine_t)(void);

typedef mach_msg_type_descriptor_t routine_arg_descriptor;
typedef mach_msg_type_descriptor_t *routine_arg_descriptor_t;
typedef mach_msg_type_descriptor_t *mig_routine_arg_descriptor_t;

#define MIG_ROUTINE_ARG_DESCRIPTOR_NULL ((mig_routine_arg_descriptor_t)0)

struct routine_descriptor {
	mig_impl_routine_t      impl_routine;                                   
	mig_stub_routine_t      stub_routine;                                   
	unsigned int            argc;                                                   
	unsigned int            descr_count;                                    
	routine_arg_descriptor_t
	    arg_descr;                                                                                  
	unsigned int            max_reply_msg;                                  
};
typedef struct routine_descriptor *routine_descriptor_t;

typedef struct routine_descriptor mig_routine_descriptor;
typedef mig_routine_descriptor *mig_routine_descriptor_t;

#define MIG_ROUTINE_DESCRIPTOR_NULL ((mig_routine_descriptor_t)0)

typedef struct mig_subsystem {
	mig_server_routine_t server;                                          
	mach_msg_id_t            start;                                             
	mach_msg_id_t            end;                                                 
	mach_msg_size_t          maxsize;                                             
	vm_address_t             reserved;                                            
	mig_routine_descriptor
	    routine[1];                                                                       
} *mig_subsystem_t;

#define MIG_SUBSYSTEM_NULL              ((mig_subsystem_t)0)

typedef struct mig_symtab {
	char                            *ms_routine_name;
	int                                     ms_routine_number;
	void                            (*ms_routine)(void);                                 
                                                                                     
                                                                                        
                                                                        
                                                            
} mig_symtab_t;

  
                                                                        
                                                                          
                                           
   
#if __has_attribute(mig_server_routine)
#define MIG_SERVER_ROUTINE __attribute__((mig_server_routine))
#else
#define MIG_SERVER_ROUTINE
#endif

#ifdef  PRIVATE

                                                           

#ifdef  KERNEL_PRIVATE

  
                                 
  
                                                           
                                                                    
  
                    
                                         
                                                              
                                                              
                                                    
                                                                   
                                        
                                                 
                                 
               
                       
   
typedef unsigned int                    mig_notify_type_t;

typedef struct MIGIID {
	unsigned long                           data1;
	unsigned short                          data2;
	unsigned short                          data3;
	unsigned char                           data4[8];
} MIGIID;

typedef struct IMIGObjectVtbl                   IMIGObjectVtbl;
typedef struct IMIGNotifyObjectVtbl             IMIGNotifyObjectVtbl;

typedef struct IMIGObject {
	const IMIGObjectVtbl                    *pVtbl;
} IMIGObject;

typedef struct IMIGNotifyObject {
	const IMIGNotifyObjectVtbl              *pVtbl;
} IMIGNotifyObject;

struct IMIGObjectVtbl {
	kern_return_t (*QueryInterface)(
		IMIGObject              *object,
		const MIGIID            *iid,
		void                    **ppv);

	unsigned long (*AddRef)(
		IMIGObject              *object);

	unsigned long (*Release)(
		IMIGObject              *object);

	unsigned long (*GetServer)(
		IMIGObject              *object,
		mig_server_routine_t    *server);

	boolean_t (*RaiseNotification)(
		IMIGObject              *object,
		mig_notify_type_t       notify_type);

	boolean_t (*RequestNotification)(
		IMIGObject              *object,
		IMIGNotifyObject        *notify,
		mig_notify_type_t       notify_type);
};

  
                   
  
                                                           
                     
  
                                                                   
                                                                    
                                                               
                                                                 
            
  
                                                                
                                                              
                              
   
struct IMIGNotifyObjectVtbl {
	kern_return_t (*QueryInterface)(
		IMIGNotifyObject        *notify,
		const MIGIID            *iid,
		void                    **ppv);

	unsigned long (*AddRef)(
		IMIGNotifyObject        *notify);

	unsigned long (*Release)(
		IMIGNotifyObject        *notify);

	unsigned long (*GetServer)(
		IMIGNotifyObject        *notify,
		mig_server_routine_t    *server);

	boolean_t (*RaiseNotification)(
		IMIGNotifyObject        *notify,
		mig_notify_type_t       notify_type);

	boolean_t (*RequestNotification)(
		IMIGNotifyObject        *notify,
		IMIGNotifyObject        *notify_notify,
		mig_notify_type_t       notify_type);

	void (*HandleNotification)(
		IMIGNotifyObject        *notify,
		IMIGObject              *object,
		mig_notify_type_t       notify_type);
};

#endif                      
#endif               

__BEGIN_DECLS

                                     
extern mach_port_t mig_get_reply_port(void);

                                       
extern void mig_dealloc_reply_port(mach_port_t reply_port);

                                           
extern void mig_put_reply_port(mach_port_t reply_port);

                         
extern int mig_strncpy(char     *dest, const char *src, int     len);
extern int mig_strncpy_zerofill(char    *dest, const char *src, int     len);

#ifdef KERNEL_PRIVATE

                                                     
extern void *mig_user_allocate(vm_size_t size);

                                                            
extern void mig_user_deallocate(char *data, vm_size_t size);

#else

                                                    
extern void mig_allocate(vm_address_t *, vm_size_t);

                                                           
extern void mig_deallocate(vm_address_t, vm_size_t);

#endif                     

__END_DECLS

#endif                    
