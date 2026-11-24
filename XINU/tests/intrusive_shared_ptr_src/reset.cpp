  
            
                          
  

#include <libkern/c++/intrusive_shared_ptr.h>
#include <darwintest.h>
#include "test_policy.h"

struct T {
	int i;
};

template <typename T>
static void
tests()
{
	T obj{3};

	                                       
	{
		tracked_shared_ptr<T> ptr(&obj, libkern::retain);
		tracking_policy::reset();
		ptr.reset();
		CHECK(tracking_policy::releases == 1);
		CHECK(tracking_policy::retains == 0);
		CHECK(ptr.get() == nullptr);
	}

	                                   
	{
		tracked_shared_ptr<T> ptr = nullptr;
		tracking_policy::reset();
		ptr.reset();
		CHECK(tracking_policy::releases == 0);
		CHECK(tracking_policy::retains == 0);
		CHECK(ptr.get() == nullptr);
	}

	                              
	{
		tracked_shared_ptr<T> ptr(&obj, libkern::retain);
		tracked_shared_ptr<T> ptr2(&obj, libkern::retain);
		CHECK(!ptr.reset());

		CHECK(&ptr.reset() == &ptr);

		                         
		bool ok =  (ptr.reset() && !ptr2.reset());
		CHECK(ptr2.get() != nullptr);
	}
}

T_DECL(reset, "intrusive_shared_ptr.reset") {
	tests<T>();
	tests<T const>();
}
