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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_FASTTRAP_H
#define	_SYS_FASTTRAP_H

#include <sys/fasttrap_isa.h>
#include <sys/dtrace.h>
#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(__APPLE__)
#define	FASTTRAPIOC		(('m' << 24) | ('r' << 16) | ('f' << 8))
#else
#define	FASTTRAPIOC		_IO('d', 0)
#endif                
#define	FASTTRAPIOC_MAKEPROBE	(FASTTRAPIOC | 1)
#define	FASTTRAPIOC_GETINSTR	(FASTTRAPIOC | 2)

typedef enum fasttrap_probe_type {
	DTFTP_NONE = 0,
	DTFTP_ENTRY,
	DTFTP_RETURN,
	DTFTP_OFFSETS,
	DTFTP_POST_OFFSETS,
	DTFTP_IS_ENABLED
} fasttrap_probe_type_t;

#if defined(__APPLE__)
typedef enum fasttrap_provider_type {
	DTFTP_PROVIDER_NONE = 0,
	DTFTP_PROVIDER_USDT, 
	DTFTP_PROVIDER_PID,
	DTFTP_PROVIDER_OBJC,
	DTFTP_PROVIDER_ONESHOT
} fasttrap_provider_type_t;

                           
#define	FASTTRAP_PID_NAME		"pid"
#define	FASTTRAP_OBJC_NAME		"objc"
#define	FASTTRAP_ONESHOT_NAME		"oneshot"

#endif

typedef struct fasttrap_probe_spec {
	pid_t				ftps_pid;
#if defined(__APPLE__)
	fasttrap_provider_type_t	ftps_provider_type;
	fasttrap_probe_type_t		ftps_probe_type;
#if defined(__arm__) || defined(__arm64__)
	uint32_t			ftps_arch_subinfo;	                                                          
#endif
#endif
	char				ftps_func[DTRACE_FUNCNAMELEN];
	char				ftps_mod[DTRACE_MODNAMELEN];

#if defined(__APPLE__)
#if defined(__arm__) || defined(__arm64__)
	                                                             
#else
#if !defined(__LP64__)
	uint32_t			pad;                                                    
#endif
#endif
#endif
	uint64_t			ftps_pc;
	uint64_t			ftps_size;
	uint64_t			ftps_noffs;
	uint64_t			ftps_offs[1];
} fasttrap_probe_spec_t;

typedef struct fasttrap_instr_query {
	uint64_t		ftiq_pc;
	pid_t			ftiq_pid;
	fasttrap_instr_t	ftiq_instr;
} fasttrap_instr_query_t;

  
                                                                        
                                                                           
                                                                        
                                                                               
                                                                    
                                                                               
                                                                         
                                                                             
                                                                                
                                                                             
                                                                       
                                                                    
                                
   
#define	PT_SUNWDTRACE_SIZE	FASTTRAP_SUNWDTRACE_SIZE

#ifdef	__cplusplus
}
#endif

#endif	                     
