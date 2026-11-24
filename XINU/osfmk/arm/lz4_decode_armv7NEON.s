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
#if LZ4_ENABLE_ASSEMBLY_DECODE_ARMV7

  

                         
                                                                                        
                                                                                                             
                                                                                      
                                                                                      
                                                                                     
 
                                                                                                                 
                                 
 
                                    
                                                                                                                                
 
  

.globl _lz4_decode_asm

#define dst                r0          
#define dst_begin          r1          
#define dst_end            r2          
#define src                r3          

#define src_end            r4          

#define n_matches          r5
#define n_literals         r10
#define copy_src           r11                               
#define copy_dst           r8                                    

#define aux1               r9

#define match_distance    r6

#define match_permtable   r12
#define match_disttable   lr

#define dst_good          [sp, #0]
#define src_good          [sp, #4]

.macro establish_frame
    ldr     ip, [sp, #0]                       
    push    {r4-r7, lr}                           
    add     r7, sp, #12                                  
    push    {r8-r11}
    mov     src_end, ip   
    push    {r0, r3}                               
    sub     sp, sp, #4+16                                                                     
.endm

.macro clear_frame_and_return
    add     sp, sp, #12+16                       
    pop     {r8-r11}
    pop     {r4-r7,pc}
.endm

                                         
                             
.macro copy_1x16
    vld1.8  {q0}, [$0]
    vst1.8  {q0}, [$1]
.endm

                                                       
                                                                 
.macro copy_1x16_and_increment
    vld1.8  {q0}, [$0]!
    vst1.8  {q0}, [$1]!
.endm

                                                       
                                                                         
.macro copy_2x16_and_increment
    vld1.8  {q0}, [$0]!
    vst1.8  {q0}, [$1]!
    vld1.8  {q0}, [$0]!
    vst1.8  {q0}, [$1]!
.endm

                                                       
                                                                    
.macro copy_1x32_and_increment
    vld1.8  {q0,q1}, [$0]!
    vst1.8  {q0,q1}, [$1]!
.endm

                                               
.macro check_src_end
    cmp     src,src_end
    bhs     L_done                                                                           
.endm

                                               
.macro check_dst_end
    cmp     dst,dst_end
    bhs     L_done                                                                           
.endm

.text
.syntax unified
.thumb
.thumb_func _lz4_decode_asm
.p2align 1
_lz4_decode_asm:
    establish_frame
    ldr     src,[src]                                          
    ldr     dst,[dst]                                          

    adr     match_permtable,L_match_permtable
    adr     match_disttable,L_match_disttable

L_decode_command:
                                                     
    str     dst, dst_good
    str     src, src_good

                   
    check_src_end
    check_dst_end

                            
    ldrb    aux1,[src],#1                                                  
    lsr     n_literals,aux1,#4                                                    
    and     n_matches,aux1,#0xf                                                  
    add     n_matches,n_matches,#4                                     

                                                                                                    
    cmp     n_literals,#14
    bls     L_copy_short_literal                                                
                                      

                                                                              
L_decode_long_literal:
    check_src_end                                                                                                    
    ldrb    aux1,[src],#1
    add     n_literals,n_literals,aux1
    cmp     aux1,#255
    beq     L_decode_long_literal                                  
                                    

                                      
L_copy_long_literal:
    mov     copy_src,src                                            
    mov     copy_dst,dst                                                 
    add     src,src,n_literals
    add     dst,dst,n_literals
    check_src_end                                                                                       
    check_dst_end

                   
    copy_1x32_and_increment copy_src,copy_dst
L_copy_long_literal_loop:
    copy_1x32_and_increment copy_src,copy_dst
    cmp     dst,copy_dst
    bhi     L_copy_long_literal_loop                                                                                            
    b       L_expand_match

                                                     
L_copy_short_literal:
    copy_1x16 src,dst
    add     src,src,n_literals
    add     dst,dst,n_literals
                               

L_expand_match:

                            
    ldrh    match_distance,[src],#2                             
    cbz     match_distance,L_fail                                        
    sub     copy_src,dst,match_distance                                           
    cmp     copy_src,dst_begin
    blo     L_fail                                                         
    mov     copy_dst,dst                                                               
    add     dst,dst,n_matches                                                                                       

                                         
    cmp     n_matches,#19
    beq     L_decode_long_match                                                                 
    cmp     n_matches,#16
    bhi     L_long_match                                                        
                                                   

                                  
L_short_match:
    cmp     match_distance,#15
    bls     L_copy_short_match_small_distance

                                                                       
    copy_1x16 copy_src,copy_dst
    b       L_decode_command

L_fail:
mov     aux1,#-1                                 
b       L_exit

L_done:
mov     aux1,#0                                
                     

L_exit:

ldr     dst,[sp, #20]                                                           
ldr     src,[sp, #24]                                                           
ldr     ip, src_good
ldr     lr, dst_good
str     ip,[src]                                          
str     lr,[dst]                                          

mov     r0,aux1                                               
clear_frame_and_return

                                                        
                                                                           
L_copy_short_match_small_distance:
    vld1.8  {q0},[copy_src]
    add     aux1,match_permtable,match_distance,lsl #5                    
    vld1.8  {q1},[aux1]                                                                    
    vtbl.8  d4, {q0}, d2                                                
    vtbl.8  d5, {q0}, d3                                                
    vst1.8  {q2},[copy_dst]
    b       L_decode_command

                                                                                              
L_decode_long_match:
    check_src_end                                                                                                    
    ldrb    aux1,[src],#1
    add     dst,dst,aux1
    cmp     aux1,#255
    beq     L_decode_long_match                               
    check_dst_end                                                                                                    
                             

                     
L_long_match:
    cmp     match_distance,#31
    bhi     L_copy_long_match_32
    cmp     match_distance,#15
    bhi     L_copy_long_match_16

                                                        
                                                                           
L_copy_long_match_small_distance:
    vld1.8  {q1}, [copy_src]                                     
    add     aux1,match_permtable,match_distance,lsl #5                    
    vld1.8  {q2-q3}, [aux1]                                              

    vtbl.8  d4, {q1}, d4                                                
    vtbl.8  d5, {q1}, d5                                                
    vtbl.8  d6, {q1}, d6                                                
    vtbl.8  d7, {q1}, d7                                                

    ldrb    aux1,[match_disttable,match_distance]                                 
            
    vst1.8  {q2-q3},[copy_dst]
    add     copy_dst,copy_dst,aux1
L_copy_long_match_small_distance_loop:
           
    vst1.8  {q2-q3},[copy_dst]
    add     copy_dst,copy_dst,aux1
    vst1.8  {q2-q3},[copy_dst]
    add     copy_dst,copy_dst,aux1
    cmp     dst,copy_dst
    bhi     L_copy_long_match_small_distance_loop
    b       L_decode_command

                                                        
L_copy_long_match_32:
                   
    copy_1x16_and_increment copy_src,copy_dst
L_copy_long_match_32_loop:
    copy_1x32_and_increment copy_src,copy_dst
    cmp     dst,copy_dst
    bhi     L_copy_long_match_32_loop
    b       L_decode_command

                                                        
L_copy_long_match_16:
                   
    copy_1x16_and_increment copy_src,copy_dst
L_copy_long_match_16_loop:
    copy_2x16_and_increment copy_src,copy_dst
    cmp     dst,copy_dst
    bhi     L_copy_long_match_16_loop
    b       L_decode_command


                                                                                              
                                  
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
