  
            
                                                                                
                                                                
                                                                
  

#include <libkern/c++/bounded_ptr.h>
#include <array>
#include <darwintest.h>
#include <darwintest_utils.h>
#include "test_utils.h"

#define _assert(...) T_ASSERT_TRUE((__VA_ARGS__), # __VA_ARGS__)

struct T {
	int i;
};

template <typename Stored, typename Left, typename Right>
static void
tests()
{
	std::array<Stored, 5> array = {Stored{0}, Stored{1}, Stored{2}, Stored{3}, Stored{4}};

	         
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 0);
	}
	{
		test_bounded_ptr<Left> const a(array.begin() + 1, array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 1);
	}
	{
		test_bounded_ptr<Left> const a(array.begin() + 2, array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 2);
	}
	{
		test_bounded_ptr<Left> const a(array.begin() + 3, array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 3);
	}
	{
		test_bounded_ptr<Left> const a(array.begin() + 4, array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 4);
	}
	{
		test_bounded_ptr<Left> const a(array.end(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin(), array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == 5);
	}

	        
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin() + 1, array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == -1);
	}
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin() + 2, array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == -2);
	}
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin() + 3, array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == -3);
	}
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin() + 4, array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == -4);
	}
	{
		test_bounded_ptr<Left> const a(array.begin(), array.begin(), array.end());
		test_bounded_ptr<Right> const b(array.begin() + 5, array.begin(), array.end());
		std::ptrdiff_t diff = a - b;
		_assert(diff == -5);
	}

	                                          
	{
		test_bounded_ptr<Left> const a(array.begin() + 2, array.begin() + 1, array.end() - 1);
		test_bounded_ptr<Right> const b(array.begin() + 4, array.begin() + 3, array.end());
		_assert(a - b == -2);
		_assert(b - a == 2);
	}

	                             
	{
		test_bounded_ptr<Left> const a(array.begin() + 2, array.begin() + 1, array.end() - 1);
		Right* b = array.begin() + 4;
		_assert(a - b == -2);
	}
	{
		Left* a = array.begin() + 4;
		test_bounded_ptr<Right> const b(array.begin() + 2, array.begin() + 1, array.end() - 1);
		_assert(a - b == 2);
	}
}

T_DECL(arith_difference, "bounded_ptr.arith.difference") {
	tests<T, T, T>();
	tests<T, T, T const>();
	tests<T, T const, T>();
	tests<T, T const, T const>();
}
