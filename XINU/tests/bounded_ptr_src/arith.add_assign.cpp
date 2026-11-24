  
            
                                              
  

#include <libkern/c++/bounded_ptr.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <darwintest.h>
#include <darwintest_utils.h>
#include "test_utils.h"

#define _assert(...) T_ASSERT_TRUE((__VA_ARGS__), # __VA_ARGS__)

struct T { int i; };

namespace {
struct tracking_policy {
	static bool did_trap;
	static void
	trap(char const*)
	{
		did_trap = true;
	}
};
bool tracking_policy::did_trap = false;
}

template <typename T, typename QualT>
static void
tests()
{
	std::array<T, 5> array = {T{0}, T{1}, T{2}, T{3}, T{4}};

	                              
	                                                               
	                                                       
	                                                       
	                                                        
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 0;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[0]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 1;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[1]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 2;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[2]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 3;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[3]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 4;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[4]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.begin(), array.begin(), array.end());
		auto& ref = ptr += 5;
		_assert(&ref == &ptr);
		_assert(ptr == array.end());
	}

	                              
	                                                               
	                                                       
	                                                       
	                                                          
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += 0;
		_assert(&ref == &ptr);
		_assert(ptr == array.end());
	}
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += -1;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[4]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += -2;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[3]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += -3;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[2]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += -4;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[1]);
	}
	{
		test_bounded_ptr<QualT> ptr(array.end(), array.begin(), array.end());
		auto& ref = ptr += -5;
		_assert(&ref == &ptr);
		_assert(&*ptr == &array[0]);
	}

	                                                                              
	{
		std::ptrdiff_t sizeof_T = sizeof(T);                                               

		                                                                                              
		std::ptrdiff_t max_n = std::numeric_limits<std::ptrdiff_t>::max() / sizeof_T;

		                                                                                               
		std::ptrdiff_t min_n = std::numeric_limits<std::ptrdiff_t>::min() / sizeof_T;

		                                  
		{
			libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin(), array.begin(), array.end());
			tracking_policy::did_trap = false;
			ptr += max_n + 1;
			_assert(tracking_policy::did_trap);
		}

		                                  
		{
			libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin(), array.begin(), array.end());
			tracking_policy::did_trap = false;
			ptr += min_n - 1;
			_assert(tracking_policy::did_trap);
		}
	}

	                                                                     
	  
	                                                                       
	                                                                    
	                                          
	  
	                                                                    
	                                       
	{
		std::int64_t sizeof_T = sizeof(T);                                               

		                                                                                                       
		std::int64_t max_n = std::numeric_limits<std::int32_t>::max() / sizeof_T;

		                                                                                                        
		std::int64_t min_n = std::numeric_limits<std::int32_t>::min() / sizeof_T;

		                       
		{
			libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin(), array.begin(), array.end());
			tracking_policy::did_trap = false;
			ptr += static_cast<ptrdiff_t>(max_n / 2);
			_assert(!tracking_policy::did_trap);
			ptr += static_cast<ptrdiff_t>(max_n / 2);
			_assert(!tracking_policy::did_trap);
			ptr += (max_n % 2);
			_assert(!tracking_policy::did_trap);                                             
			ptr += 1;
			_assert(tracking_policy::did_trap);
		}

		                       
		{
			libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin(), array.begin(), array.end());
			tracking_policy::did_trap = false;
			ptr += static_cast<ptrdiff_t>(min_n / 2);
			_assert(!tracking_policy::did_trap);
			ptr += static_cast<ptrdiff_t>(min_n / 2);
			_assert(!tracking_policy::did_trap);
			ptr += (min_n % 2);
			_assert(!tracking_policy::did_trap);                                             
			ptr += -1;
			_assert(tracking_policy::did_trap);
		}
	}
}

T_DECL(arith_add_assign, "bounded_ptr.arith.add_assign") {
	tests<T, T>();
	tests<T, T const>();
	tests<T, T volatile>();
	tests<T, T const volatile>();
}
