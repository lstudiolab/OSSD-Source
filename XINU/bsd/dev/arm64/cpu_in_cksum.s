/*
 * Copyright (c) 2012-2018 Apple Inc. All rights reserved.
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

   
                                                                           
                                                                            
                                               
  
                                                  
   

#ifdef KERNEL
#define	CKSUM_ERR _kprintf
#else
#ifndef LIBSYSCALL_INTERFACE
#error "LIBSYSCALL_INTERFACE not defined"
#endif                            
#define	CKSUM_ERR _fprintf_stderr
#endif              

  
                      
  
                                                                           
                                                                        
                                                                              
                                           
   
#if defined(__LP64__)
#define	M_NEXT	0
#define	M_DATA	16	                                                      
#define	M_LEN	24
#else
#define	M_NEXT	0
#define	M_DATA	8
#define	M_LEN	12
#endif

	.globl	_os_cpu_in_cksum_mbuf
	.text
	.align	4
_os_cpu_in_cksum_mbuf:


  
                  
  
                                                                   
                                                                     
                                                                 
                           
   

   
           
                                                                               
    
             
                          
                           
                  
                                         
  
                     
                     
  
                       
                           
                      
   

	#define	m		x0
	#define	len		x1
	#define	off		x2
	#define	sum		x3
	#define	needs_swap	x4
	#define	started_on_odd	x5
	#define	mlen			x6
	#define	Wmlen			w6
	#define t       x7
	#define	data	x8
#if defined(__LP64__)
	#define ptr_m		x0
	#define ptr_data	x8
#else
	#define ptr_m		w0
	#define ptr_data	w8
#endif


	mov	needs_swap, #0		                      
	mov	started_on_odd, #0	                          
	mov	w3, w3			                    


  
             
                                   
                                              
                 
     
                    
                     
                 
                                     
                              
     
                
                 
           
                  
    
   

0:
	cbz	m, Lin_cksum_whoops	                            
	ldr	Wmlen, [m, #M_LEN]	                   
	cmp	mlen, off
	b.le	1f
	ldr	ptr_data, [m, #M_DATA]	                     
	sub	mlen, mlen, off		               
	add	data, data, off		                                   
	b	L_post_initial_offset
1:
	sub	off, off, mlen
	cbnz	len, 2f
	mov	x0, x3
	ret	lr
2:
	ldr	ptr_m, [m, #M_NEXT]
	b	0b

L_loop:	                                   
  
                                   
                                              
                 
     
                    
                              
   
	cbz	m, Lin_cksum_whoops	                            
	ldr	Wmlen, [m, #M_LEN]	                   
	ldr	ptr_data, [m, #M_DATA]	                     

L_post_initial_offset:
  
                            
                               
                
   

	cbz	mlen, L_continue
	cmp	mlen, len
	csel	mlen, mlen, len, le
	sub	len, len, mlen

  
                
                              
                                      
                          
            
            
     
                                
   

	tst	data, #1
	mov	x7, #0
	mov	x10, #0
	b.eq	1f
	ldrb	w7, [data], #1
	eor	started_on_odd, started_on_odd, #1
	sub	mlen, mlen, #1
	lsl	w7, w7, #8
1:


  
                              
                  
                          
                                          
               
               
     
   
	tst	data, #2
	mov	needs_swap, started_on_odd
	b.eq	1f
	cmp	mlen, #2
	b.lt	L_trailing_bytes
	ldrh	w9, [data], #2
	sub	mlen, mlen, #2
	add	w7, w7, w9
1:

  
                              
                  
                    
                                          
               
               
     
   
	                                          
	tst	data, #4
	b.eq	1f
	cmp	mlen, #4
	b.lt	L2_bytes
	ldr	w9, [data], #4
	sub	mlen, mlen, #4
	adds	w7, w7, w9
	adc	x7, x7, x10                                        
1:

  
                        
                                   
                                   
                                          
                                                
                                                
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                                                 
                
                
                                                   
                       
                                   
                             
                               
                                      
                    
        
     
  

	                                                                 
	subs	mlen, mlen, #64
	b.lt	L32_bytes

	                             
	sub	sp, sp, #8*16
	mov	x11, sp
	st1.4s	{v0, v1, v2, v3}, [x11], #4*16 
	st1.4s	{v4, v5, v6, v7}, [x11], #4*16 

	                                                  
	fmov	s3, w7
	eor.16b	v0, v0, v0
	eor.16b	v1, v1, v1
	eor.16b	v2, v2, v2

	                                          
	ld1.4s	{v4,v5,v6,v7},[data],#64

	                                  
	subs	mlen, mlen, #64
	b.lt	L64_finishup

	  
                                                          
                                        
    
L64_loop:
	subs        mlen, mlen, #64                          

	uadalp.2d   v0, v4
	ld1.4s      {v4},[data], #16

	uadalp.2d   v1, v5
	ld1.4s      {v5},[data], #16

	uadalp.2d   v2, v6
	ld1.4s      {v6},[data], #16

	uadalp.2d   v3, v7
	ld1.4s      {v7},[data], #16

	b.ge        L64_loop

L64_finishup:
	uadalp.2d   v0, v4
	uadalp.2d   v1, v5
	uadalp.2d   v2, v6
	uadalp.2d   v3, v7

	add.2d      v0, v0, v1
	add.2d      v2, v2, v3
	addp.2d     d0, v0
	addp.2d     d2, v2
	add.2d      v0, v0, v2
	fmov        x7, d0			                    

	                                
	ld1.4s      {v0, v1, v2, v3}, [sp], #4*16
	ld1.4s      {v4, v5, v6, v7}, [sp], #4*16

L32_bytes:
	tst     mlen, #32
	b.eq    L16_bytes
	ldp	x9, x10, [data], #16
	ldp	x11, x12, [data], #16
	adds	x7, x7, x9
	mov	x9, #0
	adcs	x7, x7, x10
	adcs	x7, x7, x11
	adcs	x7, x7, x12
	adc	x7, x7, x9

L16_bytes:
	tst	mlen, #16
	b.eq	L8_bytes
	ldp	x9, x10, [data], #16
	adds	x7, x7, x9
	mov	x9, #0
	adcs	x7, x7, x10
	adc	x7, x7, x9

L8_bytes:
	tst     mlen, #8
	mov	x10, #0
	b.eq    L4_bytes
	ldr	x9,[data],#8
	adds	x7, x7, x9
	adc	x7, x7, x10

L4_bytes:
	tst     mlen, #4
	b.eq    L2_bytes
	ldr	w9,[data],#4
	adds	x7, x7, x9
	adc	x7, x7, x10

L2_bytes:
	tst	mlen, #2
	b.eq	L_trailing_bytes
	ldrh	w9,[data],#2
	adds	x7, x7, x9
	adc	x7, x7, x10

L_trailing_bytes:
	tst     mlen, #1
	b.eq    L0_bytes
	ldrb	w9,[data],#1
	adds	x7, x7, x9
	adc	x7, x7, x10
	eor	started_on_odd, started_on_odd, #1

L0_bytes:
  
                   
                                                
   
	cbz	needs_swap, 1f
	ror	x7, x7, #56
1:
  
                                                    
                                           
    
   

	add	x3, x3, x7, lsr #32
	mov	w7, w7
	add	x3, x3, x7
	mov	w7, w3
	add	x3, x7, x3, lsr #32

L_continue:
	cmp	len, #0
	ldr     ptr_m, [m, #M_NEXT]			                
	b.gt	L_loop

  
                                                     
                                               
                                                        
                                                        
                               
    
   

	mov	w4, #0x00ffff
	and	x0, x4, x3, lsr #48
	and	x1, x4, x3, lsr #32
	and	x2, x4, x3, lsr #16
	and	x3, x4, x3
	add	w0, w0, w1
	add	w2, w2, w3
	add	w0, w0, w2
	and	w1, w4, w0, lsr #16
	and	w0, w4, w0
	add	w0, w0, w1
	and	w1, w4, w0, lsr #16
	and	w0, w4, w0
	add	w0, w0, w1
	  
                                                      
   
                      
    
	and	w0, w0, w4

	ret	lr

Lin_cksum_whoops:
	adrp	x0, Lin_cksum_whoops_str@page
	add	x0, x0, Lin_cksum_whoops_str@pageoff
	bl	#CKSUM_ERR
	mov	x0, #-1
	ret	lr

Lin_cksum_whoops_str:
	.asciz	"os_cpu_in_cksum_mbuf: out of data\n"
	.align	5
