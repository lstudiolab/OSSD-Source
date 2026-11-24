/*
 * Copyright (c) 2004-2016 Apple Computer, Inc. All rights reserved.
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
/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>

quad_t strtoq(const char *, char **, int);
u_quad_t strtouq(const char *, char **, int);

static inline int
isspace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\12';
}

#define BUF             32                                             

  
                                
   
#define LONG            0x01                           
#define SHORT           0x04                  
#define SUPPRESS        0x08                                
#define POINTER         0x10                            
#define NOSKIP          0x20                                    
#define LONGLONG        0x400                                             
#define SHORTSHORT      0x4000                
#define UNSIGNED        0x8000                            

  
                                                      
                                                            
                                                         
   
#define SIGNOK          0x40                              
#define NDIGITS         0x80                            

#define DPTOK           0x100                                             
#define EXPOK           0x200                                                

#define PFXOK           0x100                                   
#define NZDIGITS        0x200                                

  
                    
   
#define CT_CHAR         0                          
#define CT_CCL          1                              
#define CT_STRING       2                          
#define CT_INT          3                                  

static const u_char *__sccl(char *, const u_char *);

int sscanf(const char *, const char *, ...);
int vsscanf(const char *, char const *, va_list);

int
sscanf(const char *ibuf, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsscanf(ibuf, fmt, ap);
	va_end(ap);
	return ret;
}

int
vsscanf(const char *inp, char const *fmt0, va_list ap)
{
	ssize_t inr;
	const u_char *fmt = (const u_char *)fmt0;
	ssize_t width;                                  
	char *p;                                                      
	int flags;                                          
	char *p0;                                                             
	int nassigned = 0;                                         
	int nconversions = 0;                                  
	int nread = 0;                                                         
	int base = 0;                                                         
	char ccltab[256];                                             
	char buf[BUF];                                              

	                                                                  
	static short basefix[17] =
	{ 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	inr = (ssize_t)strlen(inp);

	for (;;) {
		char c = (char)*fmt++;                                           
		if (c == 0) {
			return nassigned;
		}
		if (isspace(c)) {
			while (inr > 0 && isspace(*inp)) {
				nread++;
				inr--;
				inp++;
			}
			continue;
		}
		if (c != '%') {
			goto literal;
		}
		width = 0;
		flags = 0;
		  
                                             
                                       
     
again:
		c = (char)*fmt++;
		switch (c) {
		case '%':
literal:
			if (inr <= 0) {
				goto input_failure;
			}
			if (*inp != c) {
				goto match_failure;
			}
			inr--;
			inp++;
			nread++;
			continue;

		case '*':
			flags |= SUPPRESS;
			goto again;
		case 'l':
			if (flags & LONG) {
				flags &= ~LONG;
				flags |= LONGLONG;
			} else {
				flags |= LONG;
			}
			goto again;
		case 'q':
			flags |= LONGLONG;                     
			goto again;
		case 'h':
			if (flags & SHORT) {
				flags &= ~SHORT;
				flags |= SHORTSHORT;
			} else {
				flags |= SHORT;
			}
			goto again;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			width = width * 10 + c - '0';
			goto again;

		  
                 
     
		case 'd':
			c = CT_INT;
			base = 10;
			break;

		case 'i':
			c = CT_INT;
			base = 0;
			break;

		case 'o':
			c = CT_INT;
			flags |= UNSIGNED;
			base = 8;
			break;

		case 'u':
			c = CT_INT;
			flags |= UNSIGNED;
			base = 10;
			break;

		case 'X':
		case 'x':
			flags |= PFXOK;                          
			c = CT_INT;
			flags |= UNSIGNED;
			base = 16;
			break;

		case 's':
			c = CT_STRING;
			break;

		case '[':
			fmt = __sccl(ccltab, fmt);
			flags |= NOSKIP;
			c = CT_CCL;
			break;

		case 'c':
			flags |= NOSKIP;
			c = CT_CHAR;
			break;

		case 'p':                                       
			flags |= POINTER | PFXOK;
			c = CT_INT;
			flags |= UNSIGNED;
			base = 16;
			break;

		case 'n':
			nconversions++;
			if (flags & SUPPRESS) {          
				continue;
			}
			if (flags & SHORTSHORT) {
				*va_arg(ap, char *) = (char)nread;
			} else if (flags & SHORT) {
				*va_arg(ap, short *) = (short)nread;
			} else if (flags & LONG) {
				*va_arg(ap, long *) = (long)nread;
			} else if (flags & LONGLONG) {
				*va_arg(ap, long long *) = (long long)nread;
			} else {
				*va_arg(ap, int *) = (int)nread;
			}
			continue;
		}

		  
                                              
     
		if (inr <= 0) {
			goto input_failure;
		}

		  
                                                    
                        
     
		if ((flags & NOSKIP) == 0) {
			while (isspace(*inp)) {
				nread++;
				if (--inr > 0) {
					inp++;
				} else {
					goto input_failure;
				}
			}
			  
                                                  
                                                       
                                               
      
		}

		  
                       
     
		switch (c) {
		case CT_CHAR:
			                                             
			if (width == 0) {
				width = 1;
			}
			if (flags & SUPPRESS) {
				size_t sum = 0;
				for (;;) {
					ssize_t n = inr;
					if (n < width) {
						sum += (size_t)n;
						width -= n;
						inp += n;
						if (sum == 0) {
							goto input_failure;
						}
						break;
					} else {
						sum += (size_t)width;
						inr -= width;
						inp += width;
						break;
					}
				}
				nread += sum;
			} else {
				bcopy(inp, va_arg(ap, char *), width);
				inr -= width;
				inp += width;
				nread += width;
				nassigned++;
			}
			nconversions++;
			break;

		case CT_CCL: {
			                                                     
			if (width == 0) {
				width = SSIZE_MAX;                     
			}
			                                         
			ptrdiff_t n;
			if (flags & SUPPRESS) {
				n = 0;
				while (ccltab[(unsigned char)*inp]) {
					n++;
					inr--;
					inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						if (n == 0) {
							goto input_failure;
						}
						break;
					}
				}
				if (n == 0) {
					goto match_failure;
				}
			} else {
				p0 = p = va_arg(ap, char *);
				while (ccltab[(unsigned char)*inp]) {
					inr--;
					*p++ = *inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						if (p == p0) {
							goto input_failure;
						}
						break;
					}
				}
				n = p - p0;
				if (n == 0) {
					goto match_failure;
				}
				*p = 0;
				nassigned++;
			}
			nread += n;
			nconversions++;
			break;
		}

		case CT_STRING:
			                                                      
			if (width == 0) {
				width = SSIZE_MAX;
			}
			if (flags & SUPPRESS) {
				size_t n = 0;
				while (!isspace(*inp)) {
					n++;
					inr--;
					inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						break;
					}
				}
				nread += n;
			} else {
				p0 = p = va_arg(ap, char *);
				while (!isspace(*inp)) {
					inr--;
					*p++ = *inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						break;
					}
				}
				*p = 0;
				nread += p - p0;
				nassigned++;
			}
			nconversions++;
			continue;

		case CT_INT:
			                                                      
			if (width <= 0 || width > (ssize_t)(sizeof(buf) - 1)) {
				width = sizeof(buf) - 1;
			}
			flags |= SIGNOK | NDIGITS | NZDIGITS;
			for (p = buf; width; width--) {
				c = *inp;
				  
                                         
                                           
       
				switch (c) {
				  
                                          
                                          
                                         
                                         
                                            
                                       
                                               
                                             
                                           
                                        
       
				case '0':
					if (base == 0) {
						base = 8;
						flags |= PFXOK;
					}
					if (flags & NZDIGITS) {
						flags &= ~(SIGNOK | NZDIGITS | NDIGITS);
					} else {
						flags &= ~(SIGNOK | PFXOK | NDIGITS);
					}
					goto ok;

				                              
				case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					base = basefix[base];
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				                                          
				case '8': case '9':
					base = basefix[base];
					if (base <= 8) {
						break;                      
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				                        
				case 'A': case 'B': case 'C':
				case 'D': case 'E': case 'F':
				case 'a': case 'b': case 'c':
				case 'd': case 'e': case 'f':
					                              
					if (base <= 10) {
						break;                      
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				                                     
				case '+': case '-':
					if (flags & SIGNOK) {
						flags &= ~SIGNOK;
						goto ok;
					}
					break;

				                                        
				case 'x': case 'X':
					if (flags & PFXOK && p == buf + 1) {
						base = 16;                 
						flags &= ~PFXOK;
						goto ok;
					}
					break;
				}

				  
                                                 
                                               
       
				break;
ok:
				  
                                                 
       
				*p++ = c;
				if (--inr > 0) {
					inp++;
				} else {
					break;                            
				}
			}
			  
                                                
                                                
                                               
                                 
      
			if (flags & NDIGITS) {
				if (p > buf) {
					inp--;
					inr++;
				}
				goto match_failure;
			}
			c = p[-1];
			if (c == 'x' || c == 'X') {
				--p;
				inp--;
				inr++;
			}
			if ((flags & SUPPRESS) == 0) {
				u_quad_t res;

				*p = 0;
				if ((flags & UNSIGNED) == 0) {
					res = (u_quad_t)strtoq(buf, (char **)NULL, base);
				} else {
					res = strtouq(buf, (char **)NULL, base);
				}
				if (flags & POINTER) {
					*va_arg(ap, void **) =
					    (void *)(uintptr_t)res;
				} else if (flags & SHORTSHORT) {
					*va_arg(ap, char *) = (char)res;
				} else if (flags & SHORT) {
					*va_arg(ap, short *) = (short)res;
				} else if (flags & LONG) {
					*va_arg(ap, long *) = (long)res;
				} else if (flags & LONGLONG) {
					*va_arg(ap, long long *) = (long long)res;
				} else {
					*va_arg(ap, int *) = (int)res;
				}
				nassigned++;
			}
			nread += p - buf;
			nconversions++;
			break;
		}
	}
input_failure:
	return nconversions != 0 ? nassigned : -1;
match_failure:
	return nassigned;
}

  
                                                               
                                                                
                                                                
                                  
   
static const u_char *
__sccl(char *tab, const u_char *fmt)
{
	char v;

	                                   
	int c = *fmt++;                                                    
	if (c == '^') {
		v = 1;                                 
		c = *fmt++;                             
	} else {
		v = 0;                                 
	}
	                                                       
	(void) memset(tab, v, 256);

	if (c == 0) {
		return fmt - 1;                                   
	}
	  
                                                           
                                 
   
                                                                  
                                  
    
	v = 1 - v;
	for (;;) {
		int n;
		tab[c] = v;                                   
doswitch:
		n = *fmt++;
		switch (n) {
		case 0:                                            
			return fmt - 1;

		case '-':
			  
                           
            
                                              
                                            
                                     
                 
                                             
                                                  
                                                
                                     
     
                                                 
                                                  
                                                     
                                                      
                                      
      
			n = *fmt;
			if (n == ']' || n < c) {
				c = '-';
				break;                          
			}
			fmt++;
			                       
			do {
				tab[++c] = v;
			} while (c < n);
			c = n;
			  
                                                 
                                                   
                                               
      
			goto doswitch;

		case ']':                                   
			return fmt;

		default:                                            
			c = n;
			break;
		}
	}
	                
}
