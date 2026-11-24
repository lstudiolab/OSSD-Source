/*
 * Copyright (c) 2016-2016 Apple Inc. All rights reserved.
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

                     
              
                                                                               
                                      

#include "lz4.h"
#define memcpy __builtin_memcpy

size_t
lz4raw_decode_buffer(uint8_t * __restrict dst_buffer, size_t dst_size,
    const uint8_t * __restrict src_buffer, size_t src_size,
    void * __restrict work __attribute__((unused)))
{
	const uint8_t * src = src_buffer;
	uint8_t * dst = dst_buffer;

	                                                          
#if LZ4_ENABLE_ASSEMBLY_DECODE
	if (dst_size > LZ4_GOFAST_SAFETY_MARGIN && src_size > LZ4_GOFAST_SAFETY_MARGIN) {
		if (lz4_decode_asm(&dst, dst_buffer, dst_buffer + dst_size - LZ4_GOFAST_SAFETY_MARGIN, &src, src_buffer + src_size - LZ4_GOFAST_SAFETY_MARGIN)) {
			return 0;        
		}
	}
#endif
                                                                                   

	              
	if (lz4_decode(&dst, dst_buffer, dst_buffer + dst_size, &src, src_buffer + src_size)) {
		return 0;        
	}
	return (size_t)(dst - dst_buffer);                  
}
              
#if LZ4DEBUG
#define DEBUG_LZ4_ENCODE_ERRORS (1)
#define DEBUG_LZ4_DECODE_ERRORS (1)
#endif

#if DEBUG_LZ4_ENCODE_ERRORS
#endif

#if !LZ4_ENABLE_ASSEMBLY_ENCODE

#if defined(__x86_64__) || defined(__x86_64h__)
# define LZ4_MATCH_SEARCH_INIT_SIZE 32
# define LZ4_MATCH_SEARCH_LOOP_SIZE 32
#else
# define LZ4_MATCH_SEARCH_INIT_SIZE 8
# define LZ4_MATCH_SEARCH_LOOP_SIZE 8
#endif

                                    
static inline uint32_t
lz4_hash(uint32_t x)
{
	return (x * 2654435761U) >> (32 - LZ4_COMPRESS_HASH_BITS);
}

                           
static inline void
lz4_fill16(uint8_t * ptr)
{
	store8(ptr, -1);
	store8(ptr + 8, -1);
}

                                                             
static inline size_t
lz4_nmatch4(const uint8_t * a, const uint8_t * b)
{
	uint32_t x = load4(a) ^ load4(b);
	return (x == 0)?4:(__builtin_ctzl(x) >> 3);
}

                                                             
static inline size_t
lz4_nmatch8(const uint8_t * a, const uint8_t * b)
{
	uint64_t x = load8(a) ^ load8(b);
	return (x == 0)?8:(__builtin_ctzll(x) >> 3);
}

                                                              
static inline size_t
lz4_nmatch16(const uint8_t * a, const uint8_t * b)
{
	size_t n = lz4_nmatch8(a, b);
	return (n == 8)?(8 + lz4_nmatch8(a + 8, b + 8)):n;
}

                                                              
static inline size_t
lz4_nmatch32(const uint8_t * a, const uint8_t * b)
{
	size_t n = lz4_nmatch16(a, b);
	return (n == 16)?(16 + lz4_nmatch16(a + 16, b + 16)):n;
}

                                                              
static inline size_t
lz4_nmatch64(const uint8_t * a, const uint8_t * b)
{
	size_t n = lz4_nmatch32(a, b);
	return (n == 32)?(32 + lz4_nmatch32(a + 32, b + 32)):n;
}

                                                                                     
static inline size_t
lz4_nmatch(int N, const uint8_t * a, const uint8_t * b)
{
	switch (N) {
	case  4: return lz4_nmatch4(a, b);
	case  8: return lz4_nmatch8(a, b);
	case 16: return lz4_nmatch16(a, b);
	case 32: return lz4_nmatch32(a, b);
	case 64: return lz4_nmatch64(a, b);
	}
	__builtin_trap();        
}

                                                                                                                                      
                                                                     
                                                              
static inline uint8_t *
lz4_store_length(uint8_t * dst, const uint8_t * const end, uint32_t L)
{
	(void)end;
	while (L >= 17 * 255) {
		lz4_fill16(dst);
		dst += 16;
		L -= 16 * 255;
	}
	lz4_fill16(dst);
	                                                                       
	dst += L / 255;
	*dst++ = L % 255;
	return dst;
}

static inline uint32_t
clamp(uint32_t x, uint32_t max)
__attribute__((overloadable))
{
	return x > max ? max : x;
}

static inline uint8_t *
copy_literal(uint8_t *dst, const uint8_t * restrict src, uint32_t L)
{
	uint8_t *end = dst + L;
	{ copy16(dst, src); dst += 16; src += 16; }
	while (dst < end) {
		copy32(dst, src); dst += 32; src += 32;
	}
	return end;
}

static uint8_t *
lz4_emit_match(uint32_t L, uint32_t M, uint32_t D,
    uint8_t * restrict dst,
    const uint8_t * const end,
    const uint8_t * restrict src)
{
	                                                                  
	                                                                 
	                                 
	assert(M >= 4 && "LZ4 encoding requires that M is at least 4");
	                                                    
	M -= 4;
	                                                                    
	                                                
	assert(D <= USHRT_MAX && "LZ4 encoding requries that D can be stored in two bytes.");
	                                                                   
	                                                      
	*dst++ = clamp(L, 15) << 4 | clamp(M, 15);
	                                                                        
	if (L >= 15) {
		dst = lz4_store_length(dst, end, L - 15);
		if (dst == 0 || dst + L >= end) {
			return NULL;
		}
	}
	                                            
	dst = copy_literal(dst, src, L);
	                         
	store2(dst, D); dst += 2;
	                                                                      
	if (M >= 15) {
		dst = lz4_store_length(dst, end, M - 15);
		if (dst == 0) {
			return NULL;
		}
	}
	return dst;
}

                             
                                 
            

#if LZ4_EARLY_ABORT
int lz4_do_early_abort = 1;
int lz4_early_aborts = 0;
#define LZ4_EARLY_ABORT_EVAL (448)
#define LZ4_EARLY_ABORT_MIN_COMPRESSION_FACTOR (20)
#endif                      

void
lz4_encode_2gb(uint8_t ** dst_ptr,
    size_t dst_size,
    const uint8_t ** src_ptr,
    const uint8_t * src_begin,
    size_t src_size,
    lz4_hash_entry_t hash_table[LZ4_COMPRESS_HASH_ENTRIES],
    int skip_final_literals)
{
	uint8_t *dst = *dst_ptr;                                   
	uint8_t *end = dst + dst_size - LZ4_GOFAST_SAFETY_MARGIN;
	const uint8_t *src = *src_ptr;                                          
	const uint8_t *src_end = src + src_size - LZ4_GOFAST_SAFETY_MARGIN;
	const uint8_t *match_begin = 0;                                  
	const uint8_t *match_end = 0;                                     
#if LZ4_EARLY_ABORT
	uint8_t * const dst_begin = dst;
	uint32_t lz4_do_abort_eval = lz4_do_early_abort;
#endif

	while (dst < end) {
		ptrdiff_t match_distance = 0;
		for (match_begin = src; match_begin < src_end; match_begin += 1) {
			const uint32_t pos = (uint32_t)(match_begin - src_begin);
			const uint32_t w0 = load4(match_begin);
			const uint32_t w1 = load4(match_begin + 1);
			const uint32_t w2 = load4(match_begin + 2);
			const uint32_t w3 = load4(match_begin + 3);
			const int i0 = lz4_hash(w0);
			const int i1 = lz4_hash(w1);
			const int i2 = lz4_hash(w2);
			const int i3 = lz4_hash(w3);
			const uint8_t *c0 = src_begin + hash_table[i0].offset;
			const uint8_t *c1 = src_begin + hash_table[i1].offset;
			const uint8_t *c2 = src_begin + hash_table[i2].offset;
			const uint8_t *c3 = src_begin + hash_table[i3].offset;
			const uint32_t m0 = hash_table[i0].word;
			const uint32_t m1 = hash_table[i1].word;
			const uint32_t m2 = hash_table[i2].word;
			const uint32_t m3 = hash_table[i3].word;
			hash_table[i0].offset = pos;
			hash_table[i0].word = w0;
			hash_table[i1].offset = pos + 1;
			hash_table[i1].word = w1;

			hash_table[i2].offset = pos + 2;
			hash_table[i2].word = w2;
			hash_table[i3].offset = pos + 3;
			hash_table[i3].word = w3;

			match_distance = (match_begin - c0);
			if (w0 == m0 && match_distance < 0x10000 && match_distance > 0) {
				match_end = match_begin + 4;
				goto EXPAND_FORWARD;
			}

			match_begin++;
			match_distance = (match_begin - c1);
			if (w1 == m1 && match_distance < 0x10000 && match_distance > 0) {
				match_end = match_begin + 4;
				goto EXPAND_FORWARD;
			}

			match_begin++;
			match_distance = (match_begin - c2);
			if (w2 == m2 && match_distance < 0x10000 && match_distance > 0) {
				match_end = match_begin + 4;
				goto EXPAND_FORWARD;
			}

			match_begin++;
			match_distance = (match_begin - c3);
			if (w3 == m3 && match_distance < 0x10000 && match_distance > 0) {
				match_end = match_begin + 4;
				goto EXPAND_FORWARD;
			}

#if LZ4_EARLY_ABORT
			                                                                           
			if (lz4_do_abort_eval && ((pos) >= LZ4_EARLY_ABORT_EVAL)) {
				ptrdiff_t dstd = dst - dst_begin;

				if (dstd == 0) {
					lz4_early_aborts++;
					return;
				}

                                   
                                
                  
                                               
                                                                                
                                
                  
				lz4_do_abort_eval = 0;
			}
#endif
		}

		if (skip_final_literals) {
			*src_ptr = src; *dst_ptr = dst; return;
		}                                                                                                 

		                                                                           
		                                                                           
		size_t src_remaining = src_end + LZ4_GOFAST_SAFETY_MARGIN - src;
		if (src_remaining < 15) {
			*dst++ = (uint8_t)(src_remaining << 4);
			memcpy(dst, src, 16); dst += src_remaining;
		} else {
			*dst++ = 0xf0;
			dst = lz4_store_length(dst, end, (uint32_t)(src_remaining - 15));
			if (dst == 0 || dst + src_remaining >= end) {
				return;
			}
			memcpy(dst, src, src_remaining); dst += src_remaining;
		}
		*dst_ptr = dst;
		*src_ptr = src + src_remaining;
		return;

EXPAND_FORWARD:

		                       
		{
			const uint8_t * ref_end = match_end - match_distance;
			while (match_end < src_end) {
				size_t n = lz4_nmatch(LZ4_MATCH_SEARCH_LOOP_SIZE, ref_end, match_end);
				if (n < LZ4_MATCH_SEARCH_LOOP_SIZE) {
					match_end += n; break;
				}
				match_end += LZ4_MATCH_SEARCH_LOOP_SIZE;
				ref_end += LZ4_MATCH_SEARCH_LOOP_SIZE;
			}
		}

		                        
		{
			                                                            
			const uint8_t * match_begin_min = src_begin + match_distance;
			match_begin_min = (match_begin_min < src)?src:match_begin_min;
			const uint8_t * ref_begin = match_begin - match_distance;

			while (match_begin > match_begin_min && ref_begin[-1] == match_begin[-1]) {
				match_begin -= 1; ref_begin -= 1;
			}
		}

		             
		dst = lz4_emit_match((uint32_t)(match_begin - src), (uint32_t)(match_end - match_begin), (uint32_t)match_distance, dst, end, src);
		if (!dst) {
			return;
		}

		               
		src = match_end;

		                                                               
		*dst_ptr = dst;
		*src_ptr = src;
	}
}

#endif

size_t
lz4raw_encode_buffer(uint8_t * __restrict dst_buffer, size_t dst_size,
    const uint8_t * __restrict src_buffer, size_t src_size,
    lz4_hash_entry_t hash_table[LZ4_COMPRESS_HASH_ENTRIES])
{
	                         
	const lz4_hash_entry_t HASH_FILL = { .offset = 0x80000000, .word = 0x0 };

	const uint8_t * src = src_buffer;
	uint8_t * dst = dst_buffer;

	                                                                           
	const size_t BLOCK_SIZE = 0x7ffff000;
	while (src_size > 0) {
		                                                                
		                                                                   
		                                                            
		                                                           
		                                  
                         
                                                                           
           
                                                                                  
            

		for (int i = 0; i < LZ4_COMPRESS_HASH_ENTRIES;) {
			hash_table[i++] = HASH_FILL;
			hash_table[i++] = HASH_FILL;
			hash_table[i++] = HASH_FILL;
			hash_table[i++] = HASH_FILL;
		}

		                                
		const size_t src_to_encode = src_size > BLOCK_SIZE ? BLOCK_SIZE : src_size;

		                                                                                                       
		                                                                                                   
		uint8_t * dst_start = dst;
		const uint8_t * src_start = src;
		lz4_encode_2gb(&dst, dst_size, &src, src, src_to_encode, hash_table, src_to_encode < src_size);

		                 
		size_t dst_used = dst - dst_start;
		size_t src_used = src - src_start;                             
		if (src_to_encode == src_size && src_used < src_to_encode) {
			return 0;                                                                    
		}
		                                                                                                      
		                                                                                                       
		                                                                                           
		if (src_to_encode < src_size && src_to_encode - src_used >= (1 << 16)) {
			return 0;                                                                           
		}
		                                                          
		src_size -= src_used;
		dst_size -= dst_used;
	}

	return (size_t)(dst - dst_buffer);                  
}

typedef uint32_t lz4_uint128 __attribute__((ext_vector_type(4))) __attribute__((__aligned__(1)));

int
lz4_decode(uint8_t ** dst_ptr,
    uint8_t * dst_begin,
    uint8_t * dst_end,
    const uint8_t ** src_ptr,
    const uint8_t * src_end)
{
	uint8_t * dst = *dst_ptr;
	const uint8_t * src = *src_ptr;

	                          
	if (dst_end <= dst) {
		goto OUT_FULL;
	}

	while (src < src_end) {
		                          
		*src_ptr = src;
		*dst_ptr = dst;

		uint8_t cmd = *src++;                                                                                             
		uint32_t literalLength = (cmd >> 4) & 15;         
		uint32_t matchLength = 4 + (cmd & 15);         

		                                
		if (__improbable(literalLength == 15)) {
			uint8_t s;
			do {
#if DEBUG_LZ4_DECODE_ERRORS
				if (__improbable(src >= src_end)) {
					printf("Truncated SRC literal length\n");
				}
#endif
				if (__improbable(src >= src_end)) {
					goto IN_FAIL;                                                             
				}
				s = *src++;
				literalLength += s;
			} while (__improbable(s == 255));
		}

		               
#if DEBUG_LZ4_DECODE_ERRORS
		if (__improbable(literalLength > (size_t)(src_end - src))) {
			printf("Truncated SRC literal\n");
		}
#endif
		if (__improbable(literalLength > (size_t)(src_end - src))) {
			goto IN_FAIL;
		}
		if (__improbable(literalLength > (size_t)(dst_end - dst))) {
			                                                                
			                                   
			literalLength = (uint32_t)(dst_end - dst);
			memcpy(dst, src, literalLength);
			dst += literalLength;
			goto OUT_FULL;
		}
		memcpy(dst, src, literalLength);
		src += literalLength;
		dst += literalLength;

		if (__improbable(src >= src_end)) {
			goto OUT_FULL;                                                        
		}
#if DEBUG_LZ4_DECODE_ERRORS
		if (__improbable(2 > (size_t)(src_end - src))) {
			printf("Truncated SRC distance\n");
		}
#endif
		if (__improbable(2 > (size_t)(src_end - src))) {
			goto IN_FAIL;                                                                              
		}
		                                                                  
		                                      
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-align"

		                 
		uint64_t matchDistance = *(const uint16_t *)src;                                                 
	#pragma clang diagnostic pop
		src += 2;
#if DEBUG_LZ4_DECODE_ERRORS
		if (matchDistance == 0) {
			printf("Invalid match distance D = 0\n");
		}
#endif
		if (__improbable(matchDistance == 0)) {
			goto IN_FAIL;                                                             
		}
		uint8_t * ref = dst - matchDistance;
#if DEBUG_LZ4_DECODE_ERRORS
		if (__improbable(ref < dst_begin)) {
			printf("Invalid reference D=0x%llx dst_begin=%p dst=%p dst_end=%p\n", matchDistance, dst_begin, dst, dst_end);
		}
#endif
		if (__improbable(ref < dst_begin)) {
			goto OUT_FAIL;                                                          
		}
		                              
		if (__improbable(matchLength == 19)) {
			uint8_t s;
			do {
#if DEBUG_LZ4_DECODE_ERRORS
				if (__improbable(src >= src_end)) {
					printf("Truncated SRC match length\n");
				}
#endif
				if (__improbable(src >= src_end)) {
					goto IN_FAIL;                                                                          
				}
				s = *src++;
				matchLength += s;
			} while (__improbable(s == 255));
		}

		                           
		if (__improbable(matchLength > (size_t)(dst_end - dst))) {
			                                                              
			                                   
			matchLength = (uint32_t)(dst_end - dst);
			for (uint32_t i = 0; i < matchLength; ++i) {
				dst[i] = ref[i];
			}
			dst += matchLength;
			goto OUT_FULL;
		}
		for (uint64_t i = 0; i < matchLength; i++) {
			dst[i] = ref[i];
		}
		dst += matchLength;
	}

	                                                                   
OUT_FULL:
	                                              
	*dst_ptr = dst;
	*src_ptr = src;
	return 0;

	                    
OUT_FAIL:
IN_FAIL:
	return 1;        
}
