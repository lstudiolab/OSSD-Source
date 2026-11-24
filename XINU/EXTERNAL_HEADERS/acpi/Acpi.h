                                                                               
  
                                               
                         
  
                                                                               

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2006, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACTBL_H__
#define __ACTBL_H__

  
                                                                           
                                                                  
   
#define DSDT_SIG                "DSDT"                                                   
#define FADT_SIG                "FACP"                                        
#define FACS_SIG                "FACS"                                           
#define PSDT_SIG                "PSDT"                                               
#define RSDP_SIG                "RSD PTR "                                       
#define RSDT_SIG                "RSDT"                                         
#define XSDT_SIG                "XSDT"                                              
#define SSDT_SIG                "SSDT"                                              
#define RSDP_NAME               "RSDP"


  
                                                                  
                                                                  
   
#pragma pack(1)


  
                                                                         
  
                                                                            
                         
  
                                                                             
                                                                             
                                                    
   

                                                                                
  
                                                                         
                                                                        
                    
  
                                                                                

#define ACPI_TABLE_HEADER_DEF \
char                    Signature[4];                                       \
UINT32                  Length;                                                                       \
UINT8                   Revision;                                                        \
UINT8                   Checksum;                                                      \
char                    OemId[6];                                              \
char                    OemTableId[8];                                               \
UINT32                  OemRevision;                                      \
char                    AslCompilerId[4];                                          \
UINT32                  AslCompilerRevision;                              

typedef struct acpi_table_header {
	ACPI_TABLE_HEADER_DEF
} ACPI_TABLE_HEADER;


  
                                              
   
typedef struct acpi_generic_address {
	UINT8                   AddressSpaceId;                                                        
	UINT8                   RegisterBitWidth;                                       
	UINT8                   RegisterBitOffset;                                      
	UINT8                   AccessWidth;                                            
	UINT64                  Address;                                                      
} ACPI_GENERIC_ADDRESS;


                                                                                
  
                                                                   
  
                                                                                

typedef struct rsdp_descriptor {
	char                    Signature[8];                                                
	UINT8                   Checksum;                                  
	char                    OemId[6];                                   
	UINT8                   Revision;                                                              
	UINT32                  RsdtPhysicalAddress;                                         
	UINT32                  Length;                                                                      
	UINT64                  XsdtPhysicalAddress;                                                     
	UINT8                   ExtendedChecksum;                                             
	UINT8                   Reserved[3];                                    
} RSDP_DESCRIPTOR;

#define ACPI_RSDP_REV0_SIZE     20                                                      


                                                                                
  
                                             
  
                                                                                

typedef struct rsdt_descriptor {
	ACPI_TABLE_HEADER_DEF
	UINT32                  TableOffsetEntry[1];                                      
} RSDT_DESCRIPTOR;

typedef struct xsdt_descriptor {
	ACPI_TABLE_HEADER_DEF
	UINT64                  TableOffsetEntry[1];                                      
} XSDT_DESCRIPTOR;


                                                                                
  
                                                
  
                                                                                

typedef struct facs_descriptor {
	char                    Signature[4];                                  
	UINT32                  Length;                                                
	UINT32                  HardwareSignature;                                        
	UINT32                  FirmwareWakingVector;                                                           
	UINT32                  GlobalLock;                                                        

	                     

	UINT8                   S4Bios_f        : 1;                                      
	UINT8                                   : 7;                                   
	UINT8                   Reserved1[3];                                          

	UINT64                  XFirmwareWakingVector;                                                              
	UINT8                   Version;                                                   
	UINT8                   Reserved[31];                                   
} FACS_DESCRIPTOR;

#define ACPI_GLOCK_PENDING      0x01                                                       
#define ACPI_GLOCK_OWNED        0x02                                              


  
                                                                                        
   
typedef struct acpi_common_facs {
	UINT32                  *GlobalLock;
	UINT64                  *FirmwareWakingVector;
	UINT8                   VectorWidth;
} ACPI_COMMON_FACS;


                                                                                
  
                                                         
  
                                                                                

                                               

#define ACPI_FADT_COMMON \
ACPI_TABLE_HEADER_DEF \
UINT32                  V1_FirmwareCtrl;                                          \
UINT32                  V1_Dsdt;                                                  \
UINT8                   Reserved1;                                                             \
UINT8                   Prefer_PM_Profile;                                                            \
UINT16                  SciInt;                                                  \
UINT32                  SmiCmd;                                                    \
UINT8                   AcpiEnable;                                                        \
UINT8                   AcpiDisable;                                                        \
UINT8                   S4BiosReq;                                                                \
UINT8                   PstateCnt;                                                   \
UINT32                  V1_Pm1aEvtBlk;                                                       \
UINT32                  V1_Pm1bEvtBlk;                                                       \
UINT32                  V1_Pm1aCntBlk;                                                         \
UINT32                  V1_Pm1bCntBlk;                                                         \
UINT32                  V1_Pm2CntBlk;                                                         \
UINT32                  V1_PmTmrBlk;                                                           \
UINT32                  V1_Gpe0Blk;                                                                \
UINT32                  V1_Gpe1Blk;                                                                \
UINT8                   Pm1EvtLen;                                                     \
UINT8                   Pm1CntLen;                                                     \
UINT8                   Pm2CntLen;                                                    \
UINT8                   PmTmLen;                                                    \
UINT8                   Gpe0BlkLen;                                                \
UINT8                   Gpe1BlkLen;                                                \
UINT8                   Gpe1Base;                                                             \
UINT8                   CstCnt;                                                                                \
UINT16                  Plvl2Lat;                                                              \
UINT16                  Plvl3Lat;                                                              \
UINT16                  FlushSize;                                                              \
UINT16                  FlushStride;                                                           \
UINT8                   DutyOffset;                                                                    \
UINT8                   DutyWidth;                                                                         \
UINT8                   DayAlrm;                                                              \
UINT8                   MonAlrm;                                                               \
UINT8                   Century;                                                   \
UINT16                  IapcBootArch;                                                                          \
UINT8                   Reserved2;                                      


  
                 
   
typedef struct fadt_descriptor {
	ACPI_FADT_COMMON

	                     

	UINT8                   WbInvd      : 1;                                                  
	UINT8                   WbInvdFlush : 1;                                                       
	UINT8                   ProcC1      : 1;                                            
	UINT8                   Plvl2Up     : 1;                                        
	UINT8                   PwrButton   : 1;                                                         
	UINT8                   SleepButton : 1;                                                                         
	UINT8                   FixedRTC    : 1;                                                        
	UINT8                   Rtcs4       : 1;                                                 
	UINT8                   TmrValExt   : 1;                                         
	UINT8                   DockCap     : 1;                              
	UINT8                   ResetRegSup : 1;                                                          
	UINT8                   SealedCase  : 1;                                                                  
	UINT8                   Headless    : 1;                                                               
	UINT8                   CpuSwSleep  : 1;                                                                            

	UINT8                   PciExpWak                           : 1;                                                                 
	UINT8                   UsePlatformClock                    : 1;                                                               
	UINT8                   S4RtcStsValid                       : 1;                                                               
	UINT8                   RemotePowerOnCapable                : 1;                                                                 
	UINT8                   ForceApicClusterModel               : 1;                                                              
	UINT8                   ForceApicPhysicalDestinationMode    : 1;                                                                    
	UINT8                                                       : 4;                                   
	UINT8                   Reserved3;                                                              

	ACPI_GENERIC_ADDRESS    ResetRegister;                                            
	UINT8                   ResetValue;                                                                       
	UINT8                   Reserved4[3];                                       
	UINT64                  XFirmwareCtrl;                                       
	UINT64                  XDsdt;                                               
	ACPI_GENERIC_ADDRESS    XPm1aEvtBlk;                                                         
	ACPI_GENERIC_ADDRESS    XPm1bEvtBlk;                                                         
	ACPI_GENERIC_ADDRESS    XPm1aCntBlk;                                                       
	ACPI_GENERIC_ADDRESS    XPm1bCntBlk;                                                       
	ACPI_GENERIC_ADDRESS    XPm2CntBlk;                                                       
	ACPI_GENERIC_ADDRESS    XPmTmrBlk;                                                         
	ACPI_GENERIC_ADDRESS    XGpe0Blk;                                                                 
	ACPI_GENERIC_ADDRESS    XGpe1Blk;                                                                 
} FADT_DESCRIPTOR;


  
                                         
                                                                      
   
typedef struct fadt_descriptor_rev2_minus {
	ACPI_FADT_COMMON
	UINT32                  Flags;
	ACPI_GENERIC_ADDRESS    ResetRegister;                                            
	UINT8                   ResetValue;                                                                        
	UINT8                   Reserved7[3];                               
} FADT_DESCRIPTOR_REV2_MINUS;


  
                
                                                                      
   
typedef struct fadt_descriptor_rev1 {
	ACPI_FADT_COMMON
	UINT32                  Flags;
} FADT_DESCRIPTOR_REV1;


                                              

#define PM_UNSPECIFIED                  0
#define PM_DESKTOP                      1
#define PM_MOBILE                       2
#define PM_WORKSTATION                  3
#define PM_ENTERPRISE_SERVER            4
#define PM_SOHO_SERVER                  5
#define PM_APPLIANCE_PC                 6

                           

#define BAF_LEGACY_DEVICES              0x0001
#define BAF_8042_KEYBOARD_CONTROLLER    0x0002

#define FADT2_REVISION_ID               3
#define FADT2_MINUS_REVISION_ID         2


                              

#pragma pack()

  
                                                                    
                                             
   
#define ACPI_FLAG_OFFSET(d, f, o)         (UINT8) (ACPI_OFFSET (d,f) +\
	                                         sizeof(((d *)0)->f) + o)
  
                                
   
#include "Acpi_v1.h"

  
                                                              
                                                               
                                                                
   
#define ACPI_MEM_NOT_ALLOCATED  0
#define ACPI_MEM_ALLOCATED      1
#define ACPI_MEM_MAPPED         2

                                                                     

#define ACPI_TABLE_SINGLE       0x00
#define ACPI_TABLE_MULTIPLE     0x01
#define ACPI_TABLE_EXECUTABLE   0x02

#define ACPI_TABLE_ROOT         0x00
#define ACPI_TABLE_PRIMARY      0x10
#define ACPI_TABLE_SECONDARY    0x20
#define ACPI_TABLE_ALL          0x30
#define ACPI_TABLE_TYPE_MASK    0x30

                                      

typedef struct acpi_table_support {
	char                    *Name;
	char                    *Signature;
	void                    **GlobalPtr;
	UINT8                   SigLength;
	UINT8                   Flags;
} ACPI_TABLE_SUPPORT;


                                                              

#define ACPI_FACS_OFFSET(f)             (UINT8) ACPI_OFFSET (FACS_DESCRIPTOR,f)
#define ACPI_FADT_OFFSET(f)             (UINT8) ACPI_OFFSET (FADT_DESCRIPTOR, f)
#define ACPI_GAS_OFFSET(f)              (UINT8) ACPI_OFFSET (ACPI_GENERIC_ADDRESS,f)
#define ACPI_HDR_OFFSET(f)              (UINT8) ACPI_OFFSET (ACPI_TABLE_HEADER,f)
#define ACPI_RSDP_OFFSET(f)             (UINT8) ACPI_OFFSET (RSDP_DESCRIPTOR,f)

#define ACPI_FADT_FLAG_OFFSET(f, o)      ACPI_FLAG_OFFSET (FADT_DESCRIPTOR,f,o)
#define ACPI_FACS_FLAG_OFFSET(f, o)      ACPI_FLAG_OFFSET (FACS_DESCRIPTOR,f,o)

#endif                  
