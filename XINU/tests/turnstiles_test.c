  
                                                     
   

#ifdef T_NAMESPACE
#undef T_NAMESPACE
#endif

#include <darwintest.h>
#include <darwintest_multiprocess.h>

#include <pthread.h>
#include <launch.h>
#include <servers/bootstrap.h>
#include <stdlib.h>
#include <sys/event.h>
#include <unistd.h>
#include <crt_externs.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#define SYSCTL_TURNSTILE_TEST_USER_DEFAULT            1
#define SYSCTL_TURNSTILE_TEST_USER_HASHTABLE          2
#define SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT          3
#define SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE        4

T_GLOBAL_META(T_META_NAMESPACE("xnu.turnstiles_test"));

static void
thread_create_at_qos(qos_class_t qos, void * (*function)(void *), int type)
{
	qos_class_t qos_thread;
	pthread_t thread;
	pthread_attr_t attr;
	int ret;

	ret = setpriority(PRIO_DARWIN_ROLE, 0, PRIO_DARWIN_ROLE_UI_FOCAL);
	if (ret != 0) {
		T_LOG("set priority failed\n");
	}

	pthread_attr_init(&attr);
	pthread_attr_set_qos_class_np(&attr, qos, 0);
	pthread_create(&thread, &attr, function, (void *)type);

	T_LOG("pthread created\n");
	pthread_get_qos_class_np(thread, &qos_thread, NULL);
	T_EXPECT_EQ(qos_thread, (qos_class_t)qos, NULL);
}

static int
get_sched_pri(thread_t thread_port)
{
	kern_return_t kr;

	thread_extended_info_data_t extended_info;
	mach_msg_type_number_t count = THREAD_EXTENDED_INFO_COUNT;
	kr = thread_info(thread_port, THREAD_EXTENDED_INFO,
	    (thread_info_t)&extended_info, &count);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "thread_info");
	return extended_info.pth_curpri;
}

static int
get_base_pri(thread_t thread_port)
{
	kern_return_t kr;

	thread_extended_info_data_t extended_info;
	mach_msg_type_number_t count = THREAD_EXTENDED_INFO_COUNT;
	kr = thread_info(thread_port, THREAD_EXTENDED_INFO,
	    (thread_info_t)&extended_info, &count);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "thread_info");
	return extended_info.pth_priority;
}

static void
turnstile_prim_lock(int type)
{
	int ret;
	uint64_t tid;
	int in_val = type;
	pthread_threadid_np(NULL, &tid);
	T_LOG("sysctlbyname lock type %d called from thread %llu \n", type, tid);
	ret = sysctlbyname("kern.turnstiles_test_lock", NULL, 0, &in_val, sizeof(in_val));
	T_LOG("sysctlbyname lock returned from thread %llu with value %d \n", tid, ret);
}

static void
turnstile_prim_unlock(int type)
{
	int ret;
	uint64_t tid;
	int in_val = type;
	pthread_threadid_np(NULL, &tid);
	T_LOG("sysctlbyname unlock type %d called from thread %llu \n", type, tid);
	ret = sysctlbyname("kern.turnstiles_test_unlock", NULL, 0, &in_val, sizeof(in_val));
	T_LOG("sysctlbyname unlock returned from thread %llu with value %d \n", tid, ret);
}

struct thread_data {
	int pri_to_set;
	int lock1;
	int lock2;
	unsigned int sleep;
	int sched_pri_to_check;
	int base_pri_to_check;
};

static void *
chain_locking(void* args)
{
	struct thread_data* data = (struct thread_data*) args;
	int policy, pri;
	int ret;
	struct sched_param param;

	                                       
	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	T_QUIET; T_ASSERT_MACH_SUCCESS(ret, "pthread_getschedparam");

	param.sched_priority = data->pri_to_set;

	                                       
	ret = pthread_setschedparam(pthread_self(), policy, &param);
	T_QUIET; T_ASSERT_MACH_SUCCESS(ret, "pthread_setschedparam");

	pri = get_sched_pri(mach_thread_self());

	T_ASSERT_EQ(pri, data->pri_to_set, "Priority before holding locks");

	                
	if (data->lock1) {
		turnstile_prim_lock(data->lock1);
	}

	                
	if (data->lock2) {
		turnstile_prim_lock(data->lock2);
	}

	if (data->sleep) {
		sleep(data->sleep);
	}

	if (data->sched_pri_to_check) {
		pri = get_sched_pri(mach_thread_self());
		T_ASSERT_EQ(pri, data->sched_pri_to_check, "Sched priority while holding locks");
	}

	if (data->base_pri_to_check) {
		pri = get_base_pri(mach_thread_self());
		T_ASSERT_EQ(pri, data->base_pri_to_check, "Base priority while holding locks");
	}

	if (data->lock2) {
		turnstile_prim_unlock(data->lock2);
	}

	if (data->lock1) {
		turnstile_prim_unlock(data->lock1);
	}

	pri = get_sched_pri(mach_thread_self());
	T_ASSERT_EQ(pri, data->pri_to_set, "Priority after releasing locks");

	return NULL;
}

static void *
take_lock_check_priority(void * arg)
{
	int old_pri = get_base_pri(mach_thread_self());
	int unboosted_pri;
	int boosted_pri;
	int after_unlock_pri;
	uint64_t tid;
	int type = (int)arg;

	pthread_threadid_np(NULL, &tid);

	T_ASSERT_EQ(old_pri, 37, "thread(%llu) priority before acquiring the lock is %d\n", tid, old_pri);

	                        
	turnstile_prim_lock(type);

	unboosted_pri = get_base_pri(mach_thread_self());
	T_ASSERT_EQ(unboosted_pri, 37, "thread(%llu) priority after acquiring the lock (uncontended) is %d\n", tid, unboosted_pri);

	sleep(8);

	                                 
	boosted_pri =  get_base_pri(mach_thread_self());
	T_ASSERT_EQ(boosted_pri, 47, "thread(%llu) priority after contention by 47 thread is %d\n", tid, boosted_pri);

	                   
	turnstile_prim_unlock(type);

	                                
	after_unlock_pri =  get_base_pri(mach_thread_self());
	T_ASSERT_EQ(after_unlock_pri, 37, "thread(%llu) priority after dropping lock is %d\n", tid, after_unlock_pri);

	return NULL;
}

static void *
try_to_take_lock_and_unlock(void *arg)
{
	uint64_t tid;
	int type = (int)arg;

	pthread_threadid_np(NULL, &tid);
	sleep(4);

	int old_pri = get_base_pri(mach_thread_self());
	T_ASSERT_EQ(old_pri, 47, "thread(%llu) priority before acquiring the lock is %d\n", tid, old_pri);

	                              
	turnstile_prim_lock(type);
	sleep(2);
	turnstile_prim_unlock(type);
	return NULL;
}

static void *
take_lock_and_exit(void * arg)
{
	int old_pri = get_base_pri(mach_thread_self());
	int unboosted_pri;
	int boosted_pri;
	uint64_t tid;
	int type = (int)arg;

	pthread_threadid_np(NULL, &tid);

	T_ASSERT_EQ(old_pri, 37, "thread(%llu) priority before acquiring the lock is %d\n", tid, old_pri);

	                        
	turnstile_prim_lock(type);

	unboosted_pri =  get_base_pri(mach_thread_self());
	T_ASSERT_EQ(unboosted_pri, 37, "thread(%llu) priority after acquiring the lock (uncontended) is %d\n", tid, unboosted_pri);

	sleep(8);

	                                 
	boosted_pri =  get_base_pri(mach_thread_self());
	T_ASSERT_EQ(boosted_pri, 47, "thread(%llu) priority after contention by 47 thread is %d\n", tid, boosted_pri);

	                                       
	return NULL;
}

static void *
unlock_an_owner_exited_lock(void *arg)
{
	uint64_t tid;
	int type = (int)arg;

	pthread_threadid_np(NULL, &tid);
	sleep(12);

	int old_pri = get_base_pri(mach_thread_self());
	T_ASSERT_EQ(old_pri, 47, "thread(%llu) priority before acquiring the lock is %d\n", tid, old_pri);

	                                                                                    
	turnstile_prim_unlock(type);
	return NULL;
}

  
                                                                            
   
static void
test1(int type)
{
	T_LOG("Test 1: test if lock contended by a UI thread boosts the owner to UI qos");

	                                         
	thread_create_at_qos(QOS_CLASS_USER_INITIATED, &take_lock_check_priority, type);

	                                                
	thread_create_at_qos(QOS_CLASS_USER_INTERACTIVE, &try_to_take_lock_and_unlock, type);

	sleep(12);
	return;
}

  
                                                                              
   
static void
test2(int type)
{
	T_LOG("Test 2: test if lock contended by a 2 UI thread boosts the owner to UI qos");

	                                         
	thread_create_at_qos(QOS_CLASS_USER_INITIATED, &take_lock_check_priority, type);

	                                                
	thread_create_at_qos(QOS_CLASS_USER_INTERACTIVE, &try_to_take_lock_and_unlock, type);

	                                                
	thread_create_at_qos(QOS_CLASS_USER_INTERACTIVE, &try_to_take_lock_and_unlock, type);

	sleep(16);
	return;
}

  
                                                                                                  
   
static void
test3(int type)
{
	T_LOG("Test 3: test if lock owner thread exiting without unlocking allows turnstile to work correctly");

	                                         
	thread_create_at_qos(QOS_CLASS_USER_INITIATED, &take_lock_and_exit, type);

	                                                
	thread_create_at_qos(QOS_CLASS_USER_INTERACTIVE, &try_to_take_lock_and_unlock, type);

	                                                
	thread_create_at_qos(QOS_CLASS_USER_INTERACTIVE, &unlock_an_owner_exited_lock, type);

	sleep(16);
	return;
}

  
                                                                                                            
   
static void
test4(void)
{
	pthread_t threads[5] = {};
	struct thread_data data[5] = {};

	T_LOG("Test 4: test if a chain of user-space turnstile primitives followed by kernel primitives works correctly");

	  
                                             
                                                                      
                                                           
                                                
                                                       
    


	                                                                                      
	data[0].pri_to_set = 4;
	data[0].lock1 = SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT;                                
	data[0].lock2 = NULL;
	data[0].sleep = 10;                                                  
	data[0].sched_pri_to_check = 60;
	data[0].base_pri_to_check = 4;
	pthread_create(&threads[0], NULL, chain_locking, (void *)&data[0]);
	sleep(2);                                               

	                                                                                                                                          
	data[1].pri_to_set = 31;
	data[1].lock1 = SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE;                                
	data[1].lock2 = SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT;                            
	data[1].sleep = 0;                                                                                      
	data[1].sched_pri_to_check = 60;
	data[1].base_pri_to_check = 31;
	pthread_create(&threads[1], NULL, chain_locking, (void *)&data[1]);
	sleep(2);                                               

	                                                                                                                                          
	data[2].pri_to_set = 40;
	data[2].lock1 = SYSCTL_TURNSTILE_TEST_USER_HASHTABLE;                                
	data[2].lock2 = SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE;                            
	data[2].sleep = 0;                                                                                      
	data[2].sched_pri_to_check = 60;
	data[2].base_pri_to_check = 60;
	pthread_create(&threads[2], NULL, chain_locking, (void *)&data[2]);
	sleep(2);                                               

	                                                                                                                                      
	data[3].pri_to_set = 47;
	data[3].lock1 = SYSCTL_TURNSTILE_TEST_USER_DEFAULT;                                
	data[3].lock2 = SYSCTL_TURNSTILE_TEST_USER_HASHTABLE;                            
	data[3].sleep = 0;                                                                                      
	data[3].sched_pri_to_check = 60;
	data[3].base_pri_to_check = 60;
	pthread_create(&threads[3], NULL, chain_locking, (void *)&data[3]);
	sleep(2);                                               

	                                                                                
	data[4].pri_to_set = 60;
	data[4].lock1 = SYSCTL_TURNSTILE_TEST_USER_DEFAULT;                            
	data[4].lock2 = NULL;
	data[4].sleep = 0;                                                                                   
	data[4].sched_pri_to_check = 60;                               
	data[4].base_pri_to_check = 60;
	pthread_create(&threads[4], NULL, chain_locking, (void *)&data[4]);

	sleep(16);
	return;
}

  
                                                                                                               
   
static void
test5(void)
{
	pthread_t threads[5] = {};
	struct thread_data data[5] = {};

	T_LOG("Test 5: test if a chain of user-space turnstile primitives interleaved by kernel primitives works correctly");

	  
                                             
                                                                      
                                                           
                                                       
    

	                                                                                      
	data[0].pri_to_set = 4;
	data[0].lock1 = SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT;                                
	data[0].lock2 = NULL;
	data[0].sleep = 10;                                                  
	data[0].sched_pri_to_check = 41;
	data[0].base_pri_to_check = 4;
	pthread_create(&threads[0], NULL, chain_locking, (void *)&data[0]);
	sleep(2);                                               

	                                                                                                                                        
	data[1].pri_to_set = 31;
	data[1].lock1 = SYSCTL_TURNSTILE_TEST_USER_HASHTABLE;                                
	data[1].lock2 = SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT;                            
	data[1].sleep = 0;                                                                                      
	data[1].sched_pri_to_check = 41;
	data[1].base_pri_to_check = 41;
	pthread_create(&threads[1], NULL, chain_locking, (void *)&data[1]);
	sleep(2);                                               

	                                                                                                                                          
	data[2].pri_to_set = 41;
	data[2].lock1 = SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE;                                
	data[2].lock2 = SYSCTL_TURNSTILE_TEST_USER_HASHTABLE;                            
	data[2].sleep = 0;                                                                                      
	data[2].sched_pri_to_check = 60;
	data[2].base_pri_to_check = 41;
	pthread_create(&threads[2], NULL, chain_locking, (void *)&data[2]);
	sleep(2);                                               

	                                                                                                                                        
	data[3].pri_to_set = 47;
	data[3].lock1 = SYSCTL_TURNSTILE_TEST_USER_DEFAULT;                                
	data[3].lock2 = SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE;                            
	data[3].sleep = 0;                                                                                      
	data[3].sched_pri_to_check = 60;
	data[3].base_pri_to_check = 60;
	pthread_create(&threads[3], NULL, chain_locking, (void *)&data[3]);
	sleep(2);                                               

	                                                                                
	data[4].pri_to_set = 60;
	data[4].lock1 = SYSCTL_TURNSTILE_TEST_USER_DEFAULT;                            
	data[4].lock2 = NULL;
	data[4].sleep = 0;                                                                                   
	data[4].sched_pri_to_check = 60;                               
	data[4].base_pri_to_check = 60;
	pthread_create(&threads[4], NULL, chain_locking, (void *)&data[4]);

	sleep(16);
	return;
}

T_DECL(turnstile_test, "Turnstile test", T_META_ASROOT(YES))
{
	test1(SYSCTL_TURNSTILE_TEST_USER_DEFAULT);
	test2(SYSCTL_TURNSTILE_TEST_USER_DEFAULT);
	test3(SYSCTL_TURNSTILE_TEST_USER_DEFAULT);

	test1(SYSCTL_TURNSTILE_TEST_USER_HASHTABLE);
	test2(SYSCTL_TURNSTILE_TEST_USER_HASHTABLE);
	test3(SYSCTL_TURNSTILE_TEST_USER_HASHTABLE);

	  
                           
                                                                                       
                                                                                
                                                                 
    
	          
	          
}
