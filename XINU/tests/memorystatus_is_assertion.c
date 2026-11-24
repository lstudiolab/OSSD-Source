#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <spawn.h>
#include <spawn_private.h>
#include <stdint.h>
#include <sys/sysctl.h>
#include <sys/spawn_internal.h>
#include <sys/kern_memorystatus.h>
#include <mach-o/dyld.h>

#include <darwintest.h>
#include <darwintest_utils.h>

#include "memorystatus_assertion_helpers.h"

T_GLOBAL_META(
	T_META_NAMESPACE("xnu.vm"),
	T_META_CHECK_LEAKS(false)
	);

extern char **environ;

  
                                                                                        
                                                                                                 
                                                                                          
                                            
  
                                                                                           
                                                                          
  
                                                                                   
                                               
  
                                                                      
               
                                                                                      
                                            
                                                                          
                                                                       
  
                                                                                            
                                                                                    
                                                                 
  
                                                                      
                                              
  
                                                                               
                                                                               
                                                                                   
                                                                                  
                              
   

  
                                                                       
   
#ifndef MEMORYSTATUS_SET_PRIORITY_ASSERTION
#define MEMORYSTATUS_SET_PRIORITY_ASSERTION 0x1
#endif

static void
proc_will_set_clean(pid_t pid)
{
	proc_set_dirty(pid, false);
	T_LOG("pid[%d] --> now clean", pid);
	return;
}

static void
proc_will_set_dirty(pid_t pid)
{
	proc_set_dirty(pid, true);
	T_LOG("pid[%d] --> now dirty", pid);
	return;
}

#define kJetsamAgingPolicyNone                          (0)
#define kJetsamAgingPolicyLegacy                        (1)
#define kJetsamAgingPolicySysProcsReclaimedFirst        (2)
#define kJetsamAgingPolicyAppsReclaimedFirst            (3)
#define kJetsamAgingPolicyMax                           kJetsamAgingPolicyAppsReclaimedFirst

#ifndef kMemorystatusAssertion
#define kMemorystatusAssertion 0x40
#endif

  
                                                                                      
                                                                                     
                                                                                
                                                                                    
                                                                             
                                                                               
                                                           
                               
                                                                    
                                                
                                               
                                                                                       
                                                
                                               
                                                                                     
                                                
                                               
   

static void
memorystatus_assertion_test_repetitive(char *test, boolean_t turn_on_dirty_tracking, boolean_t start_clean)
{
	int count;
	int maxcount = 3;
	boolean_t verbose;
	uint32_t state;
	uint64_t user_data = 0;
	pid_t mypid = getpid();

	                                                   
	int             active_limit_mb = 35;                  
	int             inactive_limit_mb = 25;                 

	                                       
	int             requestedpriority = 0;
	int             assertionpriority = 0;

	T_SETUPBEGIN;

	requestedpriority =  JETSAM_PRIORITY_UI_SUPPORT;
	assertionpriority =  JETSAM_PRIORITY_FOREGROUND;
	set_memlimits(mypid, active_limit_mb, inactive_limit_mb, true, true);
	set_priority(mypid, requestedpriority, 0, false);

	if (turn_on_dirty_tracking) {
		proc_track_dirty(mypid, (PROC_DIRTY_TRACK | PROC_DIRTY_ALLOW_IDLE_EXIT | PROC_DIRTY_DEFER));

		if (start_clean) {
			proc_will_set_clean(mypid);
		} else {
			proc_will_set_dirty(mypid);
		}
	} else {
		  
                
                                            
                                                         
                                                
     
	}


	verbose = false;
	(void)get_priority_props(mypid, verbose, NULL, NULL, NULL, NULL);

	                             
	T_LOG("SETUP STATE COMPLETE: Test %s", test);

	T_SETUPEND;

	int i;
	boolean_t ret;
	for (i = 0; i < 2; i++) {
		if (i == 1 && turn_on_dirty_tracking) {
			T_LOG("Avoid idle-deferred - sleeping for 20");
			sleep(20);

			if (start_clean) {
				proc_will_set_dirty(mypid);
			} else {
				proc_will_set_clean(mypid);
			}

			(void)get_priority_props(mypid, verbose, NULL, NULL, NULL, &state);
		}

		  
                                                       
                                          
     
		for (count = 0; count < maxcount; count++) {
			if (relinquish_assertion_priority(mypid, user_data)) {
				T_ASSERT_FAIL("relinquish_assertion_priority failed");
			}
		}

		                                            
		(void)get_priority_props(mypid, verbose, NULL, NULL, NULL, &state);

		ret = verify_assertion_state(state, ASSERTION_STATE_IS_RELINQUISHED);
		T_QUIET;
		T_ASSERT_TRUE(ret, "verify_assertion_state failed");



		  
                                                       
     
		for (count = 0; count < maxcount; count++) {
			if (set_assertion_priority(mypid, assertionpriority, user_data) != 0) {
				T_ASSERT_FAIL("set_assertion_priority failed");
			}
		}

		                                              
		(void)get_priority_props(mypid, verbose, NULL, NULL, NULL, &state);

		ret = verify_assertion_state(state, ASSERTION_STATE_IS_SET);
		T_QUIET;
		T_ASSERT_TRUE(ret, "verify_assertion_state failed");
	}
}

  
                                                          
   
static void
memorystatus_assertion_test_allow_idle_exit()
{
	pid_t mypid = getpid();

	                                                   
	int active_limit_mb   = 35;                
	int inactive_limit_mb = 25;                 

	                                       
	int requestedpriority = JETSAM_PRIORITY_UI_SUPPORT;

	T_SETUPBEGIN;

	set_memlimits(mypid, active_limit_mb, inactive_limit_mb, true, true);
	set_priority(mypid, requestedpriority, 0, false);

	proc_track_dirty(mypid, (PROC_DIRTY_TRACK | PROC_DIRTY_ALLOW_IDLE_EXIT | PROC_DIRTY_DEFER));

	proc_will_set_clean(mypid);

	(void)check_properties(mypid, JETSAM_PRIORITY_IDLE_DEFERRED, inactive_limit_mb, 0x0, ASSERTION_STATE_IS_RELINQUISHED, "Clean start");

	T_LOG("SETUP STATE COMPLETE");

	int g_jetsam_aging_policy = 0;
	  
                       
                                     
    
	size_t size = sizeof(g_jetsam_aging_policy);
	if (sysctlbyname("kern.jetsam_aging_policy", &g_jetsam_aging_policy, &size, NULL, 0) != 0) {
		T_LOG(__func__, true, "Unable to retrieve jetsam aging policy (not fatal)");
	}

	T_SETUPEND;

	  
                                                                                             
    
	T_LOG("********Test0 clean: no state change on relinquish");
	relinquish_assertion_priority(mypid, 0xF00D);
	(void)check_properties(mypid, JETSAM_PRIORITY_IDLE_DEFERRED, inactive_limit_mb, 0xF00D, ASSERTION_STATE_IS_RELINQUISHED, "Test0");

	T_LOG("********Test1 clean: deferred now assertion[10]");
	set_assertion_priority(mypid, JETSAM_PRIORITY_FOREGROUND, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, inactive_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test1");

	           
	T_LOG("********Test2 clean:  assertion[10 -> 3]");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFACE);
	(void)check_properties(mypid, JETSAM_PRIORITY_BACKGROUND, inactive_limit_mb, 0xFACE, ASSERTION_STATE_IS_SET, "Test2");

	           
	T_LOG("********Test3 clean: assertion[3 -> 0], but now deferred");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_IDLE_DEFERRED, inactive_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test3");

	           
	T_LOG("********Test4 clean: deferred now assertion[10]");
	set_assertion_priority(mypid, JETSAM_PRIORITY_FOREGROUND, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, inactive_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test4");

	T_LOG("Avoid idle-deferred moving forward. Sleeping for 20");
	sleep(20);

	           
	T_LOG("********Test5 dirty: set dirty priority but assertion[10] prevails");
	proc_will_set_dirty(mypid);                                       
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test5");

	           
	T_LOG("********Test6 dirty: assertion[10 -> 3] but dirty priority prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFEEB);                               
	(void)check_properties(mypid, JETSAM_PRIORITY_UI_SUPPORT, active_limit_mb, 0xFEEB, ASSERTION_STATE_IS_SET, "Test6");

	           
	T_LOG("********Test7 dirty: assertion[3 -> 0] but dirty prevails");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_UI_SUPPORT, active_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test7");


	           
	T_LOG("********Test8 dirty: assertion[0 -> 10] overrides dirty");
	set_assertion_priority(mypid, JETSAM_PRIORITY_FOREGROUND, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test8");

	           
	T_LOG("********Test9 dirty wants to go clean, but clean state is prevented as assertion[10] prevails");
	proc_will_set_clean(mypid);
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test9");

	            
	T_LOG("********Test10 dirty goes dirty and stays dirty, and assertion[10] prevails again");
	proc_will_set_dirty(mypid);
	(void)check_properties(mypid, JETSAM_PRIORITY_FOREGROUND, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test10");

	            
	T_LOG("********Test11 dirty: assertion[10 -> 3] but dirty prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFACE);
	(void)check_properties(mypid, JETSAM_PRIORITY_UI_SUPPORT, active_limit_mb, 0xFACE, ASSERTION_STATE_IS_SET, "Test11");

	            
	T_LOG("********Test12 dirty: assertion[3 -> 0] but dirty prevails");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_UI_SUPPORT, active_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test12");


	            
	T_LOG("********Test13 dirty goes clean: both assertion[0] and clean");
	proc_will_set_clean(mypid);
	if (g_jetsam_aging_policy == kJetsamAgingPolicySysProcsReclaimedFirst) {
		                                                                                                    
		(void)check_properties(mypid, JETSAM_PRIORITY_IDLE_DEFERRED, active_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test13");
	} else {
		                                                                                           
		(void)check_properties(mypid, JETSAM_PRIORITY_IDLE, inactive_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test13");
	}
}

  
                                                                  
                                                                    
                                                                             
   
static void
memorystatus_assertion_test_do_not_allow_idle_exit()
{
	pid_t mypid = getpid();

	                                                   
	int             active_limit_mb = 35;                  
	int             inactive_limit_mb = 25;                 
	int             requestedpriority = JETSAM_PRIORITY_AUDIO_AND_ACCESSORY;

	T_SETUPBEGIN;

	set_memlimits(mypid, active_limit_mb, inactive_limit_mb, true, true);
	set_priority(mypid, requestedpriority, 0, false);
	proc_track_dirty(mypid, (PROC_DIRTY_TRACK));

	proc_will_set_dirty(mypid);

	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, active_limit_mb, 0x0, ASSERTION_STATE_IS_RELINQUISHED, "Dirty start");

	proc_will_set_clean(mypid);

	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0x0, ASSERTION_STATE_IS_RELINQUISHED, "Clean transition");

	T_LOG("SETUP STATE COMPLETE");

	T_SETUPEND;

	  
                                                                                             
    


	           
	T_LOG("********Test0 clean: no state change on relinquish");
	relinquish_assertion_priority(mypid, 0xF00D);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xF00D, ASSERTION_STATE_IS_RELINQUISHED, "Test0");

	           
	T_LOG("********Test1 clean: assertion[0 -> 10] but inactive priority prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_FOREGROUND, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test1");

	           
	T_LOG("********Test2 clean:  assertion[10 -> 3] but inactive priority prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFACE);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xFACE, ASSERTION_STATE_IS_SET, "Test2");

	           
	T_LOG("********Test3 clean: assertion[3 -> 0], but inactive priority prevails");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test3");

	           
	T_LOG("********Test4 go dirty: assertion[0] has no affect, active priority prevails");
	proc_will_set_dirty(mypid);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, active_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test4");

	           
	T_LOG("********Test5 dirty: assertion[0 -> 10] active priority prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_FOREGROUND, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test5");

	           
	T_LOG("********Test6 dirty:  assertion[10 -> 3] active priority prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFACE);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, active_limit_mb, 0xFACE, ASSERTION_STATE_IS_SET, "Test6");

	            
	T_LOG("********Test7 dirty: assertion[3 -> 0], active priority prevails");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, active_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test7");

	           
	T_LOG("********Test8 dirty: assertion[0 -> 19], dirty but now assertion[19] prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_CRITICAL, 0xFEED);
	(void)check_properties(mypid, JETSAM_PRIORITY_CRITICAL, active_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test8");


	           
	T_LOG("********Test9 go clean: inactive priority but assertion[19] prevails");
	proc_will_set_clean(mypid);
	(void)check_properties(mypid, JETSAM_PRIORITY_CRITICAL, inactive_limit_mb, 0xFEED, ASSERTION_STATE_IS_SET, "Test9");

	            
	T_LOG("********Test10 clean:  assertion[19 -> 3] inactive limit prevails");
	set_assertion_priority(mypid, JETSAM_PRIORITY_BACKGROUND, 0xFACE);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xFACE, ASSERTION_STATE_IS_SET, "Test10");


	            
	T_LOG("********Test11 clean:  assertion[3 -> 0] inactive priority still prevails");
	relinquish_assertion_priority(mypid, 0xBEEF);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test11");

	            
	T_LOG("********Test12 dirty goes clean: both assertion[0] and clean");
	proc_will_set_clean(mypid);
	(void)check_properties(mypid, JETSAM_PRIORITY_AUDIO_AND_ACCESSORY, inactive_limit_mb, 0xBEEF, ASSERTION_STATE_IS_RELINQUISHED, "Test12");
}

T_DECL(assertion_test_bad_flags, "verify bad flag returns an error", T_META_TIMEOUT(30), T_META_ASROOT(true)) {
	int err;
	uint32_t flag = 0;

	memorystatus_priority_properties_t mjp = { 0 };

	mjp.priority = JETSAM_PRIORITY_FOREGROUND;
	mjp.user_data = 0;

	  
                   
    

	flag = 0xf;

	err = memorystatus_control(MEMORYSTATUS_CMD_SET_PRIORITY_PROPERTIES, getpid(), flag, &mjp, sizeof(mjp));

	T_QUIET;
	T_ASSERT_POSIX_FAILURE(err, EINVAL, "MEMORYSTATUS_CMD_SET_PRIORITY_PROPERTIES should fail with bad flags (err=%d)", err);
}


T_DECL(assertion_test_repetitive_non_dirty_tracking, "Scenario #1 - repetitive assertion priority on non-dirty-tracking process", T_META_TIMEOUT(60), T_META_ASROOT(true)) {
	  
                                                                        
                                                             
                                             
    

	memorystatus_assertion_test_repetitive("Scenario #1", false, false);
}

T_DECL(assertion_test_repetitive_dirty_tracking_clean, "Scenario #2 - repetitive assertion priority on clean dirty-tracking process", T_META_TIMEOUT(60), T_META_ASROOT(true)) {
	  
                                                                        
                                                                     
                                   
                                           
    
	memorystatus_assertion_test_repetitive("Scenario #2", true, true);
}

T_DECL(assertion_test_repetitive_dirty_tracking_dirty, "Scenario #3 - repetitive assertion priority on dirty dirty-tracking processes", T_META_TIMEOUT(60), T_META_ASROOT(true)) {
	  
                                                                        
                                                                     
                                        
                                           
    
	memorystatus_assertion_test_repetitive("Scenario #3", true, false);
}


T_DECL(assertion_test_allow_idle_exit, "set assertion priorities on process supporting idle exit", T_META_TIMEOUT(360), T_META_ASROOT(true)) {
	memorystatus_assertion_test_allow_idle_exit();
}

T_DECL(assertion_test_do_not_allow_idle_exit, "set assertion priorities on process no idle exit allowed", T_META_TIMEOUT(360), T_META_ASROOT(true)) {
	memorystatus_assertion_test_do_not_allow_idle_exit();
}
