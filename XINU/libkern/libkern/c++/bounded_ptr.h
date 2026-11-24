  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_BOUNDED_PTR_H
#define XNU_LIBKERN_LIBKERN_CXX_BOUNDED_PTR_H

#include <stddef.h>
#include <stdint.h>
#include <os/overflow.h>
#include <os/base.h>

#if !defined(__improbable)
#   define __improbable(...) __builtin_expect((__VA_ARGS__), 0)
#endif

namespace libkern {
namespace detail {
                                                                    
                                                                  
                                    
                                                     
template <typename ...> using void_t = void;
template <typename T> T && declval() noexcept;
using nullptr_t = decltype(nullptr);
template <bool Cond, typename T = void> struct enable_if;
template <typename T> struct enable_if<true, T> { using type = T; };
template <bool Cond, typename T = void> using enable_if_t = typename enable_if<Cond, T>::type;
template <typename T1, typename T2>
constexpr bool is_convertible_v = __is_convertible_to(T1, T2);

template <typename T> inline constexpr bool is_void_v = false;
template <> inline constexpr bool is_void_v<void> = true;
template <> inline constexpr bool is_void_v<void const> = true;

template <typename T, typename U> struct copy_const { using type = U; };
template <typename T, typename U> struct copy_const<T const, U> { using type = U const; };
template <typename T, typename U> using copy_const_t = typename copy_const<T, U>::type;

template <typename T, typename U> struct copy_cv { using type = U; };
template <typename T, typename U> struct copy_cv<T const, U> { using type = U const; };
template <typename T, typename U> struct copy_cv<T volatile, U> { using type = U volatile; };
template <typename T, typename U> struct copy_cv<T const volatile, U> { using type = U const volatile; };
template <typename T, typename U> using copy_cv_t = typename copy_cv<T, U>::type;

template <typename T, typename U>
using WhenComparable = void_t<
	decltype(declval<T>() == declval<U>()),
	decltype(declval<T>() != declval<U>())
	>;

template <typename T, typename U>
using WhenOrderable = void_t <
    decltype(declval<T>() < declval<U>()),
decltype(declval<T>() > declval<U>()),
decltype(declval<T>() >= declval<U>()),
decltype(declval<T>() <= declval<U>())
>;

                                                                        
                                      
template <typename T> constexpr size_t sizeof_v = sizeof(T);
template <>           inline constexpr size_t sizeof_v<void> = 1;
template <>           inline constexpr size_t sizeof_v<void const> = 1;
template <>           inline constexpr size_t sizeof_v<void volatile> = 1;
template <>           inline constexpr size_t sizeof_v<void const volatile> = 1;
}                        

                                                                      
                                                                     
                               
  
                                                                                
                                                                         
                                                                        
                                                                           
                                                  
  
                                                                              
                                                                              
                                                     
                                                                             
                                                                                   
  
                                                                             
                                                                        
                                                                        
                                                           
  
                                                                             
                                                                             
                                                                        
                                                                            
                                                                    
  
                             
                             
                                                                         
                                                                           
                                                                            
                                                                        
                                                                            
                                                                      
                                                                  
  
                                                                            
                                                                           
                                                                            
                                                                               
                                                                                    
  
                       
                       
                                                                             
                                                                            
                                                                 
                                                            
template <typename T, typename TrappingPolicy>
struct __attribute__((trivial_abi)) bounded_ptr {
private:
	using CharType = detail::copy_cv_t<T, char>;

public:
	                                
	  
	                                                                        
	                                                                       
	                                                                       
	                                      
	OS_ALWAYS_INLINE constexpr
	bounded_ptr(detail::nullptr_t)
		: base_(nullptr), count_(0), offset_(0)
	{
	}

	OS_ALWAYS_INLINE constexpr
	explicit
	bounded_ptr()
		: bounded_ptr(nullptr)
	{
	}

	                                                                         
	                                                      
	  
	                                                                     
	                                                                             
	                                                    
	  
	                                                                            
	                                                                          
	                                                                             
	OS_ALWAYS_INLINE explicit
	bounded_ptr(T* pointer, T const* begin, T const* end)
	{
		base_ = reinterpret_cast<CharType*>(const_cast<T*>(begin));

		                                                                 
		if (__improbable(os_sub_overflow(reinterpret_cast<uintptr_t>(end),
		    reinterpret_cast<uintptr_t>(begin),
		    &count_))) {
			TrappingPolicy::trap("The range of valid memory is too large to be represented "
			    "by this type, or [begin, end) is not a well-formed range");
		}

		                                                                       
		                                                                   
		                                                                       
		                               
		if (__improbable(os_sub_overflow(reinterpret_cast<uintptr_t>(pointer),
		    reinterpret_cast<uintptr_t>(begin),
		    &offset_))) {
			TrappingPolicy::trap("The offset of the pointer inside its valid memory "
			    "range can't be represented using int32_t");
		}
	}

	                                                                            
	  
	                                                                     
	                                                                  
	                                                                   
	                                                                     
	                                                                    
	  
	                                                                   
	                          
	template <typename U, typename Policy, typename = detail::enable_if_t<detail::is_convertible_v<U*, T*> > >
	OS_ALWAYS_INLINE
	bounded_ptr(bounded_ptr<U, Policy> const & other)
		: base_(other.base_)
		, count_(other.count_)
		, offset_(static_cast<int32_t>(reinterpret_cast<CharType*>(static_cast<T*>(other.get_ptr_())) - other.base_))
	{
	}

	                                                                          
	                                          
	  
	                                                                       
	                                                                   
	                        
	template <typename U, typename Policy, typename = detail::enable_if_t<detail::is_convertible_v<U*, T*> > >
	OS_ALWAYS_INLINE bounded_ptr&
	operator=(bounded_ptr<U, Policy> const& other)
	{
		base_ = other.base_;
		count_ = other.count_;
		offset_ = static_cast<int32_t>(reinterpret_cast<CharType*>(static_cast<T*>(other.get_ptr_())) - other.base_);
		return *this;
	}

	                                
	  
	                                                                    
	                                                                   
	                                                                   
	                                                                    
	                                        
	OS_ALWAYS_INLINE bounded_ptr&
	operator=(detail::nullptr_t)
	{
		*this = bounded_ptr();
		return *this;
	}

	                                                                     
	  
	                                                          
	  
	                                                                   
	                                                                    
	                                                                  
	            
	template <typename T_ = T>                                                                          
	OS_ALWAYS_INLINE T_&
	operator*() const
	{
		if (__improbable(!in_bounds_())) {
			TrappingPolicy::trap("bounded_ptr<T>::operator*: Dereferencing this pointer "
			    "would access memory outside of the bounds set originally");
		}
		return *get_ptr_();
	}

	OS_ALWAYS_INLINE T*
	operator->() const
	{
		if (__improbable(!in_bounds_())) {
			TrappingPolicy::trap("bounded_ptr<T>::operator->: Accessing a member through this pointer "
			    "would access memory outside of the bounds set originally");
		}
		return get_ptr_();
	}

	                                                              
	  
	                                                                       
	                                                                       
	                                                                     
	                                          
	template <typename T_ = T>                                                                          
	OS_ALWAYS_INLINE T_&
	operator[](ptrdiff_t n) const
	{
		return *(*this + n);
	}

	                                                                         
	              
	  
	                                                                         
	                                                        
	OS_ALWAYS_INLINE T*
	discard_bounds() const
	{
		if (__improbable(!in_bounds_())) {
			TrappingPolicy::trap("bounded_ptr<T>::discard_bounds: Discarding the bounds on "
			    "this pointer would lose the fact that it is outside of the "
			    "bounds set originally");
		}
		return get_ptr_();
	}

	                                                                          
	                                
	  
	                                                                       
	                                                                         
	                                                                       
	                                                                         
	                                                                
	                                                                    
	                                                                  
	                                                                   
	                                                                        
	                                                                     
	OS_ALWAYS_INLINE T*
	unsafe_discard_bounds() const
	{
		return get_ptr_();
	}

	                                                                      
	  
	                                                                
	OS_ALWAYS_INLINE explicit
	operator bool() const
	{
		return get_ptr_() != nullptr;
	}

	                                       
	  
	                                                                        
	                                                                        
	OS_ALWAYS_INLINE bounded_ptr&
	operator++()
	{
		*this += 1;
		return *this;
	}
	OS_ALWAYS_INLINE bounded_ptr
	operator++(int)
	{
		bounded_ptr old = *this;
		++*this;
		return old;
	}
	OS_ALWAYS_INLINE bounded_ptr&
	operator--()
	{
		*this -= 1;
		return *this;
	}
	OS_ALWAYS_INLINE bounded_ptr
	operator--(int)
	{
		bounded_ptr old = *this;
		--*this;
		return old;
	}

	                                                            
	  
	                                                                      
	                                                                   
	                                                                      
	                                                                       
	                                                                
	  
	                                                                      
	                                                                        
	                                                                           
	                                                      
	  
	                                                                    
	                                                                        
	                                                                         
	                                                         
	template <typename T_ = T>
	OS_ALWAYS_INLINE bounded_ptr&
	operator+=(ptrdiff_t n)
	{
		static_assert(!detail::is_void_v<T_>, "Arithmetic on bounded_ptr<void> is not allowed.");

		ptrdiff_t bytes;
		if (__improbable(os_mul_overflow(n, sizeof(T), &bytes))) {
			TrappingPolicy::trap(
				"bounded_ptr<T>::operator+=(n): Calculating the number of bytes to "
				"add to the offset (n * sizeof(T)) would trigger an overflow");
		}
		if (__improbable(os_add_overflow(offset_, bytes, &offset_))) {
			TrappingPolicy::trap(
				"bounded_ptr<T>::operator+=(n): Adding the specified number of bytes "
				"to the offset representing the current position would overflow.");
		}
		return *this;
	}

	template <typename T_ = T>
	OS_ALWAYS_INLINE bounded_ptr&
	operator-=(ptrdiff_t n)
	{
		static_assert(!detail::is_void_v<T_>, "Arithmetic on bounded_ptr<void> is not allowed.");

		ptrdiff_t bytes;
		if (__improbable(os_mul_overflow(n, sizeof(T), &bytes))) {
			TrappingPolicy::trap(
				"bounded_ptr<T>::operator-=(n): Calculating the number of bytes to "
				"subtract from the offset (n * sizeof(T)) would trigger an overflow");
		}
		if (__improbable(os_sub_overflow(offset_, bytes, &offset_))) {
			TrappingPolicy::trap(
				"bounded_ptr<T>::operator-=(n): Subtracting the specified number of bytes "
				"from the offset representing the current position would overflow.");
		}
		return *this;
	}

	friend OS_ALWAYS_INLINE bounded_ptr
	operator+(bounded_ptr p, ptrdiff_t n)
	{
		p += n;
		return p;
	}
	friend OS_ALWAYS_INLINE bounded_ptr
	operator+(ptrdiff_t n, bounded_ptr p)
	{
		p += n;
		return p;
	}
	friend OS_ALWAYS_INLINE bounded_ptr
	operator-(bounded_ptr p, ptrdiff_t n)
	{
		p -= n;
		return p;
	}

	                                                     
	  
	                                                                    
	                                                                 
	             
	friend OS_ALWAYS_INLINE ptrdiff_t
	operator-(bounded_ptr const& a, bounded_ptr const& b)
	{
		return a.get_ptr_() - b.get_ptr_();
	}

	friend OS_ALWAYS_INLINE ptrdiff_t
	operator-(bounded_ptr const& a, T const* b)
	{
		return a.get_ptr_() - b;
	}

	friend OS_ALWAYS_INLINE ptrdiff_t
	operator-(T const* a, bounded_ptr const& b)
	{
		return a - b.get_ptr_();
	}

private:
	OS_ALWAYS_INLINE bool
	in_bounds_() const
	{
		static_assert(detail::sizeof_v<T> <= UINT32_MAX - INT32_MAX,
		    "The type pointed-to by bounded_ptr is too large, which would defeat "
		    "our optimization to check for inboundedness using arithmetic on unsigned");
		return offset_ >= 0 && static_cast<uint32_t>(offset_) + static_cast<uint32_t>(detail::sizeof_v<T>) <= count_;
	}

	OS_ALWAYS_INLINE T*
	get_ptr_() const
	{
		                                                 
		uintptr_t ptr;
		if (__improbable(os_add_overflow(reinterpret_cast<uintptr_t>(base_), offset_, &ptr))) {
			TrappingPolicy::trap("This bounded_ptr is pointing to memory outside of what can "
			    "be represented by a native pointer.");
		}
		return reinterpret_cast<T*>(ptr);
	}

	template <typename T_, typename U, typename Policy>
	friend bounded_ptr<T_, Policy> reinterpret_pointer_cast(bounded_ptr<U, Policy> const&) noexcept;

	template <typename U, typename P> friend struct bounded_ptr;                                             

	CharType* base_;                                                       
	uint32_t count_;                                                       
	int32_t offset_;                                           
};

                                                               
  
                                                                         
                                                                        
                                                                         
                   
  
                                                                     
                                                                       
                                                                  
                                                      
template <typename T, typename P1, typename U, typename P2, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator==(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return a.unsafe_discard_bounds() == b.unsafe_discard_bounds();
}

template <typename T, typename P1, typename U, typename P2, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator!=(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return !(a == b);
}

template <typename T, typename P, typename U, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator==(bounded_ptr<T, P> const& a, U* b)
{
	return a.unsafe_discard_bounds() == b;
}

template <typename T, typename P, typename U, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator==(U* a, bounded_ptr<T, P> const& b)
{
	return a == b.unsafe_discard_bounds();
}

template <typename T, typename P, typename U, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator!=(bounded_ptr<T, P> const& a, U* b)
{
	return !(a == b);
}

template <typename T, typename P, typename U, typename = detail::WhenComparable<T*, U*> >
OS_ALWAYS_INLINE bool
operator!=(U* a, bounded_ptr<T, P> const& b)
{
	return !(a == b);
}

template <typename T, typename Policy>
OS_ALWAYS_INLINE bool
operator==(detail::nullptr_t, bounded_ptr<T, Policy> const& p)
{
	return p.unsafe_discard_bounds() == nullptr;
}

template <typename T, typename Policy>
OS_ALWAYS_INLINE bool
operator!=(detail::nullptr_t, bounded_ptr<T, Policy> const& p)
{
	return p.unsafe_discard_bounds() != nullptr;
}

template <typename T, typename Policy>
OS_ALWAYS_INLINE bool
operator==(bounded_ptr<T, Policy> const& p, detail::nullptr_t)
{
	return p.unsafe_discard_bounds() == nullptr;
}

template <typename T, typename Policy>
OS_ALWAYS_INLINE bool
operator!=(bounded_ptr<T, Policy> const& p, detail::nullptr_t)
{
	return p.unsafe_discard_bounds() != nullptr;
}

                                                                           
                                                                             
                                 
  
                                                                        
                                                                          
                                                                            
                  
  
                                                                   
                                                                    
                                                                  
                                                     
  

template <typename T, typename U, typename P1, typename P2, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return a.unsafe_discard_bounds() < b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P1, typename P2, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<=(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return a.unsafe_discard_bounds() <= b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P1, typename P2, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return a.unsafe_discard_bounds() > b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P1, typename P2, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>=(bounded_ptr<T, P1> const& a, bounded_ptr<U, P2> const& b)
{
	return a.unsafe_discard_bounds() >= b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<(T* a, bounded_ptr<U, P> const& b)
{
	return a < b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<(bounded_ptr<T, P> const& a, U* b)
{
	return a.unsafe_discard_bounds() < b;
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<=(T* a, bounded_ptr<U, P> const& b)
{
	return a <= b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator<=(bounded_ptr<T, P> const& a, U* b)
{
	return a.unsafe_discard_bounds() <= b;
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>(T* a, bounded_ptr<U, P> const& b)
{
	return a > b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>(bounded_ptr<T, P> const& a, U* b)
{
	return a.unsafe_discard_bounds() > b;
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>=(T* a, bounded_ptr<U, P> const& b)
{
	return a >= b.unsafe_discard_bounds();
}

template <typename T, typename U, typename P, typename = detail::WhenOrderable<T*, U*> >
OS_ALWAYS_INLINE bool
operator>=(bounded_ptr<T, P> const& a, U* b)
{
	return a.unsafe_discard_bounds() >= b;
}

template <typename T, typename U>
OS_ALWAYS_INLINE T*
reinterpret_pointer_cast(U* p) noexcept
{
	return reinterpret_cast<T*>(p);
}

                                                                               
  
                                                                             
                                                                            
                                                                              
                                                                                
                               
template <typename T, typename U, typename Policy>
OS_ALWAYS_INLINE bounded_ptr<T, Policy>
reinterpret_pointer_cast(bounded_ptr<U, Policy> const& p) noexcept
{
	using CharType = detail::copy_cv_t<T, char>;
	CharType* new_begin = reinterpret_cast<CharType*>(p.base_);
	CharType* new_end = new_begin + p.count_;
	return bounded_ptr<T, Policy>(reinterpret_cast<T*>(p.get_ptr_()),
	           reinterpret_cast<T const*>(new_begin),
	           reinterpret_cast<T const*>(new_end));
}
}                         

#endif                                          
