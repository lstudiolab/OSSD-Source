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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#include <vm/lz4_assembly_select.h>
#include <arm64/asm.h>
#if LZ4_ENABLE_ASSEMBLY_DECODE_ARM64

  

                         
                                                                                        
                                                                                                             
                                                                                      
                                                                                      
                                                                                     
 
                                                                                                                 
                                 
 
                                    
                                                                                                                                
 
  

.globl _lz4_decode_asm

#define dst                x0          
#define dst_begin          x1          
#define dst_end            x2          
#define src                x3          
#define src_end            x4          

#define w_n_matches        w5                                
#define n_matches          x5
#define n_literals         x6
#define copy_src           x7                               
#define copy_dst           x8                                    

#define w_aux1             w9                           
#define aux1               x9
#define aux2              x10

#define w_match_distance  w11                                     
#define match_distance    x11

#define match_permtable   x12
#define match_disttable   x13

#define dst_good          x19
#define src_good          x20

.macro establish_frame
    ARM64_STACK_PROLOG
    stp     fp, lr,    [sp, #-16]!
    mov     fp, sp
.endm

.macro clear_frame_and_return
    ldp     fp, lr,    [sp], #16
    ARM64_STACK_EPILOG
.endm

                                         
                             
.macro copy_1x16
    ldr     q0,[$0]
    str     q0,[$1]
.endm

                                                       
                                                                 
.macro copy_1x16_and_increment
    ldr     q0,[$0],#16
    str     q0,[$1],#16
.endm

                                                       
                                                                         
.macro copy_2x16_and_increment
    ldr     q0,[$0],#16
    str     q0,[$1],#16
    ldr     q0,[$0],#16
    str     q0,[$1],#16
.endm

                                                       
                                                                    
.macro copy_1x32_and_increment
    ldp     q0,q1,[$0],#32
    stp     q0,q1,[$1],#32
.endm

                                               
.macro check_src_end
    cmp     src,src_end
    b.hs    L_done                                                                           
.endm

                                               
.macro check_dst_end
    cmp     dst,dst_end
    b.hs    L_done                                                                           
.endm

.text
.p2align 4
_lz4_decode_asm:
    establish_frame
    stp     x19,x20,[sp,#-16]!                                         
    stp     src,dst,[sp,#-16]!                                                
    ldr     src,[src]                                          
    ldr     dst,[dst]                                          
    adr     match_permtable,L_match_permtable
    adr     match_disttable,L_match_disttable

L_decode_command:
                                                     
    mov     dst_good,dst
    mov     src_good,src

                   
    check_src_end
    check_dst_end

                            
    ldrb    w_aux1,[src],#1                                                
    lsr     n_literals,aux1,#4                                                    
    and     n_matches,aux1,#0xf                                                  
    add     n_matches,n_matches,#4                                     

                                                                                                    
    cmp     n_literals,#14
    b.ls    L_copy_short_literal                                                
                                      

                                                                              
L_decode_long_literal:
    check_src_end                                                                                                    
    ldrb    w_aux1,[src],#1
    add     n_literals,n_literals,aux1
    cmp     aux1,#255
    b.eq    L_decode_long_literal                                  
                                    

                                      
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
    b.hi    L_copy_long_literal_loop                                                                                            
    b       L_expand_match

                                                     
L_copy_short_literal:
    copy_1x16 src,dst
    add     src,src,n_literals
    add     dst,dst,n_literals
                               

L_expand_match:

                            
    ldrh    w_match_distance,[src],#2                           
    cbz     match_distance,L_fail                                        
    sub     copy_src,dst,match_distance                                           
    cmp     copy_src,dst_begin
    b.lo    L_fail                                                         
    mov     copy_dst,dst                                                               
    add     dst,dst,n_matches                                                                                       

                                         
    cmp     n_matches,#19
    b.eq    L_decode_long_match                                                                 
    cmp     n_matches,#16
    b.hi    L_long_match                                                        
                                                   

                                  
L_short_match:
    cmp     match_distance,#15
    b.ls    L_copy_short_match_small_distance

                                                                       
    copy_1x16 copy_src,copy_dst
    b       L_decode_command

                                                        
                                                                           
L_copy_short_match_small_distance:
    ldr     q0,[copy_src]
    add     aux1,match_permtable,match_distance,lsl #5                    
    ldr     q1,[aux1]                                                                      
    tbl     v0.16b,{v0.16b},v1.16b                                      
    str     q0,[copy_dst]
    b       L_decode_command

                                                                                              
L_decode_long_match:
    check_src_end                                                                                                    
    ldrb    w_aux1,[src],#1
    add     dst,dst,aux1
    cmp     aux1,#255
    b.eq    L_decode_long_match                               
    check_dst_end                                                                                                    
                             

                     
L_long_match:
    cmp     match_distance,#31
    b.hi    L_copy_long_match_32
    cmp     match_distance,#15
    b.hi    L_copy_long_match_16

                                                        
                                                                           
L_copy_long_match_small_distance:
    ldr     q1,[copy_src]                                        
    add     aux1,match_permtable,match_distance,lsl #5                    
    ldp     q2,q3,[aux1]                                                 
    tbl     v0.16b,{v1.16b},v2.16b                                            
    tbl     v1.16b,{v1.16b},v3.16b                                             
    ldrb    w_aux1,[match_disttable,match_distance]                                 
            
    stp     q0,q1,[copy_dst]
    add     copy_dst,copy_dst,aux1
L_copy_long_match_small_distance_loop:
           
    stp     q0,q1,[copy_dst]
    add     copy_dst,copy_dst,aux1
    stp     q0,q1,[copy_dst]
    add     copy_dst,copy_dst,aux1
    cmp     dst,copy_dst
    b.hi    L_copy_long_match_small_distance_loop
    b       L_decode_command

                                                        
L_copy_long_match_32:
                   
    copy_1x16_and_increment copy_src,copy_dst
L_copy_long_match_32_loop:
    copy_1x32_and_increment copy_src,copy_dst
    cmp     dst,copy_dst
    b.hi    L_copy_long_match_32_loop
    b       L_decode_command

                                                        
L_copy_long_match_16:
                   
    copy_1x16_and_increment copy_src,copy_dst
L_copy_long_match_16_loop:
    copy_2x16_and_increment copy_src,copy_dst
    cmp     dst,copy_dst
    b.hi    L_copy_long_match_16_loop
    b       L_decode_command

L_fail:
    mov     aux1,#-1                                 
    b       L_exit

L_done:
    mov     aux1,#0                                
                         

L_exit:
    ldp     src,dst,[sp],#16                                                        
    str     src_good,[src]                                          
    str     dst_good,[dst]                                          
    mov     x0,aux1                                               
    ldp     x19,x20,[sp],#16                            
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
