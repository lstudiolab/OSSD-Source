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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_FASTTRAP_IMPL_H
#define	_FASTTRAP_IMPL_H

#include <sys/types.h>
#include <sys/dtrace.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/fasttrap.h>
#include <sys/fasttrap_isa.h>

                                                                       
#define proc_t struct proc                                                   

#ifdef	__cplusplus
extern "C" {
#endif

  
                                             
  
                                                                          
                                                                     
                                                                            
                                                               
                                                                          
                                                                        
                                                                             
                                                              
  
                                                                         
                                                                           
                                                                          
                                                                         
                                                                          
                                                                           
                                                                             
                                                                            
                                                      
  
                                                                         
                                                                          
                                                                          
                                                                       
                                                                      
                                                                        
                      
   

  
                                                              
   

typedef struct fasttrap_proc {
	pid_t ftpc_pid;				                              
	uint64_t ftpc_acount;			                               
	uint64_t ftpc_rcount;			                               
	lck_mtx_t ftpc_mtx;			                            
	struct fasttrap_proc *ftpc_next;	                             
} fasttrap_proc_t;

typedef struct fasttrap_provider {
	pid_t ftp_pid;				                              
	fasttrap_provider_type_t ftp_provider_type;	                                                      
	char ftp_name[DTRACE_PROVNAMELEN];	                             
	dtrace_provider_id_t ftp_provid;	                            
	uint_t ftp_marked;			                               
	uint_t ftp_retired;			                       
	lck_mtx_t ftp_mtx;			                   
	lck_mtx_t ftp_cmtx;			                             
	uint64_t ftp_pcount;			                              
	uint64_t ftp_rcount;			                              
	uint64_t ftp_ccount;			                               
	uint64_t ftp_mcount;			                         
	fasttrap_proc_t *ftp_proc;		                               
	struct fasttrap_provider *ftp_next;	                             
} fasttrap_provider_t;

typedef struct fasttrap_id fasttrap_id_t;
typedef struct fasttrap_probe fasttrap_probe_t;
typedef struct fasttrap_tracepoint fasttrap_tracepoint_t;

struct fasttrap_id {
	fasttrap_probe_t *fti_probe;		                      
	fasttrap_id_t *fti_next;		                              
	fasttrap_probe_type_t fti_ptype;	                
};

typedef struct fasttrap_id_tp {
	fasttrap_id_t fit_id;
	fasttrap_tracepoint_t *fit_tp;
} fasttrap_id_tp_t;

struct fasttrap_probe {
	dtrace_id_t ftp_id;			                             
	pid_t ftp_pid;				                        
	fasttrap_provider_t *ftp_prov;		                           
	user_addr_t ftp_faddr;			                                
	size_t ftp_fsize;			                                
	uint64_t ftp_gen;			                             
	uint64_t ftp_ntps;			                           
	uint8_t *ftp_argmap;			                               
	uint8_t ftp_nargs;			                               
	uint8_t ftp_enabled;			                           
	uint8_t ftp_triggered;
	char *ftp_xtypes;			                            
	char *ftp_ntypes;			                        
	fasttrap_id_tp_t ftp_tps[1];		                    
};

#define	FASTTRAP_ID_INDEX(id)	\
((fasttrap_id_tp_t *)(((char *)(id) - offsetof(fasttrap_id_tp_t, fit_id))) - \
&(id)->fti_probe->ftp_tps[0])

struct fasttrap_tracepoint {
	fasttrap_proc_t *ftt_proc;		                               
	user_addr_t ftt_pc;			                           
	pid_t ftt_pid;				                       
	fasttrap_machtp_t ftt_mtp;		                          
	fasttrap_id_t *ftt_ids;			                          
	fasttrap_id_t *ftt_retids;		                          
	fasttrap_tracepoint_t *ftt_next;	                         
};

typedef struct fasttrap_bucket {
	lck_mtx_t ftb_mtx;			                 
	void *ftb_data;				                  

	uint8_t ftb_pad[64 - sizeof (lck_mtx_t) - sizeof (void *)];
} fasttrap_bucket_t;

typedef struct fasttrap_hash {
	ulong_t fth_nent;			                                
	ulong_t fth_mask;			                  
	fasttrap_bucket_t *fth_table;		                      
} fasttrap_hash_t;

  
                                                                        
                                                                          
                                                                       
   
#define	fasttrap_copyout	copyout
#define	fasttrap_fuword32	fuword32
#define	fasttrap_suword32	suword32

  
                                                                
                                 
   
#define	fasttrap_fuword64	fuword64
#define	fasttrap_suword64	suword64
#define fasttrap_fuword64_noerr	fuword64_noerr
#define fasttrap_fuword32_noerr	fuword32_noerr

extern void fasttrap_sigtrap(proc_t *, uthread_t, user_addr_t);

extern dtrace_id_t 		fasttrap_probe_id;
extern fasttrap_hash_t		fasttrap_tpoints;

#define	FASTTRAP_TPOINTS_INDEX(pid, pc) \
	(((pc) / sizeof (fasttrap_instr_t) + (pid)) & fasttrap_tpoints.fth_mask)

extern void fasttrap_tracepoint_retire(proc_t *p, fasttrap_tracepoint_t *tp);

  
                                        
   
extern int fasttrap_tracepoint_init(proc_t *, fasttrap_tracepoint_t *,
    user_addr_t, fasttrap_probe_type_t);
extern int fasttrap_tracepoint_install(proc_t *, fasttrap_tracepoint_t *);
extern int fasttrap_tracepoint_remove(proc_t *, fasttrap_tracepoint_t *);

#if defined(__x86_64__)
extern int fasttrap_pid_probe(x86_saved_state_t *regs);
extern int fasttrap_return_probe(x86_saved_state_t* regs);
#elif defined(__arm__) || defined(__arm64__)
extern int fasttrap_pid_probe(arm_saved_state_t *rp);
extern int fasttrap_return_probe(arm_saved_state_t *regs);
#else
#error architecture not supported
#endif

extern uint64_t fasttrap_pid_getarg(void *, dtrace_id_t, void *, int, int);
extern uint64_t fasttrap_usdt_getarg(void *, dtrace_id_t, void *, int, int);


#ifdef	__cplusplus
}
#endif

#undef proc_t

#endif	                      
