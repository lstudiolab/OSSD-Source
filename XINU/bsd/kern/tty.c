/*
 * Copyright (c) 1997-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *
 *	@(#)tty.c	8.8 (Berkeley) 1/21/94
 */
   
        
                                                        
                                                                      
                                                                  
                                                                       
                                                                      
              
                                                                  
                      
                     
                                           
                                         
                                                         
                                                              
                                                                  
                                                                  
                                                            
                
                                                                       
                                                               
                                                                      
                                                                       
                                                                
                                                                      
                                          
   
#include <sys/param.h>
#define TTYDEFCHARS 1
#include <sys/systm.h>
#undef  TTYDEFCHARS
#include <sys/ioctl.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/file_internal.h>
#include <sys/conf.h>
#include <sys/dkstat.h>
#include <sys/uio_internal.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/user.h>
#include <sys/signalvar.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <dev/kmreg_com.h>
#include <machine/cons.h>
#include <sys/resource.h>                        
#include <kern/waitq.h>
#include <libkern/section_keywords.h>

static LCK_GRP_DECLARE(tty_lck_grp, "tty");

__private_extern__ int ttnread(struct tty *tp);
static void     ttyecho(int c, struct tty *tp);
static int      ttyoutput(int c, struct tty *tp);
static void     ttypend(struct tty *tp);
static void     ttyretype(struct tty *tp);
static void     ttyrub(int c, struct tty *tp);
static void     ttyrubo(struct tty *tp, int count);
static void     ttystop(struct tty *tp, int rw);
static void     ttyunblock(struct tty *tp);
static int      ttywflush(struct tty *tp);
static int      proc_compare(proc_t p1, proc_t p2);

void ttyhold(struct tty *tp);
static void     ttydeallocate(struct tty *tp);

static int isctty(proc_t p, struct tty  *tp);
static int isctty_sp(proc_t p, struct tty  *tp, struct session *sessp);

__private_extern__ void termios32to64(struct termios32 *in, struct user_termios *out);
__private_extern__ void termios64to32(struct user_termios *in, struct termios32 *out);

  
                                                                         
                                                                          
                                                                         
                                                                          
                                                                      
   
#define E       0x00                      
#define O       0x80                     
#define PARITY(c)       (char_type[c] & O)

#define ALPHA   0x40                              
#define ISALPHA(c)      (char_type[(c) & TTY_CHARMASK] & ALPHA)

#define CCLASSMASK      0x3f
#define CCLASS(c)       (char_type[c] & CCLASSMASK)
                                                    
#define CCONT(c)        ((c & 0xc0) == 0x80)

#define BS      BACKSPACE
#define CC      CONTROL
#define CR      RETURN
#define NA      ORDINARY | ALPHA
#define NL      NEWLINE
#define NO      ORDINARY
#define TB      TAB
#define VT      VTAB

static u_char const char_type[] = {
	E | CC, O | CC, O | CC, E | CC, O | CC, E | CC, E | CC, O | CC,                
	O | BS, E | TB, E | NL, O | CC, E | VT, O | CR, O | CC, E | CC,              
	O | CC, E | CC, E | CC, O | CC, E | CC, O | CC, O | CC, E | CC,                
	E | CC, O | CC, O | CC, E | CC, O | CC, E | CC, E | CC, O | CC,               
	O | NO, E | NO, E | NO, O | NO, E | NO, O | NO, O | NO, E | NO,             
	E | NO, O | NO, O | NO, E | NO, O | NO, E | NO, E | NO, O | NO,            
	E | NA, O | NA, O | NA, E | NA, O | NA, E | NA, E | NA, O | NA,            
	O | NA, E | NA, E | NO, O | NO, E | NO, O | NO, O | NO, E | NO,            
	O | NO, E | NA, E | NA, O | NA, E | NA, O | NA, O | NA, E | NA,            
	E | NA, O | NA, O | NA, E | NA, O | NA, E | NA, E | NA, O | NA,            
	E | NA, O | NA, O | NA, E | NA, O | NA, E | NA, E | NA, O | NA,            
	O | NA, E | NA, E | NA, O | NO, E | NO, O | NO, O | NO, O | NA,            
	E | NO, O | NA, O | NA, E | NA, O | NA, E | NA, E | NA, O | NA,            
	O | NA, E | NA, E | NA, O | NA, E | NA, O | NA, O | NA, E | NA,            
	O | NA, E | NA, E | NA, O | NA, E | NA, O | NA, O | NA, E | NA,            
	E | NA, O | NA, O | NA, E | NO, O | NO, E | NO, E | NO, O | CC,              
	  
                                                     
                                                 
    
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
	NA, NA, NA, NA, NA, NA, NA, NA,
};
#undef  BS
#undef  CC
#undef  CR
#undef  NA
#undef  NL
#undef  NO
#undef  TB
#undef  VT

                                     
#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) &= ~(f)
#define ISSET(t, f)     ((t) & (f))

  
                                                                    
                                                                       
                                  
   
#define I_HIGH_WATER    (TTYHOG - 2 * 256)               
#define I_LOW_WATER     ((TTYHOG - 2 * 256) * 7 / 8)             

__private_extern__ void
termios32to64(struct termios32 *in, struct user_termios *out)
{
	out->c_iflag = (user_tcflag_t)in->c_iflag;
	out->c_oflag = (user_tcflag_t)in->c_oflag;
	out->c_cflag = (user_tcflag_t)in->c_cflag;
	out->c_lflag = (user_tcflag_t)in->c_lflag;

	                                                               
	bcopy(in->c_cc, out->c_cc, sizeof(in->c_cc));

	out->c_ispeed = (user_speed_t)in->c_ispeed;
	out->c_ospeed = (user_speed_t)in->c_ospeed;
}

__private_extern__ void
termios64to32(struct user_termios *in, struct termios32 *out)
{
	out->c_iflag = (uint32_t)in->c_iflag;
	out->c_oflag = (uint32_t)in->c_oflag;
	out->c_cflag = (uint32_t)in->c_cflag;
	out->c_lflag = (uint32_t)in->c_lflag;

	                                                               
	bcopy(in->c_cc, out->c_cc, sizeof(in->c_cc));

	out->c_ispeed = (uint32_t)MIN(in->c_ispeed, UINT32_MAX);
	out->c_ospeed = (uint32_t)MIN(in->c_ospeed, UINT32_MAX);
}


  
           
  
                                    
  
                                            
  
                
  
                                 
   
void
tty_lock(struct tty *tp)
{
	TTY_LOCK_NOTOWNED(tp);                    
	lck_mtx_lock(&tp->t_lock);
}


  
             
  
                                      
  
                                              
  
                
  
                                   
   
void
tty_unlock(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       
	lck_mtx_unlock(&tp->t_lock);
}

  
                  
  
                                                                     
  
                                                      
   
int
ttyopen(dev_t device, struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	tp->t_dev = device;

	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		SET(tp->t_state, TS_ISOPEN);
		if (ISSET(tp->t_cflag, CLOCAL)) {
			SET(tp->t_state, TS_CONNECTED);
		}
		bzero(&tp->t_winsize, sizeof(tp->t_winsize));
	}

	return 0;
}

  
           
  
                                                                
                                                             
                                   
                                                                     
                                                                    
                                             
  
                                                      
   
int
ttyclose(struct tty *tp)
{
	struct pgrp * oldpg;
	struct session * oldsessp;
	struct knote *kn;

	TTY_LOCK_OWNED(tp);                       

	if (constty == tp) {
		constty = NULL;


		  
                                                             
                                     
     
		(*cdevsw[major(tp->t_dev)].d_ioctl)
		(tp->t_dev, KMIOCDISABLCONS, NULL, 0, current_proc());
	}

	ttyflush(tp, FREAD | FWRITE);

	tp->t_gen++;
	tp->t_line = TTYDISC;
	proc_list_lock();
	oldpg = tp->t_pgrp;
	oldsessp = tp->t_session;
	tp->t_pgrp = NULL;
	tp->t_session = NULL;
	if (oldsessp != SESSION_NULL) {
		oldsessp->s_ttypgrpid = NO_PID;
	}
	proc_list_unlock();
	                                                 
	                                               
	tty_unlock(tp);
	if (oldsessp != SESSION_NULL) {
		session_rele(oldsessp);
	}
	if (oldpg != PGRP_NULL) {
		pg_rele(oldpg);
	}
	tty_lock(tp);
	tp->t_state = 0;
	SLIST_FOREACH(kn, &tp->t_wsel.si_note, kn_selnext) {
		KNOTE_DETACH(&tp->t_wsel.si_note, kn);
	}
	selthreadclear(&tp->t_wsel);
	SLIST_FOREACH(kn, &tp->t_rsel.si_note, kn_selnext) {
		KNOTE_DETACH(&tp->t_rsel.si_note, kn);
	}
	selthreadclear(&tp->t_rsel);

	return 0;
}

#define FLUSHQ(q) {                                                     \
	if ((q)->c_cc)                                                  \
	        ndflush(q, (q)->c_cc);                                  \
}

                                                  
#define TTBREAKC(c, lflag)                                                      \
	((c) == '\n' || (((c) == cc[VEOF] ||                            \
	  (c) == cc[VEOL] || ((c) == cc[VEOL2] && lflag & IEXTEN)) &&   \
	 (c) != _POSIX_VDISABLE))

  
                   
  
                                                         
  
                                         
                                         
  
             
  
                                                      
   
int
ttyinput(int c, struct tty *tp)
{
	tcflag_t iflag, lflag;
	cc_t *cc;
	int i, err;
	int retval = 0;                                           

	TTY_LOCK_OWNED(tp);                       

	  
                                      
    
	lflag = tp->t_lflag;
	if (ISSET(lflag, PENDIN)) {
		ttypend(tp);
	}
	  
                 
    
	if (ISSET(lflag, ICANON)) {
		++tk_cancc;
		++tp->t_cancc;
	} else {
		++tk_rawcc;
		++tp->t_rawcc;
	}
	++tk_nin;

	  
                            
                                                                    
                                                          
                             
    
	iflag = tp->t_iflag;
	if (tp->t_rawq.c_cc + tp->t_canq.c_cc > I_HIGH_WATER - 3 &&
	    (!ISSET(lflag, ICANON) || tp->t_canq.c_cc != 0) &&
	    (ISSET(tp->t_cflag, CRTS_IFLOW) || ISSET(iflag, IXOFF)) &&
	    !ISSET(tp->t_state, TS_TBLOCK)) {
		ttyblock(tp);
	}

	                                                             
	cc = tp->t_cc;
	err = (ISSET(c, TTY_ERRORMASK));
	if (err) {
		CLR(c, TTY_ERRORMASK);
		if (ISSET(err, TTY_BI)) {
			if (ISSET(iflag, IGNBRK)) {
				goto out;
			}
			if (ISSET(iflag, BRKINT)) {
				ttyflush(tp, FREAD | FWRITE);
				                                               
				tty_unlock(tp);
				tty_pgsignal(tp, SIGINT, 1);
				tty_lock(tp);
				goto endcase;
			}
			if (ISSET(iflag, PARMRK)) {
				goto parmrk;
			}
		} else if ((ISSET(err, TTY_PE) && ISSET(iflag, INPCK))
		    || ISSET(err, TTY_FE)) {
			if (ISSET(iflag, IGNPAR)) {
				goto out;
			} else if (ISSET(iflag, PARMRK)) {
parmrk:
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc >
				    MAX_INPUT - 3) {
					goto input_overflow;
				}
				(void)putc(0377 | TTY_QUOTE, &tp->t_rawq);
				(void)putc(0 | TTY_QUOTE, &tp->t_rawq);
				(void)putc(c | TTY_QUOTE, &tp->t_rawq);
				goto endcase;
			} else {
				c = 0;
			}
		}
	}

	if (!ISSET(tp->t_state, TS_TYPEN) && ISSET(iflag, ISTRIP)) {
		CLR(c, 0x80);
	}
	if (!ISSET(lflag, EXTPROC)) {
		  
                                         
     
		if (ISSET(tp->t_state, TS_LNCH)) {
			SET(c, TTY_QUOTE);
			CLR(tp->t_state, TS_LNCH);
		}
		  
                                            
                                             
                                           
                                                   
                                       
     

		  
                                          
                              
     
		if (ISSET(lflag, IEXTEN)) {
			if (CCEQ(cc[VLNEXT], c)) {
				if (ISSET(lflag, ECHO)) {
					if (ISSET(lflag, ECHOE)) {
						(void)ttyoutput('^', tp);
						(void)ttyoutput('\b', tp);
					} else {
						ttyecho(c, tp);
					}
				}
				SET(tp->t_state, TS_LNCH);
				goto endcase;
			}
			if (CCEQ(cc[VDISCARD], c)) {
				if (ISSET(lflag, FLUSHO)) {
					CLR(tp->t_lflag, FLUSHO);
				} else {
					ttyflush(tp, FWRITE);
					ttyecho(c, tp);
					if (tp->t_rawq.c_cc + tp->t_canq.c_cc) {
						ttyretype(tp);
					}
					SET(tp->t_lflag, FLUSHO);
				}
				goto startoutput;
			}
		}
		  
             
     
		if (ISSET(lflag, ISIG)) {
			if (CCEQ(cc[VINTR], c) || CCEQ(cc[VQUIT], c)) {
				if (!ISSET(lflag, NOFLSH)) {
					ttyflush(tp, FREAD | FWRITE);
				}
				ttyecho(c, tp);
				  
                                                 
                                                 
                                                 
                                                  
                                           
                                             
                                            
       
				tty_unlock(tp);
				tty_pgsignal(tp,
				    CCEQ(cc[VINTR], c) ? SIGINT : SIGQUIT, 1);
				tty_lock(tp);
				goto endcase;
			}
			if (CCEQ(cc[VSUSP], c)) {
				if (!ISSET(lflag, NOFLSH)) {
					ttyflush(tp, FREAD);
				}
				ttyecho(c, tp);
				                                               
				tty_unlock(tp);
				tty_pgsignal(tp, SIGTSTP, 1);
				tty_lock(tp);
				goto endcase;
			}
		}
		  
                                  
     
		if (ISSET(iflag, IXON)) {
			if (CCEQ(cc[VSTOP], c)) {
				if (!ISSET(tp->t_state, TS_TTSTOP)) {
					SET(tp->t_state, TS_TTSTOP);
					ttystop(tp, 0);
					goto out;
				}
				if (!CCEQ(cc[VSTART], c)) {
					goto out;
				}
				  
                                     
       
				goto endcase;
			}
			if (CCEQ(cc[VSTART], c)) {
				goto restartoutput;
			}
		}
		  
                          
     
		if (c == '\r') {
			if (ISSET(iflag, IGNCR)) {
				goto out;
			} else if (ISSET(iflag, ICRNL)) {
				c = '\n';
			}
		} else if (c == '\n' && ISSET(iflag, INLCR)) {
			c = '\r';
		}
	}
	if (!ISSET(tp->t_lflag, EXTPROC) && ISSET(lflag, ICANON)) {
		  
                                               
                            
     
		  
                    
     
		if (CCEQ(cc[VERASE], c)) {
			if (tp->t_rawq.c_cc) {
				if (ISSET(iflag, IUTF8)) {
					do {
						ttyrub((c = unputc(&tp->t_rawq)), tp);
					} while (tp->t_rawq.c_cc && CCONT(c));
				} else {
					ttyrub(unputc(&tp->t_rawq), tp);
				}
			}
			goto endcase;
		}
		  
              
     
		if (CCEQ(cc[VKILL], c)) {
			if (ISSET(lflag, ECHOKE) &&
			    tp->t_rawq.c_cc == tp->t_rocount &&
			    !ISSET(lflag, ECHOPRT)) {
				while (tp->t_rawq.c_cc) {
					ttyrub(unputc(&tp->t_rawq), tp);
				}
			} else {
				ttyecho(c, tp);
				if (ISSET(lflag, ECHOK) ||
				    ISSET(lflag, ECHOKE)) {
					ttyecho('\n', tp);
				}
				FLUSHQ(&tp->t_rawq);
				tp->t_rocount = 0;
			}
			CLR(tp->t_state, TS_LOCAL);
			goto endcase;
		}
		  
                    
     
		if (CCEQ(cc[VWERASE], c) && ISSET(lflag, IEXTEN)) {
			int ctype;

			  
                      
      
			while ((c = unputc(&tp->t_rawq)) == ' ' || c == '\t') {
				ttyrub(c, tp);
			}
			if (c == -1) {
				goto endcase;
			}
			  
                                              
                                     
      
			ttyrub(c, tp);
			c = unputc(&tp->t_rawq);
			if (c == -1) {
				goto endcase;
			}
			if (c == ' ' || c == '\t') {
				(void)putc(c, &tp->t_rawq);
				goto endcase;
			}
			ctype = ISALPHA(c);
			  
                        
      
			do {
				ttyrub(c, tp);
				c = unputc(&tp->t_rawq);
				if (c == -1) {
					goto endcase;
				}
			} while (c != ' ' && c != '\t' &&
			    (!ISSET(lflag, ALTWERASE) || ISALPHA(c) == ctype));
			(void)putc(c, &tp->t_rawq);
			goto endcase;
		}
		  
                      
     
		if (CCEQ(cc[VREPRINT], c) && ISSET(lflag, IEXTEN)) {
			ttyretype(tp);
			goto endcase;
		}
		  
                                          
     
		if (CCEQ(cc[VSTATUS], c) && ISSET(lflag, IEXTEN)) {
			if (ISSET(lflag, ISIG)) {
				                                               
				tty_unlock(tp);
				tty_pgsignal(tp, SIGINFO, 1);
				tty_lock(tp);
			}
			if (!ISSET(lflag, NOKERNINFO)) {
				ttyinfo_locked(tp);
			}
			goto endcase;
		}
	}
	  
                                   
    
	if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= MAX_INPUT) {
input_overflow:
		if (ISSET(iflag, IMAXBEL)) {
			if (tp->t_outq.c_cc < tp->t_hiwat) {
				(void)ttyoutput(CTRL('g'), tp);
			}
		}
		goto endcase;
	}

	if (c == 0377 && ISSET(iflag, PARMRK) && !ISSET(iflag, ISTRIP)
	    && ISSET(iflag, IGNBRK | IGNPAR) != (IGNBRK | IGNPAR)) {
		(void)putc(0377 | TTY_QUOTE, &tp->t_rawq);
	}

	  
                                   
                                      
    
	if (putc(c, &tp->t_rawq) >= 0) {
		if (!ISSET(lflag, ICANON)) {
			ttwakeup(tp);
			ttyecho(c, tp);
			goto endcase;
		}
		if (TTBREAKC(c, lflag)) {
			tp->t_rocount = 0;
			catq(&tp->t_rawq, &tp->t_canq);
			ttwakeup(tp);
		} else if (tp->t_rocount++ == 0) {
			tp->t_rocol = tp->t_column;
		}
		if (ISSET(tp->t_state, TS_ERASE)) {
			  
                           
      
			CLR(tp->t_state, TS_ERASE);
			(void)ttyoutput('/', tp);
		}
		i = tp->t_column;
		ttyecho(c, tp);
		if (CCEQ(cc[VEOF], c) && ISSET(lflag, ECHO)) {
			  
                                              
      
			i = min(2, tp->t_column - i);
			while (i > 0) {
				(void)ttyoutput('\b', tp);
				i--;
			}
		}
	}

endcase:
	  
                                                      
    
	if (ISSET(tp->t_state, TS_TTSTOP) &&
	    !ISSET(iflag, IXANY) && cc[VSTART] != cc[VSTOP]) {
		goto out;
	}

restartoutput:
	CLR(tp->t_lflag, FLUSHO);
	CLR(tp->t_state, TS_TTSTOP);

startoutput:
	                      
	retval = ttstart(tp);

out:
	return retval;
}


  
            
  
                                                              
                                                        
  
                                          
                                              
  
                         
                                        
  
                                                               
  
                            
   
static int
ttyoutput(int c, struct tty *tp)
{
	tcflag_t oflag;
	int col;

	TTY_LOCK_OWNED(tp);                       

	oflag = tp->t_oflag;
	if (!ISSET(oflag, OPOST)) {
		if (ISSET(tp->t_lflag, FLUSHO)) {
			return -1;
		}
		if (putc(c, &tp->t_outq)) {
			return c;
		}
		tk_nout++;
		tp->t_outcc++;
		return -1;
	}
	  
                                                                   
                                                                    
                                                                   
               
    
	CLR(c, ~TTY_CHARMASK);
	if (c == '\t' &&
	    ISSET(oflag, OXTABS) && !ISSET(tp->t_lflag, EXTPROC)) {
		col = c = 8 - (tp->t_column & 7);
		if (!ISSET(tp->t_lflag, FLUSHO)) {
			c -= b_to_q((const u_char *)"        ", c, &tp->t_outq);
			tk_nout += c;
			tp->t_outcc += c;
		}
		tp->t_column += c;
		return c == col ? -1 : '\t';
	}
	if (c == CEOT && ISSET(oflag, ONOEOT)) {
		return -1;
	}

	  
                                         
                                  
    
	if (c == '\n' && ISSET(tp->t_oflag, ONLCR)) {
		tk_nout++;
		tp->t_outcc++;
		if (putc('\r', &tp->t_outq)) {
			return c;
		}
	}
	                                                
	else if (c == '\r' && ISSET(tp->t_oflag, OCRNL)) {
		c = '\n';
	}
	                                                           
	else if (c == '\r' && ISSET(tp->t_oflag, ONOCR) && tp->t_column == 0) {
		return -1;
	}
	tk_nout++;
	tp->t_outcc++;
	if (!ISSET(tp->t_lflag, FLUSHO) && putc(c, &tp->t_outq)) {
		return c;
	}

	col = tp->t_column;
	switch (CCLASS(c)) {
	case BACKSPACE:
		if (col > 0) {
			--col;
		}
		break;
	case CONTROL:
		break;
	case NEWLINE:
	case RETURN:
		col = 0;
		break;
	case ORDINARY:
		++col;
		break;
	case TAB:
		col = (col + 8) & ~7;
		break;
	}
	tp->t_column = col;
	return -1;
}

  
          
  
                                                         
  
                                     
  
                                  
  
                                                                    
                                                        
  
                                                                    
                                        
  
                                
   
int
ttioctl(struct tty *tp, u_long cmd, caddr_t data, int flag, proc_t p)
{
	int     retval;

	tty_lock(tp);
	retval = ttioctl_locked(tp, cmd, data, flag, p);
	tty_unlock(tp);

	return retval;
}


  
                 
  
                              
  
                                                        
                                   
                                         
                                            
                                            
                                             
                                            
                                          
                                              
  
                       
                                          
                     
                                 
                                               
                          
                                               
                              
                                               
                                                
                                                    
          
                            
                                        
                                           
                                       
                                            
                                                       
                                       
                         
                                                 
                  
                  
                  
                                                       
             
                                            
                                                 
                                                             
  
  
                                                                   
                                                                 
  
                                                                      
                                                              
                                         
  
                                                               
                                                               
                                                  
   
int
ttioctl_locked(struct tty *tp, u_long cmd, caddr_t data, int flag, proc_t p)
{
	int error = 0;
	int bogusData = 1;
	struct uthread *ut;
	struct pgrp *pg, *oldpg;
	struct session *sessp, *oldsessp;
	struct tty *oldtp;

	TTY_LOCK_OWNED(tp);                       

	ut = (struct uthread *)get_bsdthread_info(current_thread());
	                                                                      
	switch (cmd) {
	case TIOCIXON:
	case TIOCIXOFF:
	case  TIOCDRAIN:
	case  TIOCFLUSH:
	case TIOCSTOP:
	case TIOCSTART:
	case  TIOCSETA_32:
	case  TIOCSETA_64:
	case  TIOCSETD:
	case  TIOCSETAF_32:
	case  TIOCSETAF_64:
	case  TIOCSETAW_32:
	case  TIOCSETAW_64:
	case  TIOCSPGRP:
	case  TIOCSTAT:
	case  TIOCSTI:
	case  TIOCSWINSZ:
	case  TIOCLBIC:
	case  TIOCLBIS:
	case  TIOCLSET:
	case  TIOCSETC:
	case OTIOCSETD:
	case  TIOCSETN:
	case  TIOCSETP:
	case  TIOCSLTC:
		while (isbackground(p, tp) &&
		    (p->p_lflag & P_LPPWAIT) == 0 &&
		    (p->p_sigignore & sigmask(SIGTTOU)) == 0 &&
		    (ut->uu_sigmask & sigmask(SIGTTOU)) == 0) {
			pg = proc_pgrp(p);
			if (pg == PGRP_NULL) {
				error = EIO;
				goto out;
			}
			                                               
			tty_unlock(tp);
			if (pg->pg_jobc == 0) {
				pg_rele(pg);
				tty_lock(tp);
				error = EIO;
				goto out;
			}
			pgsignal(pg, SIGTTOU, 1);
			pg_rele(pg);
			tty_lock(tp);


			  
                                                      
                                                      
                                                  
                                                         
      
			error = EINTR;
			goto out;
		}
		break;
	}

	switch (cmd) {                                          
	case FIOASYNC:                                           
		if (*(int *)data) {
			SET(tp->t_state, TS_ASYNC);
		} else {
			CLR(tp->t_state, TS_ASYNC);
		}
		break;
	case FIONBIO:                                                   
		break;                                    
	case FIONREAD:                                           
		*(int *)data = ttnread(tp);
		break;
	case TIOCEXCL:                                                
		SET(tp->t_state, TS_XCLUDE);
		break;
	case TIOCFLUSH: {                                  
		int flags = *(int *)data;

		if (flags == 0) {
			flags = FREAD | FWRITE;
		} else {
			flags &= FREAD | FWRITE;
		}
		ttyflush(tp, flags);
		break;
	}
	case TIOCSCONS: {
		                                             
		data = (caddr_t) &bogusData;
	}
		OS_FALLTHROUGH;
	case TIOCCONS: {                                                    
		if (*(int *)data) {
			if (constty && constty != tp &&
			    ISSET(constty->t_state, TS_CONNECTED)) {
				error = EBUSY;
				goto out;
			}
			if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
				goto out;
			}
			constty = tp;
		} else if (tp == constty) {
			constty = NULL;
		}
		if (constty) {
			(*cdevsw[major(constty->t_dev)].d_ioctl)
			(constty->t_dev, KMIOCDISABLCONS, NULL, 0, p);
		} else {
			(*cdevsw[major(tp->t_dev)].d_ioctl)
			(tp->t_dev, KMIOCDISABLCONS, NULL, 0, p);
		}
		break;
	}
	case TIOCDRAIN:                                               
		error = ttywait(tp);
		if (error) {
			goto out;
		}
		break;
	case TIOCGETA_32:                                       
#ifdef __LP64__
		termios64to32((struct user_termios *)&tp->t_termios, (struct termios32 *)data);
#else
		bcopy(&tp->t_termios, data, sizeof(struct termios));
#endif
		break;
	case TIOCGETA_64:                                       
#ifdef __LP64__
		bcopy(&tp->t_termios, data, sizeof(struct termios));
#else
		termios32to64((struct termios32 *)&tp->t_termios, (struct user_termios *)data);
#endif
		break;
	case TIOCGETD:                                           
		*(int *)data = tp->t_line;
		break;
	case TIOCGWINSZ:                                     
		*(struct winsize *)data = tp->t_winsize;
		break;
	case TIOCGPGRP:                                      
		if (!isctty(p, tp)) {
			error = ENOTTY;
			goto out;
		}
		*(int *)data = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		break;
#ifdef TIOCHPCL
	case TIOCHPCL:                                             
		SET(tp->t_cflag, HUPCL);
		break;
#endif
	case TIOCNXCL:                                                  
		CLR(tp->t_state, TS_XCLUDE);
		break;
	case TIOCOUTQ:                                         
		*(int *)data = tp->t_outq.c_cc;
		break;
	case TIOCSETA_32:                                               
	case TIOCSETA_64:
	case TIOCSETAW_32:                                             
	case TIOCSETAW_64:
	case TIOCSETAF_32:                                        
	case TIOCSETAF_64:
	{                                         
		struct termios *t = (struct termios *)data;
		struct termios lcl_termios;

#ifdef __LP64__
		if (cmd == TIOCSETA_32 || cmd == TIOCSETAW_32 || cmd == TIOCSETAF_32) {
			termios32to64((struct termios32 *)data, (struct user_termios *)&lcl_termios);
			t = &lcl_termios;
		}
#else
		if (cmd == TIOCSETA_64 || cmd == TIOCSETAW_64 || cmd == TIOCSETAF_64) {
			termios64to32((struct user_termios *)data, (struct termios32 *)&lcl_termios);
			t = &lcl_termios;
		}
#endif
#if 0
		                                  
		if (t->c_ispeed < 0 || t->c_ospeed < 0) {
			error = EINVAL;
			goto out;
		}
#endif                                                          
		if (t->c_ispeed == 0) {
			t->c_ispeed = t->c_ospeed;
		}
		if (cmd == TIOCSETAW_32 || cmd == TIOCSETAF_32 ||
		    cmd == TIOCSETAW_64 || cmd == TIOCSETAF_64) {
			error = ttywait(tp);
			if (error) {
				goto out;
			}
			if (cmd == TIOCSETAF_32 || cmd == TIOCSETAF_64) {
				ttyflush(tp, FREAD);
			}
		}
		if (!ISSET(t->c_cflag, CIGNORE)) {
			  
                          
      
			if (tp->t_param && (error = (*tp->t_param)(tp, t))) {
				goto out;
			}
			if (ISSET(t->c_cflag, CLOCAL) &&
			    !ISSET(tp->t_cflag, CLOCAL)) {
				  
                                              
                                                
                                              
                                          
       
				CLR(tp->t_state, TS_ZOMBIE);

				wakeup(TSA_CARR_ON(tp));
				ttwakeup(tp);
				ttwwakeup(tp);
			}
			if ((ISSET(tp->t_state, TS_CARR_ON) ||
			    ISSET(t->c_cflag, CLOCAL)) &&
			    !ISSET(tp->t_state, TS_ZOMBIE)) {
				SET(tp->t_state, TS_CONNECTED);
			} else {
				CLR(tp->t_state, TS_CONNECTED);
			}
			tp->t_cflag = t->c_cflag;
			tp->t_ispeed = t->c_ispeed;
			tp->t_ospeed = t->c_ospeed;
			ttsetwater(tp);
		}
		if (ISSET(t->c_lflag, ICANON) != ISSET(tp->t_lflag, ICANON) &&
		    cmd != TIOCSETAF_32 && cmd != TIOCSETAF_64) {
			if (ISSET(t->c_lflag, ICANON)) {
				SET(tp->t_lflag, PENDIN);
			} else {
				  
                                             
                                               
                                              
                                 
       
				if (tp->t_rawq.c_cs && tp->t_canq.c_cs) {
					struct clist tq;

					catq(&tp->t_rawq, &tp->t_canq);
					tq = tp->t_rawq;
					tp->t_rawq = tp->t_canq;
					tp->t_canq = tq;
				}
				CLR(tp->t_lflag, PENDIN);
			}
			ttwakeup(tp);
		}
		tp->t_iflag = t->c_iflag;
		tp->t_oflag = t->c_oflag;
		  
                                    
     
		if (ISSET(tp->t_lflag, EXTPROC)) {
			SET(t->c_lflag, EXTPROC);
		} else {
			CLR(t->c_lflag, EXTPROC);
		}
		tp->t_lflag = t->c_lflag | ISSET(tp->t_lflag, PENDIN);
		if (t->c_cc[VMIN] != tp->t_cc[VMIN] ||
		    t->c_cc[VTIME] != tp->t_cc[VTIME]) {
			ttwakeup(tp);
		}
		bcopy(t->c_cc, tp->t_cc, sizeof(t->c_cc));
		break;
	}
	case TIOCSETD: {                                         
		int t = *(int *)data;
		dev_t device = tp->t_dev;

		if (t >= nlinesw || t < 0) {
			error = ENXIO;
			goto out;
		}
		  
                                                            
                                            
     
		if (t != tp->t_line) {
			(*linesw[tp->t_line].l_close)(tp, flag);
			error = (*linesw[t].l_open)(device, tp);
			if (error) {
				                                              
				(void)(*linesw[tp->t_line].l_open)(device, tp);
				goto out;
			}
			tp->t_line = t;
		}
		break;
	}
	case TIOCSTART:                                            
		if (ISSET(tp->t_state, TS_TTSTOP) ||
		    ISSET(tp->t_lflag, FLUSHO)) {
			CLR(tp->t_lflag, FLUSHO);
			CLR(tp->t_state, TS_TTSTOP);
			ttstart(tp);
		}
		break;
	case TIOCSTI:                                                
		if (suser(kauth_cred_get(), NULL) && (flag & FREAD) == 0) {
			error = EPERM;
			goto out;
		}
		if (suser(kauth_cred_get(), NULL) && !isctty(p, tp)) {
			error = EACCES;
			goto out;
		}
		(*linesw[tp->t_line].l_rint)(*(u_char *)data, tp);
		break;
	case TIOCSTOP:                                            
		if (!ISSET(tp->t_state, TS_TTSTOP)) {
			SET(tp->t_state, TS_TTSTOP);
			ttystop(tp, 0);
		}
		break;
	case TIOCIXON:
		ttyunblock(tp);
		break;
	case TIOCIXOFF:
		ttyblock(tp);
		break;
	case TIOCSCTTY:                                             
		                                                    
		sessp = proc_session(p);
		if (sessp == SESSION_NULL) {
			error = EPERM;
			goto out;
		}

		  
                                               
     
		if (!SESS_LEADER(p, sessp)) {
			                                               
			tty_unlock(tp);
			session_rele(sessp);
			tty_lock(tp);
			error = EPERM;
			goto out;
		}
		  
                                                                 
                                 
     
		if (tp->t_session == sessp) {
			                                               
			tty_unlock(tp);
			session_rele(sessp);
			tty_lock(tp);
			error = 0;
			goto out;
		}
		pg = proc_pgrp(p);
		  
                                                                   
                                              
     
		session_lock(sessp);
		if (sessp->s_ttyvp || tp->t_session) {
			session_unlock(sessp);
			                                               
			tty_unlock(tp);
			if (pg != PGRP_NULL) {
				pg_rele(pg);
			}
			session_rele(sessp);
			tty_lock(tp);
			error = EPERM;
			goto out;
		}
		sessp->s_ttypgrpid = pg->pg_id;
		oldtp = sessp->s_ttyp;
		ttyhold(tp);
		sessp->s_ttyp = tp;
		session_unlock(sessp);
		proc_list_lock();
		oldsessp = tp->t_session;
		oldpg = tp->t_pgrp;
		if (oldsessp != SESSION_NULL) {
			oldsessp->s_ttypgrpid = NO_PID;
		}
		                                                       
		tp->t_session = sessp;
		tp->t_pgrp = pg;
		proc_list_unlock();
		OSBitOrAtomic(P_CONTROLT, &p->p_flag);
		                                               
		tty_unlock(tp);
		                                                 
		if (oldsessp != SESSION_NULL) {
			session_rele(oldsessp);
		}
		if (oldpg != PGRP_NULL) {
			pg_rele(oldpg);
		}
		if (NULL != oldtp) {
			ttyfree(oldtp);
		}
		tty_lock(tp);
		break;

	case TIOCSPGRP: {                                    
		struct pgrp *pgrp = PGRP_NULL;

		sessp = proc_session(p);
		if (!isctty_sp(p, tp, sessp)) {
			if (sessp != SESSION_NULL) {
				session_rele(sessp);
			}
			error = ENOTTY;
			goto out;
		} else if ((pgrp = pgfind(*(int *)data)) == PGRP_NULL) {
			if (sessp != SESSION_NULL) {
				session_rele(sessp);
			}
			error = EINVAL;
			goto out;
		} else if (pgrp->pg_session != sessp) {
			                                               
			tty_unlock(tp);
			if (sessp != SESSION_NULL) {
				session_rele(sessp);
			}
			pg_rele(pgrp);
			tty_lock(tp);
			error = EPERM;
			goto out;
		}

		proc_list_lock();
		oldpg = tp->t_pgrp;
		tp->t_pgrp = pgrp;
		sessp->s_ttypgrpid = pgrp->pg_id;
		proc_list_unlock();

		  
                                                               
                   
    
                                                               
                                                             
                                                                
    
                                                                  
                                                               
                                                              
                                                                  
                                                              
     
		wakeup(TSA_HUP_OR_INPUT(tp));

		                                               
		tty_unlock(tp);
		if (oldpg != PGRP_NULL) {
			pg_rele(oldpg);
		}
		if (sessp != SESSION_NULL) {
			session_rele(sessp);
		}
		tty_lock(tp);
		break;
	}
	case TIOCSTAT:                                          
		ttyinfo_locked(tp);
		break;
	case TIOCSWINSZ:                                     
		if (bcmp((caddr_t)&tp->t_winsize, data,
		    sizeof(struct winsize))) {
			tp->t_winsize = *(struct winsize *)data;
			                                               
			tty_unlock(tp);
			tty_pgsignal(tp, SIGWINCH, 1);
			tty_lock(tp);
		}
		break;
	case TIOCSDRAINWAIT:
		error = suser(kauth_cred_get(), &p->p_acflag);
		if (error) {
			goto out;
		}
		tp->t_timeout = *(int *)data * hz;
		wakeup(TSA_OCOMPLETE(tp));
		wakeup(TSA_OLOWAT(tp));
		break;
	case TIOCGDRAINWAIT:
		*(int *)data = tp->t_timeout / hz;
		break;
	case TIOCREVOKE:
		SET(tp->t_state, TS_REVOKE);
		tp->t_gen++;
		  
                                                             
                                                            
                                                     
     
		wakeup(TSA_HUP_OR_INPUT(tp));
		break;
	case TIOCREVOKECLEAR:
		CLR(tp->t_state, TS_REVOKE);
		break;
	default:
		error = ttcompat(tp, cmd, data, flag, p);
		goto out;
	}

	error = 0;
out:
	return error;
}


  
                                                               
   
int
ttyselect(struct tty *tp, int rw, void *wql, proc_t p)
{
	int retval = 0;
	  
                                                                     
                                                                     
                   
    
	bool needs_selrecord = rw & FMARK;
	rw &= ~FMARK;

	if (tp == NULL) {
		return ENXIO;
	}

	TTY_LOCK_OWNED(tp);

	if (tp->t_state & TS_ZOMBIE) {
		retval = 1;
		goto out;
	}

	switch (rw) {
	case FREAD:
		retval = ttnread(tp);
		if (retval > 0) {
			break;
		}

		selrecord(p, &tp->t_rsel, wql);
		break;
	case FWRITE:
		if ((tp->t_outq.c_cc <= tp->t_lowat) &&
		    (tp->t_state & TS_CONNECTED)) {
			retval = tp->t_hiwat - tp->t_outq.c_cc;
			break;
		}

		selrecord(p, &tp->t_wsel, wql);
		break;
	}

out:
	if (retval > 0 && needs_selrecord) {
		switch (rw) {
		case FREAD:
			selrecord(p, &tp->t_rsel, wql);
			break;
		case FWRITE:
			selrecord(p, &tp->t_wsel, wql);
			break;
		}
	}

	return retval;
}


  
                                                                     
                                                      
  
                                                          
   
int
ttselect(dev_t dev, int rw, void *wql, proc_t p)
{
	int     rv;
	struct tty *tp = cdevsw[major(dev)].d_ttys[minor(dev)];

	tty_lock(tp);
	rv =  ttyselect(tp, rw, wql, p);
	tty_unlock(tp);

	return rv;
}


  
                                                               
   
__private_extern__ int
ttnread(struct tty *tp)
{
	int nread;

	TTY_LOCK_OWNED(tp);                       

	if (ISSET(tp->t_lflag, PENDIN)) {
		ttypend(tp);
	}
	nread = tp->t_canq.c_cc;
	if (!ISSET(tp->t_lflag, ICANON)) {
		nread += tp->t_rawq.c_cc;
		if (nread < tp->t_cc[VMIN] && tp->t_cc[VTIME] == 0) {
			nread = 0;
		}
	}
	return nread;
}


  
          
  
                            
  
                                                            
  
                       
                                     
                                                        
                                       
                         
                                                  
  
                                                   
  
                                                               
   
int
ttywait(struct tty *tp)
{
	int error;

	TTY_LOCK_OWNED(tp);                       

	error = 0;
	while ((tp->t_outq.c_cc || ISSET(tp->t_state, TS_BUSY)) &&
	    ISSET(tp->t_state, TS_CONNECTED) && tp->t_oproc) {
		(*tp->t_oproc)(tp);
		if ((tp->t_outq.c_cc || ISSET(tp->t_state, TS_BUSY)) &&
		    ISSET(tp->t_state, TS_CONNECTED)) {
			SET(tp->t_state, TS_SO_OCOMPLETE);
			error = ttysleep(tp, TSA_OCOMPLETE(tp),
			    TTOPRI | PCATCH, "ttywai",
			    tp->t_timeout);
			if (error) {
				if (error == EWOULDBLOCK) {
					error = EIO;
				}
				break;
			}
		} else {
			break;
		}
	}
	if (!error && (tp->t_outq.c_cc || ISSET(tp->t_state, TS_BUSY))) {
		error = EIO;
	}
	return error;
}

  
                                     
  
                                                      
   
static void
ttystop(struct tty *tp, int rw)
{
	TTY_LOCK_OWNED(tp);                       

	(*cdevsw[major(tp->t_dev)].d_stop)(tp, rw);
}

  
                              
  
                                                      
   
static int
ttywflush(struct tty *tp)
{
	int error;

	TTY_LOCK_OWNED(tp);                       

	if ((error = ttywait(tp)) == 0) {
		ttyflush(tp, FREAD);
	}
	return error;
}

  
                                                                
  
                                                      
   
void
ttyflush(struct tty *tp, int rw)
{
	TTY_LOCK_OWNED(tp);                       

#if 0
again:
#endif
	if (rw & FWRITE) {
		FLUSHQ(&tp->t_outq);
		CLR(tp->t_state, TS_TTSTOP);
	}
	ttystop(tp, rw);
	if (rw & FREAD) {
		FLUSHQ(&tp->t_canq);
		FLUSHQ(&tp->t_rawq);
		CLR(tp->t_lflag, PENDIN);
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		CLR(tp->t_state, TS_LOCAL);
		ttwakeup(tp);
		if (ISSET(tp->t_state, TS_TBLOCK)) {
			if (rw & FWRITE) {
				FLUSHQ(&tp->t_outq);
			}
			ttyunblock(tp);

			  
                                                      
                                                      
                                                      
                                                  
                                     
      
			CLR(tp->t_state, TS_TBLOCK);

#if 0                                                                         
			if (ISSET(tp->t_iflag, IXOFF)) {
				  
                                               
                                               
                                              
                                               
                                                 
                                                
                    
       
				SET(tp->t_state, TS_SO_OCOMPLETE);
				ttysleep(tp, TSA_OCOMPLETE(tp), TTOPRI,
				    "ttyfls", hz / 10);
				  
                                                  
       
				CLR(tp->t_state, TS_TBLOCK);
				goto again;
			}
#endif
		}
	}
	if (rw & FWRITE) {
		FLUSHQ(&tp->t_outq);
		ttwwakeup(tp);
	}
}

  
                                          
  
                                                      
  
                                           
   
void
termioschars(struct termios *t)
{
	bcopy(ttydefchars, t->c_cc, sizeof t->c_cc);
}


  
                                                                          
                                                                         
                                                                         
  
                                                  
   
void
ttyblock(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	SET(tp->t_state, TS_TBLOCK);
	if (ISSET(tp->t_iflag, IXOFF) && tp->t_cc[VSTOP] != _POSIX_VDISABLE &&
	    putc(tp->t_cc[VSTOP], &tp->t_outq) != 0) {
		CLR(tp->t_state, TS_TBLOCK);                         
	}
	ttstart(tp);
}


  
                                                                          
                                                                          
                                                                          
  
                                                  
   
static void
ttyunblock(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	CLR(tp->t_state, TS_TBLOCK);
	if (ISSET(tp->t_iflag, IXOFF) && tp->t_cc[VSTART] != _POSIX_VDISABLE &&
	    putc(tp->t_cc[VSTART], &tp->t_outq) != 0) {
		SET(tp->t_state, TS_TBLOCK);                         
	}
	ttstart(tp);
}


  
          
  
                   
  
                                                
  
                       
  
                                                  
  
                                                                        
  
                                                     
                                                            
   
int
ttstart(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	if (tp->t_oproc != NULL) {                                
		(*tp->t_oproc)(tp);
	}

	return 0;
}


  
                    
  
                            
  
                                                      
   
int
ttylclose(struct tty *tp, int flag)
{
	TTY_LOCK_OWNED(tp);                       

	if ((flag & FNONBLOCK) || ttywflush(tp)) {
		ttyflush(tp, FREAD | FWRITE);
	}

	return 0;
}


  
                   
  
                                            
                                       
                                                           
  
                                                      
   
int
ttymodem(struct tty *tp, int flag)
{
	int rval = 1;                                     

	TTY_LOCK_OWNED(tp);                       

	if (ISSET(tp->t_state, TS_CARR_ON) && ISSET(tp->t_cflag, MDMBUF)) {
		  
                                                      
                                                         
                                       
     
		if (flag) {
			CLR(tp->t_state, TS_CAR_OFLOW);
			CLR(tp->t_state, TS_TTSTOP);
			ttstart(tp);
		} else if (!ISSET(tp->t_state, TS_CAR_OFLOW)) {
			SET(tp->t_state, TS_CAR_OFLOW);
			SET(tp->t_state, TS_TTSTOP);
			ttystop(tp, 0);
		}
	} else if (flag == 0) {
		  
                  
     
		CLR(tp->t_state, TS_CARR_ON);
		if (ISSET(tp->t_state, TS_ISOPEN) &&
		    !ISSET(tp->t_cflag, CLOCAL)) {
			SET(tp->t_state, TS_ZOMBIE);
			CLR(tp->t_state, TS_CONNECTED);
			if (tp->t_session && tp->t_session->s_leader) {
				psignal(tp->t_session->s_leader, SIGHUP);
			}
			ttyflush(tp, FREAD | FWRITE);
			rval = 0;
			goto out;
		}
	} else {
		  
                    
     
		SET(tp->t_state, TS_CARR_ON);
		if (!ISSET(tp->t_state, TS_ZOMBIE)) {
			SET(tp->t_state, TS_CONNECTED);
		}
		wakeup(TSA_CARR_ON(tp));
		ttwakeup(tp);
		ttwwakeup(tp);
	}

out:
	return rval;
}


  
                                                
                    
  
                                                  
   
static void
ttypend(struct tty *tp)
{
	struct clist tq;
	int c;

	TTY_LOCK_OWNED(tp);                       

	CLR(tp->t_lflag, PENDIN);
	SET(tp->t_state, TS_TYPEN);
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = NULL;
	while ((c = getc(&tq)) >= 0) {
		ttyinput(c, tp);
	}
	CLR(tp->t_state, TS_TYPEN);
}


  
                 
  
                                       
  
                                                      
   
int
ttread(struct tty *tp, struct uio *uio, int flag)
{
	struct clist *qp;
	int c;
	tcflag_t lflag;
	cc_t *cc = tp->t_cc;
	proc_t p = current_proc();
	int first, error = 0;
	int has_etime = 0, last_cc = 0;
	long slp = 0;                                                   
	struct uthread *ut;
	struct pgrp * pg;

	TTY_LOCK_OWNED(tp);                       

	ut = (struct uthread *)get_bsdthread_info(current_thread());

loop:
	lflag = tp->t_lflag;
	  
                            
    
	if (ISSET(lflag, PENDIN)) {
		ttypend(tp);
		lflag = tp->t_lflag;                                   
	}

	  
                                                                    
                                                    
    
	if (isbackground(p, tp) || ISSET(tp->t_state, TS_REVOKE)) {
		if ((p->p_sigignore & sigmask(SIGTTIN)) ||
		    (ut->uu_sigmask & sigmask(SIGTTIN)) ||
		    p->p_lflag & P_LPPWAIT) {
			error = EIO;
			goto err;
		}
		pg = proc_pgrp(p);
		if (pg == PGRP_NULL) {
			error = EIO;
			goto err;
		}
		if (pg->pg_jobc == 0) {
			                                               
			tty_unlock(tp);
			pg_rele(pg);
			tty_lock(tp);
			error = EIO;
			goto err;
		}
		                                               
		tty_unlock(tp);
		pgsignal(pg, SIGTTIN, 1);
		pg_rele(pg);
		tty_lock(tp);

		  
                                                     
                                                     
                                                 
                                                        
     
		error = EINTR;
		goto err;
	}

	if (ISSET(tp->t_state, TS_ZOMBIE)) {
		                       
		goto err;
	}

	  
                                          
                           
   
                                 
    
	qp = ISSET(lflag, ICANON) ? &tp->t_canq : &tp->t_rawq;

	if (flag & IO_NDELAY) {
		if (qp->c_cc > 0) {
			goto read;
		}
		if (ISSET(lflag, ICANON) || cc[VMIN] != 0) {
			error = EWOULDBLOCK;
		}
		                                
		goto err;
	}
	if (!ISSET(lflag, ICANON)) {
		int m = cc[VMIN];
		long t = cc[VTIME];
		struct timeval timecopy;
		struct timeval etime = {.tv_sec = 0, .tv_usec = 0};                                  

		  
                                         
                                               
                                                            
                                             
                                                              
              
     
		if (t == 0) {
			if (qp->c_cc < m) {
				goto sleep;
			}
			if (qp->c_cc > 0) {
				goto read;
			}

			                                                      
			goto err;
		}
		t *= 100000;                            
#define diff(t1, t2) (((t1).tv_sec - (t2).tv_sec) * 1000000 + \
	                 ((t1).tv_usec - (t2).tv_usec))
		if (m > 0) {
			if (qp->c_cc <= 0) {
				goto sleep;
			}
			if (qp->c_cc >= m) {
				goto read;
			}
			microuptime(&timecopy);
			if (!has_etime || qp->c_cc > last_cc) {
				                                                     
				has_etime = 1;

				etime.tv_sec = t / 1000000;
				etime.tv_usec =
				    (__darwin_suseconds_t)(t - (etime.tv_sec * 1000000));
				timeradd(&etime, &timecopy, &etime);

				slp = t;
			} else {
				                               
				if (timercmp(&etime, &timecopy, <=)) {
					goto read;
				}

				slp = diff(etime, timecopy);
			}
			last_cc = qp->c_cc;
		} else {                    
			if (qp->c_cc > 0) {
				goto read;
			}
			microuptime(&timecopy);
			if (!has_etime) {
				has_etime = 1;

				etime.tv_sec = t / 1000000;
				etime.tv_usec =
				    (__darwin_suseconds_t)(t - (etime.tv_sec * 1000000));
				timeradd(&etime, &timecopy, &etime);

				slp = t;
			} else {
				if (timercmp(&etime, &timecopy, <=)) {
					                                       
					goto err;
				}
				slp = diff(etime, timecopy);
			}
		}
#undef diff
		  
                                                 
                                   
                                              
                                              
                                                   
                                                 
                                
     
		slp = (long) (((u_int32_t)slp * hz) + 999999) / 1000000;
		goto sleep;
	}
	if (qp->c_cc <= 0) {
sleep:
		  
                                                             
     
		error = ttysleep(tp, TSA_HUP_OR_INPUT(tp), TTIPRI | PCATCH,
		    ISSET(tp->t_state, TS_CONNECTED) ?
		    "ttyin" : "ttyhup", (int)slp);
		if (error == EWOULDBLOCK) {
			error = 0;
		} else if (error) {
			goto err;
		}
		  
                                                        
                                                      
                                                           
                             
     
		slp = 0;
		goto loop;
	}
read:
	  
                                                          
    
	first = 1;
	if (ISSET(lflag, ICANON)
	    || (ISSET(lflag, IEXTEN | ISIG) == (IEXTEN | ISIG))) {
		goto slowcase;
	}
	for (;;) {
		char ibuf[IBUFSIZ];
		int icc;
		ssize_t size = uio_resid(uio);
		if (size < 0) {
			error = ERANGE;
			break;
		}

		icc = (int)MIN(size, IBUFSIZ);
		icc = q_to_b(qp, (u_char *)ibuf, icc);
		if (icc <= 0) {
			if (first) {
				goto loop;
			}
			break;
		}
		error = uiomove(ibuf, icc, uio);
		  
                                                          
                                       
     
		if (error) {
			break;
		}
		if (uio_resid(uio) == 0) {
			break;
		}
		first = 0;
	}
	goto out;
slowcase:
	for (;;) {
		c = getc(qp);
		if (c < 0) {
			if (first) {
				goto loop;
			}
			break;
		}
		  
                         
     
		if (CCEQ(cc[VDSUSP], c) &&
		    ISSET(lflag, IEXTEN | ISIG) == (IEXTEN | ISIG)) {
			  
                                                   
                                                   
                                             
      
			tty_unlock(tp);
			tty_pgsignal(tp, SIGTSTP, 1);
			tty_lock(tp);
			if (first) {
				error = ttysleep(tp, &ttread, TTIPRI | PCATCH,
				    "ttybg3", hz);
				if (error) {
					break;
				}
				goto loop;
			}
			break;
		}
		  
                                          
     
		if (CCEQ(cc[VEOF], c) && ISSET(lflag, ICANON)) {
			break;
		}
		  
                         
     
		error = ureadc(c, uio);
		if (error) {
			                               
			break;
		}
		if (uio_resid(uio) == 0) {
			break;
		}
		  
                                                    
                                          
     
		if (ISSET(lflag, ICANON) && TTBREAKC(c, lflag)) {
			break;
		}
		first = 0;
	}

out:
	  
                                               
                                  
    
	if (ISSET(tp->t_state, TS_TBLOCK) &&
	    tp->t_rawq.c_cc + tp->t_canq.c_cc <= I_LOW_WATER) {
		ttyunblock(tp);
	}

err:
	return error;
}


  
                                                                            
                                                                          
                                                                            
                                                                              
          
  
                                                   
  
                                                              
   
int
ttycheckoutq(struct tty *tp, int wait)
{
	int hiwat;
	sigset_t oldsig;
	struct uthread *ut;

	TTY_LOCK_OWNED(tp);                       

	ut = (struct uthread *)get_bsdthread_info(current_thread());

	hiwat = tp->t_hiwat;
	oldsig = wait ? ut->uu_siglist : 0;
	if (tp->t_outq.c_cc > hiwat + OBUFSIZ + 100) {
		while (tp->t_outq.c_cc > hiwat) {
			ttstart(tp);
			if (tp->t_outq.c_cc <= hiwat) {
				break;
			}
			if (wait == 0 || ut->uu_siglist != oldsig) {
				return 0;
			}
			SET(tp->t_state, TS_SO_OLOWAT);
			ttysleep(tp, TSA_OLOWAT(tp), PZERO - 1, "ttoutq", hz);
		}
	}
	return 1;
}


  
                  
  
                                        
  
                                                      
   
int
ttwrite(struct tty *tp, struct uio *uio, int flag)
{
	char *cp = NULL;
	int cc, ce;
	proc_t p;
	int i, hiwat, error;
	user_ssize_t count;
	char obuf[OBUFSIZ];
	struct uthread *ut;
	struct pgrp * pg;

	TTY_LOCK_OWNED(tp);                       

	ut = (struct uthread *)get_bsdthread_info(current_thread());
	hiwat = tp->t_hiwat;
	count = uio_resid(uio);
	error = 0;
	cc = 0;
loop:
	if (ISSET(tp->t_state, TS_ZOMBIE)) {
		if (uio_resid(uio) == count) {
			error = EIO;
		}
		goto out;
	}
	if (!ISSET(tp->t_state, TS_CONNECTED)) {
		if (flag & IO_NDELAY) {
			error = EWOULDBLOCK;
			goto out;
		}
		error = ttysleep(tp, TSA_CARR_ON(tp), TTIPRI | PCATCH,
		    "ttydcd", 0);
		if (error) {
			goto out;
		}
		goto loop;
	}
	  
                                                 
    
	p = current_proc();
	if (isbackground(p, tp) &&
	    ISSET(tp->t_lflag, TOSTOP) && (p->p_lflag & P_LPPWAIT) == 0 &&
	    (p->p_sigignore & sigmask(SIGTTOU)) == 0 &&
	    (ut->uu_sigmask & sigmask(SIGTTOU)) == 0) {
		pg = proc_pgrp(p);
		if (pg == PGRP_NULL) {
			error = EIO;
			goto out;
		}
		if (pg->pg_jobc == 0) {
			                                               
			tty_unlock(tp);
			pg_rele(pg);
			tty_lock(tp);
			error = EIO;
			goto out;
		}
		                                               
		tty_unlock(tp);
		pgsignal(pg, SIGTTOU, 1);
		pg_rele(pg);
		tty_lock(tp);
		  
                                                     
                                                     
                                                 
                                                        
     
		error = EINTR;
		goto out;
	}
	  
                                                                   
                                                                
                                                        
    
	while (uio_resid(uio) > 0 || cc > 0) {
		if (ISSET(tp->t_lflag, FLUSHO)) {
			uio_setresid(uio, 0);
			return 0;
		}
		if (tp->t_outq.c_cc > hiwat) {
			goto ovhiwat;
		}
		  
                                                           
                             
     
		if (cc == 0) {
			ssize_t size = uio_resid(uio);
			if (size < 0) {
				error = ERANGE;
				break;
			}
			cc = (int)MIN((size_t)size, OBUFSIZ);
			cp = obuf;
			error = uiomove(cp, cc, uio);
			if (error) {
				cc = 0;
				break;
			}
		}
		  
                                                            
                                                         
                                                         
                                                          
                                                          
                                                             
                 
     
		while (cc > 0) {
			if (!ISSET(tp->t_oflag, OPOST)) {
				ce = cc;
			} else {
				ce = (int)((size_t)cc - scanc((size_t)cc,
				    (u_char *)cp, char_type, CCLASSMASK));
				  
                                           
                                             
       
				if (ce == 0) {
					tp->t_rocount = 0;
					if (ttyoutput(*cp, tp) >= 0) {
						                  
						goto overfull;
					}
					cp++;
					cc--;
					if (ISSET(tp->t_lflag, FLUSHO) ||
					    tp->t_outq.c_cc > hiwat) {
						goto ovhiwat;
					}
					continue;
				}
			}
			  
                                                   
                                                    
                                                 
                                                 
                                                       
                                              
      
			tp->t_rocount = 0;
			i = b_to_q((u_char *)cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_column += ce;
			cp += ce;
			cc -= ce;
			tk_nout += ce;
			tp->t_outcc += ce;
			if (i > 0) {
				                  
				goto overfull;
			}
			if (ISSET(tp->t_lflag, FLUSHO) ||
			    tp->t_outq.c_cc > hiwat) {
				break;
			}
		}
		ttstart(tp);
	}
out:
	  
                                                                     
                                                                     
                                                                  
    
	uio_setresid(uio, (uio_resid(uio) + cc));
	return error;

overfull:

	  
                                                                     
                                                                     
                                                                    
                      
    
	hiwat = tp->t_outq.c_cc - 1;

ovhiwat:
	ttstart(tp);
	  
                                                    
                                                      
    
	if (tp->t_outq.c_cc <= hiwat) {
		goto loop;
	}
	if (flag & IO_NDELAY) {
		uio_setresid(uio, (uio_resid(uio) + cc));
		return uio_resid(uio) == count ? EWOULDBLOCK : 0;
	}
	SET(tp->t_state, TS_SO_OLOWAT);
	error = ttysleep(tp, TSA_OLOWAT(tp), TTOPRI | PCATCH, "ttywri",
	    tp->t_timeout);
	if (error == EWOULDBLOCK) {
		error = EIO;
	}
	if (error) {
		goto out;
	}
	goto loop;
}


  
                                           
                          
  
                                                      
   
static void
ttyrub(int c, struct tty *tp)
{
	u_char *cp;
	int savecol;
	int tabc;

	TTY_LOCK_OWNED(tp);                       

	if (!ISSET(tp->t_lflag, ECHO) || ISSET(tp->t_lflag, EXTPROC)) {
		return;
	}
	CLR(tp->t_lflag, FLUSHO);
	if (ISSET(tp->t_lflag, ECHOE)) {
		if (tp->t_rocount == 0) {
			  
                                  
      
			ttyretype(tp);
			return;
		}
		if (c == ('\t' | TTY_QUOTE) || c == ('\n' | TTY_QUOTE)) {
			ttyrubo(tp, 2);
		} else {
			CLR(c, ~TTY_CHARMASK);
			switch (CCLASS(c)) {
			case ORDINARY:
				if (!(ISSET(tp->t_iflag, IUTF8) && CCONT(c))) {
					ttyrubo(tp, 1);
				}
				break;
			case BACKSPACE:
			case CONTROL:
			case NEWLINE:
			case RETURN:
			case VTAB:
				if (ISSET(tp->t_lflag, ECHOCTL)) {
					ttyrubo(tp, 2);
				}
				break;
			case TAB:
				if (tp->t_rocount < tp->t_rawq.c_cc) {
					ttyretype(tp);
					return;
				}
				savecol = tp->t_column;
				SET(tp->t_state, TS_CNTTB);
				SET(tp->t_lflag, FLUSHO);
				tp->t_column = tp->t_rocol;
				for (cp = firstc(&tp->t_rawq, &tabc); cp;
				    cp = nextc(&tp->t_rawq, cp, &tabc)) {
					ttyecho(tabc, tp);
				}
				CLR(tp->t_lflag, FLUSHO);
				CLR(tp->t_state, TS_CNTTB);

				                                            
				savecol -= tp->t_column;
				tp->t_column += savecol;
				if (savecol > 8) {
					savecol = 8;                        
				}
				while (--savecol >= 0) {
					(void)ttyoutput('\b', tp);
				}
				break;
			default:                                 
#define PANICSTR        "ttyrub: would panic c = %d, val = %d\n"
				printf(PANICSTR, c, CCLASS(c));
#ifdef notdef
				panic(PANICSTR, c, CCLASS(c));
#endif
			}
		}
	} else if (ISSET(tp->t_lflag, ECHOPRT)) {
		if (!ISSET(tp->t_state, TS_ERASE)) {
			SET(tp->t_state, TS_ERASE);
			(void)ttyoutput('\\', tp);
		}
		ttyecho(c, tp);
	} else {
		ttyecho(tp->t_cc[VERASE], tp);
	}
	--tp->t_rocount;
}


  
                                            
  
                                                      
   
static void
ttyrubo(struct tty *tp, int count)
{
	TTY_LOCK_OWNED(tp);                       

	while (count-- > 0) {
		(void)ttyoutput('\b', tp);
		(void)ttyoutput(' ', tp);
		(void)ttyoutput('\b', tp);
	}
}


  
               
                                                                    
                
  
                                                      
   
static void
ttyretype(struct tty *tp)
{
	u_char *cp;
	int c;

	TTY_LOCK_OWNED(tp);                       

	                                 
	if (tp->t_cc[VREPRINT] != _POSIX_VDISABLE) {
		ttyecho(tp->t_cc[VREPRINT], tp);
	}

	(void)ttyoutput('\n', tp);

	  
               
                                              
                      
    
	for (cp = firstc(&tp->t_canq, &c); cp; cp = nextc(&tp->t_canq, cp, &c)) {
		ttyecho(c, tp);
	}
	for (cp = firstc(&tp->t_rawq, &c); cp; cp = nextc(&tp->t_rawq, cp, &c)) {
		ttyecho(c, tp);
	}
	CLR(tp->t_state, TS_ERASE);

	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}


  
                                          
  
                                                      
   
static void
ttyecho(int c, struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	if (!ISSET(tp->t_state, TS_CNTTB)) {
		CLR(tp->t_lflag, FLUSHO);
	}
	if ((!ISSET(tp->t_lflag, ECHO) &&
	    (c != '\n' || !ISSET(tp->t_lflag, ECHONL))) ||
	    ISSET(tp->t_lflag, EXTPROC)) {
		return;
	}
	if (ISSET(tp->t_lflag, ECHOCTL) &&
	    ((ISSET(c, TTY_CHARMASK) <= 037 && c != '\t' && c != '\n') ||
	    ISSET(c, TTY_CHARMASK) == 0177)) {
		(void)ttyoutput('^', tp);
		CLR(c, ~TTY_CHARMASK);
		if (c == 0177) {
			c = '?';
		} else {
			c += 'A' - 1;
		}
	}
	(void)ttyoutput(c, tp);
}

static void
ttwakeup_knote(struct selinfo *sip, long hint)
{
	if ((sip->si_flags & SI_KNPOSTING) == 0) {
		sip->si_flags |= SI_KNPOSTING;
		KNOTE(&sip->si_note, hint);
		sip->si_flags &= ~SI_KNPOSTING;
	}
}


  
                                
  
                                                  
   
void
ttwakeup(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	selwakeup(&tp->t_rsel);
	ttwakeup_knote(&tp->t_rsel, 0);
	if (ISSET(tp->t_state, TS_ASYNC)) {
		  
                                                         
                                                          
                                                           
                                                         
                                                        
                        
     
		tty_unlock(tp);
		tty_pgsignal(tp, SIGIO, 1);
		tty_lock(tp);
	}
	wakeup(TSA_HUP_OR_INPUT(tp));
}


  
                    
  
                                
  
                                                      
   
void
ttwwakeup(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		selwakeup(&tp->t_wsel);
		ttwakeup_knote(&tp->t_wsel, 0);
	}
	if (ISSET(tp->t_state, TS_BUSY | TS_SO_OCOMPLETE) ==
	    TS_SO_OCOMPLETE && tp->t_outq.c_cc == 0) {
		CLR(tp->t_state, TS_SO_OCOMPLETE);
		wakeup(TSA_OCOMPLETE(tp));
	}
	if (ISSET(tp->t_state, TS_SO_OLOWAT) &&
	    tp->t_outq.c_cc <= tp->t_lowat) {
		CLR(tp->t_state, TS_SO_OLOWAT);
		wakeup(TSA_OLOWAT(tp));
	}
}


  
                                                              
                                                                       
  
                                                             
                               
   
int
ttspeedtab(int speed, struct speedtab *table)
{
	for (; table->sp_speed != -1; table++) {
		if (table->sp_speed == speed) {
			return table->sp_code;
		}
	}
	return -1;
}


  
                                  
  
                                                          
                        
  
                                                      
   
void
ttsetwater(struct tty *tp)
{
	speed_t cps;
	unsigned int x;

	TTY_LOCK_OWNED(tp);                       

#define CLAMP(x, h, l)  ((x) > h ? h : ((x) < l) ? l : (x))

	cps = tp->t_ospeed / 10;
	static_assert(TTMAXLOWAT <= UINT_MAX, "max low water fits in unsigned int");
	static_assert(TTMINLOWAT <= UINT_MAX, "min low water fits in unsigned int");
	tp->t_lowat = x = (unsigned int)CLAMP(cps / 2, TTMAXLOWAT, TTMINLOWAT);
	x += cps;
	x = CLAMP(x, TTMAXHIWAT, TTMINHIWAT);
	tp->t_hiwat = roundup(x, CBSIZE);
#undef  CLAMP
}

                                                   
#include <mach/thread_info.h>

                                                                    
extern kern_return_t    thread_info_internal(thread_t thread,
    thread_flavor_t flavor,
    thread_info_t thread_info_out,
    mach_msg_type_number_t *thread_info_count);


  
                                               
  
                                                      
   
void
ttyinfo_locked(struct tty *tp)
{
	int             load;
	thread_t        thread;
	uthread_t       uthread;
	proc_t          p;
	proc_t          pick;
	pid_t pickpid;
	const char      *state;
	struct timeval  utime;
	struct timeval  stime;
	thread_basic_info_data_t        basic_info;
	mach_msg_type_number_t          mmtn = THREAD_BASIC_INFO_COUNT;
	struct pgrp * pg;

	TTY_LOCK_OWNED(tp);                       

	if (ttycheckoutq(tp, 0) == 0) {
		return;
	}

	                         
	load = (averunnable.ldavg[0] * 100 + FSCALE / 2) >> FSHIFT;
	ttyprintf(tp, "load: %d.%02d ", load / 100, load % 100);

	  
                                                                   
                                             
    
	if (tp->t_session == NULL) {
		ttyprintf(tp, "not a controlling terminal\n");
		tp->t_rocount = 0;
		return;
	}
	if (tp->t_pgrp == NULL) {
		ttyprintf(tp, "no foreground process group\n");
		tp->t_rocount = 0;
		return;
	}
	                                    
	                                         
	if ((p = tp->t_pgrp->pg_members.lh_first) == NULL) {
		ttyprintf(tp, "empty foreground process group\n");
		tp->t_rocount = 0;
		return;
	}

	  
                                                          
                             
    
	pg = proc_pgrp(p);
	pgrp_lock(pg);
	                                                                  
	for (pick = NULL; p != NULL; p = p->p_pglist.le_next) {
		if (proc_compare(pick, p)) {
			pick = p;
			pickpid = p->p_pid;
		} else {
			pickpid = pick->p_pid;
		}
	}
	pgrp_unlock(pg);
	                                               
	tty_unlock(tp);
	pg_rele(pg);
	tty_lock(tp);

	pick = proc_find(pickpid);
	if (pick == PROC_NULL) {
		return;
	}

	if (TAILQ_EMPTY(&pick->p_uthlist) ||
	    (uthread = TAILQ_FIRST(&pick->p_uthlist)) == NULL ||
	    (thread = vfs_context_thread(&uthread->uu_context)) == NULL ||
	    (thread_info_internal(thread, THREAD_BASIC_INFO, (thread_info_t)&basic_info, &mmtn) != KERN_SUCCESS)) {
		ttyprintf(tp, "foreground process without thread\n");
		tp->t_rocount = 0;
		proc_rele(pick);
		return;
	}

	switch (basic_info.run_state) {
	case TH_STATE_RUNNING:
		state = "running";
		break;
	case TH_STATE_STOPPED:
		state = "stopped";
		break;
	case TH_STATE_WAITING:
		state = "waiting";
		break;
	case TH_STATE_UNINTERRUPTIBLE:
		state = "uninterruptible";
		break;
	case TH_STATE_HALTED:
		state = "halted";
		break;
	default:
		state = "unknown";
		break;
	}
	calcru(pick, &utime, &stime, NULL);

	                                                 
	ttyprintf(tp, " cmd: %s %d %s %ld.%02du %ld.%02ds\n",
	    pick->p_comm,
	    pick->p_pid,
	    state,
	    (long)utime.tv_sec, utime.tv_usec / 10000,
	    (long)stime.tv_sec, stime.tv_usec / 10000);

	proc_rele(pick);
	tp->t_rocount = 0;
}


  
                                      
  
                                                               
  
                                                       
                                                                    
                                                                      
                                        
                                                       
                                                         
   
#define ISRUN(p)        (((p)->p_stat == SRUN) || ((p)->p_stat == SIDL))
#define TESTAB(a, b)    ((a)<<1 | (b))
#define ONLYA   2
#define ONLYB   1
#define BOTH    3

  
                                                     
                                                             
                                           
   
static int
proc_compare(proc_t p1, proc_t p2)
{
	                                           

	if (p1 == NULL) {
		return 1;
	}
	  
                                           
    
	switch (TESTAB(ISRUN(p1), ISRUN(p2))) {
	case ONLYA:
		return 0;
	case ONLYB:
		return 1;
	case BOTH:
		  
                                                        
     
#ifdef _PROC_HAS_SCHEDINFO_
		                                                    
		if (p2->p_estcpu > p1->p_estcpu) {
			return 1;
		}
		if (p1->p_estcpu > p2->p_estcpu) {
			return 0;
		}
#endif                           
		return p2->p_pid > p1->p_pid;                               
	}
	  
                    
    
	switch (TESTAB(p1->p_stat == SZOMB, p2->p_stat == SZOMB)) {
	case ONLYA:
		return 1;
	case ONLYB:
		return 0;
	case BOTH:
		return p2->p_pid > p1->p_pid;                               
	}
	  
                                             
    
#ifdef _PROC_HAS_SCHEDINFO_
	                                                    
	if (p2->p_slptime > p1->p_slptime) {
		return 0;
	}
	if (p1->p_slptime > p2->p_slptime) {
		return 1;
	}
#endif                           
	return p2->p_pid > p1->p_pid;                                       
}


  
                                             
  
                                                      
  
                                                       
   
int
tputchar(int c, struct tty *tp)
{
	TTY_LOCK_OWNED(tp);                       

	if (!ISSET(tp->t_state, TS_CONNECTED)) {
		return -1;
	}
	if (c == '\n') {
		(void)ttyoutput('\r', tp);
	}
	(void)ttyoutput(c, tp);
	ttstart(tp);
	return 0;
}


  
           
  
                                                                          
                                
  
                                      
                                                
                             
                            
                                               
                                          
                                
  
                                   
                                                
                                       
                         
                                                 
                                                          
                                                      
  
                                                      
  
                                                                       
                                                                         
                                                                          
                            
   
int
ttysleep(struct tty *tp, void *chan, int pri, const char *wmesg, int timo)
{
	int error;
	int gen;

	TTY_LOCK_OWNED(tp);

	if (tp->t_state & TS_REVOKE) {
		return ERESTART;
	}

	gen = tp->t_gen;
	                                                           
	error = msleep0(chan, &tp->t_lock, pri, wmesg, timo, (int (*)(int))0);
	if (error) {
		return error;
	}
	return tp->t_gen == gen ? 0 : ERESTART;
}


  
                                                       
  
                   
  
                                              
                            
  
                                                                
                
   
struct tty *
ttymalloc(void)
{
	struct tty *tp;

	tp = kheap_alloc(KM_TTYS, sizeof(struct tty), Z_WAITOK | Z_ZERO);
	if (tp != NULL) {
		                                                   
		clalloc(&tp->t_rawq, TTYCLSIZE, 1);
		clalloc(&tp->t_canq, TTYCLSIZE, 1);
		                                       
		clalloc(&tp->t_outq, TTYCLSIZE, 0);
		lck_mtx_init(&tp->t_lock, &tty_lck_grp, LCK_ATTR_NULL);
		klist_init(&tp->t_rsel.si_note);
		klist_init(&tp->t_wsel.si_note);
		tp->t_refcnt = 1;
	}
	return tp;
}

  
                                          
   
void
ttyhold(struct tty *tp)
{
	TTY_LOCK_OWNED(tp);
	tp->t_refcnt++;
}

  
                                                                             
                                                              
   
void
ttyfree(struct tty *tp)
{
	TTY_LOCK_NOTOWNED(tp);

	tty_lock(tp);
	if (--tp->t_refcnt == 0) {
		tty_unlock(tp);
		ttydeallocate(tp);
	} else if (tp->t_refcnt < 0) {
		panic("%s: freeing free tty %p", __func__, tp);
	} else {
		tty_unlock(tp);
	}
}

  
                                              
  
                                                                 
                                               
   
static void
ttydeallocate(struct tty *tp)
{
	TTY_LOCK_NOTOWNED(tp);                    

#if DEBUG
	if (!(SLIST_EMPTY(&tp->t_rsel.si_note) && SLIST_EMPTY(&tp->t_wsel.si_note))) {
		panic("knotes hooked into a tty when the tty is freed.\n");
	}
#endif            

	clfree(&tp->t_rawq);
	clfree(&tp->t_canq);
	clfree(&tp->t_outq);
	lck_mtx_destroy(&tp->t_lock, &tty_lck_grp);
	kheap_free(KM_TTYS, tp, sizeof(struct tty));
}


  
                                                      
   
int
isbackground(proc_t p, struct tty  *tp)
{
	TTY_LOCK_OWNED(tp);

	return tp->t_session != NULL && p->p_pgrp != NULL && (p->p_pgrp != tp->t_pgrp) && isctty_sp(p, tp, p->p_pgrp->pg_session);
}

static int
isctty(proc_t p, struct tty  *tp)
{
	int retval;
	struct session * sessp;

	sessp = proc_session(p);
	retval = (sessp == tp->t_session && p->p_flag & P_CONTROLT);
	session_rele(sessp);
	return retval;
}

static int
isctty_sp(proc_t p, struct tty  *tp, struct session *sessp)
{
	return sessp == tp->t_session && p->p_flag & P_CONTROLT;
}


static int  filt_ttyattach(struct knote *kn, struct kevent_qos_s *kev);
static void filt_ttydetach(struct knote *kn);
static int  filt_ttyevent(struct knote *kn, long hint);
static int  filt_ttytouch(struct knote *kn, struct kevent_qos_s *kev);
static int  filt_ttyprocess(struct knote *kn, struct kevent_qos_s *kev);

SECURITY_READ_ONLY_EARLY(struct filterops) tty_filtops = {
	.f_isfd    = 1,
	.f_attach  = filt_ttyattach,
	.f_detach  = filt_ttydetach,
	.f_event   = filt_ttyevent,
	.f_touch   = filt_ttytouch,
	.f_process = filt_ttyprocess
};

  
                                                                              
              
   
static int
filt_tty_common(struct knote *kn, struct kevent_qos_s *kev, struct tty *tp)
{
	int retval = 0;
	int64_t data = 0;

	TTY_LOCK_OWNED(tp);                   

	switch (kn->kn_filter) {
	case EVFILT_READ:
		  
                                      
                                                        
     
		data = ttnread(tp);
		retval = (data != 0);
		break;
	case EVFILT_WRITE:
		if ((tp->t_outq.c_cc <= tp->t_lowat) &&
		    (tp->t_state & TS_CONNECTED)) {
			data = tp->t_hiwat - tp->t_outq.c_cc;
			retval = (data != 0);
		}
		break;
	default:
		panic("tty kevent: unexpected filter: %d, kn = %p, tty = %p",
		    kn->kn_filter, kn, tp);
		break;
	}

	  
                                                                    
                                                                   
   
                                                
                                                       
    

	if (tp->t_state & TS_ZOMBIE) {
		kn->kn_flags |= EV_EOF;
	}
	if (kn->kn_flags & EV_EOF) {
		retval = 1;
	}
	if (retval && kev) {
		knote_fill_kevent(kn, kev, data);
	}

	return retval;
}

  
                                                                               
                                                                               
   
static struct tty *
tty_from_waitq(struct waitq *wq, int seltype)
{
	struct selinfo *si;
	struct tty *tp = NULL;

	  
                                                                         
                                                               
                                                          
   
                                                                      
                                                                     
         
    
	si = qe_element(wq, struct selinfo, si_waitq);

	  
                                                                     
                                                                     
                                                
   
                                                                      
                                                                     
         
    
	switch (seltype) {
	case FREAD:
		tp = qe_element(si, struct tty, t_rsel);
		break;
	case FWRITE:
		tp = qe_element(si, struct tty, t_wsel);
		break;
	}

	return tp;
}

static struct tty *
tty_from_knote(struct knote *kn)
{
	return (struct tty *)kn->kn_hook;
}

  
                                                   
  
                                                                              
                                                                         
             
  
                                                                                
   
static int
tty_set_knote_hook(struct knote *kn)
{
	uthread_t uth;
	vfs_context_t ctx;
	vnode_t vp;
	kern_return_t kr;
	struct waitq *wq = NULL;
	struct waitq_set *old_wqs;
	struct waitq_set tmp_wqs;
	uint64_t rsvd, rsvd_arg;
	uint64_t *rlptr = NULL;
	int selres = -1;
	struct tty *tp;

	uth = get_bsdthread_info(current_thread());

	ctx = vfs_context_current();
	vp = (vnode_t)kn->kn_fp->fp_glob->fg_data;

	  
                                                              
               
    
	rsvd = rsvd_arg = waitq_link_reserve(NULL);
	rlptr = (void *)&rsvd_arg;

	  
                                                                            
                                                                            
                                                            
   
                                                                        
                                         
    
	kr = waitq_set_init(&tmp_wqs, SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST, NULL,
	    NULL);
	assert(kr == KERN_SUCCESS);

	  
                                                                  
               
    
	waitq_set_lazy_init_link(&tmp_wqs);

	old_wqs = uth->uu_wqset;
	uth->uu_wqset = &tmp_wqs;
	  
                                                                  
                                                   
   
                                                                 
    
	selres = VNOP_SELECT(vp, knote_get_seltype(kn) | FMARK, 0, rlptr, ctx);
	uth->uu_wqset = old_wqs;

	  
                                                                
                                                   
    
	waitq_link_release(rsvd);
	if (rsvd == rsvd_arg) {
		  
                                                                       
                  
     
		knote_set_error(kn, ENOTTY);
		selres = -1;
		goto out;
	}

	                                                       
	memcpy(&wq, rlptr, sizeof(void *));

	tp = tty_from_waitq(wq, knote_get_seltype(kn));
	assert(tp != NULL);

	  
                                                    
    
	tty_lock(tp);
	ttyhold(tp);
	kn->kn_hook = tp;
	tty_unlock(tp);

out:
	  
                                                                         
                                                                           
    
	waitq_set_deinit(&tmp_wqs);

	return selres;
}

static int
filt_ttyattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	int selres = 0;
	struct tty *tp;

	  
                                                                       
                                                                      
    

	                                                                        
	if (kn->kn_vnode_use_ofst || !kn->kn_vnode_kqok) {
		knote_set_error(kn, ENOTSUP);
		return 0;
	}

	  
                                                                     
                                               
    
	selres = tty_set_knote_hook(kn);
	if (selres < 0) {
		return 0;
	}

	  
                                        
    
	tp = tty_from_knote(kn);
	tty_lock(tp);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		KNOTE_ATTACH(&tp->t_rsel.si_note, kn);
		break;
	case EVFILT_WRITE:
		KNOTE_ATTACH(&tp->t_wsel.si_note, kn);
		break;
	default:
		panic("invalid knote %p attach, filter: %d", kn, kn->kn_filter);
	}

	tty_unlock(tp);

	return selres;
}

static void
filt_ttydetach(struct knote *kn)
{
	struct tty *tp = tty_from_knote(kn);

	tty_lock(tp);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		KNOTE_DETACH(&tp->t_rsel.si_note, kn);
		break;
	case EVFILT_WRITE:
		KNOTE_DETACH(&tp->t_wsel.si_note, kn);
		break;
	default:
		panic("invalid knote %p detach, filter: %d", kn, kn->kn_filter);
		break;
	}

	tty_unlock(tp);
	ttyfree(tp);
}

static int
filt_ttyevent(struct knote *kn, long hint)
{
	struct tty *tp = tty_from_knote(kn);
	int ret;

	TTY_LOCK_OWNED(tp);

	if (hint & NOTE_REVOKE) {
		kn->kn_flags |= EV_EOF | EV_ONESHOT;
		ret = 1;
	} else {
		ret = filt_tty_common(kn, NULL, tp);
	}

	return ret;
}

static int
filt_ttytouch(struct knote *kn, struct kevent_qos_s *kev)
{
	struct tty *tp = tty_from_knote(kn);
	int res = 0;

	tty_lock(tp);

	kn->kn_sdata = kev->data;
	kn->kn_sfflags = kev->fflags;

	if (kn->kn_vnode_kqok) {
		res = filt_tty_common(kn, NULL, tp);
	}

	tty_unlock(tp);

	return res;
}

static int
filt_ttyprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct tty *tp = tty_from_knote(kn);
	int res;

	tty_lock(tp);

	res = filt_tty_common(kn, kev, tp);

	tty_unlock(tp);

	return res;
}
