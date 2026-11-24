/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

                                                                          
                                   
                                                                            
                            
                                                                         
                                                                          
                                                                               
                                                    
                                                                  
                                                                                 
                                                                          
                                                                                         
                                                                        
                                                                             
                                                                              
                                                                                
                                                                                 
                                                                             
                                                                             
                          
                                                                              
                                                                               
                                                 
                             
   

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <mach/mach_time.h>
#include <libkern/OSAtomic.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/sysctl.h>

typedef struct {
	OSSpinLock tlock;
	uintptr_t taddr;
	unsigned tstate;
	uint64_t tctime;
} cpage;

cpage *parray;

#define ARENASIZE (1024)
#define NTHREADS (4)
#define PAGE_LINGER_TIME (2000000)
#define MAX_THREADS (512)
#define MYSYS (215)
#define CONSISTENCY(...) fprintf(stderr, __VA_ARGS__ );

unsigned arenasize = ARENASIZE, mapping_size;
uint64_t page_linger_time = PAGE_LINGER_TIME;
enum arenastates {MTOUCHED = 1, UNMAPPED = 2, MAPPED = 4, WP =8};
enum syscaction {MDOMAP = 1, MDOUNMAP = 2, MDOCHECK = 4};
enum ttypes {OBSERVER = 1, LOOPER = 2};
bool trymode = true;
bool all_stop = false;
bool stop_on_failure = false;
bool reuse_addrs = true;
bool dosyscall = false;

pid_t cpid;
int sleepus;

pthread_t threads[MAX_THREADS];
uint32_t roles[MAX_THREADS];

void
usage(char **a)
{
	exit(1);
}

void
set_enable(int val)
{
	int mib[6];
	size_t needed;

	mib[0] = CTL_KERN;
	mib[1] = KERN_KDEBUG;
	mib[2] = KERN_KDENABLE;
	mib[3] = val;
	mib[4] = 0;
	mib[5] = 0;

	if (sysctl(mib, 4, NULL, &needed, NULL, 0) < 0) {
		printf("trace facility failure, KERN_KDENABLE\n");
	}
}

void
initialize_arena_element(int i)
{
	__unused int sysret;
	void *hint = reuse_addrs ? (void *)0x1000 : NULL;
	parray[i].taddr = (uintptr_t)mmap(hint, mapping_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

	if (parray[i].taddr == (uintptr_t)MAP_FAILED) {
		perror("mmap");
		exit(2);
	}

#if     !defined(__LP64__)
	uint32_t pattern = parray[i].taddr;
	pattern |= cpid & 0xFFF;
                                                                    
                                                                         
                                                  

#else
	uint64_t pattern = parray[i].taddr;
	pattern |= (cpid & 0xFFF);
                                                         
#endif

	uint64_t val =  (*(uintptr_t *)parray[i].taddr);

	if (val != 0) {
		CONSISTENCY("Mismatch, actual: 0x%llx, expected: 0x%llx\n", (unsigned long long)val, 0ULL);
		if (stop_on_failure) {
			set_enable(0);
			exit(5);
		}
	}
	for (int k = 0; k < (mapping_size >> PAGE_SHIFT); k++) {
		*(uintptr_t *)(parray[i].taddr + k * PAGE_SIZE) = pattern;
	}

	parray[i].tctime = mach_absolute_time();
	parray[i].tstate = MTOUCHED;

	if (dosyscall) {
		sysret = syscall(MYSYS, MDOMAP, parray[i].taddr, pattern, i, mapping_size);
	}
}

void
initialize_arena(void)
{
	for (int i = 0; i < arenasize; i++) {
		initialize_arena_element(i);
	}
}

void *
tlbexerciser(void *targs)
{
	uint32_t role = *(uint32_t *)targs;
	__unused int sysret;
	printf("Starting thread %p, role: %u\n", pthread_self(), role);

	for (;;) {
		for (int i = 0; i < arenasize; i++) {
			if (all_stop) {
				return NULL;
			}

			if (trymode) {
				if (OSSpinLockTry(&parray[i].tlock) == false) {
					continue;
				}
			} else {
				OSSpinLockLock(&parray[i].tlock);
			}

			if (parray[i].tstate != UNMAPPED) {
				uintptr_t ad;
				ad = parray[i].taddr | (cpid & 0xFFF);
				uintptr_t val = *(uintptr_t *)parray[i].taddr;

				if (val != ad) {
					if (stop_on_failure) {
						all_stop = true;
					}
					syscall(180, 0x71BC0000, (ad >> 32), (ad & ~0), 0, 0, 0);
					CONSISTENCY("Mismatch, actual: 0x%llx, expected: 0x%llx\n", (unsigned long long)val, (unsigned long long)ad);
					if (stop_on_failure) {
						set_enable(0);
						exit(5);
					}
				}

				if (dosyscall) {
					sysret = syscall(MYSYS, MDOCHECK, parray[i].taddr, ad, i, 0);
				}

				if ((role != OBSERVER) && ((mach_absolute_time() - parray[i].tctime) > page_linger_time)) {
					parray[i].tstate = UNMAPPED;
					if (munmap((void *)parray[i].taddr, mapping_size) != 0) {
						perror("munmap");
					}

					if (dosyscall) {
						sysret = syscall(MYSYS, MDOUNMAP, parray[i].taddr, ad, i, mapping_size);
					}
				}
			} else {
				if (role != OBSERVER) {
					initialize_arena_element(i);
				}
			}

			parray[i].tlock = 0;         

			if (sleepus) {
				usleep(sleepus);
			}
		}
	}

	return NULL;
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	int arg;
	unsigned nthreads = NTHREADS;

	mapping_size = PAGE_SIZE;

	while ((arg = getopt(argc, argv, "l:t:h:s:p:z:fry")) != -1) {
		switch (arg) {
		case 'l':
			page_linger_time = strtoull(optarg, NULL, 0);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 's':
			arenasize = atoi(optarg);                             
			                                                     
			break;
		case 'f':
			stop_on_failure = true;
			break;
		case 'r':
			reuse_addrs = false;
			break;
		case 'p':
			sleepus = atoi(optarg);
			break;
		case 'y':
			dosyscall = true;
			break;
		case 'z':
			mapping_size = atoi(optarg) * PAGE_SIZE;
			break;
		case 'h':
			usage(argv);
		}
	}

	if (optind != argc) {
		usage(argv);
	}

	printf("page_linger_time: 0x%llx, nthreads: %u, arenasize: %u sleepus: %d reuse_addrs: %u, stop_on_failure: %u, dosyscall: %u, mappingsize: 0x%x\n", page_linger_time, nthreads, arenasize, sleepus, reuse_addrs, (unsigned) stop_on_failure, dosyscall, mapping_size);

	parray = calloc(arenasize, sizeof(cpage));
	cpid = getpid();

	initialize_arena();

	for (int dex = 0; dex < nthreads; dex++) {
		roles[dex] = LOOPER;
		if (dex == 0) {
			roles[dex] = OBSERVER;
		}
		int result = pthread_create(&threads[dex], NULL, tlbexerciser, &roles[dex]);
		if (result) {
			printf("pthread_create: %d starting worker thread; aborting.\n", result);
			return result;
		}
	}

	for (int dex = 0; dex < nthreads; dex++) {
		void *rtn;
		int result = pthread_join(threads[dex], &rtn);

		if (result) {
			printf("pthread_join(): %d, aborting\n", result);
			return result;
		}

		if (rtn) {
			printf("***Aborting on worker error\n");
			exit(1);
		}
	}
	return 0;
}
