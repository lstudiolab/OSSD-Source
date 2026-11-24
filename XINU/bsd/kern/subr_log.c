/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)subr_log.c	8.3 (Berkeley) 2/14/95
 */

  
                                        
   

#include <machine/atomic.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/vnode.h>
#include <stdbool.h>
#include <firehose/tracepoint_private.h>
#include <firehose/chunk_private.h>
#include <firehose/ioctl_private.h>
#include <os/firehose_buffer_private.h>

#include <os/log_private.h>
#include <sys/ioctl.h>
#include <sys/msgbuf.h>
#include <sys/file_internal.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/kernel.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/simple_lock.h>
#include <sys/lock.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/queue.h>
#include <kern/kalloc.h>
#include <pexpert/pexpert.h>
#include <mach/mach_port.h>
#include <mach/mach_vm.h>
#include <mach/vm_map.h>
#include <vm/vm_kern.h>
#include <kern/task.h>
#include <kern/locks.h>

                                                
extern void logwakeup(struct msgbuf *);
extern void oslogwakeup(void);
extern void oslog_streamwakeup(void);
static void oslog_streamwakeup_locked(void);

SECURITY_READ_ONLY_LATE(vm_offset_t) kernel_firehose_addr = 0;
SECURITY_READ_ONLY_LATE(uint8_t) __firehose_buffer_kernel_chunk_count =
    FIREHOSE_BUFFER_KERNEL_DEFAULT_CHUNK_COUNT;
SECURITY_READ_ONLY_LATE(uint8_t) __firehose_num_kernel_io_pages =
    FIREHOSE_BUFFER_KERNEL_DEFAULT_IO_PAGES;

                                             
uint32_t oslog_s_streamed_msgcount = 0;
uint32_t oslog_s_dropped_msgcount  = 0;
extern uint32_t oslog_s_error_count;

uint32_t oslog_msgbuf_dropped_charcount = 0;

#define LOG_RDPRI       (PZERO + 1)

#define LOG_NBIO        0x02
#define LOG_ASYNC       0x04
#define LOG_RDWAIT      0x08

                                                                                

static char amsg_bufc[1024];
static struct msgbuf aslbuf = {.msg_magic = MSG_MAGIC, .msg_size = sizeof(amsg_bufc), .msg_bufx = 0, .msg_bufr = 0, .msg_bufc = amsg_bufc};
struct msgbuf *aslbufp __attribute__((used)) = &aslbuf;

                                          
struct logsoftc {
	int     sc_state;                                                
	struct  selinfo sc_selp;                                       
	int     sc_pgid;                                                 
	struct msgbuf *sc_mbp;
} logsoftc;

static int log_open;
char smsg_bufc[CONFIG_MSG_BSIZE];                    
char oslog_stream_bufc[FIREHOSE_CHUNK_SIZE];                    
struct firehose_chunk_s oslog_boot_buf = {
	.fc_pos = {
		.fcp_next_entry_offs = offsetof(struct firehose_chunk_s, fc_data),
		.fcp_private_offs = FIREHOSE_CHUNK_SIZE,
		.fcp_refcnt = 1,                                                 
		.fcp_stream = firehose_stream_persist,
		.fcp_flag_io = 1,                                                        
	},
};                    
firehose_chunk_t firehose_boot_chunk = &oslog_boot_buf;
struct msgbuf msgbuf = {.msg_magic  = MSG_MAGIC, .msg_size = sizeof(smsg_bufc), .msg_bufx = 0, .msg_bufr = 0, .msg_bufc = smsg_bufc};
struct msgbuf oslog_stream_buf = {.msg_magic = MSG_MAGIC, .msg_size = 0, .msg_bufx = 0, .msg_bufr = 0, .msg_bufc = NULL};
struct msgbuf *msgbufp __attribute__((used)) = &msgbuf;
struct msgbuf *oslog_streambufp __attribute__((used)) = &oslog_stream_buf;

                                                         
static oslog_stream_buf_entry_t oslog_stream_buf_entries;

#define OSLOG_NUM_STREAM_ENTRIES        64
#define OSLOG_STREAM_BUF_SIZE           4096

int oslog_open = 0;
int os_log_wakeup = 0;
int oslog_stream_open = 0;
int oslog_stream_buf_bytesavail = 0;
int oslog_stream_buf_size = OSLOG_STREAM_BUF_SIZE;
int oslog_stream_num_entries = OSLOG_NUM_STREAM_ENTRIES;

                                              
struct oslogsoftc {
	int     sc_state;                                                
	struct  selinfo sc_selp;                                       
	int     sc_pgid;                                                 
} oslogsoftc;

struct oslog_streamsoftc {
	int     sc_state;                                                
	struct  selinfo sc_selp;                                       
	int     sc_pgid;                                                 
} oslog_streamsoftc;

STAILQ_HEAD(, oslog_stream_buf_entry_s) oslog_stream_free_head =
    STAILQ_HEAD_INITIALIZER(oslog_stream_free_head);
STAILQ_HEAD(, oslog_stream_buf_entry_s) oslog_stream_buf_head =
    STAILQ_HEAD_INITIALIZER(oslog_stream_buf_head);

                                     
extern bool bsd_log_lock(bool);
extern void bsd_log_lock_safe(void);
extern void bsd_log_unlock(void);

LCK_GRP_DECLARE(oslog_stream_lock_grp, "oslog streaming");
LCK_SPIN_DECLARE(oslog_stream_lock, &oslog_stream_lock_grp);
#define stream_lock() lck_spin_lock(&oslog_stream_lock)
#define stream_unlock() lck_spin_unlock(&oslog_stream_lock)

                                                   
extern d_open_t         logopen;
extern d_close_t        logclose;
extern d_read_t         logread;
extern d_ioctl_t        logioctl;
extern d_select_t       logselect;

                                                   
extern d_open_t         oslogopen;
extern d_close_t        oslogclose;
extern d_select_t       oslogselect;
extern d_ioctl_t        oslogioctl;

                                                   
extern d_open_t         oslog_streamopen;
extern d_close_t        oslog_streamclose;
extern d_read_t         oslog_streamread;
extern d_ioctl_t        oslog_streamioctl;
extern d_select_t       oslog_streamselect;

void oslog_setsize(int size);
void oslog_streamwrite_locked(firehose_tracepoint_id_u ftid,
    uint64_t stamp, const void *pubdata, size_t publen);
void oslog_streamwrite_metadata_locked(oslog_stream_buf_entry_t m_entry);
static oslog_stream_buf_entry_t oslog_stream_find_free_buf_entry_locked(void);
static void oslog_streamwrite_append_bytes(const char *buffer, int buflen);

  
                                                                              
                                                                               
                                                       
   

#if DEBUG
#define LOG_SETSIZE_DEBUG(x...) kprintf(x)
#else
#define LOG_SETSIZE_DEBUG(x...) do { } while(0)
#endif

static int sysctl_kern_msgbuf(struct sysctl_oid *oidp,
    void *arg1, int arg2, struct sysctl_req *req);

            
int
logopen(__unused dev_t dev, __unused int flags, __unused int mode, struct proc *p)
{
	bsd_log_lock_safe();
	if (log_open) {
		bsd_log_unlock();
		return EBUSY;
	}
	if (atm_get_diagnostic_config() & ATM_ENABLE_LEGACY_LOGGING) {
		logsoftc.sc_mbp = msgbufp;
	} else {
		  
                                               
                                                                
                                        
     
		logsoftc.sc_mbp = aslbufp;
	}
	logsoftc.sc_pgid = p->p_pid;                                     
	log_open = 1;

	bsd_log_unlock();

	return 0;
}

            
int
logclose(__unused dev_t dev, __unused int flag, __unused int devtype, __unused struct proc *p)
{
	bsd_log_lock_safe();
	logsoftc.sc_state &= ~(LOG_NBIO | LOG_ASYNC);
	selwakeup(&logsoftc.sc_selp);
	selthreadclear(&logsoftc.sc_selp);
	log_open = 0;
	bsd_log_unlock();
	return 0;
}


int
oslogopen(__unused dev_t dev, __unused int flags, __unused int mode, struct proc *p)
{
	bsd_log_lock_safe();
	if (oslog_open) {
		bsd_log_unlock();
		return EBUSY;
	}
	oslogsoftc.sc_pgid = p->p_pid;                                   
	oslog_open = 1;

	bsd_log_unlock();
	return 0;
}

int
oslogclose(__unused dev_t dev, __unused int flag, __unused int devtype, __unused struct proc *p)
{
	bsd_log_lock_safe();
	oslogsoftc.sc_state &= ~(LOG_NBIO | LOG_ASYNC);
	selwakeup(&oslogsoftc.sc_selp);
	selthreadclear(&oslogsoftc.sc_selp);
	oslog_open = 0;
	bsd_log_unlock();
	return 0;
}

int
oslog_streamopen(__unused dev_t dev, __unused int flags, __unused int mode, struct proc *p)
{
	char *oslog_stream_msg_bufc = NULL;
	oslog_stream_buf_entry_t entries = NULL;

	stream_lock();
	if (oslog_stream_open) {
		stream_unlock();
		return EBUSY;
	}
	stream_unlock();

	                             
	oslog_stream_msg_bufc = kheap_alloc(KHEAP_DATA_BUFFERS,
	    oslog_stream_buf_size, Z_WAITOK | Z_ZERO);
	if (!oslog_stream_msg_bufc) {
		return ENOMEM;
	}

	                                                      
	size_t entries_size = oslog_stream_num_entries * sizeof(struct oslog_stream_buf_entry_s);
	entries = kalloc(entries_size);
	if (!entries) {
		kheap_free(KHEAP_DATA_BUFFERS,
		    oslog_stream_msg_bufc, oslog_stream_buf_size);
		return ENOMEM;
	}
	                                                                         
	bzero(entries, entries_size);

	stream_lock();
	if (oslog_stream_open) {
		stream_unlock();
		kheap_free(KHEAP_DATA_BUFFERS,
		    oslog_stream_msg_bufc, oslog_stream_buf_size);
		kfree(entries, entries_size);
		return EBUSY;
	}

	assert(oslog_streambufp->msg_bufc == NULL);
	oslog_streambufp->msg_bufc = oslog_stream_msg_bufc;
	oslog_streambufp->msg_size = oslog_stream_buf_size;

	oslog_stream_buf_entries = entries;

	STAILQ_INIT(&oslog_stream_free_head);
	STAILQ_INIT(&oslog_stream_buf_head);

	for (int i = 0; i < oslog_stream_num_entries; i++) {
		oslog_stream_buf_entries[i].type = oslog_stream_link_type_log;
		STAILQ_INSERT_TAIL(&oslog_stream_free_head, &oslog_stream_buf_entries[i], buf_entries);
	}

	                                                      
	assert(STAILQ_EMPTY(&oslog_stream_buf_head));
	assert(oslog_streambufp->msg_bufx == 0);
	assert(oslog_streambufp->msg_bufr == 0);

	oslog_streambufp->msg_bufx = 0;
	oslog_streambufp->msg_bufr = 0;
	oslog_streamsoftc.sc_pgid = p->p_pid;                          
	oslog_stream_open = 1;
	oslog_stream_buf_bytesavail = oslog_stream_buf_size;
	stream_unlock();

	return 0;
}

int
oslog_streamclose(__unused dev_t dev, __unused int flag, __unused int devtype, __unused struct proc *p)
{
	oslog_stream_buf_entry_t next_entry = NULL;
	char *oslog_stream_msg_bufc = NULL;
	oslog_stream_buf_entry_t entries = NULL;

	stream_lock();

	if (oslog_stream_open == 0) {
		stream_unlock();
		return EBADF;
	}

	                        
	while (!STAILQ_EMPTY(&oslog_stream_buf_head)) {
		next_entry = STAILQ_FIRST(&oslog_stream_buf_head);
		STAILQ_REMOVE_HEAD(&oslog_stream_buf_head, buf_entries);
	}
	oslog_streamwakeup_locked();
	oslog_streamsoftc.sc_state &= ~(LOG_NBIO | LOG_ASYNC);
	selwakeup(&oslog_streamsoftc.sc_selp);
	selthreadclear(&oslog_streamsoftc.sc_selp);
	oslog_stream_open = 0;
	oslog_streambufp->msg_bufr = 0;
	oslog_streambufp->msg_bufx = 0;
	oslog_stream_msg_bufc = oslog_streambufp->msg_bufc;
	oslog_streambufp->msg_bufc = NULL;
	entries = oslog_stream_buf_entries;
	oslog_stream_buf_entries = NULL;
	oslog_streambufp->msg_size = 0;

	stream_unlock();

	                         
	kheap_free(KHEAP_DATA_BUFFERS, oslog_stream_msg_bufc,
	    oslog_stream_buf_size);
	                        
	kfree(entries, oslog_stream_num_entries * sizeof(struct oslog_stream_buf_entry_s));

	return 0;
}

            
int
logread(__unused dev_t dev, struct uio *uio, int flag)
{
	int error = 0;
	struct msgbuf *mbp = logsoftc.sc_mbp;
	ssize_t resid;

	bsd_log_lock_safe();
	while (mbp->msg_bufr == mbp->msg_bufx) {
		if (flag & IO_NDELAY) {
			error = EWOULDBLOCK;
			goto out;
		}
		if (logsoftc.sc_state & LOG_NBIO) {
			error = EWOULDBLOCK;
			goto out;
		}
		logsoftc.sc_state |= LOG_RDWAIT;
		bsd_log_unlock();
		  
                            
                                       
     
		if ((error = tsleep((caddr_t)mbp, LOG_RDPRI | PCATCH,
		    "klog", 5 * hz)) != 0) {
			                             
			if (error != EWOULDBLOCK) {
				return error;
			}
		}
		bsd_log_lock_safe();
	}
	logsoftc.sc_state &= ~LOG_RDWAIT;

	while ((resid = uio_resid(uio)) > 0) {
		size_t l;

		if (mbp->msg_bufx >= mbp->msg_bufr) {
			l = mbp->msg_bufx - mbp->msg_bufr;
		} else {
			l = mbp->msg_size - mbp->msg_bufr;
		}
		if ((l = MIN(l, (size_t)resid)) == 0) {
			break;
		}

		const size_t readpos = mbp->msg_bufr;

		bsd_log_unlock();
		error = uiomove((caddr_t)&mbp->msg_bufc[readpos], (int)l, uio);
		bsd_log_lock_safe();
		if (error) {
			break;
		}

		mbp->msg_bufr = (int)(readpos + l);
		if (mbp->msg_bufr >= mbp->msg_size) {
			mbp->msg_bufr = 0;
		}
	}
out:
	bsd_log_unlock();
	return error;
}

            
int
oslog_streamread(__unused dev_t dev, struct uio *uio, int flag)
{
	int error = 0;
	int copy_size = 0;
	static char logline[FIREHOSE_CHUNK_SIZE];

	stream_lock();

	if (!oslog_stream_open) {
		stream_unlock();
		return EBADF;
	}

	while (STAILQ_EMPTY(&oslog_stream_buf_head)) {
		assert(oslog_stream_buf_bytesavail == oslog_stream_buf_size);

		if (flag & IO_NDELAY || oslog_streamsoftc.sc_state & LOG_NBIO) {
			stream_unlock();
			return EWOULDBLOCK;
		}

		oslog_streamsoftc.sc_state |= LOG_RDWAIT;
		wait_result_t wr = assert_wait((event_t)oslog_streambufp,
		    THREAD_INTERRUPTIBLE);
		if (wr == THREAD_WAITING) {
			stream_unlock();
			wr = thread_block(THREAD_CONTINUE_NULL);
			stream_lock();
		}

		switch (wr) {
		case THREAD_AWAKENED:
		case THREAD_TIMED_OUT:
			break;
		default:
			stream_unlock();
			return EINTR;
		}
	}

	if (!oslog_stream_open) {
		stream_unlock();
		return EBADF;
	}

	int logpos = 0;
	oslog_stream_buf_entry_t read_entry = NULL;
	uint16_t rec_length;

	read_entry = STAILQ_FIRST(&oslog_stream_buf_head);
	assert(read_entry != NULL);
	STAILQ_REMOVE_HEAD(&oslog_stream_buf_head, buf_entries);

	                           
	memcpy(logline + logpos, &read_entry->timestamp, sizeof(uint64_t));
	logpos += sizeof(uint64_t);

	switch (read_entry->type) {
	                              
	case oslog_stream_link_type_metadata:
	{
		memcpy(logline + logpos,
		    (read_entry->metadata), read_entry->size);
		logpos += read_entry->size;

		stream_unlock();

		                      
		kfree(read_entry, sizeof(struct oslog_stream_buf_entry_s) + read_entry->size);
		break;
	}
	                         
	case oslog_stream_link_type_log:
	{
		                                                     
		assert(read_entry->offset == oslog_streambufp->msg_bufr);
		rec_length = read_entry->size;

		                                                                 
		if (read_entry->offset + rec_length <= oslog_streambufp->msg_size) {
			memcpy(logline + logpos,
			    oslog_streambufp->msg_bufc + read_entry->offset, rec_length);

			oslog_streambufp->msg_bufr += rec_length;
			if (oslog_streambufp->msg_bufr == oslog_streambufp->msg_size) {
				oslog_streambufp->msg_bufr = 0;
			}
			logpos += rec_length;
		} else {
			                                                   
			                                                
			int bytes_left = oslog_streambufp->msg_size - read_entry->offset;
			memcpy(logline + logpos,
			    oslog_streambufp->msg_bufc + read_entry->offset, bytes_left);
			logpos += bytes_left;
			rec_length -= bytes_left;

			memcpy(logline + logpos, (const void *)oslog_streambufp->msg_bufc,
			    rec_length);
			oslog_streambufp->msg_bufr = rec_length;
			logpos += rec_length;
		}

		oslog_stream_buf_bytesavail += read_entry->size;
		assert(oslog_stream_buf_bytesavail <= oslog_stream_buf_size);

		assert(oslog_streambufp->msg_bufr < oslog_streambufp->msg_size);
		STAILQ_INSERT_TAIL(&oslog_stream_free_head, read_entry, buf_entries);

		stream_unlock();
		break;
	}
	default:
	{
		panic("Got unexpected log entry type: %hhu\n", read_entry->type);
	}
	}

	copy_size = min(logpos, (int) MIN(uio_resid(uio), INT_MAX));
	if (copy_size > 0) {
		error = uiomove((caddr_t)logline, copy_size, uio);
	}
	os_atomic_inc(&oslog_s_streamed_msgcount, relaxed);

	return error;
}

            
int
logselect(__unused dev_t dev, int rw, void * wql, struct proc *p)
{
	const struct msgbuf *mbp = logsoftc.sc_mbp;

	switch (rw) {
	case FREAD:
		bsd_log_lock_safe();
		if (mbp->msg_bufr != mbp->msg_bufx) {
			bsd_log_unlock();
			return 1;
		}
		selrecord(p, &logsoftc.sc_selp, wql);
		bsd_log_unlock();
		break;
	}
	return 0;
}

int
oslogselect(__unused dev_t dev, int rw, void * wql, struct proc *p)
{
	switch (rw) {
	case FREAD:
		bsd_log_lock_safe();
		if (os_log_wakeup) {
			bsd_log_unlock();
			return 1;
		}
		selrecord(p, &oslogsoftc.sc_selp, wql);
		bsd_log_unlock();
		break;
	}
	return 0;
}

int
oslog_streamselect(__unused dev_t dev, int rw, void * wql, struct proc *p)
{
	int ret = 0;

	stream_lock();

	switch (rw) {
	case FREAD:
		if (STAILQ_EMPTY(&oslog_stream_buf_head)) {
			selrecord(p, &oslog_streamsoftc.sc_selp, wql);
		} else {
			ret = 1;
		}
		break;
	}

	stream_unlock();
	return ret;
}

void
logwakeup(struct msgbuf *mbp)
{
	                              
	if (oslog_is_safe() == FALSE) {
		return;
	}

	bsd_log_lock_safe();
	if (!log_open) {
		bsd_log_unlock();
		return;
	}
	if (NULL == mbp) {
		mbp = logsoftc.sc_mbp;
	}
	if (mbp != logsoftc.sc_mbp) {
		goto out;
	}
	selwakeup(&logsoftc.sc_selp);
	if (logsoftc.sc_state & LOG_ASYNC) {
		int pgid = logsoftc.sc_pgid;
		bsd_log_unlock();
		if (pgid < 0) {
			gsignal(-pgid, SIGIO);
		} else {
			proc_signal(pgid, SIGIO);
		}
		bsd_log_lock_safe();
	}
	if (logsoftc.sc_state & LOG_RDWAIT) {
		wakeup((caddr_t)mbp);
		logsoftc.sc_state &= ~LOG_RDWAIT;
	}
out:
	bsd_log_unlock();
}

void
oslogwakeup(void)
{
	if (!oslog_is_safe()) {
		return;
	}

	bsd_log_lock_safe();
	if (!oslog_open) {
		bsd_log_unlock();
		return;
	}
	selwakeup(&oslogsoftc.sc_selp);
	os_log_wakeup = 1;
	bsd_log_unlock();
}

static void
oslog_streamwakeup_locked(void)
{
	LCK_SPIN_ASSERT(&oslog_stream_lock, LCK_ASSERT_OWNED);
	if (!oslog_stream_open) {
		return;
	}
	selwakeup(&oslog_streamsoftc.sc_selp);
	if (oslog_streamsoftc.sc_state & LOG_RDWAIT) {
		wakeup((caddr_t)oslog_streambufp);
		oslog_streamsoftc.sc_state &= ~LOG_RDWAIT;
	}
}

void
oslog_streamwakeup(void)
{
	                              
	if (oslog_is_safe() == FALSE) {
		return;
	}

	stream_lock();
	oslog_streamwakeup_locked();
	stream_unlock();
}

            
int
logioctl(__unused dev_t dev, u_long com, caddr_t data, __unused int flag, __unused struct proc *p)
{
	int l;
	const struct msgbuf *mbp = logsoftc.sc_mbp;

	bsd_log_lock_safe();
	switch (com) {
	                                                       
	case FIONREAD:
		l = mbp->msg_bufx - mbp->msg_bufr;
		if (l < 0) {
			l += mbp->msg_size;
		}
		*(off_t *)data = l;
		break;

	case FIONBIO:
		if (*(int *)data) {
			logsoftc.sc_state |= LOG_NBIO;
		} else {
			logsoftc.sc_state &= ~LOG_NBIO;
		}
		break;

	case FIOASYNC:
		if (*(int *)data) {
			logsoftc.sc_state |= LOG_ASYNC;
		} else {
			logsoftc.sc_state &= ~LOG_ASYNC;
		}
		break;

	case TIOCSPGRP:
		logsoftc.sc_pgid = *(int *)data;
		break;

	case TIOCGPGRP:
		*(int *)data = logsoftc.sc_pgid;
		break;

	default:
		bsd_log_unlock();
		return -1;
	}
	bsd_log_unlock();
	return 0;
}

            
int
oslogioctl(__unused dev_t dev, u_long com, caddr_t data, __unused int flag, __unused struct proc *p)
{
	int ret = 0;
	mach_vm_size_t buffer_size = (__firehose_buffer_kernel_chunk_count * FIREHOSE_CHUNK_SIZE);
	firehose_buffer_map_info_t map_info = {0, 0};
	firehose_buffer_t kernel_firehose_buffer = NULL;
	mach_vm_address_t user_addr = 0;
	mach_port_t mem_entry_ptr = MACH_PORT_NULL;

	switch (com) {
	                                                       

	case LOGBUFFERMAP:
		kernel_firehose_buffer = (firehose_buffer_t)kernel_firehose_addr;

		ret = mach_make_memory_entry_64(kernel_map,
		    &buffer_size,
		    (mach_vm_offset_t) kernel_firehose_buffer,
		    (MAP_MEM_VM_SHARE | VM_PROT_READ),
		    &mem_entry_ptr,
		    MACH_PORT_NULL);
		if (ret == KERN_SUCCESS) {
			ret = mach_vm_map_kernel(get_task_map(current_task()),
			    &user_addr,
			    buffer_size,
			    0,                          
			    VM_FLAGS_ANYWHERE,
			    VM_MAP_KERNEL_FLAGS_NONE,
			    VM_KERN_MEMORY_NONE,
			    mem_entry_ptr,
			    0,                           
			    FALSE,                         
			    VM_PROT_READ,
			    VM_PROT_READ,
			    VM_INHERIT_SHARE);
		}

		if (ret == KERN_SUCCESS) {
			map_info.fbmi_addr = (uint64_t) (user_addr);
			map_info.fbmi_size = buffer_size;
			bcopy(&map_info, data, sizeof(firehose_buffer_map_info_t));
		}
		break;
	case LOGFLUSHED:
		bsd_log_lock_safe();
		os_log_wakeup = 0;
		bsd_log_unlock();
		__firehose_merge_updates(*(firehose_push_reply_t *)(data));
		break;
	default:
		return -1;
	}
	return 0;
}

            
int
oslog_streamioctl(__unused dev_t dev, u_long com, caddr_t data, __unused int flag, __unused struct proc *p)
{
	int err = 0;

	stream_lock();

	switch (com) {
	case FIONBIO:
		if (data && *(int *)data) {
			oslog_streamsoftc.sc_state |= LOG_NBIO;
		} else {
			oslog_streamsoftc.sc_state &= ~LOG_NBIO;
		}
		break;
	case FIOASYNC:
		if (data && *(int *)data) {
			oslog_streamsoftc.sc_state |= LOG_ASYNC;
		} else {
			oslog_streamsoftc.sc_state &= ~LOG_ASYNC;
		}
		break;
	default:
		err = -1;
		break;
	}

	stream_unlock();
	return err;
}

__startup_func
static void
oslog_init(void)
{
	kern_return_t kr;
	if (!PE_parse_boot_argn("firehose_chunk_count", &__firehose_buffer_kernel_chunk_count, sizeof(__firehose_buffer_kernel_chunk_count))) {
		__firehose_buffer_kernel_chunk_count = FIREHOSE_BUFFER_KERNEL_DEFAULT_CHUNK_COUNT;
	}
	if (!PE_parse_boot_argn("firehose_io_pages", &__firehose_num_kernel_io_pages, sizeof(__firehose_num_kernel_io_pages))) {
		__firehose_num_kernel_io_pages = FIREHOSE_BUFFER_KERNEL_DEFAULT_IO_PAGES;
	}
	if (!__firehose_kernel_configuration_valid(__firehose_buffer_kernel_chunk_count, __firehose_num_kernel_io_pages)) {
		printf("illegal firehose configuration %u/%u, using defaults\n", __firehose_buffer_kernel_chunk_count, __firehose_num_kernel_io_pages);
		__firehose_buffer_kernel_chunk_count = FIREHOSE_BUFFER_KERNEL_DEFAULT_CHUNK_COUNT;
		__firehose_num_kernel_io_pages = FIREHOSE_BUFFER_KERNEL_DEFAULT_IO_PAGES;
	}
	vm_size_t size = __firehose_buffer_kernel_chunk_count * FIREHOSE_CHUNK_SIZE;

	kr = kmem_alloc_flags(kernel_map, &kernel_firehose_addr,
	    size + (2 * PAGE_SIZE), VM_KERN_MEMORY_LOG,
	    KMA_GUARD_FIRST | KMA_GUARD_LAST | KMA_ZERO);
	if (kr != KERN_SUCCESS) {
		panic("Failed to allocate memory for firehose logging buffer");
	}
	kernel_firehose_addr += PAGE_SIZE;
	                                   
	kernel_firehose_addr = (vm_offset_t)__firehose_buffer_create((size_t *) &size);

	printf("oslog_init completed, %u chunks, %u io pages\n",
	    __firehose_buffer_kernel_chunk_count, __firehose_num_kernel_io_pages);
}
STARTUP(OSLOG, STARTUP_RANK_FIRST, oslog_init);

  
                  
  
                                                                           
                                                          
  
                                       
  
                  
  
                                                                    
                                                                
                                                               
                   
   
void
log_putc_locked(struct msgbuf *mbp, char c)
{
	mbp->msg_bufc[mbp->msg_bufx++] = c;
	if (mbp->msg_bufx >= mbp->msg_size) {
		mbp->msg_bufx = 0;
	}
}

static oslog_stream_buf_entry_t
oslog_stream_find_free_buf_entry_locked(void)
{
	struct msgbuf *mbp;
	oslog_stream_buf_entry_t buf_entry = NULL;

	LCK_SPIN_ASSERT(&oslog_stream_lock, LCK_ASSERT_OWNED);

	mbp = oslog_streambufp;

	buf_entry = STAILQ_FIRST(&oslog_stream_free_head);
	if (buf_entry) {
		STAILQ_REMOVE_HEAD(&oslog_stream_free_head, buf_entries);
	} else {
		                                                      
		                                                               
		oslog_stream_buf_entry_t prev_entry = NULL;

		buf_entry = STAILQ_FIRST(&oslog_stream_buf_head);
		while (buf_entry->type == oslog_stream_link_type_metadata) {
			prev_entry = buf_entry;
			buf_entry = STAILQ_NEXT(buf_entry, buf_entries);
		}

		if (prev_entry == NULL) {
			STAILQ_REMOVE_HEAD(&oslog_stream_buf_head, buf_entries);
		} else {
			STAILQ_REMOVE_AFTER(&oslog_stream_buf_head, prev_entry, buf_entries);
		}

		mbp->msg_bufr += buf_entry->size;
		oslog_s_dropped_msgcount++;
		if (mbp->msg_bufr >= mbp->msg_size) {
			mbp->msg_bufr = (mbp->msg_bufr % mbp->msg_size);
		}
	}

	return buf_entry;
}

void
oslog_streamwrite_metadata_locked(oslog_stream_buf_entry_t m_entry)
{
	LCK_SPIN_ASSERT(&oslog_stream_lock, LCK_ASSERT_OWNED);
	STAILQ_INSERT_TAIL(&oslog_stream_buf_head, m_entry, buf_entries);

	return;
}

static void
oslog_streamwrite_append_bytes(const char *buffer, int buflen)
{
	struct msgbuf *mbp;

	LCK_SPIN_ASSERT(&oslog_stream_lock, LCK_ASSERT_OWNED);

	assert(oslog_stream_buf_bytesavail >= buflen);
	oslog_stream_buf_bytesavail -= buflen;
	assert(oslog_stream_buf_bytesavail >= 0);

	mbp = oslog_streambufp;
	if (mbp->msg_bufx + buflen <= mbp->msg_size) {
		  
                                                                
                                               
     
		memcpy((void *)(mbp->msg_bufc + mbp->msg_bufx), buffer, buflen);

		mbp->msg_bufx += buflen;
		if (mbp->msg_bufx == mbp->msg_size) {
			mbp->msg_bufx = 0;
		}
	} else {
		  
                                                                       
                      
     
		int bytes_left = mbp->msg_size - mbp->msg_bufx;
		memcpy((void *)(mbp->msg_bufc + mbp->msg_bufx), buffer, bytes_left);

		buflen -= bytes_left;
		buffer += bytes_left;

		                                                              
		memcpy((void *)mbp->msg_bufc, buffer, buflen);
		mbp->msg_bufx = buflen;
	}
	return;
}

void
oslog_streamwrite_locked(firehose_tracepoint_id_u ftid,
    uint64_t stamp, const void *pubdata, size_t publen)
{
	struct msgbuf *mbp;
	oslog_stream_buf_entry_t buf_entry = NULL;
	oslog_stream_buf_entry_t next_entry = NULL;

	LCK_SPIN_ASSERT(&oslog_stream_lock, LCK_ASSERT_OWNED);

	assert(publen <= UINT16_MAX);
	const ssize_t ft_length = offsetof(struct firehose_tracepoint_s, ft_data) + publen;

	mbp = oslog_streambufp;
	if (ft_length > mbp->msg_size) {
		os_atomic_inc(&oslog_s_error_count, relaxed);
		return;
	}

	                                                     
	buf_entry = oslog_stream_find_free_buf_entry_locked();

	assert(buf_entry != NULL);

	while (ft_length > oslog_stream_buf_bytesavail) {
		oslog_stream_buf_entry_t prev_entry = NULL;

		next_entry = STAILQ_FIRST(&oslog_stream_buf_head);
		assert(next_entry != NULL);
		while (next_entry->type == oslog_stream_link_type_metadata) {
			prev_entry = next_entry;
			next_entry = STAILQ_NEXT(next_entry, buf_entries);
		}

		if (prev_entry == NULL) {
			STAILQ_REMOVE_HEAD(&oslog_stream_buf_head, buf_entries);
		} else {
			STAILQ_REMOVE_AFTER(&oslog_stream_buf_head, prev_entry, buf_entries);
		}

		mbp->msg_bufr += next_entry->size;
		if (mbp->msg_bufr >= mbp->msg_size) {
			mbp->msg_bufr = (mbp->msg_bufr % mbp->msg_size);
		}

		oslog_s_dropped_msgcount++;
		oslog_stream_buf_bytesavail += next_entry->size;
		assert(oslog_stream_buf_bytesavail <= oslog_stream_buf_size);

		STAILQ_INSERT_TAIL(&oslog_stream_free_head, next_entry, buf_entries);
	}

	assert(ft_length <= oslog_stream_buf_bytesavail);

	                                                               
	buf_entry->offset = mbp->msg_bufx;
	buf_entry->size = (uint16_t)ft_length;
	buf_entry->timestamp = stamp;
	buf_entry->type = oslog_stream_link_type_log;

	                         
	struct firehose_tracepoint_s fs = {
		.ft_thread = thread_tid(current_thread()),
		.ft_id.ftid_value = ftid.ftid_value,
		.ft_length = publen
	};

	oslog_streamwrite_append_bytes((char *)&fs, sizeof(fs));
	oslog_streamwrite_append_bytes(pubdata, (int)publen);

	assert(mbp->msg_bufr < mbp->msg_size);
	                                             
	STAILQ_INSERT_TAIL(&oslog_stream_buf_head, buf_entry, buf_entries);

	return;
}

  
           
  
                                                                           
                                                              
  
                                       
  
                  
  
                                                                      
                                                               
   
void
log_putc(char c)
{
	if (!bsd_log_lock(oslog_is_safe())) {
		os_atomic_inc(&oslog_msgbuf_dropped_charcount, relaxed);
		return;
	}

	log_putc_locked(msgbufp, c);
	int unread_count = msgbufp->msg_bufx - msgbufp->msg_bufr;

	bsd_log_unlock();

	if (unread_count < 0) {
		unread_count = 0 - unread_count;
	}
	if (c == '\n' || unread_count >= (msgbufp->msg_size / 2)) {
		logwakeup(msgbufp);
	}
}


  
                                                                  
             
                                                                 
                       
                                                                     
                                                                             
                                                                         
   
int
log_setsize(int size)
{
	char *new_logdata;
	int new_logsize, new_bufr, new_bufx;
	char *old_logdata;
	int old_logsize, old_bufr, old_bufx;
	int i, count;
	char *p, ch;

	if (size > MAX_MSG_BSIZE) {
		return EINVAL;
	}

	if (size <= 0) {
		return EINVAL;
	}

	new_logsize = size;
	new_logdata = kheap_alloc(KHEAP_DATA_BUFFERS, size, Z_WAITOK | Z_ZERO);
	if (!new_logdata) {
		printf("log_setsize: unable to allocate memory\n");
		return ENOMEM;
	}

	bsd_log_lock_safe();

	old_logsize = msgbufp->msg_size;
	old_logdata = msgbufp->msg_bufc;
	old_bufr = msgbufp->msg_bufr;
	old_bufx = msgbufp->msg_bufx;

	LOG_SETSIZE_DEBUG("log_setsize(%d): old_logdata %p old_logsize %d old_bufr %d old_bufx %d\n",
	    size, old_logdata, old_logsize, old_bufr, old_bufx);

	                                                        
	if (new_logsize <= old_bufx) {
		count = new_logsize;
		p = old_logdata + old_bufx - count;
	} else {
		  
                                                           
                                      
     
		count = MIN(new_logsize, old_logsize);
		p = old_logdata + old_logsize - (count - old_bufx);
	}
	for (i = 0; i < count; i++) {
		if (p >= old_logdata + old_logsize) {
			p = old_logdata;
		}

		ch = *p++;
		new_logdata[i] = ch;
	}

	new_bufx = i;
	if (new_bufx >= new_logsize) {
		new_bufx = 0;
	}
	msgbufp->msg_bufx = new_bufx;

	new_bufr = old_bufx - old_bufr;                                         
	if (new_bufr < 0) {
		new_bufr += old_logsize;
	}
	new_bufr = new_bufx - new_bufr;                                                
	if (new_bufr < 0) {
		new_bufr += new_logsize;
	}
	msgbufp->msg_bufr = new_bufr;

	msgbufp->msg_size = new_logsize;
	msgbufp->msg_bufc = new_logdata;

	LOG_SETSIZE_DEBUG("log_setsize(%d): new_logdata %p new_logsize %d new_bufr %d new_bufx %d\n",
	    size, new_logdata, new_logsize, new_bufr, new_bufx);

	bsd_log_unlock();

	                                                                  
                                       
	bzero(old_logdata, old_logsize);
	if (old_logdata != smsg_bufc) {
		                                       
		kheap_free(KHEAP_DATA_BUFFERS, old_logdata, old_logsize);
	}

	printf("set system log size to %d bytes\n", new_logsize);

	return 0;
}

void
oslog_setsize(int size)
{
	uint16_t scale = 0;
	                                                     
	             
	if (size <= OSLOG_STREAM_BUF_SIZE) {
		return;
	}

	scale = (uint16_t) (size / OSLOG_STREAM_BUF_SIZE);

	oslog_stream_buf_size = size;
	oslog_stream_num_entries = scale * OSLOG_NUM_STREAM_ENTRIES;
	printf("oslog_setsize: new buffer size = %d, new num entries= %d\n", oslog_stream_buf_size, oslog_stream_num_entries);
}

SYSCTL_PROC(_kern, OID_AUTO, msgbuf,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, 0, 0,
    sysctl_kern_msgbuf, "I", "");

static int
sysctl_kern_msgbuf(struct sysctl_oid *oidp __unused,
    void *arg1 __unused, int arg2 __unused, struct sysctl_req *req)
{
	int old_bufsize, bufsize;
	int error;

	bsd_log_lock_safe();
	old_bufsize = bufsize = msgbufp->msg_size;
	bsd_log_unlock();

	error = sysctl_io_number(req, bufsize, sizeof(bufsize), &bufsize, NULL);
	if (error) {
		return error;
	}

	if (bufsize != old_bufsize) {
		error = log_setsize(bufsize);
	}

	return error;
}


  
                                                         
                                                           
   
int
log_dmesg(user_addr_t buffer, uint32_t buffersize, int32_t *retval)
{
	uint32_t i;
	uint32_t localbuff_size;
	int error = 0, newl, skip;
	char *localbuff, *p, *copystart, ch;
	size_t copysize;

	bsd_log_lock_safe();
	localbuff_size = (msgbufp->msg_size + 2);                    
	bsd_log_unlock();

	                                                          
	localbuff = kheap_alloc(KHEAP_DATA_BUFFERS, localbuff_size, Z_WAITOK);
	if (!localbuff) {
		printf("log_dmesg: unable to allocate memory\n");
		return ENOMEM;
	}

	                                                                   
	bsd_log_lock_safe();

	  
                                                                   
                                          
    
	p = msgbufp->msg_bufc + msgbufp->msg_bufx;
	for (i = newl = skip = 0; p != msgbufp->msg_bufc + msgbufp->msg_bufx - 1; ++p) {
		if (p >= msgbufp->msg_bufc + msgbufp->msg_size) {
			p = msgbufp->msg_bufc;
		}
		ch = *p;
		                                     
		if (skip) {
			if (ch == '>') {
				newl = skip = 0;
			}
			continue;
		}
		if (newl && ch == '<') {
			skip = 1;
			continue;
		}
		if (ch == '\0') {
			continue;
		}
		newl = (ch == '\n');
		localbuff[i++] = ch;
		                                                          
                                                              
                                                             
                                                      
     
		if (i == (localbuff_size - 2)) {
			break;
		}
	}
	if (!newl) {
		localbuff[i++] = '\n';
	}
	localbuff[i++] = 0;

	if (buffersize >= i) {
		copystart = localbuff;
		copysize = i;
	} else {
		copystart = localbuff + i - buffersize;
		copysize = buffersize;
	}

	bsd_log_unlock();

	error = copyout(copystart, buffer, copysize);
	if (!error) {
		*retval = (int32_t)copysize;
	}

	kheap_free(KHEAP_DATA_BUFFERS, localbuff, localbuff_size);
	return error;
}

#ifdef CONFIG_XNUPOST

size_t find_pattern_in_buffer(const char *, size_t, size_t);

  
                                                      
                                                           
   
size_t
find_pattern_in_buffer(const char *pattern, size_t len, size_t expected_count)
{
	if (pattern == NULL || len == 0 || expected_count == 0) {
		return 0;
	}

	size_t msg_bufx = msgbufp->msg_bufx;
	size_t msg_size = msgbufp->msg_size;
	size_t match_count = 0;

	for (size_t i = 0; i < msg_size; i++) {
		boolean_t match = TRUE;
		for (size_t j = 0; j < len; j++) {
			size_t pos = (msg_bufx + i + j) % msg_size;
			if (msgbufp->msg_bufc[pos] != pattern[j]) {
				match = FALSE;
				break;
			}
		}
		if (match && ++match_count >= expected_count) {
			break;
		}
	}

	return match_count;
}

#endif
