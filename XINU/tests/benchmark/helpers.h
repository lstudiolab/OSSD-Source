#ifndef BENCHMARK_PERF_HELPERS_H
#define BENCHMARK_PERF_HELPERS_H

  
                                                      
   
#include <inttypes.h>
#include <time.h>
#include <stdbool.h>

  
                                     
   
unsigned char *mmap_buffer(size_t size);
  
                                 
                     
   
uint64_t timespec_difference_us(const struct timespec* a, const struct timespec* b);
  
                                                           
                                                                        
                                                                             
                            
  
                                        
   
void benchmark_log(bool verbose, const char *restrict fmt, ...) __attribute__((format(printf, 2, 3)));

static const uint64_t kNumMicrosecondsInSecond = 1000UL * 1000;
static const uint64_t kNumNanosecondsInMicrosecond = 1000UL;
static const uint64_t kNumNanosecondsInSecond = kNumNanosecondsInMicrosecond * kNumMicrosecondsInSecond;
                                                
#define current_timestamp_ns() (clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW));

int get_ncpu(void);

#endif                                         
