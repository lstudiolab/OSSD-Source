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
#if LZ4_ENABLE_ASSEMBLY_ENCODE_ARMV7

                                          
                                       
                                                
                                                 
                                       
                                                                              
                                                                                

.globl _lz4_encode_2gb
.syntax unified

#define dst_ptr r0
#define dst_end r1
#define src_ptr r2
#define src_beg r3
#define src_end r4
#define table   r5
#define mch_ptr r6
#define mch_dis r8
#define mch_len r9
#define mch_ref r10

#define margin  128

.macro establish_frame
  push   {r4-r7, lr}
  add     r7,       sp, #12
  push   {r8-r11}
  ldrd    r4, r5,  [sp, #36]
  push   {r0, r2}
  ldr     dst_ptr, [r0]
  ldr     src_ptr, [r2]
  subs    r1,       r1,  margin                                        
  bls     L_done
  add     dst_end,  dst_ptr, r1                    
  sub     r4,       r4,  margin                                                                                                                     
  add     src_end,  src_ptr, r4                     
  vmov.i8 q1,       #255                                        
.endm

.macro clear_frame_and_return
  pop    {r1, r3}
  str     dst_ptr, [r1]
  str     src_ptr, [r3]
  pop    {r8-r11}
  pop    {r4-r7, pc}
.endm

.p2align 4
_lz4_encode_2gb:
  establish_frame
L_next_match:
                                                                              
                                                                           
                                                                            
                                                                          
                                                                                
  ldr     lr,       L_hash
  sub     mch_ptr,  src_ptr, #1

L_match_candidate_loop:
                                                                               
                                                                               
                    
  cmp     mch_ptr,  src_end
  bhs     L_trailing_literal

                                                                             
                                          
  ldr     r9,      [mch_ptr, #1]!
  sub     r8,       mch_ptr, src_beg
  mul     r12,      r9, lr
  mvn     r10,      #0x7
  and     r12,      r10, r12, lsr #17                               

                                                                               
                                     
  ldrd    r10,r11, [table, r12]
  strd    r8, r9,  [table, r12]

                                                                              
                                                                                
  cmp     r9,       r11
  bne     L_match_candidate_loop

                                                                             
                                                          
  sub     mch_dis,  r8, r10
  add     mch_ref,  src_beg, r10
  cmp     mch_dis,  #0x10000
  bhs     L_match_candidate_loop

                                                                    
    
                                         
                                                                           
                                                                   
                                                                               
                                              
                                                                               
                                                                         
                                                                               
                                                                            
                                                                               
                                         
                                                        
                                                   
                                                      
                                                                 
                                                             
                      
                      
                      
    
                                                                              
                                                               
  eor     mch_len,  mch_len
0:cmp     mch_ref,  src_beg
  cmpne   mch_ptr,  src_ptr
  beq     1f
  ldrb    r11,     [mch_ref, #-1]
  ldrb    r12,     [mch_ptr, #-1]
  cmp     r11,      r12
  bne     1f
  sub     mch_ref,  #1
  sub     mch_ptr,  #1
  add     mch_len,  #1
  b       0b

                                                                         
                                                                            
                                                                              
                                                                           
                                                                            
                          
#define lit_len lr
1:sub     lit_len,  mch_ptr, src_ptr
  add     mch_ptr,  mch_len
  add     mch_ref,  mch_len

                                                                            
                                                                            
                          
0:ldr     r11,     [mch_ptr, #4]!
  ldr     r12,     [mch_ref, #4]!
  eors    r11,      r12
  bne     1f
  add     mch_len,  #4
  cmp     mch_ptr,  src_end
  blo     0b
  b       L_emit_match
                                                                               
                                                                           
1:rev     r11,      r11
  clz     r11,      r11
  add     mch_len,  r11, lsr #3

L_emit_match:
                                    
    
                                         
                                                                           
                                                                   
                                                                               
                                              
                                                                               
                                                                         
                                                                               
                                                                            
                                                                               
                                         
                                                        
                      
                                                      
                                                       
                      
                      
                      
                                                      
                                                    
    
                                                                             
                                                                  
  cmp     lit_len,  #15
  orr     r10,      mch_len, lit_len, lsl #4
  cmplo   mch_len,  #15
  bhs     L_emit_careful
                                                                            
                                                                           
                                                                               
                             
  strb    r10,     [dst_ptr], #1
                   
  vld1.8  q0,      [src_ptr]
  add     src_ptr,  lit_len
  vst1.8  q0,      [dst_ptr]
  add     dst_ptr,  lit_len
                                                          
  add     mch_len,  #4
                                                             
  strh    r8,      [dst_ptr], #2
  add     src_ptr,  mch_len
                                                                             
                    
  cmp     dst_ptr,  dst_end
  blo     L_next_match
                                                                              
                                                                         
             
L_done:
  clear_frame_and_return

                   
L_hash: .long 2654435761
L_magic: .long 0x80808081

L_emit_careful:
                                                                              
                                                                         
    
                                         
                                                                           
                                                                   
                                                                               
                                              
                                                                               
                                                                         
                                                                               
                                                                            
                                                                               
                                         
                                                        
                      
                                                      
                                                                 
                      
                      
                      
                                                      
                                                    
    
                                                                            
                                                                        
                                                      
  ldr     r12,      L_magic
  cmp     mch_len,  #15
  mov     r10,      mch_len
  movhs   r10,      #0x0f
  subs    r6,       lit_len, #15
  bhs     L_large_L
                                                                           
                                                    
  orr     r10,      lit_len, lsl #4
  strb    r10,     [dst_ptr], #1
                   
  vld1.8  q0,      [src_ptr]
  add     src_ptr,  lit_len
  vst1.8  q0,      [dst_ptr]
  add     dst_ptr,  lit_len
                               
  strh    r8,      [dst_ptr], #2
  sub     r6,       mch_len, #15
  b       L_large_M

L_large_L:
                                                                              
                                                                               
                     
  orr     r10,      #0xf0
  strb    r10,     [dst_ptr], #1
                                                                         
                                                                          
                                                                          
                                     
  umull   r10, r11, r6, r12
  mov     r12,      #255
  lsr     r10,      r11, #7                        
  mls     r11,      r10, r12, r6                   
  ldr     r12,      L_magic                                            
                                                                         
                                                     
  add     r10,      dst_ptr, r10
  cmp     r10,      dst_end
  bhs     L_done
                                                                              
0:vst1.8  q1,      [dst_ptr]!
  cmp     dst_ptr,  r10
  blo     0b
                                    
  strb    r11,     [r10], #1
                                                                             
                                         
  add     dst_ptr,  r10, lit_len
  cmp     dst_ptr,  dst_end
  bhs     L_done
                       
0:vld1.8  q0,      [src_ptr]!
  vst1.8  q0,      [r10]!
  subs    r6,       r10, dst_ptr
  blo     0b
                                                                            
                                                      
  strh    r8,      [dst_ptr], #2
  sub     src_ptr,  r6
  subs    r6,       mch_len, #15
  bhs     L_large_M
                                                                              
                                            
  add     mch_len,  #4
  add     src_ptr,  mch_len
  b       L_next_match

L_large_M:
                                                                       
                                               
  umull   r10, r11, r6, r12
  mov     r12,      #255
  lsr     r10,      r11, #7                        
  mls     r11,      r10, r12, r6                   
                                                                         
                                                     
  add     r10,      dst_ptr, r10
  cmp     r10,      dst_end
  bhs     L_done
                                                                              
0:vst1.8  q1,      [dst_ptr]!
  cmp     dst_ptr,  r10
  blo     0b
                                                                             
            
  strb    r11,     [r10]
  add     mch_len,  #4
  add     dst_ptr,  r10, #1
  add     src_ptr,  mch_len
  b       L_next_match

L_trailing_literal:
                                          
  ldr     r5,      [sp, #52]
  tst     r5,       r5
  bne     L_done
                                                                             
                                                                             
  add     src_end,  margin
  sub     lit_len,  src_end, src_ptr
  subs    r6,       lit_len, #15
  bhs     L_trailing_literal_long
  lsl     r10,      lit_len, #4
  strb    r10,     [dst_ptr], #1
  vld1.8  q0,      [src_ptr]
  mov     src_ptr,  src_end
  vst1.8  q0,      [dst_ptr]
  add     dst_ptr,  lit_len
  b       L_done

L_trailing_literal_long:
  ldr     r12,      L_magic
  mov     r10,      #0xf0
  add     dst_end,  margin
  strb    r10,     [dst_ptr], #1
  umull   r10, r11, r6, r12
  mov     r12,      #255
  lsr     r10,      r11, #7                        
  mls     r11,      r10, r12, r6                   
                                                                              
                            
  add     r10,      dst_ptr
  add     r12,      r10, lit_len
  cmp     r12,      dst_end
  bhs     L_done
                                                                           
                                                                                
                                                                         
                    
0:vst1.8  q1,      [dst_ptr]!
  cmp     dst_ptr,  r10
  blo     0b
                                    
  strb    r11,     [r10], #1
  mov     dst_ptr,  r10
                                                                        
                                                                           
                                                       
  subs    lit_len,  #16
  blo     1f
0:vld1.8  q0,      [src_ptr]!
  subs    lit_len,  #16
  vst1.8  q0,      [dst_ptr]!
  bhs     0b
1:adds    lit_len,  #16
  beq     L_done
2:ldrb    r6,      [src_ptr], #1
  subs    lit_len,  #1
  strb    r6,      [dst_ptr], #1
  bne     2b
  b       L_done

#endif
