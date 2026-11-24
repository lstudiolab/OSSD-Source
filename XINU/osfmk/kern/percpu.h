/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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

#ifndef _KERN_PERCPU_H_
#define _KERN_PERCPU_H_

#include <mach/vm_types.h>

__BEGIN_DECLS

#if XNU_KERNEL_PRIVATE
#include <libkern/section_keywords.h>
#include <os/atomic_private.h>

#pragma GCC visibility push(hidden)

   
                     
  
            
                                           
  
                                                 
                                                 
   
#define PERCPU_DECL(type_t, name) \
	extern type_t __PERCPU_NAME(name)

   
                     
  
            
                                                    
  
              
                                                                           
  
                                                                              
                                                                         
                               
  
            
         
                                       
          
  
                                                 
   
#define PERCPU_DATA(name) \
	__percpu __PERCPU_NAME(name) = {0}

   
                    
  
            
                                                                               
                                
  
              
                                                                               
                                                                    
  
                                                                              
                                                                         
                                                            
  
                                                                          
                                                                     
                                                            
  
                                                 
   
#define PERCPU_GET(name) \
	__PERCPU_CAST(name, current_percpu_base() + __PERCPU_ADDR(name))

   
                                  
  
            
                                                                     
  
                                           
   
extern vm_offset_t current_percpu_base(void);

   
                           
  
            
                                                                 
  
                                               
                                                 
   
#define PERCPU_GET_MASTER(name) \
	(&__PERCPU_NAME(name))

   
                              
  
            
                                                                           
        
  
                                               
                                                 
   
#define PERCPU_GET_WITH_BASE(base, name) \
	__PERCPU_CAST(name, base + __PERCPU_ADDR(name))

   
                             
  
            
                                                                           
             
  
               
                                                                             
                                                                              
                                                                            
  
                                                 
                                                       
                                                            
   
#define PERCPU_GET_RELATIVE(name, other, ptr) ({ \
	__PERCPU_TYPE(other) __other_ptr = (ptr);                  \
	vm_offset_t __offs = __PERCPU_ADDR(name) - __PERCPU_ADDR(other); \
	__PERCPU_CAST(name, (vm_address_t)__other_ptr + __offs); \
})

   
                               
  
            
                                         
  
                                                
   
#define percpu_foreach_base(it) \
	for (vm_offset_t it = 0, \
	    __next_ ## it = percpu_base.start, \
	    __end_ ## it = percpu_base.end; \
        \
	    it <= __end_ ## it; \
        \
	    it = __next_ ## it, \
	    __next_ ## it += percpu_section_size())

   
                          
  
            
                                             
  
                                                
                                                 
   
#define percpu_foreach(it, name) \
	for (__PERCPU_TYPE(name) it, \
	    __base_ ## it = NULL, \
	    __next_ ## it = (typeof(it))percpu_base.start, \
	    __end_ ## it = (typeof(it))percpu_base.end; \
        \
	    (it = (typeof(it))(__PERCPU_ADDR(name) + (vm_address_t)__base_ ## it), \
	    __base_ ## it <= __end_ ## it); \
        \
	    __base_ ## it = __next_ ## it, \
	    __next_ ## it = (typeof(it))((vm_address_t)__base_ ## it + percpu_section_size()))

   
                                         
  
            
                                                                   
  
                                                
   
#define percpu_foreach_secondary_base(it) \
	for (vm_offset_t it = percpu_base.start, __end_ ## it = percpu_base.end; \
	    it <= __end_ ## it; it += percpu_section_size())

   
                                    
  
            
                                                                       
  
                                                
                                                 
   
#define percpu_foreach_secondary(it, name) \
	for (__PERCPU_TYPE(name) it, \
	    __base_ ## it = (typeof(it))percpu_base.start, \
	    __end_ ## it = (typeof(it))percpu_base.end; \
        \
	    (it = (typeof(it))(__PERCPU_ADDR(name) + (vm_address_t)__base_ ## it), \
	    __base_ ## it <= __end_ ## it); \
        \
	    __base_ ## it = (typeof(it))((vm_address_t)__base_ ## it + percpu_section_size()))

#pragma mark - implementation details

  
                                                                                
                                                             
   

#define __percpu                        __attribute__((section("__DATA, __percpu")))
#define __PERCPU_NAME(name)             percpu_slot_ ## name
#define __PERCPU_ADDR(name)             ((vm_offset_t)&__PERCPU_NAME(name))
#define __PERCPU_TYPE(name)             typeof(&__PERCPU_NAME(name))
#define __PERCPU_CAST(name, expr)       ((__PERCPU_TYPE(name))(expr))

  
                         
  
                                                                   
                                                                      
  
                                                             
                                                  
  
                                                                            
                                                                         
                                           
   
extern struct percpu_base {
	vm_address_t start;
	vm_address_t end;
	vm_offset_t  size;
} percpu_base;

static __pure2 inline vm_offset_t
percpu_section_start(void)
{
	extern char __percpu_section_start[] __SECTION_START_SYM("__DATA", "__percpu");
	return (vm_offset_t)__percpu_section_start;
}

static __pure2 inline vm_offset_t
percpu_section_end(void)
{
	extern char __percpu_section_end[] __SECTION_END_SYM("__DATA", "__percpu");
	return (vm_offset_t)__percpu_section_end;
}

static __pure2 inline vm_size_t
percpu_section_size(void)
{
	return percpu_section_end() - percpu_section_start();
}

#pragma GCC visibility pop
#endif                         

__END_DECLS

#endif                      
