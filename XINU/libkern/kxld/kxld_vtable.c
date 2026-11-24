/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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
#include <string.h>
#include <mach-o/loader.h>
#include <sys/types.h>

#if KERNEL
    #ifdef MACH_ASSERT
	#undef MACH_ASSERT
    #endif
    #define MACH_ASSERT 1
    #include <kern/assert.h>
#else
    #include <assert.h>
#endif

#define DEBUG_ASSERT_COMPONENT_NAME_STRING "kxld"
#include <AssertMacros.h>

#include "kxld_demangle.h"
#include "kxld_dict.h"
#include "kxld_object.h"
#include "kxld_reloc.h"
#include "kxld_sect.h"
#include "kxld_sym.h"
#include "kxld_symtab.h"
#include "kxld_util.h"
#include "kxld_vtable.h"

#define VTABLE_ENTRY_SIZE_32 4
#define VTABLE_HEADER_LEN_32 2
#define VTABLE_HEADER_SIZE_32 (VTABLE_HEADER_LEN_32 * VTABLE_ENTRY_SIZE_32)

#define VTABLE_ENTRY_SIZE_64 8
#define VTABLE_HEADER_LEN_64 2
#define VTABLE_HEADER_SIZE_64 (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

static void  get_vtable_base_sizes(boolean_t is_32_bit, u_int *vtable_entry_size,
    u_int *vtable_header_size);

static kern_return_t init_by_relocs(KXLDVTable *vtable, const KXLDSym *vtable_sym,
    const KXLDSect *sect, const KXLDRelocator *relocator);

static kern_return_t init_by_entries_and_relocs(KXLDVTable *vtable,
    const KXLDSym *vtable_sym, const KXLDRelocator *relocator,
    const KXLDArray *relocs, const KXLDDict *defined_cxx_symbols);

static kern_return_t init_by_entries(KXLDVTable *vtable,
    const KXLDRelocator *relocator, const KXLDDict *defined_cxx_symbols);

                                                                                
                                                                                
kern_return_t
kxld_vtable_init(KXLDVTable *vtable, const KXLDSym *vtable_sym,
    const KXLDObject *object, const KXLDDict *defined_cxx_symbols)
{
	kern_return_t rval = KERN_FAILURE;
	const KXLDArray *extrelocs = NULL;
	const KXLDRelocator *relocator = NULL;
	const KXLDSect *vtable_sect = NULL;
	char *demangled_name = NULL;
	size_t demangled_length = 0;

	check(vtable);
	check(vtable_sym);
	check(object);

	relocator = kxld_object_get_relocator(object);

	vtable_sect = kxld_object_get_section_by_index(object,
	    vtable_sym->sectnum);
	require_action(vtable_sect, finish, rval = KERN_FAILURE);

	vtable->name = vtable_sym->name;
	vtable->vtable = vtable_sect->data +
	    kxld_sym_get_section_offset(vtable_sym, vtable_sect);

	if (kxld_object_is_linked(object)) {
		rval = init_by_entries(vtable, relocator, defined_cxx_symbols);
		require_noerr(rval, finish);

		vtable->is_patched = TRUE;
	} else {
		if (kxld_object_is_final_image(object)) {
			extrelocs = kxld_object_get_extrelocs(object);

			require_action(extrelocs, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogPatching, kKxldLogErr,
			    kKxldLogMalformedVTable,
			    kxld_demangle(vtable->name,
			    &demangled_name, &demangled_length)));

			rval = init_by_entries_and_relocs(vtable, vtable_sym,
			    relocator, extrelocs, defined_cxx_symbols);
			require_noerr(rval, finish);
		} else {
			require_action(kxld_sect_get_num_relocs(vtable_sect) > 0, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogPatching, kKxldLogErr,
			    kKxldLogMalformedVTable,
			    kxld_demangle(vtable->name,
			    &demangled_name, &demangled_length)));

			rval = init_by_relocs(vtable, vtable_sym, vtable_sect, relocator);
			require_noerr(rval, finish);
		}

		vtable->is_patched = FALSE;
	}

	rval = KERN_SUCCESS;
finish:

	if (demangled_name) {
		kxld_free(demangled_name, demangled_length);
	}

	return rval;
}

                                                                                
                                                                                
static void
get_vtable_base_sizes(boolean_t is_32_bit, u_int *vtable_entry_size,
    u_int *vtable_header_size)
{
	check(vtable_entry_size);
	check(vtable_header_size);

	if (is_32_bit) {
		*vtable_entry_size = VTABLE_ENTRY_SIZE_32;
		*vtable_header_size = VTABLE_HEADER_SIZE_32;
	} else {
		*vtable_entry_size = VTABLE_ENTRY_SIZE_64;
		*vtable_header_size = VTABLE_HEADER_SIZE_64;
	}
}

                                                                                
                                                                               
                                                
                                                                                
static kern_return_t
init_by_relocs(KXLDVTable *vtable, const KXLDSym *vtable_sym,
    const KXLDSect *sect, const KXLDRelocator *relocator)
{
	kern_return_t rval = KERN_FAILURE;
	KXLDReloc *reloc = NULL;
	KXLDVTableEntry *entry = NULL;
	KXLDSym *sym = NULL;
	kxld_addr_t vtable_base_offset = 0;
	kxld_addr_t entry_offset = 0;
	u_int i = 0;
	u_int nentries = 0;
	u_int vtable_entry_size = 0;
	u_int vtable_header_size = 0;
	u_int base_reloc_index = 0;
	u_int reloc_index = 0;

	check(vtable);
	check(vtable_sym);
	check(sect);
	check(relocator);

	                                                  

	(void) get_vtable_base_sizes(relocator->is_32_bit,
	    &vtable_entry_size, &vtable_header_size);

	vtable_base_offset = kxld_sym_get_section_offset(vtable_sym, sect) +
	    vtable_header_size;

	                                                          

	rval = kxld_reloc_get_reloc_index_by_offset(&sect->relocs,
	    vtable_base_offset, &base_reloc_index);
	require_noerr(rval, finish);

	                                                                           
                                                                        
                                                                         
                                                          
    

	reloc_index = base_reloc_index;
	entry_offset = vtable_base_offset;
	reloc = kxld_array_get_item(&sect->relocs, reloc_index);
	while (reloc->address == entry_offset) {
		++nentries;
		if (!reloc_index) {
			break;
		}

		--reloc_index;

		reloc = kxld_array_get_item(&sect->relocs, reloc_index);
		entry_offset += vtable_entry_size;
	}

	                               

	rval = kxld_array_init(&vtable->entries, sizeof(KXLDVTableEntry), nentries);
	require_noerr(rval, finish);

	                                            

	for (i = 0; i < vtable->entries.nitems; ++i) {
		reloc = kxld_array_get_item(&sect->relocs, base_reloc_index - i);
		entry = kxld_array_get_item(&vtable->entries, i);

		                                                               
                                                                     
                                                                     
                                                                     
                                               
     
		sym = kxld_reloc_get_symbol(relocator, reloc, sect->data);

		entry->unpatched.sym = sym;
		entry->unpatched.reloc = reloc;
	}

	rval = KERN_SUCCESS;
finish:
	return rval;
}

                                                                                
                                                                            
                                                                
                                                                                
static kern_return_t
init_by_entries(KXLDVTable *vtable, const KXLDRelocator *relocator,
    const KXLDDict *defined_cxx_symbols)
{
	kern_return_t rval = KERN_FAILURE;
	KXLDVTableEntry *tmpentry = NULL;
	KXLDSym *sym = NULL;
	kxld_addr_t entry_value = 0;
	u_long entry_offset;
	u_int vtable_entry_size = 0;
	u_int vtable_header_size = 0;
	u_int nentries = 0;
	u_int i = 0;

	check(vtable);
	check(relocator);

	(void) get_vtable_base_sizes(relocator->is_32_bit,
	    &vtable_entry_size, &vtable_header_size);

	                                                                 

	entry_offset = vtable_header_size;
	while (1) {
		entry_value = kxld_relocator_get_pointer_at_addr(relocator,
		    vtable->vtable, entry_offset);
		if (!entry_value) {
			break;
		}

		entry_offset += vtable_entry_size;
		++nentries;
	}

	                               

	rval = kxld_array_init(&vtable->entries, sizeof(KXLDVTableEntry), nentries);
	require_noerr(rval, finish);

	                                        

	for (i = 0, entry_offset = vtable_header_size;
	    i < vtable->entries.nitems;
	    ++i, entry_offset += vtable_entry_size) {
		entry_value = kxld_relocator_get_pointer_at_addr(relocator,
		    vtable->vtable, entry_offset);

		                                                                      
                                                                         
                                 
     
		tmpentry = kxld_array_get_item(&vtable->entries, i);
		sym = kxld_dict_find(defined_cxx_symbols, &entry_value);

		if (sym) {
			tmpentry->patched.name = sym->name;
			tmpentry->patched.addr = sym->link_addr;
		} else {
			tmpentry->patched.name = NULL;
			tmpentry->patched.addr = 0;
		}
	}

	rval = KERN_SUCCESS;
finish:
	return rval;
}

                                                                                
                                                                          
                                                                              
                                      
 
                                                                        
                                                                            
                                                                               
                   
                                                                                
static kern_return_t
init_by_entries_and_relocs(KXLDVTable *vtable, const KXLDSym *vtable_sym,
    const KXLDRelocator *relocator, const KXLDArray *relocs,
    const KXLDDict *defined_cxx_symbols)
{
	kern_return_t rval = KERN_FAILURE;
	KXLDReloc *reloc = NULL;
	KXLDVTableEntry *tmpentry = NULL;
	KXLDSym *sym = NULL;
	u_int vtable_entry_size = 0;
	u_int vtable_header_size = 0;
	kxld_addr_t entry_value = 0;
	u_long entry_offset = 0;
	u_int nentries = 0;
	u_int i = 0;
	char *demangled_name1 = NULL;
	size_t demangled_length1 = 0;

	check(vtable);
	check(vtable_sym);
	check(relocator);
	check(relocs);

	                                                                 

	(void) get_vtable_base_sizes(relocator->is_32_bit,
	    &vtable_entry_size, &vtable_header_size);

	                                                                   
                                                                       
                                                                        
                                                
    
	entry_offset = vtable_header_size;
	while (1) {
		entry_value = kxld_relocator_get_pointer_at_addr(relocator,
		    vtable->vtable, entry_offset);
		if (!entry_value) {
			reloc = kxld_reloc_get_reloc_by_offset(relocs,
			    vtable_sym->base_addr + entry_offset);
			if (!reloc) {
				break;
			}
		}

		++nentries;
		entry_offset += vtable_entry_size;
	}

	                               

	rval = kxld_array_init(&vtable->entries, sizeof(KXLDVTableEntry), nentries);
	require_noerr(rval, finish);

	                                            

	for (i = 0, entry_offset = vtable_header_size;
	    i < vtable->entries.nitems;
	    ++i, entry_offset += vtable_entry_size) {
		entry_value = kxld_relocator_get_pointer_at_addr(relocator,
		    vtable->vtable, entry_offset);

		                                                               
                                                                     
                                                                     
                                                                     
                                               
     
		if (entry_value) {
			reloc = NULL;
			sym = kxld_dict_find(defined_cxx_symbols, &entry_value);
		} else {
			reloc = kxld_reloc_get_reloc_by_offset(relocs,
			    vtable_sym->base_addr + entry_offset);

			require_action(reloc, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogPatching, kKxldLogErr,
			    kKxldLogMalformedVTable,
			    kxld_demangle(vtable->name, &demangled_name1,
			    &demangled_length1)));

			sym = kxld_reloc_get_symbol(relocator, reloc,            NULL);
		}

		tmpentry = kxld_array_get_item(&vtable->entries, i);
		tmpentry->unpatched.reloc = reloc;
		tmpentry->unpatched.sym = sym;
	}

	rval = KERN_SUCCESS;
finish:
	return rval;
}

                                                                                
                                                                                
void
kxld_vtable_clear(KXLDVTable *vtable)
{
	check(vtable);

	vtable->vtable = NULL;
	vtable->name = NULL;
	vtable->is_patched = FALSE;
	kxld_array_clear(&vtable->entries);
}

                                                                                
                                                                                
void
kxld_vtable_deinit(KXLDVTable *vtable)
{
	check(vtable);

	kxld_array_deinit(&vtable->entries);
	bzero(vtable, sizeof(*vtable));
}

                                                                                
                                                                                
KXLDVTableEntry *
kxld_vtable_get_entry_for_offset(const KXLDVTable *vtable, u_long offset,
    boolean_t is_32_bit)
{
	KXLDVTableEntry *rval = NULL;
	u_int vtable_entry_size = 0;
	u_int vtable_header_size = 0;
	u_int vtable_entry_idx = 0;

	(void) get_vtable_base_sizes(is_32_bit,
	    &vtable_entry_size, &vtable_header_size);

	if (offset % vtable_entry_size) {
		goto finish;
	}

	vtable_entry_idx = (u_int) ((offset - vtable_header_size) / vtable_entry_size);
	rval = kxld_array_get_item(&vtable->entries, vtable_entry_idx);
finish:
	return rval;
}

                                                                                
                                                                              
                                                                                
kern_return_t
kxld_vtable_patch(KXLDVTable *vtable, const KXLDVTable *super_vtable,
    KXLDObject *object)
{
	kern_return_t rval = KERN_FAILURE;
	const KXLDSymtab *symtab = NULL;
	const KXLDSym *sym = NULL;
	KXLDVTableEntry *child_entry = NULL;
	KXLDVTableEntry *parent_entry = NULL;
	u_int symindex = 0;
	u_int i = 0;
	char *demangled_name1 = NULL;
	char *demangled_name2 = NULL;
	char *demangled_name3 = NULL;
	size_t demangled_length1 = 0;
	size_t demangled_length2 = 0;
	size_t demangled_length3 = 0;
	boolean_t failure = FALSE;

	check(vtable);
	check(super_vtable);

	symtab = kxld_object_get_symtab(object);

	require_action(!vtable->is_patched, finish, rval = KERN_SUCCESS);
	require_action(super_vtable->is_patched, finish, rval = KERN_FAILURE);
	require_action(vtable->entries.nitems >= super_vtable->entries.nitems, finish,
	    rval = KERN_FAILURE;
	    kxld_log(kKxldLogPatching, kKxldLogErr, kKxldLogMalformedVTable,
	    kxld_demangle(vtable->name, &demangled_name1, &demangled_length1)));

	for (i = 0; i < super_vtable->entries.nitems; ++i) {
		child_entry = kxld_array_get_item(&vtable->entries, i);
		parent_entry = kxld_array_get_item(&super_vtable->entries, i);

		                                                                   
                                                                    
                  
     

		if (!child_entry->unpatched.sym) {
			continue;
		}

		                                                                  
                                                                         
                                                                       
                           
     

		if (!parent_entry->patched.name) {
			continue;
		}

		                                                       

		if (kxld_sym_is_defined_locally(child_entry->unpatched.sym)) {
			continue;
		}

		                                                            
                                                                    
                                                                  
                                                                  
                                                                   
     

		if (kxld_sym_is_pure_virtual(child_entry->unpatched.sym)) {
			continue;
		}

		                                                  

		if (streq(child_entry->unpatched.sym->name,
		    parent_entry->patched.name)) {
			continue;
		}

		                                                                      
                                                                      
                                             
     

		require_action(!kxld_sym_name_is_padslot(parent_entry->patched.name),
		    finish, rval = KERN_FAILURE;
		    kxld_log(kKxldLogPatching, kKxldLogErr,
		    kKxldLogParentOutOfDate,
		    kxld_demangle(super_vtable->name, &demangled_name1,
		    &demangled_length1),
		    kxld_demangle(vtable->name, &demangled_name2,
		    &demangled_length2)));

#if KXLD_USER_OR_STRICT_PATCHING
		                                                                      
                                                                   
                                                                        
                                                                     
                                                                     
                                                                     
                                                                    
                         
     

		if (kxld_object_target_supports_strict_patching(object) &&
		    !kxld_sym_is_defined(child_entry->unpatched.sym)) {
			char class_name[KXLD_MAX_NAME_LEN];
			char function_prefix[KXLD_MAX_NAME_LEN];
			u_long function_prefix_len = 0;

			rval = kxld_sym_get_class_name_from_vtable_name(vtable->name,
			    class_name, sizeof(class_name));
			require_noerr(rval, finish);

			function_prefix_len =
			    kxld_sym_get_function_prefix_from_class_name(class_name,
			    function_prefix, sizeof(function_prefix));
			require(function_prefix_len, finish);

			if (!strncmp(child_entry->unpatched.sym->name,
			    function_prefix, function_prefix_len)) {
				failure = TRUE;
				kxld_log(kKxldLogPatching, kKxldLogErr,
				    "The %s is unpatchable because its class declares the "
				    "method '%s' without providing an implementation.",
				    kxld_demangle(vtable->name,
				    &demangled_name1, &demangled_length1),
				    kxld_demangle(child_entry->unpatched.sym->name,
				    &demangled_name2, &demangled_length2));
				continue;
			}
		}
#endif                                   

		                                                                      
                                                                    
                                                                   
                                                                       
                                                                        
                                                                     
          
     

		sym = kxld_symtab_get_locally_defined_symbol_by_name(symtab,
		    parent_entry->patched.name);
		if (!sym) {
			rval = kxld_object_add_symbol(object, parent_entry->patched.name,
			    parent_entry->patched.addr, &sym);
			require_noerr(rval, finish);
		}
		require_action(sym, finish, rval = KERN_FAILURE);

		rval = kxld_symtab_get_sym_index(symtab, sym, &symindex);
		require_noerr(rval, finish);

		rval = kxld_reloc_update_symindex(child_entry->unpatched.reloc, symindex);
		require_noerr(rval, finish);

		kxld_log(kKxldLogPatching, kKxldLogDetail,
		    "In vtable '%s', patching '%s' with '%s'.",
		    kxld_demangle(vtable->name, &demangled_name1, &demangled_length1),
		    kxld_demangle(child_entry->unpatched.sym->name,
		    &demangled_name2, &demangled_length2),
		    kxld_demangle(sym->name, &demangled_name3, &demangled_length3));

		rval = kxld_object_patch_symbol(object, child_entry->unpatched.sym);
		require_noerr(rval, finish);

		child_entry->unpatched.sym = sym;

		  
                                                                         
                                                                        
                                                                     
                                                                         
                                                                      
             
     
		assert(kxld_sym_is_pure_virtual(sym) || !(sym->link_addr & 1));
	}

	require_action(!failure, finish, rval = KERN_FAILURE);

	                                                                    
                   
    

	for (i = 0; i < vtable->entries.nitems; ++i) {
		char *name;
		kxld_addr_t addr;

		child_entry = kxld_array_get_item(&vtable->entries, i);
		if (child_entry->unpatched.sym) {
			name = child_entry->unpatched.sym->name;
			addr = child_entry->unpatched.sym->link_addr;
		} else {
			name = NULL;
			addr = 0;
		}

		child_entry->patched.name = name;
		child_entry->patched.addr = addr;
	}

	vtable->is_patched = TRUE;
	rval = KERN_SUCCESS;

finish:
	if (demangled_name1) {
		kxld_free(demangled_name1, demangled_length1);
	}
	if (demangled_name2) {
		kxld_free(demangled_name2, demangled_length2);
	}
	if (demangled_name3) {
		kxld_free(demangled_name3, demangled_length3);
	}

	return rval;
}
