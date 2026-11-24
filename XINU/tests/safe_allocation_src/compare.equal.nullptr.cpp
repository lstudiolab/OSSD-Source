  
            
                                                                  
                                                                                        
  
                                                                  
                                                                                        
  
                                                                  
                                                                                        
  
                                                                  
                                                                                        
  

#include <libkern/c++/safe_allocation.h>
#include <darwintest.h>
#include "test_utils.h"

struct T { };

template <typename T>
static void
tests()
{
	{
		test_safe_allocation<T> const array(10, libkern::allocate_memory);
		CHECK(!(array == nullptr));
		CHECK(!(nullptr == array));
		CHECK(array != nullptr);
		CHECK(nullptr != array);
	}
	{
		test_safe_allocation<T> const array = nullptr;
		CHECK(array == nullptr);
		CHECK(nullptr == array);
		CHECK(!(array != nullptr));
		CHECK(!(nullptr != array));
	}
}

T_DECL(compare_equal_nullptr, "safe_allocation.compare.equal.nullptr") {
	tests<T>();
	tests<T const>();
}
