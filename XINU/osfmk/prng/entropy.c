/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#include <libkern/crypto/sha2.h>
#include <libkern/crypto/crypto_internal.h>
#include <os/atomic_private.h>
#include <kern/assert.h>
#include <kern/percpu.h>
#include <kern/zalloc.h>
#include <kern/lock_group.h>
#include <kern/locks.h>
#include <kern/misc_protos.h>
#include <pexpert/pexpert.h>
#include <prng/entropy.h>
#include <machine/machine_routines.h>
#include <libkern/section_keywords.h>
#include <sys/cdefs.h>

                                                          
#define ENTROPY_MAX_SAMPLE_COUNT (2048)

                                          
typedef struct entropy_cpu_data {
	                                    
	entropy_sample_t samples[ENTROPY_MAX_SAMPLE_COUNT];

	                                                                  
	                                                               
	                                                           
	                                                             
	                     
	uint32_t _Atomic sample_count;
} entropy_cpu_data_t;

                                                                      
                                                                  
typedef struct entropy_health_test {
	                                                                
	                                                             
	                   
	entropy_sample_t init_observation;

	                                                                 
	                                
	uint64_t observation_count;

	                                                               
	                                                            
	entropy_health_stats_t *stats;
} entropy_health_test_t;

typedef enum health_test_result {
	health_test_failure,
	health_test_success
} health_test_result_t;

                                                                   
                                                  
typedef struct entropy_data {
	                                                             
	                                                             
	                                                      
	SHA256_CTX sha256_ctx;

	                                                              
	                                                             
	                                                        
	                                                          
	            
	bool sha256_ctx_init;

	                                                                 
	                                                              
	                                                              
	                                                              
	                                  
	uint64_t total_sample_count;

	                                                                 
	                                                             
	                                                              
	                                                               
	                                                             
	                                                          
	                        
	uint64_t startup_sample_count;

	                                                                 
	                                                                 
	                                                              
	                                               
	uint64_t read_sample_count;

	                                                
	lck_grp_t lock_group;

	                                                                  
	                                                          
	lck_mtx_t mutex;

	                                       
	entropy_health_test_t repetition_count_test;

	                                          
	entropy_health_test_t adaptive_proportion_test;
} entropy_data_t;

static entropy_cpu_data_t PERCPU_DATA(entropy_cpu_data);

int entropy_health_startup_done;
entropy_health_stats_t entropy_health_rct_stats;
entropy_health_stats_t entropy_health_apt_stats;

static entropy_data_t entropy_data = {
	.repetition_count_test = {
		.init_observation = -1,
		.stats = &entropy_health_rct_stats,
	},
	.adaptive_proportion_test = {
		.init_observation = -1,
		.stats = &entropy_health_apt_stats,
	},
};

__security_const_late entropy_sample_t *entropy_analysis_buffer;
__security_const_late uint32_t entropy_analysis_buffer_size;
static __security_const_late uint32_t entropy_analysis_max_sample_count;
static uint32_t entropy_analysis_sample_count;

__startup_func
static void
entropy_analysis_init(uint32_t sample_count)
{
	entropy_analysis_max_sample_count = sample_count;
	entropy_analysis_buffer_size = sample_count * sizeof(entropy_sample_t);
	entropy_analysis_buffer = zalloc_permanent(entropy_analysis_buffer_size, ZALIGN(entropy_sample_t));
}

__startup_func
void
entropy_init(void)
{
	lck_grp_init(&entropy_data.lock_group, "entropy-data", LCK_GRP_ATTR_NULL);
	lck_mtx_init(&entropy_data.mutex, &entropy_data.lock_group, LCK_ATTR_NULL);

	                                                                 
	                                                                  
	                                                                 
	                                                            
	uint32_t sample_count = 0;
	if (__improbable(PE_parse_boot_argn("entropy-analysis-sample-count", &sample_count, sizeof(sample_count)))) {
		entropy_analysis_init(sample_count);
	} else if (__improbable(PE_parse_boot_argn("ebsz", &sample_count, sizeof(sample_count)))) {
		entropy_analysis_init(sample_count);
	}
}

void
entropy_collect(void)
{
	                                                                
	                                        

	entropy_cpu_data_t *e = PERCPU_GET(entropy_cpu_data);

	uint32_t sample_count = os_atomic_load(&e->sample_count, relaxed);

	assert(sample_count <= ENTROPY_MAX_SAMPLE_COUNT);

	                                                            
	           
	if (sample_count == ENTROPY_MAX_SAMPLE_COUNT) {
		return;
	}

	e->samples[sample_count] = (entropy_sample_t)ml_get_timebase_entropy();

	                                                             
	                                                            
	                                   
	(void)os_atomic_cmpxchg(&e->sample_count, sample_count, sample_count + 1, release);
}

                                                                
                                                                
                                                      
  
                                                                
                                                                  
             
                                            
                                                            
               
  
                                                                 
                                                                   
                                                                
                                                                      
            

static health_test_result_t
add_observation(entropy_health_test_t *t, uint64_t bound)
{
	t->observation_count += 1;
	t->stats->max_observation_count = MAX(t->stats->max_observation_count, (uint32_t)t->observation_count);
	if (__improbable(t->observation_count >= bound)) {
		t->stats->failure_count += 1;
		return health_test_failure;
	}

	return health_test_success;
}

static void
reset_test(entropy_health_test_t *t, entropy_sample_t observation)
{
	t->stats->reset_count += 1;
	t->init_observation = observation;
	t->observation_count = 1;
	t->stats->max_observation_count = MAX(t->stats->max_observation_count, (uint32_t)t->observation_count);
}

                              
  
                                                                     
                  
  
                             
  
             
        
                                     
  
                                                                      
                                                     

#define REPETITION_COUNT_BOUND (129)

static health_test_result_t
repetition_count_test(entropy_sample_t observation)
{
	entropy_health_test_t *t = &entropy_data.repetition_count_test;

	if (t->init_observation == observation) {
		return add_observation(t, REPETITION_COUNT_BOUND);
	} else {
		reset_test(t, observation);
	}

	return health_test_success;
}

                                 
  
                                                                      
  
                                                                    
                                                                  
           
  
                                                                      
                                                        
  
               
                                                  
  
                                                     
  
                
  
           
  
                                

#define ADAPTIVE_PROPORTION_BOUND (400)
#define ADAPTIVE_PROPORTION_WINDOW (512)

                                                              
static_assert(__builtin_popcount(ADAPTIVE_PROPORTION_WINDOW) == 1);
#define ADAPTIVE_PROPORTION_INDEX_MASK (ADAPTIVE_PROPORTION_WINDOW - 1)

static health_test_result_t
adaptive_proportion_test(entropy_sample_t observation, uint32_t offset)
{
	entropy_health_test_t *t = &entropy_data.adaptive_proportion_test;

	                                                               
	                                                               
	                                                              
	               
	uint32_t index = (entropy_data.total_sample_count + offset) & ADAPTIVE_PROPORTION_INDEX_MASK;

	if (index == 0) {
		reset_test(t, observation);
	} else if (t->init_observation == observation) {
		return add_observation(t, ADAPTIVE_PROPORTION_BOUND);
	}

	return health_test_success;
}

static health_test_result_t
entropy_health_test(uint32_t sample_count, entropy_sample_t *samples)
{
	health_test_result_t result = health_test_success;

	for (uint32_t i = 0; i < sample_count; i += 1) {
		                                                              
		                                                  
		entropy_sample_t observation = samples[i] & 0xff;

		if (__improbable(repetition_count_test(observation) == health_test_failure)) {
			result = health_test_failure;
		}

		if (__improbable(adaptive_proportion_test(observation, i) == health_test_failure)) {
			result = health_test_failure;
		}
	}

	return result;
}

static void
entropy_analysis_store(uint32_t sample_count, entropy_sample_t *samples)
{
	lck_mtx_assert(&entropy_data.mutex, LCK_MTX_ASSERT_OWNED);

	sample_count = MIN(sample_count, (entropy_analysis_max_sample_count - entropy_analysis_sample_count));
	if (sample_count == 0) {
		return;
	}

	size_t size = sample_count * sizeof(samples[0]);
	memcpy(&entropy_analysis_buffer[entropy_analysis_sample_count], samples, size);
	entropy_analysis_sample_count += sample_count;
}

int32_t
entropy_provide(size_t *entropy_size, void *entropy, __unused void *arg)
{
#if (DEVELOPMENT || DEBUG)
	if (*entropy_size < SHA256_DIGEST_LENGTH) {
		panic("[entropy_provide] recipient entropy buffer is too small\n");
	}
#endif

	int32_t sample_count = 0;
	*entropy_size = 0;

	                                                                  
	                                                           
	           
	if (__improbable(!g_crypto_funcs)) {
		return sample_count;
	}

	                                                               
	                                                                
	                                           
	if (!lck_mtx_try_lock(&entropy_data.mutex)) {
		return sample_count;
	}

	                                                                  
	                                                          
	                                     
	if (__improbable(!entropy_data.sha256_ctx_init)) {
		SHA256_Init(&entropy_data.sha256_ctx);
		entropy_data.sha256_ctx_init = true;
	}

	health_test_result_t health_test_result = health_test_success;

	                                       
	percpu_foreach(e, entropy_cpu_data) {
		                                                           
		                                                              
		                   
		uint32_t cpu_sample_count = os_atomic_load(&e->sample_count, acquire);

		assert(cpu_sample_count <= ENTROPY_MAX_SAMPLE_COUNT);

		                                                          
		                                                     
		                   
		if (__improbable(entropy_health_test(cpu_sample_count, e->samples) == health_test_failure)) {
			health_test_result = health_test_failure;
		}

		                                                           
		                          
		entropy_data.total_sample_count += cpu_sample_count;
		SHA256_Update(&entropy_data.sha256_ctx, e->samples, cpu_sample_count * sizeof(e->samples[0]));

		                                                                  
		                                            
		if (__improbable(entropy_analysis_buffer)) {
			entropy_analysis_store(cpu_sample_count, e->samples);
		}

		                                                            
		os_atomic_store(&e->sample_count, 0, relaxed);
	}

	                                  
	  
	                                                                  
	                                                                 
	if (__improbable(health_test_result == health_test_failure)) {
		entropy_health_startup_done = 0;
		entropy_data.startup_sample_count = entropy_data.total_sample_count;
		entropy_data.read_sample_count = entropy_data.total_sample_count;
		sample_count = -1;
		goto out;
	}

	                                                                  
	                                                                 
	                                     
	if (__improbable((entropy_data.total_sample_count - entropy_data.startup_sample_count) < 1024)) {
		goto out;
	}

	entropy_health_startup_done = 1;

	                                                            
	int32_t n = (int32_t)(entropy_data.total_sample_count - entropy_data.read_sample_count);

	                                                           
	                                                              
	if (n < 32) {
		goto out;
	}

	SHA256_Final(entropy, &entropy_data.sha256_ctx);
	SHA256_Init(&entropy_data.sha256_ctx);
	entropy_data.read_sample_count = entropy_data.total_sample_count;

	sample_count = n;
	*entropy_size = SHA256_DIGEST_LENGTH;

out:
	lck_mtx_unlock(&entropy_data.mutex);

	return sample_count;
}
