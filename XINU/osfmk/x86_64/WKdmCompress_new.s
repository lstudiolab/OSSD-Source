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
	.align 4,0x90

#define SV_RETURN           $0                                                               
#define MZV_MAGIC           $17185                                                                   
#define CHKPT_BYTES         416                                                                                                                 
#define CHKPT_TAG_BYTES     (CHKPT_BYTES/16)                                                    
#define CHKPT_SHRUNK_BYTES  426                                                                                                      
                                                                                                                      

#if CHKPT_BYTES > 4096
    #error CHKPT_BYTES must be <= 4096
#endif
#if CHKPT_BYTES < 4
    #error CHKPT_BYTES must be >= 4
#endif

.globl _WKdm_compress_new
_WKdm_compress_new:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$(48+64), %rsp

	#define	tempTagsArray       64(%rsp)
	#define	tempLowBitsArray	72(%rsp)

    #define start_next_full_patt  80(%rsp)
    #define start_next_input_word 88(%rsp)
    #define byte_budget           96(%rsp)
    #define start_next_qp         tempQPosArray
    #define start_next_low_bits   tempLowBitsArray 
    
	#define	next_tag			%r8
	#define	next_input_word		%rdi
	#define	end_of_input		%r13
	#define	next_full_patt		%rbx
	#define	dict_location		%rcx
	#define	next_qp				%r10
    #define checkpoint          %r11
	#define	dictionary			%rsp
	#define	dest_buf			%r12
	#define	hashTable			%r14
	#define tempQPosArray		%r15
	#define	next_low_bits		%rsi
	#define	byte_count			%r9d

	movq	%rsi, %r12						           

	movq	%rdx, tempTagsArray 			                    
	movq	%rdx, next_tag					                                                               

	leaq	1024(%rdx), tempQPosArray		                    
	movq	tempQPosArray, next_qp			          

    leaq    CHKPT_BYTES(%rdi), checkpoint                                        
	leaq	4096(%rdi), end_of_input		                                           
	leaq	268(%rsi), %rbx					                                                           

	movl	%ecx, byte_count
	subl	$(12+256), byte_count			                
	jle		L_budgetExhausted

                                                                                                          
                                                                                                      
	                      
	movl	$0, 0(dictionary)
	movl	$0, 4(dictionary)
	movl	$0, 8(dictionary)
	movl	$0, 12(dictionary)
	movl	$0, 16(dictionary)
	movl	$0, 20(dictionary)
	movl	$0, 24(dictionary)
	movl	$0, 28(dictionary)
	movl	$0, 32(dictionary)
	movl	$0, 36(dictionary)
	movl	$0, 40(dictionary)
	movl	$0, 44(dictionary)
	movl	$0, 48(dictionary)
	movl	$0, 52(dictionary)
	movl	$0, 56(dictionary)
	movl	$0, 60(dictionary)

	leaq	2048(%rdx), %rax				                       
	movq	%rax, tempLowBitsArray			                           
	movq	%rax, next_low_bits				                 

	leaq	_hashLookupTable_new(%rip), hashTable	                     

    movq    next_full_patt, start_next_full_patt
    movq    next_input_word, start_next_input_word
    movl    %ecx, byte_budget                                          


	jmp		L_scan_loop

	.align 4,0x90
L_RECORD_ZERO:
	movb	$0, -1(next_tag)						                    
	addq	$4, next_input_word 					                     
	cmpq	next_input_word, checkpoint                                
	je		CHECKPOINT

L_scan_loop:
	movl	(next_input_word), %edx
	incq	next_tag								             
	testl	%edx, %edx
	je		L_RECORD_ZERO							                                 
	movl	%edx, %eax								                       
	shrl	$10, %eax								                                           
	movzbl	%al, %eax								                                
	movsbq	(hashTable,%rax),%rax					                                       
	leaq	(dictionary, %rax), dict_location		                                                                
	movl	(dict_location), %eax					                              
	addq	$4, next_input_word						                    
	cmpl	%eax, %edx								                          
	je		L_RECORD_EXACT							                             
	xorl	%edx, %eax
	shrl	$10, %eax								                       
	je		L_RECORD_PARTIAL						                               

L_RECORD_MISS:
	movl	%edx, (next_full_patt)					                                
	addq	$4, next_full_patt						                    
	movl	%edx, (dict_location)					                              
	movb	$2, -1(next_tag)						                         
	subl	$4, byte_count							                             
	jle		L_budgetExhausted
	cmpq	next_input_word, checkpoint                                
	jne     L_scan_loop
	jmp	    CHECKPOINT	

L_done_search:

	                                                
	movq	next_full_patt, %rax					                 
	subq	dest_buf, %rax							                                    
	sarq	$2, %rax								                    
	movl	%eax, %r13d								                                     
	movl	%eax, 0(dest_buf)						                                          
	decq	next_tag
	cmpq	next_tag, tempTagsArray					                                
	jae		L13										                                                         

	                                                                                                      

	movq	dest_buf, %rdi							           
	movq	tempTagsArray, %rcx						                    

	.align 4,0x90
L_pack_2bits:
	movq	8(%rcx), %rax							     
	addq	$16, %rcx								                       
	shlq	$4, %rax
	addq	$4, %rdi								                 
	orq		-16(%rcx), %rax							     
	movq	%rax, %rdx
	shrq	$30, %rax
	orl		%edx, %eax
	cmpq	%rcx, next_tag							                           
	movl	%eax, 8(%rdi)							                                             
	ja		L_pack_2bits							                                               

	                                                                       

L13:
	mov		next_qp, %rax							          
	sub		tempQPosArray, %rax						                                                         
	addl	$7, %eax								                      
	shrl	$3, %eax								                                                  

	shll	$2, %eax								                  
	subl	%eax, byte_count						   
	jl		L_budgetExhausted
	shrl	$1, %eax 								                                           

	leaq	(tempQPosArray,%rax,4), %rcx			                                                  
	cmpq	%rcx, next_qp							                          
	jae		L16										                                                                
	movq	%rcx, %rax
	subq	next_qp, %rax
	subl	$4, %eax
	jl		1f
	.align 4,0x90
0:	movl	$0, (next_qp)	
	addq	$4, next_qp
	subl	$4, %eax
	jge		0b
1:	testl	$2, %eax
	je		1f
	movw	$0, (next_qp)	
	addq	$2, next_qp
1:	testl	$1, %eax
	je		1f
	movb	$0, (next_qp)	
	addq	$1, next_qp
1:
L16:
	movq	next_full_patt, %rdi					                 
	cmpq	tempQPosArray, %rcx						                                
	jbe		L20										                                                        
	movq	tempQPosArray, %rdx						                

	                                           

	.align 4,0x90
L_pack_4bits:
	movl	4(%rdx), %eax							              
	addq	$8, %rdx								                 
	sall	$4, %eax								                     
	addq	$4, %rdi								               
	orl		-8(%rdx), %eax							                                          
	cmpq	%rdx, %rcx								                         
	movl	%eax, -4(%rdi)							                       
	ja		L_pack_4bits							                                                

	                                                  
	movq	%rdi, %rax								               
	subq	dest_buf, %rax							                          
	movq	%rax, %r13								                          
	shrq	$2, %r13								                                   
L20:
	movl	%r13d, 4(dest_buf)						                                        

	movq	tempLowBitsArray, %rcx					                   
	movq	next_low_bits, %rbx						                
	subq	%rcx, %rbx								                                              
	sarq	$1, %rbx								                                      

	#define	size	%ebx

	subl	$3, size								                                         
	jl		1f										                                                      

	.align	4,0x90
0:
	movzwl	4(%rcx), %eax							      
	addq	$6, %rcx								                        
	sall	$10, %eax								           
	or		-4(%rcx), %ax							     
	addq	$4, %rdi								             
	sall	$10, %eax								           
	or		-6(%rcx), %ax							                             
	subl	$4, byte_count							                             
	jle		L_budgetExhausted
	subl	$3, size								                         
	movl	%eax, -4(%rdi)							                                     
	jge		0b										                                                

1: 	addl	$3, size								                                          
	je		3f										                                                                
	movzwl	(%rcx), %eax							     
	subl	$1, size								                        
	je		2f										  
	movzwl	2(%rcx), %edx							     
	sall	$10, %edx								           
	orl		%edx, %eax								                
2:
	subl	$4, byte_count							                             
	jle		L_budgetExhausted
	movl	%eax, (%rdi)							                                
	addq	$4, %rdi								             

3:	movq	%rdi, %rax								               
	subq	dest_buf, %rax							                          
	shrq	$2, %rax								                                            
	movl	%eax, 8(dest_buf)						                                               
	shlq	$2, %rax								                                            

L_done:
	                               
	addq	$(48+64), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	leave
	ret

    .align  4
L_budgetExhausted:
	mov		$-1, %rax
	jmp		L_done
	

	.align 4,0x90
L_RECORD_EXACT:
	subq	dictionary, %rcx					                             
	sarq	$2, %rcx							                              
	movb	$3, -1(next_tag)					                          
	movb	%cl, (next_qp)						                                 
	incq	next_qp								            
	cmpq	next_input_word, checkpoint                            
	jne     L_scan_loop
	jmp	    CHECKPOINT	

	.align 4,0x90
L_RECORD_PARTIAL:
	movq	%rcx, %rax							                
	movb	$1, -1(next_tag)					                                    
	subq	dictionary, %rax					                             
	movl	%edx, (%rcx)						                               
	sarq	$2, %rax							                        
	movb	%al, (next_qp)						                  
	andl	$1023, %edx							                
	incq	next_qp								            
	mov		%dx, (next_low_bits)				                     
	addq	$2, next_low_bits					                  
	cmpq	next_input_word, checkpoint                            
	jne     L_scan_loop

CHECKPOINT:

    cmpq	end_of_input, checkpoint                                                        
    jne     L_check_compression_ratio

L_check_zero_page:
                                                                                         
    cmpq    start_next_full_patt, next_full_patt
    jne     L_check_single_value_page

    cmpq    start_next_qp, next_qp                                                                 
    jne     L_check_single_value_page

    mov     SV_RETURN, %rax                                          
    jmp     L_done

L_check_single_value_page:

    movq    next_full_patt, %rax                                          
    subq    start_next_full_patt, %rax
    shrq    $2, %rax
    
    movq    next_qp, %r11                                                                 
    subq    start_next_qp, %r11
    
    movq    next_low_bits, %r13                                                 
    subq    start_next_low_bits, %r13
    shrq    $1, %r13

    movq    tempTagsArray, %r14                                                    

                                                         
                                                                               
                                                                    
      
    cmpq    $0, %r13                                                         
    jne     1f

    cmpq    $1023, %r11                                                           
    jne     1f

    cmpq    $1, %rax                                                                  
    jne     1f 

    cmpb    $2, 0(%r14)                                                        
    je      L_is_single_value_page

1:
    cmpq    $1, %r13                                                       
    jne     L_check_mostly_zero

    cmpq    $1024, %r11                                                           
    jne     L_check_mostly_zero

    cmpb    $1, 0(%r14)                                                           
    jne     L_check_mostly_zero
     
L_is_single_value_page:
    
    mov     SV_RETURN, %rax                                          
    jmp     L_done

L_check_mostly_zero:
                                                                                              
    addq    %r11, %rax                                                             
    movq    $6, %rdx
    mulq    %rdx                                                                              
    addq    $4, %rax                                                       
    movq    %rax, %r11 
                                                                                              
    movq    next_low_bits, %rax
    subq    start_next_low_bits, %rax                                                 
    movq    $1365, %rdx
    mulq    %rdx
    shrq    $11, %rax                                                                             
    movq    next_full_patt, %rdx
    subq    start_next_full_patt, %rdx                                                    
    addq    %rdx, %rax                                                                           
    movq    next_qp, %rdx
    subq    start_next_qp, %rdx
    shrq    $1, %rdx                                                                    
    addq    %rdx, %rax                                                               
    addq    $(12+256), %rax                                                               

    cmpq    %r11, %rax                                                                 
    jb      L_done_search

    cmpl    byte_budget, %r11d                                                                     
    ja      L_budgetExhausted

L_sparse_packer:

    movl    MZV_MAGIC, 0(dest_buf)                                                   
    addq    $4, dest_buf

    movq    $0, %rdx                                                                     
    movq    start_next_input_word, %r8
1:
    movq    0(%r8, %rdx), %rax                                     
	testq	%rax, %rax                                          
    jne     5f
3:
    addq    $8, %rdx                                                               
4:
    cmpq    $4096, %rdx
    jne     1b
    movq    %r11, %rax                                                                    
    jmp     L_done

5:
    testl   %eax, %eax                                               
    je      6f
    movl    %eax, 0(dest_buf)                                                             
    mov     %dx, 4(dest_buf)                                           
    addq    $6, dest_buf
6:
    shrq    $32, %rax                                                              
    testl   %eax, %eax                                               
    je      3b
    addq    $4, %rdx
    movl    %eax, 0(dest_buf)                                                       
    mov     %dx, 4(dest_buf)                                           
    addq    $6, dest_buf
    addq    $4, %rdx
    jmp     4b

L_check_compression_ratio:

    movq    end_of_input, checkpoint                                         

    movq    next_low_bits, %rax
    subq    start_next_low_bits, %rax                                                 
    movq    $1365, %rdx
    mulq    %rdx
    shrq    $11, %rax                                                                             
    
    movq    next_full_patt, %rdx
    subq    start_next_full_patt, %rdx                                                    
    addq    %rdx, %rax                                                                           

    movq    next_qp, %rdx
    subq    start_next_qp, %rdx
    shrq    $1, %rdx
    addq    %rdx, %rax                                                               

    addq    $CHKPT_TAG_BYTES, %rax                                               
    cmpq    $CHKPT_SHRUNK_BYTES, %rax
    ja      L_budgetExhausted                                                    
    jmp     L_scan_loop 

