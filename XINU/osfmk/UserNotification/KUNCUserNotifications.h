/*
 * Copyright (c) 2000-2004 Apple Inc. All rights reserved.
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

#ifndef __USERNOTIFICATION_KUNCUSERNOTIFICATIONS_H
#define __USERNOTIFICATION_KUNCUSERNOTIFICATIONS_H

#include <sys/cdefs.h>


#include <mach/message.h>
#include <mach/kern_return.h>
#include <UserNotification/UNDTypes.h>

__BEGIN_DECLS

  
                            
   
kern_return_t
KUNCUserNotificationDisplayNotice(
	int             noticeTimeout,
	unsigned        flags,
	char            *iconPath,
	char            *soundPath,
	char            *localizationPath,
	char            *alertHeader,
	char            *alertMessage,
	char            *defaultButtonTitle) __attribute__((deprecated));

  
                                                                   
                                                                                
   
kern_return_t
KUNCUserNotificationDisplayAlert(
	int             alertTimeout,
	unsigned        flags,
	char            *iconPath,
	char            *soundPath,
	char            *localizationPath,
	char            *alertHeader,
	char            *alertMessage,
	char            *defaultButtonTitle,
	char            *alternateButtonTitle,
	char            *otherButtonTitle,
	unsigned        *responseFlags) __attribute__((deprecated));


  
                                                                   
   

#define kOpenApplicationPath    0                                          
#define kOpenPreferencePanel    1                                                                                                                           
#define kOpenApplication        2                                                                             


#define kOpenAppAsRoot          0
#define kOpenAppAsConsoleUser   1

kern_return_t
KUNCExecute(
	char    *executionPath,
	int     openAsUser,
	int     pathExecutionType) __attribute__((deprecated));


                                  
  
                                                                 
                                           
  
                                                                        
                                                                                
                                         
  
              
                                               
                                                                                
  
                                                 
                                                                            
  
                                                           
                                                                             
  
                                                                   
                                                 
                                                                                    
  
                                                          
                                                  
                                                                                 
  
                                                                   
                                                                                       
  
                                                                                         
                                                                                         
  
                                                                  
                                                                                     
  
                                                            
                            
  
                                             
  
                                                                    
                      
  
                                                                   
                                                                                    
  
                                                                
                
  
                                                                     
                    
  
                                                                       
  
                                                                 
                                                                                   
  
                                                             
  
                                                              
                    
  
                                                                   
  
   

  
               
  
            
  
                   
                                                
                                                       
              
                                                                                   
           
                                                                
                
                                                              
             
                                                                 
                                         
              
                                                           
                                                             
                                                                    
                               
             
                                                                 
                                                                 
                                                            
   

typedef uintptr_t KUNCUserNotificationID;

  
                                           
  
                                                          
                                                                   
                                             
   

enum {
	kKUNCDefaultResponse        = 0,
	kKUNCAlternateResponse      = 1,
	kKUNCOtherResponse          = 2,
	kKUNCCancelResponse         = 3
};

#define KUNCCheckBoxChecked(i)  (1 << (8 + i))                                    
#define KUNCPopUpSelection(n)   (n << 24)

  
                                          
   
typedef void
(*KUNCUserNotificationCallBack)(
	int             contextKey,
	int             responseFlags,
	const void      *xmlData);

  
                        
   
KUNCUserNotificationID KUNCGetNotificationID(void) __attribute__((deprecated));

                                                                                                                                                                                       

kern_return_t
KUNCUserNotificationDisplayFromBundle(
	KUNCUserNotificationID          notificationID,
	char                            *bundleIdentifier,
	char                            *fileName,
	char                            *fileExtension,
	char                            *messageKey,
	char                            *tokenString,
	KUNCUserNotificationCallBack    callback,
	int                             contextKey) __attribute__((deprecated));


kern_return_t
KUNCUserNotificationCancel(
	KUNCUserNotificationID  notification) __attribute__((deprecated));


__END_DECLS

#endif                                                  
