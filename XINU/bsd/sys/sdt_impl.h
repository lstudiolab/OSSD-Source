/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SDT_IMPL_H
#define _SDT_IMPL_H

#ifdef  __cplusplus
extern "C" {
#endif

extern const char *sdt_prefix;

typedef struct sdt_probedesc {
	char                    *sdpd_name;                             
	int                     sdpd_namelen;
	char                    *sdpd_func;                                    
	const char              *sdpd_prov;                                    
	unsigned long           sdpd_offset;                                
	struct sdt_probedesc    *sdpd_next;                            
} sdt_probedesc_t;

#ifdef  __cplusplus
}
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#include <sys/dtrace.h>

struct module {
	int sdt_nprobes;
	sdt_probedesc_t *sdt_probes;
};

extern int sdt_invop(uintptr_t, uintptr_t *, uintptr_t);
extern uint64_t sdt_getarg(void *, dtrace_id_t, void *, int, int);

void sdt_provide_module(void *, struct modctl *);
void sdt_early_init(void);
void sdt_init(void);

extern int          sdt_probetab_size;
extern int          sdt_probetab_mask;
#define SDT_ADDR2NDX(addr)  ((((uintptr_t)(addr)) >> 4) & sdt_probetab_mask)


#if defined(__x86_64__)
typedef uint8_t sdt_instr_t;
#elif defined(__arm__)
typedef uint16_t sdt_instr_t;
#elif defined(__arm64__)
typedef uint32_t sdt_instr_t;
#else
#error Unknown implementation
#endif

typedef struct sdt_provider {
	const char                      *sdtp_name;                           
	const char                      *sdtp_prefix;                               
	dtrace_pattr_t          *sdtp_attr;                               
	dtrace_provider_id_t    sdtp_id;                         
} sdt_provider_t;

extern sdt_provider_t sdt_providers[];                                  

typedef struct sdt_probe {
	sdt_provider_t  *sdp_provider;                        
	char            *sdp_name;                                 
	int             sdp_namelen;                                          
	dtrace_id_t     sdp_id;                               
	struct modctl   *sdp_ctl;                                      
	int             sdp_loadcnt;                                       
	int             sdp_primary;                                         
	sdt_instr_t     *sdp_patchpoint;                         
	sdt_instr_t     sdp_patchval;                                     
	sdt_instr_t     sdp_savedval;                                        
	struct sdt_probe *sdp_next;                             
	struct sdt_probe *sdp_hashnext;                           
} sdt_probe_t;

typedef struct sdt_argdesc {
	const char *sda_provider;                                     
	const char *sda_name;                                      
	const int sda_ndx;                                          
	const int sda_mapping;                                           
	const char *sda_native;                                              
	const char *sda_xlate;                                              
} sdt_argdesc_t;

extern void sdt_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);

#ifdef  __cplusplus
}
#endif

#endif                   
