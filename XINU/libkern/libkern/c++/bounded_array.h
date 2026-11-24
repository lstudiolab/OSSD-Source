  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_BOUNDED_ARRAY_H
#define XNU_LIBKERN_LIBKERN_CXX_BOUNDED_ARRAY_H

#include <libkern/c++/bounded_ptr.h>
#include <stddef.h>
#include <os/base.h>

namespace libkern {
                                                               
  
                                                                            
                                                                           
                                                                      
  
        
                                                 
                                
template <typename T, size_t N, typename TrappingPolicy>
struct bounded_array {
	                                                                            
	                                                                           
	T data_[N];

	using iterator = bounded_ptr<T, TrappingPolicy>;
	using const_iterator = bounded_ptr<T const, TrappingPolicy>;

	iterator
	begin() noexcept
	{
		return iterator(data_, data_, data_ + N);
	}
	const_iterator
	begin() const noexcept
	{
		return const_iterator(data_, data_, data_ + N);
	}
	iterator
	end() noexcept
	{
		return iterator(data_ + N, data_, data_ + N);
	}
	const_iterator
	end() const noexcept
	{
		return const_iterator(data_ + N, data_, data_ + N);
	}

	constexpr size_t
	size() const
	{
		return N;
	}
	constexpr T*
	data() noexcept
	{
		return data_;
	}
	constexpr T const*
	data() const noexcept
	{
		return data_;
	}
	OS_ALWAYS_INLINE T&
	operator[](ptrdiff_t n)
	{
		return begin()[n];
	}
	OS_ALWAYS_INLINE T const&
	operator[](ptrdiff_t n) const
	{
		return begin()[n];
	}
};
}                         

#endif                                            
