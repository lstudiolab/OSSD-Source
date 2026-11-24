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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Portions Copyright (c) 2012 by Delphix. All rights reserved.
 * Portions Copyright (c) 2016 by Joyent, Inc.
 */

#ifndef _SYS_DTRACE_IMPL_H
#define	_SYS_DTRACE_IMPL_H

#ifdef	__cplusplus
extern "C" {
#endif

  
                                                                    
  
                                                                           
                                                                            
                                                                             
                                                                          
                                                                             
                                                                            
   

#include <sys/dtrace.h>

  
                              
   
extern lck_attr_t dtrace_lck_attr;
extern lck_grp_t dtrace_lck_grp;
extern lck_mtx_t dtrace_procwaitfor_lock;

  
                                               
   
#define	DTRACE_MAXPROPLEN		128
#define	DTRACE_DYNVAR_CHUNKSIZE		256

struct dtrace_probe;
struct dtrace_ecb;
struct dtrace_predicate;
struct dtrace_action;
struct dtrace_provider;
struct dtrace_state;

typedef struct dtrace_probe dtrace_probe_t;
typedef struct dtrace_ecb dtrace_ecb_t;
typedef struct dtrace_predicate dtrace_predicate_t;
typedef struct dtrace_action dtrace_action_t;
typedef struct dtrace_provider dtrace_provider_t;
typedef struct dtrace_meta dtrace_meta_t;
typedef struct dtrace_state dtrace_state_t;
typedef uint32_t dtrace_optid_t;
typedef uint32_t dtrace_specid_t;
typedef uint64_t dtrace_genid_t;

  
                
  
                                                                            
                                                                             
                                                                              
                                                                             
                                                                               
                                                                            
                                                                               
                                                                           
                                                                    
                                                                             
                                                                               
                                                                      
                                                                           
                                                                             
                                                                               
                                                                        
                                    
   
struct dtrace_probe {
	dtrace_id_t dtpr_id;			                      
	dtrace_ecb_t *dtpr_ecb;			                         
	dtrace_ecb_t *dtpr_ecb_last;		                      
	void *dtpr_arg;				                       
	dtrace_cacheid_t dtpr_predcache;	                        
	int dtpr_aframes;			                       
	dtrace_provider_t *dtpr_provider;	                         
	char *dtpr_mod;				                         
	char *dtpr_func;			                           
	char *dtpr_name;			                  
	dtrace_probe_t *dtpr_nextprov;		                           
	dtrace_probe_t *dtpr_prevprov;		                               
	dtrace_probe_t *dtpr_nextmod;		                         
	dtrace_probe_t *dtpr_prevmod;		                             
	dtrace_probe_t *dtpr_nextfunc;		                           
	dtrace_probe_t *dtpr_prevfunc;		                               
	dtrace_probe_t *dtpr_nextname;		                       
	dtrace_probe_t *dtpr_prevname;		                           
	dtrace_genid_t dtpr_gen;		                         
};

typedef int dtrace_probekey_f(const char *, const char *, int);

typedef struct dtrace_probekey {
	const char *dtpk_prov;			                            
	dtrace_probekey_f *dtpk_pmatch;		                                
	const char *dtpk_mod;			                          
	dtrace_probekey_f *dtpk_mmatch;		                              
	const char *dtpk_func;			                        
	dtrace_probekey_f *dtpk_fmatch;		                            
	const char *dtpk_name;			                   
	dtrace_probekey_f *dtpk_nmatch;		                            
	dtrace_id_t dtpk_id;			                         
} dtrace_probekey_t;

typedef struct dtrace_hashbucket {
	struct dtrace_hashbucket *dthb_next;	                        
	void *dthb_chain;			                       
	int dthb_len;				                           
} dtrace_hashbucket_t;

typedef const char* dtrace_strkey_f(void*, uintptr_t);

typedef struct dtrace_hash {
	dtrace_hashbucket_t **dth_tab;	                
	int dth_size;			                        
	int dth_mask;			                              
	int dth_nbuckets;		                             
	uintptr_t dth_nextoffs;		                               
	uintptr_t dth_prevoffs;		                               
	dtrace_strkey_f *dth_getstr;	                                     
	uintptr_t dth_stroffs;		                              
} dtrace_hash_t;

  
                                 
  
                                                                        
                                                                           
                                                                        
                                                                         
                                                                             
                                                                           
                                                                              
                                                                        
                                                   
  
                           
                      
                             
                                                                    
                                                                           
                                       
                                       
                                                                
                                                                
                                                                  
                                                                            
                                                                  
                                  
                                                   
                                                        
                                                           
                                                                       
                                                                                
                                                                                
                                                                  
                                                                  
                                                                              
                                                                            
                                  
                                                   
                                                        
                                                           
                                                                       
                                                                                
                                                                  
                                                                  
                                                                              
                                                                            
                                  
                                  
                                  
  
  
                                                                          
                                                                           
                                                                       
                                                                          
                                                                              
                                                                          
                                 
  
                                                                         
                                                                           
                                                                         
   
struct dtrace_ecb {
	dtrace_epid_t dte_epid;			                      
	uint32_t dte_alignment;			                        
	size_t dte_needed;			                                
	size_t dte_size;			                              
	dtrace_predicate_t *dte_predicate;	                       
	dtrace_action_t *dte_action;		                     
	dtrace_ecb_t *dte_next;			                       
	dtrace_state_t *dte_state;		                      
	uint32_t dte_cond;			                        
	dtrace_probe_t *dte_probe;		                      
	dtrace_action_t *dte_action_last;	                        
	uint64_t dte_uarg;			                      
};

struct dtrace_predicate {
	dtrace_difo_t *dtp_difo;		                
	dtrace_cacheid_t dtp_cacheid;		                      
	int dtp_refcnt;				                     
};

struct dtrace_action {
	dtrace_actkind_t dta_kind;		                    
	uint16_t dta_intuple;			                              
	uint32_t dta_refcnt;			                     
	dtrace_difo_t *dta_difo;		                     
	dtrace_recdesc_t dta_rec;		                        
	dtrace_action_t *dta_prev;		                     
	dtrace_action_t *dta_next;		                 
};

typedef struct dtrace_aggregation {
	dtrace_action_t dtag_action;		                           
	dtrace_aggid_t dtag_id;			                
	dtrace_ecb_t *dtag_ecb;			                       
	dtrace_action_t *dtag_first;		                           
	uint32_t dtag_base;			                         
	uint8_t dtag_hasarg;			                            
	uint64_t dtag_initial;			                   
	void (*dtag_aggregate)(uint64_t *, uint64_t, uint64_t);
} dtrace_aggregation_t;

  
                 
  
                                                                          
                                                                        
                                                                             
                                                                      
                                                                              
                                                                             
                                                                           
                                                                         
                                                                              
                                                                         
  
                                                                            
                                                                              
                                                                              
                                                                          
                                                                            
                                                                           
                                                                         
                                                                           
                                                                             
               
  
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
  
                                                                              
                                                                              
                                                                             
                                                                               
                                                                               
                                                                               
                                                                        
                 
  
                                                                              
                                                                           
                                                                            
                                                                         
                                                                               
                                                                            
                                                                               
                                                                               
                  
  
                        
  
                                                                             
                                                                           
                                                                           
                                                                       
                                                                         
                                            
  
                                                                          
                          
  
                                                                             
                             
  
                                                                         
                                                                              
                                                                          
                                                                            
                                                                              
                                                                         
                                                                             
                    
  
                                                                             
                                                                         
                                                                    
                                                                           
                                                                            
                                                                          
                                                                  
                                                                             
                                                                       
                                                                            
                 
  
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
  
                                                                      
                                                                  
  
                                                                              
                                                                 
                        
  
                                          
  
                                                                              
                                                     
  
                                                                       
                                                                        
                                                                        
          
  
                           
  
                                                                             
                                                                          
                                                                             
                                                                              
                                                                             
                                                                           
                                                                           
                                                                             
                                                                              
                                   
   
#define	DTRACEBUF_RING		0x0001		                             
#define	DTRACEBUF_FILL		0x0002		                             
#define	DTRACEBUF_NOSWITCH	0x0004		                          
#define	DTRACEBUF_WRAPPED	0x0008		                             
#define	DTRACEBUF_DROPPED	0x0010		                    
#define	DTRACEBUF_ERROR		0x0020		                     
#define	DTRACEBUF_FULL		0x0040		                           
#define	DTRACEBUF_CONSUMED	0x0080		                              
#define	DTRACEBUF_INACTIVE	0x0100		                              

typedef struct dtrace_buffer {
	uint64_t dtb_offset;			                              
	uint64_t dtb_cur_limit;			                                             
	uint64_t dtb_limit;			                            
	uint64_t dtb_size;			                    
	uint32_t dtb_flags;			           
	uint32_t dtb_drops;			                     
	caddr_t dtb_tomax;			                   
	caddr_t dtb_xamot;			                     
	uint32_t dtb_xamot_flags;		                    
	uint32_t dtb_xamot_drops;		                              
	uint64_t dtb_xamot_offset;		                               
	uint32_t dtb_errors;			                      
	uint32_t dtb_xamot_errors;		                               
#ifndef _LP64
	uint64_t dtb_pad1;
#endif
	uint64_t dtb_switched;			                         
	uint64_t dtb_interval;			                              
	uint64_t dtb_pad2[4];			                                
} dtrace_buffer_t;

  
                             
  
                                                                        
                                                                          
                                                                            
                                                                        
                                                                  
                                                                            
                                                                          
                                                                          
                                                                               
                                                                        
                                 
  
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
  
  
                                                                        
                                                                          
                                                                        
                                                                               
                
  
                                                                               
                                                                            
                                              
   
typedef struct dtrace_aggkey {
	uint32_t dtak_hashval;			                
	uint32_t dtak_action:4;			                      
	uint32_t dtak_size:28;			                     
	caddr_t dtak_data;			                  
	struct dtrace_aggkey *dtak_next;	                        
} dtrace_aggkey_t;

typedef struct dtrace_aggbuffer {
	uintptr_t dtagb_hashsize;		                       
	uintptr_t dtagb_free;			                       
	dtrace_aggkey_t **dtagb_hash;		                
} dtrace_aggbuffer_t;

  
                      
  
                                                                             
                                                                            
                                                                     
                                                                               
                                                                            
                                                                        
                                                                               
                                                                             
                                                                         
                                                                    
                                                                          
                                                             
  
                                                                    
                                                                               
                                                                   
                                                                             
                                                                    
                                                                      
                                                                       
  
                                              
  
                                                                   
                                                                   
                                                                   
                                                                   
                                                                   
                                                                                
                                                                              
                                                                   
                                                                   
                                                                       
                                                                       
                                                                       
                                                                   
                                                                   
                                                                               
                                                                               
                                                                       
                                                                   
                                                                   
                                                                   
                                                                   
                                                                   
                                                                   
                                                                       
                                                                       
                                                                       
   
typedef enum dtrace_speculation_state {
	DTRACESPEC_INACTIVE = 0,
	DTRACESPEC_ACTIVE,
	DTRACESPEC_ACTIVEONE,
	DTRACESPEC_ACTIVEMANY,
	DTRACESPEC_COMMITTING,
	DTRACESPEC_COMMITTINGMANY,
	DTRACESPEC_DISCARDING
} dtrace_speculation_state_t;

typedef struct dtrace_speculation {
	dtrace_speculation_state_t dtsp_state;	                               
	int dtsp_cleaning;			                               
	dtrace_buffer_t *dtsp_buffer;		                        
} dtrace_speculation_t;

  
                           
  
                                                                             
                                                                        
                                                                             
                                                                            
                                                                            
                                                                            
                                                                             
                                                                  
                                                                           
                                 
  
                                                                            
                                                                             
                                                                               
                                                                               
                                                                            
                                                                          
                                                                               
                                                                            
                                                                              
                                                                           
                                                                            
                                                                           
                                                                             
                                                                            
                                                                            
                                                                             
                                                              
  
                                                                              
                                                                      
                                                                       
                                   
  
                                                              
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
  
                                                                            
                                                                               
                                                
  
                                                    
  
                                                                          
                                                                       
                                                  
  
                                                                              
                 
  
                                                                             
                                                                               
                                                                            
                                                                               
                                                                              
                                                                        
                                                                               
                                                                              
                                                                             
                                                                             
                                                                           
                                                                           
                                                                             
                                                                        
                                                                               
                                                                          
                                                                              
                                
  
                                                                        
                                                                             
                                                                               
                                                                       
                                                                           
                                                                          
                                                                         
                                                                      
                                                                        
                                                                             
                                                                       
                                                                       
                                                                              
                                                                               
         
  
                                                                   
  
                                                                              
                                                                      
                                                                  
  
                                                                              
  
                                       
  
                                                                             
                                                                             
                                                                               
                
  
                                                                               
                                                                            
  
                                                                              
                            
  
                                                                            
                                                                              
                                                                              
                                                                            
                                                            
  
                                                                        
                                              
  
                                                                          
  
                                                                              
  
                                                                               
                                                                           
                                                                          
                                               
  
                                                                           
                                                                             
                                                                               
                                                                       
                                                                            
                                                                
  
                                                                             
                   
  
                                                              
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
  
                                                                               
                                                                              
                                                                             
                                                                        
                                                                             
                                                                          
                                                                           
                                                                               
                                                                             
                                                                           
                           
  
                                                
  
                                                              
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
                                                                             
  
                                                                          
                                                                     
                                                                          
                                                                               
                                                                               
                                                                              
                                                                             
                                               
   
typedef struct dtrace_key {
	uint64_t dttk_value;			                                
	uint64_t dttk_size;			                               
} dtrace_key_t;

typedef struct dtrace_tuple {
	uint32_t dtt_nkeys;			                             
	uint32_t dtt_pad;			             
	dtrace_key_t dtt_key[1];		                         
} dtrace_tuple_t;

typedef struct dtrace_dynvar {
	uint64_t dtdv_hashval;			                             
	struct dtrace_dynvar *dtdv_next;	                                
	void *dtdv_data;			                     
	dtrace_tuple_t dtdv_tuple;		               
} dtrace_dynvar_t;

typedef enum dtrace_dynvar_op {
	DTRACE_DYNVAR_ALLOC,
	DTRACE_DYNVAR_NOALLOC,
	DTRACE_DYNVAR_DEALLOC
} dtrace_dynvar_op_t;

typedef struct dtrace_dynhash {
	dtrace_dynvar_t *dtdh_chain;		                                
	uintptr_t dtdh_lock;			                       
#ifdef _LP64
	uintptr_t dtdh_pad[6];			                                
#else
	uintptr_t dtdh_pad[14];			                                
#endif
} dtrace_dynhash_t;

typedef struct dtrace_dstate_percpu {
	dtrace_dynvar_t *dtdsc_free;		                            
	dtrace_dynvar_t *dtdsc_dirty;		                             
	dtrace_dynvar_t *dtdsc_rinsing;		                               
	dtrace_dynvar_t *dtdsc_clean;		                             
	uint64_t dtdsc_drops;			                              
	uint64_t dtdsc_dirty_drops;		                           
	uint64_t dtdsc_rinsing_drops;		                             
#ifdef _LP64
	uint64_t dtdsc_pad;			                                
#else
	uint64_t dtdsc_pad[2];			                                
#endif
} dtrace_dstate_percpu_t;

typedef enum dtrace_dstate_state {
	DTRACE_DSTATE_CLEAN = 0,
	DTRACE_DSTATE_EMPTY,
	DTRACE_DSTATE_DIRTY,
	DTRACE_DSTATE_RINSING
} dtrace_dstate_state_t;

typedef struct dtrace_dstate {
	void *dtds_base;			                                
	size_t dtds_size;			                                
	size_t dtds_hashsize;			                               
	size_t dtds_chunksize;			                        
	dtrace_dynhash_t *dtds_hash;		                           
	dtrace_dstate_state_t dtds_state;	                                
	dtrace_dstate_percpu_t *dtds_percpu;	                             
} dtrace_dstate_t;

  
                        
  
                                                                               
                                                                            
                                                                             
                                                                           
                                                       
  
                                       
                                                                       
                                                                       
  
                                                                               
        
  
                                                                        
                                                                            
                                                                          
                                                                    
                                                                     
  
                                                                              
                                                                         
                                                                               
                                                     
  
                                                                               
                                                                               
                                                                       
                                                        
   
typedef struct dtrace_statvar {
	uint64_t dtsv_data;			                           
	size_t dtsv_size;			                             
	int dtsv_refcnt;			                     
	dtrace_difv_t dtsv_var;			                       
} dtrace_statvar_t;

typedef struct dtrace_vstate {
	dtrace_state_t *dtvs_state;		                           
	dtrace_statvar_t **dtvs_globals;	                                
	int dtvs_nglobals;			                       
	dtrace_difv_t *dtvs_tlocals;		                           
	int dtvs_ntlocals;			                             
	dtrace_statvar_t **dtvs_locals;		                       
	int dtvs_nlocals;			                             
	dtrace_dstate_t dtvs_dynvars;		                            
} dtrace_vstate_t;

  
                       
  
                                                                           
                                                                            
                                                                           
                                                                            
                                                                   
   
#define	DTRACE_MSTATE_ARGS		0x00000001
#define	DTRACE_MSTATE_PROBE		0x00000002
#define	DTRACE_MSTATE_EPID		0x00000004
#define	DTRACE_MSTATE_TIMESTAMP		0x00000008
#define	DTRACE_MSTATE_STACKDEPTH	0x00000010
#define	DTRACE_MSTATE_CALLER		0x00000020
#define	DTRACE_MSTATE_IPL		0x00000040
#define	DTRACE_MSTATE_FLTOFFS		0x00000080
#define	DTRACE_MSTATE_WALLTIMESTAMP	0x00000100
#define	DTRACE_MSTATE_USTACKDEPTH	0x00000200
#define	DTRACE_MSTATE_UCALLER		0x00000400
#define	DTRACE_MSTATE_MACHTIMESTAMP	0x00000800
#define	DTRACE_MSTATE_MACHCTIMESTAMP	0x00001000

typedef struct dtrace_mstate {
	uintptr_t dtms_scratch_base;		                           
	uintptr_t dtms_scratch_ptr;		                             
	size_t dtms_scratch_size;		                  
	uint32_t dtms_present;			                                
	uint64_t dtms_arg[5];			                      
	dtrace_epid_t dtms_epid;		                  
	uint64_t dtms_timestamp;		                      
	hrtime_t dtms_walltimestamp;		                           
	uint64_t dtms_machtimestamp;		                                    
	uint64_t dtms_machctimestamp;		                                      
	int dtms_stackdepth;			                       
	int dtms_ustackdepth;			                        
	struct dtrace_probe *dtms_probe;	                   
	uintptr_t dtms_caller;			                   
	uint64_t dtms_ucaller;			                              
	int dtms_ipl;				                              
	int dtms_fltoffs;			                          
	uintptr_t dtms_strtok;			                            
	uintptr_t dtms_strtok_limit;		                               
	uint32_t dtms_access;			                          
	dtrace_difo_t *dtms_difo;		                        
} dtrace_mstate_t;

#define	DTRACE_COND_OWNER	0x1
#define	DTRACE_COND_USERMODE	0x2
#define	DTRACE_COND_ZONEOWNER	0x4

#define	DTRACE_PROBEKEY_MAXDEPTH	8	                              

  
                                                 
   
#define	DTRACE_ACCESS_KERNEL	0x1		                           


  
                  
  
                                                                           
                                                                            
                                                                             
                                                                              
                                                                         
                                                                          
  
  
  
                                                                         
                                                                         
                                                                         
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                   
                                                                 
  
                                                                           
                                                                             
                           
   
typedef enum dtrace_activity {
	DTRACE_ACTIVITY_INACTIVE = 0,		                     
	DTRACE_ACTIVITY_WARMUP,			                    
	DTRACE_ACTIVITY_ACTIVE,			             
	DTRACE_ACTIVITY_DRAINING,		                     
	DTRACE_ACTIVITY_COOLDOWN,		                    
	DTRACE_ACTIVITY_STOPPED,		                    
	DTRACE_ACTIVITY_KILLED			            
} dtrace_activity_t;


  
                                               
  
                                         
  
                                                    
                                                        
                                                                       
                                                     
  
                                                                      
                                                              
  
                                                                             
                                                                               
                                                                             
                                                                         
  
                                                                        
                                                                            
                                                                          
                                                                          
                                   
   

#define DTRACE_DOF_MODE_NEVER 		0
#define DTRACE_DOF_MODE_LAZY_ON		1
#define DTRACE_DOF_MODE_LAZY_OFF	2
#define DTRACE_DOF_MODE_NON_LAZY	3

  
                                                                                
                                                                                 
                                                                               
                                                                                    
                                                                       
  
                                                                           
                                                                           
                                                                             
                                                                             
                                                                            
                                                                           
                                                                          
                                        
  
                                                                                 
  
                                                             
                                                                       
                                                                        
                                                                             
  
                                                                           
                                                                            
                                                                            
                                                                        
             
 
                                                                                  
                      
   

#define DTRACE_KERNEL_SYMBOLS_NEVER 			0
#define DTRACE_KERNEL_SYMBOLS_FROM_KERNEL		1
#define DTRACE_KERNEL_SYMBOLS_FROM_USERSPACE		2
#define DTRACE_KERNEL_SYMBOLS_ALWAYS_FROM_KERNEL	3
	

  
                               
  
                                                                           
                                                                         
                                                                              
                                                                               
                                                                               
                                                                         
                                                                           
                                                                              
                                                                              
                                                   
   
#define	DTRACE_HELPER_ACTION_USTACK	0
#define	DTRACE_NHELPER_ACTIONS		1

typedef struct dtrace_helper_action {
	int dtha_generation;			                              
	int dtha_nactions;			                       
	dtrace_difo_t *dtha_predicate;		                             
	dtrace_difo_t **dtha_actions;		                      
	struct dtrace_helper_action *dtha_next;	                        
} dtrace_helper_action_t;

typedef struct dtrace_helper_provider {
	int dthp_generation;			                                
	uint32_t dthp_ref;			                     
	dof_helper_t dthp_prov;			                                
} dtrace_helper_provider_t;

typedef struct dtrace_helpers {
	dtrace_helper_action_t **dthps_actions;	                             
	dtrace_vstate_t dthps_vstate;		                              
	dtrace_helper_provider_t **dthps_provs;	                        
	uint_t dthps_nprovs;			                        
	uint_t dthps_maxprovs;			                         
	int dthps_generation;			                        
	pid_t dthps_pid;			                            
	int dthps_deferred;			                             
	struct dtrace_helpers *dthps_next;	                  
	struct dtrace_helpers *dthps_prev;	                  
} dtrace_helpers_t;

  
                               
  
                                                                        
                                                                              
                                                                             
                                                                              
                                                                                
                                                                              
                                                                       
                                  
   
#define	DTRACE_HELPTRACE_NEXT	(-1)
#define	DTRACE_HELPTRACE_DONE	(-2)
#define	DTRACE_HELPTRACE_ERR	(-3)


typedef struct dtrace_helptrace {
	dtrace_helper_action_t	*dtht_helper;	                   
	int dtht_where;				                            
	int dtht_nlocals;			                      
	int dtht_fault;				                            
	int dtht_fltoffs;			                
	uint64_t dtht_illval;			                    
	uint64_t dtht_locals[1];		                     
} dtrace_helptrace_t;

  
                     
  
                                                                           
                                                                     
                                                                          
                                                                       
                                                                      
                     
   
#define	DTRACE_CRV_ALLPROC		0x01
#define	DTRACE_CRV_KERNEL		0x02
#define	DTRACE_CRV_ALLZONE		0x04

#define	DTRACE_CRV_ALL		(DTRACE_CRV_ALLPROC | DTRACE_CRV_KERNEL | \
	DTRACE_CRV_ALLZONE)

#define	DTRACE_CRA_PROC				0x0001
#define	DTRACE_CRA_PROC_CONTROL			0x0002
#define	DTRACE_CRA_PROC_DESTRUCTIVE_ALLUSER	0x0004
#define	DTRACE_CRA_PROC_DESTRUCTIVE_ALLZONE	0x0008
#define	DTRACE_CRA_PROC_DESTRUCTIVE_CREDCHG	0x0010
#define	DTRACE_CRA_KERNEL			0x0020
#define	DTRACE_CRA_KERNEL_DESTRUCTIVE		0x0040

#define	DTRACE_CRA_ALL		(DTRACE_CRA_PROC | \
	DTRACE_CRA_PROC_CONTROL | \
	DTRACE_CRA_PROC_DESTRUCTIVE_ALLUSER | \
	DTRACE_CRA_PROC_DESTRUCTIVE_ALLZONE | \
	DTRACE_CRA_PROC_DESTRUCTIVE_CREDCHG | \
	DTRACE_CRA_KERNEL | \
	DTRACE_CRA_KERNEL_DESTRUCTIVE)

typedef struct dtrace_cred {
	cred_t			*dcr_cred;
	uint8_t			dcr_destructive;
	uint8_t			dcr_visible;
	uint16_t		dcr_action;
} dtrace_cred_t;

typedef struct dtrace_format {
	uint64_t dtf_refcount;
	char dtf_str[];
} dtrace_format_t;

#define DTRACE_FORMAT_SIZE(fmt) (strlen(fmt->dtf_str) + 1 + sizeof(dtrace_format_t))

  
                        
  
                                                                              
                                                                               
                                                                       
                                                                             
                                                                            
                          
   
struct dtrace_state {
	dev_t dts_dev;				            
	int dts_necbs;				                          
	dtrace_ecb_t **dts_ecbs;		                   
	dtrace_epid_t dts_epid;			                           
	size_t dts_needed;			                           
	struct dtrace_state *dts_anon;		                             
	dtrace_activity_t dts_activity;		                      
	dtrace_vstate_t dts_vstate;		                    
	dtrace_buffer_t *dts_buffer;		                      
	dtrace_buffer_t *dts_aggbuffer;		                        
	dtrace_speculation_t *dts_speculations;	                       
	int dts_nspeculations;			                            
	int dts_naggregations;			                            
	dtrace_aggregation_t **dts_aggregations;                        
	vmem_t *dts_aggid_arena;		                               
	uint64_t dts_errors;			                            
	uint32_t dts_speculations_busy;		                          
	uint32_t dts_speculations_unavail;	                            
	uint32_t dts_stkstroverflows;		                                
	uint32_t dts_dblerrors;			                            
	uint32_t dts_reserve;			                            
	hrtime_t dts_laststatus;		                         
	cyclic_id_t dts_cleaner;		                     
	cyclic_id_t dts_deadman;		                    
	hrtime_t dts_alive;			                     
	char dts_speculates;			                               
	char dts_destructive;			                                
	int dts_nformats;			                       
	dtrace_format_t **dts_formats;		                         
	dtrace_optval_t dts_options[DTRACEOPT_MAX];              
	dtrace_cred_t dts_cred;			                 
	size_t dts_nretained;			                              
	uint64_t dts_arg_error_illval;
	uint32_t dts_buf_over_limit;		                                   
	uint64_t **dts_rstate;			                          
};

struct dtrace_provider {
	dtrace_pattr_t dtpv_attr;		                         
	dtrace_ppriv_t dtpv_priv;		                         
	dtrace_pops_t dtpv_pops;		                         
	char *dtpv_name;			                   
	void *dtpv_arg;				                       
	uint_t dtpv_defunct;			                               
	struct dtrace_provider *dtpv_next;	                   
	uint64_t dtpv_probe_count;		                                 
	uint64_t dtpv_ecb_count;		                                       
};

struct dtrace_meta {
	dtrace_mops_t dtm_mops;			                              
	char *dtm_name;				                        
	void *dtm_arg;				                            
	uint64_t dtm_count;			                                    
};

  
                   
  
                                                                   
                                                                          
                                                                           
                                                                      
                                                                          
                                                                         
                                                                           
                                                                            
                                                                         
               
   
typedef struct dtrace_enabling {
	dtrace_ecbdesc_t **dten_desc;		                          
	int dten_ndesc;				                                
	int dten_maxdesc;			                       
	dtrace_vstate_t *dten_vstate;		                               
	dtrace_genid_t dten_probegen;		                              
	dtrace_ecbdesc_t *dten_current;		                             
	int dten_error;				                         
	int dten_primed;			                            
	struct dtrace_enabling *dten_prev;	                       
	struct dtrace_enabling *dten_next;	                   
} dtrace_enabling_t;

  
                             
  
                                                                          
                                                                           
                                                                            
                                                                              
                                                                       
                                                                           
                    
   
typedef struct dtrace_anon {
	dtrace_state_t *dta_state;		                           
	dtrace_enabling_t *dta_enabling;	                         
	processorid_t dta_beganon;		                            
} dtrace_anon_t;

  
                         
   
#if DEBUG
#define	DTRACE_ERRDEBUG
#endif

#ifdef DTRACE_ERRDEBUG

typedef struct dtrace_errhash {
	const char	*dter_msg;	                   
	int		dter_count;	                          
} dtrace_errhash_t;

#define	DTRACE_ERRHASHSZ	256	                                  

#endif	                     

typedef struct dtrace_string dtrace_string_t;

typedef struct dtrace_string {
	dtrace_string_t *dtst_next;
	dtrace_string_t *dtst_prev;
	uint32_t dtst_refcount;
	char dtst_str[];
} dtrace_string_t;

   
                                 
  
                                                                      
                                                 
   
typedef struct dtrace_match_cond {
	int (*dmc_func)(dtrace_probedesc_t*, void*);
	void *dmc_data;
} dtrace_match_cond_t;


  
                      
  
                                                                             
                                                                     
                                                                              
                                                                            
                                                                             
                                                                              
                                                                             
                                                                              
                                                                            
                                                                          
                                                                         
             
   
typedef struct dtrace_toxrange {
	uintptr_t	dtt_base;		                         
	uintptr_t	dtt_limit;		                          
} dtrace_toxrange_t;

extern uint64_t dtrace_getarg(int, int, dtrace_mstate_t*, dtrace_vstate_t*);
extern int dtrace_getipl(void);
extern uintptr_t dtrace_caller(int);
extern uint32_t dtrace_cas32(uint32_t *, uint32_t, uint32_t);
extern void *dtrace_casptr(void *, void *, void *);
extern void dtrace_copyin(user_addr_t, uintptr_t, size_t, volatile uint16_t *);
extern void dtrace_copyinstr(user_addr_t, uintptr_t, size_t, volatile uint16_t *);
extern void dtrace_copyout(uintptr_t, user_addr_t, size_t, volatile uint16_t *);
extern void dtrace_copyoutstr(uintptr_t, user_addr_t, size_t, volatile uint16_t *);
extern void dtrace_getpcstack(pc_t *, int, int, uint32_t *);
extern uint64_t dtrace_load64(uintptr_t);
extern int dtrace_canload(uint64_t, size_t, dtrace_mstate_t*, dtrace_vstate_t*);

extern uint64_t dtrace_getreg(struct regs *, uint_t);
extern uint64_t dtrace_getvmreg(uint_t);
extern int dtrace_getstackdepth(int);
extern void dtrace_getupcstack(uint64_t *, int);
extern void dtrace_getufpstack(uint64_t *, uint64_t *, int);
extern int dtrace_getustackdepth(void);
extern uintptr_t dtrace_fulword(void *);
extern uint8_t dtrace_fuword8(user_addr_t);
extern uint16_t dtrace_fuword16(user_addr_t);
extern uint32_t dtrace_fuword32(user_addr_t);
extern uint64_t dtrace_fuword64(user_addr_t);
extern int dtrace_proc_waitfor(dtrace_procdesc_t*);
extern void dtrace_probe_error(dtrace_state_t *, dtrace_epid_t, int, int,
    int, uint64_t);
extern int dtrace_assfail(const char *, const char *, int);
extern int dtrace_attached(void);
extern hrtime_t dtrace_gethrestime(void);

extern void dtrace_flush_caches(void);

extern void dtrace_copy(uintptr_t, uintptr_t, size_t);
extern void dtrace_copystr(uintptr_t, uintptr_t, size_t, volatile uint16_t *);

extern void* dtrace_ptrauth_strip(void*, uint64_t);
extern int dtrace_is_valid_ptrauth_key(uint64_t);

extern uint64_t dtrace_physmem_read(uint64_t, size_t);
extern void dtrace_physmem_write(uint64_t, uint64_t, size_t);

  
                        
   
extern minor_t dtrace_state_reserve(void);
extern dtrace_state_t* dtrace_state_allocate(minor_t minor);
extern dtrace_state_t* dtrace_state_get(minor_t minor);
extern void dtrace_state_free(minor_t minor);

  
                            
   
extern void dtrace_restriction_policy_load(void);
extern boolean_t dtrace_is_restricted(void);
extern boolean_t dtrace_are_restrictions_relaxed(void);
extern boolean_t dtrace_fbt_probes_restricted(void);
extern boolean_t dtrace_sdt_probes_restricted(void);
extern boolean_t dtrace_can_attach_to_proc(proc_t);

  
                    
  
                                                                              
                                                                              
                                                                              
                                                                             
                                                                            
                                                                           
                                                                               
                                          
   
#undef ASSERT
#undef VERIFY

#define	VERIFY(EX)	((void)((EX) || \
			dtrace_assfail(#EX, __FILE__, __LINE__)))

#if DEBUG
#define	ASSERT(EX)	((void)((EX) || \
			dtrace_assfail(#EX, __FILE__, __LINE__)))
#else
#define	ASSERT(X)	((void)0)
#endif

#ifdef	__cplusplus
}
#endif

#endif                         

