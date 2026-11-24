/*
 * Copyright (c) 2000-2020 Apple Computer, Inc. All rights reserved.
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

  
                         
  
   

#ifndef _MACHINE_CPUID_H_
#define _MACHINE_CPUID_H_

#include <sys/appleapiopts.h>

#if defined(MACH_KERNEL_PRIVATE) && !defined(ASSEMBLER)
#include <i386/hw_defs.h>
#include <i386/pio.h>
#include <i386/machine_routines.h>
#endif

#ifdef __APPLE_API_PRIVATE

#define CPUID_VID_INTEL         "GenuineIntel"
#define CPUID_VID_AMD           "AuthenticAMD"

#define CPUID_VMM_ID_VMWARE     "VMwareVMware"
#define CPUID_VMM_ID_PARALLELS  "Parallels\0\0\0"
#define CPUID_VMM_ID_HYVE       "bhyve bhyve "
#define CPUID_VMM_ID_HVF        "HVFHVFHVFHVF"
#define CPUID_VMM_ID_KVM        "KVMKVMKVM\0\0\0"

#define CPUID_STRING_UNKNOWN    "Unknown CPU Typ"

#define _Bit(n)                 (1ULL << n)
#define _HBit(n)                (1ULL << ((n)+32))

  
                                                    
                                                           
   
#define CPUID_FEATURE_FPU       _Bit(0)                                    
#define CPUID_FEATURE_VME       _Bit(1)                               
#define CPUID_FEATURE_DE        _Bit(2)                            
#define CPUID_FEATURE_PSE       _Bit(3)                            
#define CPUID_FEATURE_TSC       _Bit(4)                           
#define CPUID_FEATURE_MSR       _Bit(5)                                 
#define CPUID_FEATURE_PAE       _Bit(6)                                   
#define CPUID_FEATURE_MCE       _Bit(7)                                
#define CPUID_FEATURE_CX8       _Bit(8)                  
#define CPUID_FEATURE_APIC      _Bit(9)                     
#define CPUID_FEATURE_SEP       _Bit(11)                        
#define CPUID_FEATURE_MTRR      _Bit(12)                                  
#define CPUID_FEATURE_PGE       _Bit(13)                          
#define CPUID_FEATURE_MCA       _Bit(14)                                  
#define CPUID_FEATURE_CMOV      _Bit(15)                                    
#define CPUID_FEATURE_PAT       _Bit(16)                            
#define CPUID_FEATURE_PSE36     _Bit(17)                                  
#define CPUID_FEATURE_PSN       _Bit(18)                               
#define CPUID_FEATURE_CLFSH     _Bit(19)                                     
#define CPUID_FEATURE_DS        _Bit(21)                   
#define CPUID_FEATURE_ACPI      _Bit(22)                                      
#define CPUID_FEATURE_MMX       _Bit(23)                     
#define CPUID_FEATURE_FXSR      _Bit(24)                                     
#define CPUID_FEATURE_SSE       _Bit(25)                                 
#define CPUID_FEATURE_SSE2      _Bit(26)                                   
#define CPUID_FEATURE_SS        _Bit(27)                  
#define CPUID_FEATURE_HTT       _Bit(28)                                  
#define CPUID_FEATURE_TM        _Bit(29)                             
#define CPUID_FEATURE_PBE       _Bit(31)                         

#define CPUID_FEATURE_SSE3      _HBit(0)                                   
#define CPUID_FEATURE_PCLMULQDQ _HBit(1)                             
#define CPUID_FEATURE_DTES64    _HBit(2)                        
#define CPUID_FEATURE_MONITOR   _HBit(3)                     
#define CPUID_FEATURE_DSCPL     _HBit(4)                       
#define CPUID_FEATURE_VMX       _HBit(5)           
#define CPUID_FEATURE_SMX       _HBit(6)           
#define CPUID_FEATURE_EST       _HBit(7)                                
#define CPUID_FEATURE_TM2       _HBit(8)                         
#define CPUID_FEATURE_SSSE3     _HBit(9)                                      
#define CPUID_FEATURE_CID       _HBit(10)                    
#define CPUID_FEATURE_SEGLIM64  _HBit(11)                                    
#define CPUID_FEATURE_FMA       _HBit(12)                                 
#define CPUID_FEATURE_CX16      _HBit(13)                             
#define CPUID_FEATURE_xTPR      _HBit(14)                              
#define CPUID_FEATURE_PDCM      _HBit(15)                                

#define CPUID_FEATURE_PCID      _HBit(17)                        
#define CPUID_FEATURE_DCA       _HBit(18)                          
#define CPUID_FEATURE_SSE4_1    _HBit(19)                                    
#define CPUID_FEATURE_SSE4_2    _HBit(20)                                    
#define CPUID_FEATURE_x2APIC    _HBit(21)                         
#define CPUID_FEATURE_MOVBE     _HBit(22)                        
#define CPUID_FEATURE_POPCNT    _HBit(23)                         
#define CPUID_FEATURE_TSCTMR    _HBit(24)                         
#define CPUID_FEATURE_AES       _HBit(25)                       
#define CPUID_FEATURE_XSAVE     _HBit(26)                         
#define CPUID_FEATURE_OSXSAVE   _HBit(27)                                 
#define CPUID_FEATURE_AVX1_0    _HBit(28)                           
#define CPUID_FEATURE_F16C      _HBit(29)                                   
#define CPUID_FEATURE_RDRAND    _HBit(30)                         
#define CPUID_FEATURE_VMM       _HBit(31)                               

  
                                         
                                                                               
   
#define CPUID_LEAF7_FEATURE_RDWRFSGS   _Bit(0)                               
#define CPUID_LEAF7_FEATURE_TSCOFF     _Bit(1)                           
#define CPUID_LEAF7_FEATURE_SGX        _Bit(2)                                   
#define CPUID_LEAF7_FEATURE_BMI1       _Bit(3)                                        
#define CPUID_LEAF7_FEATURE_HLE        _Bit(4)                              
#define CPUID_LEAF7_FEATURE_AVX2       _Bit(5)                           
#define CPUID_LEAF7_FEATURE_FDPEO      _Bit(6)                                                             
#define CPUID_LEAF7_FEATURE_SMEP       _Bit(7)                                         
#define CPUID_LEAF7_FEATURE_BMI2       _Bit(8)                                        
#define CPUID_LEAF7_FEATURE_ERMS       _Bit(9)                                  
#define CPUID_LEAF7_FEATURE_INVPCID    _Bit(10)                                
#define CPUID_LEAF7_FEATURE_RTM        _Bit(11)            
#define CPUID_LEAF7_FEATURE_PQM        _Bit(12)                                
#define CPUID_LEAF7_FEATURE_FPU_CSDS   _Bit(13)                              
#define CPUID_LEAF7_FEATURE_MPX        _Bit(14)                                     
#define CPUID_LEAF7_FEATURE_PQE        _Bit(15)                                 
#define CPUID_LEAF7_FEATURE_AVX512F    _Bit(16)                             
#define CPUID_LEAF7_FEATURE_AVX512DQ   _Bit(17)                              
#define CPUID_LEAF7_FEATURE_RDSEED     _Bit(18)                           
#define CPUID_LEAF7_FEATURE_ADX        _Bit(19)                         
#define CPUID_LEAF7_FEATURE_SMAP       _Bit(20)                                       
#define CPUID_LEAF7_FEATURE_AVX512IFMA _Bit(21)                                
#define CPUID_LEAF7_FEATURE_CLFSOPT    _Bit(23)                
#define CPUID_LEAF7_FEATURE_CLWB       _Bit(24)             
#define CPUID_LEAF7_FEATURE_IPT        _Bit(25)                              
#define CPUID_LEAF7_FEATURE_AVX512CD   _Bit(28)                              
#define CPUID_LEAF7_FEATURE_SHA        _Bit(29)                         
#define CPUID_LEAF7_FEATURE_AVX512BW   _Bit(30)                              
#define CPUID_LEAF7_FEATURE_AVX512VL   _Bit(31)                              

#define CPUID_LEAF7_FEATURE_PREFETCHWT1  _HBit(0)                              
#define CPUID_LEAF7_FEATURE_AVX512VBMI   _HBit(1)                               
#define CPUID_LEAF7_FEATURE_UMIP         _HBit(2)                                        
#define CPUID_LEAF7_FEATURE_PKU          _HBit(3)                                    
#define CPUID_LEAF7_FEATURE_OSPKE        _HBit(4)                          
#define CPUID_LEAF7_FEATURE_WAITPKG      _HBit(5)                            
#define CPUID_LEAF7_FEATURE_GFNI         _HBit(8)                                     
#define CPUID_LEAF7_FEATURE_VAES         _HBit(9)                          
#define CPUID_LEAF7_FEATURE_VPCLMULQDQ   _HBit(10)                                
#define CPUID_LEAF7_FEATURE_AVX512VNNI   _HBit(11)                                            
#define CPUID_LEAF7_FEATURE_AVX512BITALG _HBit(12)                                           
#define CPUID_LEAF7_FEATURE_AVX512VPCDQ  _HBit(14)                                   
#define CPUID_LEAF7_FEATURE_RDPID        _HBit(22)                             
#define CPUID_LEAF7_FEATURE_CLDEMOTE     _HBit(25)                        
#define CPUID_LEAF7_FEATURE_MOVDIRI      _HBit(27)                          
#define CPUID_LEAF7_FEATURE_MOVDIRI64B   _HBit(28)                             
#define CPUID_LEAF7_FEATURE_SGXLC        _HBit(30)                               

  
                                                    
   
#define CPUID_LEAF7_EXTFEATURE_AVX5124VNNIW     _Bit(2)                            
#define CPUID_LEAF7_EXTFEATURE_AVX5124FMAPS     _Bit(3)                            
#define CPUID_LEAF7_EXTFEATURE_FSREPMOV         _Bit(4)                                 
#define CPUID_LEAF7_EXTFEATURE_SRBDS_CTRL       _Bit(9)                                                        
#define CPUID_LEAF7_EXTFEATURE_MDCLEAR          _Bit(10)                                         
#define CPUID_LEAF7_EXTFEATURE_TSXFA            _Bit(13)                                     
#define CPUID_LEAF7_EXTFEATURE_IBRS             _Bit(26)                         
#define CPUID_LEAF7_EXTFEATURE_STIBP            _Bit(27)                                                      
#define CPUID_LEAF7_EXTFEATURE_L1DF             _Bit(28)                           
#define CPUID_LEAF7_EXTFEATURE_ACAPMSR          _Bit(29)                          
#define CPUID_LEAF7_EXTFEATURE_CCAPMSR          _Bit(30)                          
#define CPUID_LEAF7_EXTFEATURE_SSBD             _Bit(31)                                              

  
                                                       
                                                                    
   
#define CPUID_EXTFEATURE_SYSCALL   _Bit(11)                         
#define CPUID_EXTFEATURE_XD        _Bit(20)                          

#define CPUID_EXTFEATURE_1GBPAGE   _Bit(26)                    
#define CPUID_EXTFEATURE_RDTSCP    _Bit(27)                 
#define CPUID_EXTFEATURE_EM64T     _Bit(29)                                     

#define CPUID_EXTFEATURE_LAHF      _HBit(0)                                 
#define CPUID_EXTFEATURE_LZCNT     _HBit(5)                            
#define CPUID_EXTFEATURE_PREFETCHW _HBit(8)                                

  
                                                       
                                                                    
   
#define CPUID_EXTFEATURE_TSCI      _Bit(8)                         

  
                                                                        
                                                                          
                                                                   
   
#define CPUID_X86_64_H_FEATURE_SUBSET ( CPUID_FEATURE_FMA    | \
	                                CPUID_FEATURE_SSE4_2 | \
	                                CPUID_FEATURE_MOVBE  | \
	                                CPUID_FEATURE_POPCNT | \
	                                CPUID_FEATURE_AVX1_0   \
	                              )

#define CPUID_X86_64_H_EXTFEATURE_SUBSET ( CPUID_EXTFEATURE_LZCNT \
	                                 )

#define CPUID_X86_64_H_LEAF7_FEATURE_SUBSET ( CPUID_LEAF7_FEATURE_BMI1 | \
	                                      CPUID_LEAF7_FEATURE_AVX2 | \
	                                      CPUID_LEAF7_FEATURE_BMI2   \
	                                    )

#define CPUID_CACHE_SIZE        16                                       

#define CPUID_MWAIT_EXTENSION   _Bit(0)                                      
#define CPUID_MWAIT_BREAK       _Bit(1)                                     

#define CPUID_MODEL_PENRYN              0x17
#define CPUID_MODEL_NEHALEM             0x1A
#define CPUID_MODEL_FIELDS              0x1E                                
#define CPUID_MODEL_DALES               0x1F                               
#define CPUID_MODEL_NEHALEM_EX          0x2E
#define CPUID_MODEL_DALES_32NM          0x25                              
#define CPUID_MODEL_WESTMERE            0x2C                                   
#define CPUID_MODEL_WESTMERE_EX         0x2F
#define CPUID_MODEL_SANDYBRIDGE         0x2A
#define CPUID_MODEL_JAKETOWN            0x2D
#define CPUID_MODEL_IVYBRIDGE           0x3A
#define CPUID_MODEL_IVYBRIDGE_EP        0x3E
#define CPUID_MODEL_CRYSTALWELL         0x46
#define CPUID_MODEL_HASWELL             0x3C
#define CPUID_MODEL_HASWELL_EP          0x3F
#define CPUID_MODEL_HASWELL_ULT         0x45
#define CPUID_MODEL_BROADWELL           0x3D
#define CPUID_MODEL_BROADWELL_ULX       0x3D
#define CPUID_MODEL_BROADWELL_ULT       0x3D
#define CPUID_MODEL_BRYSTALWELL         0x47
#define CPUID_MODEL_SKYLAKE             0x4E
#define CPUID_MODEL_SKYLAKE_ULT         0x4E
#define CPUID_MODEL_SKYLAKE_ULX         0x4E
#define CPUID_MODEL_SKYLAKE_DT          0x5E
#define CPUID_MODEL_SKYLAKE_W           0x55
#define PLATID_XEON_SP_1                0x00
#define PLATID_XEON_SP_2                0x07
#define PLATID_MAYBE_XEON_SP            0x01
#define CPUID_MODEL_KABYLAKE            0x8E
#define CPUID_MODEL_KABYLAKE_ULT        0x8E
#define CPUID_MODEL_KABYLAKE_ULX        0x8E
#define CPUID_MODEL_KABYLAKE_DT         0x9E
#define CPUID_MODEL_ICELAKE             0x7E
#define CPUID_MODEL_ICELAKE_ULT         0x7E
#define CPUID_MODEL_ICELAKE_ULX         0x7E
#define CPUID_MODEL_ICELAKE_DT          0x7D
#define CPUID_MODEL_ICELAKE_H           0x9F

#define CPUID_VMM_FAMILY_NONE           0x0
#define CPUID_VMM_FAMILY_UNKNOWN        0x1
#define CPUID_VMM_FAMILY_VMWARE         0x2
#define CPUID_VMM_FAMILY_PARALLELS      0x3
#define CPUID_VMM_FAMILY_HYVE           0x4
#define CPUID_VMM_FAMILY_HVF            0x5
#define CPUID_VMM_FAMILY_KVM            0x6


#if DEBUG || DEVELOPMENT

  
                                        
                                                                   
                                                            
   

#define APPLEPV_INTERFACE_LEAF_INDEX    1
#define APPLEPV_FEATURES_LEAF_INDEX     2
#define APPLEPV_LEAF_INDEX_MAX          APPLEPV_FEATURES_LEAF_INDEX

#define APPLEPV_SIGNATURE               "apple-pv-xnu"
#define APPLEPV_INTERFACE               "AH#1"

  
                                   
                                               
   

#define CPUID_LEAF_FEATURE_COREDUMP         _Bit(0)

#endif                           


#ifndef ASSEMBLER
#include <stdint.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <mach/machine.h>


typedef enum { eax, ebx, ecx, edx } cpuid_register_t;
static inline void
cpuid(uint32_t *data)
{
	__asm__ volatile ("cpuid"
                : "=a" (data[eax]),
                  "=b" (data[ebx]),
                  "=c" (data[ecx]),
                  "=d" (data[edx])
                : "a"  (data[eax]),
                  "b"  (data[ebx]),
                  "c"  (data[ecx]),
                  "d"  (data[edx]));
}

static inline void
do_cpuid(uint32_t selector, uint32_t *data)
{
	__asm__ volatile ("cpuid"
                : "=a" (data[0]),
                  "=b" (data[1]),
                  "=c" (data[2]),
                  "=d" (data[3])
                : "a"(selector),
                  "b" (0),
                  "c" (0),
                  "d" (0));
}

  
                                                             
                            
   
typedef enum { Lnone, L1I, L1D, L2U, L3U, LCACHE_MAX } cache_type_t;
typedef struct {
	unsigned char   value;                                
	cache_type_t    type;                           
	unsigned int    size;                           
	unsigned int    linesize;                            
#ifdef KERNEL
	const char      *description;                          
#endif             
} cpuid_cache_desc_t;

#ifdef KERNEL
#define CACHE_DESC(value, type, size, linesize, text) \
	{ value, type, size, linesize, text }
#else
#define CACHE_DESC(value, type, size, linesize, text) \
	{ value, type, size, linesize }
#endif             

                         
typedef struct {
	uint32_t        linesize_min;
	uint32_t        linesize_max;
	uint32_t        extensions;
	uint32_t        sub_Cstates;
} cpuid_mwait_leaf_t;

                                        
typedef struct {
	boolean_t       sensor;
	boolean_t       dynamic_acceleration;
	boolean_t       invariant_APIC_timer;
	boolean_t       core_power_limits;
	boolean_t       fine_grain_clock_mod;
	boolean_t       package_thermal_intr;
	uint32_t        thresholds;
	boolean_t       ACNT_MCNT;
	boolean_t       hardware_feedback;
	boolean_t       energy_policy;
} cpuid_thermal_leaf_t;


                         
typedef struct {
	uint32_t        extended_state[4];                      
} cpuid_xsave_leaf_t;


                                                
typedef struct {
	uint8_t         version;
	uint8_t         number;
	uint8_t         width;
	uint8_t         events_number;
	uint32_t        events;
	uint8_t         fixed_number;
	uint8_t         fixed_width;
} cpuid_arch_perf_leaf_t;

                                                             
typedef struct {
	uint32_t        numerator;
	uint32_t        denominator;
} cpuid_tsc_leaf_t;

                                                                                           
typedef struct i386_cpu_info {
	char            cpuid_vendor[16];
	char            cpuid_brand_string[48];
	const char      *cpuid_model_string;

	cpu_type_t      cpuid_type;                                                             
	uint8_t         cpuid_family;
	uint8_t         cpuid_model;
	uint8_t         cpuid_extmodel;
	uint8_t         cpuid_extfamily;
	uint8_t         cpuid_stepping;
	uint64_t        cpuid_features;
	uint64_t        cpuid_extfeatures;
	uint32_t        cpuid_signature;
	uint8_t         cpuid_brand;
	uint8_t         cpuid_processor_flag;

	uint32_t        cache_size[LCACHE_MAX];
	uint32_t        cache_linesize;

	uint8_t         cache_info[64];                                   

	uint32_t        cpuid_cores_per_package;
	uint32_t        cpuid_logical_per_package;
	uint32_t        cache_sharing[LCACHE_MAX];
	uint32_t        cache_partitions[LCACHE_MAX];

	cpu_type_t      cpuid_cpu_type;                                       
	cpu_subtype_t   cpuid_cpu_subtype;                                    

	                     
	cpuid_mwait_leaf_t      cpuid_mwait_leaf;
#define cpuid_mwait_linesize_max        cpuid_mwait_leaf.linesize_max
#define cpuid_mwait_linesize_min        cpuid_mwait_leaf.linesize_min
#define cpuid_mwait_extensions          cpuid_mwait_leaf.extensions
#define cpuid_mwait_sub_Cstates         cpuid_mwait_leaf.sub_Cstates
	cpuid_thermal_leaf_t    cpuid_thermal_leaf;
	cpuid_arch_perf_leaf_t  cpuid_arch_perf_leaf;
	uint32_t        unused[4];                                            

	                    
	uint32_t        cpuid_cache_linesize;
	uint32_t        cpuid_cache_L2_associativity;
	uint32_t        cpuid_cache_size;

	                                        
	uint32_t        cpuid_address_bits_physical;
	uint32_t        cpuid_address_bits_virtual;

	uint32_t        cpuid_microcode_version;

	                                                                     
	uint32_t        cpuid_tlb[2][2][2];
			#define TLB_INST        0
			#define TLB_DATA        1
			#define TLB_SMALL       0
			#define TLB_LARGE       1
	uint32_t        cpuid_stlb;

	uint32_t        core_count;
	uint32_t        thread_count;

	                                       
	uint32_t        cpuid_max_basic;
	uint32_t        cpuid_max_ext;

	                                
	uint32_t                cpuid_cpufamily;
	cpuid_mwait_leaf_t      *cpuid_mwait_leafp;
	cpuid_thermal_leaf_t    *cpuid_thermal_leafp;
	cpuid_arch_perf_leaf_t  *cpuid_arch_perf_leafp;
	cpuid_xsave_leaf_t      *cpuid_xsave_leafp;
	uint64_t                cpuid_leaf7_features;
	uint64_t                cpuid_leaf7_extfeatures;
	cpuid_tsc_leaf_t        cpuid_tsc_leaf;
	cpuid_xsave_leaf_t      cpuid_xsave_leaf[2];
} i386_cpu_info_t;

#if defined(MACH_KERNEL_PRIVATE) && !defined(ASSEMBLER)
                           
#define bit32(n)                (1U << (n))
#define bitmask32(h, l)          ((bit32(h)|(bit32(h)-1)) & ~(bit32(l)-1))
#define bitfield32(x, h, l)       ((((x) & bitmask32(h,l)) >> l))

typedef struct {
	char            cpuid_vmm_vendor[16];
	uint32_t        cpuid_vmm_family;
	uint32_t        cpuid_vmm_bus_frequency;
	uint32_t        cpuid_vmm_tsc_frequency;
	uint64_t        cpuid_vmm_applepv_features;
} i386_vmm_info_t;

typedef enum {
	CPU_INTEL_SEGCHK = 1,
	CPU_INTEL_TSXFA = 2,
	CPU_INTEL_TSXDA = 4,
	CPU_INTEL_SRBDS = 8
} cpu_wa_e;

typedef enum {
	CWA_ON = 2,
	CWA_FORCE_ON = 3,                                                                    
	CWA_OFF = 4,
	CWA_FORCE_OFF = 5                                                  
} cwa_classifier_e;

static inline int
is_xeon_sp(uint8_t platid)
{
	if (platid == PLATID_XEON_SP_1 || platid == PLATID_XEON_SP_2) {
		return 1;
	}
	if (platid != PLATID_MAYBE_XEON_SP) {
		return 0;
	}

	boolean_t intrs = ml_set_interrupts_enabled(FALSE);
	outl(cfgAdr, XeonCapID5);
	uint32_t cap5reg = inl(cfgDat);
	ml_set_interrupts_enabled(intrs);
	                                                        
	if (bitfield32(cap5reg, 13, 9) == 3) {
		return 1;
	}
	return 0;
}

extern int force_tecs_at_idle;

#endif                                                          

#ifdef __cplusplus
extern "C" {
#endif

  
                        
   
extern cpu_type_t       cpuid_cputype(void);
extern cpu_subtype_t    cpuid_cpusubtype(void);
extern void             cpuid_cpu_display(const char *);
extern void             cpuid_feature_display(const char *);
extern void             cpuid_extfeature_display(const char *);
extern char *           cpuid_get_feature_names(uint64_t, char *, unsigned);
extern char *           cpuid_get_extfeature_names(uint64_t, char *, unsigned);
extern char *           cpuid_get_leaf7_feature_names(uint64_t, char *, unsigned);
extern char *           cpuid_get_leaf7_extfeature_names(uint64_t, char *, unsigned);

extern uint64_t         cpuid_features(void);
extern uint64_t         cpuid_extfeatures(void);
extern uint64_t         cpuid_leaf7_features(void);
extern uint64_t         cpuid_leaf7_extfeatures(void);
extern uint32_t         cpuid_family(void);
extern uint32_t         cpuid_cpufamily(void);

extern i386_cpu_info_t  *cpuid_info(void);
extern void             cpuid_set_info(void);
extern boolean_t        cpuid_vmm_present(void);
extern uint32_t         cpuid_vmm_family(void);

#if DEBUG || DEVELOPMENT
extern uint64_t         cpuid_vmm_get_applepv_features(void);
#endif                           

#ifdef MACH_KERNEL_PRIVATE
extern i386_vmm_info_t  *cpuid_vmm_info(void);
extern cwa_classifier_e cpuid_wa_required(cpu_wa_e wa);
extern void cpuid_do_was(void);
extern const char       *cpuid_vmm_family_string(void);
#endif

#ifdef __cplusplus
}
#endif

#endif                

#endif                          
#endif                        
