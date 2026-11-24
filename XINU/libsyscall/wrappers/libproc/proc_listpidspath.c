/*
 * Copyright (c) 2007, 2008 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <libproc.h>


typedef struct {
	              
	int                             *pids;
	int                             pids_count;
	size_t                          pids_size;

	          
	uint64_t                        *threads;
	int                             thr_count;
	size_t                          thr_size;

	                        
	struct proc_fdinfo              *fds;
	int                             fds_count;
	size_t                          fds_size;

	                          
	struct stat                     match_stat;

	        
	uint32_t                        flags;
} fdOpenInfo, *fdOpenInfoRef;


  
             
   
static fdOpenInfoRef
check_init(const char *path, uint32_t flags)
{
	fdOpenInfoRef   info;
	int             status;

	info = malloc(sizeof(*info));
	if (!info) {
		return NULL;
	}

	info->pids              = NULL;
	info->pids_count        = 0;
	info->pids_size         = 0;

	info->threads           = NULL;
	info->thr_count         = 0;
	info->thr_size          = 0;

	info->fds               = NULL;
	info->fds_count         = 0;
	info->fds_size          = 0;

	status = stat(path, &info->match_stat);
	if (status == -1) {
		goto fail;
	}

	info->flags             = flags;

	return info;

fail:

	free(info);
	return NULL;
}


  
             
   
static void
check_free(fdOpenInfoRef info)
{
	if (info->pids != NULL) {
		free(info->pids);
	}

	if (info->threads != NULL) {
		free(info->threads);
	}

	if (info->fds != NULL) {
		free(info->fds);
	}

	free(info);

	return;
}


  
             
                                            
  
                        
                      
                         
                      
   
static int
check_file(fdOpenInfoRef info, struct vinfo_stat *sb)
{
	if (sb->vst_dev == 0) {
		             
		return 0;
	}

	if (sb->vst_dev != info->match_stat.st_dev) {
		                                  
		return 0;
	}

	if (!(info->flags & PROC_LISTPIDSPATH_PATH_IS_VOLUME) &&
	    (sb->vst_ino != info->match_stat.st_ino)) {
		                            
		return 0;
	}

	return 1;
}


  
                       
                                              
                                   
  
              
                      
                         
                      
   
static int
check_process_vnodes(fdOpenInfoRef info, int pid)
{
	int                             buf_used;
	int                             status;
	struct proc_vnodepathinfo       vpi;

	buf_used = proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0, &vpi, sizeof(vpi));
	if (buf_used <= 0) {
		if (errno == ESRCH) {
			                         
			return 0;
		}
		return -1;
	} else if (buf_used < sizeof(vpi)) {
		                                      
		return -1;
	}

	                                       
	status = check_file(info, &vpi.pvi_cdir.vip_vi.vi_stat);
	if (status != 0) {
		                    
		return status;
	}

	                            
	status = check_file(info, &vpi.pvi_rdir.vip_vi.vi_stat);
	if (status != 0) {
		                    
		return status;
	}

	return 0;
}


  
                     
                                  
  
              
                      
                         
                      
   
static int
check_process_text(fdOpenInfoRef info, int pid)
{
	int             status;
	int             buf_used;
	struct proc_regionwithpathinfo  rwpi;

	if (info->flags & PROC_LISTPIDSPATH_PATH_IS_VOLUME) {
		                                                      
		buf_used = proc_pidinfo(pid, PROC_PIDREGIONPATHINFO3, info->match_stat.st_dev, &rwpi, sizeof(rwpi));
		if (buf_used <= 0) {
			if ((errno == ESRCH) || (errno == EINVAL)) {
				                                                             
				return 0;
			}
			return -1;
		} else if (buf_used < sizeof(rwpi)) {
			                                      
			return -1;
		}

		status = check_file(info, &rwpi.prp_vip.vip_vi.vi_stat);
		if (status != 0) {
			                    
			return status;
		}
	} else {
		uint64_t        a       = 0;

		while (1) {                              
			                          
			buf_used = proc_pidinfo(pid, PROC_PIDREGIONPATHINFO2, a, &rwpi, sizeof(rwpi));
			if (buf_used <= 0) {
				if ((errno == ESRCH) || (errno == EINVAL)) {
					                                                             
					break;
				}
				return -1;
			} else if (buf_used < sizeof(rwpi)) {
				                                      
				return -1;
			}

			status = check_file(info, &rwpi.prp_vip.vip_vi.vi_stat);
			if (status != 0) {
				                    
				return status;
			}

			a = rwpi.prp_prinfo.pri_address + rwpi.prp_prinfo.pri_size;
		}
	}

	return 0;
}


  
                    
                                          
  
              
                      
                         
                      
   
static int
check_process_fds(fdOpenInfoRef info, int pid)
{
	int     buf_used;
	int     i;
	int     status;

	                                    
	buf_used = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, NULL, 0);
	if (buf_used <= 0) {
		return -1;
	}

	while (1) {
		if (buf_used > info->fds_size) {
			                                      
			while (buf_used > info->fds_size) {
				info->fds_size += (sizeof(struct proc_fdinfo) * 32);
			}

			if (info->fds == NULL) {
				info->fds = malloc(info->fds_size);
			} else {
				info->fds = reallocf(info->fds, info->fds_size);
			}
			if (info->fds == NULL) {
				return -1;
			}
		}

		buf_used = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, info->fds, (int)info->fds_size);
		if (buf_used <= 0) {
			return -1;
		}

		if ((buf_used + sizeof(struct proc_fdinfo)) >= info->fds_size) {
			                                                   
			buf_used = (int)(info->fds_size + sizeof(struct proc_fdinfo));
			continue;
		}

		info->fds_count = (int)(buf_used / sizeof(struct proc_fdinfo));
		break;
	}

	                                       
	for (i = 0; i < info->fds_count; i++) {
		struct proc_fdinfo      *fdp;

		fdp = &info->fds[i];
		switch (fdp->proc_fdtype) {
		case PROX_FDTYPE_VNODE: {
			int                     buf_used;
			struct vnode_fdinfo     vi;

			buf_used = proc_pidfdinfo(pid, fdp->proc_fd, PROC_PIDFDVNODEINFO, &vi, sizeof(vi));
			if (buf_used <= 0) {
				if (errno == ENOENT) {
					  
                                                                    
                                                                      
                                                               
                                                    
        
					continue;
				}
				return -1;
			} else if (buf_used < sizeof(vi)) {
				                                      
				return -1;
			}

			if ((info->flags & PROC_LISTPIDSPATH_EXCLUDE_EVTONLY) &&
			    (vi.pfi.fi_openflags & O_EVTONLY)) {
				                                  
				continue;
			}

			status = check_file(info, &vi.pvi.vi_stat);
			if (status != 0) {
				                    
				return status;
			}
			break;
		}
		default:
			break;
		}
	}

	return 0;
}


  
                        
                                               
  
              
                      
                         
                      
   
static int
check_process_threads(fdOpenInfoRef info, int pid)
{
	int                             buf_used;
	int                             status;
	struct proc_taskallinfo         tai;

	buf_used = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &tai, sizeof(tai));
	if (buf_used <= 0) {
		if (errno == ESRCH) {
			                         
			return 0;
		}
		return -1;
	} else if (buf_used < sizeof(tai)) {
		                                      
		return -1;
	}

	                    
	if (tai.pbsd.pbi_flags & PROC_FLAG_THCWD) {
		int     i;

		                      
		buf_used = tai.ptinfo.pti_threadnum * sizeof(uint64_t);

		while (1) {
			if (buf_used > info->thr_size) {
				                                      
				while (buf_used > info->thr_size) {
					info->thr_size += (sizeof(uint64_t) * 32);
				}

				if (info->threads == NULL) {
					info->threads = malloc(info->thr_size);
				} else {
					info->threads = reallocf(info->threads, info->thr_size);
				}
				if (info->threads == NULL) {
					return -1;
				}
			}

			buf_used = proc_pidinfo(pid, PROC_PIDLISTTHREADS, 0, info->threads, (int)info->thr_size);
			if (buf_used <= 0) {
				return -1;
			}

			if ((buf_used + sizeof(uint64_t)) >= info->thr_size) {
				                                                       
				buf_used = (int)(info->thr_size + sizeof(uint64_t));
				continue;
			}

			info->thr_count = buf_used / sizeof(uint64_t);
			break;
		}

		                              
		for (i = 0; i < info->thr_count; i++) {
			uint64_t                        thr     = info->threads[i];
			struct proc_threadwithpathinfo  tpi;

			buf_used = proc_pidinfo(pid, PROC_PIDTHREADPATHINFO, thr, &tpi, sizeof(tpi));
			if (buf_used <= 0) {
				if ((errno == ESRCH) || (errno == EINVAL)) {
					                                   
					continue;
				}
			} else if (buf_used < sizeof(tai)) {
				                                      
				return -1;
			}

			status = check_file(info, &tpi.pvip.vip_vi.vi_stat);
			if (status != 0) {
				                    
				return status;
			}
		}
	}

	return 0;
}


  
                       
                                                                      
                                          
                                                                    
  
              
                      
                         
                      
   
static int
check_process_phase1(fdOpenInfoRef info, int pid)
{
	int     status;

	                                           
	status = check_process_vnodes(info, pid);
	if (status != 0) {
		                    
		return status;
	}

	                              
	status = check_process_fds(info, pid);
	if (status != 0) {
		                    
		return status;
	}

	                                       
	status = check_process_threads(info, pid);
	if (status != 0) {
		                    
		return status;
	}

	return 0;
}

  
                       
                                  
  
              
                      
                         
                      
   
static int
check_process_phase2(fdOpenInfoRef info, int pid)
{
	int     status;

	                              
	status = check_process_text(info, pid);
	if (status != 0) {
		                    
		return status;
	}

	return 0;
}

  
                    
  
               
                   
               
                    
                 
                     
                                                             
                                                              
                                                           
                                           
   
int
proc_listpidspath(uint32_t      type,
    uint32_t      typeinfo,
    const char    *path,
    uint32_t      pathflags,
    void          *buffer,
    int           buffersize)
{
	int             buf_used;
	int             *buf_next       = (int *)buffer;
	int             i;
	fdOpenInfoRef   info;
	int             status          = -1;

	if (buffer == NULL) {
		                              
		return proc_listpids(type, typeinfo, NULL, 0);
	}

	buffersize -= (buffersize % sizeof(int));                             
	if (buffersize < sizeof(int)) {
		                                       
		errno = ENOMEM;
		return -1;
	}

	       
	info = check_init(path, pathflags);
	if (info == NULL) {
		return -1;
	}

	                        
	buf_used = proc_listpids(type, typeinfo, NULL, 0);
	if (buf_used <= 0) {
		goto done;
	}

	while (1) {
		if (buf_used > info->pids_size) {
			                                      
			while (buf_used > info->pids_size) {
				info->pids_size += (sizeof(int) * 32);
			}

			if (info->pids == NULL) {
				info->pids = malloc(info->pids_size);
			} else {
				info->pids = reallocf(info->pids, info->pids_size);
			}
			if (info->pids == NULL) {
				goto done;
			}
		}

		buf_used = proc_listpids(type, typeinfo, info->pids, (int)info->pids_size);
		if (buf_used <= 0) {
			goto done;
		}

		if ((buf_used + sizeof(int)) >= info->pids_size) {
			                                                    
			buf_used = (int)(info->pids_size + sizeof(int));
			continue;
		}

		info->pids_count = buf_used / sizeof(int);
		break;
	}

	                               
	buf_used = 0;
	for (i = info->pids_count - 1; i >= 0; i--) {
		int     pid;
		int     pstatus;

		pid = info->pids[i];
		if (pid == 0) {
			continue;
		}

		pstatus = check_process_phase1(info, pid);
		if (pstatus != 1) {
			                 
			continue;
		}

		*buf_next++ = pid;
		buf_used += sizeof(int);

		if (buf_used >= buffersize) {
			                               
			break;
		}
	}

	if (buf_used >= buffersize) {
		                               
		status = buf_used;
		goto done;
	}

	                                                           
	for (i = info->pids_count - 1; i >= 0; i--) {
		int     pid;
		int     pstatus;

		pid = info->pids[i];
		if (pid == 0) {
			continue;
		}

		pstatus = check_process_phase2(info, pid);
		if (pstatus != 1) {
			                 
			continue;
		}

		*buf_next++ = pid;
		buf_used += sizeof(int);

		if (buf_used >= buffersize) {
			                               
			break;
		}
	}

	status = buf_used;

done:

	          
	check_free(info);

	return status;
}
