#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <darwintest.h>
#include <darwintest_utils.h>

T_GLOBAL_META(
	T_META_NAMESPACE("xnu.ipc"),
	T_META_RUN_CONCURRENTLY(TRUE));

#define TMP_FILE_NAME "lockf_uaf_poc_70587638"

static int fd0, fd1, fd2;

static int other_failure = 0;
static int other_failure_line = 0;

static pthread_t thr0, thr1, thr2;

#define RECORD_ERROR(err) do {                  \
	if (other_failure_line == 0) {          \
	        other_failure = (err);          \
	        other_failure_line = __LINE__;  \
	}                                       \
} while (0);
#define MYCHECK_ERRNO(res) do {                 \
	if ((res) < 0) {                        \
	        RECORD_ERROR((errno));          \
	        return NULL;                    \
	}                                                                       \
} while (0)
#define MYCHECK_POSIX(res) do {                 \
	if ((res) != 0) {                       \
	        RECORD_ERROR((res));            \
	        return NULL;                    \
	}                                       \
} while (0)

#define CHECK_OTHER_FAILURE() do {                      \
	int my_other_failure = other_failure;           \
	int my_other_failure_line = other_failure_line; \
	my_other_failure_line = 0;                      \
	T_QUIET;                                        \
	T_ASSERT_EQ(my_other_failure_line, 0,           \
	    "Other failure %d at line %d",              \
	    my_other_failure, my_other_failure_line);   \
} while (0);

static void *
thr2_func(void *arg)
{
	int res;

	  
                                                                                               
                                                 
    
	(void) sleep(1u);

	  
                                                                                           
                                                                                            
                                                               
    
	res = flock(fd2, LOCK_SH);
	MYCHECK_ERRNO(res);

	  
                                                                                             
                                                                                        
                                                                                             
                                                                                           
                                                                                             
                                                                                             
                                    
    
	res = flock(fd0, LOCK_UN);
	MYCHECK_ERRNO(res);

	return arg;
}

static void *
thr1_func(void *arg)
{
	int res;
	  
                                                                                              
                                                                                            
                                                                                                
                    
    
	(void) sleep(1u);

	                                                           
	res = pthread_create(&thr2, 0, thr2_func, 0);
	MYCHECK_POSIX(res);

	                                                         
	res = flock(fd1, LOCK_EX);
	MYCHECK_ERRNO(res);

	return arg;
}

static void *
thr0_func(void *arg)
{
	int res;

	                                 
	res = flock(fd0, LOCK_SH);
	MYCHECK_ERRNO(res);

	                                                           
	res = pthread_create(&thr1, 0, thr1_func, 0);
	MYCHECK_POSIX(res);

	                                                         
	res = flock(fd2, LOCK_EX);
	MYCHECK_ERRNO(res);

	return arg;
}

static void
sigpipe_handler(int sig __unused, siginfo_t *sa __unused, void *ign __unused)
{
	return;
}

T_DECL(lockf_uaf_poc_70587638,
    "Do a sequence which caused lf_setlock() to free something still in-use.",
    T_META_ASROOT(true), T_META_CHECK_LEAKS(false))
{
	int res;
	struct sigaction sa;

	T_SETUPBEGIN;

	(void) sigfillset(&sa.sa_mask);
	sa.sa_sigaction = sigpipe_handler;
	sa.sa_flags = SA_SIGINFO;
	T_ASSERT_POSIX_SUCCESS(sigaction(SIGPIPE, &sa, NULL), "sigaction(SIGPIPE)");

	                                                                                
	T_ASSERT_POSIX_SUCCESS(
		fd0 = open(TMP_FILE_NAME, O_RDONLY | O_CREAT, 0666),
		"open(\""TMP_FILE_NAME"\", O_RDONLY|O_CREAT, 0666)");
	T_ASSERT_POSIX_SUCCESS(
		fd1 = open(TMP_FILE_NAME, O_RDONLY, 0666),
		"open(\""TMP_FILE_NAME"\", O_RDONLY, 0666)");
	T_ASSERT_POSIX_SUCCESS(
		fd2 = open(TMP_FILE_NAME, 0, 0666),
		"open(\""TMP_FILE_NAME"\", O_RDONLY, 0666)");
	T_SETUPEND;

	  
                                                                                                
                                                                                                
                                                               
    
	res = pthread_create(&thr0, 0, thr0_func, 0);
	T_ASSERT_POSIX_ZERO(res, "pthread_create thread 0");

	  
                                                                                        
                                                                                        
                                                               
    
	res = pthread_join(thr0, NULL);
	T_ASSERT_POSIX_ZERO(res, "pthread_join thread 0");

	CHECK_OTHER_FAILURE();

	                                                                                     
	res = pthread_kill(thr1, SIGPIPE);
	T_ASSERT_POSIX_ZERO(res, "pthread_kill thread 1");

	CHECK_OTHER_FAILURE();

	  
                                                                      
                                                                         
                                                                     
                                                               
                                                                  
                                                                      
    
	res = pthread_join(thr1, NULL);
	T_ASSERT_POSIX_ZERO(res, "pthread_join thread 1");

	CHECK_OTHER_FAILURE();

	T_PASS("lockf_uaf_poc_70587638");
}
