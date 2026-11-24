/*
 * Copyright (c) 2000-2014 Apple Inc. All rights reserved.
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

  
                                                                                          

                                                                                                             

        
                                                   
                                                   
                                                              
                                                                

         

                                                                                      

                                                                                                                                         

                                       
                    
                                   
                                                                             
                                                                                       
                                                                              

                                                                                                                          

                                                                                                                                                   

                        
                    
                 
                                    
                                                                                                                                     
                                                                                                        
                                                             
    
 
                   

                                                                              
                    
  

#define PAGES_SIZE_IN_KBYTES    16
#define MZV_MAGIC               17185                                                       

#ifndef PAGES_SIZE_IN_KBYTES    
#define PAGES_SIZE_IN_KBYTES    4
#endif

#if !((PAGES_SIZE_IN_KBYTES==4) || (PAGES_SIZE_IN_KBYTES==16))
#error "Only PAGES_SIZE_IN_KBYTES = 4 or 16 is supported"
#endif

#define	scale (PAGES_SIZE_IN_KBYTES/4)


	.align	4
	.text

  
                                                                                                   
  

	.globl _WKdm_decompress_16k
_WKdm_decompress_16k:

	  
                                            
                                                                                                                               
   

	#define	src_buf			x0
	#define	dest_buf		x1
	#define	scratch			x2
    #define n_bytes         x3
	#define	dictionary		sp
	#define	rax				x13
	#define	eax				w13
	#define	rbx				x4
	#define	ebx				w4
	#define	rcx				x5
	#define	ecx				w5
	#define	rdx				x6
	#define	edx				w6
	#define	tags_counter	x7
	#define	next_tag		x12
	#define	r8				x8
	#define	r9				x9
	#define	r10				x10
	#define	r11				x11
    #define r12             x12

	   

                                                         

                        
                                
                                   
                                      

   

#if KERNEL
	sub		rax, sp, #96
	sub		sp, sp, #96
	st1.4s	{v0,v1,v2},[rax],#48
	st1.4s	{v3,v4,v5},[rax],#48
#endif

	sub		sp, sp, #64

    ldr     eax, [src_buf]                                                          
    mov     ecx, #MZV_MAGIC
    cmp     eax, ecx                                                                               
    b.ne    L_default_decompressor                                              

                                                                               
                                                    
    add     src_buf, src_buf, 4                                   
    mov     rax, dest_buf
    mov     rcx, #(PAGES_SIZE_IN_KBYTES*1024)                                 
1:
    dc      zva, rax                                                                                                             
    add     rax, rax, #64
    dc      zva, rax
    add     rax, rax, #64
    dc      zva, rax
    add     rax, rax, #64
    dc      zva, rax
    add     rax, rax, #64
    subs    rcx, rcx, #256
    b.ne    1b

    mov     r12, #4                                                                         
    mov     rdx, #0
2:
    ldr     eax, [src_buf], #4                                 
    ldrh    edx, [src_buf], #2                                  
    str     eax, [dest_buf, rdx]                                                             
    add     r12, r12, #6                                                 
    cmp     r12, n_bytes                                                             
    b.ne    2b
    b       L_done
                                                    

L_default_decompressor:

      
                                                                                                   
   
                                                                                           
	adrp    rbx, _table_2bits@GOTPAGE
    stp     xzr, xzr, [dictionary, #0]
	add		r10, src_buf, #(12+256*scale)		                
    stp     xzr, xzr, [dictionary, #16]
	add		rax, src_buf, #12			                   
    ldr     rbx, [rbx, _table_2bits@GOTPAGEOFF]
    stp     xzr, xzr, [dictionary, #32]
	mov		rcx, scratch				                
    stp     xzr, xzr, [dictionary, #48]
	ld1.4s	{v0,v1},[rbx]


	   
                                                                                          
   

	                                                                                    
  
                                                           
                                  
                                          
                                                             
                                                               
                                                                                 
  

L_WK_unpack_2bits:
	ldr		q5, [rax], #16				                                        
	dup.4s	v2, v5[0]					                          
	dup.4s	v3, v5[1]					                          
	dup.4s	v4, v5[2]					                          
	dup.4s	v5, v5[3]					                          
	ushl.4s	v2, v2, v0					                       
	ushl.4s	v3, v3, v0					                       
	ushl.4s	v4, v4, v0					                       
	ushl.4s	v5, v5, v0					                       
	and.16b	v2, v2, v1					                   
	and.16b	v3, v3, v1					                   
	and.16b	v4, v4, v1					                   
	and.16b	v5, v5, v1					                   
	cmp		r10, rax					                                   
	st1.4s	{v2,v3,v4,v5}, [rcx], #64	                                   
	b.hi	L_WK_unpack_2bits			                                                       


	                                                                                    

	ldp		w8, w9, [src_buf]			                                 
	adrp    rbx, _table_4bits@GOTPAGE
	subs	x14, r9, r8					                                            
	add		r8, src_buf, r8, lsl #2		                  
	add		r9, src_buf, r9, lsl #2		                

	b.ls	1f							                                                              
    ldr     rbx, [rbx, _table_4bits@GOTPAGEOFF]
	add		rcx, scratch, #(1024*scale)		                
	ld1.4s	{v0,v1},[rbx]
	subs	w14, w14, #1	
	b.ls	2f							                                 
L_WK_unpack_4bits:
	ldr		d2, [r8], #8				                                            
	subs	w14, w14, #2
	zip1.4s	v2, v2, v2
	ushl.4s	v2, v2, v0					                      
	and.16b	v2, v2, v1					                       
	str		q2, [rcx], #16
	b.hi	L_WK_unpack_4bits	
2:
	adds	w14, w14, #1
	b.le	1f

	ldr		s3, [r8], #4				                                            
	dup.2s  v2, v3[0]					                          
	ushl.2s	v2, v2, v0					               
	and.8b	v2, v2, v1					                       
	str		d2, [rcx], #8				                                   

1:

	                                                                                                   

	ldr		eax, [src_buf,#8]			                           
	add		r8, src_buf, rax, lsl #2	                    
	add		rcx, scratch, #(2048*scale)	                    
#if (scale==1)
	add		r11, scratch, #(4096*scale-2)	                                  
#else
	add		r11, scratch, #(4096*scale)	                                  
	sub		r11, r11, #2
#endif
	subs	r8, r8, r9					                                           
	b.ls	1f							                                            

	adrp    rbx, _table_10bits@GOTPAGE
    ldr     rbx, [rbx, _table_10bits@GOTPAGEOFF]
	ld1.4s	{v0,v1,v2,v3},[rbx]

	  
                                                                                        
                                                                                                 
                                             

                                                                                                    
                    
   
#if 1
	subs	r8, r8, #4					                     
	ldr		s4, [r9], #4				                                      
	zip1.4s	v4,	v4,	v4	                                                                                   
	ushl.4s	v5,	v4,	v0	                                                                                  
	ushl.4s	v4,	v4,	v1	                                                                                      
	and.16b	v5,	v5,	v2	                                                                           
	and.16b v4,	v4,	v3	                                                                                          
	orr.16b	v4,	v4,	v5	               
	str		d4, [rcx], #6				                      
	b.eq	1f
#endif

	subs	r8, r8, #8					                     
	b.lt	L_WK_unpack_3_tenbits

L_WK_unpack_2x_3_tenbits:
	ldr		d4, [r9], #8				                                                  
	zip1.4s	v4,	v4,	v4	                                                                                   
	ushl.4s	v5,	v4,	v0	                                                                                  
	ushl.4s	v4,	v4,	v1	                                                                                      
	and.16b	v5,	v5,	v2	                                                                           
	and.16b v4,	v4,	v3	                                                                                          
	orr.16b	v4,	v4,	v5	               
	ins		v5.d[0], v4.d[1]
	str		d4, [rcx], #6				                      
	str		d5, [rcx], #6				                      

	subs	r8, r8, #8
	b.ge	L_WK_unpack_2x_3_tenbits		                                               

	tst		r8, #4
	b.eq	1f

L_WK_unpack_3_tenbits:
	ldr		s4, [r9]					                                      
	zip1.4s	v4,	v4,	v4	                                                                                   
	ushl.4s	v5,	v4,	v0	                                                                                  
	ushl.4s	v4,	v4,	v1	                                                                                      
	and.16b	v5,	v5,	v2	                                                                           
	and.16b v4,	v4,	v3	                                                                                          
	orr.16b	v4,	v4,	v5	               
#if 0
	str		d4, [rcx]	                      
#else
	cmp		rcx, r11
	b.eq	2f
	str		d4, [rcx]	                      
	b		1f
2:
	str		h4, [rcx]	                            
#endif
1:

	  
                                                 
   
	mov		next_tag, scratch				                
	add		r8, scratch, #(1024*scale)		            
	add		r11, scratch, #(2048*scale)		                    
	adrp    rbx, _hashLookupTable@GOTPAGE
	mov		tags_counter, #(1024*scale)		               
    ldr     rbx, [rbx, _hashLookupTable@GOTPAGEOFF]

	b		L_next

	.align 4,0x90
L_ZERO_TAG:
	  
                                                            
                   
   
	str		w9, [dest_buf], #4				                  
	subs	tags_counter, tags_counter, #1	                           
	b.ls	L_done							                                          

	                               
L_next:
	ldrb	w9, [next_tag], #1				          
	cbz		w9, L_ZERO_TAG
	cmp		w9, #2	 						                      
	b.eq	L_MISS_TAG
	b.gt	L_EXACT_TAG

L_PARTIAL_TAG:
	  
                           
                                         
                                                                                 
   

	ldrb	edx, [r8], #1					                      
	ldrh	ecx, [r11], #2					                                      
	ldr		eax, [dictionary, rdx, lsl #2]	                       
	bfm		eax, ecx, #0, #9				                                            
	str		eax, [dictionary,rdx,lsl #2]	                                      
	str		eax, [dest_buf], #4				                                  
	subs	tags_counter, tags_counter, #1	                           
	b.gt	L_next							                                           

L_done:

	                                                      
	add		sp, sp, #64					                            
#if KERNEL
	ld1.4s	{v0,v1,v2},[sp],#48
	ld1.4s	{v3,v4,v5},[sp],#48
#endif
	ret		lr

	.align 4,0x90
L_MISS_TAG:
	  
                            
                    
                    
                     
                                   
   
	ldr		eax, [r10], #4					                        
	ubfm	edx, eax, #10, #17				                         
	str		eax, [dest_buf], #4				                     
	ldrb	edx, [rbx, rdx]					       
	str		eax, [dictionary,rdx]			                          
	subs	tags_counter, tags_counter, #1	                           
	b.gt	L_next							                  
	b		L_done							                                          

	.align 4,0x90
L_EXACT_TAG:
	   
                          
                                           
   
	ldrb	eax, [r8], #1					                      
	ldr		eax, [dictionary,rax,lsl #2]	                       
	str		eax, [dest_buf], #4				                  
	subs	tags_counter, tags_counter, #1	                           
	b.gt	L_next							                  
	b		L_done							                                          


