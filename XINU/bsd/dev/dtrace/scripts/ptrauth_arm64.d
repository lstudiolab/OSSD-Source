/*
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */

enum ptrauth_key {
  ptrauth_key_asia = 0,
  ptrauth_key_asib = 1,
  ptrauth_key_asda = 2,
  ptrauth_key_asdb = 3,

                                                                       
                                                                     
                                                 
  ptrauth_key_process_independent_code = ptrauth_key_asia,

                                                                    
                                                                           
                                                 
  ptrauth_key_process_dependent_code = ptrauth_key_asib,

                                                                       
                                                                     
                                                 
  ptrauth_key_process_independent_data = ptrauth_key_asda,

                                                                    
                                                                     
                                                 
  ptrauth_key_process_dependent_data = ptrauth_key_asdb,

                                              
                                   
  ptrauth_key_function_pointer = ptrauth_key_process_independent_code,

                                                        
                                                                          
                                                                              
                                                                              
  ptrauth_key_return_address = ptrauth_key_process_dependent_code,

                                                      
                                                                         
                                                                              
                                                                              
  ptrauth_key_frame_pointer = ptrauth_key_process_dependent_data,

                                                             
                            
                                   
                                      
                                           
                                          
                                                                       
               

                                                                
                                                                        
  ptrauth_key_block_function = ptrauth_key_asia,

                                               
                                   
  ptrauth_key_cxx_vtable_pointer = ptrauth_key_asda

};

