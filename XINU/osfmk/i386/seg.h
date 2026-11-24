/*
 * Copyright (c) 2000-2012 Apple Inc. All rights reserved.
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
#ifndef _I386_SEG_H_
#define _I386_SEG_H_
#ifndef __ASSEMBLER__
#include <stdint.h>
#include <mach/vm_types.h>
#include <mach/vm_param.h>
#include <architecture/i386/sel.h>

  
                     
   

static inline uint16_t
sel_to_selector(sel_t   sel)
{
	union {
		sel_t           sel;
		uint16_t        selector;
	} tconv;

	tconv.sel = sel;

	return tconv.selector;
}

static inline sel_t
selector_to_sel(uint16_t selector)
{
	union {
		uint16_t        selector;
		sel_t           sel;
	} tconv;

	tconv.selector = selector;

	return tconv.sel;
}

#define LDTSZ_MAX       8192                                                   
#define LDTSZ_DFL       (128)
#define LDTSZ           (LDTSZ_MAX)
#define LDTSZ_MIN       SEL_TO_INDEX(USER_SETTABLE)
                        

#define GDTSZ           19

  
                                              
   
#define IDTSZ           256

#include <sys/cdefs.h>

  
                           
   
struct real_descriptor {
	uint32_t        limit_low:16,                    
	    base_low:16,                                 
	    base_med:8,                                   
	    access:8,                                    
	    limit_high:4,                                 
	    granularity:4,                               
	    base_high:8;                                 
};
struct real_descriptor64 {
	uint32_t        limit_low16:16,                  
	    base_low16:16,                               
	    base_med8:8,                                  
	    access8:8,                                   
	    limit_high4:4,                                
	    granularity4:4,                              
	    base_high8:8,                                
	    base_top32:32,                               
	    reserved32:32;                                 
};
struct real_gate {
	uint32_t        offset_low:16,                    
	    selector:16,
	    word_count:8,
	    access:8,
	    offset_high:16;                                
};
struct real_gate64 {
	uint32_t        offset_low16:16,                          
	    selector16:16,
	    IST:3,
	    zeroes5:5,
	    access8:8,
	    offset_high16:16,                                      
	    offset_top32:32,                                       
	    reserved32:32;                                         
};

#define MAKE_REAL_DESCRIPTOR(base, lim, gran, acc) {\
	.limit_low = lim & 0xffff, \
	.limit_high = (lim >> 16) & 0xf, \
	.base_low = base & 0xffff, \
	.base_med = (base >> 16) & 0xff, \
	.base_high = (base >> 24) & 0xff, \
	.access = acc, \
	.granularity = gran \
}

  
                                                               
                                                              
              
   
struct fake_descriptor {
	uint32_t        offset:32;                          
	uint32_t        lim_or_seg:20;                     
	                                                                  
	uint32_t        size_or_wdct:4;                               
	                                                                  
	uint32_t        access:8;                           
};
struct fake_descriptor64 {
	uint64_t        offset64;                                          
	uint32_t        lim_or_seg:20;                     
	                                                                  
	uint32_t        size_or_IST:4;                                
	                                                           
	uint32_t        access:8;                           
	uint32_t        reserved:32;                               
};

typedef struct __attribute__((packed)) {
	uint16_t        size;
	void            *ptr;
} x86_64_desc_register_t;



  
                                          
   
extern struct real_descriptor   master_gdt[GDTSZ];
extern struct real_descriptor   master_ldt[];
extern struct i386_tss          master_ktss;
extern struct sysenter_stack    master_sstk;

extern struct fake_descriptor64 master_idt64[IDTSZ];
extern struct x86_64_tss        master_ktss64;

__BEGIN_DECLS

extern char                     df_task_stack[];
extern char                     df_task_stack_end[];
extern struct i386_tss          master_dftss;
extern void                     df_task_start(void);

extern char                     mc_task_stack[];
extern char                     mc_task_stack_end[];
extern struct i386_tss          master_mctss;
extern void                     mc_task_start(void);

__END_DECLS

#endif                   

#define SZ_64           0x2                                         
#define SZ_32           0x4                                         
#define SZ_G            0x8                                         

#define ACC_A           0x01                                  
#define ACC_TYPE        0x1e                                     

#define ACC_TYPE_SYSTEM 0x00                                             

#define ACC_LDT         0x02                                 
#define ACC_CALL_GATE_16 0x04                                             
#define ACC_TASK_GATE   0x05                                       
#define ACC_TSS         0x09                                          
#define ACC_CALL_GATE   0x0c                                       
#define ACC_INTR_GATE   0x0e                                            
#define ACC_TRAP_GATE   0x0f                                       

#define ACC_TSS_BUSY    0x02                                       

#define ACC_TYPE_USER   0x10                                          

#define ACC_DATA        0x10                                  
#define ACC_DATA_W      0x12                                            
#define ACC_DATA_E      0x14                                               
#define ACC_DATA_EW     0x16                                             
                                                                  
#define ACC_CODE        0x18                                  
#define ACC_CODE_R      0x1a                                            
#define ACC_CODE_C      0x1c                                              
#define ACC_CODE_CR     0x1e                                            
                                                            
#define ACC_PL          0x60                                        
#define ACC_PL_K        0x00                                            
#define ACC_PL_U        0x60                                     
#define ACC_P           0x80                                         

  
                           
   
#define SEL_LDTS        0x04                                        
#define SEL_PL          0x03                                          
#define SEL_PL_K        0x00                                             
#define SEL_PL_U        0x03                                           

  
                                              
   
#define sel_idx(sel)    (selector_to_sel(sel).index)
#define SEL_TO_INDEX(s) ((s)>>3)

#define NULL_SEG        0


  
                                                           
   
#define KERNEL64_CS     0x08                              
#define SYSENTER_CS     0x0b                                                 
#define KERNEL64_SS     0x10                                               
#define USER_CS         0x1b                              
#define USER_DS         0x23                                          
#define USER64_CS       0x2b                                           
#define USER64_DS       USER_DS                                          
#define KERNEL_LDT      0x30                     
                                                                             
#define KERNEL_TSS      0x40                     
                                                                             
#define KERNEL32_CS     0x50                     
#define USER_LDT        0x58                     
                                                                           
#define KERNEL_DS       0x68                                        


#define SYSENTER_TF_CS  (USER_CS|0x10000)
#define SYSENTER_DS     KERNEL64_SS                                       

#endif                    
#ifdef __x86_64__
  
                                
   
#define SYSCALL_CS      0x07                                        
#define USER_CTHREAD    0x0f                                   
#define USER_SETTABLE   0x1f                                                    
#endif
