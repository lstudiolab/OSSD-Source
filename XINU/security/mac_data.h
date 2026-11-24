/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 2006 SPARTA, Inc.
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
 */

#ifndef _SECURITY_MAC_DATA_H_
#define _SECURITY_MAC_DATA_H_

#ifndef PRIVATE
#warning "MAC policy is not KPI, see Technical Q&A QA1574, this header will be removed in next version"
#endif

   
                                 
  
                                                                 
                                                                  
                                                                
                                                                     
                                                                       
                                                                       
                                                                           
          
  
                                                           
                                                           
  
        
  
                           
          
                       
                             
                         
                
               
                                
                                       
                                
                                              
                
               
                                
                                     
                                
                                                        
                
                 
           
  
   
struct mac_module_data_element {
	unsigned int key_size;
	unsigned int value_size;
	unsigned int value_type;
	char *key;
	char *value;
};
struct mac_module_data_list {
	unsigned int count;
	unsigned int type;
	struct mac_module_data_element list[1];
};
struct mac_module_data {
	void *base_addr;                                                        
	unsigned int size;
	unsigned int count;
	struct mac_module_data_element data[1];                      
};

#define MAC_DATA_TYPE_PRIMITIVE 0                                               
#define MAC_DATA_TYPE_ARRAY     1                                               
#define MAC_DATA_TYPE_DICT      2                                               

#ifdef _SECURITY_MAC_POLICY_H_
                                                                               
int mac_find_policy_data(const mac_policy_handle_t, const char *key,
    void **valp, size_t *sizep);
int mac_find_module_data(struct mac_module_data *mmd, const char *key,
    void **valp, size_t *sizep);

  
                                                                             
                                                                             
                                   
   
static __inline void
mmd_fixup_ele(struct mac_module_data *oldbase,
    struct mac_module_data *newbase, struct mac_module_data_element *ele)
{
	if (ele->key != NULL) {                                                 
		ele->key -= (uintptr_t)oldbase;
		ele->key += (uintptr_t)newbase;
	}
	ele->value -= (uintptr_t)oldbase;
	ele->value += (uintptr_t)newbase;
}

#endif

#endif                             
