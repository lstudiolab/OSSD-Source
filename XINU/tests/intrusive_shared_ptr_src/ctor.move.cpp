  
            
                         
                                                                          
  
                                                       
  

#include <libkern/c++/intrusive_shared_ptr.h>
#include <darwintest.h>
#include <darwintest_utils.h>
#include <type_traits>
#include <utility>
#include "test_policy.h"

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

template <typename Stored, typename From, typename To>
static void
tests()
{
	Stored obj{3};

	                              
	{
		test_policy::retain_count = 0;
		libkern::intrusive_shared_ptr<From, test_policy> from(&obj, libkern::retain);
		CHECK(test_policy::retain_count == 1);
		CHECK(from.get() == &obj);

		libkern::intrusive_shared_ptr<To, test_policy> to(std::move(from));            
		CHECK(test_policy::retain_count == 1);
		CHECK(to.get() == &obj);
		CHECK(from.get() == nullptr);
	}
	{
		test_policy::retain_count = 0;
		libkern::intrusive_shared_ptr<From, test_policy> from(&obj, libkern::retain);
		CHECK(test_policy::retain_count == 1);
		CHECK(from.get() == &obj);

		libkern::intrusive_shared_ptr<To, test_policy> to{std::move(from)};            
		CHECK(test_policy::retain_count == 1);
		CHECK(to.get() == &obj);
		CHECK(from.get() == nullptr);
	}
	{
		test_policy::retain_count = 0;
		libkern::intrusive_shared_ptr<From, test_policy> from(&obj, libkern::retain);
		CHECK(test_policy::retain_count == 1);
		CHECK(from.get() == &obj);

		libkern::intrusive_shared_ptr<To, test_policy> to = std::move(from);            
		CHECK(test_policy::retain_count == 1);
		CHECK(to.get() == &obj);
		CHECK(from.get() == nullptr);
	}

	                           
	{
		test_policy::retain_count = 3;
		libkern::intrusive_shared_ptr<From, test_policy> from = nullptr;
		libkern::intrusive_shared_ptr<To, test_policy> to = std::move(from);
		CHECK(test_policy::retain_count == 3);
		CHECK(to.get() == nullptr);
		CHECK(from.get() == nullptr);
	}
}

T_DECL(ctor_move, "intrusive_shared_ptr.ctor.move") {
	tests<           Derived,          Derived,        Derived>();
	tests<           Derived,          Derived,        Derived const>();
	tests<           Derived,          Derived const,        Derived const>();

	tests<           Derived,          Derived,        Base>();
	tests<           Derived,          Derived,        Base const>();
	tests<           Derived,          Derived const,        Base const>();

	tests<           DerivedMultiple,          DerivedMultiple,        Base1>();
	tests<           DerivedMultiple,          DerivedMultiple const,        Base1 const>();

	tests<           DerivedMultiple,          DerivedMultiple,        Base2>();
	tests<           DerivedMultiple,          DerivedMultiple const,        Base2 const>();

	                                       
	static_assert(std::is_move_constructible_v<test_shared_ptr<Derived> >);

	                                   
	static_assert(!std::is_constructible_v<       test_shared_ptr<Derived>,          test_shared_ptr<Base>&&>);
	static_assert(!std::is_constructible_v<       test_shared_ptr<DerivedMultiple>,          test_shared_ptr<Base1>&&>);
	static_assert(!std::is_constructible_v<       test_shared_ptr<DerivedMultiple>,          test_shared_ptr<Base2>&&>);
	static_assert(!std::is_constructible_v<       test_shared_ptr<Base2>,          test_shared_ptr<Base1>&&>);

	                                            
	static_assert(!std::is_constructible_v<       test_shared_ptr<Derived>,          test_shared_ptr<Derived const>&&>);

	                                                    
	static_assert(!std::is_constructible_v<       test_shared_ptr<char>,          test_shared_ptr<Derived>&&>);
	static_assert(!std::is_constructible_v<       test_shared_ptr<Unrelated>,          test_shared_ptr<Derived>&&>);
	static_assert(!std::is_constructible_v<       test_shared_ptr<Base2>,          test_shared_ptr<Base1>&&>);

	                                                              
	static_assert(!std::is_constructible_v<       libkern::intrusive_shared_ptr<Derived, dummy_policy<2> >,          libkern::intrusive_shared_ptr<Derived, dummy_policy<1> >&&>);
}
