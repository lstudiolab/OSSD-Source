/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */

#include <TargetConditionals.h>

#if TARGET_OS_OSX && !defined(__i386__)
  
                                                                                     
                                                             
   

#include <fcntl.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <unistd.h>

#include "system-version-compat-support.h"

#define PLAT_PREFIX_IOS "iOS"
#define PLAT_PREFIX_MACOS ""

#define COMPAT_SUFFIX_MACOS "Compat"
#define COMPAT_SUFFIX_IOS ""

#define SYSTEM_VERSION_PLIST_FILENAME "SystemVersion.plist"
#define SYSTEM_VERSION_PLIST_PATH ("/System/Library/CoreServices/" SYSTEM_VERSION_PLIST_FILENAME)

#define SYSTEM_VERSION_COMPAT_PLIST_FILENAME(platform_prefix, compat_suffix) (platform_prefix "SystemVersion" compat_suffix ".plist")

#define SYSTEM_VERSION_PLIST_FILENAMELEN strlen(SYSTEM_VERSION_PLIST_FILENAME)

#define SYSTEM_VERSION_COMPAT_PLIST_FILENAMELEN(platform_prefix, compat_suffix) strlen(SYSTEM_VERSION_COMPAT_PLIST_FILENAME(platform_prefix, compat_suffix))

#define SYSTEM_VERSION_PLIST_PATHLEN strlen(SYSTEM_VERSION_PLIST_PATH)

extern system_version_compat_mode_t system_version_compat_mode;

  
                                                                                                      
                                                                                                  
                                                                                                 
  
                                                                                
  
                                                                                        
                     
   
__attribute__((visibility("hidden")))
bool
_system_version_compat_check_path_suffix(const char *orig_path)
{
	size_t path_str_len = strnlen(orig_path, MAXPATHLEN);
	  
                                                               
                                        
    
	if (path_str_len < SYSTEM_VERSION_PLIST_FILENAMELEN) {
		return false;
	}

	                                                                                     
	if (strncmp(&orig_path[path_str_len - SYSTEM_VERSION_PLIST_FILENAMELEN], SYSTEM_VERSION_PLIST_FILENAME,
	    SYSTEM_VERSION_PLIST_FILENAMELEN) != 0) {
		return false;
	}

	                                                                   
	if (path_str_len == MAXPATHLEN) {
		return false;
	}

	size_t compat_len = (system_version_compat_mode == SYSTEM_VERSION_COMPAT_MODE_IOS) ? SYSTEM_VERSION_COMPAT_PLIST_FILENAMELEN(PLAT_PREFIX_IOS, COMPAT_SUFFIX_IOS) : SYSTEM_VERSION_COMPAT_PLIST_FILENAMELEN(PLAT_PREFIX_MACOS, COMPAT_SUFFIX_MACOS);
	if ((compat_len - SYSTEM_VERSION_PLIST_FILENAMELEN) > (MAXPATHLEN - path_str_len - 1)) {
		return false;
	}

	                             
	return true;
}

  
                                                                                                              
                                                                                              
  
                                                                                                                   
                                                                                                              
                                                                                                                   
                                                                                                                    
                                                                                                                         
                                      
  
                                                                                                                         
                                                                              
  
                                                                                                                
  
                                                                                                                      
               
  
                                                                                                  
                                                                                                                           
                                                                            
                                                                      
                                                                     
                                                                                            
                                                                                        
                                                                         
  
                                                                                                       
                                                                                                                       
                                                                                                             
                                                                                                                    
   
__attribute__((visibility("hidden")))
int
_system_version_compat_open_shim(int opened_fd, int openat_fd, const char *orig_path, int oflag, mode_t mode,
    int (*close_syscall)(int), int (*open_syscall)(const char *, int, mode_t),
    int (*openat_syscall)(int, const char *, int, mode_t),
    int (*fcntl_syscall)(int, int, long))
{
	                                                     
	int stashed_errno = errno;
	char new_path[MAXPATHLEN];
	size_t path_str_len = strnlen(orig_path, sizeof(new_path));

	                                                    
	if (fcntl_syscall(opened_fd, F_GETPATH, new_path)) {
		if (errno == EINTR) {
			                                                                              
			close_syscall(opened_fd);
			errno = EINTR;
			return -1;
		} else {
			                                                                         
			errno = stashed_errno;
			return opened_fd;
		}
	}

	                                                                     
	size_t newpathlen = strnlen(new_path, MAXPATHLEN);
	if (newpathlen != SYSTEM_VERSION_PLIST_PATHLEN) {
		errno = stashed_errno;
		return opened_fd;
	}

	if (strncmp(new_path, SYSTEM_VERSION_PLIST_PATH, SYSTEM_VERSION_PLIST_PATHLEN) != 0) {
		errno = stashed_errno;
		return opened_fd;
	}

	new_path[0] = '\0';

	  
                                                                                   
                                                                 
    
	size_t prefix_str_len = path_str_len - SYSTEM_VERSION_PLIST_FILENAMELEN;
	strlcpy(new_path, orig_path, (prefix_str_len + 1));
	if (system_version_compat_mode == SYSTEM_VERSION_COMPAT_MODE_IOS) {
		strlcat(new_path, SYSTEM_VERSION_COMPAT_PLIST_FILENAME(PLAT_PREFIX_IOS, COMPAT_SUFFIX_IOS), MAXPATHLEN);
	} else {
		strlcat(new_path, SYSTEM_VERSION_COMPAT_PLIST_FILENAME(PLAT_PREFIX_MACOS, COMPAT_SUFFIX_MACOS), MAXPATHLEN);
	}

	int new_fd = -1;
	if (openat_syscall != NULL) {
		new_fd = openat_syscall(openat_fd, new_path, oflag, mode);
	} else {
		new_fd = open_syscall(new_path, oflag, mode);
	}
	if ((new_fd == -1) && (errno == ENOENT)) {
		                                                                  
		errno = stashed_errno;
		return opened_fd;
	}

	  
                                                                  
                                                                     
                             
    
	stashed_errno = errno;
	close_syscall(opened_fd);
	errno = stashed_errno;
	return new_fd;
}

#endif                                          
