/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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
#include <vm/lz4_assembly_select.h>
#if LZ4_ENABLE_ASSEMBLY_DECODE_X86_64

  

                         
                                                                                        
                                                                                                             
                                                                                      
                                                                                      
                                                                                     
 
                                                                                                                 
                                 
 
                                    
                                                                                                                                
 
  

#if MSVC_CALLING_CONVENTIONS
#error TODO implement MSVC calling conventions for LZ4 x86_64 assembly
#endif

                                

#define dst		%rdi           
#define dst_begin       %rsi           
#define dst_end		%rdx           
#define src		%rcx           
#define src_end		%r8            

#define n_literals	%r9
#define n_matches	%r10

#define copy_src	%r11                                
#define copy_dst	%r12	                                 
#define match_distance	%r13	                 

#define src_good        %r14
#define dst_good        %r15

.globl _lz4_decode_asm

.macro establish_frame
    push	%rbp
    mov		%rsp,%rbp
    push	%rbx
    push	%r12
    push	%r13
    push	%r14
    push	%r15
.endm

.macro clear_frame_and_return
    pop		%r15
    pop		%r14
    pop		%r13
    pop		%r12
    pop		%rbx
    pop		%rbp
#ifdef __AVX2__
    vzeroupper
#endif
    ret
.endm

                                         
                               
.macro copy_1x16
#ifdef __AVX2__
    vmovdqu	($0),%xmm0
    vmovdqu	%xmm0,($1)
#else
    movdqu	($0),%xmm0
    movdqu	%xmm0,($1)
#endif
.endm

                                                       
                                                                   
.macro copy_1x16_and_increment
#ifdef __AVX2__
    vmovdqu	($0),%xmm0
    vmovdqu	%xmm0,($1)
#else
    movdqu	($0),%xmm0
    movdqu	%xmm0,($1)
#endif
    add		$$16,$0
    add		$$16,$1
.endm

                                                       
                                                                           
.macro copy_2x16_and_increment
#ifdef __AVX2__
    vmovdqu	($0),%xmm0
    vmovdqu	%xmm0,($1)
    vmovdqu	16($0),%xmm0
    vmovdqu	%xmm0,16($1)
#else
    movdqu	($0),%xmm0
    movdqu	%xmm0,($1)
    movdqu	16($0),%xmm0
    movdqu	%xmm0,16($1)
#endif
    add		$$32,$0
    add		$$32,$1
.endm

                                                       
                                                                        
.macro copy_1x32_and_increment
#ifdef __AVX2__
    vmovdqu	($0),%ymm0
    vmovdqu	%ymm0,($1)
#else
    movdqu	($0),%xmm0
    movdqu	16($0),%xmm1
    movdqu	%xmm0,($1)
    movdqu	%xmm1,16($1)
#endif
    add		$$32,$0
    add		$$32,$1
.endm

.macro check_src_end
    cmp		src,src_end
    jbe		L_done				                         
.endm

.macro check_dst_end
    cmp		dst,dst_end
    jbe		L_done				                         
.endm

.text
.p2align 6
_lz4_decode_asm:
    establish_frame
    push        dst                                                            
    mov         (dst),dst                                                    
    push        src                                                                  
    mov         (src),src                                                    

L_decode_command:
                                   
    mov		dst,dst_good
    mov		src,src_good

                   
    check_src_end
    check_dst_end

                     
    movzb       (src),%rax			                             
    add		$1,src
    mov		%rax,n_literals
    shr		$4,n_literals			                      
    mov		%rax,n_matches
    and		$0xf,n_matches
    add		$4,n_matches			                     

                     
    cmp		$15,n_literals
    je		L_decode_long_literal

                                                     
L_copy_short_literal:
    copy_1x16	src,dst
    add		n_literals,src			                    
    add		n_literals,dst			                    
    jmp		L_expand_match				                    

                                                                              
L_decode_long_literal:
    check_src_end				                                                                       
    movzb	(src),%rax
    add		$1,src
    add		%rax,n_literals
    cmp		$255,%rax
    je		L_decode_long_literal

                                      
L_copy_long_literal:
    mov		src,copy_src			                      
    mov		dst,copy_dst			                           
    add		n_literals,src			                               
    add		n_literals,dst
    check_src_end				                                                          
    check_dst_end

                   
    copy_1x32_and_increment copy_src,copy_dst
    copy_1x32_and_increment copy_src,copy_dst
L_copy_long_literal_loop:
    copy_1x32_and_increment copy_src,copy_dst
    cmp		copy_dst,dst
    ja		L_copy_long_literal_loop
                        

L_expand_match:
                                                     
    movzw	(src),match_distance
    add		$2,src
    test        match_distance,match_distance
    jz          L_fail                                                      
    mov		dst,copy_src
    sub		match_distance,copy_src		                               
    cmp		copy_src,dst_begin
    ja		L_fail				                             

                               
    cmp		$19,n_matches
    je		L_decode_long_match		           
                                                     
    cmp		$16,n_matches
    ja		L_long_match			           

                                  
L_copy_short_match:
    cmp		$16,match_distance
    jb		L_copy_short_match_overlap

                                                                          
    copy_1x16	copy_src,dst
    add		n_matches,dst			             
    jmp		L_decode_command		                  

                                                                             
L_copy_short_match_overlap:
    lea		L_match_permtable(%rip),%rax
    shl		$5,match_distance
#ifdef __AVX2__
    vmovdqa	(%rax,match_distance),%xmm2	                                                           
    vmovdqu	(copy_src),%xmm0		                                                                                       
    vpshufb	%xmm2,%xmm0,%xmm0		                                
    vmovdqu	%xmm0,(dst)			                       
#else
    movdqa	(%rax,match_distance),%xmm2	                                                           
    movdqu	(copy_src),%xmm0		                                                                                       
    pshufb	%xmm2,%xmm0			                                
    movdqu	%xmm0,(dst)			                       
#endif
    add		n_matches,dst			             
    jmp		L_decode_command		                  

                                                                                              
L_decode_long_match:
    mov		$255,%rbx
L_decode_long_match_loop:
    check_src_end				                                                                       
    mov		(src),%rax
    add		$1,src
    and		%rbx,%rax
    add		%rax,n_matches
    cmp		%rbx,%rax
    je		L_decode_long_match_loop

                     
L_long_match:
    mov		dst,copy_dst			                                    
    add		n_matches,dst			             
    check_dst_end				                                    

    cmp		$16,match_distance
    jb		L_copy_long_match_overlap	                                        

                                                        
                   
    copy_1x16_and_increment copy_src,copy_dst
L_copy_long_match_loop:
    copy_2x16_and_increment copy_src,copy_dst
    cmp		copy_dst,dst
    ja		L_copy_long_match_loop
    jmp		L_decode_command		                  

                                                                          
L_copy_long_match_overlap:
    lea		L_match_permtable(%rip),%rax
    mov		match_distance,%rbx
    shl		$5,%rbx
#ifdef __AVX2__
    vmovdqu	(copy_src),%xmm0		                                                                                       
    vmovdqa	%xmm0,%xmm1			                                 
    vmovdqa	(%rax,%rbx),%xmm2		                           
    vpshufb	%xmm2,%xmm0,%xmm0		                                
    vmovdqa	16(%rax,%rbx),%xmm2		                            
    vpshufb	%xmm2,%xmm1,%xmm1		                                
    vinserti128	$1,%xmm1,%ymm0,%ymm0		                                            
#else
    movdqu	(copy_src),%xmm0		                                                                                       
    movdqa	%xmm0,%xmm1			                                 
    movdqa	(%rax,%rbx),%xmm2		                           
    pshufb	%xmm2,%xmm0			                                
    movdqa	16(%rax,%rbx),%xmm2		                            
    pshufb	%xmm2,%xmm1			                                
#endif
                                                                                                                           
    lea		L_match_disttable(%rip),%rax
    movzb	(%rax,match_distance),%rax	                                                                                                                      

            
#ifdef __AVX2__
    vmovdqu	%ymm0,(copy_dst)
#else
    movdqu	%xmm0,(copy_dst)
    movdqu	%xmm1,16(copy_dst)
#endif
    add		%rax,copy_dst
L_copy_long_match_overlap_loop:
           
#ifdef __AVX2__
    vmovdqu	%ymm0,(copy_dst)
#else
    movdqu	%xmm0,(copy_dst)
    movdqu	%xmm1,16(copy_dst)
#endif
    add		%rax,copy_dst
    cmp		copy_dst,dst
    ja		L_copy_long_match_overlap
    jmp		L_decode_command		                  

L_fail:
    xor		%rax,%rax
    dec		%rax				     
    jmp		L_exit

L_done:
    xor		%rax,%rax
                       

L_exit:
    pop         src
    mov         src_good,(src)
    pop         dst
    mov         dst_good,(dst)
    clear_frame_and_return

                                                                                              
                                  
.p2align 6
L_match_permtable:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0      
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0      
.byte 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1      
.byte 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0,    1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1      
.byte 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3,    0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3      
.byte 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0,    1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1      
.byte 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3,    4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1      
.byte 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 0, 1,    2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3      
.byte 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,    0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7      
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6,    7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4      
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5,    6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1       
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 0, 1, 2, 3, 4,    5, 6, 7, 8, 9,10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9       
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2, 3,    4, 5, 6, 7, 8, 9,10,11, 0, 1, 2, 3, 4, 5, 6, 7       
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 0, 1, 2,    3, 4, 5, 6, 7, 8, 9,10,11,12, 0, 1, 2, 3, 4, 5       
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0, 1,    2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0, 1, 2, 3       
.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, 0,    1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, 0, 1       

                                                                  
                                                               
.p2align 6
L_match_disttable:
.byte 32,32,32,30             
.byte 16,30,30,28             
.byte 16,27,30,22             
.byte 24,26,28,30             

#endif                                     
