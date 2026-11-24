#include <machine/asm.h>

	.text

	.balign 0x1000

	.global _hvtest_begin
_hvtest_begin:

	  
                                                                       
                                                               
                               
    

.code64

	.balign 16

	.global _save_restore_regs_entry
_save_restore_regs_entry:

    pushq %rax
    pushq %rcx

    xor %rcx, %rcx

    pushq %rbx


      
                                                       
                                                                 
                                                                   
                                                                                         
      
                                                                                 
                  
       


                                 

    incq %rcx
    movq $0x1010, %rax
    movq %ds, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $1, %rbx
    movq %rbx, %ds

    incq %rcx
    movq $0x2020, %rax
    movq %es, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $2, %rbx
    movq %rbx, %es

    incq %rcx
    movq $0x3030, %rax
    movq %fs, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $3, %rbx
    movq %rbx, %fs

    incq %rcx
    movq $0x4040, %rax
    movq %gs, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $1, %rbx
    movq %rbx, %gs

    popq %rbx

    jmp .pass

.pass2:
    pushq %rax
    pushq %rcx

    xor %rcx, %rcx

    pushq %rbx

                                 

    incq %rcx
    movq $0x1, %rax
    movq %ds, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $1, %rbx
    movq %rbx, %ds

    incq %rcx
    movq $0x2, %rax
    movq %es, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $2, %rbx
    movq %rbx, %es

    incq %rcx
    movq $0x3, %rax
    movq %fs, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $3, %rbx
    movq %rbx, %fs

    incq %rcx
    movq $0x1, %rax
    movq %gs, %rbx
    cmpq %rbx, %rax
    jne .foul
    movq $1, %rbx
    movq %rbx, %gs

    popq %rbx

.pass:
                                

    incq %rcx
    movq $0x0101010101010101, %rax
    cmpq 8(%rsp), %rax                 
    jne .foul
    notq 8(%rsp)

    incq %rcx
    movq $0x0202020202020202, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx

    incq %rcx
    movq $0x0303030303030303, %rax
    cmpq (%rsp), %rax                 
    jne .foul
    notq (%rsp)

    incq %rcx
    movq $0x0404040404040404, %rax
    cmpq %rdx, %rax
    jne .foul
    notq %rdx

    incq %rcx
    movq $0x0505050505050505, %rax
    cmpq %rsi, %rax
    jne .foul
    notq %rsi

    incq %rcx
    movq $0x0606060606060606, %rax
    cmpq %rdi, %rax
    jne .foul
    notq %rdi

    incq %rcx
    movq $0x0707070707070707, %rax
    cmpq %rbp, %rax
    jne .foul
    notq %rbp

    incq %rcx
    movq $0x0808080808080808, %rax
    cmpq %r8, %rax
    jne .foul
    notq %r8

    incq %rcx
    movq $0x0909090909090909, %rax
    cmpq %r9, %rax
    jne .foul
    notq %r9

    incq %rcx
    movq $0x0a0a0a0a0a0a0a0a, %rax
    cmpq %r10, %rax
    jne .foul
    notq %r10

    incq %rcx
    movq $0x0b0b0b0b0b0b0b0b, %rax
    cmpq %r11, %rax
    jne .foul
    notq %r11

    incq %rcx
    movq $0x0c0c0c0c0c0c0c0c, %rax
    cmpq %r12, %rax
    jne .foul
    notq %r12

    incq %rcx
    movq $0x0d0d0d0d0d0d0d0d, %rax
    cmpq %r13, %rax
    jne .foul
    notq %r13

    incq %rcx
    movq $0x0e0e0e0e0e0e0e0e, %rax
    cmpq %r14, %rax
    jne .foul
    notq %r14

    incq %rcx
    movq $0x0f0f0f0f0f0f0f0f, %rax
    cmpq %r15, %rax
    jne .foul
    notq %r15

    popq %rcx
    movq (%rsp), %rax
    vmcall

    notq %rax
    notq %rbx
    notq %rcx
    notq %rdx
    notq %rsi
    notq %rdi
    notq %rbp
    notq %r8
    notq %r9
    notq %r10
    notq %r11
    notq %r12
    notq %r13
    notq %r14
    notq %r15

    jmp .pass2

.foul:
    movq %rcx, %rax
    vmcall

	.global _save_restore_debug_regs_entry
_save_restore_debug_regs_entry:

    pushq %rax
    xor %rcx, %rcx

      
                                                       
                                                                 
                                                                   
                                                                                         
      
                                                                                 
                  
       

    incq %rcx
    movq $0x1111111111111111, %rbx
    movq %dr0, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    movq %rbx, %dr0

    incq %rcx
    movq $0x2222222222222222, %rbx
    movq %dr1, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    movq %rbx, %dr1

    incq %rcx
    movq $0x3333333333333333, %rbx
    movq %dr2, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    movq %rbx, %dr2

    incq %rcx
    movq $0x4444444444444444, %rbx
    movq %dr3, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    movq %rbx, %dr3

      
                                                                    
                                                                     
                                                         
       

    incq %rcx
    movq $0x5555555555555555, %rbx
    mov $0xffff0ff0, %rax
    orq %rax, %rbx
    movq $0xffffefff, %rax
    andq %rax, %rbx
    movq %dr6, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    mov $0xffff0ff0, %rax
    orq %rax, %rbx
    movq $0xffffefff, %rax
    andq %rax, %rbx
    movq %rbx, %dr6

    incq %rcx
    movq $0x5555555555555555, %rbx
    orq $0x400, %rbx
    movq $0xffff0fff, %rax
    andq %rax, %rbx
    movq %dr7, %rax
    cmpq %rbx, %rax
    jne .foul
    notq %rbx
    orq $0x400, %rbx
    movq $0xffff0fff, %rax
    andq %rax, %rbx
    movq %rbx, %dr7

    popq %rax
    vmcall

    movq %dr0, %rbx
    notq %rbx
    movq %rbx, %dr0

    movq %dr1, %rbx
    notq %rbx
    movq %rbx, %dr1

    movq %dr2, %rbx
    notq %rbx
    movq %rbx, %dr2

    movq %dr3, %rbx
    notq %rbx
    movq %rbx, %dr3

    movq %dr6, %rbx
    notq %rbx
    mov $0xffff0ff0, %rax
    orq %rax, %rbx
    movq $0xffffefff, %rax
    andq %rax, %rbx
    movq %rbx, %dr6

    movq %dr7, %rbx
    notq %rbx
    orq $0x400, %rbx
    movq $0xffff0fff, %rax
    andq %rax, %rbx
    movq %rbx, %dr7

    jmp _save_restore_debug_regs_entry            

.code32

	.global _simple_protected_mode_vcpu_entry
_simple_protected_mode_vcpu_entry:

    movl $0x23456, %eax
    vmcall

.code16

	.global _simple_real_mode_vcpu_entry
_simple_real_mode_vcpu_entry:

    movl $0x23456, %eax
    vmcall

.code32

	.global _radar61961809_entry
_radar61961809_entry:

	mov		$0x99999999, %ebx	                                              

	mov		$0xc0000080,%ecx	            
	rdmsr
	or		$0x100,%eax			       
	wrmsr

	vmcall

	mov		%cr0,%ecx
	or		$0x80000000,%ecx	         
	mov		%ecx,%cr0

	                                                          
	ljmp	*(%edi)

.code32

	.global _radar61961809_prepare
_radar61961809_prepare:

	  
                                                                 
                        
   
                                                                   
                                                                
                                                                
                                                                 
                                                           
                                                                
                                                               
                            
   
                                                                
                                                                  
                                                              
                                                      
    

	mov		%cr0,%ecx
	and		$~0x80000000,%ecx
	mov		%ecx,%cr0

	mov		$0x1111, %eax
	vmcall

	                                               
	mov		%cr0,%ecx
	or		$0x80000000,%ecx
	mov		%ecx,%cr0	                         

	xor		%ecx, %ecx

	add		$8,%edi
	ljmp	*(%edi)		                        

.code64

	.global _radar61961809_loop64
_radar61961809_loop64:
1:
	                                           
	                      
	                                                     
	mov		$0x1,%ebp

	                                                    
	dec		%ecx
	jnz		1b

	                                            
	mov		$0x2222, %eax
	vmcall

	.global _radar60691363_entry
_radar60691363_entry:
	movq $0x800, %rsi                 
	vmreadq %rsi, %rax
	vmcall
	movq $0x6400, %rsi                         
	vmreadq %rsi, %rax
	vmcall
	movq $0x6402, %rsi                  
	vmreadq %rsi, %rax
	vmcall

	movq $0x800, %rsi                 
	movq $0x9191, %rax
	vmwriteq %rax, %rsi
	movq $0x6400, %rsi                         
	movq $0x9898, %rax
	vmwriteq %rax, %rsi
	movq $0x6402, %rsi                  
	movq $0x7979, %rax
	vmwriteq %rax, %rsi

	movq $0x4567, %rax

	vmcall

.code16

	                                                              
	.global _pio_entry
_pio_entry:

	movl	$0xaa, %eax

	outl	%eax, $0xab

	movl	$3, %ecx
1:	outb	%al, $0xab
	loop	1b

	movl	$10, %ecx
1:	outb	%al, $0xcd
	loop	1b

	movl	$10, %ecx
1:	outb	%al, $0xef
	loop	1b

	movl	$0x23456, %eax
	vmcall

.code16
	                                
	.global _pio_entry_basic
_pio_entry_basic:

	movl	$10, %ecx
1:	outb	%al, $0xef
	loop	1b

	movl	$0x23456, %eax
	vmcall

	.global _hvtest_end
_hvtest_end:
