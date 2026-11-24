/*
 * Copyright (c) 2002-2008 by Apple Inc.. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


  
                           
 
                                                                                 
                                                                         
                                                      
 
                                                                        
                                    
 
                                                       
                                                                           
                                                                            
                                                                        
                                                                         
 
                                                           
                       

                                          
  
#ifndef __ASSERTMACROS__
#define __ASSERTMACROS__

  
                   
    
                        
                                                     
                                                                                       
    
                         
                                                                     
                                                                                       
    
                                          
                                                                                                    
                                                                                                      
                                                       
    
                                                         
   
                                                                                                           
                                                                       
                                                           
                                                                                   
    
                                                                                        
                                                                                  
                            
  
                  
  
                                                                               
                                                                                       
                                                                            
  
                                                                                 
                            
  
                                                                                      
                                                                                   
  
                                                                                   
                                                                                      
                              
  
                                                                                    
                                   
  
                                                                                     
                                                                                       
                                                                                          
                                                                                     
                                                                                      
                                                                                        
                                                                                       
                                                                                           
                                                                                         
                                                                                      
                                                                                    
                                                                              
                                                                                    
                 
   


  
                                                                             
                                                                                
                                                                           
  
                                                                        
                                                                              
                         
   
#ifndef DEBUG_ASSERT_COMPONENT_NAME_STRING
    #define DEBUG_ASSERT_COMPONENT_NAME_STRING ""
#endif


  
                                                                                     
                                                                            
  
                                                                                       
                                                          
   
#ifndef DEBUG_ASSERT_PRODUCTION_CODE
   #define DEBUG_ASSERT_PRODUCTION_CODE !DEBUG
#endif


  
                                                                                   
  
            
                                                                                   
                                                                                      
                                                                   
  
               
  
                          
                                                                 
                                                                      
                                                                    
                                                    
  
                      
                                                                
                                                                    
                                                                 
                         
      
                           
                                                                     
      
                  
                                                                           
                                                                 
                          
      
               
                                                               
                                                   
      
                 
                                                                  
                                          
      
                
                                                    
  
                                                                                   
                        
  
                                                                  
  
                                                                                                      
                                                                                                          
                                                                                                      
                                                                                                        
  
                   
                                                                                           
                                                                                        
                                                                                  
         
                                                                          
                                                                                                    
                
                                                                            
                                               
                                                                  
                                      
                                                         
                                   
                                                            
                                  
                                                               
                                 
                                                              
         
  
                                                                                      
   
#ifndef DEBUG_ASSERT_MESSAGE
   #ifdef KERNEL
      #include <libkern/libkern.h>
      #define DEBUG_ASSERT_MESSAGE(name, assertion, label, message, file, line, value) \
                                  printf( "AssertMacros: %s, %s file: %s, line: %d\n", assertion, (message!=0) ? message : "", file, line);
   #else
      #include <stdio.h>
      #define DEBUG_ASSERT_MESSAGE(name, assertion, label, message, file, line, value) \
                                  fprintf(stderr, "AssertMacros: %s, %s file: %s, line: %d\n", assertion, (message!=0) ? message : "", file, line);
   #endif
#endif





  
                           
  
            
                                                           
  
                                                       
  
               
  
              
                                
  
   
#ifndef __Debug_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Debug_String(message)
	#else
	   #define __Debug_String(message)                                             \
		  do                                                                      \
		  {                                                                       \
			  DEBUG_ASSERT_MESSAGE(                                               \
				  DEBUG_ASSERT_COMPONENT_NAME_STRING,                             \
				  "",                                                             \
				  0,                                                              \
				  message,                                                        \
				  __FILE__,                                                       \
				  __LINE__,                                                       \
				  0);                                                             \
		  } while ( 0 )
	#endif
#endif

  
                      
  
            
                                                           
  
                                                                            
                                
  
               
  
                
                                 
   
#ifndef __Check
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Check(assertion)
	#else
	   #define __Check(assertion)                                                 \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, 0, 0, __FILE__, __LINE__, 0 );                  \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nCheck
	#define __nCheck(assertion)  __Check(!(assertion))
#endif

  
                                      
  
            
                                                           
  
                                                                            
                                
  
               
  
                
                                 
  
              
                                
   
#ifndef __Check_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Check_String(assertion, message)
	#else
	   #define __Check_String(assertion, message)                                 \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, 0, message, __FILE__, __LINE__, 0 );            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nCheck_String
	#define __nCheck_String(assertion, message)  __Check_String(!(assertion), message)
#endif

  
                            
  
            
                                                           
  
                                                                                  
                                
  
               
  
                
                                                   
   
#ifndef __Check_noErr
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Check_noErr(errorCode)
	#else
	   #define __Check_noErr(errorCode)                                           \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", 0, 0, __FILE__, __LINE__, evalOnceErrorCode ); \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                            
  
            
                                                                       
              
  
                                                                                  
                                
  
               
  
                
                                                 
  
              
                                
   
#ifndef __Check_noErr_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Check_noErr_String(errorCode, message)
	#else
	   #define __Check_noErr_String(errorCode, message)                           \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", 0, message, __FILE__, __LINE__, evalOnceErrorCode ); \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                       
  
            
                                                                      
                 
  
                                                                            
                                
  
               
  
                
                                 
   
#ifndef __Verify
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify(assertion)                                                \
		  do                                                                      \
		  {                                                                       \
			  if ( !(assertion) )                                                 \
			  {                                                                   \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Verify(assertion)                                                \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, 0, 0, __FILE__, __LINE__, 0 );                  \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nVerify
	#define __nVerify(assertion)	__Verify(!(assertion))
#endif

  
                                       
  
            
                                                                      
                 
  
                                                                            
                                
  
               
  
                
                                 
  
              
                                
   
#ifndef __Verify_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify_String(assertion, message)                                \
		  do                                                                      \
		  {                                                                       \
			  if ( !(assertion) )                                                 \
			  {                                                                   \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Verify_String(assertion, message)                                \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, 0, message, __FILE__, __LINE__, 0 );            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nVerify_String
	#define __nVerify_String(assertion, message)  __Verify_String(!(assertion), message)
#endif

  
                             
  
            
                                                                      
                 
  
                                                                                  
                                
  
               
  
                
                                       
   
#ifndef __Verify_noErr
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify_noErr(errorCode)                                          \
		  do                                                                      \
		  {                                                                       \
			  if ( 0 != (errorCode) )                                             \
			  {                                                                   \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Verify_noErr(errorCode)                                          \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", 0, 0, __FILE__, __LINE__, evalOnceErrorCode ); \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                             
  
            
                                                                      
                 
  
                                                                                  
                                
  
               
  
                
                                       
  
              
                                
   
#ifndef __Verify_noErr_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify_noErr_String(errorCode, message)                          \
		  do                                                                      \
		  {                                                                       \
			  if ( 0 != (errorCode) )                                             \
			  {                                                                   \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Verify_noErr_String(errorCode, message)                          \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", 0, message, __FILE__, __LINE__, evalOnceErrorCode ); \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                            
  
            
                                                                              
                                                                 
  
                                                                                  
                                                                                 
                        
  
               
  
                
                                       
  
             
                                                    
   
#ifndef __Verify_noErr_Action
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify_noErr_Action(errorCode, action)                          \
		  if ( 0 != (errorCode) ) {                                              \
			  action;                                                            \
		  }                                                                      \
		  else do {} while (0)
	#else
	   #define __Verify_noErr_Action(errorCode, action)                          \
               do {                                                                   \
		  long evalOnceErrorCode = (errorCode);                                  \
		  if ( __builtin_expect(0 != evalOnceErrorCode, 0) ) {                   \
			  DEBUG_ASSERT_MESSAGE(                                              \
				  DEBUG_ASSERT_COMPONENT_NAME_STRING,                            \
				  #errorCode " == 0 ", 0, 0, __FILE__, __LINE__, evalOnceErrorCode );            \
			  action;                                                            \
		  }                                                                      \
	       } while (0)
	#endif
#endif

  
                                      
  
            
                                                                        
                                                                      
  
                                                                            
                                                                                 
                        
  
               
  
                
                                 
  
             
                                                    
   
#ifndef __Verify_Action
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Verify_Action(assertion, action)                                \
		  if ( __builtin_expect(!(assertion), 0) ) {                             \
			action;                                                              \
		  }                                                                      \
		  else do {} while (0)
	#else
	   #define __Verify_Action(assertion, action)                                \
		  if ( __builtin_expect(!(assertion), 0) ) {                             \
			  DEBUG_ASSERT_MESSAGE(                                              \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                        \
					  #assertion, 0, 0, __FILE__, __LINE__, 0 );                 \
			  action;                                                            \
		  }                                                                      \
		  else do {} while (0)
	#endif
#endif

  
                                        
  
            
                                                                        
                          
  
                                                                            
                                                             
  
               
  
                
                                 
  
                     
                  
   
#ifndef __Require
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require(assertion, exceptionLabel)                               \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require(assertion, exceptionLabel)                               \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) ) {                          \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, #exceptionLabel, 0, __FILE__, __LINE__,  0);    \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nRequire
	#define __nRequire(assertion, exceptionLabel)  __Require(!(assertion), exceptionLabel)
#endif

  
                                                       
  
            
                                                                        
                                                                         
                          
  
                                                                            
                                                                         
                                                      
  
               
  
                
                                 
  
                     
                  
  
             
                                                    
   
#ifndef __Require_Action
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_Action(assertion, exceptionLabel, action)                \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_Action(assertion, exceptionLabel, action)                \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, #exceptionLabel, 0,   __FILE__, __LINE__, 0);   \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nRequire_Action
	#define __nRequire_Action(assertion, exceptionLabel, action)                  \
	__Require_Action(!(assertion), exceptionLabel, action)
#endif

  
                                              
  
            
                                                                          
  
               
  
                
                                 
  
                     
                  
   
#ifndef __Require_Quiet
	#define __Require_Quiet(assertion, exceptionLabel)                            \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(!(assertion), 0) )                                \
		  {                                                                       \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif

#ifndef __nRequire_Quiet
	#define __nRequire_Quiet(assertion, exceptionLabel)  __Require_Quiet(!(assertion), exceptionLabel)
#endif

  
                                                             
  
            
                                                                        
                                                                       
  
               
  
                
                                 
  
                     
                  
  
             
                                                    
   
#ifndef __Require_Action_Quiet
	#define __Require_Action_Quiet(assertion, exceptionLabel, action)             \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(!(assertion), 0) )                                \
		  {                                                                       \
			  {                                                                   \
				  action;                                                         \
			  }                                                                   \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif

#ifndef __nRequire_Action_Quiet
	#define __nRequire_Action_Quiet(assertion, exceptionLabel, action)              \
		__Require_Action_Quiet(!(assertion), exceptionLabel, action)
#endif

  
                                                        
  
            
                                                                        
                          
  
                                                                            
                                                              
  
               
  
                
                                 
  
                     
                  
  
              
                                
   
#ifndef __Require_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_String(assertion, exceptionLabel, message)               \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_String(assertion, exceptionLabel, message)               \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, #exceptionLabel,  message,  __FILE__, __LINE__, 0); \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nRequire_String
	#define __nRequire_String(assertion, exceptionLabel, string)                  \
		__Require_String(!(assertion), exceptionLabel, string)
#endif

  
                                                                       
  
            
                                                                        
                                                                          
                          
  
                                                                            
                                                                         
                                                      
  
               
  
                
                                 
  
                     
                  
  
             
                                                    
  
              
                                
   
#ifndef __Require_Action_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_Action_String(assertion, exceptionLabel, action, message)  \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_Action_String(assertion, exceptionLabel, action, message)  \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(!(assertion), 0) )                            \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #assertion, #exceptionLabel,  message,  __FILE__,  __LINE__, 0); \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

#ifndef __nRequire_Action_String
	#define __nRequire_Action_String(assertion, exceptionLabel, action, message)    \
		__Require_Action_String(!(assertion), exceptionLabel, action, message)
#endif

  
                                              
  
            
                                                                              
                          
  
                                                                                  
                                                             
  
               
  
                
                                       
  
                     
                  
   
#ifndef __Require_noErr
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_noErr(errorCode, exceptionLabel)                         \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(0 != (errorCode), 0) )                        \
			  {                                                                   \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_noErr(errorCode, exceptionLabel)                         \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ",  #exceptionLabel,  0,  __FILE__, __LINE__, evalOnceErrorCode); \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                                             
  
            
                                                                              
                                                                    
                          
  
                                                                                  
                                                                
                                                               
  
               
  
                
                                       
  
                     
                  
  
             
                                                    
   
#ifndef __Require_noErr_Action
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_noErr_Action(errorCode, exceptionLabel, action)          \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(0 != (errorCode), 0) )                        \
			  {                                                                   \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_noErr_Action(errorCode, exceptionLabel, action)          \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", #exceptionLabel,  0,  __FILE__, __LINE__,  evalOnceErrorCode); \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                                    
  
            
                                                           
                          
  
               
  
                
                                       
  
                     
                  
   
#ifndef __Require_noErr_Quiet
	#define __Require_noErr_Quiet(errorCode, exceptionLabel)                      \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(0 != (errorCode), 0) )                            \
		  {                                                                       \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif

  
                                                                   
  
            
                                                           
                                                                    
                          
  
               
  
                
                                       
  
                     
                  
  
             
                                                    
   
#ifndef __Require_noErr_Action_Quiet
	#define __Require_noErr_Action_Quiet(errorCode, exceptionLabel, action)       \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(0 != (errorCode), 0) )                            \
		  {                                                                       \
			  {                                                                   \
				  action;                                                         \
			  }                                                                   \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif

  
                                                              
  
            
                                                                              
                          
  
                                                                                  
                                                              
  
               
  
                
                                       
  
                     
                  
  
              
                                
   
#ifndef __Require_noErr_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_noErr_String(errorCode, exceptionLabel, message)         \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(0 != (errorCode), 0) )                        \
			  {                                                                   \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_noErr_String(errorCode, exceptionLabel, message)         \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ",  #exceptionLabel, message, __FILE__,  __LINE__,  evalOnceErrorCode); \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                                                                             
  
            
                                                                              
                                                                    
                          
  
                                                                                  
                                                                         
                                                      
  
               
  
                
                                       
  
                     
                  
  
             
                                                    
  
              
                                
   
#ifndef __Require_noErr_Action_String
	#if DEBUG_ASSERT_PRODUCTION_CODE
	   #define __Require_noErr_Action_String(errorCode, exceptionLabel, action, message) \
		  do                                                                      \
		  {                                                                       \
			  if ( __builtin_expect(0 != (errorCode), 0) )                        \
			  {                                                                   \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#else
	   #define __Require_noErr_Action_String(errorCode, exceptionLabel, action, message) \
		  do                                                                      \
		  {                                                                       \
			  long evalOnceErrorCode = (errorCode);                               \
			  if ( __builtin_expect(0 != evalOnceErrorCode, 0) )                  \
			  {                                                                   \
				  DEBUG_ASSERT_MESSAGE(                                           \
					  DEBUG_ASSERT_COMPONENT_NAME_STRING,                         \
					  #errorCode " == 0 ", #exceptionLabel, message, __FILE__, __LINE__, evalOnceErrorCode); \
				  {                                                               \
					  action;                                                     \
				  }                                                               \
				  goto exceptionLabel;                                            \
			  }                                                                   \
		  } while ( 0 )
	#endif
#endif

  
                              
  
            
                                                                               
  
               
  
           
                                                                     
  
               
                                                                                          
                                                                                            
  
            
                                                  
                                                                  
                                                            
  
                                                        
                                                                                              
   
#ifndef __Check_Compile_Time
    #ifdef __GNUC__ 
        #define __Check_Compile_Time( expr )    \
            extern int compile_time_assert_failed[ ( expr ) ? 1 : -1 ] __attribute__( ( unused ) )
    #else
        #define __Check_Compile_Time( expr )    \
            extern int compile_time_assert_failed[ ( expr ) ? 1 : -1 ]
    #endif
#endif

  
                                                                                                         
                                                                                                      
                                      
  
                                                                                                 
                                                                                                       
                                                                                                           
                                                                                                          
                                                                   
                                                                                                            
                            
  
                                                                                                           
                                                                                                           
                                                 
  
                                                                             
                                                        
                                                                    
                                                                                  
                                                                      
                                                            
                                                                          
                                                                                        
                                                                        
                                                                                      
                                                                                                    
                                                                                      
                                                                          
                                                          
                                                                        
                                                                      
                                                                                    
                                                                                    
                                                                        
                                                          
                                                                        
                                                              
                                                                            
                                                                                        
                                                                                          
                                                                          
                                                                            
                                                            
                                                                          
                                                                                      
                                                                                                  
                                                                                    
                                                                        
                                                                                  
                                                                    
  
   

#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
	                                                                                                    
	#define	__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES	1
#endif

#if	__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES

	#ifndef check
	#define check(assertion)  __Check(assertion)
	#endif

	#ifndef check_noerr
	#define check_noerr(errorCode)  __Check_noErr(errorCode)
	#endif

	#ifndef check_noerr_string
		#define check_noerr_string(errorCode, message)  __Check_noErr_String(errorCode, message)
	#endif

	#ifndef check_string
		#define check_string(assertion, message)  __Check_String(assertion, message)
	#endif

	#ifndef require
		#define require(assertion, exceptionLabel)  __Require(assertion, exceptionLabel)
	#endif

	#ifndef require_action
		#define require_action(assertion, exceptionLabel, action)  __Require_Action(assertion, exceptionLabel, action)
	#endif

	#ifndef require_action_string
		#define require_action_string(assertion, exceptionLabel, action, message)  __Require_Action_String(assertion, exceptionLabel, action, message)
	#endif

	#ifndef require_noerr
		#define require_noerr(errorCode, exceptionLabel)  __Require_noErr(errorCode, exceptionLabel)
	#endif

	#ifndef require_noerr_action
		#define require_noerr_action(errorCode, exceptionLabel, action)  __Require_noErr_Action(errorCode, exceptionLabel, action)
	#endif

	#ifndef require_noerr_action_string
		#define require_noerr_action_string(errorCode, exceptionLabel, action, message)  __Require_noErr_Action_String(errorCode, exceptionLabel, action, message)
	#endif

	#ifndef require_noerr_string
		#define require_noerr_string(errorCode, exceptionLabel, message)  __Require_noErr_String(errorCode, exceptionLabel, message)
	#endif

	#ifndef require_string
		#define require_string(assertion, exceptionLabel, message)  __Require_String(assertion, exceptionLabel, message)
	#endif

	#ifndef verify
		#define verify(assertion) __Verify(assertion)
	#endif

	#ifndef verify_action
		#define verify_action(assertion, action)  __Verify_Action(assertion, action)
	#endif

	#ifndef verify_noerr
		#define verify_noerr(errorCode)  __Verify_noErr(errorCode)
	#endif

	#ifndef verify_noerr_action
		#define verify_noerr_action(errorCode, action)  __Verify_noErr_Action(errorCode, action)
	#endif

	#ifndef verify_noerr_string
		#define verify_noerr_string(errorCode, message)  __Verify_noErr_String(errorCode, message)
	#endif

	#ifndef verify_string
		#define verify_string(assertion, message)  __Verify_String(assertion, message)
	#endif

	#ifndef ncheck
		#define ncheck(assertion)  __nCheck(assertion)
	#endif

	#ifndef ncheck_string
		#define ncheck_string(assertion, message)  __nCheck_String(assertion, message)
	#endif

	#ifndef nrequire
		#define nrequire(assertion, exceptionLabel)  __nRequire(assertion, exceptionLabel)
	#endif

	#ifndef nrequire_action
		#define nrequire_action(assertion, exceptionLabel, action)  __nRequire_Action(assertion, exceptionLabel, action)
	#endif

	#ifndef nrequire_action_quiet
		#define nrequire_action_quiet(assertion, exceptionLabel, action)  __nRequire_Action_Quiet(assertion, exceptionLabel, action)
	#endif

	#ifndef nrequire_action_string
		#define nrequire_action_string(assertion, exceptionLabel, action, message)  __nRequire_Action_String(assertion, exceptionLabel, action, message)
	#endif

	#ifndef nrequire_quiet
		#define nrequire_quiet(assertion, exceptionLabel)  __nRequire_Quiet(assertion, exceptionLabel)
	#endif

	#ifndef nrequire_string
		#define nrequire_string(assertion, exceptionLabel, string)  __nRequire_String(assertion, exceptionLabel, string)
	#endif

	#ifndef nverify
		#define nverify(assertion)  __nVerify(assertion)
	#endif

	#ifndef nverify_string
		#define nverify_string(assertion, message)  __nVerify_String(assertion, message)
	#endif

	#ifndef require_action_quiet
		#define require_action_quiet(assertion, exceptionLabel, action)  __Require_Action_Quiet(assertion, exceptionLabel, action)
	#endif

	#ifndef require_noerr_action_quiet
		#define require_noerr_action_quiet(errorCode, exceptionLabel, action)  __Require_noErr_Action_Quiet(errorCode, exceptionLabel, action)
	#endif

	#ifndef require_noerr_quiet
		#define require_noerr_quiet(errorCode, exceptionLabel)  __Require_noErr_Quiet(errorCode, exceptionLabel)
	#endif

	#ifndef require_quiet
		#define require_quiet(assertion, exceptionLabel)  __Require_Quiet(assertion, exceptionLabel)
	#endif

	#ifndef check_compile_time
		#define check_compile_time( expr )  __Check_Compile_Time( expr )
	#endif

	#ifndef debug_string
		#define debug_string(message)  __Debug_String(message)
	#endif
	
#endif	                                                       


#endif                       
