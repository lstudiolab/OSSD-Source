/*
 * Copyright (c) 2004-2006 Apple Computer, Inc. All rights reserved.
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

#ifndef	_MACH_I386__STRUCTS_H_
#define	_MACH_I386__STRUCTS_H_

#include <sys/cdefs.h>                      
#include <machine/types.h>                

  
                                                              
                                                                   
  
   

#if __DARWIN_UNIX03
#define	_STRUCT_X86_THREAD_STATE32	struct __darwin_i386_thread_state
_STRUCT_X86_THREAD_STATE32
{
    unsigned int	__eax;
    unsigned int	__ebx;
    unsigned int	__ecx;
    unsigned int	__edx;
    unsigned int	__edi;
    unsigned int	__esi;
    unsigned int	__ebp;
    unsigned int	__esp;
    unsigned int	__ss;
    unsigned int	__eflags;
    unsigned int	__eip;
    unsigned int	__cs;
    unsigned int	__ds;
    unsigned int	__es;
    unsigned int	__fs;
    unsigned int	__gs;
};
#else                       
#define	_STRUCT_X86_THREAD_STATE32	struct i386_thread_state
_STRUCT_X86_THREAD_STATE32
{
    unsigned int	eax;
    unsigned int	ebx;
    unsigned int	ecx;
    unsigned int	edx;
    unsigned int	edi;
    unsigned int	esi;
    unsigned int	ebp;
    unsigned int	esp;
    unsigned int	ss;
    unsigned int	eflags;
    unsigned int	eip;
    unsigned int	cs;
    unsigned int	ds;
    unsigned int	es;
    unsigned int	fs;
    unsigned int	gs;
};
#endif                       

                                                                  

#if __DARWIN_UNIX03
#define _STRUCT_FP_CONTROL	struct __darwin_fp_control
_STRUCT_FP_CONTROL
{
    unsigned short		__invalid	:1,
    				__denorm	:1,
				__zdiv		:1,
				__ovrfl		:1,
				__undfl		:1,
				__precis	:1,
						:2,
				__pc		:2,
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FP_PREC_24B		0
#define	FP_PREC_53B		2
#define FP_PREC_64B		3
#endif                                           
				__rc		:2,
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FP_RND_NEAR		0
#define FP_RND_DOWN		1
#define FP_RND_UP		2
#define FP_CHOP			3
#endif                                           
					       	:1,
						:3;
};
typedef _STRUCT_FP_CONTROL	__darwin_fp_control_t;
#else                       
#define _STRUCT_FP_CONTROL	struct fp_control
_STRUCT_FP_CONTROL
{
    unsigned short		invalid	:1,
    				denorm	:1,
				zdiv	:1,
				ovrfl	:1,
				undfl	:1,
				precis	:1,
					:2,
				pc	:2,
#define FP_PREC_24B		0
#define	FP_PREC_53B		2
#define FP_PREC_64B		3
				rc	:2,
#define FP_RND_NEAR		0
#define FP_RND_DOWN		1
#define FP_RND_UP		2
#define FP_CHOP			3
				       	:1,
					:3;
};
typedef _STRUCT_FP_CONTROL	fp_control_t;
#endif                       

  
               
   

#if __DARWIN_UNIX03
#define _STRUCT_FP_STATUS	struct __darwin_fp_status
_STRUCT_FP_STATUS
{
    unsigned short		__invalid	:1,
    				__denorm	:1,
				__zdiv		:1,
				__ovrfl		:1,
				__undfl		:1,
				__precis	:1,
				__stkflt	:1,
				__errsumm	:1,
				__c0		:1,
				__c1		:1,
				__c2		:1,
				__tos		:3,
				__c3		:1,
				__busy		:1;
};
typedef _STRUCT_FP_STATUS	__darwin_fp_status_t;
#else                       
#define _STRUCT_FP_STATUS	struct fp_status
_STRUCT_FP_STATUS
{
    unsigned short		invalid	:1,
    				denorm	:1,
				zdiv	:1,
				ovrfl	:1,
				undfl	:1,
				precis	:1,
				stkflt	:1,
				errsumm	:1,
				c0	:1,
				c1	:1,
				c2	:1,
				tos	:3,
				c3	:1,
				busy	:1;
};
typedef _STRUCT_FP_STATUS	fp_status_t;
#endif                       
				
                                            

#if __DARWIN_UNIX03
#define _STRUCT_MMST_REG	struct __darwin_mmst_reg
_STRUCT_MMST_REG
{
	char	__mmst_reg[10];
	char	__mmst_rsrv[6];
};
#else                       
#define _STRUCT_MMST_REG	struct mmst_reg
_STRUCT_MMST_REG
{
	char	mmst_reg[10];
	char	mmst_rsrv[6];
};
#endif                       


                              

#if __DARWIN_UNIX03
#define _STRUCT_XMM_REG		struct __darwin_xmm_reg
_STRUCT_XMM_REG
{
	char		__xmm_reg[16];
};
#else                       
#define _STRUCT_XMM_REG		struct xmm_reg
_STRUCT_XMM_REG
{
	char		xmm_reg[16];
};
#endif                       

                              

#if __DARWIN_UNIX03
#define _STRUCT_YMM_REG		struct __darwin_ymm_reg
_STRUCT_YMM_REG
{
	char		__ymm_reg[32];
};
#else                       
#define _STRUCT_YMM_REG		struct ymm_reg
_STRUCT_YMM_REG
{
	char		ymm_reg[32];
};
#endif                       

                              

#if __DARWIN_UNIX03
#define _STRUCT_ZMM_REG		struct __darwin_zmm_reg
_STRUCT_ZMM_REG
{
	char		__zmm_reg[64];
};
#else                       
#define _STRUCT_ZMM_REG		struct zmm_reg
_STRUCT_ZMM_REG
{
	char		zmm_reg[64];
};
#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_OPMASK_REG	struct __darwin_opmask_reg
_STRUCT_OPMASK_REG
{
	char		__opmask_reg[8];
};
#else                       
#define _STRUCT_OPMASK_REG	struct opmask_reg
_STRUCT_OPMASK_REG
{
	char		opmask_reg[8];
};
#endif                       

   
                        
   

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FP_STATE_BYTES		512	                                                
#endif                                           

#if __DARWIN_UNIX03
#define	_STRUCT_X86_FLOAT_STATE32	struct __darwin_i386_float_state
_STRUCT_X86_FLOAT_STATE32
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    
	__uint32_t		__fpu_ip;		                                        
	__uint16_t		__fpu_cs;		                                          
	__uint16_t		__fpu_rsrv2;		              
	__uint32_t		__fpu_dp;		                                                      
	__uint16_t		__fpu_ds;		                                                        
	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	char			__fpu_rsrv4[14*16];	              
	int 			__fpu_reserved1;
};

#define	_STRUCT_X86_AVX_STATE32	struct __darwin_i386_avx_state
_STRUCT_X86_AVX_STATE32
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    
	__uint32_t		__fpu_ip;		                                        
	__uint16_t		__fpu_cs;		                                          
	__uint16_t		__fpu_rsrv2;		              
	__uint32_t		__fpu_dp;		                                                      
	__uint16_t		__fpu_ds;		                                                        
	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	char			__fpu_rsrv4[14*16];	              
	int 			__fpu_reserved1;
	char			__avx_reserved1[64];
	_STRUCT_XMM_REG		__fpu_ymmh0;		             
	_STRUCT_XMM_REG		__fpu_ymmh1;		             
	_STRUCT_XMM_REG		__fpu_ymmh2;		             
	_STRUCT_XMM_REG		__fpu_ymmh3;		             
	_STRUCT_XMM_REG		__fpu_ymmh4;		             
	_STRUCT_XMM_REG		__fpu_ymmh5;		             
	_STRUCT_XMM_REG		__fpu_ymmh6;		             
	_STRUCT_XMM_REG		__fpu_ymmh7;		             
};

#define	_STRUCT_X86_AVX512_STATE32	struct __darwin_i386_avx512_state
_STRUCT_X86_AVX512_STATE32
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    
	__uint32_t		__fpu_ip;		                                        
	__uint16_t		__fpu_cs;		                                          
	__uint16_t		__fpu_rsrv2;		              
	__uint32_t		__fpu_dp;		                                                      
	__uint16_t		__fpu_ds;		                                                        
	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	char			__fpu_rsrv4[14*16];	              
	int 			__fpu_reserved1;
	char			__avx_reserved1[64];
	_STRUCT_XMM_REG		__fpu_ymmh0;		             
	_STRUCT_XMM_REG		__fpu_ymmh1;		             
	_STRUCT_XMM_REG		__fpu_ymmh2;		             
	_STRUCT_XMM_REG		__fpu_ymmh3;		             
	_STRUCT_XMM_REG		__fpu_ymmh4;		             
	_STRUCT_XMM_REG		__fpu_ymmh5;		             
	_STRUCT_XMM_REG		__fpu_ymmh6;		             
	_STRUCT_XMM_REG		__fpu_ymmh7;		             
	_STRUCT_OPMASK_REG	__fpu_k0;		        
	_STRUCT_OPMASK_REG	__fpu_k1;		        
	_STRUCT_OPMASK_REG	__fpu_k2;		        
	_STRUCT_OPMASK_REG	__fpu_k3;		        
	_STRUCT_OPMASK_REG	__fpu_k4;		        
	_STRUCT_OPMASK_REG	__fpu_k5;		        
	_STRUCT_OPMASK_REG	__fpu_k6;		        
	_STRUCT_OPMASK_REG	__fpu_k7;		        
	_STRUCT_YMM_REG		__fpu_zmmh0;		             
	_STRUCT_YMM_REG		__fpu_zmmh1;		             
	_STRUCT_YMM_REG		__fpu_zmmh2;		             
	_STRUCT_YMM_REG		__fpu_zmmh3;		             
	_STRUCT_YMM_REG		__fpu_zmmh4;		             
	_STRUCT_YMM_REG		__fpu_zmmh5;		             
	_STRUCT_YMM_REG		__fpu_zmmh6;		             
	_STRUCT_YMM_REG		__fpu_zmmh7;		             
};

#else                       
#define	_STRUCT_X86_FLOAT_STATE32	struct i386_float_state
_STRUCT_X86_FLOAT_STATE32
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    
	__uint32_t		fpu_ip;			                                        
	__uint16_t		fpu_cs;			                                          
	__uint16_t		fpu_rsrv2;		              
	__uint32_t		fpu_dp;			                                                      
	__uint16_t		fpu_ds;			                                                        
	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;		                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	char			fpu_rsrv4[14*16];	              
	int 			fpu_reserved1;
};

#define	_STRUCT_X86_AVX_STATE32	struct i386_avx_state
_STRUCT_X86_AVX_STATE32
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    
	__uint32_t		fpu_ip;			                                        
	__uint16_t		fpu_cs;			                                          
	__uint16_t		fpu_rsrv2;		              
	__uint32_t		fpu_dp;			                                                      
	__uint16_t		fpu_ds;			                                                        
	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;		                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	char			fpu_rsrv4[14*16];	              
	int 			fpu_reserved1;
	char			avx_reserved1[64];
	_STRUCT_XMM_REG		fpu_ymmh0;		             
	_STRUCT_XMM_REG		fpu_ymmh1;		             
	_STRUCT_XMM_REG		fpu_ymmh2;		             
	_STRUCT_XMM_REG		fpu_ymmh3;		             
	_STRUCT_XMM_REG		fpu_ymmh4;		             
	_STRUCT_XMM_REG		fpu_ymmh5;		             
	_STRUCT_XMM_REG		fpu_ymmh6;		             
	_STRUCT_XMM_REG		fpu_ymmh7;		             
};

#define	_STRUCT_X86_AVX512_STATE32	struct i386_avx512_state
_STRUCT_X86_AVX512_STATE32
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    
	__uint32_t		fpu_ip;			                                        
	__uint16_t		fpu_cs;			                                          
	__uint16_t		fpu_rsrv2;		              
	__uint32_t		fpu_dp;			                                                      
	__uint16_t		fpu_ds;			                                                        
	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;		                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	char			fpu_rsrv4[14*16];	              
	int 			fpu_reserved1;
	char			avx_reserved1[64];
	_STRUCT_XMM_REG		fpu_ymmh0;		             
	_STRUCT_XMM_REG		fpu_ymmh1;		             
	_STRUCT_XMM_REG		fpu_ymmh2;		             
	_STRUCT_XMM_REG		fpu_ymmh3;		             
	_STRUCT_XMM_REG		fpu_ymmh4;		             
	_STRUCT_XMM_REG		fpu_ymmh5;		             
	_STRUCT_XMM_REG		fpu_ymmh6;		             
	_STRUCT_XMM_REG		fpu_ymmh7;		             
	_STRUCT_OPMASK_REG	fpu_k0;			        
	_STRUCT_OPMASK_REG	fpu_k1;			        
	_STRUCT_OPMASK_REG	fpu_k2;			        
	_STRUCT_OPMASK_REG	fpu_k3;			        
	_STRUCT_OPMASK_REG	fpu_k4;			        
	_STRUCT_OPMASK_REG	fpu_k5;			        
	_STRUCT_OPMASK_REG	fpu_k6;			        
	_STRUCT_OPMASK_REG	fpu_k7;			        
	_STRUCT_YMM_REG		fpu_zmmh0;		             
	_STRUCT_YMM_REG		fpu_zmmh1;		             
	_STRUCT_YMM_REG		fpu_zmmh2;		             
	_STRUCT_YMM_REG		fpu_zmmh3;		             
	_STRUCT_YMM_REG		fpu_zmmh4;		             
	_STRUCT_YMM_REG		fpu_zmmh5;		             
	_STRUCT_YMM_REG		fpu_zmmh6;		             
	_STRUCT_YMM_REG		fpu_zmmh7;		             
};

#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_X86_EXCEPTION_STATE32	struct __darwin_i386_exception_state
_STRUCT_X86_EXCEPTION_STATE32
{
	__uint16_t	__trapno;
	__uint16_t	__cpu;
	__uint32_t	__err;
	__uint32_t	__faultvaddr;
};
#else                       
#define _STRUCT_X86_EXCEPTION_STATE32	struct i386_exception_state
_STRUCT_X86_EXCEPTION_STATE32
{
	__uint16_t	trapno;
	__uint16_t	cpu;
	__uint32_t	err;
	__uint32_t	faultvaddr;
};
#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_X86_DEBUG_STATE32	struct __darwin_x86_debug_state32
_STRUCT_X86_DEBUG_STATE32
{
	unsigned int	__dr0;
	unsigned int	__dr1;
	unsigned int	__dr2;
	unsigned int	__dr3;
	unsigned int	__dr4;
	unsigned int	__dr5;
	unsigned int	__dr6;
	unsigned int	__dr7;
};

#define _STRUCT_X86_INSTRUCTION_STATE	struct __x86_instruction_state
_STRUCT_X86_INSTRUCTION_STATE
{
        int		__insn_stream_valid_bytes;
        int		__insn_offset;
	int		__out_of_synch;	  
                                                                 
                                                             
                                                                    
        
#define _X86_INSTRUCTION_STATE_MAX_INSN_BYTES    (2448 - 64 - 4)
        __uint8_t	__insn_bytes[_X86_INSTRUCTION_STATE_MAX_INSN_BYTES];
#define _X86_INSTRUCTION_STATE_CACHELINE_SIZE	64
	__uint8_t	__insn_cacheline[_X86_INSTRUCTION_STATE_CACHELINE_SIZE];
};

#define _STRUCT_LAST_BRANCH_RECORD	struct __last_branch_record
_STRUCT_LAST_BRANCH_RECORD
{
	__uint64_t	__from_ip;
	__uint64_t	__to_ip;
	__uint32_t	__mispredict : 1,
			__tsx_abort  : 1,
			__in_tsx     : 1,
			__cycle_count: 16,
			__reserved   : 13;
};

#define _STRUCT_LAST_BRANCH_STATE	struct __last_branch_state
_STRUCT_LAST_BRANCH_STATE
{
        int				__lbr_count;
	__uint32_t			__lbr_supported_tsx : 1,
					__lbr_supported_cycle_count : 1,
					__reserved : 30;
#define	__LASTBRANCH_MAX	32
	_STRUCT_LAST_BRANCH_RECORD	__lbrs[__LASTBRANCH_MAX];
};

#else                       

#define _STRUCT_X86_DEBUG_STATE32	struct x86_debug_state32
_STRUCT_X86_DEBUG_STATE32
{
	unsigned int	dr0;
	unsigned int	dr1;
	unsigned int	dr2;
	unsigned int	dr3;
	unsigned int	dr4;
	unsigned int	dr5;
	unsigned int	dr6;
	unsigned int	dr7;
};

#define _STRUCT_X86_INSTRUCTION_STATE	struct __x86_instruction_state
_STRUCT_X86_INSTRUCTION_STATE
{
        int		insn_stream_valid_bytes;
        int		insn_offset;
	int		out_of_synch;	  
                                                                 
                                                             
                                                                    
        
#define x86_INSTRUCTION_STATE_MAX_INSN_BYTES    (2448 - 64 - 4)
        __uint8_t	insn_bytes[x86_INSTRUCTION_STATE_MAX_INSN_BYTES];
#define x86_INSTRUCTION_STATE_CACHELINE_SIZE	64
	__uint8_t	insn_cacheline[x86_INSTRUCTION_STATE_CACHELINE_SIZE];
};

#define _STRUCT_LAST_BRANCH_RECORD	struct __last_branch_record
_STRUCT_LAST_BRANCH_RECORD
{
	__uint64_t	from_ip;
	__uint64_t	to_ip;
	__uint32_t	mispredict : 1,
			tsx_abort  : 1,
			in_tsx     : 1,
			cycle_count: 16,
			reserved   : 13;
};

#define _STRUCT_LAST_BRANCH_STATE	struct __last_branch_state
_STRUCT_LAST_BRANCH_STATE
{
        int				lbr_count;
	__uint32_t			lbr_supported_tsx : 1,
					lbr_supported_cycle_count : 1,
					reserved : 30;
#define	__LASTBRANCH_MAX	32
	_STRUCT_LAST_BRANCH_RECORD	lbrs[__LASTBRANCH_MAX];
};
#endif                       

#define	_STRUCT_X86_PAGEIN_STATE	struct __x86_pagein_state
_STRUCT_X86_PAGEIN_STATE
{
	int __pagein_error;
};

  
                               
   

#if __DARWIN_UNIX03
#define	_STRUCT_X86_THREAD_STATE64	struct __darwin_x86_thread_state64
_STRUCT_X86_THREAD_STATE64
{
	__uint64_t	__rax;
	__uint64_t	__rbx;
	__uint64_t	__rcx;
	__uint64_t	__rdx;
	__uint64_t	__rdi;
	__uint64_t	__rsi;
	__uint64_t	__rbp;
	__uint64_t	__rsp;
	__uint64_t	__r8;
	__uint64_t	__r9;
	__uint64_t	__r10;
	__uint64_t	__r11;
	__uint64_t	__r12;
	__uint64_t	__r13;
	__uint64_t	__r14;
	__uint64_t	__r15;
	__uint64_t	__rip;
	__uint64_t	__rflags;
	__uint64_t	__cs;
	__uint64_t	__fs;
	__uint64_t	__gs;
};
#else                       
#define	_STRUCT_X86_THREAD_STATE64	struct x86_thread_state64
_STRUCT_X86_THREAD_STATE64
{
	__uint64_t	rax;
	__uint64_t	rbx;
	__uint64_t	rcx;
	__uint64_t	rdx;
	__uint64_t	rdi;
	__uint64_t	rsi;
	__uint64_t	rbp;
	__uint64_t	rsp;
	__uint64_t	r8;
	__uint64_t	r9;
	__uint64_t	r10;
	__uint64_t	r11;
	__uint64_t	r12;
	__uint64_t	r13;
	__uint64_t	r14;
	__uint64_t	r15;
	__uint64_t	rip;
	__uint64_t	rflags;
	__uint64_t	cs;
	__uint64_t	fs;
	__uint64_t	gs;
};
#endif                       

  
                                          
   

#if __DARWIN_UNIX03
#define	_STRUCT_X86_THREAD_FULL_STATE64	struct __darwin_x86_thread_full_state64
_STRUCT_X86_THREAD_FULL_STATE64
{
	_STRUCT_X86_THREAD_STATE64	__ss64;
	__uint64_t			__ds;
	__uint64_t			__es;
	__uint64_t			__ss;
	__uint64_t			__gsbase;
};
#else                       
#define	_STRUCT_X86_THREAD_FULL_STATE64	struct x86_thread_full_state64
_STRUCT_X86_THREAD_FULL_STATE64
{
	_STRUCT_X86_THREAD_STATE64	ss64;
	__uint64_t			ds;
	__uint64_t			es;
	__uint64_t			ss;
	__uint64_t			gsbase;
};
#endif                       


#if __DARWIN_UNIX03
#define	_STRUCT_X86_FLOAT_STATE64	struct __darwin_x86_float_state64
_STRUCT_X86_FLOAT_STATE64
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    

	                                 
	__uint32_t		__fpu_ip;		            
	__uint16_t		__fpu_cs;		              

	__uint16_t		__fpu_rsrv2;		              

	                                               
	__uint32_t		__fpu_dp;		            
	__uint16_t		__fpu_ds;		              

	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	_STRUCT_XMM_REG		__fpu_xmm8;		            
	_STRUCT_XMM_REG		__fpu_xmm9;		            
	_STRUCT_XMM_REG		__fpu_xmm10;		             
	_STRUCT_XMM_REG		__fpu_xmm11;		            
	_STRUCT_XMM_REG		__fpu_xmm12;		             
	_STRUCT_XMM_REG		__fpu_xmm13;		             
	_STRUCT_XMM_REG		__fpu_xmm14;		             
	_STRUCT_XMM_REG		__fpu_xmm15;		             
	char			__fpu_rsrv4[6*16];	              
	int 			__fpu_reserved1;
};

#define	_STRUCT_X86_AVX_STATE64	struct __darwin_x86_avx_state64
_STRUCT_X86_AVX_STATE64
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    

	                                 
	__uint32_t		__fpu_ip;		            
	__uint16_t		__fpu_cs;		              

	__uint16_t		__fpu_rsrv2;		              

	                                               
	__uint32_t		__fpu_dp;		            
	__uint16_t		__fpu_ds;		              

	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	_STRUCT_XMM_REG		__fpu_xmm8;		            
	_STRUCT_XMM_REG		__fpu_xmm9;		            
	_STRUCT_XMM_REG		__fpu_xmm10;		             
	_STRUCT_XMM_REG		__fpu_xmm11;		            
	_STRUCT_XMM_REG		__fpu_xmm12;		             
	_STRUCT_XMM_REG		__fpu_xmm13;		             
	_STRUCT_XMM_REG		__fpu_xmm14;		             
	_STRUCT_XMM_REG		__fpu_xmm15;		             
	char			__fpu_rsrv4[6*16];	              
	int 			__fpu_reserved1;
	char			__avx_reserved1[64];
	_STRUCT_XMM_REG		__fpu_ymmh0;		             
	_STRUCT_XMM_REG		__fpu_ymmh1;		             
	_STRUCT_XMM_REG		__fpu_ymmh2;		             
	_STRUCT_XMM_REG		__fpu_ymmh3;		             
	_STRUCT_XMM_REG		__fpu_ymmh4;		             
	_STRUCT_XMM_REG		__fpu_ymmh5;		             
	_STRUCT_XMM_REG		__fpu_ymmh6;		             
	_STRUCT_XMM_REG		__fpu_ymmh7;		             
	_STRUCT_XMM_REG		__fpu_ymmh8;		             
	_STRUCT_XMM_REG		__fpu_ymmh9;		             
	_STRUCT_XMM_REG		__fpu_ymmh10;		              
	_STRUCT_XMM_REG		__fpu_ymmh11;		              
	_STRUCT_XMM_REG		__fpu_ymmh12;		              
	_STRUCT_XMM_REG		__fpu_ymmh13;		              
	_STRUCT_XMM_REG		__fpu_ymmh14;		              
	_STRUCT_XMM_REG		__fpu_ymmh15;		              
};

#define	_STRUCT_X86_AVX512_STATE64	struct __darwin_x86_avx512_state64
_STRUCT_X86_AVX512_STATE64
{
	int 			__fpu_reserved[2];
	_STRUCT_FP_CONTROL	__fpu_fcw;		                          
	_STRUCT_FP_STATUS	__fpu_fsw;		                         
	__uint8_t		__fpu_ftw;		                      
	__uint8_t		__fpu_rsrv1;		               
	__uint16_t		__fpu_fop;		                    

	                                 
	__uint32_t		__fpu_ip;		            
	__uint16_t		__fpu_cs;		              

	__uint16_t		__fpu_rsrv2;		              

	                                               
	__uint32_t		__fpu_dp;		            
	__uint16_t		__fpu_ds;		              

	__uint16_t		__fpu_rsrv3;		              
	__uint32_t		__fpu_mxcsr;		                          
	__uint32_t		__fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	__fpu_stmm0;		               
	_STRUCT_MMST_REG	__fpu_stmm1;		              
	_STRUCT_MMST_REG	__fpu_stmm2;		              
	_STRUCT_MMST_REG	__fpu_stmm3;		              
	_STRUCT_MMST_REG	__fpu_stmm4;		              
	_STRUCT_MMST_REG	__fpu_stmm5;		              
	_STRUCT_MMST_REG	__fpu_stmm6;		              
	_STRUCT_MMST_REG	__fpu_stmm7;		              
	_STRUCT_XMM_REG		__fpu_xmm0;		            
	_STRUCT_XMM_REG		__fpu_xmm1;		            
	_STRUCT_XMM_REG		__fpu_xmm2;		            
	_STRUCT_XMM_REG		__fpu_xmm3;		            
	_STRUCT_XMM_REG		__fpu_xmm4;		            
	_STRUCT_XMM_REG		__fpu_xmm5;		            
	_STRUCT_XMM_REG		__fpu_xmm6;		            
	_STRUCT_XMM_REG		__fpu_xmm7;		            
	_STRUCT_XMM_REG		__fpu_xmm8;		            
	_STRUCT_XMM_REG		__fpu_xmm9;		            
	_STRUCT_XMM_REG		__fpu_xmm10;		             
	_STRUCT_XMM_REG		__fpu_xmm11;		            
	_STRUCT_XMM_REG		__fpu_xmm12;		             
	_STRUCT_XMM_REG		__fpu_xmm13;		             
	_STRUCT_XMM_REG		__fpu_xmm14;		             
	_STRUCT_XMM_REG		__fpu_xmm15;		             
	char			__fpu_rsrv4[6*16];	              
	int 			__fpu_reserved1;
	char			__avx_reserved1[64];
	_STRUCT_XMM_REG		__fpu_ymmh0;		             
	_STRUCT_XMM_REG		__fpu_ymmh1;		             
	_STRUCT_XMM_REG		__fpu_ymmh2;		             
	_STRUCT_XMM_REG		__fpu_ymmh3;		             
	_STRUCT_XMM_REG		__fpu_ymmh4;		             
	_STRUCT_XMM_REG		__fpu_ymmh5;		             
	_STRUCT_XMM_REG		__fpu_ymmh6;		             
	_STRUCT_XMM_REG		__fpu_ymmh7;		             
	_STRUCT_XMM_REG		__fpu_ymmh8;		             
	_STRUCT_XMM_REG		__fpu_ymmh9;		             
	_STRUCT_XMM_REG		__fpu_ymmh10;		              
	_STRUCT_XMM_REG		__fpu_ymmh11;		              
	_STRUCT_XMM_REG		__fpu_ymmh12;		              
	_STRUCT_XMM_REG		__fpu_ymmh13;		              
	_STRUCT_XMM_REG		__fpu_ymmh14;		              
	_STRUCT_XMM_REG		__fpu_ymmh15;		              
	_STRUCT_OPMASK_REG	__fpu_k0;		        
	_STRUCT_OPMASK_REG	__fpu_k1;		        
	_STRUCT_OPMASK_REG	__fpu_k2;		        
	_STRUCT_OPMASK_REG	__fpu_k3;		        
	_STRUCT_OPMASK_REG	__fpu_k4;		        
	_STRUCT_OPMASK_REG	__fpu_k5;		        
	_STRUCT_OPMASK_REG	__fpu_k6;		        
	_STRUCT_OPMASK_REG	__fpu_k7;		        
	_STRUCT_YMM_REG		__fpu_zmmh0;		             
	_STRUCT_YMM_REG		__fpu_zmmh1;		             
	_STRUCT_YMM_REG		__fpu_zmmh2;		             
	_STRUCT_YMM_REG		__fpu_zmmh3;		             
	_STRUCT_YMM_REG		__fpu_zmmh4;		             
	_STRUCT_YMM_REG		__fpu_zmmh5;		             
	_STRUCT_YMM_REG		__fpu_zmmh6;		             
	_STRUCT_YMM_REG		__fpu_zmmh7;		             
	_STRUCT_YMM_REG		__fpu_zmmh8;		             
	_STRUCT_YMM_REG		__fpu_zmmh9;		             
	_STRUCT_YMM_REG		__fpu_zmmh10;		              
	_STRUCT_YMM_REG		__fpu_zmmh11;		              
	_STRUCT_YMM_REG		__fpu_zmmh12;		              
	_STRUCT_YMM_REG		__fpu_zmmh13;		              
	_STRUCT_YMM_REG		__fpu_zmmh14;		              
	_STRUCT_YMM_REG		__fpu_zmmh15;		              
	_STRUCT_ZMM_REG		__fpu_zmm16;		             
	_STRUCT_ZMM_REG		__fpu_zmm17;		             
	_STRUCT_ZMM_REG		__fpu_zmm18;		             
	_STRUCT_ZMM_REG		__fpu_zmm19;		             
	_STRUCT_ZMM_REG		__fpu_zmm20;		             
	_STRUCT_ZMM_REG		__fpu_zmm21;		             
	_STRUCT_ZMM_REG		__fpu_zmm22;		             
	_STRUCT_ZMM_REG		__fpu_zmm23;		             
	_STRUCT_ZMM_REG		__fpu_zmm24;		             
	_STRUCT_ZMM_REG		__fpu_zmm25;		             
	_STRUCT_ZMM_REG		__fpu_zmm26;		             
	_STRUCT_ZMM_REG		__fpu_zmm27;		             
	_STRUCT_ZMM_REG		__fpu_zmm28;		             
	_STRUCT_ZMM_REG		__fpu_zmm29;		             
	_STRUCT_ZMM_REG		__fpu_zmm30;		             
	_STRUCT_ZMM_REG		__fpu_zmm31;		             
};

#else                       
#define	_STRUCT_X86_FLOAT_STATE64	struct x86_float_state64
_STRUCT_X86_FLOAT_STATE64
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    

	                                 
	__uint32_t		fpu_ip;			            
	__uint16_t		fpu_cs;			              

	__uint16_t		fpu_rsrv2;		              

	                                               
	__uint32_t		fpu_dp;			            
	__uint16_t		fpu_ds;			              

	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;		                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	_STRUCT_XMM_REG		fpu_xmm8;		            
	_STRUCT_XMM_REG		fpu_xmm9;		            
	_STRUCT_XMM_REG		fpu_xmm10;		             
	_STRUCT_XMM_REG		fpu_xmm11;		            
	_STRUCT_XMM_REG		fpu_xmm12;		             
	_STRUCT_XMM_REG		fpu_xmm13;		             
	_STRUCT_XMM_REG		fpu_xmm14;		             
	_STRUCT_XMM_REG		fpu_xmm15;		             
	char			fpu_rsrv4[6*16];	              
	int 			fpu_reserved1;
};

#define	_STRUCT_X86_AVX_STATE64	struct x86_avx_state64
_STRUCT_X86_AVX_STATE64
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    

	                                 
	__uint32_t		fpu_ip;			            
	__uint16_t		fpu_cs;			              

	__uint16_t		fpu_rsrv2;		              

	                                               
	__uint32_t		fpu_dp;			            
	__uint16_t		fpu_ds;			              

	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;		                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	_STRUCT_XMM_REG		fpu_xmm8;		            
	_STRUCT_XMM_REG		fpu_xmm9;		            
	_STRUCT_XMM_REG		fpu_xmm10;		             
	_STRUCT_XMM_REG		fpu_xmm11;		            
	_STRUCT_XMM_REG		fpu_xmm12;		             
	_STRUCT_XMM_REG		fpu_xmm13;		             
	_STRUCT_XMM_REG		fpu_xmm14;		             
	_STRUCT_XMM_REG		fpu_xmm15;		             
	char			fpu_rsrv4[6*16];	              
	int 			fpu_reserved1;
	char			avx_reserved1[64];
	_STRUCT_XMM_REG		fpu_ymmh0;		             
	_STRUCT_XMM_REG		fpu_ymmh1;		             
	_STRUCT_XMM_REG		fpu_ymmh2;		             
	_STRUCT_XMM_REG		fpu_ymmh3;		             
	_STRUCT_XMM_REG		fpu_ymmh4;		             
	_STRUCT_XMM_REG		fpu_ymmh5;		             
	_STRUCT_XMM_REG		fpu_ymmh6;		             
	_STRUCT_XMM_REG		fpu_ymmh7;		             
	_STRUCT_XMM_REG		fpu_ymmh8;		             
	_STRUCT_XMM_REG		fpu_ymmh9;		             
	_STRUCT_XMM_REG		fpu_ymmh10;		              
	_STRUCT_XMM_REG		fpu_ymmh11;		              
	_STRUCT_XMM_REG		fpu_ymmh12;		              
	_STRUCT_XMM_REG		fpu_ymmh13;		              
	_STRUCT_XMM_REG		fpu_ymmh14;		              
	_STRUCT_XMM_REG		fpu_ymmh15;		              
};

#define	_STRUCT_X86_AVX512_STATE64	struct x86_avx512_state64
_STRUCT_X86_AVX512_STATE64
{
	int 			fpu_reserved[2];
	_STRUCT_FP_CONTROL	fpu_fcw;		                          
	_STRUCT_FP_STATUS	fpu_fsw;		                         
	__uint8_t		fpu_ftw;		                      
	__uint8_t		fpu_rsrv1;		               
	__uint16_t		fpu_fop;		                    

	                                 
	__uint32_t		fpu_ip;		            
	__uint16_t		fpu_cs;		              

	__uint16_t		fpu_rsrv2;		              

	                                               
	__uint32_t		fpu_dp;		            
	__uint16_t		fpu_ds;		              

	__uint16_t		fpu_rsrv3;		              
	__uint32_t		fpu_mxcsr;		                          
	__uint32_t		fpu_mxcsrmask;	                
	_STRUCT_MMST_REG	fpu_stmm0;		               
	_STRUCT_MMST_REG	fpu_stmm1;		              
	_STRUCT_MMST_REG	fpu_stmm2;		              
	_STRUCT_MMST_REG	fpu_stmm3;		              
	_STRUCT_MMST_REG	fpu_stmm4;		              
	_STRUCT_MMST_REG	fpu_stmm5;		              
	_STRUCT_MMST_REG	fpu_stmm6;		              
	_STRUCT_MMST_REG	fpu_stmm7;		              
	_STRUCT_XMM_REG		fpu_xmm0;		            
	_STRUCT_XMM_REG		fpu_xmm1;		            
	_STRUCT_XMM_REG		fpu_xmm2;		            
	_STRUCT_XMM_REG		fpu_xmm3;		            
	_STRUCT_XMM_REG		fpu_xmm4;		            
	_STRUCT_XMM_REG		fpu_xmm5;		            
	_STRUCT_XMM_REG		fpu_xmm6;		            
	_STRUCT_XMM_REG		fpu_xmm7;		            
	_STRUCT_XMM_REG		fpu_xmm8;		            
	_STRUCT_XMM_REG		fpu_xmm9;		            
	_STRUCT_XMM_REG		fpu_xmm10;		             
	_STRUCT_XMM_REG		fpu_xmm11;		            
	_STRUCT_XMM_REG		fpu_xmm12;		             
	_STRUCT_XMM_REG		fpu_xmm13;		             
	_STRUCT_XMM_REG		fpu_xmm14;		             
	_STRUCT_XMM_REG		fpu_xmm15;		             
	char			fpu_rsrv4[6*16];	              
	int 			fpu_reserved1;
	char			avx_reserved1[64];
	_STRUCT_XMM_REG		fpu_ymmh0;		             
	_STRUCT_XMM_REG		fpu_ymmh1;		             
	_STRUCT_XMM_REG		fpu_ymmh2;		             
	_STRUCT_XMM_REG		fpu_ymmh3;		             
	_STRUCT_XMM_REG		fpu_ymmh4;		             
	_STRUCT_XMM_REG		fpu_ymmh5;		             
	_STRUCT_XMM_REG		fpu_ymmh6;		             
	_STRUCT_XMM_REG		fpu_ymmh7;		             
	_STRUCT_XMM_REG		fpu_ymmh8;		             
	_STRUCT_XMM_REG		fpu_ymmh9;		             
	_STRUCT_XMM_REG		fpu_ymmh10;		              
	_STRUCT_XMM_REG		fpu_ymmh11;		              
	_STRUCT_XMM_REG		fpu_ymmh12;		              
	_STRUCT_XMM_REG		fpu_ymmh13;		              
	_STRUCT_XMM_REG		fpu_ymmh14;		              
	_STRUCT_XMM_REG		fpu_ymmh15;		              
	_STRUCT_OPMASK_REG	fpu_k0;			        
	_STRUCT_OPMASK_REG	fpu_k1;			        
	_STRUCT_OPMASK_REG	fpu_k2;			        
	_STRUCT_OPMASK_REG	fpu_k3;			        
	_STRUCT_OPMASK_REG	fpu_k4;			        
	_STRUCT_OPMASK_REG	fpu_k5;			        
	_STRUCT_OPMASK_REG	fpu_k6;			        
	_STRUCT_OPMASK_REG	fpu_k7;			        
	_STRUCT_YMM_REG		fpu_zmmh0;		             
	_STRUCT_YMM_REG		fpu_zmmh1;		             
	_STRUCT_YMM_REG		fpu_zmmh2;		             
	_STRUCT_YMM_REG		fpu_zmmh3;		             
	_STRUCT_YMM_REG		fpu_zmmh4;		             
	_STRUCT_YMM_REG		fpu_zmmh5;		             
	_STRUCT_YMM_REG		fpu_zmmh6;		             
	_STRUCT_YMM_REG		fpu_zmmh7;		             
	_STRUCT_YMM_REG		fpu_zmmh8;		             
	_STRUCT_YMM_REG		fpu_zmmh9;		             
	_STRUCT_YMM_REG		fpu_zmmh10;		              
	_STRUCT_YMM_REG		fpu_zmmh11;		              
	_STRUCT_YMM_REG		fpu_zmmh12;		              
	_STRUCT_YMM_REG		fpu_zmmh13;		              
	_STRUCT_YMM_REG		fpu_zmmh14;		              
	_STRUCT_YMM_REG		fpu_zmmh15;		              
	_STRUCT_ZMM_REG		fpu_zmm16;		             
	_STRUCT_ZMM_REG		fpu_zmm17;		             
	_STRUCT_ZMM_REG		fpu_zmm18;		             
	_STRUCT_ZMM_REG		fpu_zmm19;		             
	_STRUCT_ZMM_REG		fpu_zmm20;		             
	_STRUCT_ZMM_REG		fpu_zmm21;		             
	_STRUCT_ZMM_REG		fpu_zmm22;		             
	_STRUCT_ZMM_REG		fpu_zmm23;		             
	_STRUCT_ZMM_REG		fpu_zmm24;		             
	_STRUCT_ZMM_REG		fpu_zmm25;		             
	_STRUCT_ZMM_REG		fpu_zmm26;		             
	_STRUCT_ZMM_REG		fpu_zmm27;		             
	_STRUCT_ZMM_REG		fpu_zmm28;		             
	_STRUCT_ZMM_REG		fpu_zmm29;		             
	_STRUCT_ZMM_REG		fpu_zmm30;		             
	_STRUCT_ZMM_REG		fpu_zmm31;		             
};

#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_X86_EXCEPTION_STATE64	struct __darwin_x86_exception_state64
_STRUCT_X86_EXCEPTION_STATE64
{
    __uint16_t	__trapno;
    __uint16_t	__cpu;
    __uint32_t	__err;
    __uint64_t	__faultvaddr;
};
#else                       
#define _STRUCT_X86_EXCEPTION_STATE64	struct x86_exception_state64
_STRUCT_X86_EXCEPTION_STATE64
{
    __uint16_t	trapno;
    __uint16_t	cpu;
    __uint32_t	err;
    __uint64_t	faultvaddr;
};
#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_X86_DEBUG_STATE64	struct __darwin_x86_debug_state64
_STRUCT_X86_DEBUG_STATE64
{
	__uint64_t	__dr0;
	__uint64_t	__dr1;
	__uint64_t	__dr2;
	__uint64_t	__dr3;
	__uint64_t	__dr4;
	__uint64_t	__dr5;
	__uint64_t	__dr6;
	__uint64_t	__dr7;
};
#else                       
#define _STRUCT_X86_DEBUG_STATE64	struct x86_debug_state64
_STRUCT_X86_DEBUG_STATE64
{
	__uint64_t	dr0;
	__uint64_t	dr1;
	__uint64_t	dr2;
	__uint64_t	dr3;
	__uint64_t	dr4;
	__uint64_t	dr5;
	__uint64_t	dr6;
	__uint64_t	dr7;
};
#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_X86_CPMU_STATE64	struct __darwin_x86_cpmu_state64
_STRUCT_X86_CPMU_STATE64
{
	__uint64_t __ctrs[16];
};
#else                      
#define _STRUCT_X86_CPMU_STATE64	struct x86_cpmu_state64
_STRUCT_X86_CPMU_STATE64
{
	__uint64_t ctrs[16];
};
#endif                       

#endif                             
