/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1987 NeXT, Inc.
 */

struct consdev {
	char    *cn_name;                                            
	int     (*cn_probe)(void);                                          
	int     (*cn_init)(void);                               
	int     (*cn_getc)(void);                                     
	int     (*cn_putc)(void);                                     
	struct  tty *cn_tp;                                           
	dev_t   cn_dev;                                    
	short   cn_pri;                                                   
};

                                                                  
#define CN_DEAD         0                                 
#define CN_NORMAL       1                                                 
#define CN_INTERNAL     2                                          
#define CN_REMOTE       3                                                 

         
#define CONSMAJOR       0

#ifdef KERNEL

#include <sys/types.h>
#include <sys/conf.h>

extern  struct consdev constab[];
extern  struct consdev *cn_tab;
extern  struct tty *cn_tty;

extern struct tty       *constty;                                           

int consopen(dev_t, int, int, struct proc *);
int consclose(dev_t, int, int, struct proc *);
int consread(dev_t, struct uio *, int);
int conswrite(dev_t, struct uio *, int);
int consioctl(dev_t, u_long, caddr_t, int, struct proc *);
int consselect(dev_t, int, void *, struct proc *);

  
                                                                       
                                                                           
                       
   
int kmopen(dev_t, int, int, struct proc *);
int kmclose(dev_t, int, int, struct proc *);
int kmread(dev_t, struct uio *, int);
int kmwrite(dev_t, struct uio *, int);
int kmioctl(dev_t, u_long, caddr_t, int, struct proc *);
int kmputc(dev_t, char);

#endif
