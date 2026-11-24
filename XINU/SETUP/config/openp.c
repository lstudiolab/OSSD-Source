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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */
                                                    
  
          
                                            
                                       
                    
           
                                    
  
                                                     
                                                      
                                                                 
                                                        
                                                            
                                                            
                                                                
                  
  
           
                                                               
                                                             
                                                               
                                             
  
                                                               
                   
  
   

#include <stdio.h>
#include <fcntl.h>                
#include "config.h"


int openp(const char *fpath, char *file, char *complete, int flags, int mode);

static int flgs, mod, value;
static const char *ftyp;
static FILE *fvalue;

static int
func(char *fnam)
{
	value = open(fnam, flgs, mod);
	return value < 0;
}

static int
ffunc(char *fnam)
{
	fvalue = fopen(fnam, ftyp);
	return fvalue == 0;
}

int
openp(const char *fpath, char *file, char *complete, int flags, int mode)
{
	flgs = flags;
	mod = mode;
	if (searchp(fpath, file, complete, func) < 0) {
		return -1;
	}
	return value;
}

FILE *
fopenp(const char *fpath, char *file, char *complete, const char *ftype)
{
	ftyp = ftype;
	if (searchp(fpath, file, complete, ffunc) < 0) {
		return 0;
	}
	return fvalue;
}
