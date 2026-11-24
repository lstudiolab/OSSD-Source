/*
 * Copyright (c) 2019-2020 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Scooter Morris at Genentech Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *	@(#)ufs_lockf.c	8.3 (Berkeley) 1/6/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/unistd.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>
#include <sys/vnode_if.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/lockf.h>
#include <sys/sdt.h>
#include <kern/policy_internal.h>

#include <sys/file_internal.h>

#if (DEVELOPMENT || DEBUG)
#define LOCKF_DEBUGGING 1
#endif

#ifdef LOCKF_DEBUGGING
#include <sys/sysctl.h>
void lf_print(const char *tag, struct lockf *lock);
void lf_printlist(const char *tag, struct lockf *lock);

#define LF_DBG_LOCKOP   (1 << 0)                                   
#define LF_DBG_LIST     (1 << 1)                             
#define LF_DBG_IMPINH   (1 << 2)                                    
#define LF_DBG_TRACE    (1 << 3)                          
#define LF_DBG_DEADLOCK (1 << 4)                                

static int      lockf_debug = 0;                                   
SYSCTL_INT(_debug, OID_AUTO, lockf_debug, CTLFLAG_RW | CTLFLAG_LOCKED, &lockf_debug, 0, "");

  
                                                                
   
#define LOCKF_DEBUG(mask, ...)                                  \
	do {                                                    \
	        if ((mask) & lockf_debug) {        \
	                printf("%s>", __FUNCTION__);            \
	                printf(__VA_ARGS__);                    \
	        }                                               \
	} while(0)

#define LOCKF_DEBUGP(mask)                                      \
	({                                                      \
	        ((mask) & lockf_debug);                         \
	})
#else                         
#define LOCKF_DEBUG(mask, ...)                    
#endif                        

  
                                                     
                                     
   
#define KM_LOCKF       KHEAP_DEFAULT

#define NOLOCKF (struct lockf *)0
#define SELF    0x1
#define OTHERS  0x2
#define OFF_MAX 0x7fffffffffffffffULL                  

  
                          
   
typedef enum {
	OVERLAP_NONE = 0,
	OVERLAP_EQUALS_LOCK,
	OVERLAP_CONTAINS_LOCK,
	OVERLAP_CONTAINED_BY_LOCK,
	OVERLAP_STARTS_BEFORE_LOCK,
	OVERLAP_ENDS_AFTER_LOCK
} overlap_t;

static int       lf_clearlock(struct lockf *);
static overlap_t lf_findoverlap(struct lockf *,
    struct lockf *, int, struct lockf ***, struct lockf **);
static struct lockf *lf_getblock(struct lockf *, pid_t);
static int       lf_getlock(struct lockf *, struct flock *, pid_t);
static int       lf_setlock(struct lockf *, struct timespec *);
static int       lf_split(struct lockf *, struct lockf *);
static void      lf_wakelock(struct lockf *, boolean_t);
#if IMPORTANCE_INHERITANCE
static void      lf_hold_assertion(task_t, struct lockf *);
static void      lf_jump_to_queue_head(struct lockf *, struct lockf *);
static void      lf_drop_assertion(struct lockf *);
static void      lf_boost_blocking_proc(struct lockf *, struct lockf *);
static void      lf_adjust_assertion(struct lockf *block);
#endif                             

static LCK_GRP_DECLARE(lf_dead_lock_grp, "lf_dead_lock");
static LCK_MTX_DECLARE(lf_dead_lock, &lf_dead_lock_grp);

  
             
  
                                               
  
                                                           
                                            
                                      
  
                       
             
          
                                                   
                    
                     
                   
                    
                       
                      
                 
  
                                                                         
                                                               
                                  
   
int
lf_advlock(struct vnop_advlock_args *ap)
{
	struct vnode *vp = ap->a_vp;
	struct flock *fl = ap->a_fl;
	vfs_context_t context = ap->a_context;
	struct lockf *lock;
	off_t start, end, oadd;
	u_quad_t size;
	int error;
	struct lockf **head = &vp->v_lockf;

	                                                           

	  
                                                               
    
	if (*head == (struct lockf *)0) {
		if (ap->a_op != F_SETLK) {
			fl->l_type = F_UNLCK;
			LOCKF_DEBUG(LF_DBG_TRACE,
			    "lf_advlock: '%s' unlock without lock\n",
			    vfs_context_proc(context)->p_comm);
			return 0;
		}
	}

	  
                                                     
    
	switch (fl->l_whence) {
	case SEEK_SET:
	case SEEK_CUR:
		  
                                                          
                           
     
		start = fl->l_start;
		break;

	case SEEK_END:

		  
                                                               
                                                             
                                                                
                                                            
     
		if ((error = vnode_size(vp, (off_t *)&size, context))) {
			LOCKF_DEBUG(LF_DBG_TRACE,
			    "lf_advlock: vnode_getattr failed: %d\n", error);
			return error;
		}

		if (size > OFF_MAX ||
		    (fl->l_start > 0 &&
		    size > (u_quad_t)(OFF_MAX - fl->l_start))) {
			return EOVERFLOW;
		}
		start = size + fl->l_start;
		break;

	default:
		LOCKF_DEBUG(LF_DBG_TRACE, "lf_advlock: unknown whence %d\n",
		    fl->l_whence);
		return EINVAL;
	}
	if (start < 0) {
		LOCKF_DEBUG(LF_DBG_TRACE, "lf_advlock: start < 0 (%qd)\n",
		    start);
		return EINVAL;
	}
	if (fl->l_len < 0) {
		if (start == 0) {
			LOCKF_DEBUG(LF_DBG_TRACE,
			    "lf_advlock: len < 0 & start == 0\n");
			return EINVAL;
		}
		end = start - 1;
		start += fl->l_len;
		if (start < 0) {
			LOCKF_DEBUG(LF_DBG_TRACE,
			    "lf_advlock: start < 0 (%qd)\n", start);
			return EINVAL;
		}
	} else if (fl->l_len == 0) {
		end = -1;
	} else {
		oadd = fl->l_len - 1;
		if (oadd > (off_t)(OFF_MAX - start)) {
			LOCKF_DEBUG(LF_DBG_TRACE, "lf_advlock: overflow\n");
			return EOVERFLOW;
		}
		end = start + oadd;
	}
	  
                              
    
	lock = kheap_alloc(KM_LOCKF, sizeof(struct lockf), Z_WAITOK);
	if (lock == NULL) {
		return ENOLCK;
	}
	lock->lf_start = start;
	lock->lf_end = end;
	lock->lf_id = ap->a_id;
	lock->lf_vnode = vp;
	lock->lf_type = fl->l_type;
	lock->lf_head = head;
	lock->lf_next = (struct lockf *)0;
	TAILQ_INIT(&lock->lf_blkhd);
	lock->lf_flags = (short)ap->a_flags;
#if IMPORTANCE_INHERITANCE
	lock->lf_boosted = LF_NOT_BOOSTED;
#endif
	if (ap->a_flags & F_POSIX) {
		lock->lf_owner = (struct proc *)lock->lf_id;
	} else {
		lock->lf_owner = NULL;
	}

	if (ap->a_flags & F_FLOCK) {
		lock->lf_flags |= F_WAKE1_SAFE;
	}

	lck_mtx_lock(&vp->v_lock);                                  
	  
                               
    
	switch (ap->a_op) {
	case F_SETLK:
		  
                                              
                                                   
                                                   
                                                
                                                     
                                     
     
		if (ap->a_flags & F_OFD_LOCK) {
			struct fileglob *fg = (void *)lock->lf_id;
			if (fg->fg_lflags & FG_CONFINED) {
				lock->lf_owner = current_proc();
			}
		}
		error = lf_setlock(lock, ap->a_timeout);
		break;

	case F_UNLCK:
		error = lf_clearlock(lock);
		kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
		break;

	case F_GETLK:
		error = lf_getlock(lock, fl, -1);
		kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
		break;

	case F_GETLKPID:
		error = lf_getlock(lock, fl, fl->l_pid);
		kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
		break;

	default:
		kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
		error = EINVAL;
		break;
	}
	lck_mtx_unlock(&vp->v_lock);                                    

	LOCKF_DEBUG(LF_DBG_TRACE, "lf_advlock: normal exit: %d\n", error);
	return error;
}

  
                                                                       
                                                                        
                                                                           
                                                                           
   
void
lf_abort_advlocks(vnode_t vp)
{
	struct lockf *lock;

	if ((lock = vp->v_lockf) == NULL) {
		return;
	}

	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);

	if (!TAILQ_EMPTY(&lock->lf_blkhd)) {
		struct lockf *tlock;

		TAILQ_FOREACH(tlock, &lock->lf_blkhd, lf_block) {
			  
                                        
                                          
                                       
      
			tlock->lf_flags |= F_ABORT;
		}
		lf_wakelock(lock, TRUE);
	}
}

  
                                                                              
                                                                         
                                                                             
   
static void
lf_move_blocked(struct lockf *to, struct lockf *from)
{
	struct lockf *tlock;

	TAILQ_FOREACH(tlock, &from->lf_blkhd, lf_block) {
		tlock->lf_next = to;
	}

	TAILQ_CONCAT(&to->lf_blkhd, &from->lf_blkhd, lf_block);
}

  
                       
  
                                                                       
                                                              
                                              
  
                                                        
                                           
                                              
  
                  
   
static void
lf_coalesce_adjacent(struct lockf *lock)
{
	struct lockf **lf = lock->lf_head;

	while (*lf != NOLOCKF) {
		                                                        
		if ((*lf == lock) ||
		    ((*lf)->lf_id != lock->lf_id) ||
		    ((*lf)->lf_type != lock->lf_type)) {
			lf = &(*lf)->lf_next;
			continue;
		}

		  
                                                                    
                                                                      
     
		if ((*lf)->lf_end != -1 &&
		    ((*lf)->lf_end + 1) == lock->lf_start) {
			struct lockf *adjacent = *lf;

			LOCKF_DEBUG(LF_DBG_LIST, "lf_coalesce_adjacent: coalesce adjacent previous\n");
			lock->lf_start = (*lf)->lf_start;
			*lf = lock;
			lf = &(*lf)->lf_next;

			lf_move_blocked(lock, adjacent);

			kheap_free(KM_LOCKF, adjacent, sizeof(struct lockf));
			continue;
		}
		                                                           
		if (lock->lf_end != -1 &&
		    (lock->lf_end + 1) == (*lf)->lf_start) {
			struct lockf *adjacent = *lf;

			LOCKF_DEBUG(LF_DBG_LIST, "lf_coalesce_adjacent: coalesce adjacent following\n");
			lock->lf_end = (*lf)->lf_end;
			lock->lf_next = (*lf)->lf_next;
			lf = &lock->lf_next;

			lf_move_blocked(lock, adjacent);

			kheap_free(KM_LOCKF, adjacent, sizeof(struct lockf));
			continue;
		}

		                                                
		lf = &(*lf)->lf_next;
	}
}

  
             
  
                                      
  
                                                            
                                             
                                           
                                              
                           
  
                                              
                                                      
  
                       
          
           
                  
                      
               
                   
  
                                                                  
                                                               
                                               
   
static int
lf_setlock(struct lockf *lock, struct timespec *timeout)
{
	struct lockf *block;
	struct lockf **head = lock->lf_head;
	struct lockf **prev, *overlap;
	static const char lockstr[] = "lockf";
	int priority, needtolink, error;
	struct vnode *vp = lock->lf_vnode;
	overlap_t ovcase;

#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
		lf_print("lf_setlock", lock);
		lf_printlist("lf_setlock(in)", lock);
	}
#endif                      
	LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p Looking for deadlock, vnode %p\n", lock, lock->lf_vnode);

	  
                    
    
	priority = PLOCK;
	if (lock->lf_type == F_WRLCK) {
		priority += 4;
	}
	priority |= PCATCH;
scan:
	  
                                                                       
    
	while ((block = lf_getblock(lock, -1))) {
		  
                                                  
     
		if ((lock->lf_flags & F_WAIT) == 0) {
			DTRACE_FSINFO(advlock__nowait, vnode_t, vp);
			kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
			return EAGAIN;
		}

		LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p found blocking lock %p\n", lock, block);

		  
                                                  
                                                     
    
                                                  
                        
    
                                                           
    
                                                      
                                                      
                
     
		if ((lock->lf_flags & F_POSIX) &&
		    (block->lf_flags & F_POSIX)) {
			lck_mtx_lock(&lf_dead_lock);

			                                                 
			struct proc *wproc = block->lf_owner;
			proc_lock(wproc);

			LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p owned by pid %d\n", lock, proc_pid(wproc));

			struct uthread *ut;
			TAILQ_FOREACH(ut, &wproc->p_uthlist, uu_list) {
				  
                                                  
                                                 
                                                      
                                 
      
                                                     
                                                             
                                                         
                 
      
                                                                
                                                           
                                                       
                                                    
                                                             
      
                                                                 
                                                                    
                                                                
                                                                 
                                                                 
                                                              
                                                  
       
				const struct lockf *waitblock = (const void *)ut->uu_wchan;
				if ((waitblock != NULL) && (ut->uu_wmesg == lockstr)) {
					LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p which is also blocked on lock %p vnode %p\n", lock, waitblock, waitblock->lf_vnode);

					vnode_t othervp = NULL;
					if (waitblock->lf_vnode != vp) {
						  
                                                   
                                                  
                                                   
         
						othervp = waitblock->lf_vnode;
						if (!lck_mtx_try_lock(&othervp->v_lock)) {
							  
                                         
                                           
                                              
                                          
                                 
          
							proc_unlock(wproc);
							lck_mtx_unlock(&lf_dead_lock);
							static struct timespec ts = {
								.tv_sec = 0,
								.tv_nsec = 2 * NSEC_PER_MSEC,
							};
							static const char pausestr[] = "lockf:pause";
							(void) msleep(lock, &vp->v_lock, priority, pausestr, &ts);
							LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p contention for vp %p => restart\n", lock, othervp);
							goto scan;
						}
					}

					  
                                      
                                      
                                          
                                            
                                           
                                    
                   
        
					const struct lockf *nextblock = waitblock->lf_next;
					if (nextblock == NULL) {
						if (othervp) {
							lck_mtx_unlock(&othervp->v_lock);
						}
						LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p with waitblock %p and no lf_next; othervp %p\n", lock, waitblock, othervp);
						continue;
					}
					LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p which is also blocked on lock %p vnode %p\n", lock, nextblock, nextblock->lf_vnode);

					  
                                        
                                            
                                          
              
        
					if ((nextblock->lf_flags & F_POSIX) == 0) {
						if (othervp) {
							lck_mtx_unlock(&othervp->v_lock);
						}
						continue;
					}

					  
                                       
                                          
                                           
                                     
        
					struct proc *bproc = nextblock->lf_owner;
					const boolean_t deadlocked = bproc == lock->lf_owner;

					if (othervp) {
						lck_mtx_unlock(&othervp->v_lock);
					}
					LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p owned by pid %d\n", lock, proc_pid(bproc));
					if (deadlocked) {
						LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p which is me, so EDEADLK\n", lock);
						proc_unlock(wproc);
						lck_mtx_unlock(&lf_dead_lock);
						kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
						return EDEADLK;
					}
				}
				LOCKF_DEBUG(LF_DBG_DEADLOCK, "lock %p bottom of thread loop\n", lock);
			}
			proc_unlock(wproc);
			lck_mtx_unlock(&lf_dead_lock);
		}

		  
                                               
                                                  
                                   
     
		if ((lock->lf_flags & F_FLOCK) &&
		    lock->lf_type == F_WRLCK) {
			lock->lf_type = F_UNLCK;
			if ((error = lf_clearlock(lock)) != 0) {
				kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
				return error;
			}
			lock->lf_type = F_WRLCK;
		}
		  
                                                                 
                                                      
     
		lock->lf_next = block;
		TAILQ_INSERT_TAIL(&block->lf_blkhd, lock, lf_block);

		if (!(lock->lf_flags & F_FLOCK)) {
			block->lf_flags &= ~F_WAKE1_SAFE;
		}

#if IMPORTANCE_INHERITANCE
		  
                                                         
                                                 
    
                                                           
                                                            
             
    
                                                              
                                                                
                                                            
                                                          
                                         
     
		if ((lock->lf_flags & block->lf_flags & F_POSIX) != 0) {
			lf_boost_blocking_proc(lock, block);
		} else if ((lock->lf_flags & block->lf_flags & F_OFD_LOCK) &&
		    lock->lf_owner != block->lf_owner &&
		    NULL != lock->lf_owner && NULL != block->lf_owner) {
			lf_boost_blocking_proc(lock, block);
		}
#endif                             

#ifdef LOCKF_DEBUGGING
		if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
			lf_print("lf_setlock: blocking on", block);
			lf_printlist("lf_setlock(block)", block);
		}
#endif                      
		DTRACE_FSINFO(advlock__wait, vnode_t, vp);

		if (lock->lf_flags & F_POSIX) {
			error = msleep(lock, &vp->v_lock, priority, lockstr, timeout);
			  
                                                          
                                                            
                               
      
			lck_mtx_lock(&lf_dead_lock);
			lck_mtx_unlock(&lf_dead_lock);
		} else {
			static const char lockstr_np[] = "lockf:np";
			error = msleep(lock, &vp->v_lock, priority, lockstr_np, timeout);
		}

		if (error == 0 && (lock->lf_flags & F_ABORT) != 0) {
			error = EBADF;
		}

		if (lock->lf_next) {
			  
                                                    
                                                     
                                                     
     
                                                          
                                                       
      
			TAILQ_REMOVE(&lock->lf_next->lf_blkhd, lock, lf_block);
#if IMPORTANCE_INHERITANCE
			  
                                  
      
			lf_adjust_assertion(lock->lf_next);
#endif                             
			lock->lf_next = NULL;

			if (error == 0) {
				  
                                           
       
				printf("%s: spurious wakeup, retrying lock\n",
				    __func__);
				continue;
			}
		}

		if (!TAILQ_EMPTY(&lock->lf_blkhd)) {
			if ((block = lf_getblock(lock, -1)) != NULL) {
				lf_move_blocked(block, lock);
			}
		}

		if (error) {
			if (!TAILQ_EMPTY(&lock->lf_blkhd)) {
				lf_wakelock(lock, TRUE);
			}
			kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
			                                           
			if (error == EWOULDBLOCK) {
				error = ETIMEDOUT;
			}
			return error;
		}
	}

	  
                                                 
                                                   
                         
   
                                             
                                                             
    
	prev = head;
	block = *head;
	needtolink = 1;
	for (;;) {
		ovcase = lf_findoverlap(block, lock, SELF, &prev, &overlap);
		if (ovcase) {
			block = overlap->lf_next;
		}
		  
               
                  
                       
                             
                             
                                  
                               
     
		switch (ovcase) {
		case OVERLAP_NONE:
			if (needtolink) {
				*prev = lock;
				lock->lf_next = overlap;
			}
			break;

		case OVERLAP_EQUALS_LOCK:
			  
                                        
                         
      
			if (lock->lf_type == F_RDLCK &&
			    overlap->lf_type == F_WRLCK) {
				lf_wakelock(overlap, TRUE);
			}
			overlap->lf_type = lock->lf_type;
			lf_move_blocked(overlap, lock);
			kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
			lock = overlap;                                 
			break;

		case OVERLAP_CONTAINS_LOCK:
			  
                                                          
      
			if (overlap->lf_type == lock->lf_type) {
				lf_move_blocked(overlap, lock);
				kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
				lock = overlap;                                 
				break;
			}
			if (overlap->lf_start == lock->lf_start) {
				*prev = lock;
				lock->lf_next = overlap;
				overlap->lf_start = lock->lf_end + 1;
			} else {
				  
                                           
                                              
                         
       
				if (lf_split(overlap, lock)) {
					kheap_free(KM_LOCKF, lock, sizeof(struct lockf));
					return ENOLCK;
				}
			}
			lf_wakelock(overlap, TRUE);
			break;

		case OVERLAP_CONTAINED_BY_LOCK:
			  
                                                
                                          
      
			if (lock->lf_type == F_RDLCK &&
			    overlap->lf_type == F_WRLCK) {
				lf_wakelock(overlap, TRUE);
			} else {
				lf_move_blocked(lock, overlap);
			}
			  
                                                           
      
			if (needtolink) {
				*prev = lock;
				lock->lf_next = overlap->lf_next;
				prev = &lock->lf_next;
				needtolink = 0;
			} else {
				*prev = overlap->lf_next;
			}
			kheap_free(KM_LOCKF, overlap, sizeof(struct lockf));
			continue;

		case OVERLAP_STARTS_BEFORE_LOCK:
			  
                                         
      
			lock->lf_next = overlap->lf_next;
			overlap->lf_next = lock;
			overlap->lf_end = lock->lf_start - 1;
			prev = &lock->lf_next;
			lf_wakelock(overlap, TRUE);
			needtolink = 0;
			continue;

		case OVERLAP_ENDS_AFTER_LOCK:
			  
                                      
      
			if (needtolink) {
				*prev = lock;
				lock->lf_next = overlap;
			}
			overlap->lf_start = lock->lf_end + 1;
			lf_wakelock(overlap, TRUE);
			break;
		}
		break;
	}
	                                                       
	lf_coalesce_adjacent(lock);
#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
		lf_print("lf_setlock: got the lock", lock);
		lf_printlist("lf_setlock(out)", lock);
	}
#endif                      
	return 0;
}


  
               
  
                                                                          
                                                              
                                   
  
                                         
  
                       
                  
  
                                                                  
                                                          
                                               
   
static int
lf_clearlock(struct lockf *unlock)
{
	struct lockf **head = unlock->lf_head;
	struct lockf *lf = *head;
	struct lockf *overlap, **prev;
	overlap_t ovcase;

	if (lf == NOLOCKF) {
		return 0;
	}
#ifdef LOCKF_DEBUGGING
	if (unlock->lf_type != F_UNLCK) {
		panic("lf_clearlock: bad type");
	}
	if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
		lf_print("lf_clearlock", unlock);
	}
#endif                      
	prev = head;
	while ((ovcase = lf_findoverlap(lf, unlock, SELF, &prev, &overlap)) != OVERLAP_NONE) {
		  
                                            
     
		lf_wakelock(overlap, FALSE);
#if IMPORTANCE_INHERITANCE
		if (overlap->lf_boosted == LF_BOOSTED) {
			lf_drop_assertion(overlap);
		}
#endif                             

		switch (ovcase) {
		case OVERLAP_NONE:                                        
			break;

		case OVERLAP_EQUALS_LOCK:
			*prev = overlap->lf_next;
			kheap_free(KM_LOCKF, overlap, sizeof(struct lockf));
			break;

		case OVERLAP_CONTAINS_LOCK:               
			if (overlap->lf_start == unlock->lf_start) {
				overlap->lf_start = unlock->lf_end + 1;
				break;
			}
			  
                                                    
                                                     
      
			if (lf_split(overlap, unlock)) {
				return ENOLCK;
			}
			overlap->lf_next = unlock->lf_next;
			break;

		case OVERLAP_CONTAINED_BY_LOCK:
			*prev = overlap->lf_next;
			lf = overlap->lf_next;
			kheap_free(KM_LOCKF, overlap, sizeof(struct lockf));
			continue;

		case OVERLAP_STARTS_BEFORE_LOCK:
			overlap->lf_end = unlock->lf_start - 1;
			prev = &overlap->lf_next;
			lf = overlap->lf_next;
			continue;

		case OVERLAP_ENDS_AFTER_LOCK:
			overlap->lf_start = unlock->lf_end + 1;
			break;
		}
		break;
	}
#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
		lf_printlist("lf_clearlock", unlock);
	}
#endif                      
	return 0;
}


  
             
  
                                                                        
                                                         
  
                                                        
                                              
                                          
                              
                                                  
  
                       
  
                    
                                          
                                          
                         
  
                                                                       
                                                      
   
static int
lf_getlock(struct lockf *lock, struct flock *fl, pid_t matchpid)
{
	struct lockf *block;

#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
		lf_print("lf_getlock", lock);
	}
#endif                      

	if ((block = lf_getblock(lock, matchpid))) {
		fl->l_type = block->lf_type;
		fl->l_whence = SEEK_SET;
		fl->l_start = block->lf_start;
		if (block->lf_end == -1) {
			fl->l_len = 0;
		} else {
			fl->l_len = block->lf_end - block->lf_start + 1;
		}
		if (NULL != block->lf_owner) {
			  
                                             
                                             
      
			fl->l_pid = proc_pid(block->lf_owner);
		} else {
			fl->l_pid = -1;
		}
	} else {
		fl->l_type = F_UNLCK;
	}
	return 0;
}

  
              
  
                                                                        
                                                               
                                                             
                                                   
  
                                                          
                                             
                                                  
  
                                             
                                                
                    
   
static struct lockf *
lf_getblock(struct lockf *lock, pid_t matchpid)
{
	struct lockf **prev, *overlap, *lf = *(lock->lf_head);

	for (prev = lock->lf_head;
	    lf_findoverlap(lf, lock, OTHERS, &prev, &overlap) != OVERLAP_NONE;
	    lf = overlap->lf_next) {
		  
                      
    
                                                    
                                                  
                                                       
     
		if (matchpid != -1 &&
		    (overlap->lf_flags & (F_POSIX | F_OFD_LOCK)) != 0 &&
		    proc_pid(overlap->lf_owner) != matchpid) {
			continue;
		}

		  
                      
     
		if ((lock->lf_type == F_WRLCK || overlap->lf_type == F_WRLCK)) {
			return overlap;
		}
	}
	return NOLOCKF;
}


  
                 
  
                                                                            
  
                                           
                                                  
                      
                                                
                                          
                                              
                                                   
                          
  
                        
                       
                         
                             
                              
                           
  
                    
                                                  
                                             
                                           
                                              
                                                 
                                             
                                            
                                             
                                
  
                                                                    
                                                                   
                                                               
  
                                                               
                                                               
                                                                 
                                                        
  
                                                                  
                                                            
   
static overlap_t
lf_findoverlap(struct lockf *lf, struct lockf *lock, int type,
    struct lockf ***prev, struct lockf **overlap)
{
	off_t start, end;
	int found_self = 0;

	*overlap = lf;
	if (lf == NOLOCKF) {
		return 0;
	}
#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LIST)) {
		lf_print("lf_findoverlap: looking for overlap in", lock);
	}
#endif                      
	start = lock->lf_start;
	end = lock->lf_end;
	while (lf != NOLOCKF) {
		if (((type & SELF) && lf->lf_id != lock->lf_id) ||
		    ((type & OTHERS) && lf->lf_id == lock->lf_id)) {
			  
                                                        
                                                        
                                                     
                                                       
                                                        
                                                       
                              
      
			if ((type & SELF) && found_self) {
				return OVERLAP_NONE;
			}

			*prev = &lf->lf_next;
			*overlap = lf = lf->lf_next;
			continue;
		}

		if ((type & SELF)) {
			found_self = 1;
		}

#ifdef LOCKF_DEBUGGING
		if (LOCKF_DEBUGP(LF_DBG_LIST)) {
			lf_print("\tchecking", lf);
		}
#endif                      
		  
                          
     
		if ((lf->lf_end != -1 && start > lf->lf_end) ||
		    (end != -1 && lf->lf_start > end)) {
			            
			LOCKF_DEBUG(LF_DBG_LIST, "no overlap\n");

			  
                                                       
                                  
      
			if ((type & SELF) && end != -1 && lf->lf_start > end) {
				return OVERLAP_NONE;
			}
			*prev = &lf->lf_next;
			*overlap = lf = lf->lf_next;
			continue;
		}
		if ((lf->lf_start == start) && (lf->lf_end == end)) {
			LOCKF_DEBUG(LF_DBG_LIST, "overlap == lock\n");
			return OVERLAP_EQUALS_LOCK;
		}
		if ((lf->lf_start <= start) &&
		    (end != -1) &&
		    ((lf->lf_end >= end) || (lf->lf_end == -1))) {
			LOCKF_DEBUG(LF_DBG_LIST, "overlap contains lock\n");
			return OVERLAP_CONTAINS_LOCK;
		}
		if (start <= lf->lf_start &&
		    (end == -1 ||
		    (lf->lf_end != -1 && end >= lf->lf_end))) {
			LOCKF_DEBUG(LF_DBG_LIST, "lock contains overlap\n");
			return OVERLAP_CONTAINED_BY_LOCK;
		}
		if ((lf->lf_start < start) &&
		    ((lf->lf_end >= start) || (lf->lf_end == -1))) {
			LOCKF_DEBUG(LF_DBG_LIST, "overlap starts before lock\n");
			return OVERLAP_STARTS_BEFORE_LOCK;
		}
		if ((lf->lf_start > start) &&
		    (end != -1) &&
		    ((lf->lf_end > end) || (lf->lf_end == -1))) {
			LOCKF_DEBUG(LF_DBG_LIST, "overlap ends after lock\n");
			return OVERLAP_ENDS_AFTER_LOCK;
		}
		panic("lf_findoverlap: default");
	}
	return OVERLAP_NONE;
}


  
           
  
                                                                           
                 
  
                                    
                                                 
                                       
  
                       
                                   
  
                    
                                   
                                                  
                                                     
                                  
  
                                                                         
                                                                 
                                                              
   
static int
lf_split(struct lockf *lock1, struct lockf *lock2)
{
	struct lockf *splitlock;

#ifdef LOCKF_DEBUGGING
	if (LOCKF_DEBUGP(LF_DBG_LIST)) {
		lf_print("lf_split", lock1);
		lf_print("splitting from", lock2);
	}
#endif                      
	  
                                                   
    
	if (lock1->lf_start == lock2->lf_start) {
		lock1->lf_start = lock2->lf_end + 1;
		lock2->lf_next = lock1;
		return 0;
	}
	if (lock1->lf_end == lock2->lf_end) {
		lock1->lf_end = lock2->lf_start - 1;
		lock2->lf_next = lock1->lf_next;
		lock1->lf_next = lock2;
		return 0;
	}
	  
                                                  
                         
    
	splitlock = kheap_alloc(KM_LOCKF, sizeof(struct lockf), Z_WAITOK);
	if (splitlock == NULL) {
		return ENOLCK;
	}
	bcopy(lock1, splitlock, sizeof *splitlock);
	splitlock->lf_start = lock2->lf_end + 1;
	TAILQ_INIT(&splitlock->lf_blkhd);
	lock1->lf_end = lock2->lf_start - 1;
	  
                      
    
	splitlock->lf_next = lock1->lf_next;
	lock2->lf_next = splitlock;
	lock1->lf_next = lock2;

	return 0;
}


  
              
  
                                                                        
                                                     
  
                                                            
                         
  
                  
  
                                                                       
                                                                
                                                              
                                                                 
                                                                   
                                        
   
static void
lf_wakelock(struct lockf *listhead, boolean_t force_all)
{
	struct lockf *wakelock;
	boolean_t wake_all = TRUE;

	if (force_all == FALSE && (listhead->lf_flags & F_WAKE1_SAFE)) {
		wake_all = FALSE;
	}

	while (!TAILQ_EMPTY(&listhead->lf_blkhd)) {
		wakelock = TAILQ_FIRST(&listhead->lf_blkhd);
		TAILQ_REMOVE(&listhead->lf_blkhd, wakelock, lf_block);

		wakelock->lf_next = NOLOCKF;
#ifdef LOCKF_DEBUGGING
		if (LOCKF_DEBUGP(LF_DBG_LOCKOP)) {
			lf_print("lf_wakelock: awakening", wakelock);
		}
#endif                      
		if (wake_all == FALSE) {
			  
                                                     
                                                      
                                     
      
			if (!TAILQ_EMPTY(&listhead->lf_blkhd)) {
				TAILQ_CONCAT(&wakelock->lf_blkhd, &listhead->lf_blkhd, lf_block);

				struct lockf *tlock;

				TAILQ_FOREACH(tlock, &wakelock->lf_blkhd, lf_block) {
					if (TAILQ_NEXT(tlock, lf_block) == tlock) {
						                         
						panic("cycle in wakelock list");
					}
					tlock->lf_next = wakelock;
				}
			}
		}
		wakeup(wakelock);

		if (wake_all == FALSE) {
			break;
		}
	}
}


#ifdef LOCKF_DEBUGGING
#define GET_LF_OWNER_PID(lf)    (proc_pid((lf)->lf_owner))

  
                 
  
                                                                        
  
                                               
                                               
                
  
                  
   
void
lf_print(const char *tag, struct lockf *lock)
{
	printf("%s: lock %p for ", tag, (void *)lock);
	if (lock->lf_flags & F_POSIX) {
		printf("proc %p (owner %d)",
		    lock->lf_id, GET_LF_OWNER_PID(lock));
	} else if (lock->lf_flags & F_OFD_LOCK) {
		printf("fg %p (owner %d)",
		    lock->lf_id, GET_LF_OWNER_PID(lock));
	} else {
		printf("id %p", (void *)lock->lf_id);
	}
	if (lock->lf_vnode != 0) {
		printf(" in vno %p, %s, start 0x%016llx, end 0x%016llx",
		    lock->lf_vnode,
		    lock->lf_type == F_RDLCK ? "shared" :
		    lock->lf_type == F_WRLCK ? "exclusive" :
		    lock->lf_type == F_UNLCK ? "unlock" : "unknown",
		    (uint64_t)lock->lf_start, (uint64_t)lock->lf_end);
	} else {
		printf(" %s, start 0x%016llx, end 0x%016llx",
		    lock->lf_type == F_RDLCK ? "shared" :
		    lock->lf_type == F_WRLCK ? "exclusive" :
		    lock->lf_type == F_UNLCK ? "unlock" : "unknown",
		    (uint64_t)lock->lf_start, (uint64_t)lock->lf_end);
	}
	if (!TAILQ_EMPTY(&lock->lf_blkhd)) {
		printf(" block %p\n", (void *)TAILQ_FIRST(&lock->lf_blkhd));
	} else {
		printf("\n");
	}
}


  
                     
  
                                                                               
                                     
  
                                               
                                                  
                   
  
                  
   
void
lf_printlist(const char *tag, struct lockf *lock)
{
	struct lockf *lf, *blk;

	if (lock->lf_vnode == 0) {
		return;
	}

	printf("%s: Lock list for vno %p:\n",
	    tag, lock->lf_vnode);
	for (lf = lock->lf_vnode->v_lockf; lf; lf = lf->lf_next) {
		printf("\tlock %p for ", (void *)lf);
		if (lf->lf_flags & F_POSIX) {
			printf("proc %p (owner %d)",
			    lf->lf_id, GET_LF_OWNER_PID(lf));
		} else if (lf->lf_flags & F_OFD_LOCK) {
			printf("fg %p (owner %d)",
			    lf->lf_id, GET_LF_OWNER_PID(lf));
		} else {
			printf("id %p", (void *)lf->lf_id);
		}
		printf(", %s, start 0x%016llx, end 0x%016llx",
		    lf->lf_type == F_RDLCK ? "shared" :
		    lf->lf_type == F_WRLCK ? "exclusive" :
		    lf->lf_type == F_UNLCK ? "unlock" :
		    "unknown", (uint64_t)lf->lf_start, (uint64_t)lf->lf_end);
		TAILQ_FOREACH(blk, &lf->lf_blkhd, lf_block) {
			printf("\n\t\tlock request %p for ", (void *)blk);
			if (blk->lf_flags & F_POSIX) {
				printf("proc %p (owner %d)",
				    blk->lf_id, GET_LF_OWNER_PID(blk));
			} else if (blk->lf_flags & F_OFD_LOCK) {
				printf("fg %p (owner %d)",
				    blk->lf_id, GET_LF_OWNER_PID(blk));
			} else {
				printf("id %p", (void *)blk->lf_id);
			}
			printf(", %s, start 0x%016llx, end 0x%016llx",
			    blk->lf_type == F_RDLCK ? "shared" :
			    blk->lf_type == F_WRLCK ? "exclusive" :
			    blk->lf_type == F_UNLCK ? "unlock" :
			    "unknown", (uint64_t)blk->lf_start,
			    (uint64_t)blk->lf_end);
			if (!TAILQ_EMPTY(&blk->lf_blkhd)) {
				panic("lf_printlist: bad list");
			}
		}
		printf("\n");
	}
}
#endif                      

#if IMPORTANCE_INHERITANCE

  
                    
  
                                                                
  
                                                                  
                                                       
  
                                                                        
                                                       
  
                     
  
                                                                         
                                                                     
                                                                      
                              
   
static void
lf_hold_assertion(task_t block_task, struct lockf *block)
{
	if (task_importance_hold_file_lock_assertion(block_task, 1) == 0) {
		block->lf_boosted = LF_BOOSTED;
		LOCKF_DEBUG(LF_DBG_IMPINH,
		    "lf: importance hold file lock assert on pid %d lock %p\n",
		    proc_pid(block->lf_owner), block);
	}
}


  
                        
  
                                                                
             
  
                                                                   
                                                    
                                                                        
                                              
  
                     
   
static void
lf_jump_to_queue_head(struct lockf *block, struct lockf *lock)
{
	                                                   
	TAILQ_REMOVE(&block->lf_blkhd, lock, lf_block);
	TAILQ_INSERT_HEAD(&block->lf_blkhd, lock, lf_block);
}


  
                    
  
                                 
  
                                                                           
  
                     
   
static void
lf_drop_assertion(struct lockf *block)
{
	LOCKF_DEBUG(LF_DBG_IMPINH, "lf: %d: dropping assertion for lock %p\n",
	    proc_pid(block->lf_owner), block);

	task_t current_task = proc_task(block->lf_owner);
	task_importance_drop_file_lock_assertion(current_task, 1);
	block->lf_boosted = LF_NOT_BOOSTED;
}

  
                      
  
                                                          
                                                           
                         
  
                                                                
  
                  
   
static void
lf_adjust_assertion(struct lockf *block)
{
	boolean_t drop_boost = TRUE;
	struct lockf *next;

	                                       
	if (block->lf_boosted == LF_NOT_BOOSTED) {
		return;
	}

	TAILQ_FOREACH(next, &block->lf_blkhd, lf_block) {
		                                                    
		if (((block->lf_flags & next->lf_flags & F_POSIX) != 0) ||
		    ((block->lf_flags & next->lf_flags & F_OFD_LOCK) &&
		    (block->lf_owner != next->lf_owner) &&
		    (NULL != block->lf_owner && NULL != next->lf_owner))) {
			                                           
			if (task_is_importance_donor(proc_task(next->lf_owner)) &&
			    task_is_importance_receiver_type(proc_task(block->lf_owner))) {
				                                 
				drop_boost = FALSE;
				break;
			}
		}
	}

	if (drop_boost) {
		lf_drop_assertion(block);
	}
}

static void
lf_boost_blocking_proc(struct lockf *lock, struct lockf *block)
{
	task_t ltask = proc_task(lock->lf_owner);
	task_t btask = proc_task(block->lf_owner);

	  
                                             
                                                  
                                                     
                                                      
                   
   
                                                 
    
	if (task_is_importance_donor(ltask)) {
		LOCKF_DEBUG(LF_DBG_IMPINH,
		    "lf: %d: attempt to boost pid %d that holds lock %p\n",
		    proc_pid(lock->lf_owner), proc_pid(block->lf_owner), block);

		if (block->lf_boosted != LF_BOOSTED &&
		    task_is_importance_receiver_type(btask)) {
			lf_hold_assertion(btask, block);
		}
		lf_jump_to_queue_head(block, lock);
	}
}
#endif                             
