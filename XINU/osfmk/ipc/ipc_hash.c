/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                       
                      
             
  
                               
   

#include <mach/boolean.h>
#include <mach/port.h>
#include <ipc/port.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_hash.h>
#include <ipc/ipc_init.h>
#include <os/hash.h>

#include <mach_ipc_debug.h>

#if     MACH_IPC_DEBUG
#include <mach/kern_return.h>
#include <mach_debug/hash_info.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#endif                      

  
                       
   

                                                       
void ipc_hash_local_delete(
	ipc_space_t             space,
	ipc_object_t            obj,
	mach_port_index_t       index,
	ipc_entry_t             entry);

  
                           
           
                                           
                                       
              
                                                        
   

boolean_t
ipc_hash_lookup(
	ipc_space_t             space,
	ipc_object_t            obj,
	mach_port_name_t        *namep,
	ipc_entry_t             *entryp)
{
	return ipc_hash_table_lookup(space->is_table, space->is_table_size, obj, namep, entryp);
}

  
                           
           
                                                             
                                         
              
                                   
   

void
ipc_hash_insert(
	ipc_space_t             space,
	ipc_object_t            obj,
	mach_port_name_t        name,
	ipc_entry_t             entry)
{
	mach_port_index_t index;

	index = MACH_PORT_INDEX(name);
	space->is_table_hashed++;
	ipc_hash_table_insert(space->is_table, space->is_table_size, obj, index, entry);
}

  
                           
           
                                                             
              
                                   
   

void
ipc_hash_delete(
	ipc_space_t             space,
	ipc_object_t            obj,
	mach_port_name_t        name,
	ipc_entry_t             entry)
{
	mach_port_index_t index;

	index = MACH_PORT_INDEX(name);
	space->is_table_hashed--;
	ipc_hash_table_delete(space->is_table, space->is_table_size, obj, index, entry);
}

  
                                                         
                                                           
                                                    
  
                                                         
                                                       
                                                          
                                                             
                                                                    
                                                                       
                                                             
  
                                                                      
                                                                   
                                                                 
                                                                 
                                                                  
                                                          
                                                                   
                                                               
  
                                                                      
                                                                   
                                                                    
                                                              
                                                                         
                                                         
   

#define IH_TABLE_HASH(obj, size)                                \
	        ((mach_port_index_t)(os_hash_kernel_pointer(obj) % (size)))

  
                                 
           
                                           
              
                                            
   

boolean_t
ipc_hash_table_lookup(
	ipc_entry_t             table,
	ipc_entry_num_t         size,
	ipc_object_t            obj,
	mach_port_name_t        *namep,
	ipc_entry_t             *entryp)
{
	mach_port_index_t hindex, index, hdist;

	if (obj == IO_NULL) {
		return FALSE;
	}

	hindex = IH_TABLE_HASH(obj, size);
	hdist  = 0;

	  
                                                        
                                                 
                                                           
                                      
    

	while ((index = table[hindex].ie_index) != 0) {
		ipc_entry_t entry = &table[index];

		  
                                                   
                                                    
                                                      
     
		if (hdist > table[hindex].ie_dist) {
			return FALSE;
		}

		  
                                                       
                                                            
     
		if (hdist == table[hindex].ie_dist) {
			assert(index < size);
			if (entry->ie_object == obj) {
				*entryp = entry;
				*namep = MACH_PORT_MAKE(index,
				    IE_BITS_GEN(entry->ie_bits));
				return TRUE;
			}
		} else {
			assert(entry->ie_object != obj);
		}

		if (hdist < IPC_ENTRY_DIST_MAX) {
			                                                         
			++hdist;
		}
		if (++hindex == size) {
			hindex = 0;
		}
	}

	return FALSE;
}

  
                                 
           
                                                         
              
                                   
   

void
ipc_hash_table_insert(
	ipc_entry_t                     table,
	ipc_entry_num_t                 size,
	ipc_object_t                    obj,
	mach_port_index_t               index,
	__assert_only ipc_entry_t       entry)
{
	mach_port_index_t hindex, hdist;

	assert(index != 0);
	assert(obj != IO_NULL);

	hindex = IH_TABLE_HASH(obj, size);
	hdist  = 0;

	assert(entry == &table[index]);
	assert(entry->ie_object == obj);

	  
                                                             
                                                          
                     
   
                                                          
                                                         
                                                 
                                    
    
	while (table[hindex].ie_index != 0) {
		if (table[hindex].ie_dist < hdist) {
#define swap(a, b)  ({ typeof(a) _tmp = (b); (b) = (a); (a) = _tmp; })
			swap(hdist, table[hindex].ie_dist);
			swap(index, table[hindex].ie_index);
#undef swap
		}
		if (hdist < IPC_ENTRY_DIST_MAX) {
			                                                         
			++hdist;
		}
		if (++hindex == size) {
			hindex = 0;
		}
	}

	table[hindex].ie_index = index;
	table[hindex].ie_dist = hdist;
}

  
                                 
           
                                                   
              
                                  
   

void
ipc_hash_table_delete(
	ipc_entry_t                     table,
	ipc_entry_num_t                 size,
	ipc_object_t                    obj,
	mach_port_index_t               index,
	__assert_only ipc_entry_t       entry)
{
	mach_port_index_t hindex, dindex, dist;

	assert(index != MACH_PORT_NULL);
	assert(obj != IO_NULL);

	hindex = IH_TABLE_HASH(obj, size);

	assert(entry == &table[index]);
	assert(entry->ie_object == obj);

	  
                                                        
                                                   
                        
    

	while (table[hindex].ie_index != index) {
		if (++hindex == size) {
			hindex = 0;
		}
	}

	  
                                                  
                                               
                                                    
                                                 
                                                    
                                    
   
                                                       
                                                           
   
                                            
                                                          
   
                                                     
                                                            
                                         
    

	for (;;) {
		dindex = hindex + 1;
		if (dindex == size) {
			dindex = 0;
		}

		  
                                                     
                                                     
                                                    
     
		index = table[dindex].ie_index;
		dist  = table[dindex].ie_dist;
		if (index == 0 || dist == 0) {
			table[hindex].ie_index = 0;
			table[hindex].ie_dist = 0;
			return;
		}

		  
                                                                   
                                                  
     
		if (dist-- == IPC_ENTRY_DIST_MAX) {
			uint32_t desired = IH_TABLE_HASH(table[index].ie_object, size);
			if (hindex >= desired) {
				dist = hindex - desired;
			} else {
				dist = hindex + size - desired;
			}
			if (dist > IPC_ENTRY_DIST_MAX) {
				dist = IPC_ENTRY_DIST_MAX;
			}
		}

		  
                                                           
                                     
     
		table[hindex].ie_index = index;
		table[hindex].ie_dist = dist;
		hindex = dindex;
	}
}
