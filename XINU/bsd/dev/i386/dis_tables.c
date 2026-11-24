/*
 *
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
 * Copyright (c) 2015, Joyent, Inc.
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
 */

/*
 * Copyright (c) 2010, Intel Corporation.
 * All rights reserved.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

  
                                                           
                                    
  
                                         
   

#include <sys/dtrace.h>
#include <sys/dtrace_glue.h>
#include <sys/dis_tables.h>

                   

  
                                                                          
                                                                         
                                                                            
                              
  
                                                                             
  
                                         
                                             
  
                                          
  
                                                                                
                                                            
   

  
                                                                       
   
#ifdef DIS_TEXT
extern char *strncpy(char *, const char *, size_t);
extern size_t strlen(const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern size_t strlcat(char *, const char *, size_t);
#endif


#define		TERM 	0	                                          
				                                    

                                  
typedef struct	instable {
	struct instable	*it_indirect;	                         
	uchar_t		it_adrmode;
#ifdef DIS_TEXT
	char		it_name[NCPS];
	uint_t		it_suffix:1;		                             
#endif
#ifdef DIS_MEM
	uint_t		it_size:16;
#endif
	uint_t		it_invalid64:1;		                             
	uint_t		it_always64:1;		                                
	uint_t		it_invalid32:1;		                     
	uint_t		it_stackop:1;		                              
	uint_t		it_vexwoxmm:1;		                                             
	uint_t		it_avxsuf:1;		                         
} instable_t;

  
                       
   
enum {
	UNKNOWN,
	MRw,
	IMlw,
	IMw,
	IR,
	OA,
	AO,
	MS,
	SM,
	Mv,
	Mw,
	M,		                        
	MG9,		                                                     
	Mb,		                                           
	MO,		                                
	PREF,
	SWAPGS_RDTSCP,
	MONITOR_MWAIT,
	R,
	RA,
	SEG,
	MR,
	RM,
	RM_66r,		                                         
	IA,
	MA,
	SD,
	AD,
	SA,
	D,
	INM,
	SO,
	BD,
	I,
	P,
	V,
	DSHIFT,		                                                  
	U,
	OVERRIDE,
	NORM,		                                                    
	IMPLMEM,	                                                       
	O,		              
	JTAB,		                 
	IMUL,		                          
	CBW,		                                                     
	MvI,		                      
	ENTER,		                          
	RMw,		                        
	Ib,		                             
	F,		                          
	FF,		                          
	FFC,		                          
	DM,		                 
	AM,		                 
	LSEG,		                                
	MIb,		                      
	SREG,		                               
	PREFIX,		                              
	LOCK,		                               
	INT3,		                                                     
	INTx,		                                                       
	DSHIFTcl,	                                               
	CWD,		                                                     
	RET,		                                     
	MOVZ,		                                                     
	CRC32,		                                             
	XADDB,		               
	MOVSXZ,		                                                    
	MOVBE,		                       

  
                             
   

	MMO,		                                          
	MMOIMPL,	                                            
	MMO3P,		                                            
	MMOM3,		                                        
	MMOS,		                                          
	MMOMS,		                                       
	MMOPM,		                                      
	MMOPM_66o,	                                                  
	MMOPRM,		                                                
	MMOSH,		                             
	MM,		                                 
	MMS,		                                 
	MMSH,		                    
	XMMO,		                                     
	XMMOS,		                                     
	XMMOPM,		                                            
	XMMOMX,		                                    
	XMMOX3,		                                 
	XMMOXMM,	                                    
	XMMOM,		                                 
	XMMOMS,		                                 
	XMM,		                            
	XMM_66r,	                                              
	XMM_66o,	                                               
	XMMXIMPL,	                              
	XMM3P,		                             
	XMM3PM_66r,	                                                   
	XMMP,		                                   
	XMMP_66o,	                                                     
	XMMP_66r,	                                                     
	XMMPRM,		                                 
	XMMPRM_66r,	                                                   
	XMMS,		                            
	XMMM,		                        
	XMMM_66r,	                                          
	XMMMS,		                        
	XMM3MX,		                            
	XMM3MXS,	                            
	XMMSH,		                      
	XMMXM3,		                            
	XMMX3,		                        
	XMMXMM,		                           
	XMMMX,		                       
	XMMXM,		                       
        XMMX2I,		                                  
        XMM2I,		                           
	XMMFENCE,	                           
	XMMSFNC,	                               
	XGETBV_XSETBV,
	VEX_NONE,	                     
	VEX_MO,		                                                
	VEX_RMrX,	                                                    
	VEX_VRMrX,	                                                   
	VEX_RRX,	                                                   
	VEX_RMRX,	                                                    
	VEX_MX,                                                             
	VEX_MXI,                                                            
	VEX_XXI,                                                             
	VEX_MR,                                                             
	VEX_RRI,                                                                         
	VEX_RX,                                                            
	VEX_RR,                                                             
	VEX_RRi,                                                            
	VEX_RM,                                                            
	VEX_RIM,	                                                   
	VEX_RRM,                                                           
	VEX_RMX,                                                            
	VEX_SbVM,	                                                   
	VMx,		                                     
	VMxo,		                                          
	SVM,		                          
	BLS,		                        
	FMA,		                                    
	ADX		                                                      
};

  
               
   
#define VEX_2bytes	0xC5	                                     
#define VEX_3bytes	0xC4	                                       

#define	FILL	0x90	                                        

  
                                
  
#define	EAX_REGNO 0
#define	ECX_REGNO 1
#define	EDX_REGNO 2
#define	EBX_REGNO 3
#define	ESP_REGNO 4
#define	EBP_REGNO 5
#define	ESI_REGNO 6
#define	EDI_REGNO 7

  
                             
   
#define	MODE_NONE	0
#define	MODE_IPREL	1	                              
#define	MODE_SIGNED	2	                             
#define	MODE_IMPLIED	3	                                        
#define	MODE_OFFSET	4	                               
#define	MODE_RIPREL	5	                                       

  
                                        
                                               
                                                                     
                                                 
                                            
                                                                              
                                              
                                                                 
                                                             
                                                                  
                                         
                                                                            
   

#if defined(DIS_TEXT) && defined(DIS_MEM)
#define	IND(table)		{(instable_t *)table, 0, "", 0, 0, 0, 0, 0, 0, 0, 0}
#define	INDx(table)		{(instable_t *)table, 0, "", 0, 0, 1, 0, 0, 0, 0, 0}
#define	TNS(name, amode)	{TERM, amode, name, 0, 0, 0, 0, 0, 0, 0, 0}
#define	TNSu(name, amode)	{TERM, amode, name, 0, 0, 0, 0, 1, 0, 0, 0}
#define	TNSx(name, amode)	{TERM, amode, name, 0, 0, 1, 0, 0, 0, 0, 0}
#define	TNSy(name, amode)	{TERM, amode, name, 0, 0, 0, 1, 0, 0, 0, 0}
#define	TNSyp(name, amode)	{TERM, amode, name, 0, 0, 0, 1, 0, 1, 0, 0}
#define	TNSZ(name, amode, sz)	{TERM, amode, name, 0, sz, 0, 0, 0, 0, 0, 0}
#define	TNSZy(name, amode, sz)	{TERM, amode, name, 0, sz, 0, 1, 0, 0, 0, 0}
#define	TNSZvr(name, amode, sz)	{TERM, amode, name, 0, sz, 0, 0, 0, 0, 1, 0}
#define	TS(name, amode)		{TERM, amode, name, 1, 0, 0, 0, 0, 0, 0, 0}
#define	TSx(name, amode)	{TERM, amode, name, 1, 0, 1, 0, 0, 0, 0, 0}
#define	TSy(name, amode)	{TERM, amode, name, 1, 0, 0, 1, 0, 0, 0, 0}
#define	TSp(name, amode)	{TERM, amode, name, 1, 0, 0, 0, 0, 1, 0, 0}
#define	TSZ(name, amode, sz)	{TERM, amode, name, 1, sz, 0, 0, 0, 0, 0, 0}
#define	TSaZ(name, amode, sz)	{TERM, amode, name, 1, sz, 0, 0, 0, 0, 0, 1}
#define	TSZx(name, amode, sz)	{TERM, amode, name, 1, sz, 1, 0, 0, 0, 0, 0}
#define	TSZy(name, amode, sz)	{TERM, amode, name, 1, sz, 0, 1, 0, 0, 0, 0}
#define	INVALID			{TERM, UNKNOWN, "", 0, 0, 0, 0, 0, 0, 0, 0}
#elif defined(DIS_TEXT)
#define	IND(table)		{(instable_t *)table, 0, "", 0, 0, 0, 0, 0}
#define	INDx(table)		{(instable_t *)table, 0, "", 0, 1, 0, 0, 0}
#define	TNS(name, amode)	{TERM, amode, name, 0, 0, 0, 0, 0}
#define	TNSu(name, amode)	{TERM, amode, name, 0, 0, 0, 1, 0}
#define	TNSx(name, amode)	{TERM, amode, name, 0, 1, 0, 0, 0}
#define	TNSy(name, amode)	{TERM, amode, name, 0, 0, 1, 0, 0}
#define	TNSyp(name, amode)	{TERM, amode, name, 0, 0, 1, 0, 1}
#define	TNSZ(name, amode, sz)	{TERM, amode, name, 0, 0, 0, 0, 0}
#define	TNSZy(name, amode, sz)	{TERM, amode, name, 0, 0, 1, 0, 0}
#define	TNSZvr(name, amode, sz)	{TERM, amode, name, 0, 0, 0, 0, 0, 1}
#define	TS(name, amode)		{TERM, amode, name, 1, 0, 0, 0, 0}
#define	TSx(name, amode)	{TERM, amode, name, 1, 1, 0, 0, 0}
#define	TSy(name, amode)	{TERM, amode, name, 1, 0, 1, 0, 0}
#define	TSp(name, amode)	{TERM, amode, name, 1, 0, 0, 0, 1}
#define	TSZ(name, amode, sz)	{TERM, amode, name, 1, 0, 0, 0, 0}
#define	TSaZ(name, amode, sz)	{TERM, amode, name, 1, 0, 0, 0, 0, 0, 1}
#define	TSZx(name, amode, sz)	{TERM, amode, name, 1, 1, 0, 0, 0}
#define	TSZy(name, amode, sz)	{TERM, amode, name, 1, 0, 1, 0, 0}
#define	INVALID			{TERM, UNKNOWN, "", 0, 0, 0, 0, 0}
#elif defined(DIS_MEM)
#define	IND(table)		{(instable_t *)table, 0, 0, 0, 0, 0, 0}
#define	INDx(table)		{(instable_t *)table, 0, 0, 1, 0, 0, 0}
#define	TNS(name, amode)	{TERM, amode,  0, 0, 0, 0, 0, 0, 0}
#define	TNSu(name, amode)	{TERM, amode,  0, 0, 0, 1, 0, 0, 0}
#define	TNSy(name, amode)	{TERM, amode,  0, 0, 1, 0, 0, 0, 0}
#define	TNSyp(name, amode)	{TERM, amode,  0, 0, 1, 0, 1, 0, 0}
#define	TNSx(name, amode)	{TERM, amode,  0, 1, 0, 0, 0, 0, 0}
#define	TNSZ(name, amode, sz)	{TERM, amode, sz, 0, 0, 0, 0, 0, 0}
#define	TNSZy(name, amode, sz)	{TERM, amode, sz, 0, 1, 0, 0, 0, 0}
#define	TNSZvr(name, amode, sz)	{TERM, amode, sz, 0, 0, 0, 0, 1, 0}
#define	TS(name, amode)		{TERM, amode,  0, 0, 0, 0, 0, 0, 0}
#define	TSx(name, amode)	{TERM, amode,  0, 1, 0, 0, 0, 0, 0}
#define	TSy(name, amode)	{TERM, amode,  0, 0, 1, 0, 0, 0, 0}
#define	TSp(name, amode)	{TERM, amode,  0, 0, 0, 0, 1, 0, 0}
#define	TSZ(name, amode, sz)	{TERM, amode, sz, 0, 0, 0, 0, 0, 0}
#define	TSaZ(name, amode, sz)	{TERM, amode, sz, 0, 0, 0, 0, 0, 1}
#define	TSZx(name, amode, sz)	{TERM, amode, sz, 1, 0, 0, 0, 0 ,0}
#define	TSZy(name, amode, sz)	{TERM, amode, sz, 0, 1, 0, 0, 0, 0}
#define	INVALID			{TERM, UNKNOWN, 0, 0, 0, 0, 0, 0, 0}
#else
#define	IND(table)		{(instable_t *)table, 0, 0, 0, 0, 0, 0, 0}
#define	INDx(table)		{(instable_t *)table, 0, 1, 0, 0, 0, 0, 0}
#define	TNS(name, amode)	{TERM, amode,  0, 0, 0, 0, 0, 0}
#define	TNSu(name, amode)	{TERM, amode,  0, 0, 1, 0, 0, 0}
#define	TNSy(name, amode)	{TERM, amode,  0, 1, 0, 0, 0, 0}
#define	TNSyp(name, amode)	{TERM, amode,  0, 1, 0, 1, 0, 0}
#define	TNSx(name, amode)	{TERM, amode,  1, 0, 0, 0, 0, 0}
#define	TNSZ(name, amode, sz)	{TERM, amode,  0, 0, 0, 0, 0, 0}
#define	TNSZy(name, amode, sz)	{TERM, amode,  0, 1, 0, 0, 0, 0}
#define	TNSZvr(name, amode, sz)	{TERM, amode,  0, 0, 0, 0, 1, 0}
#define	TS(name, amode)		{TERM, amode,  0, 0, 0, 0, 0, 0}
#define	TSx(name, amode)	{TERM, amode,  1, 0, 0, 0, 0, 0}
#define	TSy(name, amode)	{TERM, amode,  0, 1, 0, 0, 0, 0}
#define	TSp(name, amode)	{TERM, amode,  0, 0, 0, 1, 0, 0}
#define	TSZ(name, amode, sz)	{TERM, amode,  0, 0, 0, 0, 0, 0}
#define	TSaZ(name, amode, sz)	{TERM, amode,  0, 0, 0, 0, 0, 1}
#define	TSZx(name, amode, sz)	{TERM, amode,  1, 0, 0, 0, 0, 0}
#define	TSZy(name, amode, sz)	{TERM, amode,  0, 1, 0, 0, 0, 0}
#define	INVALID			{TERM, UNKNOWN, 0, 0, 0, 0, 0, 0}
#endif

#ifdef DIS_TEXT
  
                                                               
   
const char *const dis_addr16[3][8] = {
"(%bx,%si)", "(%bx,%di)", "(%bp,%si)", "(%bp,%di)", "(%si)", "(%di)", "",
									"(%bx)",
"(%bx,%si)", "(%bx,%di)", "(%bp,%si)", "(%bp,%di)", "(%si)", "(%di", "(%bp)",
									"(%bx)",
"(%bx,%si)", "(%bx,%di)", "(%bp,%si)", "(%bp,%di)", "(%si)", "(%di)", "(%bp)",
									"(%bx)",
};


  
                                                                  
   
const char *const dis_addr32_mode0[16] = {
  "(%eax)", "(%ecx)", "(%edx)",  "(%ebx)",  "", "",        "(%esi)",  "(%edi)",
  "(%r8d)", "(%r9d)", "(%r10d)", "(%r11d)", "", "",        "(%r14d)", "(%r15d)"
};

const char *const dis_addr32_mode12[16] = {
  "(%eax)", "(%ecx)", "(%edx)",  "(%ebx)",  "", "(%ebp)",  "(%esi)",  "(%edi)",
  "(%r8d)", "(%r9d)", "(%r10d)", "(%r11d)", "", "(%r13d)", "(%r14d)", "(%r15d)"
};

  
                                                                  
   
const char *const dis_addr64_mode0[16] = {
 "(%rax)", "(%rcx)", "(%rdx)", "(%rbx)", "",       "(%rip)", "(%rsi)", "(%rdi)",
 "(%r8)",  "(%r9)",  "(%r10)", "(%r11)", "(%r12)", "(%rip)", "(%r14)", "(%r15)"
};
const char *const dis_addr64_mode12[16] = {
 "(%rax)", "(%rcx)", "(%rdx)", "(%rbx)", "",       "(%rbp)", "(%rsi)", "(%rdi)",
 "(%r8)",  "(%r9)",  "(%r10)", "(%r11)", "(%r12)", "(%r13)", "(%r14)", "(%r15)"
};

  
                                 
   
const char *const dis_scale_factor[4] = { ")", ",2)", ",4)", ",8)" };

  
                                                                                
                
   
const char *const dis_vscale_factor[4] = { ",1)", ",2)", ",4)", ",8)" };

  
                                                                            
   
const char *const dis_REG8[16] = {
	"%al",  "%cl",  "%dl",   "%bl",   "%ah",   "%ch",   "%dh",   "%bh",
	"%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"
};

const char *const dis_REG8_REX[16] = {
	"%al",  "%cl",  "%dl",   "%bl",   "%spl",  "%bpl",  "%sil",  "%dil",
	"%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"
};

const char *const dis_REG16[16] = {
	"%ax",  "%cx",  "%dx",   "%bx",   "%sp",   "%bp",   "%si",   "%di",
	"%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w"
};

const char *const dis_REG32[16] = {
	"%eax", "%ecx", "%edx",  "%ebx",  "%esp",  "%ebp",  "%esi",  "%edi",
	"%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"
};

const char *const dis_REG64[16] = {
	"%rax", "%rcx", "%rdx",  "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
	"%r8",  "%r9",  "%r10",  "%r11", "%r12", "%r13", "%r14", "%r15"
};

const char *const dis_DEBUGREG[16] = {
	"%db0", "%db1", "%db2",  "%db3",  "%db4",  "%db5",  "%db6",  "%db7",
	"%db8", "%db9", "%db10", "%db11", "%db12", "%db13", "%db14", "%db15"
};

const char *const dis_CONTROLREG[16] = {
    "%cr0", "%cr1", "%cr2", "%cr3", "%cr4", "%cr5?", "%cr6?", "%cr7?",
    "%cr8", "%cr9?", "%cr10?", "%cr11?", "%cr12?", "%cr13?", "%cr14?", "%cr15?"
};

const char *const dis_TESTREG[16] = {
	"%tr0?", "%tr1?", "%tr2?", "%tr3", "%tr4", "%tr5", "%tr6", "%tr7",
	"%tr0?", "%tr1?", "%tr2?", "%tr3", "%tr4", "%tr5", "%tr6", "%tr7"
};

const char *const dis_MMREG[16] = {
	"%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7",
	"%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
};

const char *const dis_XMMREG[16] = {
    "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
    "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
};

const char *const dis_YMMREG[16] = {
    "%ymm0", "%ymm1", "%ymm2", "%ymm3", "%ymm4", "%ymm5", "%ymm6", "%ymm7",
    "%ymm8", "%ymm9", "%ymm10", "%ymm11", "%ymm12", "%ymm13", "%ymm14", "%ymm15"
};

const char *const dis_SEGREG[16] = {
	"%es", "%cs", "%ss", "%ds", "%fs", "%gs", "<reserved>", "<reserved>",
	"%es", "%cs", "%ss", "%ds", "%fs", "%gs", "<reserved>", "<reserved>"
};

  
                          
   
const char *const dis_PREDSUFFIX[8] = {
	"eq", "lt", "le", "unord", "neq", "nlt", "nle", "ord"
};

const char *const dis_AVXvgrp7[3][8] = {
	                       
      	{"",	"",	"vpsrlw",	"",		"vpsraw",	"",	"vpsllw",	""},
      	{"",	"",	"vpsrld",	"",		"vpsrad",	"",	"vpslld",	""},
      	{"",	"",	"vpsrlq",	"vpsrldq",	"",		"",	"vpsllq",	"vpslldq"}
};

#endif	              

  
                                                                  
   
const instable_t dis_opMOVSLD = TNS("movslq",MOVSXZ);

  
                                                    
   
const instable_t dis_opPause = TNS("pause", NORM);

  
                                  
   
const instable_t dis_op0F00[8] = {

           	TNS("sldt",M),		TNS("str",M),		TNSy("lldt",M), 	TNSy("ltr",M),
           	TNSZ("verr",M,2),	TNSZ("verw",M,2),	INVALID,		INVALID,
};


  
                                  
   
const instable_t dis_op0F01[8] = {

           	TNSZ("sgdt",VMx,6),	TNSZ("sidt",MONITOR_MWAIT,6),	TNSZ("lgdt",XGETBV_XSETBV,6),	TNSZ("lidt",SVM,6),
           	TNSZ("smsw",M,2),	INVALID, 		TNSZ("lmsw",M,2),	TNS("invlpg",SWAPGS_RDTSCP),
};

  
                                                   
   
const instable_t dis_op0F18[8] = {

           	TNS("prefetchnta",PREF),TNS("prefetcht0",PREF),	TNS("prefetcht1",PREF),	TNS("prefetcht2",PREF),
           	INVALID,		INVALID,		INVALID,		INVALID,
};

  
                                                              
   
const instable_t dis_op0FAE[8] = {
           	TNSZ("fxsave",M,512),	TNSZ("fxrstor",M,512),	TNS("ldmxcsr",M),	TNS("stmxcsr",M),
           	TNSZ("xsave",M,512),	TNS("lfence",XMMFENCE), TNS("mfence",XMMFENCE),	TNS("sfence",XMMSFNC),
};

  
                                  
   

const instable_t dis_op0FBA[8] = {

           	INVALID,		INVALID,		INVALID,		INVALID,
           	TS("bt",MIb),		TS("bts",MIb),		TS("btr",MIb),		TS("btc",MIb),
};

  
                                            
   

const instable_t dis_op0FC7[8] = {

           	INVALID,		TNS("cmpxchg8b",M),	INVALID,		INVALID,
           	INVALID,		INVALID,		TNS("vmptrld",MG9),	TNS("vmptrst",MG9),
};

  
                                                   
   

const instable_t dis_op0FC7m3[8] = {

           	INVALID,		INVALID,	INVALID,		INVALID,
           	INVALID,		INVALID,	TNS("rdrand",MG9),	TNS("rdseed", MG9),
};

  
                                                   
   

const instable_t dis_op660FC7[8] = {

           	INVALID,		INVALID,		INVALID,		INVALID,
           	INVALID,		INVALID,		TNS("vmclear",M),	INVALID,
};

  
                                                   
   

const instable_t dis_opF30FC7[8] = {

           	INVALID,		INVALID,		INVALID,		INVALID,
           	INVALID,		INVALID,		TNS("vmxon",M),		INVALID,
};

  
                                                          
  
                                  
   
const instable_t dis_op0FC8[4] = {
           	TNS("bswap",R),		INVALID,		INVALID,		INVALID,
};

  
                                                                          
   
const instable_t dis_op0F7123[4][8] = {
{
             	INVALID,		INVALID,		INVALID,		INVALID,
             	INVALID,		INVALID,		INVALID,		INVALID,
}, {
             	INVALID,		INVALID,		TNS("psrlw",MMOSH),	INVALID,
             	TNS("psraw",MMOSH),	INVALID,		TNS("psllw",MMOSH),	INVALID,
}, {
             	INVALID,		INVALID,		TNS("psrld",MMOSH),	INVALID,
             	TNS("psrad",MMOSH),	INVALID,		TNS("pslld",MMOSH),	INVALID,
}, {
             	INVALID,		INVALID,		TNS("psrlq",MMOSH),	TNS("INVALID",MMOSH),
             	INVALID,		INVALID, 		TNS("psllq",MMOSH),	TNS("INVALID",MMOSH),
} };

  
                                                                   
   
const instable_t dis_opSIMD7123[32] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNS("psrlw",XMMSH),	INVALID,
            	TNS("psraw",XMMSH),	INVALID,		TNS("psllw",XMMSH),	INVALID,

            	INVALID,		INVALID,		TNS("psrld",XMMSH),	INVALID,
            	TNS("psrad",XMMSH),	INVALID,		TNS("pslld",XMMSH),	INVALID,

            	INVALID,		INVALID,		TNS("psrlq",XMMSH),	TNS("psrldq",XMMSH),
            	INVALID,		INVALID,		TNS("psllq",XMMSH),	TNS("pslldq",XMMSH),
};

  
                                                                        
                                                                    
                                                                          
                                                                        
                                                                        
                                                                       
                                           
   

  
                                                                          
   
const instable_t dis_opSIMDdata16[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("movupd",XMM,16),	TNSZ("movupd",XMMS,16),	TNSZ("movlpd",XMMM,8),	TNSZ("movlpd",XMMMS,8),
            	TNSZ("unpcklpd",XMM,16),TNSZ("unpckhpd",XMM,16),TNSZ("movhpd",XMMM,8),	TNSZ("movhpd",XMMMS,8),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("movapd",XMM,16),	TNSZ("movapd",XMMS,16),	TNSZ("cvtpi2pd",XMMOMX,8),TNSZ("movntpd",XMMOMS,16),
            	TNSZ("cvttpd2pi",XMMXMM,16),TNSZ("cvtpd2pi",XMMXMM,16),TNSZ("ucomisd",XMM,8),TNSZ("comisd",XMM,8),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNS("movmskpd",XMMOX3),	TNSZ("sqrtpd",XMM,16),	INVALID,		INVALID,
            	TNSZ("andpd",XMM,16),	TNSZ("andnpd",XMM,16),	TNSZ("orpd",XMM,16),	TNSZ("xorpd",XMM,16),
            	TNSZ("addpd",XMM,16),	TNSZ("mulpd",XMM,16),	TNSZ("cvtpd2ps",XMM,16),TNSZ("cvtps2dq",XMM,16),
            	TNSZ("subpd",XMM,16),	TNSZ("minpd",XMM,16),	TNSZ("divpd",XMM,16),	TNSZ("maxpd",XMM,16),

            	TNSZ("punpcklbw",XMM,16),TNSZ("punpcklwd",XMM,16),TNSZ("punpckldq",XMM,16),TNSZ("packsswb",XMM,16),
            	TNSZ("pcmpgtb",XMM,16),	TNSZ("pcmpgtw",XMM,16),	TNSZ("pcmpgtd",XMM,16),	TNSZ("packuswb",XMM,16),
            	TNSZ("punpckhbw",XMM,16),TNSZ("punpckhwd",XMM,16),TNSZ("punpckhdq",XMM,16),TNSZ("packssdw",XMM,16),
            	TNSZ("punpcklqdq",XMM,16),TNSZ("punpckhqdq",XMM,16),TNSZ("movd",XMM3MX,4),TNSZ("movdqa",XMM,16),

            	TNSZ("pshufd",XMMP,16),	INVALID,		INVALID,		INVALID,
            	TNSZ("pcmpeqb",XMM,16),	TNSZ("pcmpeqw",XMM,16),	TNSZ("pcmpeqd",XMM,16),	INVALID,
            	TNSZ("extrq",XMM2I,16),	TNSZ("extrq",XMM,16), INVALID,		INVALID,
            	TNSZ("haddpd",XMM,16),	TNSZ("hsubpd",XMM,16),	TNSZ("movd",XMM3MXS,4),	TNSZ("movdqa",XMMS,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("cmppd",XMMP,16),	INVALID,
            	TNSZ("pinsrw",XMMPRM,2),TNS("pextrw",XMM3P),	TNSZ("shufpd",XMMP,16),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("addsubpd",XMM,16),TNSZ("psrlw",XMM,16),	TNSZ("psrld",XMM,16),	TNSZ("psrlq",XMM,16),
            	TNSZ("paddq",XMM,16),	TNSZ("pmullw",XMM,16),	TNSZ("movq",XMMS,8),	TNS("pmovmskb",XMMX3),
            	TNSZ("psubusb",XMM,16),	TNSZ("psubusw",XMM,16),	TNSZ("pminub",XMM,16),	TNSZ("pand",XMM,16),
            	TNSZ("paddusb",XMM,16),	TNSZ("paddusw",XMM,16),	TNSZ("pmaxub",XMM,16),	TNSZ("pandn",XMM,16),

            	TNSZ("pavgb",XMM,16),	TNSZ("psraw",XMM,16),	TNSZ("psrad",XMM,16),	TNSZ("pavgw",XMM,16),
            	TNSZ("pmulhuw",XMM,16),	TNSZ("pmulhw",XMM,16),	TNSZ("cvttpd2dq",XMM,16),TNSZ("movntdq",XMMS,16),
            	TNSZ("psubsb",XMM,16),	TNSZ("psubsw",XMM,16),	TNSZ("pminsw",XMM,16),	TNSZ("por",XMM,16),
            	TNSZ("paddsb",XMM,16),	TNSZ("paddsw",XMM,16),	TNSZ("pmaxsw",XMM,16),	TNSZ("pxor",XMM,16),

            	INVALID,		TNSZ("psllw",XMM,16),	TNSZ("pslld",XMM,16),	TNSZ("psllq",XMM,16),
            	TNSZ("pmuludq",XMM,16),	TNSZ("pmaddwd",XMM,16),	TNSZ("psadbw",XMM,16),	TNSZ("maskmovdqu", XMMXIMPL,16),
            	TNSZ("psubb",XMM,16),	TNSZ("psubw",XMM,16),	TNSZ("psubd",XMM,16),	TNSZ("psubq",XMM,16),
            	TNSZ("paddb",XMM,16),	TNSZ("paddw",XMM,16),	TNSZ("paddd",XMM,16),	INVALID,
};

const instable_t dis_opAVX660F[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vmovupd",VEX_MX,16),	TNSZ("vmovupd",VEX_RX,16),	TNSZ("vmovlpd",VEX_RMrX,8),	TNSZ("vmovlpd",VEX_RM,8),
            	TNSZ("vunpcklpd",VEX_RMrX,16),TNSZ("vunpckhpd",VEX_RMrX,16),TNSZ("vmovhpd",VEX_RMrX,8),	TNSZ("vmovhpd",VEX_RM,8),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vmovapd",VEX_MX,16),	TNSZ("vmovapd",VEX_RX,16),	INVALID,		TNSZ("vmovntpd",VEX_RM,16),
            	INVALID,		INVALID,		TNSZ("vucomisd",VEX_MX,8),TNSZ("vcomisd",VEX_MX,8),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNS("vmovmskpd",VEX_MR),	TNSZ("vsqrtpd",VEX_MX,16),	INVALID,		INVALID,
            	TNSZ("vandpd",VEX_RMrX,16),	TNSZ("vandnpd",VEX_RMrX,16),	TNSZ("vorpd",VEX_RMrX,16),	TNSZ("vxorpd",VEX_RMrX,16),
            	TNSZ("vaddpd",VEX_RMrX,16),	TNSZ("vmulpd",VEX_RMrX,16),	TNSZ("vcvtpd2ps",VEX_MX,16),TNSZ("vcvtps2dq",VEX_MX,16),
            	TNSZ("vsubpd",VEX_RMrX,16),	TNSZ("vminpd",VEX_RMrX,16),	TNSZ("vdivpd",VEX_RMrX,16),	TNSZ("vmaxpd",VEX_RMrX,16),

            	TNSZ("vpunpcklbw",VEX_RMrX,16),TNSZ("vpunpcklwd",VEX_RMrX,16),TNSZ("vpunpckldq",VEX_RMrX,16),TNSZ("vpacksswb",VEX_RMrX,16),
            	TNSZ("vpcmpgtb",VEX_RMrX,16),	TNSZ("vpcmpgtw",VEX_RMrX,16),	TNSZ("vpcmpgtd",VEX_RMrX,16),	TNSZ("vpackuswb",VEX_RMrX,16),
            	TNSZ("vpunpckhbw",VEX_RMrX,16),TNSZ("vpunpckhwd",VEX_RMrX,16),TNSZ("vpunpckhdq",VEX_RMrX,16),TNSZ("vpackssdw",VEX_RMrX,16),
            	TNSZ("vpunpcklqdq",VEX_RMrX,16),TNSZ("vpunpckhqdq",VEX_RMrX,16),TNSZ("vmovd",VEX_MX,4),TNSZ("vmovdqa",VEX_MX,16),

            	TNSZ("vpshufd",VEX_MXI,16),	TNSZ("vgrp71",VEX_XXI,16),	TNSZ("vgrp72",VEX_XXI,16),		TNSZ("vgrp73",VEX_XXI,16),
            	TNSZ("vpcmpeqb",VEX_RMrX,16),	TNSZ("vpcmpeqw",VEX_RMrX,16),	TNSZ("vpcmpeqd",VEX_RMrX,16),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vhaddpd",VEX_RMrX,16),	TNSZ("vhsubpd",VEX_RMrX,16),	TNSZ("vmovd",VEX_RR,4),	TNSZ("vmovdqa",VEX_RX,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("vcmppd",VEX_RMRX,16),	INVALID,
            	TNSZ("vpinsrw",VEX_RMRX,2),TNS("vpextrw",VEX_MR),	TNSZ("vshufpd",VEX_RMRX,16),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vaddsubpd",VEX_RMrX,16),TNSZ("vpsrlw",VEX_RMrX,16),	TNSZ("vpsrld",VEX_RMrX,16),	TNSZ("vpsrlq",VEX_RMrX,16),
            	TNSZ("vpaddq",VEX_RMrX,16),	TNSZ("vpmullw",VEX_RMrX,16),	TNSZ("vmovq",VEX_RX,8),	TNS("vpmovmskb",VEX_MR),
            	TNSZ("vpsubusb",VEX_RMrX,16),	TNSZ("vpsubusw",VEX_RMrX,16),	TNSZ("vpminub",VEX_RMrX,16),	TNSZ("vpand",VEX_RMrX,16),
            	TNSZ("vpaddusb",VEX_RMrX,16),	TNSZ("vpaddusw",VEX_RMrX,16),	TNSZ("vpmaxub",VEX_RMrX,16),	TNSZ("vpandn",VEX_RMrX,16),

            	TNSZ("vpavgb",VEX_RMrX,16),	TNSZ("vpsraw",VEX_RMrX,16),	TNSZ("vpsrad",VEX_RMrX,16),	TNSZ("vpavgw",VEX_RMrX,16),
            	TNSZ("vpmulhuw",VEX_RMrX,16),	TNSZ("vpmulhw",VEX_RMrX,16),	TNSZ("vcvttpd2dq",VEX_MX,16),TNSZ("vmovntdq",VEX_RM,16),
            	TNSZ("vpsubsb",VEX_RMrX,16),	TNSZ("vpsubsw",VEX_RMrX,16),	TNSZ("vpminsw",VEX_RMrX,16),	TNSZ("vpor",VEX_RMrX,16),
            	TNSZ("vpaddsb",VEX_RMrX,16),	TNSZ("vpaddsw",VEX_RMrX,16),	TNSZ("vpmaxsw",VEX_RMrX,16),	TNSZ("vpxor",VEX_RMrX,16),

            	INVALID,		TNSZ("vpsllw",VEX_RMrX,16),	TNSZ("vpslld",VEX_RMrX,16),	TNSZ("vpsllq",VEX_RMrX,16),
            	TNSZ("vpmuludq",VEX_RMrX,16),	TNSZ("vpmaddwd",VEX_RMrX,16),	TNSZ("vpsadbw",VEX_RMrX,16),	TNS("vmaskmovdqu",VEX_MX),
            	TNSZ("vpsubb",VEX_RMrX,16),	TNSZ("vpsubw",VEX_RMrX,16),	TNSZ("vpsubd",VEX_RMrX,16),	TNSZ("vpsubq",VEX_RMrX,16),
            	TNSZ("vpaddb",VEX_RMrX,16),	TNSZ("vpaddw",VEX_RMrX,16),	TNSZ("vpaddd",VEX_RMrX,16),	INVALID,
};

  
                                                                   
   
const instable_t dis_opSIMDrepnz[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("movsd",XMM,8),	TNSZ("movsd",XMMS,8),	TNSZ("movddup",XMM,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("cvtsi2sd",XMM3MX,4),TNSZ("movntsd",XMMMS,8),
            	TNSZ("cvttsd2si",XMMXM3,8),TNSZ("cvtsd2si",XMMXM3,8),INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		TNSZ("sqrtsd",XMM,8),	INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("addsd",XMM,8),	TNSZ("mulsd",XMM,8),	TNSZ("cvtsd2ss",XMM,8),	INVALID,
            	TNSZ("subsd",XMM,8),	TNSZ("minsd",XMM,8),	TNSZ("divsd",XMM,8),	TNSZ("maxsd",XMM,8),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("pshuflw",XMMP,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("insertq",XMMX2I,16),TNSZ("insertq",XMM,8),INVALID,		INVALID,
            	TNSZ("haddps",XMM,16),	TNSZ("hsubps",XMM,16),	INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("cmpsd",XMMP,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("addsubps",XMM,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNS("movdq2q",XMMXM),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("cvtpd2dq",XMM,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNS("lddqu",XMMM),	INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVXF20F[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vmovsd",VEX_RMrX,8),	TNSZ("vmovsd",VEX_RRX,8),	TNSZ("vmovddup",VEX_MX,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vcvtsi2sd",VEX_RMrX,4),INVALID,
            	TNSZ("vcvttsd2si",VEX_MR,8),TNSZ("vcvtsd2si",VEX_MR,8),INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		TNSZ("vsqrtsd",VEX_RMrX,8),	INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vaddsd",VEX_RMrX,8),	TNSZ("vmulsd",VEX_RMrX,8),	TNSZ("vcvtsd2ss",VEX_RMrX,8),	INVALID,
            	TNSZ("vsubsd",VEX_RMrX,8),	TNSZ("vminsd",VEX_RMrX,8),	TNSZ("vdivsd",VEX_RMrX,8),	TNSZ("vmaxsd",VEX_RMrX,8),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vpshuflw",VEX_MXI,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vhaddps",VEX_RMrX,8),	TNSZ("vhsubps",VEX_RMrX,8),	INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("vcmpsd",VEX_RMRX,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vaddsubps",VEX_RMrX,8),	INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vcvtpd2dq",VEX_MX,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vlddqu",VEX_MX,16),	INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVXF20F3A[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZvr("rorx",VEX_MXI,6),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVXF20F38[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		TNSZvr("pdep",VEX_RMrX,5),TNSZvr("mulx",VEX_RMrX,5),TNSZvr("shrx",VEX_VRMrX,5),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVXF30F38[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		TNSZvr("pext",VEX_RMrX,5),INVALID,		TNSZvr("sarx",VEX_VRMrX,5),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};
  
                                                                  
   
const instable_t dis_opSIMDrepz[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("movss",XMM,4),	TNSZ("movss",XMMS,4),	TNSZ("movsldup",XMM,16),INVALID,
            	INVALID,		INVALID,		TNSZ("movshdup",XMM,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("cvtsi2ss",XMM3MX,4),TNSZ("movntss",XMMMS,4),
            	TNSZ("cvttss2si",XMMXM3,4),TNSZ("cvtss2si",XMMXM3,4),INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		TNSZ("sqrtss",XMM,4),	TNSZ("rsqrtss",XMM,4),	TNSZ("rcpss",XMM,4),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("addss",XMM,4),	TNSZ("mulss",XMM,4),	TNSZ("cvtss2sd",XMM,4),	TNSZ("cvttps2dq",XMM,16),
            	TNSZ("subss",XMM,4),	TNSZ("minss",XMM,4),	TNSZ("divss",XMM,4),	TNSZ("maxss",XMM,4),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("movdqu",XMM,16),

            	TNSZ("pshufhw",XMMP,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("movq",XMM,8),	TNSZ("movdqu",XMMS,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TS("popcnt",MRw),	INVALID,		INVALID,		INVALID,
            	TNSZ("tzcnt",MRw,5),	TS("lzcnt",MRw),	INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("cmpss",XMMP,4),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNS("movq2dq",XMMMX),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("cvtdq2pd",XMM,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVXF30F[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vmovss",VEX_RMrX,4),	TNSZ("vmovss",VEX_RRX,4),	TNSZ("vmovsldup",VEX_MX,4),	INVALID,
            	INVALID,		INVALID,		TNSZ("vmovshdup",VEX_MX,4),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vcvtsi2ss",VEX_RMrX,4),INVALID,
            	TNSZ("vcvttss2si",VEX_MR,4),TNSZ("vcvtss2si",VEX_MR,4),INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		TNSZ("vsqrtss",VEX_RMrX,4),	TNSZ("vrsqrtss",VEX_RMrX,4),	TNSZ("vrcpss",VEX_RMrX,4),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vaddss",VEX_RMrX,4),	TNSZ("vmulss",VEX_RMrX,4),	TNSZ("vcvtss2sd",VEX_RMrX,4),	TNSZ("vcvttps2dq",VEX_MX,16),
            	TNSZ("vsubss",VEX_RMrX,4),	TNSZ("vminss",VEX_RMrX,4),	TNSZ("vdivss",VEX_RMrX,4),	TNSZ("vmaxss",VEX_RMrX,4),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("vmovdqu",VEX_MX,16),

            	TNSZ("vpshufhw",VEX_MXI,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vmovq",VEX_MX,8),	TNSZ("vmovdqu",VEX_RX,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		TNSZ("vcmpss",VEX_RMRX,4),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vcvtdq2pd",VEX_MX,8),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};
  
                                                              
                                     
   
const instable_t dis_op0F38F0[2] = {
            	TNS("crc32b",CRC32),
		TS("movbe",MOVBE),
};

const instable_t dis_op0F38F1[2] = {
            	TS("crc32",CRC32),
		TS("movbe",MOVBE),
};

  
                                                                               
                    
   
const instable_t dis_op0F38F6[2] = {
            	TNS("adcx",ADX),
		TNS("adox",ADX),
};

const instable_t dis_op0F38[256] = {
            	TNSZ("pshufb",XMM_66o,16),TNSZ("phaddw",XMM_66o,16),TNSZ("phaddd",XMM_66o,16),TNSZ("phaddsw",XMM_66o,16),
            	TNSZ("pmaddubsw",XMM_66o,16),TNSZ("phsubw",XMM_66o,16),	TNSZ("phsubd",XMM_66o,16),TNSZ("phsubsw",XMM_66o,16),
            	TNSZ("psignb",XMM_66o,16),TNSZ("psignw",XMM_66o,16),TNSZ("psignd",XMM_66o,16),TNSZ("pmulhrsw",XMM_66o,16),
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("pblendvb",XMM_66r,16),INVALID,		INVALID,		INVALID,
            	TNSZ("blendvps",XMM_66r,16),TNSZ("blendvpd",XMM_66r,16),INVALID,	TNSZ("ptest",XMM_66r,16),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("pabsb",XMM_66o,16),TNSZ("pabsw",XMM_66o,16),TNSZ("pabsd",XMM_66o,16),INVALID,

            	TNSZ("pmovsxbw",XMM_66r,16),TNSZ("pmovsxbd",XMM_66r,16),TNSZ("pmovsxbq",XMM_66r,16),TNSZ("pmovsxwd",XMM_66r,16),
            	TNSZ("pmovsxwq",XMM_66r,16),TNSZ("pmovsxdq",XMM_66r,16),INVALID,	INVALID,
            	TNSZ("pmuldq",XMM_66r,16),TNSZ("pcmpeqq",XMM_66r,16),TNSZ("movntdqa",XMMM_66r,16),TNSZ("packusdw",XMM_66r,16),
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("pmovzxbw",XMM_66r,16),TNSZ("pmovzxbd",XMM_66r,16),TNSZ("pmovzxbq",XMM_66r,16),TNSZ("pmovzxwd",XMM_66r,16),
            	TNSZ("pmovzxwq",XMM_66r,16),TNSZ("pmovzxdq",XMM_66r,16),INVALID,	TNSZ("pcmpgtq",XMM_66r,16),
            	TNSZ("pminsb",XMM_66r,16),TNSZ("pminsd",XMM_66r,16),TNSZ("pminuw",XMM_66r,16),TNSZ("pminud",XMM_66r,16),
            	TNSZ("pmaxsb",XMM_66r,16),TNSZ("pmaxsd",XMM_66r,16),TNSZ("pmaxuw",XMM_66r,16),TNSZ("pmaxud",XMM_66r,16),

            	TNSZ("pmulld",XMM_66r,16),TNSZ("phminposuw",XMM_66r,16),INVALID,	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSy("invept", RM_66r),	TNSy("invvpid", RM_66r),TNSy("invpcid", RM_66r),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("sha1nexte",XMM,16),TNSZ("sha1msg1",XMM,16),TNSZ("sha1msg2",XMM,16),TNSZ("sha256rnds2",XMM,16),
            	TNSZ("sha256msg1",XMM,16),TNSZ("sha256msg2",XMM,16),INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("aesimc",XMM_66r,16),
            	TNSZ("aesenc",XMM_66r,16),TNSZ("aesenclast",XMM_66r,16),TNSZ("aesdec",XMM_66r,16),TNSZ("aesdeclast",XMM_66r,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	IND(dis_op0F38F0),	IND(dis_op0F38F1),	INVALID,		INVALID,
            	INVALID,		INVALID,		IND(dis_op0F38F6),	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVX660F38[256] = {
            	TNSZ("vpshufb",VEX_RMrX,16),TNSZ("vphaddw",VEX_RMrX,16),TNSZ("vphaddd",VEX_RMrX,16),TNSZ("vphaddsw",VEX_RMrX,16),
            	TNSZ("vpmaddubsw",VEX_RMrX,16),TNSZ("vphsubw",VEX_RMrX,16),	TNSZ("vphsubd",VEX_RMrX,16),TNSZ("vphsubsw",VEX_RMrX,16),
            	TNSZ("vpsignb",VEX_RMrX,16),TNSZ("vpsignw",VEX_RMrX,16),TNSZ("vpsignd",VEX_RMrX,16),TNSZ("vpmulhrsw",VEX_RMrX,16),
            	TNSZ("vpermilps",VEX_RMrX,8),TNSZ("vpermilpd",VEX_RMrX,16),TNSZ("vtestps",VEX_RRI,8),	TNSZ("vtestpd",VEX_RRI,16),

            	INVALID,		INVALID,		INVALID,		TNSZ("vcvtph2ps",VEX_MX,16),
            	INVALID,		INVALID,		TNSZ("vpermps",VEX_RMrX,16),TNSZ("vptest",VEX_RRI,16),
            	TNSZ("vbroadcastss",VEX_MX,4),TNSZ("vbroadcastsd",VEX_MX,8),TNSZ("vbroadcastf128",VEX_MX,16),INVALID,
            	TNSZ("vpabsb",VEX_MX,16),TNSZ("vpabsw",VEX_MX,16),TNSZ("vpabsd",VEX_MX,16),INVALID,

            	TNSZ("vpmovsxbw",VEX_MX,16),TNSZ("vpmovsxbd",VEX_MX,16),TNSZ("vpmovsxbq",VEX_MX,16),TNSZ("vpmovsxwd",VEX_MX,16),
            	TNSZ("vpmovsxwq",VEX_MX,16),TNSZ("vpmovsxdq",VEX_MX,16),INVALID,	INVALID,
            	TNSZ("vpmuldq",VEX_RMrX,16),TNSZ("vpcmpeqq",VEX_RMrX,16),TNSZ("vmovntdqa",VEX_MX,16),TNSZ("vpackusdw",VEX_RMrX,16),
            	TNSZ("vmaskmovps",VEX_RMrX,8),TNSZ("vmaskmovpd",VEX_RMrX,16),TNSZ("vmaskmovps",VEX_RRM,8),TNSZ("vmaskmovpd",VEX_RRM,16),

            	TNSZ("vpmovzxbw",VEX_MX,16),TNSZ("vpmovzxbd",VEX_MX,16),TNSZ("vpmovzxbq",VEX_MX,16),TNSZ("vpmovzxwd",VEX_MX,16),
            	TNSZ("vpmovzxwq",VEX_MX,16),TNSZ("vpmovzxdq",VEX_MX,16),TNSZ("vpermd",VEX_RMrX,16),TNSZ("vpcmpgtq",VEX_RMrX,16),
            	TNSZ("vpminsb",VEX_RMrX,16),TNSZ("vpminsd",VEX_RMrX,16),TNSZ("vpminuw",VEX_RMrX,16),TNSZ("vpminud",VEX_RMrX,16),
            	TNSZ("vpmaxsb",VEX_RMrX,16),TNSZ("vpmaxsd",VEX_RMrX,16),TNSZ("vpmaxuw",VEX_RMrX,16),TNSZ("vpmaxud",VEX_RMrX,16),

            	TNSZ("vpmulld",VEX_RMrX,16),TNSZ("vphminposuw",VEX_MX,16),INVALID,	INVALID,
            	INVALID,		TSaZ("vpsrlv",VEX_RMrX,16),TNSZ("vpsravd",VEX_RMrX,16),TSaZ("vpsllv",VEX_RMrX,16),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vpbroadcastd",VEX_MX,16),TNSZ("vpbroadcastq",VEX_MX,16),TNSZ("vbroadcasti128",VEX_MX,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vpbroadcastb",VEX_MX,16),TNSZ("vpbroadcastw",VEX_MX,16),INVALID,	INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TSaZ("vpmaskmov",VEX_RMrX,16),INVALID,		TSaZ("vpmaskmov",VEX_RRM,16),INVALID,

            	TNSZ("vpgatherd",VEX_SbVM,16),TNSZ("vpgatherq",VEX_SbVM,16),TNSZ("vgatherdp",VEX_SbVM,16),TNSZ("vgatherqp",VEX_SbVM,16),
            	INVALID,		INVALID,		TNSZ("vfmaddsub132p",FMA,16),TNSZ("vfmsubadd132p",FMA,16),
            	TNSZ("vfmadd132p",FMA,16),TNSZ("vfmadd132s",FMA,16),TNSZ("vfmsub132p",FMA,16),TNSZ("vfmsub132s",FMA,16),
            	TNSZ("vfnmadd132p",FMA,16),TNSZ("vfnmadd132s",FMA,16),TNSZ("vfnmsub132p",FMA,16),TNSZ("vfnmsub132s",FMA,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vfmaddsub213p",FMA,16),TNSZ("vfmsubadd213p",FMA,16),
            	TNSZ("vfmadd213p",FMA,16),TNSZ("vfmadd213s",FMA,16),TNSZ("vfmsub213p",FMA,16),TNSZ("vfmsub213s",FMA,16),
            	TNSZ("vfnmadd213p",FMA,16),TNSZ("vfnmadd213s",FMA,16),TNSZ("vfnmsub213p",FMA,16),TNSZ("vfnmsub213s",FMA,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vfmaddsub231p",FMA,16),TNSZ("vfmsubadd231p",FMA,16),
            	TNSZ("vfmadd231p",FMA,16),TNSZ("vfmadd231s",FMA,16),TNSZ("vfmsub231p",FMA,16),TNSZ("vfmsub231s",FMA,16),
            	TNSZ("vfnmadd231p",FMA,16),TNSZ("vfnmadd231s",FMA,16),TNSZ("vfnmsub231p",FMA,16),TNSZ("vfnmsub231s",FMA,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("vaesimc",VEX_MX,16),
            	TNSZ("vaesenc",VEX_RMrX,16),TNSZ("vaesenclast",VEX_RMrX,16),TNSZ("vaesdec",VEX_RMrX,16),TNSZ("vaesdeclast",VEX_RMrX,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	IND(dis_op0F38F0),	IND(dis_op0F38F1),	INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZvr("shlx",VEX_VRMrX,5),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_op0F3A[256] = {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("roundps",XMMP_66r,16),TNSZ("roundpd",XMMP_66r,16),TNSZ("roundss",XMMP_66r,16),TNSZ("roundsd",XMMP_66r,16),
            	TNSZ("blendps",XMMP_66r,16),TNSZ("blendpd",XMMP_66r,16),TNSZ("pblendw",XMMP_66r,16),TNSZ("palignr",XMMP_66o,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("pextrb",XMM3PM_66r,8),TNSZ("pextrw",XMM3PM_66r,16),TSZ("pextr",XMM3PM_66r,16),TNSZ("extractps",XMM3PM_66r,16),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("pinsrb",XMMPRM_66r,8),TNSZ("insertps",XMMP_66r,16),TSZ("pinsr",XMMPRM_66r,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("dpps",XMMP_66r,16),TNSZ("dppd",XMMP_66r,16),TNSZ("mpsadbw",XMMP_66r,16),INVALID,
            	TNSZ("pclmulqdq",XMMP_66r,16),INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("pcmpestrm",XMMP_66r,16),TNSZ("pcmpestri",XMMP_66r,16),TNSZ("pcmpistrm",XMMP_66r,16),TNSZ("pcmpistri",XMMP_66r,16),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("sha1rnds4",XMMP,16),INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("aeskeygenassist",XMMP_66r,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

const instable_t dis_opAVX660F3A[256] = {
            	TNSZ("vpermq",VEX_MXI,16),TNSZ("vpermpd",VEX_MXI,16),TNSZ("vpblendd",VEX_RMRX,16),INVALID,
            	TNSZ("vpermilps",VEX_MXI,8),TNSZ("vpermilpd",VEX_MXI,16),TNSZ("vperm2f128",VEX_RMRX,16),INVALID,
            	TNSZ("vroundps",VEX_MXI,16),TNSZ("vroundpd",VEX_MXI,16),TNSZ("vroundss",VEX_RMRX,16),TNSZ("vroundsd",VEX_RMRX,16),
            	TNSZ("vblendps",VEX_RMRX,16),TNSZ("vblendpd",VEX_RMRX,16),TNSZ("vpblendw",VEX_RMRX,16),TNSZ("vpalignr",VEX_RMRX,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vpextrb",VEX_RRi,8),TNSZ("vpextrw",VEX_RRi,16),TNSZ("vpextrd",VEX_RRi,16),TNSZ("vextractps",VEX_RM,16),
            	TNSZ("vinsertf128",VEX_RMRX,16),TNSZ("vextractf128",VEX_RX,16),INVALID,		INVALID,
            	INVALID,		TNSZ("vcvtps2ph",VEX_RX,16),		INVALID,		INVALID,

            	TNSZ("vpinsrb",VEX_RMRX,8),TNSZ("vinsertps",VEX_RMRX,16),TNSZ("vpinsrd",VEX_RMRX,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vinserti128",VEX_RMRX,16),TNSZ("vextracti128",VEX_RIM,16),INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vdpps",VEX_RMRX,16),TNSZ("vdppd",VEX_RMRX,16),TNSZ("vmpsadbw",VEX_RMRX,16),INVALID,
            	TNSZ("vpclmulqdq",VEX_RMRX,16),INVALID,		TNSZ("vperm2i128",VEX_RMRX,16),INVALID,
            	INVALID,		INVALID,		TNSZ("vblendvps",VEX_RMRX,8),	TNSZ("vblendvpd",VEX_RMRX,16),
            	TNSZ("vpblendvb",VEX_RMRX,16),INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	TNSZ("vpcmpestrm",VEX_MXI,16),TNSZ("vpcmpestri",VEX_MXI,16),TNSZ("vpcmpistrm",VEX_MXI,16),TNSZ("vpcmpistri",VEX_MXI,16),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNSZ("vaeskeygenassist",VEX_MXI,16),

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,

            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

  
                                                                      
                        
   
const instable_t dis_op0F0D[8] = {
            	INVALID,		TNS("prefetchw",PREF),	TNS("prefetchwt1",PREF),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
};

  
                                
   

const instable_t dis_op0F[16][16] = {
{
            	IND(dis_op0F00),	IND(dis_op0F01),	TNS("lar",MR),		TNS("lsl",MR),
            	INVALID,		TNS("syscall",NORM),	TNS("clts",NORM),	TNS("sysret",NORM),
            	TNS("invd",NORM),	TNS("wbinvd",NORM),	INVALID,		TNS("ud2",NORM),
            	INVALID,		IND(dis_op0F0D),	INVALID,		INVALID,
}, {
            	TNSZ("movups",XMMO,16),	TNSZ("movups",XMMOS,16),TNSZ("movlps",XMMO,8),	TNSZ("movlps",XMMOS,8),
            	TNSZ("unpcklps",XMMO,16),TNSZ("unpckhps",XMMO,16),TNSZ("movhps",XMMOM,8),TNSZ("movhps",XMMOMS,8),
            	IND(dis_op0F18),	INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TS("nop",Mw),
}, {
            	TSy("mov",SREG),	TSy("mov",SREG),	TSy("mov",SREG),	TSy("mov",SREG),
            	TSx("mov",SREG),	INVALID,		TSx("mov",SREG),	INVALID,
            	TNSZ("movaps",XMMO,16),	TNSZ("movaps",XMMOS,16),TNSZ("cvtpi2ps",XMMOMX,8),TNSZ("movntps",XMMOS,16),
            	TNSZ("cvttps2pi",XMMOXMM,8),TNSZ("cvtps2pi",XMMOXMM,8),TNSZ("ucomiss",XMMO,4),TNSZ("comiss",XMMO,4),
}, {
            	TNS("wrmsr",NORM),	TNS("rdtsc",NORM),	TNS("rdmsr",NORM),	TNS("rdpmc",NORM),
            	TNSx("sysenter",NORM),	TNSx("sysexit",NORM),	INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	TS("cmovx.o",MR),	TS("cmovx.no",MR),	TS("cmovx.b",MR),	TS("cmovx.ae",MR),
            	TS("cmovx.e",MR),	TS("cmovx.ne",MR),	TS("cmovx.be",MR),	TS("cmovx.a",MR),
            	TS("cmovx.s",MR),	TS("cmovx.ns",MR),	TS("cmovx.pe",MR),	TS("cmovx.po",MR),
            	TS("cmovx.l",MR),	TS("cmovx.ge",MR),	TS("cmovx.le",MR),	TS("cmovx.g",MR),
}, {
            	TNS("movmskps",XMMOX3),	TNSZ("sqrtps",XMMO,16),	TNSZ("rsqrtps",XMMO,16),TNSZ("rcpps",XMMO,16),
            	TNSZ("andps",XMMO,16),	TNSZ("andnps",XMMO,16),	TNSZ("orps",XMMO,16),	TNSZ("xorps",XMMO,16),
            	TNSZ("addps",XMMO,16),	TNSZ("mulps",XMMO,16),	TNSZ("cvtps2pd",XMMO,8),TNSZ("cvtdq2ps",XMMO,16),
            	TNSZ("subps",XMMO,16),	TNSZ("minps",XMMO,16),	TNSZ("divps",XMMO,16),	TNSZ("maxps",XMMO,16),
}, {
            	TNSZ("punpcklbw",MMO,4),TNSZ("punpcklwd",MMO,4),TNSZ("punpckldq",MMO,4),TNSZ("packsswb",MMO,8),
            	TNSZ("pcmpgtb",MMO,8),	TNSZ("pcmpgtw",MMO,8),	TNSZ("pcmpgtd",MMO,8),	TNSZ("packuswb",MMO,8),
            	TNSZ("punpckhbw",MMO,8),TNSZ("punpckhwd",MMO,8),TNSZ("punpckhdq",MMO,8),TNSZ("packssdw",MMO,8),
            	TNSZ("INVALID",MMO,0),	TNSZ("INVALID",MMO,0),	TNSZ("movd",MMO,4),	TNSZ("movq",MMO,8),
}, {
            	TNSZ("pshufw",MMOPM,8),	TNS("psrXXX",MR),	TNS("psrXXX",MR),	TNS("psrXXX",MR),
            	TNSZ("pcmpeqb",MMO,8),	TNSZ("pcmpeqw",MMO,8),	TNSZ("pcmpeqd",MMO,8),	TNS("emms",NORM),
            	TNSy("vmread",RM),	TNSy("vmwrite",MR),	INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("movd",MMOS,4),	TNSZ("movq",MMOS,8),
}, {
            	TNS("jo",D),		TNS("jno",D),		TNS("jb",D),		TNS("jae",D),
            	TNS("je",D),		TNS("jne",D),		TNS("jbe",D),		TNS("ja",D),
            	TNS("js",D),		TNS("jns",D),		TNS("jp",D),		TNS("jnp",D),
            	TNS("jl",D),		TNS("jge",D),		TNS("jle",D),		TNS("jg",D),
}, {
            	TNS("seto",Mb),		TNS("setno",Mb),	TNS("setb",Mb),		TNS("setae",Mb),
            	TNS("sete",Mb),		TNS("setne",Mb),	TNS("setbe",Mb),	TNS("seta",Mb),
            	TNS("sets",Mb),		TNS("setns",Mb),	TNS("setp",Mb),		TNS("setnp",Mb),
            	TNS("setl",Mb),		TNS("setge",Mb),	TNS("setle",Mb),	TNS("setg",Mb),
}, {
            	TSp("push",LSEG),	TSp("pop",LSEG),	TNS("cpuid",NORM),	TS("bt",RMw),
            	TS("shld",DSHIFT),	TS("shld",DSHIFTcl),	INVALID,		INVALID,
            	TSp("push",LSEG),	TSp("pop",LSEG),	TNS("rsm",NORM),	TS("bts",RMw),
            	TS("shrd",DSHIFT),	TS("shrd",DSHIFTcl),	IND(dis_op0FAE),	TS("imul",MRw),
}, {
            	TNS("cmpxchgb",RMw),	TS("cmpxchg",RMw),	TS("lss",MR),		TS("btr",RMw),
            	TS("lfs",MR),		TS("lgs",MR),		TS("movzb",MOVZ),	TNS("movzwl",MOVZ),
            	TNS("INVALID",MRw),	INVALID,		IND(dis_op0FBA),	TS("btc",RMw),
            	TS("bsf",MRw),		TS("bsr",MRw),		TS("movsb",MOVZ),	TNS("movswl",MOVZ),
}, {
            	TNS("xaddb",XADDB),	TS("xadd",RMw),		TNSZ("cmpps",XMMOPM,16),TNS("movnti",RM),
            	TNSZ("pinsrw",MMOPRM,2),TNS("pextrw",MMO3P), 	TNSZ("shufps",XMMOPM,16),IND(dis_op0FC7),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		TNSZ("psrlw",MMO,8),	TNSZ("psrld",MMO,8),	TNSZ("psrlq",MMO,8),
            	TNSZ("paddq",MMO,8),	TNSZ("pmullw",MMO,8),	TNSZ("INVALID",MMO,0),	TNS("pmovmskb",MMOM3),
            	TNSZ("psubusb",MMO,8),	TNSZ("psubusw",MMO,8),	TNSZ("pminub",MMO,8),	TNSZ("pand",MMO,8),
            	TNSZ("paddusb",MMO,8),	TNSZ("paddusw",MMO,8),	TNSZ("pmaxub",MMO,8),	TNSZ("pandn",MMO,8),
}, {
            	TNSZ("pavgb",MMO,8),	TNSZ("psraw",MMO,8),	TNSZ("psrad",MMO,8),	TNSZ("pavgw",MMO,8),
            	TNSZ("pmulhuw",MMO,8),	TNSZ("pmulhw",MMO,8),	TNS("INVALID",XMMO),	TNSZ("movntq",MMOMS,8),
            	TNSZ("psubsb",MMO,8),	TNSZ("psubsw",MMO,8),	TNSZ("pminsw",MMO,8),	TNSZ("por",MMO,8),
            	TNSZ("paddsb",MMO,8),	TNSZ("paddsw",MMO,8),	TNSZ("pmaxsw",MMO,8),	TNSZ("pxor",MMO,8),
}, {
            	INVALID,		TNSZ("psllw",MMO,8),	TNSZ("pslld",MMO,8),	TNSZ("psllq",MMO,8),
            	TNSZ("pmuludq",MMO,8),	TNSZ("pmaddwd",MMO,8),	TNSZ("psadbw",MMO,8),	TNSZ("maskmovq",MMOIMPL,8),
            	TNSZ("psubb",MMO,8),	TNSZ("psubw",MMO,8),	TNSZ("psubd",MMO,8),	TNSZ("psubq",MMO,8),
            	TNSZ("paddb",MMO,8),	TNSZ("paddw",MMO,8),	TNSZ("paddd",MMO,8),	INVALID,
} };

const instable_t dis_opAVX0F[16][16] = {
{
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	TNSZ("vmovups",VEX_MX,16),	TNSZ("vmovups",VEX_RM,16),TNSZ("vmovlps",VEX_RMrX,8),	TNSZ("vmovlps",VEX_RM,8),
            	TNSZ("vunpcklps",VEX_RMrX,16),TNSZ("vunpckhps",VEX_RMrX,16),TNSZ("vmovhps",VEX_RMrX,8),TNSZ("vmovhps",VEX_RM,8),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	TNSZ("vmovaps",VEX_MX,16),	TNSZ("vmovaps",VEX_RX,16),INVALID,		TNSZ("vmovntps",VEX_RM,16),
            	INVALID,		INVALID,		TNSZ("vucomiss",VEX_MX,4),TNSZ("vcomiss",VEX_MX,4),
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	TNS("vmovmskps",VEX_MR),	TNSZ("vsqrtps",VEX_MX,16),	TNSZ("vrsqrtps",VEX_MX,16),TNSZ("vrcpps",VEX_MX,16),
            	TNSZ("vandps",VEX_RMrX,16),	TNSZ("vandnps",VEX_RMrX,16),	TNSZ("vorps",VEX_RMrX,16),	TNSZ("vxorps",VEX_RMrX,16),
            	TNSZ("vaddps",VEX_RMrX,16),	TNSZ("vmulps",VEX_RMrX,16),	TNSZ("vcvtps2pd",VEX_MX,8),TNSZ("vcvtdq2ps",VEX_MX,16),
            	TNSZ("vsubps",VEX_RMrX,16),	TNSZ("vminps",VEX_RMrX,16),	TNSZ("vdivps",VEX_RMrX,16),	TNSZ("vmaxps",VEX_RMrX,16),
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		TNS("vzeroupper", VEX_NONE),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		TNSZ("vldmxcsr",VEX_MO,2),		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		TNSZ("vcmpps",VEX_RMRX,16),INVALID,
            	INVALID,		INVALID,	 	TNSZ("vshufps",VEX_RMRX,16),INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
}, {
            	INVALID,		INVALID,		TNSZvr("andn",VEX_RMrX,5),TNSZvr("bls",BLS,5),
            	INVALID,		TNSZvr("bzhi",VEX_VRMrX,5),INVALID,		TNSZvr("bextr",VEX_VRMrX,5),
            	INVALID,		INVALID,		INVALID,		INVALID,
            	INVALID,		INVALID,		INVALID,		INVALID,
} };

  
                                
   

const instable_t dis_op80[8] = {

           	TNS("addb",IMlw),	TNS("orb",IMw),		TNS("adcb",IMlw),	TNS("sbbb",IMlw),
           	TNS("andb",IMw),	TNS("subb",IMlw),	TNS("xorb",IMw),	TNS("cmpb",IMlw),
};


  
                                 
   

const instable_t dis_op81[8] = {

           	TS("add",IMlw),		TS("or",IMw),		TS("adc",IMlw),		TS("sbb",IMlw),
           	TS("and",IMw),		TS("sub",IMlw),		TS("xor",IMw),		TS("cmp",IMlw),
};


  
                                 
   

const instable_t dis_op82[8] = {

           	TNSx("addb",IMlw),	TNSx("orb",IMlw),	TNSx("adcb",IMlw),	TNSx("sbbb",IMlw),
           	TNSx("andb",IMlw),	TNSx("subb",IMlw),	TNSx("xorb",IMlw),	TNSx("cmpb",IMlw),
};
  
                                 
   

const instable_t dis_op83[8] = {

           	TS("add",IMlw),		TS("or",IMlw),		TS("adc",IMlw),		TS("sbb",IMlw),
           	TS("and",IMlw),		TS("sub",IMlw),		TS("xor",IMlw),		TS("cmp",IMlw),
};

  
                                 
   

const instable_t dis_opC0[8] = {

           	TNS("rolb",MvI),	TNS("rorb",MvI),	TNS("rclb",MvI),	TNS("rcrb",MvI),
           	TNS("shlb",MvI),	TNS("shrb",MvI),	INVALID,		TNS("sarb",MvI),
};

  
                                 
   

const instable_t dis_opD0[8] = {

           	TNS("rolb",Mv),		TNS("rorb",Mv),		TNS("rclb",Mv),		TNS("rcrb",Mv),
           	TNS("shlb",Mv),		TNS("shrb",Mv),		TNS("salb",Mv),		TNS("sarb",Mv),
};

  
                                 
                      
   

const instable_t dis_opC1[8] = {

           	TS("rol",MvI),		TS("ror",MvI),		TS("rcl",MvI),		TS("rcr",MvI),
           	TS("shl",MvI),		TS("shr",MvI),		TS("sal",MvI),		TS("sar",MvI),
};

  
                                 
   

const instable_t dis_opD1[8] = {

           	TS("rol",Mv),		TS("ror",Mv),		TS("rcl",Mv),		TS("rcr",Mv),
           	TS("shl",Mv),		TS("shr",Mv),		TS("sal",Mv),		TS("sar",Mv),
};


  
                                 
   

const instable_t dis_opD2[8] = {

           	TNS("rolb",Mv),		TNS("rorb",Mv),		TNS("rclb",Mv),		TNS("rcrb",Mv),
           	TNS("shlb",Mv),		TNS("shrb",Mv),		TNS("salb",Mv),		TNS("sarb",Mv),
};
  
                                 
   

const instable_t dis_opD3[8] = {

           	TS("rol",Mv),		TS("ror",Mv),		TS("rcl",Mv),		TS("rcr",Mv),
           	TS("shl",Mv),		TS("shr",Mv),		TS("salb",Mv),		TS("sar",Mv),
};


  
                                 
   

const instable_t dis_opF6[8] = {

           	TNS("testb",IMw),	TNS("testb",IMw),	TNS("notb",Mw),		TNS("negb",Mw),
           	TNS("mulb",MA),		TNS("imulb",MA),	TNS("divb",MA),		TNS("idivb",MA),
};


  
                                 
   

const instable_t dis_opF7[8] = {

           	TS("test",IMw),		TS("test",IMw),		TS("not",Mw),		TS("neg",Mw),
           	TS("mul",MA),		TS("imul",MA),		TS("div",MA),		TS("idiv",MA),
};


  
                                 
   

const instable_t dis_opFE[8] = {

           	TNS("incb",Mw),		TNS("decb",Mw),		INVALID,		INVALID,
           	INVALID,		INVALID,		INVALID,		INVALID,
};
  
                                 
   

const instable_t dis_opFF[8] = {

           	TS("inc",Mw),		TS("dec",Mw),		TNSyp("call",INM),	TNS("lcall",INM),
           	TNSy("jmp",INM),	TNS("ljmp",INM),	TSp("push",M),		INVALID,
};

                                                      

const instable_t dis_opFP1n2[8][8] = {
{
                                       
            	TNS("fadds",M),		TNS("fmuls",M),		TNS("fcoms",M),		TNS("fcomps",M),
            	TNS("fsubs",M),		TNS("fsubrs",M),	TNS("fdivs",M),		TNS("fdivrs",M),
}, {
             	TNS("flds",M),		INVALID,		TNS("fsts",M),		TNS("fstps",M),
             	TNSZ("fldenv",M,28),	TNSZ("fldcw",M,2),	TNSZ("fnstenv",M,28),	TNSZ("fnstcw",M,2),
}, {
             	TNS("fiaddl",M),	TNS("fimull",M),	TNS("ficoml",M),	TNS("ficompl",M),
             	TNS("fisubl",M),	TNS("fisubrl",M),	TNS("fidivl",M),	TNS("fidivrl",M),
}, {
             	TNS("fildl",M),		TNSZ("tisttpl",M,4),	TNS("fistl",M),		TNS("fistpl",M),
             	INVALID,		TNSZ("fldt",M,10),	INVALID,		TNSZ("fstpt",M,10),
}, {
             	TNSZ("faddl",M,8),	TNSZ("fmull",M,8),	TNSZ("fcoml",M,8),	TNSZ("fcompl",M,8),
             	TNSZ("fsubl",M,8),	TNSZ("fsubrl",M,8),	TNSZ("fdivl",M,8),	TNSZ("fdivrl",M,8),
}, {
             	TNSZ("fldl",M,8),	TNSZ("fisttpll",M,8),	TNSZ("fstl",M,8),	TNSZ("fstpl",M,8),
             	TNSZ("frstor",M,108),	INVALID,		TNSZ("fnsave",M,108),	TNSZ("fnstsw",M,2),
}, {
             	TNSZ("fiadd",M,2),	TNSZ("fimul",M,2),	TNSZ("ficom",M,2),	TNSZ("ficomp",M,2),
             	TNSZ("fisub",M,2),	TNSZ("fisubr",M,2),	TNSZ("fidiv",M,2),	TNSZ("fidivr",M,2),
}, {
             	TNSZ("fild",M,2),	TNSZ("fisttp",M,2),	TNSZ("fist",M,2),	TNSZ("fistp",M,2),
             	TNSZ("fbld",M,10),	TNSZ("fildll",M,8),	TNSZ("fbstp",M,10),	TNSZ("fistpll",M,8),
} };

const instable_t dis_opFP3[8][8] = {
{
                                       
             	TNS("fadd",FF),		TNS("fmul",FF),		TNS("fcom",F),		TNS("fcomp",F),
             	TNS("fsub",FF),		TNS("fsubr",FF),	TNS("fdiv",FF),		TNS("fdivr",FF),
}, {
             	TNS("fld",F),		TNS("fxch",F),		TNS("fnop",NORM),	TNS("fstp",F),
             	INVALID,		INVALID,		INVALID,		INVALID,
}, {
             	INVALID,		INVALID,		INVALID,		INVALID,
             	INVALID,		TNS("fucompp",NORM),	INVALID,		INVALID,
}, {
             	INVALID,		INVALID,		INVALID,		INVALID,
             	INVALID,		INVALID,		INVALID,		INVALID,
}, {
             	TNS("fadd",FF),		TNS("fmul",FF),		TNS("fcom",F),		TNS("fcomp",F),
             	TNS("fsub",FF),		TNS("fsubr",FF),	TNS("fdiv",FF),		TNS("fdivr",FF),
}, {
             	TNS("ffree",F),		TNS("fxch",F),		TNS("fst",F),		TNS("fstp",F),
             	TNS("fucom",F),		TNS("fucomp",F),	INVALID,		INVALID,
}, {
             	TNS("faddp",FF),	TNS("fmulp",FF),	TNS("fcomp",F),		TNS("fcompp",NORM),
             	TNS("fsubp",FF),	TNS("fsubrp",FF),	TNS("fdivp",FF),	TNS("fdivrp",FF),
}, {
             	TNS("ffreep",F),		TNS("fxch",F),		TNS("fstp",F),		TNS("fstp",F),
             	TNS("fnstsw",M),	TNS("fucomip",FFC),	TNS("fcomip",FFC),	INVALID,
} };

const instable_t dis_opFP4[4][8] = {
{
                                      
             	TNS("fchs",NORM),	TNS("fabs",NORM),	INVALID,		INVALID,
             	TNS("ftst",NORM),	TNS("fxam",NORM),	TNS("ftstp",NORM),	INVALID,
}, {
             	TNS("fld1",NORM),	TNS("fldl2t",NORM),	TNS("fldl2e",NORM),	TNS("fldpi",NORM),
             	TNS("fldlg2",NORM),	TNS("fldln2",NORM),	TNS("fldz",NORM),	INVALID,
}, {
             	TNS("f2xm1",NORM),	TNS("fyl2x",NORM),	TNS("fptan",NORM),	TNS("fpatan",NORM),
             	TNS("fxtract",NORM),	TNS("fprem1",NORM),	TNS("fdecstp",NORM),	TNS("fincstp",NORM),
}, {
             	TNS("fprem",NORM),	TNS("fyl2xp1",NORM),	TNS("fsqrt",NORM),	TNS("fsincos",NORM),
             	TNS("frndint",NORM),	TNS("fscale",NORM),	TNS("fsin",NORM),	TNS("fcos",NORM),
} };

const instable_t dis_opFP5[8] = {
                                      
           	TNS("feni",NORM),	TNS("fdisi",NORM),	TNS("fnclex",NORM),	TNS("fninit",NORM),
           	TNS("fsetpm",NORM),	TNS("frstpm",NORM),	INVALID,		INVALID,
};

const instable_t dis_opFP6[8] = {
                                      
            	TNS("fcmov.nb",FF),	TNS("fcmov.ne",FF),	TNS("fcmov.nbe",FF),	TNS("fcmov.nu",FF),
            	INVALID,		TNS("fucomi",F),	TNS("fcomi",F),		INVALID,
};

const instable_t dis_opFP7[8] = {
                                      
            	TNS("fcmov.b",FF),	TNS("fcmov.e",FF),	TNS("fcmov.be",FF),	TNS("fcmov.u",FF),
            	INVALID,		INVALID,		INVALID,		INVALID,
};

  
                                                             
                                                          
                                                           
                                                           
         
   

const instable_t dis_distable[16][16] = {
{
           	TNS("addb",RMw),	TS("add",RMw),		TNS("addb",MRw),	TS("add",MRw),
           	TNS("addb",IA),		TS("add",IA),		TSx("push",SEG),	TSx("pop",SEG),
           	TNS("orb",RMw),		TS("or",RMw),		TNS("orb",MRw),		TS("or",MRw),
           	TNS("orb",IA),		TS("or",IA),		TSx("push",SEG),	IND(dis_op0F),
}, {
           	TNS("adcb",RMw),	TS("adc",RMw),		TNS("adcb",MRw),	TS("adc",MRw),
           	TNS("adcb",IA),		TS("adc",IA),		TSx("push",SEG),	TSx("pop",SEG),
           	TNS("sbbb",RMw),	TS("sbb",RMw),		TNS("sbbb",MRw),	TS("sbb",MRw),
           	TNS("sbbb",IA),		TS("sbb",IA),		TSx("push",SEG),	TSx("pop",SEG),
}, {
           	TNS("andb",RMw),	TS("and",RMw),		TNS("andb",MRw),	TS("and",MRw),
           	TNS("andb",IA),		TS("and",IA),		TNS("%es:",OVERRIDE),	TNSx("daa",NORM),
           	TNS("subb",RMw),	TS("sub",RMw),		TNS("subb",MRw),	TS("sub",MRw),
           	TNS("subb",IA),		TS("sub",IA),		TNS("%cs:",OVERRIDE),	TNSx("das",NORM),
}, {
           	TNS("xorb",RMw),	TS("xor",RMw),		TNS("xorb",MRw),	TS("xor",MRw),
           	TNS("xorb",IA),		TS("xor",IA),		TNS("%ss:",OVERRIDE),	TNSx("aaa",NORM),
           	TNS("cmpb",RMw),	TS("cmp",RMw),		TNS("cmpb",MRw),	TS("cmp",MRw),
           	TNS("cmpb",IA),		TS("cmp",IA),		TNS("%ds:",OVERRIDE),	TNSx("aas",NORM),
}, {
           	TSx("inc",R),		TSx("inc",R),		TSx("inc",R),		TSx("inc",R),
           	TSx("inc",R),		TSx("inc",R),		TSx("inc",R),		TSx("inc",R),
           	TSx("dec",R),		TSx("dec",R),		TSx("dec",R),		TSx("dec",R),
           	TSx("dec",R),		TSx("dec",R),		TSx("dec",R),		TSx("dec",R),
}, {
           	TSp("push",R),		TSp("push",R),		TSp("push",R),		TSp("push",R),
           	TSp("push",R),		TSp("push",R),		TSp("push",R),		TSp("push",R),
           	TSp("pop",R),		TSp("pop",R),		TSp("pop",R),		TSp("pop",R),
           	TSp("pop",R),		TSp("pop",R),		TSp("pop",R),		TSp("pop",R),
}, {
           	TSZx("pusha",IMPLMEM,28),TSZx("popa",IMPLMEM,28), TSx("bound",MR),	TNS("arpl",RMw),
           	TNS("%fs:",OVERRIDE),	TNS("%gs:",OVERRIDE),	TNS("data16",DM),	TNS("addr16",AM),
           	TSp("push",I),		TS("imul",IMUL),	TSp("push",Ib),	TS("imul",IMUL),
           	TNSZ("insb",IMPLMEM,1),	TSZ("ins",IMPLMEM,4),	TNSZ("outsb",IMPLMEM,1),TSZ("outs",IMPLMEM,4),
}, {
           	TNSy("jo",BD),		TNSy("jno",BD),		TNSy("jb",BD),		TNSy("jae",BD),
           	TNSy("je",BD),		TNSy("jne",BD),		TNSy("jbe",BD),		TNSy("ja",BD),
           	TNSy("js",BD),		TNSy("jns",BD),		TNSy("jp",BD),		TNSy("jnp",BD),
           	TNSy("jl",BD),		TNSy("jge",BD),		TNSy("jle",BD),		TNSy("jg",BD),
}, {
           	IND(dis_op80),		IND(dis_op81),		INDx(dis_op82),		IND(dis_op83),
           	TNS("testb",RMw),	TS("test",RMw),		TNS("xchgb",RMw),	TS("xchg",RMw),
           	TNS("movb",RMw),	TS("mov",RMw),		TNS("movb",MRw),	TS("mov",MRw),
           	TNS("movw",SM),		TS("lea",MR),		TNS("movw",MS),		TSp("pop",M),
}, {
           	TNS("nop",NORM),	TS("xchg",RA),		TS("xchg",RA),		TS("xchg",RA),
           	TS("xchg",RA),		TS("xchg",RA),		TS("xchg",RA),		TS("xchg",RA),
           	TNS("cXtX",CBW),	TNS("cXtX",CWD),	TNSx("lcall",SO),	TNS("fwait",NORM),
           	TSZy("pushf",IMPLMEM,4),TSZy("popf",IMPLMEM,4),	TNS("sahf",NORM),	TNS("lahf",NORM),
}, {
           	TNS("movb",OA),		TS("mov",OA),		TNS("movb",AO),		TS("mov",AO),
           	TNSZ("movsb",SD,1),	TS("movs",SD),		TNSZ("cmpsb",SD,1),	TS("cmps",SD),
           	TNS("testb",IA),	TS("test",IA),		TNS("stosb",AD),	TS("stos",AD),
           	TNS("lodsb",SA),	TS("lods",SA),		TNS("scasb",AD),	TS("scas",AD),
}, {
           	TNS("movb",IR),		TNS("movb",IR),		TNS("movb",IR),		TNS("movb",IR),
           	TNS("movb",IR),		TNS("movb",IR),		TNS("movb",IR),		TNS("movb",IR),
           	TS("mov",IR),		TS("mov",IR),		TS("mov",IR),		TS("mov",IR),
           	TS("mov",IR),		TS("mov",IR),		TS("mov",IR),		TS("mov",IR),
}, {
           	IND(dis_opC0),		IND(dis_opC1), 		TNSyp("ret",RET),	TNSyp("ret",NORM),
           	TNSx("les",MR),		TNSx("lds",MR),		TNS("movb",IMw),	TS("mov",IMw),
           	TNSyp("enter",ENTER),	TNSyp("leave",NORM),	TNS("lret",RET),	TNS("lret",NORM),
           	TNS("int",INT3),	TNS("int",INTx),	TNSx("into",NORM),	TNS("iret",NORM),
}, {
           	IND(dis_opD0),		IND(dis_opD1),		IND(dis_opD2),		IND(dis_opD3),
           	TNSx("aam",U),		TNSx("aad",U),		TNSx("falc",NORM),	TNSZ("xlat",IMPLMEM,1),

                                                               
                                                                     
                                                                          
                                                                   
                                            
           	IND(dis_opFP1n2),	IND(dis_opFP1n2),	IND(dis_opFP1n2),	IND(dis_opFP1n2),
           	IND(dis_opFP1n2),	IND(dis_opFP1n2),	IND(dis_opFP1n2),	IND(dis_opFP1n2),
}, {
           	TNSy("loopnz",BD),	TNSy("loopz",BD),	TNSy("loop",BD),	TNSy("jcxz",BD),
           	TNS("inb",P),		TS("in",P),		TNS("outb",P),		TS("out",P),
           	TNSyp("call",D),	TNSy("jmp",D),		TNSx("ljmp",SO),		TNSy("jmp",BD),
           	TNS("inb",V),		TS("in",V),		TNS("outb",V),		TS("out",V),
}, {
           	TNS("lock",LOCK),	TNS("icebp", NORM),	TNS("repnz",PREFIX),	TNS("repz",PREFIX),
           	TNS("hlt",NORM),	TNS("cmc",NORM),	IND(dis_opF6),		IND(dis_opF7),
           	TNS("clc",NORM),	TNS("stc",NORM),	TNS("cli",NORM),	TNS("sti",NORM),
           	TNS("cld",NORM),	TNS("std",NORM),	IND(dis_opFE),		IND(dis_opFF),
} };

                 

  
                                                                         
   

  
                                                                   
                                                    
                                                           
                                                
   
#define	REX_W 0x08	                                  
#define	REX_R 0x04	                                                 
#define	REX_X 0x02	                                                 
#define	REX_B 0x01	                                                

  
                                                   
   
#define	VEX_R 0x08	                                  
#define	VEX_X 0x04	                                  
#define	VEX_B 0x02	                                  
                                                                   
#define	VEX_L 0x04
#define	VEX_W 0x08	                                     
#define	VEX_m 0x1F	                      
#define	VEX_v 0x78	                            
#define	VEX_p 0x03	                                    

                                                       
#define	VEX_m_0F 0x01                                       
#define	VEX_m_0F38 0x02                                        
#define	VEX_m_0F3A 0x03                                        

                                                                       
#define	VEX_p_66 0x01
#define	VEX_p_F3 0x02
#define	VEX_p_F2 0x03

  
                                                                             
   
static int isize[] = {1, 2, 4, 4};
static int isize64[] = {1, 2, 4, 8};

  
                                 
   
#define	WBIT(x)	(x & 0x1)		                  
#define	REGNO(x) (x & 0x7)		                           
#define	VBIT(x)	((x)>>1 & 0x1)		                    
#define	OPSIZE(osize, wbit) ((wbit) ? isize[osize] : 1)
#define	OPSIZE64(osize, wbit) ((wbit) ? isize64[osize] : 1)

#define	REG_ONLY 3	                                                      

#define	BYTE_OPND	0	                                          
#define	LONG_OPND	1	                                               
#define	MM_OPND		2	                                        
#define	XMM_OPND	3	                                        
#define	SEG_OPND	4	                                            
#define	CONTROL_OPND	5	                                            
#define	DEBUG_OPND	6	                                          
#define	TEST_OPND	7	                                         
#define	WORD_OPND	8	                                          
#define	YMM_OPND	9	                                        

  
                                                                             
                                                                               
                                                                             
                                                                            
                                                          
  
                                                                            
                                                                              
        
  
                                                                              
                                                       
                                  
   
                 
typedef struct dis_gather_regs {
	uint_t dgr_arg0;	             
	uint_t dgr_arg1;	              
	uint_t dgr_arg2;	             
	const char *dgr_suffix;	                      
} dis_gather_regs_t;

static dis_gather_regs_t dis_vgather[4][2][2] = {
	{
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "d" },
			{ YMM_OPND, YMM_OPND, YMM_OPND, "d" }
		},
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "q" },
			{ YMM_OPND, XMM_OPND, YMM_OPND, "q" }
		}
	},
	{
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "d" },
			{ XMM_OPND, YMM_OPND, XMM_OPND, "d" },
		},
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "q" },
			{ YMM_OPND, YMM_OPND, YMM_OPND, "q" },
		}
	},
	{
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "s" },
			{ YMM_OPND, YMM_OPND, YMM_OPND, "s" }
		},
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "d" },
			{ YMM_OPND, XMM_OPND, YMM_OPND, "d" }
		}
	},
	{
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "s" },
			{ XMM_OPND, YMM_OPND, XMM_OPND, "s" }
		},
		                  
		{
			{ XMM_OPND, XMM_OPND, XMM_OPND, "d" },
			{ YMM_OPND, YMM_OPND, YMM_OPND, "d" }
		}
	}
};

  
                                                                            
   
static int
dtrace_get_opcode(dis86_t *x, uint_t *high, uint_t *low)
{
	int byte;

	  
                                                                    
                        
    
	if (x->d86_len >= 15)
		return (x->d86_error = 1);

	if (x->d86_error)
		return (1);
	byte = x->d86_get_byte(x->d86_data);
	if (byte < 0)
		return (x->d86_error = 1);
	x->d86_bytes[x->d86_len++] = byte;
	*low = byte & 0xf;		                         
	*high = byte >> 4 & 0xf;	                          
	return (0);
}

  
                                                 
   
static void
dtrace_get_SIB(dis86_t *x, uint_t *ss, uint_t *index, uint_t *base)
{
	int byte;

	if (x->d86_error)
		return;

	byte = x->d86_get_byte(x->d86_data);
	if (byte < 0) {
		x->d86_error = 1;
		return;
	}
	x->d86_bytes[x->d86_len++] = byte;

	*base = byte & 0x7;
	*index = (byte >> 3) & 0x7;
	*ss = (byte >> 6) & 0x3;
}

  
                                                              
                                  
   
static void
dtrace_get_modrm(dis86_t *x, uint_t *mode, uint_t *reg, uint_t *r_m)
{
	if (x->d86_got_modrm == 0) {
		if (x->d86_rmindex == -1)
			x->d86_rmindex = x->d86_len;
		dtrace_get_SIB(x, mode, reg, r_m);
		x->d86_got_modrm = 1;
	}
}

  
                                                                  
   
            
static void
dtrace_rex_adjust(uint_t rex_prefix, uint_t mode, uint_t *reg, uint_t *r_m)
{
#pragma unused (mode)
	if (reg != NULL && r_m == NULL) {
		if (rex_prefix & REX_B)
			*reg += 8;
	} else {
		if (reg != NULL && (REX_R & rex_prefix) != 0)
			*reg += 8;
		if (r_m != NULL && (REX_B & rex_prefix) != 0)
			*r_m += 8;
	}
}

  
                                                                  
                                                                               
   
            
static void
dtrace_vex_adjust(uint_t vex_byte1, uint_t mode, uint_t *reg, uint_t *r_m)
{
#pragma unused (mode)
	if (reg != NULL && r_m == NULL) {
		if (!(vex_byte1 & VEX_B))
			*reg += 8;
	} else {
		if (reg != NULL && ((VEX_R & vex_byte1) == 0))
			*reg += 8;
		if (r_m != NULL && ((VEX_B & vex_byte1) == 0))
			*r_m += 8;
	}
}

  
                                                                   
   
static void
dtrace_imm_opnd(dis86_t *x, int wbit, int size, int opindex)
{
	int i;
	int byte;
	int valsize = 0;

	if (x->d86_numopnds < (uint_t)opindex + 1)
		x->d86_numopnds = (uint_t)opindex + 1;

	switch (wbit) {
	case BYTE_OPND:
		valsize = 1;
		break;
	case LONG_OPND:
		if (x->d86_opnd_size == SIZE16)
			valsize = 2;
		else if (x->d86_opnd_size == SIZE32)
			valsize = 4;
		else
			valsize = 8;
		break;
	case MM_OPND:
	case XMM_OPND:
	case YMM_OPND:
	case SEG_OPND:
	case CONTROL_OPND:
	case DEBUG_OPND:
	case TEST_OPND:
		valsize = size;
		break;
	case WORD_OPND:
		valsize = 2;
		break;
	}
	if (valsize < size)
		valsize = size;

	if (x->d86_error)
		return;
	x->d86_opnd[opindex].d86_value = 0;
	for (i = 0; i < size; ++i) {
		byte = x->d86_get_byte(x->d86_data);
		if (byte < 0) {
			x->d86_error = 1;
			return;
		}
		x->d86_bytes[x->d86_len++] = byte;
		x->d86_opnd[opindex].d86_value |= (uint64_t)byte << (i * 8);
	}
	                       
	if (x->d86_bytes[x->d86_len - 1] & 0x80) {
		for (; i < (int)sizeof (uint64_t); i++)
			x->d86_opnd[opindex].d86_value |=
			    (uint64_t)0xff << (i * 8);
	}
#ifdef DIS_TEXT
	x->d86_opnd[opindex].d86_mode = MODE_SIGNED;
	x->d86_opnd[opindex].d86_value_size = valsize;
	x->d86_imm_bytes += size;
#endif
}

  
                                                                     
   
static void
dtrace_disp_opnd(dis86_t *x, int wbit, int size, int opindex)
{
	dtrace_imm_opnd(x, wbit, size, opindex);
#ifdef DIS_TEXT
	x->d86_opnd[opindex].d86_mode = MODE_IPREL;
#endif
}

  
                                                              
                                                            
                                   
   
            
static void
dtrace_check_override(dis86_t *x, int opindex)
{
#ifdef DIS_TEXT
	if (x->d86_seg_prefix) {
		(void) strlcat(x->d86_opnd[opindex].d86_prefix,
		    x->d86_seg_prefix, PFIXLEN);
	}
#else
	#pragma unused (opindex)
#endif
	x->d86_seg_prefix = NULL;
}


  
                                                           
  
                                         
                                                        
                                                                           
                                                          
  
                                                                              
   
            
static void
dtrace_get_operand(dis86_t *x, uint_t mode, uint_t r_m, int wbit, int opindex)
{
	int have_SIB = 0;	                                       
	uint_t ss;		                              
	uint_t index;		                           
	uint_t base;		                          
	int dispsize;   	                                   
#ifdef DIS_TEXT
	char *opnd = x->d86_opnd[opindex].d86_opnd;
#else
	#pragma unused (wbit)
#endif

	if (x->d86_numopnds < (uint_t)opindex + 1)
		x->d86_numopnds = (uint_t)opindex + 1;

	if (x->d86_error)
		return;

	  
                                  
    
	if (mode == REG_ONLY) {
#ifdef DIS_TEXT
		switch (wbit) {
		case MM_OPND:
			(void) strlcat(opnd, dis_MMREG[r_m], OPLEN);
			break;
		case XMM_OPND:
			(void) strlcat(opnd, dis_XMMREG[r_m], OPLEN);
			break;
		case YMM_OPND:
			(void) strlcat(opnd, dis_YMMREG[r_m], OPLEN);
			break;
		case SEG_OPND:
			(void) strlcat(opnd, dis_SEGREG[r_m], OPLEN);
			break;
		case CONTROL_OPND:
			(void) strlcat(opnd, dis_CONTROLREG[r_m], OPLEN);
			break;
		case DEBUG_OPND:
			(void) strlcat(opnd, dis_DEBUGREG[r_m], OPLEN);
			break;
		case TEST_OPND:
			(void) strlcat(opnd, dis_TESTREG[r_m], OPLEN);
			break;
		case BYTE_OPND:
			if (x->d86_rex_prefix == 0)
				(void) strlcat(opnd, dis_REG8[r_m], OPLEN);
			else
				(void) strlcat(opnd, dis_REG8_REX[r_m], OPLEN);
			break;
		case WORD_OPND:
			(void) strlcat(opnd, dis_REG16[r_m], OPLEN);
			break;
		case LONG_OPND:
			if (x->d86_opnd_size == SIZE16)
				(void) strlcat(opnd, dis_REG16[r_m], OPLEN);
			else if (x->d86_opnd_size == SIZE32)
				(void) strlcat(opnd, dis_REG32[r_m], OPLEN);
			else
				(void) strlcat(opnd, dis_REG64[r_m], OPLEN);
			break;
		}
#endif               
		return;
	}

	  
                                                            
    
	dtrace_check_override(x, opindex);

	  
                                                            
                                
                                      
                                       
                                                            
    
	if (x->d86_addr_size == SIZE16) {
		if ((mode == 0 && r_m == 6) || mode == 2)
			dtrace_imm_opnd(x, WORD_OPND, 2, opindex);
		else if (mode == 1)
			dtrace_imm_opnd(x, BYTE_OPND, 1, opindex);
#ifdef DIS_TEXT
		if (mode == 0 && r_m == 6)
			x->d86_opnd[opindex].d86_mode = MODE_SIGNED;
		else if (mode == 0)
			x->d86_opnd[opindex].d86_mode = MODE_NONE;
		else
			x->d86_opnd[opindex].d86_mode = MODE_OFFSET;
		(void) strlcat(opnd, dis_addr16[mode][r_m], OPLEN);
#endif
		return;
	}

	  
                                                              
                                                              
    
	if (r_m == ESP_REGNO || r_m == ESP_REGNO + 8) {
		have_SIB = 1;
		dtrace_get_SIB(x, &ss, &index, &base);
		if (x->d86_error)
			return;
		if (base != 5 || mode != 0)
			if (x->d86_rex_prefix & REX_B)
				base += 8;
		if (x->d86_rex_prefix & REX_X)
			index += 8;
	} else {
		base = r_m;
	}

	  
                                                   
    
	dispsize = 0;

	if (mode == 1)
		dispsize = 1;
	else if (mode == 2)
		dispsize = 4;
	else if ((r_m & 7) == EBP_REGNO ||
	    (have_SIB && (base & 7) == EBP_REGNO))
		dispsize = 4;

	if (dispsize > 0) {
		dtrace_imm_opnd(x, dispsize == 4 ? LONG_OPND : BYTE_OPND,
		    dispsize, opindex);
		if (x->d86_error)
			return;
	}

#ifdef DIS_TEXT
	if (dispsize > 0)
		x->d86_opnd[opindex].d86_mode = MODE_OFFSET;

	if (have_SIB == 0) {
		if (x->d86_mode == SIZE32) {
			if (mode == 0)
				(void) strlcat(opnd, dis_addr32_mode0[r_m],
				    OPLEN);
			else
				(void) strlcat(opnd, dis_addr32_mode12[r_m],
				    OPLEN);
		} else {
			if (mode == 0) {
				(void) strlcat(opnd, dis_addr64_mode0[r_m],
				    OPLEN);
				if (r_m == 5) {
					x->d86_opnd[opindex].d86_mode =
					    MODE_RIPREL;
				}
			} else {
				(void) strlcat(opnd, dis_addr64_mode12[r_m],
				    OPLEN);
			}
		}
	} else {
		uint_t need_paren = 0;
		char **regs;
		char **bregs;
		const char *const *sf;
		if (x->d86_mode == SIZE32)                                  
			regs = (char **)dis_REG32;
		else
			regs = (char **)dis_REG64;

		if (x->d86_vsib != 0) {
			if (wbit == YMM_OPND)                                  
				bregs = (char **)dis_YMMREG;
			else
				bregs = (char **)dis_XMMREG;
			sf = dis_vscale_factor;
		} else {
			bregs = regs;
			sf = dis_scale_factor;
		}

		  
                            
     
		if (base == EBP_REGNO && mode == 0) {
			if (index != ESP_REGNO || x->d86_vsib != 0) {
				(void) strlcat(opnd, "(", OPLEN);
				need_paren = 1;
			}
		} else {
			(void) strlcat(opnd, "(", OPLEN);
			(void) strlcat(opnd, regs[base], OPLEN);
			need_paren = 1;
		}

		  
                             
     
		if (index != ESP_REGNO || x->d86_vsib) {
			(void) strlcat(opnd, ",", OPLEN);
			(void) strlcat(opnd, bregs[index], OPLEN);
			(void) strlcat(opnd, sf[ss], OPLEN);
		} else
			if (need_paren)
				(void) strlcat(opnd, ")", OPLEN);
	}
#endif
}

  
                                                                   
                                   
                                                     
                                                                             
   
#define	STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, vbit)  {	\
		dtrace_get_modrm(x, &mode, &reg, &r_m);			\
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);	\
		dtrace_get_operand(x, mode, r_m, wbit, vbit);		\
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 1 - vbit);	\
}

  
                                                                       
                      
                              
                            
   
#define	MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, w2, vbit)	{	\
		dtrace_get_modrm(x, &mode, &reg, &r_m);			\
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);	\
		dtrace_get_operand(x, mode, r_m, wbit, vbit);		\
		dtrace_get_operand(x, REG_ONLY, reg, w2, 1 - vbit);	\
}

  
                                                 
                           
                                 
                             
   
#define	THREEOPERAND(x, mode, reg, r_m, rex_prefix, wbit, w2, immsize, vbit) { \
		dtrace_get_modrm(x, &mode, &reg, &r_m);			\
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);	\
		dtrace_get_operand(x, mode, r_m, wbit, 2-vbit);		\
		dtrace_get_operand(x, REG_ONLY, reg, w2, 1+vbit);	\
		dtrace_imm_opnd(x, wbit, immsize, 0);			\
}

  
                                                   
   
#define	FOUROPERAND(x, mode, reg, r_m, rex_prefix, wbit, w2, immsize) { \
		dtrace_get_modrm(x, &mode, &reg, &r_m);			\
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);	\
		dtrace_get_operand(x, mode, r_m, wbit, 2);		\
		dtrace_get_operand(x, REG_ONLY, reg, w2, 3);		\
		dtrace_imm_opnd(x, wbit, immsize, 1);			\
		dtrace_imm_opnd(x, wbit, immsize, 0);			\
}

  
                                  
   
#define	ONEOPERAND_TWOIMM(x, mode, reg, r_m, rex_prefix, wbit, immsize) { \
		dtrace_get_modrm(x, &mode, &reg, &r_m);			\
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);	\
		dtrace_get_operand(x, mode, r_m, wbit, 2);		\
		dtrace_imm_opnd(x, wbit, immsize, 1);			\
		dtrace_imm_opnd(x, wbit, immsize, 0);			\
}

  
                                                  
  
                                                                        
                                 
  
                                  
   
int
dtrace_disx86(dis86_t *x, uint_t cpu_mode)
{
	instable_t *dp;		                             
#ifdef DIS_TEXT
	uint_t i;
#endif
#ifdef DIS_MEM
	uint_t nomem = 0;
#define	NOMEM	(nomem = 1)
#else
#define	NOMEM	             
#endif
	uint_t opnd_size;	                              
	uint_t addr_size;	                              
	uint_t wbit = 0;	                                               
	uint_t w2;		                                   
	uint_t vbit;
	uint_t mode = 0;	                                
	uint_t reg = 0;		                               
	uint_t r_m = 0;		                               

	uint_t opcode1 = 0;		                             
	uint_t opcode2 = 0;		                            
	uint_t opcode3 = 0;		                                               
	uint_t opcode4 = 0;		                             
	uint_t opcode5 = 0;		                            
	uint_t opcode6 = 0;		                             
	uint_t opcode7 = 0;		                            
	uint_t opcode_bytes = 1;

	  
                                                                       
    
	uint_t	opnd_size_prefix = 0;
	uint_t	addr_size_prefix = 0;
	uint_t	segment_prefix = 0;
	uint_t	lock_prefix = 0;
	uint_t	rep_prefix = 0;
	uint_t	rex_prefix = 0;	                                     

	  
                                                    
    

	                                                          
	uint_t vex_prefix = 0;

	  
                                                      
                        
    
	uint_t vex_byte1 = 0;

	  
                                                        
                                       
    
	uint_t vex_prefetch = 0;

	uint_t vex_m = 0;
	uint_t vex_v = 0;
	uint_t vex_p = 0;
	uint_t vex_R = 1;
	uint_t vex_X = 1;
	uint_t vex_B = 1;
	uint_t vex_W = 0;
	uint_t vex_L = 0;
	dis_gather_regs_t *vreg;

#ifdef	DIS_TEXT
	                                                      
	char *blsinstr;
#endif

	size_t	off;

	instable_t dp_mmx;

	x->d86_len = 0;
	x->d86_rmindex = -1;
	x->d86_error = 0;
#ifdef DIS_TEXT
	x->d86_numopnds = 0;
	x->d86_seg_prefix = NULL;
	x->d86_mnem[0] = 0;
	for (i = 0; i < 4; ++i) {
		x->d86_opnd[i].d86_opnd[0] = 0;
		x->d86_opnd[i].d86_prefix[0] = 0;
		x->d86_opnd[i].d86_value_size = 0;
		x->d86_opnd[i].d86_value = 0;
		x->d86_opnd[i].d86_mode = MODE_NONE;
	}
#endif
	x->d86_rex_prefix = 0;
	x->d86_got_modrm = 0;
	x->d86_memsize = 0;
	x->d86_vsib = 0;

	if (cpu_mode == SIZE16) {
		opnd_size = SIZE16;
		addr_size = SIZE16;
	} else if (cpu_mode == SIZE32) {
		opnd_size = SIZE32;
		addr_size = SIZE32;
	} else {
		opnd_size = SIZE32;
		addr_size = SIZE64;
	}

	  
                                                               
                
    
	if (dtrace_get_opcode(x, &opcode1, &opcode2) != 0)
		goto error;

	if (opcode1 == 0 && opcode2 == 0 &&
	    x->d86_check_func != NULL && x->d86_check_func(x->d86_data)) {
#ifdef DIS_TEXT
		(void) strncpy(x->d86_mnem, ".byte\t0", OPLEN);
#endif
		goto done;
	}

	  
                                      
    
	for (;;) {
		uint_t *which_prefix = NULL;

		dp = (instable_t *)&dis_distable[opcode1][opcode2];

		switch (dp->it_adrmode) {
		case PREFIX:
			which_prefix = &rep_prefix;
			break;
		case LOCK:
			which_prefix = &lock_prefix;
			break;
		case OVERRIDE:
			which_prefix = &segment_prefix;
#ifdef DIS_TEXT
			x->d86_seg_prefix = (char *)dp->it_name;
#endif
			if (dp->it_invalid64 && cpu_mode == SIZE64)
				goto error;
			break;
		case AM:
			which_prefix = &addr_size_prefix;
			break;
		case DM:
			which_prefix = &opnd_size_prefix;
			break;
		}
		if (which_prefix == NULL)
			break;
		*which_prefix = (opcode1 << 4) | opcode2;
		if (dtrace_get_opcode(x, &opcode1, &opcode2) != 0)
			goto error;
	}

	  
                                    
                                                                       
                                                  
    
	if (cpu_mode == SIZE64) {
		if (segment_prefix != 0x64 && segment_prefix != 0x65)
			segment_prefix = 0;

		if (opcode1 == 0x4) {
			rex_prefix = (opcode1 << 4) | opcode2;
			if (dtrace_get_opcode(x, &opcode1, &opcode2) != 0)
				goto error;
			dp = (instable_t *)&dis_distable[opcode1][opcode2];
		} else if (opcode1 == 0xC &&
		    (opcode2 == 0x4 || opcode2 == 0x5)) {
			                      
			vex_prefix = (opcode1 << 4) | opcode2;
			x->d86_rex_prefix = 0x40;
		}
	} else if (opcode1 == 0xC && (opcode2 == 0x4 || opcode2 == 0x5)) {
		                     
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		vex_prefetch = 1;

		if (mode == REG_ONLY) {
			         
			vex_prefix = (opcode1 << 4) | opcode2;
			x->d86_rex_prefix = 0x40;
			opcode3 = (((mode << 3) | reg)>>1) & 0x0F;
			opcode4 = ((reg << 3) | r_m) & 0x0F;
		}
	}

	if (vex_prefix == VEX_2bytes) {
		if (!vex_prefetch) {
			if (dtrace_get_opcode(x, &opcode3, &opcode4) != 0)
				goto error;
		}
		vex_R = ((opcode3 & VEX_R) & 0x0F) >> 3;
		vex_L = ((opcode4 & VEX_L) & 0x0F) >> 2;
		vex_v = (((opcode3 << 4) | opcode4) & VEX_v) >> 3;
		vex_p = opcode4 & VEX_p;
		  
                                                          
                                                
     
		vex_byte1 = (opcode3 & VEX_R) | VEX_X | VEX_B;

		if (vex_R == 0)
			x->d86_rex_prefix |= REX_R;

		if (dtrace_get_opcode(x, &opcode1, &opcode2) != 0)
			goto error;

		switch (vex_p) {
			case VEX_p_66:
				dp = (instable_t *)
				    &dis_opAVX660F[(opcode1 << 4) | opcode2];
				break;
			case VEX_p_F3:
				dp = (instable_t *)
				    &dis_opAVXF30F[(opcode1 << 4) | opcode2];
				break;
			case VEX_p_F2:
				dp = (instable_t *)
				    &dis_opAVXF20F [(opcode1 << 4) | opcode2];
				break;
			default:
				dp = (instable_t *)
				    &dis_opAVX0F[opcode1][opcode2];

		}

	} else if (vex_prefix == VEX_3bytes) {
		if (!vex_prefetch) {
			if (dtrace_get_opcode(x, &opcode3, &opcode4) != 0)
				goto error;
		}
		vex_R = (opcode3 & VEX_R) >> 3;
		vex_X = (opcode3 & VEX_X) >> 2;
		vex_B = (opcode3 & VEX_B) >> 1;
		vex_m = (((opcode3 << 4) | opcode4) & VEX_m);
		vex_byte1 = opcode3 & (VEX_R | VEX_X | VEX_B);

		if (vex_R == 0)
			x->d86_rex_prefix |= REX_R;
		if (vex_X == 0)
			x->d86_rex_prefix |= REX_X;
		if (vex_B == 0)
			x->d86_rex_prefix |= REX_B;

		if (dtrace_get_opcode(x, &opcode5, &opcode6) != 0)
			goto error;
		vex_W = (opcode5 & VEX_W) >> 3;
		vex_L = (opcode6 & VEX_L) >> 2;
		vex_v = (((opcode5 << 4) | opcode6) & VEX_v) >> 3;
		vex_p = opcode6 & VEX_p;

		if (vex_W)
			x->d86_rex_prefix |= REX_W;

		                                                              
		if ((vex_m != VEX_m_0F) && (vex_m != VEX_m_0F38) &&
		    (vex_m != VEX_m_0F3A))
			goto error;

		if (dtrace_get_opcode(x, &opcode1, &opcode2) != 0)
			goto error;

		switch (vex_p) {
			case VEX_p_66:
				if (vex_m == VEX_m_0F) {
					dp = (instable_t *)
					    &dis_opAVX660F
					    [(opcode1 << 4) | opcode2];
				} else if (vex_m == VEX_m_0F38) {
					dp = (instable_t *)
					    &dis_opAVX660F38
					    [(opcode1 << 4) | opcode2];
				} else if (vex_m == VEX_m_0F3A) {
					dp = (instable_t *)
					    &dis_opAVX660F3A
					    [(opcode1 << 4) | opcode2];
				} else {
					goto error;
				}
				break;
			case VEX_p_F3:
				if (vex_m == VEX_m_0F) {
					dp = (instable_t *)
					    &dis_opAVXF30F
					    [(opcode1 << 4) | opcode2];
				} else if (vex_m == VEX_m_0F38) {
					dp = (instable_t *)
					    &dis_opAVXF30F38
					    [(opcode1 << 4) | opcode2];
				} else {
					goto error;
				}
				break;
			case VEX_p_F2:
				if (vex_m == VEX_m_0F) {
					dp = (instable_t *)
					    &dis_opAVXF20F
					    [(opcode1 << 4) | opcode2];
				} else if (vex_m == VEX_m_0F3A) {
					dp = (instable_t *)
					    &dis_opAVXF20F3A
					    [(opcode1 << 4) | opcode2];
				} else if (vex_m == VEX_m_0F38) {
					dp = (instable_t *)
					    &dis_opAVXF20F38
					    [(opcode1 << 4) | opcode2];
				} else {
					goto error;
				}
				break;
			default:
				dp = (instable_t *)
				    &dis_opAVX0F[opcode1][opcode2];

		}
	}
	if (vex_prefix) {
		if (dp->it_vexwoxmm) {
			wbit = LONG_OPND;
		} else {
			if (vex_L)
				wbit = YMM_OPND;
			else
				wbit = XMM_OPND;
		}
	}

	  
                                                        
                                                                   
            
    
	if (cpu_mode == SIZE64) {
		if ((rex_prefix & REX_W) || vex_W)
			opnd_size = SIZE64;
		else if (opnd_size_prefix)
			opnd_size = SIZE16;

		if (addr_size_prefix)
			addr_size = SIZE32;
	} else if (cpu_mode == SIZE32) {
		if (opnd_size_prefix)
			opnd_size = SIZE16;
		if (addr_size_prefix)
			addr_size = SIZE16;
	} else {
		if (opnd_size_prefix)
			opnd_size = SIZE32;
		if (addr_size_prefix)
			addr_size = SIZE32;
	}
	  
                                                           
                                                             
                         
    
	if (rep_prefix == 0xf3 && opcode1 == 0x9 && opcode2 == 0x0) {
		rep_prefix = 0;
		dp = (instable_t *)&dis_opPause;
	}

	  
                                                                   
                                                       
    
	if (dp->it_indirect == (instable_t *)dis_op0F) {
		if (dtrace_get_opcode(x, &opcode4, &opcode5) != 0)
			goto error;
		opcode_bytes = 2;
		if (opcode4 == 0x7 && opcode5 >= 0x1 && opcode5 <= 0x3) {
			uint_t	subcode;

			if (dtrace_get_opcode(x, &opcode6, &opcode7) != 0)
				goto error;
			opcode_bytes = 3;
			subcode = ((opcode6 & 0x3) << 1) |
			    ((opcode7 & 0x8) >> 3);
			dp = (instable_t *)&dis_op0F7123[opcode5][subcode];
		} else if ((opcode4 == 0xc) && (opcode5 >= 0x8)) {
			dp = (instable_t *)&dis_op0FC8[0];
		} else if ((opcode4 == 0x3) && (opcode5 == 0xA)) {
			opcode_bytes = 3;
			if (dtrace_get_opcode(x, &opcode6, &opcode7) != 0)
				goto error;
			if (opnd_size == SIZE16)
				opnd_size = SIZE32;

			dp = (instable_t *)&dis_op0F3A[(opcode6<<4)|opcode7];
#ifdef DIS_TEXT
			if (LIT_STRNEQL(dp->it_name, "INVALID"))
				goto error;
#endif
			switch (dp->it_adrmode) {
				case XMMP:
					break;
				case XMMP_66r:
				case XMMPRM_66r:
				case XMM3PM_66r:
					if (opnd_size_prefix == 0) {
						goto error;
					}
					break;
				case XMMP_66o:
					if (opnd_size_prefix == 0) {
						                            
						dp_mmx = *dp;
						dp = &dp_mmx;
						dp->it_adrmode = MMOPM_66o;
#ifdef	DIS_MEM
						dp->it_size = 8;
#endif
					}
					break;
				default:
					goto error;
			}
		} else if ((opcode4 == 0x3) && (opcode5 == 0x8)) {
			opcode_bytes = 3;
			if (dtrace_get_opcode(x, &opcode6, &opcode7) != 0)
				goto error;
			dp = (instable_t *)&dis_op0F38[(opcode6<<4)|opcode7];

			  
                                                   
                                                    
                                                 
      
			if (dp->it_indirect == (instable_t *)dis_op0F38F0 ||
			    dp->it_indirect == (instable_t *)dis_op0F38F1) {

				dp = dp->it_indirect;
				if (rep_prefix != 0xF2) {
					                 
					dp++;
				}
			}

			  
                                                    
                                                     
                                                     
                                                  
                                                         
                                                        
                                                       
                                                       
     
                                                       
                                                           
                                                    
                                                          
                                                        
                                                          
      
			if (dp->it_indirect == (instable_t *)dis_op0F38F6) {
				dp = dp->it_indirect;
				if (opnd_size_prefix == 0 &&
				    rep_prefix == 0xf3) {
					                
					dp++;
				} else if (opnd_size_prefix != 0x66 &&
				    rep_prefix != 0) {
					                   
					goto error;
				}
				opnd_size_prefix = 0;
				rep_prefix = 0;
				opnd_size = SIZE32;
				if (rex_prefix & REX_W)
					opnd_size = SIZE64;
			}

#ifdef DIS_TEXT
			if (LIT_STRNEQL(dp->it_name, "INVALID"))
				goto error;
#endif
			switch (dp->it_adrmode) {
				case ADX:
				case XMM:
					break;
				case RM_66r:
				case XMM_66r:
				case XMMM_66r:
					if (opnd_size_prefix == 0) {
						goto error;
					}
					break;
				case XMM_66o:
					if (opnd_size_prefix == 0) {
						                            
						dp_mmx = *dp;
						dp = &dp_mmx;
						dp->it_adrmode = MM;
#ifdef	DIS_MEM
						dp->it_size = 8;
#endif
					}
					break;
				case CRC32:
					if (rep_prefix != 0xF2) {
						goto error;
					}
					rep_prefix = 0;
					break;
				case MOVBE:
					if (rep_prefix != 0x0) {
						goto error;
					}
					break;
				default:
					goto error;
			}
		} else {
			dp = (instable_t *)&dis_op0F[opcode4][opcode5];
		}
	}

	  
                                                          
                                                         
    
	x->d86_got_modrm = 0;
	if (dp->it_indirect != TERM) {
		dtrace_get_modrm(x, &mode, &opcode3, &r_m);
		if (x->d86_error)
			goto error;
		reg = opcode3;

		  
                                                 
     
		if (opcode1 == 0xD && opcode2 >= 0x8) {
			if (opcode2 == 0xB && mode == 0x3 && opcode3 == 4)
				dp = (instable_t *)&dis_opFP5[r_m];
			else if (opcode2 == 0xA && mode == 0x3 && opcode3 < 4)
				dp = (instable_t *)&dis_opFP7[opcode3];
			else if (opcode2 == 0xB && mode == 0x3)
				dp = (instable_t *)&dis_opFP6[opcode3];
			else if (opcode2 == 0x9 && mode == 0x3 && opcode3 >= 4)
				dp = (instable_t *)&dis_opFP4[opcode3 - 4][r_m];
			else if (mode == 0x3)
				dp = (instable_t *)
				    &dis_opFP3[opcode2 - 8][opcode3];
			else
				dp = (instable_t *)
				    &dis_opFP1n2[opcode2 - 8][opcode3];
		} else {
			dp = (instable_t *)dp->it_indirect + opcode3;
		}
	}

	  
                                                       
                                 
    
	if ((vex_prefix == 0) && cpu_mode == SIZE64 &&
	    opcode1 == 0x6 && opcode2 == 0x3)
	{
		dp = (instable_t *)&dis_opMOVSLD;
	}

	  
                                                              
    
	if ((cpu_mode == SIZE64 && dp->it_invalid64) ||
	    (cpu_mode != SIZE64 && dp->it_invalid32))
		goto error;
	if (dp->it_indirect != TERM)
		goto error;

	  
                                                                        
                                                                   
                                                              
    
	switch (dp->it_adrmode) {
	case UNKNOWN:
	case MMO:
	case MMOIMPL:
	case MMO3P:
	case MMOM3:
	case MMOMS:
	case MMOPM:
	case MMOPRM:
	case MMOS:
	case XMMO:
	case XMMOM:
	case XMMOMS:
	case XMMOPM:
	case XMMOS:
	case XMMOMX:
	case XMMOX3:
	case XMMOXMM:
		  
                                                       
                                                          
                                                          
                                                         
                                                          
                                                      
    
                                                   
                                                         
                                                          
                                                         
                                                       
           
     

		  
                                     
     
		if ((uintptr_t)dp - (uintptr_t)dis_op0F > sizeof (dis_op0F))
			goto error;

		off = ((uintptr_t)dp - (uintptr_t)dis_op0F) /
		    sizeof (instable_t);

		  
                                                                
     
		if (rep_prefix) {
			if (rep_prefix == 0xf2)
				dp = (instable_t *)&dis_opSIMDrepnz[off];
			else
				dp = (instable_t *)&dis_opSIMDrepz[off];
			rep_prefix = 0;
		} else if (opnd_size_prefix) {
			dp = (instable_t *)&dis_opSIMDdata16[off];
			opnd_size_prefix = 0;
			if (opnd_size == SIZE16)
				opnd_size = SIZE32;
		}
		break;

	case MG9:
		  
                                                                
                                                                 
                                                               
                                                                
                               
    
                                                                  
                                                                 
                                                                
                                                               
                                                                  
                                                                 
                                     
     

		  
                                                           
     
		if ((uintptr_t)dp - (uintptr_t)dis_op0FC7 > sizeof (dis_op0FC7))
			goto error;

		off = ((uintptr_t)dp - (uintptr_t)dis_op0FC7) /
		    sizeof (instable_t);

		  
                                                            
     
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode == REG_ONLY) {
			dp = (instable_t *)&dis_op0FC7m3[off];
			break;
		}

		  
                                                                
     
		if (rep_prefix) {
			if (rep_prefix == 0xf3)
				dp = (instable_t *)&dis_opF30FC7[off];
			else
				goto error;
			rep_prefix = 0;
		} else if (opnd_size_prefix) {
			dp = (instable_t *)&dis_op660FC7[off];
			opnd_size_prefix = 0;
			if (opnd_size == SIZE16)
				opnd_size = SIZE32;
		}
		break;


	case MMOSH:
		  
                                                    
                                                
                                                        
                                                        
                                                 
     

		  
                                         
     
		if ((uintptr_t)dp - (uintptr_t)dis_op0F7123 >
		    sizeof (dis_op0F7123))
			goto error;

		if (opnd_size_prefix) {
			off = ((uintptr_t)dp - (uintptr_t)dis_op0F7123) /
			    sizeof (instable_t);
			dp = (instable_t *)&dis_opSIMD7123[off];
			opnd_size_prefix = 0;
			if (opnd_size == SIZE16)
				opnd_size = SIZE32;
		}
		break;
	case MRw:
		if (rep_prefix) {
			if (rep_prefix == 0xf3) {

				  
                                       
       
				if ((uintptr_t)dp - (uintptr_t)dis_op0F
				    > sizeof (dis_op0F))
					goto error;

				off = ((uintptr_t)dp - (uintptr_t)dis_op0F) /
				    sizeof (instable_t);

				dp = (instable_t *)&dis_opSIMDrepz[off];
				rep_prefix = 0;
			} else {
				goto error;
			}
		}
		break;
	}

	  
                                                                       
    
	if (cpu_mode == SIZE64)
		if (dp->it_always64 || (opnd_size == SIZE32 && dp->it_stackop))
			opnd_size = SIZE64;

#ifdef DIS_TEXT
	  
                                                                  
                           
    
	if (lock_prefix)
		(void) strlcat(x->d86_mnem, "lock ", OPLEN);

	if (rep_prefix == 0xf2)
		(void) strlcat(x->d86_mnem, "repnz ", OPLEN);
	else if (rep_prefix == 0xf3)
		(void) strlcat(x->d86_mnem, "repz ", OPLEN);

	if (cpu_mode == SIZE64 && addr_size_prefix)
		(void) strlcat(x->d86_mnem, "addr32 ", OPLEN);

	if (dp->it_adrmode != CBW &&
	    dp->it_adrmode != CWD &&
	    dp->it_adrmode != XMMSFNC) {
		if (LIT_STRNEQL(dp->it_name, "INVALID"))
			goto error;
		(void) strlcat(x->d86_mnem, dp->it_name, OPLEN);
		if (dp->it_avxsuf && dp->it_suffix) {
			(void) strlcat(x->d86_mnem, vex_W != 0 ? "q" : "d",
			    OPLEN);
		} else if (dp->it_suffix) {
			char *types[] = {"", "w", "l", "q"};
			if (opcode_bytes == 2 && opcode4 == 4) {
				                                           
				for (i = 5; i < OPLEN; i++) {
					if (x->d86_mnem[i] == '.')
						break;
				}
				x->d86_mnem[i - 1] = *types[opnd_size];
			} else if ((opnd_size == 2) && (opcode_bytes == 3) &&
			    ((opcode6 == 1 && opcode7 == 6) ||
			    (opcode6 == 2 && opcode7 == 2))) {
				  
                                  
       
				(void) strlcat(x->d86_mnem, "d", OPLEN);
			} else {
				(void) strlcat(x->d86_mnem, types[opnd_size],
				    OPLEN);
			}
		}
	}
#endif

	  
                                                   
    
	x->d86_mode = cpu_mode;
	  
                                             
    
	if (!vex_prefix)
		x->d86_rex_prefix = rex_prefix;
	x->d86_opnd_size = opnd_size;
	x->d86_addr_size = addr_size;
	vbit = 0;		                                   
	switch (dp->it_adrmode) {
		  
                                                             
                                
     
	case MOVSXZ:
#ifdef DIS_TEXT
		if (rex_prefix == 0) {
			(void) strncpy(x->d86_mnem, "movzld", OPLEN);
			x->d86_mnem[OPLEN - 1] = '\0';
		}
#endif
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		x->d86_opnd_size = SIZE64;
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
		x->d86_opnd_size = opnd_size = SIZE32;
		wbit = LONG_OPND;
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;

		  
                                                            
                                                            
                                               
                        
     
	case MOVZ:
		if (rex_prefix & REX_W) {
			                                   
			x->d86_mnem[5] = 'q';
		}
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
		x->d86_opnd_size = opnd_size = SIZE16;
		wbit = WBIT(opcode5);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;
	case CRC32:
		opnd_size = SIZE32;
		if (rex_prefix & REX_W)
			opnd_size = SIZE64;
		x->d86_opnd_size = opnd_size;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
		wbit = WBIT(opcode7);
		if (opnd_size_prefix)
			x->d86_opnd_size = opnd_size = SIZE16;
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;
	case MOVBE:
		opnd_size = SIZE32;
		if (rex_prefix & REX_W)
			opnd_size = SIZE64;
		x->d86_opnd_size = opnd_size;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		wbit = WBIT(opcode7);
		if (opnd_size_prefix)
			x->d86_opnd_size = opnd_size = SIZE16;
		if (wbit) {
			                
			dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 0);
			dtrace_get_operand(x, mode, r_m, wbit, 1);
		} else {
			                
			dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
			dtrace_get_operand(x, mode, r_m, wbit, 0);
		}
		break;

	  
                                                           
                                                                      
    
	case IMUL:
		wbit = LONG_OPND;
		THREEOPERAND(x, mode, reg, r_m, rex_prefix, wbit, LONG_OPND,
		    OPSIZE(opnd_size, opcode2 == 0x9), 1);
		break;

	                                                          
	case MRw:
	case ADX:
		wbit = WBIT(opcode2);
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 0);
		break;

	                                                          
	                                                     
	case RMw:
		if (opcode_bytes == 2)
			wbit = WBIT(opcode5);
		else
			wbit = WBIT(opcode2);
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 1);
		break;

	                       
	case XADDB:
		wbit = 0;
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 1);
		break;

	                                                 
	case MMS:
	case MMOS:
#ifdef DIS_TEXT
		wbit = !LIT_STRNEQL(dp->it_name, "movd") ? MM_OPND : LONG_OPND;
#else
		wbit = LONG_OPND;
#endif
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, MM_OPND, 1);
		break;

	                            
	case MMOMS:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode == REG_ONLY)
			goto error;
		wbit = MM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, MM_OPND, 1);
		break;

	                                                               
	case DSHIFT:
		wbit = LONG_OPND;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 2);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	  
                                                                 
    
	case DSHIFTcl:
		wbit = LONG_OPND;
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 1);
		break;

	                                             
	case IMlw:
		wbit = WBIT(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 1);
		  
                                                               
     
		dtrace_imm_opnd(x, wbit, OPSIZE(opnd_size, opcode2 == 1), 0);
		break;

	                                                      
	                         
	case IMw:
		wbit = WBIT(opcode2);
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 1);
		dtrace_imm_opnd(x, wbit, OPSIZE(opnd_size, wbit), 0);
		break;

	                                                       
	                     
	case IR:
		                                     
		wbit = opcode2 >>3 & 0x1;
		reg = REGNO(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, &reg, NULL);
		mode = REG_ONLY;
		r_m = reg;
		dtrace_get_operand(x, mode, r_m, wbit, 1);
		dtrace_imm_opnd(x, wbit, OPSIZE64(opnd_size, wbit), 0);
		break;

	                                     
	case MMSH:
	case MMOSH:
		wbit = MM_OPND;
		goto mm_shift;	                  

	                                      
	case XMMSH:
		wbit = XMM_OPND;
mm_shift:
		reg = REGNO(opcode7);
		dtrace_rex_adjust(rex_prefix, mode, &reg, NULL);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
		dtrace_imm_opnd(x, wbit, 1, 0);
		NOMEM;
		break;

	                                   
	case AO:
		vbit = 1;
		OS_FALLTHROUGH;

	                                   
	case OA:
		wbit = WBIT(opcode2);
		dtrace_get_operand(x, REG_ONLY, EAX_REGNO, wbit, 1 - vbit);
		dtrace_imm_opnd(x, wbit, OPSIZE64(addr_size, LONG_OPND), vbit);
#ifdef DIS_TEXT
		x->d86_opnd[vbit].d86_mode = MODE_OFFSET;
#endif
		break;


	                                                    
	case SM:
		vbit = 1;
		OS_FALLTHROUGH;

	                                                    
	case MS:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, LONG_OPND, vbit);
		dtrace_get_operand(x, REG_ONLY, reg, SEG_OPND, 1 - vbit);
		break;

	  
                                                         
                                                     
    
	case Mv:
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 1);
#ifdef DIS_TEXT
		if (vbit) {
			(void) strlcat(x->d86_opnd[0].d86_opnd, "%cl", OPLEN);
		} else {
			x->d86_opnd[0].d86_mode = MODE_SIGNED;
			x->d86_opnd[0].d86_value_size = 1;
			x->d86_opnd[0].d86_value = 1;
		}
#endif
		break;
	  
                                          
    
	case MvI:
		wbit = WBIT(opcode2);
normal_imm_mem:
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 1);
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	                           
	case MIb:
		wbit = LONG_OPND;
		goto normal_imm_mem;

	                                                            
	case Mw:
		wbit = WBIT(opcode2);
just_mem:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;

	case SWAPGS_RDTSCP:
		if (cpu_mode == SIZE64 && mode == 3 && r_m == 0) {
#ifdef DIS_TEXT
			(void) strncpy(x->d86_mnem, "swapgs", OPLEN);
			x->d86_mnem[OPLEN - 1] = '\0';
#endif
			NOMEM;
			break;
		} else if (mode == 3 && r_m == 1) {
#ifdef DIS_TEXT
			(void) strncpy(x->d86_mnem, "rdtscp", OPLEN);
#endif
			NOMEM;
			break;
		}

		OS_FALLTHROUGH;

	                                                                
	case PREF:
		NOMEM;
		OS_FALLTHROUGH;

	                                       
	case M:
	case MG9:
		wbit = LONG_OPND;
		goto just_mem;

	                                            
	case Mb:
		wbit = BYTE_OPND;
		goto just_mem;

	case VMx:
		if (mode == 3) {
#ifdef DIS_TEXT
			char *vminstr;

			switch (r_m) {
			case 1:
				vminstr = "vmcall";
				break;
			case 2:
				vminstr = "vmlaunch";
				break;
			case 3:
				vminstr = "vmresume";
				break;
			case 4:
				vminstr = "vmxoff";
				break;
			default:
				goto error;
			}

			(void) strncpy(x->d86_mnem, vminstr, OPLEN);
#else
			if (r_m < 1 || r_m > 4)
				goto error;
#endif

			NOMEM;
			break;
		}
		OS_FALLTHROUGH;
	case SVM:
		if (mode == 3) {
#if DIS_TEXT
			char *vinstr;

			switch (r_m) {
			case 0:
				vinstr = "vmrun";
				break;
			case 1:
				vinstr = "vmmcall";
				break;
			case 2:
				vinstr = "vmload";
				break;
			case 3:
				vinstr = "vmsave";
				break;
			case 4:
				vinstr = "stgi";
				break;
			case 5:
				vinstr = "clgi";
				break;
			case 6:
				vinstr = "skinit";
				break;
			case 7:
				vinstr = "invlpga";
				break;
			}

			(void) strncpy(x->d86_mnem, vinstr, OPLEN);
#endif
			NOMEM;
			break;
		}
		OS_FALLTHROUGH;
	case MONITOR_MWAIT:
		if (mode == 3) {
			if (r_m == 0) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "monitor", OPLEN);
				x->d86_mnem[OPLEN - 1] = '\0';
#endif
				NOMEM;
				break;
			} else if (r_m == 1) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "mwait", OPLEN);
				x->d86_mnem[OPLEN - 1] = '\0';
#endif
				NOMEM;
				break;
			} else if (r_m == 2) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "clac", OPLEN);
#endif
				NOMEM;
				break;
			} else if (r_m == 3) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "stac", OPLEN);
#endif
				NOMEM;
				break;
			} else {
				goto error;
			}
		}
		OS_FALLTHROUGH;
	case XGETBV_XSETBV:
		if (mode == 3) {
			if (r_m == 0) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "xgetbv", OPLEN);
#endif
				NOMEM;
				break;
			} else if (r_m == 1) {
#ifdef DIS_TEXT
				(void) strncpy(x->d86_mnem, "xsetbv", OPLEN);
#endif
				NOMEM;
				break;
			} else {
				goto error;
			}

		}
		OS_FALLTHROUGH;
	case MO:
		                                                         
		wbit = LONG_OPND;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode == 3)
			goto error;
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;

	                                                          
	case SREG:
		switch (opcode5) {

		case 2:
			vbit = 1;
			OS_FALLTHROUGH;
		case 0:
			wbit = CONTROL_OPND;
			break;

		case 3:
			vbit = 1;
			OS_FALLTHROUGH;
		case 1:
			wbit = DEBUG_OPND;
			break;

		case 6:
			vbit = 1;
			OS_FALLTHROUGH;
		case 4:
			wbit = TEST_OPND;
			break;

		}
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, vbit);
		dtrace_get_operand(x, REG_ONLY, r_m, LONG_OPND, 1 - vbit);
		NOMEM;
		break;

	  
                                                      
                   
    
	case R:
		if (opcode_bytes == 2)
			reg = REGNO(opcode5);
		else
			reg = REGNO(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, &reg, NULL);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 0);
		NOMEM;
		break;

	  
                                                      
                                      
    
	case RA:
		NOMEM;
		reg = REGNO(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, &reg, NULL);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 0);
		dtrace_get_operand(x, REG_ONLY, EAX_REGNO, LONG_OPND, 1);
		break;

	  
                                                     
                            
    
	case SEG:
		NOMEM;
		reg = (x->d86_bytes[x->d86_len - 1] >> 3) & 0x3;
		dtrace_get_operand(x, REG_ONLY, reg, SEG_OPND, 0);
		break;

	  
                                                     
                       
    
	case LSEG:
		NOMEM;
		                              
		reg = (x->d86_bytes[x->d86_len - 1] >> 3) & 0x7;
		dtrace_get_operand(x, REG_ONLY, reg, SEG_OPND, 0);
		break;

	                                            
	case MR:
		if (vex_prefetch)
			x->d86_got_modrm = 1;
		wbit = LONG_OPND;
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 0);
		break;

	case RM:
	case RM_66r:
		wbit = LONG_OPND;
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 1);
		break;

	                                              
	case MM:
	case MMO:
#ifdef DIS_TEXT
		wbit = !LIT_STRNEQL(dp->it_name, "movd") ? MM_OPND : LONG_OPND;
#else
		wbit = LONG_OPND;
#endif
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, MM_OPND, 0);
		break;

	case MMOIMPL:
#ifdef DIS_TEXT
		wbit = !LIT_STRNEQL(dp->it_name, "movd") ? MM_OPND : LONG_OPND;
#else
		wbit = LONG_OPND;
#endif
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode != REG_ONLY)
			goto error;

		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		dtrace_get_operand(x, REG_ONLY, reg, MM_OPND, 1);
		mode = 0;	                                      
		break;

	                                                       
	case MMO3P:
		wbit = MM_OPND;
		goto xmm3p;
	case XMM3P:
		wbit = XMM_OPND;
xmm3p:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode != REG_ONLY)
			goto error;

		THREEOPERAND(x, mode, reg, r_m, rex_prefix, wbit, LONG_OPND, 1,
		    1);
		NOMEM;
		break;

	case XMM3PM_66r:
		THREEOPERAND(x, mode, reg, r_m, rex_prefix, LONG_OPND, XMM_OPND,
		    1, 0);
		break;

	                                               
	case MMOPRM:
		wbit = LONG_OPND;
		w2 = MM_OPND;
		goto xmmprm;
	case XMMPRM:
	case XMMPRM_66r:
		wbit = LONG_OPND;
		w2 = XMM_OPND;
xmmprm:
		THREEOPERAND(x, mode, reg, r_m, rex_prefix, wbit, w2, 1, 1);
		break;

	                                              
	case MMOPM:
	case MMOPM_66o:
		wbit = w2 = MM_OPND;
		goto xmmprm;

	                                
	case MMOM3:
		NOMEM;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode != REG_ONLY)
			goto error;
		wbit = MM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, LONG_OPND, 0);
		break;

	                                                
	case XMM:
	case XMM_66o:
	case XMM_66r:
	case XMMO:
	case XMMXIMPL:
		wbit = XMM_OPND;
		STANDARD_MODRM(x, mode, reg, r_m, rex_prefix, wbit, 0);

		if (dp->it_adrmode == XMMXIMPL && mode != REG_ONLY)
			goto error;

#ifdef DIS_TEXT
		  
                                                          
                                                 
                                         
     
		if (mode == REG_ONLY) {
			if (LIT_STRNEQL(dp->it_name, "movlps")) {
				(void) strncpy(x->d86_mnem, "movhlps", OPLEN);
				x->d86_mnem[OPLEN - 1] = '\0';
			} else if (strcmp(dp->it_name, "movhps") == 0) {
				(void) strncpy(x->d86_mnem, "movlhps", OPLEN);
				x->d86_mnem[OPLEN - 1] = '\0';
			}
		}
#endif
		if (dp->it_adrmode == XMMXIMPL)
			mode = 0;	                                      
		break;

	                                       
	case XMMS:
	case XMMOS:
	case XMMMS:
	case XMMOMS:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
#ifdef DIS_TEXT
		if ((LIT_STRNEQL(dp->it_name, "movlps") ||
		     LIT_STRNEQL(dp->it_name, "movhps") ||
		     LIT_STRNEQL(dp->it_name, "movntps")) &&
		    mode == REG_ONLY)
			goto error;
#endif
		wbit = XMM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 1);
		break;

	                            
	case XMMM:
	case XMMM_66r:
	case XMMOM:
		wbit = XMM_OPND;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
#ifdef DIS_TEXT
		if (mode == REG_ONLY) {
			if (LIT_STRNEQL(dp->it_name, "movhps")) {
				(void) strncpy(x->d86_mnem, "movlhps", OPLEN);
				x->d86_mnem[OPLEN - 1] = '\0';
			} else
				goto error;
		}
#endif
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 0);
		break;

	                                     
	case XMM3MX:
		wbit = LONG_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 0);
		break;

	case XMM3MXS:
		wbit = LONG_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 1);
		break;

	                                        
	case XMMOMX:
	                    
	case XMMMX:
		wbit = MM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 0);
		break;

	                                        
	case XMMXMM:
	case XMMOXMM:
	case XMMXM:
		wbit = XMM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, MM_OPND, 0);
		break;


	                                     
	case XMMXM3:
		wbit = XMM_OPND;
		MIXED_MM(x, mode, reg, r_m, rex_prefix, wbit, LONG_OPND, 0);
		break;

	                         
	case XMMX3:
	case XMMOX3:
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		if (mode != REG_ONLY)
			goto error;
		dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, XMM_OPND, 0);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, 1);
		NOMEM;
		break;

	                                                       
	case XMMP:
	case XMMP_66r:
	case XMMP_66o:
	case XMMOPM:
		wbit = XMM_OPND;
		THREEOPERAND(x, mode, reg, r_m, rex_prefix, wbit, XMM_OPND, 1,
		    1);

#ifdef DIS_TEXT
		  
                                                      
                                                    
                                                      
                   
     
		if (dp->it_name[0] == 'c' &&
		    dp->it_name[1] == 'm' &&
		    dp->it_name[2] == 'p' &&
		    strlen(dp->it_name) == 5) {
			uchar_t pred = x->d86_opnd[0].d86_value & 0xff;

			if (pred >= (sizeof (dis_PREDSUFFIX) / sizeof (char *)))
				goto error;

			(void) strncpy(x->d86_mnem, "cmp", OPLEN);
			x->d86_mnem[OPLEN - 1] = '\0';
			(void) strlcat(x->d86_mnem, dis_PREDSUFFIX[pred],
			    OPLEN);
			(void) strlcat(x->d86_mnem,
			    dp->it_name + strlen(dp->it_name) - 2,
			    OPLEN);
			x->d86_opnd[0] = x->d86_opnd[1];
			x->d86_opnd[1] = x->d86_opnd[2];
			x->d86_numopnds = 2;
		}
#endif
		break;

	case XMMX2I:
		FOUROPERAND(x, mode, reg, r_m, rex_prefix, XMM_OPND, XMM_OPND,
		    1);
		NOMEM;
		break;

	case XMM2I:
		ONEOPERAND_TWOIMM(x, mode, reg, r_m, rex_prefix, XMM_OPND, 1);
		NOMEM;
		break;

	                                      
	case IA:
		wbit = WBIT(opcode2);
		dtrace_get_operand(x, REG_ONLY, EAX_REGNO, wbit, 1);
		dtrace_imm_opnd(x, wbit, OPSIZE(opnd_size, wbit), 0);
		NOMEM;
		break;

	                                               
	case MA:
		wbit = WBIT(opcode2);
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;

	                                                          
	case SD:
#ifdef DIS_TEXT
		dtrace_check_override(x, 0);
		x->d86_numopnds = 2;
		if (addr_size == SIZE64) {
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%rsi)",
			    OPLEN);
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%rdi)",
			    OPLEN);
		} else if (addr_size == SIZE32) {
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%esi)",
			    OPLEN);
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%edi)",
			    OPLEN);
		} else {
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%si)",
			    OPLEN);
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%di)",
			    OPLEN);
		}
#endif
		wbit = LONG_OPND;
		break;

	                                   
	case AD:
		wbit = WBIT(opcode2);
#ifdef DIS_TEXT
		dtrace_check_override(x, 1);
		x->d86_numopnds = 2;
		dtrace_get_operand(x, REG_ONLY, EAX_REGNO, wbit, 0);
		if (addr_size == SIZE64)
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%rdi)",
			    OPLEN);
		else if (addr_size == SIZE32)
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%edi)",
			    OPLEN);
		else
			(void) strlcat(x->d86_opnd[1].d86_opnd, "(%di)",
			    OPLEN);
#endif
		break;

	                                   
	case SA:
		wbit = WBIT(opcode2);
#ifdef DIS_TEXT
		dtrace_check_override(x, 0);
		x->d86_numopnds = 2;
		if (addr_size == SIZE64)
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%rsi)",
			    OPLEN);
		else if (addr_size == SIZE32)
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%esi)",
			    OPLEN);
		else
			(void) strlcat(x->d86_opnd[0].d86_opnd, "(%si)",
			    OPLEN);
		dtrace_get_operand(x, REG_ONLY, EAX_REGNO, wbit, 1);
#endif
		break;

	  
                                            
    
	case D:
		wbit = LONG_OPND;
		dtrace_disp_opnd(x, wbit, OPSIZE(opnd_size, LONG_OPND), 0);
		NOMEM;
		break;

	                                                      
	case INM:
#ifdef DIS_TEXT
		(void) strlcat(x->d86_opnd[0].d86_prefix, "*", OPLEN);
#endif
		dtrace_rex_adjust(rex_prefix, mode, NULL, &r_m);
		dtrace_get_operand(x, mode, r_m, LONG_OPND, 0);
		wbit = LONG_OPND;
		break;

	  
                                                       
                                                    
                                                    
    
	case SO:
		dtrace_check_override(x, 1);
		wbit = LONG_OPND;
		dtrace_imm_opnd(x, wbit, OPSIZE(opnd_size, LONG_OPND), 1);
#ifdef DIS_TEXT
		x->d86_opnd[1].d86_mode = MODE_SIGNED;
#endif
		                                  
		dtrace_imm_opnd(x, wbit, 2, 0);
		break;

	  
                                                 
                                      
    
	case BD:
		dtrace_disp_opnd(x, BYTE_OPND, 1, 0);
		NOMEM;
		break;

	                                          
	case I:
		wbit = LONG_OPND;
		dtrace_imm_opnd(x, wbit, OPSIZE(opnd_size, LONG_OPND), 0);
		break;

	                                      
	case Ib:
		wbit = LONG_OPND;
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	case ENTER:
		wbit = LONG_OPND;
		dtrace_imm_opnd(x, wbit, 2, 0);
		dtrace_imm_opnd(x, wbit, 1, 1);
		switch (opnd_size) {
		case SIZE64:
			x->d86_memsize = (x->d86_opnd[1].d86_value + 1) * 8;
			break;
		case SIZE32:
			x->d86_memsize = (x->d86_opnd[1].d86_value + 1) * 4;
			break;
		case SIZE16:
			x->d86_memsize = (x->d86_opnd[1].d86_value + 1) * 2;
			break;
		}

		break;

	                              
	case RET:
		wbit = LONG_OPND;
		dtrace_imm_opnd(x, wbit, 2, 0);
		break;

	                                  
	case P:
		dtrace_check_override(x, 0);
		dtrace_imm_opnd(x, BYTE_OPND, 1, 0);
		NOMEM;
		break;

	                                                             
	case V:
		x->d86_numopnds = 1;
		dtrace_check_override(x, 0);
#ifdef DIS_TEXT
		(void) strlcat(x->d86_opnd[0].d86_opnd, "(%dx)", OPLEN);
#endif
		NOMEM;
		break;

	  
                                             
                         
                                                 
                                                           
                                                 
                                                     
                                   
    
	case INT3:
#ifdef DIS_TEXT
		x->d86_numopnds = 1;
		x->d86_opnd[0].d86_mode = MODE_SIGNED;
		x->d86_opnd[0].d86_value_size = 1;
		x->d86_opnd[0].d86_value = 3;
#endif
		NOMEM;
		break;

	                                      
	case INTx:
		dtrace_imm_opnd(x, BYTE_OPND, 1, 0);
		NOMEM;
		break;

	                                      
	case U:
		if (x->d86_get_byte(x->d86_data) < 0)
			goto error;
		x->d86_len++;
		NOMEM;
		break;

	case CBW:
#ifdef DIS_TEXT
		if (opnd_size == SIZE16)
			(void) strlcat(x->d86_mnem, "cbtw", OPLEN);
		else if (opnd_size == SIZE32)
			(void) strlcat(x->d86_mnem, "cwtl", OPLEN);
		else
			(void) strlcat(x->d86_mnem, "cltq", OPLEN);
#endif
		wbit = LONG_OPND;
		NOMEM;
		break;

	case CWD:
#ifdef DIS_TEXT
		if (opnd_size == SIZE16)
			(void) strlcat(x->d86_mnem, "cwtd", OPLEN);
		else if (opnd_size == SIZE32)
			(void) strlcat(x->d86_mnem, "cltd", OPLEN);
		else
			(void) strlcat(x->d86_mnem, "cqtd", OPLEN);
#endif
		wbit = LONG_OPND;
		NOMEM;
		break;

	case XMMSFNC:
		  
                                                         
                                            
     
		dtrace_get_modrm(x, &mode, &reg, &r_m);

		                                  
#ifdef DIS_TEXT
		if (mode == REG_ONLY) {
			(void) strlcat(x->d86_mnem, "sfence", OPLEN);
		} else {
			(void) strlcat(x->d86_mnem, "clflush", OPLEN);
			dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
			dtrace_get_operand(x, mode, r_m, BYTE_OPND, 0);
			NOMEM;
		}
#else
		if (mode != REG_ONLY) {
			dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
			dtrace_get_operand(x, mode, r_m, LONG_OPND, 0);
			NOMEM;
		}
#endif
		break;

	  
                                                           
    
	case NORM:
		if (dp->it_invalid32 && cpu_mode != SIZE64)
			goto error;
		NOMEM;
		OS_FALLTHROUGH;
	case IMPLMEM:
		break;

	case XMMFENCE:
		  
                                                               
     
		dtrace_get_modrm(x, &mode, &reg, &r_m);

		if (mode == REG_ONLY) {
			  
                                                          
     
                      
                      
      
			if ((uint8_t)x->d86_bytes[x->d86_len - 1] != 0xe8 &&
			    (uint8_t)x->d86_bytes[x->d86_len - 1] != 0xf0)
				goto error;
		} else {
#ifdef DIS_TEXT
			(void) strncpy(x->d86_mnem, "xrstor", OPLEN);
#endif
			dtrace_rex_adjust(rex_prefix, mode, &reg, &r_m);
			dtrace_get_operand(x, mode, r_m, BYTE_OPND, 0);
		}
		break;

	               
	case F:
#ifdef DIS_TEXT
		x->d86_numopnds = 1;
		(void) strlcat(x->d86_opnd[0].d86_opnd, "%st(X)", OPLEN);
		x->d86_opnd[0].d86_opnd[4] = r_m + '0';
#endif
		NOMEM;
		break;

	                                                  
	case FF:
		vbit = opcode2 >> 2 & 0x1;	                           
		OS_FALLTHROUGH;
	case FFC:				                              
#ifdef DIS_TEXT
		x->d86_numopnds = 2;
		(void) strlcat(x->d86_opnd[1 - vbit].d86_opnd, "%st", OPLEN);
		(void) strlcat(x->d86_opnd[vbit].d86_opnd, "%st(X)", OPLEN);
		x->d86_opnd[vbit].d86_opnd[4] = r_m + '0';
#endif
		NOMEM;
		break;

	                      
	case VEX_MO:
		                    
		x->d86_numopnds = 1;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
#ifdef DIS_TEXT
		if ((dp == &dis_opAVX0F[0xA][0xE]) && (reg == 3))
			(void) strncpy(x->d86_mnem, "vstmxcsr", OPLEN);
#endif
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;
	case VEX_RMrX:
	case FMA:
		                                            
		x->d86_numopnds = 3;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		  
                                                             
                                                               
                                                                  
                                                                  
                                                                 
                                 
     
#ifdef DIS_TEXT
		if (dp->it_adrmode == FMA) {
			size_t len = strlen(dp->it_name);
			(void) strncpy(x->d86_mnem, dp->it_name, OPLEN);
			if (len + 1 < OPLEN) {
				(void) strncpy(x->d86_mnem + len,
				    vex_W != 0 ? "d" : "s", OPLEN - len);
			}
		}
#endif

		if (mode != REG_ONLY) {
			if ((dp == &dis_opAVXF20F[0x10]) ||
			    (dp == &dis_opAVXF30F[0x10])) {
				                         
				                            
				x->d86_numopnds = 2;
				goto L_VEX_MX;
			}
		}

		dtrace_get_operand(x, REG_ONLY, reg, wbit, 2);
		  
                                                          
                 
     
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 1);

		if ((dp == &dis_opAVXF20F[0x2A]) ||
		    (dp == &dis_opAVXF30F[0x2A])) {
			  
                                                         
                  
      
			wbit = LONG_OPND;
		}
#ifdef DIS_TEXT
		else if ((mode == REG_ONLY) &&
		    (dp == &dis_opAVX0F[0x1][0x6])) {	              
			(void) strncpy(x->d86_mnem, "vmovlhps", OPLEN);
		} else if ((mode == REG_ONLY) &&
		    (dp == &dis_opAVX0F[0x1][0x2])) {	              
			(void) strncpy(x->d86_mnem, "vmovhlps", OPLEN);
		}
#endif
		dtrace_get_operand(x, mode, r_m, wbit, 0);

		break;

	case VEX_VRMrX:
		                                            
		x->d86_numopnds = 3;
		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		dtrace_get_operand(x, REG_ONLY, reg, wbit, 2);
		  
                                                          
                 
     
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 0);

		dtrace_get_operand(x, mode, r_m, wbit, 1);
		break;

	case VEX_SbVM:
		                                                  
		x->d86_numopnds = 3;
		x->d86_vsib = 1;

		  
                                                                 
                                                               
     

		vreg = &dis_vgather[opcode2][vex_W][vex_L];

#ifdef DIS_TEXT
		(void) strncpy(x->d86_mnem, dp->it_name, OPLEN);
		(void) strlcat(x->d86_mnem + strlen(dp->it_name),
		    vreg->dgr_suffix, OPLEN - strlen(dp->it_name));
#endif

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		dtrace_get_operand(x, REG_ONLY, reg, vreg->dgr_arg2, 2);
		  
                                                          
                 
     
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), vreg->dgr_arg0,
		    0);
		dtrace_get_operand(x, mode, r_m, vreg->dgr_arg1, 1);
		break;

	case VEX_RRX:
		                                           
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		if (mode != REG_ONLY) {
			if ((dp == &dis_opAVXF20F[0x11]) ||
			    (dp == &dis_opAVXF30F[0x11])) {
				                         
				                            
				x->d86_numopnds = 2;
				goto L_VEX_RM;
			}
		}

		dtrace_get_operand(x, mode, r_m, wbit, 2);
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 1);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 0);
		break;

	case VEX_RMRX:
		                                                       
		x->d86_numopnds = 4;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 3);
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 2);
		if (dp == &dis_opAVX660F3A[0x18]) {
			                                             
			dtrace_get_operand(x, mode, r_m, XMM_OPND, 1);
		} else if ((dp == &dis_opAVX660F3A[0x20]) ||
		    (dp == & dis_opAVX660F[0xC4])) {
			                                            
			                                               
			dtrace_get_operand(x, mode, r_m, LONG_OPND, 1);
		} else if (dp == &dis_opAVX660F3A[0x22]) {
			                                              
#ifdef DIS_TEXT
			if (vex_W)
				x->d86_mnem[6] = 'q';
#endif
			dtrace_get_operand(x, mode, r_m, LONG_OPND, 1);
		} else {
			dtrace_get_operand(x, mode, r_m, wbit, 1);
		}

		                               
		dtrace_imm_opnd(x, wbit, 1, 0);

		                                                                
		if ((dp == &dis_opAVX660F3A[0x4A]) ||
		    (dp == &dis_opAVX660F3A[0x4B]) ||
		    (dp == &dis_opAVX660F3A[0x4C])) {
#ifdef DIS_TEXT
			int regnum = (x->d86_opnd[0].d86_value & 0xF0) >> 4;
#endif
			x->d86_opnd[0].d86_mode = MODE_NONE;
#ifdef DIS_TEXT
			if (vex_L)
				(void) strncpy(x->d86_opnd[0].d86_opnd,
				    dis_YMMREG[regnum], OPLEN);
			else
				(void) strncpy(x->d86_opnd[0].d86_opnd,
				    dis_XMMREG[regnum], OPLEN);
#endif
		}
		break;

	case VEX_MX:
		                                 
		x->d86_numopnds = 2;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
L_VEX_MX:

		if ((dp == &dis_opAVXF20F[0xE6]) ||
		    (dp == &dis_opAVX660F[0x5A]) ||
		    (dp == &dis_opAVX660F[0xE6])) {
			                            
			                               
			                                
			dtrace_get_operand(x, REG_ONLY, reg, XMM_OPND, 1);
			dtrace_get_operand(x, mode, r_m, wbit, 0);
		} else if ((dp == &dis_opAVXF30F[0xE6]) ||
		    (dp == &dis_opAVX0F[0x5][0xA]) ||
		    (dp == &dis_opAVX660F38[0x13]) ||
		    (dp == &dis_opAVX660F38[0x18]) ||
		    (dp == &dis_opAVX660F38[0x19]) ||
		    (dp == &dis_opAVX660F38[0x58]) ||
		    (dp == &dis_opAVX660F38[0x78]) ||
		    (dp == &dis_opAVX660F38[0x79]) ||
		    (dp == &dis_opAVX660F38[0x59])) {
			                            
			                               
			                               
			                                  
			dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
			dtrace_get_operand(x, mode, r_m, XMM_OPND, 0);
		} else if (dp == &dis_opAVX660F[0x6E]) {
			                                    
#ifdef DIS_TEXT
			if (vex_W)
				x->d86_mnem[4] = 'q';
#endif
			dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
			dtrace_get_operand(x, mode, r_m, LONG_OPND, 0);
		} else {
			dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
			dtrace_get_operand(x, mode, r_m, wbit, 0);
		}

		break;

	case VEX_MXI:
		                                       
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		dtrace_get_operand(x, REG_ONLY, reg, wbit, 2);
		dtrace_get_operand(x, mode, r_m, wbit, 1);

		                               
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	case VEX_XXI:
		                                     
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
#ifdef DIS_TEXT
		(void) strncpy(x->d86_mnem, dis_AVXvgrp7[opcode2 - 1][reg],
		    OPLEN);
#endif
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 2);
		dtrace_get_operand(x, REG_ONLY, r_m, wbit, 1);

		                               
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	case VEX_MR:
		                                            
		if (dp == &dis_opAVX660F[0xC5]) {
			                                  
			x->d86_numopnds = 2;
			vbit = 2;
		} else {
			x->d86_numopnds = 2;
			vbit = 1;
		}

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, LONG_OPND, vbit);
		dtrace_get_operand(x, mode, r_m, wbit, vbit - 1);

		if (vbit == 2)
			dtrace_imm_opnd(x, wbit, 1, 0);

		break;

	case VEX_RRI:
		                                                       
		x->d86_numopnds = 2;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;

	case VEX_RX:
		                                 
		                               
		if (dp == &dis_opAVX660F3A[0x19] ||
		    dp == &dis_opAVX660F3A[0x1d]) {
			x->d86_numopnds = 3;

			dtrace_get_modrm(x, &mode, &reg, &r_m);
			dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

			dtrace_get_operand(x, mode, r_m, XMM_OPND, 2);
			dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);

			                               
			dtrace_imm_opnd(x, wbit, 1, 0);
			break;
		}

		x->d86_numopnds = 2;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 1);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 0);
		break;

	case VEX_RR:
		                                 
		x->d86_numopnds = 2;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		if (dp == &dis_opAVX660F[0x7E]) {
			                                    
#ifdef DIS_TEXT
			if (vex_W)
				x->d86_mnem[4] = 'q';
#endif
			dtrace_get_operand(x, mode, r_m, LONG_OPND, 1);
		} else
			dtrace_get_operand(x, mode, r_m, wbit, 1);

		dtrace_get_operand(x, REG_ONLY, reg, wbit, 0);
		break;

	case VEX_RRi:
		                                      
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

#ifdef DIS_TEXT
		if (dp == &dis_opAVX660F3A[0x16]) {
			                                             
			if (vex_W)
				x->d86_mnem[6] = 'q';
		}
#endif
		dtrace_get_operand(x, mode, r_m, LONG_OPND, 2);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);

		                               
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;
	case VEX_RIM:
		                                      
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		dtrace_get_operand(x, mode, r_m, XMM_OPND, 2);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
		                               
		dtrace_imm_opnd(x, wbit, 1, 0);
		break;

	case VEX_RM:
		                                 
		if (dp == &dis_opAVX660F3A[0x17]) {	                
			x->d86_numopnds = 3;

			dtrace_get_modrm(x, &mode, &reg, &r_m);
			dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

			dtrace_get_operand(x, mode, r_m, LONG_OPND, 2);
			dtrace_get_operand(x, REG_ONLY, reg, wbit, 1);
			                               
			dtrace_imm_opnd(x, wbit, 1, 0);
			break;
		}
		x->d86_numopnds = 2;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
L_VEX_RM:
		vbit = 1;
		dtrace_get_operand(x, mode, r_m, wbit, vbit);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, vbit - 1);

		break;

	case VEX_RRM:
		                                           
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, mode, r_m, wbit, 2);
		                                                             
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 1);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 0);
		break;

	case VEX_RMX:
		                                           
		x->d86_numopnds = 3;

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);
		dtrace_get_operand(x, REG_ONLY, reg, wbit, 2);
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 1);
		dtrace_get_operand(x, REG_ONLY, r_m, wbit, 0);
		break;

	case VEX_NONE:
#ifdef DIS_TEXT
		if (vex_L)
			(void) strncpy(x->d86_mnem, "vzeroall", OPLEN);
#endif
		break;
	case BLS: {

		  
                                                                
                                                               
                                                              
                                                                 
                                                               
                        
     

		dtrace_get_modrm(x, &mode, &reg, &r_m);
		dtrace_vex_adjust(vex_byte1, mode, &reg, &r_m);

		switch (reg) {
		case 1:
#ifdef	DIS_TEXT
			blsinstr = "blsr";
#endif
			break;
		case 2:
#ifdef	DIS_TEXT
			blsinstr = "blsmsk";
#endif
			break;
		case 3:
#ifdef	DIS_TEXT
			blsinstr = "blsi";
#endif
			break;
		default:
			goto error;
		}

		x->d86_numopnds = 2;
#ifdef DIS_TEXT
		(void) strncpy(x->d86_mnem, blsinstr, OPLEN);
#endif
		dtrace_get_operand(x, REG_ONLY, (0xF - vex_v), wbit, 1);
		dtrace_get_operand(x, mode, r_m, wbit, 0);
		break;
	}
	                        
	case AM:
	case DM:
	case OVERRIDE:
	case PREFIX:
	case UNKNOWN:
		NOMEM;
		OS_FALLTHROUGH;
	default:
		goto error;
	}                 
	if (x->d86_error)
		goto error;

done:
#ifdef DIS_MEM
	  
                                                              
    
	if (x->d86_memsize != 0) {
		return (0);
	} else if (dp->it_stackop) {
		switch (opnd_size) {
		case SIZE16:
			x->d86_memsize = 2;
			break;
		case SIZE32:
			x->d86_memsize = 4;
			break;
		case SIZE64:
			x->d86_memsize = 8;
			break;
		}
	} else if (nomem || mode == REG_ONLY) {
		x->d86_memsize = 0;

	} else if (dp->it_size != 0) {
		  
                                            
                                                        
     
		if (x->d86_mode == SIZE64 && dp->it_size == 6)
			x->d86_memsize = 10;
		else if (x->d86_mode == SIZE64 && opcode1 == 0x9 &&
		    (opcode2 == 0xc || opcode2 == 0xd))
			x->d86_memsize = 8;
		else
			x->d86_memsize = dp->it_size;

	} else if (wbit == 0) {
		x->d86_memsize = 1;

	} else if (wbit == LONG_OPND) {
		if (opnd_size == SIZE64)
			x->d86_memsize = 8;
		else if (opnd_size == SIZE32)
			x->d86_memsize = 4;
		else
			x->d86_memsize = 2;

	} else if (wbit == SEG_OPND) {
		x->d86_memsize = 4;

	} else {
		x->d86_memsize = 8;
	}
#endif
	return (0);

error:
#ifdef DIS_TEXT
	(void) strlcat(x->d86_mnem, "undef", OPLEN);
#endif
	return (1);
}

#ifdef DIS_TEXT

  
                                                           
                                                       
   
static char *unsigned_ops[] = {
	"or", "and", "xor", "test", "in", "out", "lcall", "ljmp",
	"rcr", "rcl", "ror", "rol", "shl", "shr", "sal", "psr", "psl",
	0
};


static int
isunsigned_op(char *opcode)
{
	char *where;
	int i;
	int is_unsigned = 0;

	  
                                                          
                             
    
	where = opcode + strlen(opcode) - 1;
	while (where > opcode && *where != ' ')
		--where;
	if (*where == ' ')
		++where;

	for (i = 0; unsigned_ops[i]; ++i) {
		if (strncmp(where, unsigned_ops[i],
		    strlen(unsigned_ops[i])))
			continue;
		is_unsigned = 1;
		break;
	}
	return (is_unsigned);
}

  
                                                                    
                                                                  
                                                                
                                                                        
                                                                    
                            
  
                                                              
  
                                                              
   

#define	NEG_LIMIT	-255
enum {IMM, DISP};
enum {POS, TRY_NEG};

static int
print_imm(dis86_t *dis, uint64_t usv, uint64_t mask, char *buf,
    size_t buflen, int disp, int try_neg)
{
	int curlen;
	int64_t sv = (int64_t)usv;
	int octal = dis->d86_flags & DIS_F_OCTAL;

	curlen = strlen(buf);

	if (try_neg == TRY_NEG && sv < 0 &&
	    (disp || sv >= NEG_LIMIT) &&
	    !isunsigned_op(dis->d86_mnem)) {
		dis->d86_sprintf_func(buf + curlen, buflen - curlen,
		    octal ? "-0%llo" : "-0x%llx", (-sv) & mask);
		return (1);
	} else {
		if (disp == DISP)
			dis->d86_sprintf_func(buf + curlen, buflen - curlen,
			    octal ? "+0%llo" : "+0x%llx", usv & mask);
		else
			dis->d86_sprintf_func(buf + curlen, buflen - curlen,
			    octal ? "0%llo" : "0x%llx", usv & mask);
		return (0);

	}
}


static int
log2(int size)
{
	switch (size) {
	case 1: return (0);
	case 2: return (1);
	case 4: return (2);
	case 8: return (3);
	}
	return (0);
}

              
void
dtrace_disx86_str(dis86_t *dis, uint_t mode, uint64_t pc, char *buf,
    size_t buflen)
{
	uint64_t reltgt = 0;
	uint64_t tgt = 0;
	int curlen;
	int (*lookup)(void *, uint64_t, char *, size_t);
	int i;
	int64_t sv;
	uint64_t usv, mask, save_mask, save_usv;
	static uint64_t masks[] =
	    {0xffU, 0xffffU, 0xffffffffU, 0xffffffffffffffffULL};
	save_usv = 0;

	dis->d86_sprintf_func(buf, buflen, "%-6s ", dis->d86_mnem);

	  
                                                                       
                                                    
    
	pc += dis->d86_len;

	for (i = 0; i < dis->d86_numopnds; i++) {
		d86opnd_t *op = &dis->d86_opnd[i];

		if (i != 0)
			(void) strlcat(buf, ",", buflen);

		(void) strlcat(buf, op->d86_prefix, buflen);

		  
                                                          
                                                    
                                    
     

		sv = usv = op->d86_value;

		  
                                               
                                               
                                                   
                                               
                                             
                                                     
                                                         
     

		                                                       
		mask = masks[dis->d86_addr_size];

		                                                   
		if (op->d86_mode == MODE_SIGNED ||
		    op->d86_mode == MODE_IMPLIED)
			mask = masks[log2(op->d86_value_size)];

		switch (op->d86_mode) {

		case MODE_NONE:

			(void) strlcat(buf, op->d86_opnd, buflen);
			break;

		case MODE_SIGNED:
		case MODE_IMPLIED:
		case MODE_OFFSET:

			tgt = usv;

			if (dis->d86_seg_prefix)
				(void) strlcat(buf, dis->d86_seg_prefix,
				    buflen);

			if (op->d86_mode == MODE_SIGNED ||
			    op->d86_mode == MODE_IMPLIED) {
				(void) strlcat(buf, "$", buflen);
			}

			if (print_imm(dis, usv, mask, buf, buflen,
			    IMM, TRY_NEG) &&
			    (op->d86_mode == MODE_SIGNED ||
			    op->d86_mode == MODE_IMPLIED)) {

				  
                                         
                              
                                              
                                     
                  
       

				save_usv = usv;
				save_mask = mask;
			}
			(void) strlcat(buf, op->d86_opnd, buflen);

			break;

		case MODE_IPREL:
		case MODE_RIPREL:

			reltgt = pc + sv;

			switch (mode) {
			case SIZE16:
				reltgt = (uint16_t)reltgt;
				break;
			case SIZE32:
				reltgt = (uint32_t)reltgt;
				break;
			}

			(void) print_imm(dis, usv, mask, buf, buflen,
			    DISP, TRY_NEG);

			if (op->d86_mode == MODE_RIPREL)
				(void) strlcat(buf, "(%rip)", buflen);
			break;
		}
	}

	  
                                                     
                                                               
                                                      
    

	lookup = dis->d86_sym_lookup;
	if (tgt != 0) {
		if ((dis->d86_flags & DIS_F_NOIMMSYM) == 0 &&
		    lookup(dis->d86_data, tgt, NULL, 0) == 0) {
			(void) strlcat(buf, "\t<", buflen);
			curlen = strlen(buf);
			lookup(dis->d86_data, tgt, buf + curlen,
			    buflen - curlen);
			(void) strlcat(buf, ">", buflen);
		}

		  
                                                        
                                                 
     
		if (save_usv) {
			(void) strlcat(buf, "\t<", buflen);
			(void) print_imm(dis, save_usv, save_mask, buf, buflen,
			    IMM, POS);
			(void) strlcat(buf, ">", buflen);
		}
	}

	if (reltgt != 0) {
		                                                  

		(void) strlcat(buf, "\t<", buflen);
		curlen = strlen(buf);
		lookup(dis->d86_data, reltgt, buf + curlen,
		    buflen - curlen);
		(void) strlcat(buf, ">", buflen);
	}
}

#endif               
