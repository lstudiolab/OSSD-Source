/*
 * Copyright (c) 2000-2013 Apple Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                                                          

                                                                                                             

        
                                                   
                                                   
                                                                            
                                                         

         

                                                                                      

                                                                                                                                         

                                       
                    
                                   
                                                                             
                                                                                       
                                                                              

                                                                                                                                         

                                                                                                                                                   

                        
                    
                 
                                    
                                                                                                                                     
                                                                                                        
                                                             
    
 
                

                                                                              
                    

  

    #define MZV_MAGIC           17185                                                       

	#define	ZERO				0
	#define	PARTIAL_MATCH		1
	#define	MISS_TAG			2
	#define	MATCH				3

	.text
	.syntax unified
	.align	4

	                                                                                                    

	.globl _WKdm_decompress_new
_WKdm_decompress_new:

	  
                                            
                                                                                                                               
   

	#define	src_buf			r0
	#define	dest_buf		r1
	#define	scratch			r2
	#define	eax				r3
	#define	ebx				r4
	#define	hashTable		r4
	#define	ecx				r5
	#define	edx				r6
    #define n_bytes         r8
	#define	next_tag		r12
	#define	tags_counter	lr
	#define	dictionary		sp
	#define	v0		q0
	#define	v1		q1
	#define	v2		q2
	#define	v3		q3
	#define	v4		q4
	#define	v5		q5

	                                         

                           
                                           
                                                
                                                  

	push	{r7, lr}
	mov		r7, sp
	push	{r4-r6,r8-r11}
#if KERNEL
	sub		ecx, sp, #96
	sub		sp, sp, #96
	vst1.64	{q0,q1},[ecx]!
	vst1.64	{q2,q3},[ecx]!
	vst1.64	{q4,q5},[ecx]!
#endif
	sub		sp, sp, #64			                          

    mov     n_bytes, r3                                                                    
    ldr     eax, [src_buf]                                                          
    mov     ecx, #MZV_MAGIC
    cmp     eax, ecx                                                                               
    bne     L_default_decompressor                                              

                                                                               
                                                    
    add     src_buf, src_buf, 4                                   
    mov     eax, dest_buf
    mov     ecx, #4096                                                        
    mov     r9, #0
    mov     r10, #0
    mov     r11, #0
    mov     r12, #0
1:
    subs    ecx, ecx, #64
    stmia   eax!, {r9-r12}
    stmia   eax!, {r9-r12}
    stmia   eax!, {r9-r12}
    stmia   eax!, {r9-r12}
    bne     1b

    mov     r12, #4                                                                         
2:
    ldr     eax, [src_buf], #4                                 
    ldrh    edx, [src_buf], #2                                  
    str     eax, [dest_buf, edx]                                                             
    add     r12, r12, #6                                                 
    cmp     r12, n_bytes                                                             
    bne     2b
    b       L_done
                                                    

L_default_decompressor:
	
      
                                                                                                   
   
                                                                                           
	vmov.i32	q0, #0
	mov		r8, sp
	adr		ebx, _table_2bits
    vst1.64	{q0}, [r8]!
	add		r10, src_buf, #268			                
    vst1.64	{q0}, [r8]!
	add		eax, src_buf, #12			                   
    vst1.64	{q0}, [r8]!
	mov		ecx, scratch				                
    vst1.64	{q0}, [r8]!
	vld1.64	{q0,q1},[ebx,:128]


	                                                                                    
  
                                                           
                                  
                                          
                                                             
                                                               
                                                                                 
  

L_WK_unpack_2bits:
	vld1.64	{v5}, [eax]!				                                        
	vdup.32	v2, d10[0]					                          
	vdup.32	v3, d10[1]					                          
	vdup.32	v4, d11[0]					                          
	vdup.32	v5, d11[1]					                          
	vshl.u32	v2, v2, v0				                       
	vshl.u32	v3, v3, v0				                       
	vshl.u32	v4, v4, v0				                       
	vshl.u32	v5, v5, v0				                       
	vand	v2, v2, v1					                   
	vand	v3, v3, v1					                   
	vand	v4, v4, v1					                   
	vand	v5, v5, v1					                   
	vst1.64	{v2,v3}, [ecx,:128]!		                                   
	cmp		r10, eax					                                   
	vst1.64	{v4,v5}, [ecx,:128]!		                                   
	bhi	L_WK_unpack_2bits				                                                       


	                                                                                    

	ldm		src_buf, {r8,r9}			                                 
	adr		ebx, _table_4bits
	subs	r12, r9, r8					                                            
	add		r8, src_buf, r8, lsl #2		                  
	add		r9, src_buf, r9, lsl #2		                
	bls		1f							                                                              
	add		ecx, scratch, #1024			                
	vld1.64	{v0,v1},[ebx,:128]

	subs	r12, r12, #1	
	bls		2f							                                 
L_WK_unpack_4bits:
	vld1.64	{d4}, [r8]!					                                            
	subs	r12, r12, #2
	vmov	d5, d4
	vzip.32	d4, d5
	vshl.u32	v2, v2, v0				                      
	vand	v2, v2, v1					                       
	vst1.64	{q2}, [ecx,:128]!
	bhi		L_WK_unpack_4bits	
2:
	adds	r12, r12, #1	
	ble	1f

	ldr		r12, [r8], #4				                                            
	vdup.32	d4, r12						                          
	vshl.u32	v2, v2, v0				               
	vand	v2, v2, v1					                       
	vst1.64	{d4}, [ecx,:64]!			                                   

1:

	                                                                                                   

	ldr		eax, [src_buf,#8]			                           
	add		r8, src_buf, eax, lsl #2	                    
	cmp		r8, r9						                                           
	add		ecx, scratch, #2048			                    
	add		edx, scratch, #4096			               
	bls		1f							                                            

	adr		ebx, _table_10bits
	vld1.64	{v0,v1},[ebx,:128]

	mov		r11, #0x03ff
L_WK_unpack_3_tenbits:
	ldr		r12, [r9], #4				                                       
	and		lr, r11, r12
	strh	lr, [ecx], #2
	cmp		ecx, edx
	and		lr, r11, r12, lsr #10
	beq		1f
	strh	lr, [ecx], #2
	and		lr, r11, r12, lsr #20
	strh	lr, [ecx], #2

	cmp		r8, r9						                                           
	bhi		L_WK_unpack_3_tenbits		                                               
 
1:
	  
                                                 
   

	mov		next_tag, scratch			                
	add		r8, scratch, #1024			            
	add		r11, scratch, #2048			                    
#if defined(KERNEL) && !SLIDABLE
    adr     hashTable, L_table
    ldr     hashTable, [hashTable]
#else
    ldr     hashTable, L_table
L_table0:
    ldr     hashTable, [pc, hashTable]
#endif
	mov		tags_counter, #1024			               

	b		L_next

	.align 4,0x90
L_ZERO_TAG:
	  
                                                            
                   
   
	subs	tags_counter,tags_counter,#1	                 
	str		r9, [dest_buf], #4				                  
	ble		L_done							                                          

	                               
L_next:
	ldrb	r9, [next_tag], #1				          
	cmp		r9, #0
	beq		L_ZERO_TAG 
	cmp		r9, #2							                      
	beq		L_MISS_TAG
	bgt		L_EXACT_TAG

L_PARTIAL_TAG:
	  
                           
                                         
                                                                                 
   
	ldrb	edx, [r8], #1					                      
	ldrh	ecx, [r11], #2					                                      
	ldr		eax, [dictionary, edx, lsl #2]	                       
	subs	tags_counter,tags_counter,#1	                 
	lsr		eax, eax, #10					                      
	orr		eax, ecx, eax, lsl #10			                                            
	str		eax, [dictionary,edx,lsl #2]	                                      
	str		eax, [dest_buf], #4				                                  
	bgt		L_next							                                           

L_done:

	add		sp, sp, #64			                            

	                                                      
#if KERNEL
	vld1.64	{q0,q1},[sp]!
	vld1.64	{q2,q3},[sp]!
	vld1.64	{q4,q5},[sp]!
#endif
	pop		{r4-r6,r8-r11}
	pop		{r7,pc}

	.align 4,0x90
L_MISS_TAG:
	  
                            
                    
                    
                     
                                   
   
	subs	tags_counter,tags_counter,#1	                 
	ldr		eax, [r10], #4					                        
	lsr		edx, eax, #10					        
	str		eax, [dest_buf], #4				                     
	and		edx, edx, #0x0ff				                         
	ldrb	edx, [ebx, edx]					       
	str		eax, [dictionary,edx]			                          
	bgt		L_next							                  
	b		L_done							                                          

	.align 4,0x90

L_EXACT_TAG:
	   
                          
                                           
   

	ldrb	eax, [r8], #1					                      
	subs	tags_counter,tags_counter,#1	                 
	ldr		eax, [dictionary,eax,lsl #2]	                       
	str		eax, [dest_buf], #4				                  
	bgt		L_next							                  
	b		L_done							                                          


	.align 4

_table_2bits:
	.word	0
	.word	-2
	.word	-4
	.word	-6
	.word	0x03030303
	.word	0x03030303
	.word	0x03030303
	.word	0x03030303

_table_4bits:
	.word	0
	.word	-4
	.word	0
	.word	-4
	.word	0x0f0f0f0f
	.word	0x0f0f0f0f
	.word	0x0f0f0f0f
	.word	0x0f0f0f0f

_table_10bits:
	.word	0
	.word	-10
	.word	-20
	.word	0
	.word	1023
	.word	1023
	.word	1023
	.word	0


#if defined(KERNEL) && !SLIDABLE
    .align  2
L_table:
    .long   _hashLookupTable_new
#else
    .align  2
L_table:
    .long   L_Tab$non_lazy_ptr-(L_table0+8)

     .section    __DATA,__nl_symbol_ptr,non_lazy_symbol_pointers
    .align  2
L_Tab$non_lazy_ptr:
    .indirect_symbol    _hashLookupTable_new
    .long   0
#endif

