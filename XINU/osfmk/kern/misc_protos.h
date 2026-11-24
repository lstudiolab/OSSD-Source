/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
  
                                                                         
                                                                          
                                                                        
               
   

#ifndef _MISC_PROTOS_H_
#define _MISC_PROTOS_H_

#include <stdarg.h>
#include <string.h>
#include <machine/setjmp.h>
#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <ipc/ipc_types.h>
#include <kern/debug.h>
#include <libkern/copyio.h>

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif          
#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif           

                              
extern void setbit(
	int             which,
	int             *bitmap);

                                
extern void clrbit(
	int             which,
	int             *bitmap);

                                           
extern int ffsbit(
	int             *bitmap);
extern int ffs(
	unsigned int    mask);
extern int ffsll(
	unsigned long long mask);

                                          
extern int fls(
	unsigned int    mask);
extern int flsll(
	unsigned long long mask);

  
                                              
   
extern int testbit(
	int             which,
	int             *bitmap);

  
                                                                    
                                  
   
extern int copyin_atomic32(
	const user_addr_t   user_addr,
	uint32_t            *kernel_addr);

extern int copyin_atomic64(
	const user_addr_t   user_addr,
	uint64_t            *kernel_addr);

  
                                                                   
                                                       
  
                                                                            
  
           
                                                                        
                                                               
                                                   
   
extern int copyin_atomic32_wait_if_equals(
	const user_addr_t   user_addr,
	uint32_t            value);

  
                                                           
                                   
   
extern int copyout_atomic32(
	uint32_t            u32,
	user_addr_t         user_addr);

extern int copyout_atomic64(
	uint64_t            u64,
	user_addr_t         user_addr);

                                                                    
extern int copyinstr(
	const user_addr_t   user_addr,
	char                *kernel_addr,
	vm_size_t           max,
	vm_size_t           *actual);

                                                                     
extern int copyinmsg(
	const user_addr_t   user_addr,
	char                *kernel_addr,
	mach_msg_size_t     nbytes);

                                                                     
extern int copyoutmsg(
	const char      *kernel_addr,
	user_addr_t     user_addr,
	mach_msg_size_t nbytes);

extern int sscanf(const char *input, const char *fmt, ...) __scanflike(2, 3);

                                                                   
extern integer_t sprintf(char *buf, const char *fmt, ...) __printflike(2, 3) __deprecated;

extern int printf(const char *format, ...) __printflike(1, 2);
extern int vprintf(const char *format, va_list ap);

#if KERNEL_PRIVATE
int     _consume_printf_args(int, ...);
#endif

#if CONFIG_NO_PRINTF_STRINGS
#if KERNEL_PRIVATE
#define printf(x, ...)  _consume_printf_args( 0, ## __VA_ARGS__ )
#else
#define printf(x, ...)  do {} while (0)
#endif
#endif

extern int paniclog_append_noflush(const char *format, ...) __printflike(1, 2);

extern int kdb_printf(const char *format, ...) __printflike(1, 2);

extern int kdb_log(const char *format, ...) __printflike(1, 2);

extern int kdb_printf_unbuffered(const char *format, ...) __printflike(1, 2);

extern int snprintf(char *, size_t, const char *, ...) __printflike(3, 4);
extern int scnprintf(char *, size_t, const char *, ...) __printflike(3, 4);

extern void log(int level, char *fmt, ...) __printflike(2, 3);

void
_doprnt(
	const char      *fmt,
	va_list                 *argp,
	void                    (*putc)(char),
	int                     radix);

void
_doprnt_log(
	const char      *fmt,
	va_list                 *argp,
	void                    (*putc)(char),
	int                     radix);

int
__doprnt(
	const char      *fmt,
	va_list                 argp,
	void                    (*putc)(int, void *),
	void                    *arg,
	int                     radix,
	int                     is_log);

extern void safe_gets(
	char    *str,
	int     maxlen);

extern void cnputcusr(char);

extern void cnputsusr(char *, int);

extern void conslog_putc(char);

extern void cons_putc_locked(char);

extern void consdebug_putc(char);

extern void consdebug_log(char);

extern void consdebug_putc_unbuffered(char);

extern void cnputc(char);

extern void cnputc_unbuffered(char);

extern void console_write(char *, int);

extern void console_suspend(void);

extern void console_resume(void);

extern int cngetc(void);

extern int cnmaygetc(void);

extern int _setjmp(
	jmp_buf_t       *jmp_buf);

extern int _longjmp(
	jmp_buf_t       *jmp_buf,
	int             value);

extern void bootstrap_create(void);

#if     DIPC
extern boolean_t        no_bootstrap_task(void);
extern ipc_port_t       get_root_master_device_port(void);
#endif            

extern kern_return_t    kernel_set_special_port(
	host_priv_t     host_priv,
	int             which,
	ipc_port_t      port);

extern kern_return_t    kernel_get_special_port(
	host_priv_t     host_priv,
	int             which,
	ipc_port_t      *portp);

user_addr_t get_useraddr(void);

                   
#ifndef __cplusplus
struct kmod_info_t;
#endif

extern uint64_t early_random(void);

#endif                       
