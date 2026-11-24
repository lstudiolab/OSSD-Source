  
                
                                                                                       
   
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <darwintest.h>

                                                               
#define RLIMIT_NLIMITS 9

                                                           
static const char *RESOURCE_STRING[] = {
	"RLIMIT_CPU",                                    
	"RLIMIT_FSIZE",                                  
	"RLIMIT_DATA",                                   
	"RLIMIT_STACK",                                  
	"RLIMIT_CORE",                                   
	"RLIMIT_AS/RSS",                                 
	                                       
	"RLIMIT_MEMLOCK",                                
	"RLIMIT_NPROC",                                  
	"RLIMIT_NOFILE"                                  
};

                                                  
#define LIMIT_DIFF 64

                
#define SOFT_LIMIT 0
#define HARD_LIMIT 1

                                     
#define LOWER 0
#define RAISE 1

static struct rlimit orig_rlimit[RLIMIT_NLIMITS];

                                                         
static rlim_t maxfilesperproc;
static size_t maxfilesperproc_size = sizeof(maxfilesperproc);

                                                        
static rlim_t maxfiles;
static size_t maxfiles_size = sizeof(maxfiles);

                                                                     
static rlim_t maxprocperuid;
static size_t maxprocperuid_size = sizeof(maxprocperuid);

                                                                    
static rlim_t maxproc;
static size_t maxproc_size = sizeof(maxproc);

static bool superuser = FALSE;

static int
get_initial_rlimits(void)
{
	int err = -1;
	int i;

	for (i = 0; i < RLIMIT_NLIMITS; i++) {
		err = getrlimit(i, &orig_rlimit[i]);
		T_QUIET; T_EXPECT_EQ(0, err, "getrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) %s", RESOURCE_STRING[i], orig_rlimit[i].rlim_cur, orig_rlimit[i].rlim_max, err == 0 ? "" : strerror(errno));
	}
	return err;
}

static void
print_rlimits(bool initial_limits)
{
	int err;
	int i;

	for (i = 0; i < RLIMIT_NLIMITS; i++) {
		struct rlimit lim;

		if (initial_limits) {
			lim = orig_rlimit[i];
		} else {
			err = getrlimit(i, &lim);
			T_QUIET; T_EXPECT_EQ(0, err, "getrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) %s", RESOURCE_STRING[i], lim.rlim_cur, lim.rlim_max, err == 0 ? "" : strerror(errno));
		}
		T_LOG("%35s soft: 0x%16llx hard 0x%16llx", RESOURCE_STRING[i], lim.rlim_cur, lim.rlim_max);
	}
}

  
                                                                 
  
                                    
                     
                          
   
static void
change_rlimits(int limit_type, rlim_t amount, int action)
{
	int err = -1;
	int i;

	for (i = 0; i < RLIMIT_NLIMITS; i++) {
		struct rlimit newlim;                     
		struct rlimit verifylim;                  
		bool expect_failure = FALSE;
		int expect_errno = 0;

		                                  
		err = getrlimit(i, &newlim);
		T_EXPECT_EQ(0, err, "getrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) %s", RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, err == 0 ? "" : strerror(errno));

		                         
		if (limit_type == SOFT_LIMIT) {
			if (action == RAISE) {
				                                                                                                                   
				if (newlim.rlim_cur + amount > newlim.rlim_max) {
					expect_failure = TRUE;
					expect_errno = EINVAL;
				}
				newlim.rlim_cur += amount;
			} else if (action == LOWER) {
				if (newlim.rlim_cur == 0) {
					                                                           
				} else {
					newlim.rlim_cur -= amount;
				}
			} else {
				T_FAIL("Unknown action on soft limit: %d", action);
			}
		}
		                         
		else if (limit_type == HARD_LIMIT) {
			if (action == RAISE) {
				newlim.rlim_max += amount;

				                                                                                                             
				expect_failure = TRUE;
				expect_errno = EPERM;
			} else if (action == LOWER) {
				if (newlim.rlim_max == 0) {
					                                                                                      
				} else {
					newlim.rlim_max -= amount;
				}
				                                                                                         
				if (newlim.rlim_cur > newlim.rlim_max) {
					newlim.rlim_cur = newlim.rlim_max;
				}
			} else {
				T_FAIL("Unknown action on hard limit: %d", action);
			}
		}
		                                 
		else {
			T_FAIL("Unknown limit type: %d", limit_type);
		}

		                                               
		err = setrlimit(i, &newlim);

		if (expect_failure) {
			                                          
			T_EXPECT_EQ(-1, err, "setrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) failed as expected: %s", RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, strerror(errno));
			T_EXPECT_EQ(expect_errno, errno, "Expect errno %d, errno returned %d", expect_errno, errno);
			continue;
		} else {
			T_EXPECT_EQ(0, err, "setrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) %s", RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, err == 0 ? "" : strerror(errno));
		}

		                                                          
		err = getrlimit(i, &verifylim);
		T_EXPECT_EQ(0, err, "getrlimit(%15s, soft: 0x%16llx, hard 0x%16llx) %s", RESOURCE_STRING[i], verifylim.rlim_cur, verifylim.rlim_max, err == 0 ? "" : strerror(errno));

		                                                                                                                                                
		if (i == RLIMIT_NOFILE && limit_type == HARD_LIMIT && newlim.rlim_max > maxfilesperproc) {
			if (newlim.rlim_cur != verifylim.rlim_cur ||
			    maxfilesperproc != verifylim.rlim_max) {
				T_FAIL("Mismatch limit values %s despite a successful setrlimit call (setrlimit'd soft 0x%16llx hard 0x%16llx but getrlimit'd soft 0x%16llx hard 0x%16llx)",
				    RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, verifylim.rlim_cur, verifylim.rlim_max);
			}
		}
		                                                                                                                                                
		else if (i == RLIMIT_NPROC && newlim.rlim_max > maxprocperuid) {
			if (newlim.rlim_cur != verifylim.rlim_cur ||
			    maxprocperuid != verifylim.rlim_max) {
				T_FAIL("Mismatch limit values %s despite a successful setrlimit call (setrlimit'd soft 0x%16llx hard 0x%16llx but getrlimit'd soft 0x%16llx hard 0x%16llx)",
				    RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, verifylim.rlim_cur, verifylim.rlim_max);
			}
		} else {
			if (newlim.rlim_cur != verifylim.rlim_cur ||
			    newlim.rlim_max != verifylim.rlim_max) {
				T_FAIL("Mismatch limit values %s despite a successful setrlimit call (setrlimit'd soft 0x%16llx hard 0x%16llx but getrlimit'd soft 0x%16llx hard 0x%16llx)",
				    RESOURCE_STRING[i], newlim.rlim_cur, newlim.rlim_max, verifylim.rlim_cur, verifylim.rlim_max);
			}
		}
	}
}

T_DECL(proc_rlimit,
    "Test basic functionalities of the getrlimit and setrlimit")
{
	int err;
	struct rlimit lim;

	T_SETUPBEGIN;

	if (geteuid() == 0) {
		superuser = TRUE;
		T_SKIP("This test should not be run as super user.");
	}

	                                                                                                          
	err = sysctlbyname("kern.maxfilesperproc", &maxfilesperproc, &maxfilesperproc_size, NULL, 0);
	T_EXPECT_EQ_INT(0, err, "maxfilesperproc: %llu", maxfilesperproc);

	err = sysctlbyname("kern.maxprocperuid", &maxprocperuid, &maxprocperuid_size, NULL, 0);
	T_EXPECT_EQ_INT(0, err, "maxprocperuid: %llu", maxprocperuid);

	                                                                                                                                                   
	err = sysctlbyname("kern.maxfiles", &maxfiles, &maxfiles_size, NULL, 0);
	T_EXPECT_EQ_INT(0, err, "maxfiles: %llu", maxfiles);

	err = sysctlbyname("kern.maxproc", &maxproc, &maxproc_size, NULL, 0);
	T_EXPECT_EQ_INT(0, err, "maxproc: %llu", maxproc);

	                                                                                                    
	err = get_initial_rlimits();
	T_EXPECT_EQ(0, err, "Obtained initial resource values.");

	                                                                                      
	T_LOG("Resource limits before the test:");
	print_rlimits(TRUE);

	T_SETUPEND;

	                                           
	T_LOG("---------Lowering soft limits by 0x%x---------:\n", LIMIT_DIFF);
	change_rlimits(SOFT_LIMIT, LIMIT_DIFF, LOWER);

	                                                  
	T_LOG("---------Raising soft limits by 0x%x---------:\n", LIMIT_DIFF);
	change_rlimits(SOFT_LIMIT, LIMIT_DIFF, RAISE);

	                       
	T_LOG("---------Lowering hard limits by 0x%x---------:", LIMIT_DIFF);
	change_rlimits(HARD_LIMIT, LIMIT_DIFF, LOWER);

	                                                                                                     
	T_LOG("---------Attempting to raised soft limits by 0x%x to exceed hard limits---------:", LIMIT_DIFF);
	change_rlimits(SOFT_LIMIT, LIMIT_DIFF, RAISE);

	                                                                               
	T_LOG("---------Attempting to raise hard limits by 0x%x---------:", LIMIT_DIFF);
	change_rlimits(HARD_LIMIT, LIMIT_DIFF, RAISE);

	                                               
	T_LOG("---------Accessing a non-existing resource---------:");
	err = getrlimit(RLIMIT_NLIMITS + 1, &lim);
	T_EXPECT_EQ(-1, err, "Expect getrlimit to fail when accessing a non-existing resource: %s\n", strerror(errno));
	T_EXPECT_EQ(EINVAL, errno, "Expect errno %d, errno returned %d", EINVAL, errno);

	err = setrlimit(RLIMIT_NLIMITS + 1, &lim);
	T_EXPECT_EQ(-1, err, "Expect setrlimit to fail when accessing a non-existing resource: %s\n", strerror(errno));
	T_EXPECT_EQ(EINVAL, errno, "Expect errno %d, errno returned %d", EINVAL, errno);

	T_LOG("Resource limits after the test:");
	print_rlimits(FALSE);
}
