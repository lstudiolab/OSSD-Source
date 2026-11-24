/*
 * Copyright (c) 1998-2010 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#ifndef IOKIT_IOTIMESTAMP_H
#define IOKIT_IOTIMESTAMP_H

#include <sys/kdebug.h>

static inline void
IOTimeStampStartConstant(unsigned int csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG_CONSTANT(((uint32_t)csc) | DBG_FUNC_START, a, b, c, d, 0);
}

static inline void
IOTimeStampEndConstant(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG_CONSTANT(((uint32_t)csc) | DBG_FUNC_END, a, b, c, d, 0);
}

static inline void
IOTimeStampConstant(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG_CONSTANT(((uint32_t)csc) | DBG_FUNC_NONE, a, b, c, d, 0);
}

static inline void
IOTimeStampConstantFiltered(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG_CONSTANT_FILTERED(((uint32_t)csc) | DBG_FUNC_NONE, a, b, c, d, 0);
}

  
                                                                           
                                                             
                                                                
                                         
                                                                
   
class IOTimeStampIntervalConstantFiltered
{
public:
	IOTimeStampIntervalConstantFiltered(unsigned int csc,
	    uintptr_t arg1 = 0, uintptr_t arg2 = 0,
	    uintptr_t arg3 = 0, uintptr_t arg4 = 0)
		: _csc(csc), _endArg1(0), _endArg2(0), _endArg3(0), _endArg4(0)
	{
		(void)csc;
		(void)arg1;
		(void)arg2;
		(void)arg3;
		(void)arg4;
		KDBG_FILTERED(((uint32_t)csc) | DBG_FUNC_START, arg1, arg2, arg3, arg4);
	}
	                                  
	void
	setEndCodes(uintptr_t arg1 = 0, uintptr_t arg2 = 0,
	    uintptr_t arg3 = 0, uintptr_t arg4 = 0)
	{
		_endArg1 = arg1;
		_endArg2 = arg2;
		_endArg3 = arg3;
		_endArg4 = arg4;
	}
	void
	setEndArg1(uintptr_t arg)
	{
		_endArg1  = arg;
	}
	void
	setEndArg2(uintptr_t arg)
	{
		_endArg2  = arg;
	}
	void
	setEndArg3(uintptr_t arg)
	{
		_endArg3  = arg;
	}
	void
	setEndArg4(uintptr_t arg)
	{
		_endArg4  = arg;
	}
	~IOTimeStampIntervalConstantFiltered()
	{
		KDBG_FILTERED(((uint32_t)_csc) | DBG_FUNC_END, _endArg1, _endArg2, _endArg3, _endArg4);
	}
private:
#if (KDEBUG_LEVEL < KDEBUG_LEVEL_STANDARD)
	__unused
#endif                                           
	unsigned int _csc;
	                  
#if (KDEBUG_LEVEL < KDEBUG_LEVEL_STANDARD)
	__unused
#endif                                           
	uintptr_t _endArg1, _endArg2, _endArg3, _endArg4;
};

#if KDEBUG

static inline void
IOTimeStampStart(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG(((uint32_t)csc) | DBG_FUNC_START, a, b, c, d, 0);
}

static inline void
IOTimeStampEnd(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG(((uint32_t)csc) | DBG_FUNC_END, a, b, c, d, 0);
}

static inline void
IOTimeStamp(uintptr_t csc,
    uintptr_t a = 0, uintptr_t b = 0,
    uintptr_t c = 0, uintptr_t d = 0)
{
	(void)csc;
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	KERNEL_DEBUG(((uint32_t)csc) | DBG_FUNC_NONE, a, b, c, d, 0);
}

#endif             

#define IODBG_STORAGE(code)                     (KDBG_CODE(DBG_IOKIT, DBG_IOSTORAGE, code))
#define IODBG_NETWORK(code)                     (KDBG_CODE(DBG_IOKIT, DBG_IONETWORK, code))
#define IODBG_KEYBOARD(code)            (KDBG_CODE(DBG_IOKIT, DBG_IOKEYBOARD, code))
#define IODBG_HID(code)                         (KDBG_CODE(DBG_IOKIT, DBG_IOHID, code))
#define IODBG_AUDIO(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOAUDIO, code))
#define IODBG_SERIAL(code)                      (KDBG_CODE(DBG_IOKIT, DBG_IOSERIAL, code))
#define IODBG_TTY(code)                         (KDBG_CODE(DBG_IOKIT, DBG_IOTTY, code))
#define IODBG_SAM(code)                         (KDBG_CODE(DBG_IOKIT, DBG_IOSAM, code))
#define IODBG_PARALLELATA(code)         (KDBG_CODE(DBG_IOKIT, DBG_IOPARALLELATA, code))
#define IODBG_PARALLELSCSI(code)        (KDBG_CODE(DBG_IOKIT, DBG_IOPARALLELSCSI, code))
#define IODBG_SATA(code)                        (KDBG_CODE(DBG_IOKIT, DBG_IOSATA, code))
#define IODBG_SAS(code)                         (KDBG_CODE(DBG_IOKIT, DBG_IOSAS, code))
#define IODBG_FIBRECHANNEL(code)        (KDBG_CODE(DBG_IOKIT, DBG_IOFIBRECHANNEL, code))
#define IODBG_USB(code)                         (KDBG_CODE(DBG_IOKIT, DBG_IOUSB, code))
#define IODBG_BLUETOOTH(code)           (KDBG_CODE(DBG_IOKIT, DBG_IOBLUETOOTH, code))
#define IODBG_FIREWIRE(code)            (KDBG_CODE(DBG_IOKIT, DBG_IOFIREWIRE, code))
#define IODBG_INFINIBAND(code)          (KDBG_CODE(DBG_IOKIT, DBG_IOINFINIBAND, code))


                             
#define IODBG_DISK(code)                        IODBG_STORAGE(code)
#define IODBG_POINTING(code)            IODBG_HID(code)


                                     
#define IODBG_INTC(code)                        (KDBG_CODE(DBG_IOKIT, DBG_IOINTC, code))
#define IODBG_WORKLOOP(code)            (KDBG_CODE(DBG_IOKIT, DBG_IOWORKLOOP, code))
#define IODBG_INTES(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOINTES, code))
#define IODBG_TIMES(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOCLKES, code))
#define IODBG_CMDQ(code)                        (KDBG_CODE(DBG_IOKIT, DBG_IOCMDQ, code))
#define IODBG_MCURS(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOMCURS, code))
#define IODBG_MDESC(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOMDESC, code))
#define IODBG_POWER(code)                       (KDBG_CODE(DBG_IOKIT, DBG_IOPOWER, code))
#define IODBG_IOSERVICE(code)           (KDBG_CODE(DBG_IOKIT, DBG_IOSERVICE, code))
#define IODBG_IOREGISTRY(code)          (KDBG_CODE(DBG_IOKIT, DBG_IOREGISTRY, code))
#define IODBG_IOMDPA(code)                      (KDBG_CODE(DBG_IOKIT, DBG_IOMDPA, code))

                                                 

                                

                                   

                                    

                               

                                 

                                  

                               

                                
#define IOINTC_HANDLER  1                       
#define IOINTC_SPURIOUS 2                       

                                    
#define IOWL_CLIENT             1                       
#define IOWL_WORK               2                       

                                 
#define IOINTES_CLIENT  1                       
#define IOINTES_LAT             2                       
#define IOINTES_SEMA    3                       
#define IOINTES_INTCTXT 4                       
#define IOINTES_INTFLTR 5                       
#define IOINTES_ACTION  6                       
#define IOINTES_FILTER  7                       

                                 
#define IOTIMES_CLIENT  1                       
#define IOTIMES_LAT             2                       
#define IOTIMES_SEMA    3                       
#define IOTIMES_ACTION  4                       

                                
#define IOCMDQ_CLIENT   1                       
#define IOCMDQ_LAT              2                       
#define IOCMDQ_SEMA             3                       
#define IOCMDQ_PSEMA    4                       
#define IOCMDQ_PLOCK    5                       
#define IOCMDQ_ACTION   6                       

                                 

                                 
#define IOMDESC_WIRE 1                       
#define IOMDESC_PREPARE 2                    
#define IOMDESC_MAP 3                        
#define IOMDESC_UNMAP 4                      
#define IOMDESC_DMA_MAP 5                    
#define IOMDESC_DMA_UNMAP 6                  
#define IOMDESC_COMPLETE 7                   

                          
#define IOMDPA_MAPPED           1                       
#define IOMDPA_UNMAPPED         2                       
#define IOMDPA_SEGMENTS_PAGE    3                       
#define IOMDPA_SEGMENTS_LONG    4                       

                                 
                                                             

                                   
#define IOSERVICE_BUSY          1                       
#define IOSERVICE_NONBUSY       2                       
#define IOSERVICE_MODULESTALL   3                       
#define IOSERVICE_MODULEUNSTALL 4                       

#define IOSERVICE_TERMINATE_PHASE1              5                       
#define IOSERVICE_TERMINATE_REQUEST_OK          6                       
#define IOSERVICE_TERMINATE_REQUEST_FAIL        7                       
#define IOSERVICE_TERMINATE_SCHEDULE_STOP       8                       
#define IOSERVICE_TERMINATE_SCHEDULE_FINALIZE   9                       
#define IOSERVICE_TERMINATE_WILL                10                      
#define IOSERVICE_TERMINATE_DID                 11                      
#define IOSERVICE_TERMINATE_DID_DEFER           12                      
#define IOSERVICE_TERMINATE_FINALIZE            13                      
#define IOSERVICE_TERMINATE_STOP                14                      
#define IOSERVICE_TERMINATE_STOP_NOP            15                      
#define IOSERVICE_TERMINATE_STOP_DEFER          16                      
#define IOSERVICE_TERMINATE_DONE                17                      

#define IOSERVICE_KEXTD_ALIVE                   18                      
#define IOSERVICE_KEXTD_READY                   19                      
#define IOSERVICE_REGISTRY_QUIET                20                      

#define IOSERVICE_TERM_SET_INACTIVE             21                      
#define IOSERVICE_TERM_SCHED_PHASE2             22                      
#define IOSERVICE_TERM_START_PHASE2             23                      
#define IOSERVICE_TERM_TRY_PHASE2               24                      
#define IOSERVICE_TERM_UC_DEFER                 25                      
#define IOSERVICE_DETACH                        26                      

                                    
#define IOREGISTRYENTRY_NAME_STRING              1                      
#define IOREGISTRYENTRY_NAME                     2                      

#endif                            
