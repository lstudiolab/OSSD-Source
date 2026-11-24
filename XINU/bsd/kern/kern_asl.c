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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/vnode_internal.h>
#include <sys/fslog.h>
#include <sys/mount_internal.h>
#include <sys/kasl.h>

#include <dev/random/randomdev.h>

#include <uuid/uuid.h>

#include <stdarg.h>

                                                                 
#define KASL_KEYVAL_FMT "[%s %s] "
#define KASL_KEYVAL_FMT_LEN     (sizeof(KASL_KEYVAL_FMT) - 1)

#define KASL_NEWLINE_CHAR       "\n"
#define KASL_NEWLINE_CHAR_LEN   (sizeof(KASL_NEWLINE_CHAR) - 1)

                                                                             
#define KASL_ASL_MSG_LEN        "         0"

                                                          
#define MAX_FMT_LEN             256


                                                              
                                                                       
                                                                      
                                                                         
                           
  
               
                                                       
                                             
                                                                    
                                       
  
            
                                                                   
                                                                    
                             
   
static int
kern_asl_msg_va(int level, const char *facility, size_t num_pairs, va_list vargs)
{
	int err = 0;
	char fmt[MAX_FMT_LEN];                                         
	size_t calc_pairs = 0;
	size_t len;

	                                                  
	level = LOG_PRI(level);

	                                                           
                                        
    
	if (facility) {
		snprintf(fmt, MAX_FMT_LEN, "%s [%s %d] [%s %s] ",
		    KASL_ASL_MSG_LEN,
		    KASL_KEY_LEVEL, level,
		    KASL_KEY_FACILITY, facility);
	} else {
		snprintf(fmt, MAX_FMT_LEN, "%s [%s %d] ",
		    KASL_ASL_MSG_LEN,
		    KASL_KEY_LEVEL, level);
	}

	                                                               
                                                                      
                                                                      
                                                                   
                                                                    
                                                        
    
	len = MAX_FMT_LEN - strlen(fmt) - KASL_NEWLINE_CHAR_LEN - 1;
	calc_pairs = len / KASL_KEYVAL_FMT_LEN;
	if (num_pairs <= calc_pairs) {
		calc_pairs = num_pairs;
	} else {
		err = E2BIG;
	}

	                                                                    
	len = MAX_FMT_LEN - KASL_NEWLINE_CHAR_LEN;
	for (size_t i = 0; i < calc_pairs; i++) {
		(void) strlcat(fmt, KASL_KEYVAL_FMT, len);
	}

	                    
	(void) strlcat(fmt, KASL_NEWLINE_CHAR, MAX_FMT_LEN);

	                                             
	vaddlog(fmt, vargs);

	  
                                                            
                                                          
                                                       
                            
    

	return err;
}

int
kern_asl_msg(int level, const char *facility, size_t num_pairs, ...)
{
	int err;
	va_list ap;

	va_start(ap, num_pairs);
	err = kern_asl_msg_va(level, facility,
	    num_pairs, ap);
	va_end(ap);

	return err;
}

                                                                     
                                                                        
                                             
  
               
                                                                           
                                                                   
                                                   
   
int
escape_str(char *str, size_t len, size_t buflen)
{
	size_t count;
	char *src, *dst;

	                                          
	src = str;
	count = 0;
	do {
		if ((*src == '[') || (*src == ']')) {
			count++;
		}
	} while (*src++);

	if (count) {
		  
                                                       
               
     
		if ((buflen - len) < count) {
			return ENOSPC;
		}

		src = str + len;
		dst = src + count;
		while (count) {
			*dst-- = *src;
			if ((*src == '[') || (*src == ']')) {
				                                          
				*dst-- = '\\';
				count--;
			}
			src--;
		}
	}

	return 0;
}
