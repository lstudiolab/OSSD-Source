  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_SAFE_ALLOCATION_H
#define XNU_LIBKERN_LIBKERN_CXX_SAFE_ALLOCATION_H

#include <stddef.h>
#include <stdint.h>
#include <os/base.h>
#include <libkern/c++/bounded_ptr.h>

void* operator new(size_t, void*);                                                

namespace libkern {
namespace sa_detail {
                                                                       
using nullptr_t = decltype(nullptr);
template <typename T>
constexpr bool is_trivially_destructible_v = __is_trivially_destructible(T);
template <typename T>
constexpr bool is_empty_v = __is_empty(T);
template <typename T>
constexpr bool is_nothrow_default_constructible_v = __is_nothrow_constructible(T);

template <bool Cond, typename T = void> struct enable_if;
template <typename T> struct enable_if<true, T> { using type = T; };
template <bool Cond, typename T = void> using enable_if_t = typename enable_if<Cond, T>::type;

template <typename T> struct remove_const { using type = T; };
template <typename T> struct remove_const<T const> { using type = T; };
template <typename T> using remove_const_t = typename remove_const<T>::type;

template <typename T>
void
generic_swap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

template <typename T, enable_if_t<!is_trivially_destructible_v<T> >* = nullptr>
void
destroy(T* first, T* last)
{
	for (; first != last; ++first) {
		first->~T();
	}
}

template <typename T, enable_if_t<is_trivially_destructible_v<T> >* = nullptr>
void
destroy(T*, T*)
{
	                                                         
}

template <typename T>
void
uninitialized_value_construct(T* first, T* last)
{
	for (; first != last; ++first) {
		::new (static_cast<void*>(first)) T();
	}
}
}                           

struct adopt_memory_t {
	explicit constexpr
	adopt_memory_t() = default;
};
inline constexpr adopt_memory_t adopt_memory{};

struct allocate_memory_t {
	explicit constexpr
	allocate_memory_t() = default;
};
inline constexpr allocate_memory_t allocate_memory{};

                                                                         
                                         
  
                                            
                                                                              
                                                                                    
                                                                                        
  
                                                                               
                                                                                    
                                                                                  
                                                                                
                                                                              
  
                                                                                                 
                                                                                        
                                                                        
  
                                        
                                        
                                                                              
                                                                         
                                                                          
                                                                     
                                                                             
                                  
  
                                                       
                                                       
                                                                               
                                                                          
                                                                        
                                                                              
                                                                           
                                                                              
                                                                                
template <typename T, typename Allocator, typename TrappingPolicy>
struct safe_allocation {
	static_assert(sa_detail::is_empty_v<Allocator>,
	    "safe_allocation<T, Alloc, ...> requires the Allocator to be stateless");

	                                                   
	  
	                                                                         
	                                                                       
	                            
	  
	                                                                        
	                                                                     
	              
	explicit constexpr safe_allocation() noexcept : data_(nullptr), size_(0)
	{
	}

	constexpr safe_allocation(sa_detail::nullptr_t) noexcept : safe_allocation()
	{
	}

	                                                                             
	  
	                                                                         
	                                                                         
	                                                                  
	                                                                        
	                     
	  
	                                                                           
	                                                         
	  
	                                                                             
	                                                                     
	                                                                     
	                                                                      
	                                                                            
	explicit safe_allocation(T* data, size_t n, adopt_memory_t) : data_(data)
	{
		if (__improbable(n > UINT32_MAX)) {
			TrappingPolicy::trap("safe_allocation size exceeds UINT32_MAX");
		}

		size_ = static_cast<uint32_t>(n);
	}

	                                                              
	  
	                                                                       
	                                                                         
	                                                                            
	  
	                                                                   
	                                                                      
	                                                                        
	explicit safe_allocation(size_t n, allocate_memory_t)
	{
		size_t bytes;
		if (__improbable(os_mul_overflow(n, sizeof(T), &bytes) || (n > UINT32_MAX))) {
			data_ = nullptr;
			size_ = 0;
		} else {
			data_ = reinterpret_cast<T*>(Allocator::allocate(bytes));
			size_ = static_cast<uint32_t>(n);
			using RawT = sa_detail::remove_const_t<T>;
			RawT* data = const_cast<RawT*>(data_);
			sa_detail::uninitialized_value_construct(data, data + size_);
		}
	}

	                                                                        
	                                                                        
	          
	safe_allocation(safe_allocation const&) = delete;
	safe_allocation& operator=(safe_allocation const&) = delete;

	                                                                     
	               
	  
	                                                                      
	                                                                     
	                                                                    
	                                                                      
	                                                                   
	                     
	safe_allocation(safe_allocation&& other) noexcept : data_(other.data_), size_(other.size_)
	{
		other.data_ = nullptr;
		other.size_ = 0;
	}

	                                                           
	  
	                                                                     
	                                                         
	safe_allocation&
	operator=(sa_detail::nullptr_t)
	{
		if (data_ != nullptr) {
			destroy_dealloc_(data_, size_);
		}
		data_ = nullptr;
		size_ = 0;
		return *this;
	}

	                                                                     
	               
	  
	                                                                      
	                                                                       
	                                  
	  
	                                                                         
	                                                                    
	                          
	  
	                                                    
	safe_allocation&
	operator=(safe_allocation&& other)
	{
		if (&other == this) {
			return *this;
		}

		T* old_data = data_;
		size_t old_size = size_;

		data_ = other.data_;
		size_ = other.size_;
		other.data_ = nullptr;
		other.size_ = 0;

		if (old_data != nullptr) {
			destroy_dealloc_(old_data, old_size);
		}

		return *this;
	}

	                                                                 
	                                                           
	  
	                                                                  
	~safe_allocation()
	{
		if (data_ != nullptr) {
			destroy_dealloc_(data_, size_);
		}
	}

	                                                                      
	                           
	explicit
	operator bool() const noexcept
	{
		return data_ != nullptr;
	}

	using iterator = bounded_ptr<T, TrappingPolicy>;
	using const_iterator = bounded_ptr<T const, TrappingPolicy>;

	                                                                    
	                                      
	  
	                                                                      
	                                                                       
	                   
	  
	                                                                   
	                                                                          
	                                                                         
	                             
	  
	                                                                            
	                                                               
	OS_ALWAYS_INLINE iterator
	begin() noexcept
	{
		if (data_ == nullptr) {
			return iterator();
		} else {
			return iterator(data_, data_, data_ + size_);
		}
	}
	OS_ALWAYS_INLINE const_iterator
	begin() const noexcept
	{
		if (data_ == nullptr) {
			return const_iterator();
		} else {
			return const_iterator(data_, data_, data_ + size_);
		}
	}
	iterator
	end() noexcept
	{
		if (data_ == nullptr) {
			return iterator();
		} else {
			return iterator(data_ + size_, data_, data_ + size_);
		}
	}
	const_iterator
	end() const noexcept
	{
		if (data_ == nullptr) {
			return const_iterator();
		} else {
			return const_iterator(data_ + size_, data_, data_ + size_);
		}
	}

	                                                   
	  
	                                                                   
	                                                 
	size_t
	size() const
	{
		return size_;
	}

	                                                                     
	                     
	  
	                                                                      
	                                                
	T*
	data() noexcept
	{
		return data_;
	}
	T const*
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

private:
	               
	friend void
	swap(safe_allocation& a, safe_allocation& b) noexcept
	{
		sa_detail::generic_swap(a.data_, b.data_);
		sa_detail::generic_swap(a.size_, b.size_);
	}

	static void
	destroy_dealloc_(T* ptr, size_t size)
	{
		sa_detail::destroy(ptr, ptr + size);
		                                                           
		                                                          
		using RawT = sa_detail::remove_const_t<T>;
		Allocator::deallocate(const_cast<RawT*>(ptr), size * sizeof(T));
	}

	T* data_;
	uint32_t size_;
};

                                                                               
                  
template <typename T, typename A, typename P>
bool
operator==(safe_allocation<T, A, P> const& x, sa_detail::nullptr_t)
{
	return !static_cast<bool>(x);
}

template <typename T, typename A, typename P>
bool
operator!=(safe_allocation<T, A, P> const& x, sa_detail::nullptr_t)
{
	return !(x == nullptr);
}

template <typename T, typename A, typename P>
bool
operator==(sa_detail::nullptr_t, safe_allocation<T, A, P> const& x)
{
	return x == nullptr;
}

template <typename T, typename A, typename P>
bool
operator!=(sa_detail::nullptr_t, safe_allocation<T, A, P> const& x)
{
	return !(x == nullptr);
}
}                         

#endif                                              
