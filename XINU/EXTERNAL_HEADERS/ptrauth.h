/*===---- ptrauth.h - Pointer authentication -------------------------------===
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __PTRAUTH_H
#define __PTRAUTH_H

#include <stdint.h>

typedef enum {
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

                                               
                                   
  ptrauth_key_cxx_vtable_pointer = ptrauth_key_asda,

                                                                  

} ptrauth_key;

                                                                         
typedef uintptr_t ptrauth_extra_data_t;

                                                                      
typedef uintptr_t ptrauth_generic_signature_t;

                                                                   
                                                                     
                                                              
                                                                    
                                                               
                                                                 
                                                                   
                                                     

                                                                  
                                                        
                                                              
                                                             
                               

                                                                
                                                                 
                                                                 
                                                                 
                                                            
                                                       
                                                              
                         

                                                                
                                                          

#ifdef __PTRAUTH_INTRINSICS__

                                                              

                                                               
                                                               
                                                                  

                                                   
                                                             
                                                               
#define ptrauth_strip(__value, __key) \
  __builtin_ptrauth_strip(__value, __key)

                                                               
                                                               
                            

                                                                    

                                                                   
                                                                     
                                                                  

                                                            
                                                             
                                          
#define ptrauth_blend_discriminator(__pointer, __integer) \
  __builtin_ptrauth_blend_discriminator(__pointer, __integer)

                                                              

                               
  
#if __has_builtin(__builtin_ptrauth_type_discriminator)
#define ptrauth_type_discriminator(__type) \
  __builtin_ptrauth_type_discriminator(__type)
#else
#define ptrauth_type_discriminator(__type) ((uintptr_t)0)
#endif

                                                                             
                                 

                                                              
#if __has_feature(ptrauth_function_pointer_type_discrimination)
#define ptrauth_function_pointer_type_discriminator(__type) \
  __builtin_ptrauth_type_discriminator(__type)
#else
#define ptrauth_function_pointer_type_discriminator(__type) ((uintptr_t)0)
#endif

                                                                   
                                                               

                                                           
                                                             
                                                                           
                                                             
                                                            

                                                                   
                            
#define ptrauth_sign_constant(__value, __key, __data) \
  __builtin_ptrauth_sign_constant(__value, __key, __data)

                                                                   
                                                               

                                                                 
                                                               
                      

                                                   
                                                             
                                                                   
                                                             
                                                               
#define ptrauth_sign_unauthenticated(__value, __key, __data) \
  __builtin_ptrauth_sign_unauthenticated(__value, __key, __data)

                                                                       

                                                                         
                                                                  
                         

                                                   
                                                             
                                                                   
                                                             
                                                            

                                                                   
                                               

                                                              
                                         
#define ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data) \
  __builtin_ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data)

                                                               
                    

                                                                         
                                                                  
                         

                                                            
                                                             
                                                                   
                                                             
                                                            

                                                                   
                                                                     
                                                                   
                                                           
#define ptrauth_auth_function(__value, __old_key, __old_data) \
  ptrauth_auth_and_resign(__value, __old_key, __old_data, ptrauth_key_function_pointer, 0)

                                                                   

                                                      
                                                                           
                                            
                                                   

                                                                   
#define ptrauth_nop_cast(__type, __value)        \
  ({ union {                                     \
      typeof(__value) __fptr;                    \
      typeof(__type) __opaque;                   \
  } __storage;                                   \
  __storage.__fptr = (__value);                  \
  __storage.__opaque; })

                               

                                                                
                                                             
                                                                   
                                                             
                                                            

                                                                   
                
#define ptrauth_auth_data(__value, __old_key, __old_data) \
  __builtin_ptrauth_auth(__value, __old_key, __old_data)

                                                                
                                                               
                                             

                                         
                                                                
#define ptrauth_string_discriminator(__string) \
  __builtin_ptrauth_string_discriminator(__string)

                                                               
                                         

                                                                
                             

                                                               
                                                                  
                                                                  
                                                                   
                                                        

                                                                 
                                               
                                                                   
                                                             
                                                         

                                                                       
                                               
#define ptrauth_sign_generic_data(__value, __data) \
  __builtin_ptrauth_sign_generic_data(__value, __data)


                                                                
#define __ptrauth_function_pointer            \
  __ptrauth(ptrauth_key_function_pointer,0,0)
#define __ptrauth_return_address              \
  __ptrauth(ptrauth_key_return_address,1,0)
#define __ptrauth_block_invocation_pointer    \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_copy_helper           \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_destroy_helper        \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_byref_copy_helper     \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_byref_destroy_helper  \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_objc_method_list_imp        \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_cxx_vtable_pointer          \
  __ptrauth(ptrauth_key_cxx_vtable_pointer,0,0)
#define __ptrauth_cxx_vtt_vtable_pointer      \
  __ptrauth(ptrauth_key_cxx_vtable_pointer,0,0)
#define __ptrauth_swift_heap_object_destructor \
  __ptrauth(ptrauth_key_function_pointer,1,0xbbbf)

                                                                     
                                            
#define __ptrauth_cxx_virtual_function_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_function_pointer(__typekey) \
  __ptrauth(ptrauth_key_function_pointer,0,__typekey)
#define __ptrauth_swift_class_method_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_protocol_witness_function_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_value_witness_function_pointer(__key) \
  __ptrauth(ptrauth_key_function_pointer,1,__key)

#else

#define ptrauth_strip(__value, __key) ({ (void)__key; __value; })
#define ptrauth_blend_discriminator(__pointer, __integer) ({ (void)__pointer; (void)__integer; (uintptr_t)0; })
#define ptrauth_type_discriminator(__type) ((uintptr_t)0)
#define ptrauth_function_pointer_type_discriminator(__type) ((uintptr_t)0)
#define ptrauth_sign_constant(__value, __key, __data) ({ (void)__key; (void)__data; __value; })
#define ptrauth_sign_unauthenticated(__value, __key, __data) ({ (void)__key; (void)__data; __value; })
#define ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data) ({ \
      (void)__old_key; \
      (void)__old_data; \
      (void)__new_key; \
      (void)__new_data; \
      __value; })
#define ptrauth_auth_function(__value, __old_key, __old_data) ({ (void)__old_key; (void)__old_data; __value; })
#define ptrauth_nop_cast(__type, __value) ((__type)__value)
#define ptrauth_auth_data(__value, __old_key, __old_data) ({ (void)__old_key; (void)__old_data; __value; })
#define ptrauth_string_discriminator(__string) ({ (void)__string; (int)0; })
#define ptrauth_sign_generic_data(__value, __data) ({ (void)__value; (void)__data; (ptrauth_generic_signature_t)0; })

#define __ptrauth_function_pointer
#define __ptrauth_return_address
#define __ptrauth_block_invocation_pointer
#define __ptrauth_block_copy_helper
#define __ptrauth_block_destroy_helper
#define __ptrauth_block_byref_copy_helper
#define __ptrauth_block_byref_destroy_helper
#define __ptrauth_objc_method_list_imp
#define __ptrauth_cxx_vtable_pointer
#define __ptrauth_cxx_vtt_vtable_pointer
#define __ptrauth_swift_heap_object_destructor
#define __ptrauth_cxx_virtual_function_pointer(__declkey)
#define __ptrauth_swift_function_pointer(__typekey)
#define __ptrauth_swift_class_method_pointer(__declkey)
#define __ptrauth_swift_protocol_witness_function_pointer(__declkey)
#define __ptrauth_swift_value_witness_function_pointer(__key)

#endif                             

#endif                  
