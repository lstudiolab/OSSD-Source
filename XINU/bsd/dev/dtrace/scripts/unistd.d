/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

inline int DTRACEFLT_UNKNOWN = 0;	                   
#pragma D binding "1.0" DTRACEFLT_UNKNOWN

inline int DTRACEFLT_BADADDR = 1;	                 
#pragma D binding "1.0" DTRACEFLT_BADADDR

inline int DTRACEFLT_BADALIGN = 2;	                   
#pragma D binding "1.0" DTRACEFLT_BADALIGN

inline int DTRACEFLT_ILLOP = 3;		                       
#pragma D binding "1.0" DTRACEFLT_ILLOP

inline int DTRACEFLT_DIVZERO = 4;	                    
#pragma D binding "1.0" DTRACEFLT_DIVZERO

inline int DTRACEFLT_NOSCRATCH = 5;	                          
#pragma D binding "1.0" DTRACEFLT_NOSCRATCH

inline int DTRACEFLT_KPRIV = 6;		                           
#pragma D binding "1.0" DTRACEFLT_KPRIV

inline int DTRACEFLT_UPRIV = 7;		                         
#pragma D binding "1.0" DTRACEFLT_UPRIV

inline int DTRACEFLT_TUPOFLOW = 8;	                          
#pragma D binding "1.0" DTRACEFLT_TUPOFLOW

inline int DTRACEFLT_BADSTACK = 9;	               
#pragma D binding "1.4.1" DTRACEFLT_BADSTACK
