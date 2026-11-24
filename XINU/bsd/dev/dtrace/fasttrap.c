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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <sys/codesign.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/kauth.h>
#include <sys/utfconv.h>

#include <sys/fasttrap.h>
#include <sys/fasttrap_impl.h>
#include <sys/fasttrap_isa.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <sys/proc.h>

#include <security/mac_framework.h>

#include <miscfs/devfs/devfs.h>
#include <sys/proc_internal.h>
#include <sys/dtrace_glue.h>
#include <sys/dtrace_ptss.h>

#include <kern/cs_blobs.h>
#include <kern/thread.h>
#include <kern/zalloc.h>

#include <mach/thread_act.h>

extern kern_return_t kernel_thread_start_priority(thread_continue_t continuation, void *parameter, integer_t priority, thread_t *new_thread);

                                                                       
#define proc_t struct proc                                                   

__private_extern__
void
qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *));

  
                               
                               
  
                                                                             
                                                                 
                                                                        
                                                                          
                                                    
  
  
                          
                          
  
                                                                       
                                                                          
                                                                             
                                                                             
                                                                            
                                                                            
                                                                   
             
  
                                                                       
                                                                             
                                                                             
                                                                           
                                                                      
                                                                         
                                                                             
                                                                          
                                                                           
                                                                          
                     
  
                                                                            
                                                                          
                                                                        
                                                                           
                                                                       
                                                         
  
                                                                             
                                                                          
                                                                    
                                                                    
              
  
  
                
                
  
                                                                            
                                                                            
                                                                            
                                                                      
                                                                             
                                                                              
                                                                             
                                                                            
                                                        
  
                                                                         
                                                                            
                                                                            
                                                                           
                                                                              
                                                                          
                 
  
           
                                    
                              
                              
                                                                
   

static dtrace_meta_provider_id_t fasttrap_meta_id;

static thread_t fasttrap_cleanup_thread;

static LCK_GRP_DECLARE(fasttrap_lck_grp, "fasttrap");
static LCK_ATTR_DECLARE(fasttrap_lck_attr, 0, 0);
static LCK_MTX_DECLARE_ATTR(fasttrap_cleanup_mtx,
    &fasttrap_lck_grp, &fasttrap_lck_attr);


#define FASTTRAP_CLEANUP_PROVIDER 0x1
#define FASTTRAP_CLEANUP_TRACEPOINT 0x2

static uint32_t fasttrap_cleanup_work = 0;

  
                                                                           
   
static volatile uint64_t fasttrap_mod_gen;

  
                                                                             
                                                                          
                                                                            
                                                   
   

static uint32_t fasttrap_max;
static uint32_t fasttrap_retired;
static uint32_t fasttrap_total;


#define	FASTTRAP_TPOINTS_DEFAULT_SIZE	0x4000
#define	FASTTRAP_PROVIDERS_DEFAULT_SIZE	0x100
#define	FASTTRAP_PROCS_DEFAULT_SIZE	0x100

fasttrap_hash_t			fasttrap_tpoints;
static fasttrap_hash_t		fasttrap_provs;
static fasttrap_hash_t		fasttrap_procs;

static uint64_t			fasttrap_pid_count;	                   
static LCK_MTX_DECLARE_ATTR(fasttrap_count_mtx,	                       
    &fasttrap_lck_grp, &fasttrap_lck_attr);

#define	FASTTRAP_ENABLE_FAIL	1
#define	FASTTRAP_ENABLE_PARTIAL	2

static int fasttrap_tracepoint_enable(proc_t *, fasttrap_probe_t *, uint_t);
static void fasttrap_tracepoint_disable(proc_t *, fasttrap_probe_t *, uint_t);

static fasttrap_provider_t *fasttrap_provider_lookup(proc_t*, fasttrap_provider_type_t, const char *,
    const dtrace_pattr_t *);
static void fasttrap_provider_retire(proc_t*, const char *, int);
static void fasttrap_provider_free(fasttrap_provider_t *);

static fasttrap_proc_t *fasttrap_proc_lookup(pid_t);
static void fasttrap_proc_release(fasttrap_proc_t *);

#define	FASTTRAP_PROVS_INDEX(pid, name) \
	((fasttrap_hash_str(name) + (pid)) & fasttrap_provs.fth_mask)

#define	FASTTRAP_PROCS_INDEX(pid) ((pid) & fasttrap_procs.fth_mask)

  
                                                                       
                                                                  
                                                             
                                                          
   

ZONE_DECLARE(fasttrap_tracepoint_t_zone, "dtrace.fasttrap_tracepoint_t",
    sizeof(fasttrap_tracepoint_t), ZC_NONE);

  
                                                                                      
                                                                          
                                                            
   
#define FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS 4

struct zone *fasttrap_probe_t_zones[FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS];

static const char *fasttrap_probe_t_zone_names[FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS] = {
	"",
	"dtrace.fasttrap_probe_t[1]",
	"dtrace.fasttrap_probe_t[2]",
	"dtrace.fasttrap_probe_t[3]"
};

static int
fasttrap_highbit(ulong_t i)
{
	int h = 1;

	if (i == 0)
		return (0);
#ifdef _LP64
	if (i & 0xffffffff00000000ul) {
		h += 32; i >>= 32;
	}
#endif
	if (i & 0xffff0000) {
		h += 16; i >>= 16;
	}
	if (i & 0xff00) {
		h += 8; i >>= 8;
	}
	if (i & 0xf0) {
		h += 4; i >>= 4;
	}
	if (i & 0xc) {
		h += 2; i >>= 2;
	}
	if (i & 0x2) {
		h += 1;
	}
	return (h);
}

static uint_t
fasttrap_hash_str(const char *p)
{
	unsigned int g;
	uint_t hval = 0;

	while (*p) {
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}
	return (hval);
}

  
                                               
   
void
fasttrap_sigtrap(proc_t *p, uthread_t t, user_addr_t pc)
{
#pragma unused(p, t, pc)

#if !defined(__APPLE__)
	sigqueue_t *sqp = kmem_zalloc(sizeof (sigqueue_t), KM_SLEEP);

	sqp->sq_info.si_signo = SIGTRAP;
	sqp->sq_info.si_code = TRAP_DTRACE;
	sqp->sq_info.si_addr = (caddr_t)pc;

	mutex_enter(&p->p_lock);
	sigaddqa(p, t, sqp);
	mutex_exit(&p->p_lock);

	if (t != NULL)
		aston(t);
#endif                

	printf("fasttrap_sigtrap called with no implementation.\n");
}

  
                                                                      
                                                  
   
static void
fasttrap_mod_barrier(uint64_t gen)
{
	unsigned int i;

	if (gen < fasttrap_mod_gen)
		return;

	fasttrap_mod_gen++;

	for (i = 0; i < NCPU; i++) {
		lck_mtx_lock(&cpu_core[i].cpuc_pid_lock);
		lck_mtx_unlock(&cpu_core[i].cpuc_pid_lock);
	}
}

static void fasttrap_pid_cleanup(uint32_t);

static unsigned int
fasttrap_pid_cleanup_providers(void)
{
	fasttrap_provider_t **fpp, *fp;
	fasttrap_bucket_t *bucket;
	dtrace_provider_id_t provid;
	unsigned int later = 0, i;

	  
                                                              
                                                          
                                                              
             
    
	for (i = 0; i < fasttrap_provs.fth_nent; i++) {
		bucket = &fasttrap_provs.fth_table[i];
		lck_mtx_lock(&bucket->ftb_mtx);
		fpp = (fasttrap_provider_t **)&bucket->ftb_data;

		while ((fp = *fpp) != NULL) {
			if (!fp->ftp_marked) {
				fpp = &fp->ftp_next;
				continue;
			}

			lck_mtx_lock(&fp->ftp_mtx);

			  
                                             
                                               
                                                
                       
      
			if (fp->ftp_ccount != 0 ||
			    fp->ftp_mcount != 0) {
				fp->ftp_marked = 0;
				lck_mtx_unlock(&fp->ftp_mtx);
				continue;
			}

			if (!fp->ftp_retired || fp->ftp_rcount != 0)
				fp->ftp_marked = 0;

			lck_mtx_unlock(&fp->ftp_mtx);

			  
                                        
                                             
                                               
                                               
                                               
                                              
                                           
                                             
                                     
      
			provid = fp->ftp_provid;
			if (dtrace_unregister(provid) != 0) {
				if (fasttrap_total > fasttrap_max / 2)
					(void) dtrace_condense(provid);
				later += fp->ftp_marked;
				fpp = &fp->ftp_next;
			} else {
				*fpp = fp->ftp_next;
				fasttrap_provider_free(fp);
			}
		}
		lck_mtx_unlock(&bucket->ftb_mtx);
	}

	return later;
}

typedef struct fasttrap_tracepoint_spec {
	pid_t fttps_pid;
	user_addr_t fttps_pc;
} fasttrap_tracepoint_spec_t;

static fasttrap_tracepoint_spec_t *fasttrap_retired_spec;
static size_t fasttrap_cur_retired = 0, fasttrap_retired_size;
static LCK_MTX_DECLARE_ATTR(fasttrap_retired_mtx,
    &fasttrap_lck_grp, &fasttrap_lck_attr);

#define DEFAULT_RETIRED_SIZE 256

static void
fasttrap_tracepoint_cleanup(void)
{
	size_t i;
	pid_t pid = 0;
	user_addr_t pc;
	proc_t *p = PROC_NULL;
	fasttrap_tracepoint_t *tp = NULL;
	lck_mtx_lock(&fasttrap_retired_mtx);
	fasttrap_bucket_t *bucket;
	for (i = 0; i < fasttrap_cur_retired; i++) {
		pc = fasttrap_retired_spec[i].fttps_pc;
		if (fasttrap_retired_spec[i].fttps_pid != pid) {
			pid = fasttrap_retired_spec[i].fttps_pid;
			if (p != PROC_NULL) {
				sprunlock(p);
			}
			if ((p = sprlock(pid)) == PROC_NULL) {
				pid = 0;
				continue;
			}
		}
		bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];
		lck_mtx_lock(&bucket->ftb_mtx);
		for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
			if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
			tp->ftt_proc->ftpc_acount != 0)
				break;
		}
		  
                                                          
                                   
     
		if (tp == NULL || tp->ftt_retired == 0) {
			lck_mtx_unlock(&bucket->ftb_mtx);
			continue;
		}
		fasttrap_tracepoint_remove(p, tp);
		lck_mtx_unlock(&bucket->ftb_mtx);
	}
	if (p != PROC_NULL) {
		sprunlock(p);
	}

	fasttrap_cur_retired = 0;

	lck_mtx_unlock(&fasttrap_retired_mtx);
}

void
fasttrap_tracepoint_retire(proc_t *p, fasttrap_tracepoint_t *tp)
{
	if (tp->ftt_retired)
		return;
	lck_mtx_lock(&fasttrap_retired_mtx);
	fasttrap_tracepoint_spec_t *s = &fasttrap_retired_spec[fasttrap_cur_retired++];
	s->fttps_pid = p->p_pid;
	s->fttps_pc = tp->ftt_pc;

	if (fasttrap_cur_retired == fasttrap_retired_size) {
		fasttrap_tracepoint_spec_t *new_retired = kmem_zalloc(
					fasttrap_retired_size * 2 *
					sizeof(*fasttrap_retired_spec),
					KM_SLEEP);
		memcpy(new_retired, fasttrap_retired_spec, sizeof(*fasttrap_retired_spec) * fasttrap_retired_size);
		kmem_free(fasttrap_retired_spec, sizeof(*fasttrap_retired_spec) * fasttrap_retired_size);
		fasttrap_retired_size *= 2;
		fasttrap_retired_spec = new_retired;
	}

	lck_mtx_unlock(&fasttrap_retired_mtx);

	tp->ftt_retired = 1;

	fasttrap_pid_cleanup(FASTTRAP_CLEANUP_TRACEPOINT);
}

static void
fasttrap_pid_cleanup_compute_priority(void)
{
	if (fasttrap_total > (fasttrap_max / 100 * 90) || fasttrap_retired > fasttrap_max / 2) {
		thread_precedence_policy_data_t precedence = {12                           };
		thread_policy_set(fasttrap_cleanup_thread, THREAD_PRECEDENCE_POLICY, (thread_policy_t) &precedence, THREAD_PRECEDENCE_POLICY_COUNT);
	}
	else {
		thread_precedence_policy_data_t precedence = {-39                             };
		thread_policy_set(fasttrap_cleanup_thread, THREAD_PRECEDENCE_POLICY, (thread_policy_t) &precedence, THREAD_PRECEDENCE_POLICY_COUNT);

	}
}

  
                                                                         
          
   
            
__attribute__((noreturn))
static void
fasttrap_pid_cleanup_cb(void)
{
	uint32_t work = 0;
	lck_mtx_lock(&fasttrap_cleanup_mtx);
	msleep(&fasttrap_pid_cleanup_cb, &fasttrap_cleanup_mtx, PRIBIO, "fasttrap_pid_cleanup_cb", NULL);
	while (1) {
		unsigned int later = 0;

		work = os_atomic_xchg(&fasttrap_cleanup_work, 0, relaxed);
		lck_mtx_unlock(&fasttrap_cleanup_mtx);
		if (work & FASTTRAP_CLEANUP_PROVIDER) {
			later = fasttrap_pid_cleanup_providers();
		}
		if (work & FASTTRAP_CLEANUP_TRACEPOINT) {
			fasttrap_tracepoint_cleanup();
		}
		lck_mtx_lock(&fasttrap_cleanup_mtx);

		fasttrap_pid_cleanup_compute_priority();
		if (!fasttrap_cleanup_work) {
			  
                                                                     
                                                                       
                                                                      
                                                                        
                                                                     
                                                                   
                                                                       
                        
      
			if (later > 0) {
				struct timespec t = {.tv_sec = 1, .tv_nsec = 0};
				msleep(&fasttrap_pid_cleanup_cb, &fasttrap_cleanup_mtx, PRIBIO, "fasttrap_pid_cleanup_cb", &t);
			}
			else
				msleep(&fasttrap_pid_cleanup_cb, &fasttrap_cleanup_mtx, PRIBIO, "fasttrap_pid_cleanup_cb", NULL);
		}
	}

}

  
                                                
   
static void
fasttrap_pid_cleanup(uint32_t work)
{
	lck_mtx_lock(&fasttrap_cleanup_mtx);
	os_atomic_or(&fasttrap_cleanup_work, work, relaxed);
	fasttrap_pid_cleanup_compute_priority();
	wakeup(&fasttrap_pid_cleanup_cb);
	lck_mtx_unlock(&fasttrap_cleanup_mtx);
}

static int
fasttrap_setdebug(proc_t *p)
{
	LCK_MTX_ASSERT(&p->p_mlock, LCK_MTX_ASSERT_OWNED);

	  
                                                                   
                                                             
                                       
    
	if ((p->p_csflags & (CS_KILL|CS_HARD))) {
		proc_unlock(p);
		for (int i = 0; i < DTRACE_NCLIENTS; i++) {
			dtrace_state_t *state = dtrace_state_get(i);
			if (state == NULL)
				continue;
			if (state->dts_cred.dcr_cred == NULL)
				continue;
			  
                                                        
                                                  
                                                          
                                     
      

			  
                                                         
                                                          
                                                     
                                                        
                
      
			struct proc_ident pident = proc_ident(p);
			sprunlock(p);
			p = PROC_NULL;

			(void) mac_proc_check_get_task(state->dts_cred.dcr_cred, &pident, TASK_FLAVOR_CONTROL);

			p = sprlock(pident.p_pid);
			if (p == PROC_NULL) {
				return (ESRCH);
			}
		}
		int rc = cs_allow_invalid(p);
		proc_lock(p);
		if (rc == 0) {
			return (EACCES);
		}
	}
	return (0);
}

  
                                                                    
                                                                           
                                                                             
          
   
static void
fasttrap_fork(proc_t *p, proc_t *cp)
{
	pid_t ppid = p->p_pid;
	unsigned int i;

	ASSERT(current_proc() == p);
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);
	ASSERT(p->p_dtrace_count > 0);
	ASSERT(cp->p_dtrace_count == 0);

	  
                                                                 
                                                                      
                                                             
                                                               
                                                              
                                                                  
                                                        
    

	  
                                                               
                            
    
	if (cp != sprlock(cp->p_pid)) {
		printf("fasttrap_fork: sprlock(%d) returned a different proc\n", cp->p_pid);
		return;
	}

	proc_lock(cp);
	if (fasttrap_setdebug(cp) == ESRCH) {
		printf("fasttrap_fork: failed to re-acquire proc\n");
		return;
	}
	proc_unlock(cp);

	  
                                                                     
                                                           
    
	for (i = 0; i < fasttrap_tpoints.fth_nent; i++) {
		fasttrap_tracepoint_t *tp;
		fasttrap_bucket_t *bucket = &fasttrap_tpoints.fth_table[i];

		lck_mtx_lock(&bucket->ftb_mtx);
		for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
			if (tp->ftt_pid == ppid &&
			    tp->ftt_proc->ftpc_acount != 0) {
				fasttrap_tracepoint_remove(cp, tp);

				  
                                                
                                              
                                                 
                                             
                
       
				 ASSERT(tp->ftt_proc->ftpc_acount != 0);
			}
		}
		lck_mtx_unlock(&bucket->ftb_mtx);
	}

	  
                                             
    
	dtrace_ptss_fork(p, cp);

	sprunlock(cp);
}

  
                                                                           
                                                                        
                                
   
static void
fasttrap_exec_exit(proc_t *p)
{
	ASSERT(p == current_proc());
	LCK_MTX_ASSERT(&p->p_mlock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_NOTOWNED);


	                                                                  
                                                                     
                                                                      
                                                      
   
                                                                 
                                   
   
                                             
    
	proc_unlock(p);

	  
                                                                 
                                                                      
    
	fasttrap_provider_retire(p, FASTTRAP_PID_NAME, 0);

	  
                                                             
                                                                 
                              
    
	fasttrap_provider_retire(p, FASTTRAP_OBJC_NAME, 0);
	fasttrap_provider_retire(p, FASTTRAP_ONESHOT_NAME, 0);

	  
                                                                   
                                                                     
    
	lck_mtx_lock(&p->p_dtrace_sprlock);
	dtrace_ptss_exec_exit(p);
	lck_mtx_unlock(&p->p_dtrace_sprlock);

	proc_lock(p);
}


            
static void
fasttrap_pid_provide(void *arg, const dtrace_probedesc_t *desc)
{
#pragma unused(arg, desc)
	  
                                      
    
}

static int
fasttrap_tracepoint_enable(proc_t *p, fasttrap_probe_t *probe, uint_t index)
{
	fasttrap_tracepoint_t *tp, *new_tp = NULL;
	fasttrap_bucket_t *bucket;
	fasttrap_id_t *id;
	pid_t pid;
	user_addr_t pc;

	ASSERT(index < probe->ftp_ntps);

	pid = probe->ftp_pid;
	pc = probe->ftp_tps[index].fit_tp->ftt_pc;
	id = &probe->ftp_tps[index].fit_id;

	ASSERT(probe->ftp_tps[index].fit_tp->ftt_pid == pid);

	                                

	  
                                                                       
                                                            
    
	fasttrap_mod_barrier(probe->ftp_gen);

	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	  
                                                                      
                                                                  
                                                                      
                                                                    
                                                
   
                                                                     
    
again:
	lck_mtx_lock(&bucket->ftb_mtx);
	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		int rc = 0;
		  
                                                          
                                                                
                                                             
               
     
		if (tp->ftt_pid != pid || tp->ftt_pc != pc ||
		    tp->ftt_proc->ftpc_acount == 0)
			continue;

		  
                                                            
                                                          
                                                              
                                                      
     
		if (!tp->ftt_installed) {
			if (fasttrap_tracepoint_install(p, tp) != 0)
				rc = FASTTRAP_ENABLE_PARTIAL;
		}
		  
                                                            
                                                        
                                                            
                                                          
                                                    
     
		ASSERT(tp->ftt_ids != NULL || tp->ftt_retids != NULL);

		switch (id->fti_ptype) {
		case DTFTP_ENTRY:
		case DTFTP_OFFSETS:
		case DTFTP_IS_ENABLED:
			id->fti_next = tp->ftt_ids;
			dtrace_membar_producer();
			tp->ftt_ids = id;
			dtrace_membar_producer();
			break;

		case DTFTP_RETURN:
		case DTFTP_POST_OFFSETS:
			id->fti_next = tp->ftt_retids;
			dtrace_membar_producer();
			tp->ftt_retids = id;
			dtrace_membar_producer();
			break;

		default:
			ASSERT(0);
		}

		tp->ftt_retired = 0;

		lck_mtx_unlock(&bucket->ftb_mtx);

		if (new_tp != NULL) {
			new_tp->ftt_ids = NULL;
			new_tp->ftt_retids = NULL;
		}

		return rc;
	}

	  
                                                                  
                                                       
    
	if (new_tp != NULL) {
		int rc = 0;

		new_tp->ftt_next = bucket->ftb_data;
		dtrace_membar_producer();
		bucket->ftb_data = new_tp;
		dtrace_membar_producer();
		lck_mtx_unlock(&bucket->ftb_mtx);

		  
                                                        
                                                      
                                                         
                                              
     
		if (fasttrap_tracepoint_install(p, new_tp) != 0)
			rc = FASTTRAP_ENABLE_PARTIAL;
		  
                                                               
                        
     
		                                     
		p->p_dtrace_count++;


		return (rc);
	}

	lck_mtx_unlock(&bucket->ftb_mtx);

	  
                                                                      
    
	new_tp = probe->ftp_tps[index].fit_tp;
	new_tp->ftt_retired = 0;

	ASSERT(new_tp->ftt_pid == pid);
	ASSERT(new_tp->ftt_pc == pc);
	ASSERT(new_tp->ftt_proc == probe->ftp_prov->ftp_proc);
	ASSERT(new_tp->ftt_ids == NULL);
	ASSERT(new_tp->ftt_retids == NULL);

	switch (id->fti_ptype) {
	case DTFTP_ENTRY:
	case DTFTP_OFFSETS:
	case DTFTP_IS_ENABLED:
		id->fti_next = NULL;
		new_tp->ftt_ids = id;
		break;

	case DTFTP_RETURN:
	case DTFTP_POST_OFFSETS:
		id->fti_next = NULL;
		new_tp->ftt_retids = id;
		break;

	default:
		ASSERT(0);
	}

	  
                                                                    
                                                              
    
	if (fasttrap_tracepoint_init(p, new_tp, pc, id->fti_ptype) == 0)
		goto again;

	new_tp->ftt_ids = NULL;
	new_tp->ftt_retids = NULL;

	return (FASTTRAP_ENABLE_FAIL);
}

static void
fasttrap_tracepoint_disable(proc_t *p, fasttrap_probe_t *probe, uint_t index)
{
	fasttrap_bucket_t *bucket;
	fasttrap_provider_t *provider = probe->ftp_prov;
	fasttrap_tracepoint_t **pp, *tp;
	fasttrap_id_t *id, **idp;
	pid_t pid;
	user_addr_t pc;

	ASSERT(index < probe->ftp_ntps);

	pid = probe->ftp_pid;
	pc = probe->ftp_tps[index].fit_tp->ftt_pc;
	id = &probe->ftp_tps[index].fit_id;

	ASSERT(probe->ftp_tps[index].fit_tp->ftt_pid == pid);

	  
                                                               
                            
    
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];
	lck_mtx_lock(&bucket->ftb_mtx);
	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (tp->ftt_pid == pid && tp->ftt_pc == pc &&
		    tp->ftt_proc == provider->ftp_proc)
			break;
	}

	  
                                                                 
    
	ASSERT(tp != NULL);

	switch (id->fti_ptype) {
		case DTFTP_ENTRY:
		case DTFTP_OFFSETS:
		case DTFTP_IS_ENABLED:
			ASSERT(tp->ftt_ids != NULL);
			idp = &tp->ftt_ids;
			break;

		case DTFTP_RETURN:
		case DTFTP_POST_OFFSETS:
			ASSERT(tp->ftt_retids != NULL);
			idp = &tp->ftt_retids;
			break;

		default:
			                             
			idp = NULL;
			ASSERT(0);
	}

	while ((*idp)->fti_probe != probe) {
		idp = &(*idp)->fti_next;
		ASSERT(*idp != NULL);
	}

	id = *idp;
	*idp = id->fti_next;
	dtrace_membar_producer();

	ASSERT(id->fti_probe == probe);

	  
                                                                     
                                                                
                                                   
    
	if (tp->ftt_ids != NULL || tp->ftt_retids != NULL) {

		  
                                                         
                              
     
		if (tp == probe->ftp_tps[index].fit_tp) {
			fasttrap_probe_t *tmp_probe;
			fasttrap_tracepoint_t **tmp_tp;
			uint_t tmp_index;

			if (tp->ftt_ids != NULL) {
				tmp_probe = tp->ftt_ids->fti_probe;
				                        
				tmp_index = FASTTRAP_ID_INDEX(tp->ftt_ids);
				tmp_tp = &tmp_probe->ftp_tps[tmp_index].fit_tp;
			} else {
				tmp_probe = tp->ftt_retids->fti_probe;
				                        
				tmp_index = FASTTRAP_ID_INDEX(tp->ftt_retids);
				tmp_tp = &tmp_probe->ftp_tps[tmp_index].fit_tp;
			}

			ASSERT(*tmp_tp != NULL);
			ASSERT(*tmp_tp != probe->ftp_tps[index].fit_tp);
			ASSERT((*tmp_tp)->ftt_ids == NULL);
			ASSERT((*tmp_tp)->ftt_retids == NULL);

			probe->ftp_tps[index].fit_tp = *tmp_tp;
			*tmp_tp = tp;

		}

		lck_mtx_unlock(&bucket->ftb_mtx);

		  
                                                               
             
     
		probe->ftp_gen = fasttrap_mod_gen;
		return;
	}

	lck_mtx_unlock(&bucket->ftb_mtx);

	  
                                                                
                                                                     
                                                             
                                                                       
                                                                   
                                                                     
         
    

	  
                                                                  
                          
    
	if (p != NULL) {
		  
                                                          
                                                          
           
     
		if (fasttrap_tracepoint_remove(p, tp) != 0)
			fasttrap_sigtrap(p, NULL, pc);

		  
                                                            
                           
     
		                                     
		p->p_dtrace_count--;
	}

	  
                                                               
    
	lck_mtx_lock(&bucket->ftb_mtx);
	pp = (fasttrap_tracepoint_t **)&bucket->ftb_data;
	ASSERT(*pp != NULL);
	while (*pp != tp) {
		pp = &(*pp)->ftt_next;
		ASSERT(*pp != NULL);
	}

	*pp = tp->ftt_next;
	dtrace_membar_producer();

	lck_mtx_unlock(&bucket->ftb_mtx);

	  
                                                                       
    
	probe->ftp_gen = fasttrap_mod_gen;
}

static void
fasttrap_enable_callbacks(void)
{
	  
                                                             
                                                            
                                                            
                         
    
	lck_mtx_lock(&fasttrap_count_mtx);
	if (fasttrap_pid_count == 0) {
		ASSERT(dtrace_pid_probe_ptr == NULL);
		ASSERT(dtrace_return_probe_ptr == NULL);
		dtrace_pid_probe_ptr = &fasttrap_pid_probe;
		dtrace_return_probe_ptr = &fasttrap_return_probe;
	}
	ASSERT(dtrace_pid_probe_ptr == &fasttrap_pid_probe);
	ASSERT(dtrace_return_probe_ptr == &fasttrap_return_probe);
	fasttrap_pid_count++;
	lck_mtx_unlock(&fasttrap_count_mtx);
}

static void
fasttrap_disable_callbacks(void)
{
	                                

	lck_mtx_lock(&fasttrap_count_mtx);
	ASSERT(fasttrap_pid_count > 0);
	fasttrap_pid_count--;
	if (fasttrap_pid_count == 0) {
		dtrace_cpu_t *cur, *cpu = CPU;

		  
                                                             
                                                                 
                                                               
                                                                 
                                                                 
     
		for (cur = cpu->cpu_next; cur != cpu; cur = cur->cpu_next) {
			lck_rw_lock_exclusive(&cur->cpu_ft_lock);
			                                          
		}

		dtrace_pid_probe_ptr = NULL;
		dtrace_return_probe_ptr = NULL;

		for (cur = cpu->cpu_next; cur != cpu; cur = cur->cpu_next) {
			lck_rw_unlock_exclusive(&cur->cpu_ft_lock);
			                              
		}
	}
	lck_mtx_unlock(&fasttrap_count_mtx);
}

            
static int
fasttrap_pid_enable(void *arg, dtrace_id_t id, void *parg)
{
#pragma unused(arg, id)
	fasttrap_probe_t *probe = parg;
	proc_t *p;
	int i, rc;

	ASSERT(probe != NULL);
	ASSERT(!probe->ftp_enabled);
	ASSERT(id == probe->ftp_id);
	                                 

	  
                                                                   
                                                               
                                                                 
               
    
	lck_mtx_lock(&probe->ftp_prov->ftp_mtx);
	probe->ftp_prov->ftp_rcount++;
	lck_mtx_unlock(&probe->ftp_prov->ftp_mtx);

	  
                                                                  
                                                                        
                                                         
    
	if (probe->ftp_prov->ftp_retired)
	    return(0);

	  
                                                                        
                                                                      
                                                        
    
	if ((p = sprlock(probe->ftp_pid)) == PROC_NULL) {
		  
                                                                   
                                                                  
                       
     
	    return(0);
	}

	proc_lock(p);
	int p_pid = proc_pid(p);

	rc = fasttrap_setdebug(p);
	switch (rc) {
	case EACCES:
		proc_unlock(p);
		sprunlock(p);
		cmn_err(CE_WARN, "Failed to install fasttrap probe for pid %d: "
		    "Process does not allow invalid code pages\n", p_pid);
		return (0);
	case ESRCH:
		cmn_err(CE_WARN, "Failed to install fasttrap probe for pid %d: "
		    "Failed to re-acquire process\n", p_pid);
		return (0);
	default:
		assert(rc == 0);
		break;
	}

	  
                                                                         
                                                                                 
                                                                            
                                                                              
                                                                             
                                                              
    
	if (p->p_dtrace_ptss_pages == NULL) {
		dtrace_ptss_enable(p);
	}

	                                 
	proc_unlock(p);

	  
                                                                       
                                                                   
                            
    
	fasttrap_enable_callbacks();

	  
                                                              
                                       
    
	for (i = 0; i < (int)probe->ftp_ntps; i++) {
		if ((rc = fasttrap_tracepoint_enable(p, probe, i)) != 0) {
			  
                                                   
                                                 
                                          
      
			if (rc == FASTTRAP_ENABLE_FAIL)
				i--;
			else
				ASSERT(rc == FASTTRAP_ENABLE_PARTIAL);

			  
                                                    
                                    
      
			while (i >= 0) {
				fasttrap_tracepoint_disable(p, probe, i);
				i--;
			}

			sprunlock(p);

			  
                                                   
                                                 
      
			fasttrap_disable_callbacks();
			return(0);
		}
	}

	sprunlock(p);

	probe->ftp_enabled = 1;
	return (0);
}

            
static void
fasttrap_pid_disable(void *arg, dtrace_id_t id, void *parg)
{
#pragma unused(arg, id)
	fasttrap_probe_t *probe = parg;
	fasttrap_provider_t *provider = probe->ftp_prov;
	proc_t *p;
	int i, whack = 0;

	ASSERT(id == probe->ftp_id);

	  
                                                                 
                                                                  
                                                                 
                                               
    
	if ((p = sprlock(probe->ftp_pid)) != PROC_NULL) {
		                                 
	}

	lck_mtx_lock(&provider->ftp_mtx);

	  
                                                                      
    
	if (probe->ftp_enabled) {
		for (i = 0; i < (int)probe->ftp_ntps; i++) {
			fasttrap_tracepoint_disable(p, probe, i);
		}
	}

	ASSERT(provider->ftp_rcount > 0);
	provider->ftp_rcount--;

	if (p != NULL) {
		  
                                                             
                                                              
                              
     
		if (provider->ftp_retired && !provider->ftp_marked)
			whack = provider->ftp_marked = 1;
		lck_mtx_unlock(&provider->ftp_mtx);

		sprunlock(p);
	} else {
		  
                                                       
                                                     
     
		if (provider->ftp_rcount == 0 && !provider->ftp_marked)
			whack = provider->ftp_marked = 1;
		lck_mtx_unlock(&provider->ftp_mtx);
	}

	if (whack) {
		fasttrap_pid_cleanup(FASTTRAP_CLEANUP_PROVIDER);
	}

	if (!probe->ftp_enabled)
		return;

	probe->ftp_enabled = 0;

	                                 
	fasttrap_disable_callbacks();
}

            
static void
fasttrap_pid_getargdesc(void *arg, dtrace_id_t id, void *parg,
    dtrace_argdesc_t *desc)
{
#pragma unused(arg, id)
	fasttrap_probe_t *probe = parg;
	char *str;
	int i, ndx;

	desc->dtargd_native[0] = '\0';
	desc->dtargd_xlate[0] = '\0';

	if (probe->ftp_prov->ftp_retired != 0 ||
	    desc->dtargd_ndx >= probe->ftp_nargs) {
		desc->dtargd_ndx = DTRACE_ARGNONE;
		return;
	}

	ndx = (probe->ftp_argmap != NULL) ?
		probe->ftp_argmap[desc->dtargd_ndx] : desc->dtargd_ndx;

	str = probe->ftp_ntypes;
	for (i = 0; i < ndx; i++) {
		str += strlen(str) + 1;
	}

	(void) strlcpy(desc->dtargd_native, str, sizeof(desc->dtargd_native));

	if (probe->ftp_xtypes == NULL)
		return;

	str = probe->ftp_xtypes;
	for (i = 0; i < desc->dtargd_ndx; i++) {
		str += strlen(str) + 1;
	}

	(void) strlcpy(desc->dtargd_xlate, str, sizeof(desc->dtargd_xlate));
}

            
static void
fasttrap_pid_destroy(void *arg, dtrace_id_t id, void *parg)
{
#pragma unused(arg, id)
	fasttrap_probe_t *probe = parg;
	unsigned int i;

	ASSERT(probe != NULL);
	ASSERT(!probe->ftp_enabled);
	ASSERT(fasttrap_total >= probe->ftp_ntps);

	os_atomic_sub(&fasttrap_total, probe->ftp_ntps, relaxed);
	os_atomic_sub(&fasttrap_retired, probe->ftp_ntps, relaxed);

	if (probe->ftp_gen + 1 >= fasttrap_mod_gen)
		fasttrap_mod_barrier(probe->ftp_gen);

	for (i = 0; i < probe->ftp_ntps; i++) {
		zfree(fasttrap_tracepoint_t_zone, probe->ftp_tps[i].fit_tp);
	}

	if (probe->ftp_ntps < FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS) {
		zfree(fasttrap_probe_t_zones[probe->ftp_ntps], probe);
	} else {
		size_t size = offsetof(fasttrap_probe_t, ftp_tps[probe->ftp_ntps]);
		kmem_free(probe, size);
	}
}


static const dtrace_pattr_t pid_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
};

static dtrace_pops_t pid_pops = {
	.dtps_provide =		fasttrap_pid_provide,
	.dtps_provide_module =	NULL,
	.dtps_enable =		fasttrap_pid_enable,
	.dtps_disable =		fasttrap_pid_disable,
	.dtps_suspend =		NULL,
	.dtps_resume =		NULL,
	.dtps_getargdesc =	fasttrap_pid_getargdesc,
	.dtps_getargval =	fasttrap_pid_getarg,
	.dtps_usermode =	NULL,
	.dtps_destroy =		fasttrap_pid_destroy
};

static dtrace_pops_t usdt_pops = {
	.dtps_provide =		fasttrap_pid_provide,
	.dtps_provide_module =	NULL,
	.dtps_enable =		fasttrap_pid_enable,
	.dtps_disable =		fasttrap_pid_disable,
	.dtps_suspend =		NULL,
	.dtps_resume =		NULL,
	.dtps_getargdesc =	fasttrap_pid_getargdesc,
	.dtps_getargval =	fasttrap_usdt_getarg,
	.dtps_usermode =	NULL,
	.dtps_destroy =		fasttrap_pid_destroy
};

static fasttrap_proc_t *
fasttrap_proc_lookup(pid_t pid)
{
	fasttrap_bucket_t *bucket;
	fasttrap_proc_t *fprc, *new_fprc;

	bucket = &fasttrap_procs.fth_table[FASTTRAP_PROCS_INDEX(pid)];
	lck_mtx_lock(&bucket->ftb_mtx);

	for (fprc = bucket->ftb_data; fprc != NULL; fprc = fprc->ftpc_next) {
		if (fprc->ftpc_pid == pid && fprc->ftpc_acount != 0) {
			lck_mtx_lock(&fprc->ftpc_mtx);
			lck_mtx_unlock(&bucket->ftb_mtx);
			fprc->ftpc_rcount++;
			os_atomic_inc(&fprc->ftpc_acount, relaxed);
			ASSERT(fprc->ftpc_acount <= fprc->ftpc_rcount);
			lck_mtx_unlock(&fprc->ftpc_mtx);

			return (fprc);
		}
	}

	  
                                                              
                        
    
	lck_mtx_unlock(&bucket->ftb_mtx);

	new_fprc = kmem_zalloc(sizeof (fasttrap_proc_t), KM_SLEEP);
	ASSERT(new_fprc != NULL);
	new_fprc->ftpc_pid = pid;
	new_fprc->ftpc_rcount = 1;
	new_fprc->ftpc_acount = 1;

	lck_mtx_lock(&bucket->ftb_mtx);

	  
                                                                
                                                                     
    
	for (fprc = bucket->ftb_data; fprc != NULL; fprc = fprc->ftpc_next) {
		if (fprc->ftpc_pid == pid && fprc->ftpc_acount != 0) {
			lck_mtx_lock(&fprc->ftpc_mtx);
			lck_mtx_unlock(&bucket->ftb_mtx);
			fprc->ftpc_rcount++;
			os_atomic_inc(&fprc->ftpc_acount, relaxed);
			ASSERT(fprc->ftpc_acount <= fprc->ftpc_rcount);
			lck_mtx_unlock(&fprc->ftpc_mtx);

			kmem_free(new_fprc, sizeof (fasttrap_proc_t));

			return (fprc);
		}
	}

	  
                                                          
    
	lck_mtx_init(&new_fprc->ftpc_mtx, &fasttrap_lck_grp, &fasttrap_lck_attr);

	new_fprc->ftpc_next = bucket->ftb_data;
	bucket->ftb_data = new_fprc;

	lck_mtx_unlock(&bucket->ftb_mtx);

	return (new_fprc);
}

static void
fasttrap_proc_release(fasttrap_proc_t *proc)
{
	fasttrap_bucket_t *bucket;
	fasttrap_proc_t *fprc, **fprcp;
	pid_t pid = proc->ftpc_pid;

	lck_mtx_lock(&proc->ftpc_mtx);

	ASSERT(proc->ftpc_rcount != 0);
	ASSERT(proc->ftpc_acount <= proc->ftpc_rcount);

	if (--proc->ftpc_rcount != 0) {
		lck_mtx_unlock(&proc->ftpc_mtx);
		return;
	}

	lck_mtx_unlock(&proc->ftpc_mtx);

	  
                                                                     
                          
    
	 ASSERT(proc->ftpc_acount == 0);

	bucket = &fasttrap_procs.fth_table[FASTTRAP_PROCS_INDEX(pid)];
	lck_mtx_lock(&bucket->ftb_mtx);

	fprcp = (fasttrap_proc_t **)&bucket->ftb_data;
	while ((fprc = *fprcp) != NULL) {
		if (fprc == proc)
			break;

		fprcp = &fprc->ftpc_next;
	}

	  
                                                             
    
	ASSERT(fprc != NULL);

	*fprcp = fprc->ftpc_next;

	lck_mtx_unlock(&bucket->ftb_mtx);

	  
                                                                            
                                                                       
    
	lck_mtx_destroy(&fprc->ftpc_mtx, &fasttrap_lck_grp);

	kmem_free(fprc, sizeof (fasttrap_proc_t));
}

  
                                                                            
                                                                     
                                                                             
                                                                          
                      
   
static fasttrap_provider_t *
fasttrap_provider_lookup(proc_t *p, fasttrap_provider_type_t provider_type, const char *name,
    const dtrace_pattr_t *pattr)
{
	pid_t pid = p->p_pid;
	fasttrap_provider_t *fp, *new_fp = NULL;
	fasttrap_bucket_t *bucket;
	char provname[DTRACE_PROVNAMELEN];
	cred_t *cred;

	ASSERT(strlen(name) < sizeof (fp->ftp_name));
	ASSERT(pattr != NULL);

	bucket = &fasttrap_provs.fth_table[FASTTRAP_PROVS_INDEX(pid, name)];
	lck_mtx_lock(&bucket->ftb_mtx);

	  
                                                                   
    
	for (fp = bucket->ftb_data; fp != NULL; fp = fp->ftp_next) {
		if (fp->ftp_pid == pid &&
		    fp->ftp_provider_type == provider_type &&
		    strncmp(fp->ftp_name, name, sizeof(fp->ftp_name)) == 0 &&
		    !fp->ftp_retired) {
			lck_mtx_lock(&fp->ftp_mtx);
			lck_mtx_unlock(&bucket->ftb_mtx);
			return (fp);
		}
	}

	  
                                                              
                        
    
	lck_mtx_unlock(&bucket->ftb_mtx);

	  
                                                             
                                                           
    
	proc_lock(p);
	if (p->p_lflag & (P_LINVFORK | P_LEXIT)) {
		proc_unlock(p);
		return (NULL);
	}

	  
                                                                    
                                                                    
                                       
    
	p->p_dtrace_probes++;

	  
                                                    
                                           
                                                 
                                                 
    
	cred = kauth_cred_proc_ref(p);
	proc_unlock(p);

	new_fp = kmem_zalloc(sizeof (fasttrap_provider_t), KM_SLEEP);
	ASSERT(new_fp != NULL);
	new_fp->ftp_pid = p->p_pid;
	new_fp->ftp_proc = fasttrap_proc_lookup(pid);
	new_fp->ftp_provider_type = provider_type;

	  
                                            
    
	lck_mtx_init(&new_fp->ftp_mtx, &fasttrap_lck_grp, &fasttrap_lck_attr);
	lck_mtx_init(&new_fp->ftp_cmtx, &fasttrap_lck_grp, &fasttrap_lck_attr);

	ASSERT(new_fp->ftp_proc != NULL);

	lck_mtx_lock(&bucket->ftb_mtx);

	  
                                                                    
                                                                     
    
	for (fp = bucket->ftb_data; fp != NULL; fp = fp->ftp_next) {
		if (fp->ftp_pid == pid && strncmp(fp->ftp_name, name, sizeof(fp->ftp_name)) == 0 &&
		    !fp->ftp_retired) {
			lck_mtx_lock(&fp->ftp_mtx);
			lck_mtx_unlock(&bucket->ftb_mtx);
			fasttrap_provider_free(new_fp);
			kauth_cred_unref(&cred);
			return (fp);
		}
	}

	(void) strlcpy(new_fp->ftp_name, name, sizeof(new_fp->ftp_name));

	  
                                                                
                                                            
                                                                 
                                                                  
              
    
	if (snprintf(provname, sizeof (provname), "%s%u", name, (uint_t)pid) >=
	    (int)sizeof (provname) ||
	    dtrace_register(provname, pattr,
	    DTRACE_PRIV_PROC | DTRACE_PRIV_OWNER | DTRACE_PRIV_ZONEOWNER, cred,
	    pattr == &pid_attr ? &pid_pops : &usdt_pops, new_fp,
	    &new_fp->ftp_provid) != 0) {
		lck_mtx_unlock(&bucket->ftb_mtx);
		fasttrap_provider_free(new_fp);
		kauth_cred_unref(&cred);
		return (NULL);
	}

	new_fp->ftp_next = bucket->ftb_data;
	bucket->ftb_data = new_fp;

	lck_mtx_lock(&new_fp->ftp_mtx);
	lck_mtx_unlock(&bucket->ftb_mtx);

	kauth_cred_unref(&cred);

	return (new_fp);
}

static void
fasttrap_provider_free(fasttrap_provider_t *provider)
{
	pid_t pid = provider->ftp_pid;
	proc_t *p;

	  
                                                               
                                                                     
    
	ASSERT(provider->ftp_rcount == 0);
	ASSERT(provider->ftp_ccount == 0);
	ASSERT(provider->ftp_mcount == 0);

	  
                                                                        
                                                                  
    
	if (!provider->ftp_retired) {
		os_atomic_dec(&provider->ftp_proc->ftpc_acount, relaxed);
		ASSERT(provider->ftp_proc->ftpc_acount <
		provider->ftp_proc->ftpc_rcount);
	}

	fasttrap_proc_release(provider->ftp_proc);

	  
                                                                             
                                                                       
    
	lck_mtx_destroy(&provider->ftp_mtx, &fasttrap_lck_grp);
	lck_mtx_destroy(&provider->ftp_cmtx, &fasttrap_lck_grp);

	kmem_free(provider, sizeof (fasttrap_provider_t));

	  
                                                                 
                                                                  
                                                              
                                                                 
                                                       
    
	if ((p = proc_find(pid)) == NULL) {
		return;
	}

	proc_lock(p);
	p->p_dtrace_probes--;
	proc_unlock(p);

	proc_rele(p);
}

static void
fasttrap_provider_retire(proc_t *p, const char *name, int mprov)
{
	fasttrap_provider_t *fp;
	fasttrap_bucket_t *bucket;
	dtrace_provider_id_t provid;
	ASSERT(strlen(name) < sizeof (fp->ftp_name));

	bucket = &fasttrap_provs.fth_table[FASTTRAP_PROVS_INDEX(p->p_pid, name)];
	lck_mtx_lock(&bucket->ftb_mtx);

	for (fp = bucket->ftb_data; fp != NULL; fp = fp->ftp_next) {
		if (fp->ftp_pid == p->p_pid && strncmp(fp->ftp_name, name, sizeof(fp->ftp_name)) == 0 &&
		    !fp->ftp_retired)
			break;
	}

	if (fp == NULL) {
		lck_mtx_unlock(&bucket->ftb_mtx);
		return;
	}

	lck_mtx_lock(&fp->ftp_mtx);
	ASSERT(!mprov || fp->ftp_mcount > 0);
	if (mprov && --fp->ftp_mcount != 0)  {
		lck_mtx_unlock(&fp->ftp_mtx);
		lck_mtx_unlock(&bucket->ftb_mtx);
		return;
	}

	  
                                                                        
                                                                        
                                                                       
                                                                    
                                                                    
                                                                   
   
                                                                      
                                                                
                                                                 
                                                                     
                                                                     
          
    
	os_atomic_dec(&fp->ftp_proc->ftpc_acount, relaxed);
	ASSERT(fp->ftp_proc->ftpc_acount < fp->ftp_proc->ftpc_rcount);

	  
                                                     
                                     
    
	os_atomic_add(&fasttrap_retired, fp->ftp_pcount, relaxed);
	fp->ftp_pcount = 0;

	fp->ftp_retired = 1;
	fp->ftp_marked = 1;
	provid = fp->ftp_provid;
	lck_mtx_unlock(&fp->ftp_mtx);

	  
                                                                     
                                                                    
                           
    
	dtrace_invalidate(provid);

	lck_mtx_unlock(&bucket->ftb_mtx);

	fasttrap_pid_cleanup(FASTTRAP_CLEANUP_PROVIDER);
}

static int
fasttrap_uint32_cmp(const void *ap, const void *bp)
{
	return (*(const uint32_t *)ap - *(const uint32_t *)bp);
}

static int
fasttrap_uint64_cmp(const void *ap, const void *bp)
{
	return (*(const uint64_t *)ap - *(const uint64_t *)bp);
}

static int
fasttrap_add_probe(fasttrap_probe_spec_t *pdata)
{
	proc_t *p;
	fasttrap_provider_t *provider;
	fasttrap_probe_t *pp;
	fasttrap_tracepoint_t *tp;
	const char *name;
	unsigned int i, aframes, whack;

	  
                                                       
    
	 if (pdata->ftps_noffs == 0)
		return (EINVAL);

	switch (pdata->ftps_probe_type) {
	case DTFTP_ENTRY:
		name = "entry";
		aframes = FASTTRAP_ENTRY_AFRAMES;
		break;
	case DTFTP_RETURN:
		name = "return";
		aframes = FASTTRAP_RETURN_AFRAMES;
		break;
	case DTFTP_OFFSETS:
		aframes = 0;
		name = NULL;
		break;
	default:
		return (EINVAL);
	}

	const char* provider_name;
	switch (pdata->ftps_provider_type) {
		case DTFTP_PROVIDER_PID:
			provider_name = FASTTRAP_PID_NAME;
			break;
		case DTFTP_PROVIDER_OBJC:
			provider_name = FASTTRAP_OBJC_NAME;
			break;
		case DTFTP_PROVIDER_ONESHOT:
			provider_name = FASTTRAP_ONESHOT_NAME;
			break;
		default:
			return (EINVAL);
	}

	p = proc_find(pdata->ftps_pid);
	if (p == PROC_NULL)
		return (ESRCH);

	if ((provider = fasttrap_provider_lookup(p, pdata->ftps_provider_type,
						 provider_name, &pid_attr)) == NULL) {
		proc_rele(p);
		return (ESRCH);
	}

	proc_rele(p);
	  
                                                                 
                                                                   
                                                                   
                                                            
    
	provider->ftp_ccount++;
	lck_mtx_unlock(&provider->ftp_mtx);

	  
                                                                 
                                                                  
                                                                 
                                                                  
                     
    
	lck_mtx_lock(&provider->ftp_cmtx);

	if (name == NULL) {
		for (i = 0; i < pdata->ftps_noffs; i++) {
			char name_str[17];

			(void) snprintf(name_str, sizeof(name_str), "%llx",
			    (uint64_t)pdata->ftps_offs[i]);

			if (dtrace_probe_lookup(provider->ftp_provid,
			    pdata->ftps_mod, pdata->ftps_func, name_str) != 0)
				continue;

			os_atomic_inc(&fasttrap_total, relaxed);
			if (fasttrap_total > fasttrap_max) {
				os_atomic_dec(&fasttrap_total, relaxed);
				goto no_mem;
			}
			provider->ftp_pcount++;

			pp = zalloc(fasttrap_probe_t_zones[1]);
			bzero(pp, sizeof (fasttrap_probe_t));

			pp->ftp_prov = provider;
			pp->ftp_faddr = pdata->ftps_pc;
			pp->ftp_fsize = pdata->ftps_size;
			pp->ftp_pid = pdata->ftps_pid;
			pp->ftp_ntps = 1;

			tp = zalloc(fasttrap_tracepoint_t_zone);
			bzero(tp, sizeof (fasttrap_tracepoint_t));

			tp->ftt_proc = provider->ftp_proc;
			tp->ftt_pc = pdata->ftps_offs[i] + pdata->ftps_pc;
			tp->ftt_pid = pdata->ftps_pid;

#if defined(__arm__) || defined(__arm64__)
			  
                                                           
                                                           
                                                      
                      
      
			tp->ftt_fntype = pdata->ftps_arch_subinfo;
#endif

			pp->ftp_tps[0].fit_tp = tp;
			pp->ftp_tps[0].fit_id.fti_probe = pp;
			pp->ftp_tps[0].fit_id.fti_ptype = pdata->ftps_probe_type;
			pp->ftp_id = dtrace_probe_create(provider->ftp_provid,
			    pdata->ftps_mod, pdata->ftps_func, name_str,
			    FASTTRAP_OFFSET_AFRAMES, pp);
		}

	} else if (dtrace_probe_lookup(provider->ftp_provid, pdata->ftps_mod,
	    pdata->ftps_func, name) == 0) {
		os_atomic_add(&fasttrap_total, pdata->ftps_noffs, relaxed);

		if (fasttrap_total > fasttrap_max) {
			os_atomic_sub(&fasttrap_total, pdata->ftps_noffs, relaxed);
			goto no_mem;
		}

		  
                                                                
                                                               
                    
     
		qsort(pdata->ftps_offs, pdata->ftps_noffs,
			sizeof (uint64_t), fasttrap_uint64_cmp);
		for (i = 1; i < pdata->ftps_noffs; i++) {
			if (pdata->ftps_offs[i] > pdata->ftps_offs[i - 1])
				continue;

			os_atomic_sub(&fasttrap_total, pdata->ftps_noffs, relaxed);
			goto no_mem;
		}
		provider->ftp_pcount += pdata->ftps_noffs;
		ASSERT(pdata->ftps_noffs > 0);
		if (pdata->ftps_noffs < FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS) {
			pp = zalloc(fasttrap_probe_t_zones[pdata->ftps_noffs]);
			bzero(pp, offsetof(fasttrap_probe_t, ftp_tps[pdata->ftps_noffs]));
		} else {
			pp = kmem_zalloc(offsetof(fasttrap_probe_t, ftp_tps[pdata->ftps_noffs]), KM_SLEEP);
		}

		pp->ftp_prov = provider;
		pp->ftp_faddr = pdata->ftps_pc;
		pp->ftp_fsize = pdata->ftps_size;
		pp->ftp_pid = pdata->ftps_pid;
		pp->ftp_ntps = pdata->ftps_noffs;

		for (i = 0; i < pdata->ftps_noffs; i++) {
			tp = zalloc(fasttrap_tracepoint_t_zone);
			bzero(tp, sizeof (fasttrap_tracepoint_t));
			tp->ftt_proc = provider->ftp_proc;
			tp->ftt_pc = pdata->ftps_offs[i] + pdata->ftps_pc;
			tp->ftt_pid = pdata->ftps_pid;

#if defined(__arm__) || defined (__arm64__)
			  
                                                           
                                                           
                                                      
                      
      

			tp->ftt_fntype = pdata->ftps_arch_subinfo;
#endif
			pp->ftp_tps[i].fit_tp = tp;
			pp->ftp_tps[i].fit_id.fti_probe = pp;
			pp->ftp_tps[i].fit_id.fti_ptype = pdata->ftps_probe_type;
		}

		pp->ftp_id = dtrace_probe_create(provider->ftp_provid,
		    pdata->ftps_mod, pdata->ftps_func, name, aframes, pp);
	}

	lck_mtx_unlock(&provider->ftp_cmtx);

	  
                                                                     
                                                                        
                                                                      
                                                           
    
	lck_mtx_lock(&provider->ftp_mtx);
	provider->ftp_ccount--;
	whack = provider->ftp_retired;
	lck_mtx_unlock(&provider->ftp_mtx);

	if (whack)
		fasttrap_pid_cleanup(FASTTRAP_CLEANUP_PROVIDER);

	return (0);

no_mem:
	  
                                                                   
                                                                  
                                                               
                              
    
	lck_mtx_unlock(&provider->ftp_cmtx);
	lck_mtx_lock(&provider->ftp_mtx);
	provider->ftp_ccount--;
	provider->ftp_marked = 1;
	lck_mtx_unlock(&provider->ftp_mtx);

	fasttrap_pid_cleanup(FASTTRAP_CLEANUP_PROVIDER);

	return (ENOMEM);
}

            
static void *
fasttrap_meta_provide(void *arg, dtrace_helper_provdesc_t *dhpv, proc_t *p)
{
#pragma unused(arg)
	fasttrap_provider_t *provider;

	  
                                                             
                                                                 
                                            
    
	if (strlen(dhpv->dthpv_provname) + 10 >=
	    sizeof (provider->ftp_name)) {
		cmn_err(CE_WARN, "failed to instantiate provider %s: "
		    "name too long to accomodate pid", dhpv->dthpv_provname);
		return (NULL);
	}

	  
                                                
    
	if (strncmp(dhpv->dthpv_provname, FASTTRAP_PID_NAME, sizeof(FASTTRAP_PID_NAME)) == 0) {
		cmn_err(CE_WARN, "failed to instantiate provider %s: "
		    "%s is an invalid name", dhpv->dthpv_provname,
		    FASTTRAP_PID_NAME);
		return (NULL);
	}

	  
                                                                             
    
	if (strncmp(dhpv->dthpv_provname, FASTTRAP_OBJC_NAME, sizeof(FASTTRAP_OBJC_NAME)) == 0) {
		cmn_err(CE_WARN, "failed to instantiate provider %s: "
		    "%s is an invalid name", dhpv->dthpv_provname,
		    FASTTRAP_OBJC_NAME);
		return (NULL);
	}
	if (strncmp(dhpv->dthpv_provname, FASTTRAP_ONESHOT_NAME, sizeof(FASTTRAP_ONESHOT_NAME)) == 0) {
		cmn_err(CE_WARN, "failed to instantiate provider %s: "
		    "%s is an invalid name", dhpv->dthpv_provname,
		    FASTTRAP_ONESHOT_NAME);
		return (NULL);
	}

	  
                                                                  
                                                  
    
	if (dhpv->dthpv_pattr.dtpa_provider.dtat_class > DTRACE_CLASS_ISA)
		dhpv->dthpv_pattr.dtpa_provider.dtat_class = DTRACE_CLASS_ISA;
	if (dhpv->dthpv_pattr.dtpa_mod.dtat_class > DTRACE_CLASS_ISA)
		dhpv->dthpv_pattr.dtpa_mod.dtat_class = DTRACE_CLASS_ISA;
	if (dhpv->dthpv_pattr.dtpa_func.dtat_class > DTRACE_CLASS_ISA)
		dhpv->dthpv_pattr.dtpa_func.dtat_class = DTRACE_CLASS_ISA;
	if (dhpv->dthpv_pattr.dtpa_name.dtat_class > DTRACE_CLASS_ISA)
		dhpv->dthpv_pattr.dtpa_name.dtat_class = DTRACE_CLASS_ISA;
	if (dhpv->dthpv_pattr.dtpa_args.dtat_class > DTRACE_CLASS_ISA)
		dhpv->dthpv_pattr.dtpa_args.dtat_class = DTRACE_CLASS_ISA;

	if ((provider = fasttrap_provider_lookup(p, DTFTP_PROVIDER_USDT, dhpv->dthpv_provname,
	    &dhpv->dthpv_pattr)) == NULL) {
		cmn_err(CE_WARN, "failed to instantiate provider %s for "
		    "process %u",  dhpv->dthpv_provname, (uint_t)p->p_pid);
		return (NULL);
	}

	  
               
   
                                                                    
                                                                      
                                                                    
                                                                      
                                                                      
                                                                    
                                                                         
                                                                    
                                                                     
                                                                     
                  
   
                                                                        
                                                                     
                                                                       
    

	  
                                                                   
                                                 
    
	provider->ftp_mcount++;

	lck_mtx_unlock(&provider->ftp_mtx);

	return (provider);
}

            
static void
fasttrap_meta_create_probe(void *arg, void *parg,
    dtrace_helper_probedesc_t *dhpb)
{
#pragma unused(arg)
	fasttrap_provider_t *provider = parg;
	fasttrap_probe_t *pp;
	fasttrap_tracepoint_t *tp;
	unsigned int i, j;
	uint32_t ntps;

	  
                                                                    
                                     
    
	ASSERT(provider->ftp_mcount > 0);

	  
                               
    
	qsort(dhpb->dthpb_offs, dhpb->dthpb_noffs, sizeof (uint32_t),
		fasttrap_uint32_cmp);
	for (i = 1; i < dhpb->dthpb_noffs; i++) {
		if (dhpb->dthpb_base + dhpb->dthpb_offs[i] <=
			dhpb->dthpb_base + dhpb->dthpb_offs[i - 1])
				return;
	}

	qsort(dhpb->dthpb_enoffs, dhpb->dthpb_nenoffs, sizeof (uint32_t),
		fasttrap_uint32_cmp);
	for (i = 1; i < dhpb->dthpb_nenoffs; i++) {
		if (dhpb->dthpb_base + dhpb->dthpb_enoffs[i] <=
			dhpb->dthpb_base + dhpb->dthpb_enoffs[i - 1])
				return;
	}

	  
                                                                 
                                                                  
                                  
    
	lck_mtx_lock(&provider->ftp_cmtx);

#if 0
	  
                                                        
                                                           
                  
    
	if (dtrace_probe_lookup(provider->ftp_provid, dhpb->dthpb_mod,
	    dhpb->dthpb_func, dhpb->dthpb_name) != 0) {
		lck_mtx_unlock(&provider->ftp_cmtx);
		return;
	}
#endif

	ntps = dhpb->dthpb_noffs + dhpb->dthpb_nenoffs;
	ASSERT(ntps > 0);

	os_atomic_add(&fasttrap_total, ntps, relaxed);

	if (fasttrap_total > fasttrap_max) {
		os_atomic_sub(&fasttrap_total, ntps, relaxed);
		lck_mtx_unlock(&provider->ftp_cmtx);
		return;
	}

	provider->ftp_pcount += ntps;

	if (ntps < FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS) {
		pp = zalloc(fasttrap_probe_t_zones[ntps]);
		bzero(pp, offsetof(fasttrap_probe_t, ftp_tps[ntps]));
	} else {
		pp = kmem_zalloc(offsetof(fasttrap_probe_t, ftp_tps[ntps]), KM_SLEEP);
	}

	pp->ftp_prov = provider;
	pp->ftp_pid = provider->ftp_pid;
	pp->ftp_ntps = ntps;
	pp->ftp_nargs = dhpb->dthpb_xargc;
	pp->ftp_xtypes = dhpb->dthpb_xtypes;
	pp->ftp_ntypes = dhpb->dthpb_ntypes;

	  
                                                                
    
	for (i = 0; i < dhpb->dthpb_noffs; i++) {
		tp = zalloc(fasttrap_tracepoint_t_zone);
		bzero(tp, sizeof (fasttrap_tracepoint_t));

		tp->ftt_proc = provider->ftp_proc;

		  
                                                                                           
                                                                                         
                                                                                                  
     
#if defined(__x86_64__)
		  
                                                                         
     
		tp->ftt_pc = dhpb->dthpb_base + (int64_t)dhpb->dthpb_offs[i] - 1;
#elif defined(__arm__) || defined(__arm64__)
		  
                                                                      
                                                             
                                                           
     
		tp->ftt_pc = (dhpb->dthpb_base + (int64_t)dhpb->dthpb_offs[i]) & ~0x1UL;
		tp->ftt_fntype = FASTTRAP_FN_USDT;
#else
#error "Architecture not supported"
#endif

		tp->ftt_pid = provider->ftp_pid;

		pp->ftp_tps[i].fit_tp = tp;
		pp->ftp_tps[i].fit_id.fti_probe = pp;
		pp->ftp_tps[i].fit_id.fti_ptype = DTFTP_OFFSETS;
	}

	  
                                                       
    
	for (j = 0; i < ntps; i++, j++) {
		tp = zalloc(fasttrap_tracepoint_t_zone);
		bzero(tp, sizeof (fasttrap_tracepoint_t));

		tp->ftt_proc = provider->ftp_proc;

		  
                                                                                           
                                                                                         
                                                                                                  
     
#if defined(__x86_64__)
		  
                                                                                  
     
		tp->ftt_pc = dhpb->dthpb_base + (int64_t)dhpb->dthpb_enoffs[j] + 2;
#elif defined(__arm__) || defined(__arm64__)
		  
                                                                      
                                                             
                                                           
     
		tp->ftt_pc = (dhpb->dthpb_base + (int64_t)dhpb->dthpb_enoffs[j]) & ~0x1UL;
		tp->ftt_fntype = FASTTRAP_FN_USDT;
#else
#error "Architecture not supported"
#endif

		tp->ftt_pid = provider->ftp_pid;

		pp->ftp_tps[i].fit_tp = tp;
		pp->ftp_tps[i].fit_id.fti_probe = pp;
		pp->ftp_tps[i].fit_id.fti_ptype = DTFTP_IS_ENABLED;
	}

	  
                                                                      
                                                                   
                             
    
	for (i = 0; i < dhpb->dthpb_xargc; i++) {
		if (dhpb->dthpb_args[i] != i) {
			pp->ftp_argmap = dhpb->dthpb_args;
			break;
		}
	}

	  
                                                              
    
	pp->ftp_id = dtrace_probe_create(provider->ftp_provid, dhpb->dthpb_mod,
	    dhpb->dthpb_func, dhpb->dthpb_name, FASTTRAP_OFFSET_AFRAMES, pp);

	lck_mtx_unlock(&provider->ftp_cmtx);
}

            
static void
fasttrap_meta_remove(void *arg, dtrace_helper_provdesc_t *dhpv, proc_t *p)
{
#pragma unused(arg)
	  
                                                                    
                                                                   
                                                                 
                              
    
	fasttrap_provider_retire(p, dhpv->dthpv_provname, 1);
}

static char*
fasttrap_meta_provider_name(void *arg)
{
	fasttrap_provider_t *fprovider = arg;
	dtrace_provider_t *provider = (dtrace_provider_t*)(fprovider->ftp_provid);
	return provider->dtpv_name;
}

static dtrace_mops_t fasttrap_mops = {
	.dtms_create_probe =	fasttrap_meta_create_probe,
	.dtms_provide_proc =	fasttrap_meta_provide,
	.dtms_remove_proc =	fasttrap_meta_remove,
	.dtms_provider_name =	fasttrap_meta_provider_name
};

  
                                                                    
                                                                       
            
  
                           
                                                                       
   
static int
fasttrap_validatestr(char const* str, size_t maxlen) {
	size_t len;

	assert(str);
	assert(maxlen != 0);

	                                             
	len = strnlen(str, maxlen);
	if (len >= maxlen)
		return -1;

	                                       
	return utf8_validatestr((unsigned const char*) str, len);
}

  
                                                                        
   
static int
fasttrap_check_cred_priv(cred_t *cr, proc_t *p)
{
	int err = 0;

	                               
	if (!kauth_cred_issuser(cr)) {
		err = EPERM;
		goto out;
	}

	                                     
	if (ISSET(p->p_lflag, P_LNOATTACH)) {
		err = EBUSY;
		goto out;
	}

#if CONFIG_MACF
	                                            
	struct proc_ident cur_ident = proc_ident(current_proc());
	struct proc_ident p_ident = proc_ident(p);

	                                                     
	proc_rele(p);
	err = mac_proc_check_debug(&cur_ident, cr, &p_ident);

	if (proc_find_ident(&p_ident) == PROC_NULL) {
		err = ESRCH;
		goto out_no_proc;
	}
#endif                  

out:
	proc_rele(p);

out_no_proc:
	return err;
}

            
static int
fasttrap_ioctl(dev_t dev, u_long cmd, user_addr_t arg, int md, cred_t *cr, int *rv)
{
#pragma unused(dev, md, rv)
	if (!dtrace_attached())
		return (EAGAIN);

	if (cmd == FASTTRAPIOC_MAKEPROBE) {
		fasttrap_probe_spec_t *probe;
		uint64_t noffs;
		size_t size;
		int ret;

		if (copyin(arg + __offsetof(fasttrap_probe_spec_t, ftps_noffs), &noffs,
		    sizeof (probe->ftps_noffs)))
			return (EFAULT);

		  
                                              
     
		if (noffs == 0)
			return (EINVAL);

		  
                                                      
                                                        
     
		if (noffs > ((1024 * 1024) - sizeof(fasttrap_probe_spec_t)) / sizeof(probe->ftps_offs[0]))
			return (ENOMEM);

		size = sizeof (fasttrap_probe_spec_t) +
		    sizeof (probe->ftps_offs[0]) * (noffs - 1);

		probe = kmem_alloc(size, KM_SLEEP);

		if (copyin(arg, probe, size) != 0 ||
		    probe->ftps_noffs != noffs) {
			kmem_free(probe, size);
			return (EFAULT);
		}

		  
                                                           
                      
     

		if (fasttrap_validatestr(probe->ftps_func, sizeof(probe->ftps_func)) != 0) {
			ret = EINVAL;
			goto err;
		}

		if (fasttrap_validatestr(probe->ftps_mod, sizeof(probe->ftps_mod)) != 0) {
			ret = EINVAL;
			goto err;
		}

		if (!PRIV_POLICY_CHOICE(cr, PRIV_ALL, B_FALSE)) {
			proc_t *p;
			pid_t pid = probe->ftps_pid;

			  
                                                  
                                   
      
			if ((p = proc_find(pid)) == PROC_NULL || p->p_stat == SIDL) {
				if (p != PROC_NULL)
					proc_rele(p);
				ret = ESRCH;
				goto err;
			}

			ret = fasttrap_check_cred_priv(cr, p);
			if (ret != 0) {
				goto err;
			}
		}

		ret = fasttrap_add_probe(probe);

err:
		kmem_free(probe, size);

		return (ret);

	} else if (cmd == FASTTRAPIOC_GETINSTR) {
		fasttrap_instr_query_t instr;
		fasttrap_tracepoint_t *tp;
		uint_t index;
		int ret;

		if (copyin(arg, &instr, sizeof (instr)) != 0)
			return (EFAULT);

		if (!PRIV_POLICY_CHOICE(cr, PRIV_ALL, B_FALSE)) {
			proc_t *p;
			pid_t pid = instr.ftiq_pid;

			  
                                                  
                                   
      
			if ((p = proc_find(pid)) == NULL || p->p_stat == SIDL) {
				if (p != PROC_NULL)
					proc_rele(p);
				return (ESRCH);
			}

			ret = fasttrap_check_cred_priv(cr, p);
			if (ret != 0) {
				return (ret);
			}
		}

		index = FASTTRAP_TPOINTS_INDEX(instr.ftiq_pid, instr.ftiq_pc);

		lck_mtx_lock(&fasttrap_tpoints.fth_table[index].ftb_mtx);
		tp = fasttrap_tpoints.fth_table[index].ftb_data;
		while (tp != NULL) {
			if (instr.ftiq_pid == tp->ftt_pid &&
			    instr.ftiq_pc == tp->ftt_pc &&
			    tp->ftt_proc->ftpc_acount != 0)
				break;

			tp = tp->ftt_next;
		}

		if (tp == NULL) {
			lck_mtx_unlock(&fasttrap_tpoints.fth_table[index].ftb_mtx);
			return (ENOENT);
		}

		bcopy(&tp->ftt_instr, &instr.ftiq_instr,
		    sizeof (instr.ftiq_instr));
		lck_mtx_unlock(&fasttrap_tpoints.fth_table[index].ftb_mtx);

		if (copyout(&instr, arg, sizeof (instr)) != 0)
			return (EFAULT);

		return (0);
	}

	return (EINVAL);
}

static void
fasttrap_attach(void)
{
	ulong_t nent;
	unsigned int i;

	  
                                                         
    
	dtrace_fasttrap_fork_ptr = &fasttrap_fork;
	dtrace_fasttrap_exit_ptr = &fasttrap_exec_exit;
	dtrace_fasttrap_exec_ptr = &fasttrap_exec_exit;

	  
                                                              
                                                                  
                       
    
	fasttrap_max = (sane_size >> 28) * 100000;

	if (fasttrap_max == 0)
		fasttrap_max = 50000;

	fasttrap_total = 0;
	fasttrap_retired = 0;

	  
                                           
    
#ifdef illumos
	nent = ddi_getprop(DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
	    "fasttrap-hash-size", FASTTRAP_TPOINTS_DEFAULT_SIZE);
#else
	nent = FASTTRAP_TPOINTS_DEFAULT_SIZE;
#endif

	if (nent <= 0 || nent > 0x1000000)
		nent = FASTTRAP_TPOINTS_DEFAULT_SIZE;

	if ((nent & (nent - 1)) == 0)
		fasttrap_tpoints.fth_nent = nent;
	else
		fasttrap_tpoints.fth_nent = 1 << fasttrap_highbit(nent);
	ASSERT(fasttrap_tpoints.fth_nent > 0);
	fasttrap_tpoints.fth_mask = fasttrap_tpoints.fth_nent - 1;
	fasttrap_tpoints.fth_table = kmem_zalloc(fasttrap_tpoints.fth_nent *
	    sizeof (fasttrap_bucket_t), KM_SLEEP);
	ASSERT(fasttrap_tpoints.fth_table != NULL);

	for (i = 0; i < fasttrap_tpoints.fth_nent; i++) {
		lck_mtx_init(&fasttrap_tpoints.fth_table[i].ftb_mtx, &fasttrap_lck_grp,
		    &fasttrap_lck_attr);
	}

	  
                                       
    
	nent = FASTTRAP_PROVIDERS_DEFAULT_SIZE;
	if ((nent & (nent - 1)) == 0)
		fasttrap_provs.fth_nent = nent;
	else
		fasttrap_provs.fth_nent = 1 << fasttrap_highbit(nent);
	ASSERT(fasttrap_provs.fth_nent > 0);
	fasttrap_provs.fth_mask = fasttrap_provs.fth_nent - 1;
	fasttrap_provs.fth_table = kmem_zalloc(fasttrap_provs.fth_nent *
	    sizeof (fasttrap_bucket_t), KM_SLEEP);
	ASSERT(fasttrap_provs.fth_table != NULL);

	for (i = 0; i < fasttrap_provs.fth_nent; i++) {
		lck_mtx_init(&fasttrap_provs.fth_table[i].ftb_mtx, &fasttrap_lck_grp,
		    &fasttrap_lck_attr);
	}

	  
                                 
    
	nent = FASTTRAP_PROCS_DEFAULT_SIZE;
	if ((nent & (nent - 1)) == 0)
		fasttrap_procs.fth_nent = nent;
	else
		fasttrap_procs.fth_nent = 1 << fasttrap_highbit(nent);
	ASSERT(fasttrap_procs.fth_nent > 0);
	fasttrap_procs.fth_mask = fasttrap_procs.fth_nent - 1;
	fasttrap_procs.fth_table = kmem_zalloc(fasttrap_procs.fth_nent *
	    sizeof (fasttrap_bucket_t), KM_SLEEP);
	ASSERT(fasttrap_procs.fth_table != NULL);

#ifndef illumos
	for (i = 0; i < fasttrap_procs.fth_nent; i++) {
		lck_mtx_init(&fasttrap_procs.fth_table[i].ftb_mtx, &fasttrap_lck_grp,
		    &fasttrap_lck_attr);
	}
#endif

	(void) dtrace_meta_register("fasttrap", &fasttrap_mops, NULL,
	    &fasttrap_meta_id);
}

static int
_fasttrap_open(dev_t dev, int flags, int devtype, struct proc *p)
{
#pragma unused(dev, flags, devtype, p)
	return  0;
}

static int
_fasttrap_ioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	int err, rv = 0;
	user_addr_t uaddrp;

	if (proc_is64bit(p)) {
		uaddrp = *(user_addr_t *)data;
	} else {
		uaddrp = (user_addr_t) *(uint32_t *)data;
	}

	err = fasttrap_ioctl(dev, cmd, uaddrp, fflag, CRED(), &rv);

	                                                                                                       
	if (err != 0) {
		ASSERT( (err & 0xfffff000) == 0 );
		return (err & 0xfff);                                                             
	} else if (rv != 0) {
		ASSERT( (rv & 0xfff00000) == 0 );
		return (((rv & 0xfffff) << 12));                                                               
	} else
		return 0;
}

static int fasttrap_inited = 0;

#define FASTTRAP_MAJOR  -24                                            

static const struct cdevsw fasttrap_cdevsw =
{
	.d_open = _fasttrap_open,
	.d_close = eno_opcl,
	.d_read = eno_rdwrt,
	.d_write = eno_rdwrt,
	.d_ioctl = _fasttrap_ioctl,
	.d_stop = (stop_fcn_t *)nulldev,
	.d_reset = (reset_fcn_t *)nulldev,
	.d_select = eno_select,
	.d_mmap = eno_mmap,
	.d_strategy = eno_strat,
	.d_reserved_1 = eno_getc,
	.d_reserved_2 = eno_putc,
};

void fasttrap_init(void);

void
fasttrap_init( void )
{
	  
                                                                             
                                                                        
   
                                                                                       
    
	if (!fasttrap_inited) {
		int majdevno = cdevsw_add(FASTTRAP_MAJOR, &fasttrap_cdevsw);

		if (majdevno < 0) {
			                                                   
			printf("fasttrap_init: failed to allocate a major number!\n");
			return;
		}

		dev_t device = makedev( (uint32_t)majdevno, 0 );
		if (NULL == devfs_make_node( device, DEVFS_CHAR, UID_ROOT, GID_WHEEL, 0666, "fasttrap", 0 )) {
			return;
		}

		  
                                                                         
                                 
     
		int i;
		for (i=1; i<FASTTRAP_PROBE_T_ZONE_MAX_TRACEPOINTS; i++) {
			fasttrap_probe_t_zones[i] =
			    zone_create(fasttrap_probe_t_zone_names[i],
				    offsetof(fasttrap_probe_t, ftp_tps[i]), ZC_NONE);
		}


		fasttrap_attach();

		  
                                      
     
		kern_return_t res = kernel_thread_start_priority((thread_continue_t)fasttrap_pid_cleanup_cb, NULL, 46                         , &fasttrap_cleanup_thread);
		if (res != KERN_SUCCESS) {
			panic("Could not create fasttrap_cleanup_thread");
		}
		thread_set_thread_name(fasttrap_cleanup_thread, "dtrace_fasttrap_cleanup_thread");

		fasttrap_retired_size = DEFAULT_RETIRED_SIZE;
		fasttrap_retired_spec = kmem_zalloc(fasttrap_retired_size * sizeof(*fasttrap_retired_spec),
					KM_SLEEP);

		fasttrap_inited = 1;
	}
}

#undef FASTTRAP_MAJOR
