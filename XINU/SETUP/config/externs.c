/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */
/* Copyright (c) Apple Computer, Inc. All rights reserved. */

#include <config.h>


  
                                                      
                                                           
                                                                      
                                                                
                         
   
const char      *machinename;

  
                                                                  
                                                      
   
const char *source_directory;
const char *object_directory;
char *config_directory;

  
                                                                   
                                                     
                                                                   
   
struct opt *opt, *mkopt, *opt_tail, *mkopt_tail;

int     do_trace;

struct  device *dtab;

char    errbuf[80];
int     yyline;

struct  file_list *ftab, *conf_list, **confp;
char    *build_directory;

int     profiling = 0;
