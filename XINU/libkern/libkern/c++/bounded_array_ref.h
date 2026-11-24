  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_BOUNDED_ARRAY_REF_H
#define XNU_LIBKERN_LIBKERN_CXX_BOUNDED_ARRAY_REF_H

#include <libkern/c++/bounded_array.h>
#include <libkern/c++/bounded_ptr.h>
#include <stddef.h>
#include <os/base.h>

namespace libkern {
namespace bar_detail {
using nullptr_t = decltype(nullptr);
}

                                                                              
                                             
  
                                                                          
                                                                            
                                                
  
                                                                            
                                                                         
                                                                           
                                                  
  
                                                                              
template <typename T, typename TrappingPolicy>
struct bounded_array_ref {
	                                        
	  
	                                                                   
	                                          
	explicit constexpr bounded_array_ref() noexcept : data_(nullptr), size_(0)
	{
	}

	                                                                   
	  
	                                                                     
	                                                                         
	                                                                         
	             
	explicit constexpr bounded_array_ref(bounded_ptr<T, TrappingPolicy> data, size_t n)
		: data_(data.unsafe_discard_bounds()), size_(static_cast<uint32_t>(n))
	{
		if (n != 0) {
			data[n - 1];                                  
			                                     
		}
		if (__improbable(n > UINT32_MAX)) {
			TrappingPolicy::trap("bounded_array_ref: Can't construct from a size greater than UINT32_MAX");
		}
	}

	                                                               
	  
	                                                                     
	                                                                         
	                                                                 
	explicit constexpr bounded_array_ref(T* data, size_t n) : data_(data), size_(static_cast<uint32_t>(n))
	{
		if (__improbable(n > UINT32_MAX)) {
			TrappingPolicy::trap("bounded_array_ref: Can't construct from a size greater than UINT32_MAX");
		}
	}

	                                                                        
	  
	                                                                         
	                                                                     
	                                                                       
	                                                    
	explicit constexpr bounded_array_ref(T* first, T* last) : data_(first), size_(static_cast<uint32_t>(last - first))
	{
		if (__improbable(first > last)) {
			TrappingPolicy::trap("bounded_array_ref: The [first, last) constructor requires a valid range.");
		}
		if (__improbable(last - first > UINT32_MAX)) {
			TrappingPolicy::trap("bounded_array_ref: Can't construct from a size greater than UINT32_MAX");
		}
	}

	                                                        
	  
	                                                                       
	                                                                          
	template <size_t N>
	constexpr bounded_array_ref(bounded_array<T, N, TrappingPolicy>& data) : data_(data.data()), size_(static_cast<uint32_t>(data.size()))
	{
		if (__improbable(data.size() > UINT32_MAX)) {
			TrappingPolicy::trap("bounded_array_ref: Can't construct from a size greater than UINT32_MAX");
		}
	}

	                                                      
	  
	                                                                       
	                                                               
	template <size_t N>
	constexpr bounded_array_ref(T (&array)[N]) : data_(array), size_(static_cast<uint32_t>(N))
	{
		if (__improbable(N > UINT32_MAX)) {
			TrappingPolicy::trap("bounded_array_ref: Can't construct from a size greater than UINT32_MAX");
		}
	}

	constexpr
	bounded_array_ref(bounded_array_ref const&) = default;
	constexpr
	bounded_array_ref(bounded_array_ref&& other) noexcept = default;

	constexpr bounded_array_ref& operator=(bounded_array_ref const&) = default;
	constexpr bounded_array_ref& operator=(bounded_array_ref&& other) = default;
	~bounded_array_ref() = default;

	                                                                       
	  
	                                                                        
	                                                                    
	                                                                     
	                                                                   
	explicit
	operator bool() const noexcept
	{
		return data_ != nullptr;
	}

	using iterator = bounded_ptr<T, TrappingPolicy>;

	                                                                    
	                                        
	  
	                                                                        
	                                                                     
	                   
	  
	                                                                       
	                                                                         
	                                                                         
	  
	                                                                      
	                                                                 
	                
	iterator
	begin() const noexcept
	{
		return iterator(data_, data_, data_ + size_);
	}
	iterator
	end() const noexcept
	{
		return iterator(data_ + size_, data_, data_ + size_);
	}

	                                                                
	                       
	  
	                                                                    
	                                                        
	constexpr size_t
	size() const
	{
		return size_;
	}

	                                                                        
	                       
	  
	                                                                        
	                                                
	constexpr T*
	data() const noexcept
	{
		return data_;
	}

	                                                    
	  
	                                                                   
	                                                                
	  
	               
	                                                                        
	                                                                        
	                                                                     
	                                                                     
	                                   
	OS_ALWAYS_INLINE T&
	operator[](ptrdiff_t n) const
	{
		return begin()[n];
	}

	                                                                      
	                
	  
	                                                                          
	                                                                         
	                                                                 
	  
	                                                                     
	                                                                       
	                                                                          
	                                                                        
	            
	bounded_array_ref<T, TrappingPolicy>
	slice(size_t n, size_t m) const
	{
		uint32_t total;
		if (__improbable(os_add_overflow(n, m, &total))) {
			TrappingPolicy::trap("bounded_array_ref: n + m is larger than the size of any bounded_array_ref");
		}
		if (__improbable(total > size())) {
			TrappingPolicy::trap("bounded_array_ref: invalid slice provided, the indices are of bounds for the bounded_array_ref");
		}
		return bounded_array_ref(data_ + n, m);
	}

private:
	T* data_;
	uint32_t size_;
};

                                                                    
                                                    
template <typename T, typename P>
bool
operator==(bounded_array_ref<T, P> const& x, bar_detail::nullptr_t)
{
	return !static_cast<bool>(x);
}

template <typename T, typename P>
bool
operator!=(bounded_array_ref<T, P> const& x, bar_detail::nullptr_t)
{
	return !(x == nullptr);
}

template <typename T, typename P>
bool
operator==(bar_detail::nullptr_t, bounded_array_ref<T, P> const& x)
{
	return x == nullptr;
}

template <typename T, typename P>
bool
operator!=(bar_detail::nullptr_t, bounded_array_ref<T, P> const& x)
{
	return x != nullptr;
}
}                         

#endif                                                
