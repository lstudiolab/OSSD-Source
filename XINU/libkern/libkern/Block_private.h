/*
 * Block_private.h
 *
 * SPI for Blocks
 *
 * Copyright (c) 2008-2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LLVM_LICENSE_HEADER@
 *
 */

#ifndef _BLOCK_PRIVATE_H_
#define _BLOCK_PRIVATE_H_

#include <Availability.h>
#include <AvailabilityMacros.h>
#ifndef KERNEL
#include <TargetConditionals.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#ifdef KERNEL
#include <sys/systm.h>
#else
#include <stdio.h>
#endif


#ifdef KERNEL
#include <libkern/Block.h>
struct Block_byref;
#else
#include <Block.h>
#endif

#if __has_include(<ptrauth.h>)
#include <ptrauth.h>
#endif

#if __has_feature(ptrauth_calls) && __cplusplus < 201103L

                               

#define _Block_set_function_pointer(field, value)                       \
    ((value)                                                            \
     ? ((field) =                                                       \
	(__typeof__(field))                                             \
	ptrauth_auth_and_resign((void*)(value),                         \
	                        ptrauth_key_function_pointer, 0,        \
	                        ptrauth_key_block_function, &(field)))  \
     : ((field) = 0))

#define _Block_get_function_pointer(field)                              \
    ((field)                                                            \
     ? (__typeof__(field))                                              \
       ptrauth_auth_function((void*)(field),                            \
	                     ptrauth_key_block_function, &(field))      \
     : (__typeof__(field))0)

#else

                              

#define _Block_set_function_pointer(field, value)       \
    (field) = (value)

#define _Block_get_function_pointer(field)      \
    (field)

#endif


#if __has_feature(ptrauth_calls) && __cplusplus >= 201103L

                                                                          
                                                                       
                         
                                                              
template <typename Fn, ptrauth_key Key>
class StorageSignedFunctionPointer {
	uintptr_t bits;

public:

                                                            
                                                                 
                                          
	uintptr_t
	prepareWrite(Fn fn) const
	{
		if (fn == nullptr) {
			return 0;
		} else {
			return (uintptr_t)
			       ptrauth_auth_and_resign(fn, ptrauth_key_function_pointer, ptrauth_function_pointer_type_discriminator(Fn),
			           Key, &bits);
		}
	}

                                          
                                       
                                          
	uintptr_t
	prepareWrite(const StorageSignedFunctionPointer& other) const
	{
		if (other.bits == 0) {
			return 0;
		} else {
			return (uintptr_t)
			       ptrauth_auth_and_resign((void*)other.bits, Key, &other.bits,
			           Key, &bits);
		}
	}

                                                                
                                      
                                           
	Fn
	completeReadFn(uintptr_t ptr) const
	{
		if (ptr == 0) {
			return nullptr;
		} else {
			return (Fn)ptrauth_auth_function((void *)ptr, Key, &bits);
		}
	}

                                                         
                                          
                                           
	void*
	completeReadRaw(uintptr_t ptr) const
	{
		if (ptr == 0) {
			return nullptr;
		} else {
			return ptrauth_auth_data((void*)ptr, Key, &bits);
		}
	}

	StorageSignedFunctionPointer()
	{
	}

	StorageSignedFunctionPointer(Fn value)
		: bits(prepareWrite(value))
	{
	}

	StorageSignedFunctionPointer(const StorageSignedFunctionPointer& value)
		: bits(prepareWrite(value))
	{
	}

	StorageSignedFunctionPointer&
	operator =(Fn rhs)
	{
		bits = prepareWrite(rhs);
		return *this;
	}

	StorageSignedFunctionPointer&
	operator =(const StorageSignedFunctionPointer& rhs)
	{
		bits = prepareWrite(rhs);
		return *this;
	}

	operator Fn() const {
		return completeReadFn(bits);
	}

	explicit
	operator void*() const
	{
		return completeReadRaw(bits);
	}

	explicit
	operator bool() const
	{
		return completeReadRaw(bits) != nullptr;
	}
};

using BlockCopyFunction = StorageSignedFunctionPointer
    <void (*)(void *, const void *),
    ptrauth_key_block_function>;

using BlockDisposeFunction = StorageSignedFunctionPointer
    <void (*)(const void *),
    ptrauth_key_block_function>;

using BlockInvokeFunction = StorageSignedFunctionPointer
    <void (*)(void *, ...),
    ptrauth_key_block_function>;

using BlockByrefKeepFunction = StorageSignedFunctionPointer
    <void (*)(struct Block_byref *, struct Block_byref *),
    ptrauth_key_block_function>;

using BlockByrefDestroyFunction = StorageSignedFunctionPointer
    <void (*)(struct Block_byref *),
    ptrauth_key_block_function>;

                          
#elif !__has_feature(ptrauth_calls)
                    

typedef void (*BlockCopyFunction)(void *, const void *);
typedef void (*BlockDisposeFunction)(const void *);
typedef void (*BlockInvokeFunction)(void *, ...);
typedef void (*BlockByrefKeepFunction)(struct Block_byref*, struct Block_byref*);
typedef void (*BlockByrefDestroyFunction)(struct Block_byref *);

#else
                              

typedef uintptr_t BlockCopyFunction;
typedef uintptr_t BlockDisposeFunction;
typedef uintptr_t BlockInvokeFunction;
typedef uintptr_t BlockByrefKeepFunction;
typedef uintptr_t BlockByrefDestroyFunction;

#endif

#if __has_feature(ptrauth_calls)
#define _Block_get_relative_function_pointer(field, type)           \
	((type)ptrauth_sign_unauthenticated(                              \
	                (void *)((uintptr_t)(intptr_t)(field) + (uintptr_t)&(field)), \
	                ptrauth_key_function_pointer, 0))
#else
#define _Block_get_relative_function_pointer(field, type)       \
	((type)((uintptr_t)(intptr_t)(field) + (uintptr_t)&(field)))
#endif

#define _Block_descriptor_ptrauth_discriminator 0xC0BB

                                                           
enum {
	BLOCK_DEALLOCATING =      (0x0001),          
	BLOCK_REFCOUNT_MASK =     (0xfffe),          
	BLOCK_INLINE_LAYOUT_STRING = (1 << 21),            
	BLOCK_SMALL_DESCRIPTOR =  (1 << 22),            
	BLOCK_IS_NOESCAPE =       (1 << 23),            
	BLOCK_NEEDS_FREE =        (1 << 24),          
	BLOCK_HAS_COPY_DISPOSE =  (1 << 25),           
	BLOCK_HAS_CTOR =          (1 << 26),                                  
	BLOCK_IS_GC =             (1 << 27),          
	BLOCK_IS_GLOBAL =         (1 << 28),           
	BLOCK_USE_STRET =         (1 << 29),                                              
	BLOCK_HAS_SIGNATURE  =    (1 << 30),           
	BLOCK_HAS_EXTENDED_LAYOUT=(1 << 31)            
};

#define BLOCK_DESCRIPTOR_1 1
struct Block_descriptor_1 {
	uintptr_t reserved;
	uintptr_t size;
};

#define BLOCK_DESCRIPTOR_2 1
struct Block_descriptor_2 {
	                                  
	BlockCopyFunction copy;
	BlockDisposeFunction dispose;
};

#define BLOCK_DESCRIPTOR_3 1
struct Block_descriptor_3 {
	                               
	const char *signature;
	const char *layout;                                                
};

struct Block_descriptor_small {
	uint32_t size;

	int32_t signature;
	int32_t layout;

	                                                   
                                                         
	int32_t copy;
	int32_t dispose;
};


struct Block_layout {
	void *isa;
	volatile int32_t flags;                      
	int32_t reserved;
	BlockInvokeFunction invoke;
	struct Block_descriptor_1 *descriptor;
	                     
};


                                                              
enum {
	                                                                    
	                                                  
	                                                  

	BLOCK_BYREF_LAYOUT_MASK =       (0xf << 28),           
	BLOCK_BYREF_LAYOUT_EXTENDED =   (1 << 28),           
	BLOCK_BYREF_LAYOUT_NON_OBJECT = (2 << 28),            
	BLOCK_BYREF_LAYOUT_STRONG =     (3 << 28),           
	BLOCK_BYREF_LAYOUT_WEAK =       (4 << 28),           
	BLOCK_BYREF_LAYOUT_UNRETAINED = (5 << 28),            

	BLOCK_BYREF_IS_GC =             (1 << 27),          

	BLOCK_BYREF_HAS_COPY_DISPOSE =  (1 << 25),           
	BLOCK_BYREF_NEEDS_FREE =        (1 << 24),          
};

struct Block_byref {
	void *isa;
	struct Block_byref *forwarding;
	volatile int32_t flags;                      
	uint32_t size;
};

struct Block_byref_2 {
	                                        
	BlockByrefKeepFunction byref_keep;
	BlockByrefDestroyFunction byref_destroy;
};

struct Block_byref_3 {
	                                       
	const char *layout;
};


                            

                                                                       
                                                                 

                                                                         
                                                               
                        

                                                           
                                                         
                                                                              
                                                                              

enum {
	BLOCK_LAYOUT_ESCAPE = 0,                                                 
	BLOCK_LAYOUT_NON_OBJECT_BYTES = 1,                       
	BLOCK_LAYOUT_NON_OBJECT_WORDS = 2,                       
	BLOCK_LAYOUT_STRONG           = 3,                          
	BLOCK_LAYOUT_BYREF            = 4,                         
	BLOCK_LAYOUT_WEAK             = 5,                        
	BLOCK_LAYOUT_UNRETAINED       = 6,                              
	BLOCK_LAYOUT_UNKNOWN_WORDS_7  = 7,                    
	BLOCK_LAYOUT_UNKNOWN_WORDS_8  = 8,                    
	BLOCK_LAYOUT_UNKNOWN_WORDS_9  = 9,                    
	BLOCK_LAYOUT_UNKNOWN_WORDS_A  = 0xA,                    
	BLOCK_LAYOUT_UNUSED_B         = 0xB,                        
	BLOCK_LAYOUT_UNUSED_C         = 0xC,                        
	BLOCK_LAYOUT_UNUSED_D         = 0xD,                        
	BLOCK_LAYOUT_UNUSED_E         = 0xE,                        
	BLOCK_LAYOUT_UNUSED_F         = 0xF,                        
};


                                                                                  

                                                                           
enum {
	                                                                                               
	BLOCK_FIELD_IS_OBJECT   =  3,                                                      
	BLOCK_FIELD_IS_BLOCK    =  7,                   
	BLOCK_FIELD_IS_BYREF    =  8,                                                      
	BLOCK_FIELD_IS_WEAK     = 16,                                                   
	BLOCK_BYREF_CALLER      = 128,                                                             
};

enum {
	BLOCK_ALL_COPY_DISPOSE_FLAGS =
	    BLOCK_FIELD_IS_OBJECT | BLOCK_FIELD_IS_BLOCK | BLOCK_FIELD_IS_BYREF |
	    BLOCK_FIELD_IS_WEAK | BLOCK_BYREF_CALLER
};


                             

static inline __typeof__(void (*)(void *, ...))
_Block_get_invoke_fn(struct Block_layout *block)
{
	return (void (*)(void *, ...))_Block_get_function_pointer(block->invoke);
}

static inline void
_Block_set_invoke_fn(struct Block_layout *block, void (*fn)(void *, ...))
{
	_Block_set_function_pointer(block->invoke, fn);
}

static inline void *
_Block_get_descriptor(struct Block_layout *aBlock)
{
#if __has_feature(ptrauth_signed_block_descriptors)
	uintptr_t disc = ptrauth_blend_discriminator(
		&aBlock->descriptor, _Block_descriptor_ptrauth_discriminator);
	return ptrauth_auth_data(aBlock->descriptor, ptrauth_key_asda, disc);
#else
	return aBlock->descriptor;
#endif
}

static inline __typeof__(void (*)(void *, const void *))
_Block_get_copy_fn(struct Block_descriptor_2 *desc)
{
	return (void (*)(void *, const void *))_Block_get_function_pointer(desc->copy);
}

static inline void
_Block_set_copy_fn(struct Block_descriptor_2 *desc,
    void (*fn)(void *, const void *))
{
	_Block_set_function_pointer(desc->copy, fn);
}


static inline __typeof__(void (*)(const void *))
_Block_get_dispose_fn(struct Block_descriptor_2 *desc)
{
	return (void (*)(const void *))_Block_get_function_pointer(desc->dispose);
}

static inline void
_Block_set_dispose_fn(struct Block_descriptor_2 *desc,
    void (*fn)(const void *))
{
	_Block_set_function_pointer(desc->dispose, fn);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

static inline __typeof__(void (*)(void *, const void *))
_Block_get_copy_function(struct Block_layout *aBlock)
{
	if (!(aBlock->flags & BLOCK_HAS_COPY_DISPOSE)) {
		return NULL;
	}

	void *desc = _Block_get_descriptor(aBlock);
	if (aBlock->flags & BLOCK_SMALL_DESCRIPTOR) {
		struct Block_descriptor_small *bds =
		    (struct Block_descriptor_small *)desc;
		return _Block_get_relative_function_pointer(
			bds->copy, void (*)(void *, const void *));
	}

	struct Block_descriptor_2 *bd2 =
	    (struct Block_descriptor_2 *)((unsigned char *)desc +
	    sizeof(struct Block_descriptor_1));
	return _Block_get_copy_fn(bd2);
}

static inline __typeof__(void (*)(const void *))
_Block_get_dispose_function(struct Block_layout *aBlock)
{
	if (!(aBlock->flags & BLOCK_HAS_COPY_DISPOSE)) {
		return NULL;
	}

	void *desc = _Block_get_descriptor(aBlock);
	if (aBlock->flags & BLOCK_SMALL_DESCRIPTOR) {
		struct Block_descriptor_small *bds =
		    (struct Block_descriptor_small *)desc;
		return _Block_get_relative_function_pointer(
			bds->dispose, void (*)(const void *));
	}

	struct Block_descriptor_2 *bd2 =
	    (struct Block_descriptor_2 *)((unsigned char *)desc +
	    sizeof(struct Block_descriptor_1));
	return _Block_get_dispose_fn(bd2);
}

#pragma clang diagnostic pop

                          


                                               
BLOCK_EXPORT size_t Block_size(void *aBlock);

                                                                                             
BLOCK_EXPORT bool _Block_has_signature(void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);

                                                                         
BLOCK_EXPORT bool _Block_use_stret(void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);

                                                                      
                                                          
                                                        
BLOCK_EXPORT const char * _Block_signature(void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);

                                                     
                                       
                   
BLOCK_EXPORT const char * _Block_layout(void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);

                                                  
                                                        
                   
BLOCK_EXPORT const char * _Block_extended_layout(void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_7_0);

                                                                    
BLOCK_EXPORT bool _Block_tryRetain(const void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);

                                                                    
BLOCK_EXPORT bool _Block_isDeallocating(const void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);


                                                    
                                                                  
BLOCK_EXPORT void * _NSConcreteMallocBlock[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteAutoBlock[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteFinalizingBlock[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteWeakBlockVariable[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
                      
                                                  
                                                 


#if !KERNEL
struct Block_callbacks_RR {
	size_t  size;                                                           
	void  (*retain)(const void *);
	void  (*release)(const void *);
	void  (*destructInstance)(const void *);
};
typedef struct Block_callbacks_RR Block_callbacks_RR;

BLOCK_EXPORT void _Block_use_RR2(const Block_callbacks_RR *callbacks);
#endif           

#endif
