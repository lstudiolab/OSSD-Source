/*
 * Copyright (c) 2015-2018 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */


#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <syslog.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <darwintest.h>
#include <darwintest_utils.h>
#include <darwintest_multiprocess.h>

#define TEST_TIMEOUT    10

  
                                                                              
                                                                 
   
void
handle_sigttin(int signal)
{
	return;
}

  
                                                                          
                                                                                
           
   
int
get_new_session_and_terminal_and_fork_child_to_read(char *pty_name)
{
	int sock_fd[2];
	int pty_fd;
	pid_t pid;
	char buf[10];

	  
                                                                         
          
    
	T_ASSERT_POSIX_SUCCESS(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fd),
	    NULL);

	  
                                                                  
                   
    
	T_ASSERT_POSIX_SUCCESS(setsid(), NULL);

	                                                              
	T_ASSERT_POSIX_SUCCESS(pty_fd = open(pty_name, O_RDWR), NULL);

	T_ASSERT_POSIX_SUCCESS(pid = fork(), NULL);

	if (pid == 0) {            
		int pty_fd_child;
		char buf[10];

		T_ASSERT_POSIX_SUCCESS(close(sock_fd[0]), NULL);
		T_ASSERT_POSIX_SUCCESS(close(pty_fd), NULL);

		                                            
		T_ASSERT_POSIX_SUCCESS(setpgid(0, 0), NULL);

		T_ASSERT_POSIX_SUCCESS(pty_fd_child = open(pty_name, O_RDWR),
		    NULL);

		                                                     
		write(sock_fd[1], "done", sizeof("done"));

		                                                               
		read(sock_fd[1], buf, sizeof(buf));

		  
                                                           
                               
    
                                                                  
                                                                
                                
    
                                                                 
                                                                
            
     
		signal(SIGTTIN, handle_sigttin);
		(void)read(pty_fd_child, buf, sizeof(buf));
		  
                                                               
                                     
     
		exit(0);
	}

	T_ASSERT_POSIX_SUCCESS(close(sock_fd[1]), NULL);

	                                                                   
	T_ASSERT_POSIX_SUCCESS(read(sock_fd[0], buf, sizeof(buf)), NULL);

	  
                                                 
   
                                                                       
   
                             
                                                            
                       
   
                                                                   
                                               
    

	T_ASSERT_POSIX_SUCCESS(tcsetpgrp(pty_fd, pid), NULL);

	                                                                       
	T_ASSERT_POSIX_SUCCESS(write(sock_fd[0], "done", sizeof("done")), NULL);

	  
                                                                
   
                                                                    
    
	sleep(1);

	  
                                                          
                                                                         
                                     
   
                                                                     
                                      
    
	signal(SIGTTOU, SIG_IGN);
	T_ASSERT_POSIX_SUCCESS(tcsetpgrp(pty_fd, getpid()), NULL);

	return 0;
}

  
                                                                               
                                                                                 
                                                             
                                                                          
                           
                                                                             
                                            
   
void
run_test(int do_revoke)
{
	int master_fd;
	char *slave_pty;
	pid_t pid;

	T_WITH_ERRNO;
	T_QUIET;

	T_SETUPBEGIN;

	slave_pty = NULL;
	T_ASSERT_POSIX_SUCCESS(master_fd = posix_openpt(O_RDWR | O_NOCTTY),
	    NULL);
	(void)fcntl(master_fd, F_SETFL, O_NONBLOCK);
	T_ASSERT_POSIX_SUCCESS(grantpt(master_fd), NULL);
	T_ASSERT_POSIX_SUCCESS(unlockpt(master_fd), NULL);
	slave_pty = ptsname(master_fd);
	T_ASSERT_NOTNULL(slave_pty, NULL);
	T_LOG("slave pty is %s\n", slave_pty);

	T_SETUPEND;

	  
                                                                        
                                                                        
                                                                    
                            
    
	T_ASSERT_POSIX_SUCCESS(pid = fork(), NULL);

	if (pid == 0) {            
		T_ASSERT_POSIX_SUCCESS(close(master_fd), NULL);
		get_new_session_and_terminal_and_fork_child_to_read(slave_pty);

		  
                                                            
                                                               
                                                          
                                                      
                                                                 
                                                              
                                                                 
                                            
     
		if (do_revoke) {
			                                                  
			T_ASSERT_POSIX_SUCCESS(revoke(slave_pty), NULL);
		}
		  
                                                              
                                                               
                                                                
                                      
                                                                 
                                                          
     
		exit(0);
	}

	int status;
	int sig;

	dt_waitpid(pid, &status, &sig, 0);
	if (sig) {
		T_FAIL("Test failed because child received signal %s\n",
		    strsignal(sig));
	} else if (status) {
		T_FAIL("Test failed because child exited with status %d\n",
		    status);
	} else {
		T_PASS("test_passed\n");
	}
	  
                                                                       
                            
    
}


                                                                     
T_HELPER_DECL(create_new_session_and_exit, "create_new_session_and_exit") {
	run_test(0);
}

T_DECL(tty_exit_bgread_hang_test, "test for background read hang on ttys with proc exit")
{
	dt_helper_t helpers[1];

	helpers[0] = dt_fork_helper("create_new_session_and_exit");
	dt_run_helpers(helpers, 1, TEST_TIMEOUT);
}
                                                                       

                                                                       
T_HELPER_DECL(create_new_session_and_revoke_terminal, "create_new_session_and_revoke_terminal") {
	run_test(1);
}

T_DECL(tty_revoke_bgread_hang_test, "test for background read hang on ttys with revoke")
{
	dt_helper_t helpers[1];

	helpers[0] = dt_fork_helper("create_new_session_and_revoke_terminal");
	dt_run_helpers(helpers, 1, TEST_TIMEOUT);
}
                                                                       
