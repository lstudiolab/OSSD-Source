/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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

#include <machine/asm.h>

                                                                                 
                                                                
  
                                                                           
                                                                            
                                                                        
  
                                                                               
                                                                       
                                                                           
                                                                           
                                                                            
                                                                              
                                                                               
                                                                            
                                                                                
                                                                                
                                                                     
  
                                                                              
                                                                           
                                                                                
                                                                             
                                                                            
          
  
                    
                    
                    
  
                        
                                         
                                                                      
  
                                                                         
  
                                                                                
                           
  
                            
                                                         
                                                          
                             
                     
  
                                                                            
                       
  
                                                                            
                                                                          
                                                                              
                         
  
                                            
            
                                               
                                                                        
                                                               
                  
                                    
                                                                      
                                                                      
                                                  
                                              
     
  
                    
                 
         
                                                
                     
                                        
                
                    
             
                                                           
                                      
                                                
       
      
     
  
            
                          
                                    
                    
                           
                
            
                 
     
  
  
                    
             
                                                                        
                                                               
                  
                                     
                                      
            
                                                                  
                                                                        
            
         
                                     
                         
             
              
                                    
       
                
     
   

                        

.macro BACKOFF lock_addr
	                                  
	stp		x0, x1, [sp, #-16]!
	stp		x2, x9, [sp, #-16]!

	                                        
	mov		x0, \lock_addr
	bl		_backoff			                               

	                    
	ldp		x2, x9, [sp], #16
	ldp		x0, x1, [sp], #16
.endmacro

                             
                                      
                                           
                       
  
                                                                
   
.macro TRYLOCK_ENQUEUE result
	stp		x0, xzr, [sp, #-16]!                                                    

	bl	_pfz_trylock_and_enqueue
	mov		\result, x0

	ldp		x0, xzr, [sp], #16                           
.endmacro

                             
                                           
                       
  
                                                                
   
.macro TRYLOCK_DEQUEUE result
	stp		x0, xzr, [sp, #-16]!                                                    

	bl	_pfz_trylock_and_dequeue
	mov		\result, x0

	ldp		x0, xzr, [sp], #16                           
.endmacro

  
                                                                      
             
  
               
   
.macro PREEMPT_SELF_THEN branch_to_take_on_success
	cbz		x15,  \branch_to_take_on_success                                                 

	mov		x15, xzr				                                        
	bl _preempt_self
	b \branch_to_take_on_success
.endmacro

	.section __TEXT_EXEC,__commpage_text,regular,pure_instructions

	                               
	.align 2
_jump_table:				                                       
	b	_pfz_enqueue
	b	_pfz_dequeue
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666
	brk #666


  
                            
                                                         
                                                          
                             
                     
   

                                                  
                                                                                                           
  
                                       
                                         
                                           
                       
  
                                                                  
  
                                                     
   
	.globl _pfz_trylock_and_enqueue
	.align 2
_pfz_trylock_and_enqueue:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	mov		w10, wzr		                          
	mov		w11, #1			                          

	                       
	casa	w10, w11, [x3]	                                   
	cbz		w10, Ltrylock_enqueue_success

	mov		x0, #-1			         
	b Ltrylock_enqueue_exit

	                                          

Ltrylock_enqueue_success:
	ldr		x10, [x0, #8]	                           
	cbnz	x10, Lnon_empty_queue                 
	str		x1, [x0]		                           
	b		Lset_new_tail

Lnon_empty_queue:
	str		x1, [x10, x2]	                                    

Lset_new_tail:
	str		x1, [x0, #8]		                      

	                                                                   
	stlr	wzr, [x3]

	mov		x0, xzr				               

Ltrylock_enqueue_exit:
	POP_FRAME
	ARM64_STACK_EPILOG

                                                  
                                                                                                
  
                                       
                                         
                       
  
                                                                  
  
                                                                  
   
	.globl _pfz_trylock_and_dequeue
	.align 2
_pfz_trylock_and_dequeue:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	                       
	mov		w10, wzr		                          
	mov		w11, #1			                          

	casa	w10, w11, [x2]	                                   
	cbz		w10, Ltrylock_dequeue_success

	mov		x0, #-1			         
	b Ltrylock_dequeue_exit

	                                          
Ltrylock_dequeue_success:
	ldr		x10, [x0]	                           
	cbz		x10, Lreturn_head                           

	ldr		x11, [x10, x1]	                      
	cbnz	x11, Lupdate_new_head                                                                     

	                 
	str		xzr, [x0, #8]	                                                      

Lupdate_new_head:
	str		xzr, [x10, x1]	                                
	str		x11, [x0]		                    

Lreturn_head:
	mov		x0, x10			                  
	stlr	wzr, [x2]		                                                                   

Ltrylock_dequeue_exit:
	POP_FRAME
	ARM64_STACK_EPILOG


	                           
	.private_extern _commpage_text_preemptible_functions
_commpage_text_preemptible_functions:


  
                                                                           
                             
                                      
                                           
   
	.globl _pfz_enqueue

	.align 2
_pfz_enqueue:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	str		xzr, [x1, x2]	                                           
	mov		x15, xzr		                                                        

	add		x3, x0, #16		                                 
Lenqueue_trylock_loop:

	             
	TRYLOCK_ENQUEUE x9
	PREEMPT_SELF_THEN Lenqueue_determine_success

Lenqueue_determine_success:

	cbz		x9, Lenqueue_success                               

	ldxr	w9, [x3]		                                       
	cbz		w9, Lenqueue_clear_monitor                             

	wfe						                          

	             
	TRYLOCK_ENQUEUE x9
	cbz		x9, Lenqueue_take_delayed_preemption_upon_success                                

	                                           

	BACKOFF x3
	b Lenqueue_trylock_loop

Lenqueue_clear_monitor:
	clrex							                      

	                                                                  
	PREEMPT_SELF_THEN Lenqueue_trylock_loop

Lenqueue_take_delayed_preemption_upon_success:
	PREEMPT_SELF_THEN Lenqueue_success

Lenqueue_success:
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                               
                             
                                           
  
                                                                                
                             
   
	.globl	_pfz_dequeue
	.align 2
_pfz_dequeue:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	mov		x15, xzr		                                                        

	add		x2, x0, #16		                                 
Ldequeue_trylock_loop:

	             
	TRYLOCK_DEQUEUE x9
	PREEMPT_SELF_THEN Ldequeue_determine_success

Ldequeue_determine_success:
	cmp		x9, #-1			                              
	b.ne	Ldequeue_success                    

	ldxr	w9, [x2]		                                       
	cbz		w9, Ldequeue_clear_monitor                             

	wfe						                          

	             
	TRYLOCK_DEQUEUE x9
	cmp		x9, #-1		               
	b.ne	Ldequeue_take_delayed_preemption_upon_success                    

	                                           

	BACKOFF x2
	b	Ldequeue_trylock_loop

Ldequeue_take_delayed_preemption_upon_success:
	                                                                           
	PREEMPT_SELF_THEN Ldequeue_success

Ldequeue_clear_monitor:
	clrex							                      
	                                                                   
	PREEMPT_SELF_THEN Ldequeue_trylock_loop

Ldequeue_success:
	mov		x0, x9		                                                  
	POP_FRAME
	ARM64_STACK_EPILOG


                          
  
                                                                        
   
	.align 2
_preempt_self:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	                                                                           
	                     
	stp		x0, x1, [sp, #-16]!

	                                                                         
	                                                            

	                                          
	mov		x16, #-58	                 
	svc		#0x80

	                               
	ldp		x0, x1, [sp], #16

	POP_FRAME
	ARM64_STACK_EPILOG

  
                                     
                                                                            
                                   
  
                    
   
	.align 2
	.globl _backoff
_backoff:
	ARM64_STACK_PROLOG
	PUSH_FRAME

	cbz		x15, Lno_preempt	                                                  

	mov		x15, xzr	                                        
	bl _preempt_self

Lno_preempt:
	ldxr	w9, [x0]		                                    
	cbz		w9, Lend_backoff                                          

	wfe						        

	b	Lno_preempt

Lend_backoff:
	clrex

	POP_FRAME
	ARM64_STACK_EPILOG
