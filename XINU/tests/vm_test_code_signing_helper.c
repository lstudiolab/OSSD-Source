#include <TargetConditionals.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __has_include(<ptrauth.h>)
#include <ptrauth.h>
#endif

#include <sys/mman.h>
#include <sys/syslimits.h>

char *cmdname;

int
main(
	int argc,
	char *argv[])
{
	uint32_t page_size;
	void *page;
	int ch;
	int opt_interactive;

	cmdname = argv[0];

	opt_interactive = 0;
	while ((ch = getopt(argc, argv, "i")) != -1) {
		switch (ch) {
		case 'i':
			opt_interactive = 1;
			break;
		case '?':
		default:
			fprintf(stdout,
			    "Usage: %s [-i]\n"
			    "\t-i: interactive\n",
			    cmdname);
			exit(1);
		}
	}

	page_size = getpagesize();
	page = mmap(NULL, page_size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_SHARED, -1, 0);
	if (!page) {
		fprintf(stderr, "%s:%d mmap() error %d (%s)\n",
		    cmdname, __LINE__,
		    errno, strerror(errno));
		exit(1);
	}
	if (opt_interactive) {
		fprintf(stdout, "allocated page at %p\n",
		    page);
	}

	if (mprotect(page, page_size, PROT_READ | PROT_WRITE) != 0) {
		fprintf(stderr, "%s:%d mprotect(RW) error %d (%s)\n",
		    cmdname, __LINE__,
		    errno, strerror(errno));
		exit(1);
	}

#if __arm64__
	                        
	char chdir_code[] =  {
		0x90, 0x01, 0x80, 0xd2,                    
		0x01, 0x10, 0x00, 0xd4,                
		0xc0, 0x03, 0x5f, 0xd6,       
	};
#elif __arm__
	                        
	char chdir_code[] = {
		0x0c, 0xc0, 0xa0, 0xe3,                   
		0x80, 0x00, 0x00, 0xef,                
		0x1e, 0xff, 0x2f, 0xe1,         
	};
#elif __x86_64__
	                         
	char chdir_code[] = {
		0xb8, 0x0c, 0x00, 0x00, 0x02,                             
		0x49, 0x89, 0xca,                                   
		0x0f, 0x05,                               
		0xc3,                                  
	};
#elif __i386__
	                       
	char chdir_code[] = {
		0x90,         
		0xc3,          
	};
#endif
	memcpy(page, chdir_code, sizeof chdir_code);

	if (opt_interactive) {
		fprintf(stdout,
		    "changed page protection to r/w and copied code at %p\n",
		    page);
		fprintf(stdout, "pausing...\n");
		fflush(stdout);
		getchar();
	}

	if (mprotect(page, page_size, PROT_READ | PROT_EXEC) != 0) {
		fprintf(stderr, "%s:%d mprotect(RX) error %d (%s)\n",
		    cmdname, __LINE__,
		    errno, strerror(errno));
		exit(1);
	}

	if (opt_interactive) {
		fprintf(stdout,
		    "changed page protection to r/x at %p\n",
		    page);
		fprintf(stdout, "pausing...\n");
		fflush(stdout);
		getchar();
	}

	char origdir[PATH_MAX];
	getcwd(origdir, sizeof(origdir) - 1);

	chdir("/");
	if (opt_interactive) {
		fprintf(stdout, "cwd before = %s\n", getwd(NULL));
	}

	void (*mychdir)(char *) = page;
#if __has_feature(ptrauth_calls)
	mychdir = ptrauth_sign_unauthenticated(mychdir, ptrauth_key_function_pointer, 0);
#endif
	mychdir(getenv("HOME"));
	if (opt_interactive) {
		fprintf(stdout, "cwd after = %s\n", getwd(NULL));
		fprintf(stdout, "pausing...\n");
		fflush(stdout);
		getchar();
	}

	fprintf(stdout, "%s: WARNING: unsigned code was executed\n",
	    cmdname);

#if !TARGET_OS_OSX
	                                      
	fprintf(stdout, "%s: FAIL\n", cmdname);
	exit(1);
#else                     
	                                                                     
	fprintf(stdout, "%s: SUCCESS\n", cmdname);
	exit(0);
#endif                     
}
