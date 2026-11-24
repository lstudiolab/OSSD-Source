/*
 * Copyright (c) 2006-2012 Apple Inc. All rights reserved.
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

  
                                 
   

#define CONFIG_MEMORYSTATUS 1                             
#include <sys/types.h>                      
#include <spawn.h>
#include <spawn_private.h>
#include <sys/spawn_internal.h>
#include <sys/process_policy.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>                                 
#include <string.h>
#include <strings.h>
#include <mach/port.h>
#include <mach/mach_param.h>                                 
#include <mach/exception_types.h>
#include <mach/coalition.h>                             
#include <sys/kern_memorystatus.h>

  
                       
  
                                                                             
  
                                                       
                  
  
                       
                                          
                                              
  
                                                                  
                                                                   
                                                                  
                                                             
           
  
                                                               
                                                             
                                                                
                                                              
                                                                  
                                         
   
int
posix_spawnattr_init(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t *psattrp = (_posix_spawnattr_t *)attr;
	int     err = 0;

	if ((*psattrp = (_posix_spawnattr_t)malloc(sizeof(struct _posix_spawnattr))) == NULL) {
		err = ENOMEM;
	} else {
		  
                                                          
                   
     
		(*psattrp)->psa_flags = 0;

		  
                                                          
               
     
		(*psattrp)->psa_sigdefault = 0;

		                                                        
		(*psattrp)->psa_sigmask = 0;

		                                                       
		(*psattrp)->psa_pgroup = 0;                         

		                                                               
		memset((*psattrp)->psa_binprefs, 0,
		    sizeof((*psattrp)->psa_binprefs));
		memset((*psattrp)->psa_subcpuprefs, 0xff                      ,
		    sizeof((*psattrp)->psa_subcpuprefs));

		                                        
		(*psattrp)->psa_ports = NULL;

		  
                                                       
                                           
     
		(*psattrp)->psa_pcontrol = 0;

		  
                                         
     

		(*psattrp)->short_padding = 0;
		(*psattrp)->flags_padding = 0;

		                                         
		(*psattrp)->psa_apptype = POSIX_SPAWN_PROCESS_TYPE_DEFAULT;

		                    
		(*psattrp)->psa_jetsam_flags = 0;
		(*psattrp)->psa_priority = -1;
		(*psattrp)->psa_memlimit_active = -1;
		(*psattrp)->psa_memlimit_inactive = -1;

		                                
		(*psattrp)->psa_thread_limit = 0;

		                                             
		(*psattrp)->psa_cpumonitor_percent = 0;
		(*psattrp)->psa_cpumonitor_interval = 0;

		                                          
		(*psattrp)->psa_mac_extensions = NULL;

		                                                 
		(*psattrp)->psa_coalition_info = NULL;

		(*psattrp)->psa_persona_info = NULL;

		(*psattrp)->psa_posix_cred_info = NULL;

		  
                        
                                                          
                                                           
                                                         
                  
                                                       
     
		(*psattrp)->psa_reserved = 1;

		                             
		(*psattrp)->psa_qos_clamp = POSIX_SPAWN_PROC_CLAMP_NONE;

		                                  
		(*psattrp)->psa_darwin_role = POSIX_SPAWN_DARWIN_ROLE_NONE;

		(*psattrp)->psa_max_addr = 0;

		(*psattrp)->psa_no_smt = false;
		(*psattrp)->psa_tecs = false;

		                                       
		(*psattrp)->psa_subsystem_root_path = NULL;

		                                  
		(*psattrp)->psa_platform = 0;

		                          
		(*psattrp)->psa_options = PSA_OPTION_NONE;
	}

	return err;
}


  
                          
  
                                                                     
                                                         
                                                                 
  
                                                       
                 
  
                       
  
                                                                     
                                                              
                                                               
                           
  
                                                    
                                                  
   
static int posix_spawn_destroyportactions_np(posix_spawnattr_t *);
static int posix_spawn_destroycoalition_info_np(posix_spawnattr_t *);
static int posix_spawn_destroypersona_info_np(posix_spawnattr_t *);
static int posix_spawn_destroyposix_cred_info_np(posix_spawnattr_t *);
static int posix_spawn_destroymacpolicy_info_np(posix_spawnattr_t *);
static int posix_spawn_destroysubsystem_root_path_np(posix_spawnattr_t *);

int
posix_spawnattr_destroy(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	posix_spawn_destroyportactions_np(attr);
	posix_spawn_destroycoalition_info_np(attr);
	posix_spawn_destroypersona_info_np(attr);
	posix_spawn_destroyposix_cred_info_np(attr);
	posix_spawn_destroymacpolicy_info_np(attr);
	posix_spawn_destroysubsystem_root_path_np(attr);

	free(psattr);
	*attr = NULL;

	return 0;
}


  
                           
  
                                                                            
                          
  
                                                             
                    
                                  
  
                       
  
                                                    
                                                  
                                                             
   
int
posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_flags = flags;

	return 0;
}


  
                           
  
                                                                           
                                                            
                                   
  
                                                             
                          
                                                  
                
  
                       
  
                    
                                                     
                        
  
                                                    
                                                  
                                                             
   
int
posix_spawnattr_getflags(const posix_spawnattr_t * __restrict attr,
    short * __restrict flagsp)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*flagsp = psattr->psa_flags;

	return 0;
}


  
                                
  
                                                                             
                                                                
                                                                
                
  
                                                       
                                           
                   
                                                    
                     
  
                       
  
                    
                                                               
                                       
   
int
posix_spawnattr_getsigdefault(const posix_spawnattr_t * __restrict attr,
    sigset_t * __restrict sigdefault)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*sigdefault = psattr->psa_sigdefault;

	return 0;
}


  
                            
  
                                                                              
                                                              
                                                         
  
                                                       
                                         
                
                                                  
                    
  
                       
  
                    
                                                             
                              
   
int
posix_spawnattr_getpgroup(const posix_spawnattr_t * __restrict attr,
    pid_t * __restrict pgroup)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*pgroup = psattr->psa_pgroup;

	return 0;
}


  
                             
  
                                                                            
                                                              
                                                                
             
  
                                                       
                                          
                   
                                                 
                     
  
                       
  
                    
                                                         
                                       
   
int
posix_spawnattr_getsigmask(const posix_spawnattr_t * __restrict attr,
    sigset_t * __restrict sigmask)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*sigmask = psattr->psa_sigmask;

	return 0;
}

  
                                
  
                                                                               
                                                                              
                                                
  
                                                       
                                             
                                            
                                
                                     
  
                                           
                                                        
                                                                       
  
                    
                                                 
                                       
   
int
posix_spawnattr_getbinpref_np(const posix_spawnattr_t * __restrict attr,
    size_t count, cpu_type_t *pref, size_t * __restrict ocount)
{
	_posix_spawnattr_t psattr;
	int i = 0;

	if (attr == NULL || *attr == NULL || pref == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	for (i = 0; i < count && i < NBINPREFS; i++) {
		pref[i] = psattr->psa_binprefs[i];
	}

	if (ocount) {
		*ocount = i;
	}
	return 0;
}

  
                                 
  
                                                                               
                                                                              
                                                              
  
                                                       
                                             
                                            
                                
                                      
                                     
  
                                               
                                                    
                                                                       
  
                    
                                              
                                       
                                                    
                                       
   
int
posix_spawnattr_getarchpref_np(const posix_spawnattr_t * __restrict attr,
    size_t count, cpu_type_t *pref, cpu_subtype_t *subpref, size_t * __restrict ocount)
{
	_posix_spawnattr_t psattr;
	int i = 0;

	if (attr == NULL || *attr == NULL || pref == NULL || subpref == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	for (i = 0; i < count && i < NBINPREFS; i++) {
		pref[i] = psattr->psa_binprefs[i];
		subpref[i] = psattr->psa_subcpuprefs[i];
	}

	if (ocount) {
		*ocount = i;
	}
	return 0;
}


  
                                 
  
                                                                               
                                                                
                                                                
              
  
                                                       
                                           
                   
                                             
                               
  
                       
  
                    
                                                             
                                       
   
int
posix_spawnattr_getpcontrol_np(const posix_spawnattr_t * __restrict attr,
    int * __restrict pcontrol)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*pcontrol = psattr->psa_pcontrol;

	return 0;
}

  
                                    
  
                                                                             
                                                            
                                                                
              
  
                                                       
                                           
                   
                                             
                            
  
                       
  
                    
                                                      
                                       
   
int
posix_spawnattr_getprocesstype_np(const posix_spawnattr_t * __restrict attr,
    int * __restrict proctype)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*proctype = psattr->psa_apptype;

	return 0;
}
  
                                
  
                                                                         
                                                        
                                                      
  
                                                       
                                           
             
                                                       
                            
  
                       
   
int
posix_spawnattr_setsigdefault(posix_spawnattr_t * __restrict attr,
    const sigset_t * __restrict sigdefault)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_sigdefault = *sigdefault;

	return 0;
}


  
                            
  
                                                                          
                                                               
               
  
                                                           
                                             
          
                                     
  
                       
   
int
posix_spawnattr_setpgroup(posix_spawnattr_t * attr, pid_t pgroup)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_pgroup = pgroup;

	return 0;
}


  
                             
  
                                                                           
                                                             
                                    
  
                                                       
                                          
             
                                                    
                            
  
                       
   
int
posix_spawnattr_setsigmask(posix_spawnattr_t * __restrict attr,
    const sigset_t * __restrict sigmask)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_sigmask = *sigmask;

	return 0;
}


  
                                
  
                                                                            
                                                             
                                                          
  
                                                       
                                                                        
                                                              
                                                             
                                     
  
                                     
                                                  
  
                                                                      
                                                                             
                                                                  
   
int
posix_spawnattr_setbinpref_np(posix_spawnattr_t * __restrict attr,
    size_t count, cpu_type_t *pref, size_t * __restrict ocount)
{
	_posix_spawnattr_t psattr;
	int i = 0;

	if (attr == NULL || *attr == NULL || pref == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	for (i = 0; i < count && i < NBINPREFS; i++) {
		psattr->psa_binprefs[i] = pref[i];
		psattr->psa_subcpuprefs[i] = CPU_SUBTYPE_ANY;
	}

	                                           
	if (ocount) {
		*ocount = i;
	}

	for (; i < NBINPREFS; i++) {
		psattr->psa_binprefs[i] = 0;
		psattr->psa_subcpuprefs[i] = CPU_SUBTYPE_ANY;
	}

	return 0;
}

  
                                 
  
                                                                            
                                                             
                                                                  
                                               
  
                                                       
                                                                        
                                                              
                                                                 
                                                                          
                                     
  
                                     
                                                  
  
                                                        
                                               
                                                                             
                                                                  
   
int
posix_spawnattr_setarchpref_np(posix_spawnattr_t * __restrict attr,
    size_t count, cpu_type_t *pref, cpu_subtype_t *subpref,
    size_t * __restrict ocount)
{
	_posix_spawnattr_t psattr;
	int i = 0;

	if (attr == NULL || *attr == NULL || pref == NULL || subpref == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	for (i = 0; i < count && i < NBINPREFS; i++) {
		psattr->psa_binprefs[i] = pref[i];
		psattr->psa_subcpuprefs[i] = subpref[i];
	}

	                                           
	if (ocount) {
		*ocount = i;
	}

	for (; i < NBINPREFS; i++) {
		psattr->psa_binprefs[i] = 0;
		psattr->psa_subcpuprefs[i] = CPU_SUBTYPE_ANY;
	}

	return 0;
}

  
                                 
  
                                                             
                                                        
                                       
  
                                                       
                                           
             
                                                      
  
                       
   
int
posix_spawnattr_setpcontrol_np(posix_spawnattr_t * __restrict attr,
    const int pcontrol)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_pcontrol = pcontrol;

	return 0;
}


  
                                    
  
                                                                      
                                                        
                                       
  
                                                       
                                           
             
                                                   
  
                       
   
int
posix_spawnattr_setprocesstype_np(posix_spawnattr_t * __restrict attr,
    const int proctype)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_apptype = proctype;

	return 0;
}

  
                                   
                                                                     
                                    
   
static int
posix_spawn_createportactions_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_port_actions_t acts;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	acts = (_posix_spawn_port_actions_t)malloc(PS_PORT_ACTIONS_SIZE(2));
	if (acts == NULL) {
		return ENOMEM;
	}

	acts->pspa_alloc = 2;
	acts->pspa_count = 0;

	psattr->psa_ports = acts;
	return 0;
}

  
                                 
                                                            
   
static int
posix_spawn_growportactions_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_port_actions_t acts;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	acts = psattr->psa_ports;
	if (acts == NULL) {
		return EINVAL;
	}

	                                                 
	int newnum = 0;
	if (os_mul_overflow(acts->pspa_alloc, 2, &newnum)) {
		return ENOMEM;
	}
	size_t newsize = PS_PORT_ACTIONS_SIZE(newnum);
	if (newsize == 0) {
		return ENOMEM;
	}

	acts = realloc(acts, newsize);
	if (acts == NULL) {
		return ENOMEM;
	}

	acts->pspa_alloc = newnum;
	psattr->psa_ports = acts;
	return 0;
}

  
                                    
                                                                     
   
static int
posix_spawn_destroyportactions_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_port_actions_t acts;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	acts = psattr->psa_ports;
	if (acts == NULL) {
		return EINVAL;
	}

	free(acts);
	return 0;
}

  
                                       
                                                                        
   
static int
posix_spawn_destroycoalition_info_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_coalition_info *coal_info;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	coal_info = psattr->psa_coalition_info;
	if (coal_info == NULL) {
		return EINVAL;
	}

	psattr->psa_coalition_info = NULL;
	free(coal_info);
	return 0;
}

  
                                     
                                                                      
   
static int
posix_spawn_destroypersona_info_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_persona_info *persona;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	persona = psattr->psa_persona_info;
	if (persona == NULL) {
		return EINVAL;
	}

	psattr->psa_persona_info = NULL;
	free(persona);
	return 0;
}

  
                                        
                                                                         
   
static int
posix_spawn_destroyposix_cred_info_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_posix_cred_info *pspci;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	pspci = psattr->psa_posix_cred_info;
	if (pspci == NULL) {
		return EINVAL;
	}

	psattr->psa_posix_cred_info = NULL;
	free(pspci);
	return 0;
}

  
                                      
                                                                            
   
int
posix_spawnattr_set_subsystem_root_path_np(posix_spawnattr_t *attr, char *path)
{
	_posix_spawnattr_t psattr;
	char * buf = NULL;
	char * old_buf;
	size_t bytes;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	if (path) {
		buf = malloc(MAXPATHLEN);

		if (buf == NULL) {
			return ENOMEM;
		}

		bytes = strlcpy(buf, path, MAXPATHLEN);

		if (bytes >= MAXPATHLEN) {
			free(buf);
			return ENAMETOOLONG;
		}
	}

	old_buf = psattr->psa_subsystem_root_path;
	psattr->psa_subsystem_root_path = buf;

	free(old_buf);

	return 0;
}

  
                                             
                                                                             
   
static int
posix_spawn_destroysubsystem_root_path_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	char * subsystem_root_path;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	subsystem_root_path = psattr->psa_subsystem_root_path;

	if (subsystem_root_path == NULL) {
		return EINVAL;
	}

	psattr->psa_subsystem_root_path = NULL;
	free(subsystem_root_path);
	return 0;
}

  
                                  
                                                           
  
                     
   
int
posix_spawnattr_set_platform_np(posix_spawnattr_t *attr, int platform, uint32_t flags)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_platform = platform;

	(void)flags;
	return 0;
}

  
                                             
                                                       
   
int
posix_spawnattr_disable_ptr_auth_a_keys_np(posix_spawnattr_t *attr, uint32_t flags)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	psattr->psa_options |= PSA_OPTION_PLUGIN_HOST_DISABLE_A_KEYS;
	(void)flags;
	return 0;
}

  
                                  
                                                                 
   
static int
posix_spawn_appendportaction_np(posix_spawnattr_t *attr, _ps_port_action_t *act)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_port_actions_t acts;

	if (attr == NULL || *attr == NULL || act == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	acts = psattr->psa_ports;

	                                          
	if (acts == NULL) {
		int err = posix_spawn_createportactions_np(attr);
		if (err) {
			return err;
		}
		acts = psattr->psa_ports;
	}

	                        
	if (acts->pspa_alloc == acts->pspa_count) {
		int err = posix_spawn_growportactions_np(attr);
		if (err) {
			return err;
		}
		acts = psattr->psa_ports;
	}

	                                        
	acts->pspa_actions[acts->pspa_count] = *act;
	acts->pspa_count++;

	return 0;
}

  
                                    
  
                                                                            
  
                                                         
                                                   
                                                            
                                                     
                                                                               
  
                       
                                                 
   
int
posix_spawnattr_setspecialport_np(
	posix_spawnattr_t *attr,
	mach_port_t      new_port,
	int             which)
{
	_ps_port_action_t action = {
		.port_type = PSPA_SPECIAL,
		.new_port = new_port,
		.which = which,
	};
	return posix_spawn_appendportaction_np(attr, &action);
}

  
                                     
  
                                                                                 
  
                                                         
                                                   
                                                
  
                       
   
int
posix_spawnattr_setsuidcredport_np(posix_spawnattr_t *attr, mach_port_t port)
{
	_ps_port_action_t action = {
		.port_type = PSPA_SUID_CRED,
		.new_port = port,
	};
	return posix_spawn_appendportaction_np(attr, &action);
}

  
                                       
  
                                                                                
  
                                                         
                                                   
                                                             
                                                                  
                                                              
                                                           
                                                        
                                                                      
  
                       
   
int
posix_spawnattr_setexceptionports_np(
	posix_spawnattr_t       *attr,
	exception_mask_t        mask,
	mach_port_t              new_port,
	exception_behavior_t    behavior,
	thread_state_flavor_t   flavor)
{
	_ps_port_action_t action = {
		.port_type = PSPA_EXCEPTION,
		.mask = mask,
		.new_port = new_port,
		.behavior = behavior,
		.flavor = flavor,
	};
	return posix_spawn_appendportaction_np(attr, &action);
}

  
                                         
  
                                                                                
                                                                  
                 
  
                                                         
                                                   
                                                                 
  
                       
   
int
posix_spawnattr_setauditsessionport_np(
	posix_spawnattr_t       *attr,
	mach_port_t              au_sessionport)
{
	_ps_port_action_t action = {
		.port_type = PSPA_AU_SESSION,
		.new_port = au_sessionport,
	};
	return posix_spawn_appendportaction_np(attr, &action);
}


  
                                
  
                                                                               
  
                                                                
                  
  
                       
                                          
                                        
              
  
                                                                  
                                                             
                                                          
                                                              
                                  
  
                                                                 
                                                             
                                                                  
                                                              
                                                        
                                                       
   
int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions)
{
	_posix_spawn_file_actions_t *psactsp = (_posix_spawn_file_actions_t *)file_actions;
	int     err = 0;

	if ((*psactsp = (_posix_spawn_file_actions_t)malloc(PSF_ACTIONS_SIZE(PSF_ACTIONS_INIT_COUNT))) == NULL) {
		err = ENOMEM;
	} else {
		(*psactsp)->psfa_act_alloc = PSF_ACTIONS_INIT_COUNT;
		(*psactsp)->psfa_act_count = 0;
	}

	return err;
}


  
                                   
  
                                                                       
                                                                  
                                                                 
  
                                                         
                 
  
                       
  
                                                                        
                                                              
                                                               
                           
  
                                                    
                                                          
   
int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions)
{
	_posix_spawn_file_actions_t psacts;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psacts = *(_posix_spawn_file_actions_t *)file_actions;
	free(psacts);
	*file_actions = NULL;

	return 0;
}


  
                                 
  
                                                                           
                                                                  
                             
  
                                                               
              
  
                       
                                              
  
                                                                 
                                                                  
                                                             
   
static int
_posix_spawn_file_actions_grow(_posix_spawn_file_actions_t *psactsp)
{
	int newnum = 0;
	if (os_mul_overflow((*psactsp)->psfa_act_alloc, 2, &newnum)) {
		return ENOMEM;
	}

	size_t newsize = PSF_ACTIONS_SIZE(newnum);
	if (newsize == 0) {
		return ENOMEM;
	}

	  
                                                                   
                                                                    
                                                                 
                                    
    
	_posix_spawn_file_actions_t new_psacts;
	if ((new_psacts = (_posix_spawn_file_actions_t)realloc((*psactsp), newsize)) == NULL) {
		return ENOMEM;
	}
	new_psacts->psfa_act_alloc = newnum;
	*psactsp = new_psacts;

	return 0;
}


  
                                   
  
                                                                             
                                                                  
                                                                  
                                                          
  
                                                          
                                    
                               
                           
                         
  
                       
                                            
                                           
                  
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_addopen(
	posix_spawn_file_actions_t * __restrict file_actions,
	int filedes, const char * __restrict path, int oflag,
	mode_t mode)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                    
	if (filedes < 0 || filedes >= OPEN_MAX) {
		return EBADF;
	}

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_OPEN;
	psfileact->psfaa_filedes = filedes;
	psfileact->psfaa_openargs.psfao_oflag = oflag;
	psfileact->psfaa_openargs.psfao_mode = mode;
	strlcpy(psfileact->psfaa_openargs.psfao_path, path, PATH_MAX);

	return 0;
}


  
                                    
  
                                                                             
                                                          
                                                  
  
                                                          
                         
  
                       
                                            
                                           
                  
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
    int filedes)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                    
	if (filedes < 0 || filedes >= OPEN_MAX) {
		return EBADF;
	}

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_CLOSE;
	psfileact->psfaa_filedes = filedes;

	return 0;
}


  
                                   
  
                                                                            
                                                          
                                                                 
                    
  
                                                          
                        
                                
  
                       
                                                
                                              
                                   
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
    int filedes, int newfiledes)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                    
	if (filedes < 0 || filedes >= OPEN_MAX ||
	    newfiledes < 0 || newfiledes >= OPEN_MAX) {
		return EBADF;
	}

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_DUP2;
	psfileact->psfaa_filedes = filedes;
	psfileact->psfaa_dup2args.psfad_newfiledes = newfiledes;

	return 0;
}

  
                                               
  
                                                                            
                                                           
                                                                 
                    
  
                                                          
                              
                                
  
                       
                                               
                                       
                                           
                  
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_add_fileportdup2_np(
	posix_spawn_file_actions_t *file_actions,
	mach_port_t fileport, int newfiledes)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                    
	if (!MACH_PORT_VALID(fileport) ||
	    newfiledes < 0 || newfiledes >= OPEN_MAX) {
		return EBADF;
	}

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_FILEPORT_DUP2;
	psfileact->psfaa_fileport = fileport;
	psfileact->psfaa_dup2args.psfad_newfiledes = newfiledes;

	return 0;
}

  
                                         
  
                                                                    
                                                         
                                                        
                                    
  
                                                   
                                                      
                                                      
                                                     
                                                  
                                                        
                                                         
                                                
  
                                                        
                                                             
  
                                                          
                            
  
                       
                                            
                                           
                  
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_addinherit_np(posix_spawn_file_actions_t *file_actions,
    int filedes)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                    
	if (filedes < 0 || filedes >= OPEN_MAX) {
		return EBADF;
	}

#if defined(POSIX_SPAWN_CLOEXEC_DEFAULT)                                  
	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_INHERIT;
	psfileact->psfaa_filedes = filedes;
#endif
	return 0;
}


  
                                       
  
                                                                             
                                                                          
                                                            
  
                                                          
                                                
  
                       
                                                 
                                     
                                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_addchdir_np(
	posix_spawn_file_actions_t * __restrict file_actions,
	const char * __restrict path)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_CHDIR;
	if (strlcpy(psfileact->psfaa_chdirargs.psfac_path, path, PATH_MAX) >= PATH_MAX) {
		(*psactsp)->psfa_act_count--;
		return ENAMETOOLONG;
	}

	return 0;
}


  
                                     
  
                                                                              
                                                                          
                                                                              
  
                                                          
                            
  
                       
                                                               
                                                                    
                                                 
                                     
  
                                                    
                                                          
   
int
posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t *file_actions,
    int filedes)
{
	_posix_spawn_file_actions_t *psactsp;
	_psfa_action_t *psfileact;

	if (file_actions == NULL || *file_actions == NULL) {
		return EINVAL;
	}

	psactsp = (_posix_spawn_file_actions_t *)file_actions;
	                                     
	if (filedes < 0 || filedes >= OPEN_MAX) {
		return EBADF;
	}

	                                                        
	if ((*psactsp)->psfa_act_count == (*psactsp)->psfa_act_alloc) {
		                                         
		if (_posix_spawn_file_actions_grow(psactsp)) {
			return ENOMEM;
		}
	}

	  
                                                
    
	psfileact = &(*psactsp)->psfa_act_acts[(*psactsp)->psfa_act_count++];

	psfileact->psfaa_type = PSFA_FCHDIR;
	psfileact->psfaa_filedes = filedes;

	return 0;
}

int
posix_spawnattr_setcpumonitor_default(posix_spawnattr_t * __restrict attr)
{
	return posix_spawnattr_setcpumonitor(attr, PROC_POLICY_CPUMON_DEFAULTS, 0);
}

int
posix_spawnattr_setcpumonitor(posix_spawnattr_t * __restrict attr,
    uint64_t percent, uint64_t interval)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	psattr->psa_cpumonitor_percent = percent;
	psattr->psa_cpumonitor_interval = interval;

	return 0;
}

int
posix_spawnattr_getcpumonitor(posix_spawnattr_t * __restrict attr,
    uint64_t *percent, uint64_t *interval)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	*percent = psattr->psa_cpumonitor_percent;
	*interval = psattr->psa_cpumonitor_interval;

	return 0;
}

#if (TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
  
                            
  
                                                                    
                          
  
                                             
                                      
                                                    
                                                  
                                               
  
                       
  
                                                    
  
   
int
posix_spawnattr_setjetsam(posix_spawnattr_t * __restrict attr,
    short flags, int priority, int memlimit)
{
	short flags_ext = flags;

	if (flags & POSIX_SPAWN_JETSAM_MEMLIMIT_FATAL) {
		flags_ext |= POSIX_SPAWN_JETSAM_MEMLIMIT_ACTIVE_FATAL;
		flags_ext |= POSIX_SPAWN_JETSAM_MEMLIMIT_INACTIVE_FATAL;
	} else {
		flags_ext &= ~POSIX_SPAWN_JETSAM_MEMLIMIT_ACTIVE_FATAL;
		flags_ext &= ~POSIX_SPAWN_JETSAM_MEMLIMIT_INACTIVE_FATAL;
	}

	return posix_spawnattr_setjetsam_ext(attr, flags_ext, priority, memlimit, memlimit);
}
#endif                                                 

  
                                
  
                                                                    
                          
  
                                             
                                      
                                                         
                                        
                                        
                                                          
                                        
                                          
  
                       
   
int
posix_spawnattr_setjetsam_ext(posix_spawnattr_t * __restrict attr,
    short flags, int priority, int memlimit_active, int memlimit_inactive)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	psattr->psa_jetsam_flags = flags;
	psattr->psa_jetsam_flags |= POSIX_SPAWN_JETSAM_SET;
	psattr->psa_priority = priority;
	psattr->psa_memlimit_active = memlimit_active;
	psattr->psa_memlimit_inactive = memlimit_inactive;

	return 0;
}

int
posix_spawnattr_set_threadlimit_ext(posix_spawnattr_t * __restrict attr,
    int thread_limit)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	psattr->psa_thread_limit = thread_limit;

	return 0;
}


  
                                              
  
                                                      
                                                                   
                                                                     
                                                                          
                                                                                            
                                                                                    
  
                                                                                                          
  
                                                           
                                               
  
                               
                                      
                                                            
                                                      
   
int
posix_spawnattr_set_importancewatch_port_np(posix_spawnattr_t * __restrict attr,
    int count, mach_port_t portarray[])
{
	int err = 0, i;

	if (count < 0 || count > POSIX_SPAWN_IMPORTANCE_PORT_COUNT) {
		return EINVAL;
	}

	for (i = 0; i < count; i++) {
		_ps_port_action_t action = {
			.port_type = PSPA_IMP_WATCHPORTS,
			.new_port = portarray[i],
		};
		err = posix_spawn_appendportaction_np(attr, &action);
		if (err) {
			break;
		}
	}
	return err;
}

int
posix_spawnattr_set_registered_ports_np(posix_spawnattr_t * __restrict attr,
    mach_port_t portarray[], uint32_t count)
{
	int err = 0;

	if (count > TASK_PORT_REGISTER_MAX) {
		return EINVAL;
	}

	for (uint32_t i = 0; i < count; i++) {
		_ps_port_action_t action = {
			.port_type = PSPA_REGISTERED_PORTS,
			.new_port = portarray[i],
		};
		err = posix_spawn_appendportaction_np(attr, &action);
		if (err) {
			break;
		}
	}
	return err;
}

int
posix_spawnattr_set_ptrauth_task_port_np(posix_spawnattr_t * __restrict attr,
    mach_port_t port)
{
	int err = 0;

	_ps_port_action_t action = {
		.port_type = PSPA_PTRAUTH_TASK_PORT,
		.new_port = port,
	};

	err = posix_spawn_appendportaction_np(attr, &action);
	return err;
}

static
_ps_mac_policy_extension_t *
posix_spawnattr_macpolicyinfo_lookup(_posix_spawn_mac_policy_extensions_t psmx, const char *policyname)
{
	int i;

	if (psmx == NULL) {
		return NULL;
	}

	for (i = 0; i < psmx->psmx_count; i++) {
		_ps_mac_policy_extension_t *extension = &psmx->psmx_extensions[i];
		if (strcmp(extension->policyname, policyname) == 0) {
			return extension;
		}
	}
	return NULL;
}

int
posix_spawnattr_getmacpolicyinfo_np(const posix_spawnattr_t * __restrict attr,
    const char *policyname, void **datap, size_t *datalenp)
{
	_posix_spawnattr_t psattr;
	_ps_mac_policy_extension_t *extension;

	if (attr == NULL || *attr == NULL || policyname == NULL || datap == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	extension = posix_spawnattr_macpolicyinfo_lookup(psattr->psa_mac_extensions, policyname);
	if (extension == NULL) {
		return ESRCH;
	}
	*datap = (void *)(uintptr_t)extension->data;
	if (datalenp != NULL) {
		*datalenp = (size_t)extension->datalen;
	}
	return 0;
}

int
posix_spawnattr_setmacpolicyinfo_np(posix_spawnattr_t * __restrict attr,
    const char *policyname, void *data, size_t datalen)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_mac_policy_extensions_t psmx;
	_ps_mac_policy_extension_t *extension;

	if (attr == NULL || *attr == NULL || policyname == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psmx = psattr->psa_mac_extensions;
	extension = posix_spawnattr_macpolicyinfo_lookup(psattr->psa_mac_extensions, policyname);
	if (extension != NULL) {
		extension->data = (uintptr_t)data;
		extension->datalen = datalen;
		return 0;
	} else if (psmx == NULL) {
		psmx = psattr->psa_mac_extensions = malloc(PS_MAC_EXTENSIONS_SIZE(PS_MAC_EXTENSIONS_INIT_COUNT));
		if (psmx == NULL) {
			return ENOMEM;
		}
		psmx->psmx_alloc = PS_MAC_EXTENSIONS_INIT_COUNT;
		psmx->psmx_count = 0;
	} else if (psmx->psmx_count == psmx->psmx_alloc) {
		int newnum = 0;
		if (os_mul_overflow(psmx->psmx_alloc, 2, &newnum)) {
			return ENOMEM;
		}
		size_t extsize = PS_MAC_EXTENSIONS_SIZE(newnum);
		if (extsize == 0) {
			return ENOMEM;
		}
		psmx = psattr->psa_mac_extensions = reallocf(psmx, extsize);
		if (psmx == NULL) {
			return ENOMEM;
		}
		psmx->psmx_alloc = newnum;
	}
	extension = &psmx->psmx_extensions[psmx->psmx_count];
	strlcpy(extension->policyname, policyname, sizeof(extension->policyname));
	extension->data = (uintptr_t)data;
	extension->datalen = datalen;
	psmx->psmx_count += 1;
	return 0;
}

  
                                       
                                                                      
   
static int
posix_spawn_destroymacpolicy_info_np(posix_spawnattr_t *attr)
{
	_posix_spawnattr_t psattr;
	_posix_spawn_mac_policy_extensions_t psmx;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psmx = psattr->psa_mac_extensions;
	if (psmx == NULL) {
		return EINVAL;
	}

	psattr->psa_mac_extensions = NULL;
	free(psmx);
	return 0;
}

int
posix_spawnattr_setcoalition_np(const posix_spawnattr_t * __restrict attr,
    uint64_t coalitionid, int type, int role)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_coalition_info *coal_info;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}
	if (type < 0 || type > COALITION_TYPE_MAX) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	coal_info = psattr->psa_coalition_info;
	if (!coal_info) {
		coal_info = (struct _posix_spawn_coalition_info *)malloc(sizeof(*coal_info));
		if (!coal_info) {
			return ENOMEM;
		}
		memset(coal_info, 0, sizeof(*coal_info));
		psattr->psa_coalition_info = coal_info;
	}

	coal_info->psci_info[type].psci_id   = coalitionid;
	coal_info->psci_info[type].psci_role = role;

	return 0;
}


int
posix_spawnattr_set_qos_clamp_np(const posix_spawnattr_t * __restrict attr, uint64_t qos_clamp)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	if (qos_clamp >= POSIX_SPAWN_PROC_CLAMP_LAST) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_qos_clamp = qos_clamp;

	return 0;
}

int
posix_spawnattr_get_qos_clamp_np(const posix_spawnattr_t * __restrict attr, uint64_t * __restrict qos_clampp)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*qos_clampp = psattr->psa_qos_clamp;

	return 0;
}

int
posix_spawnattr_set_darwin_role_np(const posix_spawnattr_t * __restrict attr, uint64_t darwin_role)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_darwin_role = darwin_role;

	return 0;
}

int
posix_spawnattr_get_darwin_role_np(const posix_spawnattr_t * __restrict attr, uint64_t * __restrict darwin_rolep)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	*darwin_rolep = psattr->psa_darwin_role;

	return 0;
}


int
posix_spawnattr_set_persona_np(const posix_spawnattr_t * __restrict attr, uid_t persona_id, uint32_t flags)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_persona_info *persona;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	if (flags & ~POSIX_SPAWN_PERSONA_ALL_FLAGS) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	persona = psattr->psa_persona_info;
	if (!persona) {
		persona = (struct _posix_spawn_persona_info *)malloc(sizeof(*persona));
		if (!persona) {
			return ENOMEM;
		}
		persona->pspi_uid = 0;
		persona->pspi_gid = 0;
		persona->pspi_ngroups = 0;
		persona->pspi_groups[0] = 0;
		persona->pspi_gmuid = 0;

		psattr->psa_persona_info = persona;
	}

	persona->pspi_id = persona_id;
	persona->pspi_flags = flags;

	return 0;
}

int
posix_spawnattr_set_persona_uid_np(const posix_spawnattr_t * __restrict attr, uid_t uid)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_persona_info *persona;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	persona = psattr->psa_persona_info;
	if (!persona) {
		return EINVAL;
	}

	if (!(persona->pspi_flags & (POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE | POSIX_SPAWN_PERSONA_FLAGS_VERIFY))) {
		return EINVAL;
	}

	persona->pspi_uid = uid;

	persona->pspi_flags |= POSIX_SPAWN_PERSONA_UID;

	return 0;
}

int
posix_spawnattr_set_persona_gid_np(const posix_spawnattr_t * __restrict attr, gid_t gid)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_persona_info *persona;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	persona = psattr->psa_persona_info;
	if (!persona) {
		return EINVAL;
	}

	if (!(persona->pspi_flags & (POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE | POSIX_SPAWN_PERSONA_FLAGS_VERIFY))) {
		return EINVAL;
	}

	persona->pspi_gid = gid;

	persona->pspi_flags |= POSIX_SPAWN_PERSONA_GID;

	return 0;
}

int
posix_spawnattr_set_persona_groups_np(const posix_spawnattr_t * __restrict attr, int ngroups, gid_t *gidarray, uid_t gmuid)
{
	_posix_spawnattr_t psattr;
	struct _posix_spawn_persona_info *persona;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	if (gidarray == NULL) {
		return EINVAL;
	}

	if (ngroups > NGROUPS || ngroups < 0) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	persona = psattr->psa_persona_info;
	if (!persona) {
		return EINVAL;
	}

	if (!(persona->pspi_flags & (POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE | POSIX_SPAWN_PERSONA_FLAGS_VERIFY))) {
		return EINVAL;
	}

	persona->pspi_ngroups = ngroups;
	for (int i = 0; i < ngroups; i++) {
		persona->pspi_groups[i] = gidarray[i];
	}

	persona->pspi_gmuid = gmuid;

	persona->pspi_flags |= POSIX_SPAWN_PERSONA_GROUPS;

	return 0;
}

int
posix_spawnattr_set_max_addr_np(const posix_spawnattr_t * __restrict attr, uint64_t max_addr)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_max_addr = max_addr;

	return 0;
}

int
posix_spawnattr_setnosmt_np(const posix_spawnattr_t * __restrict attr)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;
	psattr->psa_no_smt = true;

	return 0;
}

int
posix_spawnattr_set_csm_np(const posix_spawnattr_t * __restrict attr, uint32_t flags)
{
	_posix_spawnattr_t psattr;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	const uint32_t mask = POSIX_SPAWN_NP_CSM_ALL | POSIX_SPAWN_NP_CSM_TECS | POSIX_SPAWN_NP_CSM_NOSMT;
	if ((flags & ~mask) != 0) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	if (flags & (POSIX_SPAWN_NP_CSM_TECS | POSIX_SPAWN_NP_CSM_ALL)) {
		psattr->psa_tecs = true;
	}
	if (flags & (POSIX_SPAWN_NP_CSM_NOSMT | POSIX_SPAWN_NP_CSM_ALL)) {
		psattr->psa_no_smt = true;
	}

	return 0;
}

static struct _posix_spawn_posix_cred_info *
_posix_spawnattr_get_posix_creds_info(_posix_spawnattr_t psattr)
{
	struct _posix_spawn_posix_cred_info *pspci = psattr->psa_posix_cred_info;

	if (pspci == NULL) {
		pspci = malloc(sizeof(struct _posix_spawn_posix_cred_info));
		if (pspci != NULL) {
			pspci->pspci_flags = 0;
			pspci->pspci_uid = 0;
			pspci->pspci_gid = 0;
			pspci->pspci_ngroups = 0;
			pspci->pspci_groups[0] = 0;
			pspci->pspci_gmuid = 0;
			pspci->pspci_login[0] = '\0';
			psattr->psa_posix_cred_info = pspci;
		}
	}
	return pspci;
}

int
posix_spawnattr_set_uid_np(const posix_spawnattr_t *attr, uid_t uid)
{
	struct _posix_spawn_posix_cred_info *pspci;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	pspci = _posix_spawnattr_get_posix_creds_info(*(_posix_spawnattr_t *)attr);
	if (pspci == NULL) {
		return ENOMEM;
	}

	pspci->pspci_uid = uid;

	pspci->pspci_flags |= POSIX_SPAWN_POSIX_CRED_UID;

	return 0;
}

int
posix_spawnattr_set_gid_np(const posix_spawnattr_t *attr, gid_t gid)
{
	struct _posix_spawn_posix_cred_info *pspci;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	pspci = _posix_spawnattr_get_posix_creds_info(*(_posix_spawnattr_t *)attr);
	if (pspci == NULL) {
		return ENOMEM;
	}

	pspci->pspci_gid = gid;

	pspci->pspci_flags |= POSIX_SPAWN_POSIX_CRED_GID;

	return 0;
}

int
posix_spawnattr_set_groups_np(const posix_spawnattr_t *attr,
    int ngroups, gid_t *gidarray, uid_t gmuid)
{
	struct _posix_spawn_posix_cred_info *pspci;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	if (gidarray == NULL) {
		return EINVAL;
	}

	if (ngroups > NGROUPS || ngroups < 0) {
		return EINVAL;
	}

	pspci = _posix_spawnattr_get_posix_creds_info(*(_posix_spawnattr_t *)attr);
	if (pspci == NULL) {
		return ENOMEM;
	}

	pspci->pspci_ngroups = ngroups;
	for (int i = 0; i < ngroups; i++) {
		pspci->pspci_groups[i] = gidarray[i];
	}

	pspci->pspci_gmuid = gmuid;

	pspci->pspci_flags |= POSIX_SPAWN_POSIX_CRED_GROUPS;

	return 0;
}

int
posix_spawnattr_set_login_np(const posix_spawnattr_t *attr, const char *login)
{
	struct _posix_spawn_posix_cred_info *pspci;

	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	if (strlen(login) > MAXLOGNAME) {
		return ERANGE;
	}

	pspci = _posix_spawnattr_get_posix_creds_info(*(_posix_spawnattr_t *)attr);
	if (pspci == NULL) {
		return ENOMEM;
	}

	strlcpy(pspci->pspci_login, login, sizeof(pspci->pspci_login));

	pspci->pspci_flags |= POSIX_SPAWN_POSIX_CRED_LOGIN;

	return 0;
}

  
                                    
  
                                                                                            
                                                                                            
                                                                                            
                                                               
  
                                                                         
                                                                       
  
                               
                                                            
   
int
posix_spawnattr_set_jetsam_ttr_np(const posix_spawnattr_t * __restrict attr, uint32_t count, uint32_t *ttrs_millis)
{
	_posix_spawnattr_t psattr;

	  
                                                                                      
                                                                      
    
	static const uint32_t relaunch_buckets_msecs[POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_BUCKETS] = {
		5000,
		10000,
		UINT32_MAX
	};
	static const uint32_t relaunch_jetsam_flags[POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_BUCKETS] = {
		POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_HIGH,
		POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MED,
		POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_LOW
	};

	                                         
	if (attr == NULL || *attr == NULL) {
		return EINVAL;
	}

	                                                
	if (count == 0) {
		return EINVAL;
	}

	psattr = *(_posix_spawnattr_t *)attr;

	                                                                                                     
	int relaunch_histogram[POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_BUCKETS] = {0};
	int max_frequency = -1;
	int highest_frequency_bucket = -1;

	for (uint32_t i = 0; i < count; i++) {
		                                                                            
		for (uint32_t bucket = 0; bucket < POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_BUCKETS; bucket++) {
			if (ttrs_millis[i] <= relaunch_buckets_msecs[bucket]) {
				relaunch_histogram[bucket]++;

				                                                             
				if (relaunch_histogram[bucket] > max_frequency) {
					max_frequency = relaunch_histogram[bucket];
					highest_frequency_bucket = bucket;
				}
				break;
			}
		}
	}
	psattr->psa_jetsam_flags |= relaunch_jetsam_flags[highest_frequency_bucket];
	return 0;
}

  
              
  
                                                                            
                                 
  
                                                     
                                      
                                    
                                       
                                                  
                                    
                                  
                                          
                                  
                                
                                       
                  
                                          
                  
  
                        
                                       
                                     
  
                                                                    
                                                                
                                                   
   
extern int __posix_spawn(pid_t * __restrict, const char * __restrict,
    struct _posix_spawn_args_desc *,
    char *const argv[__restrict], char *const envp[__restrict]);

int
posix_spawn(pid_t * __restrict pid, const char * __restrict path,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t * __restrict attrp,
    char *const argv[__restrict], char *const envp[__restrict])
{
	int saveerrno = errno;
	int ret;
	  
                                                                    
                                                                      
                                                                      
                                                                     
                                                                        
   
                                                            
                                                          
                                                         
                                                         
                            
    
	if ((file_actions != NULL && (*file_actions != NULL) && (*(_posix_spawn_file_actions_t *)file_actions)->psfa_act_count > 0) || attrp != NULL) {
		struct _posix_spawn_args_desc   ad;

		memset(&ad, 0, sizeof(ad));
		if (attrp != NULL && *attrp != NULL) {
			_posix_spawnattr_t psattr = *(_posix_spawnattr_t *)attrp;
			ad.attr_size = sizeof(struct _posix_spawnattr);
			ad.attrp = psattr;

			if (psattr->psa_ports != NULL) {
				size_t psact_size = PS_PORT_ACTIONS_SIZE(psattr->psa_ports->pspa_count);
				if (psact_size == 0 && psattr->psa_ports->pspa_count != 0) {
					errno = EINVAL;
					ret = -1;
					goto out;
				}
				ad.port_actions = psattr->psa_ports;
				ad.port_actions_size = psact_size;
			}
			if (psattr->psa_mac_extensions != NULL) {
				size_t macext_size = PS_MAC_EXTENSIONS_SIZE(psattr->psa_mac_extensions->psmx_count);
				if (macext_size == 0 && psattr->psa_mac_extensions->psmx_count != 0) {
					errno = EINVAL;
					ret = -1;
					goto out;
				}
				ad.mac_extensions = psattr->psa_mac_extensions;
				ad.mac_extensions_size = macext_size;
			}
			if (psattr->psa_coalition_info != NULL) {
				ad.coal_info_size = sizeof(struct _posix_spawn_coalition_info);
				ad.coal_info = psattr->psa_coalition_info;
			}
			if (psattr->psa_persona_info != NULL) {
				ad.persona_info_size = sizeof(struct _posix_spawn_persona_info);
				ad.persona_info = psattr->psa_persona_info;
			}
			if (psattr->psa_posix_cred_info != NULL) {
				ad.posix_cred_info_size = sizeof(struct _posix_spawn_posix_cred_info);
				ad.posix_cred_info = psattr->psa_posix_cred_info;
			}
			if (psattr->psa_subsystem_root_path != NULL) {
				ad.subsystem_root_path_size = MAXPATHLEN;
				ad.subsystem_root_path = psattr->psa_subsystem_root_path;
			}
		}
		if (file_actions != NULL && *file_actions != NULL) {
			_posix_spawn_file_actions_t psactsp =
			    *(_posix_spawn_file_actions_t *)file_actions;

			if (psactsp->psfa_act_count > 0) {
				size_t fa_size = PSF_ACTIONS_SIZE(psactsp->psfa_act_count);
				if (fa_size == 0 && psactsp->psfa_act_count != 0) {
					errno = EINVAL;
					ret = -1;
					goto out;
				}
				ad.file_actions_size = fa_size;
				ad.file_actions = psactsp;
			}
		}

		ret = __posix_spawn(pid, path, &ad, argv, envp);
	} else {
		ret = __posix_spawn(pid, path, NULL, argv, envp);
	}

out:
	if (ret < 0) {
		ret = errno;
	}
	errno = saveerrno;
	return ret;
}
