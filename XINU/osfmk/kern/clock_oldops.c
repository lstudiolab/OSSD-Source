/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
  
                                            
  
                                                       
                                                
                                                
                                             
   

#include <mach/mach_types.h>

#include <kern/host.h>
#include <kern/spl.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/ipc_host.h>
#include <kern/clock.h>
#include <kern/zalloc.h>

#include <ipc/ipc_types.h>
#include <ipc/ipc_port.h>

#include <mach/mach_traps.h>
#include <mach/mach_time.h>

#include <mach/clock_server.h>
#include <mach/clock_reply.h>
#include <mach/clock_priv_server.h>

#include <mach/mach_host_server.h>
#include <mach/host_priv_server.h>
#include <libkern/section_keywords.h>

  
                                                                
                                                                
                                                                
                             
   
struct  alarm {
	struct  alarm   *al_next;                                        
	struct  alarm   *al_prev;                                            
	int                             al_status;                                
	mach_timespec_t al_time;                                
	struct {                                                        
		int                             type;                           
		ipc_port_t              port;                           
		mach_msg_type_name_t
		    port_type;                                                       
		struct  clock   *clock;                          
		void                    *data;                          
	} al_alrm;
#define al_type         al_alrm.type
#define al_port         al_alrm.port
#define al_port_type    al_alrm.port_type
#define al_clock        al_alrm.clock
#define al_data         al_alrm.data
	long                    al_seqno;                                          
};
typedef struct alarm    alarm_data_t;

                  
#define ALARM_FREE      0                                          
#define ALARM_SLEEP     1                                         
#define ALARM_CLOCK     2                                         
#define ALARM_DONE      4                                      

                             
decl_simple_lock_data(static, alarm_lock);                                  
                          
static ZONE_DECLARE(alarm_zone, "alarms", sizeof(struct alarm), ZC_NONE);
static struct   alarm           *alrmfree;                                           
static struct   alarm           *alrmdone;                                           
static struct   alarm           *alrmlist;
static long                     alrm_seqno;                                             
static thread_call_data_t       alarm_done_call;
static timer_call_data_t        alarm_expire_timer;

extern  struct clock    clock_list[];
extern  int             clock_count;

static void             post_alarm(
	alarm_t                 alarm);

static void             set_alarm(
	mach_timespec_t *alarm_time);

static int              check_time(
	alarm_type_t    alarm_type,
	mach_timespec_t *alarm_time,
	mach_timespec_t *clock_time);

static void             alarm_done(void);

static void             alarm_expire(void);

static kern_return_t    clock_sleep_internal(
	clock_t                         clock,
	sleep_type_t            sleep_type,
	mach_timespec_t         *sleep_time);

int             rtclock_init(void);

kern_return_t   rtclock_gettime(
	mach_timespec_t                 *cur_time);

kern_return_t   rtclock_getattr(
	clock_flavor_t                  flavor,
	clock_attr_t                    attr,
	mach_msg_type_number_t  *count);

SECURITY_READ_ONLY_EARLY(struct clock_ops) sysclk_ops = {
	.c_config   = NULL,
	.c_init     = rtclock_init,
	.c_gettime  = rtclock_gettime,
	.c_getattr  = rtclock_getattr,
};

kern_return_t   calend_gettime(
	mach_timespec_t                 *cur_time);

kern_return_t   calend_getattr(
	clock_flavor_t                  flavor,
	clock_attr_t                    attr,
	mach_msg_type_number_t  *count);

SECURITY_READ_ONLY_EARLY(struct clock_ops) calend_ops = {
	.c_config   = NULL,
	.c_init     = NULL,
	.c_gettime  = calend_gettime,
	.c_getattr  = calend_getattr,
};

  
                         
   
SECURITY_READ_ONLY_LATE(struct clock) clock_list[] = {
	[SYSTEM_CLOCK] = {
		.cl_ops     = &sysclk_ops,
		.cl_service = IPC_PORT_NULL,
		.cl_control = IPC_PORT_NULL,
	},
	[CALENDAR_CLOCK] = {
		.cl_ops     = &calend_ops,
		.cl_service = IPC_PORT_NULL,
		.cl_control = IPC_PORT_NULL,
	},
};
int     clock_count = sizeof(clock_list) / sizeof(clock_list[0]);

  
                                      
   
#define LOCK_ALARM(s)                   \
	s = splclock();                 \
	simple_lock(&alarm_lock, LCK_GRP_NULL);

#define UNLOCK_ALARM(s)                 \
	simple_unlock(&alarm_lock);     \
	splx(s);

void
clock_oldconfig(void)
{
	clock_t                 clock;
	int     i;

	simple_lock_init(&alarm_lock, 0);
	thread_call_setup(&alarm_done_call, (thread_call_func_t)alarm_done, NULL);
	timer_call_setup(&alarm_expire_timer, (timer_call_func_t)alarm_expire, NULL);

	  
                            
    
	for (i = 0; i < clock_count; i++) {
		clock = &clock_list[i];
		if (clock->cl_ops && clock->cl_ops->c_config) {
			if ((*clock->cl_ops->c_config)() == 0) {
				clock->cl_ops = NULL;
			}
		}
	}

	                                       
	alrm_seqno = 0;
}

void
clock_oldinit(void)
{
	clock_t                 clock;
	int     i;

	  
                                      
    
	for (i = 0; i < clock_count; i++) {
		clock = &clock_list[i];
		if (clock->cl_ops && clock->cl_ops->c_init) {
			(*clock->cl_ops->c_init)();
		}
	}
}

  
                                             
   
void
clock_service_create(void)
{
	  
                                  
    
	for (int i = 0; i < clock_count; i++) {
		clock_t clock = &clock_list[i];
		if (clock->cl_ops) {
			ipc_clock_init(clock);
			ipc_clock_enable(clock);
		}
	}
}

  
                                   
   
kern_return_t
host_get_clock_service(
	host_t                  host,
	clock_id_t              clock_id,
	clock_t                 *clock)                  
{
	if (host == HOST_NULL || clock_id < 0 || clock_id >= clock_count) {
		*clock = CLOCK_NULL;
		return KERN_INVALID_ARGUMENT;
	}

	*clock = &clock_list[clock_id];
	if ((*clock)->cl_ops == 0) {
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

  
                                   
   
kern_return_t
host_get_clock_control(
	host_priv_t             host_priv,
	clock_id_t              clock_id,
	clock_t                 *clock)                  
{
	if (host_priv == HOST_PRIV_NULL ||
	    clock_id < 0 || clock_id >= clock_count) {
		*clock = CLOCK_NULL;
		return KERN_INVALID_ARGUMENT;
	}

	*clock = &clock_list[clock_id];
	if ((*clock)->cl_ops == 0) {
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

  
                              
   
kern_return_t
clock_get_time(
	clock_t                 clock,
	mach_timespec_t *cur_time)               
{
	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	return (*clock->cl_ops->c_gettime)(cur_time);
}

kern_return_t
rtclock_gettime(
	mach_timespec_t         *time)           
{
	clock_sec_t             secs;
	clock_nsec_t    nsecs;

	clock_get_system_nanotime(&secs, &nsecs);
	time->tv_sec = (unsigned int)secs;
	time->tv_nsec = nsecs;

	return KERN_SUCCESS;
}

kern_return_t
calend_gettime(
	mach_timespec_t         *time)           
{
	clock_sec_t             secs;
	clock_nsec_t    nsecs;

	clock_get_calendar_nanotime(&secs, &nsecs);
	time->tv_sec = (unsigned int)secs;
	time->tv_nsec = nsecs;

	return KERN_SUCCESS;
}

  
                        
   
kern_return_t
clock_get_attributes(
	clock_t                                 clock,
	clock_flavor_t                  flavor,
	clock_attr_t                    attr,                    
	mach_msg_type_number_t  *count)                     
{
	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	if (clock->cl_ops->c_getattr) {
		return clock->cl_ops->c_getattr(flavor, attr, count);
	}
	return KERN_FAILURE;
}

kern_return_t
rtclock_getattr(
	clock_flavor_t                  flavor,
	clock_attr_t                    attr,                    
	mach_msg_type_number_t  *count)                     
{
	if (*count != 1) {
		return KERN_FAILURE;
	}

	switch (flavor) {
	case CLOCK_GET_TIME_RES:                    
	case CLOCK_ALARM_CURRES:                         
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = NSEC_PER_SEC / 100;
		break;

	default:
		return KERN_INVALID_VALUE;
	}

	return KERN_SUCCESS;
}

kern_return_t
calend_getattr(
	clock_flavor_t                  flavor,
	clock_attr_t                    attr,                    
	mach_msg_type_number_t  *count)                     
{
	if (*count != 1) {
		return KERN_FAILURE;
	}

	switch (flavor) {
	case CLOCK_GET_TIME_RES:                    
		*(clock_res_t *) attr = NSEC_PER_SEC / 100;
		break;

	case CLOCK_ALARM_CURRES:                         
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = 0;
		break;

	default:
		return KERN_INVALID_VALUE;
	}

	return KERN_SUCCESS;
}

  
                              
   
kern_return_t
clock_set_time(
	clock_t                                 clock,
	__unused mach_timespec_t        new_time)
{
	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	return KERN_FAILURE;
}

  
                                  
   
kern_return_t
clock_set_attributes(
	clock_t                                         clock,
	__unused clock_flavor_t                 flavor,
	__unused clock_attr_t                   attr,
	__unused mach_msg_type_number_t count)
{
	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	return KERN_FAILURE;
}

  
                       
   
kern_return_t
clock_alarm(
	clock_t                                 clock,
	alarm_type_t                    alarm_type,
	mach_timespec_t                 alarm_time,
	ipc_port_t                              alarm_port,
	mach_msg_type_name_t    alarm_port_type)
{
	alarm_t                                 alarm;
	mach_timespec_t                 clock_time;
	int                                             chkstat;
	kern_return_t                   reply_code;
	spl_t                                   s;

	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	if (clock != &clock_list[SYSTEM_CLOCK]) {
		return KERN_FAILURE;
	}
	if (IP_VALID(alarm_port) == 0) {
		return KERN_INVALID_CAPABILITY;
	}

	  
                                                      
                                   
    
	(*clock->cl_ops->c_gettime)(&clock_time);
	chkstat = check_time(alarm_type, &alarm_time, &clock_time);
	if (chkstat <= 0) {
		reply_code = (chkstat < 0 ? KERN_INVALID_VALUE : KERN_SUCCESS);
		clock_alarm_reply(alarm_port, alarm_port_type,
		    reply_code, alarm_type, clock_time);
		return KERN_SUCCESS;
	}

	  
                                          
    

	LOCK_ALARM(s);
	if ((alarm = alrmfree) == 0) {
		UNLOCK_ALARM(s);
		alarm = (alarm_t) zalloc(alarm_zone);
		if (alarm == 0) {
			return KERN_RESOURCE_SHORTAGE;
		}
		LOCK_ALARM(s);
	} else {
		alrmfree = alarm->al_next;
	}

	alarm->al_status = ALARM_CLOCK;
	alarm->al_time = alarm_time;
	alarm->al_type = alarm_type;
	alarm->al_port = alarm_port;
	alarm->al_port_type = alarm_port_type;
	alarm->al_clock = clock;
	alarm->al_seqno = alrm_seqno++;
	post_alarm(alarm);
	UNLOCK_ALARM(s);

	return KERN_SUCCESS;
}

  
                                                                
                                                                      
                                                                
                              
   
kern_return_t
clock_sleep_trap(
	struct clock_sleep_trap_args *args)
{
	mach_port_name_t        clock_name = args->clock_name;
	sleep_type_t            sleep_type = args->sleep_type;
	int                                     sleep_sec = args->sleep_sec;
	int                                     sleep_nsec = args->sleep_nsec;
	mach_vm_address_t       wakeup_time_addr = args->wakeup_time;
	clock_t                         clock;
	mach_timespec_t         swtime = {};
	kern_return_t           rvalue;

	  
                                
    
	if (clock_name == MACH_PORT_NULL) {
		clock = &clock_list[SYSTEM_CLOCK];
	} else {
		clock = port_name_to_clock(clock_name);
	}

	swtime.tv_sec  = sleep_sec;
	swtime.tv_nsec = sleep_nsec;

	  
                                        
    
	rvalue = clock_sleep_internal(clock, sleep_type, &swtime);

	  
                                       
    
	if (rvalue != KERN_INVALID_ARGUMENT && rvalue != KERN_FAILURE) {
		copyout((char *)&swtime, wakeup_time_addr, sizeof(mach_timespec_t));
	}
	return rvalue;
}

static kern_return_t
clock_sleep_internal(
	clock_t                         clock,
	sleep_type_t            sleep_type,
	mach_timespec_t         *sleep_time)
{
	alarm_t                         alarm;
	mach_timespec_t         clock_time;
	kern_return_t           rvalue;
	int                                     chkstat;
	spl_t                           s;

	if (clock == CLOCK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (clock != &clock_list[SYSTEM_CLOCK]) {
		return KERN_FAILURE;
	}

	  
                                                     
                                                  
    
	(*clock->cl_ops->c_gettime)(&clock_time);

	chkstat = check_time(sleep_type, sleep_time, &clock_time);
	if (chkstat < 0) {
		return KERN_INVALID_VALUE;
	}
	rvalue = KERN_SUCCESS;
	if (chkstat > 0) {
		wait_result_t wait_result;

		  
                                           
     

		LOCK_ALARM(s);
		if ((alarm = alrmfree) == 0) {
			UNLOCK_ALARM(s);
			alarm = (alarm_t) zalloc(alarm_zone);
			if (alarm == 0) {
				return KERN_RESOURCE_SHORTAGE;
			}
			LOCK_ALARM(s);
		} else {
			alrmfree = alarm->al_next;
		}

		  
                             
     
		wait_result = assert_wait((event_t)alarm, THREAD_ABORTSAFE);
		if (wait_result == THREAD_WAITING) {
			alarm->al_time = *sleep_time;
			alarm->al_status = ALARM_SLEEP;
			post_alarm(alarm);
			UNLOCK_ALARM(s);

			wait_result = thread_block(THREAD_CONTINUE_NULL);

			  
                                                  
                                                
                                                  
      
			LOCK_ALARM(s);
			if (alarm->al_status != ALARM_DONE) {
				assert(wait_result != THREAD_AWAKENED);
				if (((alarm->al_prev)->al_next = alarm->al_next) != NULL) {
					(alarm->al_next)->al_prev = alarm->al_prev;
				}
				rvalue = KERN_ABORTED;
			}
			*sleep_time = alarm->al_time;
			alarm->al_status = ALARM_FREE;
		} else {
			assert(wait_result == THREAD_INTERRUPTED);
			assert(alarm->al_status == ALARM_FREE);
			rvalue = KERN_ABORTED;
		}
		alarm->al_next = alrmfree;
		alrmfree = alarm;
		UNLOCK_ALARM(s);
	} else {
		*sleep_time = clock_time;
	}

	return rvalue;
}

  
                                   
   
static void
alarm_expire(void)
{
	clock_t                         clock;
	alarm_t alrm1;
	alarm_t alrm2;
	mach_timespec_t         clock_time;
	mach_timespec_t         *alarm_time;
	spl_t                           s;

	clock = &clock_list[SYSTEM_CLOCK];
	(*clock->cl_ops->c_gettime)(&clock_time);

	  
                                                          
                                                             
    
	LOCK_ALARM(s);
	alrm1 = (alarm_t)&alrmlist;
	while ((alrm2 = alrm1->al_next) != NULL) {
		alarm_time = &alrm2->al_time;
		if (CMP_MACH_TIMESPEC(alarm_time, &clock_time) > 0) {
			break;
		}

		  
                                             
                      
     
		if ((alrm1->al_next = alrm2->al_next) != NULL) {
			(alrm1->al_next)->al_prev = alrm1;
		}

		  
                                                
                                         
     
		if (alrm2->al_status == ALARM_SLEEP) {
			alrm2->al_next = NULL;
			alrm2->al_status = ALARM_DONE;
			alrm2->al_time = clock_time;
			thread_wakeup((event_t)alrm2);
		}
		  
                                                 
                                               
                        
     
		else {
			assert(alrm2->al_status == ALARM_CLOCK);
			if ((alrm2->al_next = alrmdone) != NULL) {
				alrmdone->al_prev = alrm2;
			} else {
				thread_call_enter(&alarm_done_call);
			}
			alrm2->al_prev = (alarm_t)&alrmdone;
			alrmdone = alrm2;
			alrm2->al_status = ALARM_DONE;
			alrm2->al_time = clock_time;
		}
	}

	  
                                               
    
	if (alrm2) {
		set_alarm(alarm_time);
	}
	UNLOCK_ALARM(s);
}

static void
alarm_done(void)
{
	alarm_t alrm;
	kern_return_t           code;
	spl_t                           s;

	LOCK_ALARM(s);
	while ((alrm = alrmdone) != NULL) {
		if ((alrmdone = alrm->al_next) != NULL) {
			alrmdone->al_prev = (alarm_t)&alrmdone;
		}
		UNLOCK_ALARM(s);

		code = (alrm->al_status == ALARM_DONE? KERN_SUCCESS: KERN_ABORTED);
		if (alrm->al_port != IP_NULL) {
			                                        
			if (IP_VALID(alrm->al_port)) {
				clock_alarm_reply(alrm->al_port, alrm->al_port_type, code,
				    alrm->al_type, alrm->al_time);
			}

			LOCK_ALARM(s);
			alrm->al_status = ALARM_FREE;
			alrm->al_next = alrmfree;
			alrmfree = alrm;
		} else {
			panic("clock_alarm_deliver");
		}
	}

	UNLOCK_ALARM(s);
}

  
                                          
  
                                                         
   
static void
post_alarm(
	alarm_t                         alarm)
{
	alarm_t alrm1, alrm2;
	mach_timespec_t         *alarm_time;
	mach_timespec_t         *queue_time;

	  
                                                   
                                       
    
	alarm_time = &alarm->al_time;
	alrm1 = (alarm_t)&alrmlist;
	while ((alrm2 = alrm1->al_next) != NULL) {
		queue_time = &alrm2->al_time;
		if (CMP_MACH_TIMESPEC(queue_time, alarm_time) > 0) {
			break;
		}
		alrm1 = alrm2;
	}
	alrm1->al_next = alarm;
	alarm->al_next = alrm2;
	alarm->al_prev = alrm1;
	if (alrm2) {
		alrm2->al_prev  = alarm;
	}

	  
                                                  
                                                  
    
	if (alrmlist == alarm) {
		set_alarm(alarm_time);
	}
}

static void
set_alarm(
	mach_timespec_t         *alarm_time)
{
	uint64_t        abstime;

	nanotime_to_absolutetime(alarm_time->tv_sec, alarm_time->tv_nsec, &abstime);
	timer_call_enter_with_leeway(&alarm_expire_timer, NULL, abstime, 0, TIMER_CALL_USER_NORMAL, FALSE);
}

  
                                                                 
                                                                    
                                                                  
                           
   
static int
check_time(
	alarm_type_t            alarm_type,
	mach_timespec_t         *alarm_time,
	mach_timespec_t         *clock_time)
{
	int                                     result;

	if (BAD_ALRMTYPE(alarm_type)) {
		return -1;
	}
	if (BAD_MACH_TIMESPEC(alarm_time)) {
		return -1;
	}
	if ((alarm_type & ALRMTYPE) == TIME_RELATIVE) {
		ADD_MACH_TIMESPEC(alarm_time, clock_time);
	}

	result = CMP_MACH_TIMESPEC(alarm_time, clock_time);

	return (result >= 0)? result: 0;
}

#ifndef __LP64__

mach_timespec_t
clock_get_system_value(void)
{
	clock_t                         clock = &clock_list[SYSTEM_CLOCK];
	mach_timespec_t         value;

	(void) (*clock->cl_ops->c_gettime)(&value);

	return value;
}

mach_timespec_t
clock_get_calendar_value(void)
{
	clock_t                         clock = &clock_list[CALENDAR_CLOCK];
	mach_timespec_t         value = MACH_TIMESPEC_ZERO;

	(void) (*clock->cl_ops->c_gettime)(&value);

	return value;
}

#endif                
