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

#include "test_utils.h"

T_GLOBAL_META(
	T_META_NAMESPACE("xnu.vm"),
	T_META_CHECK_LEAKS(false)
	);

extern char **environ;

  
                                                                
                                                                  
                                                                   
                                                 
  
                                                                         
                                                              
                                                                                 
  
                                                               
                                                                       
                                     
   

                   
#define TEST_ALLOWED     0x1
#define TEST_NOT_ALLOWED 0x2

  
                                                          
                                          
                                                 
   
#define MEMORYSTATUS_VM_MAP_FORK_ALLOWED        0x100000000ul
#define MEMORYSTATUS_VM_MAP_FORK_NOT_ALLOWED 0x200000000ul

  
                                                         
                                                         
                                                 
   
static char testpath[PATH_MAX];
static uint32_t testpath_size = sizeof(testpath);
#define LIMIT_DELTA_MB 5                               
#define MEGABYTE        (1024 * 1024)

  
                                                                    
   
enum child_exits {
	NORMAL_EXIT = 0,
	NO_MEMSIZE_ARG,
	INVALID_MEMSIZE,
	MALLOC_FAILED,
	NUM_CHILD_EXIT
};
static char *child_exit_why[] = {
	"normal exit",
	"no memsize argument to child",
	"invalid memsize argument to child",
	"malloc() failed",
};

  
                                                                    
                                                          
   
static void
set_memorystatus_vm_map_fork_pidwatch(pid_t pid)
{
	uint64_t new_value = (uint64_t)pid;
	size_t new_len = sizeof(new_value);
	int err;

	err = sysctlbyname("kern.memorystatus_vm_map_fork_pidwatch", NULL, NULL, &new_value, new_len);
	T_QUIET;
	T_ASSERT_POSIX_SUCCESS(err, "set sysctlbyname(kern.memorystatus_vm_map_fork_pidwatch...) failed");
	return;
}

static uint64_t
get_memorystatus_vm_map_fork_pidwatch()
{
	uint64_t value = 0;
	size_t val_len = sizeof(value);
	int err;

	err = sysctlbyname("kern.memorystatus_vm_map_fork_pidwatch", &value, &val_len, NULL, 0);
	T_QUIET;
	T_ASSERT_POSIX_SUCCESS(err, "get sysctlbyname(kern.memorystatus_vm_map_fork_pidwatch...) failed");

	return value;
}

  
                                                                                  
                                                                          
   
static void
wait_for_free_mem(int need_mb)
{
	int64_t         memsize;
	int             memorystatus_level;
	size_t          size;
	int64_t         avail;
	int             err;
	int             try;

	  
                                       
    
	size = sizeof(memsize);
	err = sysctlbyname("hw.memsize", &memsize, &size, NULL, 0);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "sysctlbyname(hw.memsize...) failed");

	  
                                                               
    
	try = 1;
	for (;;) {
		  
                                                                                                
                                                                                         
     
		size = sizeof(memorystatus_level);
		if (sysctlbyname("kern.memorystatus_level", &memorystatus_level, &size, NULL, 0) != 0) {
			return;
		}
		T_QUIET; T_ASSERT_LE(memorystatus_level, 100, "memorystatus_level too high");
		T_QUIET; T_ASSERT_GT(memorystatus_level, 0, "memorystatus_level negative");

		  
                                                                                                  
     
		avail = MAX(0, (memsize * (memorystatus_level - 15)) / 100);

		  
                                                            
     
		if ((int64_t)need_mb * MEGABYTE < avail) {
			return;
		}

		  
                                                                              
     
		if (try-- == 0) {
			break;
		}
		T_LOG("Need %d MB, only %d MB available. sleeping 5 seconds for more to free. memorystatus_level %d",
		    need_mb, (int)(avail / MEGABYTE), memorystatus_level);
		sleep(5);
	}
	T_SKIP("Needed %d MB, but only %d MB available. Skipping test to avoid jetsam issues.",
	    need_mb, (int)(avail / MEGABYTE));
}


  
                                                                     
                                                                     
                                             
                                                                         
                                                                    
   
static pid_t
spawn_child_process(
	char * const executable,
	char * const memlimit,
	short flags,
	int priority,
	int active_limit_mb,
	int inactive_limit_mb)
{
	posix_spawnattr_t spawn_attrs;
	int err;
	pid_t child_pid;
	char * const argv_child[] = { executable, "-n", "child_process", "--", memlimit, NULL };

	err = posix_spawnattr_init(&spawn_attrs);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "  posix_spawnattr_init() failed");

	err = posix_spawnattr_setflags(&spawn_attrs, POSIX_SPAWN_START_SUSPENDED);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "  posix_spawnattr_setflags() failed");

	err = posix_spawnattr_setjetsam_ext(&spawn_attrs, flags, priority, active_limit_mb, inactive_limit_mb);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "  posix_spawnattr_setjetsam_ext() failed");

	err = posix_spawn(&child_pid, executable, NULL, &spawn_attrs, argv_child, environ);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "  posix_spawn() failed");

	return child_pid;
}


  
                                                                                   
                                                                         
   
static void
test_child_process(pid_t child_pid, int *status, struct rusage *ru)
{
	int err = 0;
	pid_t got_pid;

	T_LOG("  continuing child[%d]\n", child_pid);

	err = kill(child_pid, SIGCONT);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(err, "  kill(%d, SIGCONT) failed", child_pid);

	T_LOG("  waiting for child[%d] to exit", child_pid);

	got_pid = wait4(child_pid, status, 0, ru);
	T_QUIET; T_ASSERT_EQ(child_pid, got_pid, "  wait4(%d, ...) returned %d", child_pid, got_pid);
}

  
                                                                                               
                                                   
   
T_HELPER_DECL(child_process, "child allocates memory to failure")
{
#define BYTESPERALLOC   MEGABYTE
#define BYTESINEXCESS   (2 * MEGABYTE)                       
	char *limit;
	long limit_mb = 0;
	long max_bytes_to_munch, bytes_remaining, bytes_this_munch;
	void *mem = NULL;

	  
                                                                       
                                                                       
    
	if (argc != 1) {
		exit(NO_MEMSIZE_ARG);
	}

	limit = argv[0];
	errno = 0;
	limit_mb = strtol(limit, NULL, 10);
	if (errno != 0 || limit_mb <= 0) {
		exit(INVALID_MEMSIZE);
	}

	                                         
	max_bytes_to_munch = limit_mb * MEGABYTE;
	max_bytes_to_munch += BYTESINEXCESS;

	for (bytes_remaining = max_bytes_to_munch; bytes_remaining > 0; bytes_remaining -= bytes_this_munch) {
		bytes_this_munch = MIN(bytes_remaining, BYTESPERALLOC);

		mem = malloc((size_t)bytes_this_munch);
		if (mem == NULL) {
			exit(MALLOC_FAILED);
		}
		arc4random_buf(mem, (size_t)bytes_this_munch);
	}

	                                                   
	exit(NORMAL_EXIT);
}


  
                    
   
static void
memorystatus_vm_map_fork_parent(int test_variant)
{
	int             max_task_pmem = 0;         
	size_t          size = 0;
	int             active_limit_mb = 0;
	int             inactive_limit_mb = 0;
	short           flags = 0;
	char            memlimit_str[16];
	pid_t           child_pid;
	int             child_status;
	uint64_t        kernel_pidwatch_val;
	uint64_t        expected_pidwatch_val;
	int             ret;
	struct rusage   ru;
	enum child_exits exit_val;

	  
                                                      
                                                     
    
	if (!is_development_kernel()) {
		T_SKIP("Can't test on release kernel");
	}

	  
                                                               
    
	size = sizeof(max_task_pmem);
	(void)sysctlbyname("kern.max_task_pmem", &max_task_pmem, &size, NULL, 0);
	if (max_task_pmem <= 0) {
		max_task_pmem = 0;
	}

	if (test_variant == TEST_ALLOWED) {
		  
                                                                    
     
		if (max_task_pmem / 4 - LIMIT_DELTA_MB <= 0) {
			active_limit_mb = LIMIT_DELTA_MB;
		} else {
			active_limit_mb = max_task_pmem / 4 - LIMIT_DELTA_MB;
		}
		expected_pidwatch_val = MEMORYSTATUS_VM_MAP_FORK_ALLOWED;
	} else {                       
		  
                                                                    
     
		active_limit_mb = (max_task_pmem / 4) + LIMIT_DELTA_MB;
		if (max_task_pmem == 0) {
			expected_pidwatch_val = MEMORYSTATUS_VM_MAP_FORK_ALLOWED;
		} else {
			expected_pidwatch_val = MEMORYSTATUS_VM_MAP_FORK_NOT_ALLOWED;
		}
	}
	inactive_limit_mb = active_limit_mb;
	T_LOG("using limit of %d Meg", active_limit_mb);

	  
                                                       
                                                        
                                                         
                                                       
                          
    
	wait_for_free_mem(active_limit_mb);

#if TARGET_OS_OSX
	  
                                               
    
	expected_pidwatch_val = MEMORYSTATUS_VM_MAP_FORK_ALLOWED;
#endif

	  
                                                            
    
	memset(memlimit_str, 0, sizeof(memlimit_str));
	(void)sprintf(memlimit_str, "%d", active_limit_mb);

	ret = _NSGetExecutablePath(testpath, &testpath_size);
	T_QUIET; T_ASSERT_POSIX_SUCCESS(ret, "_NSGetExecutablePath(%s, ...)", testpath);

	  
                                                                                 
    
	child_pid = spawn_child_process(testpath, memlimit_str, flags,
	    JETSAM_PRIORITY_FOREGROUND, active_limit_mb, inactive_limit_mb);

	expected_pidwatch_val |= (uint64_t)child_pid;

	  
                                                                    
    
	T_LOG("  spawned child_pid[%d] with memlimit %s (%d)MB\n",
	    child_pid, memlimit_str, active_limit_mb);

	  
                                                    
    
	(void)set_memorystatus_vm_map_fork_pidwatch((pid_t)0);
	(void)set_memorystatus_vm_map_fork_pidwatch(child_pid);

	  
                                                
    
	test_child_process(child_pid, &child_status, &ru);
	T_LOG("Child exited with max_rss of %ld", ru.ru_maxrss);

	  
                                                                  
                                     
    
	kernel_pidwatch_val = get_memorystatus_vm_map_fork_pidwatch();
	(void)set_memorystatus_vm_map_fork_pidwatch((pid_t)0);

	  
                                                      
    
	if (!WIFEXITED(child_status)) {
		if (WIFSIGNALED(child_status)) {
			                                         
			if (WTERMSIG(child_status) == SIGKILL) {
				T_LOG("Child appears to have been a jetsam victim");
			}
			T_SKIP("Child terminated by signal %d test result invalid", WTERMSIG(child_status));
		}
		T_SKIP("child did not exit normally (status=%d) test result invalid", child_status);
	}

	  
                                                                       
    
	exit_val = (enum child_exits)WEXITSTATUS(child_status);
	T_QUIET; T_ASSERT_EQ(exit_val, NORMAL_EXIT, "child exit due to: %s",
	    (0 < exit_val && exit_val < NUM_CHILD_EXIT) ? child_exit_why[exit_val] : "unknown");

	  
                                                                                     
    
	if (kernel_pidwatch_val == -1ull) {
		T_SKIP("corpse generation was aborted by kernel");
	}

	  
                                                                                               
    
	T_QUIET; T_ASSERT_NE_ULLONG(kernel_pidwatch_val, (uint64_t)child_pid, "child didn't trigger corpse generation");

	T_EXPECT_EQ(kernel_pidwatch_val, expected_pidwatch_val, "kernel value 0x%llx - expected 0x%llx",
	    kernel_pidwatch_val, expected_pidwatch_val);
}

  
                                                                                                         
  
                                                                                                              
                                                                                               
   
T_DECL(memorystatus_vm_map_fork_test_not_allowed, "test that corpse generation was not allowed", T_META_ASROOT(true))
{
	memorystatus_vm_map_fork_parent(TEST_NOT_ALLOWED);
}

T_DECL(memorystatus_vm_map_fork_test_allowed, "test corpse generation allowed", T_META_ASROOT(true))
{
	memorystatus_vm_map_fork_parent(TEST_ALLOWED);
}
