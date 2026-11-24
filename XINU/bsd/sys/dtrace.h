/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Portions copyright (c) 2013, Joyent, Inc. All rights reserved.
 * Portions Copyright (c) 2013 by Delphix. All rights reserved.
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Portions Copyright (c) 2012 by Delphix. All rights reserved.
 */

#ifndef _SYS_DTRACE_H
#define _SYS_DTRACE_H

#ifdef  __cplusplus
extern "C" {
#endif

  
                                                     
  
                                                                           
                                                                            
                                                                             
                                                                          
                                                                             
                                                                            
   

#ifndef _ASM

#if !defined(__APPLE__)
#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/processor.h>
#include <sys/systm.h>
#include <sys/ctf_api.h>
#include <sys/cyclic.h>
#include <sys/int_limits.h>
#else                        

#if defined(__LP64__)
#if !defined(_LP64)
#define _LP64                         
#endif
#else
#if !defined(_ILP32)
#define _ILP32                         
#endif
#endif

#if defined(__BIG_ENDIAN__)
#if !defined(_BIG_ENDIAN)
#define _BIG_ENDIAN                         
#endif
#elif defined(__LITTLE_ENDIAN__)
#if !defined(_LITTLE_ENDIAN)
#define _LITTLE_ENDIAN                         
#endif
#else
#error Unknown endian-ness
#endif

#ifdef KERNEL
#ifndef _KERNEL
#define _KERNEL                         
#endif
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)                           
#endif

#define SEC			1
#define MILLISEC	1000
#define MICROSEC	1000000
#define NANOSEC		1000000000

#define S_ROUND(x, a)   ((x) + (((a) ? (a) : 1) - 1) & ~(((a) ? (a) : 1) - 1))
#define P2ROUNDUP(x, align)             (-(-(x) & -(align)))
#define	P2PHASEUP(x, align, phase)	((phase) - (((phase) - (x)) & -(align)))

#define	CTF_MODEL_ILP32	1	                                
#define	CTF_MODEL_LP64	2	                               
#ifdef __LP64__
#define	CTF_MODEL_NATIVE	CTF_MODEL_LP64
#else
#define	CTF_MODEL_NATIVE	CTF_MODEL_ILP32
#endif

typedef uint8_t		uchar_t;
typedef uint16_t	ushort_t;
typedef uint32_t	uint_t;
typedef unsigned long	ulong_t;
typedef uint64_t	u_longlong_t;
typedef int64_t		longlong_t;
typedef int64_t		off64_t;
typedef int			processorid_t;
typedef int64_t		hrtime_t;

typedef enum { B_FALSE = 0, B_TRUE = 1 } _dtrace_boolean;

typedef uint8_t UUID[16];                                 

struct modctl;                                        
                                                         
#include <sys/ioctl.h>                                       
#ifdef KERNEL
                                                      
#else
                                        
typedef struct ctf_file ctf_file_t;
typedef long ctf_id_t;
#endif
                                                     
                                                         

typedef uint32_t        zoneid_t;

#include <sys/dtrace_glue.h>

#include <stdarg.h> 
typedef va_list __va_list;

                                                                       
#define proc_t struct proc                                                   
#endif                

  
                                          
   
#define	DTRACE_CPUALL		-1	              
#define	DTRACE_IDNONE		0	                              
#define	DTRACE_EPIDNONE		0	                                      
#define	DTRACE_AGGIDNONE	0	                                    
#define	DTRACE_AGGVARIDNONE	0	                                     
#define	DTRACE_CACHEIDNONE	0	                             
#define	DTRACE_PROVNONE		0	                                 
#define	DTRACE_METAPROVNONE	0	                                      
#define	DTRACE_ARGNONE		-1	                            

#define	DTRACE_PROVNAMELEN	64
#define	DTRACE_MODNAMELEN	64
#define	DTRACE_FUNCNAMELEN	128
#define	DTRACE_NAMELEN		64
#define	DTRACE_FULLNAMELEN	(DTRACE_PROVNAMELEN + DTRACE_MODNAMELEN + \
				DTRACE_FUNCNAMELEN + DTRACE_NAMELEN + 4)
#define	DTRACE_ARGTYPELEN	128

typedef uint32_t dtrace_id_t;		                      
typedef uint32_t dtrace_epid_t;		                              
typedef uint32_t dtrace_aggid_t;	                            
typedef int64_t dtrace_aggvarid_t;	                                     
typedef uint16_t dtrace_actkind_t;	                 
typedef int64_t dtrace_optval_t;	                  
typedef uint32_t dtrace_cacheid_t;	                                

typedef enum dtrace_probespec {
        DTRACE_PROBESPEC_NONE = -1,
        DTRACE_PROBESPEC_PROVIDER = 0,
        DTRACE_PROBESPEC_MOD,
        DTRACE_PROBESPEC_FUNC,
        DTRACE_PROBESPEC_NAME
} dtrace_probespec_t;

  
                                   
  
                                                                             
                                                                     
                                                                            
                                                                               
                                                                         
                                                           
   
#define	DIF_VERSION_1	1		                                    
#define	DIF_VERSION_2	2		                                   
#define	DIF_VERSION	DIF_VERSION_2	                                        
#define	DIF_DIR_NREGS	8		                                     
#define	DIF_DTR_NREGS	8		                                   

#define	DIF_OP_OR	1		                   
#define	DIF_OP_XOR	2		                    
#define	DIF_OP_AND	3		                    
#define	DIF_OP_SLL	4		                    
#define	DIF_OP_SRL	5		                    
#define	DIF_OP_SUB	6		                    
#define	DIF_OP_ADD	7		                    
#define	DIF_OP_MUL	8		                    
#define	DIF_OP_SDIV	9		                     
#define	DIF_OP_UDIV	10		                     
#define	DIF_OP_SREM	11		                     
#define	DIF_OP_UREM	12		                     
#define	DIF_OP_NOT	13		                
#define	DIF_OP_MOV	14		                
#define	DIF_OP_CMP	15		                
#define	DIF_OP_TST	16		             
#define	DIF_OP_BA	17		              
#define	DIF_OP_BE	18		              
#define	DIF_OP_BNE	19		               
#define	DIF_OP_BG	20		              
#define	DIF_OP_BGU	21		               
#define	DIF_OP_BGE	22		               
#define	DIF_OP_BGEU	23		                
#define	DIF_OP_BL	24		              
#define	DIF_OP_BLU	25		               
#define	DIF_OP_BLE	26		               
#define	DIF_OP_BLEU	27		                
#define	DIF_OP_LDSB	28		                   
#define	DIF_OP_LDSH	29		                   
#define	DIF_OP_LDSW	30		                   
#define	DIF_OP_LDUB	31		                   
#define	DIF_OP_LDUH	32		                   
#define	DIF_OP_LDUW	33		                   
#define	DIF_OP_LDX	34		                  
#define	DIF_OP_RET	35		            
#define	DIF_OP_NOP	36		         
#define	DIF_OP_SETX	37		                       
#define	DIF_OP_SETS	38		                       
#define	DIF_OP_SCMP	39		                 
#define	DIF_OP_LDGA	40		                      
#define	DIF_OP_LDGS	41		                  
#define	DIF_OP_STGS	42		                  
#define	DIF_OP_LDTA	43		                      
#define	DIF_OP_LDTS	44		                  
#define	DIF_OP_STTS	45		                  
#define	DIF_OP_SRA	46		                    
#define	DIF_OP_CALL	47		                   
#define	DIF_OP_PUSHTR	48		                         
#define	DIF_OP_PUSHTV	49		                         
#define	DIF_OP_POPTS	50		           
#define	DIF_OP_FLUSHTS	51		             
#define	DIF_OP_LDGAA	52		                   
#define	DIF_OP_LDTAA	53		                   
#define	DIF_OP_STGAA	54		                   
#define	DIF_OP_STTAA	55		                   
#define	DIF_OP_LDLS	56		                  
#define	DIF_OP_STLS	57		                  
#define	DIF_OP_ALLOCS	58		                   
#define	DIF_OP_COPYS	59		                       
#define	DIF_OP_STB	60		                  
#define	DIF_OP_STH	61		                  
#define	DIF_OP_STW	62		                  
#define	DIF_OP_STX	63		                  
#define	DIF_OP_ULDSB	64		                    
#define	DIF_OP_ULDSH	65		                    
#define	DIF_OP_ULDSW	66		                    
#define	DIF_OP_ULDUB	67		                    
#define	DIF_OP_ULDUH	68		                    
#define	DIF_OP_ULDUW	69		                    
#define	DIF_OP_ULDX	70		                    
#define	DIF_OP_RLDSB	71		                    
#define	DIF_OP_RLDSH	72		                    
#define	DIF_OP_RLDSW	73		                    
#define	DIF_OP_RLDUB	74		                    
#define	DIF_OP_RLDUH	75		                    
#define	DIF_OP_RLDUW	76		                    
#define	DIF_OP_RLDX	77		                    
#define	DIF_OP_XLATE	78		                        
#define	DIF_OP_XLARG	79		                        
#define	DIF_OP_STRIP	80		                       

#define	DIF_INTOFF_MAX		0xffff	                                  
#define	DIF_STROFF_MAX		0xffff	                                 
#define	DIF_REGISTER_MAX	0xff	                             
#define	DIF_VARIABLE_MAX	0xffff	                                 
#define	DIF_SUBROUTINE_MAX	0xffff	                             

#define	DIF_VAR_ARRAY_MIN	0x0000	                                    
#define	DIF_VAR_ARRAY_UBASE	0x0080	                               
#define	DIF_VAR_ARRAY_MAX	0x00ff	                                     

#define	DIF_VAR_OTHER_MIN	0x0100	                                    
#define	DIF_VAR_OTHER_UBASE	0x0500	                                        
#define	DIF_VAR_OTHER_MAX	0xffff	                                     

#define	DIF_VAR_ARGS		0x0000	                     
#define	DIF_VAR_REGS		0x0001	                     
#define	DIF_VAR_UREGS		0x0002	                          
#define	DIF_VAR_VMREGS		0x0003	                                     
#define	DIF_VAR_CURTHREAD	0x0100	                    
#define	DIF_VAR_TIMESTAMP	0x0101	               
#define	DIF_VAR_VTIMESTAMP	0x0102	                       
#define	DIF_VAR_IPL		0x0103	                              
#define	DIF_VAR_EPID		0x0104	                      
#define	DIF_VAR_ID		0x0105	              
#define	DIF_VAR_ARG0		0x0106	                    
#define	DIF_VAR_ARG1		0x0107	                     
#define	DIF_VAR_ARG2		0x0108	                    
#define	DIF_VAR_ARG3		0x0109	                     
#define	DIF_VAR_ARG4		0x010a	                    
#define	DIF_VAR_ARG5		0x010b	                    
#define	DIF_VAR_ARG6		0x010c	                      
#define	DIF_VAR_ARG7		0x010d	                     
#define	DIF_VAR_ARG8		0x010e	                    
#define	DIF_VAR_ARG9		0x010f	                    
#define	DIF_VAR_STACKDEPTH	0x0110	                 
#define	DIF_VAR_CALLER		0x0111	            
#define	DIF_VAR_PROBEPROV	0x0112	                    
#define	DIF_VAR_PROBEMOD	0x0113	                  
#define	DIF_VAR_PROBEFUNC	0x0114	                    
#define	DIF_VAR_PROBENAME	0x0115	                
#define	DIF_VAR_PID		0x0116	                
#define	DIF_VAR_TID		0x0117	                             
#define	DIF_VAR_EXECNAME	0x0118	                        
#define	DIF_VAR_ZONENAME	0x0119	                                       
#define	DIF_VAR_WALLTIMESTAMP	0x011a	                          
#define	DIF_VAR_USTACKDEPTH	0x011b	                           
#define	DIF_VAR_UCALLER		0x011c	                       
#define	DIF_VAR_PPID		0x011d	                       
#define	DIF_VAR_UID		0x011e	                     
#define	DIF_VAR_GID		0x011f	                      
#define	DIF_VAR_ERRNO		0x0120	                  
#if defined(__APPLE__)
#define DIF_VAR_PTHREAD_SELF	0x0200	                                                            
#define DIF_VAR_DISPATCHQADDR	0x0201	                                        
#define DIF_VAR_MACHTIMESTAMP	0x0202	                          
#define DIF_VAR_CPU		0x0203	                
#define DIF_VAR_CPUINSTRS	0x0204	                      
#define DIF_VAR_CPUCYCLES	0x0205	                
#define DIF_VAR_VINSTRS		0x0206	                          
#define DIF_VAR_VCYCLES		0x0207	                    
#define DIF_VAR_MACHCTIMESTAMP	0x0208	                            
#endif                 

#define	DIF_SUBR_RAND			0
#define	DIF_SUBR_MUTEX_OWNED		1
#define	DIF_SUBR_MUTEX_OWNER		2
#define	DIF_SUBR_MUTEX_TYPE_ADAPTIVE	3
#define	DIF_SUBR_MUTEX_TYPE_SPIN	4
#define	DIF_SUBR_RW_READ_HELD		5
#define	DIF_SUBR_RW_WRITE_HELD		6
#define	DIF_SUBR_RW_ISWRITER		7
#define	DIF_SUBR_COPYIN			8
#define	DIF_SUBR_COPYINSTR		9
#define	DIF_SUBR_SPECULATION		10
#define	DIF_SUBR_PROGENYOF		11
#define	DIF_SUBR_STRLEN			12
#define	DIF_SUBR_COPYOUT		13
#define	DIF_SUBR_COPYOUTSTR		14
#define	DIF_SUBR_ALLOCA			15
#define	DIF_SUBR_BCOPY			16
#define	DIF_SUBR_COPYINTO		17
#define	DIF_SUBR_MSGDSIZE		18
#define	DIF_SUBR_MSGSIZE		19
#define	DIF_SUBR_GETMAJOR		20
#define	DIF_SUBR_GETMINOR		21
#define	DIF_SUBR_DDI_PATHNAME		22
#define	DIF_SUBR_STRJOIN		23
#define	DIF_SUBR_LLTOSTR		24
#define	DIF_SUBR_BASENAME		25
#define	DIF_SUBR_DIRNAME		26
#define	DIF_SUBR_CLEANPATH		27
#define	DIF_SUBR_STRCHR			28
#define	DIF_SUBR_STRRCHR		29
#define	DIF_SUBR_STRSTR			30
#define	DIF_SUBR_STRTOK			31
#define	DIF_SUBR_SUBSTR			32
#define	DIF_SUBR_INDEX			33
#define	DIF_SUBR_RINDEX			34
#define	DIF_SUBR_HTONS			35
#define	DIF_SUBR_HTONL			36
#define	DIF_SUBR_HTONLL			37
#define	DIF_SUBR_NTOHS			38
#define	DIF_SUBR_NTOHL			39
#define	DIF_SUBR_NTOHLL			40
#define	DIF_SUBR_INET_NTOP		41
#define	DIF_SUBR_INET_NTOA		42
#define	DIF_SUBR_INET_NTOA6		43
#define	DIF_SUBR_TOUPPER		44
#define	DIF_SUBR_TOLOWER		45
#define DIF_SUBR_JSON			46
#define DIF_SUBR_STRTOLL		47
#define DIF_SUBR_STRIP			48
#define DIF_SUBR_MAX			48                                

                                
#if defined(__APPLE__)
#define DIF_SUBR_APPLE_MIN		200	                                         
#define DIF_SUBR_VM_KERNEL_ADDRPERM	200
#define DIF_SUBR_KDEBUG_TRACE		201
#define DIF_SUBR_KDEBUG_TRACE_STRING	202
#define DIF_SUBR_MTONS			203
#define DIF_SUBR_PHYSMEM_READ		204
#define DIF_SUBR_PHYSMEM_WRITE		205
#define DIF_SUBR_KVTOPHYS		206
#define DIF_SUBR_APPLE_MAX		206                                               
#endif                

typedef uint32_t dif_instr_t;

#define DIF_INSTR_OP(i)                 (((i) >> 24) & 0xff)
#define DIF_INSTR_R1(i)                 (((i) >> 16) & 0xff)
#define DIF_INSTR_R2(i)                 (((i) >>  8) & 0xff)
#define DIF_INSTR_RD(i)                 ((i) & 0xff)
#define DIF_INSTR_RS(i)                 ((i) & 0xff)
#define DIF_INSTR_IMM2(i)               (((i) >>  8) & 0xff)
#define DIF_INSTR_LABEL(i)              ((i) & 0xffffff)
#define DIF_INSTR_VAR(i)                (((i) >>  8) & 0xffff)
#define DIF_INSTR_INTEGER(i)            (((i) >>  8) & 0xffff)
#define DIF_INSTR_STRING(i)             (((i) >>  8) & 0xffff)
#define DIF_INSTR_SUBR(i)               (((i) >>  8) & 0xffff)
#define DIF_INSTR_TYPE(i)               (((i) >> 16) & 0xff)
#define DIF_INSTR_XLREF(i)              (((i) >>  8) & 0xffff)

#define DIF_INSTR_FMT(op, r1, r2, d) \
        (((op) << 24) | ((r1) << 16) | ((r2) << 8) | (d))

#define DIF_INSTR_NOT(r1, d)            (DIF_INSTR_FMT(DIF_OP_NOT, r1, 0, d))
#define DIF_INSTR_MOV(r1, d)            (DIF_INSTR_FMT(DIF_OP_MOV, r1, 0, d))
#define DIF_INSTR_CMP(op, r1, r2)       (DIF_INSTR_FMT(op, r1, r2, 0))
#define DIF_INSTR_TST(r1)               (DIF_INSTR_FMT(DIF_OP_TST, r1, 0, 0))
#define DIF_INSTR_BRANCH(op, label)     (((op) << 24) | (label))
#define DIF_INSTR_LOAD(op, r1, d)       (DIF_INSTR_FMT(op, r1, 0, d))
#define DIF_INSTR_STORE(op, r1, d)      (DIF_INSTR_FMT(op, r1, 0, d))
#define DIF_INSTR_SETX(i, d)            ((DIF_OP_SETX << 24) | ((i) << 8) | (d))
#define DIF_INSTR_SETS(s, d)            ((DIF_OP_SETS << 24) | ((s) << 8) | (d))
#define DIF_INSTR_RET(d)                (DIF_INSTR_FMT(DIF_OP_RET, 0, 0, d))
#define DIF_INSTR_NOP                   (DIF_OP_NOP << 24)
#define DIF_INSTR_LDA(op, v, r, d)      (DIF_INSTR_FMT(op, v, r, d))
#define DIF_INSTR_LDV(op, v, d)         (((op) << 24) | ((v) << 8) | (d))
#define DIF_INSTR_STV(op, v, rs)        (((op) << 24) | ((v) << 8) | (rs))
#define DIF_INSTR_CALL(s, d)            ((DIF_OP_CALL << 24) | ((s) << 8) | (d))
#define DIF_INSTR_PUSHTS(op, t, r2, rs) (DIF_INSTR_FMT(op, t, r2, rs))
#define DIF_INSTR_POPTS                 (DIF_OP_POPTS << 24)
#define DIF_INSTR_FLUSHTS               (DIF_OP_FLUSHTS << 24)
#define DIF_INSTR_ALLOCS(r1, d)         (DIF_INSTR_FMT(DIF_OP_ALLOCS, r1, 0, d))
#define DIF_INSTR_COPYS(r1, r2, d)      (DIF_INSTR_FMT(DIF_OP_COPYS, r1, r2, d))
#define DIF_INSTR_XLATE(op, r, d)       (((op) << 24) | ((r) << 8) | (d))

#define DIF_REG_R0      0                                              

  
                                                                              
                                                                              
                                                                             
                                              
   
typedef struct dtrace_diftype {
        uint8_t dtdt_kind;                                         
        uint8_t dtdt_ckind;                                   
        uint8_t dtdt_flags;                                         
        uint8_t dtdt_pad;                                            
        uint32_t dtdt_size;                                                     
} dtrace_diftype_t;

#define DIF_TYPE_CTF            0                               
#define DIF_TYPE_STRING         1                               

#define DIF_TF_BYREF            0x1                                      
#define DIF_TF_BYUREF           0x2                                           

  
                                                                               
                                                                               
                                                                            
                                                       
   
typedef struct dtrace_difv {
        uint32_t dtdv_name;                                                     
        uint32_t dtdv_id;                                                  
        uint8_t dtdv_kind;                                             
        uint8_t dtdv_scope;                                             
        uint16_t dtdv_flags;                                            
        dtrace_diftype_t dtdv_type;                                    
} dtrace_difv_t;

#define DIFV_KIND_ARRAY         0                                               
#define DIFV_KIND_SCALAR        1                                          

#define DIFV_SCOPE_GLOBAL       0                                      
#define DIFV_SCOPE_THREAD       1                                      
#define DIFV_SCOPE_LOCAL        2                                     

#define DIFV_F_REF              0x1                                         
#define DIFV_F_MOD              0x2                                      

  
                 
  
                                                                              
                                                                        
           
  
                                                                                
                                                                       
                                                                          
                                                                      
                                                                           
                                                                
                                                        
   
#define DTRACEACT_NONE                  0                      
#define DTRACEACT_DIFEXPR               1                                     
#define DTRACEACT_EXIT                  2                          
#define DTRACEACT_PRINTF                3                            
#define DTRACEACT_PRINTA                4                            
#define DTRACEACT_LIBACT                5                                      
#define DTRACEACT_TRACEMEM              6                              
#define DTRACEACT_TRACEMEM_DYNSIZE      7                                    

#if defined(__APPLE__)
#define DTRACEACT_APPLEBINARY           50                                      
#endif                

#define DTRACEACT_PROC                  0x0100
#define DTRACEACT_USTACK                (DTRACEACT_PROC + 1)
#define DTRACEACT_JSTACK                (DTRACEACT_PROC + 2)
#define DTRACEACT_USYM                  (DTRACEACT_PROC + 3)
#define DTRACEACT_UMOD                  (DTRACEACT_PROC + 4)
#define DTRACEACT_UADDR                 (DTRACEACT_PROC + 5)

#define DTRACEACT_PROC_DESTRUCTIVE      0x0200
#define DTRACEACT_STOP                  (DTRACEACT_PROC_DESTRUCTIVE + 1)
#define DTRACEACT_RAISE                 (DTRACEACT_PROC_DESTRUCTIVE + 2)
#define DTRACEACT_SYSTEM                (DTRACEACT_PROC_DESTRUCTIVE + 3)
#define DTRACEACT_FREOPEN               (DTRACEACT_PROC_DESTRUCTIVE + 4)
	
#if defined(__APPLE__)
  
                                                                 
                                             
   
	
#define DTRACEACT_PIDRESUME		(DTRACEACT_PROC_DESTRUCTIVE + 50)
#endif                	

#define DTRACEACT_PROC_CONTROL          0x0300

#define DTRACEACT_KERNEL                0x0400
#define DTRACEACT_STACK                 (DTRACEACT_KERNEL + 1)
#define DTRACEACT_SYM                   (DTRACEACT_KERNEL + 2)
#define DTRACEACT_MOD                   (DTRACEACT_KERNEL + 3)

#define DTRACEACT_KERNEL_DESTRUCTIVE    0x0500
#define DTRACEACT_BREAKPOINT            (DTRACEACT_KERNEL_DESTRUCTIVE + 1)
#define DTRACEACT_PANIC                 (DTRACEACT_KERNEL_DESTRUCTIVE + 2)
#define DTRACEACT_CHILL                 (DTRACEACT_KERNEL_DESTRUCTIVE + 3)

#define DTRACEACT_SPECULATIVE           0x0600
#define DTRACEACT_SPECULATE             (DTRACEACT_SPECULATIVE + 1)
#define DTRACEACT_COMMIT                (DTRACEACT_SPECULATIVE + 2)
#define DTRACEACT_DISCARD               (DTRACEACT_SPECULATIVE + 3)

#define DTRACEACT_CLASS(x)              ((x) & 0xff00)

#define DTRACEACT_ISDESTRUCTIVE(x)      \
        (DTRACEACT_CLASS(x) == DTRACEACT_PROC_DESTRUCTIVE || \
        DTRACEACT_CLASS(x) == DTRACEACT_KERNEL_DESTRUCTIVE)

#define DTRACEACT_ISSPECULATIVE(x)      \
        (DTRACEACT_CLASS(x) == DTRACEACT_SPECULATIVE)

#define DTRACEACT_ISPRINTFLIKE(x)       \
        ((x) == DTRACEACT_PRINTF || (x) == DTRACEACT_PRINTA || \
        (x) == DTRACEACT_SYSTEM || (x) == DTRACEACT_FREOPEN)

  
                             
  
                                                            
  
                                                                  
  
                                                                              
                                                                               
                                                                               
                                                                         
   
#define DTRACEACT_AGGREGATION           0x0700
#define DTRACEAGG_COUNT                 (DTRACEACT_AGGREGATION + 1)
#define DTRACEAGG_MIN                   (DTRACEACT_AGGREGATION + 2)
#define DTRACEAGG_MAX                   (DTRACEACT_AGGREGATION + 3)
#define DTRACEAGG_AVG                   (DTRACEACT_AGGREGATION + 4)
#define DTRACEAGG_SUM                   (DTRACEACT_AGGREGATION + 5)
#define DTRACEAGG_STDDEV                (DTRACEACT_AGGREGATION + 6)
#define DTRACEAGG_QUANTIZE              (DTRACEACT_AGGREGATION + 7)
#define DTRACEAGG_LQUANTIZE             (DTRACEACT_AGGREGATION + 8)
#define DTRACEAGG_LLQUANTIZE            (DTRACEACT_AGGREGATION + 9)

#define DTRACEACT_ISAGG(x)              \
        (DTRACEACT_CLASS(x) == DTRACEACT_AGGREGATION)

#if !defined(__APPLE__)                               
#define DTRACE_QUANTIZE_NBUCKETS        \
        (((sizeof (uint64_t) * NBBY) - 1) * 2 + 1)

#define DTRACE_QUANTIZE_ZEROBUCKET      ((sizeof (uint64_t) * NBBY) - 1)
#else
#define DTRACE_QUANTIZE_NBUCKETS        \
        (int)(((sizeof (uint64_t) * NBBY) - 1) * 2 + 1)

#define DTRACE_QUANTIZE_ZEROBUCKET      (int64_t)((sizeof (uint64_t) * NBBY) - 1)
#endif                

#define DTRACE_QUANTIZE_BUCKETVAL(buck)                                 \
        (int64_t)((buck) < DTRACE_QUANTIZE_ZEROBUCKET ?                 \
        -(1LL << (DTRACE_QUANTIZE_ZEROBUCKET - 1 - (buck))) :           \
        (buck) == DTRACE_QUANTIZE_ZEROBUCKET ? 0 :                      \
        1LL << ((buck) - DTRACE_QUANTIZE_ZEROBUCKET - 1))

#define DTRACE_LQUANTIZE_STEPSHIFT              48
#define DTRACE_LQUANTIZE_STEPMASK               ((uint64_t)UINT16_MAX << 48)
#define DTRACE_LQUANTIZE_LEVELSHIFT             32
#define DTRACE_LQUANTIZE_LEVELMASK              ((uint64_t)UINT16_MAX << 32)
#define DTRACE_LQUANTIZE_BASESHIFT              0
#define DTRACE_LQUANTIZE_BASEMASK               UINT32_MAX

#define DTRACE_LQUANTIZE_STEP(x)                \
        (uint16_t)(((x) & DTRACE_LQUANTIZE_STEPMASK) >> \
        DTRACE_LQUANTIZE_STEPSHIFT)

#define DTRACE_LQUANTIZE_LEVELS(x)              \
        (uint16_t)(((x) & DTRACE_LQUANTIZE_LEVELMASK) >> \
        DTRACE_LQUANTIZE_LEVELSHIFT)

#define DTRACE_LQUANTIZE_BASE(x)                \
        (int32_t)(((x) & DTRACE_LQUANTIZE_BASEMASK) >> \
        DTRACE_LQUANTIZE_BASESHIFT)

#define  DTRACE_LLQUANTIZE_FACTORSHIFT          48
#define  DTRACE_LLQUANTIZE_FACTORMASK           ((uint64_t)UINT16_MAX << 48)
#define  DTRACE_LLQUANTIZE_LOWSHIFT             32
#define  DTRACE_LLQUANTIZE_LOWMASK              ((uint64_t)UINT16_MAX << 32)
#define  DTRACE_LLQUANTIZE_HIGHSHIFT            16
#define  DTRACE_LLQUANTIZE_HIGHMASK             ((uint64_t)UINT16_MAX << 16)
#define  DTRACE_LLQUANTIZE_NSTEPSHIFT           0
#define  DTRACE_LLQUANTIZE_NSTEPMASK            UINT16_MAX

#define  DTRACE_LLQUANTIZE_FACTOR(x)   \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_FACTORMASK) >> \
        DTRACE_LLQUANTIZE_FACTORSHIFT)

#define  DTRACE_LLQUANTIZE_LOW(x)    \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_LOWMASK) >> \
        DTRACE_LLQUANTIZE_LOWSHIFT)

#define  DTRACE_LLQUANTIZE_HIGH(x)   \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_HIGHMASK) >> \
        DTRACE_LLQUANTIZE_HIGHSHIFT)

#define  DTRACE_LLQUANTIZE_NSTEP(x)    \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_NSTEPMASK) >> \
        DTRACE_LLQUANTIZE_NSTEPSHIFT)

#define DTRACE_USTACK_NFRAMES(x)        (uint32_t)((x) & UINT32_MAX)
#define DTRACE_USTACK_STRSIZE(x)        (uint32_t)((x) >> 32)
#define DTRACE_USTACK_ARG(x, y)         \
        ((((uint64_t)(y)) << 32) | ((x) & UINT32_MAX))

#if !defined(__APPLE__)

#ifndef _LP64
#ifndef _LITTLE_ENDIAN
#define DTRACE_PTR(type, name)  uint32_t name##pad; type *name
#else
#define DTRACE_PTR(type, name)  type *name; uint32_t name##pad
#endif
#else
#define DTRACE_PTR(type, name)  type *name
#endif

#else

#ifndef _LP64
#define DTRACE_PTR(type, name)  user_addr_t name
#else
#define DTRACE_PTR(type, name)  type *name
#endif

#endif                

  
                             
  
                                                                             
                                                                            
                                                                          
                                                                           
                                                                            
                                                                            
                                                                       
  
                                            
  
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
  
                                                                            
                                                                             
                                                                             
                                                                            
                                                                            
                                                                              
                                                                             
  
                                                                             
                                                                              
                                                                             
                                                                             
                                                                             
                                                                             
  
                                                                              
                                                                            
                                                                             
                                                                               
                                                                               
                                                                             
                                                                              
  
                                                                         
                                                                   
                                                                            
                                                                              
  
                                                                             
                                                                            
                                                                               
   

#define DOF_ID_SIZE     16                                               

typedef struct dof_hdr {
        uint8_t dofh_ident[DOF_ID_SIZE];                                       
        uint32_t dofh_flags;                                               
        uint32_t dofh_hdrsize;                                            
        uint32_t dofh_secsize;                                               
        uint32_t dofh_secnum;                                          
        uint64_t dofh_secoff;                                               
        uint64_t dofh_loadsz;                                              
        uint64_t dofh_filesz;                                             
        uint64_t dofh_pad;                                           
} dof_hdr_t;

#define DOF_ID_MAG0     0                                       
#define DOF_ID_MAG1     1                                        
#define DOF_ID_MAG2     2                                       
#define DOF_ID_MAG3     3                                        
#define DOF_ID_MODEL    4                                       
#define DOF_ID_ENCODING 5                                          
#define DOF_ID_VERSION  6                                                      
#define DOF_ID_DIFVERS  7                                        
#define DOF_ID_DIFIREG  8                                                   
#define DOF_ID_DIFTREG  9                                                 
#define DOF_ID_PAD      10                                               

#define DOF_MAG_MAG0    0x7F                         
#define DOF_MAG_MAG1    'D'
#define DOF_MAG_MAG2    'O'
#define DOF_MAG_MAG3    'F'

#define DOF_MAG_STRING  "\177DOF"
#define DOF_MAG_STRLEN  4

#define DOF_MODEL_NONE  0                         
#define DOF_MODEL_ILP32 1
#define DOF_MODEL_LP64  2

#ifdef _LP64
#define DOF_MODEL_NATIVE        DOF_MODEL_LP64
#else
#define DOF_MODEL_NATIVE        DOF_MODEL_ILP32
#endif

#define DOF_ENCODE_NONE 0                            
#define DOF_ENCODE_LSB  1
#define DOF_ENCODE_MSB  2

#ifdef _BIG_ENDIAN
#define DOF_ENCODE_NATIVE       DOF_ENCODE_MSB
#else
#define DOF_ENCODE_NATIVE       DOF_ENCODE_LSB
#endif

#define	DOF_VERSION_1	1	                                   
#define	DOF_VERSION_2	2	                                         
#if !defined(__APPLE__)
#define	DOF_VERSION	DOF_VERSION_2	                        
#else
#define	DOF_VERSION_3	3	                                                
#define	DOF_VERSION	DOF_VERSION_3	                        
#endif                

#define DOF_FL_VALID    0                                              

typedef uint32_t dof_secidx_t;                                       
typedef uint32_t dof_stridx_t;                               

#define DOF_SECIDX_NONE (-1U)                                       
#define DOF_STRIDX_NONE (-1U)                                      

typedef struct dof_sec {
        uint32_t dofs_type;                                   
        uint32_t dofs_align;                                       
        uint32_t dofs_flags;                                
        uint32_t dofs_entsize;                                        
        uint64_t dofs_offset;                                           
        uint64_t dofs_size;                                        
} dof_sec_t;

#define	DOF_SECT_NONE		0	                  
#define	DOF_SECT_COMMENTS	1	                       
#define	DOF_SECT_SOURCE		2	                           
#define	DOF_SECT_ECBDESC	3	                   
#define	DOF_SECT_PROBEDESC	4	                     
#define	DOF_SECT_ACTDESC	5	                         
#define	DOF_SECT_DIFOHDR	6	                                     
#define	DOF_SECT_DIF		7	                                 
#define	DOF_SECT_STRTAB		8	                  
#define	DOF_SECT_VARTAB		9	                         
#define	DOF_SECT_RELTAB		10	                          
#define	DOF_SECT_TYPTAB		11	                            
#define	DOF_SECT_URELHDR	12	                                      
#define	DOF_SECT_KRELHDR	13	                                        
#define	DOF_SECT_OPTDESC	14	                         
#define	DOF_SECT_PROVIDER	15	                    
#define	DOF_SECT_PROBES		16	                       
#define	DOF_SECT_PRARGS		17	                                        
#define	DOF_SECT_PROFFS		18	                                        
#define	DOF_SECT_INTTAB		19	                    
#define	DOF_SECT_UTSNAME	20	                    
#define	DOF_SECT_XLTAB		21	                       
#define	DOF_SECT_XLMEMBERS	22	                          
#define	DOF_SECT_XLIMPORT	23	                  
#define	DOF_SECT_XLEXPORT	24	                  
#define	DOF_SECT_PREXPORT	25	                                        
#define	DOF_SECT_PRENOFFS	26	                                      

#define DOF_SECF_LOAD           1                                     

typedef struct dof_ecbdesc {
        dof_secidx_t dofe_probes;                                       
        dof_secidx_t dofe_pred;                                       
        dof_secidx_t dofe_actions;                                    
        uint32_t dofe_pad;                                           
        uint64_t dofe_uarg;                                                 
} dof_ecbdesc_t;

typedef struct dof_probedesc {
        dof_secidx_t dofp_strtab;                                            
        dof_stridx_t dofp_provider;                          
        dof_stridx_t dofp_mod;                             
        dof_stridx_t dofp_func;                              
        dof_stridx_t dofp_name;                          
        uint32_t dofp_id;                                               
} dof_probedesc_t;

typedef struct dof_actdesc {
        dof_secidx_t dofa_difo;                                       
        dof_secidx_t dofa_strtab;                                            
        uint32_t dofa_kind;                                                     
        uint32_t dofa_ntuple;                                                   
        uint64_t dofa_arg;                                          
        uint64_t dofa_uarg;                                         
} dof_actdesc_t;

typedef struct dof_difohdr {
        dtrace_diftype_t dofd_rtype;                                       
        dof_secidx_t dofd_links[1];                                           
} dof_difohdr_t;

typedef struct dof_relohdr {
        dof_secidx_t dofr_strtab;                                              
        dof_secidx_t dofr_relsec;                                              
        dof_secidx_t dofr_tgtsec;                                              
} dof_relohdr_t;

typedef struct dof_relodesc {
        dof_stridx_t dofr_name;                                               
        uint32_t dofr_type;                                                  
        uint64_t dofr_offset;                                           
        uint64_t dofr_data;                                                
} dof_relodesc_t;

#define DOF_RELO_NONE   0                                           
#define DOF_RELO_SETX   1                                        

typedef struct dof_optdesc {
        uint32_t dofo_option;                                  
        dof_secidx_t dofo_strtab;                                           
        uint64_t dofo_value;                                              
} dof_optdesc_t;

typedef uint32_t dof_attr_t;                                              

#define DOF_ATTR(n, d, c)       (((n) << 24) | ((d) << 16) | ((c) << 8))
#define DOF_ATTR_NAME(a)        (((a) >> 24) & 0xff)
#define DOF_ATTR_DATA(a)        (((a) >> 16) & 0xff)
#define DOF_ATTR_CLASS(a)       (((a) >>  8) & 0xff)

typedef struct dof_provider {
	dof_secidx_t dofpv_strtab;	                                     
	dof_secidx_t dofpv_probes;	                                     
	dof_secidx_t dofpv_prargs;	                                     
	dof_secidx_t dofpv_proffs;	                                     
	dof_stridx_t dofpv_name;	                          
	dof_attr_t dofpv_provattr;	                         
	dof_attr_t dofpv_modattr;	                       
	dof_attr_t dofpv_funcattr;	                         
	dof_attr_t dofpv_nameattr;	                     
	dof_attr_t dofpv_argsattr;	                     
	dof_secidx_t dofpv_prenoffs;	                                       
} dof_provider_t;

typedef struct dof_probe {
	uint64_t dofpr_addr;		                                  
	dof_stridx_t dofpr_func;	                           
	dof_stridx_t dofpr_name;	                       
	dof_stridx_t dofpr_nargv;	                                  
	dof_stridx_t dofpr_xargv;	                                      
	uint32_t dofpr_argidx;		                                     
	uint32_t dofpr_offidx;		                                 
	uint8_t dofpr_nargc;		                           
	uint8_t dofpr_xargc;		                               
	uint16_t dofpr_noffs;		                                        
	uint32_t dofpr_enoffidx;	                                      
	uint16_t dofpr_nenoffs;		                                  
	uint16_t dofpr_pad1;		                             
	uint32_t dofpr_pad2;		                             
} dof_probe_t;

typedef struct dof_xlator {
        dof_secidx_t dofxl_members;                                             
        dof_secidx_t dofxl_strtab;                                           
        dof_stridx_t dofxl_argv;                                          
        uint32_t dofxl_argc;                                             
        dof_stridx_t dofxl_type;                                     
        dof_attr_t dofxl_attr;                                           
} dof_xlator_t;

typedef struct dof_xlmember {
        dof_secidx_t dofxm_difo;                                             
        dof_stridx_t dofxm_name;                         
        dtrace_diftype_t dofxm_type;                     
} dof_xlmember_t;

typedef struct dof_xlref {
        dof_secidx_t dofxr_xlator;                                            
        uint32_t dofxr_member;                                                
        uint32_t dofxr_argn;                                                    
} dof_xlref_t;

  
                                           
  
                                                                          
                                                                          
                                                                     
                                                                            
                                                                             
                                                                              
                                                                          
                                                                            
                                                                            
                                                                          
   
typedef struct dtrace_difo {
        dif_instr_t *dtdo_buf;                                  
        uint64_t *dtdo_inttab;                                        
        char *dtdo_strtab;                                           
        dtrace_difv_t *dtdo_vartab;                                    
        uint_t dtdo_len;                                                  
        uint_t dtdo_intlen;                                          
        uint_t dtdo_strlen;                                         
        uint_t dtdo_varlen;                                           
        dtrace_diftype_t dtdo_rtype;                     
        uint_t dtdo_refcnt;                                        
        uint_t dtdo_destructive;                                             
#ifndef _KERNEL
        dof_relodesc_t *dtdo_kreltab;                           
        dof_relodesc_t *dtdo_ureltab;                         
        struct dt_node **dtdo_xlmtab;                              
        uint_t dtdo_krelen;                                        
        uint_t dtdo_urelen;                                        
        uint_t dtdo_xlmlen;                                             
#endif
} dtrace_difo_t;

  
                                         
  
                                                                              
                                                                     
                                                                       
                                                             
                                                                      
                                                                              
                                                                           
                                                                      
                                                                             
                                                                            
                                                                        
                                                                      
                                                                               
           
   
struct dtrace_predicate;

typedef struct dtrace_probedesc {
        dtrace_id_t dtpd_id;                                          
        char dtpd_provider[DTRACE_PROVNAMELEN];                          
        char dtpd_mod[DTRACE_MODNAMELEN];                              
        char dtpd_func[DTRACE_FUNCNAMELEN];                              
        char dtpd_name[DTRACE_NAMELEN];                         
} dtrace_probedesc_t;

typedef struct dtrace_repldesc {
        dtrace_probedesc_t dtrpd_match;                                    
        dtrace_probedesc_t dtrpd_create;                                    
} dtrace_repldesc_t;

typedef struct dtrace_preddesc {
        dtrace_difo_t *dtpdd_difo;                                         
        struct dtrace_predicate *dtpdd_predicate;                           
} dtrace_preddesc_t;

typedef struct dtrace_actdesc {
        dtrace_difo_t *dtad_difo;                                          
        struct dtrace_actdesc *dtad_next;                        
        dtrace_actkind_t dtad_kind;                                 
        uint32_t dtad_ntuple;                                        
        uint64_t dtad_arg;                                           
        uint64_t dtad_uarg;                                        
        int dtad_refcnt;                                             
} dtrace_actdesc_t;


typedef struct dtrace_ecbdesc {
        dtrace_actdesc_t *dted_action;                                     
        dtrace_preddesc_t dted_pred;                                       
        dtrace_probedesc_t dted_probe;                                 
        uint64_t dted_uarg;                                           
        int dted_refcnt;                                             
        uint64_t dted_probegen;                                               
} dtrace_ecbdesc_t;

  
                                                                  
                                                                        
   

  
                                         
  
                                                                             
                                                                            
                                                                           
                                                                              
                                                                           
                                                                          
                                                                              
                                                                              
                                                                            
                                                                               
                                                                            
                                                                             
                                                            
   
typedef struct dtrace_recdesc {
        dtrace_actkind_t dtrd_action;                               
        uint32_t dtrd_size;                                         
        uint32_t dtrd_offset;                                             
        uint16_t dtrd_alignment;                                        
        uint16_t dtrd_format;                                       
        uint64_t dtrd_arg;                                           
        uint64_t dtrd_uarg;                                        
} dtrace_recdesc_t;

typedef struct dtrace_eprobedesc {
        dtrace_epid_t dtepd_epid;                                     
        dtrace_id_t dtepd_probeid;                            
        uint64_t dtepd_uarg;                                          
        uint32_t dtepd_size;                                    
        int dtepd_nrecs;                                               
        dtrace_recdesc_t dtepd_rec[1];                                  
} dtrace_eprobedesc_t;

typedef struct dtrace_aggdesc {
	DTRACE_PTR(char, dtagd_name);		                             
	dtrace_aggvarid_t dtagd_varid;		                             
	int dtagd_flags;			                             
	dtrace_aggid_t dtagd_id;		                    
	dtrace_epid_t dtagd_epid;		                      
	uint32_t dtagd_size;			                   
	int dtagd_nrecs;			                       
	uint32_t dtagd_pad;			                      
	dtrace_recdesc_t dtagd_rec[1];		                         
} dtrace_aggdesc_t;

typedef struct dtrace_fmtdesc {
        DTRACE_PTR(char, dtfd_string);                             
        int dtfd_length;                                                     
        uint16_t dtfd_format;                                          
} dtrace_fmtdesc_t;

#define DTRACE_SIZEOF_EPROBEDESC(desc)                          \
        (sizeof (dtrace_eprobedesc_t) + ((desc)->dtepd_nrecs ?  \
        (((desc)->dtepd_nrecs - 1) * sizeof (dtrace_recdesc_t)) : 0))

#define DTRACE_SIZEOF_AGGDESC(desc)                             \
        (sizeof (dtrace_aggdesc_t) + ((desc)->dtagd_nrecs ?     \
        (((desc)->dtagd_nrecs - 1) * sizeof (dtrace_recdesc_t)) : 0))

  
                          
  
                                                                              
                                                                               
                                                                              
                                                                             
                                                                              
                                                                              
                                                                             
                                                            
                              
   
#define	DTRACEOPT_BUFSIZE	0	                 
#define	DTRACEOPT_BUFPOLICY	1	                   
#define	DTRACEOPT_DYNVARSIZE	2	                           
#define	DTRACEOPT_AGGSIZE	3	                      
#define	DTRACEOPT_SPECSIZE	4	                      
#define	DTRACEOPT_NSPEC		5	                            
#define	DTRACEOPT_STRSIZE	6	                 
#define	DTRACEOPT_CLEANRATE	7	                          
#define	DTRACEOPT_CPU		8	                  
#define	DTRACEOPT_BUFRESIZE	9	                            
#define	DTRACEOPT_GRABANON	10	                                  
#define	DTRACEOPT_FLOWINDENT	11	                                  
#define	DTRACEOPT_QUIET		12	                                        
#define	DTRACEOPT_STACKFRAMES	13	                            
#define	DTRACEOPT_USTACKFRAMES	14	                                 
#define	DTRACEOPT_AGGRATE	15	                               
#define	DTRACEOPT_SWITCHRATE	16	                           
#define	DTRACEOPT_STATUSRATE	17	                 
#define	DTRACEOPT_DESTRUCTIVE	18	                                 
#define	DTRACEOPT_STACKINDENT	19	                                    
#define	DTRACEOPT_RAWBYTES	20	                                    
#define	DTRACEOPT_JSTACKFRAMES	21	                               
#define	DTRACEOPT_JSTACKSTRSIZE	22	                                   
#define	DTRACEOPT_AGGSORTKEY	23	                              
#define	DTRACEOPT_AGGSORTREV	24	                               
#define	DTRACEOPT_AGGSORTPOS	25	                              
#define	DTRACEOPT_AGGSORTKEYPOS	26	                                  
#define	DTRACEOPT_AGGHIST	27 	                                  
#define	DTRACEOPT_AGGPACK	28 	                               
#define	DTRACEOPT_AGGZOOM	29 	                                
#define	DTRACEOPT_TEMPORAL	30	                               
#if !defined(__APPLE__)
#define DTRACEOPT_MAX           31                             
#else
#define DTRACEOPT_STACKSYMBOLS  31                                                
#define DTRACEOPT_BUFLIMIT      32	                                             
#define DTRACEOPT_MAX           33                             
#endif                

#define	DTRACEOPT_UNSET		(dtrace_optval_t)-2	                  

#define	DTRACEOPT_BUFPOLICY_RING	0	                 
#define	DTRACEOPT_BUFPOLICY_FILL	1	                            
#define	DTRACEOPT_BUFPOLICY_SWITCH	2	                    

#define DTRACEOPT_BUFRESIZE_AUTO        0                               
#define DTRACEOPT_BUFRESIZE_MANUAL      1                            

  
                          
  
                                                                     
                                                                               
                                                                        
                                                                    
                                                                          
                                                                               
                                                                          
                                
  
                                                                      
                                                                         
                                                                              
                                                                      
   
typedef struct dtrace_bufdesc {
        uint64_t dtbd_size;                                         
        uint32_t dtbd_cpu;                                                
        uint32_t dtbd_errors;                                         
        uint64_t dtbd_drops;                                         
        DTRACE_PTR(char, dtbd_data);                      
        uint64_t dtbd_oldest;                                                
	uint64_t dtbd_timestamp;		                        
} dtrace_bufdesc_t;

  
                                                                           
                                                                          
                                              
   
typedef struct dtrace_rechdr {
	dtrace_epid_t dtrh_epid;		                      
	uint32_t dtrh_timestamp_hi;		                           
	uint32_t dtrh_timestamp_lo;		                          
} dtrace_rechdr_t;

#define	DTRACE_RECORD_LOAD_TIMESTAMP(dtrh)			\
	((dtrh)->dtrh_timestamp_lo +				\
	((uint64_t)(dtrh)->dtrh_timestamp_hi << 32))

#define	DTRACE_RECORD_STORE_TIMESTAMP(dtrh, hrtime) {		\
	(dtrh)->dtrh_timestamp_lo = (uint32_t)hrtime;		\
	(dtrh)->dtrh_timestamp_hi = hrtime >> 32;		\
}

  
                
  
                                                                         
                                                                          
                                                                            
                                                                               
                                                                             
                                                                         
                                                                             
                                                                            
                                                                          
                                                                             
                                                                          
                                                                         
   
typedef struct dtrace_status {
        uint64_t dtst_dyndrops;                                    
        uint64_t dtst_dyndrops_rinsing;                                       
        uint64_t dtst_dyndrops_dirty;                                       
        uint64_t dtst_specdrops;                                       
        uint64_t dtst_specdrops_busy;                                       
        uint64_t dtst_specdrops_unavail;                                       
        uint64_t dtst_errors;                                     
        uint64_t dtst_filled;                                              
        uint64_t dtst_stkstroverflows;                                          
        uint64_t dtst_dblerrors;                                            
        char dtst_killed;                                               
        char dtst_exiting;                                                     
        char dtst_pad[6];                                                    
} dtrace_status_t;

  
                       
  
                                                                       
                                                                       
                                          
   
typedef struct dtrace_conf {
        uint_t dtc_difversion;                                             
        uint_t dtc_difintregs;                                                  
        uint_t dtc_diftupregs;                                                
        uint_t dtc_ctfmodel;                                        
        uint_t dtc_pad[8];                                                   
} dtrace_conf_t;

  
                
  
                                                                          
                                                                   
                                                                         
                                                                              
                                                  
   
#define DTRACEFLT_UNKNOWN               0                          
#define DTRACEFLT_BADADDR               1                        
#define DTRACEFLT_BADALIGN              2                          
#define DTRACEFLT_ILLOP                 3                              
#define DTRACEFLT_DIVZERO               4                           
#define DTRACEFLT_NOSCRATCH             5                                 
#define DTRACEFLT_KPRIV                 6                                  
#define DTRACEFLT_UPRIV                 7                                
#define DTRACEFLT_TUPOFLOW              8                                 
#define	DTRACEFLT_BADSTACK		9	               

#define DTRACEFLT_LIBRARY               1000                             

  
                        
  
                                                                           
                                                                    
                                                                           
                                                                            
                                                                             
                                                                      
                                                                             
                                                                             
                                                                             
                                                                           
                                                                             
                                        
   
typedef struct dtrace_argdesc {
        dtrace_id_t dtargd_id;                                        
        int dtargd_ndx;                                                       
        int dtargd_mapping;                                        
        char dtargd_native[DTRACE_ARGTYPELEN];                        
        char dtargd_xlate[DTRACE_ARGTYPELEN];                             
} dtrace_argdesc_t;

  
                              
  
                                                                             
                                                                           
                                                                             
                                                                       
   
typedef uint8_t dtrace_stability_t;                                             
typedef uint8_t dtrace_class_t;                                             

#define	DTRACE_STABILITY_INTERNAL	0	                              
#define	DTRACE_STABILITY_PRIVATE	1	                               
#define	DTRACE_STABILITY_OBSOLETE	2	                           
#define	DTRACE_STABILITY_EXTERNAL	3	                           
#define	DTRACE_STABILITY_UNSTABLE	4	                             
#define	DTRACE_STABILITY_EVOLVING	5	                           
#define	DTRACE_STABILITY_STABLE		6	                               
#define	DTRACE_STABILITY_STANDARD	7	                       
#define	DTRACE_STABILITY_MAX		7	                             

#define	DTRACE_CLASS_UNKNOWN	0	                                      
#define	DTRACE_CLASS_CPU	1	                         
#define	DTRACE_CLASS_PLATFORM	2	                                  
#define	DTRACE_CLASS_GROUP	3	                                        
#define	DTRACE_CLASS_ISA	4	                             
#define	DTRACE_CLASS_COMMON	5	                           
#define	DTRACE_CLASS_MAX	5	                         

#define	DTRACE_PRIV_NONE	0x0000
#define	DTRACE_PRIV_KERNEL	0x0001
#define	DTRACE_PRIV_USER	0x0002
#define	DTRACE_PRIV_PROC	0x0004
#define	DTRACE_PRIV_OWNER	0x0008
#define	DTRACE_PRIV_ZONEOWNER	0x0010

#define	DTRACE_PRIV_ALL	\
	(DTRACE_PRIV_KERNEL | DTRACE_PRIV_USER | \
	DTRACE_PRIV_PROC | DTRACE_PRIV_OWNER | DTRACE_PRIV_ZONEOWNER)

typedef struct dtrace_ppriv {
	uint32_t dtpp_flags;			                     
	uid_t dtpp_uid;				             
	zoneid_t dtpp_zoneid;			             
} dtrace_ppriv_t;

typedef struct dtrace_attribute {
        dtrace_stability_t dtat_name;                                      
        dtrace_stability_t dtat_data;                                      
        dtrace_class_t dtat_class;                                          
} dtrace_attribute_t;

typedef struct dtrace_pattr {
        dtrace_attribute_t dtpa_provider;                                
        dtrace_attribute_t dtpa_mod;                                   
        dtrace_attribute_t dtpa_func;                                    
        dtrace_attribute_t dtpa_name;                                
        dtrace_attribute_t dtpa_args;                                  
} dtrace_pattr_t;

typedef struct dtrace_providerdesc {
        char dtvd_name[DTRACE_PROVNAMELEN];                        
        dtrace_pattr_t dtvd_attr;                                         
        dtrace_ppriv_t dtvd_priv;                                        
} dtrace_providerdesc_t;

  
                                
  
                                                                         
                                                                           
          
   
#if !defined(__APPLE__)
#define DTRACEIOC               (('d' << 24) | ('t' << 16) | ('r' << 8))
#define DTRACEIOC_PROVIDER      (DTRACEIOC | 1)                             
#define DTRACEIOC_PROBES        (DTRACEIOC | 2)                          
#define DTRACEIOC_BUFSNAP       (DTRACEIOC | 4)                              
#define DTRACEIOC_PROBEMATCH    (DTRACEIOC | 5)                           
#define DTRACEIOC_ENABLE        (DTRACEIOC | 6)                            
#define DTRACEIOC_AGGSNAP       (DTRACEIOC | 7)                            
#define DTRACEIOC_EPROBE        (DTRACEIOC | 8)                               
#define DTRACEIOC_PROBEARG      (DTRACEIOC | 9)                            
#define DTRACEIOC_CONF          (DTRACEIOC | 10)                         
#define DTRACEIOC_STATUS        (DTRACEIOC | 11)                        
#define DTRACEIOC_GO            (DTRACEIOC | 12)                           
#define DTRACEIOC_STOP          (DTRACEIOC | 13)                          
#define DTRACEIOC_AGGDESC       (DTRACEIOC | 15)                            
#define DTRACEIOC_FORMAT        (DTRACEIOC | 16)                            
#define DTRACEIOC_DOFGET        (DTRACEIOC | 17)                     
#define DTRACEIOC_REPLICATE     (DTRACEIOC | 18)                            
#else
                                                                                 
#define DTRACEIOC               _IO('d',0)
#define DTRACEIOC_PROVIDER      (DTRACEIOC | 1)                             
#define DTRACEIOC_PROBES        (DTRACEIOC | 2)                          
#define DTRACEIOC_BUFSNAP       (DTRACEIOC | 4)                              
#define DTRACEIOC_PROBEMATCH    (DTRACEIOC | 5)                           
#define DTRACEIOC_ENABLE        (DTRACEIOC | 6)                            
#define DTRACEIOC_AGGSNAP       (DTRACEIOC | 7)                            
#define DTRACEIOC_EPROBE        (DTRACEIOC | 8)                               
#define DTRACEIOC_PROBEARG      (DTRACEIOC | 9)                            
#define DTRACEIOC_CONF          (DTRACEIOC | 10)                         
#define DTRACEIOC_STATUS        (DTRACEIOC | 11)                        
#define DTRACEIOC_GO            (DTRACEIOC | 12)                           
#define DTRACEIOC_STOP          (DTRACEIOC | 13)                          
#define DTRACEIOC_AGGDESC       (DTRACEIOC | 15)                            
#define DTRACEIOC_FORMAT        (DTRACEIOC | 16)                            
#define DTRACEIOC_DOFGET        (DTRACEIOC | 17)                     
#define DTRACEIOC_REPLICATE     (DTRACEIOC | 18)                            
#define DTRACEIOC_MODUUIDSLIST	(DTRACEIOC | 30)	                                                        
#define DTRACEIOC_PROVMODSYMS	(DTRACEIOC | 31)	                                                            
#define DTRACEIOC_PROCWAITFOR	(DTRACEIOC | 32)	                                       
#define DTRACEIOC_SLEEP 	(DTRACEIOC | 33)	                       
#define DTRACEIOC_SIGNAL	(DTRACEIOC | 34)	                                         

  
                                                                                             
   

typedef struct dtrace_symbol {
	uint64_t	dtsym_addr;			                           
	uint64_t	dtsym_size;			                                                                                                        
	char 		dtsym_name[DTRACE_FUNCNAMELEN];	                 
} dtrace_symbol_t;

typedef struct dtrace_module_symbols {
	UUID		dtmodsyms_uuid;
	uint64_t	dtmodsyms_count;
	dtrace_symbol_t	dtmodsyms_symbols[1];
} dtrace_module_symbols_t;

#define DTRACE_MODULE_SYMBOLS_SIZE(count) (sizeof(dtrace_module_symbols_t) + ((count - 1) * sizeof(dtrace_symbol_t)))
#define DTRACE_MODULE_SYMBOLS_COUNT(size) ((size - sizeof(dtrace_module_symbols_t)) / sizeof(dtrace_symbol_t) + 1)

typedef struct dtrace_module_uuids_list {
	uint64_t	dtmul_count;
	UUID		dtmul_uuid[1];
} dtrace_module_uuids_list_t;

#define DTRACE_MODULE_UUIDS_LIST_SIZE(count) (sizeof(dtrace_module_uuids_list_t) + ((count - 1) * sizeof(UUID)))

typedef struct dtrace_procdesc {
	                                     
	char		p_name[128];
	                                   
	int		p_name_length;
	pid_t		p_pid;
} dtrace_procdesc_t;

   
                       
                                                                           
                                                                               
                     
   
#define DTRACE_WAKE_TIMEOUT 0                                                 
#define DTRACE_WAKE_BUF_LIMIT 1                                                           

#endif                

  
                 
  
                                                                          
                                                                              
                                                                            
                                                                             
                                                                        
                                                                             
                                                                              
                                                                             
                                                                              
                                                                          
                                                                              
                                                                            
                                                                            
                                                                             
                                                                         
         
  
                                                                              
                                                                              
                                                                          
                                                                        
                                                                             
                                                                            
                                                                          
                                                                           
                                                                             
                                                                             
                                                                          
                                                                           
                                                                             
                                                            
  
                                                                           
                                                     
                                                                            
                                                                        
                                                                          
  
                                                                   
  
                                                
                                                
                                                 
                                                
                                      
                                       
                                              
                                        
  
                                                                            
                                                                        
                                                                             
                                                                      
                                                                           
                                                                            
                                                                            
                                                                          
                                                                            
                               
  
                                                                             
                                                                           
                                                                      
                                                                      
                                                                       
                                                        
  
                                                                        
                                                                          
                     
   
#if !defined(__APPLE__)
#define DTRACEHIOC              (('d' << 24) | ('t' << 16) | ('h' << 8))
#define DTRACEHIOC_ADD          (DTRACEHIOC | 1)                        
#define DTRACEHIOC_REMOVE       (DTRACEHIOC | 2)                           
#define DTRACEHIOC_ADDDOF       (DTRACEHIOC | 3)                            
#else
#define DTRACEHIOC_REMOVE	_IO('h', 2)			                   
#define DTRACEHIOC_ADDDOF	_IOW('h', 4, user_addr_t)	                    
#endif                

typedef struct dof_helper {
        char dofhp_mod[DTRACE_MODNAMELEN];                                      
        uint64_t dofhp_addr;                                                
        uint64_t dofhp_dof;                                                
} dof_helper_t;

#if defined(__APPLE__)
  
                                                                  
                                                              
                                                                 
                
  
                                                                  
   
typedef struct dof_ioctl_data {
	  
                                                             
                                                         
    
	uint64_t dofiod_count;
	dof_helper_t dofiod_helpers[1];
} dof_ioctl_data_t;

#define DOF_IOCTL_DATA_T_SIZE(count) (sizeof(dof_ioctl_data_t) + ((count - 1) * sizeof(dof_helper_t)))

#endif

#define DTRACEMNR_DTRACE        "dtrace"                                 
#if !defined(__APPLE__)
#define	DTRACEMNR_HELPER	"helper"	                      
#else
#define DTRACEMNR_HELPER        "dtracehelper"                        
#endif                
#define DTRACEMNRN_DTRACE       0                                         
#define DTRACEMNRN_HELPER       1                                      
#define DTRACEMNRN_CLONE        2                                      

#ifdef _KERNEL

  
                      
  
                                                                          
                                                                           
                                                                         
                                                                             
  
                                                                            
                                                               
  
                              
  
                
  
                                                                            
                                                                           
                                               
  
                                                                
                                                                       
                                                       
                                                        
                                                        
                                                       
                                                                         
                                                                              
                                                                             
                                                                             
  
                                                                    
  
                  
  
                                                                            
                                                                              
                                                                             
                                                                         
                           
  
                             
  
                                                                          
                                                                              
                                                                               
                                                                            
                                                                             
                                                                              
                                                                               
                                                                     
                                                        
  
                      
  
          
  
                          
  
                                                                               
                                                                              
                                                                               
                                                                                
                                                                     
                                                                              
                                                                          
                                                                             
                            
  
                                                              
  
                  
  
                                                                          
                      
  
                             
  
                                                                          
                                                                          
                                               
  
                      
  
          
  
                          
  
                                                                            
                                                                             
                                                                        
                                                                           
                                                                           
                                                                     
                                                                                
                                                                          
                                                                                
                                                                 
                                                                            
           
  
                                                              
  
                  
  
                                          
  
                             
  
                                                                          
                                                                             
                                                                       
                                                                         
                                                                             
                                                                          
                                                                           
                                                                               
                                                                             
                   
  
                      
  
                                                                        
              
  
                          
  
                                                                           
                                                                           
                 
  
                                                                
  
                  
  
                                           
  
                             
  
                                                                          
                                                                              
                                                                       
                                                                              
                                                                            
                                                                    
  
                      
  
          
  
                          
  
                                                                           
                                                                           
                 
  
                                                                
  
                  
  
                                                                            
                                                                             
                                                                         
                              
  
                             
  
                                                                          
                                                                         
                                                                             
                                                                          
                                                                            
                                    
  
                      
  
          
  
                          
  
                                                                               
                                                                        
                                                                          
                                                                            
            
  
                                                               
  
                  
  
                                                                           
                                                                            
                                                                  
  
                             
  
                                                                          
                                                                       
                                                                             
                                                                         
                                                                           
                                   
  
                      
  
          
  
                          
  
                                                                               
                                                                        
                                                                         
                                                                            
            
  
                                                                   
                                    
  
                  
  
                                                                         
  
                             
  
                                                                         
                                                                      
                                                                           
                                                                    
                                                                        
                                                                       
                                                                           
                                                                             
                       
  
                      
  
                                                                          
                                                                     
  
                          
  
                                                                            
                                                                        
                                                                           
                                                                           
                                                                     
                       
  
                                                                      
                                        
  
                  
  
                                                                
  
                             
  
                                                                         
                                                                      
                                                                           
                                                                           
                                                                            
                                                                     
                                                                              
                                                                           
                                                                           
                      
  
                      
  
                               
  
                          
  
                                                                      
                                                                    
  
                                                                 
  
                   
  
                                                                  
  
                              
  
                                                                         
                                                                      
                                                                             
                                                                           
                                                                         
                                     
  
                       
  
                     
  
                           
  
                                                                      
                                                                    
  
                                                                
  
                  
  
                                           
  
                             
  
                                                                          
                                                                               
                                                                            
                                                                             
                                                                              
                                                                              
                                                                             
                               
  
                      
  
          
  
                          
  
                                                                           
                                                                               
              
  
  
                              
  
                
  
                                                                           
                                                                         
                                                                           
               
  
                                                                              
                                                                        
                                                                  
                                                                     
                                                                             
                                                      
                                                                        
                                                                               
                                                         
  
                                                                        
                                                                            
                                      
  
                  
  
                                                                     
                                                                           
                            
  
                             
  
                                                                              
                                                                              
                                                                              
  
                                                                             
  
                                                                              
                                                               
  
                                                                              
                                                               
  
                                                                            
                                                                   
  
                                                                           
                                                                             
                                                                             
                                                                           
                                                                    
  
                                                                           
                                                                             
                                                                             
                                                                           
                                                                   
  
                                                                        
                                                         
  
                                                                      
                                                                         
                                                                          
                                                                        
                                                                           
                                                                           
                 
  
                                                                              
                                                                           
                                                                           
                                                                            
                                                                            
                                                                          
                                                                            
                              
  
                                                                             
                                                                            
               
  
                                                                 
                                                                              
                                                                             
              
  
                      
  
                                                                          
                                                                            
                                        
  
                                                                               
                                                                            
                                                                        
                                                                      
  
                                       
  
                          
  
                                                                              
                                                                        
                                                               
  
                                                   
  
                  
  
                                                                             
                                                                             
  
                             
  
                                                                     
                                                                  
                                                                               
                                                                               
                                                                               
                                   
  
                      
  
                                                                              
                      
  
                                                                               
                                                                           
                             
  
                                       
  
                          
  
                                                                       
                                                                       
                                                                        
                                                                     
  
                                                       
  
                  
  
                                                                              
                                                                      
  
                            
  
                                                                     
                                                                           
                                                                             
                                                                           
                                                                           
                                                                          
                       
  
                                                   
  
                  
  
                                                                              
                                                                      
                                                              
  
                             
  
                                                                              
                                                                             
                                                                            
                                                                           
                      
  
                      
  
                                                                               
                                                                            
                          
  
                          
  
                                                                         
                                                                            
                                    
  
                            
  
                  
  
                                                  
  
                             
  
                                                                          
                                                                          
                                                                       
                                                                           
                                                                            
                                                                              
                                                                         
  
                      
  
                                                                     
  
                                                                      
                                                                       
  
                  
  
                                                                         
  
                             
  
                                                                      
                                                                         
                                                                  
                                                                            
                                                                               
                                                                           
            
  
                                                                             
                                                                               
                                                                            
                                                                             
                                                                            
          
  
                                                                           
                                                                    
                                         
  
                                                                         
                                                                               
                                                                              
                                   
  
                      
  
                                                                             
                                
  
                          
  
                                                                        
                                                                             
                                                                     
  
                                                                              
                                               
  
                  
  
                                                                        
                                  
  
                             
  
                                                                      
                                                                         
                                                                  
                                                                               
                                                                        
                                                                
                                                                          
                                                                        
  
                      
  
                                                                               
                          
  
                          
  
                                                                        
                                                                            
                                                                           
  
                                                                       
  
                  
  
                                                                    
  
                             
  
                                                                      
                                                                          
                                                          
                                                                   
                                                                           
                                                                            
                                            
  
                      
  
                                                                      
                                                                              
                                                 
  
                          
  
                                                                     
                                                                            
                                                                           
  
                                                                             
                                                               
  
                   
  
                                                                            
               
  
                              
  
                                                            
                                                                              
                                                                            
                    
  
                                                                              
                                     
  
                       
  
          
  
                           
  
                                                                          
                                                                              
                                                                             
                                                                        
                                                                        
                                                                       
                                                                          
                                                                           
                                                                              
                                                                           
                                                                      
                                                                             
              
   
typedef struct dtrace_pops {
        void (*dtps_provide)(void *arg, const dtrace_probedesc_t *spec);
        void (*dtps_provide_module)(void *arg, struct modctl *mp);
        int (*dtps_enable)(void *arg, dtrace_id_t id, void *parg);
        void (*dtps_disable)(void *arg, dtrace_id_t id, void *parg);
        void (*dtps_suspend)(void *arg, dtrace_id_t id, void *parg);
        void (*dtps_resume)(void *arg, dtrace_id_t id, void *parg);
        void (*dtps_getargdesc)(void *arg, dtrace_id_t id, void *parg,
            dtrace_argdesc_t *desc);
        uint64_t (*dtps_getargval)(void *arg, dtrace_id_t id, void *parg,
            int argno, int aframes);
        int (*dtps_usermode)(void *arg, dtrace_id_t id, void *parg);
        void (*dtps_destroy)(void *arg, dtrace_id_t id, void *parg);
} dtrace_pops_t;

typedef uintptr_t       dtrace_provider_id_t;

extern int dtrace_register(const char *, const dtrace_pattr_t *, uint32_t,
    cred_t *, const dtrace_pops_t *, void *, dtrace_provider_id_t *);
extern int dtrace_unregister(dtrace_provider_id_t);
extern int dtrace_condense(dtrace_provider_id_t);
extern void dtrace_invalidate(dtrace_provider_id_t);
extern dtrace_id_t dtrace_probe_lookup(dtrace_provider_id_t, const char *,
    const char *, const char *);
extern dtrace_id_t dtrace_probe_create(dtrace_provider_id_t, const char *,
    const char *, const char *, int, void *);
extern void *dtrace_probe_arg(dtrace_provider_id_t, dtrace_id_t);
#if !defined(__APPLE__)
extern void dtrace_probe(dtrace_id_t, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);
#else
extern void dtrace_probe(dtrace_id_t, uint64_t arg0, uint64_t arg1,
    uint64_t arg2, uint64_t arg3, uint64_t arg4);
#endif                

  
                           
  
                                                                          
                                                                        
                                                                     
                                                                     
                                                                      
                                                                       
                                                                        
                             
  
                          
  
               
  
                                                                            
                                                                            
                                                                     
  
                                                                         
                                                                              
                                                                         
  
                                                     
                                                   
  
                  
  
                                                                       
                                   
  
                             
  
                                                                          
                                                                            
                                                                             
                                              
  
                      
  
         
  
                          
  
                                                                              
                                                                             
                                                                          
                                                                                
                                                                          
                                                          
  
                                                                         
                           
  
                  
  
                                                                           
                                                                      
                                                                           
                               
  
                             
  
                                                                          
                                                                       
                                                                      
                                                                         
                                                                           
                                                                     
                            
  
                      
  
                                                                        
                                                        
  
                          
  
                                                                              
                                                                             
                                                                          
                                                                             
                                                                 
                                                                       
  
                                                                       
                         
  
                  
  
                                                                            
                                                                       
                                                                      
                                                                            
                                       
  
                             
  
                                                                          
                                                                          
                                                                       
                                       
  
                      
  
         
  
                          
  
                                                                        
                                                                             
                                                                          
                                                                             
                                                                
                                                                         
   
typedef struct dtrace_helper_probedesc {
	char *dthpb_mod;			                  
	char *dthpb_func; 			                    
	char *dthpb_name; 			                
	uint64_t dthpb_base;			                  
#if !defined(__APPLE__)
	uint32_t *dthpb_offs;			                   
	uint32_t *dthpb_enoffs;			                              
#else
	int32_t *dthpb_offs;			                            
	int32_t *dthpb_enoffs;			                                       
#endif
	uint32_t dthpb_noffs;			                   
	uint32_t dthpb_nenoffs;			                              
	uint8_t *dthpb_args;			                            
	uint8_t dthpb_xargc;			                               
	uint8_t dthpb_nargc;			                           
	char *dthpb_xtypes;			                              
	char *dthpb_ntypes;			                          
} dtrace_helper_probedesc_t;

typedef struct dtrace_helper_provdesc {
        char *dthpv_provname;                                      
        dtrace_pattr_t dthpv_pattr;                                       
} dtrace_helper_provdesc_t;

  
                                                                     
                                                                    
                                
  
                                                                      
   

typedef struct dtrace_mops {
        void (*dtms_create_probe)(void *, void *, dtrace_helper_probedesc_t *);
        void *(*dtms_provide_proc)(void *, dtrace_helper_provdesc_t *, proc_t*);
        void (*dtms_remove_proc)(void *, dtrace_helper_provdesc_t *, proc_t*);
        char* (*dtms_provider_name)(void *);
} dtrace_mops_t;

typedef uintptr_t       dtrace_meta_provider_id_t;

extern int dtrace_meta_register(const char *, const dtrace_mops_t *, void *,
    dtrace_meta_provider_id_t *);
extern int dtrace_meta_unregister(dtrace_meta_provider_id_t);

  
                      
  
                                                                               
                                                                              
                                                                      
                                                                           
   

typedef enum dtrace_vtime_state {
        DTRACE_VTIME_INACTIVE = 0,                             
        DTRACE_VTIME_ACTIVE,                                             
        DTRACE_VTIME_INACTIVE_TNF,                                 
        DTRACE_VTIME_ACTIVE_TNF                                            
} dtrace_vtime_state_t;

extern dtrace_vtime_state_t dtrace_vtime_active;
extern void dtrace_vtime_switch(kthread_t *next);
extern void dtrace_vtime_enable_tnf(void);
extern void dtrace_vtime_disable_tnf(void);
extern void dtrace_vtime_enable(void);
extern void dtrace_vtime_disable(void);

#if !defined(__APPLE__)
struct regs;

extern int (*dtrace_pid_probe_ptr)(struct regs *);
extern int (*dtrace_return_probe_ptr)(struct regs *);
#else
#if defined (__i386__) || defined(__x86_64__)
extern int (*dtrace_pid_probe_ptr)(x86_saved_state_t *regs);
extern int (*dtrace_return_probe_ptr)(x86_saved_state_t* regs);
#elif defined (__arm__) || defined(__arm64__)
extern int (*dtrace_pid_probe_ptr)(arm_saved_state_t *regs);
extern int (*dtrace_return_probe_ptr)(arm_saved_state_t *regs);
#else
#error architecture not supported
#endif
#endif                
extern void (*dtrace_fasttrap_fork_ptr)(proc_t *, proc_t *);
extern void (*dtrace_fasttrap_exec_ptr)(proc_t *);
extern void (*dtrace_fasttrap_exit_ptr)(proc_t *);
extern void dtrace_fasttrap_fork(proc_t *, proc_t *);

typedef uintptr_t dtrace_icookie_t;
typedef void (*dtrace_xcall_t)(void *);

extern dtrace_icookie_t dtrace_interrupt_disable(void);
extern void dtrace_interrupt_enable(dtrace_icookie_t);

extern void dtrace_membar_producer(void);
extern void dtrace_membar_consumer(void);

extern void (*dtrace_cpu_init)(processorid_t);
#if !defined(__APPLE__)
extern void (*dtrace_modload)(struct modctl *);
extern void (*dtrace_modunload)(struct modctl *);
#else
extern int (*dtrace_modload)(struct kmod_info *, uint32_t);
extern int (*dtrace_modunload)(struct kmod_info *);
#endif                
extern void (*dtrace_helpers_cleanup)(proc_t*);
extern void (*dtrace_helpers_fork)(proc_t *parent, proc_t *child);
extern void (*dtrace_cpustart_init)(void);
extern void (*dtrace_cpustart_fini)(void);

extern void (*dtrace_kreloc_init)(void);
extern void (*dtrace_kreloc_fini)(void);

extern void (*dtrace_debugger_init)(void);
extern void (*dtrace_debugger_fini)(void);
extern dtrace_cacheid_t dtrace_predcache_id;

extern hrtime_t dtrace_gethrtime(void);
extern void dtrace_sync(void);
extern void dtrace_toxic_ranges(void (*)(uintptr_t, uintptr_t));
extern void dtrace_xcall(processorid_t, dtrace_xcall_t, void *);

#if defined(__i386__) || defined(__x86_64__)
extern int dtrace_instr_size(uchar_t *instr);
extern int dtrace_instr_size_isa(uchar_t *, model_t, int *);
extern void dtrace_invop_add(int (*)(uintptr_t, uintptr_t *, uintptr_t));
extern void dtrace_invop_remove(int (*)(uintptr_t, uintptr_t *, uintptr_t));
extern void *dtrace_invop_callsite_pre;
extern void *dtrace_invop_callsite_post;
#endif

#if defined(__arm__)
extern int dtrace_instr_size(uint32_t instr, int thumb_mode);
#endif
#if defined(__arm__) || defined(__arm64__)
extern void dtrace_invop_add(int (*)(uintptr_t, uintptr_t *, uintptr_t));    
extern void dtrace_invop_remove(int (*)(uintptr_t, uintptr_t *, uintptr_t));
extern void *dtrace_invop_callsite_pre;
extern void *dtrace_invop_callsite_post;
#endif

#undef proc_t

#define DTRACE_CPUFLAG_ISSET(flag) \
        (cpu_core[CPU->cpu_id].cpuc_dtrace_flags & (flag))

#define DTRACE_CPUFLAG_SET(flag) \
        (cpu_core[CPU->cpu_id].cpuc_dtrace_flags |= (flag))

#define DTRACE_CPUFLAG_CLEAR(flag) \
        (cpu_core[CPU->cpu_id].cpuc_dtrace_flags &= ~(flag))

#endif              

#endif            

#if defined(__i386__) || defined(__x86_64__)

#define DTRACE_INVOP_PUSHL_EBP          1
#define DTRACE_INVOP_POPL_EBP           2
#define DTRACE_INVOP_LEAVE              3
#define DTRACE_INVOP_NOP                4
#define DTRACE_INVOP_RET                5

#endif

#if defined(__arm__) || defined(__arm64__)

#define DTRACE_INVOP_NOP                4
#define DTRACE_INVOP_RET                5
#define DTRACE_INVOP_B			6

#endif

#ifdef  __cplusplus
}
#endif

#endif                     
