/*
 * Copyright (c) 2006-2007 Apple Inc. All rights reserved.
 */

#ifndef _ARM__PARAM_H_
#define _ARM__PARAM_H_

#include <arm/_types.h>

  
                                                                          
                                                                        
                                    
   
#define __DARWIN_ALIGNBYTES     (sizeof(__darwin_size_t) - 1)
#define __DARWIN_ALIGN(p)       ((__darwin_size_t)((__darwin_size_t)(p) + __DARWIN_ALIGNBYTES) &~ __DARWIN_ALIGNBYTES)

#define      __DARWIN_ALIGNBYTES32     (sizeof(__uint32_t) - 1)
#define       __DARWIN_ALIGN32(p)       ((__darwin_size_t)((__darwin_size_t)(p) + __DARWIN_ALIGNBYTES32) &~ __DARWIN_ALIGNBYTES32)


#endif                     
