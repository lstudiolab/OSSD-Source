/*
 * Copyright (c) 2000-2011 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1989, 1993
 *     The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the University of
 *     California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)xdr_subs.h  8.3 (Berkeley) 3/30/95
 * FreeBSD-Id: xdr_subs.h,v 1.9 1997/02/22 09:42:53 peter Exp $
 */

#ifndef _NFS_XDR_SUBS_H_
#define _NFS_XDR_SUBS_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE
  
                                                                  
                                                        
                                                                     
  
                                                                        
                                                                    
                                                                     
                                                 
   

#define fxdr_unsigned(t, v)     ((t)ntohl((uint32_t)(v)))
#define txdr_unsigned(v)        (htonl((uint32_t)(v)))

#define fxdr_hyper(f, t) { \
	((uint32_t *)(t))[_QUAD_HIGHWORD] = ntohl(((uint32_t *)(f))[0]); \
	((uint32_t *)(t))[_QUAD_LOWWORD] = ntohl(((uint32_t *)(f))[1]); \
}
#define txdr_hyper(f, t) { \
	((uint32_t *)(t))[0] = htonl(((uint32_t *)(f))[_QUAD_HIGHWORD]); \
	((uint32_t *)(t))[1] = htonl(((uint32_t *)(f))[_QUAD_LOWWORD]); \
}


  
         
  
                                                                             
   
typedef enum xdrbuf_type {
	XDRBUF_NONE   = 0,
	XDRBUF_BUFFER = 1,
} xdrbuf_type;

struct xdrbuf {
	union {
		struct {
			char *                  xbb_base;                                   
			size_t                  xbb_size;                           
			size_t                  xbb_len;                                      
		} xb_buffer;
	} xb_u;
	char *          xb_ptr;                                          
	size_t          xb_left;                                               
	size_t          xb_growsize;                                        
	xdrbuf_type     xb_type;                                
	uint32_t        xb_flags;                             
};

#define XB_CLEANUP      0x0001                     

#define XDRWORD         4                                                                   
#define xdr_rndup(a)    (((a)+3)&(~0x3))                                      
#define xdr_pad(a)      (xdr_rndup(a) - (a))                                    

void xb_init(struct xdrbuf *, xdrbuf_type);
void xb_init_buffer(struct xdrbuf *, char *, size_t);
void xb_cleanup(struct xdrbuf *);
void *xb_malloc(size_t);
void xb_free(void *);
int xb_grow(struct xdrbuf *);
void xb_set_cur_buf_len(struct xdrbuf *);
char *xb_buffer_base(struct xdrbuf *);
int xb_advance(struct xdrbuf *, size_t);
size_t xb_offset(struct xdrbuf *);
int xb_seek(struct xdrbuf *, size_t);
int xb_add_bytes(struct xdrbuf *, const char *, size_t, int);
int xb_get_bytes(struct xdrbuf *, char *, uint32_t, int);

#ifdef _NFS_XDR_SUBS_FUNCS_

  
                                           
   
void
xb_init(struct xdrbuf *xbp, xdrbuf_type type)
{
	bzero(xbp, sizeof(*xbp));
	xbp->xb_type = type;
	xbp->xb_flags |= XB_CLEANUP;
}

  
                                    
   
void
xb_init_buffer(struct xdrbuf *xbp, char *buf, size_t buflen)
{
	xb_init(xbp, XDRBUF_BUFFER);
	xbp->xb_u.xb_buffer.xbb_base = buf;
	xbp->xb_u.xb_buffer.xbb_size = buflen;
	xbp->xb_u.xb_buffer.xbb_len = buflen;
	xbp->xb_growsize = 512;
	xbp->xb_ptr = buf;
	xbp->xb_left = buflen;
	if (buf) {                                                                 
		xbp->xb_flags &= ~XB_CLEANUP;
	}
}

  
                                                       
   
char *
xb_buffer_base(struct xdrbuf *xbp)
{
	return xbp->xb_u.xb_buffer.xbb_base;
}

  
                                           
   
void
xb_cleanup(struct xdrbuf *xbp)
{
	if (!(xbp->xb_flags & XB_CLEANUP)) {
		return;
	}
	switch (xbp->xb_type) {
	case XDRBUF_BUFFER:
		if (xbp->xb_u.xb_buffer.xbb_base) {
			xb_free(xbp->xb_u.xb_buffer.xbb_base);
		}
		break;
	default:
		break;
	}
	xbp->xb_flags &= ~XB_CLEANUP;
}

  
                                                        
                                                  
   
void
xb_set_cur_buf_len(struct xdrbuf *xbp)
{
	switch (xbp->xb_type) {
	case XDRBUF_BUFFER:
		xbp->xb_u.xb_buffer.xbb_len = xbp->xb_ptr - xbp->xb_u.xb_buffer.xbb_base;
		break;
	default:
		break;
	}
}

  
                                                  
   
int
xb_advance(struct xdrbuf *xbp, size_t len)
{
	size_t tlen;

	while (len) {
		if (xbp->xb_left <= 0) {
			return EBADRPC;
		}
		tlen = MIN(xbp->xb_left, len);
		if (tlen) {
			xbp->xb_ptr += tlen;
			xbp->xb_left -= tlen;
			len -= tlen;
		}
	}
	return 0;
}

  
                                                  
   
size_t
xb_offset(struct xdrbuf *xbp)
{
	size_t offset = 0;

	switch (xbp->xb_type) {
	case XDRBUF_BUFFER:
		offset = xbp->xb_ptr - xbp->xb_u.xb_buffer.xbb_base;
		break;
	default:
		break;
	}

	return offset;
}

  
                                                                   
   
int
xb_seek(struct xdrbuf *xbp, size_t offset)
{
	switch (xbp->xb_type) {
	case XDRBUF_BUFFER:
		xbp->xb_ptr = xbp->xb_u.xb_buffer.xbb_base + offset;
		xbp->xb_left = xbp->xb_u.xb_buffer.xbb_len - offset;
		break;
	default:
		break;
	}

	return 0;
}

  
                  
   
void *
xb_malloc(size_t size)
{
	void *buf = NULL;

#ifdef KERNEL
	MALLOC(buf, void *, size, M_TEMP, M_WAITOK);
#else
	buf = malloc(size);
#endif
	return buf;
}
  
                                                    
   
void
xb_free(void *buf)
{
#ifdef KERNEL
	FREE(buf, M_TEMP);
#else
	free(buf);
#endif
}

  
                                                       
   
int
xb_grow(struct xdrbuf *xbp)
{
	char *newbuf, *oldbuf;
	size_t newsize, oldsize;

	switch (xbp->xb_type) {
	case XDRBUF_BUFFER:
		oldsize = xbp->xb_u.xb_buffer.xbb_size;
		oldbuf = xbp->xb_u.xb_buffer.xbb_base;
		newsize = oldsize + xbp->xb_growsize;
		if (newsize < oldsize) {
			return ENOMEM;
		}
		newbuf = xb_malloc(newsize);
		if (newbuf == NULL) {
			return ENOMEM;
		}
		if (oldbuf != NULL) {
			bcopy(oldbuf, newbuf, oldsize);
			xb_free(oldbuf);
		}
		xbp->xb_u.xb_buffer.xbb_base = newbuf;
		xbp->xb_u.xb_buffer.xbb_size = newsize;
		xbp->xb_ptr = newbuf + oldsize;
		xbp->xb_left = xbp->xb_growsize;
		break;
	default:
		break;
	}

	return 0;
}

  
                 
  
                                                                                
   
int
xb_add_bytes(struct xdrbuf *xbp, const char *buf, size_t count, int nopad)
{
	size_t len, tlen;
	int error;

	len = nopad ? count : xdr_rndup(count);

	                                                      
	while (len) {
		if (xbp->xb_left <= 0) {
			                     
			if ((error = xb_grow(xbp))) {
				return error;
			}
			if (xbp->xb_left <= 0) {
				return ENOMEM;
			}
		}
		tlen = MIN(xbp->xb_left, len);
		if (tlen) {
			if (count) {
				if (tlen > count) {
					tlen = count;
				}
				bcopy(buf, xbp->xb_ptr, tlen);
			} else {
				bzero(xbp->xb_ptr, tlen);
			}
			xbp->xb_ptr += tlen;
			xbp->xb_left -= tlen;
			len -= tlen;
			if (count) {
				buf += tlen;
				count -= tlen;
			}
		}
	}
	return 0;
}

  
                 
  
                                                              
   
int
xb_get_bytes(struct xdrbuf *xbp, char *buf, uint32_t count, int nopad)
{
	size_t len, tlen;

	len = nopad ? count : xdr_rndup(count);

	                                                      
	while (len) {
		if (xbp->xb_left <= 0) {
			return ENOMEM;
		}
		tlen = MIN(xbp->xb_left, len);
		if (tlen) {
			if (count) {
				if (tlen > count) {
					tlen = count;
				}
				bcopy(xbp->xb_ptr, buf, tlen);
			}
			xbp->xb_ptr += tlen;
			xbp->xb_left -= tlen;
			len -= tlen;
			if (count) {
				buf += tlen;
				count -= tlen;
			}
		}
	}
	return 0;
}

#endif                           


  
                               
   

                                                         
#define xb_build_done(E, XB) \
	do { \
	        if (E) break; \
	        xb_set_cur_buf_len(XB); \
	} while (0)

                        
#define xb_add_32(E, XB, VAL) \
	do { \
	        uint32_t __tmp; \
	        if (E) break; \
	        __tmp = txdr_unsigned(VAL); \
	        (E) = xb_add_bytes((XB), (void*)&__tmp, XDRWORD, 0); \
	} while (0)

                        
#define xb_add_64(E, XB, VAL) \
	do { \
	        uint64_t __tmp1, __tmp2; \
	        if (E) break; \
	        __tmp1 = (VAL); \
	        txdr_hyper(&__tmp1, &__tmp2); \
	        (E) = xb_add_bytes((XB), (char*)&__tmp2, 2 * XDRWORD, 0); \
	} while (0)

                               
#define xb_add_word_array(E, XB, A, LEN) \
	do { \
	        uint32_t __i; \
	        xb_add_32((E), (XB), (LEN)); \
	        for (__i=0; __i < (uint32_t)(LEN); __i++) \
	                xb_add_32((E), (XB), (A)[__i]); \
	} while (0)
#define xb_add_bitmap(E, XB, B, LEN)    xb_add_word_array((E), (XB), (B), (LEN))

                       
#define xb_add_fh(E, XB, FHP, FHLEN) \
	do { \
	        xb_add_32((E), (XB), (FHLEN)); \
	        if (E) break; \
	        (E) = xb_add_bytes((XB), (char*)(FHP), (FHLEN), 0); \
	} while (0)

                  
#define xb_add_string(E, XB, S, LEN) \
	do { \
	        xb_add_32((E), (XB), (LEN)); \
	        if (E) break; \
	        (E) = xb_add_bytes((XB), (const char*)(S), (LEN), 0); \
	} while (0)


  
                               
   

                                  
#define xb_skip(E, XB, LEN) \
	do { \
	        if (E) break; \
	        (E) = xb_advance((XB), (LEN)); \
	} while (0)

                        
#define xb_get_32(E, XB, LVAL) \
	do { \
	        uint32_t __tmp; \
	        (LVAL) = (typeof((LVAL))) 0; \
	        if (E) break; \
	        (E) = xb_get_bytes((XB), (char*)&__tmp, XDRWORD, 0); \
	        if (E) break; \
	        (LVAL) = fxdr_unsigned(uint32_t, __tmp); \
	} while (0)

                        
#define xb_get_64(E, XB, LVAL) \
	do { \
	        uint64_t __tmp; \
	        (LVAL) = 0; \
	        if (E) break; \
	        (E) = xb_get_bytes((XB), (char*)&__tmp, 2 * XDRWORD, 0); \
	        if (E) break; \
	        fxdr_hyper(&__tmp, &(LVAL)); \
	} while (0)

                                                                    
#define xb_get_word_array(E, XB, A, LEN) \
	do { \
	        uint32_t __len = 0, __i; \
	        xb_get_32((E), (XB), __len); \
	        if (E) break; \
	        for (__i=0; __i < MIN(__len, (uint32_t)(LEN)); __i++) \
	                xb_get_32((E), (XB), (A)[__i]); \
	        if (E) break; \
	        for (; __i < __len; __i++) \
	                xb_skip((E), (XB), XDRWORD); \
	        for (; __i < (uint32_t)(LEN); __i++) \
	                (A)[__i] = 0; \
	        (LEN) = __len; \
	} while (0)
#define xb_get_bitmap(E, XB, B, LEN)    xb_get_word_array((E), (XB), (B), (LEN))

#endif                          
#endif                       
