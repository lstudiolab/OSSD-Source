  
                                                                          
                                                                         
                  
  
                                                                            
  

#include <darwintest.h>
#include "abi_helper.h"

                                                                              
T_DECL(abi_caller_raw, "intrusive_shared_ptr.abi.caller.raw") {
	T obj{10};
	T* expected = &obj;
	T* result = return_shared_as_raw(expected);
	CHECK(result == expected);

	                                                                     
	                                                                       
	                                                                    
	                                                  
	CHECK(result->i == 10);
}
