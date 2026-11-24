  
            
                         
                          
  

#include <libkern/c++/bounded_ptr.h>
#include <array>
#include <darwintest.h>
#include <darwintest_utils.h>
#include "test_utils.h"

#define _assert(...) T_ASSERT_TRUE((__VA_ARGS__), # __VA_ARGS__)

struct T {
	int i;
	friend constexpr bool
	operator==(T const volatile& a, T const& b)
	{
		return a.i == b.i;
	}
};

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
		                                                               
		                                                       
		                                                       
		                                                        
		test_bounded_ptr<QualT> ptr(array.begin() + 0, array.begin(), array.end());
		QualT& ref = *ptr;
		_assert(ref == T{0});
		_assert(&ref == &array[0]);

		_assert(ptr->i == 0);
		_assert(&ptr->i == &array[0].i);
	}
	{
		                                                               
		                                                       
		                                                       
		                                                        
		test_bounded_ptr<QualT> ptr(array.begin() + 1, array.begin(), array.end());
		QualT& ref = *ptr;
		_assert(ref == T{1});
		_assert(&ref == &array[1]);

		_assert(ptr->i == 1);
		_assert(&ptr->i == &array[1].i);
	}
	{
		                                                               
		                                                       
		                                                       
		                                                        
		test_bounded_ptr<QualT> ptr(array.begin() + 2, array.begin(), array.end());
		QualT& ref = *ptr;
		_assert(ref == T{2});
		_assert(&ref == &array[2]);

		_assert(ptr->i == 2);
		_assert(&ptr->i == &array[2].i);
	}
	{
		                                                               
		                                                       
		                                                       
		                                                        
		test_bounded_ptr<QualT> ptr(array.begin() + 4, array.begin(), array.end());
		QualT& ref = *ptr;
		_assert(ref == T{4});
		_assert(&ref == &array[4]);

		_assert(ptr->i == 4);
		_assert(&ptr->i == &array[4].i);
	}

	                                                                  
	{
		                                                               
		                                                        
		                                                        
		                                                         
		libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin() + 1, array.begin(), array.end());

		tracking_policy::did_trap = false;
		(void)*ptr;
		(void)ptr->i;
		_assert(!tracking_policy::did_trap);
	}

	                                                                
	{
		                                                               
		                                          
		                                          
		                                           
		libkern::bounded_ptr<QualT, tracking_policy> ptr(array.end() - 1, array.begin(), array.end() - 2);

		tracking_policy::did_trap = false;
		(void)*ptr;
		_assert(tracking_policy::did_trap);

		tracking_policy::did_trap = false;
		(void)ptr->i;
		_assert(tracking_policy::did_trap);
	}
	{
		                                                               
		                                                        
		                                                        
		                                                         
		libkern::bounded_ptr<QualT, tracking_policy> ptr(array.begin(), array.begin() + 1, array.end());

		tracking_policy::did_trap = false;
		(void)*ptr;
		_assert(tracking_policy::did_trap);

		tracking_policy::did_trap = false;
		(void)ptr->i;
		_assert(tracking_policy::did_trap);
	}
	{
		                                                               
		                                          
		                                          
		                                           
		T* t3 = const_cast<T*>(array.begin() + 3);
		char* just_off = reinterpret_cast<char*>(t3) + 1;              
		libkern::bounded_ptr<QualT, tracking_policy> ptr(reinterpret_cast<QualT*>(just_off), array.begin(), array.end() - 1);

		tracking_policy::did_trap = false;
		(void)*ptr;
		_assert(tracking_policy::did_trap);

		tracking_policy::did_trap = false;
		(void)ptr->i;
		_assert(tracking_policy::did_trap);
	}
}

T_DECL(deref, "bounded_ptr.deref") {
	tests<T, T>();
	tests<T, T const>();
	tests<T, T volatile>();
	tests<T, T const volatile>();

	                                                                    
	                                              
	test_bounded_ptr<void> p1;
	test_bounded_ptr<void const> p2;
	test_bounded_ptr<void volatile> p3;
	test_bounded_ptr<void const volatile> p4;
}
