/*
 * Copyright (c) 2005, 2007 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */

#if defined(__LP64__) && (defined(VARIANT_CANCELABLE) || defined(VARIANT_PRE1050))
#undef __DARWIN_NON_CANCELABLE
#define __DARWIN_NON_CANCELABLE 0
#endif                                                          

#if defined(VARIANT_DARWIN_EXTSN)
#define _DARWIN_C_SOURCE
#define _DARWIN_UNLIMITED_SELECT
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <sys/signal.h>
#include "_errno.h"

#if defined(VARIANT_CANCELABLE) || defined(VARIANT_PRE1050)
#if !defined(VARIANT_DARWIN_EXTSN)
extern int __select(int, fd_set * __restrict, fd_set * __restrict,
    fd_set * __restrict, struct timeval * __restrict);
#endif
int __pselect(int, fd_set * __restrict, fd_set * __restrict,
    fd_set * __restrict, const struct timespec * __restrict, const sigset_t * __restrict);
#else                                              
#if !defined(VARIANT_DARWIN_EXTSN)
int __select_nocancel(int, fd_set * __restrict, fd_set * __restrict,
    fd_set * __restrict, struct timeval * __restrict);
#endif
int __pselect_nocancel(int, fd_set * __restrict, fd_set * __restrict,
    fd_set * __restrict, const struct timespec * __restrict, const sigset_t * __restrict);
#endif                                            

#if !defined(VARIANT_DARWIN_EXTSN)
  
                                                                              
                                                                                 
                                       
   
int
select(int nfds, fd_set * __restrict readfds, fd_set * __restrict writefds,
    fd_set * __restrict exceptfds, struct timeval * __restrict
#if defined(VARIANT_LEGACY) || defined(VARIANT_PRE1050)
    intimeout
#else                                          
    timeout
#endif                                        
    )
{
#if defined(VARIANT_LEGACY) || defined(VARIANT_PRE1050)
	struct timeval tb, *timeout;

	  
                                                                      
    
	if (intimeout && intimeout->tv_sec == 0 && intimeout->tv_usec > 0 && intimeout->tv_usec < 10000) {
		tb.tv_sec = 0;
		tb.tv_usec = 10000;
		timeout = &tb;
	} else {
		timeout = intimeout;
	}
#else                                          
	if (nfds > FD_SETSIZE) {
		errno = EINVAL;
		return -1;
	}
#endif

#if defined(VARIANT_CANCELABLE) || defined(VARIANT_PRE1050)
	return __select(nfds, readfds, writefds, exceptfds, timeout);
#else                                              
	return __select_nocancel(nfds, readfds, writefds, exceptfds, timeout);
#endif                                            
}
#endif                                     


  
                                                    
                                                          
   
extern int __pthread_sigmask(int, const sigset_t *, sigset_t *);
static int
_pselect_emulated(int count, fd_set * __restrict rfds, fd_set * __restrict wfds,
    fd_set * __restrict efds, const struct timespec * __restrict timo,
    const sigset_t * __restrict mask)
{
	sigset_t omask;
	struct timeval tvtimo, *tvp;
	int rv, sverrno;

	if (timo) {
		tvtimo.tv_sec = timo->tv_sec;
		tvtimo.tv_usec = (__darwin_suseconds_t)(timo->tv_nsec / 1000);
		tvp = &tvtimo;
	} else {
		tvp = 0;
	}

	if (mask != 0) {
		rv = __pthread_sigmask(SIG_SETMASK, mask, &omask);
		if (rv != 0) {
			return rv;
		}
	}

	rv = select(count, rfds, wfds, efds, tvp);
	if (mask != 0) {
		sverrno = errno;
		__pthread_sigmask(SIG_SETMASK, &omask, (sigset_t *)0);
		errno = sverrno;
	}

	return rv;
}

  
                                                                               
                                                                            
                                  
   
int
pselect(int nfds, fd_set * __restrict readfds, fd_set * __restrict writefds,
    fd_set * __restrict exceptfds, const struct timespec * __restrict
#if defined(VARIANT_LEGACY) || defined(VARIANT_PRE1050)
    intimeout,
#else                                          
    timeout,
#endif                                        
    const sigset_t * __restrict sigmask)
{
	int ret;
#if defined(VARIANT_LEGACY) || defined(VARIANT_PRE1050)
	struct timespec tb;
	const struct timespec *timeout;

	  
                                                                      
    
	if (intimeout && intimeout->tv_sec == 0 && intimeout->tv_nsec > 0 && intimeout->tv_nsec < 10000000L) {
		tb.tv_sec = 0;
		tb.tv_nsec = 10000000L;
		timeout = &tb;
	} else {
		timeout = intimeout;
	}
#elif defined(VARIANT_DARWIN_EXTSN)
#else
	                  
	if (nfds > FD_SETSIZE) {
		errno = EINVAL;
		return -1;
	}
#endif

#if defined(VARIANT_CANCELABLE) || defined(VARIANT_PRE1050)
	ret = __pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
#else                                              
	ret = __pselect_nocancel(nfds, readfds, writefds, exceptfds, timeout, sigmask);
#endif                                            

	if (ret == -1 && errno == ENOSYS) {
		ret = _pselect_emulated(nfds, readfds, writefds, exceptfds, timeout, sigmask);
	}

	return ret;
}
