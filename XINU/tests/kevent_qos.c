  
                                                  
   

#ifdef T_NAMESPACE
#undef T_NAMESPACE
#endif

#include <darwintest.h>
#include <darwintest_multiprocess.h>

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <launch.h>
#include <mach/mach.h>
#include <mach/message.h>
#include <mach/mach_voucher.h>
#include <pthread/workqueue_private.h>
#include <voucher/ipc_pthread_priority_types.h>
#include <servers/bootstrap.h>
#include <stdlib.h>
#include <sys/event.h>
#include <unistd.h>
#include <crt_externs.h>
#include <mach/mach_port.h>
#include <mach/mach_sync_ipc.h>

T_GLOBAL_META(T_META_NAMESPACE("xnu.kevent_qos"));

#define ARRAYLEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define INTERMITTENT_TIMEOUT_SEC (3)
#define RECV_TIMEOUT_SECS   (4)
#define SEND_TIMEOUT_SECS   (6)
#define HELPER_TIMEOUT_SECS (15)

#define ENV_VAR_QOS (3)
static const char *qos_env[ENV_VAR_QOS] = {"XNU_TEST_QOS_BO", "XNU_TEST_QOS_QO", "XNU_TEST_QOS_AO"};
static const char *qos_name_env[ENV_VAR_QOS] = {"XNU_TEST_QOS_NAME_BO", "XNU_TEST_QOS_NAME_QO", "XNU_TEST_QOS_NAME_AO"};

#define ENV_VAR_FUNCTION (1)
static const char *wl_function_name = "XNU_TEST_WL_FUNCTION";

static qos_class_t g_expected_qos[ENV_VAR_QOS];
static const char *g_expected_qos_name[ENV_VAR_QOS];

#define ENV_QOS_BEFORE_OVERRIDE (0)
#define ENV_QOS_QUEUE_OVERRIDE  (1)
#define ENV_QOS_AFTER_OVERRIDE  (2)

struct test_msg {
	mach_msg_header_t header;
	mach_msg_body_t body;
	mach_msg_port_descriptor_t port_descriptor;
	mach_msg_option_t opts;
	mach_msg_priority_t qos;
};

#pragma mark pthread callbacks

static pthread_t
thread_create_at_qos(qos_class_t qos, void * (*function)(void *));
static void
send(mach_port_t send_port, mach_port_t reply_port, mach_port_t msg_port, mach_msg_priority_t qos, mach_msg_option_t options);
static void
enable_kevent(uint64_t *workloop_id, unsigned long long port);
static void
populate_kevent(struct kevent_qos_s *kev, unsigned long long port);

static void
worker_cb(pthread_priority_t __unused priority)
{
	T_FAIL("a worker thread was created");
}

static void
event_cb(void ** __unused events, int * __unused nevents)
{
	T_FAIL("a kevent routine was called instead of workloop");
}

static uint32_t
get_user_promotion_basepri(void)
{
	mach_msg_type_number_t count = THREAD_POLICY_STATE_COUNT;
	struct thread_policy_state thread_policy;
	boolean_t get_default = FALSE;
	mach_port_t thread_port = pthread_mach_thread_np(pthread_self());

	kern_return_t kr = thread_policy_get(thread_port, THREAD_POLICY_STATE,
	    (thread_policy_t)&thread_policy, &count, &get_default);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "thread_policy_get");
	return thread_policy.thps_user_promotion_basepri;
}

#define EXPECT_QOS_EQ(qos, ...) do { \
	        if ((qos) == QOS_CLASS_USER_INTERACTIVE) { \
	                T_EXPECT_EFFECTIVE_QOS_EQ(QOS_CLASS_USER_INITIATED, __VA_ARGS__); \
	                T_EXPECT_EQ(47u, get_user_promotion_basepri(), __VA_ARGS__); \
	        } else { \
	                T_EXPECT_EFFECTIVE_QOS_EQ(qos, __VA_ARGS__); \
	        } \
	} while (0)

#define EXPECT_TEST_MSG(_ke)  do { \
	        struct kevent_qos_s *ke = _ke; \
	        mach_msg_header_t *hdr = (mach_msg_header_t *)ke->ext[0]; \
	        T_ASSERT_NOTNULL(hdr, "has a message"); \
	        T_ASSERT_EQ(hdr->msgh_size, (uint32_t)sizeof(struct test_msg), "of the right size"); \
	        struct test_msg *tmsg = (struct test_msg *)hdr; \
	        if (tmsg->opts & MACH_SEND_PROPAGATE_QOS) { \
	                T_EXPECT_EQ(tmsg->qos, ((uint32_t)(ke->ext[2] >> 32)), \
	                                "propagation works"); \
	        } \
	} while (0)

  
                                                                         
                                        
   
static void
workloop_cb_test_intransit(uint64_t *workloop_id __unused, void **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_intransit called. "
	    "Will wait for %d seconds to make sure client enqueues the sync msg \n",
	    2 * RECV_TIMEOUT_SECS);

	EXPECT_TEST_MSG(*eventslist);

	                                                                               
	sleep(2 * RECV_TIMEOUT_SECS);

	                                         
	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	                                                                 
	EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	*events = 0;
	T_END;
}

  
                                                                  
   
static void
workloop_cb_test_sync_send(uint64_t *workloop_id __unused, void **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send called");

	EXPECT_TEST_MSG(*eventslist);

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	                                                                 
	EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	*events = 0;
	T_END;
}

  
                                                                                   
                                                           
   
static void
workloop_cb_test_sync_send_and_enable(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	unsigned override_priority;
	unsigned reenable_priority;

	T_LOG("Workloop handler workloop_cb_test_sync_send_and_enable called");

	EXPECT_TEST_MSG(*eventslist);

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	                                                                 
	EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	                                            
	override_priority = get_user_promotion_basepri();

	                      
	struct kevent_qos_s *kev = *eventslist;
	enable_kevent(workloop_id, kev->ident);

	  
                                                                                   
                                 
    
	reenable_priority = get_user_promotion_basepri();
	T_EXPECT_LT(reenable_priority, override_priority,
	    "thread's current override priority %d should be less than override priority prior to enabling knote %d",
	    reenable_priority, override_priority);

	*events = 0;
	T_END;
}

  
                                                                        
                                                                                               
   
static void
workloop_cb_test_sync_send_and_enable_handoff(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	unsigned override_priority;
	int error;

	T_LOG("Workloop handler workloop_cb_test_sync_send_and_enable_handoff called");

	EXPECT_TEST_MSG(*eventslist);

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	                                                                 
	EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "dispatch_source event handler QoS should be %s",
	    g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	                                            
	override_priority = get_user_promotion_basepri();

	struct kevent_qos_s *kev = *eventslist;
	mach_msg_header_t *hdr = (mach_msg_header_t *)kev->ext[0];

	                     
	struct kevent_qos_s handoff_kev = {
		.filter = EVFILT_WORKLOOP,
		.ident = hdr->msgh_remote_port,
		.flags = EV_ADD | EV_DISABLE,
		.fflags = 0x80000000,
	};

	error = kevent_id(*workloop_id, &handoff_kev, 1, &handoff_kev, 1, NULL,
	    NULL, KEVENT_FLAG_WORKLOOP | KEVENT_FLAG_ERROR_EVENTS | KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(error, "kevent_id");
	T_ASSERT_EQ(0, error, "Handed off the sync IPC");

	                      
	enable_kevent(workloop_id, kev->ident);

	  
                                               
    
	EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "dispatch_source event handler QoS should still be %s",
	    g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	*events = 0;
	T_END;
}

  
                                                                                             
                                                            
   
static int send_two_sync_handler_called = 0;
static void
workloop_cb_test_send_two_sync(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_send_two_sync called for %d time", send_two_sync_handler_called + 1);

	EXPECT_TEST_MSG(*eventslist);

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_LOG("Number of events received is %d\n", *events);

	if (send_two_sync_handler_called == 0) {
		                                                                 
		EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
		    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

		                                         
		struct kevent_qos_s *kev = *eventslist;
		uint64_t port = kev->ident;
		populate_kevent(kev, port);
		*events = 1;
	} else {
		EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE],
		    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_BEFORE_OVERRIDE]);
		*events = 0;
		T_END;
	}
	send_two_sync_handler_called++;
}

  
                                                                        
                                                                       
   
static boolean_t two_send_and_destroy_test_passed = FALSE;
static int two_send_and_destroy_handler = 0;
static void
workloop_cb_test_two_send_and_destroy(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist __unused, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_two_send_and_destroy called %d times", two_send_and_destroy_handler + 1);

	EXPECT_TEST_MSG(*eventslist);

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	if (two_send_and_destroy_handler == 0) {
		                                             
		sleep(RECV_TIMEOUT_SECS);

		                                                                 
		EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
		    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

		sleep(SEND_TIMEOUT_SECS);

		                                                                    
		EXPECT_QOS_EQ(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE],
		    "dispatch_source event handler QoS should be %s", g_expected_qos_name[ENV_QOS_BEFORE_OVERRIDE]);

		two_send_and_destroy_test_passed = TRUE;
	} else {
		if (two_send_and_destroy_test_passed) {
			T_END;
		}
	}

	                                          
	struct kevent_qos_s *kev = *eventslist;
	uint64_t port = kev->ident;
	populate_kevent(kev, port);
	*events = 1;
	two_send_and_destroy_handler++;
	T_LOG("Handler returning \n");
}

static mach_port_type_t
get_reply_port(struct kevent_qos_s *kev)
{
	mach_msg_header_t *hdr;
	mach_port_t reply_port;
	mach_port_type_t type;
	kern_return_t kr;

	hdr = (void*)kev->ext[0];
	T_QUIET; T_ASSERT_NOTNULL(hdr, "msg hdr");

	reply_port = hdr->msgh_remote_port;
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(reply_port), "reply port valid");
	kr = mach_port_type(mach_task_self(), reply_port, &type);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_port_type");
	T_QUIET; T_ASSERT_TRUE(type & MACH_PORT_TYPE_SEND_ONCE, "send once received");

	return reply_port;
}

static void
send_reply(mach_port_t reply_port)
{
	kern_return_t kr;

	struct {
		mach_msg_header_t header;
	} send_msg = {
		.header = {
			.msgh_remote_port = reply_port,
			.msgh_local_port  = MACH_PORT_NULL,
			.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, 0),
			.msgh_id          = 0x100,
			.msgh_size        = sizeof(send_msg),
		},
	};

	kr = mach_msg(&(send_msg.header),
	    MACH_SEND_MSG,
	    send_msg.header.msgh_size,
	    0,
	    MACH_PORT_NULL,
	    0,
	    0);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "server mach_msg");
}

static void
populate_kevent(struct kevent_qos_s *kev, unsigned long long port)
{
	memset(kev, 0, sizeof(struct kevent_qos_s));
	kev->ident = port;
	kev->filter = EVFILT_MACHPORT;
	kev->flags = EV_ADD | EV_ENABLE | EV_UDATA_SPECIFIC | EV_DISPATCH | EV_VANISHED;
	kev->fflags = (MACH_RCV_MSG | MACH_RCV_VOUCHER | MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY |
	    MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AV) |
	    MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0));
	kev->data = 1;
}

static void
enable_kevent(uint64_t *workloop_id, unsigned long long port)
{
	struct kevent_qos_s kev;
	int error;

	populate_kevent(&kev, port);
	struct kevent_qos_s kev_err[] = {{ 0 }};

	error = kevent_id(*workloop_id, &kev, 1, kev_err, 1, NULL,
	    NULL, KEVENT_FLAG_WORKLOOP | KEVENT_FLAG_ERROR_EVENTS | KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(error, "kevent_id");
}

  
                                                           
   
static void
workloop_cb_test_sync_send_reply(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_reply called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	                
	send_reply(get_reply_port(*eventslist));

	*events = 0;
}

  
                                           
   
static void
workloop_cb_test_sync_send_deallocate(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	mach_port_t reply_port;
	kern_return_t kr;

	T_LOG("Workloop handler workloop_cb_test_sync_send_deallocate called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	reply_port = get_reply_port(*eventslist);

	                     
	kr = mach_port_deallocate(mach_task_self(), reply_port);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_deallocate");

	*events = 0;

	T_LOG("Handler returning \n");
}


  
                                                                                     
   
static void
workloop_cb_test_sync_send_reply_kevent(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_reply_kevent called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT(((*eventslist)->filter), EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;

	                
	send_reply(get_reply_port(kev));

	                      
	enable_kevent(workloop_id, kev->ident);

	*events = 0;

	T_LOG("Handler returning \n");
}

  
                                                                                     
   
static void
workloop_cb_test_sync_send_reply_kevent_pthread(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_reply_kevent_pthread called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;

	                
	send_reply(get_reply_port(kev));

	populate_kevent(kev, kev->ident);

	*events = 1;

	T_LOG("Handler returning \n");
}

  
                                                                         
   
static void
workloop_cb_test_sync_send_kevent_reply(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("workloop handler workloop_cb_test_sync_send_kevent_reply called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;
	mach_port_t reply_port = get_reply_port(*eventslist);

	                      
	enable_kevent(workloop_id, kev->ident);

	                
	send_reply(reply_port);

	*events = 0;

	T_LOG("Handler returning \n");
}

  
                                
   
static void
workloop_cb_test_sync_send_do_nothing(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_do_nothing called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	                

	*events = 0;

	T_LOG("Handler returning \n");
}

  
                                                 
   
static void
workloop_cb_test_sync_send_do_nothing_kevent_pthread(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_do_nothing_kevent_pthread called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;
	populate_kevent(kev, kev->ident);

	*events = 1;

	T_LOG("handler returning \n");
}

  
                         
   
static void
workloop_cb_test_sync_send_do_nothing_exit(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, __unused int *events)
{
	T_LOG("workloop handler workloop_cb_test_sync_send_do_nothing_exit called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	               
	exit(0);
}

  
                    
                                                                  
                                                      
   
static void
workloop_cb_test_sync_send_reply_kevent_reply_kevent(uint64_t *workloop_id __unused, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("Workloop handler workloop_cb_test_sync_send_reply_kevent_reply_kevent called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;

	                
	send_reply(get_reply_port(kev));

	populate_kevent(kev, kev->ident);

	*events = 1;

	T_LOG("Handler returning \n");
}

  
                    
                                                    
                                                      
   
static int workloop_cb_test_sync_send_kevent_reply_reply_kevent_handler_called = 0;
static void
workloop_cb_test_sync_send_kevent_reply_reply_kevent(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("workloop handler workloop_cb_test_sync_send_kevent_reply_reply_kevent called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;
	mach_port_t reply_port = get_reply_port(kev);

	if (workloop_cb_test_sync_send_kevent_reply_reply_kevent_handler_called == 0) {
		workloop_cb_test_sync_send_kevent_reply_reply_kevent_handler_called = 1;

		                      
		enable_kevent(workloop_id, kev->ident);

		                
		send_reply(reply_port);

		*events = 0;
	} else {
		                
		send_reply(reply_port);

		                      
		enable_kevent(workloop_id, kev->ident);

		*events = 0;
	}

	T_LOG("Handler returning \n");
}

  
                    
                                                    
                                                     
   
static void
workloop_cb_test_sync_send_kevent_reply_kevent_reply(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("workloop handler workloop_cb_test_sync_send_kevent_reply_kevent_reply called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;
	mach_port_t reply_port = get_reply_port(kev);

	                      
	enable_kevent(workloop_id, kev->ident);

	                
	send_reply(reply_port);

	*events = 0;
	T_LOG("Handler returning \n");
}

  
                    
                                                   
                                                     
   
static int workloop_cb_test_sync_send_reply_kevent_kevent_reply_handler_called = 0;
static void
workloop_cb_test_sync_send_reply_kevent_kevent_reply(uint64_t *workloop_id, struct kevent_qos_s **eventslist, int *events)
{
	T_LOG("workloop handler workloop_cb_test_sync_send_reply_kevent_kevent_reply called");

	if (geteuid() != 0) {
		T_SKIP("kevent_qos test requires root privileges to run.");
	}

	T_QUIET; T_ASSERT_EQ_INT(*events, 1, "events received");
	T_QUIET; T_ASSERT_EQ_INT((*eventslist)->filter, EVFILT_MACHPORT, "received EVFILT_MACHPORT");

	struct kevent_qos_s *kev = *eventslist;
	mach_port_t reply_port = get_reply_port(kev);

	if (workloop_cb_test_sync_send_reply_kevent_kevent_reply_handler_called == 0) {
		workloop_cb_test_sync_send_reply_kevent_kevent_reply_handler_called = 1;

		                
		send_reply(reply_port);

		populate_kevent(kev, kev->ident);

		*events = 1;
	} else {
		                      
		enable_kevent(workloop_id, kev->ident);
		                
		send_reply(reply_port);

		*events = 0;
	}

	T_LOG("Handler returning \n");
}
#pragma mark Mach receive

#define KEVENT_QOS_SERVICE_NAME "com.apple.xnu.test.kevent_qos"

static mach_port_t
get_server_port(void)
{
	mach_port_t port;
	kern_return_t kr = bootstrap_check_in(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "server bootstrap_check_in");
	return port;
}

static void
env_set_qos(char **env, qos_class_t qos[], const char *qos_name[], const char *wl_function)
{
	int i;
	char *qos_str, *qos_name_str;
	for (i = 0; i < ENV_VAR_QOS; i++) {
		T_QUIET; T_ASSERT_POSIX_SUCCESS(asprintf(&qos_str, "%s=%d", qos_env[i], qos[i]),
		    NULL);
		T_QUIET; T_ASSERT_POSIX_SUCCESS(
			asprintf(&qos_name_str, "%s=%s", qos_name_env[i], qos_name[i]), NULL);
		env[2 * i] = qos_str;
		env[2 * i + 1] = qos_name_str;
	}
	T_QUIET; T_ASSERT_POSIX_SUCCESS(asprintf(&env[2 * i], "%s=%s", wl_function_name, wl_function),
	    NULL);
	env[2 * i + 1] = NULL;
}

static void
environ_get_qos(qos_class_t qos[], const char *qos_name[], const char **wl_function)
{
	char *qos_str;
	char *qos_end;
	int i;

	for (i = 0; i < ENV_VAR_QOS; i++) {
		qos_str = getenv(qos_env[i]);
		T_QUIET; T_ASSERT_NOTNULL(qos_str, "getenv(%s)", qos_env[i]);

		unsigned long qos_l = strtoul(qos_str, &qos_end, 10);
		T_QUIET; T_ASSERT_EQ(*qos_end, '\0', "getenv(%s) = '%s' should be an "
		    "integer", qos_env[i], qos_str);

		T_QUIET; T_ASSERT_LT(qos_l, (unsigned long)100, "getenv(%s) = '%s' should "
		    "be less than 100", qos_env[i], qos_str);

		qos[i] = (qos_class_t)qos_l;
		qos_name[i] = getenv(qos_name_env[i]);
		T_QUIET; T_ASSERT_NOTNULL(qos_name[i], "getenv(%s)", qos_name_env[i]);
	}
	*wl_function = getenv(wl_function_name);
	T_QUIET; T_ASSERT_NOTNULL(*wl_function, "getenv(%s)", wl_function_name);
}

static mach_voucher_t
create_pthpriority_voucher(mach_msg_priority_t qos)
{
	char voucher_buf[sizeof(mach_voucher_attr_recipe_data_t) + sizeof(ipc_pthread_priority_value_t)];

	mach_voucher_t voucher = MACH_PORT_NULL;
	kern_return_t ret;
	ipc_pthread_priority_value_t ipc_pthread_priority_value =
	    (ipc_pthread_priority_value_t)qos;

	mach_voucher_attr_raw_recipe_array_t recipes;
	mach_voucher_attr_raw_recipe_size_t recipe_size = 0;
	mach_voucher_attr_recipe_t recipe =
	    (mach_voucher_attr_recipe_t)&voucher_buf[recipe_size];

	recipe->key = MACH_VOUCHER_ATTR_KEY_PTHPRIORITY;
	recipe->command = MACH_VOUCHER_ATTR_PTHPRIORITY_CREATE;
	recipe->previous_voucher = MACH_VOUCHER_NULL;
	memcpy((char *)&recipe->content[0], &ipc_pthread_priority_value, sizeof(ipc_pthread_priority_value));
	recipe->content_size = sizeof(ipc_pthread_priority_value_t);
	recipe_size += sizeof(mach_voucher_attr_recipe_data_t) + recipe->content_size;

	recipes = (mach_voucher_attr_raw_recipe_array_t)&voucher_buf[0];

	ret = host_create_mach_voucher(mach_host_self(),
	    recipes,
	    recipe_size,
	    &voucher);

	T_QUIET; T_ASSERT_MACH_SUCCESS(ret, "client host_create_mach_voucher");
	return voucher;
}

static void
send(
	mach_port_t send_port,
	mach_port_t reply_port,
	mach_port_t msg_port,
	mach_msg_priority_t qos,
	mach_msg_option_t options)
{
	kern_return_t ret = 0;

	struct test_msg send_msg = {
		.header = {
			.msgh_remote_port = send_port,
			.msgh_local_port  = reply_port,
			.msgh_bits        = MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND,
	    reply_port ? MACH_MSG_TYPE_MAKE_SEND_ONCE : 0,
	    MACH_MSG_TYPE_MOVE_SEND,
	    MACH_MSGH_BITS_COMPLEX),
			.msgh_id          = 0x100,
			.msgh_size        = sizeof(send_msg),
		},
		.body = {
			.msgh_descriptor_count = 1,
		},
		.port_descriptor = {
			.name        = msg_port,
			.disposition = MACH_MSG_TYPE_MOVE_RECEIVE,
			.type        = MACH_MSG_PORT_DESCRIPTOR,
		},
		.opts = options,
	};

	if (msg_port == MACH_PORT_NULL) {
		send_msg.body.msgh_descriptor_count = 0;
	}

	if ((options & MACH_SEND_PROPAGATE_QOS) == 0) {
		send_msg.header.msgh_voucher_port = create_pthpriority_voucher(qos);
		send_msg.qos = qos;
	} else {
		qos_class_t qc;
		int relpri;
		pthread_get_qos_class_np(pthread_self(), &qc, &relpri);
		send_msg.qos = (uint32_t)_pthread_qos_class_encode(qc, relpri, 0);
	}

	mach_msg_option_t send_opts = options;
	if (reply_port) {
		send_opts |= MACH_SEND_SYNC_OVERRIDE;
	}
	send_opts |= MACH_SEND_MSG | MACH_SEND_TIMEOUT | MACH_SEND_OVERRIDE;

	ret = mach_msg(&send_msg.header, send_opts, send_msg.header.msgh_size,
	    0, MACH_PORT_NULL, 10000, qos);

	T_QUIET; T_ASSERT_MACH_SUCCESS(ret, "client mach_msg");
}

static kern_return_t
receive(
	mach_port_t rcv_port,
	mach_port_t notify_port)
{
	kern_return_t ret = 0;

	struct test_msg rcv_msg = {
		.header = {
			.msgh_remote_port = MACH_PORT_NULL,
			.msgh_local_port  = rcv_port,
			.msgh_size        = sizeof(rcv_msg),
		},
	};

	T_LOG("Client: Starting sync receive\n");

	ret = mach_msg(&(rcv_msg.header),
	    MACH_RCV_MSG |
	    MACH_RCV_TIMEOUT |
	    MACH_RCV_SYNC_WAIT,
	    0,
	    rcv_msg.header.msgh_size,
	    rcv_port,
	    SEND_TIMEOUT_SECS * 1000,
	    notify_port);

	return ret;
}

T_HELPER_DECL(qos_get_special_reply_port,
    "Test get_special_reply_port and it's corner cases.")
{
	mach_port_t special_reply_port;
	mach_port_t new_special_reply_port;

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	new_special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(new_special_reply_port), "get_thread_special_reply_port");

	mach_port_destroy(mach_task_self(), special_reply_port);
	mach_port_destroy(mach_task_self(), new_special_reply_port);

	new_special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(new_special_reply_port), "get_thread_special_reply_port");

	T_END;
}

static void *
qos_client_send_to_intransit(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t msg_port;
	mach_port_t special_reply_port;

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                         
	kr = mach_port_allocate(mach_task_self(),
	    MACH_PORT_RIGHT_RECEIVE,
	    &msg_port);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client mach_port_allocate");

	kr = mach_port_insert_right(mach_task_self(),
	    msg_port,
	    msg_port,
	    MACH_MSG_TYPE_MAKE_SEND);

	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client mach_port_insert_right");

	                                                         
	send(qos_send_port, MACH_PORT_NULL, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);

	                                             
	sleep(3);

	                                                                                      
	send(qos_send_port, MACH_PORT_NULL, msg_port,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);

	                                                               
	for (int i = 0; i < 5; i++) {
		send(msg_port, MACH_PORT_NULL, MACH_PORT_NULL,
		    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);
	}

	T_LOG("Sent 5 msgs, now trying to send sync ipc message, which will block with a timeout\n");
	                                                                                              
	send(msg_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);
	T_LOG("Client done sending messages, now waiting for server to end the test");

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

T_HELPER_DECL(qos_client_send_to_intransit_with_thr_pri,
    "Send synchronous messages from a pri thread to an intransit port")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_client_send_to_intransit);
	sleep(HELPER_TIMEOUT_SECS);
}

static pthread_t
thread_create_at_qos(qos_class_t qos, void * (*function)(void *))
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
	pthread_create(&thread, &attr, function, NULL);

	T_LOG("pthread created\n");
	pthread_get_qos_class_np(thread, &qos_thread, NULL);
	T_EXPECT_EQ(qos_thread, (qos_class_t)qos, NULL);
	return thread;
}

static void *
qos_send_and_sync_rcv(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	T_LOG("Client: from created thread\n");
	T_EXPECT_EFFECTIVE_QOS_EQ(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    "pthread QoS should be %s", g_expected_qos_name[ENV_QOS_AFTER_OVERRIDE]);

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                                                
	send(qos_send_port, MACH_PORT_NULL, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_QUEUE_OVERRIDE], 0, 0), 0);

	send(qos_send_port, MACH_PORT_NULL, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_QUEUE_OVERRIDE], 0, 0), 0);

	sleep(SEND_TIMEOUT_SECS);

	                           
	receive(special_reply_port, qos_send_port);

	T_LOG("Client done doing sync rcv, now waiting for server to end the test");
	sleep(SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

static void *
qos_sync_rcv(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	T_LOG("Client: from created thread\n");

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                                                
	send(qos_send_port, MACH_PORT_NULL, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_QUEUE_OVERRIDE], 0, 0), 0);

	sleep(RECV_TIMEOUT_SECS);

	                           
	receive(special_reply_port, qos_send_port);

	T_LOG("Client done doing sync rcv, now waiting for server to end the test");
	sleep(SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

static void
thread_wait_to_block(mach_port_t thread_port)
{
	thread_extended_info_data_t extended_info;
	kern_return_t kr;

	while (1) {
		mach_msg_type_number_t count = THREAD_EXTENDED_INFO_COUNT;
		kr = thread_info(thread_port, THREAD_EXTENDED_INFO,
		    (thread_info_t)&extended_info, &count);

		T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "thread_info");

		if (extended_info.pth_run_state == TH_STATE_WAITING) {
			T_LOG("Target thread blocked\n");
			break;
		}
		thread_switch(thread_port, SWITCH_OPTION_DEPRESS, 0);
	}
}

T_HELPER_DECL(qos_client_send_sync_and_sync_rcv,
    "Send messages and syncronously wait for rcv")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_send_and_sync_rcv);
	sleep(HELPER_TIMEOUT_SECS);
}

T_HELPER_DECL(qos_client_sync_rcv_qos_change,
    "Send messages and syncronously wait for rcv and change qos of waiting thread")
{
	pthread_t rcv_thread;

	rcv_thread = thread_create_at_qos(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], qos_sync_rcv);

	T_LOG("Waiting for %d seconds before changing qos of rcv thread", SEND_TIMEOUT_SECS);
	sleep(SEND_TIMEOUT_SECS);

	                                  
	thread_wait_to_block(pthread_mach_thread_np(rcv_thread));

	                                 
	pthread_override_qos_class_start_np(rcv_thread, g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0);

	sleep(HELPER_TIMEOUT_SECS);
}

static void *
qos_client_send_sync_msg_and_test_link(void *arg)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;
	boolean_t in_effect = FALSE;
	kern_return_t kr;
	unsigned long expected_result = (unsigned long) arg;

	kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	                                    
	kr = mach_sync_ipc_link_monitoring_start(&special_reply_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_sync_ipc_link_monitoring_start");

	                                  
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);

	  
                      
                                                    
              
    
	receive(special_reply_port, qos_send_port);

	                          
	kr = mach_sync_ipc_link_monitoring_stop(special_reply_port, &in_effect);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_sync_ipc_link_monitoring_stop");

	if (!in_effect) {
		T_LOG("Link was broken");
	} else {
		T_LOG("Link correct");
	}

	if (expected_result == 1) {
		T_ASSERT_TRUE(in_effect, "special reply port link after rcv");
	} else {
		T_ASSERT_FALSE(in_effect, "special reply port link after rcv");
	}
	T_END;
}

static void *
qos_client_send_2sync_msg_and_test_link(void *arg)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;
	boolean_t in_effect = FALSE;
	kern_return_t kr;
	unsigned long expected_result = (unsigned long) arg;

	kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	                                    
	kr = mach_sync_ipc_link_monitoring_start(&special_reply_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_sync_ipc_link_monitoring_start");

	                                        
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);

	                        
	kr = receive(special_reply_port, qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "receive");

	                                         
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);

	                        
	kr = receive(special_reply_port, qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "receive");

	                          
	kr = mach_sync_ipc_link_monitoring_stop(special_reply_port, &in_effect);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_sync_ipc_link_monitoring_stop");

	if (!in_effect) {
		T_LOG("Link was broken");
	} else {
		T_LOG("Link correct");
	}

	if (expected_result == 1) {
		T_ASSERT_TRUE(in_effect, "special reply port link after rcv");
	} else {
		T_ASSERT_FALSE(in_effect, "special reply port link after rcv");
	}
	T_END;
}
T_HELPER_DECL(qos_client_send_sync_msg_with_link_check_correct_server,
    "Send sync message, wait for reply and check sync ipc link")
{
	pthread_t thread;
	pthread_attr_t attr;
	unsigned long expected_result = 1;

	pthread_attr_init(&attr);
	pthread_create(&thread, &attr, qos_client_send_sync_msg_and_test_link, (void *)expected_result);

	sleep(HELPER_TIMEOUT_SECS);
}

T_HELPER_DECL(qos_client_send_sync_msg_with_link_check_incorrect_server,
    "Send sync message, wait for reply and check sync ipc link")
{
	pthread_t thread;
	pthread_attr_t attr;
	unsigned long expected_result = 0;

	pthread_attr_init(&attr);
	pthread_create(&thread, &attr, qos_client_send_sync_msg_and_test_link, (void *)expected_result);

	sleep(HELPER_TIMEOUT_SECS);
}

T_HELPER_DECL(qos_client_send_2sync_msg_with_link_check_correct_server,
    "Send sync message, wait for reply and check sync ipc link")
{
	pthread_t thread;
	pthread_attr_t attr;
	unsigned long expected_result = 1;

	pthread_attr_init(&attr);
	pthread_create(&thread, &attr, qos_client_send_2sync_msg_and_test_link, (void *)expected_result);

	sleep(HELPER_TIMEOUT_SECS);
}

T_HELPER_DECL(qos_client_send_2sync_msg_with_link_check_incorrect_server,
    "Send sync message, wait for reply and check sync ipc link")
{
	pthread_t thread;
	pthread_attr_t attr;
	unsigned long expected_result = 0;

	pthread_attr_init(&attr);
	pthread_create(&thread, &attr, qos_client_send_2sync_msg_and_test_link, (void *)expected_result);

	sleep(HELPER_TIMEOUT_SECS);
}

static void *
qos_client_send_sync_msg(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                  
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);

	                        
	receive(special_reply_port, qos_send_port);

	T_LOG("Client done sending messages, now waiting for server to end the test");
	sleep(2 * SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

T_HELPER_DECL(qos_client_send_sync_msg_with_pri,
    "Send sync message and wait for reply")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_client_send_sync_msg);
	sleep(HELPER_TIMEOUT_SECS);
}

static void *
qos_client_send_two_sync_msg_high_qos(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                  
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], 0, 0), 0);

	                        
	receive(special_reply_port, qos_send_port);

	T_LOG("Client done sending messages, now waiting for server to end the test");
	sleep(SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

static void *
qos_client_send_two_sync_msg_low_qos(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	                                  
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);

	                        
	receive(special_reply_port, qos_send_port);

	T_LOG("Client done sending messages, now waiting for server to end the test");
	sleep(SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

T_HELPER_DECL(qos_client_send_two_sync_msg_with_thr_pri,
    "Send messages sync msgs from 2 threads at given thread pri")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_client_send_two_sync_msg_high_qos);
	sleep(INTERMITTENT_TIMEOUT_SEC);
	thread_create_at_qos(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], qos_client_send_two_sync_msg_low_qos);
	sleep(HELPER_TIMEOUT_SECS);
}

static mach_port_t other_thread_reply_port = MACH_PORT_NULL;
static void *
qos_client_destroy_other_threads_port(void *arg __unused)
{
	T_LOG("Waiting 6 seconds before destroying other thread's reply port");
	sleep(SEND_TIMEOUT_SECS);

	T_LOG("Destroying other thread's special reply port ");
	mach_port_destroy(mach_task_self(), other_thread_reply_port);

	T_LOG("Other thread done destroying ");
	sleep(3 * SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

static void *
qos_client_create_sepcial_reply_and_spawn_thread(void *arg __unused)
{
	mach_port_t qos_send_port;
	mach_port_t special_reply_port;

	kern_return_t kr = bootstrap_look_up(bootstrap_port,
	    KEVENT_QOS_SERVICE_NAME, &qos_send_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	special_reply_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(special_reply_port), "get_thread_special_reply_port");

	other_thread_reply_port = special_reply_port;

	                           
	send(qos_send_port, MACH_PORT_NULL, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);

	                               
	send(qos_send_port, special_reply_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0), 0);

	                                                                            
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_client_destroy_other_threads_port);

	                                        
	receive(special_reply_port, qos_send_port);

	sleep(3 * SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

T_HELPER_DECL(qos_client_send_two_msg_and_destroy,
    "Send a message with another threads special reply port while that thread destroys the port")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE], qos_client_create_sepcial_reply_and_spawn_thread);
	sleep(HELPER_TIMEOUT_SECS);
}

static mach_port_t send_complex_connection_port = MACH_PORT_NULL;

static void *
qos_client_send_complex_msg_to_service_port(void *arg __unused)
{
	mach_port_t svc_port, tsr_port, conn_port;
	kern_return_t kr;

	kr = bootstrap_look_up(bootstrap_port, KEVENT_QOS_SERVICE_NAME, &svc_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "client bootstrap_look_up");

	tsr_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(tsr_port), "get_thread_special_reply_port");

	conn_port = send_complex_connection_port;

	T_LOG("Sending to the service port with a sync IPC");
	send(svc_port, tsr_port, conn_port,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0),
	    MACH_SEND_PROPAGATE_QOS);

	receive(tsr_port, svc_port);

	sleep(3 * SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

static void *
qos_client_send_to_connection_then_service_port(void *arg __unused)
{
	mach_port_t tsr_port, conn_port;
	mach_port_options_t opts = {
		.flags = MPO_INSERT_SEND_RIGHT,
	};
	kern_return_t kr;

	kr = mach_port_construct(mach_task_self(), &opts, 0ull, &conn_port);
	T_QUIET; T_ASSERT_MACH_SUCCESS(kr, "mach_port_construct");
	send_complex_connection_port = conn_port;

	tsr_port = thread_get_special_reply_port();
	T_QUIET; T_ASSERT_TRUE(MACH_PORT_VALID(tsr_port), "get_thread_special_reply_port");

	T_LOG("Sending to the connection port with a sync IPC");
	send(conn_port, tsr_port, MACH_PORT_NULL,
	    (uint32_t)_pthread_qos_class_encode(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE], 0, 0),
	    MACH_SEND_PROPAGATE_QOS);

	thread_create_at_qos(g_expected_qos[ENV_QOS_AFTER_OVERRIDE],
	    qos_client_send_complex_msg_to_service_port);

	receive(tsr_port, conn_port);

	sleep(3 * SEND_TIMEOUT_SECS);

	T_ASSERT_FAIL("client timed out");
	return NULL;
}

T_HELPER_DECL(qos_client_send_complex_msg_with_pri,
    "Send a message with several ports causing links")
{
	thread_create_at_qos(g_expected_qos[ENV_QOS_BEFORE_OVERRIDE],
	    qos_client_send_to_connection_then_service_port);
	sleep(HELPER_TIMEOUT_SECS);
}

static void
run_client_server(const char *server_name, const char *client_name, qos_class_t qos[],
    const char *qos_name[], const char *wl_function)
{
	char *env[2 * ENV_VAR_QOS + ENV_VAR_FUNCTION + 1];
	env_set_qos(env, qos, qos_name, wl_function);

	for (int i = 0; i < ENV_VAR_QOS; i++) {
		g_expected_qos[i] = qos[i];
		g_expected_qos_name[i] = qos_name[i];
	}

	dt_helper_t helpers[] = {
		dt_launchd_helper_env("com.apple.xnu.test.kevent_qos.plist",
	    server_name, env),
		dt_fork_helper(client_name)
	};
	dt_run_helpers(helpers, 2, HELPER_TIMEOUT_SECS);
}

#pragma mark Mach receive - kevent_qos

static void
expect_kevent_id_recv(mach_port_t port, qos_class_t qos[], const char *qos_name[], const char *wl_function)
{
	int r;

	                                     
	for (int i = 0; i < ENV_VAR_QOS; i++) {
		g_expected_qos[i] = qos[i];
		g_expected_qos_name[i] = qos_name[i];
	}

	if (strcmp(wl_function, "workloop_cb_test_intransit") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_intransit, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_and_enable") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_and_enable, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_and_enable_handoff") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_and_enable_handoff, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_send_two_sync") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_send_two_sync, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_two_send_and_destroy") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_two_send_and_destroy, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_reply") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_reply, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_deallocate") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_deallocate, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_reply_kevent") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_reply_kevent, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_reply_kevent_pthread") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_reply_kevent_pthread, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_kevent_reply") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_kevent_reply, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_do_nothing") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_do_nothing, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_do_nothing_kevent_pthread") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_do_nothing_kevent_pthread, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_do_nothing_exit") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_do_nothing_exit, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_reply_kevent_reply_kevent") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_reply_kevent_reply_kevent, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_kevent_reply_reply_kevent") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_kevent_reply_reply_kevent, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_kevent_reply_kevent_reply") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_kevent_reply_kevent_reply, 0, 0), NULL);
	} else if (strcmp(wl_function, "workloop_cb_test_sync_send_reply_kevent_kevent_reply") == 0) {
		T_QUIET; T_ASSERT_POSIX_ZERO(_pthread_workqueue_init_with_workloop(
			    worker_cb, event_cb,
			    (pthread_workqueue_function_workloop_t)workloop_cb_test_sync_send_reply_kevent_kevent_reply, 0, 0), NULL);
	} else {
		T_ASSERT_FAIL("no workloop function specified \n");
	}

	struct kevent_qos_s kev[] = {{
					     .ident = port,
					     .filter = EVFILT_MACHPORT,
					     .flags = EV_ADD | EV_UDATA_SPECIFIC | EV_DISPATCH | EV_VANISHED,
					     .fflags = (MACH_RCV_MSG | MACH_RCV_VOUCHER | MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY |
	    MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AV) |
	    MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0)),
					     .data = 1,
					     .qos = (int32_t)_pthread_qos_class_encode(qos[ENV_QOS_QUEUE_OVERRIDE], 0, 0)
				     }};

	struct kevent_qos_s kev_err[] = {{ 0 }};

	                                     
	r = kevent_id(25, kev, 1, kev_err, 1, NULL,
	    NULL, KEVENT_FLAG_WORKLOOP | KEVENT_FLAG_ERROR_EVENTS);

	T_QUIET; T_ASSERT_POSIX_SUCCESS(r, "kevent_id");
	T_QUIET; T_ASSERT_EQ(r, 0, "no errors returned from kevent_id");
	sleep(HELPER_TIMEOUT_SECS);
}

T_HELPER_DECL(server_kevent_id,
    "Reply with the QoS that a dispatch source event handler ran with")
{
	qos_class_t qos[ENV_VAR_QOS];
	const char *qos_name[ENV_VAR_QOS];
	const char *wl_function;
	environ_get_qos(qos, qos_name, &wl_function);

	expect_kevent_id_recv(get_server_port(), qos, qos_name, wl_function);
	sleep(HELPER_TIMEOUT_SECS);
	T_ASSERT_FAIL("should receive a message within %d seconds",
	    RECV_TIMEOUT_SECS);
}

static void *
special_reply_port_thread(void *ctxt)
{
	kern_return_t ret;
	mach_port_t rcv_port = *(mach_port_t *)ctxt;
	struct test_msg rcv_msg = {
		.header = {
			.msgh_remote_port = MACH_PORT_NULL,
			.msgh_local_port  = rcv_port,
			.msgh_size        = sizeof(rcv_msg),
		},
	};

	ret = mach_msg(&rcv_msg.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
	    rcv_msg.header.msgh_size, rcv_port, 1000, MACH_PORT_NULL);

	T_EXPECT_EQ(ret, MACH_RCV_TIMED_OUT, "receive should not panic");

	*(mach_port_t *)ctxt = MACH_PORT_NULL;

	sleep(1);                                  

	ret = mach_msg(&rcv_msg.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
	    rcv_msg.header.msgh_size, rcv_port, 1000, MACH_PORT_NULL);

	T_EXPECT_EQ(ret, MACH_RCV_TIMED_OUT, "receive should not panic");

	T_END;
}

T_DECL(special_reply_port, "basic special reply port robustness checks",
    T_META_RUN_CONCURRENTLY(true))
{
	pthread_t thread;
	mach_port_t srp = thread_get_special_reply_port();

	pthread_create(&thread, NULL, special_reply_port_thread, &srp);

	while (srp) {
		usleep(1000);
	}

	pthread_exit(NULL);
}

#define TEST_QOS(server_name, client_name, name, wl_function_name, qos_bo, qos_bo_name, qos_qo, qos_qo_name, qos_ao, qos_ao_name) \
	T_DECL(server_kevent_id_##name, \
	                "Event delivery at " qos_ao_name " QoS using a kevent_id", \
	                T_META_ASROOT(YES)) \
	{ \
	        qos_class_t qos_array[ENV_VAR_QOS] = {qos_bo, qos_qo, qos_ao};  \
	        const char *qos_name_array[ENV_VAR_QOS] = {qos_bo_name, qos_qo_name, qos_ao_name}; \
	        run_client_server(server_name, client_name, qos_array, qos_name_array, wl_function_name); \
	}
  
                                      
  
                                                                     
                                                                             
   
TEST_QOS("server_kevent_id", "qos_get_special_reply_port", special_reply_port, "workloop_cb_test_intransit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

  
                                                   
  
                                                                               
                                                                      
                     
   
TEST_QOS("server_kevent_id", "qos_client_send_to_intransit_with_thr_pri", transit_IN, "workloop_cb_test_intransit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INITIATED, "user initiated")

  
                                                   
  
                                                                               
                                                                      
                     
   
TEST_QOS("server_kevent_id", "qos_client_send_to_intransit_with_thr_pri", transit_UI, "workloop_cb_test_intransit",
    QOS_CLASS_USER_INITIATED, "user initiated",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                          
  
                                                            
                                                                      
                                                   
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_and_sync_rcv", rcv_IN, "workloop_cb_test_intransit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INITIATED, "user initiated")

  
                                                          
  
                                                            
                                                                      
                                                   
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_and_sync_rcv", rcv_UI, "workloop_cb_test_intransit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INTERACTIVE, "user interactive with 47 basepri promotion")

  
                                                                                       
  
                                                                            
                                                        
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_pri", send_sync_IN, "workloop_cb_test_sync_send",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INITIATED, "user initiated")

  
                                                                                       
  
                                                                            
                                                        
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_pri", send_sync_UI, "workloop_cb_test_sync_send",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                                                                         
  
                                                                         
                                                                               
                                     
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_pri", send_sync_UI_and_enable, "workloop_cb_test_sync_send_and_enable",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                                                          
  
                                                                                 
                                                                                    
   
TEST_QOS("server_kevent_id", "qos_client_send_two_sync_msg_with_thr_pri", send_two_sync_UI, "workloop_cb_test_send_two_sync",
    QOS_CLASS_BACKGROUND, "background",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                                     
  
                                                                      
                                                                     
                                                          
   
TEST_QOS("server_kevent_id", "qos_client_send_two_msg_and_destroy", send_two_UI_and_destroy, "workloop_cb_test_two_send_and_destroy",
    QOS_CLASS_BACKGROUND, "background",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                
  
                                                                              
                                        
   
TEST_QOS("server_kevent_id", "qos_client_send_complex_msg_with_pri", send_complex_sync_UI_and_enable, "workloop_cb_test_sync_send_and_enable",
    QOS_CLASS_USER_INITIATED, "user initiated",
    QOS_CLASS_USER_INITIATED, "user initiated",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                
  
                                                                              
                                        
   
TEST_QOS("server_kevent_id", "qos_client_send_complex_msg_with_pri", send_complex_sync_UI_and_enable_and_handoff, "workloop_cb_test_sync_send_and_enable_handoff",
    QOS_CLASS_USER_INITIATED, "user initiated",
    QOS_CLASS_USER_INITIATED, "user initiated",
    QOS_CLASS_USER_INTERACTIVE, "user initiated with 47 basepri promotion")

  
                                                                   
  
                                                                             
                                                         
   
TEST_QOS("server_kevent_id", "qos_client_sync_rcv_qos_change", qos_change_to_IN, "workloop_cb_test_intransit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_MAINTENANCE, "maintenance",
    QOS_CLASS_USER_INITIATED, "user initiated")

  
               
  
                                                                                          
   
TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_s, "workloop_cb_test_sync_send_reply",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_d, "workloop_cb_test_sync_send_deallocate",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_sk, "workloop_cb_test_sync_send_reply_kevent",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_skp, "workloop_cb_test_sync_send_reply_kevent_pthread",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_incorrect_server", send_sync_link_incorrect_server_ks, "workloop_cb_test_sync_send_kevent_reply",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_n, "workloop_cb_test_sync_send_do_nothing",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_incorrect_server", send_sync_link_incorrect_server_kp, "workloop_cb_test_sync_send_do_nothing_kevent_pthread",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_sync_msg_with_link_check_correct_server", send_sync_link_correct_server_e, "workloop_cb_test_sync_send_do_nothing_exit",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

  
               
  
                                                                            
   
TEST_QOS("server_kevent_id", "qos_client_send_2sync_msg_with_link_check_correct_server", send_2sync_link_correct_server_sksk, "workloop_cb_test_sync_send_reply_kevent_reply_kevent",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_2sync_msg_with_link_check_incorrect_server", send_2sync_link_incorrect_server_kssk, "workloop_cb_test_sync_send_kevent_reply_reply_kevent",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_2sync_msg_with_link_check_incorrect_server", send_2sync_link_incorrect_server_ksks, "workloop_cb_test_sync_send_kevent_reply_kevent_reply",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")

TEST_QOS("server_kevent_id", "qos_client_send_2sync_msg_with_link_check_incorrect_server", send_2sync_link_incorrect_server_skks, "workloop_cb_test_sync_send_reply_kevent_kevent_reply",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default",
    QOS_CLASS_DEFAULT, "default")
