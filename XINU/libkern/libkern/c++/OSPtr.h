  
// Copyright (c) 2019 Apple, Inc. All rights reserved.
  
                                           
  
                                                                         
// as defined in and that are subject to the Apple Public Source License
                                                                   
                                                                           
                                                                          
                                                                    
                                                                        
                                                                 
  
                                         
                                                                            
  
                                                                       
                                                                        
                                                                      
                                                                   
                                                                         
                                                                        
                                 
  
                                         
  

#ifndef XNU_LIBKERN_LIBKERN_CXX_OS_PTR_H
#define XNU_LIBKERN_LIBKERN_CXX_OS_PTR_H

  
                                                                           
                             
  
                                                                              
                                                                        
                                                                       
                                                                         
                                                    
  
                                                                               
                                                                            
                          
  
                                                                           
                                                 
  

#if !defined(PRIVATE)                                                            
#   if defined(IOKIT_ENABLE_SHARED_PTR)
#       if !defined(IOKIT_ENABLE_EXPERIMENTAL_SHARED_PTR_IN_API)
#           error It seems that you have defined IOKIT_ENABLE_SHARED_PTR to \
        ask IOKit to return shared pointers from many of its API \
        functions. This is great! However, please note that we may \
        transition more IOKit APIs to shared pointers in the future, \
        so if you enable IOKIT_ENABLE_SHARED_PTR right now, your \
        code may fail to compile with future versions of IOKit \
        (which would return shared pointers where you expect raw \
        pointers). If you are OK with that, please define the \
        IOKIT_ENABLE_EXPERIMENTAL_SHARED_PTR_IN_API macro to \
        silence this error. If that is not acceptable, please hold \
        off on enabling shared pointers in IOKit APIs until we have \
        committed to API stability for it.
#       endif
#   endif
#endif

#if defined(IOKIT_ENABLE_SHARED_PTR)

#if __cplusplus < 201703L
#error "Your code must compile with C++17 or later to adopt shared pointers. Use Xcode's 'C++ Language Dialect' setting, or on clang's command-line use -std=gnu++17"
#endif

#include <libkern/c++/OSSharedPtr.h>

template <typename T>
using OSPtr = OSSharedPtr<T>;

class OSCollection;                                                             

template <typename T>
using OSTaggedPtr = OSTaggedSharedPtr<T, OSCollection>;

#else

template <typename T>
class __attribute__((trivial_abi)) OSSharedPtr;

template <typename T, typename Tag>
class __attribute__((trivial_abi)) OSTaggedSharedPtr;

                                                                      
                       
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-extensions"

template <typename T>
using OSPtr = T *;

template <typename T>
using OSTaggedPtr = T *;

#pragma clang diagnostic pop

#endif

                                   
  
                                                                         
                                                                         
#if !__has_feature(cxx_nullptr) && !defined(nullptr)
# define nullptr NULL
#endif

#endif                                     
