/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/

#include <sys/dtrace.h>
#include <sys/dtrace_glue.h>

#include <sys/dis_tables.h>

  
                                                                            
                                                                             
                      
  
                                                                               
                                                                         
                                                                             
                             
  
                                                                      
                                                                           
                                           
   

typedef enum dis_isize {
	DIS_ISIZE_INSTR,
	DIS_ISIZE_OPERAND
} dis_isize_t;


  
                                     
   
static int
dtrace_dis_get_byte(void *p)
{
	int ret;
	uchar_t **instr = p;

	ret = **instr;
	*instr += 1;

	return (ret);
}

  
                                                                                
                                                                           
                                                                             
                                                                               
                                                                         
                                       
   
               
static __attribute__((noinline)) int
dtrace_dis_isize(uchar_t *instr, dis_isize_t which, model_t model, int *rmindex)
{
	int sz;
	dis86_t	x;
	uint_t mode = SIZE32;

	mode = (model == DATAMODEL_LP64) ? SIZE64 : SIZE32;

	x.d86_data = (void **)&instr;
	x.d86_get_byte = dtrace_dis_get_byte;
	x.d86_check_func = NULL;

	if (dtrace_disx86(&x, mode) != 0)
		return (-1);

	if (which == DIS_ISIZE_INSTR)
		sz = x.d86_len;		                               
	else
		sz = x.d86_memsize;	                              

	if (rmindex != NULL)
		*rmindex = x.d86_rmindex;
	return (sz);
}

int
dtrace_instr_size_isa(uchar_t *instr, model_t model, int *rmindex)
{
	return (dtrace_dis_isize(instr, DIS_ISIZE_INSTR, model, rmindex));
}

int
dtrace_instr_size(uchar_t *instr)
{
	return (dtrace_dis_isize(instr, DIS_ISIZE_INSTR, DATAMODEL_NATIVE,
	    NULL));
}
