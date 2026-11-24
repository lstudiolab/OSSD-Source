#include <unistd.h>
#include <stdio.h>

#include <darwintest.h>
#include <darwintest_utils.h>

T_GLOBAL_META(T_META_RUN_CONCURRENTLY(false));

  
                                                
                             
                                       
                                              
   
static int
my_system(const char *command)
{
	pid_t pid;
	int status = 0;
	int signal = 0;
	int err;
	const char *argv[] = {
		"/bin/sh",
		"-c",
		command,
		NULL
	};

	if (dt_launch_tool(&pid, (char **)(void *)argv, FALSE, NULL, NULL)) {
		return -1;
	}

	err = dt_waitpid(pid, &status, &signal, 30);
	if (err) {
		return 0;
	}

	return signal;
}


  
                                            
  
             
                               
             
  
              
                                 
              
  
                                                                                           
  
                                                                                              
   
T_DECL(text_corruption_recovery, "test detection/recovery of text corruption",
    T_META_IGNORECRASHES(".*text_corruption_helper.*"),
    T_META_ASROOT(true))
{
	int ret;

	ret = my_system("./text_corruption_helper foo");
	T_QUIET; T_ASSERT_EQ(ret, 0, "First call of foo");

	ret = my_system("./text_corruption_helper Xfoo");
	T_QUIET; T_ASSERT_EQ(ret, SIGILL, "Call of corrupted foo");

	ret = my_system("./text_corruption_helper foo");
	T_QUIET; T_ASSERT_EQ(ret, 0, "Fixed call of foo");

	ret = my_system("./text_corruption_helper atan");
	T_QUIET; T_ASSERT_EQ(ret, 0, "First call of atan");

	ret = my_system("./text_corruption_helper Xatan");
	T_QUIET; T_ASSERT_EQ(ret, SIGILL, "Call of corrupted atan");

	ret = my_system("./text_corruption_helper atan");
	T_QUIET; T_ASSERT_EQ(ret, 0, "Fixed call of atan");
}
