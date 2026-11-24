/*
 * Copyright (c) 2010-2020 Apple Inc. All rights reserved.
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
#include <i386/asm.h>
#include <assym.s>
#include <debug.h>
#include "dwarf_unwind.h"
#include <i386/eflags.h>
#include <i386/rtclock_asm.h>
#include <i386/trap.h>
#define _ARCH_I386_ASM_HELP_H_	                                      
#include <mach/i386/syscall_sw.h>
#include <i386/postcode.h>
#include <i386/proc_reg.h>
#include <mach/exception_types.h>

#if DEBUG
#define	DEBUG_IDT64 		1	
#endif

  
                                                                         
                                                                       
                                          
  
                                      
  
                                                  
                                                                
                                                          
                                                                
                            
  
                                                                        
                                                              
                                         
  
                                                                    
  
                                                  
                                                                    
                                                      
                                              
                        
   
  
                                               
   
#define	HNDL_ALLINTRS		0
#define	HNDL_ALLTRAPS		1
#define	HNDL_SYSENTER		2
#define	HNDL_SYSCALL		3
#define	HNDL_UNIX_SCALL		4
#define	HNDL_MACH_SCALL		5
#define	HNDL_MDEP_SCALL		6
#define	HNDL_DOUBLE_FAULT	7
#define	HNDL_MACHINE_CHECK	8

	
                                            
	
.section	__HIB, __desc
.globl EXT(idt64_hndl_table0)
EXT(idt64_hndl_table0):
          	.quad EXT(ks_dispatch)
          	.quad EXT(ks_64bit_return)
          	.quad 0                                            
          	.quad EXT(ks_32bit_return)
#define	TBL0_OFF_DISP_USER_WITH_POPRAX	0x20
          	.quad EXT(ks_dispatch_user_with_pop_rax)
#define	TBL0_OFF_DISP_KERN_WITH_POPRAX	0x28
          	.quad EXT(ks_dispatch_kernel_with_pop_rax)
#define	TBL0_OFF_PTR_KERNEL_STACK_MASK	0x30
          	.quad 0                         

EXT(idt64_hndl_table1):
	.quad	EXT(hndl_allintrs)
	.quad	EXT(hndl_alltraps)
	.quad	EXT(hndl_sysenter)
	.quad	EXT(hndl_syscall)
	.quad	EXT(hndl_unix_scall)
	.quad	EXT(hndl_mach_scall)
	.quad	EXT(hndl_mdep_scall)
	.quad	EXT(hndl_double_fault)
	.quad	EXT(hndl_machine_check)
.text


                                                      
                                                    
                                                   
   
#if DEBUG_IDT64
#define IDT_ENTRY_WRAPPER(n, f)			 \
	push	%rax				;\
	POSTCODE2(0x6400+n)			;\
	pop	%rax				;\
	pushq	$(f)				;\
	pushq	$(n)				;\
	jmp L_dispatch
#else
#define IDT_ENTRY_WRAPPER(n, f)			 \
	pushq	$(f)				;\
	pushq	$(n)				;\
	jmp L_dispatch
#endif

                                                               
#define TRAP_ERR(n, f)				 \
	Entry(f)				;\
	IDT_ENTRY_WRAPPER(n, HNDL_ALLTRAPS)

                   
#define TRAP(n, f)				 \
	Entry(f)				;\
	pushq	$0          			;\
	IDT_ENTRY_WRAPPER(n, HNDL_ALLTRAPS)

#define USER_TRAP TRAP

                  
#define INTERRUPT(n)			 	\
	Entry(_intr_ ## n)			;\
	pushq	$0          			;\
	IDT_ENTRY_WRAPPER(n, HNDL_ALLINTRS)

                                                                                
#define TRAP_SPC(n, f)
#define TRAP_IST1(n, f)
#define TRAP_IST2(n, f)
#define USER_TRAP_SPC(n, f)

                                      
.section __HIB, __text
                            
#include "idt_table.h"

Entry(idt64_page_fault)
	pushq	$(HNDL_ALLTRAPS)
#if !(DEVELOPMENT || DEBUG)
	pushq	$(T_PAGE_FAULT)
	jmp	L_dispatch
#else
	pushq	$(T_PAGE_FAULT)

	pushq	%rax
	pushq	%rbx
	pushq	%rcx
	testb	$3, 8+8+8+ISF64_CS(%rsp)	                            
	jz	L_pfkern		                                              

	  
                                                                         
                                                                           
                                    
    
	movq	8+8+8+ISF64_RIP(%rsp), %rbx
	movq	%cr2, %rcx
	cmpq	%rbx, %rcx

	                                                            
	swapgs
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	mov	16(%rax), %rax	                              

	jne	L_dispatch_from_user_with_rbx_rcx_pushes
	jmp	abort_rip_cacheline_read

L_pfkern:
	  
                     
                                                                                                      
                                                   
    
	movq	8+8+8+ISF64_RIP(%rsp), %rbx
	leaq	rip_cacheline_read(%rip), %rcx
	cmpq	%rcx, %rbx
	jb	regular_kernel_page_fault
	leaq	rip_cacheline_read_end(%rip), %rcx
	cmpq	%rcx, %rbx
	jbe	L_pf_on_clread	                                                 

regular_kernel_page_fault:
	                            
	popq	%rcx
	popq	%rbx
	jmp	L_dispatch_from_kernel_no_push_rax

L_pf_on_clread:
	  
                                                                                                            
                                                                                                            
                                                                      
    
	popq	%rcx
	movq	%rcx, %cr2
	popq	%rbx
	leaq	abort_rip_cacheline_read(%rip), %rax
	movq	%rax, 8+ISF64_RIP(%rsp)
	popq	%rax
	addq	$24, %rsp	                                       
	iretq			                                           
#endif                              

  
                                                                                                  
                                           
   
Entry(idt64_debug)
	                                             
	push	$0			                
	pushq	$(HNDL_ALLTRAPS)
	pushq	$(T_DEBUG)
	                          
	push	%rax
	push	%rcx
	push	%rdx
	mov	$(MSR_IA32_GS_BASE), %ecx
	rdmsr					                                  
	test	$0x80000000, %edx		                                          
	jnz	1f

	  
                                                                                                  
                                                                                                      
                                                                                       
    
	testb	$3, 8+8+8+ISF64_CS(%rsp)
	jnz	2f
	pop	%rdx
	pop	%rcx
	pop	%rax
	addq	$0x18, %rsp	                                              
	jmp	EXT(ret64_iret)
2:
	swapgs					                      
1:
	pop	%rdx

	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	mov	16(%rax), %rax                               

	mov	%gs:CPU_SHADOWTASK_CR3(%rax), %rax
	mov	%rax, %cr3

	pop	%rcx

	                                                                        

	leaq    EXT(idt64_hndl_table0)(%rip), %rax
	jmp	*(%rax)

  
                                              
                                                                            
                                                                            
                            
   

#ifndef UNIX_INT
#error NO UNIX INT!!!
#endif
Entry(idt64_unix_scall)
	pushq	%rax			                             
	pushq	$(HNDL_UNIX_SCALL)
	pushq	$(UNIX_INT)
	jmp	L_u64bit_entry_check
	
Entry(idt64_mach_scall)
	pushq	%rax			                             
	pushq	$(HNDL_MACH_SCALL)
	pushq	$(MACH_INT)
	jmp	L_u64bit_entry_check
	
Entry(idt64_mdep_scall)
	pushq	%rax			                             
	pushq	$(HNDL_MDEP_SCALL)
	pushq	$(MACHDEP_INT)
	jmp	L_u64bit_entry_check

  
                                              
                                                                       
                                         
                                                                          
                                                                          
                                                                          
            
   
Entry(idt64_segnp)
	pushq	$(HNDL_ALLTRAPS)
	pushq	$(T_SEGMENT_NOT_PRESENT)
	jmp	L_check_for_kern_flt

Entry(idt64_gen_prot)
	pushq	$(HNDL_ALLTRAPS)
	pushq	$(T_GENERAL_PROTECTION)
	jmp	L_check_for_kern_flt

Entry(idt64_stack_fault)
	pushq	$(HNDL_ALLTRAPS)
	pushq	$(T_STACK_FAULT)
	jmp	L_check_for_kern_flt

L_check_for_kern_flt:
	  
                                                                  
                                                                
                                                                     
                                        
    
	pushq	%rax
	testb	$3, 8+ISF64_CS(%rsp)
	jnz	L_dispatch_from_user_no_push_rax		                                              

	                                                                                                         
	leaq	L_32bit_seg_restore_begin(%rip), %rax
	cmpq	%rax, 8+ISF64_RIP(%rsp)
	jb	L_not_32bit_segrestores
	leaq	L_32bit_seg_restore_done(%rip), %rax
	cmpq	%rax, 8+ISF64_RIP(%rsp)
	jae	L_not_32bit_segrestores
	jmp	1f
L_not_32bit_segrestores:
	leaq	EXT(ret32_iret)(%rip), %rax
	cmpq	%rax, 8+ISF64_RIP(%rsp)
	je	1f
	leaq	EXT(ret64_iret)(%rip), %rax
	cmpq	%rax, 8+ISF64_RIP(%rsp)
	je	1f
	jmp	L_dispatch_from_kernel_no_push_rax
	  
                                                                    
                                                                
   
                                                               
   
    
                              
                              
                              
                             
                           
                                                                        
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
    
1:
	pushq	%rbx
	movq	16+ISF64_RSP(%rsp), %rbx
	movq	ISF64_CS-24(%rbx), %rax
	testb	$3, %al					                                                    
	jnz	2f
	popq	%rbx
	jmp	L_dispatch_from_kernel_no_push_rax	                                                    
2:
	  
                                                                              
    

	movq	%rax, 16+ISF64_CS(%rsp)

	movq	ISF64_RSP-24(%rbx), %rax
	movq	%rax, 16+ISF64_RSP(%rsp)

	movq	ISF64_RIP-24(%rbx), %rax
	movq	%rax, 16+ISF64_RIP(%rsp)

	movq	ISF64_SS-24(%rbx), %rax
	movq	%rax, 16+ISF64_SS(%rsp)

	movq	ISF64_RFLAGS-24(%rbx), %rax
	movq	%rax, 16+ISF64_RFLAGS(%rsp)

	popq	%rbx
	jmp	L_dispatch_from_user_no_push_rax


  
                            
   
Entry(idt64_db_task_dbl_fault)
	pushq	$(HNDL_DOUBLE_FAULT)
	pushq	$(T_DOUBLE_FAULT)
	jmp	L_dispatch

Entry(idt64_db_task_stk_fault)
	pushq	$(HNDL_DOUBLE_FAULT)
	pushq	$(T_STACK_FAULT)
	jmp	L_dispatch

Entry(idt64_mc)
	push	$(0)			           
	pushq	$(HNDL_MACHINE_CHECK)
	pushq	$(T_MACHINE_CHECK)
	jmp	L_dispatch

  
      
                                                            
                                                                      
  
                                                                
                                                    
   
Entry(idt64_nmi)
	push	%rax
	push	%rcx
	push	%rdx
	testb	$3, ISF64_CS(%rsp)
	jz	1f

	                                                       
	swapgs

	leaq    EXT(idt64_hndl_table0)(%rip), %rax
	mov     16(%rax), %rax                               
	mov     %gs:CPU_SHADOWTASK_CR3(%rax), %rax
	mov     %rax, %cr3			                                                                    

	mov	%gs:CPU_UBER_ISF, %rcx		                    
	add	$(ISF64_SIZE), %rcx		                           

	leaq    TBL0_OFF_DISP_USER_WITH_POPRAX+EXT(idt64_hndl_table0)(%rip), %rax		                                   
	jmp	4f						                       

1:
	  
                      
                                                   
                                                                                                   
    
	mov	$(MSR_IA32_GS_BASE), %ecx
	rdmsr					                        
	test	$0x80000000, %edx		                         
	jnz	2f
	swapgs					             
	movl	$1, ISF64_CS+4(%rsp)		                             
2:

	leaq    EXT(idt64_hndl_table0)(%rip), %rax
	mov     16(%rax), %rax                               
	mov	%cr3, %rdx
	mov     %gs:CPU_SHADOWTASK_CR3(%rax), %rax
	mov     %rax, %cr3                                                          

	  
                                                            
                     
    
	mov	ISF64_RSP(%rsp), %rcx
	mov	%gs:CPU_KERNEL_STACK, %rax
	xor	%rcx, %rax
	movq	TBL0_OFF_PTR_KERNEL_STACK_MASK+EXT(idt64_hndl_table0)(%rip), %rdx
	mov	(%rdx), %rdx		                            
	and	%rdx, %rax
	test	%rax, %rax		                                 
	jz	3f			         

	mov	%gs:CPU_INT_STACK_TOP, %rax
	cmp	%rcx, %rax		                                    
	jb	5f			        
	leaq	-INTSTACK_SIZE(%rax), %rax
	cmp	%rcx, %rax
	jb	3f			         
5:
	mov    %gs:CPU_KERNEL_STACK, %rcx
3:
	                                                         
	and	$0xFFFFFFFFFFFFFFF0, %rcx

	leaq    TBL0_OFF_DISP_KERN_WITH_POPRAX+EXT(idt64_hndl_table0)(%rip), %rax		                                     
4:
	  
                                                                   
                                                           
                               
                               
                               
    
	xchg	%rsp, %rcx			                    
	push	ISF64_SS(%rcx)
	push	ISF64_RSP(%rcx)
	push	ISF64_RFLAGS(%rcx)
	push	ISF64_CS(%rcx)
	push	ISF64_RIP(%rcx)
	                                             
	push	$(0)				                  
	push	$(HNDL_ALLINTRS)		                     
	push	$(T_NMI)			                  
	push	ISF64_ERR(%rcx)			                                                                    
	mov	ISF64_TRAPNO(%rcx), %rdx
	mov	ISF64_TRAPFN(%rcx), %rcx

	jmp	*(%rax)		                                            

Entry(idt64_double_fault)
	pushq	$(HNDL_DOUBLE_FAULT)
	pushq	$(T_DOUBLE_FAULT)
	jmp	L_dispatch

Entry(hi64_syscall)
Entry(idt64_syscall)
	swapgs
                                                                       
                                                                       
               
        
	shlq	$32, %rax
	or 	%rax, %r11
.globl EXT(dblsyscall_patch_point)
EXT(dblsyscall_patch_point):
                                       
                                                                
                
        
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	mov	16(%rax), %rax
	mov     %rsp, %gs:CPU_UBER_TMP(%rax)                       
	mov     %gs:CPU_ESTACK(%rax), %rsp                                      
	sub	$(ISF64_SIZE), %rsp

	  
                                                  
    
	movl	$(USER_DS), ISF64_SS(%rsp)
	mov	%rcx, ISF64_RIP(%rsp)		         

	mov	%gs:CPU_UBER_TMP(%rax), %rcx
	mov	%rcx, ISF64_RSP(%rsp)		                          

	mov	%r11, %rax
	shrq	$32, %rax		                 
	mov	%r11d, %r11d		                      

	mov	%r11, ISF64_RFLAGS(%rsp)	            
	movl	$(SYSCALL_CS), ISF64_CS(%rsp)	                           
	mov	%rax, ISF64_ERR(%rsp)		                            
	movq	$(HNDL_SYSCALL), ISF64_TRAPFN(%rsp)
	movq	$(T_SYSCALL), ISF64_TRAPNO(%rsp)	            
	swapgs
	jmp	L_dispatch			                             

Entry(hi64_sysenter)
Entry(idt64_sysenter)
	                                               
                    
    
	push	$(USER_DS)		        
	push	%rcx			          
	pushf				           
	  
                                                        
                                               
    
	push	$0
	popf
	push	$(SYSENTER_CS)		         
L_sysenter_continue:
	push	%rdx			         
	push	%rax			                            
	pushq	$(HNDL_SYSENTER)
	pushq	$(T_SYSENTER)
	orl	$(EFL_IF), ISF64_RFLAGS(%rsp)
	jmp	L_u64bit_entry_check

#if DEVELOPMENT || DEBUG
do_cacheline_stash:
	  
                                                                                      
                                                        
    

	pushq	%rbx
	pushq	%rcx
L_dispatch_from_user_with_rbx_rcx_pushes:
	movq	8+8+8+ISF64_RIP(%rsp), %rbx
	andq	$-64, %rbx	                                         
	pushf
	  
                                                                                            
                                                                                         
    
	orq	$(1 << 18), (%rsp)
	popf
	  
                                                                                          
                                                                                          
                                                                                         
    
	movq	%cr2, %rcx	                                                                             
				                                                                            
				                                                                               
				                                           
rip_cacheline_read:
	mov	(%rbx), %rcx
	                                                                                                      
	mov	%rcx, %gs:CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	8(%rbx), %rcx
	mov	%rcx, %gs:8+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	16(%rbx), %rcx
	mov	%rcx, %gs:16+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	24(%rbx), %rcx
	mov	%rcx, %gs:24+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	32(%rbx), %rcx
	mov	%rcx, %gs:32+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	40(%rbx), %rcx
	mov	%rcx, %gs:40+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
	mov	48(%rbx), %rcx
	mov	%rcx, %gs:48+CPU_RTIMES(%rax)
	movq    %cr2, %rcx
rip_cacheline_read_end:
	mov	56(%rbx), %rcx
	mov	%rcx, %gs:56+CPU_RTIMES(%rax)

	pushf
	andq	$~(1 << 18), (%rsp) 	                   
	popf

	jmp	cacheline_read_cleanup_stack

abort_rip_cacheline_read:
	pushf
	andq	$~(1 << 18), (%rsp) 	                   
	popf
abort_rip_cacheline_read_no_smap_reenable:
	movl	$0xdeadc0de, %ecx			                                                                  
	shlq	$32, %rcx
	movl	$0xbeefcafe, %ebx
	orq	%rbx, %rcx
	movq	%rcx, %gs:CPU_RTIMES(%rax)
	movq	%rcx, %gs:8+CPU_RTIMES(%rax)

cacheline_read_cleanup_stack:
	popq	%rcx
	popq	%rbx
	jmp	L_dispatch_kgsb
#endif                              

  
                         
                                                                       
             
                                                    
                                                                   
                                             
                               
   

L_dispatch:
	pushq	%rax
	testb	$3, 8+ISF64_CS(%rsp)
	jz	1f
L_dispatch_from_user_no_push_rax:
	swapgs
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	mov	16(%rax), %rax	                              

#if DEVELOPMENT || DEBUG
	                                               
	cmpl	$(T_INVALID_OPCODE), 8+ISF64_TRAPNO(%rsp)
	je	do_cacheline_stash
	cmpl	$(T_PAGE_FAULT), 8+ISF64_TRAPNO(%rsp)
	je	do_cacheline_stash
	cmpl	$(T_GENERAL_PROTECTION), 8+ISF64_TRAPNO(%rsp)
	je	do_cacheline_stash
#endif

L_dispatch_kgsb:
	mov	%gs:CPU_SHADOWTASK_CR3(%rax), %rax
	mov	%rax, %cr3
#if	DEBUG
	mov	%rax, %gs:CPU_ENTRY_CR3
#endif
L_dispatch_from_kernel_no_push_rax:
1:
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	                                                                                                           
	                                                     
	jmp *(%rax)
                                                                         
Entry(ks_64bit_return)

	mov	R64_R14(%r15), %r14
	mov	R64_R13(%r15), %r13
	mov	R64_R12(%r15), %r12
	mov	R64_R11(%r15), %r11
	mov	R64_R10(%r15), %r10
	mov	R64_R9(%r15),  %r9
	mov	R64_R8(%r15),  %r8
	mov	R64_RSI(%r15), %rsi
	mov	R64_RDI(%r15), %rdi
	mov	R64_RBP(%r15), %rbp
	mov	R64_RDX(%r15), %rdx
	mov	R64_RCX(%r15), %rcx
	mov	R64_RBX(%r15), %rbx
	mov	R64_RAX(%r15), %rax
	                                       
	mov	%gs:CPU_ESTACK, %rsp

	                                                                           
	push	R64_SS(%r15)
	push	R64_RSP(%r15)
	push	R64_RFLAGS(%r15)
	push	R64_CS(%r15)
	push	R64_RIP(%r15)

	cmpq	$(KERNEL64_CS), 8(%rsp)
	jne	1f			                                                                        
	mov	R64_R15(%r15), %r15
	jmp	L_64b_kernel_return	                         

1:
	push	%rax				         
	movl	%gs:CPU_NEED_SEGCHK, %eax
	push	%rax				         

	                       
	cmpl	$0, %gs:CPU_CURTASK_HAS_LDT	                                                                    
	jne	L_64b_segops_island

	  
                                                            
                                                              
                                                            
                                                               
                     
    
	mov	R64_R15(%r15), %r15

	  
                                                           
                                                              
                           
    
	                            
	mov	%gs:CPU_UCR3, %rax
#if	DEBUG
	mov	%rax, %gs:CPU_EXIT_CR3
#endif
	mov	%rax, %cr3
	                                                              
	swapgs

L_chk_sysret:
	pop	%rax	                                             

	  
                                      
   
                    
                        
                        
                        
                        
                       
                             
                    
    
	cmpl	$(SYSCALL_CS), 16(%rsp)                               
	je      L_sysret

	cmpl	$1, %eax
	je	L_verw_island_2

	pop	%rax		                           

L_64b_kernel_return:
.globl EXT(ret64_iret)
EXT(ret64_iret):
        iretq			                           


L_sysret:
	cmpl	$1, %eax
	je	L_verw_island_3

	pop	%rax		                           
	  
                                                                          
                 
                   
                          
    
	pop	%rcx
	add	$8, %rsp
	pop	%r11
	pop	%rsp
	sysretq			                             


L_verw_island_2:

	pop	%rax		                           
	verw	40(%rsp)	                                                         
	jmp	EXT(ret64_iret)


L_verw_island_3:

	pop	%rax		                           

	  
                                                                          
                 
                   
                          
    
	pop	%rcx
	add	$8, %rsp
	pop	%r11
	verw	8(%rsp)		                                                         
	pop	%rsp
	sysretq			                             


L_64b_segops_island:

	                                                                                                            
	                                                                               
	cmpl	$(USER64_CS), R64_CS(%r15)
	jz 	11f
	larw	R64_CS(%r15), %ax
	jnz	L_64_reset_cs
	                                                                                                 
	testw	$0x800, %ax
	jz	L_64_reset_cs		                                                           
	jmp	11f
L_64_reset_cs:
	movl	$(USER64_CS), R64_CS(%r15)
11:
	cmpl	$0, R64_DS(%r15)
	jz 	22f
	larw	R64_DS(%r15), %ax
	jz	22f
	movl	$0, R64_DS(%r15)
22:
	cmpl	$0, R64_ES(%r15)
	jz 	33f
	larw	R64_ES(%r15), %ax
	jz	33f
	movl	$0, R64_ES(%r15)
33:
	cmpl	$0, R64_FS(%r15)
	jz 	44f
	larw	R64_FS(%r15), %ax
	jz	44f
	movl	$0, R64_FS(%r15)
44:
	cmpl	$0, R64_GS(%r15)
	jz	55f
	larw	R64_GS(%r15), %ax
	jz	55f
	movl	$0, R64_GS(%r15)
55:
	  
                                                            
                                        
                                                       
                                                  
    
	mov	%gs:CPU_ACTIVE_THREAD, %rax	                           
	cmpq	$0, TH_CTH_SELF(%rax)
	je	L_restore_gs
	movw	$0xFFFF, %ax
	jmp	1f
L_restore_gs:
	movw	R64_GS(%r15), %ax
1:
	shlq	$16, %rax
	movw	R64_FS(%r15), %ax
	shlq	$16, %rax
	movw	R64_ES(%r15), %ax
	shlq	$16, %rax
	movw	R64_DS(%r15), %ax

	  
                                                        
                                                         
    
	mov	R64_R15(%r15), %r15

	                            
	push	%rax
	mov	%gs:CPU_UCR3, %rax
#if	DEBUG
	mov	%rax, %gs:CPU_EXIT_CR3
#endif
	mov	%rax, %cr3
	                                                              
	pop	%rax
	swapgs

	  
                                                                   
                                                        
   
                                                                  
                                                             
                                                      
    
	movw	%ax, %ds
	shrq	$16, %rax

	movw	%ax, %es
	shrq	$16, %rax

	movw	%ax, %fs
	shrq	$16, %rax

	  
                                                             
                                                              
                                                                  
    
	cmpw	$0xFFFF, %ax
	je	L_chk_sysret
	movw	%ax, %gs		                                   
	jmp	L_chk_sysret


L_u64bit_entry_check:
	  
                                           
    
	pushq	%rax
	swapgs
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	mov	16(%rax), %rax

	cmpl	$(TASK_MAP_32BIT), %gs:CPU_TASK_MAP(%rax)
	jne	L_64bit_entry_reject
	jmp	L_dispatch_kgsb

L_64bit_entry_reject:
	  
                                                              
    
	movq	$(HNDL_ALLTRAPS), 8+ISF64_TRAPFN(%rsp)
	movq	$(T_INVALID_OPCODE), 8+ISF64_TRAPNO(%rsp)
	jmp 	L_dispatch_kgsb

Entry(ks_32bit_return)

	                                                                                                            
	                                                                                      
	cmpl	$(USER_CS), R32_CS(%r15)
	jz 	11f
	larw	R32_CS(%r15), %ax
	jnz	L_32_reset_cs
	                                                                                                 
	testw	$0x800, %ax
	jz	L_32_reset_cs		                                                           
	jmp	11f
L_32_reset_cs:
	movl	$(USER_CS), R32_CS(%r15)
11:
	cmpl	$(USER_DS), R32_DS(%r15)
	jz	22f
	cmpl	$0, R32_DS(%r15)
	jz 	22f
	larw	R32_DS(%r15), %ax
	jz	22f
	movl	$(USER_DS), R32_DS(%r15)
22:
	cmpl	$(USER_DS), R32_ES(%r15)
	jz	33f
	cmpl	$0, R32_ES(%r15)
	jz 	33f
	larw	R32_ES(%r15), %ax
	jz	33f
	movl	$(USER_DS), R32_ES(%r15)
33:
	cmpl	$(USER_DS), R32_FS(%r15)
	jz	44f
	cmpl	$0, R32_FS(%r15)
	jz 	44f
	larw	R32_FS(%r15), %ax
	jz	44f
	movl	$(USER_DS), R32_FS(%r15)
44:
	cmpl	$(USER_CTHREAD), R32_GS(%r15)
	jz	55f
	cmpl	$0, R32_GS(%r15)
	jz 	55f
	larw	R32_GS(%r15), %ax
	jz	55f
	movl	$(USER_CTHREAD), R32_GS(%r15)
55:

	  
                                    
    
	movl	R32_EAX(%r15), %eax
	movl	R32_EBX(%r15), %ebx
	movl	R32_ECX(%r15), %ecx
	movl	R32_EDX(%r15), %edx
	movl	R32_EBP(%r15), %ebp
	movl	R32_ESI(%r15), %esi
	movl	R32_EDI(%r15), %edi
	movl	R32_DS(%r15), %r8d
	movl	R32_ES(%r15), %r9d
	movl	R32_FS(%r15), %r10d
	movl	R32_GS(%r15), %r11d

	                                                          
	mov	%gs:CPU_ESTACK, %rsp

	                                                                                
	movl	R32_SS(%r15), %r12d
	push	%r12
	movl	R32_UESP(%r15), %r12d
	push	%r12
	movl	R32_EFLAGS(%r15), %r12d
	push	%r12
	movl	R32_CS(%r15), %r12d
	push	%r12
	movl	R32_EIP(%r15), %r12d
	push	%r12

	movl	%gs:CPU_NEED_SEGCHK, %r14d	                                               

	  
                                                                         
                                                                             
                                                                     
    
	                            
	mov	%gs:CPU_UCR3, %r13
#if	DEBUG
	mov	%r13, %gs:CPU_EXIT_CR3
#endif
	mov	%r13, %cr3

	swapgs

	  
                                                                          
                                                                        
                                                                           
                                     
    
L_32bit_seg_restore_begin:
	mov	%r8, %ds
	mov	%r9, %es
	mov	%r10, %fs
	mov	%r11, %gs
L_32bit_seg_restore_done:

	                                                      
	xor	%r8, %r8
	xor	%r9, %r9
	xor	%r10, %r10
	xor	%r11, %r11
	xor	%r12, %r12
	xor	%r13, %r13
	xor	%r15, %r15

	  
                                      
   
                    
                        
                        
                        
                       
                            
                    
    

	cmpl	$(SYSENTER_CS), 8(%rsp)
					                      
	je      L_rtu_via_sysexit

	cmpl	$1, %r14d
	je	L_verw_island

L_after_verw:
	xor	%r14, %r14

.globl EXT(ret32_iret)
EXT(ret32_iret):
	iretq				                           

L_verw_island:
	verw	32(%rsp)
	jmp	L_after_verw

L_verw_island_1:
	verw	16(%rsp)
	jmp	L_after_verw_1

L_rtu_via_sysexit:
	pop	%rdx			                     
	pop	%rcx			                     
	andl	$(~EFL_IF), (%rsp)	                                        

	  
                          
    
	cmpl	$1, %r14d
	je	L_verw_island_1
L_after_verw_1:
	xor	%r14, %r14

	popf				                                   
	pop	%rcx			                     


	sti				                                      
	sysexitl			                    

                               
.text

Entry(ks_dispatch)
	popq	%rax
	cmpl	$(KERNEL64_CS), ISF64_CS(%rsp)
	je	EXT(ks_dispatch_kernel)

	mov 	%rax, %gs:CPU_UBER_TMP
	mov 	%gs:CPU_UBER_ISF, %rax
	add 	$(ISF64_SIZE), %rax

	xchg	%rsp, %rax
                                               
                                                                       
                                     
   
	push	ISF64_SS(%rax)
	push	ISF64_RSP(%rax)
	push	ISF64_RFLAGS(%rax)
	push	ISF64_CS(%rax)
	push	ISF64_RIP(%rax)
	push	ISF64_ERR(%rax)
	push	ISF64_TRAPFN(%rax)
	push 	ISF64_TRAPNO(%rax)
	mov	%gs:CPU_UBER_TMP, %rax
	jmp	EXT(ks_dispatch_user)

Entry(ks_dispatch_user_with_pop_rax)
	pop	%rax
	jmp	EXT(ks_dispatch_user)

Entry(ks_dispatch_user)
	cmpl	$(TASK_MAP_32BIT), %gs:CPU_TASK_MAP
	je	L_dispatch_U32		                      

L_dispatch_U64:
	subq	$(ISS64_OFFSET), %rsp
	mov	%r15, R64_R15(%rsp)
	mov	%rsp, %r15
	mov	%gs:CPU_KERNEL_STACK, %rsp
	jmp	L_dispatch_64bit

Entry(ks_dispatch_kernel_with_pop_rax)
	pop	%rax
	jmp	EXT(ks_dispatch_kernel)

Entry(ks_dispatch_kernel)
	subq	$(ISS64_OFFSET), %rsp
	mov	%r15, R64_R15(%rsp)
	mov	%rsp, %r15

  
                                      
   
L_dispatch_64bit:
	movl	$(SS_64), SS_FLAVOR(%r15)

	  
                                          
                                            
    
	cmpl	$0, %gs:CPU_CURTASK_HAS_LDT
	je	L_skip_save_extra_segregs

	mov	%ds, R64_DS(%r15)
	mov	%es, R64_ES(%r15)

L_skip_save_extra_segregs:
	mov	%fs, R64_FS(%r15)
	mov	%gs, R64_GS(%r15)


	                                    
	mov	%rax, R64_RAX(%r15)
	mov	%rbx, R64_RBX(%r15)
	mov	%rcx, R64_RCX(%r15)
	mov	%rdx, R64_RDX(%r15)
	mov	%rbp, R64_RBP(%r15)
	mov	%rdi, R64_RDI(%r15)
	mov	%rsi, R64_RSI(%r15)
	mov	%r8,  R64_R8(%r15)
	mov	%r9,  R64_R9(%r15)
	mov	%r10, R64_R10(%r15)
	mov	%r11, R64_R11(%r15)
	mov	%r12, R64_R12(%r15)
	mov	%r13, R64_R13(%r15)
	mov	%r14, R64_R14(%r15)

	                                                                                                       
	xor	%ecx, %ecx
	xor	%edi, %edi
	xor	%r8, %r8
	xor	%r9, %r9
	xor	%r10, %r10
	xor	%r11, %r11
	xor	%r12, %r12
	xor	%r13, %r13
	xor	%r14, %r14

	                                             
	mov	%cr2, %rax
	mov	%rax, R64_CR2(%r15)

L_dispatch_U64_after_fault:
	mov	R64_TRAPNO(%r15), %ebx	                              
	mov	R64_TRAPFN(%r15), %rdx	                              
	mov	R64_CS(%r15), %esi	                          

	jmp	L_common_dispatch

L_dispatch_U32:                       
	subq	$(ISS64_OFFSET), %rsp
	mov	%rsp, %r15
	mov	%gs:CPU_KERNEL_STACK, %rsp
	movl	$(SS_32), SS_FLAVOR(%r15)

	  
                     
    
	mov	%ds, R32_DS(%r15)
	mov	%es, R32_ES(%r15)
	mov	%fs, R32_FS(%r15)
	mov	%gs, R32_GS(%r15)

	  
                                 
    
	mov	%eax, R32_EAX(%r15)
	mov	%ebx, R32_EBX(%r15)
	mov	%ecx, R32_ECX(%r15)
	mov	%edx, R32_EDX(%r15)
	mov	%ebp, R32_EBP(%r15)
	mov	%esi, R32_ESI(%r15)
	mov	%edi, R32_EDI(%r15)

	                                                              
	mov	%cr2, %rax
	mov	%eax, R32_CR2(%r15)
	                                                                                                           
	xor	%ecx, %ecx
	xor	%edi, %edi
	xor	%r8, %r8
	xor	%r9, %r9
	xor	%r10, %r10
	xor	%r11, %r11
	xor	%r12, %r12
	xor	%r13, %r13
	xor	%r14, %r14

	  
                                                      
                                                             
    
	mov	R64_RIP(%r15), %eax
	mov	%eax, R32_EIP(%r15)
	mov	R64_RFLAGS(%r15), %eax
	mov	%eax, R32_EFLAGS(%r15)
	mov	R64_RSP(%r15), %eax
	mov	%eax, R32_UESP(%r15)
	mov	R64_SS(%r15), %eax
	mov	%eax, R32_SS(%r15)
L_dispatch_U32_after_fault:
	mov	R64_CS(%r15), %esi		                           
	mov	%esi, R32_CS(%r15)
	mov	R64_TRAPNO(%r15), %ebx		                              
	mov	%ebx, R32_TRAPNO(%r15)
	mov	R64_ERR(%r15), %eax
	mov	%eax, R32_ERR(%r15)
	mov	R64_TRAPFN(%r15), %rdx		                              

L_common_dispatch:
	cld		                                                      
	cmpl    $0, EXT(pmap_smap_enabled)(%rip)
	je	1f
	clac		                                                
1:
	  
                                                                     
                                                                               
                                                            
                                                                        
                                                         
    
	mov	%gs:CPU_KERNEL_CR3, %rcx
	mov	%rcx, %gs:CPU_ACTIVE_CR3
	test	$3, %esi			                      
	jz	2f				                                    
	xor	%ebp, %ebp
	cmpl	$0, %gs:CPU_PAGEZERO_MAPPED
	jnz	11f
	cmpl	$0, EXT(no_shared_cr3)(%rip)
	je	2f
11:
	xor	%eax, %eax
	movw	%gs:CPU_KERNEL_PCID, %ax
	or	%rax, %rcx
	mov	%rcx, %cr3			                     
	jmp	4f
2:
	                                                                           
	mov     %gs:CPU_ACTIVE_CR3+4, %rcx
	shr     $32, %rcx
	testl   %ecx, %ecx
	jz      4f
	movl    $0, %gs:CPU_TLB_INVALID
	cmpb	$0, EXT(invpcid_enabled)(%rip)
	jz	L_cr4_island
	movl	$2, %ecx
	invpcid %gs:CPU_IP_DESC, %rcx
4:
L_set_act:
	mov	%gs:CPU_ACTIVE_THREAD, %rcx	                           
	testq	%rcx, %rcx
	je	L_intcnt
	movl	$-1, TH_IOTIER_OVERRIDE(%rcx)	                                                       
	cmpq	$0, TH_PCB_IDS(%rcx)	                                      
	jnz	L_dr7_island
L_intcnt:
	incl	%gs:hwIntCnt(,%ebx,4)		                           
	                                     
	cmp	EXT(dblmap_base)(%rip), %rsp
	jb	66f
	cmp	EXT(dblmap_max)(%rip), %rsp
	jge	66f
	subq	EXT(dblmap_dist)(%rip), %rsp
	subq	EXT(dblmap_dist)(%rip), %r15
66:
	leaq	EXT(idt64_hndl_table1)(%rip), %rax
	jmp	*(%rax, %rdx, 8)

L_cr4_island:
	mov	%cr4, %rcx                                               
	and	$(~CR4_PGE), %rcx
	mov	%rcx, %cr4
	or	$(CR4_PGE), %rcx
	mov	%rcx, %cr4
	jmp	L_set_act
L_dr7_island:
	xor	%ecx, %ecx		                                    
	mov	%rcx, %dr7
	jmp	L_intcnt
  
                                            
    
Entry(return_to_user)
	TIME_TRAP_UEXIT

Entry(ret_to_user)
	mov	%gs:CPU_ACTIVE_THREAD, %rdx
	cmpq	$0, TH_PCB_IDS(%rdx)	                                        
	jnz	L_dr_restore_island
L_post_dr_restore:
	  
                                                                     
                                                          
    
	mov	%gs:CPU_TASK_CR3, %rcx
	mov	%rcx, %gs:CPU_ACTIVE_CR3
	cmpl	$0, %gs:CPU_PAGEZERO_MAPPED
	jnz	L_cr3_switch_island
	movl	EXT(no_shared_cr3)(%rip), %eax
	test	%eax, %eax		                    
	jnz	L_cr3_switch_island

L_cr3_switch_return:
	mov	%gs:CPU_DR7, %rax	                                       
	cmp	$0, %rax
	je	4f
	mov	%rax, %dr7		             
	movq	$0, %gs:CPU_DR7
4:
	cmpl	$(SS_64), SS_FLAVOR(%r15)	                   
	jne	L_32bit_return

	  
                                     
                                               
    
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	jmp	*8(%rax)


L_32bit_return:
#if DEBUG_IDT64
	cmpl	$(SS_32), SS_FLAVOR(%r15)	                   
	je	1f
	cli
	POSTCODE2(0x6432)
	CCALL1(panic_idt64, %r15)
1:
#endif                  

	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	jmp	*0x18(%rax)


L_dr_restore_island:
	movq    TH_PCB_IDS(%rdx),%rax                                         
	cmpl	$(TASK_MAP_32BIT), %gs:CPU_TASK_MAP                            
	jne	1f
	movl	DS_DR0(%rax), %ecx	                                
	movq	%rcx, %dr0
	movl	DS_DR1(%rax), %ecx
	movq	%rcx, %dr1
	movl	DS_DR2(%rax), %ecx
	movq	%rcx, %dr2
	movl	DS_DR3(%rax), %ecx
	movq	%rcx, %dr3
	movl	DS_DR7(%rax), %ecx
	movq 	%rcx, %gs:CPU_DR7
	jmp 	2f
1:
	mov	DS64_DR0(%rax), %rcx	                            
	mov	%rcx, %dr0
	mov	DS64_DR1(%rax), %rcx
	mov	%rcx, %dr1
	mov	DS64_DR2(%rax), %rcx
	mov	%rcx, %dr2
	mov	DS64_DR3(%rax), %rcx
	mov	%rcx, %dr3
	mov	DS64_DR7(%rax), %rcx
	mov 	%rcx, %gs:CPU_DR7
2:
	jmp	L_post_dr_restore
L_cr3_switch_island:
	xor	%eax, %eax
	movw	%gs:CPU_ACTIVE_PCID, %ax
	or	%rax, %rcx
	mov	%rcx, %cr3
	jmp	L_cr3_switch_return

ret_to_kernel:
#if DEBUG_IDT64
	cmpl	$(SS_64), SS_FLAVOR(%r15)	                   
	je	1f
	cli
	POSTCODE2(0x6464)
	CCALL1(panic_idt64, %r15)
	hlt
1:
	cmpl	$(KERNEL64_CS), R64_CS(%r15)
	je	2f
	CCALL1(panic_idt64, %r15)
	hlt
2:
#endif
	  
                                     
                                               
    
	leaq	EXT(idt64_hndl_table0)(%rip), %rax
	jmp *8(%rax)

                                              
                                
                                                                      
                 
   
                                       
                                       
                      
                         
   
Entry(hndl_alltraps)
	mov	%esi, %eax
	testb	$3, %al
	jz	trap_from_kernel

	TIME_TRAP_UENTRY

	                                                  
	mov	%gs:CPU_ACTIVE_THREAD, %rcx
	movl	$-1, TH_IOTIER_OVERRIDE(%rcx)	                                                                 
	mov	TH_TASK(%rcx), %rbx
	TASK_VTIMER_CHECK(%rbx, %rcx)

	CCALL1(user_trap, %r15)			                            
	                                    
	cli					                                       
	xorl	%ecx, %ecx			                                     


Entry(return_from_trap)
	movq	%gs:CPU_ACTIVE_THREAD,%r15	                        
	movl	$-1, TH_IOTIER_OVERRIDE(%r15)	                                                                

	cmpl	$0, TH_RWLOCK_COUNT(%r15)	                                                       
	jz	1f
	xorq	%rbp, %rbp			                        
	mov	%r15, %rdi			                               
	CCALL(lck_rw_clear_promotions_x86)	                                
1:	

	cmpl	$0, TH_TMP_ALLOC_CNT(%r15)	                                                  
	jz	1f
	xorq	%rbp, %rbp			                        
	mov	%r15, %rdi			                               
	CCALL(kheap_temp_leak_panic)
1:

	movq	TH_PCB_ISS(%r15), %r15		               
	movl	%gs:CPU_PENDING_AST,%eax
	testl	%eax,%eax
	je	EXT(return_to_user)		                      

L_return_from_trap_with_ast:
	testl	%ecx, %ecx		                                                   
	je	2f			                           
	cmpl	$(SS_64), SS_FLAVOR(%r15)	                           
	je	1f
					                            
	movl	R32_EIP(%r15), %edi
	xorq	%rbp, %rbp		                        
	CCALL(commpage_is_in_pfz32)
	testl	%eax, %eax
	je	2f			                                      
	movl	%eax, R32_EBX(%r15)	                                          
	jmp	EXT(return_to_user)
1:
	movq	R64_RIP(%r15), %rdi
	xorq	%rbp, %rbp		                        
	CCALL(commpage_is_in_pfz64)
	testl	%eax, %eax
	je	2f			                                      
	movl	%eax, R64_RBX(%r15)	                                          
	jmp	EXT(return_to_user)
2:

	xorq	%rbp, %rbp		                        
	CCALL(ast_taken_user)		                                                                       

	cli
	mov	%rsp, %r15		                                    
	xorl	%ecx, %ecx		                                     
	jmp	EXT(return_from_trap)	                            

  
                                                    
                                                                          
                                                                        
   
   
trap_from_kernel:

UNWIND_PROLOGUE	
	
	movq	%r15, %rdi		                      

UNWIND_DIRECTIVES	

	pushq   R64_RIP(%r15)                                                 
	pushq   %rbp                                                   
	movq    %rsp, %rbp
	CCALLWITHSP(kernel_trap)	                            
	popq    %rbp
	addq    $8, %rsp
	mov	%rsp, %r15		                                     
	cli

	movl	%gs:CPU_PENDING_AST,%eax	                      
	testl	$(AST_URGENT),%eax		                            
	je	ret_to_kernel			                       
	cmpl	$(T_PREEMPT),R64_TRAPNO(%r15)
	je	ret_to_kernel			                                        
	testl	$(EFL_IF),R64_RFLAGS(%r15)	                          
	je	ret_to_kernel
	cmpl	$0,%gs:CPU_PREEMPTION_LEVEL	                          
	jne	ret_to_kernel
	movq	%gs:CPU_KERNEL_STACK,%rax
	movq	%rsp,%rcx
	xorq	%rax,%rcx
	andq	EXT(kernel_stack_mask)(%rip),%rcx
	testq	%rcx,%rcx		                                 
	jne	ret_to_kernel		                 

	CCALL(ast_taken_kernel)                           

	mov	%rsp, %r15		                                    
	jmp	ret_to_kernel

UNWIND_EPILOGUE
	
  
                                               
                         
                                 
                  
  
                                       
                      
                         
   
Entry(hndl_allintrs)

UNWIND_PROLOGUE	
	
	  
                                           
    
	movq	%gs:CPU_INT_STACK_TOP,%rcx
	cmpq	%rsp,%rcx
	jb	1f
	leaq	-INTSTACK_SIZE(%rcx),%rdx
	cmpq	%rsp,%rdx
	jb	int_from_intstack
1:
	xchgq	%rcx,%rsp		                               

	mov	%cr0,%rax		             
	orl	$(CR0_TS),%eax		                  
	mov	%rax,%cr0		             

	pushq	%rcx			                               
	pushq	%gs:CPU_INT_STATE	                              
	movq	%r15,%gs:CPU_INT_STATE	                    

UNWIND_DIRECTIVES	
	
	TIME_INT_ENTRY			               

	                                                  
	mov	%gs:CPU_ACTIVE_THREAD, %rcx
	mov	TH_TASK(%rcx), %rbx
	TASK_VTIMER_CHECK(%rbx, %rcx)

	incl	%gs:CPU_PREEMPTION_LEVEL
	incl	%gs:CPU_INTERRUPT_LEVEL

	CCALL1(interrupt, %r15)		                                    
	
UNWIND_EPILOGUE

.globl	EXT(return_to_iret)
LEXT(return_to_iret)			                                         

	decl	%gs:CPU_INTERRUPT_LEVEL
	decl	%gs:CPU_PREEMPTION_LEVEL

	TIME_INT_EXIT			               

	popq	%gs:CPU_INT_STATE 	                                    
	popq	%rsp			                              

	movq	%gs:CPU_ACTIVE_THREAD,%rax
	movq	TH_PCB_FPS(%rax),%rax	                    
	cmpq	$0,%rax			                        
	je	1f			                   
	movl	FP_VALID(%rax),%eax	                   
	cmpl	$0,%eax			                    
	jne	1f			                     
	clts				              
	jmp	2f
1:
	mov	%cr0,%rax		             
	orl	$(CR0_TS),%eax		                  
	mov	%rax,%cr0		             
2:
	                                             
	movl	R64_CS(%r15),%eax	                         
	cmpl	$(SS_32),SS_FLAVOR(%r15)             
#if DEBUG_IDT64
	jne	5f
	movl	R32_CS(%r15),%eax	                      
	jmp	3f
5:
	cmpl    $(SS_64),SS_FLAVOR(%r15)
	je	3f
	POSTCODE2(0x6431)
	CCALL1(panic_idt64, %r15)
	hlt
#else
	je	4f
#endif
3:
	testb	$3,%al			                
	jnz	ast_from_interrupt_user	                              
	  
                                                 
                                            
                                 
    
	movl	%gs:CPU_PENDING_AST,%eax	
	testl	$(AST_URGENT),%eax		                          
	je	ret_to_kernel			                       

	cmpl	$0,%gs:CPU_PREEMPTION_LEVEL	                          
	jne	ret_to_kernel			                  

	  
                                                                  
                                                                
          
    
	CCALL(ast_taken_kernel)

	mov	%rsp, %r15		                                    
	jmp	ret_to_kernel
4:
	movl	R32_CS(%r15),%eax	                      
	jmp	3b


  
                                                         
   
int_from_intstack:
	incl	%gs:CPU_PREEMPTION_LEVEL
	incl	%gs:CPU_INTERRUPT_LEVEL
	incl	%gs:CPU_NESTED_ISTACK

	push	%gs:CPU_INT_STATE
	mov	%r15, %gs:CPU_INT_STATE

	CCALL1(interrupt, %r15)

	pop	%gs:CPU_INT_STATE

	decl	%gs:CPU_INTERRUPT_LEVEL
	decl	%gs:CPU_PREEMPTION_LEVEL
	decl	%gs:CPU_NESTED_ISTACK

	jmp	ret_to_kernel

  
                                       
   
ast_from_interrupt_user:
	movl	%gs:CPU_PENDING_AST,%eax
	testl	%eax,%eax		                   
	je	EXT(ret_to_user)	                       

	TIME_TRAP_UENTRY

	movl	$1, %ecx		                               
	jmp	L_return_from_trap_with_ast	            


                                

  
  
              
                                                 
  
                           
                    
  
                                       
                      
                         
   

Entry(hndl_sysenter)
	  
                                                               
                                         
    
	movl	R32_EAX(%r15),%eax
	testl	%eax,%eax
	js	EXT(hndl_mach_scall)		                 
						                 
	
Entry(hndl_unix_scall)

        TIME_TRAP_UENTRY

	movq	%gs:CPU_ACTIVE_THREAD,%rcx	                            
	movq	TH_TASK(%rcx),%rbx		                            
	incl	TH_SYSCALLS_UNIX(%rcx)		                            

	                                                  
	TASK_VTIMER_CHECK(%rbx,%rcx)

	sti

	CCALL1(unix_syscall, %r15)
	  
                                                  
    


Entry(hndl_mach_scall)
	TIME_TRAP_UENTRY

	movq	%gs:CPU_ACTIVE_THREAD,%rcx	                            
	movq	TH_TASK(%rcx),%rbx		                            
	incl	TH_SYSCALLS_MACH(%rcx)		                            

	                                                  
	TASK_VTIMER_CHECK(%rbx,%rcx)

	sti

	CCALL1(mach_call_munger, %r15)
	  
                                                  
    


Entry(hndl_mdep_scall)
	TIME_TRAP_UENTRY

	                                                  
	movq	%gs:CPU_ACTIVE_THREAD,%rcx	                            
	movq	TH_TASK(%rcx),%rbx		                            
	TASK_VTIMER_CHECK(%rbx,%rcx)

	sti

	CCALL1(machdep_syscall, %r15)
	  
                                                  
    

  
              
                                        
  
                           
                    
  
                                       
                      
                         
   

Entry(hndl_syscall)
	TIME_TRAP_UENTRY

	movq	%gs:CPU_ACTIVE_THREAD,%rcx	                            
	movl	$-1, TH_IOTIER_OVERRIDE(%rcx)	                                                          
	movq	TH_TASK(%rcx),%rbx		                            

	                                                  
	TASK_VTIMER_CHECK(%rbx,%rcx)

	  
                                                                   
                                      
    
	movl	R64_RAX(%r15), %eax		                          
	movl	%eax, %edx
	andl	$(SYSCALL_CLASS_MASK), %edx	                   
	cmpl	$(SYSCALL_CLASS_MACH<<SYSCALL_CLASS_SHIFT), %edx
	je	EXT(hndl_mach_scall64)
	cmpl	$(SYSCALL_CLASS_UNIX<<SYSCALL_CLASS_SHIFT), %edx
	je	EXT(hndl_unix_scall64)
	cmpl	$(SYSCALL_CLASS_MDEP<<SYSCALL_CLASS_SHIFT), %edx
	je	EXT(hndl_mdep_scall64)
	cmpl	$(SYSCALL_CLASS_DIAG<<SYSCALL_CLASS_SHIFT), %edx
	je	EXT(hndl_diag_scall64)

	                           
	sti
	CCALL3(i386_exception, $(EXC_SYSCALL), %rax, $1)
	               


Entry(hndl_unix_scall64)
	incl	TH_SYSCALLS_UNIX(%rcx)		                            
	sti

	CCALL1(unix_syscall64, %r15)
	  
                                                  
    


Entry(hndl_mach_scall64)
	incl	TH_SYSCALLS_MACH(%rcx)		                            
	sti

	CCALL1(mach_call_munger64, %r15)
	  
                                                  
    



Entry(hndl_mdep_scall64)
	sti

	CCALL1(machdep_syscall64, %r15)
	  
                                                  
    

Entry(hndl_diag_scall64)
	CCALL1(diagCall64, %r15)	                   
	test	%eax, %eax		                               
	je	1f			                         
	jmp	EXT(return_to_user)	                                      
1:
	sti
	CCALL3(i386_exception, $EXC_SYSCALL, $0x6000, $1)
	               
                                                                                                       
Entry(hndl_machine_check)
	                                                                      
	CCALL1(panic_machine_check64, %r15)
	hlt

Entry(hndl_double_fault)
	CCALL1(panic_double_fault64, %r15)
	hlt
