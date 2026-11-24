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

  
                                                                                        

                                                                                                       

        
                                                       
                                                                  
                                                                              
                                                   

         

                                                                                                                                                                                          
                                                                                                               
                                                                                                                                                        
                                                                                                                                       
                                                                                                                                                                                     

                                                                      
                                                                                                                                                                 
                                                                                                  

                                                         

                                                                                                                                                  
                                                                                                                                                                                      
                                                    
                                                                                  
                                                                                                        

                                                         
           
                                                                                      
                                                                                                                      
                                                                   

                                                                                                                
                                    
                                                                              
                                                      
                                                        

                                                                                                                                                                                 

                                               
                    
                                   
                                                                             
                                                                                       
                                                                              

                                                                                                                                            
                                                                                                                                                                                    


                                                                                                                                                                                      

                     

                                                                                   
                                                                                           
                                                                                                                  

                                                                                                                                                                                      
                        

                                                                                                                     

                                                                                                                                     

                                                        

                                              

                      

                                                                     

                                                                                                                                                               

                                                                                

                              
                              
                                    

                       
                  
                                     
                 
           

                                           
                    
                              
                                       

                                          
                                       
                 
                          
                                                                  
                                                               
                 
                                         
                           
                                
                           
                                     
                 
                     
                                
     
    
  

                                                                                                              

                                                                        
                                                                                           
                                                                                               

                

                                                                              
                    
  
	.text
	.align 4

	                                                                                                        
 
.globl _WKdm_compress_new
_WKdm_compress_new:

  
                                                                                                       
  

	#define	src_buf				r0
	#define	next_input_word		r0
	#define	dest_buf			r1
	#define	scratch				r2
	#define	dictionary			sp
	#define	byte_count			r3

	#define	next_tag			r12

	#define	remaining			r4
	#define	next_full_patt		r5
	#define	dict_location		r6
	#define	next_qp				r8
	#define	hashTable			r9
	#define	next_low_bits		r10
	#define	eax					r11
	#define	ecx					r12
	#define	edx					lr
	#define	rdi					r6	

    #define tempTagsArray       scratch
    #define R11                 r0                                                                     
    #define R13                 r4                                                                     
   
                                                                                                            

                                                                               
                 
                             
                                
                                   

                     

  

	#define	TagsArray_offset	0
	#define	QPosArray_offset	1024
	#define	LowBitsArray_offset	2048

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

    push    {r7,lr}
    mov     r7, sp
    push    {r4-r6,r8-r11}

#if KERNEL
	sub		sp, sp, #32
	vst1.64 {q0,q1}, [sp]
#endif

	sub		sp, sp, #(64+24)					                                             

  
                                                                  
  
                                                                                                          
                                                                                                      
	mov		eax, #0

	mov		next_tag, scratch 				                    
	vdup.32 q0, eax

	add		next_qp, scratch, #QPosArray_offset		          
	mov		lr, sp
	mov		remaining, #(CHKPT_WORDS)                                                              
	vst1.64 {q0}, [lr]!
	add		next_full_patt, dest_buf, #268 	                                                
	vst1.64 {q0}, [lr]!
	vst1.64 {q0}, [lr]!
	add		next_low_bits, scratch, #LowBitsArray_offset	                       
	vst1.64 {q0}, [lr]!

#if defined(KERNEL) && !SLIDABLE
    adr     hashTable, L_table
    ldr     hashTable, [hashTable]
#else
    ldr     hashTable, L_table
L_table0:
    ldr     hashTable, [pc, hashTable]
#endif

#define EARLYCHECK              0
#define NORMAL                  1

#define mode                    [sp, #64]
#define start_next_full_patt    [sp, #68]
#define start_next_input_word   [sp, #72]
#define start_next_low_bits     [sp, #76]
#define byte_budget             [sp, #80]

    mov     edx, #EARLYCHECK
    str     edx, mode                                                                                  
    str     next_full_patt, start_next_full_patt                                           
    str     next_input_word, start_next_input_word                                          
    str     next_low_bits, start_next_low_bits                                            
    str     byte_count, byte_budget                                            

	sub		byte_count, byte_count, #(12+256)	                                         
	b		L_scan_loop

	.align	4, 0x90
L_RECORD_ZERO:
	                                                  
	strb	edx, [next_tag], #1						                      
	subs	remaining, remaining, #1				                        
	ble		CHECKPOINT                                                        

	                                 
L_scan_loop:
	ldr		edx, [next_input_word], #4
	cmp		edx, #0
	beq		L_RECORD_ZERO							                                 

	  
                                                                                                           
   
	and		eax, edx, #(0xff<<10)					                                          
	lsr		eax, eax, #10							                                
	ldrb	eax, [hashTable, eax]					                                       
	add		dict_location, dictionary, eax			                                                                
	ldr		eax, [dictionary, eax]					                              
	cmp		eax, edx								                          
	beq		L_RECORD_EXACT							                             

	eor		eax, eax, edx
	lsrs	eax, eax, #10							                       
	beq		L_RECORD_PARTIAL						                               

L_RECORD_MISS:
  
                                                                        
                                        
                                             
  

	subs	byte_count, byte_count, #4
	ble		L_budgetExhausted						                                                          
	str		edx, [next_full_patt], #4				                                  
	mov		eax, #2
	str		edx, [dict_location]					                              
	strb	eax, [next_tag], #1						                           
	subs	remaining, remaining, #1				                        
	bgt		L_scan_loop								                                           
    b       CHECKPOINT

L_done_search:

	                                                
	sub		eax, next_full_patt, dest_buf			                                    
	lsr		eax, eax, #2							                       
	str		eax, [dest_buf]							                                          


	                                                                                                             
	                                                                                                      

	add		rdi, dest_buf, #12						           
	mov		eax, scratch	 						                    
	sub		edx, next_tag, scratch					                      

	vld1.64	{q0,q1}, [eax,:128]!
	subs	edx, edx, #32				                      
L_pack_2bits:
	subs	edx, edx, #32
	vshl.i64	d1, d1, #4
	vshl.i64	d3, d3, #4
	vorr	d0, d0, d1
	vorr	d2, d2, d3
	vshr.u64	d1, d0, #30
	vshr.u64	d3, d2, #30
	vorr	d0, d0, d1
	vorr	d2, d2, d3
	vzip.32	d0, d2	
	vst1.64	{d0}, [rdi]!
	vld1.64	{q0,q1}, [eax,:128]!
	bgt		L_pack_2bits	
	vshl.i64	d1, d1, #4
	vshl.i64	d3, d3, #4
	vorr	d0, d0, d1
	vorr	d2, d2, d3
	vshr.u64	d1, d0, #30
	vshr.u64	d3, d2, #30
	vorr	d0, d0, d1
	vorr	d2, d2, d3
	vzip.32	d0, d2	
	vst1.64	{d0}, [rdi]


	                                                                                                                           

	                                                                                            
	add		ecx, scratch, #QPosArray_offset			                
	sub		eax, next_qp, ecx 						                                                               
	add		eax, eax, #7							                      
	lsr		eax, eax, #3							                                                  
	subs	byte_count, byte_count, eax, lsl #2		                                 
	blt		L_budgetExhausted						                                                          
	add		ecx, ecx, eax, lsl #3					                                                    
	cmp		ecx, next_qp							                          
	bls		L16										                                                                
	sub		eax, ecx, next_qp
	mov		edx, #0
	tst		eax, #4
	beq		1f
	str		edx, [next_qp], #4
1:	tst		eax, #2
	beq		1f
	strh	edx, [next_qp], #2
1:	tst		eax, #1
	beq		1f
	strb	edx, [next_qp], #1
1:
L16:
	add		edx, scratch, #QPosArray_offset			                
	mov		rdi, next_full_patt						                 
	cmp		ecx, edx								                                
	ldr		eax, [dest_buf] 
	bls		L20										                                                        

	                                               
L_pack_4bits:
	vld1.64	{d0}, [edx,:64]!							                          
	vshr.u64	d1, d0, #28							                     
	vorr	d0, d0, d1								                                   
	cmp		ecx, edx								                         
	vstr	s0, [rdi]
	add		rdi, rdi, #4
	bhi		L_pack_4bits							                                                

	                                                                                                                                          
	                                                  
	sub		eax, rdi, dest_buf						                          
	lsr		eax, eax, #2							                                   
L20:
	str		eax, [dest_buf,#4]						                                        

	add		ecx, scratch, #LowBitsArray_offset		                   
    sub		edx, next_low_bits, ecx					                                              
	lsr		edx, edx, #1							                                      
	subs	edx, edx, #3							                                         
	blt		1f										                                                      
0:
	subs	byte_count, byte_count, #4				                  
	ble		L_budgetExhausted						                                                          
	ldr		r4,[ecx, #2]							              
	ldrh	r0,[ecx], #6							     
	uxth	r5, r4, ror #16							      
	uxth	r4, r4									     
	orr		r0, r0, r4, lsl #10						        
	subs	edx, edx, #3							                         
	orr		r0, r0, r5, lsl #20						           
	str		r0, [rdi], #4							                                     
	bge		0b										                                                

1: 	adds	edx, edx, #3							                                          
	beq		3f										                                                                
	subs	byte_count, byte_count, #4				                  
	ble		L_budgetExhausted						                                                          
	ldrh	eax,[ecx]								     
	subs	edx, edx, #1							                        
	beq		2f										  
	ldrh	edx, [ecx, #2]							     
	orr		eax, eax, edx, lsl #10					                

2:	str		eax, [rdi], #4							                                

3:	sub		eax, rdi, dest_buf						                          
	lsr		eax, eax, #2							                                            
	str		eax, [dest_buf, #8]						                                               
	lsl		r0, eax, #2								                                            

L_done:
	                               

	add		sp, sp, #(64+24)			                                                 
#if KERNEL
	vld1.64 {q0,q1}, [sp]!
#endif
    pop     {r4-r6,r8-r11}
    pop     {r7,pc}

	.align	4
L_budgetExhausted:
	mov		r0, #-1
	b		L_done


	.align 4,0x90
L_RECORD_EXACT:
  
                                                                               
                                                
  
	sub		edx, dict_location, dictionary		                             
	mov		eax, #3
	lsr		edx, edx, #2						                              
	strb	eax, [next_tag], #1					                            
	strb	edx, [next_qp], #1					                                 
	subs	remaining, remaining, #1			                        
	bgt		L_scan_loop							                                           
	b		CHECKPOINT                                                    

	.align 4,0x90
L_RECORD_PARTIAL:
  
                                                                                               
                                                            
  
	sub		eax, dict_location, dictionary		                             
	str		edx, [dict_location]				                               
	lsr		eax, eax, #2						                        
	lsl		edx, edx, #22
	strb	eax, [next_qp], #1					                    
	mov		eax, #1
	lsr		edx, edx, #22						                
	strb	eax, [next_tag], #1					                                      
	strh	edx, [next_low_bits], #2			                       
	subs	remaining, remaining, #1			                        
	bgt		L_scan_loop							                                           

CHECKPOINT:
    ldr     eax, mode                                                   
    cmp     eax, #EARLYCHECK
    beq     L_check_compression_ratio                                       

L_check_zero_page:

    ldr     eax, start_next_full_patt                                                            
    cmp     eax, next_full_patt
    bne     L_check_single_value_page

	add		eax, scratch, #QPosArray_offset		                            
    cmp     eax, next_qp                                                                                   

    moveq   r0, #SV_RETURN                                                   
	beq     L_done

L_check_single_value_page:

    ldr     eax, start_next_full_patt                                             
    sub     eax, next_full_patt, eax
    lsr     eax, eax, #2

	add		R11, scratch, #QPosArray_offset		                            
    sub     R11, next_qp, R11                                                                     

    ldr     R13, start_next_low_bits
    sub     R13, next_low_bits, R13                                                     
    lsrs    R13, R13, #1

                                                         
                                                                               
                                                                    
      
    bne     1f                                                                       

    mov     edx, #1023
    cmp     R11, edx                                                                      
    bne     1f

    cmp     eax, #1                                                                           
    bne     1f

    ldrb    edx, [tempTagsArray]                                                
    cmp     edx, #2                                                                    
    beq     L_is_single_value_page

1:
    cmp     R13, #1                                                                
    bne     L_check_mostly_zero

    mov     edx, #1024
    cmp     R11, edx                                                                      
    bne     L_check_mostly_zero

    ldrb    edx, [tempTagsArray]                                                
    cmp     edx, #1                                                                       
    bne     L_is_single_value_page

L_is_single_value_page:
    
    moveq   r0, #SV_RETURN                                                   
	beq     L_done

L_check_mostly_zero:
                                                                                                      
    add     eax, eax, R11                                                                  
    mov     edx, #6
    mov     R11, #4
    mla     R11, eax, edx, R11                                                                                                 

    ldr     eax, start_next_low_bits
    sub     eax, next_low_bits, eax                                                           
    mov     edx, #1365
    mul     eax, eax, edx

    ldr     edx, start_next_full_patt
    sub     edx, next_full_patt, edx                                                              
    add     eax, edx, eax, lsr #11                                                                                                                  

	add		edx, scratch, #QPosArray_offset		                            
    sub     edx, next_qp, edx
    add     eax, eax, edx, lsr #1                                                            
    mov     edx, #(12+256)
    add     eax, eax, edx                                                                         

    cmp     eax, R11                                                                               
    blt     L_done_search

    ldr     edx, byte_budget    
    cmp     R11, edx                                                                                       
    bgt     L_budgetExhausted

L_sparse_packer:

    mov     edx, #MZV_MAGIC
    str     edx, [dest_buf], #4                                                              

    ldr     R13, start_next_input_word                                                    
    mov     edx, #0
    mov     ecx, #4096
	
1:
    ldm     R13!, {r2, r3, r5, r6, r7, r8, r9, r10}

    teq     r2, #0
    teqeq   r3, #0
    teqeq   r5, #0
    teqeq   r6, #0
    teqeq   r7, #0
    teqeq   r8, #0
    teqeq   r9, #0
    teqeq   r10, #0

    bne     2f
    subs    ecx, ecx, #32
    add     edx, edx, #32                                                                   
    bne     1b
    mov     r0, R11                                                                               
    b       L_done

2:
    teq     r2, #0
    strne   r2, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4

    teq     r3, #0
    strne   r3, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r5, #0
    strne   r5, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r6, #0
    strne   r6, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r7, #0
    strne   r7, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r8, #0
    strne   r8, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r9, #0
    strne   r9, [dest_buf], #4                                                                   
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    teq     r10, #0
    strne   r10, [dest_buf], #4                                                                  
    strhne  edx, [dest_buf], #2                                               
    add     edx, edx, 4
    
    subs    ecx, ecx, #32
    bne     1b
    mov     r0, R11                                                                               
    b       L_done

L_check_compression_ratio:

    mov     eax, #NORMAL
    str     eax, mode
    mov     remaining, #(1024 - CHKPT_WORDS)                                               
    cmp     remaining, #0
    beq     CHECKPOINT                                                                               

    ldr     eax, start_next_low_bits
    sub     eax, next_low_bits, eax                                                           
    mov     edx, #1365
    mul     eax, eax, edx

    ldr     edx, start_next_full_patt
    sub     edx, next_full_patt, edx                                                              
    add     eax, edx, eax, lsr #11                                                                                                                  

	add		edx, scratch, #QPosArray_offset		                            
    sub     edx, next_qp, edx
    add     eax, eax, edx, lsr #1                                                            
    mov     edx, #(CHKPT_SHRUNK_BYTES - CHKPT_TAG_BYTES)
    subs    eax, eax, edx                                                                                   
    bgt     L_budgetExhausted                                                                   
    b       L_scan_loop                                               


#if defined(KERNEL) && !SLIDABLE
    .align  2
L_table:
    .long   _hashLookupTable_new
#else
	.align	2
L_table:
	.long   L_Tab$non_lazy_ptr-(L_table0+8)

	 .section    __DATA,__nl_symbol_ptr,non_lazy_symbol_pointers
    .align  2
L_Tab$non_lazy_ptr:
    .indirect_symbol    _hashLookupTable_new
    .long   0
#endif

