  
                                                                            
                                                                           
              
  
                                                                            
              
  

#define USE_SHARED_PTR

#include <libkern/c++/intrusive_shared_ptr.h>
#include <darwintest.h>
#include "abi_helper.h"

static_assert(sizeof(SharedPtr<T>) == sizeof(T*));
static_assert(alignof(SharedPtr<T>) == alignof(T*));

                                                                               
T_DECL(abi_caller_smart, "intrusive_shared_ptr.abi.caller.smart") {
	T obj{3};
	T* expected = &obj;
	SharedPtr<T> result = return_raw_as_shared(expected);
	CHECK(result.get() == expected);

	                                                                     
	                                                                       
	                                                                         
	                                           
	CHECK(result->i == 3);
}
