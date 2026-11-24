  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_INTRUSIVE_SHARED_PTR_H
#define XNU_LIBKERN_LIBKERN_CXX_INTRUSIVE_SHARED_PTR_H

namespace libkern {
namespace isp_detail {
                                                                                
using nullptr_t = decltype(nullptr);

template <typename T> T && declval() noexcept;

template <typename ...> using void_t = void;

template <typename T> struct is_lvalue_reference { static constexpr bool value = false; };
template <typename T> struct is_lvalue_reference<T&> { static constexpr bool value = true; };
template <typename T> constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

template <typename T> constexpr bool is_empty_v = __is_empty(T);

template <typename T> struct remove_reference { using type = T; };
template <typename T> struct remove_reference<T&> { using type = T; };
template <typename T> struct remove_reference<T &&> { using type = T; };
template <typename T> using remove_reference_t = typename remove_reference<T>::type;

template <bool Cond, typename T = void> struct enable_if;
template <typename T> struct enable_if<true, T> { using type = T; };
template <bool Cond, typename T = void> using enable_if_t = typename enable_if<Cond, T>::type;

template <typename From, typename To> constexpr bool is_convertible_v = __is_convertible_to(From, To);

template <typename T>
constexpr T && forward(remove_reference_t<T>&t) noexcept {
	return static_cast<T &&>(t);
}

template <typename T>
constexpr T && forward(remove_reference_t<T>&& t) noexcept {
	static_assert(!is_lvalue_reference_v<T>,
	    "can not forward an rvalue as an lvalue");
	return static_cast<T &&>(t);
}

template <typename T>
constexpr remove_reference_t<T>&& move(T && t) noexcept {
	using RvalueRef = remove_reference_t<T>&&;
	return static_cast<RvalueRef>(t);
}

template <typename T, typename U>
using WhenComparable = void_t<
	decltype(declval<T>() == declval<U>()),
	decltype(declval<T>() != declval<U>())
	>;
}                            

struct no_retain_t {
	explicit constexpr no_retain_t()
	{
	}
};
struct retain_t {
	explicit constexpr retain_t()
	{
	}
};
inline constexpr no_retain_t no_retain{};
inline constexpr retain_t retain{};

                                                
  
                                                                       
                                                                
                                                                
                            
  
                                                                             
  
                                               
                                                
  
                                                                             
                                                                              
                                                                          
                                                                         
                                                                          
                                                                         
                                  
  
                                                                           
                                                                            
                                                                               
          
  
                                                                            
                                                                             
template <typename T, typename RefcountPolicy>
struct __attribute__((trivial_abi)) intrusive_shared_ptr {
	static_assert(isp_detail::is_empty_v<RefcountPolicy>,
	    "intrusive_shared_ptr only allows a stateless RefcountPolicy "
	    "because it must be ABI compatible with raw pointers.");

	                                                                   

	using pointer = T *;
	using element_type = T;

	                                    
	  
	                                                                     
	                                         
	constexpr intrusive_shared_ptr() noexcept : ptr_(nullptr) {
	}
	constexpr intrusive_shared_ptr(isp_detail::nullptr_t) noexcept : ptr_(nullptr) {
	}

	                                                                    
	                            
	  
	                                                                    
	                                                                     
	                                                                        
	                                                                       
	                                           
	  
	                                                                        
	                                       
	explicit
	intrusive_shared_ptr(pointer p, retain_t) noexcept : ptr_(p) {
		if (ptr_ != nullptr) {
			RefcountPolicy::retain(*ptr_);
		}
	}

	                                                                        
	                                
	  
	                                                                    
	                                                                     
	                                                                
	                                                                   
	                         
	  
	                                                                      
	                                                                 
	explicit constexpr
	intrusive_shared_ptr(pointer p, no_retain_t) noexcept : ptr_(p) {
	}

	                                                               
	  
	                                                                   
	                                                                
	                             
	intrusive_shared_ptr(intrusive_shared_ptr const & other) : ptr_(other.ptr_) {
		if (ptr_ != nullptr) {
			RefcountPolicy::retain(*ptr_);
		}
	}

	                                                                           
	                           
	  
	                                                                     
	                                                                  
	                                  
	  
	                                                                   
	                                                                
	                             
	template <typename U, typename = isp_detail::enable_if_t<isp_detail::is_convertible_v<U*, T*> > >
	intrusive_shared_ptr(intrusive_shared_ptr<U, RefcountPolicy> const & other) : ptr_(other.ptr_) {
		if (ptr_ != nullptr) {
			RefcountPolicy::retain(*ptr_);
		}
	}

	                                                               
	  
	                                                                 
	                                                                      
	                                                                 
	constexpr intrusive_shared_ptr(intrusive_shared_ptr && other) noexcept : ptr_(other.ptr_) {
		other.ptr_ = nullptr;
	}

	                                                             
	                 
	  
	                                                                     
	                                                                  
	                                  
	  
	                                                                 
	                                                                      
	                                                                 
	template <typename U, typename = isp_detail::enable_if_t<isp_detail::is_convertible_v<U*, T*> > >
	constexpr intrusive_shared_ptr(intrusive_shared_ptr<U, RefcountPolicy>&& other) noexcept : ptr_(other.ptr_) {
		other.ptr_ = nullptr;
	}

	                             
	  
	                                                                         
	                                                                       
	                                    
	  
	                                                                    
	                                                                    
	                                                           
	~intrusive_shared_ptr() {
		reset();
	}

	                                 
	  
	                                                                   
	                                                                
	                             
	  
	                                                                    
	                                                                   
	                               
	intrusive_shared_ptr&
	operator=(intrusive_shared_ptr const& other)
	{
		reset(other.get(), retain);
		return *this;
	}

	                                                                
	  
	                                                                         
	                                                                  
	                                  
	  
	                                                                   
	                                                                
	                             
	  
	                                                                    
	                                                                   
	                               
	template <typename U, typename = isp_detail::enable_if_t<isp_detail::is_convertible_v<U*, T*> > >
	intrusive_shared_ptr&
	operator=(intrusive_shared_ptr<U, RefcountPolicy> const& other)
	{
		reset(other.get(), retain);
		return *this;
	}

	                                 
	  
	                                                                 
	                                                                      
	                                                                 
	  
	                                                                    
	                                                                   
	                               
	intrusive_shared_ptr&
	operator=(intrusive_shared_ptr&& other)
	{
		reset(other.get(), no_retain);
		other.ptr_ = nullptr;
		return *this;
	}

	                                                                
	  
	                                                                         
	                                                                  
	                                  
	  
	                                                                 
	                                                                      
	                                                                 
	  
	                                                                    
	                                                                   
	                               
	template <typename U, typename = isp_detail::enable_if_t<isp_detail::is_convertible_v<U*, T*> > >
	intrusive_shared_ptr&
	operator=(intrusive_shared_ptr<U, RefcountPolicy>&& other)
	{
		reset(other.get(), no_retain);
		other.ptr_ = nullptr;
		return *this;
	}

	                                                                      
	  
	                                                                    
	                                                                   
	                               
	intrusive_shared_ptr&
	operator=(isp_detail::nullptr_t) noexcept
	{
		reset();
		return *this;
	}

	                                                                      
	constexpr T&
	operator*() const noexcept
	{
		return *ptr_;
	}
	constexpr pointer
	operator->() const noexcept
	{
		return ptr_;
	}

	                                                                             
	explicit constexpr
	operator bool() const noexcept
	{
		return ptr_ != nullptr;
	}

	                                 
	  
	                                                                  
	                              
	intrusive_shared_ptr&
	reset() noexcept
	{
		if (ptr_ != nullptr) {
			RefcountPolicy::release(*ptr_);
		}
		ptr_ = nullptr;
		return *this;
	}

	                                                                        
	  
	                                                                          
	                                  
	  
	                                                                         
	                                              
	intrusive_shared_ptr&
	reset(pointer p, no_retain_t) noexcept
	{
		if (ptr_ != nullptr) {
			RefcountPolicy::release(*ptr_);
		}
		ptr_ = p;
		return *this;
	}

	                                                                        
	  
	                                                                  
	                                  
	  
	                                                                         
	                                              
	intrusive_shared_ptr&
	reset(pointer p, retain_t) noexcept
	{
		                                                                    
		pointer old = ptr_;
		ptr_ = p;
		if (ptr_ != nullptr) {
			RefcountPolicy::retain(*ptr_);
		}
		if (old != nullptr) {
			RefcountPolicy::release(*old);
		}
		return *this;
	}

	                                                      
	  
	                                                                         
	                                                                      
	                                                                             
	  
	                                                                       
	                                     
	  
	                                                                     
	                                                                       
	                                                                         
	                                                                         
	constexpr pointer
	get() const & noexcept
	{
		return ptr_;
	}

	constexpr pointer
	    get() const&& noexcept = delete;

	                                                                   
	                                                
	  
	                                                                   
	                                                                      
	                                                                     
	                                                                  
	                                      
	  
	                                                                     
	                                                                       
	               
	  
	                                                                      
	                            
	constexpr pointer
	detach() noexcept
	{
		pointer tmp = ptr_;
		ptr_ = nullptr;
		return tmp;
	}

private:
	friend constexpr void
	swap(intrusive_shared_ptr& a, intrusive_shared_ptr& b) noexcept
	{
		pointer tmp = a.ptr_;
		a.ptr_ = b.ptr_;
		b.ptr_ = tmp;
	}

	                                                    
	template <typename U, typename Policy>
	friend struct intrusive_shared_ptr;

	pointer ptr_;
};

                                                                         
                                                      
  
                                                                          
                                                                             
                                                                             
                            
  
                                                                       
                                                                              
                                                                            
                                
template <typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
static_pointer_cast(intrusive_shared_ptr<From, R> const& ptr)
{
	return intrusive_shared_ptr<To, R>(static_cast<To*>(ptr.get()), retain);
}
template <typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
static_pointer_cast(intrusive_shared_ptr<From, R>&& ptr)
{
	return intrusive_shared_ptr<To, R>(static_cast<To*>(ptr.detach()), no_retain);
}

                                                                         
                                                                        
                
  
                                                                          
                                                                             
                                                                             
                            
  
                                                                       
                                                                              
                                                                            
                                
template <typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
const_pointer_cast(intrusive_shared_ptr<From, R> const& ptr) noexcept
{
	return intrusive_shared_ptr<To, R>(const_cast<To*>(ptr.get()), retain);
}
template <typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
const_pointer_cast(intrusive_shared_ptr<From, R>&& ptr) noexcept
{
	return intrusive_shared_ptr<To, R>(const_cast<To*>(ptr.detach()), no_retain);
}

                                                                         
                                                           
  
                                                                          
                                                                             
                                                                             
                            
  
                                                                       
                                                                              
                                                                            
                                
  
           
                                                                            
                                                                            
                                                                          
                 
template<typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
reinterpret_pointer_cast(intrusive_shared_ptr<From, R> const& ptr) noexcept
{
	return intrusive_shared_ptr<To, R>(reinterpret_cast<To*>(ptr.get()), retain);
}
template<typename To, typename From, typename R>
intrusive_shared_ptr<To, R>
reinterpret_pointer_cast(intrusive_shared_ptr<From, R>&& ptr) noexcept
{
	return intrusive_shared_ptr<To, R>(reinterpret_cast<To*>(ptr.detach()), no_retain);
}

                                 
                        
                                   
                                       
template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator==(intrusive_shared_ptr<T, R> const& x, intrusive_shared_ptr<U, R> const& y)
{
	return x.get() == y.get();
}

template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator!=(intrusive_shared_ptr<T, R> const& x, intrusive_shared_ptr<U, R> const& y)
{
	return x.get() != y.get();
}

template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator==(intrusive_shared_ptr<T, R> const& x, U* y)
{
	return x.get() == y;
}

template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator!=(intrusive_shared_ptr<T, R> const& x, U* y)
{
	return x.get() != y;
}

template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator==(T* x, intrusive_shared_ptr<U, R> const& y)
{
	return x == y.get();
}

template <typename T, typename U, typename R, typename = isp_detail::WhenComparable<T*, U*> >
bool
operator!=(T* x, intrusive_shared_ptr<U, R> const& y)
{
	return x != y.get();
}

template <typename T, typename R>
bool
operator==(intrusive_shared_ptr<T, R> const& x, isp_detail::nullptr_t) noexcept
{
	return x.get() == nullptr;
}

template <typename T, typename R>
bool
operator==(isp_detail::nullptr_t, intrusive_shared_ptr<T, R> const& x) noexcept
{
	return nullptr == x.get();
}

template <typename T, typename R>
bool
operator!=(intrusive_shared_ptr<T, R> const& x, isp_detail::nullptr_t) noexcept
{
	return x.get() != nullptr;
}

template <typename T, typename R>
bool
operator!=(isp_detail::nullptr_t, intrusive_shared_ptr<T, R> const& x) noexcept
{
	return nullptr != x.get();
}
}                         

#endif                                                   
