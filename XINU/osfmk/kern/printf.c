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

/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

  
                                 
  
                                                                       
                                                                 
                                                                   
                                                                     
                                                                      
                                                                      
  
                                                                 
  
                            
            
                
    
                              
    
  
                                                                       
                                                                  
            
  
                                                          
  
                        
                         
                            
                                                 
                                                                                      
                                                                                            
                      
               
            
                              
                        
                     
                                                
  
                                                  
  
                                                                    
  
                                                                   
                                        
  
                        
  
                                                                        
                              
   

  
                                               
  
                                             
  
                                                                    
                                                                        
                                                                      
                                                                        
                              
                                                 
                
                         
  
                                                                   
                                                                            
                                                                         
                                              
                                                              
                
                           
  
                                                                   
                                                                     
                          
   
  
                         
                                 
                   
                       
                          
                              
  
                               
                    
                      
  
                                                    
                   
   
  
                                                       
                                                     
                                                       
   

#include <debug.h>
#include <mach_kdp.h>
#include <mach/boolean.h>
#include <kern/cpu_number.h>
#include <kern/thread.h>
#include <kern/debug.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <stdarg.h>
#include <string.h>
#include <mach_assert.h>
#ifdef  MACH_BSD
#include <sys/msgbuf.h>
#endif
#include <console/serial_protos.h>
#include <os/log_private.h>

#ifdef __x86_64__
#include <i386/cpu_data.h>
#endif                 

#if __arm__ || __arm64__
#include <arm/cpu_data_internal.h>
#endif

#ifdef HAS_APPLE_PAC
#include <mach/vm_param.h>
#include <ptrauth.h>
#endif                    

#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define Ctod(c) ((c) - '0')

#define MAXBUF (sizeof(long long int) * 8)                             
static char digs[] = "0123456789abcdef";

#if CONFIG_NO_PRINTF_STRINGS
                                                    
#undef printf
#endif

int
_consume_printf_args(int a __unused, ...)
{
	return 0;
}
void
_consume_kprintf_args(int a __unused, ...)
{
}

static int
printnum(
	unsigned long long int  u,                           
	int             base,
	void                    (*putc)(int, void *),
	void                    *arg)
{
	char    buf[MAXBUF];                           
	char *  p = &buf[MAXBUF - 1];
	int nprinted = 0;

	do {
		*p-- = digs[u % base];
		u /= base;
	} while (u != 0);

	while (++p != &buf[MAXBUF]) {
		(*putc)(*p, arg);
		nprinted++;
	}

	return nprinted;
}

boolean_t       _doprnt_truncates = FALSE;

#if (DEVELOPMENT || DEBUG)
boolean_t       doprnt_hide_pointers = FALSE;
#else
boolean_t       doprnt_hide_pointers = TRUE;
#endif

int
__doprnt(
	const char      *fmt,
	va_list                 argp,
	                              
	void                    (*putc)(int, void *arg),
	void                    *arg,
	int                     radix,                                        
	int                     is_log)
{
	int             length;
	int             prec;
	boolean_t       ladjust;
	char            padc;
	long long               n;
	unsigned long long      u;
	int             plus_sign;
	int             sign_char;
	boolean_t       altfmt, truncate;
	int             base;
	char    c;
	int             capitals;
	int             long_long;
	enum {
		INT,
		SHORT,
		CHAR,
	} numeric_type = INT;
	int             nprinted = 0;

	if (radix < 2 || radix > 36) {
		radix = 10;
	}

	while ((c = *fmt) != '\0') {
		if (c != '%') {
			(*putc)(c, arg);
			nprinted++;
			fmt++;
			continue;
		}

		fmt++;

		long_long = 0;
		numeric_type = INT;
		length = 0;
		prec = -1;
		ladjust = FALSE;
		padc = ' ';
		plus_sign = 0;
		sign_char = 0;
		altfmt = FALSE;

		while (TRUE) {
			c = *fmt;
			if (c == '#') {
				altfmt = TRUE;
			} else if (c == '-') {
				ladjust = TRUE;
			} else if (c == '+') {
				plus_sign = '+';
			} else if (c == ' ') {
				if (plus_sign == 0) {
					plus_sign = ' ';
				}
			} else {
				break;
			}
			fmt++;
		}

		if (c == '0') {
			padc = '0';
			c = *++fmt;
		}

		if (isdigit(c)) {
			while (isdigit(c)) {
				length = 10 * length + Ctod(c);
				c = *++fmt;
			}
		} else if (c == '*') {
			length = va_arg(argp, int);
			c = *++fmt;
			if (length < 0) {
				ladjust = !ladjust;
				length = -length;
			}
		}

		if (c == '.') {
			c = *++fmt;
			if (isdigit(c)) {
				prec = 0;
				while (isdigit(c)) {
					prec = 10 * prec + Ctod(c);
					c = *++fmt;
				}
			} else if (c == '*') {
				prec = va_arg(argp, int);
				c = *++fmt;
			}
		}

		if (c == 'l') {
			c = *++fmt;                                            
			if (sizeof(int) < sizeof(long)) {
				long_long = 1;
			}
			if (c == 'l') {
				long_long = 1;
				c = *++fmt;
			}
		} else if (c == 'h') {
			c = *++fmt;
			numeric_type = SHORT;
			if (c == 'h') {
				numeric_type = CHAR;
				c = *++fmt;
			}
		} else if (c == 'q' || c == 'L') {
			long_long = 1;
			c = *++fmt;
		}

		if (c == 'z' || c == 'Z') {
			c = *++fmt;
			if (sizeof(size_t) == sizeof(unsigned long long)) {
				long_long = 1;
			}
		}

		truncate = FALSE;
		capitals = 0;                                   

		switch (c) {
		case 'b':
		case 'B':
		{
			char *p;
			boolean_t     any;
			int  i;

			if (long_long) {
				u = va_arg(argp, unsigned long long);
			} else {
				u = va_arg(argp, unsigned int);
			}
			p = va_arg(argp, char *);
			base = *p++;
			nprinted += printnum(u, base, putc, arg);

			if (u == 0) {
				break;
			}

			any = FALSE;
			while ((i = *p++) != '\0') {
				if (*fmt == 'B') {
					i = 33 - i;
				}
				if (*p <= 32) {
					  
                 
        
					int j;
					if (any) {
						(*putc)(',', arg);
					} else {
						(*putc)('<', arg);
						any = TRUE;
					}
					nprinted++;
					j = *p++;
					if (*fmt == 'B') {
						j = 32 - j;
					}
					for (; (c = *p) > 32; p++) {
						(*putc)(c, arg);
						nprinted++;
					}
					nprinted += printnum((unsigned)((u >> (j - 1)) & ((2 << (i - j)) - 1)),
					    base, putc, arg);
				} else if (u & (1 << (i - 1))) {
					if (any) {
						(*putc)(',', arg);
					} else {
						(*putc)('<', arg);
						any = TRUE;
					}
					nprinted++;
					for (; (c = *p) > 32; p++) {
						(*putc)(c, arg);
						nprinted++;
					}
				} else {
					for (; *p > 32; p++) {
						continue;
					}
				}
			}
			if (any) {
				(*putc)('>', arg);
				nprinted++;
			}
			break;
		}

		case 'c':
			c = (char)va_arg(argp, int);
			(*putc)(c, arg);
			nprinted++;
			break;

		case 's':
		{
			const char *p;
			const char *p2;

			if (prec == -1) {
				prec = 0x7fffffff;             
			}
			p = va_arg(argp, char *);

			if (p == NULL) {
				p = "";
			}

			if (length > 0 && !ladjust) {
				n = 0;
				p2 = p;

				for (; *p != '\0' && n < prec; p++) {
					n++;
				}

				p = p2;

				while (n < length) {
					(*putc)(' ', arg);
					n++;
					nprinted++;
				}
			}

			n = 0;

			while ((n < prec) && (!(length > 0 && n >= length))) {
				if (*p == '\0') {
					break;
				}
				(*putc)(*p++, arg);
				nprinted++;
				n++;
			}

			if (n < length && ladjust) {
				while (n < length) {
					(*putc)(' ', arg);
					n++;
					nprinted++;
				}
			}

			break;
		}

		case 'o':
			truncate = _doprnt_truncates;
			OS_FALLTHROUGH;
		case 'O':
			base = 8;
			goto print_unsigned;

		case 'D': {
			unsigned char *up;
			char *q, *p;

			up = (unsigned char *)va_arg(argp, unsigned char *);
			p = (char *)va_arg(argp, char *);
			if (length == -1) {
				length = 16;
			}
			while (length--) {
				(*putc)(digs[(*up >> 4)], arg);
				(*putc)(digs[(*up & 0x0f)], arg);
				nprinted += 2;
				up++;
				if (length) {
					for (q = p; *q; q++) {
						(*putc)(*q, arg);
						nprinted++;
					}
				}
			}
			break;
		}

		case 'd':
		case 'i':
			truncate = _doprnt_truncates;
			base = 10;
			goto print_signed;

		case 'u':
			truncate = _doprnt_truncates;
			OS_FALLTHROUGH;
		case 'U':
			base = 10;
			goto print_unsigned;

		case 'p':
			altfmt = TRUE;
			if (sizeof(int) < sizeof(void *)) {
				long_long = 1;
			}
			OS_FALLTHROUGH;
		case 'x':
			truncate = _doprnt_truncates;
			base = 16;
			goto print_unsigned;

		case 'X':
			base = 16;
			capitals = 16;                           
			goto print_unsigned;

		case 'r':
			truncate = _doprnt_truncates;
			OS_FALLTHROUGH;
		case 'R':
			base = radix;
			goto print_signed;

		case 'n':
			truncate = _doprnt_truncates;
			OS_FALLTHROUGH;
		case 'N':
			base = radix;
			goto print_unsigned;

print_signed:
			if (long_long) {
				n = va_arg(argp, long long);
			} else {
				n = va_arg(argp, int);
			}
			switch (numeric_type) {
			case SHORT:
				n = (short)n;
				break;
			case CHAR:
				n = (char)n;
				break;
			default:
				break;
			}
			if (n >= 0) {
				u = n;
				sign_char = plus_sign;
			} else {
				u = -n;
				sign_char = '-';
			}
			goto print_num;

print_unsigned:
			if (long_long) {
				u = va_arg(argp, unsigned long long);
			} else {
				u = va_arg(argp, unsigned int);
			}
			switch (numeric_type) {
			case SHORT:
				u = (unsigned short)u;
				break;
			case CHAR:
				u = (unsigned char)u;
				break;
			default:
				break;
			}
			goto print_num;

print_num:
			{
				char        buf[MAXBUF];                       
				char *      p = &buf[MAXBUF - 1];
				static char digits[] = "0123456789abcdef0123456789ABCDEF";
				const char *prefix = NULL;

				if (truncate) {
					u = (long long)((int)(u));
				}

				if (doprnt_hide_pointers && is_log) {
					const char str[] = "<ptr>";
					const char* strp = str;
					int strl = sizeof(str) - 1;

#ifdef HAS_APPLE_PAC
					   
                                                        
                                                         
        
					u = (unsigned long long)VM_KERNEL_STRIP_PTR(u);
#endif                    

					if (u >= VM_MIN_KERNEL_AND_KEXT_ADDRESS && u <= VM_MAX_KERNEL_ADDRESS) {
						while (*strp != '\0') {
							(*putc)(*strp, arg);
							strp++;
						}
						nprinted += strl;
						break;
					}
				}

				if (u != 0 && altfmt) {
					if (base == 8) {
						prefix = "0";
					} else if (base == 16) {
						prefix = "0x";
					}
				}

				do {
					                               
					*p-- = digits[(u % base) + capitals];
					u /= base;
				} while (u != 0);

				length -= (int)(&buf[MAXBUF - 1] - p);
				if (sign_char) {
					length--;
				}
				if (prefix) {
					length -= (int)strlen(prefix);
				}

				if (padc == ' ' && !ladjust) {
					                                      
					while (--length >= 0) {
						(*putc)(' ', arg);
						nprinted++;
					}
				}
				if (sign_char) {
					(*putc)(sign_char, arg);
					nprinted++;
				}
				if (prefix) {
					while (*prefix) {
						(*putc)(*prefix++, arg);
						nprinted++;
					}
				}
				if (padc == '0') {
					                                             
					while (--length >= 0) {
						(*putc)('0', arg);
						nprinted++;
					}
				}
				while (++p != &buf[MAXBUF]) {
					(*putc)(*p, arg);
					nprinted++;
				}

				if (ladjust) {
					while (--length >= 0) {
						(*putc)(' ', arg);
						nprinted++;
					}
				}
				break;
			}

		case '\0':
			fmt--;
			break;

		default:
			(*putc)(c, arg);
			nprinted++;
		}
		fmt++;
	}

	return nprinted;
}

static void
dummy_putc(int ch, void *arg)
{
	void (*real_putc)(char) = arg;

	  
                                                                     
                                                               
                                                                    
                                                                   
                                                                    
                                                                  
                                                                 
                                                                    
    
	if (real_putc) {
		real_putc((char)ch);
	}
}

void
_doprnt(
	const char      *fmt,
	va_list                 *argp,
	                              
	void                    (*putc)(char),
	int                     radix)                                        
{
	__doprnt(fmt, *argp, dummy_putc, putc, radix, FALSE);
}

void
_doprnt_log(
	const char      *fmt,
	va_list                 *argp,
	                              
	void                    (*putc)(char),
	int                     radix)                                        
{
	__doprnt(fmt, *argp, dummy_putc, putc, radix, TRUE);
}

#if     MP_PRINTF
boolean_t       new_printf_cpu_number = FALSE;
#endif                 

SIMPLE_LOCK_DECLARE(bsd_log_spinlock, 0);

bool bsd_log_lock(bool);
void bsd_log_lock_safe(void);
void bsd_log_unlock(void);

  
                                                                             
                                                                                
                                                                      
  
                                                                        
  
                                                                
                              
                              
                         
                     
   
bool
bsd_log_lock(bool safe)
{
	if (!safe) {
		assert(!oslog_is_safe());
		return simple_lock_try(&bsd_log_spinlock, LCK_GRP_NULL);
	}
	simple_lock(&bsd_log_spinlock, LCK_GRP_NULL);
	return true;
}

  
                                                                             
               
   
void
bsd_log_lock_safe(void)
{
	(void) bsd_log_lock(true);
}

void
bsd_log_unlock(void)
{
	simple_unlock(&bsd_log_spinlock);
}

                            
void
safe_gets(
	char    *str,
	int     maxlen)
{
	char *lp;
	char c;
	char *strmax = str + maxlen - 1;                                 

	lp = str;
	for (;;) {
		c = (char)cngetc();
		switch (c) {
		case '\n':
		case '\r':
			printf("\n");
			*lp++ = 0;
			return;

		case '\b':
		case '#':
		case '\177':
			if (lp > str) {
				printf("\b \b");
				lp--;
			}
			continue;

		case '@':
		case 'u'&037:
			lp = str;
			printf("\n\r");
			continue;

		default:
			if (c >= ' ' && c < '\177') {
				if (lp < strmax) {
					*lp++ = c;
					printf("%c", c);
				} else {
					printf("%c", '\007');           
				}
			}
		}
	}
}

extern int disableConsoleOutput;

void
conslog_putc(
	char c)
{
	if (!disableConsoleOutput) {
		cnputc(c);
	}

#ifdef  MACH_BSD
	if (!kernel_debugger_entry_count) {
		log_putc(c);
	}
#endif
}

void
cons_putc_locked(
	char c)
{
	if (!disableConsoleOutput) {
		cnputc(c);
	}
}

static int
vprintf_internal(const char *fmt, va_list ap_in, void *caller)
{
	cpu_data_t * cpu_data_p;
	if (fmt) {
		struct console_printbuf_state info_data;
		cpu_data_p = current_cpu_datap();

		va_list ap;
		va_copy(ap, ap_in);
		  
                                                       
                                
     
		if (cpu_data_p->cpu_console_buf != NULL) {
			console_printbuf_state_init(&info_data, TRUE, TRUE);
			__doprnt(fmt, ap, console_printbuf_putc, &info_data, 16, TRUE);
			console_printbuf_clear(&info_data);
		} else {
			disable_preemption();
			_doprnt_log(fmt, &ap, cons_putc_locked, 16);
			enable_preemption();
		}

		va_end(ap);

		os_log_with_args(OS_LOG_DEFAULT, OS_LOG_TYPE_DEFAULT, fmt, ap_in, caller);
	}
	return 0;
}

__attribute__((noinline, not_tail_called))
int
printf(const char *fmt, ...)
{
	int ret;

	va_list ap;
	va_start(ap, fmt);
	ret = vprintf_internal(fmt, ap, __builtin_return_address(0));
	va_end(ap);

	return ret;
}

__attribute__((noinline, not_tail_called))
int
vprintf(const char *fmt, va_list ap)
{
	return vprintf_internal(fmt, ap, __builtin_return_address(0));
}

void
consdebug_putc(char c)
{
	if (!disableConsoleOutput) {
		cnputc(c);
	}

	debug_putc(c);

	if (!console_is_serial() && !disable_serial_output) {
		PE_kputc(c);
	}
}

void
consdebug_putc_unbuffered(char c)
{
	if (!disableConsoleOutput) {
		cnputc_unbuffered(c);
	}

	debug_putc(c);

	if (!console_is_serial() && !disable_serial_output) {
		PE_kputc(c);
	}
}

void
consdebug_log(char c)
{
	debug_putc(c);
}

  
                                                         
                                                          
                                                      
                                                    
   
int
paniclog_append_noflush(const char *fmt, ...)
{
	va_list listp;

	va_start(listp, fmt);
	_doprnt_log(fmt, &listp, consdebug_putc_unbuffered, 16);
	va_end(listp);

	return 0;
}

int
kdb_printf(const char *fmt, ...)
{
	va_list listp;

	va_start(listp, fmt);
	_doprnt_log(fmt, &listp, consdebug_putc, 16);
	va_end(listp);

#if defined(__arm__) || defined(__arm64__)
	paniclog_flush();
#endif

	return 0;
}

int
kdb_log(const char *fmt, ...)
{
	va_list listp;

	va_start(listp, fmt);
	_doprnt(fmt, &listp, consdebug_log, 16);
	va_end(listp);

#if defined(__arm__) || defined(__arm64__)
	paniclog_flush();
#endif

	return 0;
}

int
kdb_printf_unbuffered(const char *fmt, ...)
{
	va_list listp;

	va_start(listp, fmt);
	_doprnt(fmt, &listp, consdebug_putc_unbuffered, 16);
	va_end(listp);

#if defined(__arm__) || defined(__arm64__)
	paniclog_flush();
#endif

	return 0;
}

#if CONFIG_VSPRINTF
static void
copybyte(int c, void *arg)
{
	  
                                                     
                                                   
                                                      
                       
    
	char** p = arg;                           
	**p = (char)c;                       
	(*p)++;                                       
}

  
                       
                                                                
   
int
sprintf(char *buf, const char *fmt, ...)
{
	va_list listp;
	char *copybyte_str;

	va_start(listp, fmt);
	copybyte_str = buf;
	__doprnt(fmt, listp, copybyte, &copybyte_str, 16, FALSE);
	va_end(listp);
	*copybyte_str = '\0';
	return (int)strlen(buf);
}
#endif                      
