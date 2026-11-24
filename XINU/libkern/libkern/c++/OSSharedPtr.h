  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_OS_SHARED_PTR_H
#define XNU_LIBKERN_LIBKERN_CXX_OS_SHARED_PTR_H

#include <libkern/c++/intrusive_shared_ptr.h>
#include <libkern/c++/OSMetaClass.h>

struct intrusive_osobject_retainer {
	static void
	retain(OSMetaClassBase const& obj)
	{
		obj.retain();
	}
	static void
	release(OSMetaClassBase const& obj)
	{
		obj.release();
	}
};

template <typename Tag>
struct intrusive_tagged_osobject_retainer {
	static void
	retain(OSMetaClassBase const& obj)
	{
		obj.taggedRetain(OSTypeID(Tag));
	}
	static void
	release(OSMetaClassBase const& obj)
	{
		obj.taggedRelease(OSTypeID(Tag));
	}
};

inline constexpr auto OSNoRetain = libkern::no_retain;
inline constexpr auto OSRetain = libkern::retain;

template <typename T>
class __attribute__((trivial_abi)) OSSharedPtr: public libkern::intrusive_shared_ptr<T, intrusive_osobject_retainer> {
	using libkern::intrusive_shared_ptr<T, intrusive_osobject_retainer>::intrusive_shared_ptr;
};

template <typename T, typename Tag>
class __attribute__((trivial_abi)) OSTaggedSharedPtr: public libkern::intrusive_shared_ptr<T, intrusive_tagged_osobject_retainer<Tag> > {
	using libkern::intrusive_shared_ptr<T, intrusive_tagged_osobject_retainer<Tag> >::intrusive_shared_ptr;
};

template <typename T>
OSSharedPtr<T>
OSMakeShared()
{
	T* memory = OSTypeAlloc(T);
	                                                                     
	                                               
	return OSSharedPtr<T>(memory, OSNoRetain);
}

template <typename Destination, typename Source>
OSSharedPtr<Destination>
OSDynamicPtrCast(OSSharedPtr<Source> const& source)
{
	Destination* raw = OSDynamicCast(Destination, source.get());
	if (raw == nullptr) {
		return nullptr;
	} else {
		OSSharedPtr<Destination> dest(raw, OSRetain);
		return dest;
	}
}

template <typename Destination, typename Source>
OSSharedPtr<Destination>
OSDynamicPtrCast(OSSharedPtr<Source> && source)
{
	Destination* raw = OSDynamicCast(Destination, source.get());
	if (raw == nullptr) {
		return nullptr;
	} else {
		OSSharedPtr<Destination> dest(raw, OSNoRetain);
		source.detach();                        
		return dest;
	}
}

template <typename Destination, typename Tag, typename Source>
OSTaggedSharedPtr<Destination, Tag>
OSDynamicPtrCast(OSTaggedSharedPtr<Source, Tag> const& source)
{
	Destination* raw = OSDynamicCast(Destination, source.get());
	if (raw == nullptr) {
		return nullptr;
	} else {
		OSTaggedSharedPtr<Destination, Tag> dest(raw, OSRetain);
		return dest;
	}
}

template <typename To, typename From>
OSSharedPtr<To>
OSStaticPtrCast(OSSharedPtr<From> const& ptr) noexcept
{
	return OSSharedPtr<To>(static_cast<To*>(ptr.get()), OSRetain);
}

template <typename To, typename From>
OSSharedPtr<To>
OSStaticPtrCast(OSSharedPtr<From>&& ptr) noexcept
{
	return OSSharedPtr<To>(static_cast<To*>(ptr.detach()), OSNoRetain);
}

template <typename To, typename From>
OSSharedPtr<To>
OSConstPtrCast(OSSharedPtr<From> const& ptr) noexcept
{
	return OSSharedPtr<To>(const_cast<To*>(ptr.get()), OSRetain);
}

template <typename To, typename From>
OSSharedPtr<To>
OSConstPtrCast(OSSharedPtr<From>&& ptr) noexcept
{
	return OSSharedPtr<To>(const_cast<To*>(ptr.detach()), OSNoRetain);
}

#endif                                            
