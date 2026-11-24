/*
 * Copyright (c) 2000-2012 Apple Inc. All rights reserved.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/sysctl.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/codesign.h>

#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/file_internal.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/proc.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>

#include <sys/ubc.h>
#include <sys/ubc_internal.h>

#include <security/mac.h>
#include <security/mac_policy.h>
#include <security/mac_framework.h>

#include <mach/mach_types.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>

#include <kern/kern_types.h>
#include <kern/startup.h>
#include <kern/task.h>

#include <vm/vm_map.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>


#include <kern/assert.h>

#include <pexpert/pexpert.h>

#include <mach/shared_region.h>

#include <libkern/section_keywords.h>
#include <libkern/ptrauth_utils.h>


unsigned long cs_procs_killed = 0;
unsigned long cs_procs_invalidated = 0;

int cs_force_kill = 0;
int cs_force_hard = 0;
int cs_debug = 0;
                                                                                           
int cs_debug_fail_on_unsigned_code = 0;
                                                                         
unsigned int cs_debug_unsigned_exec_failures = 0;
unsigned int cs_debug_unsigned_mmap_failures = 0;

#if CONFIG_ENFORCE_SIGNED_CODE
#define DEFAULT_CS_SYSTEM_ENFORCEMENT_ENABLE 1
#define DEFAULT_CS_PROCESS_ENFORCEMENT_ENABLE 1
#else
#define DEFAULT_CS_SYSTEM_ENFORCEMENT_ENABLE 1
#define DEFAULT_CS_PROCESS_ENFORCEMENT_ENABLE 0
#endif

#if CONFIG_ENFORCE_LIBRARY_VALIDATION
#define DEFAULT_CS_LIBRARY_VA_ENABLE 1
#else
#define DEFAULT_CS_LIBRARY_VA_ENABLE 0
#endif

#if SECURE_KERNEL

  
                                                                                                           
  
                                                                                                        
                                                                                                            
          
  
                                                                                                        
                                                           
  
                                                                                                      
                                                                                                          
   
SECURITY_READ_ONLY_EARLY(int) cs_system_enforcement_enable = DEFAULT_CS_SYSTEM_ENFORCEMENT_ENABLE;
SECURITY_READ_ONLY_EARLY(int) cs_process_enforcement_enable = DEFAULT_CS_PROCESS_ENFORCEMENT_ENABLE;
SECURITY_READ_ONLY_EARLY(int) cs_library_val_enable = DEFAULT_CS_LIBRARY_VA_ENABLE;

#else                     
int cs_enforcement_panic = 0;
int cs_relax_platform_task_ports = 0;

SECURITY_READ_ONLY_LATE(int) cs_system_enforcement_enable = DEFAULT_CS_SYSTEM_ENFORCEMENT_ENABLE;
SECURITY_READ_ONLY_LATE(int) cs_process_enforcement_enable = DEFAULT_CS_PROCESS_ENFORCEMENT_ENABLE;

SECURITY_READ_ONLY_LATE(int) cs_library_val_enable = DEFAULT_CS_LIBRARY_VA_ENABLE;

#endif                     
int cs_all_vnodes = 0;

SYSCTL_INT(_vm, OID_AUTO, cs_force_kill, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_force_kill, 0, "");
SYSCTL_INT(_vm, OID_AUTO, cs_force_hard, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_force_hard, 0, "");
SYSCTL_INT(_vm, OID_AUTO, cs_debug, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_debug, 0, "");
SYSCTL_INT(_vm, OID_AUTO, cs_debug_fail_on_unsigned_code, CTLFLAG_RW | CTLFLAG_LOCKED,
    &cs_debug_fail_on_unsigned_code, 0, "");
SYSCTL_UINT(_vm, OID_AUTO, cs_debug_unsigned_exec_failures, CTLFLAG_RD | CTLFLAG_LOCKED,
    &cs_debug_unsigned_exec_failures, 0, "");
SYSCTL_UINT(_vm, OID_AUTO, cs_debug_unsigned_mmap_failures, CTLFLAG_RD | CTLFLAG_LOCKED,
    &cs_debug_unsigned_mmap_failures, 0, "");

SYSCTL_INT(_vm, OID_AUTO, cs_all_vnodes, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_all_vnodes, 0, "");

#if !SECURE_KERNEL
SYSCTL_INT(_vm, OID_AUTO, cs_system_enforcement, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_system_enforcement_enable, 0, "");
SYSCTL_INT(_vm, OID_AUTO, cs_process_enforcement, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_process_enforcement_enable, 0, "");
SYSCTL_INT(_vm, OID_AUTO, cs_enforcement_panic, CTLFLAG_RW | CTLFLAG_LOCKED, &cs_enforcement_panic, 0, "");

#if !CONFIG_ENFORCE_LIBRARY_VALIDATION
SYSCTL_INT(_vm, OID_AUTO, cs_library_validation, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_library_val_enable, 0, "");
#endif
#endif                     

int panic_on_cs_killed = 0;

__startup_func
static void
cs_init(void)
{
#if MACH_ASSERT
#if PLATFORM_WatchOS || __x86_64__
	panic_on_cs_killed = 1;
#endif                        
#endif                  
	PE_parse_boot_argn("panic_on_cs_killed", &panic_on_cs_killed,
	    sizeof(panic_on_cs_killed));
#if !SECURE_KERNEL
	int disable_cs_enforcement = 0;
	PE_parse_boot_argn("cs_enforcement_disable", &disable_cs_enforcement,
	    sizeof(disable_cs_enforcement));
	if (disable_cs_enforcement && PE_i_can_has_debugger(NULL) != 0) {
		cs_system_enforcement_enable = 0;
		cs_process_enforcement_enable = 0;
	} else {
		int panic = 0;
		PE_parse_boot_argn("cs_enforcement_panic", &panic, sizeof(panic));
		cs_enforcement_panic = (panic != 0);
	}

	PE_parse_boot_argn("cs_relax_platform_task_ports",
	    &cs_relax_platform_task_ports,
	    sizeof(cs_relax_platform_task_ports));

	PE_parse_boot_argn("cs_debug", &cs_debug, sizeof(cs_debug));

#if !CONFIG_ENFORCE_LIBRARY_VALIDATION
	PE_parse_boot_argn("cs_library_val_enable", &cs_library_val_enable,
	    sizeof(cs_library_val_enable));
#endif
#endif                     
}
STARTUP(CODESIGNING, STARTUP_RANK_FIRST, cs_init);

int
cs_allow_invalid(struct proc *p)
{
#if MACH_ASSERT
	lck_mtx_assert(&p->p_mlock, LCK_MTX_ASSERT_NOTOWNED);
#endif
#if CONFIG_MACF
	                                                                     
                                                         
                                                               
                          
    
	if (0 != mac_proc_check_run_cs_invalid(p)) {
		if (cs_debug) {
			printf("CODE SIGNING: cs_allow_invalid() "
			    "not allowed: pid %d\n",
			    p->p_pid);
		}
		return 0;
	}
	if (cs_debug) {
		printf("CODE SIGNING: cs_allow_invalid() "
		    "allowed: pid %d\n",
		    p->p_pid);
	}
	proc_lock(p);
	p->p_csflags &= ~(CS_KILL | CS_HARD);
	if (p->p_csflags & CS_VALID) {
		p->p_csflags |= CS_DEBUGGED;
	}
#if PMAP_CS
	task_t procTask = proc_task(p);
	if (procTask) {
		vm_map_t proc_map = get_task_map_reference(procTask);
		if (proc_map) {
			if (vm_map_cs_wx_enable(proc_map) != KERN_SUCCESS) {
				printf("CODE SIGNING: cs_allow_invalid() not allowed by pmap: pid %d\n", p->p_pid);
			}
			vm_map_deallocate(proc_map);
		}
	}
#endif          
	proc_unlock(p);

	                                                                
	task_set_memory_ownership_transfer(p->task, TRUE);

	vm_map_switch_protect(get_task_map(p->task), FALSE);
	vm_map_cs_debugged_set(get_task_map(p->task), TRUE);
#endif
	return (p->p_csflags & (CS_KILL | CS_HARD)) == 0;
}

int
cs_invalid_page(addr64_t vaddr, boolean_t *cs_killed)
{
	struct proc     *p;
	int             send_kill = 0, retval = 0, verbose = cs_debug;

	p = current_proc();

	if (verbose) {
		printf("CODE SIGNING: cs_invalid_page(0x%llx): p=%d[%s]\n",
		    vaddr, p->p_pid, p->p_comm);
	}

	proc_lock(p);

	                     
	if (cs_force_kill) {
		p->p_csflags |= CS_KILL;
	}
	if (cs_force_hard) {
		p->p_csflags |= CS_HARD;
	}

	                                                                                   
	if (p->p_csflags & CS_KILL) {
		p->p_csflags |= CS_KILLED;
		cs_procs_killed++;
		send_kill = 1;
		retval = 1;
	}

	                                                                          
	if (p->p_csflags & CS_HARD) {
		retval = 1;
	} else {
		if (p->p_csflags & CS_VALID) {
			p->p_csflags &= ~CS_VALID;
			cs_procs_invalidated++;
			verbose = 1;
			cs_process_invalidated(NULL);
		}
	}
	proc_unlock(p);

	if (verbose) {
		printf("CODE SIGNING: cs_invalid_page(0x%llx): "
		    "p=%d[%s] final status 0x%x, %s page%s\n",
		    vaddr, p->p_pid, p->p_comm, p->p_csflags,
		    retval ? "denying" : "allowing (remove VALID)",
		    send_kill ? " sending SIGKILL" : "");
	}

	if (send_kill) {
		                                                      
		threadsignal(current_thread(), SIGKILL, EXC_BAD_ACCESS, FALSE);
		if (cs_killed) {
			*cs_killed = TRUE;
		}
	} else if (cs_killed) {
		*cs_killed = FALSE;
	}

	return retval;
}

  
                                                                 
                                                              
                                                      
                                                                   
                                                                
                                                               
                                            
   
void
cs_process_invalidated(struct proc * __unused p)
{
#if CONFIG_MACF
	if (p == NULL) {
		p = current_proc();
	}
	mac_proc_notify_cs_invalidated(p);
#endif
}

  
                                                       
   

int
cs_process_enforcement(struct proc *p)
{
	if (cs_process_enforcement_enable) {
		return 1;
	}

	if (p == NULL) {
		p = current_proc();
	}

	if (p != NULL && (p->p_csflags & CS_ENFORCEMENT)) {
		return 1;
	}

	return 0;
}

int
cs_process_global_enforcement(void)
{
	return cs_process_enforcement_enable ? 1 : 0;
}

int
cs_system_enforcement(void)
{
	return cs_system_enforcement_enable ? 1 : 0;
}

int
cs_vm_supports_4k_translations(void)
{
	return 0;
}


  
                                                  
   
int
cs_valid(struct proc *p)
{
	if (p == NULL) {
		p = current_proc();
	}

	if (p != NULL && (p->p_csflags & CS_VALID)) {
		return 1;
	}

	return 0;
}

  
                               
   
int
cs_require_lv(struct proc *p)
{
	if (cs_library_val_enable) {
		return 1;
	}

	if (p == NULL) {
		p = current_proc();
	}

	if (p != NULL && (p->p_csflags & CS_REQUIRE_LV)) {
		return 1;
	}

	return 0;
}

int
csproc_forced_lv(struct proc* p)
{
	if (p == NULL) {
		p = current_proc();
	}
	if (p != NULL && (p->p_csflags & CS_FORCED_LV)) {
		return 1;
	}
	return 0;
}

  
                                                                
                                                         
   
int
cs_system_require_lv(void)
{
	return cs_library_val_enable ? 1 : 0;
}

  
                                   
  
                                                                                          
                     
   

off_t
csblob_get_base_offset(struct cs_blob *blob)
{
	return blob->csb_base_offset;
}

  
                            
  
                                                               
   

vm_size_t
csblob_get_size(struct cs_blob *blob)
{
	return blob->csb_mem_size;
}

  
                            
  
                                                                  
   

vm_address_t
csblob_get_addr(struct cs_blob *blob)
{
	return (vm_address_t)blob->csb_mem_kaddr;
}

  
                                       
  
                                                           
                       
   

int
csblob_get_platform_binary(struct cs_blob *blob)
{
	if (blob && blob->csb_platform_binary) {
		return 1;
	}
	return 0;
}

  
                             
  
                                                                
   

unsigned int
csblob_get_flags(struct cs_blob *blob)
{
	return blob->csb_flags;
}

  
                                
  
                                                                    
   

uint8_t
csblob_get_hashtype(struct cs_blob const * const blob)
{
	return blob->csb_hashtype != NULL ? cs_hash_type(blob->csb_hashtype) : 0;
}

  
                            
  
                                                 
                     
   
struct cs_blob *
csproc_get_blob(struct proc *p)
{
	if (NULL == p) {
		return NULL;
	}

	if (NULL == p->p_textvp) {
		return NULL;
	}

	if ((p->p_csflags & CS_SIGNED) == 0) {
		return NULL;
	}

	return ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff);
}

  
                             
  
                                                 
                    
   
struct cs_blob *
csvnode_get_blob(struct vnode *vp, off_t offset)
{
	return ubc_cs_blob_get(vp, -1, -1, offset);
}

  
                              
  
                                                      
                     
   
const char *
csblob_get_teamid(struct cs_blob *csblob)
{
	return csblob->csb_teamid;
}

  
                                
  
                                                      
                   
   
const char *
csblob_get_identity(struct cs_blob *csblob)
{
	const CS_CodeDirectory *cd;

	cd = (const CS_CodeDirectory *)csblob_find_blob(csblob, CSSLOT_CODEDIRECTORY, CSMAGIC_CODEDIRECTORY);
	if (cd == NULL) {
		return NULL;
	}

	if (cd->identOffset == 0) {
		return NULL;
	}

	return ((const char *)cd) + ntohl(cd->identOffset);
}

  
                              
  
                                                      
                                    
   
const uint8_t *
csblob_get_cdhash(struct cs_blob *csblob)
{
	ptrauth_utils_auth_blob_generic(csblob->csb_cdhash,
	    sizeof(csblob->csb_cdhash),
	    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
	    PTRAUTH_ADDR_DIVERSIFY,
	    csblob->csb_cdhash_signature);
	return csblob->csb_cdhash;
}

  
                                   
  
                                                     
                 
   
unsigned int
csblob_get_signer_type(struct cs_blob *csblob)
{
	return csblob->csb_signer_type;
}

void *
csblob_entitlements_dictionary_copy(struct cs_blob *csblob)
{
	if (!csblob->csb_entitlements) {
		return NULL;
	}
	osobject_retain(csblob->csb_entitlements);
	return csblob->csb_entitlements;
}

void
csblob_entitlements_dictionary_set(struct cs_blob *csblob, void * entitlements)
{
	assert(csblob->csb_entitlements == NULL);
	if (entitlements) {
		osobject_retain(entitlements);
	}
	csblob->csb_entitlements = entitlements;
}

  
                              
  
                                                      
                            
   
const char *
csproc_get_teamid(struct proc *p)
{
	struct cs_blob *csblob;

	csblob = csproc_get_blob(p);
	if (csblob == NULL) {
		return NULL;
	}

	return csblob_get_teamid(csblob);
}

const char *
csproc_get_identity(struct proc *p)
{
	struct cs_blob *csblob = NULL;

	csblob = csproc_get_blob(p);
	if (csblob == NULL) {
		return NULL;
	}

	return csblob_get_identity(csblob);
}

  
                                   
  
                                                     
                    
   
unsigned int
csproc_get_signer_type(struct proc *p)
{
	struct cs_blob *csblob;

	csblob = csproc_get_blob(p);
	if (csblob == NULL) {
		return CS_SIGNER_TYPE_UNKNOWN;
	}

	return csblob_get_signer_type(csblob);
}

  
                               
  
                                                      
                                                         
   
const char *
csvnode_get_teamid(struct vnode *vp, off_t offset)
{
	struct cs_blob *csblob;

	if (vp == NULL) {
		return NULL;
	}

	csblob = ubc_cs_blob_get(vp, -1, -1, offset);
	if (csblob == NULL) {
		return NULL;
	}

	return csblob_get_teamid(csblob);
}

  
                                       
  
                                               
                                           
   
int
csproc_get_platform_binary(struct proc *p)
{
	struct cs_blob *csblob;

	csblob = csproc_get_blob(p);

	                                               
                                                   
	return (csblob == NULL) ? 0 : csblob->csb_platform_binary;
}

int
csproc_get_platform_path(struct proc *p)
{
	struct cs_blob *csblob;

	csblob = csproc_get_blob(p);

	return (csblob == NULL) ? 0 : csblob->csb_platform_path;
}

#if DEVELOPMENT || DEBUG
void
csproc_clear_platform_binary(struct proc *p)
{
	struct cs_blob *csblob = csproc_get_blob(p);

	if (csblob == NULL) {
		return;
	}

	if (cs_debug) {
		printf("clearing platform binary on proc/task: pid = %d\n", p->p_pid);
	}

	csblob->csb_platform_binary = 0;
	csblob->csb_platform_path = 0;
	task_set_platform_binary(proc_task(p), FALSE);
}
#endif

void
csproc_disable_enforcement(struct proc* __unused p)
{
#if !CONFIG_ENFORCE_SIGNED_CODE
	if (p != NULL) {
		proc_lock(p);
		p->p_csflags &= (~CS_ENFORCEMENT);
		vm_map_cs_enforcement_set(get_task_map(p->task), FALSE);
		proc_unlock(p);
	}
#endif
}

                                        
  
                                                                                  
                                                                            
                                                       
   
void
csproc_mark_invalid_allowed(struct proc* __unused p)
{
#if !CONFIG_ENFORCE_SIGNED_CODE
	if (p != NULL) {
		proc_lock(p);
		p->p_csflags |= CS_INVALID_ALLOWED;
		proc_unlock(p);
	}
#endif
}

  
                                         
  
                                                                                 
                                                        
   
int
csproc_check_invalid_allowed(struct proc* __unused p)
{
#if !CONFIG_ENFORCE_SIGNED_CODE
	if (p == NULL) {
		p = current_proc();
	}

	if (p != NULL && (p->p_csflags & CS_INVALID_ALLOWED)) {
		return 1;
	}
#endif
	return 0;
}

  
                                   
  
                                                                             
                                                                            
                                              
   
int
csproc_get_prod_signed(struct proc *p)
{
	return (p->p_csflags & CS_DEV_CODE) == 0;
}


  
                                     
  
                                         
                                 
                           
   
int
csfg_get_platform_binary(struct fileglob *fg)
{
	int platform_binary = 0;
	struct ubc_info *uip;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return 0;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return 0;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blobs == NULL) {
		goto out;
	}

	                                                     
                                                              
	platform_binary = uip->cs_blobs->csb_platform_binary;
out:
	vnode_unlock(vp);

	return platform_binary;
}

int
csfg_get_supplement_platform_binary(struct fileglob *fg __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	int platform_binary = 0;
	struct ubc_info *uip;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return 0;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return 0;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blob_supplement == NULL) {
		goto out;
	}

	platform_binary = uip->cs_blob_supplement->csb_platform_binary;
out:
	vnode_unlock(vp);

	return platform_binary;
#else
	                                                                             
	                                         
	return 0;
#endif
}

uint8_t *
csfg_get_cdhash(struct fileglob *fg, uint64_t offset, size_t *cdhash_size)
{
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	struct cs_blob *csblob = NULL;
	if ((csblob = ubc_cs_blob_get(vp, -1, -1, offset)) == NULL) {
		return NULL;
	}

	if (cdhash_size) {
		*cdhash_size = CS_CDHASH_LEN;
	}
	ptrauth_utils_auth_blob_generic(csblob->csb_cdhash,
	    sizeof(csblob->csb_cdhash),
	    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
	    PTRAUTH_ADDR_DIVERSIFY,
	    csblob->csb_cdhash_signature);
	return csblob->csb_cdhash;
}

uint8_t *
csfg_get_supplement_cdhash(struct fileglob *fg __unused, uint64_t offset __unused, size_t *cdhash_size __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	struct cs_blob *csblob = NULL;
	if ((csblob = ubc_cs_blob_get_supplement(vp, offset)) == NULL) {
		return NULL;
	}

	if (cdhash_size) {
		*cdhash_size = CS_CDHASH_LEN;
	}
	ptrauth_utils_auth_blob_generic(csblob->csb_cdhash,
	    sizeof(csblob->csb_cdhash),
	    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
	    PTRAUTH_ADDR_DIVERSIFY,
	    csblob->csb_cdhash_signature);
	return csblob->csb_cdhash;
#else
	                                                                               
	                                        
	return NULL;
#endif
}

const uint8_t *
csfg_get_supplement_linkage_cdhash(struct fileglob *fg __unused, uint64_t offset __unused, size_t *cdhash_size __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	struct cs_blob *csblob = NULL;
	if ((csblob = ubc_cs_blob_get_supplement(vp, offset)) == NULL) {
		return NULL;
	}

	if (cdhash_size) {
		*cdhash_size = CS_CDHASH_LEN;
	}

	return csblob->csb_linkage;
#else
	                                                                               
	                                        
	return NULL;
#endif
}

  
                                 
  
                                            
                                   
   
unsigned int
csfg_get_signer_type(struct fileglob *fg)
{
	struct ubc_info *uip;
	unsigned int signer_type = CS_SIGNER_TYPE_UNKNOWN;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return CS_SIGNER_TYPE_UNKNOWN;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return CS_SIGNER_TYPE_UNKNOWN;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blobs == NULL) {
		goto out;
	}

	                                                           
                                                                    
	signer_type = uip->cs_blobs->csb_signer_type;
out:
	vnode_unlock(vp);

	return signer_type;
}

unsigned int
csfg_get_supplement_signer_type(struct fileglob *fg __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	struct ubc_info *uip;
	unsigned int signer_type = CS_SIGNER_TYPE_UNKNOWN;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return CS_SIGNER_TYPE_UNKNOWN;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return CS_SIGNER_TYPE_UNKNOWN;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blob_supplement == NULL) {
		goto out;
	}

	signer_type = uip->cs_blob_supplement->csb_signer_type;
out:
	vnode_unlock(vp);

	return signer_type;
#else
	                                                                               
	                                
	return CS_SIGNER_TYPE_UNKNOWN;
#endif
}

  
                            
  
                                         
                                              
   
const char *
csfg_get_teamid(struct fileglob *fg)
{
	struct ubc_info *uip;
	const char *str = NULL;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blobs == NULL) {
		goto out;
	}

	                                                     
                                                              
	str = uip->cs_blobs->csb_teamid;
out:
	vnode_unlock(vp);

	return str;
}

const char *
csfg_get_supplement_teamid(struct fileglob *fg __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	struct ubc_info *uip;
	const char *str = NULL;
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blob_supplement == NULL) {
		goto out;
	}

	str = uip->cs_blob_supplement->csb_supplement_teamid;
out:
	vnode_unlock(vp);

	return str;
#else
	                                                                               
	                             
	return NULL;
#endif
}

  
                                 
  
                                                                          
                                                                            
                                              
   
int
csfg_get_prod_signed(struct fileglob *fg)
{
	struct ubc_info *uip;
	vnode_t vp;
	int prod_signed = 0;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return 0;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return 0;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blobs == NULL) {
		goto out;
	}

	                                                   
                                                                
	prod_signed = (uip->cs_blobs->csb_flags & CS_DEV_CODE) == 0;
out:
	vnode_unlock(vp);

	return prod_signed;
}

int
csfg_get_supplement_prod_signed(struct fileglob *fg __unused)
{
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	struct ubc_info *uip;
	vnode_t vp;
	int prod_signed = 0;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return 0;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return 0;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		goto out;
	}

	uip = vp->v_ubcinfo;
	if (uip == NULL) {
		goto out;
	}

	if (uip->cs_blob_supplement == NULL) {
		goto out;
	}

	                                                   
                                                                
	prod_signed = (uip->cs_blob_supplement->csb_flags & CS_DEV_CODE) == 0;
out:
	vnode_unlock(vp);

	return prod_signed;
#else
	                                                                               
	                                                                
	return 0;
#endif
}

  
                              
  
                                                           
                    
   
const char *
csfg_get_identity(struct fileglob *fg, off_t offset)
{
	vnode_t vp;
	struct cs_blob *csblob = NULL;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return NULL;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return NULL;
	}

	csblob = ubc_cs_blob_get(vp, -1, -1, offset);
	if (csblob == NULL) {
		return NULL;
	}

	return csblob_get_identity(csblob);
}

  
                                         
  
                                                           
                                                      
                                                       
   
uint8_t
csfg_get_platform_identifier(struct fileglob *fg, off_t offset)
{
	vnode_t vp;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return 0;
	}

	vp = (struct vnode *)fg->fg_data;
	if (vp == NULL) {
		return 0;
	}

	return csvnode_get_platform_identifier(vp, offset);
}

  
                                            
  
                                                           
                                                        
            
   
uint8_t
csvnode_get_platform_identifier(struct vnode *vp, off_t offset)
{
	struct cs_blob *csblob;
	const CS_CodeDirectory *code_dir;

	csblob = ubc_cs_blob_get(vp, -1, -1, offset);
	if (csblob == NULL) {
		return 0;
	}

	code_dir = csblob->csb_cd;
	if (code_dir == NULL || ntohl(code_dir->length) < 8) {
		return 0;
	}

	return code_dir->platform;
}

  
                                           
  
                                                           
                                                      
                       
   
uint8_t
csproc_get_platform_identifier(struct proc *p)
{
	if (NULL == p->p_textvp) {
		return 0;
	}

	return csvnode_get_platform_identifier(p->p_textvp, p->p_textoff);
}

uint32_t
cs_entitlement_flags(struct proc *p)
{
	return p->p_csflags & CS_ENTITLEMENT_FLAGS;
}

int
cs_restricted(struct proc *p)
{
	return (p->p_csflags & CS_RESTRICT) ? 1 : 0;
}

int
csproc_hardened_runtime(struct proc* p)
{
	return (p->p_csflags & CS_RUNTIME) ? 1 : 0;
}

  
                          
  
                                                   
                              
                                   
                                          
                                           
   
int
csfg_get_path(struct fileglob *fg, char *path, int *len)
{
	vnode_t vp = NULL;

	if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
		return -1;
	}

	vp = (struct vnode *)fg->fg_data;

	                                    
                        
	return vn_getpath(vp, path, len);
}

  
                                             
           
                                                     
                                         
                         
  
                                                         
                                                          
                                 
   
int
cs_entitlements_blob_get_vnode(vnode_t vnode, off_t offset, void **out_start, size_t *out_length)
{
	struct cs_blob *csblob;

	*out_start = NULL;
	*out_length = 0;

	if (vnode == NULL) {
		return EINVAL;
	}

	if ((csblob = ubc_cs_blob_get(vnode, -1, -1, offset)) == NULL) {
		return 0;
	}

	return csblob_get_entitlements(csblob, out_start, out_length);
}

  
                                                
           
                                                     
                                         
                         
  
                                                         
                                                          
                                 
   
int
cs_entitlements_blob_get(proc_t p, void **out_start, size_t *out_length)
{
	if ((p->p_csflags & CS_SIGNED) == 0) {
		return 0;
	}

	return cs_entitlements_blob_get_vnode(p->p_textvp, p->p_textoff, out_start, out_length);
}


                                                 
           
                                                     
                                         
                         
  
                                                                 
   

int
cs_entitlements_dictionary_copy(proc_t p, void **entitlements)
{
	struct cs_blob *csblob;

	*entitlements = NULL;

	if ((p->p_csflags & CS_SIGNED) == 0) {
		return 0;
	}

	if (NULL == p->p_textvp) {
		return EINVAL;
	}

	if ((csblob = ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff)) == NULL) {
		return 0;
	}

	*entitlements = csblob_entitlements_dictionary_copy(csblob);
	return 0;
}

                                                
           
                          
                           
   

const char *
cs_identity_get(proc_t p)
{
	struct cs_blob *csblob;

	if ((p->p_csflags & CS_SIGNED) == 0) {
		return NULL;
	}

	if (NULL == p->p_textvp) {
		return NULL;
	}

	if ((csblob = ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff)) == NULL) {
		return NULL;
	}

	return csblob_get_identity(csblob);
}

  
                            
                                                    
  
                                                              
                                                            
                
   

int
cs_blob_get(proc_t p, void **out_start, size_t *out_length)
{
	struct cs_blob *csblob;

	*out_start = NULL;
	*out_length = 0;

	if (NULL == p->p_textvp) {
		return EINVAL;
	}

	if ((csblob = ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff)) == NULL) {
		return 0;
	}

	*out_start = csblob->csb_mem_kaddr;
	*out_length = csblob->csb_mem_size;

	return 0;
}

  
                                                              
   

uint8_t *
cs_get_cdhash(struct proc *p)
{
	struct cs_blob *csblob;

	if ((p->p_csflags & CS_SIGNED) == 0) {
		return NULL;
	}

	if (NULL == p->p_textvp) {
		return NULL;
	}

	if ((csblob = ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff)) == NULL) {
		return NULL;
	}

	ptrauth_utils_auth_blob_generic(csblob->csb_cdhash,
	    sizeof(csblob->csb_cdhash),
	    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
	    PTRAUTH_ADDR_DIVERSIFY,
	    csblob->csb_cdhash_signature);
	return csblob->csb_cdhash;
}
