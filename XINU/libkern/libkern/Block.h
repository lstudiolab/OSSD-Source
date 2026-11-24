/*
 *  Block.h
 *
 * Copyright (c) 2008-2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LLVM_LICENSE_HEADER@
 *
 */

#ifndef _Block_H_
#define _Block_H_

#if !defined(BLOCK_EXPORT)
#   if defined(__cplusplus)
#       define BLOCK_EXPORT extern "C"
#   else
#       define BLOCK_EXPORT extern
#   endif
#endif

#include <Availability.h>
#ifndef KERNEL
#include <TargetConditionals.h>
#endif             

#if __cplusplus
extern "C" {
#endif

                                                                                    
                                                                              
                                        
BLOCK_EXPORT void *_Block_copy(const void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

                                                                               
BLOCK_EXPORT void _Block_release(const void *aBlock)
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);


                                                            
BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int)
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

                                                            
BLOCK_EXPORT void _Block_object_dispose(const void *, const int)
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);

                                                             
BLOCK_EXPORT void * _NSConcreteGlobalBlock[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteStackBlock[32]
__OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);


#if __cplusplus
}
#endif

                      

#define Block_copy(...) ((__typeof(__VA_ARGS__))_Block_copy((const void *)(__VA_ARGS__)))
#define Block_release(...) _Block_release((const void *)(__VA_ARGS__))


#endif
