  
                                                                        
  
                                                                         
                                                                           
                                                                           
                                                               
  

#include <libkern/c++/bounded_ptr.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <darwintest.h>
#include "test_utils.h"

test_bounded_ptr<void>
bounded_malloc(std::size_t size)
{
	void* p = std::malloc(size);
	void* end = static_cast<char*>(p) + size;
	test_bounded_ptr<void> with_bounds(p, p, end);
	return with_bounds;
}

void
bounded_free(test_bounded_ptr<void> ptr)
{
	std::free(ptr.discard_bounds());
}

struct SomeType {
	std::uint32_t idx;
};

                                                                       
                                                                            
                                                                           
                                                       
void
use(SomeType*)
{
}
void
require(bool condition)
{
	if (!condition) {
		std::exit(EXIT_FAILURE);
	}
}

T_DECL(example_malloc, "bounded_ptr.example.malloc") {
	test_bounded_ptr<SomeType> array = nullptr;
	std::uint32_t count = 100;
	std::uint32_t alloc_size = count * sizeof(SomeType);

	                                           
	                                                                        
	array = libkern::reinterpret_pointer_cast<SomeType>(bounded_malloc(alloc_size));

	require(array != nullptr);                                                                    
	use(array.discard_bounds());                                     

	for (std::uint32_t i = 0; i < count; i++) {
		std::uint32_t& idx = array[i].idx;
		idx = i;
		use(&array[idx]);
	}

	if (array) {
		bounded_free(array);                                      
	}

	T_PASS("bounded_ptr.example.malloc test done");
}
