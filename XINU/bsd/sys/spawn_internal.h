/*
 * Copyright (c) 2006 Apple Computer, Inc. All rights reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */


  
                                 
  
                                                                               
                                                                        
                                                                        
                                                                             
                                                                       
   

#ifndef _SYS_SPAWN_INTERNAL_H_
#define _SYS_SPAWN_INTERNAL_H_

#include <sys/_types.h>                                            
#include <sys/param.h>
#include <sys/syslimits.h>                    
#include <sys/spawn.h>
#include <mach/machine.h>
#include <mach/port.h>
#include <mach/exception_types.h>
#include <mach/coalition.h>                              
#include <mach/task_policy.h>
#include <os/overflow.h>

  
                                                                       
                                                                     
  
                                                                          
   
#define PS_ACTION_SIZE(x, _type, _member_type) ({\
	size_t _ps_count = (size_t)x; \
	size_t _ps_size = 0; \
	                                                     \
	if (os_mul_and_add_overflow(_ps_count, \
	                            sizeof(_member_type), \
	                            sizeof(_type), \
	                            &_ps_size)) { \
	        _ps_size = 0; \
	} \
	_ps_size; })

  
                                            
   
typedef enum {
	PSPA_SPECIAL = 0,
	PSPA_EXCEPTION = 1,
	PSPA_AU_SESSION = 2,
	PSPA_IMP_WATCHPORTS = 3,
	PSPA_REGISTERED_PORTS = 4,
	PSPA_PTRAUTH_TASK_PORT = 5,
	PSPA_SUID_CRED = 6,
} pspa_t;

  
                                                                  
                                                                    
                                                         
   
typedef struct _ps_port_action {
	pspa_t                  port_type;
	exception_mask_t        mask;
	mach_port_name_t        new_port;
	exception_behavior_t    behavior;
	thread_state_flavor_t   flavor;
	int                     which;
} _ps_port_action_t;

  
                                                                     
   
typedef struct _posix_spawn_port_actions {
	int                     pspa_alloc;
	int                     pspa_count;
	_ps_port_action_t       pspa_actions[];
} *_posix_spawn_port_actions_t;

  
                                                                           
   
#define PS_PORT_ACTIONS_SIZE(x) \
	PS_ACTION_SIZE(x, struct _posix_spawn_port_actions, _ps_port_action_t)

#define NBINPREFS       4

  
                                                                      
   
typedef struct _ps_mac_policy_extension {
	char                    policyname[128];
	union {
		uint64_t        data;
		void            *datap;                                       
	};
	uint64_t                datalen;
} _ps_mac_policy_extension_t;

  
                                                                                   
   
typedef struct _posix_spawn_mac_policy_extensions {
	int                     psmx_alloc;
	int                     psmx_count;
	_ps_mac_policy_extension_t psmx_extensions[];
} *_posix_spawn_mac_policy_extensions_t;

  
                                                                                    
   
#define PS_MAC_EXTENSIONS_SIZE(x)     \
	PS_ACTION_SIZE(x, struct _posix_spawn_mac_policy_extensions, _ps_mac_policy_extension_t)

#define PS_MAC_EXTENSIONS_INIT_COUNT    2

  
                                   
   
struct _posix_spawn_coalition_info {
	struct {
		uint64_t psci_id;
		uint32_t psci_role;
		uint32_t psci_reserved1;
		uint64_t psci_reserved2;
	} psci_info[COALITION_NUM_TYPES];
};

  
                     
   
struct _posix_spawn_posix_cred_info {
	uint32_t pspci_flags;                             
	uid_t    pspci_uid;                                     
	gid_t    pspci_gid;                                    
	uint32_t pspci_ngroups;                                 
	gid_t    pspci_groups[NGROUPS];
	uid_t    pspci_gmuid;                              
	char     pspci_login[MAXLOGNAME + 1];
};

#define POSIX_SPAWN_POSIX_CRED_UID          0x00010000
#define POSIX_SPAWN_POSIX_CRED_GID          0x00020000
#define POSIX_SPAWN_POSIX_CRED_GROUPS       0x00040000
#define POSIX_SPAWN_POSIX_CRED_LOGIN        0x00080000

  
                     
   
struct _posix_spawn_persona_info {
	uid_t    pspi_id;                                  
	uint32_t pspi_flags;                             
	uid_t    pspi_uid;                                     
	gid_t    pspi_gid;                                    
	uint32_t pspi_ngroups;                                 
	gid_t    pspi_groups[NGROUPS];
	uid_t    pspi_gmuid;                              
};

#define POSIX_SPAWN_PERSONA_FLAGS_NONE      0x0
#define POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE  0x1
#define POSIX_SPAWN_PERSONA_FLAGS_VERIFY    0x2

#define POSIX_SPAWN_PERSONA_ALL_FLAGS \
	(POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE \
	 | POSIX_SPAWN_PERSONA_FLAGS_VERIFY \
	)

#define POSIX_SPAWN_PERSONA_UID             POSIX_SPAWN_POSIX_CRED_UID
#define POSIX_SPAWN_PERSONA_GID             POSIX_SPAWN_POSIX_CRED_GID
#define POSIX_SPAWN_PERSONA_GROUPS          POSIX_SPAWN_POSIX_CRED_GROUPS


  
                                                                          
                                                                         
                                                                           
                                                        
  
                                                                       
            
   

typedef struct _posix_spawnattr {
	short           psa_flags;                                         
	short           flags_padding;                                       
	sigset_t        psa_sigdefault;                                    
	sigset_t        psa_sigmask;                                    
	pid_t           psa_pgroup;                                       
	cpu_type_t      psa_binprefs[NBINPREFS];                          
	int             psa_pcontrol;                                                            
	int             psa_apptype;                                                 
	uint64_t        psa_cpumonitor_percent;                                   
	uint64_t        psa_cpumonitor_interval;                                             
	uint64_t        psa_reserved;

	short       psa_jetsam_flags;                             
	short           short_padding;                                            
	int         psa_priority;                                               
	int         psa_memlimit_active;                                                                
	int         psa_memlimit_inactive;                                                                

	uint64_t        psa_qos_clamp;                                                   
	task_role_t     psa_darwin_role;                                                           
	int             psa_thread_limit;                         

	uint64_t        psa_max_addr;                                     
	bool            psa_no_smt;
	bool            psa_tecs;
	int             psa_platform;                                       

	cpu_subtype_t      psa_subcpuprefs[NBINPREFS];                             
	uint32_t        psa_options;                                                           
	  
                                                                
                                                                          
                                 
    
	_posix_spawn_port_actions_t    psa_ports;                               
	_posix_spawn_mac_policy_extensions_t psa_mac_extensions;                                      
	struct _posix_spawn_coalition_info *psa_coalition_info;                      
	struct _posix_spawn_persona_info   *psa_persona_info;                                              
	struct _posix_spawn_posix_cred_info *psa_posix_cred_info;                                  
	char                                *psa_subsystem_root_path;                                       
} *_posix_spawnattr_t;

  
                                     
   
#define POSIX_SPAWN_JETSAM_SET                      0x8000

#define POSIX_SPAWN_JETSAM_USE_EFFECTIVE_PRIORITY       0x01
#define POSIX_SPAWN_JETSAM_HIWATER_BACKGROUND           0x02                        
#define POSIX_SPAWN_JETSAM_MEMLIMIT_FATAL               0x04                        

  
                                          
                                           
   
#define POSIX_SPAWN_JETSAM_MEMLIMIT_ACTIVE_FATAL        0x04                                                           
#define POSIX_SPAWN_JETSAM_MEMLIMIT_INACTIVE_FATAL      0x08                                                           


  
                                                          
                                                       
   
                                                  
#define POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_BUCKETS    (0x3)
#define POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MASK       (0x30)

                                           
#define POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_HIGH       (0x30)
#define POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MED        (0x20)
#define POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_LOW        (0x10)

  
                                          
  
                                              
                                              
                                              
                                              
                                              
                                              
                                              
   

  
                                            
  
                                                              
                                                              
                                                              
                                                              
                                                              
                                                              
  
                                                              
                                                              
                                                              
                                                              
   

  
                                                 
                                                                      
   
#define POSIX_SPAWN_PROCESS_TYPE_NORMAL             0x00000000
#define POSIX_SPAWN_PROCESS_TYPE_DEFAULT            POSIX_SPAWN_PROCESS_TYPE_NORMAL

#define POSIX_SPAWN_PROC_TYPE_MASK                  0x00000F00

#define POSIX_SPAWN_PROC_TYPE_APP_DEFAULT           0x00000100
#define POSIX_SPAWN_PROC_TYPE_APP_TAL               0x00000200             

#define POSIX_SPAWN_PROC_TYPE_DAEMON_STANDARD       0x00000300
#define POSIX_SPAWN_PROC_TYPE_DAEMON_INTERACTIVE    0x00000400
#define POSIX_SPAWN_PROC_TYPE_DAEMON_BACKGROUND     0x00000500
#define POSIX_SPAWN_PROC_TYPE_DAEMON_ADAPTIVE       0x00000600

#define POSIX_SPAWN_PROC_TYPE_DRIVER                0x00000700

#define POSIX_SPAWN_PROC_CLAMP_NONE                 0x00000000
#define POSIX_SPAWN_PROC_CLAMP_UTILITY              0x00000001
#define POSIX_SPAWN_PROC_CLAMP_BACKGROUND           0x00000002
#define POSIX_SPAWN_PROC_CLAMP_MAINTENANCE          0x00000003
#define POSIX_SPAWN_PROC_CLAMP_LAST                 0x00000004

#define POSIX_SPAWN_ENTITLEMENT_DRIVER "com.apple.private.spawn-driver"
                                                  
#define POSIX_SPAWN_DARWIN_ROLE_NONE                0x00000000
                                                                               

                                                          
__options_decl(posix_spawn_options, uint32_t, {
	PSA_OPTION_NONE                         = 0,
	PSA_OPTION_PLUGIN_HOST_DISABLE_A_KEYS   = 0x1,
});

  
                                       
   
typedef enum {
	PSFA_OPEN = 0,
	PSFA_CLOSE = 1,
	PSFA_DUP2 = 2,
	PSFA_INHERIT = 3,
	PSFA_FILEPORT_DUP2 = 4,
	PSFA_CHDIR = 5,
	PSFA_FCHDIR = 6
} psfa_t;


  
                                                         
  
                                                                   
                                                               
                                                         
  
                                                              
                                                               
                                                              
                       
   
typedef struct _psfa_action {
	psfa_t  psfaa_type;                                               
	union {
		int psfaa_filedes;                                        
		mach_port_name_t psfaa_fileport;                                
	};
	union {
		struct {
			int     psfao_oflag;                                   
			mode_t  psfao_mode;                                
			char    psfao_path[PATH_MAX];                     
		} psfaa_openargs;
		struct {
			int psfad_newfiledes;                                           
		} psfaa_dup2args;
		struct {
			char    psfac_path[PATH_MAX];                      
		} psfaa_chdirargs;
	};
} _psfa_action_t;


  
                                                                  
  
                                                                    
                                                                
                                                                  
                                                                
                                                                
          
  
                                                              
                                                              
                                                               
                                                               
                                                              
                                      
   
typedef struct _posix_spawn_file_actions {
	int             psfa_act_alloc;                                      
	int             psfa_act_count;                                       
	_psfa_action_t  psfa_act_acts[];                                      
} *_posix_spawn_file_actions_t;

  
                                                                             
                         
   
#define PSF_ACTIONS_SIZE(x)     \
	PS_ACTION_SIZE(x, struct _posix_spawn_file_actions, _psfa_action_t)

  
                                                                             
                                                                               
                                                            
   
#define PSF_ACTIONS_INIT_COUNT  2

  
                                                                              
                                                                        
                                                                              
                                                                              
                            
   
struct _posix_spawn_args_desc {
	__darwin_size_t         attr_size;                                    
	_posix_spawnattr_t      attrp;                                
	__darwin_size_t file_actions_size;                                      
	_posix_spawn_file_actions_t
	    file_actions;                                             
	__darwin_size_t port_actions_size;                                      
	_posix_spawn_port_actions_t
	    port_actions;                                                  
	__darwin_size_t mac_extensions_size;
	_posix_spawn_mac_policy_extensions_t
	    mac_extensions;                                                  
                                                         
	__darwin_size_t coal_info_size;
	struct _posix_spawn_coalition_info *coal_info;                                 

	__darwin_size_t persona_info_size;
	struct _posix_spawn_persona_info   *persona_info;

	__darwin_size_t posix_cred_info_size;
	struct _posix_spawn_posix_cred_info *posix_cred_info;

	__darwin_size_t subsystem_root_path_size;
	char *subsystem_root_path;
};

#ifdef KERNEL
#include <sys/appleapiopts.h>
#ifdef __APPLE_API_PRIVATE

#if __DARWIN_ALIGN_NATURAL
#pragma options align=natural
#endif

struct user32__posix_spawn_args_desc {
	uint32_t        attr_size;                                            
	uint32_t        attrp;                                        
	uint32_t        file_actions_size;                                      
	uint32_t        file_actions;                                 
	uint32_t        port_actions_size;                                      
	uint32_t        port_actions;                                 
	uint32_t        mac_extensions_size;
	uint32_t        mac_extensions;
	uint32_t        coal_info_size;
	uint32_t        coal_info;
	uint32_t        persona_info_size;
	uint32_t        persona_info;
	uint32_t        posix_cred_info_size;
	uint32_t        posix_cred_info;
	uint32_t        subsystem_root_path_size;
	uint32_t        subsystem_root_path;
};

struct user__posix_spawn_args_desc {
	user_size_t     attr_size;                                            
	user_addr_t     attrp;                                        
	user_size_t     file_actions_size;                                      
	user_addr_t     file_actions;                                 
	user_size_t     port_actions_size;                                      
	user_addr_t     port_actions;                                 
	user_size_t     mac_extensions_size;                                     
	user_addr_t     mac_extensions;                               
	user_size_t     coal_info_size;
	user_addr_t     coal_info;
	user_size_t     persona_info_size;
	user_addr_t     persona_info;
	user_size_t     posix_cred_info_size;
	user_addr_t     posix_cred_info;
	user_size_t     subsystem_root_path_size;
	user_addr_t     subsystem_root_path;
};


#if __DARWIN_ALIGN_NATURAL
#pragma options align=reset
#endif

#endif                           
#endif              

#endif                              
