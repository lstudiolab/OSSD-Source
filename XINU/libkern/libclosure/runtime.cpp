/*
 * runtime.c
 * libclosure
 *
 * Copyright (c) 2008-2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LLVM_LICENSE_HEADER@
 */


#ifndef KERNEL

#include "Block_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <os/assumes.h>

#else              
#define TARGET_OS_WIN32 0

#include <libkern/Block_private.h>
__BEGIN_DECLS
#include <kern/kalloc.h>
__END_DECLS

static inline void *
malloc(size_t size)
{
	if (size == 0) {
		return NULL;
	}
	return kheap_alloc_tag_bt(KHEAP_DEFAULT, size,
	           (zalloc_flags_t) (Z_WAITOK | Z_ZERO), VM_KERN_MEMORY_LIBKERN);
}

static inline void
free(void *addr)
{
	kheap_free_addr(KHEAP_DEFAULT, addr);
}

#endif             

#include <machine/atomic.h>
#include <string.h>
#include <stdint.h>
#ifndef os_assumes
#define os_assumes(_x) (_x)
#endif
#ifndef os_assert
#define os_assert(_x) assert(_x)
#endif

#if TARGET_OS_WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
static __inline bool
OSAtomicCompareAndSwapLong(long oldl, long newl, long volatile *dst)
{
	                                             
	long original = InterlockedCompareExchange(dst, newl, oldl);
	return original == oldl;
}

static __inline bool
OSAtomicCompareAndSwapInt(int oldi, int newi, int volatile *dst)
{
	                                             
	int original = InterlockedCompareExchange(dst, newi, oldi);
	return original == oldi;
}
#else
#define OSAtomicCompareAndSwapLong(_Old, _New, _Ptr) os_atomic_cmpxchg(_Ptr, _Old, _New, relaxed)
#define OSAtomicCompareAndSwapInt(_Old, _New, _Ptr) os_atomic_cmpxchg(_Ptr, _Old, _New, relaxed)
#endif


                                                                                
                      
                                                                                  

static int32_t
latching_incr_int(volatile int32_t *where)
{
	while (1) {
		int32_t old_value = *where;
		if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
			return BLOCK_REFCOUNT_MASK;
		}
		if (OSAtomicCompareAndSwapInt(old_value, old_value + 2, where)) {
			return old_value + 2;
		}
	}
}

static bool
latching_incr_int_not_deallocating(volatile int32_t *where)
{
	while (1) {
		int32_t old_value = *where;
		if (old_value & BLOCK_DEALLOCATING) {
			                                   
			return false;
		}
		if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
			                                                       
			return true;
		}
		if (OSAtomicCompareAndSwapInt(old_value, old_value + 2, where)) {
			                                                                                 
			return true;
		}
	}
}


                            
static bool
latching_decr_int_should_deallocate(volatile int32_t *where)
{
	while (1) {
		int32_t old_value = *where;
		if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
			return false;                
		}
		if ((old_value & BLOCK_REFCOUNT_MASK) == 0) {
			return false;                        
		}
		int32_t new_value = old_value - 2;
		bool result = false;
		if ((old_value & (BLOCK_REFCOUNT_MASK | BLOCK_DEALLOCATING)) == 2) {
			new_value = old_value - 1;
			result = true;
		}
		if (OSAtomicCompareAndSwapInt(old_value, new_value, where)) {
			return result;
		}
	}
}


                                                                           
                                                                   
                                                                             
#if !TARGET_OS_WIN32
#pragma mark Framework Callback Routines
#endif
#if KERNEL
static inline void
_Block_retain_object(const void *ptr __unused)
{
}

static inline void
_Block_release_object(const void *ptr __unused)
{
}

static inline void
_Block_destructInstance(const void *aBlock __unused)
{
}

#else

static void
_Block_retain_object_default(const void *ptr __unused)
{
}

static void
_Block_release_object_default(const void *ptr __unused)
{
}

static void
_Block_destructInstance_default(const void *aBlock __unused)
{
}

static void (*_Block_retain_object)(const void *ptr) = _Block_retain_object_default;
static void (*_Block_release_object)(const void *ptr) = _Block_release_object_default;
static void (*_Block_destructInstance) (const void *aBlock) = _Block_destructInstance_default;


                                                                           
                                                              
                                                                             

void
_Block_use_RR2(const Block_callbacks_RR *callbacks)
{
	_Block_retain_object = callbacks->retain;
	_Block_release_object = callbacks->release;
	_Block_destructInstance = callbacks->destructInstance;
}
#endif           

                                                                             
                                         
                                                                               

template <class T>
static T *
unwrap_relative_pointer(int32_t &offset)
{
	if (offset == 0) {
		return nullptr;
	}

	uintptr_t base = (uintptr_t)&offset;
	uintptr_t extendedOffset = (uintptr_t)(intptr_t)offset;
	uintptr_t pointer = base + extendedOffset;
	return (T *)pointer;
}

#if 0
static struct Block_descriptor_2 *
_Block_descriptor_2(struct Block_layout *aBlock)
{
	uint8_t *desc = (uint8_t *)_Block_get_descriptor(aBlock);
	desc += sizeof(struct Block_descriptor_1);
	return __IGNORE_WCASTALIGN((struct Block_descriptor_2 *)desc);
}
#endif

static struct Block_descriptor_3 *
_Block_descriptor_3(struct Block_layout *aBlock)
{
	uint8_t *desc = (uint8_t *)_Block_get_descriptor(aBlock);
	desc += sizeof(struct Block_descriptor_1);
	if (aBlock->flags & BLOCK_HAS_COPY_DISPOSE) {
		desc += sizeof(struct Block_descriptor_2);
	}
	return __IGNORE_WCASTALIGN((struct Block_descriptor_3 *)desc);
}

static void
_Block_call_copy_helper(void *result, struct Block_layout *aBlock)
{
	if (auto *pFn = _Block_get_copy_function(aBlock)) {
		pFn(result, aBlock);
	}
}

static void
_Block_call_dispose_helper(struct Block_layout *aBlock)
{
	if (auto *pFn = _Block_get_dispose_function(aBlock)) {
		pFn(aBlock);
	}
}

                                                                                
                                         
                                                                                  

#if !TARGET_OS_WIN32
#pragma mark Copy/Release support
#endif

                                                                                           
void *
_Block_copy(const void *arg)
{
	struct Block_layout *aBlock;

	if (!arg) {
		return NULL;
	}

	                                                           
	aBlock = (struct Block_layout *)arg;
	if (aBlock->flags & BLOCK_NEEDS_FREE) {
		                  
		latching_incr_int(&aBlock->flags);
		return aBlock;
	} else if (aBlock->flags & BLOCK_IS_GLOBAL) {
		return aBlock;
	} else {
		                                   
		size_t size = Block_size(aBlock);
		struct Block_layout *result = (struct Block_layout *)malloc(size);
		if (!result) {
			return NULL;
		}
		memmove(result, aBlock, size);                 
#if __has_feature(ptrauth_calls)
		                                                               
		result->invoke = aBlock->invoke;

#if __has_feature(ptrauth_signed_block_descriptors)
		uintptr_t oldDesc =
		    ptrauth_blend_discriminator(
			&aBlock->descriptor, _Block_descriptor_ptrauth_discriminator);
		uintptr_t newDesc =
		    ptrauth_blend_discriminator(
			&result->descriptor, _Block_descriptor_ptrauth_discriminator);

		result->descriptor =
		    ptrauth_auth_and_resign(aBlock->descriptor, ptrauth_key_asda, oldDesc,
		    ptrauth_key_asda, newDesc);
#endif
#endif

		                 
		result->flags &= ~(BLOCK_REFCOUNT_MASK | BLOCK_DEALLOCATING);                  
		result->flags |= BLOCK_NEEDS_FREE | 2;                      
		_Block_call_copy_helper(result, aBlock);
		                                                                        
		result->isa = _NSConcreteMallocBlock;
		return result;
	}
}


                                                                                   

                                                                                                                
                                                                                                                             
                                                                                                                          
                                                                       
static struct Block_byref *
_Block_byref_copy(const void *arg)
{
	struct Block_byref *src = (struct Block_byref *)arg;

	if ((src->forwarding->flags & BLOCK_REFCOUNT_MASK) == 0) {
		                      
		struct Block_byref *copy = (struct Block_byref *)malloc(src->size);
		copy->isa = NULL;
		                                                                        
		copy->flags = src->flags | BLOCK_BYREF_NEEDS_FREE | 4;
		copy->forwarding = copy;                                      
		src->forwarding = copy;                                     
		copy->size = src->size;

		if (src->flags & BLOCK_BYREF_HAS_COPY_DISPOSE) {
			                                                   
			                                                                     
			struct Block_byref_2 *src2 = (struct Block_byref_2 *)(src + 1);
			struct Block_byref_2 *copy2 = (struct Block_byref_2 *)(copy + 1);
			copy2->byref_keep = src2->byref_keep;
			copy2->byref_destroy = src2->byref_destroy;

			if (src->flags & BLOCK_BYREF_LAYOUT_EXTENDED) {
				struct Block_byref_3 *src3 = (struct Block_byref_3 *)(src2 + 1);
				struct Block_byref_3 *copy3 = (struct Block_byref_3*)(copy2 + 1);
				copy3->layout = src3->layout;
			}

			(*src2->byref_keep)(copy, src);
		} else {
			                
			                                            
			memmove(copy + 1, src + 1, src->size - sizeof(*src));
		}
	}
	                         
	else if ((src->forwarding->flags & BLOCK_BYREF_NEEDS_FREE) == BLOCK_BYREF_NEEDS_FREE) {
		latching_incr_int(&src->forwarding->flags);
	}

	return src->forwarding;
}

static void
_Block_byref_release(const void *arg)
{
	struct Block_byref *byref = (struct Block_byref *)arg;

	                                                                                         
	byref = byref->forwarding;

	if (byref->flags & BLOCK_BYREF_NEEDS_FREE) {
		__assert_only int32_t refcount = byref->flags & BLOCK_REFCOUNT_MASK;
		os_assert(refcount);
		if (latching_decr_int_should_deallocate(&byref->flags)) {
			if (byref->flags & BLOCK_BYREF_HAS_COPY_DISPOSE) {
				struct Block_byref_2 *byref2 = (struct Block_byref_2 *)(byref + 1);
				(*byref2->byref_destroy)(byref);
			}
			free(byref);
		}
	}
}


                                                             
  
                     
                                                        
  
                                                             

#if !TARGET_OS_WIN32
#pragma mark SPI/API
#endif


                                            
void
_Block_release(const void *arg)
{
	struct Block_layout *aBlock = (struct Block_layout *)arg;
	if (!aBlock) {
		return;
	}
	if (aBlock->flags & BLOCK_IS_GLOBAL) {
		return;
	}
	if (!(aBlock->flags & BLOCK_NEEDS_FREE)) {
		return;
	}

	if (latching_decr_int_should_deallocate(&aBlock->flags)) {
		_Block_call_dispose_helper(aBlock);
		_Block_destructInstance(aBlock);
		free(aBlock);
	}
}

bool
_Block_tryRetain(const void *arg)
{
	struct Block_layout *aBlock = (struct Block_layout *)arg;
	return latching_incr_int_not_deallocating(&aBlock->flags);
}

bool
_Block_isDeallocating(const void *arg)
{
	struct Block_layout *aBlock = (struct Block_layout *)arg;
	return (aBlock->flags & BLOCK_DEALLOCATING) != 0;
}


                                                             
  
                           
  
                                                             

size_t
Block_size(void *aBlock)
{
	auto *layout = (Block_layout *)aBlock;
	void *desc = _Block_get_descriptor(layout);
	if (layout->flags & BLOCK_SMALL_DESCRIPTOR) {
		return ((Block_descriptor_small *)desc)->size;
	}
	return ((Block_descriptor_1 *)desc)->size;
}

bool
_Block_use_stret(void *aBlock)
{
	struct Block_layout *layout = (struct Block_layout *)aBlock;

	int requiredFlags = BLOCK_HAS_SIGNATURE | BLOCK_USE_STRET;
	return (layout->flags & requiredFlags) == requiredFlags;
}

                                                                        
bool
_Block_has_signature(void *aBlock)
{
	return _Block_signature(aBlock) ? true : false;
}

const char *
_Block_signature(void *aBlock)
{
	struct Block_layout *layout = (struct Block_layout *)aBlock;
	if (!(layout->flags & BLOCK_HAS_SIGNATURE)) {
		return nullptr;
	}

	if (layout->flags & BLOCK_SMALL_DESCRIPTOR) {
		auto *bds = (Block_descriptor_small *)_Block_get_descriptor(layout);
		return unwrap_relative_pointer<const char>(bds->signature);
	}

	struct Block_descriptor_3 *desc3 = _Block_descriptor_3(layout);
	return desc3->signature;
}

const char *
_Block_layout(void *aBlock)
{
	                                                                  
	Block_layout *layout = (Block_layout *)aBlock;
	if ((layout->flags & BLOCK_HAS_EXTENDED_LAYOUT) ||
	    !(layout->flags & BLOCK_HAS_SIGNATURE)) {
		return nullptr;
	}

	if (layout->flags & BLOCK_SMALL_DESCRIPTOR) {
		auto *bds = (Block_descriptor_small *)_Block_get_descriptor(layout);
		return unwrap_relative_pointer<const char>(bds->layout);
	}

	Block_descriptor_3 *desc = _Block_descriptor_3(layout);
	return desc->layout;
}

const char *
_Block_extended_layout(void *aBlock)
{
	                                                                  
	Block_layout *layout = (Block_layout *)aBlock;
	if (!(layout->flags & BLOCK_HAS_EXTENDED_LAYOUT) ||
	    !(layout->flags & BLOCK_HAS_SIGNATURE)) {
		return nullptr;
	}

	const char *extLayout;
	if (layout->flags & BLOCK_SMALL_DESCRIPTOR) {
		auto *bds = (Block_descriptor_small *)_Block_get_descriptor(layout);
		if (layout->flags & BLOCK_INLINE_LAYOUT_STRING) {
			extLayout = (const char *)(uintptr_t)bds->layout;
		} else {
			extLayout = unwrap_relative_pointer<const char>(bds->layout);
		}
	} else {
		Block_descriptor_3 *desc3 = _Block_descriptor_3(layout);
		extLayout = desc3->layout;
	}

	                                                             
	                                                              
	if (!extLayout) {
		extLayout = "";
	}
	return extLayout;
}

#if !TARGET_OS_WIN32
#pragma mark Compiler SPI entry points
#endif


                                                        
  
                                                     
  
  
                                                                                                                
                              
                                        
                   
                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                
  
                                                                                   
                                                                    
                                                               
                                                                
                                                                                             
  
                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
  
                                                                                                                                                                                                                                   
                                                    
                                                    
                                                    
                                                    
  
  
                                                          

  
                                                                                                
                        
  
void
_Block_object_assign(void *destArg, const void *object, const int flags)
{
	const void **dest = (const void **)destArg;
	switch (os_assumes(flags & BLOCK_ALL_COPY_DISPOSE_FLAGS)) {
	case BLOCK_FIELD_IS_OBJECT:
		        
                      
                          
            

		_Block_retain_object(object);
		*dest = object;
		break;

	case BLOCK_FIELD_IS_BLOCK:
		        
                                 
                          
            

		*dest = _Block_copy(object);
		break;

	case BLOCK_FIELD_IS_BYREF | BLOCK_FIELD_IS_WEAK:
	case BLOCK_FIELD_IS_BYREF:
		        
                                                       
                                                        
                                                                 
                    
                           
                     
            

		*dest = _Block_byref_copy(object);
		break;

	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_OBJECT:
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_BLOCK:
		        
                                                            
                                                  
                                                                     
                        
                                   
                          
            

		*dest = object;
		break;

	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_OBJECT | BLOCK_FIELD_IS_WEAK:
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_BLOCK  | BLOCK_FIELD_IS_WEAK:
		        
                                                            
                                                        
                                                                 
                               
                                          
                          
            

		*dest = object;
		break;

	default:
		break;
	}
}

                                                                                               
                                  
void
_Block_object_dispose(const void *object, const int flags)
{
	switch (os_assumes(flags & BLOCK_ALL_COPY_DISPOSE_FLAGS)) {
	case BLOCK_FIELD_IS_BYREF | BLOCK_FIELD_IS_WEAK:
	case BLOCK_FIELD_IS_BYREF:
		                                                        
		_Block_byref_release(object);
		break;
	case BLOCK_FIELD_IS_BLOCK:
		_Block_release(object);
		break;
	case BLOCK_FIELD_IS_OBJECT:
		_Block_release_object(object);
		break;
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_OBJECT:
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_BLOCK:
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_OBJECT | BLOCK_FIELD_IS_WEAK:
	case BLOCK_BYREF_CALLER | BLOCK_FIELD_IS_BLOCK  | BLOCK_FIELD_IS_WEAK:
		break;
	default:
		break;
	}
}


                                                                                
__attribute__((used))
static int let_there_be_data = 42;
