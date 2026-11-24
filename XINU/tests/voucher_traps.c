  
                          
                                                                              
                                                                            
                                                                         
                         
  
                                                                                                       
  
                            
   

#include <stdint.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/mach_traps.h>

#include <atm/atm_types.h>

#include <darwintest.h>

T_GLOBAL_META(T_META_RUN_CONCURRENTLY(true));

static mach_port_t
get_user_data_port(mach_msg_type_number_t *size)
{
#define DATA "Hello World!"
	struct {
		mach_voucher_attr_recipe_data_t recipe;
		char data[sizeof(DATA)];
	} buf = {
		.recipe = {
			.key     = MACH_VOUCHER_ATTR_KEY_USER_DATA,
			.command = MACH_VOUCHER_ATTR_USER_DATA_STORE,
			.content_size = sizeof(DATA),
		},
		.data = DATA,
	};

	mach_port_t port = MACH_PORT_NULL;
	kern_return_t kr = host_create_mach_voucher(mach_host_self(),
	    (mach_voucher_attr_raw_recipe_array_t)&buf,
	    sizeof(buf), &port);
	T_ASSERT_MACH_SUCCESS(kr, "Create USER_DATA voucher: 0x%x",
	    (unsigned int)port);

	if (size) {
		*size = sizeof(buf);
	}
	return port;
}


T_DECL(voucher_extract_attr_recipe, "voucher_extract_attr_recipe")
{
	kern_return_t kr;
	mach_vm_size_t alloc_sz;
	mach_port_t port;
	mach_vm_address_t alloc_addr;
	mach_msg_type_number_t expected_size;

	                                                              
	alloc_sz = (mach_vm_size_t)round_page(MACH_VOUCHER_TRAP_STACK_LIMIT + 1);

	  
                                                                     
                                                                   
                                              
    
	alloc_addr = (mach_vm_address_t)round_page(MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE + 1);
	kr = mach_vm_allocate(mach_task_self(), &alloc_addr,
	    alloc_sz, VM_FLAGS_ANYWHERE);

	  
                                                                   
                                                                     
                              
    
	T_ASSERT_GT_ULLONG((uint64_t)alloc_addr,
	    (uint64_t)MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE,
	    "Recipe addr (%llu bytes): 0x%llx > max recipe sz: %llu",
	    (uint64_t)alloc_sz, (uint64_t)alloc_addr,
	    (uint64_t)MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE);

	                                                       
	mach_msg_type_number_t *recipe_size;
	recipe_size = (mach_msg_type_number_t *)((uintptr_t)alloc_addr);
	bzero(recipe_size, (unsigned long)alloc_sz);
	if (alloc_sz > MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE) {
		*recipe_size = MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE;
	} else {
		*recipe_size = (mach_msg_type_number_t)alloc_sz;
	}

	                                                                   
	size_t size = (size_t)(10 * 1024 * 1024);
	void *recipe = malloc(size);
	memset(recipe, 0x41, size);

	port = get_user_data_port(&expected_size);

	  
                                                                            
                                       
    
	kr = mach_voucher_extract_attr_recipe_trap(port,
	    MACH_VOUCHER_ATTR_KEY_USER_DATA, recipe, recipe_size);
	T_ASSERT_MACH_SUCCESS(kr, "Extract attribute data with recipe: heap");
	T_ASSERT_EQ(*recipe_size, expected_size, "size should match");

	                             
	memset(recipe, 0x41, size);
	                                                              
	*recipe_size = MACH_VOUCHER_TRAP_STACK_LIMIT - 1;

	  
                                                                            
                 
    
	kr = mach_voucher_extract_attr_recipe_trap(port,
	    MACH_VOUCHER_ATTR_KEY_USER_DATA, recipe, recipe_size);
	T_ASSERT_MACH_SUCCESS(kr, "Extract attribute data with recipe: stack");
	T_ASSERT_EQ(*recipe_size, expected_size, "size should match");

	             

	free(recipe);
	kr = mach_vm_deallocate(mach_task_self(), alloc_addr, alloc_sz);
	T_ASSERT_MACH_SUCCESS(kr, "Deallocate recipe buffers");
}
