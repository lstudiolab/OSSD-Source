/*
 * Copyright (c) 1998-2020 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)uipc_mbuf.c	8.2 (Berkeley) 1/4/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <ptrauth.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/file_internal.h>

#include <dev/random/randomdev.h>

#include <kern/kern_types.h>
#include <kern/simple_lock.h>
#include <kern/queue.h>
#include <kern/sched_prim.h>
#include <kern/backtrace.h>
#include <kern/percpu.h>
#include <kern/zalloc.h>

#include <libkern/OSAtomic.h>
#include <libkern/OSDebug.h>
#include <libkern/libkern.h>

#include <os/log.h>
#include <os/ptrtools.h>

#include <IOKit/IOMapper.h>

#include <machine/limits.h>
#include <machine/machine_routines.h>

#include <sys/mcache.h>
#include <net/ntstat.h>

  
                             
  
                                        
  
           
                                                                
                                                                  
                                                        
  
         
                                                                   
                                                               
                                                                
  
            
                                                                      
                                                                  
                                                               
  
              
                                                                 
                                                                 
                                                                   
                       
  
                 
                                                                     
                                                                    
                                                                   
                       
  
                     
  
                                                                      
                                                                      
                                                                     
                                                                       
                                                                       
                                                                         
                                                                          
                                                                    
  
                         
  
                                              
                                                     
                                  
        
        
                                
           
                                                       
           
           
                                             
           
           
                          
           
           
                                                    
       
       
                    
       
       
                             
  
                       
  
                                                        
        
        
                                
           
                                                        
           
           
                                             
           
           
                           
           
           
                                             
           
           
                            
                                                    
  
                                                                       
                                                                          
                                                                            
                                                                         
                                                                           
                                                                   
  
                       
  
                                                                        
                                                                        
                                                                       
                                                                         
                                                                        
                                                         
  
                         
  
                                                              
        
        
                                
           
                                      
           
           
                         
           
           
                                             
           
           
                         
           
           
                                             
                                             
  
                       
  
                               
        
        
                                
           
                                      
           
           
                          
           
           
                                             
           
           
                          
           
           
                                             
           
           
                            
                                                    
  
                                                                      
                                                                          
                                                                         
                                                                          
                                        
  
             
  
                                                                         
                                                                          
                                                                          
                                                                      
                                                                         
                                                                         
  
                                                                           
                                                                          
                                                                           
                                                                          
  
                                   
                                   
                                   
                             
                                    
                                                          
                                  
                              
                                        
                              
                                            
                  
  
                                                                             
                                                                           
                                                                             
                                                                           
                                                                             
                                                                             
                                                    
   

                                    
                       
extern ppnum_t pmap_find_phys(pmap_t pmap, addr64_t va);
extern vm_map_t mb_map;                          

static uint32_t mb_kmem_contig_failed;
static uint32_t mb_kmem_failed;
static uint32_t mb_kmem_one_failed;
                                       
static uint64_t mb_kmem_contig_failed_ts;
static uint64_t mb_kmem_failed_ts;
static uint64_t mb_kmem_one_failed_ts;
static uint64_t mb_kmem_contig_failed_size;
static uint64_t mb_kmem_failed_size;
static uint32_t mb_kmem_stats[6];
static const char *mb_kmem_stats_labels[] = { "INVALID_ARGUMENT",
	                                      "INVALID_ADDRESS",
	                                      "RESOURCE_SHORTAGE",
	                                      "NO_SPACE",
	                                      "KERN_FAILURE",
	                                      "OTHERS" };

                 
static LCK_GRP_DECLARE(mbuf_mlock_grp, "mbuf");
static LCK_MTX_DECLARE(mbuf_mlock_data, &mbuf_mlock_grp);
static lck_mtx_t *const mbuf_mlock = &mbuf_mlock_data;

                             
static uint64_t mb_expand_cnt;
static uint64_t mb_expand_cl_cnt;
static uint64_t mb_expand_cl_total;
static uint64_t mb_expand_bigcl_cnt;
static uint64_t mb_expand_bigcl_total;
static uint64_t mb_expand_16kcl_cnt;
static uint64_t mb_expand_16kcl_total;
static boolean_t mbuf_worker_needs_wakeup;                                   
static uint32_t mbuf_worker_run_cnt;
static uint64_t mbuf_worker_last_runtime;
static uint64_t mbuf_drain_last_runtime;
static int mbuf_worker_ready;                                  
static unsigned int ncpu;                                    
static ppnum_t *mcl_paddr;                                               
static ppnum_t mcl_pages;                                             
static ppnum_t mcl_paddr_base;                                                
static mcache_t *ref_cache;                                             
static mcache_t *mcl_audit_con_cache;                           
static unsigned int mbuf_debug;                                  
static unsigned int mb_normalized;                                     

#define MB_GROWTH_AGGRESSIVE    1                                    
#define MB_GROWTH_NORMAL        2                                    

typedef enum {
	MC_MBUF = 0,                      
	MC_CL,                       
	MC_BIGCL,                                
	MC_16KCL,                                 
	MC_MBUF_CL,                         
	MC_MBUF_BIGCL,                                  
	MC_MBUF_16KCL                                    
} mbuf_class_t;

#define MBUF_CLASS_MIN          MC_MBUF
#define MBUF_CLASS_MAX          MC_MBUF_16KCL
#define MBUF_CLASS_LAST         MC_16KCL
#define MBUF_CLASS_VALID(c) \
	((int)(c) >= MBUF_CLASS_MIN && (int)(c) <= MBUF_CLASS_MAX)
#define MBUF_CLASS_COMPOSITE(c) \
	((int)(c) > MBUF_CLASS_LAST)


  
                                                 
   
#define MCR_COMP        MCR_USR1                                          

  
                              
  
                                                                         
                                                                             
                                                                         
                                                                         
                                                                           
                                                                          
                                                                         
                         
  
                                                                         
                                                                        
                                                                        
                                                                        
                                     
  
                                                                       
                                                                         
                                                                       
                                         
  
                                       
   
typedef struct mcl_slab {
	struct mcl_slab *sl_next;                             
	u_int8_t        sl_class;                                   
	int8_t          sl_refcnt;                                   
	int8_t          sl_chunks;                                      
	u_int16_t       sl_flags;                                   
	u_int16_t       sl_len;                          
	void            *sl_base;                                     
	void            *sl_head;                              
	TAILQ_ENTRY(mcl_slab) sl_link;                                  
} mcl_slab_t;

#define SLF_MAPPED      0x0001                                       
#define SLF_PARTIAL     0x0002                                    
#define SLF_DETACHED    0x0004                                    

  
                                                                        
                                                                     
                                                                       
            
   
#define NSLABSPMB       ((1 << MBSHIFT) >> PAGE_SHIFT)

typedef struct mcl_slabg {
	mcl_slab_t      *slg_slab;                          
} mcl_slabg_t;

  
                                                           
   
#define NSLABSP16KB     (M16KCLBYTES >> PAGE_SHIFT)

  
                               
   
typedef struct {
	mcache_audit_t  **cl_audit;                          
} mcl_audit_t;

typedef struct {
	struct thread   *msa_thread;                                  
	struct thread   *msa_pthread;                                    
	uint32_t        msa_tstamp;                                     
	uint32_t        msa_ptstamp;                                         
	uint16_t        msa_depth;                          
	uint16_t        msa_pdepth;                                        
	void            *msa_stack[MCACHE_STACK_DEPTH];
	void            *msa_pstack[MCACHE_STACK_DEPTH];
} mcl_scratch_audit_t;

typedef struct {
	  
                                                                 
                                                                     
                                                                     
                                                                       
                                                                    
                                                                   
                                                                 
                                                              
    
	u_int8_t                sc_mbuf[(MSIZE - _MHLEN) + sizeof(_m_ext_t)];
	mcl_scratch_audit_t     sc_scratch __attribute__((aligned(8)));
} mcl_saved_contents_t;

#define AUDIT_CONTENTS_SIZE     (sizeof (mcl_saved_contents_t))

#define MCA_SAVED_MBUF_PTR(_mca)                                        \
	((struct mbuf *)(void *)((mcl_saved_contents_t *)               \
	(_mca)->mca_contents)->sc_mbuf)
#define MCA_SAVED_MBUF_SIZE                                             \
	(sizeof (((mcl_saved_contents_t *)0)->sc_mbuf))
#define MCA_SAVED_SCRATCH_PTR(_mca)                                     \
	(&((mcl_saved_contents_t *)(_mca)->mca_contents)->sc_scratch)

  
                                   
   
#define MB_INUSE        0x01                                              
#define MB_COMP_INUSE   0x02                                               
#define MB_SCVALID      0x04                                         

  
                                                                    
   
static mcl_audit_t *mclaudit;                                           
static unsigned int maxclaudit;                                      
static mcl_slabg_t **slabstbl;                           
static unsigned int maxslabgrp;                                      
static unsigned int slabgrp;                                     

             
int nclusters;                                                                  
int njcl;                                                          
int njclbytes;                                               
unsigned char *mbutl;                                             
unsigned char *embutl;                                                   
int _max_linkhdr;                                              
int _max_protohdr;                                           
int max_hdr;                                                      
int max_datalen;                                     

static boolean_t mclverify;                                  
static boolean_t mcltrace;                                
static boolean_t mclfindleak;                              
static boolean_t mclexpleak;                                               

static struct timeval mb_start;                        

                                   
static struct mleak_table mleak_table;
static mleak_stat_t *mleak_stat;

#define MLEAK_STAT_SIZE(n) \
	__builtin_offsetof(mleak_stat_t, ml_trace[n])

struct mallocation {
	mcache_obj_t *element;                                            
	u_int32_t trace_index;                                                
	u_int32_t count;                                             
	u_int64_t hitcount;                                             
};

struct mtrace {
	u_int64_t       collisions;
	u_int64_t       hitcount;
	u_int64_t       allocs;
	u_int64_t       depth;
	uintptr_t       addr[MLEAK_STACK_DEPTH];
};

                                                                                
#define MLEAK_ALLOCATION_MAP_NUM        512
#define MLEAK_TRACE_MAP_NUM             256

  
                                                                       
                                       
   
#define MLEAK_SAMPLE_FACTOR             500

  
                                  
   
#define MLEAK_NUM_TRACES                5

#define MB_LEAK_SPACING_64 "                    "
#define MB_LEAK_SPACING_32 "            "


#define MB_LEAK_HDR_32  "\n\
    trace [1]   trace [2]   trace [3]   trace [4]   trace [5]  \n\
    ----------  ----------  ----------  ----------  ---------- \n\
"

#define MB_LEAK_HDR_64  "\n\
    trace [1]           trace [2]           trace [3]       \
	trace [4]           trace [5]      \n\
    ------------------  ------------------  ------------------  \
    ------------------  ------------------ \n\
"

static uint32_t mleak_alloc_buckets = MLEAK_ALLOCATION_MAP_NUM;
static uint32_t mleak_trace_buckets = MLEAK_TRACE_MAP_NUM;

                                                            
static struct mallocation *mleak_allocations;
static struct mtrace *mleak_traces;
static struct mtrace *mleak_top_trace[MLEAK_NUM_TRACES];

                                                               
static LCK_GRP_DECLARE(mleak_lock_grp, "mleak_lock");
static LCK_MTX_DECLARE(mleak_lock_data, &mleak_lock_grp);
static lck_mtx_t *const mleak_lock = &mleak_lock_data;

                                 
struct mtracelarge {
	uint64_t        size;
	uint64_t        depth;
	uintptr_t       addr[MLEAK_STACK_DEPTH];
};

#define MTRACELARGE_NUM_TRACES          5
static struct mtracelarge mtracelarge_table[MTRACELARGE_NUM_TRACES];

static void mtracelarge_register(size_t size);

                                                   
static LCK_GRP_DECLARE(mbuf_tx_compl_tbl_lck_grp, "mbuf_tx_compl_tbl");
LCK_RW_DECLARE(mbuf_tx_compl_tbl_lock, &mbuf_tx_compl_tbl_lck_grp);

extern u_int32_t high_sb_max;

                                                                 
#define MINCL           32
#define MINBIGCL        (MINCL >> 1)
#define MIN16KCL        (MINCL >> 2)

                                                                  
#define MBIGCL_LOWAT    MINBIGCL
#define M16KCL_LOWAT    MIN16KCL

typedef struct {
	mbuf_class_t    mtbl_class;                     
	mcache_t        *mtbl_cache;                                      
	TAILQ_HEAD(mcl_slhead, mcl_slab) mtbl_slablist;                
	mcache_obj_t    *mtbl_cobjlist;                                 
	mb_class_stat_t *mtbl_stats;                                         
	u_int32_t       mtbl_maxsize;                            
	int             mtbl_minlimit;                       
	int             mtbl_maxlimit;                       
	u_int32_t       mtbl_wantpurge;                                
	uint32_t        mtbl_avgtotal;                            
	u_int32_t       mtbl_expand;                                        
} mbuf_table_t;

#define m_class(c)      mbuf_table[c].mtbl_class
#define m_cache(c)      mbuf_table[c].mtbl_cache
#define m_slablist(c)   mbuf_table[c].mtbl_slablist
#define m_cobjlist(c)   mbuf_table[c].mtbl_cobjlist
#define m_maxsize(c)    mbuf_table[c].mtbl_maxsize
#define m_minlimit(c)   mbuf_table[c].mtbl_minlimit
#define m_maxlimit(c)   mbuf_table[c].mtbl_maxlimit
#define m_wantpurge(c)  mbuf_table[c].mtbl_wantpurge
#define m_cname(c)      mbuf_table[c].mtbl_stats->mbcl_cname
#define m_size(c)       mbuf_table[c].mtbl_stats->mbcl_size
#define m_total(c)      mbuf_table[c].mtbl_stats->mbcl_total
#define m_active(c)     mbuf_table[c].mtbl_stats->mbcl_active
#define m_infree(c)     mbuf_table[c].mtbl_stats->mbcl_infree
#define m_slab_cnt(c)   mbuf_table[c].mtbl_stats->mbcl_slab_cnt
#define m_alloc_cnt(c)  mbuf_table[c].mtbl_stats->mbcl_alloc_cnt
#define m_free_cnt(c)   mbuf_table[c].mtbl_stats->mbcl_free_cnt
#define m_notified(c)   mbuf_table[c].mtbl_stats->mbcl_notified
#define m_purge_cnt(c)  mbuf_table[c].mtbl_stats->mbcl_purge_cnt
#define m_fail_cnt(c)   mbuf_table[c].mtbl_stats->mbcl_fail_cnt
#define m_ctotal(c)     mbuf_table[c].mtbl_stats->mbcl_ctotal
#define m_peak(c)       mbuf_table[c].mtbl_stats->mbcl_peak_reported
#define m_release_cnt(c) mbuf_table[c].mtbl_stats->mbcl_release_cnt
#define m_region_expand(c)      mbuf_table[c].mtbl_expand

static mbuf_table_t mbuf_table[] = {
	  
                                                            
                                                                  
                          
    
	{ MC_MBUF, NULL, TAILQ_HEAD_INITIALIZER(m_slablist(MC_MBUF)),
	  NULL, NULL, 0, 0, 0, 0, 3000, 0 },
	{ MC_CL, NULL, TAILQ_HEAD_INITIALIZER(m_slablist(MC_CL)),
	  NULL, NULL, 0, 0, 0, 0, 2000, 0 },
	{ MC_BIGCL, NULL, TAILQ_HEAD_INITIALIZER(m_slablist(MC_BIGCL)),
	  NULL, NULL, 0, 0, 0, 0, 1000, 0 },
	{ MC_16KCL, NULL, TAILQ_HEAD_INITIALIZER(m_slablist(MC_16KCL)),
	  NULL, NULL, 0, 0, 0, 0, 200, 0 },
	  
                                                                
                                                               
                                                                  
                                                               
                                                           
                                                       
    
	{ MC_MBUF_CL, NULL, { NULL, NULL }, NULL, NULL, 0, 0, 0, 0, 2000, 0 },
	{ MC_MBUF_BIGCL, NULL, { NULL, NULL }, NULL, NULL, 0, 0, 0, 0, 1000, 0 },
	{ MC_MBUF_16KCL, NULL, { NULL, NULL }, NULL, NULL, 0, 0, 0, 0, 200, 0 },
};

#define NELEM(a)        (sizeof (a) / sizeof ((a)[0]))


static uint32_t
m_avgtotal(mbuf_class_t c)
{
	return mbuf_table[c].mtbl_avgtotal;
}

static void *mb_waitchan = &mbuf_table;                                  
static int mb_waiters;                                         

boolean_t mb_peak_newreport = FALSE;
boolean_t mb_peak_firstreport = FALSE;

                                                         
#define MBUF_PEAK_FIRST_REPORT_THRESHOLD        604800

#define MB_WDT_MAXTIME  10                                                   
static struct timeval mb_wdtstart;                                    
static char *mbuf_dump_buf;

#define MBUF_DUMP_BUF_SIZE      4096

  
                                                                     
                               
                                                                  
                                                                          
                                     
   
#if !XNU_TARGET_OS_OSX || DEVELOPMENT || DEBUG
static unsigned int mb_watchdog = 1;
#else                                                  
static unsigned int mb_watchdog = 0;
#endif                                                  
#if !XNU_TARGET_OS_OSX
static unsigned int mb_drain_maxint = 60;
#else                        
static unsigned int mb_drain_maxint = 0;
#endif                        
static unsigned int mb_memory_pressure_percentage = 80;

uintptr_t mb_obscure_extfree __attribute__((visibility("hidden")));
uintptr_t mb_obscure_extref __attribute__((visibility("hidden")));

              
static u_int32_t mb_redzone_cookie;
static void m_redzone_init(struct mbuf *);
static void m_redzone_verify(struct mbuf *m);

                                                     
static boolean_t mb_clalloc_busy;
static void *mb_clalloc_waitchan = &mb_clalloc_busy;
static int mb_clalloc_waiters;

static void mbuf_mtypes_sync(boolean_t);
static int mbstat_sysctl SYSCTL_HANDLER_ARGS;
static void mbuf_stat_sync(void);
static int mb_stat_sysctl SYSCTL_HANDLER_ARGS;
static int mleak_top_trace_sysctl SYSCTL_HANDLER_ARGS;
static int mleak_table_sysctl SYSCTL_HANDLER_ARGS;
static char *mbuf_dump(void);
static void mbuf_table_init(void);
static inline void m_incref(struct mbuf *);
static inline u_int16_t m_decref(struct mbuf *);
static int m_clalloc(const u_int32_t, const int, const u_int32_t);
static void mbuf_worker_thread_init(void);
static mcache_obj_t *slab_alloc(mbuf_class_t, int);
static void slab_free(mbuf_class_t, mcache_obj_t *);
static unsigned int mbuf_slab_alloc(void *, mcache_obj_t ***,
    unsigned int, int);
static void mbuf_slab_free(void *, mcache_obj_t *, int);
static void mbuf_slab_audit(void *, mcache_obj_t *, boolean_t);
static void mbuf_slab_notify(void *, u_int32_t);
static unsigned int cslab_alloc(mbuf_class_t, mcache_obj_t ***,
    unsigned int);
static unsigned int cslab_free(mbuf_class_t, mcache_obj_t *, int);
static unsigned int mbuf_cslab_alloc(void *, mcache_obj_t ***,
    unsigned int, int);
static void mbuf_cslab_free(void *, mcache_obj_t *, int);
static void mbuf_cslab_audit(void *, mcache_obj_t *, boolean_t);
static int freelist_populate(mbuf_class_t, unsigned int, int);
static void freelist_init(mbuf_class_t);
static boolean_t mbuf_cached_above(mbuf_class_t, int);
static boolean_t mbuf_steal(mbuf_class_t, unsigned int);
static void m_reclaim(mbuf_class_t, unsigned int, boolean_t);
static int m_howmany(int, size_t);
static void mbuf_worker_thread(void);
static void mbuf_watchdog(void);
static boolean_t mbuf_sleep(mbuf_class_t, unsigned int, int);

static void mcl_audit_init(void *, mcache_audit_t **, mcache_obj_t **,
    size_t, unsigned int);
static void mcl_audit_free(void *, unsigned int);
static mcache_audit_t *mcl_audit_buf2mca(mbuf_class_t, mcache_obj_t *);
static void mcl_audit_mbuf(mcache_audit_t *, void *, boolean_t, boolean_t);
static void mcl_audit_cluster(mcache_audit_t *, void *, size_t, boolean_t,
    boolean_t);
static void mcl_audit_restore_mbuf(struct mbuf *, mcache_audit_t *, boolean_t);
static void mcl_audit_save_mbuf(struct mbuf *, mcache_audit_t *);
static void mcl_audit_scratch(mcache_audit_t *);
static void mcl_audit_mcheck_panic(struct mbuf *);
static void mcl_audit_verify_nextptr(void *, mcache_audit_t *);

static void mleak_activate(void);
static void mleak_logger(u_int32_t, mcache_obj_t *, boolean_t);
static boolean_t mleak_log(uintptr_t *, mcache_obj_t *, uint32_t, int);
static void mleak_free(mcache_obj_t *);
static void mleak_sort_traces(void);
static void mleak_update_stats(void);

static mcl_slab_t *slab_get(void *);
static void slab_init(mcl_slab_t *, mbuf_class_t, u_int32_t,
    void *, void *, unsigned int, int, int);
static void slab_insert(mcl_slab_t *, mbuf_class_t);
static void slab_remove(mcl_slab_t *, mbuf_class_t);
static boolean_t slab_inrange(mcl_slab_t *, void *);
static void slab_nextptr_panic(mcl_slab_t *, void *);
static void slab_detach(mcl_slab_t *);
static boolean_t slab_is_detached(mcl_slab_t *);

static int m_copyback0(struct mbuf **, int, int, const void *, int, int);
static struct mbuf *m_split0(struct mbuf *, int, int, int);
__private_extern__ void mbuf_report_peak_usage(void);
static boolean_t mbuf_report_usage(mbuf_class_t);
#if DEBUG || DEVELOPMENT
#define mbwdog_logger(fmt, ...)  _mbwdog_logger(__func__, __LINE__, fmt, ## __VA_ARGS__)
static void _mbwdog_logger(const char *func, const int line, const char *fmt, ...);
static char *mbwdog_logging;
const unsigned mbwdog_logging_size = 4096;
static size_t mbwdog_logging_used;
#else
#define mbwdog_logger(fmt, ...)  do { } while (0)
#endif
static void mbuf_drain_locked(boolean_t);

                           
#define M_COPYBACK0_COPYBACK    0x0001                        
#define M_COPYBACK0_PRESERVE    0x0002                              
#define M_COPYBACK0_COW         0x0004                        
#define M_COPYBACK0_EXTEND      0x0008                    

  
                                                                         
                                                                        
                                                                          
                                                                        
                                                                            
                                                                           
                                                                         
                                                                        
                                                   
   
#define EXTF_COMPOSITE  0x1

  
                                                                         
                                                                           
           
   
#define EXTF_READONLY   0x2
  
                                                                         
                                                                         
                                                                      
                         
   
#define EXTF_PAIRED     0x4

#define EXTF_MASK       \
	(EXTF_COMPOSITE | EXTF_READONLY | EXTF_PAIRED)

#define MEXT_MINREF(m)          ((m_get_rfa(m))->minref)
#define MEXT_REF(m)             ((m_get_rfa(m))->refcnt)
#define MEXT_PREF(m)            ((m_get_rfa(m))->prefcnt)
#define MEXT_FLAGS(m)           ((m_get_rfa(m))->flags)
#define MEXT_PRIV(m)            ((m_get_rfa(m))->priv)
#define MEXT_PMBUF(m)           ((m_get_rfa(m))->paired)
#define MEXT_TOKEN(m)           ((m_get_rfa(m))->ext_token)
#define MBUF_IS_COMPOSITE(m)                                            \
	(MEXT_REF(m) == MEXT_MINREF(m) &&                               \
	(MEXT_FLAGS(m) & EXTF_MASK) == EXTF_COMPOSITE)
  
                                                                      
                                                                       
                                                                            
                                            
   
#define MBUF_IS_PAIRED(m)                                               \
	(((m)->m_flags & M_EXT) &&                                      \
	(MEXT_FLAGS(m) & EXTF_MASK) == EXTF_PAIRED &&                   \
	MEXT_PMBUF(m) == (m))

  
                                                   
   
#define _MCHECK(m) {                                                    \
	if ((m)->m_type != MT_FREE && !MBUF_IS_PAIRED(m)) {             \
	        if (mclaudit == NULL)                                   \
	                panic("MCHECK: m_type=%d m=%p",                 \
	                    (u_int16_t)(m)->m_type, m);                 \
	        else                                                    \
	                mcl_audit_mcheck_panic(m);                      \
	}                                                               \
}

#define MBUF_IN_MAP(addr)                                               \
	((unsigned char *)(addr) >= mbutl &&                            \
	(unsigned char *)(addr) < embutl)

#define MRANGE(addr) {                                                  \
	if (!MBUF_IN_MAP(addr))                                         \
	        panic("MRANGE: address out of range 0x%p", addr);       \
}

  
                         
   
#define MTOD(m, t)      ((t)((m)->m_data))

  
                                                           
   
#define MTOPG(x)        (((unsigned char *)x - mbutl) >> PAGE_SHIFT)
#define PGTOM(x)        (mbutl + (x << PAGE_SHIFT))

  
                                                   
   
#define MBPAGEIDX(c, m) \
	(((unsigned char *)(m) - (unsigned char *)(c)) >> MSIZESHIFT)

  
                                    
   
#define CLPAGEIDX(c, m) \
	(((unsigned char *)(m) - (unsigned char *)(c)) >> MCLSHIFT)

  
                                                     
   
#define BCLPAGEIDX(c, m) \
	(((unsigned char *)(m) - (unsigned char *)(c)) >> MBIGCLSHIFT)

  
                                                      
   
#define MBUF_INIT_PKTHDR(m) {                                           \
	(m)->m_pkthdr.rcvif = NULL;                                     \
	(m)->m_pkthdr.pkt_hdr = NULL;                                   \
	(m)->m_pkthdr.len = 0;                                          \
	(m)->m_pkthdr.csum_flags = 0;                                   \
	(m)->m_pkthdr.csum_data = 0;                                    \
	(m)->m_pkthdr.vlan_tag = 0;                                     \
	(m)->m_pkthdr.comp_gencnt = 0;                                  \
	m_classifier_init(m, 0);                                        \
	m_tag_init(m, 1);                                               \
	m_scratch_init(m);                                              \
	m_redzone_init(m);                                              \
}

#define MBUF_INIT(m, pkthdr, type) {                                    \
	_MCHECK(m);                                                     \
	(m)->m_next = (m)->m_nextpkt = NULL;                            \
	(m)->m_len = 0;                                                 \
	(m)->m_type = type;                                             \
	if ((pkthdr) == 0) {                                            \
	        (m)->m_data = (m)->m_dat;                               \
	        (m)->m_flags = 0;                                       \
	} else {                                                        \
	        (m)->m_data = (m)->m_pktdat;                            \
	        (m)->m_flags = M_PKTHDR;                                \
	        MBUF_INIT_PKTHDR(m);                                    \
	}                                                               \
}

#define MEXT_INIT(m, buf, size, free, arg, rfa, min, ref, pref, flag,   \
	    priv, pm) {                                                 \
	(m)->m_data = (m)->m_ext.ext_buf = (buf);                       \
	(m)->m_flags |= M_EXT;                                          \
	m_set_ext((m), (rfa), (free), (arg));                           \
	(m)->m_ext.ext_size = (size);                                   \
	MEXT_MINREF(m) = (min);                                         \
	MEXT_REF(m) = (ref);                                            \
	MEXT_PREF(m) = (pref);                                          \
	MEXT_FLAGS(m) = (flag);                                         \
	MEXT_PRIV(m) = (priv);                                          \
	MEXT_PMBUF(m) = (pm);                                           \
}

#define MBUF_CL_INIT(m, buf, rfa, ref, flag)    \
	MEXT_INIT(m, buf, m_maxsize(MC_CL), NULL, NULL, rfa, 0,         \
	    ref, 0, flag, 0, NULL)

#define MBUF_BIGCL_INIT(m, buf, rfa, ref, flag) \
	MEXT_INIT(m, buf, m_maxsize(MC_BIGCL), m_bigfree, NULL, rfa, 0, \
	    ref, 0, flag, 0, NULL)

#define MBUF_16KCL_INIT(m, buf, rfa, ref, flag) \
	MEXT_INIT(m, buf, m_maxsize(MC_16KCL), m_16kfree, NULL, rfa, 0, \
	    ref, 0, flag, 0, NULL)

  
                                                     
   
#define MSLEEPF(f)      ((!((f) & M_DONTWAIT)) ? MCR_SLEEP : MCR_NOSLEEP)

  
                                                                            
                                                                         
                                                                          
                                                                      
   
struct mb_stat *mb_stat;
struct omb_stat *omb_stat;                                       

#define MB_STAT_SIZE(n) \
	__builtin_offsetof(mb_stat_t, mbs_class[n])
#define OMB_STAT_SIZE(n) \
	__builtin_offsetof(struct omb_stat, mbs_class[n])

  
                                                                      
                                                                        
                                                                          
                                                                             
                                                                           
                                                                           
                                                                         
                                                                          
                                         
   
struct mbstat mbstat;

#define MBSTAT_MTYPES_MAX \
	(sizeof (mbstat.m_mtypes) / sizeof (mbstat.m_mtypes[0]))

  
                                                                           
                                                                           
                                                                           
                                                                         
                                                                            
                                                                           
                                                                            
                                                                          
                                                                     
   
typedef struct {
	unsigned int cpu_mtypes[MT_MAX];
} mbuf_mtypes_t;

static mbuf_mtypes_t PERCPU_DATA(mbuf_mtypes);

#define mtype_stat_add(type, n) {                                       \
	if ((unsigned)(type) < MT_MAX) {                                \
	        mbuf_mtypes_t *mbs = PERCPU_GET(mbuf_mtypes);           \
	        atomic_add_32(&mbs->cpu_mtypes[type], n);               \
	} else if ((unsigned)(type) < (unsigned)MBSTAT_MTYPES_MAX) {    \
	        atomic_add_16((int16_t *)&mbstat.m_mtypes[type], n);    \
	}                                                               \
}

#define mtype_stat_sub(t, n)    mtype_stat_add(t, -(n))
#define mtype_stat_inc(t)       mtype_stat_add(t, 1)
#define mtype_stat_dec(t)       mtype_stat_sub(t, 1)

static void
mbuf_mtypes_sync(boolean_t locked)
{
	mbuf_mtypes_t mtc;

	if (locked) {
		LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
	}

	mtc = *PERCPU_GET_MASTER(mbuf_mtypes);
	percpu_foreach_secondary(mtype, mbuf_mtypes) {
		for (int n = 0; n < MT_MAX; n++) {
			mtc.cpu_mtypes[n] += mtype->cpu_mtypes[n];
		}
	}

	if (!locked) {
		lck_mtx_lock(mbuf_mlock);
	}
	for (int n = 0; n < MT_MAX; n++) {
		mbstat.m_mtypes[n] = mtc.cpu_mtypes[n];
	}
	if (!locked) {
		lck_mtx_unlock(mbuf_mlock);
	}
}

static int
mbstat_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	mbuf_mtypes_sync(FALSE);

	return SYSCTL_OUT(req, &mbstat, sizeof(mbstat));
}

static void
mbuf_stat_sync(void)
{
	mb_class_stat_t *sp;
	mcache_cpu_t *ccp;
	mcache_t *cp;
	int k, m, bktsize;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	for (k = 0; k < NELEM(mbuf_table); k++) {
		cp = m_cache(k);
		ccp = &cp->mc_cpu[0];
		bktsize = ccp->cc_bktsize;
		sp = mbuf_table[k].mtbl_stats;

		if (cp->mc_flags & MCF_NOCPUCACHE) {
			sp->mbcl_mc_state = MCS_DISABLED;
		} else if (cp->mc_purge_cnt > 0) {
			sp->mbcl_mc_state = MCS_PURGING;
		} else if (bktsize == 0) {
			sp->mbcl_mc_state = MCS_OFFLINE;
		} else {
			sp->mbcl_mc_state = MCS_ONLINE;
		}

		sp->mbcl_mc_cached = 0;
		for (m = 0; m < ncpu; m++) {
			ccp = &cp->mc_cpu[m];
			if (ccp->cc_objs > 0) {
				sp->mbcl_mc_cached += ccp->cc_objs;
			}
			if (ccp->cc_pobjs > 0) {
				sp->mbcl_mc_cached += ccp->cc_pobjs;
			}
		}
		sp->mbcl_mc_cached += (cp->mc_full.bl_total * bktsize);
		sp->mbcl_active = sp->mbcl_total - sp->mbcl_mc_cached -
		    sp->mbcl_infree;

		sp->mbcl_mc_waiter_cnt = cp->mc_waiter_cnt;
		sp->mbcl_mc_wretry_cnt = cp->mc_wretry_cnt;
		sp->mbcl_mc_nwretry_cnt = cp->mc_nwretry_cnt;

		                                                  
		sp->mbcl_ctotal = sp->mbcl_total;
		switch (m_class(k)) {
		case MC_MBUF:
			                                           
			sp->mbcl_ctotal -= (m_total(MC_MBUF_CL) +
			    m_total(MC_MBUF_BIGCL));
			break;

		case MC_CL:
			                                             
			sp->mbcl_ctotal -= m_total(MC_MBUF_CL);
			break;

		case MC_BIGCL:
			                                             
			sp->mbcl_ctotal -= m_total(MC_MBUF_BIGCL);
			break;

		case MC_16KCL:
			                                             
			sp->mbcl_ctotal -= m_total(MC_MBUF_16KCL);
			break;

		default:
			break;
		}
	}
}

static int
mb_stat_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	void *statp;
	int k, statsz, proc64 = proc_is64bit(req->p);

	lck_mtx_lock(mbuf_mlock);
	mbuf_stat_sync();

	if (!proc64) {
		struct omb_class_stat *oc;
		struct mb_class_stat *c;

		omb_stat->mbs_cnt = mb_stat->mbs_cnt;
		oc = &omb_stat->mbs_class[0];
		c = &mb_stat->mbs_class[0];
		for (k = 0; k < omb_stat->mbs_cnt; k++, oc++, c++) {
			(void) snprintf(oc->mbcl_cname, sizeof(oc->mbcl_cname),
			    "%s", c->mbcl_cname);
			oc->mbcl_size = c->mbcl_size;
			oc->mbcl_total = c->mbcl_total;
			oc->mbcl_active = c->mbcl_active;
			oc->mbcl_infree = c->mbcl_infree;
			oc->mbcl_slab_cnt = c->mbcl_slab_cnt;
			oc->mbcl_alloc_cnt = c->mbcl_alloc_cnt;
			oc->mbcl_free_cnt = c->mbcl_free_cnt;
			oc->mbcl_notified = c->mbcl_notified;
			oc->mbcl_purge_cnt = c->mbcl_purge_cnt;
			oc->mbcl_fail_cnt = c->mbcl_fail_cnt;
			oc->mbcl_ctotal = c->mbcl_ctotal;
			oc->mbcl_release_cnt = c->mbcl_release_cnt;
			oc->mbcl_mc_state = c->mbcl_mc_state;
			oc->mbcl_mc_cached = c->mbcl_mc_cached;
			oc->mbcl_mc_waiter_cnt = c->mbcl_mc_waiter_cnt;
			oc->mbcl_mc_wretry_cnt = c->mbcl_mc_wretry_cnt;
			oc->mbcl_mc_nwretry_cnt = c->mbcl_mc_nwretry_cnt;
		}
		statp = omb_stat;
		statsz = OMB_STAT_SIZE(NELEM(mbuf_table));
	} else {
		statp = mb_stat;
		statsz = MB_STAT_SIZE(NELEM(mbuf_table));
	}

	lck_mtx_unlock(mbuf_mlock);

	return SYSCTL_OUT(req, statp, statsz);
}

static int
mleak_top_trace_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int i;

	                                   
	if (!mclfindleak || !mclexpleak) {
		return ENXIO;
	}

	lck_mtx_lock(mleak_lock);
	mleak_update_stats();
	i = SYSCTL_OUT(req, mleak_stat, MLEAK_STAT_SIZE(MLEAK_NUM_TRACES));
	lck_mtx_unlock(mleak_lock);

	return i;
}

static int
mleak_table_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int i = 0;

	                                   
	if (!mclfindleak || !mclexpleak) {
		return ENXIO;
	}

	lck_mtx_lock(mleak_lock);
	i = SYSCTL_OUT(req, &mleak_table, sizeof(mleak_table));
	lck_mtx_unlock(mleak_lock);

	return i;
}

static inline void
m_incref(struct mbuf *m)
{
	UInt16 old, new;
	volatile UInt16 *addr = (volatile UInt16 *)&MEXT_REF(m);

	do {
		old = *addr;
		new = old + 1;
		VERIFY(new != 0);
	} while (!OSCompareAndSwap16(old, new, addr));

	  
                                                              
                                                              
                                                          
    
	if (new > (MEXT_MINREF(m) + 1) && !(MEXT_FLAGS(m) & EXTF_READONLY)) {
		(void) OSBitOrAtomic16(EXTF_READONLY, &MEXT_FLAGS(m));
	}
}

static inline u_int16_t
m_decref(struct mbuf *m)
{
	UInt16 old, new;
	volatile UInt16 *addr = (volatile UInt16 *)&MEXT_REF(m);

	do {
		old = *addr;
		new = old - 1;
		VERIFY(old != 0);
	} while (!OSCompareAndSwap16(old, new, addr));

	return new;
}

static void
mbuf_table_init(void)
{
	unsigned int b, c, s;
	int m, config_mbuf_jumbo = 0;

	omb_stat = zalloc_permanent(OMB_STAT_SIZE(NELEM(mbuf_table)),
	    ZALIGN(struct omb_stat));

	mb_stat = zalloc_permanent(MB_STAT_SIZE(NELEM(mbuf_table)),
	    ZALIGN(mb_stat_t));

	mb_stat->mbs_cnt = NELEM(mbuf_table);
	for (m = 0; m < NELEM(mbuf_table); m++) {
		mbuf_table[m].mtbl_stats = &mb_stat->mbs_class[m];
	}

#if CONFIG_MBUF_JUMBO
	config_mbuf_jumbo = 1;
#endif                        

	if (config_mbuf_jumbo == 1 || PAGE_SIZE == M16KCLBYTES) {
		  
                                                    
                                                       
                             
     
		njcl = nmbclusters / 3;
		njclbytes = M16KCLBYTES;
	}

	  
                                                              
                               
    
	nclusters = P2ROUNDDOWN(nmbclusters - njcl, NCLPG);
	if (njcl > 0) {
		  
                                                     
                                                      
                                                
     
		njcl = P2ROUNDDOWN(nmbclusters - nclusters, NCLPJCL);

		                                                      
		nclusters = P2ROUNDDOWN(nmbclusters - njcl, NCLPG);
	}

	  
                                                               
                                                              
                                                            
                                                                
                                                                 
                                                               
             
   
                                                               
                                                               
                                                               
   
                                                                
                           
    
	c = P2ROUNDDOWN((nclusters >> 6), NCLPG);                        
	b = P2ROUNDDOWN((nclusters >> (6 + NCLPBGSHIFT)), NBCLPG);                  
	s = nclusters - (c + (b << NCLPBGSHIFT));                        

	  
                                            
    
	m_minlimit(MC_CL) = c;
	m_maxlimit(MC_CL) = s + c;                                       
	m_maxsize(MC_CL) = m_size(MC_CL) = MCLBYTES;
	(void) snprintf(m_cname(MC_CL), MAX_MBUF_CNAME, "cl");

	  
                                                               
                                                   
    
	m_minlimit(MC_BIGCL) = b;
	m_maxlimit(MC_BIGCL) = (s >> NCLPBGSHIFT) + b;                   
	m_maxsize(MC_BIGCL) = m_size(MC_BIGCL) = MBIGCLBYTES;
	(void) snprintf(m_cname(MC_BIGCL), MAX_MBUF_CNAME, "bigcl");

	  
                                                                   
    
	m_minlimit(MC_MBUF) = 0;
	m_maxlimit(MC_MBUF) = (s << NMBPCLSHIFT);                         
	m_maxsize(MC_MBUF) = m_size(MC_MBUF) = MSIZE;
	(void) snprintf(m_cname(MC_MBUF), MAX_MBUF_CNAME, "mbuf");

	  
                                         
    
	m_minlimit(MC_MBUF_CL) = 0;
	m_maxlimit(MC_MBUF_CL) = m_maxlimit(MC_CL);
	m_maxsize(MC_MBUF_CL) = MCLBYTES;
	m_size(MC_MBUF_CL) = m_size(MC_MBUF) + m_size(MC_CL);
	(void) snprintf(m_cname(MC_MBUF_CL), MAX_MBUF_CNAME, "mbuf_cl");

	m_minlimit(MC_MBUF_BIGCL) = 0;
	m_maxlimit(MC_MBUF_BIGCL) = m_maxlimit(MC_BIGCL);
	m_maxsize(MC_MBUF_BIGCL) = MBIGCLBYTES;
	m_size(MC_MBUF_BIGCL) = m_size(MC_MBUF) + m_size(MC_BIGCL);
	(void) snprintf(m_cname(MC_MBUF_BIGCL), MAX_MBUF_CNAME, "mbuf_bigcl");

	  
                          
    
	m_minlimit(MC_16KCL) = 0;
	m_maxlimit(MC_16KCL) = (njcl >> NCLPJCLSHIFT);                    
	m_maxsize(MC_16KCL) = m_size(MC_16KCL) = M16KCLBYTES;
	(void) snprintf(m_cname(MC_16KCL), MAX_MBUF_CNAME, "16kcl");

	m_minlimit(MC_MBUF_16KCL) = 0;
	m_maxlimit(MC_MBUF_16KCL) = m_maxlimit(MC_16KCL);
	m_maxsize(MC_MBUF_16KCL) = M16KCLBYTES;
	m_size(MC_MBUF_16KCL) = m_size(MC_MBUF) + m_size(MC_16KCL);
	(void) snprintf(m_cname(MC_MBUF_16KCL), MAX_MBUF_CNAME, "mbuf_16kcl");

	  
                                           
    
	bzero(&mbstat, sizeof(mbstat));
	mbstat.m_msize = m_maxsize(MC_MBUF);
	mbstat.m_mclbytes = m_maxsize(MC_CL);
	mbstat.m_minclsize = MINCLSIZE;
	mbstat.m_mlen = MLEN;
	mbstat.m_mhlen = MHLEN;
	mbstat.m_bigmclbytes = m_maxsize(MC_BIGCL);
}

int
mbuf_get_class(struct mbuf *m)
{
	if (m->m_flags & M_EXT) {
		uint32_t composite = (MEXT_FLAGS(m) & EXTF_COMPOSITE);
		m_ext_free_func_t m_free_func = m_get_ext_free(m);

		if (m_free_func == NULL) {
			if (composite) {
				return MC_MBUF_CL;
			} else {
				return MC_CL;
			}
		} else if (m_free_func == m_bigfree) {
			if (composite) {
				return MC_MBUF_BIGCL;
			} else {
				return MC_BIGCL;
			}
		} else if (m_free_func == m_16kfree) {
			if (composite) {
				return MC_MBUF_16KCL;
			} else {
				return MC_16KCL;
			}
		}
	}

	return MC_MBUF;
}

bool
mbuf_class_under_pressure(struct mbuf *m)
{
	int mclass = mbuf_get_class(m);

	if (m_total(mclass) - m_infree(mclass) >= (m_maxlimit(mclass) * mb_memory_pressure_percentage) / 100) {
		  
                                                                       
                                                            
                                                                 
                                   
    
                                                                        
                                                                
                                                                 
                                                              
     
		mcache_t *cp = m_cache(mclass);
		mcache_cpu_t *ccp = &cp->mc_cpu[0];

		int bktsize = os_access_once(ccp->cc_bktsize);
		uint32_t bl_total = os_access_once(cp->mc_full.bl_total);
		uint32_t cached = 0;
		int i;

		for (i = 0; i < ncpu; i++) {
			ccp = &cp->mc_cpu[i];

			int cc_objs = os_access_once(ccp->cc_objs);
			if (cc_objs > 0) {
				cached += cc_objs;
			}

			int cc_pobjs = os_access_once(ccp->cc_pobjs);
			if (cc_pobjs > 0) {
				cached += cc_pobjs;
			}
		}
		cached += (bl_total * bktsize);

		if (m_total(mclass) - m_infree(mclass) - cached >= (m_maxlimit(mclass) * mb_memory_pressure_percentage) / 100) {
			os_log(OS_LOG_DEFAULT,
			    "%s memory-pressure on mbuf due to class %u, total %u free %u cached %u max %u",
			    __func__, mclass, m_total(mclass), m_infree(mclass), cached, m_maxlimit(mclass));
			return true;
		}
	}

	return false;
}

#if defined(__LP64__)
typedef struct ncl_tbl {
	uint64_t nt_maxmem;                             
	uint32_t nt_mbpool;                         
} ncl_tbl_t;

static const ncl_tbl_t ncl_table[] = {
	{ (1ULL << GBSHIFT)            , (64 << MBSHIFT)              },
	{ (1ULL << (GBSHIFT + 2))            , (96 << MBSHIFT)              },
	{ (1ULL << (GBSHIFT + 3))           , (128 << MBSHIFT)              },
	{ (1ULL << (GBSHIFT + 4))            , (256 << MBSHIFT)              },
	{ (1ULL << (GBSHIFT + 5))            , (512 << MBSHIFT)              },
	{ 0, 0 }
};
#endif               

__private_extern__ unsigned int
mbuf_default_ncl(uint64_t mem)
{
#if !defined(__LP64__)
	unsigned int n;
	  
                                                                
    
	if ((n = ((mem / 16) / MCLBYTES)) > 32768) {
		n = 32768;
	}
#else
	unsigned int n, i;
	  
                                                  
    
	n = ncl_table[0].nt_mbpool;
	for (i = 0; ncl_table[i].nt_mbpool != 0; i++) {
		if (mem < ncl_table[i].nt_maxmem) {
			break;
		}
		n = ncl_table[i].nt_mbpool;
	}
	n >>= MCLSHIFT;
#endif                
	return n;
}

__private_extern__ void
mbinit(void)
{
	unsigned int m;
	unsigned int initmcl = 0;
	thread_t thread = THREAD_NULL;

	microuptime(&mb_start);

	  
                                                                   
    
	_CASSERT(MBUF_EXT == M_EXT);
	_CASSERT(MBUF_PKTHDR == M_PKTHDR);
	_CASSERT(MBUF_EOR == M_EOR);
	_CASSERT(MBUF_LOOP == M_LOOP);
	_CASSERT(MBUF_BCAST == M_BCAST);
	_CASSERT(MBUF_MCAST == M_MCAST);
	_CASSERT(MBUF_FRAG == M_FRAG);
	_CASSERT(MBUF_FIRSTFRAG == M_FIRSTFRAG);
	_CASSERT(MBUF_LASTFRAG == M_LASTFRAG);
	_CASSERT(MBUF_PROMISC == M_PROMISC);
	_CASSERT(MBUF_HASFCS == M_HASFCS);

	_CASSERT(MBUF_TYPE_FREE == MT_FREE);
	_CASSERT(MBUF_TYPE_DATA == MT_DATA);
	_CASSERT(MBUF_TYPE_HEADER == MT_HEADER);
	_CASSERT(MBUF_TYPE_SOCKET == MT_SOCKET);
	_CASSERT(MBUF_TYPE_PCB == MT_PCB);
	_CASSERT(MBUF_TYPE_RTABLE == MT_RTABLE);
	_CASSERT(MBUF_TYPE_HTABLE == MT_HTABLE);
	_CASSERT(MBUF_TYPE_ATABLE == MT_ATABLE);
	_CASSERT(MBUF_TYPE_SONAME == MT_SONAME);
	_CASSERT(MBUF_TYPE_SOOPTS == MT_SOOPTS);
	_CASSERT(MBUF_TYPE_FTABLE == MT_FTABLE);
	_CASSERT(MBUF_TYPE_RIGHTS == MT_RIGHTS);
	_CASSERT(MBUF_TYPE_IFADDR == MT_IFADDR);
	_CASSERT(MBUF_TYPE_CONTROL == MT_CONTROL);
	_CASSERT(MBUF_TYPE_OOBDATA == MT_OOBDATA);

	_CASSERT(MBUF_TSO_IPV4 == CSUM_TSO_IPV4);
	_CASSERT(MBUF_TSO_IPV6 == CSUM_TSO_IPV6);
	_CASSERT(MBUF_CSUM_REQ_SUM16 == CSUM_PARTIAL);
	_CASSERT(MBUF_CSUM_TCP_SUM16 == MBUF_CSUM_REQ_SUM16);
	_CASSERT(MBUF_CSUM_REQ_ZERO_INVERT == CSUM_ZERO_INVERT);
	_CASSERT(MBUF_CSUM_REQ_IP == CSUM_IP);
	_CASSERT(MBUF_CSUM_REQ_TCP == CSUM_TCP);
	_CASSERT(MBUF_CSUM_REQ_UDP == CSUM_UDP);
	_CASSERT(MBUF_CSUM_REQ_TCPIPV6 == CSUM_TCPIPV6);
	_CASSERT(MBUF_CSUM_REQ_UDPIPV6 == CSUM_UDPIPV6);
	_CASSERT(MBUF_CSUM_DID_IP == CSUM_IP_CHECKED);
	_CASSERT(MBUF_CSUM_IP_GOOD == CSUM_IP_VALID);
	_CASSERT(MBUF_CSUM_DID_DATA == CSUM_DATA_VALID);
	_CASSERT(MBUF_CSUM_PSEUDO_HDR == CSUM_PSEUDO_HDR);

	_CASSERT(MBUF_WAITOK == M_WAIT);
	_CASSERT(MBUF_DONTWAIT == M_DONTWAIT);
	_CASSERT(MBUF_COPYALL == M_COPYALL);

	_CASSERT(MBUF_SC2TC(MBUF_SC_BK_SYS) == MBUF_TC_BK);
	_CASSERT(MBUF_SC2TC(MBUF_SC_BK) == MBUF_TC_BK);
	_CASSERT(MBUF_SC2TC(MBUF_SC_BE) == MBUF_TC_BE);
	_CASSERT(MBUF_SC2TC(MBUF_SC_RD) == MBUF_TC_BE);
	_CASSERT(MBUF_SC2TC(MBUF_SC_OAM) == MBUF_TC_BE);
	_CASSERT(MBUF_SC2TC(MBUF_SC_AV) == MBUF_TC_VI);
	_CASSERT(MBUF_SC2TC(MBUF_SC_RV) == MBUF_TC_VI);
	_CASSERT(MBUF_SC2TC(MBUF_SC_VI) == MBUF_TC_VI);
	_CASSERT(MBUF_SC2TC(MBUF_SC_SIG) == MBUF_TC_VI);
	_CASSERT(MBUF_SC2TC(MBUF_SC_VO) == MBUF_TC_VO);
	_CASSERT(MBUF_SC2TC(MBUF_SC_CTL) == MBUF_TC_VO);

	_CASSERT(MBUF_TC2SCVAL(MBUF_TC_BK) == SCVAL_BK);
	_CASSERT(MBUF_TC2SCVAL(MBUF_TC_BE) == SCVAL_BE);
	_CASSERT(MBUF_TC2SCVAL(MBUF_TC_VI) == SCVAL_VI);
	_CASSERT(MBUF_TC2SCVAL(MBUF_TC_VO) == SCVAL_VO);

	                                                                  
	_CASSERT(!(offsetof(struct mbuf, m_pkthdr.pkt_mpriv) %
	    sizeof(uint32_t)));

	                                               
	_CASSERT((offsetof(struct mbuf, m_pktdat) % 16) == 0);

	                                             
	_CASSERT(sizeof(mb_redzone_cookie) ==
	    sizeof(((struct pkthdr *)0)->redzone));
	read_random(&mb_redzone_cookie, sizeof(mb_redzone_cookie));
	read_random(&mb_obscure_extref, sizeof(mb_obscure_extref));
	read_random(&mb_obscure_extfree, sizeof(mb_obscure_extfree));
	mb_obscure_extref |= 0x3;
	mb_obscure_extfree |= 0x3;

	                                                 
	_CASSERT(MCA_SAVED_MBUF_SIZE <= sizeof(struct mbuf));

	if (nmbclusters == 0) {
		nmbclusters = NMBCLUSTERS;
	}

	                                                        
	VERIFY(nmbclusters != 0 && !(nmbclusters & 0x1));

	                          
	mbuf_table_init();

	  
                                 
   
                                           
   
                                                                      
                                                             
    
	maxslabgrp =
	    (P2ROUNDUP(nmbclusters, (MBSIZE >> MCLSHIFT)) << MCLSHIFT) >> MBSHIFT;
	slabstbl = zalloc_permanent(maxslabgrp * sizeof(mcl_slabg_t *),
	    ZALIGN(mcl_slabg_t));

	  
                                         
   
                                                       
   
                                                                
    
	PE_parse_boot_argn("mbuf_debug", &mbuf_debug, sizeof(mbuf_debug));
	mbuf_debug |= mcache_getflags();
	if (mbuf_debug & MCF_DEBUG) {
		int l;
		mcl_audit_t *mclad;
		maxclaudit = ((maxslabgrp << MBSHIFT) >> PAGE_SHIFT);
		mclaudit = zalloc_permanent(maxclaudit * sizeof(*mclaudit),
		    ZALIGN(mcl_audit_t));
		for (l = 0, mclad = mclaudit; l < maxclaudit; l++) {
			mclad[l].cl_audit = zalloc_permanent(NMBPG * sizeof(mcache_audit_t *),
			    ZALIGN_PTR);
		}

		mcl_audit_con_cache = mcache_create("mcl_audit_contents",
		    AUDIT_CONTENTS_SIZE, sizeof(u_int64_t), 0, MCR_SLEEP);
		VERIFY(mcl_audit_con_cache != NULL);
	}
	mclverify = (mbuf_debug & MCF_VERIFY);
	mcltrace = (mbuf_debug & MCF_TRACE);
	mclfindleak = !(mbuf_debug & MCF_NOLEAKLOG);
	mclexpleak = mclfindleak && (mbuf_debug & MCF_EXPLEAKLOG);

	                                                                 

	mleak_activate();

	  
                                                            
                                                              
                                                           
                                  
    
	ncpu = ml_wait_max_cpus();

	                                                                
	mcl_pages = (nmbclusters << MCLSHIFT) / PAGE_SIZE;
	mcl_paddr = zalloc_permanent(mcl_pages * sizeof(ppnum_t),
	    ZALIGN(ppnum_t));

	                                      
	mcl_paddr_base = IOMapperIOVMAlloc(mcl_pages);

	embutl = (mbutl + (nmbclusters * MCLBYTES));
	VERIFY(((embutl - mbutl) % MBIGCLBYTES) == 0);

	                           
	PE_parse_boot_argn("initmcl", &initmcl, sizeof(initmcl));
	if (initmcl != 0) {
		initmcl >>= NCLPBGSHIFT;                              
		if (initmcl > m_maxlimit(MC_BIGCL)) {
			initmcl = m_maxlimit(MC_BIGCL);
		}
	}
	if (initmcl < m_minlimit(MC_BIGCL)) {
		initmcl = m_minlimit(MC_BIGCL);
	}

	lck_mtx_lock(mbuf_mlock);

	  
                                                                      
                                                         
    
	VERIFY(m_total(MC_BIGCL) == 0 && m_minlimit(MC_BIGCL) != 0);
	freelist_populate(m_class(MC_BIGCL), initmcl, M_WAIT);
	VERIFY(m_total(MC_BIGCL) >= m_minlimit(MC_BIGCL));
	freelist_init(m_class(MC_CL));

	for (m = 0; m < NELEM(mbuf_table); m++) {
		                                  
		VERIFY(m_minlimit(m_class(m)) == 0 ||
		    m_total(m_class(m)) >= m_minlimit(m_class(m)));

		                                                         
		m_peak(m_class(m)) = m_total(m_class(m));
	}
	mb_peak_newreport = FALSE;

	lck_mtx_unlock(mbuf_mlock);

	(void) kernel_thread_start((thread_continue_t)mbuf_worker_thread_init,
	    NULL, &thread);
	thread_deallocate(thread);

	ref_cache = mcache_create("mext_ref", sizeof(struct ext_ref),
	    0, 0, MCR_SLEEP);

	                                     
	for (m = 0; m < NELEM(mbuf_table); m++) {
		void *allocfunc, *freefunc, *auditfunc, *logfunc;
		u_int32_t flags;

		flags = mbuf_debug;
		if (m_class(m) == MC_MBUF_CL || m_class(m) == MC_MBUF_BIGCL ||
		    m_class(m) == MC_MBUF_16KCL) {
			allocfunc = mbuf_cslab_alloc;
			freefunc = mbuf_cslab_free;
			auditfunc = mbuf_cslab_audit;
			logfunc = mleak_logger;
		} else {
			allocfunc = mbuf_slab_alloc;
			freefunc = mbuf_slab_free;
			auditfunc = mbuf_slab_audit;
			logfunc = mleak_logger;
		}

		  
                                                      
                                                      
                                                      
                                                
     
		if ((m_class(m) == MC_MBUF_16KCL || m_class(m) == MC_16KCL) &&
		    njcl == 0) {
			flags |= MCF_NOCPUCACHE;
		}

		if (!mclfindleak) {
			flags |= MCF_NOLEAKLOG;
		}

		m_cache(m) = mcache_create_ext(m_cname(m), m_maxsize(m),
		    allocfunc, freefunc, auditfunc, logfunc, mbuf_slab_notify,
		    (void *)(uintptr_t)m, flags, MCR_SLEEP);
	}

	  
                                                            
                                       
    
	high_sb_max = (nmbclusters << (MCLSHIFT - 4));
	if (high_sb_max < sb_max) {
		                                                               
		if (high_sb_max > (1 << MBSHIFT)) {
			                                       
			sb_max = high_sb_max;
		} else if ((nmbclusters << MCLSHIFT) > (1 << MBSHIFT)) {
			  
                                                          
                        
      
			sb_max = high_sb_max = (1 << MBSHIFT);
		} else {
			sb_max = high_sb_max;
		}
	}

	                                      
	mbuf_dump_buf = zalloc_permanent(MBUF_DUMP_BUF_SIZE, ZALIGN_NONE);

	if (mbuf_debug & MCF_DEBUG) {
		printf("%s: MLEN %d, MHLEN %d\n", __func__,
		    (int)_MLEN, (int)_MHLEN);
	}

	printf("%s: done [%d MB total pool size, (%d/%d) split]\n", __func__,
	    (nmbclusters << MCLSHIFT) >> MBSHIFT,
	    (nclusters << MCLSHIFT) >> MBSHIFT,
	    (njcl << MCLSHIFT) >> MBSHIFT);
}

  
                                                        
   
static mcache_obj_t *
slab_alloc(mbuf_class_t class, int wait)
{
	mcl_slab_t *sp;
	mcache_obj_t *buf;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	                                       
	VERIFY(m_cobjlist(class) == NULL);

	  
                                                              
                                                                
                                                               
                                                             
                                                           
    
	if ((class == MC_MBUF || class == MC_CL || class == MC_BIGCL)
	    && (wait & MCR_COMP)) {
		sp = (mcl_slab_t *)TAILQ_LAST(&m_slablist(class), mcl_slhead);
	} else {
		sp = (mcl_slab_t *)TAILQ_FIRST(&m_slablist(class));
	}

	if (sp == NULL) {
		VERIFY(m_infree(class) == 0 && m_slab_cnt(class) == 0);
		                                           
		return NULL;
	}

	VERIFY(m_infree(class) > 0);
	VERIFY(!slab_is_detached(sp));
	VERIFY(sp->sl_class == class &&
	    (sp->sl_flags & (SLF_MAPPED | SLF_PARTIAL)) == SLF_MAPPED);
	buf = sp->sl_head;
	VERIFY(slab_inrange(sp, buf) && sp == slab_get(buf));
	sp->sl_head = buf->obj_next;
	                              
	sp->sl_refcnt++;

	VERIFY(sp->sl_head != NULL || sp->sl_refcnt == sp->sl_chunks);

	if (sp->sl_head != NULL && !slab_inrange(sp, sp->sl_head)) {
		slab_nextptr_panic(sp, sp->sl_head);
		                                                       
		VERIFY(slab_inrange(sp, sp->sl_head));
		                
	}

	if (mclaudit != NULL) {
		mcache_audit_t *mca = mcl_audit_buf2mca(class, buf);
		mca->mca_uflags = 0;
		                                        
		if (class == MC_MBUF) {
			mca->mca_uflags |= MB_SCVALID;
		}
	}

	if (class == MC_CL) {
		mbstat.m_clfree = (--m_infree(MC_CL)) + m_infree(MC_MBUF_CL);
		  
                                                         
     
		VERIFY(sp->sl_refcnt >= 1 && sp->sl_refcnt <= NCLPG &&
		    sp->sl_chunks == NCLPG && sp->sl_len == PAGE_SIZE);
		VERIFY(sp->sl_refcnt < NCLPG || sp->sl_head == NULL);
	} else if (class == MC_BIGCL) {
		mbstat.m_bigclfree = (--m_infree(MC_BIGCL)) +
		    m_infree(MC_MBUF_BIGCL);
		  
                                                  
     
		VERIFY(sp->sl_refcnt >= 1 && sp->sl_chunks == NBCLPG &&
		    sp->sl_len == PAGE_SIZE &&
		    (sp->sl_refcnt < NBCLPG || sp->sl_head == NULL));
	} else if (class == MC_16KCL) {
		mcl_slab_t *nsp;
		int k;

		--m_infree(MC_16KCL);
		VERIFY(sp->sl_refcnt == 1 && sp->sl_chunks == 1 &&
		    sp->sl_len == m_maxsize(class) && sp->sl_head == NULL);
		  
                                                              
                                                               
                      
     
		for (nsp = sp, k = 1; k < NSLABSP16KB; k++) {
			nsp = nsp->sl_next;
			                                       
			VERIFY(nsp != NULL);
			nsp->sl_refcnt++;
			VERIFY(!slab_is_detached(nsp));
			VERIFY(nsp->sl_class == MC_16KCL &&
			    nsp->sl_flags == (SLF_MAPPED | SLF_PARTIAL) &&
			    nsp->sl_refcnt == 1 && nsp->sl_chunks == 0 &&
			    nsp->sl_len == 0 && nsp->sl_base == sp->sl_base &&
			    nsp->sl_head == NULL);
		}
	} else {
		VERIFY(class == MC_MBUF);
		--m_infree(MC_MBUF);
		  
                                            
                                               
     
		if (mclaudit == NULL) {
			_MCHECK((struct mbuf *)buf);
		}
		  
                                                         
                                                           
                                                         
                             
     
		VERIFY(sp->sl_refcnt >= 1 && sp->sl_refcnt <= NMBPG &&
		    sp->sl_chunks == NMBPG &&
		    sp->sl_len == PAGE_SIZE);
		VERIFY(sp->sl_refcnt < NMBPG || sp->sl_head == NULL);
	}

	                                                          
	if (sp->sl_head == NULL) {
		VERIFY(class != MC_MBUF || sp->sl_refcnt == NMBPG);
		VERIFY(class != MC_CL || sp->sl_refcnt == NCLPG);
		VERIFY(class != MC_BIGCL || sp->sl_refcnt == NBCLPG);
		slab_remove(sp, class);
	}

	return buf;
}

  
                                                           
   
static void
slab_free(mbuf_class_t class, mcache_obj_t *buf)
{
	mcl_slab_t *sp;
	boolean_t reinit_supercl = false;
	mbuf_class_t super_class;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	VERIFY(class != MC_16KCL || njcl > 0);
	VERIFY(buf->obj_next == NULL);

	  
                                                                    
                          
    
	while (mb_clalloc_busy) {
		mb_clalloc_waiters++;
		(void) msleep(mb_clalloc_waitchan, mbuf_mlock,
		    (PZERO - 1), "m_clalloc", NULL);
		LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
	}

	                                                    
	mb_clalloc_busy = TRUE;

	sp = slab_get(buf);
	VERIFY(sp->sl_class == class && slab_inrange(sp, buf) &&
	    (sp->sl_flags & (SLF_MAPPED | SLF_PARTIAL)) == SLF_MAPPED);

	                              
	sp->sl_refcnt--;

	if (class == MC_CL) {
		VERIFY(IS_P2ALIGNED(buf, MCLBYTES));
		  
                                                            
                                                   
     
		VERIFY(sp->sl_refcnt >= 0 && sp->sl_refcnt <= (NCLPG - 1) &&
		    sp->sl_chunks == NCLPG && sp->sl_len == PAGE_SIZE);
		VERIFY(sp->sl_refcnt < (NCLPG - 1) ||
		    (slab_is_detached(sp) && sp->sl_head == NULL));
	} else if (class == MC_BIGCL) {
		VERIFY(IS_P2ALIGNED(buf, MBIGCLBYTES));

		                                                           
		VERIFY(sp->sl_refcnt >= 0 && sp->sl_chunks == NBCLPG);
		VERIFY(sp->sl_refcnt < (NBCLPG - 1) ||
		    (slab_is_detached(sp) && sp->sl_head == NULL));
	} else if (class == MC_16KCL) {
		mcl_slab_t *nsp;
		int k;
		  
                                                     
                          
     
		VERIFY(IS_P2ALIGNED(buf, PAGE_SIZE));
		VERIFY(sp->sl_refcnt == 0 && sp->sl_chunks == 1 &&
		    sp->sl_len == m_maxsize(class) && sp->sl_head == NULL);
		VERIFY(slab_is_detached(sp));
		for (nsp = sp, k = 1; k < NSLABSP16KB; k++) {
			nsp = nsp->sl_next;
			                                       
			VERIFY(nsp != NULL);
			nsp->sl_refcnt--;
			VERIFY(slab_is_detached(nsp));
			VERIFY(nsp->sl_class == MC_16KCL &&
			    (nsp->sl_flags & (SLF_MAPPED | SLF_PARTIAL)) &&
			    nsp->sl_refcnt == 0 && nsp->sl_chunks == 0 &&
			    nsp->sl_len == 0 && nsp->sl_base == sp->sl_base &&
			    nsp->sl_head == NULL);
		}
	} else {
		  
                                                        
                                                       
                                                      
             
     
		VERIFY(class == MC_MBUF);
		VERIFY(sp->sl_refcnt >= 0 &&
		    sp->sl_refcnt <= (NMBPG - 1) &&
		    sp->sl_chunks == NMBPG &&
		    sp->sl_len == PAGE_SIZE);
		VERIFY(sp->sl_refcnt < (NMBPG - 1) ||
		    (slab_is_detached(sp) && sp->sl_head == NULL));
	}

	  
                                                          
                                                          
                                 
    
	if (mclaudit != NULL) {
		mcache_audit_t *mca = mcl_audit_buf2mca(class, buf);
		if (mclverify) {
			mcache_audit_free_verify(mca, buf, 0,
			    m_maxsize(class));
		}
		mca->mca_uflags &= ~MB_SCVALID;
	}

	if (class == MC_CL) {
		mbstat.m_clfree = (++m_infree(MC_CL)) + m_infree(MC_MBUF_CL);
		buf->obj_next = sp->sl_head;
	} else if (class == MC_BIGCL) {
		mbstat.m_bigclfree = (++m_infree(MC_BIGCL)) +
		    m_infree(MC_MBUF_BIGCL);
		buf->obj_next = sp->sl_head;
	} else if (class == MC_16KCL) {
		++m_infree(MC_16KCL);
	} else {
		++m_infree(MC_MBUF);
		buf->obj_next = sp->sl_head;
	}
	sp->sl_head = buf;

	  
                                                                    
                                                               
                                                  
    
	if (m_maxsize(MC_BIGCL) == PAGE_SIZE) {
		super_class = MC_BIGCL;
	} else {
		VERIFY(PAGE_SIZE == m_maxsize(MC_16KCL));
		super_class = MC_16KCL;
	}
	if (class == MC_MBUF && sp->sl_refcnt == 0 &&
	    m_total(class) >= (m_minlimit(class) + NMBPG) &&
	    m_total(super_class) < m_maxlimit(super_class)) {
		int i = NMBPG;

		m_total(MC_MBUF) -= NMBPG;
		mbstat.m_mbufs = m_total(MC_MBUF);
		m_infree(MC_MBUF) -= NMBPG;
		mtype_stat_add(MT_FREE, -((unsigned)NMBPG));

		while (i--) {
			struct mbuf *m = sp->sl_head;
			VERIFY(m != NULL);
			sp->sl_head = m->m_next;
			m->m_next = NULL;
		}
		reinit_supercl = true;
	} else if (class == MC_CL && sp->sl_refcnt == 0 &&
	    m_total(class) >= (m_minlimit(class) + NCLPG) &&
	    m_total(super_class) < m_maxlimit(super_class)) {
		int i = NCLPG;

		m_total(MC_CL) -= NCLPG;
		mbstat.m_clusters = m_total(MC_CL);
		m_infree(MC_CL) -= NCLPG;

		while (i--) {
			union mcluster *c = sp->sl_head;
			VERIFY(c != NULL);
			sp->sl_head = c->mcl_next;
			c->mcl_next = NULL;
		}
		reinit_supercl = true;
	} else if (class == MC_BIGCL && super_class != MC_BIGCL &&
	    sp->sl_refcnt == 0 &&
	    m_total(class) >= (m_minlimit(class) + NBCLPG) &&
	    m_total(super_class) < m_maxlimit(super_class)) {
		int i = NBCLPG;

		VERIFY(super_class == MC_16KCL);
		m_total(MC_BIGCL) -= NBCLPG;
		mbstat.m_bigclusters = m_total(MC_BIGCL);
		m_infree(MC_BIGCL) -= NBCLPG;

		while (i--) {
			union mbigcluster *bc = sp->sl_head;
			VERIFY(bc != NULL);
			sp->sl_head = bc->mbc_next;
			bc->mbc_next = NULL;
		}
		reinit_supercl = true;
	}

	if (reinit_supercl) {
		VERIFY(sp->sl_head == NULL);
		VERIFY(m_total(class) >= m_minlimit(class));
		slab_remove(sp, class);

		                                                      
		m_total(super_class)++;
		m_infree(super_class)++;
		VERIFY(sp->sl_flags == (SLF_MAPPED | SLF_DETACHED) &&
		    sp->sl_len == PAGE_SIZE && sp->sl_refcnt == 0);

		slab_init(sp, super_class, SLF_MAPPED, sp->sl_base,
		    sp->sl_base, PAGE_SIZE, 0, 1);
		if (mclverify) {
			mcache_set_pattern(MCACHE_FREE_PATTERN,
			    (caddr_t)sp->sl_base, sp->sl_len);
		}
		((mcache_obj_t *)(sp->sl_base))->obj_next = NULL;

		if (super_class == MC_BIGCL) {
			mbstat.m_bigclusters = m_total(MC_BIGCL);
			mbstat.m_bigclfree = m_infree(MC_BIGCL) +
			    m_infree(MC_MBUF_BIGCL);
		}

		VERIFY(slab_is_detached(sp));
		VERIFY(m_total(super_class) <= m_maxlimit(super_class));

		                              
		class = super_class;
	}

	                                                
	if (slab_is_detached(sp)) {
		slab_insert(sp, class);
	}

	                                  
	mb_clalloc_busy = FALSE;
	if (mb_clalloc_waiters > 0) {
		mb_clalloc_waiters = 0;
		wakeup(mb_clalloc_waitchan);
	}
}

  
                                                                         
                                                                             
                                                                             
                                                                          
                        
   
static unsigned int
mbuf_slab_alloc(void *arg, mcache_obj_t ***plist, unsigned int num, int wait)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	unsigned int need = num;
	mcache_obj_t **list = *plist;

	ASSERT(MBUF_CLASS_VALID(class) && !MBUF_CLASS_COMPOSITE(class));
	ASSERT(need > 0);

	lck_mtx_lock(mbuf_mlock);

	for (;;) {
		if ((*list = slab_alloc(class, wait)) != NULL) {
			(*list)->obj_next = NULL;
			list = *plist = &(*list)->obj_next;

			if (--need == 0) {
				  
                                                
                                                  
                                                  
                                            
       
				if (!mbuf_cached_above(class, wait) &&
				    m_infree(class) < (m_total(class) >> 5)) {
					(void) freelist_populate(class, 1,
					    M_DONTWAIT);
				}
				break;
			}
		} else {
			VERIFY(m_infree(class) == 0 || class == MC_CL);

			(void) freelist_populate(class, 1,
			    (wait & MCR_NOSLEEP) ? M_DONTWAIT : M_WAIT);

			if (m_infree(class) > 0) {
				continue;
			}

			                                                  
			if (mbuf_cached_above(class, wait)) {
				break;
			}

			                         
			mbuf_watchdog();

			                                               
			if (wait & MCR_NOSLEEP) {
				if (!(wait & MCR_TRYHARD)) {
					m_fail_cnt(class)++;
					mbstat.m_drops++;
					break;
				}
			}

			  
                                                      
                                                      
                                                   
                                                    
                                            
      
			if (mbuf_worker_ready &&
			    mbuf_sleep(class, need, wait)) {
				break;
			}

			LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
		}
	}

	m_alloc_cnt(class) += num - need;
	lck_mtx_unlock(mbuf_mlock);

	return num - need;
}

  
                                                                      
                                                                         
                   
   
static void
mbuf_slab_free(void *arg, mcache_obj_t *list, __unused int purged)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	mcache_obj_t *nlist;
	unsigned int num = 0;
	int w;

	ASSERT(MBUF_CLASS_VALID(class) && !MBUF_CLASS_COMPOSITE(class));

	lck_mtx_lock(mbuf_mlock);

	for (;;) {
		nlist = list->obj_next;
		list->obj_next = NULL;
		slab_free(class, list);
		++num;
		if ((list = nlist) == NULL) {
			break;
		}
	}
	m_free_cnt(class) += num;

	if ((w = mb_waiters) > 0) {
		mb_waiters = 0;
	}
	if (w) {
		mbwdog_logger("waking up all threads");
	}
	lck_mtx_unlock(mbuf_mlock);

	if (w != 0) {
		wakeup(mb_waitchan);
	}
}

  
                                                                       
                                                                        
                                                                      
                                                                       
                                                                     
                            
   
static void
mbuf_slab_audit(void *arg, mcache_obj_t *list, boolean_t alloc)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	mcache_audit_t *mca;

	ASSERT(MBUF_CLASS_VALID(class) && !MBUF_CLASS_COMPOSITE(class));

	while (list != NULL) {
		lck_mtx_lock(mbuf_mlock);
		mca = mcl_audit_buf2mca(class, list);

		                          
		if (class == MC_MBUF) {
			mcl_audit_mbuf(mca, list, FALSE, alloc);
			ASSERT(mca->mca_uflags & MB_SCVALID);
		} else {
			mcl_audit_cluster(mca, list, m_maxsize(class),
			    alloc, TRUE);
			ASSERT(!(mca->mca_uflags & MB_SCVALID));
		}
		                             
		if (mcltrace) {
			mcache_buffer_log(mca, list, m_cache(class), &mb_start);
		}

		if (alloc) {
			mca->mca_uflags |= MB_INUSE;
		} else {
			mca->mca_uflags &= ~MB_INUSE;
		}
		                                         
		mca->mca_uptr = NULL;
		lck_mtx_unlock(mbuf_mlock);

		list = list->obj_next;
	}
}

  
                                                                     
                                                                    
                                                                  
                       
   
static void
mbuf_slab_notify(void *arg, u_int32_t reason)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	int w;

	ASSERT(MBUF_CLASS_VALID(class));

	if (reason != MCN_RETRYALLOC) {
		return;
	}

	lck_mtx_lock(mbuf_mlock);
	if ((w = mb_waiters) > 0) {
		m_notified(class)++;
		mb_waiters = 0;
	}
	if (w) {
		mbwdog_logger("waking up all threads");
	}
	lck_mtx_unlock(mbuf_mlock);

	if (w != 0) {
		wakeup(mb_waitchan);
	}
}

  
                                                        
   
static unsigned int
cslab_alloc(mbuf_class_t class, mcache_obj_t ***plist, unsigned int num)
{
	unsigned int need = num;
	mcl_slab_t *sp, *clsp, *nsp;
	struct mbuf *m;
	mcache_obj_t **list = *plist;
	void *cl;

	VERIFY(need > 0);
	VERIFY(class != MC_MBUF_16KCL || njcl > 0);
	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	                                       
	while ((*list = m_cobjlist(class)) != NULL) {
		MRANGE(*list);

		m = (struct mbuf *)*list;
		sp = slab_get(m);
		cl = m->m_ext.ext_buf;
		clsp = slab_get(cl);
		VERIFY(m->m_flags == M_EXT && cl != NULL);
		VERIFY(m_get_rfa(m) != NULL && MBUF_IS_COMPOSITE(m));

		if (class == MC_MBUF_CL) {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NCLPG);
		} else {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NBCLPG);
		}

		if (class == MC_MBUF_16KCL) {
			int k;
			for (nsp = clsp, k = 1; k < NSLABSP16KB; k++) {
				nsp = nsp->sl_next;
				                                       
				VERIFY(nsp != NULL);
				VERIFY(nsp->sl_refcnt == 1);
			}
		}

		if ((m_cobjlist(class) = (*list)->obj_next) != NULL &&
		    !MBUF_IN_MAP(m_cobjlist(class))) {
			slab_nextptr_panic(sp, m_cobjlist(class));
			                
		}
		(*list)->obj_next = NULL;
		list = *plist = &(*list)->obj_next;

		if (--need == 0) {
			break;
		}
	}
	m_infree(class) -= (num - need);

	return num - need;
}

  
                                                          
   
static unsigned int
cslab_free(mbuf_class_t class, mcache_obj_t *list, int purged)
{
	mcache_obj_t *o, *tail;
	unsigned int num = 0;
	struct mbuf *m, *ms;
	mcache_audit_t *mca = NULL;
	mcache_obj_t *ref_list = NULL;
	mcl_slab_t *clsp, *nsp;
	void *cl;
	mbuf_class_t cl_class;

	ASSERT(MBUF_CLASS_VALID(class) && MBUF_CLASS_COMPOSITE(class));
	VERIFY(class != MC_MBUF_16KCL || njcl > 0);
	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	if (class == MC_MBUF_CL) {
		cl_class = MC_CL;
	} else if (class == MC_MBUF_BIGCL) {
		cl_class = MC_BIGCL;
	} else {
		VERIFY(class == MC_MBUF_16KCL);
		cl_class = MC_16KCL;
	}

	o = tail = list;

	while ((m = ms = (struct mbuf *)o) != NULL) {
		mcache_obj_t *rfa, *nexto = o->obj_next;

		                               
		if (mclaudit != NULL) {
			mca = mcl_audit_buf2mca(MC_MBUF, (mcache_obj_t *)m);
			if (mclverify) {
				mcache_audit_free_verify(mca, m, 0,
				    m_maxsize(MC_MBUF));
			}
			ms = MCA_SAVED_MBUF_PTR(mca);
		}

		                                  
		cl = ms->m_ext.ext_buf;
		clsp = slab_get(cl);
		if (mclverify) {
			size_t size = m_maxsize(cl_class);
			mcache_audit_free_verify(mcl_audit_buf2mca(cl_class,
			    (mcache_obj_t *)cl), cl, 0, size);
		}
		VERIFY(ms->m_type == MT_FREE);
		VERIFY(ms->m_flags == M_EXT);
		VERIFY(m_get_rfa(ms) != NULL && MBUF_IS_COMPOSITE(ms));
		if (cl_class == MC_CL) {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NCLPG);
		} else {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NBCLPG);
		}
		if (cl_class == MC_16KCL) {
			int k;
			for (nsp = clsp, k = 1; k < NSLABSP16KB; k++) {
				nsp = nsp->sl_next;
				                                       
				VERIFY(nsp != NULL);
				VERIFY(nsp->sl_refcnt == 1);
			}
		}

		  
                                                           
                                                              
                                                           
                                                                 
     
		if (purged) {
			                                     
			if (mclaudit != NULL) {
				mcl_audit_restore_mbuf(m, mca, TRUE);
			}

			MEXT_MINREF(m) = 0;
			MEXT_REF(m) = 0;
			MEXT_PREF(m) = 0;
			MEXT_FLAGS(m) = 0;
			MEXT_PRIV(m) = 0;
			MEXT_PMBUF(m) = NULL;
			MEXT_TOKEN(m) = 0;

			rfa = (mcache_obj_t *)(void *)m_get_rfa(m);
			m_set_ext(m, NULL, NULL, NULL);
			rfa->obj_next = ref_list;
			ref_list = rfa;

			m->m_type = MT_FREE;
			m->m_flags = m->m_len = 0;
			m->m_next = m->m_nextpkt = NULL;

			                                              
			if (mclaudit != NULL) {
				mcl_audit_mbuf(mca, o, FALSE, FALSE);
			}

			VERIFY(m_total(class) > 0);
			m_total(class)--;

			                   
			o->obj_next = NULL;
			slab_free(MC_MBUF, o);

			                          
			((mcache_obj_t *)cl)->obj_next = NULL;
			if (class == MC_MBUF_CL) {
				slab_free(MC_CL, cl);
			} else if (class == MC_MBUF_BIGCL) {
				slab_free(MC_BIGCL, cl);
			} else {
				slab_free(MC_16KCL, cl);
			}
		}

		++num;
		tail = o;
		o = nexto;
	}

	if (!purged) {
		tail->obj_next = m_cobjlist(class);
		m_cobjlist(class) = list;
		m_infree(class) += num;
	} else if (ref_list != NULL) {
		mcache_free_ext(ref_cache, ref_list);
	}

	return num;
}

  
                                                                       
                                                                         
                                                                        
                                                                          
                                                                         
                                         
   
static unsigned int
mbuf_cslab_alloc(void *arg, mcache_obj_t ***plist, unsigned int needed,
    int wait)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	mbuf_class_t cl_class = 0;
	unsigned int num = 0, cnum = 0, want = needed;
	mcache_obj_t *ref_list = NULL;
	mcache_obj_t *mp_list = NULL;
	mcache_obj_t *clp_list = NULL;
	mcache_obj_t **list;
	struct ext_ref *rfa;
	struct mbuf *m;
	void *cl;

	ASSERT(MBUF_CLASS_VALID(class) && MBUF_CLASS_COMPOSITE(class));
	ASSERT(needed > 0);

	VERIFY(class != MC_MBUF_16KCL || njcl > 0);

	                                                 
	VERIFY(m_slab_cnt(class) == 0 &&
	    m_slablist(class).tqh_first == NULL &&
	    m_slablist(class).tqh_last == NULL);

	lck_mtx_lock(mbuf_mlock);

	                                  
	num = cslab_alloc(class, plist, needed);
	list = *plist;
	if (num == needed) {
		m_alloc_cnt(class) += num;
		lck_mtx_unlock(mbuf_mlock);
		return needed;
	}

	lck_mtx_unlock(mbuf_mlock);

	  
                                                              
                                                            
                                                           
    
	needed -= num;

	  
                                                                    
                                                                  
                                                                  
                                                                 
                                                               
    
	wait |= MCR_COMP;
	if (!(wait & MCR_NOSLEEP)) {
		wait |= MCR_FAILOK;
	}

	                    
	needed = mcache_alloc_ext(m_cache(MC_MBUF), &mp_list, needed, wait);
	if (needed == 0) {
		ASSERT(mp_list == NULL);
		goto fail;
	}

	                       
	if (class == MC_MBUF_CL) {
		cl_class = MC_CL;
	} else if (class == MC_MBUF_BIGCL) {
		cl_class = MC_BIGCL;
	} else {
		VERIFY(class == MC_MBUF_16KCL);
		cl_class = MC_16KCL;
	}
	needed = mcache_alloc_ext(m_cache(cl_class), &clp_list, needed, wait);
	if (needed == 0) {
		ASSERT(clp_list == NULL);
		goto fail;
	}

	needed = mcache_alloc_ext(ref_cache, &ref_list, needed, wait);
	if (needed == 0) {
		ASSERT(ref_list == NULL);
		goto fail;
	}

	  
                                                               
                                                                
    
	for (cnum = 0; cnum < needed; cnum++) {
		struct mbuf *ms;

		m = ms = (struct mbuf *)mp_list;
		mp_list = mp_list->obj_next;

		cl = clp_list;
		clp_list = clp_list->obj_next;
		((mcache_obj_t *)cl)->obj_next = NULL;

		rfa = (struct ext_ref *)ref_list;
		ref_list = ref_list->obj_next;
		((mcache_obj_t *)(void *)rfa)->obj_next = NULL;

		  
                                                      
                                                         
                                                       
                                        
     
		if (mclaudit != NULL) {
			mcache_audit_t *mca, *cl_mca;

			lck_mtx_lock(mbuf_mlock);
			mca = mcl_audit_buf2mca(MC_MBUF, (mcache_obj_t *)m);
			ms = MCA_SAVED_MBUF_PTR(mca);
			cl_mca = mcl_audit_buf2mca(cl_class,
			    (mcache_obj_t *)cl);

			  
                                                       
                                                     
                                                    
                                                        
                                                        
                                                      
      
			mca->mca_uptr = cl_mca;
			cl_mca->mca_uptr = mca;

			ASSERT(mca->mca_uflags & MB_SCVALID);
			ASSERT(!(cl_mca->mca_uflags & MB_SCVALID));
			lck_mtx_unlock(mbuf_mlock);

			                                           
			if (mclverify) {
				size_t size;

				mcache_set_pattern(MCACHE_FREE_PATTERN, m,
				    m_maxsize(MC_MBUF));

				if (class == MC_MBUF_CL) {
					size = m_maxsize(MC_CL);
				} else if (class == MC_MBUF_BIGCL) {
					size = m_maxsize(MC_BIGCL);
				} else {
					size = m_maxsize(MC_16KCL);
				}

				mcache_set_pattern(MCACHE_FREE_PATTERN, cl,
				    size);
			}
		}

		MBUF_INIT(ms, 0, MT_FREE);
		if (class == MC_MBUF_16KCL) {
			MBUF_16KCL_INIT(ms, cl, rfa, 0, EXTF_COMPOSITE);
		} else if (class == MC_MBUF_BIGCL) {
			MBUF_BIGCL_INIT(ms, cl, rfa, 0, EXTF_COMPOSITE);
		} else {
			MBUF_CL_INIT(ms, cl, rfa, 0, EXTF_COMPOSITE);
		}
		VERIFY(ms->m_flags == M_EXT);
		VERIFY(m_get_rfa(ms) != NULL && MBUF_IS_COMPOSITE(ms));

		*list = (mcache_obj_t *)m;
		(*list)->obj_next = NULL;
		list = *plist = &(*list)->obj_next;
	}

fail:
	  
                                     
    
	if (mp_list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF), mp_list);
	}
	if (clp_list != NULL) {
		mcache_free_ext(m_cache(cl_class), clp_list);
	}
	if (ref_list != NULL) {
		mcache_free_ext(ref_cache, ref_list);
	}

	lck_mtx_lock(mbuf_mlock);
	if (num > 0 || cnum > 0) {
		m_total(class) += cnum;
		VERIFY(m_total(class) <= m_maxlimit(class));
		m_alloc_cnt(class) += num + cnum;
	}
	if ((num + cnum) < want) {
		m_fail_cnt(class) += (want - (num + cnum));
	}
	lck_mtx_unlock(mbuf_mlock);

	return num + cnum;
}

  
                                                                    
                                                                         
                   
   
static void
mbuf_cslab_free(void *arg, mcache_obj_t *list, int purged)
{
	mbuf_class_t class = (mbuf_class_t)arg;
	unsigned int num;
	int w;

	ASSERT(MBUF_CLASS_VALID(class) && MBUF_CLASS_COMPOSITE(class));

	lck_mtx_lock(mbuf_mlock);

	num = cslab_free(class, list, purged);
	m_free_cnt(class) += num;

	if ((w = mb_waiters) > 0) {
		mb_waiters = 0;
	}
	if (w) {
		mbwdog_logger("waking up all threads");
	}

	lck_mtx_unlock(mbuf_mlock);

	if (w != 0) {
		wakeup(mb_waitchan);
	}
}

  
                                                                     
                                                                        
                                                                      
                                                                       
                                                                     
                            
   
static void
mbuf_cslab_audit(void *arg, mcache_obj_t *list, boolean_t alloc)
{
	mbuf_class_t class = (mbuf_class_t)arg, cl_class;
	mcache_audit_t *mca;
	struct mbuf *m, *ms;
	mcl_slab_t *clsp, *nsp;
	size_t cl_size;
	void *cl;

	ASSERT(MBUF_CLASS_VALID(class) && MBUF_CLASS_COMPOSITE(class));
	if (class == MC_MBUF_CL) {
		cl_class = MC_CL;
	} else if (class == MC_MBUF_BIGCL) {
		cl_class = MC_BIGCL;
	} else {
		cl_class = MC_16KCL;
	}
	cl_size = m_maxsize(cl_class);

	while ((m = ms = (struct mbuf *)list) != NULL) {
		lck_mtx_lock(mbuf_mlock);
		                                                          
		mca = mcl_audit_buf2mca(MC_MBUF, (mcache_obj_t *)m);
		mcl_audit_mbuf(mca, m, TRUE, alloc);
		if (mcltrace) {
			mcache_buffer_log(mca, m, m_cache(class), &mb_start);
		}

		if (alloc) {
			mca->mca_uflags |= MB_COMP_INUSE;
		} else {
			mca->mca_uflags &= ~MB_COMP_INUSE;
		}

		  
                                                         
                                                            
                                                          
     
		if (!alloc && mclverify) {
			ms = MCA_SAVED_MBUF_PTR(mca);
		}

		                                                             
		cl = ms->m_ext.ext_buf;
		clsp = slab_get(cl);
		VERIFY(ms->m_flags == M_EXT && cl != NULL);
		VERIFY(m_get_rfa(ms) != NULL && MBUF_IS_COMPOSITE(ms));
		if (class == MC_MBUF_CL) {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NCLPG);
		} else {
			VERIFY(clsp->sl_refcnt >= 1 &&
			    clsp->sl_refcnt <= NBCLPG);
		}

		if (class == MC_MBUF_16KCL) {
			int k;
			for (nsp = clsp, k = 1; k < NSLABSP16KB; k++) {
				nsp = nsp->sl_next;
				                                       
				VERIFY(nsp != NULL);
				VERIFY(nsp->sl_refcnt == 1);
			}
		}


		mca = mcl_audit_buf2mca(cl_class, cl);
		mcl_audit_cluster(mca, cl, cl_size, alloc, FALSE);
		if (mcltrace) {
			mcache_buffer_log(mca, cl, m_cache(class), &mb_start);
		}

		if (alloc) {
			mca->mca_uflags |= MB_COMP_INUSE;
		} else {
			mca->mca_uflags &= ~MB_COMP_INUSE;
		}
		lck_mtx_unlock(mbuf_mlock);

		list = list->obj_next;
	}
}

static void
m_vm_error_stats(uint32_t *cnt, uint64_t *ts, uint64_t *size,
    uint64_t alloc_size, kern_return_t error)
{
	*cnt = *cnt + 1;
	*ts = net_uptime();
	if (size) {
		*size = alloc_size;
	}
	_CASSERT(sizeof(mb_kmem_stats) / sizeof(mb_kmem_stats[0]) ==
	    sizeof(mb_kmem_stats_labels) / sizeof(mb_kmem_stats_labels[0]));
	switch (error) {
	case KERN_SUCCESS:
		break;
	case KERN_INVALID_ARGUMENT:
		mb_kmem_stats[0]++;
		break;
	case KERN_INVALID_ADDRESS:
		mb_kmem_stats[1]++;
		break;
	case KERN_RESOURCE_SHORTAGE:
		mb_kmem_stats[2]++;
		break;
	case KERN_NO_SPACE:
		mb_kmem_stats[3]++;
		break;
	case KERN_FAILURE:
		mb_kmem_stats[4]++;
		break;
	default:
		mb_kmem_stats[5]++;
		break;
	}
}

static vm_offset_t
kmem_mb_alloc(vm_map_t mbmap, int size, int physContig, kern_return_t *err)
{
	vm_offset_t addr = 0;
	kern_return_t kr = KERN_SUCCESS;

	if (!physContig) {
		kr = kernel_memory_allocate(mbmap, &addr, size, 0,
		    KMA_KOBJECT | KMA_LOMEM, VM_KERN_MEMORY_MBUF);
	} else {
		kr = kmem_alloc_contig(mbmap, &addr, size, PAGE_MASK, 0xfffff,
		    0, KMA_KOBJECT | KMA_LOMEM, VM_KERN_MEMORY_MBUF);
	}

	if (kr != KERN_SUCCESS) {
		addr = 0;
	}
	if (err) {
		*err = kr;
	}

	return addr;
}

  
                                                                       
   
static int
m_clalloc(const u_int32_t num, const int wait, const u_int32_t bufsize)
{
	int i, count = 0;
	vm_size_t size = 0;
	int numpages = 0, large_buffer;
	vm_offset_t page = 0;
	mcache_audit_t *mca_list = NULL;
	mcache_obj_t *con_list = NULL;
	mcl_slab_t *sp;
	mbuf_class_t class;
	kern_return_t error;

	                                                                   
	large_buffer = ((bufsize == m_maxsize(MC_16KCL)) &&
	    PAGE_SIZE < M16KCLBYTES);
	VERIFY(bufsize == m_maxsize(MC_BIGCL) ||
	    bufsize == m_maxsize(MC_16KCL));

	VERIFY((bufsize == PAGE_SIZE) ||
	    (bufsize > PAGE_SIZE && bufsize == m_maxsize(MC_16KCL)));

	if (bufsize == m_size(MC_BIGCL)) {
		class = MC_BIGCL;
	} else {
		class = MC_16KCL;
	}

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	  
                                                                
                                                                   
                                                                   
                                                                     
                                                                   
                                                         
    
	while (mb_clalloc_busy) {
		mb_clalloc_waiters++;
		(void) msleep(mb_clalloc_waitchan, mbuf_mlock,
		    (PZERO - 1), "m_clalloc", NULL);
		LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
	}

	                                                    
	mb_clalloc_busy = TRUE;

	  
                                                                 
                                                                 
    
	i = m_howmany(num, bufsize);
	if (i <= 0 || (wait & M_DONTWAIT)) {
		goto out;
	}

	lck_mtx_unlock(mbuf_mlock);

	size = round_page(i * bufsize);
	page = kmem_mb_alloc(mb_map, size, large_buffer, &error);

	  
                                                           
                                                           
                
    
	net_update_uptime();
	if (large_buffer && page == 0) {
		m_vm_error_stats(&mb_kmem_contig_failed,
		    &mb_kmem_contig_failed_ts,
		    &mb_kmem_contig_failed_size,
		    size, error);
		page = kmem_mb_alloc(mb_map, size, 0, &error);
	}

	if (page == 0) {
		m_vm_error_stats(&mb_kmem_failed,
		    &mb_kmem_failed_ts,
		    &mb_kmem_failed_size,
		    size, error);
#if PAGE_SIZE == 4096
		if (bufsize == m_maxsize(MC_BIGCL)) {
#else
		if (bufsize >= m_maxsize(MC_BIGCL)) {
#endif
			                              
			size = PAGE_SIZE;
			page = kmem_mb_alloc(mb_map, size, 0, &error);
			if (page == 0) {
				m_vm_error_stats(&mb_kmem_one_failed,
				    &mb_kmem_one_failed_ts,
				    NULL, size, error);
			}
		}

		if (page == 0) {
			lck_mtx_lock(mbuf_mlock);
			goto out;
		}
	}

	VERIFY(IS_P2ALIGNED(page, PAGE_SIZE));
	numpages = size / PAGE_SIZE;

	                                                               
	if (mclaudit != NULL) {
		int needed;

		  
                                                          
                                                         
                                                         
                                                        
                                                         
                                                    
     
		if (bufsize == PAGE_SIZE) {
			needed = numpages * NMBPG;

			i = mcache_alloc_ext(mcl_audit_con_cache,
			    &con_list, needed, MCR_SLEEP);

			VERIFY(con_list != NULL && i == needed);
		} else {
			  
                                               
                 
      
			needed = numpages / NSLABSP16KB;
		}

		i = mcache_alloc_ext(mcache_audit_cache,
		    (mcache_obj_t **)&mca_list, needed, MCR_SLEEP);

		VERIFY(mca_list != NULL && i == needed);
	}

	lck_mtx_lock(mbuf_mlock);

	for (i = 0; i < numpages; i++, page += PAGE_SIZE) {
		ppnum_t offset =
		    ((unsigned char *)page - mbutl) >> PAGE_SHIFT;
		ppnum_t new_page = pmap_find_phys(kernel_pmap, page);

		  
                                                     
                                                    
                                                         
     
		VERIFY(offset < mcl_pages);
		if (mcl_paddr_base != 0) {
			bzero((void *)(uintptr_t) page, PAGE_SIZE);
			new_page = IOMapperInsertPage(mcl_paddr_base,
			    offset, new_page);
		}
		mcl_paddr[offset] = new_page;

		                                  
		if (mclverify) {
			mcache_set_pattern(MCACHE_FREE_PATTERN,
			    (caddr_t)page, PAGE_SIZE);
		}
		if (bufsize == PAGE_SIZE) {
			mcache_obj_t *buf;
			                             
			sp = slab_get((void *)page);
			if (mclaudit != NULL) {
				mcl_audit_init((void *)page,
				    &mca_list, &con_list,
				    AUDIT_CONTENTS_SIZE, NMBPG);
			}
			VERIFY(sp->sl_refcnt == 0 && sp->sl_flags == 0);
			slab_init(sp, class, SLF_MAPPED, (void *)page,
			    (void *)page, PAGE_SIZE, 0, 1);
			buf = (mcache_obj_t *)page;
			buf->obj_next = NULL;

			                      
			slab_insert(sp, class);

			                                                    
			++m_infree(class);
			++m_total(class);
			VERIFY(m_total(class) <= m_maxlimit(class));
			if (class == MC_BIGCL) {
				mbstat.m_bigclfree = m_infree(MC_BIGCL) +
				    m_infree(MC_MBUF_BIGCL);
				mbstat.m_bigclusters = m_total(MC_BIGCL);
			}
			++count;
		} else if ((bufsize > PAGE_SIZE) &&
		    (i % NSLABSP16KB) == 0) {
			union m16kcluster *m16kcl = (union m16kcluster *)page;
			mcl_slab_t *nsp;
			int k;

			                             
			sp = slab_get(m16kcl);
			if (mclaudit != NULL) {
				mcl_audit_init(m16kcl, &mca_list, NULL, 0, 1);
			}

			VERIFY(sp->sl_refcnt == 0 && sp->sl_flags == 0);
			slab_init(sp, MC_16KCL, SLF_MAPPED,
			    m16kcl, m16kcl, bufsize, 0, 1);
			m16kcl->m16kcl_next = NULL;

			  
                                                   
                             
      
			for (k = 1; k < NSLABSP16KB; k++) {
				nsp = slab_get(((union mbigcluster *)page) + k);
				VERIFY(nsp->sl_refcnt == 0 &&
				    nsp->sl_flags == 0);
				slab_init(nsp, MC_16KCL,
				    SLF_MAPPED | SLF_PARTIAL,
				    m16kcl, NULL, 0, 0, 0);
			}
			                      
			slab_insert(sp, MC_16KCL);

			                                                    
			++m_infree(MC_16KCL);
			++m_total(MC_16KCL);
			VERIFY(m_total(MC_16KCL) <= m_maxlimit(MC_16KCL));
			++count;
		}
	}
	VERIFY(mca_list == NULL && con_list == NULL);

	if (!mb_peak_newreport && mbuf_report_usage(class)) {
		mb_peak_newreport = TRUE;
	}

	                                  
	mb_clalloc_busy = FALSE;
	if (mb_clalloc_waiters > 0) {
		mb_clalloc_waiters = 0;
		wakeup(mb_clalloc_waitchan);
	}

	return count;
out:
	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	mtracelarge_register(size);

	                                  
	mb_clalloc_busy = FALSE;
	if (mb_clalloc_waiters > 0) {
		mb_clalloc_waiters = 0;
		wakeup(mb_clalloc_waitchan);
	}

	  
                                                             
                                                                  
    
	if (i > 0 && mbuf_worker_ready && mbuf_worker_needs_wakeup) {
		mbwdog_logger("waking up the worker thread to to grow %s by %d",
		    m_cname(class), i);
		wakeup((caddr_t)&mbuf_worker_needs_wakeup);
		mbuf_worker_needs_wakeup = FALSE;
	}
	if (class == MC_BIGCL) {
		if (i > 0) {
			  
                                                  
                   
      
			i += m_total(MC_BIGCL);
			if (i > m_region_expand(MC_BIGCL)) {
				m_region_expand(MC_BIGCL) = i;
			}
		}
		if (m_infree(MC_BIGCL) >= num) {
			return 1;
		}
	} else {
		if (i > 0) {
			  
                                                   
                   
      
			i += m_total(MC_16KCL);
			if (i > m_region_expand(MC_16KCL)) {
				m_region_expand(MC_16KCL) = i;
			}
		}
		if (m_infree(MC_16KCL) >= num) {
			return 1;
		}
	}
	return 0;
}

  
                                                                  
   
static int
freelist_populate(mbuf_class_t class, unsigned int num, int wait)
{
	mcache_obj_t *o = NULL;
	int i, numpages = 0, count;
	mbuf_class_t super_class;

	VERIFY(class == MC_MBUF || class == MC_CL || class == MC_BIGCL ||
	    class == MC_16KCL);

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	VERIFY(PAGE_SIZE == m_maxsize(MC_BIGCL) ||
	    PAGE_SIZE == m_maxsize(MC_16KCL));

	if (m_maxsize(class) >= PAGE_SIZE) {
		return m_clalloc(num, wait, m_maxsize(class)) != 0;
	}

	  
                                                               
                               
    

	numpages = (num * m_size(class) + PAGE_SIZE - 1) / PAGE_SIZE;

	                                               
	if (PAGE_SIZE == m_maxsize(MC_BIGCL)) {
		super_class = MC_BIGCL;
	} else {
		super_class = MC_16KCL;
	}

	i = m_clalloc(numpages, wait, m_maxsize(super_class));

	                                                 
	int numobj = PAGE_SIZE / m_maxsize(class);

	for (count = 0; count < numpages; count++) {
		                                        
		if (m_total(super_class) <= m_minlimit(super_class) ||
		    m_total(class) >= m_maxlimit(class)) {
			break;
		}

		if ((o = slab_alloc(super_class, wait)) == NULL) {
			break;
		}

		struct mbuf *m = (struct mbuf *)o;
		union mcluster *c = (union mcluster *)o;
		union mbigcluster *mbc = (union mbigcluster *)o;
		mcl_slab_t *sp = slab_get(o);
		mcache_audit_t *mca = NULL;

		  
                                                        
                                                           
               
     
		VERIFY(sp->sl_refcnt == 1 && slab_is_detached(sp));
		VERIFY((sp->sl_flags & (SLF_MAPPED | SLF_PARTIAL)) == SLF_MAPPED);
		  
                                             
                      
     
		if (mclverify) {
			mca = mcl_audit_buf2mca(super_class,
			    (mcache_obj_t *)o);
			mcache_audit_free_verify(mca,
			    (mcache_obj_t *)o, 0, m_maxsize(super_class));
		}

		                                                 
		slab_init(sp, class, sp->sl_flags,
		    sp->sl_base, NULL, PAGE_SIZE, 0, numobj);

		VERIFY(sp->sl_head == NULL);

		VERIFY(m_total(super_class) >= 1);
		m_total(super_class)--;

		if (super_class == MC_BIGCL) {
			mbstat.m_bigclusters = m_total(MC_BIGCL);
		}

		m_total(class) += numobj;
		VERIFY(m_total(class) <= m_maxlimit(class));
		m_infree(class) += numobj;

		if (!mb_peak_newreport && mbuf_report_usage(class)) {
			mb_peak_newreport = TRUE;
		}

		i = numobj;
		if (class == MC_MBUF) {
			mbstat.m_mbufs = m_total(MC_MBUF);
			mtype_stat_add(MT_FREE, NMBPG);
			while (i--) {
				  
                                            
                                         
                                 
                                          
                                       
                       
       
				if (mclaudit != NULL) {
					struct mbuf *ms;
					mca = mcl_audit_buf2mca(MC_MBUF,
					    (mcache_obj_t *)m);
					ms = MCA_SAVED_MBUF_PTR(mca);
					ms->m_type = MT_FREE;
				} else {
					m->m_type = MT_FREE;
				}
				m->m_next = sp->sl_head;
				sp->sl_head = (void *)m++;
			}
		} else if (class == MC_CL) {            
			mbstat.m_clfree =
			    m_infree(MC_CL) + m_infree(MC_MBUF_CL);
			mbstat.m_clusters = m_total(MC_CL);
			while (i--) {
				c->mcl_next = sp->sl_head;
				sp->sl_head = (void *)c++;
			}
		} else {
			VERIFY(class == MC_BIGCL);
			mbstat.m_bigclusters = m_total(MC_BIGCL);
			mbstat.m_bigclfree = m_infree(MC_BIGCL) +
			    m_infree(MC_MBUF_BIGCL);
			while (i--) {
				mbc->mbc_next = sp->sl_head;
				sp->sl_head = (void *)mbc++;
			}
		}

		                                                
		slab_insert(sp, class);

		if ((i = mb_waiters) > 0) {
			mb_waiters = 0;
		}
		if (i != 0) {
			mbwdog_logger("waking up all threads");
			wakeup(mb_waitchan);
		}
	}
	return count != 0;
}

  
                                                                        
   
static void
freelist_init(mbuf_class_t class)
{
	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	VERIFY(class == MC_CL || class == MC_BIGCL);
	VERIFY(m_total(class) == 0);
	VERIFY(m_minlimit(class) > 0);

	while (m_total(class) < m_minlimit(class)) {
		(void) freelist_populate(class, m_minlimit(class), M_WAIT);
	}

	VERIFY(m_total(class) >= m_minlimit(class));
}

  
                                                               
                                                             
                                                
   
static boolean_t
mbuf_cached_above(mbuf_class_t class, int wait)
{
	switch (class) {
	case MC_MBUF:
		if (wait & MCR_COMP) {
			return !mcache_bkt_isempty(m_cache(MC_MBUF_CL)) ||
			       !mcache_bkt_isempty(m_cache(MC_MBUF_BIGCL));
		}
		break;

	case MC_CL:
		if (wait & MCR_COMP) {
			return !mcache_bkt_isempty(m_cache(MC_MBUF_CL));
		}
		break;

	case MC_BIGCL:
		if (wait & MCR_COMP) {
			return !mcache_bkt_isempty(m_cache(MC_MBUF_BIGCL));
		}
		break;

	case MC_16KCL:
		if (wait & MCR_COMP) {
			return !mcache_bkt_isempty(m_cache(MC_MBUF_16KCL));
		}
		break;

	case MC_MBUF_CL:
	case MC_MBUF_BIGCL:
	case MC_MBUF_16KCL:
		break;

	default:
		VERIFY(0);
		                
	}

	return !mcache_bkt_isempty(m_cache(class));
}

  
                                                        
   
static boolean_t
mbuf_steal(mbuf_class_t class, unsigned int num)
{
	mcache_obj_t *top = NULL;
	mcache_obj_t **list = &top;
	unsigned int tot = 0;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	switch (class) {
	case MC_MBUF:
	case MC_CL:
	case MC_BIGCL:
	case MC_16KCL:
		return FALSE;

	case MC_MBUF_CL:
	case MC_MBUF_BIGCL:
	case MC_MBUF_16KCL:
		                                                                
		if (m_infree(class) > m_minlimit(class)) {
			tot = cslab_alloc(class, &list,
			    MIN(num, m_infree(class)));
		}

		                                                  
		if (top != NULL) {
			(void) cslab_free(class, top, 1);
		}
		break;

	default:
		VERIFY(0);
		                
	}

	return tot == num;
}

static void
m_reclaim(mbuf_class_t class, unsigned int num, boolean_t comp)
{
	int m, bmap = 0;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	VERIFY(m_total(MC_CL) <= m_maxlimit(MC_CL));
	VERIFY(m_total(MC_BIGCL) <= m_maxlimit(MC_BIGCL));
	VERIFY(m_total(MC_16KCL) <= m_maxlimit(MC_16KCL));

	  
                                                        
                                                   
    
	switch (class) {
	case MC_MBUF:
		m_wantpurge(MC_CL)++;
		m_wantpurge(MC_BIGCL)++;
		m_wantpurge(MC_MBUF_CL)++;
		m_wantpurge(MC_MBUF_BIGCL)++;
		break;

	case MC_CL:
		m_wantpurge(MC_MBUF)++;
		m_wantpurge(MC_BIGCL)++;
		m_wantpurge(MC_MBUF_BIGCL)++;
		if (!comp) {
			m_wantpurge(MC_MBUF_CL)++;
		}
		break;

	case MC_BIGCL:
		m_wantpurge(MC_MBUF)++;
		m_wantpurge(MC_CL)++;
		m_wantpurge(MC_MBUF_CL)++;
		if (!comp) {
			m_wantpurge(MC_MBUF_BIGCL)++;
		}
		break;

	case MC_16KCL:
		if (!comp) {
			m_wantpurge(MC_MBUF_16KCL)++;
		}
		break;

	default:
		VERIFY(0);
		                
	}

	  
                                                                
                                                                
                                                               
                                                         
    
	for (m = 0; m < NELEM(mbuf_table); m++) {
		if (m_wantpurge(m) > 0) {
			m_wantpurge(m) = 0;
			  
                                                      
                                                    
                                                     
                                                 
      
			if (!mbuf_steal(m, num)) {
				bmap |= (1 << m);
			}
		}
	}

	lck_mtx_unlock(mbuf_mlock);

	if (bmap != 0) {
		                                 
		net_drain_domains();

		                                                               
		for (m = 0; m < NELEM(mbuf_table); m++) {
			if ((bmap & (1 << m)) &&
			    mcache_purge_cache(m_cache(m), TRUE)) {
				lck_mtx_lock(mbuf_mlock);
				m_purge_cnt(m)++;
				mbstat.m_drain++;
				lck_mtx_unlock(mbuf_mlock);
			}
		}
	} else {
		  
                                                                  
                                                                  
              
     
		mcache_reap();
	}
	lck_mtx_lock(mbuf_mlock);
}

static inline struct mbuf *
m_get_common(int wait, short type, int hdr)
{
	struct mbuf *m;
	int mcflags = MSLEEPF(wait);

	                                                                  
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	m = mcache_alloc(m_cache(MC_MBUF), mcflags);
	if (m != NULL) {
		MBUF_INIT(m, hdr, type);
		mtype_stat_inc(type);
		mtype_stat_dec(MT_FREE);
	}
	return m;
}

  
                                                                
                      
   
#define _M_GET(wait, type)      m_get_common(wait, type, 0)
#define _M_GETHDR(wait, type)   m_get_common(wait, type, 1)
#define _M_RETRY(wait, type)    _M_GET(wait, type)
#define _M_RETRYHDR(wait, type) _M_GETHDR(wait, type)
#define _MGET(m, how, type)     ((m) = _M_GET(how, type))
#define _MGETHDR(m, how, type)  ((m) = _M_GETHDR(how, type))

struct mbuf *
m_get(int wait, int type)
{
	return _M_GET(wait, type);
}

struct mbuf *
m_gethdr(int wait, int type)
{
	return _M_GETHDR(wait, type);
}

struct mbuf *
m_retry(int wait, int type)
{
	return _M_RETRY(wait, type);
}

struct mbuf *
m_retryhdr(int wait, int type)
{
	return _M_RETRYHDR(wait, type);
}

struct mbuf *
m_getclr(int wait, int type)
{
	struct mbuf *m;

	_MGET(m, wait, type);
	if (m != NULL) {
		bzero(MTOD(m, caddr_t), MLEN);
	}
	return m;
}

static int
m_free_paired(struct mbuf *m)
{
	VERIFY((m->m_flags & M_EXT) && (MEXT_FLAGS(m) & EXTF_PAIRED));

	membar_sync();
	if (MEXT_PMBUF(m) == m) {
		volatile UInt16 *addr = (volatile UInt16 *)&MEXT_PREF(m);
		int16_t oprefcnt, prefcnt;

		  
                                                       
                                                       
                                                         
                                                           
     
		do {
			oprefcnt = *addr;
			prefcnt = oprefcnt - 1;
		} while (!OSCompareAndSwap16(oprefcnt, prefcnt, addr));

		if (prefcnt > 1) {
			return 1;
		} else if (prefcnt == 1) {
			(*(m_get_ext_free(m)))(m->m_ext.ext_buf,
			    m->m_ext.ext_size, m_get_ext_arg(m));
			return 1;
		} else if (prefcnt == 0) {
			VERIFY(MBUF_IS_PAIRED(m));

			  
                                                  
                                                 
                     
      
			MEXT_MINREF(m) = 0;

			  
                                                    
                                                     
                          
      
			atomic_set_ptr(&MEXT_PMBUF(m), NULL);

			switch (m->m_ext.ext_size) {
			case MCLBYTES:
				m_set_ext(m, m_get_rfa(m), NULL, NULL);
				break;

			case MBIGCLBYTES:
				m_set_ext(m, m_get_rfa(m), m_bigfree, NULL);
				break;

			case M16KCLBYTES:
				m_set_ext(m, m_get_rfa(m), m_16kfree, NULL);
				break;

			default:
				VERIFY(0);
				                
			}
		}
	}

	  
                                                               
                                                                 
                   
    
	return 0;
}

struct mbuf *
m_free(struct mbuf *m)
{
	struct mbuf *n = m->m_next;

	if (m->m_type == MT_FREE) {
		panic("m_free: freeing an already freed mbuf");
	}

	if (m->m_flags & M_PKTHDR) {
		                                     
		m_redzone_verify(m);
		                                                
		m_tag_delete_chain(m, NULL);

		m_do_tx_compl_callback(m, NULL);
	}

	if (m->m_flags & M_EXT) {
		uint16_t refcnt;
		uint32_t composite;
		m_ext_free_func_t m_free_func;

		if (MBUF_IS_PAIRED(m) && m_free_paired(m)) {
			return n;
		}

		refcnt = m_decref(m);
		composite = (MEXT_FLAGS(m) & EXTF_COMPOSITE);
		m_free_func = m_get_ext_free(m);

		if (refcnt == MEXT_MINREF(m) && !composite) {
			if (m_free_func == NULL) {
				mcache_free(m_cache(MC_CL), m->m_ext.ext_buf);
			} else if (m_free_func == m_bigfree) {
				mcache_free(m_cache(MC_BIGCL),
				    m->m_ext.ext_buf);
			} else if (m_free_func == m_16kfree) {
				mcache_free(m_cache(MC_16KCL),
				    m->m_ext.ext_buf);
			} else {
				(*m_free_func)(m->m_ext.ext_buf,
				    m->m_ext.ext_size, m_get_ext_arg(m));
			}
			mcache_free(ref_cache, m_get_rfa(m));
			m_set_ext(m, NULL, NULL, NULL);
		} else if (refcnt == MEXT_MINREF(m) && composite) {
			VERIFY(!(MEXT_FLAGS(m) & EXTF_PAIRED));
			VERIFY(m->m_type != MT_FREE);

			mtype_stat_dec(m->m_type);
			mtype_stat_inc(MT_FREE);

			m->m_type = MT_FREE;
			m->m_flags = M_EXT;
			m->m_len = 0;
			m->m_next = m->m_nextpkt = NULL;

			MEXT_FLAGS(m) &= ~EXTF_READONLY;

			                                        
			if (m_free_func == NULL) {
				mcache_free(m_cache(MC_MBUF_CL), m);
			} else if (m_free_func == m_bigfree) {
				mcache_free(m_cache(MC_MBUF_BIGCL), m);
			} else {
				VERIFY(m_free_func == m_16kfree);
				mcache_free(m_cache(MC_MBUF_16KCL), m);
			}
			return n;
		}
	}

	if (m->m_type != MT_FREE) {
		mtype_stat_dec(m->m_type);
		mtype_stat_inc(MT_FREE);
	}

	m->m_type = MT_FREE;
	m->m_flags = m->m_len = 0;
	m->m_next = m->m_nextpkt = NULL;

	mcache_free(m_cache(MC_MBUF), m);

	return n;
}

__private_extern__ struct mbuf *
m_clattach(struct mbuf *m, int type, caddr_t extbuf,
    void (*extfree)(caddr_t, u_int, caddr_t), u_int extsize, caddr_t extarg,
    int wait, int pair)
{
	struct ext_ref *rfa = NULL;

	  
                                                                    
                                                                  
                                                  
    
	if ((m != NULL && MBUF_IS_PAIRED(m)) ||
	    (m == NULL && (m = _M_GETHDR(wait, type)) == NULL)) {
		return NULL;
	}

	if (m->m_flags & M_EXT) {
		u_int16_t refcnt;
		u_int32_t composite;
		m_ext_free_func_t m_free_func;

		refcnt = m_decref(m);
		composite = (MEXT_FLAGS(m) & EXTF_COMPOSITE);
		VERIFY(!(MEXT_FLAGS(m) & EXTF_PAIRED) && MEXT_PMBUF(m) == NULL);
		m_free_func = m_get_ext_free(m);
		if (refcnt == MEXT_MINREF(m) && !composite) {
			if (m_free_func == NULL) {
				mcache_free(m_cache(MC_CL), m->m_ext.ext_buf);
			} else if (m_free_func == m_bigfree) {
				mcache_free(m_cache(MC_BIGCL),
				    m->m_ext.ext_buf);
			} else if (m_free_func == m_16kfree) {
				mcache_free(m_cache(MC_16KCL),
				    m->m_ext.ext_buf);
			} else {
				(*m_free_func)(m->m_ext.ext_buf,
				    m->m_ext.ext_size, m_get_ext_arg(m));
			}
			                                    
			rfa = m_get_rfa(m);
		} else if (refcnt == MEXT_MINREF(m) && composite) {
			VERIFY(m->m_type != MT_FREE);

			mtype_stat_dec(m->m_type);
			mtype_stat_inc(MT_FREE);

			m->m_type = MT_FREE;
			m->m_flags = M_EXT;
			m->m_len = 0;
			m->m_next = m->m_nextpkt = NULL;

			MEXT_FLAGS(m) &= ~EXTF_READONLY;

			                                        
			if (m_free_func == NULL) {
				mcache_free(m_cache(MC_MBUF_CL), m);
			} else if (m_free_func == m_bigfree) {
				mcache_free(m_cache(MC_MBUF_BIGCL), m);
			} else {
				VERIFY(m_free_func == m_16kfree);
				mcache_free(m_cache(MC_MBUF_16KCL), m);
			}
			  
                                                  
                                              
      
			if ((m = _M_GETHDR(wait, type)) == NULL) {
				return NULL;
			}
		}
	}

	if (rfa == NULL &&
	    (rfa = mcache_alloc(ref_cache, MSLEEPF(wait))) == NULL) {
		m_free(m);
		return NULL;
	}

	if (!pair) {
		MEXT_INIT(m, extbuf, extsize, extfree, extarg, rfa,
		    0, 1, 0, 0, 0, NULL);
	} else {
		MEXT_INIT(m, extbuf, extsize, extfree, (caddr_t)m, rfa,
		    1, 1, 1, EXTF_PAIRED, 0, m);
	}

	return m;
}

  
                                                                         
                                                                          
   
struct mbuf *
m_getcl(int wait, int type, int flags)
{
	struct mbuf *m;
	int mcflags = MSLEEPF(wait);
	int hdr = (flags & M_PKTHDR);

	                                                                  
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	m = mcache_alloc(m_cache(MC_MBUF_CL), mcflags);
	if (m != NULL) {
		u_int16_t flag;
		struct ext_ref *rfa;
		void *cl;

		VERIFY(m->m_type == MT_FREE && m->m_flags == M_EXT);
		cl = m->m_ext.ext_buf;
		rfa = m_get_rfa(m);

		ASSERT(cl != NULL && rfa != NULL);
		VERIFY(MBUF_IS_COMPOSITE(m) && m_get_ext_free(m) == NULL);

		flag = MEXT_FLAGS(m);

		MBUF_INIT(m, hdr, type);
		MBUF_CL_INIT(m, cl, rfa, 1, flag);

		mtype_stat_inc(type);
		mtype_stat_dec(MT_FREE);
	}
	return m;
}

                                                     
struct mbuf *
m_mclget(struct mbuf *m, int wait)
{
	struct ext_ref *rfa;

	if ((rfa = mcache_alloc(ref_cache, MSLEEPF(wait))) == NULL) {
		return m;
	}

	m->m_ext.ext_buf = m_mclalloc(wait);
	if (m->m_ext.ext_buf != NULL) {
		MBUF_CL_INIT(m, m->m_ext.ext_buf, rfa, 1, 0);
	} else {
		mcache_free(ref_cache, rfa);
	}
	return m;
}

                              
caddr_t
m_mclalloc(int wait)
{
	int mcflags = MSLEEPF(wait);

	                                                                  
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	return mcache_alloc(m_cache(MC_CL), mcflags);
}

                          
void
m_mclfree(caddr_t p)
{
	mcache_free(m_cache(MC_CL), p);
}

  
                                                                     
                                                                    
   
int
m_mclhasreference(struct mbuf *m)
{
	if (!(m->m_flags & M_EXT)) {
		return 0;
	}

	ASSERT(m_get_rfa(m) != NULL);

	return (MEXT_FLAGS(m) & EXTF_READONLY) ? 1 : 0;
}

__private_extern__ caddr_t
m_bigalloc(int wait)
{
	int mcflags = MSLEEPF(wait);

	                                                                  
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	return mcache_alloc(m_cache(MC_BIGCL), mcflags);
}

__private_extern__ void
m_bigfree(caddr_t p, __unused u_int size, __unused caddr_t arg)
{
	mcache_free(m_cache(MC_BIGCL), p);
}

                                                          
__private_extern__ struct mbuf *
m_mbigget(struct mbuf *m, int wait)
{
	struct ext_ref *rfa;

	if ((rfa = mcache_alloc(ref_cache, MSLEEPF(wait))) == NULL) {
		return m;
	}

	m->m_ext.ext_buf =  m_bigalloc(wait);
	if (m->m_ext.ext_buf != NULL) {
		MBUF_BIGCL_INIT(m, m->m_ext.ext_buf, rfa, 1, 0);
	} else {
		mcache_free(ref_cache, rfa);
	}
	return m;
}

__private_extern__ caddr_t
m_16kalloc(int wait)
{
	int mcflags = MSLEEPF(wait);

	                                                                  
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	return mcache_alloc(m_cache(MC_16KCL), mcflags);
}

__private_extern__ void
m_16kfree(caddr_t p, __unused u_int size, __unused caddr_t arg)
{
	mcache_free(m_cache(MC_16KCL), p);
}

                                                          
__private_extern__ struct mbuf *
m_m16kget(struct mbuf *m, int wait)
{
	struct ext_ref *rfa;

	if ((rfa = mcache_alloc(ref_cache, MSLEEPF(wait))) == NULL) {
		return m;
	}

	m->m_ext.ext_buf =  m_16kalloc(wait);
	if (m->m_ext.ext_buf != NULL) {
		MBUF_16KCL_INIT(m, m->m_ext.ext_buf, rfa, 1, 0);
	} else {
		mcache_free(ref_cache, rfa);
	}
	return m;
}

  
                                          
                                                         
   
void
m_copy_pkthdr(struct mbuf *to, struct mbuf *from)
{
	VERIFY(from->m_flags & M_PKTHDR);

	                                     
	m_redzone_verify(from);

	if (to->m_flags & M_PKTHDR) {
		                                     
		m_redzone_verify(to);
		                                             
		m_tag_delete_chain(to, NULL);
	}
	to->m_pkthdr = from->m_pkthdr;                               
	m_classifier_init(from, 0);                                        
	m_tag_init(from, 1);                                                 
	m_scratch_init(from);                                               
	to->m_flags = (from->m_flags & M_COPYFLAGS) | (to->m_flags & M_EXT);
	if ((to->m_flags & M_EXT) == 0) {
		to->m_data = to->m_pktdat;
	}
	m_redzone_init(to);                                                
}

  
                                          
                                                         
                                                           
   
static int
m_dup_pkthdr(struct mbuf *to, struct mbuf *from, int how)
{
	VERIFY(from->m_flags & M_PKTHDR);

	                                     
	m_redzone_verify(from);

	if (to->m_flags & M_PKTHDR) {
		                                     
		m_redzone_verify(to);
		                                             
		m_tag_delete_chain(to, NULL);
	}
	to->m_flags = (from->m_flags & M_COPYFLAGS) | (to->m_flags & M_EXT);
	if ((to->m_flags & M_EXT) == 0) {
		to->m_data = to->m_pktdat;
	}
	to->m_pkthdr = from->m_pkthdr;
	m_redzone_init(to);                                                
	m_tag_init(to, 0);                                                    
	return m_tag_copy_chain(to, from, how);
}

void
m_copy_pftag(struct mbuf *to, struct mbuf *from)
{
	memcpy(m_pftag(to), m_pftag(from), sizeof(struct pf_mtag));
#if PF_ECN
	m_pftag(to)->pftag_hdr = NULL;
	m_pftag(to)->pftag_flags &= ~(PF_TAG_HDR_INET | PF_TAG_HDR_INET6);
#endif             
}

void
m_copy_necptag(struct mbuf *to, struct mbuf *from)
{
	memcpy(m_necptag(to), m_necptag(from), sizeof(struct necp_mtag_));
}

void
m_classifier_init(struct mbuf *m, uint32_t pktf_mask)
{
	VERIFY(m->m_flags & M_PKTHDR);

	m->m_pkthdr.pkt_proto = 0;
	m->m_pkthdr.pkt_flowsrc = 0;
	m->m_pkthdr.pkt_flowid = 0;
	m->m_pkthdr.pkt_flags &= pktf_mask;                              
	                                                                    
	if (!(m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		(void) m_set_service_class(m, MBUF_SC_BE);
	}
	if (!(m->m_pkthdr.pkt_flags & PKTF_IFAINFO)) {
		m->m_pkthdr.pkt_ifainfo = 0;
	}
	  
                                   
    
	if (!(m->m_pkthdr.pkt_flags & PKTF_TS_VALID)) {
		m->m_pkthdr.pkt_timestamp = 0;
	}
}

void
m_copy_classifier(struct mbuf *to, struct mbuf *from)
{
	VERIFY(to->m_flags & M_PKTHDR);
	VERIFY(from->m_flags & M_PKTHDR);

	to->m_pkthdr.pkt_proto = from->m_pkthdr.pkt_proto;
	to->m_pkthdr.pkt_flowsrc = from->m_pkthdr.pkt_flowsrc;
	to->m_pkthdr.pkt_flowid = from->m_pkthdr.pkt_flowid;
	to->m_pkthdr.pkt_flags = from->m_pkthdr.pkt_flags;
	(void) m_set_service_class(to, from->m_pkthdr.pkt_svc);
	to->m_pkthdr.pkt_ifainfo  = from->m_pkthdr.pkt_ifainfo;
}

  
                                                                          
                                                                            
                                                                            
                                                                         
                                                                       
                                                      
   
__private_extern__ struct mbuf *
m_getpackets_internal(unsigned int *num_needed, int num_with_pkthdrs,
    int wait, int wantall, size_t bufsize)
{
	struct mbuf *m;
	struct mbuf **np, *top;
	unsigned int pnum, needed = *num_needed;
	mcache_obj_t *mp_list = NULL;
	int mcflags = MSLEEPF(wait);
	u_int16_t flag;
	struct ext_ref *rfa;
	mcache_t *cp;
	void *cl;

	ASSERT(bufsize == m_maxsize(MC_CL) ||
	    bufsize == m_maxsize(MC_BIGCL) ||
	    bufsize == m_maxsize(MC_16KCL));

	  
                                                 
                                                     
    
	VERIFY(bufsize != m_maxsize(MC_16KCL) || njcl > 0);

	top = NULL;
	np = &top;
	pnum = 0;

	  
                                                                 
                                                                   
                                                               
                                    
    
	if (!wantall || (mcflags & MCR_NOSLEEP)) {
		mcflags |= MCR_TRYHARD;
	}

	                                                                   
	if (bufsize == m_maxsize(MC_CL)) {
		cp = m_cache(MC_MBUF_CL);
	} else if (bufsize == m_maxsize(MC_BIGCL)) {
		cp = m_cache(MC_MBUF_BIGCL);
	} else {
		cp = m_cache(MC_MBUF_16KCL);
	}
	needed = mcache_alloc_ext(cp, &mp_list, needed, mcflags);

	for (pnum = 0; pnum < needed; pnum++) {
		m = (struct mbuf *)mp_list;
		mp_list = mp_list->obj_next;

		VERIFY(m->m_type == MT_FREE && m->m_flags == M_EXT);
		cl = m->m_ext.ext_buf;
		rfa = m_get_rfa(m);

		ASSERT(cl != NULL && rfa != NULL);
		VERIFY(MBUF_IS_COMPOSITE(m));

		flag = MEXT_FLAGS(m);

		MBUF_INIT(m, num_with_pkthdrs, MT_DATA);
		if (bufsize == m_maxsize(MC_16KCL)) {
			MBUF_16KCL_INIT(m, cl, rfa, 1, flag);
		} else if (bufsize == m_maxsize(MC_BIGCL)) {
			MBUF_BIGCL_INIT(m, cl, rfa, 1, flag);
		} else {
			MBUF_CL_INIT(m, cl, rfa, 1, flag);
		}

		if (num_with_pkthdrs > 0) {
			--num_with_pkthdrs;
		}

		*np = m;
		if (num_with_pkthdrs > 0) {
			np = &m->m_nextpkt;
		} else {
			np = &m->m_next;
		}
	}
	ASSERT(pnum != *num_needed || mp_list == NULL);
	if (mp_list != NULL) {
		mcache_free_ext(cp, mp_list);
	}

	if (pnum > 0) {
		mtype_stat_add(MT_DATA, pnum);
		mtype_stat_sub(MT_FREE, pnum);
	}

	if (wantall && (pnum != *num_needed)) {
		if (top != NULL) {
			m_freem_list(top);
		}
		return NULL;
	}

	if (pnum > *num_needed) {
		printf("%s: File a radar related to <rdar://10146739>. \
			needed = %u, pnum = %u, num_needed = %u \n",
		    __func__, needed, pnum, *num_needed);
	}

	*num_needed = pnum;
	return top;
}

  
                                                                    
                                                                          
                                                                        
                                                                          
                                                                        
                                                                            
                                                                        
                                                                              
                                                                        
                             
   
__private_extern__ struct mbuf *
m_allocpacket_internal(unsigned int *numlist, size_t packetlen,
    unsigned int *maxsegments, int wait, int wantall, size_t wantsize)
{
	struct mbuf **np, *top, *first = NULL;
	size_t bufsize, r_bufsize;
	unsigned int num = 0;
	unsigned int nsegs = 0;
	unsigned int needed, resid;
	int mcflags = MSLEEPF(wait);
	mcache_obj_t *mp_list = NULL, *rmp_list = NULL;
	mcache_t *cp = NULL, *rcp = NULL;

	if (*numlist == 0) {
		return NULL;
	}

	top = NULL;
	np = &top;

	if (wantsize == 0) {
		if (packetlen <= MINCLSIZE) {
			bufsize = packetlen;
		} else if (packetlen > m_maxsize(MC_CL)) {
			                                                   
			if (packetlen <= m_maxsize(MC_BIGCL) || njcl == 0) {
				bufsize = m_maxsize(MC_BIGCL);
			} else {
				bufsize = m_maxsize(MC_16KCL);
			}
		} else {
			bufsize = m_maxsize(MC_CL);
		}
	} else if (wantsize == m_maxsize(MC_CL) ||
	    wantsize == m_maxsize(MC_BIGCL) ||
	    (wantsize == m_maxsize(MC_16KCL) && njcl > 0)) {
		bufsize = wantsize;
	} else {
		*numlist = 0;
		return NULL;
	}

	if (bufsize <= MHLEN) {
		nsegs = 1;
	} else if (bufsize <= MINCLSIZE) {
		if (maxsegments != NULL && *maxsegments == 1) {
			bufsize = m_maxsize(MC_CL);
			nsegs = 1;
		} else {
			nsegs = 2;
		}
	} else if (bufsize == m_maxsize(MC_16KCL)) {
		VERIFY(njcl > 0);
		nsegs = ((packetlen - 1) >> M16KCLSHIFT) + 1;
	} else if (bufsize == m_maxsize(MC_BIGCL)) {
		nsegs = ((packetlen - 1) >> MBIGCLSHIFT) + 1;
	} else {
		nsegs = ((packetlen - 1) >> MCLSHIFT) + 1;
	}
	if (maxsegments != NULL) {
		if (*maxsegments && nsegs > *maxsegments) {
			*maxsegments = nsegs;
			*numlist = 0;
			return NULL;
		}
		*maxsegments = nsegs;
	}

	  
                                                                 
                                                                   
                                                               
                                    
    
	if (!wantall || (mcflags & MCR_NOSLEEP)) {
		mcflags |= MCR_TRYHARD;
	}

	  
                                                                 
                                                                    
                                                                   
                                                                  
                                                           
    
	if (bufsize <= MINCLSIZE) {
		                                                           
		ASSERT(bufsize <= MHLEN || nsegs == 2);
		cp = m_cache(MC_MBUF);
		needed = mcache_alloc_ext(cp, &mp_list,
		    (*numlist) * nsegs, mcflags);

		  
                                                            
                                                            
                                                           
                                                         
                                      
     
		if (bufsize > MHLEN && (needed & 0x1)) {
			needed--;
		}

		while (num < needed) {
			struct mbuf *m;

			m = (struct mbuf *)mp_list;
			mp_list = mp_list->obj_next;
			ASSERT(m != NULL);

			MBUF_INIT(m, 1, MT_DATA);
			num++;
			if (bufsize > MHLEN) {
				                                          
				m->m_next = (struct mbuf *)mp_list;
				mp_list = mp_list->obj_next;
				ASSERT(m->m_next != NULL);

				MBUF_INIT(m->m_next, 0, MT_DATA);
				num++;
			}
			*np = m;
			np = &m->m_nextpkt;
		}
		ASSERT(num != *numlist || mp_list == NULL);

		if (num > 0) {
			mtype_stat_add(MT_DATA, num);
			mtype_stat_sub(MT_FREE, num);
		}
		num /= nsegs;

		                                          
		if (num == *numlist) {
			return top;
		}

		goto fail;
	}

	  
                                                                     
                                                                      
                              
   
                                                                
   
                                                                      
                                                                      
                                                                   
                                                                
                                                                  
                                                                      
                                                                    
                                 
    
	r_bufsize = bufsize;
	resid = packetlen > bufsize ? packetlen % bufsize : 0;
	if (resid > 0) {
		                                                         
		if (wantsize == 0 && packetlen > MINCLSIZE) {
			  
                                                    
                                                     
                                   
      
			if (njcl > 0 && resid > m_maxsize(MC_BIGCL)) {
				r_bufsize = m_maxsize(MC_16KCL);
			} else if (resid > m_maxsize(MC_CL)) {
				r_bufsize = m_maxsize(MC_BIGCL);
			} else {
				r_bufsize = m_maxsize(MC_CL);
			}
		} else {
			                                                     
			resid = 0;
		}
	}

	needed = *numlist;
	if (resid > 0) {
		  
                                                              
                                                               
                                                            
                                            
     
		if (r_bufsize <= m_maxsize(MC_CL)) {
			rcp = m_cache(MC_MBUF_CL);
		} else if (r_bufsize <= m_maxsize(MC_BIGCL)) {
			rcp = m_cache(MC_MBUF_BIGCL);
		} else {
			rcp = m_cache(MC_MBUF_16KCL);
		}
		needed = mcache_alloc_ext(rcp, &rmp_list, *numlist, mcflags);

		if (needed == 0) {
			goto fail;
		}

		                                                 
		ASSERT(nsegs > 1);
		nsegs--;
	}

	  
                                                                
                                                           
    
	if (bufsize <= m_maxsize(MC_CL)) {
		cp = m_cache(MC_MBUF_CL);
	} else if (bufsize <= m_maxsize(MC_BIGCL)) {
		cp = m_cache(MC_MBUF_BIGCL);
	} else {
		cp = m_cache(MC_MBUF_16KCL);
	}
	needed = mcache_alloc_ext(cp, &mp_list, needed * nsegs, mcflags);

	                                                             
	needed = (needed / nsegs) * nsegs;
	if (needed == 0) {
		goto fail;
	}

	if (resid > 0) {
		  
                                                             
                                                             
                                                         
                                          
     
		ASSERT(nsegs > 0);
		needed += needed / nsegs;
		nsegs++;
	}

	for (;;) {
		struct mbuf *m;
		u_int16_t flag;
		struct ext_ref *rfa;
		void *cl;
		int pkthdr;
		m_ext_free_func_t m_free_func;

		++num;
		if (nsegs == 1 || (num % nsegs) != 0 || resid == 0) {
			m = (struct mbuf *)mp_list;
			mp_list = mp_list->obj_next;
		} else {
			m = (struct mbuf *)rmp_list;
			rmp_list = rmp_list->obj_next;
		}
		m_free_func = m_get_ext_free(m);
		ASSERT(m != NULL);
		VERIFY(m->m_type == MT_FREE && m->m_flags == M_EXT);
		VERIFY(m_free_func == NULL || m_free_func == m_bigfree ||
		    m_free_func == m_16kfree);

		cl = m->m_ext.ext_buf;
		rfa = m_get_rfa(m);

		ASSERT(cl != NULL && rfa != NULL);
		VERIFY(MBUF_IS_COMPOSITE(m));

		flag = MEXT_FLAGS(m);

		pkthdr = (nsegs == 1 || (num % nsegs) == 1);
		if (pkthdr) {
			first = m;
		}
		MBUF_INIT(m, pkthdr, MT_DATA);
		if (m_free_func == m_16kfree) {
			MBUF_16KCL_INIT(m, cl, rfa, 1, flag);
		} else if (m_free_func == m_bigfree) {
			MBUF_BIGCL_INIT(m, cl, rfa, 1, flag);
		} else {
			MBUF_CL_INIT(m, cl, rfa, 1, flag);
		}

		*np = m;
		if ((num % nsegs) == 0) {
			np = &first->m_nextpkt;
		} else {
			np = &m->m_next;
		}

		if (num == needed) {
			break;
		}
	}

	if (num > 0) {
		mtype_stat_add(MT_DATA, num);
		mtype_stat_sub(MT_FREE, num);
	}

	num /= nsegs;

	                                          
	if (num == *numlist) {
		ASSERT(mp_list == NULL && rmp_list == NULL);
		return top;
	}

fail:
	                                      
	if (mp_list != NULL) {
		mcache_free_ext(cp, mp_list);
	}
	if (rmp_list != NULL) {
		mcache_free_ext(rcp, rmp_list);
	}
	if (wantall && top != NULL) {
		m_freem_list(top);
		*numlist = 0;
		return NULL;
	}
	*numlist = num;
	return top;
}

  
                                                                            
                           
   
__private_extern__ struct mbuf *
m_getpacket_how(int wait)
{
	unsigned int num_needed = 1;

	return m_getpackets_internal(&num_needed, 1, wait, 1,
	           m_maxsize(MC_CL));
}

  
                                                                            
                           
   
struct mbuf *
m_getpacket(void)
{
	unsigned int num_needed = 1;

	return m_getpackets_internal(&num_needed, 1, M_WAIT, 1,
	           m_maxsize(MC_CL));
}

  
                                                                          
                                                                           
                                                                             
                                                                             
                                                      
   
struct mbuf *
m_getpackets(int num_needed, int num_with_pkthdrs, int how)
{
	unsigned int n = num_needed;

	return m_getpackets_internal(&n, num_with_pkthdrs, how, 0,
	           m_maxsize(MC_CL));
}

  
                                                                    
                         
   
struct mbuf *
m_getpackethdrs(int num_needed, int how)
{
	struct mbuf *m;
	struct mbuf **np, *top;

	top = NULL;
	np = &top;

	while (num_needed--) {
		m = _M_RETRYHDR(how, MT_DATA);
		if (m == NULL) {
			break;
		}

		*np = m;
		np = &m->m_nextpkt;
	}

	return top;
}

  
                                                                           
                                                 
   
int
m_freem_list(struct mbuf *m)
{
	struct mbuf *nextpkt;
	mcache_obj_t *mp_list = NULL;
	mcache_obj_t *mcl_list = NULL;
	mcache_obj_t *mbc_list = NULL;
	mcache_obj_t *m16k_list = NULL;
	mcache_obj_t *m_mcl_list = NULL;
	mcache_obj_t *m_mbc_list = NULL;
	mcache_obj_t *m_m16k_list = NULL;
	mcache_obj_t *ref_list = NULL;
	int pktcount = 0;
	int mt_free = 0, mt_data = 0, mt_header = 0, mt_soname = 0, mt_tag = 0;

	while (m != NULL) {
		pktcount++;

		nextpkt = m->m_nextpkt;
		m->m_nextpkt = NULL;

		while (m != NULL) {
			struct mbuf *next = m->m_next;
			mcache_obj_t *o, *rfa;
			u_int32_t composite;
			u_int16_t refcnt;
			m_ext_free_func_t m_free_func;

			if (m->m_type == MT_FREE) {
				panic("m_free: freeing an already freed mbuf");
			}

			if (m->m_flags & M_PKTHDR) {
				                                     
				m_redzone_verify(m);
				                                                
				m_tag_delete_chain(m, NULL);
			}

			if (!(m->m_flags & M_EXT)) {
				mt_free++;
				goto simple_free;
			}

			if (MBUF_IS_PAIRED(m) && m_free_paired(m)) {
				m = next;
				continue;
			}

			mt_free++;

			o = (mcache_obj_t *)(void *)m->m_ext.ext_buf;
			refcnt = m_decref(m);
			composite = (MEXT_FLAGS(m) & EXTF_COMPOSITE);
			m_free_func = m_get_ext_free(m);
			if (refcnt == MEXT_MINREF(m) && !composite) {
				if (m_free_func == NULL) {
					o->obj_next = mcl_list;
					mcl_list = o;
				} else if (m_free_func == m_bigfree) {
					o->obj_next = mbc_list;
					mbc_list = o;
				} else if (m_free_func == m_16kfree) {
					o->obj_next = m16k_list;
					m16k_list = o;
				} else {
					(*(m_free_func))((caddr_t)o,
					    m->m_ext.ext_size,
					    m_get_ext_arg(m));
				}
				rfa = (mcache_obj_t *)(void *)m_get_rfa(m);
				rfa->obj_next = ref_list;
				ref_list = rfa;
				m_set_ext(m, NULL, NULL, NULL);
			} else if (refcnt == MEXT_MINREF(m) && composite) {
				VERIFY(!(MEXT_FLAGS(m) & EXTF_PAIRED));
				VERIFY(m->m_type != MT_FREE);
				  
                                              
                                             
       
				if (m->m_type == MT_DATA) {
					mt_data++;
				} else if (m->m_type == MT_HEADER) {
					mt_header++;
				} else if (m->m_type == MT_SONAME) {
					mt_soname++;
				} else if (m->m_type == MT_TAG) {
					mt_tag++;
				} else {
					mtype_stat_dec(m->m_type);
				}

				m->m_type = MT_FREE;
				m->m_flags = M_EXT;
				m->m_len = 0;
				m->m_next = m->m_nextpkt = NULL;

				MEXT_FLAGS(m) &= ~EXTF_READONLY;

				                                        
				o = (mcache_obj_t *)m;
				if (m_free_func == NULL) {
					o->obj_next = m_mcl_list;
					m_mcl_list = o;
				} else if (m_free_func == m_bigfree) {
					o->obj_next = m_mbc_list;
					m_mbc_list = o;
				} else {
					VERIFY(m_free_func == m_16kfree);
					o->obj_next = m_m16k_list;
					m_m16k_list = o;
				}
				m = next;
				continue;
			}
simple_free:
			  
                                             
                                            
      
			if (m->m_type == MT_DATA) {
				mt_data++;
			} else if (m->m_type == MT_HEADER) {
				mt_header++;
			} else if (m->m_type == MT_SONAME) {
				mt_soname++;
			} else if (m->m_type == MT_TAG) {
				mt_tag++;
			} else if (m->m_type != MT_FREE) {
				mtype_stat_dec(m->m_type);
			}

			m->m_type = MT_FREE;
			m->m_flags = m->m_len = 0;
			m->m_next = m->m_nextpkt = NULL;

			((mcache_obj_t *)m)->obj_next = mp_list;
			mp_list = (mcache_obj_t *)m;

			m = next;
		}

		m = nextpkt;
	}

	if (mt_free > 0) {
		mtype_stat_add(MT_FREE, mt_free);
	}
	if (mt_data > 0) {
		mtype_stat_sub(MT_DATA, mt_data);
	}
	if (mt_header > 0) {
		mtype_stat_sub(MT_HEADER, mt_header);
	}
	if (mt_soname > 0) {
		mtype_stat_sub(MT_SONAME, mt_soname);
	}
	if (mt_tag > 0) {
		mtype_stat_sub(MT_TAG, mt_tag);
	}

	if (mp_list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF), mp_list);
	}
	if (mcl_list != NULL) {
		mcache_free_ext(m_cache(MC_CL), mcl_list);
	}
	if (mbc_list != NULL) {
		mcache_free_ext(m_cache(MC_BIGCL), mbc_list);
	}
	if (m16k_list != NULL) {
		mcache_free_ext(m_cache(MC_16KCL), m16k_list);
	}
	if (m_mcl_list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF_CL), m_mcl_list);
	}
	if (m_mbc_list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF_BIGCL), m_mbc_list);
	}
	if (m_m16k_list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF_16KCL), m_m16k_list);
	}
	if (ref_list != NULL) {
		mcache_free_ext(ref_cache, ref_list);
	}

	return pktcount;
}

void
m_freem(struct mbuf *m)
{
	while (m != NULL) {
		m = m_free(m);
	}
}

  
                            
   
  
                                                                             
                                                           
  
                                                                      
                                                                           
                                                                         
                                                                      
                                 
   
void
m_align(struct mbuf *m, int len)
{
	int adjust = 0;

	                                            
	VERIFY(m->m_data == M_START(m));
	VERIFY(len >= 0);
	VERIFY(len <= M_SIZE(m));
	adjust = M_SIZE(m) - len;
	m->m_data += adjust & ~(sizeof(long) - 1);
}

  
                                                                         
                                                          
   
struct mbuf *
m_prepend(struct mbuf *m, int len, int how)
{
	struct mbuf *mn;

	_MGET(mn, how, m->m_type);
	if (mn == NULL) {
		m_freem(m);
		return NULL;
	}
	if (m->m_flags & M_PKTHDR) {
		M_COPY_PKTHDR(mn, m);
		m->m_flags &= ~M_PKTHDR;
	}
	mn->m_next = m;
	m = mn;
	if (m->m_flags & M_PKTHDR) {
		VERIFY(len <= MHLEN);
		MH_ALIGN(m, len);
	} else {
		VERIFY(len <= MLEN);
		M_ALIGN(m, len);
	}
	m->m_len = len;
	return m;
}

  
                                                                       
                                             
   
struct mbuf *
m_prepend_2(struct mbuf *m, int len, int how, int align)
{
	if (M_LEADINGSPACE(m) >= len &&
	    (!align || IS_P2ALIGNED((m->m_data - len), sizeof(u_int32_t)))) {
		m->m_data -= len;
		m->m_len += len;
	} else {
		m = m_prepend(m, len, how);
	}
	if ((m) && (m->m_flags & M_PKTHDR)) {
		m->m_pkthdr.len += len;
	}
	return m;
}

  
                                                                         
                                                                         
                                                                   
   
int MCFail;

struct mbuf *
m_copym_mode(struct mbuf *m, int off0, int len, int wait, uint32_t mode)
{
	struct mbuf *n, *mhdr = NULL, **np;
	int off = off0;
	struct mbuf *top;
	int copyhdr = 0;

	if (off < 0 || len < 0) {
		panic("m_copym: invalid offset %d or len %d", off, len);
	}

	VERIFY((mode != M_COPYM_MUST_COPY_HDR &&
	    mode != M_COPYM_MUST_MOVE_HDR) || (m->m_flags & M_PKTHDR));

	if ((off == 0 && (m->m_flags & M_PKTHDR)) ||
	    mode == M_COPYM_MUST_COPY_HDR || mode == M_COPYM_MUST_MOVE_HDR) {
		mhdr = m;
		copyhdr = 1;
	}

	while (off >= m->m_len) {
		if (m->m_next == NULL) {
			panic("m_copym: invalid mbuf chain");
		}
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = NULL;

	while (len > 0) {
		if (m == NULL) {
			if (len != M_COPYALL) {
				panic("m_copym: len != M_COPYALL");
			}
			break;
		}

		if (copyhdr) {
			n = _M_RETRYHDR(wait, m->m_type);
		} else {
			n = _M_RETRY(wait, m->m_type);
		}
		*np = n;

		if (n == NULL) {
			goto nospace;
		}

		if (copyhdr != 0) {
			if ((mode == M_COPYM_MOVE_HDR) ||
			    (mode == M_COPYM_MUST_MOVE_HDR)) {
				M_COPY_PKTHDR(n, mhdr);
			} else if ((mode == M_COPYM_COPY_HDR) ||
			    (mode == M_COPYM_MUST_COPY_HDR)) {
				if (m_dup_pkthdr(n, mhdr, wait) == 0) {
					goto nospace;
				}
			}
			if (len == M_COPYALL) {
				n->m_pkthdr.len -= off0;
			} else {
				n->m_pkthdr.len = len;
			}
			copyhdr = 0;
			  
                                                       
                                                        
      
			if (mhdr != m) {
				np = &n->m_next;
				continue;
			}
		}
		n->m_len = MIN(len, (m->m_len - off));
		if (m->m_flags & M_EXT) {
			n->m_ext = m->m_ext;
			m_incref(m);
			n->m_data = m->m_data + off;
			n->m_flags |= M_EXT;
		} else {
			  
                                              
      
			if (n->m_flags & M_PKTHDR) {
				n->m_len = MIN(n->m_len, MHLEN);
			} else {
				n->m_len = MIN(n->m_len, MLEN);
			}

			if (MTOD(n, char *) + n->m_len > ((char *)n) + MSIZE) {
				panic("%s n %p copy overflow",
				    __func__, n);
			}

			bcopy(MTOD(m, caddr_t) + off, MTOD(n, caddr_t),
			    (unsigned)n->m_len);
		}
		if (len != M_COPYALL) {
			len -= n->m_len;
		}
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}

	if (top == NULL) {
		MCFail++;
	}

	return top;
nospace:

	m_freem(top);
	MCFail++;
	return NULL;
}


struct mbuf *
m_copym(struct mbuf *m, int off0, int len, int wait)
{
	return m_copym_mode(m, off0, len, wait, M_COPYM_MOVE_HDR);
}

  
                                                                          
                                                                         
                                                                          
                                         
   
struct mbuf *
m_copym_with_hdrs(struct mbuf *m0, int off0, int len0, int wait,
    struct mbuf **m_lastm, int *m_off, uint32_t mode)
{
	struct mbuf *m = m0, *n, **np = NULL;
	int off = off0, len = len0;
	struct mbuf *top = NULL;
	int mcflags = MSLEEPF(wait);
	int copyhdr = 0;
	int type = 0;
	mcache_obj_t *list = NULL;
	int needed = 0;

	if (off == 0 && (m->m_flags & M_PKTHDR)) {
		copyhdr = 1;
	}

	if (m_lastm != NULL && *m_lastm != NULL) {
		m = *m_lastm;
		off = *m_off;
	} else {
		while (off >= m->m_len) {
			off -= m->m_len;
			m = m->m_next;
		}
	}

	n = m;
	while (len > 0) {
		needed++;
		ASSERT(n != NULL);
		len -= MIN(len, (n->m_len - ((needed == 1) ? off : 0)));
		n = n->m_next;
	}
	needed++;
	len = len0;

	  
                                                               
                                                                
                     
    
	if (mcflags & MCR_NOSLEEP) {
		mcflags |= MCR_TRYHARD;
	}

	if (mcache_alloc_ext(m_cache(MC_MBUF), &list, needed,
	    mcflags) != needed) {
		goto nospace;
	}

	needed = 0;
	while (len > 0) {
		n = (struct mbuf *)list;
		list = list->obj_next;
		ASSERT(n != NULL && m != NULL);

		type = (top == NULL) ? MT_HEADER : m->m_type;
		MBUF_INIT(n, (top == NULL), type);

		if (top == NULL) {
			top = n;
			np = &top->m_next;
			continue;
		} else {
			needed++;
			*np = n;
		}

		if (copyhdr) {
			if ((mode == M_COPYM_MOVE_HDR) ||
			    (mode == M_COPYM_MUST_MOVE_HDR)) {
				M_COPY_PKTHDR(n, m);
			} else if ((mode == M_COPYM_COPY_HDR) ||
			    (mode == M_COPYM_MUST_COPY_HDR)) {
				if (m_dup_pkthdr(n, m, wait) == 0) {
					goto nospace;
				}
			}
			n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		n->m_len = MIN(len, (m->m_len - off));

		if (m->m_flags & M_EXT) {
			n->m_ext = m->m_ext;
			m_incref(m);
			n->m_data = m->m_data + off;
			n->m_flags |= M_EXT;
		} else {
			if (MTOD(n, char *) + n->m_len > ((char *)n) + MSIZE) {
				panic("%s n %p copy overflow",
				    __func__, n);
			}

			bcopy(MTOD(m, caddr_t) + off, MTOD(n, caddr_t),
			    (unsigned)n->m_len);
		}
		len -= n->m_len;

		if (len == 0) {
			if (m_lastm != NULL && m_off != NULL) {
				if ((off + n->m_len) == m->m_len) {
					*m_lastm = m->m_next;
					*m_off  = 0;
				} else {
					*m_lastm = m;
					*m_off  = off + n->m_len;
				}
			}
			break;
		}
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}

	mtype_stat_inc(MT_HEADER);
	mtype_stat_add(type, needed);
	mtype_stat_sub(MT_FREE, needed + 1);

	ASSERT(list == NULL);
	return top;

nospace:
	if (list != NULL) {
		mcache_free_ext(m_cache(MC_MBUF), list);
	}
	if (top != NULL) {
		m_freem(top);
	}
	MCFail++;
	return NULL;
}

  
                                                                        
                                                         
   
void
m_copydata(struct mbuf *m, int off, int len, void *vp)
{
	int off0 = off, len0 = len;
	struct mbuf *m0 = m;
	unsigned count;
	char *cp = vp;

	if (__improbable(off < 0 || len < 0)) {
		panic("%s: invalid offset %d or len %d", __func__, off, len);
		                
	}

	while (off > 0) {
		if (__improbable(m == NULL)) {
			panic("%s: invalid mbuf chain %p [off %d, len %d]",
			    __func__, m0, off0, len0);
			                
		}
		if (off < m->m_len) {
			break;
		}
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (__improbable(m == NULL)) {
			panic("%s: invalid mbuf chain %p [off %d, len %d]",
			    __func__, m0, off0, len0);
			                
		}
		count = MIN(m->m_len - off, len);
		bcopy(MTOD(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

  
                                                                       
                                                
   
void
m_cat(struct mbuf *m, struct mbuf *n)
{
	while (m->m_next) {
		m = m->m_next;
	}
	while (n) {
		if ((m->m_flags & M_EXT) ||
		    m->m_data + m->m_len + n->m_len >= &m->m_dat[MLEN]) {
			                              
			m->m_next = n;
			return;
		}
		                                            
		bcopy(MTOD(n, caddr_t), MTOD(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

void
m_adj(struct mbuf *mp, int req_len)
{
	int len = req_len;
	struct mbuf *m;
	int count;

	if ((m = mp) == NULL) {
		return;
	}
	if (len >= 0) {
		  
                    
     
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_data += len;
				len = 0;
			}
		}
		m = mp;
		if (m->m_flags & M_PKTHDR) {
			m->m_pkthdr.len -= (req_len - len);
		}
	} else {
		  
                                          
                                                      
                                                        
                                                       
                              
     
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0) {
				break;
			}
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			m = mp;
			if (m->m_flags & M_PKTHDR) {
				m->m_pkthdr.len -= len;
			}
			return;
		}
		count -= len;
		if (count < 0) {
			count = 0;
		}
		  
                                         
                                                     
                                                 
     
		m = mp;
		if (m->m_flags & M_PKTHDR) {
			m->m_pkthdr.len = count;
		}
		for (; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while ((m = m->m_next)) {
			m->m_len = 0;
		}
	}
}

  
                                                          
                                                         
                                                                 
                                                               
                                                                          
                                                                   
   
int MPFail;

struct mbuf *
m_pullup(struct mbuf *n, int len)
{
	struct mbuf *m;
	int count;
	int space;

	                             
	if (n == NULL) {
		panic("%s: n == NULL", __func__);
	}
	if (len < 0) {
		os_log_info(OS_LOG_DEFAULT, "%s: failed negative len %d",
		    __func__, len);
		goto bad;
	}
	if (len > MLEN) {
		os_log_info(OS_LOG_DEFAULT, "%s: failed len %d too big",
		    __func__, len);
		goto bad;
	}
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data >= &n->m_dat[MLEN]) {
		os_log_info(OS_LOG_DEFAULT, "%s: m_data out of bounds",
		    __func__);
		goto bad;
	}

	  
                                                            
                                                  
                                                          
    
	if ((n->m_flags & M_EXT) == 0 &&
	    len < &n->m_dat[MLEN] - n->m_data && n->m_next != NULL) {
		if (n->m_len >= len) {
			return n;
		}
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MHLEN) {
			goto bad;
		}
		_MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0) {
			goto bad;
		}
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR) {
			M_COPY_PKTHDR(m, n);
			n->m_flags &= ~M_PKTHDR;
		}
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = MIN(MIN(MAX(len, max_protohdr), space), n->m_len);
		bcopy(MTOD(n, caddr_t), MTOD(m, caddr_t) + m->m_len,
		    (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len != 0) {
			n->m_data += count;
		} else {
			n = m_free(n);
		}
	} while (len > 0 && n != NULL);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return m;
bad:
	m_freem(n);
	MPFail++;
	return 0;
}

  
                                                                       
                                                                            
                                                           
   
__private_extern__ int MSFail = 0;

__private_extern__ struct mbuf *
m_copyup(struct mbuf *n, int len, int dstoff)
{
	struct mbuf *m;
	int count, space;

	VERIFY(len >= 0 && dstoff >= 0);

	if (len > (MHLEN - dstoff)) {
		goto bad;
	}
	MGET(m, M_DONTWAIT, n->m_type);
	if (m == NULL) {
		goto bad;
	}
	m->m_len = 0;
	if (n->m_flags & M_PKTHDR) {
		m_copy_pkthdr(m, n);
		n->m_flags &= ~M_PKTHDR;
	}
	m->m_data += dstoff;
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		memcpy(mtod(m, caddr_t) + m->m_len, mtod(n, caddr_t),
		    (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len) {
			n->m_data += count;
		} else {
			n = m_free(n);
		}
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return m;
bad:
	m_freem(n);
	MSFail++;
	return NULL;
}

  
                                                               
                                                                         
                                                       
   
struct mbuf *
m_split(struct mbuf *m0, int len0, int wait)
{
	return m_split0(m0, len0, wait, 1);
}

static struct mbuf *
m_split0(struct mbuf *m0, int len0, int wait, int copyhdr)
{
	struct mbuf *m, *n;
	unsigned len = len0, remain;

	  
                                                              
                       
    
	for (m = m0; m && len > m->m_len; m = m->m_next) {
		len -= m->m_len;
	}
	if (m == NULL) {
		return NULL;
	}
	  
                                                    
                                        
   
                                   
                                         
    
	remain = m->m_len - len;

	  
                                                                 
                                                                 
                                                 
    
	if (copyhdr && (m0->m_flags & M_PKTHDR) && remain == 0) {
		_MGETHDR(n, wait, m0->m_type);
		if (n == NULL) {
			return NULL;
		}
		n->m_next = m->m_next;
		m->m_next = NULL;
		n->m_pkthdr.rcvif = m0->m_pkthdr.rcvif;
		n->m_pkthdr.len = m0->m_pkthdr.len - len0;
		m0->m_pkthdr.len = len0;
		return n;
	}
	if (copyhdr && (m0->m_flags & M_PKTHDR)) {
		_MGETHDR(n, wait, m0->m_type);
		if (n == NULL) {
			return NULL;
		}
		n->m_pkthdr.rcvif = m0->m_pkthdr.rcvif;
		n->m_pkthdr.len = m0->m_pkthdr.len - len0;
		m0->m_pkthdr.len = len0;

		  
                                          
                                              
                                                  
                                       
     
		if (m->m_flags & M_EXT) {
			goto extpacket;
		}
		if (remain > MHLEN) {
			                                
			MH_ALIGN(n, 0);
			n->m_next = m_split(m, len, wait);
			if (n->m_next == NULL) {
				(void) m_free(n);
				return NULL;
			} else {
				return n;
			}
		} else {
			MH_ALIGN(n, remain);
		}
	} else if (remain == 0) {
		n = m->m_next;
		m->m_next = NULL;
		return n;
	} else {
		_MGET(n, wait, m->m_type);
		if (n == NULL) {
			return NULL;
		}

		if ((m->m_flags & M_EXT) == 0) {
			VERIFY(remain <= MLEN);
			M_ALIGN(n, remain);
		}
	}
extpacket:
	if (m->m_flags & M_EXT) {
		n->m_flags |= M_EXT;
		n->m_ext = m->m_ext;
		m_incref(m);
		n->m_data = m->m_data + len;
	} else {
		bcopy(MTOD(m, caddr_t) + len, MTOD(n, caddr_t), remain);
	}
	n->m_len = remain;
	m->m_len = len;
	n->m_next = m->m_next;
	m->m_next = NULL;
	return n;
}

  
                                                       
   
struct mbuf *
m_devget(char *buf, int totlen, int off0, struct ifnet *ifp,
    void (*copy)(const void *, void *, size_t))
{
	struct mbuf *m;
	struct mbuf *top = NULL, **mp = &top;
	int off = off0, len;
	char *cp;
	char *epkt;

	cp = buf;
	epkt = cp + totlen;
	if (off) {
		  
                                                          
                                                   
     
		cp += off + 2 * sizeof(u_int16_t);
		totlen -= 2 * sizeof(u_int16_t);
	}
	_MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		return NULL;
	}
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	while (totlen > 0) {
		if (top != NULL) {
			_MGET(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				m_freem(top);
				return NULL;
			}
			m->m_len = MLEN;
		}
		len = MIN(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT) {
				m->m_len = len = MIN(len, m_maxsize(MC_CL));
			} else {
				                                            
				if (top != NULL) {
					m_freem(top);
				}
				m_freem(m);
				return NULL;
			}
		} else {
			  
                                                       
      
			if (len < m->m_len) {
				if (top == NULL &&
				    len + max_linkhdr <= m->m_len) {
					m->m_data += max_linkhdr;
				}
				m->m_len = len;
			} else {
				len = m->m_len;
			}
		}
		if (copy) {
			copy(cp, MTOD(m, caddr_t), (unsigned)len);
		} else {
			bcopy(cp, MTOD(m, caddr_t), (unsigned)len);
		}
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt) {
			cp = buf;
		}
	}
	return top;
}

#ifndef MBUF_GROWTH_NORMAL_THRESH
#define MBUF_GROWTH_NORMAL_THRESH 25
#endif

  
                                     
   
static int
m_howmany(int num, size_t bufsize)
{
	int i = 0, j = 0;
	u_int32_t m_mbclusters, m_clusters, m_bigclusters, m_16kclusters;
	u_int32_t m_mbfree, m_clfree, m_bigclfree, m_16kclfree;
	u_int32_t sumclusters, freeclusters;
	u_int32_t percent_pool, percent_kmem;
	u_int32_t mb_growth, mb_growth_thresh;

	VERIFY(bufsize == m_maxsize(MC_BIGCL) ||
	    bufsize == m_maxsize(MC_16KCL));

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	                                 
	m_mbclusters = m_total(MC_MBUF) >> NMBPCLSHIFT;
	m_clusters = m_total(MC_CL);
	m_bigclusters = m_total(MC_BIGCL) << NCLPBGSHIFT;
	m_16kclusters = m_total(MC_16KCL);
	sumclusters = m_mbclusters + m_clusters + m_bigclusters;

	m_mbfree = m_infree(MC_MBUF) >> NMBPCLSHIFT;
	m_clfree = m_infree(MC_CL);
	m_bigclfree = m_infree(MC_BIGCL) << NCLPBGSHIFT;
	m_16kclfree = m_infree(MC_16KCL);
	freeclusters = m_mbfree + m_clfree + m_bigclfree;

	                                                 
	if ((bufsize == m_maxsize(MC_BIGCL) && sumclusters >= nclusters) ||
	    (njcl > 0 && bufsize == m_maxsize(MC_16KCL) &&
	    (m_16kclusters << NCLPJCLSHIFT) >= njcl)) {
		mbwdog_logger("maxed out nclusters (%u >= %u) or njcl (%u >= %u)",
		    sumclusters, nclusters,
		    (m_16kclusters << NCLPJCLSHIFT), njcl);
		return 0;
	}

	if (bufsize == m_maxsize(MC_BIGCL)) {
		                   
		if (m_bigclusters < m_minlimit(MC_BIGCL)) {
			return m_minlimit(MC_BIGCL) - m_bigclusters;
		}

		percent_pool =
		    ((sumclusters - freeclusters) * 100) / sumclusters;
		percent_kmem = (sumclusters * 100) / nclusters;

		  
                                                      
                                             
     
		if (percent_kmem < MBUF_GROWTH_NORMAL_THRESH) {
			mb_growth = MB_GROWTH_NORMAL;
		} else {
			mb_growth = MB_GROWTH_AGGRESSIVE;
		}

		if (percent_kmem < 5) {
			                             
			i = num;
		} else {
			                                                  
			if (m_infree(MC_BIGCL) >= MBIGCL_LOWAT &&
			    m_total(MC_BIGCL) >=
			    MBIGCL_LOWAT + m_minlimit(MC_BIGCL)) {
				return 0;
			}

			                                                 
			if (num >= m_infree(MC_BIGCL)) {
				i = num - m_infree(MC_BIGCL);
			}
			if (num > m_total(MC_BIGCL) - m_minlimit(MC_BIGCL)) {
				j = num - (m_total(MC_BIGCL) -
				    m_minlimit(MC_BIGCL));
			}

			i = MAX(i, j);

			  
                                                    
                                               
      
			mb_growth_thresh = 100 - (100 / (1 << mb_growth));
			if (percent_pool > mb_growth_thresh) {
				j = ((sumclusters + num) >> mb_growth) -
				    freeclusters;
			}
			i = MAX(i, j);
		}

		                                              
		if (i + m_bigclusters >= m_maxlimit(MC_BIGCL)) {
			i = m_maxlimit(MC_BIGCL) - m_bigclusters;
		}
		if ((i << 1) + sumclusters >= nclusters) {
			i = (nclusters - sumclusters) >> 1;
		}
		VERIFY((m_total(MC_BIGCL) + i) <= m_maxlimit(MC_BIGCL));
		VERIFY(sumclusters + (i << 1) <= nclusters);
	} else {             
		VERIFY(njcl > 0);
		                                                
		if (num >= m_16kclfree) {
			i = num - m_16kclfree;
		}

		                                         
		if (((m_16kclusters + num) >> 1) > m_16kclfree) {
			j = ((m_16kclusters + num) >> 1) - m_16kclfree;
		}
		i = MAX(i, j);

		                                            
		if ((i + m_total(MC_16KCL)) >= m_maxlimit(MC_16KCL)) {
			i = m_maxlimit(MC_16KCL) - m_total(MC_16KCL);
		}
	}
	return i;
}
  
                                                   
   
unsigned int
m_length(struct mbuf *m)
{
	struct mbuf *m0;
	unsigned int pktlen;

	if (m->m_flags & M_PKTHDR) {
		return m->m_pkthdr.len;
	}

	pktlen = 0;
	for (m0 = m; m0 != NULL; m0 = m0->m_next) {
		pktlen += m0->m_len;
	}
	return pktlen;
}

  
                                                              
                                                              
                      
   
void
m_copyback(struct mbuf *m0, int off, int len, const void *cp)
{
#if DEBUG
	struct mbuf *origm = m0;
	int error;
#endif            

	if (m0 == NULL) {
		return;
	}

#if DEBUG
	error =
#endif            
	m_copyback0(&m0, off, len, cp,
	    M_COPYBACK0_COPYBACK | M_COPYBACK0_EXTEND, M_DONTWAIT);

#if DEBUG
	if (error != 0 || (m0 != NULL && origm != m0)) {
		panic("m_copyback");
	}
#endif            
}

struct mbuf *
m_copyback_cow(struct mbuf *m0, int off, int len, const void *cp, int how)
{
	int error;

	                                   
	VERIFY(off + len <= m_length(m0));

	error = m_copyback0(&m0, off, len, cp,
	    M_COPYBACK0_COPYBACK | M_COPYBACK0_COW, how);
	if (error) {
		  
                                            
                         
     
		m_freem(m0);
		return NULL;
	}
	return m0;
}

  
                                                       
   
int
m_makewritable(struct mbuf **mp, int off, int len, int how)
{
	int error;
#if DEBUG
	struct mbuf *n;
	int origlen, reslen;

	origlen = m_length(*mp);
#endif            

#if 0                                
	if (len == M_COPYALL) {
		len = m_length(*mp) - off;          
	}
#endif

	error = m_copyback0(mp, off, len, NULL,
	    M_COPYBACK0_PRESERVE | M_COPYBACK0_COW, how);

#if DEBUG
	reslen = 0;
	for (n = *mp; n; n = n->m_next) {
		reslen += n->m_len;
	}
	if (origlen != reslen) {
		panic("m_makewritable: length changed");
	}
	if (((*mp)->m_flags & M_PKTHDR) && reslen != (*mp)->m_pkthdr.len) {
		panic("m_makewritable: inconsist");
	}
#endif            

	return error;
}

static int
m_copyback0(struct mbuf **mp0, int off, int len, const void *vp, int flags,
    int how)
{
	int mlen;
	struct mbuf *m, *n;
	struct mbuf **mp;
	int totlen = 0;
	const char *cp = vp;

	VERIFY(mp0 != NULL);
	VERIFY(*mp0 != NULL);
	VERIFY((flags & M_COPYBACK0_PRESERVE) == 0 || cp == NULL);
	VERIFY((flags & M_COPYBACK0_COPYBACK) == 0 || cp != NULL);

	  
                                                                      
                                                                       
    

	VERIFY((~flags & (M_COPYBACK0_EXTEND | M_COPYBACK0_COW)) != 0);

	mp = mp0;
	m = *mp;
	while (off > (mlen = m->m_len)) {
		off -= mlen;
		totlen += mlen;
		if (m->m_next == NULL) {
			int tspace;
extend:
			if (!(flags & M_COPYBACK0_EXTEND)) {
				goto out;
			}

			  
                                               
      

			mlen = m->m_len;
			if (off + len >= MINCLSIZE &&
			    !(m->m_flags & M_EXT) && m->m_len == 0) {
				MCLGET(m, how);
			}
			tspace = M_TRAILINGSPACE(m);
			if (tspace > 0) {
				tspace = MIN(tspace, off + len);
				VERIFY(tspace > 0);
				bzero(mtod(m, char *) + m->m_len,
				    MIN(off, tspace));
				m->m_len += tspace;
				off += mlen;
				totlen -= mlen;
				continue;
			}

			  
                               
      

			if (off + len >= MINCLSIZE) {
				n = m_getcl(how, m->m_type, 0);
			} else {
				n = _M_GET(how, m->m_type);
			}
			if (n == NULL) {
				goto out;
			}
			n->m_len = 0;
			n->m_len = MIN(M_TRAILINGSPACE(n), off + len);
			bzero(mtod(n, char *), MIN(n->m_len, off));
			m->m_next = n;
		}
		mp = &m->m_next;
		m = m->m_next;
	}
	while (len > 0) {
		mlen = m->m_len - off;
		if (mlen != 0 && m_mclhasreference(m)) {
			char *datap;
			int eatlen;

			  
                             
                                                 
      

#if DIAGNOSTIC
			if (!(flags & M_COPYBACK0_COW)) {
				panic("m_copyback0: read-only");
			}
#endif                 

			  
                                                
                             
      
			if (off > 0 && len < mlen) {
				n = m_split0(m, off, how, 0);
				if (n == NULL) {
					goto enobufs;
				}
				m->m_next = n;
				mp = &m->m_next;
				m = n;
				off = 0;
				continue;
			}

			  
                                                 
                                      
      

			  
                                                         
      
			n = _M_GET(how, m->m_type);
			if (n == NULL) {
				goto enobufs;
			}
			if (off == 0 && (m->m_flags & M_PKTHDR)) {
				M_COPY_PKTHDR(n, m);
				n->m_len = MHLEN;
			} else {
				if (len >= MINCLSIZE) {
					MCLGET(n, M_DONTWAIT);
				}
				n->m_len =
				    (n->m_flags & M_EXT) ? MCLBYTES : MLEN;
			}
			if (n->m_len > len) {
				n->m_len = len;
			}

			  
                                                 
                                               
      
			if (flags & M_COPYBACK0_PRESERVE) {
				datap = mtod(n, char *);
			} else {
				datap = NULL;
			}
			eatlen = n->m_len;
			VERIFY(off == 0 || eatlen >= mlen);
			if (off > 0) {
				VERIFY(len >= mlen);
				m->m_len = off;
				m->m_next = n;
				if (datap) {
					m_copydata(m, off, mlen, datap);
					datap += mlen;
				}
				eatlen -= mlen;
				mp = &m->m_next;
				m = m->m_next;
			}
			while (m != NULL && m_mclhasreference(m) &&
			    n->m_type == m->m_type && eatlen > 0) {
				mlen = MIN(eatlen, m->m_len);
				if (datap) {
					m_copydata(m, 0, mlen, datap);
					datap += mlen;
				}
				m->m_data += mlen;
				m->m_len -= mlen;
				eatlen -= mlen;
				if (m->m_len == 0) {
					*mp = m = m_free(m);
				}
			}
			if (eatlen > 0) {
				n->m_len -= eatlen;
			}
			n->m_next = m;
			*mp = m = n;
			continue;
		}
		mlen = MIN(mlen, len);
		if (flags & M_COPYBACK0_COPYBACK) {
			bcopy(cp, mtod(m, caddr_t) + off, (unsigned)mlen);
			cp += mlen;
		}
		len -= mlen;
		mlen += off;
		off = 0;
		totlen += mlen;
		if (len == 0) {
			break;
		}
		if (m->m_next == NULL) {
			goto extend;
		}
		mp = &m->m_next;
		m = m->m_next;
	}
out:
	if (((m = *mp0)->m_flags & M_PKTHDR) && (m->m_pkthdr.len < totlen)) {
		VERIFY(flags & M_COPYBACK0_EXTEND);
		m->m_pkthdr.len = totlen;
	}

	return 0;

enobufs:
	return ENOBUFS;
}

uint64_t
mcl_to_paddr(char *addr)
{
	vm_offset_t base_phys;

	if (!MBUF_IN_MAP(addr)) {
		return 0;
	}
	base_phys = mcl_paddr[atop_64(addr - (char *)mbutl)];

	if (base_phys == 0) {
		return 0;
	}
	return (uint64_t)(ptoa_64(base_phys) | ((uint64_t)addr & PAGE_MASK));
}

  
                                                                             
                                                                        
                                                                          
                                                                       
                                                              
   
int MDFail;

struct mbuf *
m_dup(struct mbuf *m, int how)
{
	struct mbuf *n, **np;
	struct mbuf *top;
	int copyhdr = 0;

	np = &top;
	top = NULL;
	if (m->m_flags & M_PKTHDR) {
		copyhdr = 1;
	}

	  
                                                            
                                               
    
	if (m->m_next == NULL) {
		                                                         
		if (copyhdr) {
			if (m->m_pkthdr.len <= MHLEN && m->m_len <= MHLEN) {
				if ((n = _M_GETHDR(how, m->m_type)) == NULL) {
					return NULL;
				}
				n->m_len = m->m_len;
				m_dup_pkthdr(n, m, how);
				bcopy(m->m_data, n->m_data, m->m_len);
				return n;
			}
		} else if (m->m_len <= MLEN) {
			if ((n = _M_GET(how, m->m_type)) == NULL) {
				return NULL;
			}
			bcopy(m->m_data, n->m_data, m->m_len);
			n->m_len = m->m_len;
			return n;
		}
	}
	while (m != NULL) {
#if BLUE_DEBUG
		printf("<%x: %x, %x, %x\n", m, m->m_flags, m->m_len,
		    m->m_data);
#endif
		if (copyhdr) {
			n = _M_GETHDR(how, m->m_type);
		} else {
			n = _M_GET(how, m->m_type);
		}
		if (n == NULL) {
			goto nospace;
		}
		if (m->m_flags & M_EXT) {
			if (m->m_len <= m_maxsize(MC_CL)) {
				MCLGET(n, how);
			} else if (m->m_len <= m_maxsize(MC_BIGCL)) {
				n = m_mbigget(n, how);
			} else if (m->m_len <= m_maxsize(MC_16KCL) && njcl > 0) {
				n = m_m16kget(n, how);
			}
			if (!(n->m_flags & M_EXT)) {
				(void) m_free(n);
				goto nospace;
			}
		} else {
			VERIFY((copyhdr == 1 && m->m_len <= MHLEN) ||
			    (copyhdr == 0 && m->m_len <= MLEN));
		}
		*np = n;
		if (copyhdr) {
			                                              
			m_dup_pkthdr(n, m, how);
			copyhdr = 0;
			if (!(n->m_flags & M_EXT)) {
				n->m_data = n->m_pktdat;
			}
		}
		n->m_len = m->m_len;
		  
                                                 
                                                                
                            
     
		bcopy(MTOD(m, caddr_t), MTOD(n, caddr_t), (unsigned)n->m_len);
		m = m->m_next;
		np = &n->m_next;
#if BLUE_DEBUG
		printf(">%x: %x, %x, %x\n", n, n->m_flags, n->m_len,
		    n->m_data);
#endif
	}

	if (top == NULL) {
		MDFail++;
	}
	return top;

nospace:
	m_freem(top);
	MDFail++;
	return NULL;
}

#define MBUF_MULTIPAGES(m)                                              \
	(((m)->m_flags & M_EXT) &&                                      \
	((IS_P2ALIGNED((m)->m_data, PAGE_SIZE)                          \
	&& (m)->m_len > PAGE_SIZE) ||                                   \
	(!IS_P2ALIGNED((m)->m_data, PAGE_SIZE) &&                       \
	P2ROUNDUP((m)->m_data, PAGE_SIZE) < ((uintptr_t)(m)->m_data + (m)->m_len))))

static struct mbuf *
m_expand(struct mbuf *m, struct mbuf **last)
{
	struct mbuf *top = NULL;
	struct mbuf **nm = &top;
	uintptr_t data0, data;
	unsigned int len0, len;

	VERIFY(MBUF_MULTIPAGES(m));
	VERIFY(m->m_next == NULL);
	data0 = (uintptr_t)m->m_data;
	len0 = m->m_len;
	*last = top;

	for (;;) {
		struct mbuf *n;

		data = data0;
		if (IS_P2ALIGNED(data, PAGE_SIZE) && len0 > PAGE_SIZE) {
			len = PAGE_SIZE;
		} else if (!IS_P2ALIGNED(data, PAGE_SIZE) &&
		    P2ROUNDUP(data, PAGE_SIZE) < (data + len0)) {
			len = P2ROUNDUP(data, PAGE_SIZE) - data;
		} else {
			len = len0;
		}

		VERIFY(len > 0);
		VERIFY(m->m_flags & M_EXT);
		m->m_data = (void *)data;
		m->m_len = len;

		*nm = *last = m;
		nm = &m->m_next;
		m->m_next = NULL;

		data0 += len;
		len0 -= len;
		if (len0 == 0) {
			break;
		}

		n = _M_RETRY(M_DONTWAIT, MT_DATA);
		if (n == NULL) {
			m_freem(top);
			top = *last = NULL;
			break;
		}

		n->m_ext = m->m_ext;
		m_incref(m);
		n->m_flags |= M_EXT;
		m = n;
	}
	return top;
}

struct mbuf *
m_normalize(struct mbuf *m)
{
	struct mbuf *top = NULL;
	struct mbuf **nm = &top;
	boolean_t expanded = FALSE;

	while (m != NULL) {
		struct mbuf *n;

		n = m->m_next;
		m->m_next = NULL;

		                                                      
		if (MBUF_MULTIPAGES(m)) {
			struct mbuf *last;
			if ((m = m_expand(m, &last)) == NULL) {
				m_freem(n);
				m_freem(top);
				top = NULL;
				break;
			}
			*nm = m;
			nm = &last->m_next;
			expanded = TRUE;
		} else {
			*nm = m;
			nm = &m->m_next;
		}
		m = n;
	}
	if (expanded) {
		atomic_add_32(&mb_normalized, 1);
	}
	return top;
}

  
                                                         
                                                        
                  
  
                                                     
   
int
m_append(struct mbuf *m0, int len, caddr_t cp)
{
	struct mbuf *m, *n;
	int remainder, space;

	for (m = m0; m->m_next != NULL; m = m->m_next) {
		;
	}
	remainder = len;
	space = M_TRAILINGSPACE(m);
	if (space > 0) {
		  
                               
     
		if (space > remainder) {
			space = remainder;
		}
		bcopy(cp, mtod(m, caddr_t) + m->m_len, space);
		m->m_len += space;
		cp += space;
		remainder -= space;
	}
	while (remainder > 0) {
		  
                                           
                                    
     
		n = m_get(M_WAITOK, m->m_type);
		if (n == NULL) {
			break;
		}
		n->m_len = min(MLEN, remainder);
		bcopy(cp, mtod(n, caddr_t), n->m_len);
		cp += n->m_len;
		remainder -= n->m_len;
		m->m_next = n;
		m = n;
	}
	if (m0->m_flags & M_PKTHDR) {
		m0->m_pkthdr.len += len - remainder;
	}
	return remainder == 0;
}

struct mbuf *
m_last(struct mbuf *m)
{
	while (m->m_next != NULL) {
		m = m->m_next;
	}
	return m;
}

unsigned int
m_fixhdr(struct mbuf *m0)
{
	u_int len;

	VERIFY(m0->m_flags & M_PKTHDR);

	len = m_length2(m0, NULL);
	m0->m_pkthdr.len = len;
	return len;
}

unsigned int
m_length2(struct mbuf *m0, struct mbuf **last)
{
	struct mbuf *m;
	u_int len;

	len = 0;
	for (m = m0; m != NULL; m = m->m_next) {
		len += m->m_len;
		if (m->m_next == NULL) {
			break;
		}
	}
	if (last != NULL) {
		*last = m;
	}
	return len;
}

  
                                                                          
                                                                             
                                                                         
                                                                        
  
                                                                       
                        
  
                                                                         
                                                     
  
                                                                       
                                       
   
struct mbuf *
m_defrag_offset(struct mbuf *m0, u_int32_t off, int how)
{
	struct mbuf *m_new = NULL, *m_final = NULL;
	int progress = 0, length, pktlen;

	if (!(m0->m_flags & M_PKTHDR)) {
		return m0;
	}

	VERIFY(off < MHLEN);
	m_fixhdr(m0);                          

	pktlen = m0->m_pkthdr.len + off;
	if (pktlen > MHLEN) {
		m_final = m_getcl(how, MT_DATA, M_PKTHDR);
	} else {
		m_final = m_gethdr(how, MT_DATA);
	}

	if (m_final == NULL) {
		goto nospace;
	}

	if (off > 0) {
		pktlen -= off;
		m_final->m_data += off;
	}

	  
                                                            
                                                             
                                                        
    
	VERIFY(m0->m_pkthdr.pkt_hdr == NULL);

	if (m_dup_pkthdr(m_final, m0, how) == 0) {
		goto nospace;
	}

	m_new = m_final;

	while (progress < pktlen) {
		length = pktlen - progress;
		if (length > MCLBYTES) {
			length = MCLBYTES;
		}
		length -= ((m_new == m_final) ? off : 0);
		if (length < 0) {
			goto nospace;
		}

		if (m_new == NULL) {
			if (length > MLEN) {
				m_new = m_getcl(how, MT_DATA, 0);
			} else {
				m_new = m_get(how, MT_DATA);
			}
			if (m_new == NULL) {
				goto nospace;
			}
		}

		m_copydata(m0, progress, length, mtod(m_new, caddr_t));
		progress += length;
		m_new->m_len = length;
		if (m_new != m_final) {
			m_cat(m_final, m_new);
		}
		m_new = NULL;
	}
	m_freem(m0);
	m0 = m_final;
	return m0;
nospace:
	if (m_final) {
		m_freem(m_final);
	}
	return NULL;
}

struct mbuf *
m_defrag(struct mbuf *m0, int how)
{
	return m_defrag_offset(m0, 0, how);
}

void
m_mchtype(struct mbuf *m, int t)
{
	mtype_stat_inc(t);
	mtype_stat_dec(m->m_type);
	(m)->m_type = t;
}

void *
m_mtod(struct mbuf *m)
{
	return MTOD(m, void *);
}

struct mbuf *
m_dtom(void *x)
{
	return (struct mbuf *)((uintptr_t)(x) & ~(MSIZE - 1));
}

void
m_mcheck(struct mbuf *m)
{
	_MCHECK(m);
}

  
                                                             
   
struct mbuf *
m_getptr(struct mbuf *m, int loc, int *off)
{
	while (loc >= 0) {
		                           
		if (m->m_len > loc) {
			*off = loc;
			return m;
		} else {
			loc -= m->m_len;
			if (m->m_next == NULL) {
				if (loc == 0) {
					                                     
					*off = m->m_len;
					return m;
				}
				return NULL;
			}
			m = m->m_next;
		}
	}
	return NULL;
}

  
                                                                  
   
static void
mbuf_waiter_inc(mbuf_class_t class, boolean_t comp)
{
	mcache_waiter_inc(m_cache(class));
	if (comp) {
		if (class == MC_CL) {
			mcache_waiter_inc(m_cache(MC_MBUF_CL));
		} else if (class == MC_BIGCL) {
			mcache_waiter_inc(m_cache(MC_MBUF_BIGCL));
		} else if (class == MC_16KCL) {
			mcache_waiter_inc(m_cache(MC_MBUF_16KCL));
		} else {
			mcache_waiter_inc(m_cache(MC_MBUF_CL));
			mcache_waiter_inc(m_cache(MC_MBUF_BIGCL));
		}
	}
}

  
                                                                        
   
static void
mbuf_waiter_dec(mbuf_class_t class, boolean_t comp)
{
	mcache_waiter_dec(m_cache(class));
	if (comp) {
		if (class == MC_CL) {
			mcache_waiter_dec(m_cache(MC_MBUF_CL));
		} else if (class == MC_BIGCL) {
			mcache_waiter_dec(m_cache(MC_MBUF_BIGCL));
		} else if (class == MC_16KCL) {
			mcache_waiter_dec(m_cache(MC_MBUF_16KCL));
		} else {
			mcache_waiter_dec(m_cache(MC_MBUF_CL));
			mcache_waiter_dec(m_cache(MC_MBUF_BIGCL));
		}
	}
}

static bool mbuf_watchdog_defunct_active = false;

static uint32_t
mbuf_watchdog_socket_space(struct socket *so)
{
	if (so == NULL) {
		return 0;
	}

	return so->so_snd.sb_mbcnt + so->so_rcv.sb_mbcnt;
}

struct mbuf_watchdog_defunct_args {
	struct proc *top_app;
	uint32_t top_app_space_used;
};

static int
mbuf_watchdog_defunct_iterate(proc_t p, void *arg)
{
	struct fileproc *fp = NULL;
	struct mbuf_watchdog_defunct_args *args =
	    (struct mbuf_watchdog_defunct_args *)arg;
	uint32_t space_used = 0;

	proc_fdlock(p);
	fdt_foreach(fp, p) {
		struct fileglob *fg = fp->fp_glob;
		struct socket *so = NULL;

		if (FILEGLOB_DTYPE(fg) != DTYPE_SOCKET) {
			continue;
		}
		so = (struct socket *)fp->fp_glob->fg_data;
		  
                                              
                                             
                                              
                                
    
                                                
                                     
     
		space_used += mbuf_watchdog_socket_space(so);
	}
	proc_fdunlock(p);
	if (space_used > args->top_app_space_used) {
		if (args->top_app != NULL) {
			proc_rele(args->top_app);
		}
		args->top_app = p;
		args->top_app_space_used = space_used;

		return PROC_CLAIMED;
	} else {
		return PROC_RETURNED;
	}
}

extern char *proc_name_address(void *p);

static void
mbuf_watchdog_defunct(thread_call_param_t arg0, thread_call_param_t arg1)
{
#pragma unused(arg0, arg1)
	struct mbuf_watchdog_defunct_args args = {};
	struct fileproc *fp = NULL;

	proc_iterate(PROC_ALLPROCLIST,
	    mbuf_watchdog_defunct_iterate, &args, NULL, NULL);

	  
                                      
    
	if (args.top_app != NULL) {
		os_log(OS_LOG_DEFAULT, "%s: defuncting all sockets from %s.%d",
		    __func__,
		    proc_name_address(args.top_app),
		    proc_pid(args.top_app));
		proc_fdlock(args.top_app);
		fdt_foreach(fp, args.top_app) {
			struct fileglob *fg = fp->fp_glob;
			struct socket *so = NULL;

			if (FILEGLOB_DTYPE(fg) != DTYPE_SOCKET) {
				continue;
			}
			so = (struct socket *)fp->fp_glob->fg_data;
			socket_lock(so, 0);
			if (sosetdefunct(args.top_app, so,
			    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_ALL,
			    TRUE) == 0) {
				sodefunct(args.top_app, so,
				    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_ALL);
			}
			socket_unlock(so, 0);
		}
		proc_fdunlock(args.top_app);
		proc_rele(args.top_app);
		mbstat.m_forcedefunct++;
	}
	mbuf_watchdog_defunct_active = false;
}

  
                                                                       
                                                                         
                                                          
   
static void
mbuf_watchdog(void)
{
	struct timeval now;
	unsigned int since;
	static thread_call_t defunct_tcall = NULL;

	if (mb_waiters == 0 || !mb_watchdog) {
		return;
	}

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	microuptime(&now);
	since = now.tv_sec - mb_wdtstart.tv_sec;

	  
                                                 
                                                 
                                             
   
                                                 
                                                       
    
	if (since >= MB_WDT_MAXTIME / 2 && !mbuf_watchdog_defunct_active) {
		  
                                      
                                               
             
     
		if (defunct_tcall == NULL) {
			defunct_tcall =
			    thread_call_allocate_with_options(mbuf_watchdog_defunct,
			    NULL,
			    THREAD_CALL_PRIORITY_KERNEL,
			    THREAD_CALL_OPTIONS_ONCE);
		}
		if (defunct_tcall != NULL) {
			mbuf_watchdog_defunct_active = true;
			thread_call_enter(defunct_tcall);
		}
	}
	if (since >= MB_WDT_MAXTIME) {
		panic_plain("%s: %d waiters stuck for %u secs\n%s", __func__,
		    mb_waiters, since, mbuf_dump());
		                
	}
}

  
                                                                          
                                                                          
                         
   
static boolean_t
mbuf_sleep(mbuf_class_t class, unsigned int num, int wait)
{
	boolean_t mcache_retry = FALSE;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	                                                  
	if (mbuf_cached_above(class, wait)) {
		mcache_retry = TRUE;
		goto done;
	}

	                                                      
	m_reclaim(class, num, (wait & MCR_COMP));

	                                      
	if (m_infree(class) > 0) {
		mbstat.m_wait++;
		goto done;
	} else if (mbuf_cached_above(class, wait)) {
		mbstat.m_wait++;
		mcache_retry = TRUE;
		goto done;
	} else if (wait & MCR_TRYHARD) {
		mcache_retry = TRUE;
		goto done;
	}

	  
                                                       
                                                          
    
	mbuf_waiter_inc(class, (wait & MCR_COMP));

	VERIFY(!(wait & MCR_NOSLEEP));

	  
                                                                   
                                                                 
    
	if (mb_waiters == 0) {
		microuptime(&mb_wdtstart);
	} else {
		mbuf_watchdog();
	}

	mb_waiters++;
	m_region_expand(class) += m_total(class) + num;
	                               
	if (mbuf_worker_ready &&
	    mbuf_worker_needs_wakeup) {
		wakeup((caddr_t)&mbuf_worker_needs_wakeup);
		mbuf_worker_needs_wakeup = FALSE;
	}
	mbwdog_logger("waiting (%d mbufs in class %s)", num, m_cname(class));
	(void) msleep(mb_waitchan, mbuf_mlock, (PZERO - 1), m_cname(class), NULL);
	mbwdog_logger("woke up (%d mbufs in class %s) ", num, m_cname(class));

	                                                           
	mbuf_waiter_dec(class, (wait & MCR_COMP));

	                                 
	if (m_infree(class) > 0) {
		mbstat.m_wait++;
		goto done;
	} else if (mbuf_cached_above(class, wait)) {
		mbstat.m_wait++;
		mcache_retry = TRUE;
	}
done:
	return mcache_retry;
}

__attribute__((noreturn))
static void
mbuf_worker_thread(void)
{
	int mbuf_expand;

	while (1) {
		lck_mtx_lock(mbuf_mlock);
		mbwdog_logger("worker thread running");
		mbuf_worker_run_cnt++;
		mbuf_expand = 0;
		  
                                                               
                                                                   
     
#if PAGE_SIZE == 4096
		uint32_t m_mbclusters = m_total(MC_MBUF) >> NMBPCLSHIFT;
		uint32_t m_clusters = m_total(MC_CL);
		uint32_t m_bigclusters = m_total(MC_BIGCL) << NCLPBGSHIFT;
		uint32_t sumclusters = m_mbclusters + m_clusters + m_bigclusters;
		if (sumclusters >= nclusters) {
			mbwdog_logger("reclaiming bigcl");
			mbuf_drain_locked(TRUE);
			m_reclaim(MC_BIGCL, 4, FALSE);
		}
#else
		uint32_t m_16kclusters = m_total(MC_16KCL);
		if (njcl > 0 && (m_16kclusters << NCLPJCLSHIFT) >= njcl) {
			mbwdog_logger("reclaiming 16kcl");
			mbuf_drain_locked(TRUE);
			m_reclaim(MC_16KCL, 4, FALSE);
		}
#endif
		if (m_region_expand(MC_CL) > 0) {
			int n;
			mb_expand_cl_cnt++;
			                                                
			n = m_region_expand(MC_CL) -
			    (m_total(MC_CL) - m_infree(MC_CL));
			if ((n + m_total(MC_CL)) > m_maxlimit(MC_CL)) {
				n = m_maxlimit(MC_CL) - m_total(MC_CL);
			}
			if (n > 0) {
				mb_expand_cl_total += n;
			}
			m_region_expand(MC_CL) = 0;

			if (n > 0) {
				mbwdog_logger("expanding MC_CL by %d", n);
				freelist_populate(MC_CL, n, M_WAIT);
			}
		}
		if (m_region_expand(MC_BIGCL) > 0) {
			int n;
			mb_expand_bigcl_cnt++;
			                                                     
			n = m_region_expand(MC_BIGCL) -
			    (m_total(MC_BIGCL) - m_infree(MC_BIGCL));
			if ((n + m_total(MC_BIGCL)) > m_maxlimit(MC_BIGCL)) {
				n = m_maxlimit(MC_BIGCL) - m_total(MC_BIGCL);
			}
			if (n > 0) {
				mb_expand_bigcl_total += n;
			}
			m_region_expand(MC_BIGCL) = 0;

			if (n > 0) {
				mbwdog_logger("expanding MC_BIGCL by %d", n);
				freelist_populate(MC_BIGCL, n, M_WAIT);
			}
		}
		if (m_region_expand(MC_16KCL) > 0) {
			int n;
			mb_expand_16kcl_cnt++;
			                                                      
			n = m_region_expand(MC_16KCL) -
			    (m_total(MC_16KCL) - m_infree(MC_16KCL));
			if ((n + m_total(MC_16KCL)) > m_maxlimit(MC_16KCL)) {
				n = m_maxlimit(MC_16KCL) - m_total(MC_16KCL);
			}
			if (n > 0) {
				mb_expand_16kcl_total += n;
			}
			m_region_expand(MC_16KCL) = 0;

			if (n > 0) {
				mbwdog_logger("expanding MC_16KCL by %d", n);
				(void) freelist_populate(MC_16KCL, n, M_WAIT);
			}
		}

		  
                                                             
                                                            
                                                               
                        
     
		mbwdog_logger("totals: MC_MBUF %d MC_BIGCL %d MC_CL %d MC_16KCL %d",
		    m_total(MC_MBUF), m_total(MC_BIGCL), m_total(MC_CL),
		    m_total(MC_16KCL));
		uint32_t total_mbufs = m_total(MC_MBUF);
		uint32_t total_clusters = m_total(MC_BIGCL) + m_total(MC_CL) +
		    m_total(MC_16KCL);
		if (total_mbufs < total_clusters) {
			mbwdog_logger("expanding MC_MBUF by %d",
			    total_clusters - total_mbufs);
		}
		while (total_mbufs < total_clusters) {
			mb_expand_cnt++;
			if (freelist_populate(MC_MBUF, 1, M_WAIT) == 0) {
				break;
			}
			total_mbufs = m_total(MC_MBUF);
			total_clusters = m_total(MC_BIGCL) + m_total(MC_CL) +
			    m_total(MC_16KCL);
		}

		mbuf_worker_needs_wakeup = TRUE;
		  
                                                            
                                                            
                                 
     
		net_update_uptime();
		mbuf_worker_last_runtime = net_uptime();
		assert_wait((caddr_t)&mbuf_worker_needs_wakeup,
		    THREAD_UNINT);
		mbwdog_logger("worker thread sleeping");
		lck_mtx_unlock(mbuf_mlock);
		(void) thread_block((thread_continue_t)mbuf_worker_thread);
	}
}

__attribute__((noreturn))
static void
mbuf_worker_thread_init(void)
{
	mbuf_worker_ready++;
	mbuf_worker_thread();
}

static mcl_slab_t *
slab_get(void *buf)
{
	mcl_slabg_t *slg;
	unsigned int ix, k;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);

	VERIFY(MBUF_IN_MAP(buf));
	ix = ((unsigned char *)buf - mbutl) >> MBSHIFT;
	VERIFY(ix < maxslabgrp);

	if ((slg = slabstbl[ix]) == NULL) {
		  
                                                             
                                                            
                                                            
                                                    
     
		++slabgrp;
		VERIFY(ix < slabgrp);
		  
                                                           
                                                             
                                                             
     
		VERIFY(mb_clalloc_busy);
		lck_mtx_unlock(mbuf_mlock);

		                                                         
		slg = zalloc_permanent_type(mcl_slabg_t);
		slg->slg_slab = zalloc_permanent(sizeof(mcl_slab_t) * NSLABSPMB,
		    ZALIGN(mcl_slab_t));

		lck_mtx_lock(mbuf_mlock);
		  
                                                           
                                                         
     
		VERIFY(mb_clalloc_busy);

		slabstbl[ix] = slg;

		                                                          
		for (k = 1; k < NSLABSPMB; k++) {
			slg->slg_slab[k - 1].sl_next = &slg->slg_slab[k];
		}
		VERIFY(slg->slg_slab[NSLABSPMB - 1].sl_next == NULL);

		                                                           
		if (ix > 0) {
			VERIFY(slabstbl[ix - 1]->
			    slg_slab[NSLABSPMB - 1].sl_next == NULL);
			slabstbl[ix - 1]->slg_slab[NSLABSPMB - 1].sl_next =
			    &slg->slg_slab[0];
		}
	}

	ix = MTOPG(buf) % NSLABSPMB;
	VERIFY(ix < NSLABSPMB);

	return &slg->slg_slab[ix];
}

static void
slab_init(mcl_slab_t *sp, mbuf_class_t class, u_int32_t flags,
    void *base, void *head, unsigned int len, int refcnt, int chunks)
{
	sp->sl_class = class;
	sp->sl_flags = flags;
	sp->sl_base = base;
	sp->sl_head = head;
	sp->sl_len = len;
	sp->sl_refcnt = refcnt;
	sp->sl_chunks = chunks;
	slab_detach(sp);
}

static void
slab_insert(mcl_slab_t *sp, mbuf_class_t class)
{
	VERIFY(slab_is_detached(sp));
	m_slab_cnt(class)++;
	TAILQ_INSERT_TAIL(&m_slablist(class), sp, sl_link);
	sp->sl_flags &= ~SLF_DETACHED;

	  
                                                                 
                
    
	if (class == MC_16KCL) {
		int k;
		for (k = 1; k < NSLABSP16KB; k++) {
			sp = sp->sl_next;
			                                       
			VERIFY(sp != NULL && slab_is_detached(sp));
			sp->sl_flags &= ~SLF_DETACHED;
		}
	}
}

static void
slab_remove(mcl_slab_t *sp, mbuf_class_t class)
{
	int k;
	VERIFY(!slab_is_detached(sp));
	VERIFY(m_slab_cnt(class) > 0);
	m_slab_cnt(class)--;
	TAILQ_REMOVE(&m_slablist(class), sp, sl_link);
	slab_detach(sp);
	if (class == MC_16KCL) {
		for (k = 1; k < NSLABSP16KB; k++) {
			sp = sp->sl_next;
			                                       
			VERIFY(sp != NULL);
			VERIFY(!slab_is_detached(sp));
			slab_detach(sp);
		}
	}
}

static boolean_t
slab_inrange(mcl_slab_t *sp, void *buf)
{
	return (uintptr_t)buf >= (uintptr_t)sp->sl_base &&
	       (uintptr_t)buf < ((uintptr_t)sp->sl_base + sp->sl_len);
}

#undef panic

static void
slab_nextptr_panic(mcl_slab_t *sp, void *addr)
{
	int i;
	unsigned int chunk_len = sp->sl_len / sp->sl_chunks;
	uintptr_t buf = (uintptr_t)sp->sl_base;

	for (i = 0; i < sp->sl_chunks; i++, buf += chunk_len) {
		void *next = ((mcache_obj_t *)buf)->obj_next;
		if (next != addr) {
			continue;
		}
		if (!mclverify) {
			if (next != NULL && !MBUF_IN_MAP(next)) {
				mcache_t *cp = m_cache(sp->sl_class);
				panic("%s: %s buffer %p in slab %p modified "
				    "after free at offset 0: %p out of range "
				    "[%p-%p)\n", __func__, cp->mc_name,
				    (void *)buf, sp, next, mbutl, embutl);
				                
			}
		} else {
			mcache_audit_t *mca = mcl_audit_buf2mca(sp->sl_class,
			    (mcache_obj_t *)buf);
			mcl_audit_verify_nextptr(next, mca);
		}
	}
}

static void
slab_detach(mcl_slab_t *sp)
{
	sp->sl_link.tqe_next = (mcl_slab_t *)-1;
	sp->sl_link.tqe_prev = (mcl_slab_t **)-1;
	sp->sl_flags |= SLF_DETACHED;
}

static boolean_t
slab_is_detached(mcl_slab_t *sp)
{
	return (intptr_t)sp->sl_link.tqe_next == -1 &&
	       (intptr_t)sp->sl_link.tqe_prev == -1 &&
	       (sp->sl_flags & SLF_DETACHED);
}

static void
mcl_audit_init(void *buf, mcache_audit_t **mca_list,
    mcache_obj_t **con_list, size_t con_size, unsigned int num)
{
	mcache_audit_t *mca, *mca_tail;
	mcache_obj_t *con = NULL;
	boolean_t save_contents = (con_list != NULL);
	unsigned int i, ix;

	ASSERT(num <= NMBPG);
	ASSERT(con_list == NULL || con_size != 0);

	ix = MTOPG(buf);
	VERIFY(ix < maxclaudit);

	                                           
	for (i = 0; i < num; i++) {
		VERIFY(mclaudit[ix].cl_audit[i] == NULL);
	}

	mca = mca_tail = *mca_list;
	if (save_contents) {
		con = *con_list;
	}

	for (i = 0; i < num; i++) {
		mcache_audit_t *next;

		next = mca->mca_next;
		bzero(mca, sizeof(*mca));
		mca->mca_next = next;
		mclaudit[ix].cl_audit[i] = mca;

		                                             
		if (save_contents) {
			mcl_saved_contents_t *msc =
			    (mcl_saved_contents_t *)(void *)con;

			VERIFY(msc != NULL);
			VERIFY(IS_P2ALIGNED(msc, sizeof(u_int64_t)));
			VERIFY(con_size == sizeof(*msc));
			mca->mca_contents_size = con_size;
			mca->mca_contents = msc;
			con = con->obj_next;
			bzero(mca->mca_contents, mca->mca_contents_size);
		}

		mca_tail = mca;
		mca = mca->mca_next;
	}

	if (save_contents) {
		*con_list = con;
	}

	*mca_list = mca_tail->mca_next;
	mca_tail->mca_next = NULL;
}

static void
mcl_audit_free(void *buf, unsigned int num)
{
	unsigned int i, ix;
	mcache_audit_t *mca, *mca_list;

	ix = MTOPG(buf);
	VERIFY(ix < maxclaudit);

	if (mclaudit[ix].cl_audit[0] != NULL) {
		mca_list = mclaudit[ix].cl_audit[0];
		for (i = 0; i < num; i++) {
			mca = mclaudit[ix].cl_audit[i];
			mclaudit[ix].cl_audit[i] = NULL;
			if (mca->mca_contents) {
				mcache_free(mcl_audit_con_cache,
				    mca->mca_contents);
			}
		}
		mcache_free_ext(mcache_audit_cache,
		    (mcache_obj_t *)mca_list);
	}
}

  
                                                           
                                                     
   
static mcache_audit_t *
mcl_audit_buf2mca(mbuf_class_t class, mcache_obj_t *mobj)
{
	mcache_audit_t *mca = NULL;
	int ix = MTOPG(mobj), m_idx = 0;
	unsigned char *page_addr;

	VERIFY(ix < maxclaudit);
	VERIFY(IS_P2ALIGNED(mobj, MIN(m_maxsize(class), PAGE_SIZE)));

	page_addr = PGTOM(ix);

	switch (class) {
	case MC_MBUF:
		  
                                                  
                                                      
                                                 
                                                 
                                      
     
		m_idx = MBPAGEIDX(page_addr, mobj);
		VERIFY(m_idx < (int)NMBPG);
		mca = mclaudit[ix].cl_audit[m_idx];
		break;

	case MC_CL:
		  
                                                         
     
		m_idx = CLPAGEIDX(page_addr, mobj);
		VERIFY(m_idx < (int)NCLPG);
		mca = mclaudit[ix].cl_audit[m_idx];
		break;

	case MC_BIGCL:
		m_idx = BCLPAGEIDX(page_addr, mobj);
		VERIFY(m_idx < (int)NBCLPG);
		mca = mclaudit[ix].cl_audit[m_idx];
		break;
	case MC_16KCL:
		  
                                                      
     
		mca = mclaudit[ix].cl_audit[0];
		break;

	default:
		VERIFY(0);
		                
	}

	return mca;
}

static void
mcl_audit_mbuf(mcache_audit_t *mca, void *addr, boolean_t composite,
    boolean_t alloc)
{
	struct mbuf *m = addr;
	mcache_obj_t *next = ((mcache_obj_t *)m)->obj_next;

	VERIFY(mca->mca_contents != NULL &&
	    mca->mca_contents_size == AUDIT_CONTENTS_SIZE);

	if (mclverify) {
		mcl_audit_verify_nextptr(next, mca);
	}

	if (!alloc) {
		                                  
		mcl_audit_save_mbuf(m, mca);
		if (mclverify) {
			mcache_set_pattern(MCACHE_FREE_PATTERN, m,
			    m_maxsize(MC_MBUF));
		}
		((mcache_obj_t *)m)->obj_next = next;
		return;
	}

	                                                              
	if (mclverify) {
		mcache_audit_free_verify_set(mca, addr, 0, m_maxsize(MC_MBUF));
	}
	                                     
	mcl_audit_restore_mbuf(m, mca, composite);
}

static void
mcl_audit_restore_mbuf(struct mbuf *m, mcache_audit_t *mca, boolean_t composite)
{
	struct mbuf *ms = MCA_SAVED_MBUF_PTR(mca);

	if (composite) {
		struct mbuf *next = m->m_next;
		VERIFY(ms->m_flags == M_EXT && m_get_rfa(ms) != NULL &&
		    MBUF_IS_COMPOSITE(ms));
		VERIFY(mca->mca_contents_size == AUDIT_CONTENTS_SIZE);
		  
                                                          
                                                      
                                                           
                                                         
     
		bcopy(ms, m, MCA_SAVED_MBUF_SIZE);
		m->m_next = next;
	} else {
		  
                                                             
                                                            
                   
     
		m->m_type = ms->m_type;
	}
	_MCHECK(m);
}

static void
mcl_audit_save_mbuf(struct mbuf *m, mcache_audit_t *mca)
{
	VERIFY(mca->mca_contents_size == AUDIT_CONTENTS_SIZE);
	_MCHECK(m);
	bcopy(m, MCA_SAVED_MBUF_PTR(mca), MCA_SAVED_MBUF_SIZE);
}

static void
mcl_audit_cluster(mcache_audit_t *mca, void *addr, size_t size, boolean_t alloc,
    boolean_t save_next)
{
	mcache_obj_t *next = ((mcache_obj_t *)addr)->obj_next;

	if (!alloc) {
		if (mclverify) {
			mcache_set_pattern(MCACHE_FREE_PATTERN, addr, size);
		}
		if (save_next) {
			mcl_audit_verify_nextptr(next, mca);
			((mcache_obj_t *)addr)->obj_next = next;
		}
	} else if (mclverify) {
		                                                              
		mcl_audit_verify_nextptr(next, mca);
		mcache_audit_free_verify_set(mca, addr, 0, size);
	}
}

static void
mcl_audit_scratch(mcache_audit_t *mca)
{
	void *stack[MCACHE_STACK_DEPTH + 1];
	mcl_scratch_audit_t *msa;
	struct timeval now;

	VERIFY(mca->mca_contents != NULL);
	msa = MCA_SAVED_SCRATCH_PTR(mca);

	msa->msa_pthread = msa->msa_thread;
	msa->msa_thread = current_thread();
	bcopy(msa->msa_stack, msa->msa_pstack, sizeof(msa->msa_pstack));
	msa->msa_pdepth = msa->msa_depth;
	bzero(stack, sizeof(stack));
	msa->msa_depth = OSBacktrace(stack, MCACHE_STACK_DEPTH + 1) - 1;
	bcopy(&stack[1], msa->msa_stack, sizeof(msa->msa_stack));

	msa->msa_ptstamp = msa->msa_tstamp;
	microuptime(&now);
	                                         
	msa->msa_tstamp = ((now.tv_usec - mb_start.tv_usec) / 1000);
	if ((now.tv_sec - mb_start.tv_sec) > 0) {
		msa->msa_tstamp += ((now.tv_sec - mb_start.tv_sec) * 1000);
	}
}

__abortlike
static void
mcl_audit_mcheck_panic(struct mbuf *m)
{
	char buf[DUMP_MCA_BUF_SIZE];
	mcache_audit_t *mca;

	MRANGE(m);
	mca = mcl_audit_buf2mca(MC_MBUF, (mcache_obj_t *)m);

	panic("mcl_audit: freed mbuf %p with type 0x%x (instead of 0x%x)\n%s\n",
	    m, (u_int16_t)m->m_type, MT_FREE, mcache_dump_mca(buf, mca));
	                
}

__abortlike
static void
mcl_audit_verify_nextptr_panic(void *next, mcache_audit_t *mca)
{
	char buf[DUMP_MCA_BUF_SIZE];
	panic("mcl_audit: buffer %p modified after free at offset 0: "
	    "%p out of range [%p-%p)\n%s\n",
	    mca->mca_addr, next, mbutl, embutl, mcache_dump_mca(buf, mca));
	                
}

static void
mcl_audit_verify_nextptr(void *next, mcache_audit_t *mca)
{
	if (next != NULL && !MBUF_IN_MAP(next) &&
	    (next != (void *)MCACHE_FREE_PATTERN || !mclverify)) {
		mcl_audit_verify_nextptr_panic(next, mca);
	}
}

                                                
static void
mleak_activate(void)
{
	mleak_table.mleak_sample_factor = MLEAK_SAMPLE_FACTOR;
	PE_parse_boot_argn("mleak_sample_factor",
	    &mleak_table.mleak_sample_factor,
	    sizeof(mleak_table.mleak_sample_factor));

	if (mleak_table.mleak_sample_factor == 0) {
		mclfindleak = 0;
	}

	if (mclfindleak == 0) {
		return;
	}

	vm_size_t alloc_size =
	    mleak_alloc_buckets * sizeof(struct mallocation);
	vm_size_t trace_size = mleak_trace_buckets * sizeof(struct mtrace);

	mleak_allocations = zalloc_permanent(alloc_size, ZALIGN(struct mallocation));
	mleak_traces = zalloc_permanent(trace_size, ZALIGN(struct mtrace));
	mleak_stat = zalloc_permanent(MLEAK_STAT_SIZE(MLEAK_NUM_TRACES),
	    ZALIGN(mleak_stat_t));

	mleak_stat->ml_cnt = MLEAK_NUM_TRACES;
#ifdef __LP64__
	mleak_stat->ml_isaddr64 = 1;
#endif               
}

static void
mleak_logger(u_int32_t num, mcache_obj_t *addr, boolean_t alloc)
{
	int temp;

	if (mclfindleak == 0) {
		return;
	}

	if (!alloc) {
		return mleak_free(addr);
	}

	temp = atomic_add_32_ov(&mleak_table.mleak_capture, 1);

	if ((temp % mleak_table.mleak_sample_factor) == 0 && addr != NULL) {
		uintptr_t bt[MLEAK_STACK_DEPTH];
		int logged = backtrace(bt, MLEAK_STACK_DEPTH, NULL);
		mleak_log(bt, addr, logged, num);
	}
}

  
                                                                      
                                                                             
                                                                          
                                         
   
static boolean_t
mleak_log(uintptr_t *bt, mcache_obj_t *addr, uint32_t depth, int num)
{
	struct mallocation *allocation;
	struct mtrace *trace;
	uint32_t trace_index;

	                                               
	if (!lck_mtx_try_lock_spin(mleak_lock)) {
		mleak_table.total_conflicts++;
		return FALSE;
	}

	allocation = &mleak_allocations[hashaddr((uintptr_t)addr,
	    mleak_alloc_buckets)];
	trace_index = hashbacktrace(bt, depth, mleak_trace_buckets);
	trace = &mleak_traces[trace_index];

	VERIFY(allocation <= &mleak_allocations[mleak_alloc_buckets - 1]);
	VERIFY(trace <= &mleak_traces[mleak_trace_buckets - 1]);

	allocation->hitcount++;
	trace->hitcount++;

	  
                                                
                                                   
    
	if (allocation->element != NULL &&
	    trace_index == allocation->trace_index) {
		mleak_table.alloc_collisions++;
		lck_mtx_unlock(mleak_lock);
		return TRUE;
	}

	  
                                            
                                        
    
	if (trace->allocs > 0 &&
	    bcmp(trace->addr, bt, (depth * sizeof(uintptr_t))) != 0) {
		                                                           
		trace->collisions++;
		mleak_table.trace_collisions++;
		lck_mtx_unlock(mleak_lock);
		return TRUE;
	} else if (trace->allocs > 0) {
		                                                      
		trace->allocs++;
	} else {
		                                                            
		if (trace->depth != 0) {
			                                                        
			mleak_table.trace_overwrites++;
		}
		mleak_table.trace_recorded++;
		trace->allocs = 1;
		memcpy(trace->addr, bt, (depth * sizeof(uintptr_t)));
		trace->depth = depth;
		trace->collisions = 0;
	}

	                                                                  
	if (allocation->element != NULL) {
		  
                                                         
                                                       
                     
     
		mleak_table.alloc_collisions++;
	} else if (allocation->trace_index != 0) {
		mleak_table.alloc_overwrites++;
	}
	allocation->element = addr;
	allocation->trace_index = trace_index;
	allocation->count = num;
	mleak_table.alloc_recorded++;
	mleak_table.outstanding_allocs++;

	lck_mtx_unlock(mleak_lock);
	return TRUE;
}

static void
mleak_free(mcache_obj_t *addr)
{
	while (addr != NULL) {
		struct mallocation *allocation = &mleak_allocations
		    [hashaddr((uintptr_t)addr, mleak_alloc_buckets)];

		if (allocation->element == addr &&
		    allocation->trace_index < mleak_trace_buckets) {
			lck_mtx_lock_spin(mleak_lock);
			if (allocation->element == addr &&
			    allocation->trace_index < mleak_trace_buckets) {
				struct mtrace *trace;
				trace = &mleak_traces[allocation->trace_index];
				                                             
				if (trace->allocs > 0) {
					trace->allocs--;
				}
				if (trace->allocs == 0) {
					trace->depth = 0;
				}
				                                               
				allocation->element = NULL;
				mleak_table.outstanding_allocs--;
			}
			lck_mtx_unlock(mleak_lock);
		}
		addr = addr->obj_next;
	}
}

static void
mleak_sort_traces()
{
	int i, j, k;
	struct mtrace *swap;

	for (i = 0; i < MLEAK_NUM_TRACES; i++) {
		mleak_top_trace[i] = NULL;
	}

	for (i = 0, j = 0; j < MLEAK_NUM_TRACES && i < mleak_trace_buckets; i++) {
		if (mleak_traces[i].allocs <= 0) {
			continue;
		}

		mleak_top_trace[j] = &mleak_traces[i];
		for (k = j; k > 0; k--) {
			if (mleak_top_trace[k]->allocs <=
			    mleak_top_trace[k - 1]->allocs) {
				break;
			}

			swap = mleak_top_trace[k - 1];
			mleak_top_trace[k - 1] = mleak_top_trace[k];
			mleak_top_trace[k] = swap;
		}
		j++;
	}

	j--;
	for (; i < mleak_trace_buckets; i++) {
		if (mleak_traces[i].allocs <= mleak_top_trace[j]->allocs) {
			continue;
		}

		mleak_top_trace[j] = &mleak_traces[i];

		for (k = j; k > 0; k--) {
			if (mleak_top_trace[k]->allocs <=
			    mleak_top_trace[k - 1]->allocs) {
				break;
			}

			swap = mleak_top_trace[k - 1];
			mleak_top_trace[k - 1] = mleak_top_trace[k];
			mleak_top_trace[k] = swap;
		}
	}
}

static void
mleak_update_stats()
{
	mleak_trace_stat_t *mltr;
	int i;

	VERIFY(mleak_stat != NULL);
#ifdef __LP64__
	VERIFY(mleak_stat->ml_isaddr64);
#else
	VERIFY(!mleak_stat->ml_isaddr64);
#endif                
	VERIFY(mleak_stat->ml_cnt == MLEAK_NUM_TRACES);

	mleak_sort_traces();

	mltr = &mleak_stat->ml_trace[0];
	bzero(mltr, sizeof(*mltr) * MLEAK_NUM_TRACES);
	for (i = 0; i < MLEAK_NUM_TRACES; i++) {
		int j;

		if (mleak_top_trace[i] == NULL ||
		    mleak_top_trace[i]->allocs == 0) {
			continue;
		}

		mltr->mltr_collisions   = mleak_top_trace[i]->collisions;
		mltr->mltr_hitcount     = mleak_top_trace[i]->hitcount;
		mltr->mltr_allocs       = mleak_top_trace[i]->allocs;
		mltr->mltr_depth        = mleak_top_trace[i]->depth;

		VERIFY(mltr->mltr_depth <= MLEAK_STACK_DEPTH);
		for (j = 0; j < mltr->mltr_depth; j++) {
			mltr->mltr_addr[j] = mleak_top_trace[i]->addr[j];
		}

		mltr++;
	}
}

static struct mbtypes {
	int             mt_type;
	const char      *mt_name;
} mbtypes[] = {
	{ MT_DATA, "data" },
	{ MT_OOBDATA, "oob data" },
	{ MT_CONTROL, "ancillary data" },
	{ MT_HEADER, "packet headers" },
	{ MT_SOCKET, "socket structures" },
	{ MT_PCB, "protocol control blocks" },
	{ MT_RTABLE, "routing table entries" },
	{ MT_HTABLE, "IMP host table entries" },
	{ MT_ATABLE, "address resolution tables" },
	{ MT_FTABLE, "fragment reassembly queue headers" },
	{ MT_SONAME, "socket names and addresses" },
	{ MT_SOOPTS, "socket options" },
	{ MT_RIGHTS, "access rights" },
	{ MT_IFADDR, "interface addresses" },
	{ MT_TAG, "packet tags" },
	{ 0, NULL }
};

#define MBUF_DUMP_BUF_CHK() {   \
	clen -= k;              \
	if (clen < 1)           \
	        goto done;      \
	c += k;                 \
}

static char *
mbuf_dump(void)
{
	unsigned long totmem = 0, totfree = 0, totmbufs, totused, totpct,
	    totreturned = 0;
	u_int32_t m_mbufs = 0, m_clfree = 0, m_bigclfree = 0;
	u_int32_t m_mbufclfree = 0, m_mbufbigclfree = 0;
	u_int32_t m_16kclusters = 0, m_16kclfree = 0, m_mbuf16kclfree = 0;
	int nmbtypes = sizeof(mbstat.m_mtypes) / sizeof(short);
	uint8_t seen[256];
	struct mbtypes *mp;
	mb_class_stat_t *sp;
	mleak_trace_stat_t *mltr;
	char *c = mbuf_dump_buf;
	int i, j, k, clen = MBUF_DUMP_BUF_SIZE;
	bool printed_banner = false;

	mbuf_dump_buf[0] = '\0';

	                                                  
	mbuf_stat_sync();
	mbuf_mtypes_sync(TRUE);

	sp = &mb_stat->mbs_class[0];
	for (i = 0; i < mb_stat->mbs_cnt; i++, sp++) {
		u_int32_t mem;

		if (m_class(i) == MC_MBUF) {
			m_mbufs = sp->mbcl_active;
		} else if (m_class(i) == MC_CL) {
			m_clfree = sp->mbcl_total - sp->mbcl_active;
		} else if (m_class(i) == MC_BIGCL) {
			m_bigclfree = sp->mbcl_total - sp->mbcl_active;
		} else if (njcl > 0 && m_class(i) == MC_16KCL) {
			m_16kclfree = sp->mbcl_total - sp->mbcl_active;
			m_16kclusters = sp->mbcl_total;
		} else if (m_class(i) == MC_MBUF_CL) {
			m_mbufclfree = sp->mbcl_total - sp->mbcl_active;
		} else if (m_class(i) == MC_MBUF_BIGCL) {
			m_mbufbigclfree = sp->mbcl_total - sp->mbcl_active;
		} else if (njcl > 0 && m_class(i) == MC_MBUF_16KCL) {
			m_mbuf16kclfree = sp->mbcl_total - sp->mbcl_active;
		}

		mem = sp->mbcl_ctotal * sp->mbcl_size;
		totmem += mem;
		totfree += (sp->mbcl_mc_cached + sp->mbcl_infree) *
		    sp->mbcl_size;
		totreturned += sp->mbcl_release_cnt;
	}

	                                                    
	m_clfree += m_mbufclfree;
	m_bigclfree += m_mbufbigclfree;
	m_16kclfree += m_mbuf16kclfree;

	totmbufs = 0;
	for (mp = mbtypes; mp->mt_name != NULL; mp++) {
		totmbufs += mbstat.m_mtypes[mp->mt_type];
	}
	if (totmbufs > m_mbufs) {
		totmbufs = m_mbufs;
	}
	k = scnprintf(c, clen, "%lu/%u mbufs in use:\n", totmbufs, m_mbufs);
	MBUF_DUMP_BUF_CHK();

	bzero(&seen, sizeof(seen));
	for (mp = mbtypes; mp->mt_name != NULL; mp++) {
		if (mbstat.m_mtypes[mp->mt_type] != 0) {
			seen[mp->mt_type] = 1;
			k = scnprintf(c, clen, "\t%u mbufs allocated to %s\n",
			    mbstat.m_mtypes[mp->mt_type], mp->mt_name);
			MBUF_DUMP_BUF_CHK();
		}
	}
	seen[MT_FREE] = 1;
	for (i = 0; i < nmbtypes; i++) {
		if (!seen[i] && mbstat.m_mtypes[i] != 0) {
			k = scnprintf(c, clen, "\t%u mbufs allocated to "
			    "<mbuf type %d>\n", mbstat.m_mtypes[i], i);
			MBUF_DUMP_BUF_CHK();
		}
	}
	if ((m_mbufs - totmbufs) > 0) {
		k = scnprintf(c, clen, "\t%lu mbufs allocated to caches\n",
		    m_mbufs - totmbufs);
		MBUF_DUMP_BUF_CHK();
	}
	k = scnprintf(c, clen, "%u/%u mbuf 2KB clusters in use\n"
	    "%u/%u mbuf 4KB clusters in use\n",
	    (unsigned int)(mbstat.m_clusters - m_clfree),
	    (unsigned int)mbstat.m_clusters,
	    (unsigned int)(mbstat.m_bigclusters - m_bigclfree),
	    (unsigned int)mbstat.m_bigclusters);
	MBUF_DUMP_BUF_CHK();

	if (njcl > 0) {
		k = scnprintf(c, clen, "%u/%u mbuf %uKB clusters in use\n",
		    m_16kclusters - m_16kclfree, m_16kclusters,
		    njclbytes / 1024);
		MBUF_DUMP_BUF_CHK();
	}
	totused = totmem - totfree;
	if (totmem == 0) {
		totpct = 0;
	} else if (totused < (ULONG_MAX / 100)) {
		totpct = (totused * 100) / totmem;
	} else {
		u_long totmem1 = totmem / 100;
		u_long totused1 = totused / 100;
		totpct = (totused1 * 100) / totmem1;
	}
	k = scnprintf(c, clen, "%lu KB allocated to network (approx. %lu%% "
	    "in use)\n", totmem / 1024, totpct);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "%lu KB returned to the system\n",
	    totreturned / 1024);
	MBUF_DUMP_BUF_CHK();

	net_update_uptime();
	k = scnprintf(c, clen,
	    "VM allocation failures: contiguous %u, normal %u, one page %u\n",
	    mb_kmem_contig_failed, mb_kmem_failed, mb_kmem_one_failed);
	MBUF_DUMP_BUF_CHK();
	if (mb_kmem_contig_failed_ts || mb_kmem_failed_ts ||
	    mb_kmem_one_failed_ts) {
		k = scnprintf(c, clen,
		    "VM allocation failure timestamps: contiguous %llu "
		    "(size %llu), normal %llu (size %llu), one page %llu "
		    "(now %llu)\n",
		    mb_kmem_contig_failed_ts, mb_kmem_contig_failed_size,
		    mb_kmem_failed_ts, mb_kmem_failed_size,
		    mb_kmem_one_failed_ts, net_uptime());
		MBUF_DUMP_BUF_CHK();
		k = scnprintf(c, clen,
		    "VM return codes: ");
		MBUF_DUMP_BUF_CHK();
		for (i = 0;
		    i < sizeof(mb_kmem_stats) / sizeof(mb_kmem_stats[0]);
		    i++) {
			k = scnprintf(c, clen, "%s: %u ", mb_kmem_stats_labels[i],
			    mb_kmem_stats[i]);
			MBUF_DUMP_BUF_CHK();
		}
		k = scnprintf(c, clen, "\n");
		MBUF_DUMP_BUF_CHK();
	}
	k = scnprintf(c, clen,
	    "worker thread runs: %u, expansions: %llu, cl %llu/%llu, "
	    "bigcl %llu/%llu, 16k %llu/%llu\n", mbuf_worker_run_cnt,
	    mb_expand_cnt, mb_expand_cl_cnt, mb_expand_cl_total,
	    mb_expand_bigcl_cnt, mb_expand_bigcl_total, mb_expand_16kcl_cnt,
	    mb_expand_16kcl_total);
	MBUF_DUMP_BUF_CHK();
	if (mbuf_worker_last_runtime != 0) {
		k = scnprintf(c, clen, "worker thread last run time: "
		    "%llu (%llu seconds ago)\n",
		    mbuf_worker_last_runtime,
		    net_uptime() - mbuf_worker_last_runtime);
		MBUF_DUMP_BUF_CHK();
	}
	if (mbuf_drain_last_runtime != 0) {
		k = scnprintf(c, clen, "drain routine last run time: "
		    "%llu (%llu seconds ago)\n",
		    mbuf_drain_last_runtime,
		    net_uptime() - mbuf_drain_last_runtime);
		MBUF_DUMP_BUF_CHK();
	}

#if DEBUG || DEVELOPMENT
	k = scnprintf(c, clen, "\nworker thread log:\n%s\n", mbwdog_logging);
	MBUF_DUMP_BUF_CHK();
#endif

	for (j = 0; j < MTRACELARGE_NUM_TRACES; j++) {
		struct mtracelarge *trace = &mtracelarge_table[j];
		if (trace->size == 0 || trace->depth == 0) {
			continue;
		}
		if (printed_banner == false) {
			k = scnprintf(c, clen,
			    "\nlargest allocation failure backtraces:\n");
			MBUF_DUMP_BUF_CHK();
			printed_banner = true;
		}
		k = scnprintf(c, clen, "size %llu: < ", trace->size);
		MBUF_DUMP_BUF_CHK();
		for (i = 0; i < trace->depth; i++) {
			if (mleak_stat->ml_isaddr64) {
				k = scnprintf(c, clen, "0x%0llx ",
				    (uint64_t)VM_KERNEL_UNSLIDE(
					    trace->addr[i]));
			} else {
				k = scnprintf(c, clen,
				    "0x%08x ",
				    (uint32_t)VM_KERNEL_UNSLIDE(
					    trace->addr[i]));
			}
			MBUF_DUMP_BUF_CHK();
		}
		k = scnprintf(c, clen, ">\n");
		MBUF_DUMP_BUF_CHK();
	}

	                                    
	mleak_update_stats();

	k = scnprintf(c, clen, "\nmbuf leak detection table:\n");
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\ttotal captured: %u (one per %u)\n",
	    mleak_table.mleak_capture / mleak_table.mleak_sample_factor,
	    mleak_table.mleak_sample_factor);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\ttotal allocs outstanding: %llu\n",
	    mleak_table.outstanding_allocs);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\tnew hash recorded: %llu allocs, %llu traces\n",
	    mleak_table.alloc_recorded, mleak_table.trace_recorded);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\thash collisions: %llu allocs, %llu traces\n",
	    mleak_table.alloc_collisions, mleak_table.trace_collisions);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\toverwrites: %llu allocs, %llu traces\n",
	    mleak_table.alloc_overwrites, mleak_table.trace_overwrites);
	MBUF_DUMP_BUF_CHK();
	k = scnprintf(c, clen, "\tlock conflicts: %llu\n\n",
	    mleak_table.total_conflicts);
	MBUF_DUMP_BUF_CHK();

	k = scnprintf(c, clen, "top %d outstanding traces:\n",
	    mleak_stat->ml_cnt);
	MBUF_DUMP_BUF_CHK();
	for (i = 0; i < mleak_stat->ml_cnt; i++) {
		mltr = &mleak_stat->ml_trace[i];
		k = scnprintf(c, clen, "[%d] %llu outstanding alloc(s), "
		    "%llu hit(s), %llu collision(s)\n", (i + 1),
		    mltr->mltr_allocs, mltr->mltr_hitcount,
		    mltr->mltr_collisions);
		MBUF_DUMP_BUF_CHK();
	}

	if (mleak_stat->ml_isaddr64) {
		k = scnprintf(c, clen, MB_LEAK_HDR_64);
	} else {
		k = scnprintf(c, clen, MB_LEAK_HDR_32);
	}
	MBUF_DUMP_BUF_CHK();

	for (i = 0; i < MLEAK_STACK_DEPTH; i++) {
		k = scnprintf(c, clen, "%2d: ", (i + 1));
		MBUF_DUMP_BUF_CHK();
		for (j = 0; j < mleak_stat->ml_cnt; j++) {
			mltr = &mleak_stat->ml_trace[j];
			if (i < mltr->mltr_depth) {
				if (mleak_stat->ml_isaddr64) {
					k = scnprintf(c, clen, "0x%0llx  ",
					    (uint64_t)VM_KERNEL_UNSLIDE(
						    mltr->mltr_addr[i]));
				} else {
					k = scnprintf(c, clen,
					    "0x%08x  ",
					    (uint32_t)VM_KERNEL_UNSLIDE(
						    mltr->mltr_addr[i]));
				}
			} else {
				if (mleak_stat->ml_isaddr64) {
					k = scnprintf(c, clen,
					    MB_LEAK_SPACING_64);
				} else {
					k = scnprintf(c, clen,
					    MB_LEAK_SPACING_32);
				}
			}
			MBUF_DUMP_BUF_CHK();
		}
		k = scnprintf(c, clen, "\n");
		MBUF_DUMP_BUF_CHK();
	}
done:
	return mbuf_dump_buf;
}

#undef MBUF_DUMP_BUF_CHK

  
                                                                             
                                                                            
   
int
m_reinit(struct mbuf *m, int hdr)
{
	int ret = 0;

	if (hdr) {
		VERIFY(!(m->m_flags & M_PKTHDR));
		if (!(m->m_flags & M_EXT) &&
		    (m->m_data != m->m_dat || m->m_len > 0)) {
			  
                                                     
                                                  
                                                  
                                                  
                    
      
			printf("%s: cannot set M_PKTHDR on altered mbuf %llx, "
			    "m_data %llx (expected %llx), "
			    "m_len %d (expected 0)\n",
			    __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m),
			    (uint64_t)VM_KERNEL_ADDRPERM(m->m_data),
			    (uint64_t)VM_KERNEL_ADDRPERM(m->m_dat), m->m_len);
			ret = EBUSY;
		} else {
			VERIFY((m->m_flags & M_EXT) || m->m_data == m->m_dat);
			m->m_flags |= M_PKTHDR;
			MBUF_INIT_PKTHDR(m);
		}
	} else {
		                                     
		m_redzone_verify(m);
		                                                
		m_tag_delete_chain(m, NULL);
		m->m_flags &= ~M_PKTHDR;
	}

	return ret;
}

int
m_ext_set_prop(struct mbuf *m, uint32_t o, uint32_t n)
{
	ASSERT(m->m_flags & M_EXT);
	return atomic_test_set_32(&MEXT_PRIV(m), o, n);
}

uint32_t
m_ext_get_prop(struct mbuf *m)
{
	ASSERT(m->m_flags & M_EXT);
	return MEXT_PRIV(m);
}

int
m_ext_paired_is_active(struct mbuf *m)
{
	return MBUF_IS_PAIRED(m) ? (MEXT_PREF(m) > MEXT_MINREF(m)) : 1;
}

void
m_ext_paired_activate(struct mbuf *m)
{
	struct ext_ref *rfa;
	int hdr, type;
	caddr_t extbuf;
	m_ext_free_func_t extfree;
	u_int extsize;

	VERIFY(MBUF_IS_PAIRED(m));
	VERIFY(MEXT_REF(m) == MEXT_MINREF(m));
	VERIFY(MEXT_PREF(m) == MEXT_MINREF(m));

	hdr = (m->m_flags & M_PKTHDR);
	type = m->m_type;
	extbuf = m->m_ext.ext_buf;
	extfree = m_get_ext_free(m);
	extsize = m->m_ext.ext_size;
	rfa = m_get_rfa(m);

	VERIFY(extbuf != NULL && rfa != NULL);

	  
                                                       
                                                       
                                                      
                                        
    
	MBUF_INIT(m, hdr, type);
	MEXT_INIT(m, extbuf, extsize, extfree, (caddr_t)m, rfa,
	    1, 1, 2, EXTF_PAIRED, MEXT_PRIV(m), m);
}

void
m_scratch_init(struct mbuf *m)
{
	struct pkthdr *pkt = &m->m_pkthdr;

	VERIFY(m->m_flags & M_PKTHDR);

	                                               
	if (pkt->pkt_flags & PKTF_PRIV_GUARDED) {
		panic_plain("Invalid attempt to modify guarded module-private "
		    "area: mbuf %p, pkt_flags 0x%x\n", m, pkt->pkt_flags);
		                
	}

	bzero(&pkt->pkt_mpriv, sizeof(pkt->pkt_mpriv));
}

  
                                                                         
                                                                       
                                                                          
                                                                        
                                                     
   
u_int32_t
m_scratch_get(struct mbuf *m, u_int8_t **p)
{
	struct pkthdr *pkt = &m->m_pkthdr;

	VERIFY(m->m_flags & M_PKTHDR);

	                                               
	if (pkt->pkt_flags & PKTF_PRIV_GUARDED) {
		panic_plain("Invalid attempt to access guarded module-private "
		    "area: mbuf %p, pkt_flags 0x%x\n", m, pkt->pkt_flags);
		                
	}

	if (mcltrace) {
		mcache_audit_t *mca;

		lck_mtx_lock(mbuf_mlock);
		mca = mcl_audit_buf2mca(MC_MBUF, (mcache_obj_t *)m);
		if (mca->mca_uflags & MB_SCVALID) {
			mcl_audit_scratch(mca);
		}
		lck_mtx_unlock(mbuf_mlock);
	}

	*p = (u_int8_t *)&pkt->pkt_mpriv;
	return sizeof(pkt->pkt_mpriv);
}

static void
m_redzone_init(struct mbuf *m)
{
	VERIFY(m->m_flags & M_PKTHDR);
	  
                                                           
                                                       
    
	m->m_pkthdr.redzone = ((u_int32_t)(uintptr_t)m) ^ mb_redzone_cookie;
}

static void
m_redzone_verify(struct mbuf *m)
{
	u_int32_t mb_redzone;

	VERIFY(m->m_flags & M_PKTHDR);

	mb_redzone = ((u_int32_t)(uintptr_t)m) ^ mb_redzone_cookie;
	if (m->m_pkthdr.redzone != mb_redzone) {
		panic("mbuf %p redzone violation with value 0x%x "
		    "(instead of 0x%x, using cookie 0x%x)\n",
		    m, m->m_pkthdr.redzone, mb_redzone, mb_redzone_cookie);
		                
	}
}

__private_extern__ inline void
m_set_ext(struct mbuf *m, struct ext_ref *rfa, m_ext_free_func_t ext_free,
    caddr_t ext_arg)
{
	VERIFY(m->m_flags & M_EXT);
	if (rfa != NULL) {
		m->m_ext.ext_refflags =
		    (struct ext_ref *)(((uintptr_t)rfa) ^ mb_obscure_extref);
		if (ext_free != NULL) {
			rfa->ext_token = ((uintptr_t)&rfa->ext_token) ^
			    mb_obscure_extfree;
			uintptr_t ext_free_val = ptrauth_nop_cast(uintptr_t, ext_free) ^ rfa->ext_token;
			m->m_ext.ext_free = ptrauth_nop_cast(m_ext_free_func_t, ext_free_val);
			if (ext_arg != NULL) {
				m->m_ext.ext_arg =
				    (caddr_t)(((uintptr_t)ext_arg) ^ rfa->ext_token);
			} else {
				m->m_ext.ext_arg = NULL;
			}
		} else {
			rfa->ext_token = 0;
			m->m_ext.ext_free = NULL;
			m->m_ext.ext_arg = NULL;
		}
	} else {
		  
                                                        
                                                       
                                                  
     
		if (ext_free != NULL) {
			uintptr_t ext_free_val = ptrauth_nop_cast(uintptr_t, ext_free) ^ mb_obscure_extfree;
			m->m_ext.ext_free = ptrauth_nop_cast(m_ext_free_func_t, ext_free_val);
			if (ext_arg != NULL) {
				m->m_ext.ext_arg =
				    (caddr_t)((uintptr_t)ext_arg ^
				    mb_obscure_extfree);
			} else {
				m->m_ext.ext_arg = NULL;
			}
		} else {
			m->m_ext.ext_free = NULL;
			m->m_ext.ext_arg = NULL;
		}
		m->m_ext.ext_refflags = NULL;
	}
}

__private_extern__ inline struct ext_ref *
m_get_rfa(struct mbuf *m)
{
	if (m->m_ext.ext_refflags == NULL) {
		return NULL;
	} else {
		return (struct ext_ref *)(((uintptr_t)m->m_ext.ext_refflags) ^ mb_obscure_extref);
	}
}

__private_extern__ inline m_ext_free_func_t
m_get_ext_free(struct mbuf *m)
{
	struct ext_ref *rfa;
	if (m->m_ext.ext_free == NULL) {
		return NULL;
	}

	rfa = m_get_rfa(m);
	if (rfa == NULL) {
		uintptr_t ext_free_val = ptrauth_nop_cast(uintptr_t, m->m_ext.ext_free) ^ mb_obscure_extfree;
		return ptrauth_nop_cast(m_ext_free_func_t, ext_free_val);
	} else {
		uintptr_t ext_free_val = ptrauth_nop_cast(uintptr_t, m->m_ext.ext_free) ^ rfa->ext_token;
		return ptrauth_nop_cast(m_ext_free_func_t, ext_free_val);
	}
}

__private_extern__ inline caddr_t
m_get_ext_arg(struct mbuf *m)
{
	struct ext_ref *rfa;
	if (m->m_ext.ext_arg == NULL) {
		return NULL;
	}

	rfa = m_get_rfa(m);
	if (rfa == NULL) {
		return (caddr_t)((uintptr_t)m->m_ext.ext_arg ^ mb_obscure_extfree);
	} else {
		return (caddr_t)(((uintptr_t)m->m_ext.ext_arg) ^
		       rfa->ext_token);
	}
}

  
                                                                       
                                                                   
  
                                                                      
                         
   
static boolean_t
mbuf_report_usage(mbuf_class_t cl)
{
	                                                       
	if (mb_peak_newreport) {
		return TRUE;
	}

	if (m_total(cl) > m_peak(cl) &&
	    m_total(cl) >= (m_maxlimit(cl) >> 4) &&
	    (m_total(cl) - m_peak(cl)) >= (m_peak(cl) >> 5)) {
		return TRUE;
	}
	return FALSE;
}

__private_extern__ void
mbuf_report_peak_usage(void)
{
	int i = 0;
	u_int64_t uptime;
	struct nstat_sysinfo_data ns_data;
	uint32_t memreleased = 0;
	static uint32_t prevmemreleased;

	uptime = net_uptime();
	lck_mtx_lock(mbuf_mlock);

	                                                       
	if (!mb_peak_firstreport &&
	    uptime > MBUF_PEAK_FIRST_REPORT_THRESHOLD) {
		mb_peak_newreport = TRUE;
		mb_peak_firstreport = TRUE;
	}

	if (!mb_peak_newreport) {
		lck_mtx_unlock(mbuf_mlock);
		return;
	}

	  
                                                    
                                             
    
	if (uptime < MBUF_PEAK_FIRST_REPORT_THRESHOLD) {
		mb_peak_firstreport = TRUE;
	}

	for (i = 0; i < NELEM(mbuf_table); i++) {
		m_peak(m_class(i)) = m_total(m_class(i));
		memreleased += m_release_cnt(i);
	}
	memreleased = memreleased - prevmemreleased;
	prevmemreleased = memreleased;
	mb_peak_newreport = FALSE;
	lck_mtx_unlock(mbuf_mlock);

	bzero(&ns_data, sizeof(ns_data));
	ns_data.flags = NSTAT_SYSINFO_MBUF_STATS;
	ns_data.u.mb_stats.total_256b = m_peak(MC_MBUF);
	ns_data.u.mb_stats.total_2kb = m_peak(MC_CL);
	ns_data.u.mb_stats.total_4kb = m_peak(MC_BIGCL);
	ns_data.u.mb_stats.total_16kb = m_peak(MC_16KCL);
	ns_data.u.mb_stats.sbmb_total = total_sbmb_cnt_peak;
	ns_data.u.mb_stats.sb_atmbuflimit = sbmb_limreached;
	ns_data.u.mb_stats.draincnt = mbstat.m_drain;
	ns_data.u.mb_stats.memreleased = memreleased;
	ns_data.u.mb_stats.sbmb_floor = total_sbmb_cnt_floor;

	nstat_sysinfo_send_data(&ns_data);

	  
                                            
                                                
                                       
                                            
    
	total_sbmb_cnt_floor = total_sbmb_cnt_peak;
}

  
                                                                
              
   
static int
mbuf_drain_checks(boolean_t ignore_waiters)
{
	if (mb_drain_maxint == 0) {
		return 0;
	}
	if (!ignore_waiters && mb_waiters != 0) {
		return 0;
	}

	return 1;
}

  
                                                                     
                             
   
static void
mbuf_drain_locked(boolean_t ignore_waiters)
{
	mbuf_class_t mc;
	mcl_slab_t *sp, *sp_tmp, *nsp;
	unsigned int num, k, interval, released = 0;
	unsigned long total_mem = 0, use_mem = 0;
	boolean_t ret, purge_caches = FALSE;
	ppnum_t offset;
	mcache_obj_t *obj;
	unsigned long per;
	static unsigned char scratch[32];
	static ppnum_t scratch_pa = 0;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
	if (!mbuf_drain_checks(ignore_waiters)) {
		return;
	}
	if (scratch_pa == 0) {
		bzero(scratch, sizeof(scratch));
		scratch_pa = pmap_find_phys(kernel_pmap, (addr64_t)scratch);
		VERIFY(scratch_pa);
	} else if (mclverify) {
		  
                                                   
     
		for (k = 0; k < sizeof(scratch); k++) {
			if (scratch[k]) {
				panic("suspect DMA to freed address");
			}
		}
	}
	  
                                                             
                                                                    
                          
    
	if (mbuf_drain_last_runtime != 0) {
		interval = net_uptime() - mbuf_drain_last_runtime;
		if (interval <= mb_drain_maxint) {
			return;
		}
		if (interval <= mb_drain_maxint * 5) {
			purge_caches = TRUE;
		}
	}
	mbuf_drain_last_runtime = net_uptime();
	  
                                                     
    
	for (mc = 0; mc < NELEM(mbuf_table); mc++) {
		total_mem += m_total(mc) * m_maxsize(mc);
		use_mem += m_active(mc) * m_maxsize(mc);
	}
	per = (use_mem * 100) / total_mem;
	if (per >= 60) {
		return;
	}
	  
                                                    
                                                              
                         
    
	if (purge_caches == TRUE) {
		for (mc = 0; mc < NELEM(mbuf_table); mc++) {
			if (m_total(mc) < m_avgtotal(mc)) {
				continue;
			}
			lck_mtx_unlock(mbuf_mlock);
			ret = mcache_purge_cache(m_cache(mc), FALSE);
			lck_mtx_lock(mbuf_mlock);
			if (ret == TRUE) {
				m_purge_cnt(mc)++;
			}
		}
	}
	  
                                                         
                                                                    
                          
    
	for (mc = 0; mc < NELEM(mbuf_table); mc++) {
		while (m_cobjlist(mc) &&
		    m_total(mc) < m_avgtotal(mc) &&
		    m_infree(mc) > 0.1 * m_avgtotal(mc) + m_minlimit(mc)) {
			obj = m_cobjlist(mc);
			m_cobjlist(mc) = obj->obj_next;
			obj->obj_next = NULL;
			num = cslab_free(mc, obj, 1);
			VERIFY(num == 1);
			m_free_cnt(mc)++;
			m_infree(mc)--;
			                                  
		}
	}
	  
                                                                    
                      
   
                                                                     
    
	for (mc = NELEM(mbuf_table) - 1; (int)mc >= 0; mc--) {
		TAILQ_FOREACH_SAFE(sp, &m_slablist(mc), sl_link, sp_tmp) {
			  
                                                 
      
			if (sp->sl_refcnt != 0 || sp->sl_len == 0 ||
			    sp->sl_base == NULL) {
				continue;
			}
			if (m_total(mc) < m_avgtotal(mc) ||
			    m_infree(mc) < 0.1 * m_avgtotal(mc) + m_minlimit(mc)) {
				break;
			}
			slab_remove(sp, mc);
			switch (mc) {
			case MC_MBUF:
				m_infree(mc) -= NMBPG;
				m_total(mc) -= NMBPG;
				if (mclaudit != NULL) {
					mcl_audit_free(sp->sl_base, NMBPG);
				}
				break;
			case MC_CL:
				m_infree(mc) -= NCLPG;
				m_total(mc) -= NCLPG;
				if (mclaudit != NULL) {
					mcl_audit_free(sp->sl_base, NMBPG);
				}
				break;
			case MC_BIGCL:
			{
				m_infree(mc) -= NBCLPG;
				m_total(mc) -= NBCLPG;
				if (mclaudit != NULL) {
					mcl_audit_free(sp->sl_base, NMBPG);
				}
				break;
			}
			case MC_16KCL:
				m_infree(mc)--;
				m_total(mc)--;
				for (nsp = sp, k = 1; k < NSLABSP16KB; k++) {
					nsp = nsp->sl_next;
					VERIFY(nsp->sl_refcnt == 0 &&
					    nsp->sl_base != NULL &&
					    nsp->sl_len == 0);
					slab_init(nsp, 0, 0, NULL, NULL, 0, 0,
					    0);
					nsp->sl_flags = 0;
				}
				if (mclaudit != NULL) {
					if (sp->sl_len == PAGE_SIZE) {
						mcl_audit_free(sp->sl_base,
						    NMBPG);
					} else {
						mcl_audit_free(sp->sl_base, 1);
					}
				}
				break;
			default:
				  
                                           
                                        
                                        
       
				VERIFY(0);
			}
			m_release_cnt(mc) += m_size(mc);
			released += m_size(mc);
			VERIFY(sp->sl_base != NULL &&
			    sp->sl_len >= PAGE_SIZE);
			offset = MTOPG(sp->sl_base);
			  
                                                   
                                                      
                               
      
			IOMapperInsertPage(mcl_paddr_base, offset, scratch_pa);
			mcl_paddr[offset] = 0;
			kmem_free(mb_map, (vm_offset_t)sp->sl_base,
			    sp->sl_len);
			slab_init(sp, 0, 0, NULL, NULL, 0, 0, 0);
			sp->sl_flags = 0;
		}
	}
	mbstat.m_drain++;
	mbstat.m_bigclusters = m_total(MC_BIGCL);
	mbstat.m_clusters = m_total(MC_CL);
	mbstat.m_mbufs = m_total(MC_MBUF);
	mbuf_stat_sync();
	mbuf_mtypes_sync(TRUE);
}

__private_extern__ void
mbuf_drain(boolean_t ignore_waiters)
{
	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_NOTOWNED);
	if (!mbuf_drain_checks(ignore_waiters)) {
		return;
	}
	lck_mtx_lock(mbuf_mlock);
	mbuf_drain_locked(ignore_waiters);
	lck_mtx_unlock(mbuf_mlock);
}


static int
m_drain_force_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int val = 0, err;

	err = sysctl_handle_int(oidp, &val, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}
	if (val) {
		mbuf_drain(TRUE);
	}

	return err;
}

#if DEBUG || DEVELOPMENT
static void
_mbwdog_logger(const char *func, const int line, const char *fmt, ...)
{
	va_list ap;
	struct timeval now;
	char str[384], p[256];
	int len;

	LCK_MTX_ASSERT(mbuf_mlock, LCK_MTX_ASSERT_OWNED);
	if (mbwdog_logging == NULL) {
		  
                                                              
                                          
     
		mbwdog_logging = zalloc_permanent(mbwdog_logging_size,
		    ZALIGN_NONE);
	}
	va_start(ap, fmt);
	vsnprintf(p, sizeof(p), fmt, ap);
	va_end(ap);
	microuptime(&now);
	len = scnprintf(str, sizeof(str),
	    "\n%ld.%d (%d/%llx) %s:%d %s",
	    now.tv_sec, now.tv_usec,
	    current_proc()->p_pid,
	    (uint64_t)VM_KERNEL_ADDRPERM(current_thread()),
	    func, line, p);
	if (len < 0) {
		return;
	}
	if (mbwdog_logging_used + len > mbwdog_logging_size) {
		mbwdog_logging_used = mbwdog_logging_used / 2;
		memmove(mbwdog_logging, mbwdog_logging + mbwdog_logging_used,
		    mbwdog_logging_size - mbwdog_logging_used);
		mbwdog_logging[mbwdog_logging_used] = 0;
	}
	strlcat(mbwdog_logging, str, mbwdog_logging_size);
	mbwdog_logging_used += len;
}

static int
sysctl_mbwdog_log SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	return SYSCTL_OUT(req, mbwdog_logging, mbwdog_logging_used);
}
SYSCTL_DECL(_kern_ipc);
SYSCTL_PROC(_kern_ipc, OID_AUTO, mbwdog_log,
    CTLTYPE_STRING | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_mbwdog_log, "A", "");

#endif                        

static void
mtracelarge_register(size_t size)
{
	int i;
	struct mtracelarge *trace;
	uintptr_t bt[MLEAK_STACK_DEPTH];
	unsigned int depth;

	depth = backtrace(bt, MLEAK_STACK_DEPTH, NULL);
	                                                 
	for (i = 0; i < MTRACELARGE_NUM_TRACES; i++) {
		trace = &mtracelarge_table[i];
		if (trace->size == size && trace->depth == depth &&
		    memcmp(bt, trace->addr, depth * sizeof(uintptr_t)) == 0) {
			return;
		}
	}
	for (i = 0; i < MTRACELARGE_NUM_TRACES; i++) {
		trace = &mtracelarge_table[i];
		if (size > trace->size) {
			trace->depth = depth;
			memcpy(trace->addr, bt, depth * sizeof(uintptr_t));
			trace->size = size;
			break;
		}
	}
}

SYSCTL_DECL(_kern_ipc);
#if DEBUG || DEVELOPMENT
#endif
SYSCTL_PROC(_kern_ipc, KIPC_MBSTAT, mbstat,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, mbstat_sysctl, "S,mbstat", "");
SYSCTL_PROC(_kern_ipc, OID_AUTO, mb_stat,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, mb_stat_sysctl, "S,mb_stat", "");
SYSCTL_PROC(_kern_ipc, OID_AUTO, mleak_top_trace,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, mleak_top_trace_sysctl, "S,mb_top_trace", "");
SYSCTL_PROC(_kern_ipc, OID_AUTO, mleak_table,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, mleak_table_sysctl, "S,mleak_table", "");
SYSCTL_INT(_kern_ipc, OID_AUTO, mleak_sample_factor,
    CTLFLAG_RW | CTLFLAG_LOCKED, &mleak_table.mleak_sample_factor, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, mb_normalized,
    CTLFLAG_RD | CTLFLAG_LOCKED, &mb_normalized, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, mb_watchdog,
    CTLFLAG_RW | CTLFLAG_LOCKED, &mb_watchdog, 0, "");
SYSCTL_PROC(_kern_ipc, OID_AUTO, mb_drain_force,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, NULL, 0,
    m_drain_force_sysctl, "I",
    "Forces the mbuf garbage collection to run");
SYSCTL_INT(_kern_ipc, OID_AUTO, mb_drain_maxint,
    CTLFLAG_RW | CTLFLAG_LOCKED, &mb_drain_maxint, 0,
    "Minimum time interval between garbage collection");
SYSCTL_INT(_kern_ipc, OID_AUTO, mb_memory_pressure_percentage,
    CTLFLAG_RW | CTLFLAG_LOCKED, &mb_memory_pressure_percentage, 0,
    "Percentage of when we trigger memory-pressure for an mbuf-class");
