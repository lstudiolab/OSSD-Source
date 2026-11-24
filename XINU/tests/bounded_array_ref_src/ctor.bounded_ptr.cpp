  
            
                                                                              
  

#include <libkern/c++/bounded_array_ref.h>
#include "test_policy.h"
#include <cstddef>
#include <darwintest.h>
#include <darwintest_utils.h>

struct T { int i; };
inline bool
operator==(T const& a, T const& b)
{
	return a.i == b.i;
};

template <typename T>
static void
tests()
{
	T array[5] = {T{0}, T{1}, T{2}, T{3}, T{4}};
	T* const begin = &array[0];
	T* const end = &array[5];

	                                                               
	                                                       
	                                                       
	                                                        
	  
	                                                       
	{
		test_bounded_ptr<T> ptr(&array[0], begin, end);
		test_bounded_array_ref<T> view(ptr, 5);
		CHECK(view.data() == &array[0]);
		CHECK(view.size() == 5);
		CHECK(view[0] == T{0});
		CHECK(view[1] == T{1});
		CHECK(view[2] == T{2});
		CHECK(view[3] == T{3});
		CHECK(view[4] == T{4});
	}
	                                                               
	                                                       
	                                                       
	                                                        
	  
	                       
	{
		test_bounded_ptr<T> ptr(&array[0], begin, end);
		test_bounded_array_ref<T> view(ptr, 3);
		CHECK(view.data() == &array[0]);
		CHECK(view.size() == 3);
		CHECK(view[0] == T{0});
		CHECK(view[1] == T{1});
		CHECK(view[2] == T{2});
	}
	                                                               
	                                                       
	                                                       
	                                                        
	  
	                                                       
	{
		test_bounded_ptr<T> ptr(&array[3], begin, end);
		test_bounded_array_ref<T> view(ptr, 2);
		CHECK(view.data() == &array[3]);
		CHECK(view.size() == 2);
		CHECK(view[0] == T{3});
		CHECK(view[1] == T{4});
	}
	                                                    
	{
		test_bounded_ptr<T> ptr(&array[0], begin, end);
		test_bounded_array_ref<T> view(ptr, 0);
		CHECK(view.size() == 0);
	}
	                                                   
	{
		test_bounded_ptr<T> ptr = nullptr;
		test_bounded_array_ref<T> view(ptr, 0);
		CHECK(view.size() == 0);
	}
	                                                                   
	{
		test_bounded_ptr<T> ptr(end, begin, end);
		test_bounded_array_ref<T> view(ptr, 0);
		CHECK(view.size() == 0);
	}
	                                                       
	{
		test_bounded_ptr<T> ptr(begin, begin, end);
		std::ptrdiff_t size = sizeof(array) / sizeof(*array);
		test_bounded_array_ref<T> view(ptr, size);
		CHECK(view.data() == &array[0]);
		CHECK(view.size() == 5);
	}
}

T_DECL(ctor_bounded_ptr, "bounded_array_ref.ctor.bounded_ptr") {
	tests<T>();
	tests<T const>();
}
