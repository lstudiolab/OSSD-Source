#ifndef MEMORYSTATUS_ASSERTION_HELPERS_H
#define MEMORYSTATUS_ASSERTION_HELPERS_H

#include <stdlib.h>
#include <stdint.h>

#define ASSERTION_STATE_IS_SET          true
#define ASSERTION_STATE_IS_RELINQUISHED false

                                                                    
                
   

  
                                                       
  
                                                       
                                        
  
                                                      
                                             
  
                                                         
                                      
  
                           
                      
   
int
set_priority(pid_t pid, int32_t priority, uint64_t user_data, boolean_t is_assertion_driven);

  
                          
                                                                      
   
boolean_t
check_properties(pid_t pid, int32_t expected_priority, int32_t expected_limit_mb, uint64_t expected_user_data, boolean_t expected_assertion_state, const char *test);

  
                                                        
                                     
  
                            
                                   
   
int
set_memlimits(
	pid_t pid,
	int32_t active_limit_mb, int32_t inactive_limit_mb,
	boolean_t active_is_fatal, boolean_t inactive_is_fatal);

  
                           
                      
   
int
set_assertion_priority(pid_t pid, int32_t priority, uint64_t user_data);

  
                           
                      
   
int
relinquish_assertion_priority(pid_t pid, uint64_t user_data);

  
                                                    
  
                                                                      
                                                           
                                                               
  
                          
                                                            
   
boolean_t
get_priority_props(pid_t pid, boolean_t verbose, int32_t *priority, int32_t *limit_mb, uint64_t *user_data, uint32_t *state);

  
         
                                                                
                                 
                                                         
                                                           
  
                                     
                                
   
boolean_t
verify_assertion_state(uint32_t state, boolean_t expected_assertion_state);

#endif                                       
