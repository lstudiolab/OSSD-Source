/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */
/*
 * Copyright (c) 1997 by Apple Computer, Inc., all rights reserved
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/sysproto.h>
#include <sys/user.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <vm/vm_map.h>


  
                                                                            
                                                                           
                                                                        
                                                                      
                                                                       
             
   
int
copyoutstr(const void *from, user_addr_t to, size_t maxlen, size_t * lencopied)
{
	size_t          slen;
	size_t          len;
	int             error = copyoutstr_prevalidate(from, to, maxlen);

	if (__improbable(error)) {
		return error;
	}

	slen = strlen(from) + 1;
	if (slen > maxlen) {
		error = ENAMETOOLONG;
	}

	len = MIN(maxlen, slen);
	if (copyout(from, to, len)) {
		error = EFAULT;
	}
	*lencopied = len;

	return error;
}


  
                                                                        
                                                                          
                                                                    
                                                                            
                                                              
   
                                              
int
copystr(const void *vfrom, void *vto, size_t maxlen, size_t * lencopied)
{
	size_t          l;
	char const     *from = (char const *) vfrom;
	char           *to = (char *) vto;

	for (l = 0; l < maxlen; l++) {
		if ((*to++ = *from++) == '\0') {
			if (lencopied) {
				*lencopied = l + 1;
			}
			return 0;
		}
	}
	if (lencopied) {
		*lencopied = maxlen;
	}
	return ENAMETOOLONG;
}

int
copywithin(void *src, void *dst, size_t count)
{
	bcopy(src, dst, count);
	return 0;
}

