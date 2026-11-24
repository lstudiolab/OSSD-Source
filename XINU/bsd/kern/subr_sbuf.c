/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#include <kern/kalloc.h>
#include <libkern/libkern.h>
#include <os/base.h>
#include <os/overflow.h>
#include <sys/param.h>
#include <sys/sbuf.h>
#include <sys/uio.h>

#if DEBUG || DEVELOPMENT
#include <kern/macro_help.h>
#include <sys/errno.h>
#include <sys/sysctl.h>
#endif                           

#define SBUF_ISSET(s, f)        ((s)->s_flags & (f))
#define SBUF_SETFLAG(s, f)      do { (s)->s_flags |= (f); } while (0)
#define SBUF_CLEARFLAG(s, f)    do { (s)->s_flags &= ~(f); } while (0)

#define SBUF_CANEXTEND(s)       SBUF_ISSET(s, SBUF_AUTOEXTEND)
#define SBUF_HASOVERFLOWED(s)   SBUF_ISSET(s, SBUF_OVERFLOWED)
#define SBUF_ISDYNAMIC(s)       SBUF_ISSET(s, SBUF_DYNAMIC)
#define SBUF_ISDYNSTRUCT(s)     SBUF_ISSET(s, SBUF_DYNSTRUCT)
#define SBUF_ISFINISHED(s)      SBUF_ISSET(s, SBUF_FINISHED)

#define SBUF_MINEXTENDSIZE      16
#define SBUF_MAXEXTENDSIZE      PAGE_SIZE
#define SBUF_MAXEXTENDINCR      PAGE_SIZE

   
                        
  
         
                                                                               
                                                                             
                                  
  
           
                       
   
void
sbuf_delete(struct sbuf *s)
{
	if (SBUF_ISDYNAMIC(s) && s->s_buf) {
		kheap_free(KHEAP_DATA_BUFFERS, s->s_buf, s->s_size);
		s->s_buf = NULL;
	}

	if (SBUF_ISDYNSTRUCT(s)) {
		kheap_free(KHEAP_DEFAULT, s, sizeof(*s));
	}
}

   
                            
  
         
                                                                          
  
              
                                                                            
                                                                         
                   
  
           
                               
   
static int
sbuf_extendsize(size_t *size)
{
	size_t target_size = *size;
	size_t new_size;

	if (target_size > INT_MAX) {
		return -1;
	}

	if (target_size < SBUF_MAXEXTENDSIZE) {
		new_size = SBUF_MINEXTENDSIZE;
		while (new_size < target_size) {
			new_size *= 2;
		}
	} else {
		                               
		new_size = (target_size + PAGE_SIZE - 1) & ~PAGE_MASK;
	}

	if (new_size > INT_MAX) {
		return -1;
	}

	*size = new_size;
	return 0;
}

   
                     
  
         
                                        
  
           
                                                                               
            
  
             
                                                                             
                                
  
                 
                                                                          
         
  
               
                                                      
  
                                                                             
                                                     
                                                                             
                                         
  
           
                                                                  
   
struct sbuf *
sbuf_new(struct sbuf *s, char *buf, int length_, int flags)
{
	size_t length = (size_t)length_;

	if (length > INT_MAX || flags & ~SBUF_USRFLAGMSK) {
		return NULL;
	}

	if (s == NULL) {
		s = (struct sbuf *)kheap_alloc(KHEAP_DEFAULT, sizeof(*s), Z_WAITOK);
		if (NULL == s) {
			return NULL;
		}

		bzero(s, sizeof(*s));
		s->s_flags = flags;
		SBUF_SETFLAG(s, SBUF_DYNSTRUCT);
	} else {
		bzero(s, sizeof(*s));
		s->s_flags = flags;
	}

	if (buf) {
		s->s_size = (int)length;
		s->s_buf = buf;
		return s;
	}

	if (SBUF_CANEXTEND(s) && (-1 == sbuf_extendsize(&length))) {
		goto fail;
	}

	  
                                                                       
                               
    
	if (length == 0) {
		goto fail;
	}

	s->s_buf = (char *)kheap_alloc(KHEAP_DATA_BUFFERS, length, Z_WAITOK);
	if (NULL == s->s_buf) {
		goto fail;
	}
	bzero(s->s_buf, length);
	s->s_size = (int)length;

	SBUF_SETFLAG(s, SBUF_DYNAMIC);
	return s;

fail:
	sbuf_delete(s);
	return NULL;
}

   
                        
  
         
                                        
  
           
                      
  
             
                                                                                
  
           
                               
   
int
sbuf_setpos(struct sbuf *s, int pos)
{
	if (pos < 0 || pos > s->s_len) {
		return -1;
	}

	s->s_len = pos;
	return 0;
}

   
                       
  
         
                                                                              
                      
  
           
                     
   
void
sbuf_clear(struct sbuf *s)
{
	SBUF_CLEARFLAG(s, SBUF_FINISHED);
	SBUF_CLEARFLAG(s, SBUF_OVERFLOWED);
	sbuf_setpos(s, 0);
}

   
                        
  
         
                                                                             
  
           
                      
  
                
                                          
  
           
                               
   
static int OS_WARN_RESULT
sbuf_extend(struct sbuf *s, size_t addlen)
{
	char *new_buf;
	size_t new_size;

	if (addlen == 0) {
		return 0;
	}

	if (!SBUF_CANEXTEND(s)) {
		return -1;
	}

	if (os_add_overflow((size_t)s->s_size, addlen, &new_size)) {
		return -1;
	}

	if (-1 == sbuf_extendsize(&new_size)) {
		return -1;
	}

	new_buf = (char *)kheap_alloc(KHEAP_DATA_BUFFERS, new_size, Z_WAITOK);
	if (NULL == new_buf) {
		return -1;
	}

	bcopy(s->s_buf, new_buf, (size_t)s->s_size);
	if (SBUF_ISDYNAMIC(s)) {
		kheap_free(KHEAP_DATA_BUFFERS, s->s_buf, (size_t)s->s_size);
	} else {
		SBUF_SETFLAG(s, SBUF_DYNAMIC);
	}

	s->s_buf = new_buf;
	s->s_size = (int)new_size;
	return 0;
}

   
                          
  
         
                                                                               
                                 
  
           
                                   
  
           
                             
   
static size_t
sbuf_capacity(const struct sbuf *s)
{
	                             
	return (size_t)(s->s_size - s->s_len - 1);
}

   
                                 
  
         
                                                                         
                               
  
           
            
  
                
                                           
  
           
                                                            
   
static int
sbuf_ensure_capacity(struct sbuf *s, size_t wanted)
{
	size_t size;

	size = sbuf_capacity(s);
	if (size >= wanted) {
		return 0;
	}

	return sbuf_extend(s, wanted - size);
}

   
                      
  
         
                          
  
           
            
  
              
                      
  
             
                          
  
           
                                                                          
              
   
int
sbuf_bcat(struct sbuf *s, const void *data, size_t len)
{
	if (SBUF_HASOVERFLOWED(s)) {
		return -1;
	}

	if (-1 == sbuf_ensure_capacity(s, len)) {
		SBUF_SETFLAG(s, SBUF_OVERFLOWED);
		return -1;
	}

	bcopy(data, s->s_buf + s->s_len, len);
	s->s_len += (int)len;           

	return 0;
}

   
                      
  
         
                                                                        
               
  
           
            
  
              
                   
  
             
                                 
  
           
                                                                            
   
int
sbuf_bcpy(struct sbuf *s, const void *data, size_t len)
{
	sbuf_clear(s);
	return sbuf_bcat(s, data, len);
}

   
                     
  
         
                                                                       
               
  
           
            
  
             
                        
  
           
                                                                      
              
   
int
sbuf_cat(struct sbuf *s, const char *str)
{
	return sbuf_bcat(s, str, strlen(str));
}

   
                     
  
         
                                                                        
                                                               
  
           
            
  
             
                                      
  
           
                                                                  
   
int
sbuf_cpy(struct sbuf *s, const char *str)
{
	sbuf_clear(s);
	return sbuf_cat(s, str);
}

   
                         
  
         
                                                
  
           
            
  
             
                     
  
            
                                   
  
           
                                                                      
              
   
int
sbuf_vprintf(struct sbuf *s, const char *fmt, va_list ap)
{
	va_list ap_copy;
	int result;
	size_t capacity;
	size_t len;

	if (SBUF_HASOVERFLOWED(s)) {
		return -1;
	}

	do {
		capacity = sbuf_capacity(s);

		va_copy(ap_copy, ap);
		                                                           
		result = vsnprintf(&s->s_buf[s->s_len], capacity + 1, fmt, ap_copy);
		va_end(ap_copy);

		if (result < 0) {
			return -1;
		}

		len = (size_t)result;
		if (len <= capacity) {
			s->s_len += (int)len;
			return 0;
		}
	} while (-1 != sbuf_ensure_capacity(s, len));

	SBUF_SETFLAG(s, SBUF_OVERFLOWED);
	return -1;
}

   
                        
  
         
                                                         
  
           
            
  
             
                     
  
           
                                                                      
              
   
int
sbuf_printf(struct sbuf *s, const char *fmt, ...)
{
	va_list ap;
	int result;

	va_start(ap, fmt);
	result = sbuf_vprintf(s, fmt, ap);
	va_end(ap);
	return result;
}

   
                      
  
         
                                                       
  
           
            
  
            
                           
  
           
                                                                              
                        
   
int
sbuf_putc(struct sbuf *s, int c_)
{
	char c = (char)c_;

	if (SBUF_HASOVERFLOWED(s)) {
		return -1;
	}

	if (-1 == sbuf_ensure_capacity(s, 1)) {
		SBUF_SETFLAG(s, SBUF_OVERFLOWED);
		return -1;
	}

	if (c != '\0') {
		s->s_buf[s->s_len++] = c;
	}

	return 0;
}

static inline int
isspace(char ch)
{
	return ch == ' ' || ch == '\n' || ch == '\t';
}

   
                      
  
         
                                              
  
           
            
  
           
                                                          
   
int
sbuf_trim(struct sbuf *s)
{
	if (SBUF_HASOVERFLOWED(s)) {
		return -1;
	}

	while (s->s_len > 0 && isspace(s->s_buf[s->s_len - 1])) {
		--s->s_len;
	}

	return 0;
}

   
                            
  
         
                                                      
  
           
            
  
           
                                               
   
int
sbuf_overflowed(struct sbuf *s)
{
	return !!SBUF_HASOVERFLOWED(s);
}

   
                        
  
         
                                               
  
           
            
   
void
sbuf_finish(struct sbuf *s)
{
	                                                              
	s->s_buf[s->s_len] = '\0';
	SBUF_CLEARFLAG(s, SBUF_OVERFLOWED);
	SBUF_SETFLAG(s, SBUF_FINISHED);
}

   
                      
  
         
                                           
  
           
            
  
           
                              
   
char *
sbuf_data(struct sbuf *s)
{
	return s->s_buf;
}

   
                     
  
         
                                                 
  
           
           
  
           
                                                                         
   
int
sbuf_len(struct sbuf *s)
{
	if (SBUF_HASOVERFLOWED(s)) {
		return -1;
	}

	return s->s_len;
}

   
                      
  
         
                                           
  
           
            
  
           
                                                   
   
int
sbuf_done(struct sbuf *s)
{
	return !!SBUF_ISFINISHED(s);
}

#if DEBUG || DEVELOPMENT

  
                       
   
#define SBUF_FAIL(a)                                                             \
    MACRO_BEGIN                                                                  \
	printf("sbuf_tests: failed assertion: %s\n", a);                         \
	if (what != NULL && should != NULL) {                                    \
	    printf("sbuf_tests: while testing: %s should %s\n", what, should);   \
	}                                                                        \
	goto fail;                                                               \
    MACRO_END

#define SBUF_PASS \
    ++passed

  
                 
   
#define SBUF_ASSERT(x)     \
    MACRO_BEGIN            \
	if (x) {           \
	    SBUF_PASS;     \
	} else {           \
	    SBUF_FAIL(#x); \
	}                  \
    MACRO_END

#define SBUF_ASSERT_NOT(x) \
    SBUF_ASSERT(!(x))

  
               
             
                 
   
#define SBUF_ASSERT_CMP(e, a, c)         \
    MACRO_BEGIN                          \
	if ((a) c (e)) {                 \
	    SBUF_PASS;                   \
	} else {                         \
	    SBUF_FAIL(#a " " #c " " #e); \
	}                                \
    MACRO_END

#define SBUF_ASSERT_EQ(e, a)    SBUF_ASSERT_CMP(e, a, ==)
#define SBUF_ASSERT_NE(e, a)    SBUF_ASSERT_CMP(e, a, !=)
#define SBUF_ASSERT_GT(e, a)    SBUF_ASSERT_CMP(e, a, >)
#define SBUF_ASSERT_GTE(e, a)   SBUF_ASSERT_CMP(e, a, >=)
#define SBUF_ASSERT_LT(e, a)    SBUF_ASSERT_CMP(e, a, <)
#define SBUF_ASSERT_LTE(e, a)   SBUF_ASSERT_CMP(e, a, <=)

#define SBUF_TEST_BEGIN      \
    size_t passed = 0;       \
    const char *what = NULL; \
    const char *should = NULL;

  
                                                                              
              
   
#define SBUF_TESTING(f) \
    MACRO_BEGIN         \
	what = (f);     \
    MACRO_END;

#define SBUF_SHOULD(s) \
    MACRO_BEGIN        \
	should = (s);  \
    MACRO_END;

#define SBUF_TEST_END                                      \
    printf("sbuf_tests: %zu assertions passed\n", passed); \
    return 0;                                              \
fail:                                                      \
    return ENOTRECOVERABLE;

static int
sysctl_sbuf_tests SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int rval = 0;
	char str[32] = { 'o', 'k', 0 };

	rval = sysctl_handle_string(oidp, str, sizeof(str), req);
	if (rval != 0 || req->newptr == 0 || req->newlen < 1) {
		return rval;
	}

	SBUF_TEST_BEGIN;

	SBUF_TESTING("sbuf_new")
	{
		SBUF_SHOULD("fail to allocate >INT_MAX")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, INT_MAX + 1, 0);
			SBUF_ASSERT_EQ(NULL, s);
		}

		SBUF_SHOULD("fail when claiming a backing buffer >INT_MAX")
		{
			struct sbuf *s = NULL;
			char buf[4] = { 0 };

			s = sbuf_new(NULL, buf, INT_MAX + 1, 0);
			SBUF_ASSERT_EQ(NULL, s);
		}

		SBUF_SHOULD("fail to allocate a zero-length sbuf")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 0, 0);
			SBUF_ASSERT_EQ(NULL, s);
		}

		SBUF_SHOULD("not accept invalid flags")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0x10000);
			SBUF_ASSERT_EQ(NULL, s);
		}

		SBUF_SHOULD("succeed when passed an existing sbuf")
		{
			struct sbuf *s = NULL;
			struct sbuf existing;

			memset(&existing, 0x41, sizeof(existing));
			s = sbuf_new(&existing, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(&existing, s);
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_AUTOEXTEND));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_DYNAMIC));
			SBUF_ASSERT_NE(NULL, s->s_buf);
			SBUF_ASSERT_NE(0, s->s_size);
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed when passed an existing sbuf and buffer")
		{
			struct sbuf *s = NULL;
			struct sbuf existing;
			char buf[4] = { 0 };

			memset(&existing, 0x41, sizeof(existing));
			s = sbuf_new(&existing, buf, sizeof(buf), 0);
			SBUF_ASSERT_EQ(&existing, s);
			SBUF_ASSERT_EQ(buf, s->s_buf);
			SBUF_ASSERT_EQ(4, s->s_size);
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed without an existing sbuf or buffer")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_NE(NULL, s);
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_DYNAMIC));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_DYNSTRUCT));
			SBUF_ASSERT_NE(NULL, s->s_buf);
			SBUF_ASSERT_NE(0, s->s_size);
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed without an existing sbuf, but with a buffer")
		{
			struct sbuf *s = NULL;
			char buf[4] = { 0 };

			s = sbuf_new(NULL, buf, sizeof(buf), 0);
			SBUF_ASSERT_NE(NULL, s);
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_DYNSTRUCT));
			SBUF_ASSERT_EQ(buf, s->s_buf);
			SBUF_ASSERT_EQ(4, s->s_size);
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("round up the requested size if SBUF_AUTOEXTEND")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 1, SBUF_AUTOEXTEND);
			SBUF_ASSERT_GT(1, s->s_size);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_clear")
	{
		SBUF_SHOULD("clear the overflowed and finished flags")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);

			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));
			SBUF_SETFLAG(s, SBUF_FINISHED);
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_FINISHED));
			sbuf_clear(s);
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_OVERFLOWED));
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_FINISHED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("reset the position to zero")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);

			s->s_len = 1;
			sbuf_clear(s);
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_extend")
	{
		SBUF_SHOULD("allow zero")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_extend(s, 0));
			SBUF_ASSERT_EQ(size_before, s->s_size);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail for sbuf not marked as SBUF_AUTOEXTEND")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_extend(s, 10));

			sbuf_delete(s);
		}

		SBUF_SHOULD("accommodate reasonable requests")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;

			SBUF_ASSERT_EQ(0, sbuf_extend(s, 10));
			SBUF_ASSERT_GTE(10, s->s_size - size_before);

			sbuf_delete(s);
		}

		SBUF_SHOULD("reject requests that cause overflows")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(-1, sbuf_extend(s, SIZE_MAX));
			SBUF_ASSERT_EQ(-1, sbuf_extend(s, INT_MAX));

			sbuf_delete(s);
		}

		SBUF_SHOULD("transform the sbuf into an SBUF_DYNAMIC one")
		{
			struct sbuf *s = NULL;
			char buf[4] = { 0 };

			s = sbuf_new(NULL, buf, sizeof(buf), SBUF_AUTOEXTEND);
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_DYNAMIC));
			SBUF_ASSERT_EQ(0, sbuf_extend(s, 10));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_DYNAMIC));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_capacity")
	{
		SBUF_SHOULD("account for the trailing nul byte")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(s->s_size - s->s_len - 1, sbuf_capacity(s));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_ensure_capacity")
	{
		SBUF_SHOULD("return 0 if the sbuf already has enough capacity")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_ensure_capacity(s, 5));
			SBUF_ASSERT_EQ(size_before, s->s_size);

			sbuf_delete(s);
		}

		SBUF_SHOULD("extend the buffer as needed")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_ensure_capacity(s, 30));
			SBUF_ASSERT_GT(size_before, s->s_size);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_bcat")
	{
		SBUF_SHOULD("fail if the sbuf is marked as overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_bcat(s, "A", 1));

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if len is too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(-1, sbuf_bcat(s, "A", INT_MAX));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf within limits")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "ABC", 3));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for binary data, even with nul bytes")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "A\0C", 3));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('\0', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("append to existing data")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "ABC", 3));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "DEF", 3));
			SBUF_ASSERT_EQ(6, s->s_len);
			SBUF_ASSERT_EQ('D', s->s_buf[3]);
			SBUF_ASSERT_EQ('E', s->s_buf[4]);
			SBUF_ASSERT_EQ('F', s->s_buf[5]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf right up to the limit")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "0123456789abcde", 15));
			SBUF_ASSERT_EQ(15, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail for a fixed buf if too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_bcat(s, "0123456789abcdef", 16));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("expand the backing buffer as needed")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_bcat(s, "0123456789abcdef", 16));
			SBUF_ASSERT_GT(size_before, s->s_size);
			SBUF_ASSERT_EQ(16, s->s_len);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_bcpy")
	{
		SBUF_SHOULD("overwrite any existing data")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "ABC", 3));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "XYZ123", 6));
			SBUF_ASSERT_EQ(6, s->s_len);
			SBUF_ASSERT_EQ('X', s->s_buf[0]);
			SBUF_ASSERT_EQ('Y', s->s_buf[1]);
			SBUF_ASSERT_EQ('Z', s->s_buf[2]);
			SBUF_ASSERT_EQ('1', s->s_buf[3]);
			SBUF_ASSERT_EQ('2', s->s_buf[4]);
			SBUF_ASSERT_EQ('3', s->s_buf[5]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed if the sbuf is marked as overflowed, but there is space")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "A", 1));

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if len is too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(-1, sbuf_bcpy(s, "A", INT_MAX));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf within limits")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "ABC", 3));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf right up to the limit")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "0123456789abcde", 15));
			SBUF_ASSERT_EQ(15, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail for a fixed buf if too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_bcpy(s, "0123456789abcdef", 16));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("expand the backing buffer as needed")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "0123456789abcdef", 16));
			SBUF_ASSERT_GT(size_before, s->s_size);
			SBUF_ASSERT_EQ(16, s->s_len);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_cat")
	{
		SBUF_SHOULD("fail if the sbuf is marked as overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_cat(s, "A"));

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf within limits")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cat(s, "ABC"));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("only copy up to a nul byte")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cat(s, "A\0C"));
			SBUF_ASSERT_EQ(1, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("append to existing data")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cat(s, "ABC"));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			SBUF_ASSERT_EQ(0, sbuf_cat(s, "DEF"));
			SBUF_ASSERT_EQ(6, s->s_len);
			SBUF_ASSERT_EQ('D', s->s_buf[3]);
			SBUF_ASSERT_EQ('E', s->s_buf[4]);
			SBUF_ASSERT_EQ('F', s->s_buf[5]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf right up to the limit")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cat(s, "0123456789abcde"));
			SBUF_ASSERT_EQ(15, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail for a fixed buf if too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_cat(s, "0123456789abcdef"));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("expand the backing buffer as needed")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_cat(s, "0123456789abcdef"));
			SBUF_ASSERT_GT(size_before, s->s_size);
			SBUF_ASSERT_EQ(16, s->s_len);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_cpy")
	{
		SBUF_SHOULD("overwrite any existing data")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "ABC"));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "XYZ123"));
			SBUF_ASSERT_EQ(6, s->s_len);
			SBUF_ASSERT_EQ('X', s->s_buf[0]);
			SBUF_ASSERT_EQ('Y', s->s_buf[1]);
			SBUF_ASSERT_EQ('Z', s->s_buf[2]);
			SBUF_ASSERT_EQ('1', s->s_buf[3]);
			SBUF_ASSERT_EQ('2', s->s_buf[4]);
			SBUF_ASSERT_EQ('3', s->s_buf[5]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed if the sbuf is marked as overflowed, but there is space")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(0, sbuf_bcpy(s, "A", 1));

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf within limits")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "ABC"));
			SBUF_ASSERT_EQ(3, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ('B', s->s_buf[1]);
			SBUF_ASSERT_EQ('C', s->s_buf[2]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("succeed for a fixed buf right up to the limit")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "0123456789abcde"));
			SBUF_ASSERT_EQ(15, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail for a fixed buf if too big")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_cpy(s, "0123456789abcdef"));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("expand the backing buffer as needed")
		{
			struct sbuf *s = NULL;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "0123456789abcdef"));
			SBUF_ASSERT_GT(size_before, s->s_size);
			SBUF_ASSERT_EQ(16, s->s_len);

			sbuf_delete(s);
		}
	}

	                              
	SBUF_TESTING("sbuf_printf")
	{
		SBUF_SHOULD("support simple printing")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(0, sbuf_printf(s, "hello"));
			SBUF_ASSERT_EQ(5, s->s_len);
			SBUF_ASSERT_EQ('h', s->s_buf[0]);
			SBUF_ASSERT_EQ('e', s->s_buf[1]);
			SBUF_ASSERT_EQ('l', s->s_buf[2]);
			SBUF_ASSERT_EQ('l', s->s_buf[3]);
			SBUF_ASSERT_EQ('o', s->s_buf[4]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("support format strings")
		{
			struct sbuf *s = NULL;
			char data1 = 'A';
			int data2 = 123;
			const char *data3 = "foo";

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(0, sbuf_printf(s, "%c %d %s", data1, data2, data3));
			SBUF_ASSERT_EQ(9, s->s_len);
			SBUF_ASSERT_EQ('A', s->s_buf[0]);
			SBUF_ASSERT_EQ(' ', s->s_buf[1]);
			SBUF_ASSERT_EQ('1', s->s_buf[2]);
			SBUF_ASSERT_EQ('2', s->s_buf[3]);
			SBUF_ASSERT_EQ('3', s->s_buf[4]);
			SBUF_ASSERT_EQ(' ', s->s_buf[5]);
			SBUF_ASSERT_EQ('f', s->s_buf[6]);
			SBUF_ASSERT_EQ('o', s->s_buf[7]);
			SBUF_ASSERT_EQ('o', s->s_buf[8]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("work with the fact we reserve a nul byte at the end")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_printf(s, "0123456789abcde"));
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("mark the sbuf as overflowed if we try to write too much")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(-1, sbuf_printf(s, "0123456789abcdef"));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("auto-extend as necessary")
		{
			struct sbuf *s = NULL;
			const char *data = "0123456789abcdef";
			int size_before;
			size_t n;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_printf(s, "%s", data));
			SBUF_ASSERT_GT(size_before, s->s_size);

			for (n = 0; n < strlen(data); ++n) {
				SBUF_ASSERT_EQ(data[n], s->s_buf[n]);
			}

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if the sbuf is marked as overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_printf(s, "A"));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_putc")
	{
		SBUF_SHOULD("work where we have capacity")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_putc(s, 'a'));
			SBUF_ASSERT_EQ(1, s->s_len);
			SBUF_ASSERT_EQ('a', s->s_buf[0]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if we have a full, fixedlen sbuf")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_FIXEDLEN);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "0123456789abcd"));
			SBUF_ASSERT_EQ(0, sbuf_putc(s, 'e'));
			SBUF_ASSERT_EQ(-1, sbuf_putc(s, 'f'));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_OVERFLOWED));

			sbuf_delete(s);
		}

		SBUF_SHOULD("ignore nul")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(0, sbuf_putc(s, '\0'));
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("auto-extend if necessary")
		{
			struct sbuf *s = NULL;
			int len_before;
			int size_before;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "0123456789abcde"));
			len_before = s->s_len;
			size_before = s->s_size;
			SBUF_ASSERT_EQ(0, sbuf_putc(s, 'f'));
			SBUF_ASSERT_EQ(len_before + 1, s->s_len);
			SBUF_ASSERT_GT(size_before, s->s_size);
			SBUF_ASSERT_EQ('f', s->s_buf[s->s_len - 1]);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if the sbuf is overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, SBUF_AUTOEXTEND);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_putc(s, 'a'));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_trim")
	{
		SBUF_SHOULD("remove trailing spaces, tabs and newlines")
		{
			struct sbuf *s = NULL;
			const char *test = "foo    \t\t\n\t";

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, test));
			SBUF_ASSERT_EQ(strlen(test), s->s_len);
			SBUF_ASSERT_EQ(0, sbuf_trim(s));
			SBUF_ASSERT_EQ(3, s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("do nothing if there is no trailing whitespace")
		{
			struct sbuf *s = NULL;
			const char *test = "foo";

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, test));
			SBUF_ASSERT_EQ(strlen(test), s->s_len);
			SBUF_ASSERT_EQ(0, sbuf_trim(s));
			SBUF_ASSERT_EQ(strlen(test), s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("fail if the sbuf is overflowed")
		{
			struct sbuf *s = NULL;
			const char *test = "foo   ";

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, test));
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_trim(s));
			SBUF_ASSERT_EQ(strlen(test), s->s_len);

			sbuf_delete(s);
		}

		SBUF_SHOULD("work on empty strings")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_trim(s));
			SBUF_ASSERT_EQ(0, s->s_len);

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_overflowed")
	{
		SBUF_SHOULD("return false if it hasn't overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_NOT(sbuf_overflowed(s));

			sbuf_delete(s);
		}

		SBUF_SHOULD("return true if it has overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT(sbuf_overflowed(s));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_finish")
	{
		SBUF_SHOULD("insert a nul byte, clear the overflowed flag and set the finished flag")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_putc(s, 'A'));
			s->s_buf[s->s_len] = 'x';
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_FINISHED));

			sbuf_finish(s);

			SBUF_ASSERT_EQ(0, s->s_buf[s->s_len]);
			SBUF_ASSERT_NOT(SBUF_ISSET(s, SBUF_OVERFLOWED));
			SBUF_ASSERT(SBUF_ISSET(s, SBUF_FINISHED));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_data")
	{
		SBUF_SHOULD("return the s_buf pointer")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(s->s_buf, sbuf_data(s));

			sbuf_delete(s);
		}

		SBUF_SHOULD("return the buffer we gave it")
		{
			struct sbuf *s = NULL;
			char buf[4] = { 0 };

			s = sbuf_new(NULL, buf, sizeof(buf), 0);
			SBUF_ASSERT_EQ(buf, sbuf_data(s));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_len")
	{
		SBUF_SHOULD("return the length of the sbuf data")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "hello"));
			SBUF_ASSERT_EQ(5, sbuf_len(s));

			sbuf_delete(s);
		}

		SBUF_SHOULD("return -1 if the sbuf is overflowed")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_EQ(0, sbuf_cpy(s, "hello"));
			SBUF_ASSERT_EQ(5, sbuf_len(s));
			SBUF_SETFLAG(s, SBUF_OVERFLOWED);
			SBUF_ASSERT_EQ(-1, sbuf_len(s));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_done")
	{
		SBUF_SHOULD("return false if the sbuf isn't finished")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_NOT(sbuf_done(s));

			sbuf_delete(s);
		}

		SBUF_SHOULD("return true if the sbuf has finished")
		{
			struct sbuf *s = NULL;

			s = sbuf_new(NULL, NULL, 16, 0);
			SBUF_ASSERT_NOT(sbuf_done(s));
			SBUF_SETFLAG(s, SBUF_FINISHED);
			SBUF_ASSERT(sbuf_done(s));

			sbuf_delete(s);
		}
	}

	SBUF_TESTING("sbuf_delete")
	{
		SBUF_SHOULD("just free the backing buffer if we supplied an sbuf")
		{
			struct sbuf *s = NULL;
			struct sbuf existing = {};

			s = sbuf_new(&existing, NULL, 16, 0);
			SBUF_ASSERT_NE(NULL, s->s_buf);

			sbuf_delete(s);
			SBUF_ASSERT_EQ(NULL, s->s_buf);
		}
	}

	SBUF_TEST_END;
}

SYSCTL_PROC(_kern, OID_AUTO, sbuf_test, CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_KERN | CTLFLAG_MASKED, 0, 0, sysctl_sbuf_tests, "A", "sbuf tests");

#endif                           
