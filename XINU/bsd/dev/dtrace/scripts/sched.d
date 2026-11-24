/*
 * Copyright (c) 2006-2008 Apple Computer, Inc.  All Rights Reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */

#pragma D depends_on library darwin.d
#pragma D depends_on module mach_kernel
#pragma D depends_on provider sched

struct _processor_info {
    int pi_state;                                           
    char    pi_processor_type[32];                      
    char    pi_fputypes[32];                         
    int pi_clock;                                      
};

typedef struct _processor_info _processor_info_t;

typedef int chipid_t;
typedef int lgrp_id_t;

struct cpuinfo {
	processorid_t cpu_id;		                    
	psetid_t cpu_pset;		                              
	chipid_t cpu_chip;		                     
	lgrp_id_t cpu_lgrp;		                              
	_processor_info_t cpu_info;	                     
};

typedef struct cpuinfo cpuinfo_t;

translator cpuinfo_t < processor_t P > {
	cpu_id = P->cpu_id;
	cpu_pset = -1;                                            
	cpu_chip = P->cpu_id;          
	cpu_lgrp = 0;          
	cpu_info = *((_processor_info_t *)`dtrace_zero);                  
}; 

inline cpuinfo_t *curcpu = xlate <cpuinfo_t *> (curthread->last_processor);
#pragma D attributes Stable/Stable/Common curcpu
#pragma D binding "1.0" curcpu

  
                                                   
   

inline psetid_t pset = curcpu->cpu_pset;
#pragma D attributes Stable/Stable/Common pset
#pragma D binding "1.0" pset

inline chipid_t chip = curcpu->cpu_chip;
#pragma D attributes Stable/Stable/Common chip
#pragma D binding "1.0" chip

inline lgrp_id_t lgrp = curcpu->cpu_lgrp;
#pragma D attributes Stable/Stable/Common lgrp
#pragma D binding "1.0" lgrp

