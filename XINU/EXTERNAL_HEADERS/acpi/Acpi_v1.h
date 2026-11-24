                                                                               
  
                                                     
                         
  
                                                                               

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

#ifndef __ACTBL1_H__
#define __ACTBL1_H__


                                                                                
  
                         
  
                                                                          
                                                                    
  
                                                                                


  
                                                                           
                                                                  
   
#define ACPI_SIG_ASF            "ASF!"                                       
#define ACPI_SIG_BOOT           "BOOT"                                  
#define ACPI_SIG_CPEP           "CPEP"                                                  
#define ACPI_SIG_DBGP           "DBGP"                            
#define ACPI_SIG_ECDT           "ECDT"                                                    
#define ACPI_SIG_HPET           "HPET"                                            
#define ACPI_SIG_MADT           "APIC"                                           
#define ACPI_SIG_MCFG           "MCFG"                                                 
#define ACPI_SIG_SBST           "SBST"                                             
#define ACPI_SIG_SLIT           "SLIT"                                                      
#define ACPI_SIG_SPCR           "SPCR"                                                 
#define ACPI_SIG_SPMI           "SPMI"                                                      
#define ACPI_SIG_SRAT           "SRAT"                                          
#define ACPI_SIG_TCPA           "TCPA"                                                     
#define ACPI_SIG_WDRT           "WDRT"                                   

                  

#define APIC_SIG                "APIC"                                           
#define BOOT_SIG                "BOOT"                                  
#define SBST_SIG                "SBST"                                             


  
                                                                        
                                              
   
#pragma pack(1)

  
                                                                             
                                                                             
                                                    
   


                                                                                
  
                                                       
  
                                                                                

typedef struct acpi_table_asf {
	ACPI_TABLE_HEADER_DEF
} ACPI_TABLE_ASF;

#define ACPI_ASF_HEADER_DEF \
UINT8                   Type; \
UINT8                   Reserved; \
UINT16                  Length;

typedef struct acpi_asf_header {
	ACPI_ASF_HEADER_DEF
} ACPI_ASF_HEADER;


                           

#define ASF_INFO                0
#define ASF_ALERT               1
#define ASF_CONTROL             2
#define ASF_BOOT                3
#define ASF_ADDRESS             4
#define ASF_RESERVED            5

  
                
   

                        

typedef struct acpi_asf_info {
	ACPI_ASF_HEADER_DEF
	UINT8                   MinResetValue;
	UINT8                   MinPollInterval;
	UINT16                  SystemId;
	UINT32                  MfgId;
	UINT8                   Flags;
	UINT8                   Reserved2[3];
} ACPI_ASF_INFO;

                   

typedef struct acpi_asf_alert {
	ACPI_ASF_HEADER_DEF
	UINT8                   AssertMask;
	UINT8                   DeassertMask;
	UINT8                   Alerts;
	UINT8                   DataLength;
	UINT8                   Array[1];
} ACPI_ASF_ALERT;

                           

typedef struct acpi_asf_remote {
	ACPI_ASF_HEADER_DEF
	UINT8                   Controls;
	UINT8                   DataLength;
	UINT16                  Reserved2;
	UINT8                   Array[1];
} ACPI_ASF_REMOTE;

                              

typedef struct acpi_asf_rmcp {
	ACPI_ASF_HEADER_DEF
	UINT8                   Capabilities[7];
	UINT8                   CompletionCode;
	UINT32                  EnterpriseId;
	UINT8                   Command;
	UINT16                  Parameter;
	UINT16                  BootOptions;
	UINT16                  OemParameters;
} ACPI_ASF_RMCP;

                    

typedef struct acpi_asf_address {
	ACPI_ASF_HEADER_DEF
	UINT8                   EpromAddress;
	UINT8                   Devices;
	UINT8                   SmbusAddresses[1];
} ACPI_ASF_ADDRESS;


                                                                                
  
                                
  
                                                                                

typedef struct acpi_table_boot {
	ACPI_TABLE_HEADER_DEF
	UINT8                   CmosIndex;                                                   
	UINT8                   Reserved[3];
} ACPI_TABLE_BOOT;


                                                                                
  
                                                
  
                                                                                

typedef struct acpi_table_cpep {
	ACPI_TABLE_HEADER_DEF
	UINT64                  Reserved;
} ACPI_TABLE_CPEP;

              

typedef struct acpi_cpep_polling {
	UINT8                   Type;
	UINT8                   Length;
	UINT8                   ProcessorId;                      
	UINT8                   ProcessorEid;                      
	UINT32                  PollingInterval;                             
} ACPI_CPEP_POLLING;


                                                                                
  
                          
  
                                                                                

typedef struct acpi_table_dbgp {
	ACPI_TABLE_HEADER_DEF
	UINT8                   InterfaceType;                                       
	UINT8                   Reserved[3];
	ACPI_GENERIC_ADDRESS    DebugPort;
} ACPI_TABLE_DBGP;


                                                                                
  
                                                  
  
                                                                                

typedef struct ec_boot_resources {
	ACPI_TABLE_HEADER_DEF
	ACPI_GENERIC_ADDRESS    EcControl;                                                 
	ACPI_GENERIC_ADDRESS    EcData;                                          
	UINT32                  Uid;                                                                
	UINT8                   GpeBit;                                 
	UINT8                   EcId[1];                                                           
} EC_BOOT_RESOURCES;


                                                                                
  
                                          
  
                                                                                

typedef struct acpi_hpet_table {
	ACPI_TABLE_HEADER_DEF
	UINT32                  HardwareId;                                           
	ACPI_GENERIC_ADDRESS    BaseAddress;                                      
	UINT8                   HpetNumber;                               
	UINT16                  ClockTick;                                                
	UINT8                   Attributes;
} HPET_TABLE;

#if 0                                           
struct                     
{
	UINT8                   PageProtect     :1;                               
	UINT8                   PageProtect4    :1;                               
	UINT8                   PageProtect64   :1;                                
	UINT8                                   :5;                                   
} Flags;
#endif


                                                                                
  
                                         
  
                                                                                

typedef struct multiple_apic_table {
	ACPI_TABLE_HEADER_DEF
	UINT32                  LocalApicAddress;                                    

	                     

	UINT8                   PCATCompat      : 1;                                       
	UINT8                                   : 7;                                   
	UINT8                   Reserved1[3];                                          
} MULTIPLE_APIC_TABLE;

                                

#define DUAL_PIC                0
#define MULTIPLE_APIC           1


                                  

#define APIC_HEADER_DEF \
UINT8                   Type; \
UINT8                   Length;

typedef struct apic_header {
	APIC_HEADER_DEF
} APIC_HEADER;

                                    

#define APIC_PROCESSOR          0
#define APIC_IO                 1
#define APIC_XRUPT_OVERRIDE     2
#define APIC_NMI                3
#define APIC_LOCAL_NMI          4
#define APIC_ADDRESS_OVERRIDE   5
#define APIC_IO_SAPIC           6
#define APIC_LOCAL_SAPIC        7
#define APIC_XRUPT_SOURCE       8
#define APIC_RESERVED           9                                           


                                          

#define ACPI_MADT_IFLAGS                            \
UINT8                   Polarity        : 2;                                                    \
UINT8                   TriggerMode     : 2;                                                    \
UINT8                                   : 4;                                        \
UINT8                   Reserved1;                                                 

#define ACPI_MADT_LFLAGS                                   \
UINT8                   ProcessorEnabled: 1;                                            \
UINT8                                   : 7;                                        \
UINT8                   Reserved2[3];                                              


                               

#define POLARITY_CONFORMS       0
#define POLARITY_ACTIVE_HIGH    1
#define POLARITY_RESERVED       2
#define POLARITY_ACTIVE_LOW     3

#define TRIGGER_CONFORMS        0
#define TRIGGER_EDGE            1
#define TRIGGER_RESERVED        2
#define TRIGGER_LEVEL           3


  
                                                     
   

                       

typedef struct madt_processor_apic {
	APIC_HEADER_DEF
	UINT8                   ProcessorId;                           
	UINT8                   LocalApicId;                                   
	ACPI_MADT_LFLAGS
} MADT_PROCESSOR_APIC;

                

typedef struct madt_io_apic {
	APIC_HEADER_DEF
	UINT8                   IoApicId;                        
	UINT8                   Reserved;                                    
	UINT32                  Address;                                   
	UINT32                  Interrupt;                                                          
} MADT_IO_APIC;

                           

typedef struct madt_interrupt_override {
	APIC_HEADER_DEF
	UINT8                   Bus;                         
	UINT8                   Source;                                     
	UINT32                  Interrupt;                                   
	ACPI_MADT_IFLAGS
} MADT_INTERRUPT_OVERRIDE;

                    

typedef struct madt_nmi_source {
	APIC_HEADER_DEF
	ACPI_MADT_IFLAGS
	UINT32                  Interrupt;                                   
} MADT_NMI_SOURCE;

                       

typedef struct madt_local_apic_nmi {
	APIC_HEADER_DEF
	UINT8                   ProcessorId;                           
	ACPI_MADT_IFLAGS
	UINT8                   Lint;                                                    
} MADT_LOCAL_APIC_NMI;

                         

typedef struct madt_address_override {
	APIC_HEADER_DEF
	UINT16                  Reserved;                                   
	UINT64                  Address;                                   
} MADT_ADDRESS_OVERRIDE;

                  

typedef struct madt_io_sapic {
	APIC_HEADER_DEF
	UINT8                   IoSapicId;                        
	UINT8                   Reserved;                                   
	UINT32                  InterruptBase;                                        
	UINT64                  Address;                                    
} MADT_IO_SAPIC;

                    

typedef struct madt_local_sapic {
	APIC_HEADER_DEF
	UINT8                   ProcessorId;                           
	UINT8                   LocalSapicId;                 
	UINT8                   LocalSapicEid;                 
	UINT8                   Reserved[3];                                
	ACPI_MADT_LFLAGS
	UINT32                  ProcessorUID;                                       
	char                    ProcessorUIDString[1];                            
} MADT_LOCAL_SAPIC;

                                  

typedef struct madt_interrupt_source {
	APIC_HEADER_DEF
	ACPI_MADT_IFLAGS
	UINT8                   InterruptType;                                  
	UINT8                   ProcessorId;                      
	UINT8                   ProcessorEid;                      
	UINT8                   IoSapicVector;                                       
	UINT32                  Interrupt;                                   
	UINT32                  Flags;                                      
} MADT_INTERRUPT_SOURCE;


                                                                                
  
                                                             
  
                                                                                

typedef struct acpi_table_mcfg {
	ACPI_TABLE_HEADER_DEF
	UINT8                   Reserved[8];
} ACPI_TABLE_MCFG;

typedef struct acpi_mcfg_allocation {
	UINT64                  BaseAddress;                                          
	UINT16                  PciSegment;                                   
	UINT8                   StartBusNumber;                              
	UINT8                   EndBusNumber;                             
	UINT32                  Reserved;
} ACPI_MCFG_ALLOCATION;


                                                                                
  
                                           
  
                                                                                

typedef struct smart_battery_table {
	ACPI_TABLE_HEADER_DEF
	UINT32                  WarningLevel;
	UINT32                  LowLevel;
	UINT32                  CriticalLevel;
} SMART_BATTERY_TABLE;


                                                                                
  
                                                    
  
                                                                                

typedef struct system_locality_info {
	ACPI_TABLE_HEADER_DEF
	UINT64                  LocalityCount;
	UINT8                   Entry[1][1];
} SYSTEM_LOCALITY_INFO;


                                                                                
  
                                               
  
                                                                                

typedef struct acpi_table_spcr {
	ACPI_TABLE_HEADER_DEF
	UINT8                   InterfaceType;                                       
	UINT8                   Reserved[3];
	ACPI_GENERIC_ADDRESS    SerialPort;
	UINT8                   InterruptType;
	UINT8                   PcInterrupt;
	UINT32                  Interrupt;
	UINT8                   BaudRate;
	UINT8                   Parity;
	UINT8                   StopBits;
	UINT8                   FlowControl;
	UINT8                   TerminalType;
	UINT8                   Reserved2;
	UINT16                  PciDeviceId;
	UINT16                  PciVendorId;
	UINT8                   PciBus;
	UINT8                   PciDevice;
	UINT8                   PciFunction;
	UINT32                  PciFlags;
	UINT8                   PciSegment;
	UINT32                  Reserved3;
} ACPI_TABLE_SPCR;


                                                                                
  
                                                    
  
                                                                                

typedef struct acpi_table_spmi {
	ACPI_TABLE_HEADER_DEF
	UINT8                   Reserved;
	UINT8                   InterfaceType;
	UINT16                  SpecRevision;                        
	UINT8                   InterruptType;
	UINT8                   GpeNumber;                        
	UINT8                   Reserved2;
	UINT8                   PciDeviceFlag;
	UINT32                  Interrupt;
	ACPI_GENERIC_ADDRESS    IpmiRegister;
	UINT8                   PciSegment;
	UINT8                   PciBus;
	UINT8                   PciDevice;
	UINT8                   PciFunction;
} ACPI_TABLE_SPMI;


                                                                                
  
                                        
  
                                                                                

typedef struct system_resource_affinity {
	ACPI_TABLE_HEADER_DEF
	UINT32                  Reserved1;                             
	UINT64                  Reserved2;                                  
} SYSTEM_RESOURCE_AFFINITY;


                                  

#define SRAT_SUBTABLE_HEADER \
UINT8                   Type; \
UINT8                   Length;

                           

#define SRAT_CPU_AFFINITY       0
#define SRAT_MEMORY_AFFINITY    1
#define SRAT_RESERVED           2


                     

typedef struct static_resource_alloc {
	SRAT_SUBTABLE_HEADER
	UINT8                   ProximityDomainLo;
	UINT8                   ApicId;

	                     

	UINT8                   Enabled         :1;                                   
	UINT8                                   :7;                                   
	UINT8                   Reserved3[3];                                      

	UINT8                   LocalSapicEid;
	UINT8                   ProximityDomainHi[3];
	UINT32                  Reserved4;                                  
} STATIC_RESOURCE_ALLOC;

typedef struct memory_affinity {
	SRAT_SUBTABLE_HEADER
	UINT32                  ProximityDomain;
	UINT16                  Reserved3;
	UINT64                  BaseAddress;
	UINT64                  AddressLength;
	UINT32                  Reserved4;

	                     

	UINT8                   Enabled         :1;                                   
	UINT8                   HotPluggable    :1;                                           
	UINT8                   NonVolatile     :1;                                   
	UINT8                                   :5;                                   
	UINT8                   Reserved5[3];                                      

	UINT64                  Reserved6;                                  
} MEMORY_AFFINITY;


                                                                                
  
                                                   
  
                                                                                

typedef struct acpi_table_tcpa {
	ACPI_TABLE_HEADER_DEF
	UINT16                  Reserved;
	UINT32                  MaxLogLength;                                              
	UINT64                  LogAddress;                                        
} ACPI_TABLE_TCPA;


                                                                                
  
                                 
  
                                                                                

typedef struct acpi_table_wdrt {
	ACPI_TABLE_HEADER_DEF
	UINT32                  HeaderLength;                               
	UINT8                   PciSegment;                             
	UINT8                   PciBus;                             
	UINT8                   PciDevice;                             
	UINT8                   PciFunction;                             
	UINT32                  TimerPeriod;                                          
	UINT32                  MaxCount;                                            
	UINT32                  MinCount;                                  
	UINT8                   Flags;
	UINT8                   Reserved[3];
	UINT32                  Entries;                                                    
} ACPI_TABLE_WDRT;

#if 0                                         
UINT8                   Enabled         :1;                           
UINT8                                   :6;                      
UINT8                   SleepStop       :1;                                          
#endif


                                                              

#define ACPI_ASF0_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_ASF_INFO,f)
#define ACPI_ASF1_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_ASF_ALERT,f)
#define ACPI_ASF2_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_ASF_REMOTE,f)
#define ACPI_ASF3_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_ASF_RMCP,f)
#define ACPI_ASF4_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_ASF_ADDRESS,f)
#define ACPI_BOOT_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_BOOT,f)
#define ACPI_CPEP_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_CPEP,f)
#define ACPI_CPEP0_OFFSET(f)            (UINT8) ACPI_OFFSET (ACPI_CPEP_POLLING,f)
#define ACPI_DBGP_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_DBGP,f)
#define ACPI_ECDT_OFFSET(f)             (UINT8) ACPI_OFFSET (EC_BOOT_RESOURCES,f)
#define ACPI_HPET_OFFSET(f)             (UINT8) ACPI_OFFSET (HPET_TABLE,f)
#define ACPI_MADT_OFFSET(f)             (UINT8) ACPI_OFFSET (MULTIPLE_APIC_TABLE,f)
#define ACPI_MADT0_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_PROCESSOR_APIC,f)
#define ACPI_MADT1_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_IO_APIC,f)
#define ACPI_MADT2_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_INTERRUPT_OVERRIDE,f)
#define ACPI_MADT3_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_NMI_SOURCE,f)
#define ACPI_MADT4_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_LOCAL_APIC_NMI,f)
#define ACPI_MADT5_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_ADDRESS_OVERRIDE,f)
#define ACPI_MADT6_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_IO_SAPIC,f)
#define ACPI_MADT7_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_LOCAL_SAPIC,f)
#define ACPI_MADT8_OFFSET(f)            (UINT8) ACPI_OFFSET (MADT_INTERRUPT_SOURCE,f)
#define ACPI_MADTH_OFFSET(f)            (UINT8) ACPI_OFFSET (APIC_HEADER,f)
#define ACPI_MCFG_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_MCFG,f)
#define ACPI_MCFG0_OFFSET(f)            (UINT8) ACPI_OFFSET (ACPI_MCFG_ALLOCATION,f)
#define ACPI_SBST_OFFSET(f)             (UINT8) ACPI_OFFSET (SMART_BATTERY_TABLE,f)
#define ACPI_SLIT_OFFSET(f)             (UINT8) ACPI_OFFSET (SYSTEM_LOCALITY_INFO,f)
#define ACPI_SPCR_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_SPCR,f)
#define ACPI_SPMI_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_SPMI,f)
#define ACPI_SRAT_OFFSET(f)             (UINT8) ACPI_OFFSET (SYSTEM_RESOURCE_AFFINITY,f)
#define ACPI_SRAT0_OFFSET(f)            (UINT8) ACPI_OFFSET (STATIC_RESOURCE_ALLOC,f)
#define ACPI_SRAT1_OFFSET(f)            (UINT8) ACPI_OFFSET (MEMORY_AFFINITY,f)
#define ACPI_TCPA_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_TCPA,f)
#define ACPI_WDRT_OFFSET(f)             (UINT8) ACPI_OFFSET (ACPI_TABLE_WDRT,f)


#define ACPI_HPET_FLAG_OFFSET(f, o)      ACPI_FLAG_OFFSET (HPET_TABLE,f,o)
#define ACPI_SRAT0_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (STATIC_RESOURCE_ALLOC,f,o)
#define ACPI_SRAT1_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MEMORY_AFFINITY,f,o)
#define ACPI_MADT_FLAG_OFFSET(f, o)      ACPI_FLAG_OFFSET (MULTIPLE_APIC_TABLE,f,o)
#define ACPI_MADT0_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_PROCESSOR_APIC,f,o)
#define ACPI_MADT2_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_INTERRUPT_OVERRIDE,f,o)
#define ACPI_MADT3_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_NMI_SOURCE,f,o)
#define ACPI_MADT4_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_LOCAL_APIC_NMI,f,o)
#define ACPI_MADT7_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_LOCAL_SAPIC,f,o)
#define ACPI_MADT8_FLAG_OFFSET(f, o)     ACPI_FLAG_OFFSET (MADT_INTERRUPT_SOURCE,f,o)


                              

#pragma pack()

#endif                   
