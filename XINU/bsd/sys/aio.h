/*
 * Copyright (c) 2003-2006 Apple Computer, Inc. All rights reserved.
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
  
                  
                                                
                                
  
                                             
  
   

#ifndef _SYS_AIO_H_
#define _SYS_AIO_H_

#include <sys/signal.h>
#include <sys/_types.h>
#include <sys/cdefs.h>

  
                                                                         
                                                                     
  
                                                                      
   
#include <sys/_types/_timespec.h>
#ifdef KERNEL
#include <sys/_types/_user64_timespec.h>
#include <sys/_types/_user32_timespec.h>
#endif             

#include <sys/_types/_off_t.h>
#include <sys/_types/_ssize_t.h>

  
                                                                         
                                                                           
                                          
  
                       
   
#include <sys/_types/_o_sync.h>
#include <sys/_types/_o_dsync.h>

#ifndef KERNEL
struct aiocb {
	int             aio_fildes;                                  
	off_t           aio_offset;                              
	volatile void   *aio_buf;                                       
	size_t          aio_nbytes;                                     
	int             aio_reqprio;                                         
	struct sigevent aio_sigevent;                                        
	int             aio_lio_opcode;                                        
};
#endif             

#ifdef KERNEL

struct user_aiocb {
	int             aio_fildes;                                  
	off_t           aio_offset;                  
	user_addr_t     aio_buf;                                        
	user_size_t     aio_nbytes;                                     
	int             aio_reqprio;                                 
	struct user_sigevent aio_sigevent;                                   
	int             aio_lio_opcode;                                        
};

struct user64_aiocb {
	int             aio_fildes;                                  
	user64_off_t            aio_offset;                  
	user64_addr_t   aio_buf;                                        
	user64_size_t   aio_nbytes;                                     
	int             aio_reqprio;                                 
	struct user64_sigevent aio_sigevent;                                 
	int             aio_lio_opcode;                                        
};

struct user32_aiocb {
	int             aio_fildes;                                  
	user32_off_t            aio_offset;                              
	user32_addr_t   aio_buf;                                        
	user32_size_t   aio_nbytes;                                     
	int             aio_reqprio;                                         
	struct user32_sigevent  aio_sigevent;                                        
	int             aio_lio_opcode;                                        
};

#endif          

  
                             
   

  
                                                                    
                    
   
#define AIO_ALLDONE                     0x1

                                                 
#define AIO_CANCELED            0x2

  
                                                               
                       
   
#define AIO_NOTCANCELED         0x4


  
                               
   

#define LIO_NOP                 0x0                                                          
#define LIO_READ                0x1                                           
#define LIO_WRITE               0x2                                            

  
                     
   

  
                                                      
                                                         
                                                        
                                                       
   
#define LIO_NOWAIT              0x1

  
                                                      
                                                  
                                      
   
#define LIO_WAIT                0x2

  
                                                         
   
#define AIO_LISTIO_MAX          16


#ifndef KERNEL
  
             
   

__BEGIN_DECLS

  
                                                                                
                                                                                 
                                                                                  
                                                                                     
   
int             aio_cancel( int fd,
    struct aiocb * aiocbp );

  
                                                                                
                                                                                   
                                                                            
                                                                                  
                             
   
int             aio_error( const struct aiocb * aiocbp );

  
                                                                                
                                                                                 
                                                                           
                                                                              
                                                                               
                                     
                                                                              
                                                                               
                                                                                  
                                                                                  
                                                                          
                                                                                 
                    
   
int             aio_fsync( int op,
    struct aiocb * aiocbp );

  
                                                                                
                                                                                 
                                                 
                                                                                 
                                                                              
                                                                          
                                                                            
                                                                              
                                                                                  
                                                                              
                                                                                
                                                                           
   
int             aio_read( struct aiocb * aiocbp );

  
                                                                             
                                                                               
                                                                            
                                                                            
                                                                             
                                                                                 
                                                                             
                                                                              
                                                                           
                                                                        
                                                
   
ssize_t aio_return( struct aiocb * aiocbp );

  
                                                                        
                                                                                
                                                                      
                                                                            
                                                                                 
                                                                           
                                                                             
                                                                              
                                                                             
                                                                         
                                                                              
                                                      
   
int             aio_suspend( const struct aiocb *const aiocblist[],
    int nent,
    const struct timespec * timeoutp ) __DARWIN_ALIAS_C(aio_suspend);

  
                                                                               
                                                                                
                                                             
                                                                                 
                                                                                 
                                                 
   
int             aio_write( struct aiocb * aiocbp );

  
                                                                         
                                                                                 
                                                                               
                                                                             
                                                                              
           
                                                                                 
                                                                                 
                                                                                  
               
   
int             lio_listio( int mode,
    struct aiocb *const aiocblist[],
    int nent,
    struct sigevent *sigp );
__END_DECLS

#endif             
#endif                  
