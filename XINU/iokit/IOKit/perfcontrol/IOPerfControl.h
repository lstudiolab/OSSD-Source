/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
 */

#pragma once

#ifdef KERNEL_PRIVATE
#ifdef __cplusplus

#include <IOKit/IOService.h>
#include <stdatomic.h>
#include <kern/bits.h>
#include <libkern/c++/OSPtr.h>

struct thread_group;

enum{
	kIOPerfControlClientWorkUntracked = 0,
};

   
                                               
                                                                                                               
                   
   
class IOPerfControlClient final : public OSObject
{
	OSDeclareDefaultStructors(IOPerfControlClient);

protected:
	virtual bool init(IOService *driver, uint64_t maxWorkCapacity);

public:
   
                       
                                                                                                    
                                           
                                                                     
                                                                                                     
                                               
   
	static IOPerfControlClient *copyClient(IOService *driver, uint64_t maxWorkCapacity);

   
                           
                                                                                      
                                                                                                        
                                                                                                        
                                                                                                         
                                                       
                                                                                                         
                                                      
   
	virtual IOReturn registerDevice(IOService *driver, IOService *device);

   
                             
                                                                                                   
                                                                                                        
                                                                                                       
                                                                      
                                                                                                         
   
	virtual void unregisterDevice(IOService *driver, IOService *device);

   
                         
                                                                                                               
                                                                                                            
                                            
   
	struct WorkSubmitArgs {
		uint32_t version;
		uint32_t size;
		uint64_t submit_time;
		uint64_t reserved[4];
		void *driver_data;
	};

   
                       
                                                                     
                                                                                             
                                  
                                                                                              
                                                                                                          
                                                                                                            
                                           
   
	virtual uint64_t workSubmit(IOService *device, WorkSubmitArgs *args = nullptr);

   
                        
                                                                                                          
                                                                                                           
                                            
   
	struct WorkBeginArgs {
		uint32_t version;
		uint32_t size;
		uint64_t begin_time;
		uint64_t reserved[4];
		void *driver_data;
	};

   
                               
                                                                                                     
                                                                                             
                                  
                                                                                                    
                                                                                              
                                                                                                          
                                                                                                            
                                           
   
	virtual uint64_t workSubmitAndBegin(IOService *device, WorkSubmitArgs *submitArgs = nullptr,
	    WorkBeginArgs *beginArgs = nullptr);

   
                      
                                                                                            
                                                                                             
                                  
                                                                                         
   
	virtual void workBegin(IOService *device, uint64_t token, WorkBeginArgs *args = nullptr);

   
                      
                                                                                                        
                                                                                                         
                                            
   
	struct WorkEndArgs {
		uint32_t version;
		uint32_t size;
		uint64_t end_time;
		uint64_t reserved[4];
		void *driver_data;
	};

   
                    
                                                                                             
                                                                                         
                                  
                                                                                       
                                                                                                        
                                                                                                                    
                                                                                                             
   
	virtual void workEnd(IOService *device, uint64_t token, WorkEndArgs *args = nullptr, bool done = true);

   
                            
                                                                                                             
                                                                                             
                                    
   
	OSPtr<OSObject> copyWorkContext();

   
                                          
                                                                                                    
                                                                                             
                                  
                                                                                                                      
                  
                                                                                                    
                                                                                              
                                                                         
                                                                                                                    
   
	bool workSubmitAndBeginWithContext(IOService *device, OSObject *context, WorkSubmitArgs *submitArgs = nullptr,
	    WorkBeginArgs *beginArgs = nullptr);

   
                                  
                                                                     
                                                                                             
                                  
                                                                                                                      
                  
                                                                                              
                                                                         
   
	bool workSubmitWithContext(IOService *device, OSObject *context, WorkSubmitArgs *args = nullptr);

   
                                 
                                                                                            
                                                                                             
                                  
                                                                                                                         
                                                                                         
                                                                                                                                                 
   
	void workBeginWithContext(IOService *device, OSObject *context, WorkBeginArgs *args = nullptr);

   
                               
                                                                                             
                                                                                         
                                  
                                                                                                                         
                                                                                       
                                                                                                        
                                                                                                   
                                                                                                                                           
                                                                                           
   
	void workEndWithContext(IOService *device, OSObject *context, WorkEndArgs *args = nullptr, bool done = true);

   
                                  
                                                                                                            
   
	struct PerfControllerInterface {
		struct WorkState {
			uint64_t thread_group_id;
			void *thread_group_data;
			void *work_data;
			uint32_t work_data_size;
			uint32_t started : 1;
			uint32_t reserved : 31;
		};

		using RegisterDeviceFunction = IOReturn (*)(IOService *);
		using WorkCanSubmitFunction = bool (*)(IOService *, WorkState *, WorkSubmitArgs *);
		using WorkSubmitFunction = void (*)(IOService *, uint64_t, WorkState *, WorkSubmitArgs *);
		using WorkBeginFunction = void (*)(IOService *, uint64_t, WorkState *, WorkBeginArgs *);
		using WorkEndFunction = void (*)(IOService *, uint64_t, WorkState *, WorkEndArgs *, bool);

		uint64_t version;
		RegisterDeviceFunction registerDevice;
		RegisterDeviceFunction unregisterDevice;
		WorkCanSubmitFunction workCanSubmit;
		WorkSubmitFunction workSubmit;
		WorkBeginFunction workBegin;
		WorkEndFunction workEnd;
	};

	struct IOPerfControlClientShared {
		atomic_uint_fast8_t maxDriverIndex;
		PerfControllerInterface interface;
		IOLock *interfaceLock;
		OSSet *deviceRegistrationList;
	};

   
                                          
                                                                                                
                                                                                                   
                                                                                       
   
	virtual IOReturn registerPerformanceController(PerfControllerInterface interface);

private:
	struct WorkTableEntry {
		struct thread_group *thread_group;
		bool started;
		uint8_t perfcontrol_data[32];
	};

	static constexpr size_t kMaxWorkTableNumEntries = 1024;
	static constexpr size_t kWorkTableIndexBits = 24;
	static constexpr size_t kWorkTableMaxSize = (1 << kWorkTableIndexBits) - 1;             
	                                                   
	static constexpr size_t kWorkTableIndexMask = (const size_t)mask(kWorkTableIndexBits);

	uint64_t allocateToken(thread_group *thread_group);
	void deallocateToken(uint64_t token);
	WorkTableEntry *getEntryForToken(uint64_t token);
	void markEntryStarted(uint64_t token, bool started);
	inline uint64_t tokenToGlobalUniqueToken(uint64_t token);

	uint8_t driverIndex;
	IOPerfControlClientShared *shared;
	WorkTableEntry *workTable;
	size_t workTableLength;
	size_t workTableNextIndex;
	IOSimpleLock *workTableLock;
};

#endif                  
#endif                     
