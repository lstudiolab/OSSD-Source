  
            
                         
                                                                                     
  
                                                                  
  

#include <libkern/c++/intrusive_shared_ptr.h>
#include <darwintest.h>
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
	Stored obj1{1};
	Stored obj2{2};

	                                   
	{
		tracked_shared_ptr<From> from(&obj1, libkern::retain);
		tracked_shared_ptr<To> to(&obj2, libkern::retain);
		tracking_policy::reset();
		tracked_shared_ptr<To>& ref = (to = std::move(from));
		CHECK(tracking_policy::releases == 1);
		CHECK(tracking_policy::retains == 0);
		CHECK(&ref == &to);
		CHECK(from.get() == nullptr);
		CHECK(to.get() == &obj1);
	}

	                               
	{
		tracked_shared_ptr<From> from(&obj1, libkern::retain);
		tracked_shared_ptr<To> to = nullptr;
		tracking_policy::reset();
		tracked_shared_ptr<To>& ref = (to = std::move(from));
		CHECK(tracking_policy::releases == 0);
		CHECK(tracking_policy::retains == 0);
		CHECK(&ref == &to);
		CHECK(from.get() == nullptr);
		CHECK(to.get() == &obj1);
	}

	                               
	{
		tracked_shared_ptr<From> from = nullptr;
		tracked_shared_ptr<To> to(&obj2, libkern::retain);
		tracking_policy::reset();
		tracked_shared_ptr<To>& ref = (to = std::move(from));
		CHECK(tracking_policy::releases == 1);
		CHECK(tracking_policy::retains == 0);
		CHECK(&ref == &to);
		CHECK(from.get() == nullptr);
		CHECK(to.get() == nullptr);
	}

	                           
	{
		tracked_shared_ptr<From> from = nullptr;
		tracked_shared_ptr<To> to = nullptr;
		tracking_policy::reset();
		tracked_shared_ptr<To>& ref = (to = std::move(from));
		CHECK(tracking_policy::releases == 0);
		CHECK(tracking_policy::retains == 0);
		CHECK(&ref == &to);
		CHECK(from.get() == nullptr);
		CHECK(to.get() == nullptr);
	}
}

T_DECL(assign_move, "intrusive_shared_ptr.assign.move") {
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

	                                       
	static_assert(std::is_move_assignable_v<test_shared_ptr<Derived> >);

	                                   
	static_assert(!std::is_assignable_v<       test_shared_ptr<Derived>,          test_shared_ptr<Base>&&>);
	static_assert(!std::is_assignable_v<       test_shared_ptr<DerivedMultiple>,          test_shared_ptr<Base1>&&>);
	static_assert(!std::is_assignable_v<       test_shared_ptr<DerivedMultiple>,          test_shared_ptr<Base2>&&>);
	static_assert(!std::is_assignable_v<       test_shared_ptr<Base2>,          test_shared_ptr<Base1>&&>);

	                                            
	static_assert(!std::is_assignable_v<       test_shared_ptr<Derived>,          test_shared_ptr<Derived const>&&>);

	                                                    
	static_assert(!std::is_assignable_v<       test_shared_ptr<char>,          test_shared_ptr<Derived>&&>);
	static_assert(!std::is_assignable_v<       test_shared_ptr<Unrelated>,          test_shared_ptr<Derived>&&>);
	static_assert(!std::is_assignable_v<       test_shared_ptr<Base2>,          test_shared_ptr<Base1>&&>);

	                                                              
	static_assert(!std::is_assignable_v<       libkern::intrusive_shared_ptr<Derived, dummy_policy<2> >,          libkern::intrusive_shared_ptr<Derived, dummy_policy<1> >&&>);
}
