/*
 * Copyright (c) 2006-2008 Apple Inc. All rights reserved.
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
 *
 *
 * Copyright (c) 1999 John D. Polstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _SYS_LINKER_SET_H_
#define _SYS_LINKER_SET_H_

#include <sys/appleapiopts.h>
#if !defined(KERNEL) || defined(__APPLE_API_PRIVATE)

  
                                                                         
                                                                    
                                                                         
                                                                                
                                         
  
                                                                  
                                                                        
                                                      
   

#ifdef KERNEL
# include <mach-o/loader.h>
# include <libkern/kernel_mach_header.h>

# define MACH_HEADER_TYPE kernel_mach_header_t
# define GETSECTIONDATA_VARIANT getsectdatafromheader
# define SECTDATA_SIZE_TYPE unsigned long
# define MH_EXECUTE_HEADER &_mh_execute_header
# define IMAGE_SLIDE_CORRECT 0
#else
# include <mach-o/ldsyms.h>
# include <mach-o/getsect.h>
# include <mach-o/loader.h>
# include <mach-o/dyld.h>
# include <crt_externs.h>

# if __LP64__
#  define MACH_HEADER_TYPE struct mach_header_64
#  define GETSECTIONDATA_VARIANT getsectdatafromheader_64
#  define SECTDATA_SIZE_TYPE uint64_t
#  define MH_EXECUTE_HEADER _NSGetMachExecuteHeader()
# else
#  define MACH_HEADER_TYPE struct mach_header
#  define GETSECTIONDATA_VARIANT getsectdatafromheader
#  define SECTDATA_SIZE_TYPE uint32_t
#  define MH_EXECUTE_HEADER _NSGetMachExecuteHeader()
# endif
#endif

#if __LP64__
# define LINKER_SET_ENTRY_PACKED
# define LINKER_SET_SEGMENT __DATA_CONST
# define LINKER_SET_SEGMENT_CSTR "__DATA_CONST"
#else
# define LINKER_SET_ENTRY_PACKED __attribute__((packed))
# define LINKER_SET_SEGMENT __DATA
# define LINKER_SET_SEGMENT_CSTR "__DATA"
#endif

  
                                                           
  
                                                                        
                                  
  
                                                                                       
   

                                                               
struct linker_set_entry {
	void *ptr;
} LINKER_SET_ENTRY_PACKED;

#ifdef __LS_VA_STRINGIFY__
#  undef __LS_VA_STRINGIFY__
#endif
#ifdef __LS_VA_STRCONCAT__
#  undef __LS_VA_STRCONCAT__
#endif
#define __LS_VA_STRINGIFY(_x ...)        #_x
#define __LS_VA_STRCONCAT(_x, _y)        __LS_VA_STRINGIFY(_x,_y)
#define __LINKER_MAKE_SET(_set, _sym)                                   \
	                        const struct linker_set_entry           __set_##_set##_sym_##_sym               \
	__attribute__ ((section(__LS_VA_STRCONCAT(LINKER_SET_SEGMENT,_set)),used)) = { (void *)&_sym }
                                                                        
                                                               

  
                 
   
#define LINKER_SET_ENTRY(_set, _sym)    __LINKER_MAKE_SET(_set, _sym)

  
                         
   
#ifdef __APPLE_API_OBSOLETE
# define TEXT_SET(_set, _sym)   __LINKER_MAKE_SET(_set, _sym)
# define DATA_SET(_set, _sym)   __LINKER_MAKE_SET(_set, _sym)
# define BSS_SET(_set, _sym)    __LINKER_MAKE_SET(_set, _sym)
# define ABS_SET(_set, _sym)    __LINKER_MAKE_SET(_set, _sym)
# define SET_ENTRY(_set, _sym)  __LINKER_MAKE_SET(_set, _sym)
#endif                           

  
                           
  
                                                                  
                                                                 
           
  
                                                                  
                  
   

  
                    
  
                                                
                                                                             
                                                
                                                                             
                                                                             
                                                                            
                                                                           
                
                                                             
  
                                                                              
                                           
  
   

#define LINKER_SET_OBJECT_BEGIN(_object, _set)  __linker_set_object_begin(_object, _set)
#define LINKER_SET_OBJECT_LIMIT(_object, _set)  __linker_set_object_limit(_object, _set)

#define LINKER_SET_OBJECT_FOREACH(_object, _pvar, _cast, _set)          \
	for (_pvar = (_cast) LINKER_SET_OBJECT_BEGIN(_object, _set);    \
	     _pvar < (_cast) LINKER_SET_OBJECT_LIMIT(_object, _set);    \
	     _pvar++)

#define LINKER_SET_OBJECT_ITEM(_object, _cast, _set, _i)                \
	(((_cast)(LINKER_SET_OBJECT_BEGIN(_object, _set)))[_i])

#define LINKER_SET_FOREACH(_pvar, _cast, _set)                                  \
	LINKER_SET_OBJECT_FOREACH((MACH_HEADER_TYPE *)MH_EXECUTE_HEADER, _pvar, _cast, _set)

  
                  
  
                                                  
                                                            
                                                 
                                                          
   

static __inline intptr_t
__linker_get_slide(struct mach_header *_header)
{
#ifndef KERNEL
	  
          
   
                                                                         
                                                                    
    
	uint32_t i = 0;
	for (i = 0; i < _dyld_image_count(); i++) {
		const struct mach_header *hdr = _dyld_get_image_header(i);
		if (_header == hdr) {
			return _dyld_get_image_vmaddr_slide(i);
		}
	}
	return 0;
#else
	(void)_header;
	return 0;
#endif
}

static __inline void **
__linker_set_object_begin(MACH_HEADER_TYPE *_header, const char *_set)
__attribute__((__const__));
static __inline void **
__linker_set_object_begin(MACH_HEADER_TYPE *_header, const char *_set)
{
	char *_set_begin;
	SECTDATA_SIZE_TYPE _size;

	_set_begin = (char *)GETSECTIONDATA_VARIANT(_header, LINKER_SET_SEGMENT_CSTR, _set, &_size);
	_set_begin += __linker_get_slide((struct mach_header *)_header);
	return (void **)(uintptr_t)_set_begin;
}

static __inline void **
__linker_set_object_limit(MACH_HEADER_TYPE *_header, const char *_set)
__attribute__((__const__));
static __inline void **
__linker_set_object_limit(MACH_HEADER_TYPE *_header, const char *_set)
{
	char *_set_begin;
	SECTDATA_SIZE_TYPE _size;

	_set_begin = (char *)GETSECTIONDATA_VARIANT(_header, LINKER_SET_SEGMENT_CSTR, _set, &_size);
	_set_begin += __linker_get_slide((struct mach_header *)_header);

	return (void **) ((uintptr_t) _set_begin + _size);
}

#endif                                     

#endif                         
