/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */
#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_OBSOLETE
#define LABELSECTOR     (1024 / DEV_BSIZE)                                   
#define LABELOFFSET     0                                                      
#define MAXPARTITIONS   8                                                 
#define RAW_PART        2                                                

                  
struct cpu_disklabel {
	int     cd_dummy;                                                   
};
#endif                           

#endif                            
