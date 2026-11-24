/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
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
#ifndef _SYS_PERSONA_H_
#define _SYS_PERSONA_H_

#ifdef PRIVATE
#include <sys/param.h>

#ifdef KERNEL
__enum_decl(persona_type_t, int, {
#else              
enum {
#endif             
	PERSONA_INVALID      = 0,
	PERSONA_GUEST        = 1,
	PERSONA_MANAGED      = 2,
	PERSONA_PRIV         = 3,
	PERSONA_SYSTEM       = 4,
	PERSONA_DEFAULT      = 5,
	PERSONA_SYSTEM_PROXY = 6,
	PERSONA_SYS_EXT      = 7,
	PERSONA_ENTERPRISE   = 8,

	PERSONA_TYPE_MAX     = PERSONA_ENTERPRISE,
#ifdef KERNEL
});
#else              
};
#endif             

#define PERSONA_ID_NONE ((uid_t)-1)

struct kpersona_info {
	uint32_t persona_info_version;

	uid_t    persona_id;                        
	int      persona_type;
	gid_t    persona_gid;
	uint32_t persona_ngroups;
	gid_t    persona_groups[NGROUPS];
	uid_t    persona_gmuid;
	char     persona_name[MAXLOGNAME + 1];

	                          
};

#define PERSONA_INFO_V1       1
#define PERSONA_INFO_V1_SIZE  (sizeof(struct kpersona_info))


#define PERSONA_OP_ALLOC    1
#define PERSONA_OP_PALLOC   2
#define PERSONA_OP_DEALLOC  3
#define PERSONA_OP_GET      4
#define PERSONA_OP_INFO     5
#define PERSONA_OP_PIDINFO  6
#define PERSONA_OP_FIND     7
#define PERSONA_OP_GETPATH  8
#define PERSONA_OP_FIND_BY_TYPE  9

#define PERSONA_MGMT_ENTITLEMENT "com.apple.private.persona-mgmt"

#ifndef KERNEL
  
                               
   

  
                                                   
  
              
                                                               
                                                              
  
                                                           
  
        
                                                                  
  
          
                     
                       
   
int kpersona_alloc(struct kpersona_info *info, uid_t *id);

  
                                                                     
                           
  
              
                                                               
                                                              
  
                                                                     
                                                                     
                                      
  
                                                           
  
        
                                                                  
  
          
                     
                       
   
int kpersona_palloc(struct kpersona_info *info, uid_t *id, char path[MAXPATHLEN]);

  
                                                          
  
              
                                               
  
          
                    
                      
   
int kpersona_dealloc(uid_t id);

  
                                                                              
  
                                                     
  
              
                                                                                  
                                                                                     
                                                                                      
  
          
                                                      
                                                                 
   
int kpersona_get(uid_t *id);

  
                                                       
  
              
                                
  
                                                        
                                                        
  
          
                    
                      
   
int kpersona_getpath(uid_t id, char path[MAXPATHLEN]);

  
                                                     
  
              
                                               
                                                                                      
                                                                                     
                                                                       
  
                                                                    
  
          
                    
                      
   
int kpersona_info(uid_t id, struct kpersona_info *info);

  
                                                            
  
              
                                                                    
  
                                                                    
  
          
                    
                      
   
int kpersona_pidinfo(pid_t pid, struct kpersona_info *info);

  
                                                       
  
              
                                               
                                                          
  
                                  
                                                        
  
                                                              
                               
  
                                                             
                                                                  
                                                                          
        
                                                    
  
          
                     
                                                                     
   
int kpersona_find(const char *name, uid_t uid, uid_t *id, size_t *idlen);

  
                                                        
  
              
                                               
  
                                                                
                                 
  
                                                               
                                                                    
                                                                          
          
                     
                                                                     
   
int kpersona_find_by_type(int persona_type, uid_t *id, size_t *idlen);
#endif              

#ifdef KERNEL_PRIVATE
                                  
#include <sys/cdefs.h>
#include <sys/kauth.h>
#include <libkern/libkern.h>
#include <os/refcnt.h>

#ifdef PERSONA_DEBUG
#include <os/log.h>
#define persona_dbg(fmt, ...) \
	os_log(OS_LOG_DEFAULT, "[%4d] %s:  " fmt "\n", \
	       current_proc() ? current_proc()->p_pid : -1, \
	       __func__, ## __VA_ARGS__)
#else
#define persona_dbg(fmt, ...) do { } while (0)
#endif

  
          
   
#ifdef XNU_KERNEL_PRIVATE
                                                        
struct persona {
	os_refcnt_t  pna_refcount;
	int32_t      pna_valid;

	uid_t        pna_id;
	persona_type_t pna_type;
	char         pna_login[MAXLOGNAME + 1];
	char         *pna_path;

	kauth_cred_t pna_cred;
	uid_t        pna_pgid;

	int          pna_cred_locked;                              

	LIST_ENTRY(persona) pna_list;

	                                               
	LIST_HEAD(, proc)   pna_members;

	lck_mtx_t    pna_lock;

	  
                                                           
    
#ifdef PERSONA_DEBUG
	char         pna_desc[128];
#endif
};

#define persona_lock(persona)     lck_mtx_lock(&(persona)->pna_lock)
#define persona_unlock(persona)   lck_mtx_unlock(&(persona)->pna_lock)
#define persona_try_lock(persona) lck_mtx_try_lock(&(persona)->pna_lock)

#define persona_lock_assert_held(persona) \
	LCK_MTX_ASSERT(&(persona)->pna_lock, LCK_MTX_ASSERT_OWNED)

#ifdef PERSONA_DEBUG
static inline const char *
persona_desc(struct persona *persona, int locked)
{
	if (!persona) {
		return "<none>";
	}

	if (persona->pna_desc[0] != 0) {
		return persona->pna_desc;
	}

	if (!locked) {
		persona_lock(persona);
	}
	if (persona->pna_desc[0] != 0) {
		goto out_unlock;
	}

	char *p = &persona->pna_desc[0];
	char *end = p + sizeof(persona->pna_desc) - 1;

	*end = 0;
	p += scnprintf(p, end - p, "%s/%d:%d",
	    persona->pna_login,
	    kauth_cred_getuid(persona->pna_cred),
	    kauth_cred_getgid(persona->pna_cred));

	if (p <= end) {
		*p = 0;
	}
out_unlock:
	if (!locked) {
		persona_unlock(persona);
	}

	return persona->pna_desc;
}
#else                     
static inline const char *
persona_desc(struct persona *persona, int locked)
{
	(void)persona;
	(void)locked;
	return "<persona>";
}
#endif

#else                          
                                                       
struct persona;
#endif

__BEGIN_DECLS

#ifndef _KAUTH_CRED_T
#define _KAUTH_CRED_T
typedef struct ucred *kauth_cred_t;
#endif                      

                                                           
uid_t persona_get_id(struct persona *persona);

                                                                           
int persona_get_type(struct persona *persona);

                                                                             
kauth_cred_t persona_get_cred(struct persona *persona);

                                                                  
struct persona *persona_lookup(uid_t id);

  
                                           
  
              
                                               
                                                          
  
                                  
                                                        
  
                                                                      
                                                                    
  
                                                                       
                                                                                                          
  
          
                       
                                                         
   
int persona_find(const char *login, uid_t uid,
    struct persona **persona, size_t *plen);

                                                                  
struct persona *persona_proc_get(pid_t pid);

                                                                                               
struct persona *current_persona_get(void);

                                            
struct persona *persona_get(struct persona *persona);

                                                
void persona_put(struct persona *persona);

  
                                                       
  
              
                                             
  
                                                                          
                                                               
  
                                                                           
                                                                                                              
  
          
                       
                                                         
   
int persona_find_by_type(persona_type_t persona_type, struct persona **persona,
    size_t *plen);

#ifdef XNU_KERNEL_PRIVATE

#if CONFIG_PERSONAS
#include <sys/proc_internal.h>

  
                        
   
extern const uint32_t g_max_personas;

void personas_bootstrap(void);

struct persona *persona_alloc(uid_t id, const char *login,
    persona_type_t type, char *path, int *error);

int persona_init_begin(struct persona *persona);
void persona_init_end(struct persona *persona, int error);

struct persona *persona_lookup_and_invalidate(uid_t id);
int persona_verify_and_set_uniqueness(struct persona *persona);
boolean_t persona_is_unique(struct persona *persona);

static inline int
proc_has_persona(proc_t p)
{
	if (p && p->p_persona) {
		return 1;
	}
	return 0;
}

static inline uid_t
persona_id_from_proc(proc_t p)
{
	if (p && p->p_persona) {
		return p->p_persona->pna_id;
	}
	return PERSONA_ID_NONE;
}

int persona_proc_inherit(proc_t child, proc_t parent);

int persona_proc_adopt_id(proc_t p, uid_t id,
    kauth_cred_t auth_override);
int persona_proc_adopt(proc_t p, struct persona *persona,
    kauth_cred_t auth_override);
int persona_proc_drop(proc_t p);

int persona_set_cred(struct persona *persona, kauth_cred_t cred);
int persona_set_cred_from_proc(struct persona *persona, proc_t proc);

uid_t persona_get_uid(struct persona *persona);

int persona_set_gid(struct persona *persona, gid_t gid);
gid_t persona_get_gid(struct persona *persona);

int persona_set_groups(struct persona *persona, gid_t *groups, size_t ngroups, uid_t gmuid);
int persona_get_groups(struct persona *persona, size_t *ngroups, gid_t *groups, size_t groups_sz);

uid_t persona_get_gmuid(struct persona *persona);

int persona_get_login(struct persona *persona, char login[MAXLOGNAME + 1]);

                                                                  
struct persona *persona_proc_get(pid_t pid);

int persona_find_all(const char *login, uid_t uid, persona_type_t persona_type,
    struct persona **persona, size_t *plen);

#else                       

static inline int
proc_has_persona(__unused proc_t p)
{
	return 0;
}

static inline uid_t
persona_id_from_proc(__unused proc_t p)
{
	return PERSONA_ID_NONE;
}

#endif                      
#endif                         
__END_DECLS

#endif                     

#endif              
#endif                      
