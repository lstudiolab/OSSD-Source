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
                                                 
  
                                                  
                                    
                 
  
                                                            
                                                                 
                                                                 
                                                          
                                                            
                                                           
                                                                 
                                                                   
                                      
                                                         
                                                            
                                                            
                                                             
                                                                
                                                                
  
           
                                                             
                                                           
                                                            
            
  
                                                               
                                                            
                                                                 
                                                                 
                                                              
               
  
                                                               
                                                                   
                                                                  
           
  
   
#include "config.h"

int
searchp(const char *spath, char *file, char *fullname, int (*func)(char *))
{
	const char *nextpath, *nextchar, *lastchar;
	char *fname;
	int failure;

	nextpath = ((*file == '/') ? "" : spath);
	do {
		fname = fullname;
		nextchar = nextpath;
		while (*nextchar && (*nextchar != ':')) {
			*fname++ = *nextchar++;
		}
		if (nextchar != nextpath && *file) {
			*fname++ = '/';
		}
		lastchar = nextchar;
		nextpath = ((*nextchar) ? nextchar + 1 : nextchar);
		nextchar = file;                         
		while (*nextchar) {
			*fname++ = *nextchar++;
		}
		*fname = '\0';
		failure = (*func)(fullname);
	}while (failure && (*lastchar));
	return failure ? -1 : 0;
}
