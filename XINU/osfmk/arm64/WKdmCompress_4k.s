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

  
                                                                                        

                                                                                                       

        
                                                       
                                                                  
                                                                              
                                                   

         

                                                                                                                                                                                          
                                                                                                               
                                                                                                                                                        
                                                                                                                                       
                                                                                                                                                                                     

                                                                      
                                                                                                                                                                 
                                                                                                  

                                                         

                                                                                                                                                  
                                                                                                                                                                                      
                                                    
                                                                                  
                                                                                                        

                                                         
           
                                                                                      
                                                                                                                      
                                                                   

                                                                                                                
                                    
                                                                              
                                                      
                                                        

                                                                                                                                                                                 

                                               
                    
                                   
                                                                             
                                                                                       
                                                                              

                                                                                                                                            
                                                                                                                                                                                    


                                                                                                                                                                                      

                     

                                                                                   
                                                                                           
                                                                                                                  

                                                                                                                                                                                      
                        

                                                                                                                     

                                                                                                                                              

                                                             

                                              

                      

                                                                                                                                           

                                                                                

                              
                              
                                    

                       
                  
                                     
                 
           

                                           
                    
                              
                                       

                                          
                                       
                 
                          
                                                                  
                                                               
                 
                                         
                           
                                
                           
                                     
                 
                     
                                
     
    
  

                                                                                                              

                                                                        
                                                                                           
                                                                                               

                

                                                                              
                    
  

#ifndef PAGES_SIZE_IN_KBYTES    
#define PAGES_SIZE_IN_KBYTES    4
#endif

#if !((PAGES_SIZE_IN_KBYTES==4) || (PAGES_SIZE_IN_KBYTES==16))
#error "Only PAGES_SIZE_IN_KBYTES = 4 or 16 is supported"
#endif


	.text
	.align 4

  
                                                                                                      
  
 
.globl _WKdm_compress_4k
_WKdm_compress_4k:

  
                                                                                                       
  
	#define	src_buf				x0
	#define	next_input_word		x0
	#define	dest_buf			x1
	#define	scratch				x2
	#define	byte_count			x3
	#define	next_tag			x4
	#define	tempTagsArray		x2		          
	#define	dictionary			x5
	#define	remaining			x6
	#define	next_full_patt		x7
	#define	dict_location		x8
	#define	wdict_location		w8
	#define	next_qp				x9
	#define	hashTable			x10
	#define tempQPosArray		x11
	#define	next_low_bits		x12

  
                                                               
                                                                             
                                               
  
	#define	rax					x13
	#define	eax					w13
	#define	rcx					x14
	#define	ecx					w14
	#define	rdx					x15
	#define	edx					w15
	#define	rdi					x0			                                                       	


   
                                                                                      

                                                                                                   
                 
                             
                                
                                
                                   
  

#define	scale	(PAGES_SIZE_IN_KBYTES/4)

#define SV_RETURN           0                                                                
#define MZV_MAGIC           17185                                                                    
#define CHKPT_BYTES         416                                                                                                                 
#define CHKPT_WORDS         (CHKPT_BYTES/4)                                     
#define CHKPT_TAG_BYTES     (CHKPT_BYTES/16)                                                    
#define CHKPT_SHRUNK_BYTES  426                                                                                                      
                                                                                                                      
#if CHKPT_BYTES > 4096
    #error CHKPT_BYTES must be <= 4096
#endif
#if CHKPT_BYTES < 4
    #error CHKPT_BYTES must be >= 4
#endif

#if KERNEL
    sub     sp, sp, #64
    st1.4s  {v0,v1,v2,v3},[sp]
#endif

    sub     sp, sp, #64					                          
	mov		dictionary, sp				                                                      

    sub     sp, sp, #64                                                                    
	mov		x15, sp
    stp     x20, x21, [x15, #0]                         
    stp     x22, x23, [x15, #16]                        
    stp     x24, x25, [x15, #32]                        
    stp     x26, x27, [x15, #48]                        

  
                                                                                                         
  

                                                                                                          
                                                                                                      
	mov		next_tag, tempTagsArray			                    
	add		next_qp, scratch, #(1024*scale)	          
	mov		remaining, #(CHKPT_WORDS*scale)                                                        
	add		next_full_patt, dest_buf, #(12+256*scale) 	                                                           
	sub		byte_count, byte_count, #(12+256*scale)	                            
	add		next_low_bits, scratch, #(2048*scale)	                       
	stp		xzr, xzr, [dictionary, #0]		                        
	adrp    hashTable, _hashLookupTable@GOTPAGE
	stp		xzr, xzr, [dictionary, #16]		                        
	stp		xzr, xzr, [dictionary, #32]		                        
    ldr 	hashTable, [hashTable, _hashLookupTable@GOTPAGEOFF]
	stp		xzr, xzr, [dictionary, #48]		                        

#define EARLYCHECK              0
#define NORMAL                  1

#define mode                    w20
#define start_next_full_patt    x21
#define start_next_input_word   x22
#define start_next_low_bits     x23
#define r11                     x24
#define r13                     x25
#define byte_budget             x26
#define start_next_qp           tempQPosArray

	add		tempQPosArray, scratch, #(1024*scale)	                        
    mov     mode, EARLYCHECK                                                                               
    mov     start_next_full_patt, next_full_patt                                               
    mov     start_next_input_word, next_input_word                                              
    mov     start_next_low_bits, next_low_bits                                               
    add     byte_budget, byte_count, #(12+256*scale)                               

	b		L_loop

	.align	4, 0x90

	                                                  
L_RECORD_ZERO:
	strb	edx, [next_tag], #1				                                                                            
	subs	remaining, remaining, #1		             
	b.le	CHECKPOINT   					                          

	                                                                     
L_loop:

	                                
	ldr		edx, [next_input_word], #4
	cbz		edx, L_RECORD_ZERO							                                 

	  
                                                                                                           
   
	ubfm	eax, edx, #10, #17
	ldrb	wdict_location, [hashTable, rax]		                                       
	ldr		eax, [dictionary, dict_location]		                              

	                                                                        
	eor		eax, eax, edx							                          
	cbz		eax, L_RECORD_EXACT						                             
	lsr		eax, eax, #10							                                  
	cbz		eax, L_RECORD_PARTIAL					                               

L_RECORD_MISS:
  
                                                                        
                                        
                                             
  
	subs	byte_count, byte_count, #4				                  
	b.le	L_budgetExhausted						                                                    
	str		edx, [next_full_patt], #4				                                  
	mov		eax, #2									               
	subs	remaining, remaining, #1				             
	str		edx, [dictionary, dict_location]		                              
	strb	eax, [next_tag], #1						                           
	b.gt	L_loop									                              
    b       CHECKPOINT

L_done_search:

	                                                
	                                                                                         

	sub		rax, next_full_patt, dest_buf			                                    
	lsr		eax, eax, #2							                       
	str		eax, [dest_buf]							                                          

	                                                                                                             
	                                                                                                      

	add		rdi, dest_buf, #12						           
	mov		rcx, tempTagsArray						                    

L_pack_2bits:
	ld1.2s  {v0,v1,v2,v3},[rcx],#32

	shl.2d	v1,v1,#4
	shl.2d	v3,v3,#4

	orr.8b	v0, v0, v1
	orr.8b	v2, v2, v3

	ushr.2d	v1, v0, #30
	ushr.2d	v3, v2, #30

	orr.8b	v0, v0, v1
	orr.8b	v2, v2, v3

	zip1.2s	v0, v0, v2
	st1.2s  {v0},[rdi],#8
	cmp		next_tag, rcx
	b.hi	L_pack_2bits	

	                                                                                                                           

	                                                                                            
	sub		rax, next_qp, tempQPosArray				                                                               
	add		eax, eax, #7							                      
	lsr		eax, eax, #3							                                                  
	add		rcx, tempQPosArray, rax, lsl #3			                                                    
	lsl		rax, rax, #2
	subs	byte_count, byte_count, rax
	b.lt	L_budgetExhausted	

	cmp		rcx, next_qp							                          
	b.ls	2f 										                                                                
	sub		rax, rcx, next_qp
	mov		edx, #0
	tst		eax, #4
	b.eq	1f
	str		edx, [next_qp], #4
1:	tst		eax, #2
	b.eq	1f
	strh	edx, [next_qp], #2
1:	tst		eax, #1
	b.eq	2f
	strb	edx, [next_qp], #1
2:
	mov		rdi, next_full_patt						                 
	cmp		rcx, tempQPosArray						                                
	ldr		eax, [dest_buf] 
	b.ls	L20										                                                        
	mov		rdx, tempQPosArray						                

	                                               
L_pack_4bits:
	ldr		rax, [rdx], #8							                          
	orr		rax, rax, rax, lsr #28					                                         
	cmp		rcx, rdx								                         
	str		eax, [rdi], #4							                       
	b.hi	L_pack_4bits							                                                

	                                                  
	sub		rax, rdi, dest_buf						                          
	lsr		eax, eax, #2							                                   
L20:
	str		eax, [dest_buf,#4]						                                        



	                                                                                                                                          

	add		rcx, scratch, #(2048*scale)				                   
    sub		rdx, next_low_bits, rcx					                                              
	lsr		rdx, rdx, #1							                                      
	subs	edx, edx, #3							                                         
	b.lt	1f										                                                      
0:
	subs	byte_count, byte_count, #4				                  
	b.le	L_budgetExhausted						                                                    
	subs	edx, edx, #3							                         
	ldr		rax, [rcx], #6
	bfm		rax, rax, #58, #9						                      
	bfm		rax, rax, #58, #25						                          
	lsr		rax, rax, #12	
	str		eax, [rdi], #4							                                     
	b.ge	0b										                                                

1: 	adds	edx, edx, #3							                                          
	b.eq	3f										                                                                
	subs	byte_count, byte_count, #4				                  
	b.le	L_budgetExhausted						                                                    
	ldrh	eax,[rcx]								     
	subs	edx, edx, #1							                        
	b.eq	2f										  
	ldrh	edx, [rcx, #2]							     
	orr		eax, eax, edx, lsl #10					                

2:	str		eax, [rdi], #4							                                

3:	sub		rax, rdi, dest_buf						                          
	lsr		eax, eax, #2							                                            
	str		eax, [dest_buf, #8]						                                               
	lsl		w0, eax, #2								                                            

L_done:

	                               
	mov		x15, sp
    ldp     x20, x21, [x15, #0]                                
    ldp     x22, x23, [x15, #16]                               
    ldp     x24, x25, [x15, #32]                               
    ldp     x26, x27, [x15, #48]                               
    add     sp, sp, #128					                                                   

#if KERNEL
	ld1.4s  {v0,v1,v2,v3},[sp],#64
#endif
	ret		lr

    .align  4
L_budgetExhausted:
    mov     x0, #-1
    b       L_done


	.align 4,0x90
L_RECORD_EXACT:
  
                                                                               
                                                
  
	mov		eax, #3
	lsr		w14, wdict_location, #2				                              
	subs	remaining, remaining, #1			             
	strb	eax, [next_tag], #1					                            
	strb	w14, [next_qp], #1					                                 
	b.gt	L_loop
	b		CHECKPOINT   						                          

	.align 4,0x90
L_RECORD_PARTIAL:
  
                                                                                               
                                                            
  
	mov		ecx, #1
	strb	ecx, [next_tag], #1					                                      
	str		edx, [dictionary, dict_location]	                               
	subs	remaining, remaining, #1			             
	lsr		eax, wdict_location, #2				                        
	and		edx, edx, #1023						                
	strb	eax, [next_qp], #1					                    
	strh	edx, [next_low_bits], #2			                       
	b.gt	L_loop

CHECKPOINT:

    cbz     mode, L_check_compression_ratio                                                   
    
L_check_zero_page:

    cmp     start_next_full_patt, next_full_patt                                                 
    b.ne    L_check_single_value_page

    cmp     start_next_qp, next_qp                                                                         
    b.ne    L_check_single_value_page

    mov     x0, #SV_RETURN                                                   
    b       L_done

L_check_single_value_page:

    sub     rax, next_full_patt, start_next_full_patt                             
    lsr     rax, rax, #2

    sub     r11, next_qp, start_next_qp                                                           
    
    sub     r13, next_low_bits, start_next_low_bits                                     
    lsr     r13, r13, #1

                                                         
                                                                                            
                                                                                 
      
    cbnz    r13, 1f                                                                  

    cmp     r11, #(256*PAGES_SIZE_IN_KBYTES - 1)                                          
    b.ne    1f
    
    cmp     rax, #1                                                                           
    b.ne    1f
    
    ldrb    edx, [tempTagsArray]                                                
    cmp     edx, #2                                                                    
    b.eq    L_is_single_value_page

1:
    cmp     r13, #1                                                                
    b.ne    L_check_mostly_zero

    cmp     r11, #(256*PAGES_SIZE_IN_KBYTES)                                             
    b.ne    L_check_mostly_zero

    ldrb    edx, [tempTagsArray]                                                
    cmp     edx, #1                                                                       
    b.ne    L_check_mostly_zero

L_is_single_value_page:

    mov     x0, #SV_RETURN                                                   
    b       L_done

L_check_mostly_zero:
                                                                                                      
    add     rax, rax, r11                                                                  
    mov     rdx, #6
    mov     rcx, #4
    madd    r11, rax, rdx, rcx                                                                                                 

    sub     rax, next_low_bits, start_next_low_bits                                           
    mov     rdx, #1365
    mul     rax, rax, rdx

    sub     rdx, next_full_patt, start_next_full_patt                                             
    add     rax, rdx, rax, lsr #11                                                                                                                  
    
    sub     rdx, next_qp, start_next_qp
    add     rax, rax, rdx, lsr #1                                                            
    add     rax, rax, #(12+256*scale)                                                             

    cmp     rax, r11                                                                               
    b.lt    L_done_search

    cmp     r11, byte_budget                                                                               
    b.gt    L_budgetExhausted

L_sparse_packer:
    mov     edx, #MZV_MAGIC
    str     edx, [dest_buf], #4                                                              

    mov     rdx, #0                                                                              
1:
    ldr     rax, [start_next_input_word, rdx]                              
    cbnz    rax, 5f                                                     
3:
    add     rdx, rdx, #8                                                                   
4:
    cmp     rdx, #(4096*scale)                                                               
    b.ne    1b
    mov     x0, r11                                                                               
    b       L_done

5:
    cbz     eax, 6f                                                          
    str     eax, [dest_buf], #4                                                                   
    strh    edx, [dest_buf], #2                                                
6:
    lsr     rax, rax, 32                                                                   
    cbz     eax, 3b                                                     
    add     rdx, rdx, #4
    str     eax, [dest_buf], #4                                                                   
    strh    edx, [dest_buf], #2                                                
    add     rdx, rdx, #4
    b       4b

L_check_compression_ratio:

    mov     mode, NORMAL
	mov		remaining, #((1024 - CHKPT_WORDS)*scale)                                       
    cbz     remaining, CHECKPOINT                                                                    
    
    sub     rax, next_low_bits, start_next_low_bits                                           
    mov     rdx, #1365
    mul     rax, rax, rdx

    sub     rdx, next_full_patt, start_next_full_patt                                             
    add     rax, rdx, rax, lsr #11                                                                                                                  

    sub     rdx, next_qp, start_next_qp
    add     rax, rax, rdx, lsr #1                                                            
    subs    rax, rax, #((CHKPT_SHRUNK_BYTES - CHKPT_TAG_BYTES)*scale)
                                                                                                            

    b.gt    L_budgetExhausted                                                                   
    b       L_loop                                                    
