/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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
/* Copyright (c) 1997 Apple Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      @(#)tty_compat.c        8.1 (Berkeley) 6/10/93
 */

  
                                                          
  
                                                                        
                                                                         
                                 
  
                                                               
                                                           
           
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc_internal.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/file_internal.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>

static int ttcompatgetflags(struct tty *tp);
static void ttcompatsetflags(struct tty *tp, struct termios *t);
static void ttcompatsetlflags(struct tty *tp, struct termios *t);
static unsigned int ttcompatspeedtab(speed_t speed, struct speedtab *table);

  
                                                                    
                                                                         
                                                                      
                                                                        
              
   
static struct speedtab compatspeeds[] = {
#define MAX_SPEED 17
	{ .sp_speed = 115200, .sp_code = 17 },
	{ .sp_speed = 57600, .sp_code = 16 },
	{ .sp_speed = 38400, .sp_code = 15 },
	{ .sp_speed = 19200, .sp_code = 14 },
	{ .sp_speed = 9600, .sp_code = 13 },
	{ .sp_speed = 4800, .sp_code = 12 },
	{ .sp_speed = 2400, .sp_code = 11 },
	{ .sp_speed = 1800, .sp_code = 10 },
	{ .sp_speed = 1200, .sp_code = 9 },
	{ .sp_speed = 600, .sp_code = 8 },
	{ .sp_speed = 300, .sp_code = 7 },
	{ .sp_speed = 200, .sp_code = 6 },
	{ .sp_speed = 150, .sp_code = 5 },
	{ .sp_speed = 134, .sp_code = 4 },
	{ .sp_speed = 110, .sp_code = 3 },
	{ .sp_speed = 75, .sp_code = 2 },
	{ .sp_speed = 50, .sp_code = 1 },
	{ .sp_speed = 0, .sp_code = 0 },
	{ .sp_speed = -1, .sp_code = -1 },
};
static int compatspcodes[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200,
};

  
                   
  
                                                                        
                                                                   
                                                              
                                                   
  
                                       
                                                              
  
                                                             
                                                           
                                                           
                                                          
                                                            
                                                              
                                                               
                                                       
  
                                                                     
   
static unsigned int
ttcompatspeedtab(speed_t speed, struct speedtab *table)
{
	if (speed == 0) {
		return 0;             
	}
	for (; table->sp_speed > 0; table++) {
		if (table->sp_speed <= speed) {                                
			return (unsigned int)table->sp_code;
		}
	}
	return 1;                             
}


  
              
  
                                                                       
            
  
                        
                        
                        
                        
                        
                        
                        
  
                                                                 
                                                            
                                                           
  
                                                               
                       
                                                       
                                            
                                            
                             
                                                       
                                     
                                                             
                                           
                                        
  
                                                      
                                        
                                    
                                 
  
                                                                  
                                                               
                          
  
                                                               
                                                              
                                                         
                                                          
  
                                                                    
                                                         
                                                            
                            
   
static int
ttsetcompat(struct tty *tp, u_long *com, caddr_t data, struct termios *term)
{
	switch (*com) {
	case TIOCSETP:
	  
                                                            
                                                         
                                                         
                                                        
    
	case TIOCSETN:
		  
                                                             
                                    
     
	{
		__IGNORE_WCASTALIGN(struct sgttyb *sg = (struct sgttyb *)data);
		if (sg->sg_ispeed < 0) {
			return EINVAL;
		}
		unsigned int ispeed = (unsigned int)sg->sg_ispeed;
		if (ispeed > MAX_SPEED) {
			return EINVAL;
		}
		if (ispeed != ttcompatspeedtab(tp->t_ispeed, compatspeeds)) {
			term->c_ispeed = compatspcodes[ispeed];
		} else {
			term->c_ispeed = tp->t_ispeed;
		}

		  
                                                          
                                              
    
                                       
                                     
    
                                      
     
		if (sg->sg_ospeed < 0) {
			return EINVAL;
		}
		unsigned int ospeed = (unsigned int)sg->sg_ospeed;
		if (ospeed > MAX_SPEED) {
			return EINVAL;
		}
		if (ospeed != ttcompatspeedtab(tp->t_ospeed, compatspeeds)) {
			term->c_ospeed = compatspcodes[ospeed];
		} else {
			term->c_ospeed = tp->t_ospeed;
		}

		term->c_cc[VERASE] = sg->sg_erase;
		term->c_cc[VKILL] = sg->sg_kill;
		tp->t_flags = (tp->t_flags & 0xffff0000) | (sg->sg_flags & 0xffff);
		ttcompatsetflags(tp, term);
		*com = (*com == TIOCSETP) ? TIOCSETAF : TIOCSETA;
		break;
	}
	case TIOCSETC:
		  
                                                            
                                             
     
	{
		__IGNORE_WCASTALIGN(struct tchars *tc = (struct tchars *)data);
		cc_t *cc;

		cc = term->c_cc;
		cc[VINTR] = tc->t_intrc;
		cc[VQUIT] = tc->t_quitc;
		cc[VSTART] = tc->t_startc;
		cc[VSTOP] = tc->t_stopc;
		cc[VEOF] = tc->t_eofc;
		cc[VEOL] = tc->t_brkc;
		if (tc->t_brkc == -1) {
			cc[VEOL2] = _POSIX_VDISABLE;
		}
		*com = TIOCSETA;
		break;
	}
	case TIOCSLTC:
		  
                                                            
                                              
     
	{
		__IGNORE_WCASTALIGN(struct ltchars *ltc = (struct ltchars *)data);
		cc_t *cc;

		cc = term->c_cc;
		cc[VSUSP] = ltc->t_suspc;
		cc[VDSUSP] = ltc->t_dsuspc;
		cc[VREPRINT] = ltc->t_rprntc;
		cc[VDISCARD] = ltc->t_flushc;
		cc[VWERASE] = ltc->t_werasc;
		cc[VLNEXT] = ltc->t_lnextc;
		*com = TIOCSETA;
		break;
	}
	case TIOCLBIS:
	  
                                                       
                                                     
                                                   
    
	case TIOCLBIC:
	  
                                                         
                                                           
                                             
    
	case TIOCLSET:
		  
                                                             
                                                           
            
     
	{
		__IGNORE_WCASTALIGN(int set = *(int *)data);
		if (*com == TIOCLSET) {
			tp->t_flags = (tp->t_flags & 0xffff) | set << 16;
		} else {
			tp->t_flags =
			    (ttcompatgetflags(tp) & 0xffff0000) | (tp->t_flags & 0xffff);
			if (*com == TIOCLBIS) {
				tp->t_flags |= set << 16;
			} else {
				tp->t_flags &= ~(set << 16);
			}
		}
		ttcompatsetlflags(tp, term);
		*com = TIOCSETA;
		break;
	}
	}
	return 0;
}

  
           
  
                                                                        
                                                              
                                                                
                                                              
                                                             
  
                                                               
                       
                                                 
                       
                                                       
                                 
                                               
                                                       
                 
  
                                                     
                             
                                              
                                          
                                           
                                           
                         
                                                
                                         
                                               
                                     
  
                                                                   
                                                                
            
   
            
__private_extern__ int
ttcompat(struct tty *tp, u_long com, caddr_t data, int flag, struct proc *p)
{
	switch (com) {
	case TIOCSETP:
	case TIOCSETN:
	case TIOCSETC:
	case TIOCSLTC:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
		  
                                                              
                               
     
	{
		struct termios term;
		int error;

		term = tp->t_termios;
		if ((error = ttsetcompat(tp, &com, data, &term)) != 0) {
			return error;
		}
		return ttioctl_locked(tp, com, (caddr_t) &term, flag, p);
	}
	case TIOCGETP:
		  
                                                        
                                                    
     
	{
		__IGNORE_WCASTALIGN(struct sgttyb *sg = (struct sgttyb *)data);
		cc_t *cc = tp->t_cc;

		static_assert(MAX_SPEED <= CHAR_MAX, "maximum speed fits in a char");
		sg->sg_ospeed = (char)ttcompatspeedtab(tp->t_ospeed, compatspeeds);
		if (tp->t_ispeed == 0) {
			sg->sg_ispeed = sg->sg_ospeed;
		} else {
			sg->sg_ispeed = (char)ttcompatspeedtab(tp->t_ispeed, compatspeeds);
		}
		sg->sg_erase = cc[VERASE];
		sg->sg_kill = cc[VKILL];
		tp->t_flags = ttcompatgetflags(tp);
		sg->sg_flags = (short)tp->t_flags;
		break;
	}
	case TIOCGETC:
		  
                                                        
                                  
     
	{
		struct tchars *tc = (struct tchars *)data;
		cc_t *cc = tp->t_cc;

		tc->t_intrc = cc[VINTR];
		tc->t_quitc = cc[VQUIT];
		tc->t_startc = cc[VSTART];
		tc->t_stopc = cc[VSTOP];
		tc->t_eofc = cc[VEOF];
		tc->t_brkc = cc[VEOL];
		break;
	}
	case TIOCGLTC:
		  
                                                        
                                   
     
	{
		struct ltchars *ltc = (struct ltchars *)data;
		cc_t *cc = tp->t_cc;

		ltc->t_suspc = cc[VSUSP];
		ltc->t_dsuspc = cc[VDSUSP];
		ltc->t_rprntc = cc[VREPRINT];
		ltc->t_flushc = cc[VDISCARD];
		ltc->t_werasc = cc[VWERASE];
		ltc->t_lnextc = cc[VLNEXT];
		break;
	}
	case TIOCLGET:
		  
                                                            
                                
     
		tp->t_flags =
		    (ttcompatgetflags(tp) & 0xffff0000UL)
		    | (tp->t_flags & 0xffff);
		*(int *)data = tp->t_flags >> 16;
		break;

	case OTIOCGETD:
		  
                                                            
               
     
		*(int *)data = tp->t_line ? tp->t_line : 2;
		break;

	case OTIOCSETD:
		  
                                                              
                              
     
	{
		int ldisczero = 0;

		return ttioctl_locked(tp, TIOCSETD,
		           *(int *)data == 2 ? (caddr_t)&ldisczero : data, flag, p);
	}

	case OTIOCCONS:
		  
                               
     
		*(int *)data = 1;
		return ttioctl_locked(tp, TIOCCONS, data, flag, p);

	case TIOCGSID:
		  
                                                           
     
		if (tp->t_session == NULL) {
			return ENOTTY;
		}

		if (tp->t_session->s_leader == NULL) {
			return ENOTTY;
		}

		*(int *) data =  tp->t_session->s_leader->p_pid;
		break;

	default:
		  
                                             
     
		return ENOTTY;
	}

	  
                               
    
	return 0;
}

  
                   
  
                                                                             
                                                         
  
                                                               
                       
  
                                                  
                                         
            
  
                                                                     
                                                                 
   
static int
ttcompatgetflags(struct tty *tp)
{
	tcflag_t iflag  = tp->t_iflag;
	tcflag_t lflag  = tp->t_lflag;
	tcflag_t oflag  = tp->t_oflag;
	tcflag_t cflag  = tp->t_cflag;
	int flags = 0;

	if (iflag & IXOFF) {
		flags |= TANDEM;
	}
	if (iflag & ICRNL || oflag & ONLCR) {
		flags |= CRMOD;
	}
	if ((cflag & CSIZE) == CS8) {
		flags |= PASS8;
		if (iflag & ISTRIP) {
			flags |= ANYP;
		}
	} else if (cflag & PARENB) {
		if (iflag & INPCK) {
			if (cflag & PARODD) {
				flags |= ODDP;
			} else {
				flags |= EVENP;
			}
		} else {
			flags |= EVENP | ODDP;
		}
	}

	if ((lflag & ICANON) == 0) {
		           
		if (iflag & (INPCK | ISTRIP | IXON) || lflag & (IEXTEN | ISIG)
		    || (cflag & (CSIZE | PARENB)) != CS8) {
			flags |= CBREAK;
		} else {
			flags |= RAW;
		}
	}
	if (!(flags & RAW) && !(oflag & OPOST) && (cflag & (CSIZE | PARENB)) == CS8) {
		flags |= LITOUT;
	}
	if (cflag & MDMBUF) {
		flags |= MDMBUF;
	}
	if ((cflag & HUPCL) == 0) {
		flags |= NOHANG;
	}
	if (oflag & OXTABS) {
		flags |= XTABS;
	}
	if (lflag & ECHOE) {
		flags |= CRTERA | CRTBS;
	}
	if (lflag & ECHOKE) {
		flags |= CRTKIL | CRTBS;
	}
	if (lflag & ECHOPRT) {
		flags |= PRTERA;
	}
	if (lflag & ECHOCTL) {
		flags |= CTLECH;
	}
	if ((iflag & IXANY) == 0) {
		flags |= DECCTQ;
	}
	flags |= lflag & (ECHO | TOSTOP | FLUSHO | PENDIN | NOFLSH);
	return flags;
}

  
                   
  
                                                                             
                                                                  
                            
  
                                                                
                       
                                                         
                                  
  
                                           
   
static void
ttcompatsetflags(struct tty *tp, struct termios *t)
{
	int flags = tp->t_flags;
	tcflag_t iflag  = t->c_iflag;
	tcflag_t oflag  = t->c_oflag;
	tcflag_t lflag  = t->c_lflag;
	tcflag_t cflag  = t->c_cflag;

	if (flags & RAW) {
		iflag = IGNBRK;
		lflag &= ~(ECHOCTL | ISIG | ICANON | IEXTEN);
	} else {
		iflag &= ~(PARMRK | IGNPAR | IGNCR | INLCR);
		iflag |= BRKINT | IXON | IMAXBEL;
		lflag |= ISIG | IEXTEN | ECHOCTL;                             
		if (flags & XTABS) {
			oflag |= OXTABS;
		} else {
			oflag &= ~OXTABS;
		}
		if (flags & CBREAK) {
			lflag &= ~ICANON;
		} else {
			lflag |= ICANON;
		}
		if (flags & CRMOD) {
			iflag |= ICRNL;
			oflag |= ONLCR;
		} else {
			iflag &= ~ICRNL;
			oflag &= ~ONLCR;
		}
	}
	if (flags & ECHO) {
		lflag |= ECHO;
	} else {
		lflag &= ~ECHO;
	}

	cflag &= ~(CSIZE | PARENB);
	if (flags & (RAW | LITOUT | PASS8)) {
		cflag |= CS8;
		if (!(flags & (RAW | PASS8))
		    || (flags & (RAW | PASS8 | ANYP)) == (PASS8 | ANYP)) {
			iflag |= ISTRIP;
		} else {
			iflag &= ~ISTRIP;
		}
		if (flags & (RAW | LITOUT)) {
			oflag &= ~OPOST;
		} else {
			oflag |= OPOST;
		}
	} else {
		cflag |= CS7 | PARENB;
		iflag |= ISTRIP;
		oflag |= OPOST;
	}
	                                          
	if ((flags & (EVENP | ODDP)) == EVENP) {
		iflag |= INPCK;
		cflag &= ~PARODD;
	} else if ((flags & (EVENP | ODDP)) == ODDP) {
		iflag |= INPCK;
		cflag |= PARODD;
	} else {
		iflag &= ~INPCK;
	}
	if (flags & TANDEM) {
		iflag |= IXOFF;
	} else {
		iflag &= ~IXOFF;
	}
	if ((flags & DECCTQ) == 0) {
		iflag |= IXANY;
	} else {
		iflag &= ~IXANY;
	}
	t->c_iflag = iflag;
	t->c_oflag = oflag;
	t->c_lflag = lflag;
	t->c_cflag = cflag;
}

  
                    
  
                                                                        
                                                         
                                                               
  
                                                                
                       
                                                         
                                        
  
                                           
   
static void
ttcompatsetlflags(struct tty *tp, struct termios *t)
{
	int flags = tp->t_flags;
	tcflag_t iflag  = t->c_iflag;
	tcflag_t oflag  = t->c_oflag;
	tcflag_t lflag  = t->c_lflag;
	tcflag_t cflag  = t->c_cflag;

	iflag &= ~(PARMRK | IGNPAR | IGNCR | INLCR);
	if (flags & CRTERA) {
		lflag |= ECHOE;
	} else {
		lflag &= ~ECHOE;
	}
	if (flags & CRTKIL) {
		lflag |= ECHOKE;
	} else {
		lflag &= ~ECHOKE;
	}
	if (flags & PRTERA) {
		lflag |= ECHOPRT;
	} else {
		lflag &= ~ECHOPRT;
	}
	if (flags & CTLECH) {
		lflag |= ECHOCTL;
	} else {
		lflag &= ~ECHOCTL;
	}
	if (flags & TANDEM) {
		iflag |= IXOFF;
	} else {
		iflag &= ~IXOFF;
	}
	if ((flags & DECCTQ) == 0) {
		iflag |= IXANY;
	} else {
		iflag &= ~IXANY;
	}
	if (flags & MDMBUF) {
		cflag |= MDMBUF;
	} else {
		cflag &= ~MDMBUF;
	}
	if (flags & NOHANG) {
		cflag &= ~HUPCL;
	} else {
		cflag |= HUPCL;
	}
	lflag &= ~(TOSTOP | FLUSHO | PENDIN | NOFLSH);
	lflag |= flags & (TOSTOP | FLUSHO | PENDIN | NOFLSH);

	  
                                                                   
                                                              
                                                               
                                                                  
                                                                    
                                       
    
	cflag &= ~(CSIZE | PARENB);
	if (flags & (RAW | LITOUT | PASS8)) {
		cflag |= CS8;
		if (!(flags & (RAW | PASS8))
		    || (flags & (RAW | PASS8 | ANYP)) == (PASS8 | ANYP)) {
			iflag |= ISTRIP;
		} else {
			iflag &= ~ISTRIP;
		}
		if (flags & (RAW | LITOUT)) {
			oflag &= ~OPOST;
		} else {
			oflag |= OPOST;
		}
	} else {
		cflag |= CS7 | PARENB;
		iflag |= ISTRIP;
		oflag |= OPOST;
	}
	t->c_iflag = iflag;
	t->c_oflag = oflag;
	t->c_lflag = lflag;
	t->c_cflag = cflag;
}
