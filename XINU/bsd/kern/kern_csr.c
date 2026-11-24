/*
 * Copyright (c) 2014 Apple Inc. All rights reserved.
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

#include <pexpert/pexpert.h>
#include <sys/csr.h>
#include <sys/errno.h>
#include <sys/sysproto.h>
#include <sys/systm.h>
#include <sys/types.h>

#if CONFIG_CSR_FROM_DT

  
                                                                    
         
   

#include <libkern/section_keywords.h>
#include <pexpert/device_tree.h>

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
#include <arm64/amcc_rorgn.h>
#endif

static SECURITY_READ_ONLY_LATE(csr_config_t) csr_config = 0;

                                                                  
static bool
_csr_get_dt_bool(DTEntry *entry, char const *name, bool *out)
{
	const uint32_t     *value;
	unsigned int size;

	if (SecureDTGetProperty(*entry, name, (const void**)&value, &size) != kSuccess) {
		return false;
	}

	if (size != sizeof(uint32_t)) {
		panic("unexpected size %xu for bool property '%s'", size, name);
	}

	*out = (bool)*value;
	return true;
}

                                                                  
static bool
_csr_get_dt_uint64(DTEntry *entry, char const *name, uint64_t *out)
{
	const uint64_t     *value;
	unsigned int size;

	if (SecureDTGetProperty(*entry, name, (const void**)&value, &size) != kSuccess) {
		return false;
	}

	if (size != sizeof(uint64_t)) {
		panic("unexpected size %xu for uint64 property '%s'", size, name);
	}

	*out = *value;
	return true;
}

                                                                  
static bool
_csr_dt_string_is_equal(DTEntry *entry, const char *name, const char *str)
{
	const void       *value;
	unsigned         size;
	size_t           str_size;

	str_size = strlen(str) + 1;
	return entry != NULL &&
	       SecureDTGetProperty(*entry, name, &value, &size) == kSuccess &&
	       value != NULL &&
	       size == str_size &&
	       strncmp(str, value, str_size) == 0;
}

static bool
_csr_is_recovery_environment(void)
{
	DTEntry chosen;

	return SecureDTLookupEntry(0, "/chosen", &chosen) == kSuccess &&
	       _csr_dt_string_is_equal(&chosen, "osenvironment", "recoveryos");
}

static bool
_csr_is_iuou_or_iuos_device(void)
{
	DTEntry entry;
	bool    bool_value;

	return (SecureDTLookupEntry(0, "/chosen", &entry) == kSuccess &&
	       (_csr_get_dt_bool(&entry, "internal-use-only-unit", &bool_value) && bool_value)) ||
	       (SecureDTLookupEntry(0, "/chosen/manifest-properties", &entry) == kSuccess &&
	       (_csr_get_dt_bool(&entry, "iuos", &bool_value) && bool_value));
}

static bool
_csr_should_allow_device_configuration(void)
{
	  
                                                                      
                                                                   
                                       
    
	DTEntry chosen;
	bool    bool_value;

	return _csr_is_recovery_environment() || (
		SecureDTLookupEntry(0, "/chosen", &chosen) == kSuccess &&
		_csr_get_dt_bool(&chosen, "csr-allow-device-configuration", &bool_value) && bool_value);
}

  
                                       
  
                                                                   
                                                               
                                                    
  
                                                                               
                                                                                
                                                                           
                    
  
                                                                   
                                                                  
                                                                     
                                                          
  
   
__startup_func
static void
csr_bootstrap(void)
{
	DTEntry         entry;
	uint64_t        uint64_value;
	bool            config_active   = false;
	bool            bool_value;

	csr_config = 0;                                              

	if (SecureDTLookupEntry(0, "/chosen/asmb", &entry) == kSuccess &&
	    _csr_get_dt_uint64(&entry, "lp-sip0", &uint64_value)) {
		csr_config = (uint32_t)uint64_value;                                   
		config_active = true;
	}

	  
                                                                           
                                                                             
                                                                          
                                      
    
	if (!_csr_is_iuou_or_iuos_device()) {
		csr_config &= ~CSR_ALLOW_APPLE_INTERNAL;
	} else if (!config_active) {
		                                                                       
		                               
		csr_config |= CSR_ALLOW_APPLE_INTERNAL;
	}

	if (_csr_should_allow_device_configuration()) {
		csr_config |= CSR_ALLOW_DEVICE_CONFIGURATION;
	}

	                                                                        
	                            
	if (_csr_get_dt_bool(&entry, "lp-sip1", &bool_value) && bool_value) {
		csr_config |= CSR_ALLOW_UNAUTHENTICATED_ROOT;
	} else {
		csr_config &= ~CSR_ALLOW_UNAUTHENTICATED_ROOT;
	}

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	                                         
	                                                            
	                                                                            
	csr_unsafe_kernel_text = _csr_get_dt_bool(&entry, "lp-sip2", &bool_value) && bool_value;
#endif
}
STARTUP(TUNABLES, STARTUP_RANK_FIRST, csr_bootstrap);

int
csr_get_active_config(csr_config_t * config)
{
	*config = (csr_config & CSR_VALID_FLAGS);

	return 0;
}

int
csr_check(csr_config_t mask)
{
	csr_config_t config;
	int ret = csr_get_active_config(&config);

	if (ret != 0) {
		return ret;
	}

	                                                                     
	                                                           
	                                                                    
	                                                                    
	                                  
	if ((config & (CSR_ALLOW_UNTRUSTED_KEXTS | CSR_ALLOW_APPLE_INTERNAL)) != 0) {
		config |= CSR_ALLOW_KERNEL_DEBUGGER;
	}

	return ((config & mask) == mask) ? 0 : EPERM;
}

#else

  
                                                      
   

#include <libkern/section_keywords.h>

                                   
static SECURITY_READ_ONLY_LATE(int) csr_allow_all = 0;

  
                                                   
  
                                                         
                                                              
   
__startup_func
static void
csr_bootstrap(void)
{
	boot_args *args = (boot_args *)PE_state.bootArgs;
	if (args->flags & kBootArgsFlagCSRBoot) {
		                                      
		csr_allow_all = 1;
	}
}
STARTUP(TUNABLES, STARTUP_RANK_FIRST, csr_bootstrap);


int
csr_get_active_config(csr_config_t *config)
{
	boot_args *args = (boot_args *)PE_state.bootArgs;
	if (args->flags & kBootArgsFlagCSRActiveConfig) {
		*config = args->csrActiveConfig & CSR_VALID_FLAGS;
	} else {
		*config = 0;
	}

	return 0;
}

int
csr_check(csr_config_t mask)
{
	boot_args *args = (boot_args *)PE_state.bootArgs;
	if (mask & CSR_ALLOW_DEVICE_CONFIGURATION) {
		return (args->flags & kBootArgsFlagCSRConfigMode) ? 0 : EPERM;
	}

	csr_config_t config;
	int ret = csr_get_active_config(&config);
	if (ret) {
		return ret;
	}

	                                                                     
	                                                           
	                                                                    
	                                                                    
	                                  
	if ((config & (CSR_ALLOW_UNTRUSTED_KEXTS | CSR_ALLOW_APPLE_INTERNAL)) != 0) {
		config |= CSR_ALLOW_KERNEL_DEBUGGER;
	}

	ret = ((config & mask) == mask) ? 0 : EPERM;
	if (ret == EPERM) {
		                                                                                                                                 
		if (csr_allow_all && (mask & CSR_ALWAYS_ENFORCED_FLAGS) == 0) {
			ret = 0;
		}
	}

	return ret;
}

#endif                         

  
                
   

int syscall_csr_check(struct csrctl_args *args);
int syscall_csr_get_active_config(struct csrctl_args *args);


int
syscall_csr_check(struct csrctl_args *args)
{
	csr_config_t mask = 0;
	int error = 0;

	if (args->useraddr == 0 || args->usersize != sizeof(mask)) {
		return EINVAL;
	}

	error = copyin(args->useraddr, &mask, sizeof(mask));
	if (error) {
		return error;
	}

	return csr_check(mask);
}

int
syscall_csr_get_active_config(struct csrctl_args *args)
{
	csr_config_t config = 0;
	int error = 0;

	if (args->useraddr == 0 || args->usersize != sizeof(config)) {
		return EINVAL;
	}

	error = csr_get_active_config(&config);
	if (error) {
		return error;
	}

	return copyout(&config, args->useraddr, sizeof(config));
}

  
                     
   

int
csrctl(__unused proc_t p, struct csrctl_args *args, __unused int32_t *retval)
{
	switch (args->op) {
	case CSR_SYSCALL_CHECK:
		return syscall_csr_check(args);
	case CSR_SYSCALL_GET_ACTIVE_CONFIG:
		return syscall_csr_get_active_config(args);
	default:
		return ENOSYS;
	}
}
