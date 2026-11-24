  
            
                                    
                                                
  

#include <libkern/c++/safe_allocation.h>
#include <darwintest.h>
#include "test_utils.h"
#include <cstddef>
#include <limits>

struct T {
	long i;
};

template <typename RawT, typename QualT>
static void
tests()
{
	                             
	{
		RawT* memory = reinterpret_cast<RawT*>(malloc_allocator::allocate(10 * sizeof(RawT)));
		for (RawT* ptr = memory; ptr != memory + 10; ++ptr) {
			*ptr = RawT{ptr - memory};                      
		}

		test_safe_allocation<QualT> array(memory, 10, libkern::adopt_memory);
		for (std::ptrdiff_t n = 0; n != 10; ++n) {
			QualT& element = array[n];
			CHECK(&element == memory + n);
		}
	}

	                         
	{
		RawT* memory = reinterpret_cast<RawT*>(malloc_allocator::allocate(10 * sizeof(RawT)));
		for (RawT* ptr = memory; ptr != memory + 10; ++ptr) {
			*ptr = RawT{ptr - memory};                      
		}

		test_safe_allocation<QualT> const array(memory, 10, libkern::adopt_memory);
		for (std::ptrdiff_t n = 0; n != 10; ++n) {
			QualT const& element = array[n];
			CHECK(&element == memory + n);
		}
	}

	                                      
	{
		using Alloc = libkern::safe_allocation<RawT, malloc_allocator, tracking_trapping_policy>;
		RawT* memory = reinterpret_cast<RawT*>(malloc_allocator::allocate(10 * sizeof(RawT)));
		Alloc const array(memory, 10, libkern::adopt_memory);

		                   
		{
			tracking_trapping_policy::reset();
			(void)array[-1];
			CHECK(tracking_trapping_policy::did_trap);
		}
		{
			tracking_trapping_policy::reset();
			(void)array[-10];
			CHECK(tracking_trapping_policy::did_trap);
		}

		                    
		{
			tracking_trapping_policy::reset();
			(void)array[10];
			CHECK(tracking_trapping_policy::did_trap);
		}
		{
			tracking_trapping_policy::reset();
			(void)array[11];
			CHECK(tracking_trapping_policy::did_trap);
		}
	}
}

T_DECL(operator_subscript, "safe_allocation.operator.subscript") {
	tests<T, T>();
	tests<T, T const>();
}
