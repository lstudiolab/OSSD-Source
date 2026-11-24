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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <vm/lz4_assembly_select.h>
#include <vm/lz4_constants.h>
#include <arm64/asm.h>

#if LZ4_ENABLE_ASSEMBLY_ENCODE_ARM64

                                          
                                       
                                                
                                                 
                                       
                                                                              
                                                                                

.globl _lz4_encode_2gb

#define dst_ptr             x0
#define dst_size            x1
#define src_ptr             x2
#define src_begin           x3
#define src_size            x4
#define hash_table          x5
#define skip_final_literals x6

.text
.p2align 4
_lz4_encode_2gb:

                      
    ARM64_STACK_PROLOG
    stp     fp, lr,    [sp, #-16]!
    mov     fp, sp

    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!

                         
    adr x7, L_constant
    ldr w28, [x7, #4]                                                                           
    ldr w7, [x7]                                                                
    mov x27, #-1                                                        
    dup.4s v1, w27                                                                                    


                           
                                    
    ldr x9, [x0]                                   
    add x10, x9, x1                                    
    sub x10, x10, #LZ4_GOFAST_SAFETY_MARGIN                            
    cmp x10, x9                                                                  
    b.lt L_done

                           
                                       
    ldr x11, [x2]                                  
    add x12, x11, x4                                   
    sub x12, x12, #LZ4_GOFAST_SAFETY_MARGIN                            
    cmp x12, x11                                                                                     
    b.lt L_trailing_literals


                                                     
                                                                            
L_search_next_available_match:
    mov x13, x11                                                  
    sub x14, x13, x3                                                                  

                                                
                                               

L_hash_match:
    ldr x15, [x13]                                                
    umull x20, w7, w15                                                                 
    lsr w20, w20, #LZ4_COMPRESS_HASH_SHIFT                                               
    add x20, x5, x20, lsl #3                                                        

    ldp w19, w22, [x20]                                                                            
    stp w14, w15, [x20]                                                                                     

    add x26, x14, #1                                         
    lsr x25, x15, #8                                                   
    umull x21, w7, w25                                                                 
    lsr w21, w21, #LZ4_COMPRESS_HASH_SHIFT                                               
    add x21, x5, x21, lsl #3                                                        

    ldp w23, w24, [x21]                                                                            
    stp w26, w25, [x21]                                                                               

    cmp w15, w22
    b.ne L_try_next_match_0                                                                  
    sub w19, w14, w19                                                                    
    cmp w19, #0x10000
    ccmp w19, #0, #0xf, lo
    b.eq L_try_next_match_0                                                 
    b L_found_valid_match

L_try_next_match_0:
    add x13, x13, #1
    add x14, x14, #1

    add x26, x14, #1                                         
    lsr x15, x15, #16                                                  
    umull x20, w7, w15                                                                 
    lsr w20, w20, #LZ4_COMPRESS_HASH_SHIFT                                               
    add x20, x5, x20, lsl #3                                                        

    ldp w21, w22, [x20]                                                                            
    stp w26, w15, [x20]                                                                                     

    cmp w25, w24
    b.ne L_try_next_match_1                                                                  
    sub w19, w14, w23                                                                    
    cmp w19, #0x10000
    ccmp w19, #0, #0xf, lo
    b.eq L_try_next_match_1                                                 
    b L_found_valid_match

L_try_next_match_1:
    add x13, x13, #1
    add x14, x14, #1

    add x26, x14, #1                                         
    lsr x25, x15, #8                                                   
    umull x20, w7, w25                                                                 
    lsr w20, w20, #LZ4_COMPRESS_HASH_SHIFT                                               
    add x20, x5, x20, lsl #3                                                        

    ldp w23, w24, [x20]                                                                            
    stp w26, w25, [x20]                                                                               

    cmp w15, w22
    b.ne L_try_next_match_2                                                                  
    sub w19, w14, w21                                                                    
    cmp w19, #0x10000
    ccmp w19, #0, #0xf, lo
    b.eq L_try_next_match_2                                                 
    b L_found_valid_match

L_try_next_match_2:
    add x13, x13, #1
    add x14, x14, #1

    add x26, x14, #1                                         
    lsr x15, x15, #16                                                  
    umull x20, w7, w15                                                                 
    lsr w20, w20, #LZ4_COMPRESS_HASH_SHIFT                                               
    add x20, x5, x20, lsl #3                                                        

    ldp w21, w22, [x20]                                                                            
    stp w26, w15, [x20]                                                                                     

    cmp w25, w24
    b.ne L_try_next_match_3                                                                  
    sub w19, w14, w23                                                                    
    cmp w19, #0x10000
    ccmp w19, #0, #0xf, lo
    b.eq L_try_next_match_3                                                 
    b L_found_valid_match

L_try_next_match_3:
    add x13, x13, #1
    add x14, x14, #1

    cmp w15, w22
    b.ne L_try_next_matchs                                                                  
    sub w19, w14, w21                                                                    
    cmp w19, #0x10000
    ccmp w19, #0, #0xf, lo
    b.eq L_try_next_matchs                                                 
    b L_found_valid_match

                                                            
                                               
                                                
L_found_valid_match:
    add x20, x13, #4                                                                                         
    sub x21, x20, x19                                                      
L_found_valid_match_expand_forward_loop:
    ldr x22, [x20], #8                                                                                           
    ldr x23, [x21], #8                                                   
    cmp x22, x23
    b.ne L_found_valid_match_expand_forward_partial
    cmp x20, x12                                                                 
    b.lo L_found_valid_match_expand_forward_loop
    b L_found_valid_match_expand_backward
L_found_valid_match_expand_forward_partial:
    sub  x20, x20, #8                                                                                           
    eor  x22, x22, x23                                                   
    rbit x22, x22                                                                                                                  
    clz  x22, x22                                                                                            
    add  x20, x20, x22, lsr #3                                                                           
L_found_valid_match_expand_backward:
    sub  x15, x13, x19                                                       
L_found_valid_match_expand_backward_loop:
    cmp  x13, x11                                                                                   
    ccmp x15, x3, #0xd, gt                                                           
    b.le L_found_valid_match_emit_match
    ldrb w22, [x13, #-1]!                                                                                        
    ldrb w23, [x15, #-1]!                                                
    cmp w22, w23
    b.eq L_found_valid_match_expand_backward_loop
    add x13, x13, #1                                                                

                                          
                                                                                      
                                                                                  
L_found_valid_match_emit_match:
    sub  x21, x20, x13                                                                
    sub  x21, x21, #4                                                                          
    sub  x22, x13, x11                                                                           
    sub  x26, x10, x9                                                              
    sub  x26, x26, x22                                                                
    subs x26, x26, #3                                                                          
    b.lo L_done                                                            

    and x23, x21, #0xf                                          
    add x23, x23, x22, lsl #4                                 
    add x15, x9, #1                                              
    cmp x22, #15                                                                      
    b.lo L_found_valid_match_copy_literals
    orr x23, x23, #0xf0                                                    
    sub x24, x22, #15                                                            
    sub x26, x26, #1                                                                         
    b.lo L_done
    cmp x24, #255                                                                            
    b.lo L_found_valid_match_skip_L_255_tokens
    umull x25, w24, w28                                                                           
    lsr   x25, x25, #39                                                                                                      
    subs  x26, x26, x25                                                                             
    b.lo L_done
    mov x13, #255
    umsubl x24, w25, w13, x24                                                                                                  
L_found_valid_match_L_255_tokens_loop:
    str q1, [x15], #16                                                                                                       
    subs x25, x25, #16                                                                                
    b.hi L_found_valid_match_L_255_tokens_loop
    add x15, x15, x25                                                                         
L_found_valid_match_skip_L_255_tokens:
    strb w24, [x15], #1                                           
L_found_valid_match_copy_literals:
    ldr q0, [x11], #16                                                                                             
    str q0, [x15], #16                                                                                             
    subs x22, x22, #16
    b.gt L_found_valid_match_copy_literals
    add x15, x15, x22                                                                      
    strh w19, [x15], #2                                         
    cmp x21, #15                                                                      
    b.lo L_found_valid_match_finish_writing_match
    orr x23, x23, #0xf                                                     
    sub x24, x21, #15                                                      
    sub x26, x26, #1                                                                         
    b.lo L_done
    cmp x24, #255                                                                            
    b.lo L_found_valid_match_skip_M_255_tokens
    umull x25, w24, w28                                                                      
    lsr   x25, x25, #39                                                                                                 
    subs  x26, x26, x25                                                                             
    b.lo L_done
    mov x13, #255
    umsubl x24, w25, w13, x24                                                                                          
L_found_valid_match_M_255_tokens_loop:
    str q1, [x15], #16                                                                                                       
    subs x25, x25, #16                                                                                
    b.hi L_found_valid_match_M_255_tokens_loop
    add x15, x15, x25                                                                         
L_found_valid_match_skip_M_255_tokens:
    strb w24, [x15], #1                                           
L_found_valid_match_finish_writing_match:
    strb w23, [x9]                                                               
    mov  x9, x15                                                                  
    mov x11, x20                                                                                    
    cmp x11, x12                                                            
    ccmp x9, x10, #9, lt                                                    
    b.ge L_trailing_literals
    b L_search_next_available_match
                                                                            
                                                                            
                                                             
                                                                  
                                                                             
                                                                                         
                     

L_try_next_matchs:
    add x13, x13, #1                                              
    add x14, x14, #1                                                 
    cmp x13, x12                                                                      
    b.lo L_hash_match

L_trailing_literals:
                                        
                                           
                                                                   
                                                                          

    tst x6, x6
    b.ne L_done                                                                                

    add  x12, x12, #LZ4_GOFAST_SAFETY_MARGIN                     
    subs x13, x12, x11                                       
    b.eq L_done                                                                        

    add x10, x10, #LZ4_GOFAST_SAFETY_MARGIN                      
    sub x14, x10, x9                                                         
    sub x14, x14, #1                                                                             
    subs x14, x14, x13                                                                                               
    b.le L_done                                                                                     

    cmp  x13, #15
    b.lt L_trailing_literals_store_less_than_15_literals
    subs x14, x14, #1                                                                     
    b.mi L_done
    mov w15, #0xf0
    strb w15, [x9], #1                                                                                                                                        
    sub  x15, x13, #15
    cmp  x15, #255
    b.lo L_trailing_literals_no_255_tokens
    umull x19, w15, w28                                                                           
    lsr   x19, x19, #39                                                                                                      
    subs  x14, x14, x19
    b.mi L_revert_x9_and_done
    mov x26, #255
    umsubl x15, w26, w19, x15                                                                                                  
L_tariling_literals_write_16_255_tokens:
    str q1, [x9], #16                                                                                                                                             
    subs x19, x19, #16
    b.gt L_tariling_literals_write_16_255_tokens
    add x9, x9, x19                                                                                                     
L_trailing_literals_no_255_tokens:
    strb w15, [x9], #1                                               
    lsr  x14, x13, #4                                                                                      
    tst  x14, x14
    b.eq L_trailing_literals_copy_less_than_16_literals
L_trailing_literals_copy_16_literals:
    ldr q0, [x11], #16                                                  
    str q0, [ x9], #16                                                        
    subs x14, x14, #1
    b.gt L_trailing_literals_copy_16_literals
    cmp x11, x12
    b.lo L_trailing_literals_copy_less_than_16_literals
    b L_done

L_trailing_literals_store_less_than_15_literals:
    lsl x14, x13, #4                                                                           
    strb w14, [x9], #1                                                      
L_trailing_literals_copy_less_than_16_literals:
    ldrb w13, [x11], #1                                             
    strb w13, [ x9], #1                                                 
    cmp x11, x12
    b.lo L_trailing_literals_copy_less_than_16_literals

                                                            
L_done:
    str  x9, [x0]
    str x11, [x2]

    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16

                  
    ldp     fp, lr,    [sp], #16
    ARM64_STACK_EPILOG

L_revert_x9_and_done:
    sub x9, x9, #1
    b L_done

.p2align 2
L_constant:
.long LZ4_COMPRESS_HASH_MULTIPLY
.long 0x80808081

#endif

