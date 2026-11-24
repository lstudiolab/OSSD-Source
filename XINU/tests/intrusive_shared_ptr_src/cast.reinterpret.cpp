  
            
                                                    
                                                                                                            
  
                                                    
                                                                                                      
  

#include <libkern/c++/intrusive_shared_ptr.h>
#include <utility>
#include <darwintest.h>
#include "test_policy.h"

struct Base { int i; };
struct Derived : Base { };

                                 
struct Unrelated { int i; };

template <typename Stored, typename From, typename To>
static void
tests()
{
	Stored obj{3};

	{
		tracked_shared_ptr<From> const from(&obj, libkern::no_retain);
		tracking_policy::reset();
		tracked_shared_ptr<To> to = libkern::reinterpret_pointer_cast<To>(from);
		CHECK(tracking_policy::retains == 1);
		CHECK(tracking_policy::releases == 0);
		CHECK(to.get() == reinterpret_cast<To const*>(&obj));
		CHECK(from.get() == &obj);
	}
	{
		tracked_shared_ptr<From> from(&obj, libkern::no_retain);
		tracking_policy::reset();
		tracked_shared_ptr<To> to = libkern::reinterpret_pointer_cast<To>(std::move(from));
		CHECK(tracking_policy::retains == 0);
		CHECK(tracking_policy::releases == 0);
		CHECK(to.get() == reinterpret_cast<To const*>(&obj));
		CHECK(from.get() == nullptr);
	}

	                                                    
	{
		tracked_shared_ptr<From> const from = nullptr;
		tracking_policy::reset();
		tracked_shared_ptr<To> to = libkern::reinterpret_pointer_cast<To>(from);
		CHECK(tracking_policy::retains == 0);
		CHECK(tracking_policy::releases == 0);
		CHECK(to.get() == nullptr);
		CHECK(from.get() == nullptr);
	}
	{
		tracked_shared_ptr<From> from = nullptr;
		tracking_policy::reset();
		tracked_shared_ptr<To> to = libkern::reinterpret_pointer_cast<To>(std::move(from));
		CHECK(tracking_policy::retains == 0);
		CHECK(tracking_policy::releases == 0);
		CHECK(to.get() == nullptr);
		CHECK(from.get() == nullptr);
	}
}

T_DECL(cast_reinterpret, "intrusive_shared_ptr.cast.reinterpret") {
	tests<           Derived,          Derived,        Base>();
	tests<           Derived,          Derived const,        Base const>();

	tests<           Derived,          Derived,        char>();
	tests<           Derived,          Derived const,        char const>();

	tests<           Derived,          Derived,        Unrelated>();
	tests<           Derived,          Derived const,        Unrelated const>();
}
