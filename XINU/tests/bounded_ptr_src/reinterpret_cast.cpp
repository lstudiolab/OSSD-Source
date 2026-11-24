  
            
                                                      
                                                                                             
  

#include <libkern/c++/bounded_ptr.h>
#include <array>
#include <darwintest.h>
#include <darwintest_utils.h>
#include "test_utils.h"

#define _assert(...) T_ASSERT_TRUE((__VA_ARGS__), # __VA_ARGS__)

struct Base { int i; };
struct Derived : Base { };

struct Base1 { int i; };
struct Base2 { long l; };
struct DerivedMultiple : Base1, Base2 {
	DerivedMultiple(int i) : Base1{i}, Base2{i + 10}
	{
	}
};

struct non_default_policy {
	static constexpr void
	trap(char const*)
	{
	}
};

template <typename Stored, typename From, typename To>
static void
tests()
{
	std::array<Stored, 5> array = {Stored{0}, Stored{1}, Stored{2}, Stored{3}, Stored{4}};

	{
		test_bounded_ptr<From> from(array.begin() + 2, array.begin(), array.end());
		test_bounded_ptr<To> to = libkern::reinterpret_pointer_cast<To>(from);
		_assert(to.discard_bounds() == reinterpret_cast<To const*>(from.discard_bounds()));
	}

	{
		test_bounded_ptr<From> from(array.begin() + 2, array.begin(), array.end());
		test_bounded_ptr<To> to = libkern::reinterpret_pointer_cast<To>(from);
		_assert(to.discard_bounds() == reinterpret_cast<To const volatile*>(from.discard_bounds()));
	}

	                                                    
	{
		test_bounded_ptr<From> from(nullptr, nullptr, nullptr);
		test_bounded_ptr<To> to = libkern::reinterpret_pointer_cast<To>(from);
		_assert(to.unsafe_discard_bounds() == nullptr);
	}

	                                 
	{
		libkern::bounded_ptr<From, non_default_policy> from(array.begin(), array.begin(), array.end());
		libkern::bounded_ptr<To, non_default_policy> to = libkern::reinterpret_pointer_cast<To>(from);
		_assert(to.discard_bounds() == reinterpret_cast<To const*>(from.discard_bounds()));
	}
}

T_DECL(reinterpret_cast_, "bounded_ptr.reinterpret_cast") {
	tests<           Derived,          Derived,        Base>();
	tests<           Derived,          Derived const,        Base const>();
	tests<           Derived,          Derived volatile,        Base volatile>();
	tests<           Derived,          Derived const volatile,        Base const volatile>();

	tests<           DerivedMultiple,          DerivedMultiple,        Base1>();
	tests<           DerivedMultiple,          DerivedMultiple const,        Base1 const>();
	tests<           DerivedMultiple,          DerivedMultiple volatile,        Base1 volatile>();
	tests<           DerivedMultiple,          DerivedMultiple const volatile,        Base1 const volatile>();

	tests<           Derived,          Derived,        void>();
	tests<           Derived,          Derived const,        void const>();
	tests<           Derived,          Derived volatile,        void volatile>();
	tests<           Derived,          Derived const volatile,        void const volatile>();

	tests<           Derived,          Derived,        char>();
	tests<           Derived,          Derived const,        char const>();
	tests<           Derived,          Derived volatile,        char volatile>();
	tests<           Derived,          Derived const volatile,        char const volatile>();
}
