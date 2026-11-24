/*
 * Copyright (c) 1999-2009 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)config.h	5.8 (Berkeley) 6/18/88
 */

  
          
   

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

struct file_list {
	struct  file_list *f_next;
	char    *f_fn;                                
	u_char  f_type;                                
	u_char  f_flags;                               
	short   f_special;                                              
	char    *f_needs;
	char    *f_extra;                                              
};

  
         
   
#define DRIVER          1
#define NORMAL          2
#define INVISIBLE       3
#define PROFILING       4

  
                      
   
#define CONFIGDEP       0x01                   
#define OPTIONSDEF      0x02                                  
#define LIBRARYDEP      0x04                                       

struct device {
	int     d_type;                                                      
	const char      *d_name;                                        
	int     d_slave;                                  
#define QUES    -1                        
#define UNKNOWN -2                                
	int     d_flags;                                           
	struct  device *d_next;                               
	char    *d_init;                                                     
};

  
                                                      
                                                           
                                                                      
                                                                
                         
   
extern const char       *machinename;

  
                                                                  
                                                      
   
extern const char *source_directory;
extern const char *object_directory;
extern char *config_directory;

FILE *fopenp(const char *fpath, char *file, char *complete, const char *ftype);
const char *get_VPATH(void);
#define VPATH   get_VPATH()

  
                                                                   
                                                     
                                                                   
   
struct opt {
	char    *op_name;
	char    *op_value;
	struct  opt *op_next;
};

extern struct opt *opt, *mkopt, *opt_tail, *mkopt_tail;

const char      *get_word(FILE *fp);
char    *ns(const char *str);
char    *qu(int num);
char    *path(const char *file);

extern int      do_trace;

extern struct   device *dtab;
dev_t   nametodev(char *name, int defunit, char defpartition);
char    *devtoname(dev_t dev);

extern char     errbuf[80];
extern int      yyline;

extern struct   file_list *ftab, *conf_list, **confp;
extern char     *build_directory;

extern int      profiling;

#define eq(a, b) (!strcmp(a,b))

#define DEV_MASK 0x7
#define DEV_SHIFT  3

                                  
char *get_rest(FILE *fp);

int yyparse(void);
void yyerror(const char *s);

void mkioconf(void);

void makefile(void);
void headers(void);
int opteq(const char *cp, const char *dp);

void init_dev(struct device *dp);
void newdev(struct device *dp);
void dev_param(struct device *dp, const char *str, long num);

int searchp(const char *spath, char *file, char *fullname, int (*func)(char *));
