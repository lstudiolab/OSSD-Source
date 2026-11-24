/*
 * Copyright (c) 2004-2006 Apple Computer, Inc. All rights reserved.
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

#ifdef KERNEL_PRIVATE

#ifndef _KERN_PMS_H_
#define _KERN_PMS_H_

#define pmsMaxStates 64
#define HalfwayToForever 0x7FFFFFFFFFFFFFFFULL
#define century 790560000000000ULL

typedef void (*pmsSetFunc_t)(uint32_t, uint32_t, uint32_t);                                                    
typedef uint32_t (*pmsQueryFunc_t)(uint32_t, uint32_t);                                                  

typedef struct pmsStat {
	uint64_t        stTime[2];                                                                
	uint32_t        stCnt[2];                                                                  
} pmsStat;

typedef struct pmsDef {
	uint64_t        pmsLimit;                                                                   
	uint32_t        pmsStepID;                                                   
	uint32_t        pmsSetCmd;                                                         
#define pmsCngXClk  0x80000000                                     
#define pmsXUnk         0x7F                                                 
#define pmsXClk     0x7F000000                                        
#define pmsCngCPU   0x00800000                                     
#define pmsSync     0x00400000                                                                           
#define pmsMustCmp  0x00200000                                                      
#define pmsCPU      0x001F0000                             
#define pmsCPUUnk       0x1F                                               
#define pmsCngVolt  0x00008000                              
#define pmsVoltage  0x00007F00                       
#define pmsVoltUnk      0x7F                                         
#define pmsPowerID  0x000000FF                                          

                                       
#define pmsDelay    0xFFFFFFFD                                                                                                                
#define pmsParkIt       0xFFFFFFFF                                                                                                 
#define pmsCInit        ((pmsXUnk << 24) | (pmsCPUUnk << 16) | (pmsVoltUnk << 8))                                              
                                                                                                   
                                                                                              
                                                                                            
                                                                                            
   
	union sf {
		pmsSetFunc_t    pmsSetFunc;                                                    
		uint32_t        pmsSetFuncInd;                                           
	} sf;

	uint32_t        pmsDown;                                                       
	uint32_t        pmsNext;                                               
	uint32_t        pmsTDelay;                                                                           
} pmsDef;

typedef struct pmsCtl {
	pmsStat(*pmsStats)[pmsMaxStates];                                                                        
	pmsDef          *pmsDefs[pmsMaxStates];                                
} pmsCtl;

  
                                                                                
                                      
   

typedef struct pmsd {
	uint32_t        pmsState;                                                           
	uint32_t        pmsCSetCmd;                                                 
	uint64_t        pmsPop;                                                
	uint64_t        pmsStamp;                                                                
	uint64_t        pmsTime;                                                      
} pmsd;

  
                                          
   

enum {
	pmsIdle      = 0,                                                             
	pmsNorm      = 1,                                                                    
	pmsNormHigh  = 2,                                                                 
	pmsBoost     = 3,                                                         
	pmsLow       = 4,                                                                                
	pmsHigh      = 5,                                                                           
	pmsPrepCng   = 6,                                                                  
	pmsPrepSleep = 7,                                                      
	pmsOverTemp  = 8,                                                       
	pmsEnterNorm = 9,                                                                       
	pmsFree      = 10,                                                              
	pmsStartUp   = 0xFFFFFFFE,                                  
	pmsParked    = 0xFFFFFFFF                                                                     
};

  
                                            
   

enum {
	pmsCPark = 0,                                                          
	pmsCStart = 1,                                                               
	pmsCFLow = 2,                                                         
	pmsCFHigh = 3,                                                         
	pmsCCnfg = 4,                                                                  
	pmsCQuery = 5,                                                                    
	pmsCExperimental = 6,                                                
	pmsGCtls = 7,
	pmsGStats = 8,
	pmsCVID = 9,
	pmsCFree = 10                                                                            
};

  
                                                  
   
typedef struct {
	uint32_t        request;                                                     
	uint32_t        reqsize;                                          
	void            *reqaddr;                                                   
} pmsctl_t;

extern pmsCtl pmsCtls;                                                                
extern uint32_t pmsBroadcastWait;                                             
extern int pmsInstalled;
extern int pmsExperimental;

#define pmsSetFuncMax 32
extern pmsSetFunc_t pmsFuncTab[pmsSetFuncMax];
extern pmsQueryFunc_t pmsQueryFunc;
extern uint32_t pmsPlatformData;

extern kern_return_t pmsControl(uint32_t request, user_addr_t reqaddr, uint32_t reqsize);
extern void pmsInit(void);
extern void pmsStep(int timer);
extern void pmsDown(void);
extern void pmsSetStep(uint32_t nstep, int dir);
extern void pmsRunLocal(uint32_t nstep);
extern void pmsCPUSet(uint32_t sel);
extern uint32_t pmsCPUQuery(void);
extern uint32_t pmsCPUPackageQuery(void);
extern void pmsCPUConf(void);
extern void pmsCPUMachineInit(void);
extern void pmsCPUInit(void);
extern void pmsCPURun(uint32_t nstep);

extern void pmsCPUYellowFlag(void);
extern void pmsCPUGreenFlag(void);

#ifdef __cplusplus
extern "C" {
#endif

extern kern_return_t pmsBuild(pmsDef *pd, uint32_t pdsize, pmsSetFunc_t *functab, uint32_t platformData, pmsQueryFunc_t queryFunc);
extern void pmsRun(uint32_t nstep);
extern void pmsPark(void);
extern void pmsStart(void);
extern kern_return_t pmsCPULoadVIDTable(uint16_t *tablep, int nstates);                
extern kern_return_t pmsCPUSetPStateLimit(uint32_t limit);
#ifdef __cplusplus
}
#endif

#endif                   
#endif                     
