  
                                                                     
                                                                        
                                                 
  
                                                                            
              
  

#define USE_SHARED_PTR

#include "abi_helper.h"

SharedPtr<T>
return_shared_as_raw(T* ptr)
{
	return SharedPtr<T>(ptr, libkern::no_retain);
}
