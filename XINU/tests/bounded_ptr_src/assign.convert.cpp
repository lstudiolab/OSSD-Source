  
            
                                          
                                                                
  

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

struct Unrelated { };

struct dummy_policy1 {
	static constexpr void
	trap(char const*)
	{
	}
};
struct dummy_policy2 {
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
	Stored* const ptr1 = array.begin() + 2;
	Stored* const ptr2 = array.begin() + 3;

	{
		test_bounded_ptr<From> const from(ptr1, array.begin(), array.end());
		test_bounded_ptr<To> to;
		test_bounded_ptr<To>& ref = (to = from);
		_assert(to.discard_bounds() == static_cast<To const*>(ptr1));
		_assert(&ref == &to);                             
	}

	                                       
	{
		test_bounded_ptr<From> const from(ptr1, array.begin(), array.end());
		test_bounded_ptr<To> to(ptr2, array.begin(), array.end());
		_assert(to.discard_bounds() == static_cast<To const*>(ptr2));

		test_bounded_ptr<To>& ref = (to = from);
		_assert(to.discard_bounds() == static_cast<To const*>(ptr1));
		_assert(&ref == &to);                             
	}

	                                     
	{
		test_bounded_ptr<From> const from = nullptr;
		test_bounded_ptr<To> to;
		test_bounded_ptr<To>& ref = (to = from);
		_assert(to.unsafe_discard_bounds() == nullptr);
		_assert(&ref == &to);                             
	}

	                               
	{
		libkern::bounded_ptr<From, dummy_policy1> from(ptr1, array.begin(), array.end());
		libkern::bounded_ptr<To, dummy_policy2> to;
		libkern::bounded_ptr<To, dummy_policy2>& ref = (to = from);
		_assert(to.discard_bounds() == static_cast<To const*>(ptr1));
		_assert(&ref == &to);                             
	}

	                                                               
	                                                       
	                                                       
	                                                        
	{
		test_bounded_ptr<From> const from(array.begin(), array.begin() + 1, array.end());
		test_bounded_ptr<To> to;
		to = from;
		_assert(to.unsafe_discard_bounds() == static_cast<To const*>(array.begin()));
	}
}

T_DECL(assign_convert, "bounded_ptr.assign.convert") {
	tests<           Derived,          Derived,        Derived>();
	tests<           Derived,          Derived const,        Derived const>();
	tests<           Derived,          Derived volatile,        Derived volatile>();
	tests<           Derived,          Derived const volatile,        Derived const volatile>();

	tests<           Derived,          Derived,        Base>();
	tests<           Derived,          Derived const,        Base const>();
	tests<           Derived,          Derived volatile,        Base volatile>();
	tests<           Derived,          Derived const volatile,        Base const volatile>();

	tests<           DerivedMultiple,          DerivedMultiple,        Base1>();
	tests<           DerivedMultiple,          DerivedMultiple const,        Base1 const>();
	tests<           DerivedMultiple,          DerivedMultiple volatile,        Base1 volatile>();
	tests<           DerivedMultiple,          DerivedMultiple const volatile,        Base1 const volatile>();

	tests<           DerivedMultiple,          DerivedMultiple,        Base2>();
	tests<           DerivedMultiple,          DerivedMultiple const,        Base2 const>();
	tests<           DerivedMultiple,          DerivedMultiple volatile,        Base2 volatile>();
	tests<           DerivedMultiple,          DerivedMultiple const volatile,        Base2 const volatile>();

	tests<           Derived,          Derived,        void>();
	tests<           Derived,          Derived const,        void const>();
	tests<           Derived,          Derived volatile,        void volatile>();
	tests<           Derived,          Derived const volatile,        void const volatile>();

	                                   
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Derived>,          test_bounded_ptr<Base> >);
	static_assert(!std::is_assignable_v<       test_bounded_ptr<DerivedMultiple>,          test_bounded_ptr<Base1> >);
	static_assert(!std::is_assignable_v<       test_bounded_ptr<DerivedMultiple>,          test_bounded_ptr<Base2> >);
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Base2>,          test_bounded_ptr<Base1> >);

	                                            
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Derived>,          test_bounded_ptr<Derived const> >);

	                                                               
	static_assert(!std::is_assignable_v<       test_bounded_ptr<char>,          test_bounded_ptr<Derived> >);
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Unrelated>,          test_bounded_ptr<Derived> >);
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Base2>,          test_bounded_ptr<Base1> >);

	                                              
	static_assert(!std::is_assignable_v<       test_bounded_ptr<Derived>,          Derived*>);
}
