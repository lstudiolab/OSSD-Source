/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   

#include <debug.h>

#include <i386/asm.h>
#include <i386/proc_reg.h>
#include <i386/postcode.h>
#include <assym.s>

#include <i386/cpuid.h>
#include <i386/acpi.h>

.code32


  
                                                    
                                                                            
   

                                                                            
	.section __HIB, __data
	.align	12

	.globl	EXT(low_intstack)
EXT(low_intstack):
	.globl  EXT(gIOHibernateRestoreStack)
EXT(gIOHibernateRestoreStack):

	.space	INTSTACK_SIZE

	.globl	EXT(low_eintstack)
EXT(low_eintstack:)
	.globl  EXT(gIOHibernateRestoreStackEnd)
EXT(gIOHibernateRestoreStackEnd):

	                                         

	.section __DATA, __data

  
                                   
   
	.align	12
	.globl	EXT(mc_task_stack)
EXT(mc_task_stack):
	.space	INTSTACK_SIZE
	.globl	EXT(mc_task_stack_end)
EXT(mc_task_stack_end):

	                          
#define SWITCH_TO_64BIT_MODE					 \
	movl	$(CR4_PAE),%eax		                	  
	movl	%eax,%cr4					  
	movl    $MSR_IA32_EFER,%ecx				  
	rdmsr							  
	                          				  
	orl	$(MSR_IA32_EFER_LME | MSR_IA32_EFER_NXE),%eax	  
	wrmsr							  
	movl	$EXT(BootPML4),%eax				  
	movl	%eax,%cr3					  
	movl	%cr0,%eax					  
	orl	$(CR0_PG|CR0_WP),%eax	                   	  
	movl	%eax,%cr0					  
	ljmpl	$KERNEL64_CS,$64f				  
64:								  
	.code64

  
                      
                               
  
               
                                                        
                                                         
   
	
.code32
	.text
	.section __HIB, __text
	.align	ALIGN
	.globl	EXT(_start)
	.globl	EXT(pstart)
LEXT(_start)
LEXT(pstart)

  
                                                                               
  
                         
  
                            
        
                       
        
                                                                
               
                            
                     
                                         
        
               
        
                              
  
   	
	mov	%eax, %edi	                         

	                                                
	movl	$EXT(low_eintstack), %esp
	
	POSTCODE(PSTART_ENTRY)

	  
                       
    
	movl	$EXT(protected_mode_gdtr), %eax
	lgdtl	(%eax)

	  
                                                   
    
	movl	$EXT(BootPML4), %eax			           
	add	%eax, 0*8+0(%eax)			         
	add	%eax, KERNEL_PML4_INDEX*8+0(%eax)	                  

	movl	$EXT(BootPDPT), %edx			           
	add	%eax, 0*8+0(%edx)
	add	%eax, 1*8+0(%edx)
	add	%eax, 2*8+0(%edx)
	add	%eax, 3*8+0(%edx)

	POSTCODE(PSTART_REBASE)

                                                                       
L_pstart_common:
	  
                         
    
	SWITCH_TO_64BIT_MODE

	                                  
	xor	%eax, %eax
	mov	%ax, %ss
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs

	test	%edi, %edi                                   
	jz	Lvstartshim

	mov	$1, %eax
	cpuid
	test	$(1 << 30), %ecx
	jz	Lnon_rdrand
	rdrand	%rax		                                    
	jnc	Lnon_rdrand	                                                

Lstore_random_guard:
	xor	%ah, %ah	                                                
	movq	%rax, ___stack_chk_guard(%rip)
	                                   
Lvstartshim:	

	POSTCODE(PSTART_VSTART)

	                            
	
	leaq	_vstart(%rip), %rcx
	movq	$(KERNEL_BASE), %rax		                            
	or	%rax, %rsp			                                
	or	%rcx, %rax
	andq	$0xfffffffffffffff0, %rsp	                 
	xorq	%rbp, %rbp			                        
	callq	*%rax

Lnon_rdrand:
	rdtsc                     
	                               
	mov	%eax, %ecx
	xor	%al, %ah
	shl	$16, %rcx
	xor	%rcx, %rax
	xor	%eax, %edx

	                                      
	lea	(%rip), %rcx
	shr	$21, %rcx
	movzbl	%cl, %ecx
	shl	$16, %ecx
	xor	%ecx, %edx

	mov	%ah, %cl
	ror	%cl, %edx                                                       
	shl	$32, %rdx
	xor	%rdx, %rax
	mov	%cl, %al
	jmp	Lstore_random_guard
  
                              
  
               
                                                        
                                                         
   
	.align	ALIGN
	.globl	EXT(slave_pstart)
LEXT(slave_pstart)
	.code32
	cli				                                     
					                          
	POSTCODE(SLAVE_PSTART)

	movl	$EXT(mp_slave_stack) + PAGE_SIZE, %esp

	xor 	%edi, %edi		                             

	jmp	L_pstart_common		                            


                          

.section __HIB, __text
  
                                                                       
                                                                            
                                                                          
                                                                      
                                                                        
                                      
   

	.align	ALIGN
	.globl	EXT(hibernate_machine_entrypoint)
.code32
LEXT(hibernate_machine_entrypoint)
	movl    %eax, %edi                                    

	                                                
	movl $EXT(low_eintstack), %esp
	
	  
              
    
	movl	$EXT(master_gdtr), %eax
	lgdtl	(%eax)

	                                      
	SWITCH_TO_64BIT_MODE

	leaq	EXT(hibernate_kernel_entrypoint)(%rip),%rcx

	                                       
	movq	$(KERNEL_BASE), %rax
	orq	%rax, %rsp
	orq	%rcx, %rax

	                                                
	xorl	%esi, %esi			                  
	xorl	%edx, %edx			                  
	xorl	%ecx, %ecx			                  
	andq	$0xfffffffffffffff0, %rsp	                 

	                                                              
	xorq	%rbp, %rbp			                        
	call	*%rax

	                
	hlt

                        

#if CONFIG_SLEEP
                            

#include <i386/acpi.h>


  
                  
   

.section __TEXT,__text
.code64

  
                                                          
  
                                                          
                     
   

ENTRY(acpi_sleep_cpu)
	push	%rbp
	mov	%rsp, %rbp

	                
	pushf

	                                    
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rbp
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15

	mov	%rsp, saved_rsp(%rip)

	                              
	mov	%cr3,%rax
	mov	%rax,%cr3
	
	                            
	mov	%cr0, %rax
	mov	%rax, saved_cr0(%rip)
	mov	%cr2, %rax
	mov	%rax, saved_cr2(%rip)
	mov	%cr3, %rax
	mov	%rax, saved_cr3(%rip)
	mov	%cr4, %rax
	mov	%rax, saved_cr4(%rip)

	                            
	movw	%es, saved_es(%rip)
	movw	%fs, saved_fs(%rip)
	movw	%gs, saved_gs(%rip)
	movw	%ss, saved_ss(%rip)	

	                                            
	                                                        
	mov	$MSR_IA32_KERNEL_GS_BASE, %rcx
	rdmsr
	movl	%eax, saved_ugs_base(%rip)
	movl	%edx, saved_ugs_base+4(%rip)
	swapgs
	rdmsr
	movl	%eax, saved_kgs_base(%rip)
	movl	%edx, saved_kgs_base+4(%rip)
	swapgs

	                                     
	sgdt	saved_gdt(%rip)
	sldt	saved_ldt(%rip)
	sidt	saved_idt(%rip)
	str	saved_tr(%rip)

	  
                                                                    
                                         
    

	xchgq %rdi, %rsi
	call	*%rsi

	                                       
	jmp	wake_restore

.section __HIB, __text
.code32
.globl EXT(acpi_wake_prot)
EXT(acpi_wake_prot):
	                                     
	movl	$EXT(low_eintstack), %esp

	SWITCH_TO_64BIT_MODE

	jmp	Lwake_64

.section __TEXT,__text
.code64

.globl EXT(acpi_wake_prot_entry)
EXT(acpi_wake_prot_entry):
	POSTCODE(ACPI_WAKE_PROT_ENTRY)
	                                                                        
    
Lwake_64:
	  
                                                         
    
	mov	saved_cr4(%rip), %rcx
	mov	%rcx, %cr4

	mov	$(MSR_IA32_EFER), %ecx		                       
	rdmsr					                          
	or	$(MSR_IA32_EFER_NXE), %eax	                                
	wrmsr					            

	movq	saved_cr2(%rip), %rax
	mov	%rax, %cr2

	                                 
	mov	saved_cr0(%rip), %rax
	mov	%rax, %cr0

	                             
	mov	saved_cr3(%rip), %rax
	mov	%rax, %cr3

	                                    
	POSTCODE(ACPI_WAKE_PAGED_ENTRY)

	                                 
	xor	%eax, %eax
	movw	%ax, %ss
	movw	%ax, %ds

	                               
	lgdt	saved_gdt(%rip)
	lldt	saved_ldt(%rip)
	lidt	saved_idt(%rip)

	                               
	movw	saved_es(%rip), %es
	movw	saved_fs(%rip), %fs
	movw	saved_gs(%rip), %gs
	movw	saved_ss(%rip), %ss

	                                               
	mov	$MSR_IA32_KERNEL_GS_BASE, %rcx
	movl	saved_kgs_base(%rip),   %eax 
	movl	saved_kgs_base+4(%rip), %edx 
	wrmsr
	swapgs
	movl	saved_ugs_base(%rip),   %eax 
	movl	saved_ugs_base+4(%rip), %edx 
	wrmsr

	  
                                                                 
                                         
    
	lea	saved_gdt(%rip), %rax
	movq	2(%rax), %rdx			                               
	movl	$(KERNEL_TSS), %eax		                          
	movb	$(K_TSS), 5(%rdx, %rax)		                     

	ltr	saved_tr(%rip)			                

wake_restore:
	mov	saved_rsp(%rip), %rsp 

	                                       
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rbp
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax

	                   
	popf

	leave
	ret

                          
#endif                   

                                                  

#define	operand_size_prefix	.byte 0x66
#define	address_size_prefix	.byte 0x67
#define	cs_base_prefix		.byte 0x2e

#define	LJMP(segment,address)			\
	operand_size_prefix			  
	.byte	0xea				  
	.long	address-EXT(real_mode_bootstrap_base)	  
	.word	segment

#define	LGDT(address)				\
	cs_base_prefix				  
	address_size_prefix			  
	operand_size_prefix			  
	.word	0x010f				  
	.byte	0x15				  
	.long	address-EXT(real_mode_bootstrap_base)

.section __HIB, __text
.align	12	                                        
.code32
Entry(real_mode_bootstrap_base)
	cli

	LGDT(EXT(protected_mode_gdtr))

	                           
	mov	%cr0, %eax
	inc %eax
	mov	%eax, %cr0 

	                        
	LJMP(KERNEL32_CS, 1f + REAL_MODE_BOOTSTRAP_OFFSET)
1:
	
	                                  
	                                  
	mov	$KERNEL_DS, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	xor	%eax,%eax
	movw	%ax, %fs
	movw	%ax, %gs

	POSTCODE(SLAVE_STARTPROG_ENTRY) 

	mov	PROT_MODE_START+REAL_MODE_BOOTSTRAP_OFFSET, %ecx
	jmp 	*%ecx

Entry(protected_mode_gdtr)
	.short	160		                       
	.quad	EXT(master_gdt)

Entry(real_mode_bootstrap_end)

                                      
.section __HIB, __data
.align	2

                                                                          
Entry(master_gdtr)			
		.word 160		                       
		.quad EXT(master_gdt)

saved_gdt:	.word 0
		.quad 0
saved_rsp:	.quad 0
saved_es:	.word 0
saved_fs:	.word 0
saved_gs:	.word 0
saved_ss:	.word 0
saved_cr0:	.quad 0
saved_cr2:	.quad 0
saved_cr3:	.quad 0
saved_cr4:	.quad 0
saved_idt:	.word 0
		.quad 0
saved_ldt:	.word 0
saved_tr:	.word 0
saved_kgs_base:	.quad 0
saved_ugs_base:	.quad 0

