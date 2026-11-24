#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/clock_types.h>
#include <sys/timex.h>
#include <mach/mach.h>
#include <darwintest.h>
#include <darwintest_utils.h>


#define DAY 86400                 
#define ERROR 2                            

T_DECL(settimeofday_29192647,
    "Verify that the syscall settimeofday is effective",
    T_META_ASROOT(true), T_META_CHECK_LEAKS(NO), T_META_LTEPHASE(LTE_POSTINIT))
{
	struct timeval time;
	long new_time;

	if (geteuid() != 0) {
		T_SKIP("settimeofday_29192647 test requires root privileges to run.");
	}

	T_QUIET;
	T_ASSERT_POSIX_ZERO(gettimeofday(&time, NULL), NULL);

	                                   
	new_time = time.tv_sec + DAY;

	time.tv_sec = new_time;
	time.tv_usec = 0;

	T_LOG("Attemping to set the time one day after.");

	T_WITH_ERRNO;
	T_ASSERT_POSIX_ZERO(settimeofday(&time, NULL), NULL);

	T_QUIET;
	T_ASSERT_POSIX_ZERO(gettimeofday(&time, NULL), NULL);

	                                
	T_EXPECT_GE_LONG(time.tv_sec, new_time, "Time successfully changed");

	                                         
	if (time.tv_sec >= new_time) {
		time.tv_sec = time.tv_sec - DAY;
		time.tv_usec = 0;

		T_WITH_ERRNO;
		T_ASSERT_POSIX_ZERO(settimeofday(&time, NULL), NULL);
	}
}

static void
get_abs_to_us_scale_factor(uint64_t* numer, uint64_t* denom)
{
	struct timespec time;
	uint64_t old_abstime, new_abstime;
	uint64_t old_time_usec, new_time_usec;
	uint64_t time_conv1, diff;
	mach_timebase_info_data_t timebaseInfo = { 0, 0 };

	T_QUIET; T_ASSERT_EQ(mach_get_times(&old_abstime, NULL, &time), KERN_SUCCESS, NULL);

	old_time_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	sleep(1);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&new_abstime, NULL, &time), KERN_SUCCESS, NULL);

	new_time_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	                                                  
	T_ASSERT_EQ(mach_timebase_info(&timebaseInfo), KERN_SUCCESS, NULL);

	new_time_usec -= old_time_usec;
	new_abstime -= old_abstime;

	time_conv1 = new_abstime;
	time_conv1 *= timebaseInfo.numer;
	time_conv1 /= timebaseInfo.denom * 1000;

	if (time_conv1 > new_time_usec) {
		diff = time_conv1 - new_time_usec;
	} else {
		diff = new_time_usec - time_conv1;
	}

	T_EXPECT_LE_ULLONG(diff, (unsigned long long)ERROR, "Check scale factor time base (%u/%u) delta read usec %llu delta converted %llu delta abs %llu", timebaseInfo.numer, timebaseInfo.denom, time_conv1, new_time_usec, new_abstime);

	*numer = (uint64_t)timebaseInfo.numer;
	*denom = (uint64_t)timebaseInfo.denom * 1000;
}


#define ADJSTMENT 3333            
#define ADJTIME_OFFSET_PER_SEC 500

T_DECL(adjtime_29192647,
    "Verify that the syscall adjtime is effective",
    T_META_CHECK_LEAKS(NO), T_META_LTEPHASE(LTE_POSTINIT), T_META_ASROOT(true))
{
	struct timespec time;
	struct timeval adj;
	uint64_t old_abstime, new_abstime, abs_delta;
	uint64_t old_time_usec, new_time_usec, us_delta, num, den;
	unsigned int sleep_time;
	long diff;
	const char * lterdos_env = NULL;

#if defined(__i386__) || defined(__x86_64__)
	T_SKIP("adjtime_29192647 test requires LTE to run.");
#endif

	if (geteuid() != 0) {
		T_SKIP("adjtime_29192647 test requires root privileges to run.");
	}

	lterdos_env = getenv("LTERDOS");

	if (lterdos_env != NULL) {
		if (!(strcmp(lterdos_env, "YES") == 0)) {
			T_SKIP("adjtime_29192647 test requires LTE to run.");
		}
	} else {
		T_SKIP("adjtime_29192647 test requires LTE to run.");
	}

	  
                                                               
    
	get_abs_to_us_scale_factor(&num, &den);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&old_abstime, NULL, &time), KERN_SUCCESS, NULL);

	old_time_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	adj.tv_sec = 0;
	adj.tv_usec = ADJSTMENT;

	T_LOG("Attemping to adjust the time of %d", ADJSTMENT);

	  
                                         
                                                         
                                                          
    
	T_WITH_ERRNO;
	T_ASSERT_POSIX_ZERO(adjtime(&adj, NULL), NULL);

	  
                                             
                                                              
                                                    
    
	sleep_time = (ADJSTMENT) / (ADJTIME_OFFSET_PER_SEC)+2;

	T_LOG("Waiting for %u sec\n", sleep_time);
	sleep(sleep_time);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&new_abstime, NULL, &time), KERN_SUCCESS, NULL);

	new_time_usec =  (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	us_delta = new_time_usec - old_time_usec;
	us_delta -= ADJSTMENT;

	                                         
	abs_delta = new_abstime - old_abstime;

	abs_delta *= num;
	abs_delta /= den;

	diff = (long) us_delta - (long) abs_delta;

	                                       
	T_EXPECT_LE_LONG(diff, (long) ERROR, "Check abs time vs calendar time");

	T_EXPECT_GE_LONG(diff, (long) -ERROR, "Check abs time vs calendar time");
}

#define FREQ_PPM 222                  
#define SHIFT_PLL 4
#define OFFSET_US 123          

T_DECL(ntp_adjtime_29192647,
    "Verify that the syscall ntp_adjtime is effective",
    T_META_CHECK_LEAKS(NO), T_META_LTEPHASE(LTE_POSTINIT), T_META_ASROOT(true))
{
	struct timespec time;
	struct timex ntptime;
	uint64_t abstime1, abstime2, abs_delta, num, den, time_delta;
	uint64_t time1_usec, time2_usec, time_conv, us_delta, app;
	int64_t offset;
	long diff, freq;
	unsigned int sleep_time;
	const char * lterdos_env = NULL;

#if defined(__i386__) || defined(__x86_64__)
	T_SKIP("ntp_adjtime_29192647 test requires LTE to run.");
#endif

	if (geteuid() != 0) {
		T_SKIP("ntp_adjtime_29192647 test requires root privileges to run.");
	}

	lterdos_env = getenv("LTERDOS");

	if (lterdos_env != NULL) {
		if (!(strcmp(lterdos_env, "YES") == 0)) {
			T_SKIP("adjtime_29192647 test requires LTE to run.");
		}
	} else {
		T_SKIP("adjtime_29192647 test requires LTE to run.");
	}

	  
                                                               
    
	get_abs_to_us_scale_factor(&num, &den);

	  
                                      
    
	memset(&ntptime, 0, sizeof(ntptime));

	ntptime.modes = MOD_STATUS;
	ntptime.status = TIME_OK;
	                                                                 
	freq = (FREQ_PPM) * 65536;
	ntptime.modes |= MOD_FREQUENCY;
	ntptime.freq = freq;

	T_LOG("Attemping to change calendar frequency of %d ppm", FREQ_PPM);

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntp_adjtime(&ntptime), TIME_OK, NULL);

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntptime.freq, freq, NULL);

	sleep(2);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&abstime1, NULL, &time), KERN_SUCCESS, NULL);

	time1_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	sleep(1);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&abstime2, NULL, &time), KERN_SUCCESS, NULL);

	time2_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	abs_delta = abstime2 - abstime1;
	us_delta = time2_usec - time1_usec;

	time_conv = abs_delta;
	time_conv *= num;
	time_conv /= den;

	app = time_conv / USEC_PER_SEC;              

	time_delta = time_conv;
	time_delta += app * (FREQ_PPM);

	app = time_conv % USEC_PER_SEC;

	time_delta += (app * (FREQ_PPM)) / USEC_PER_SEC;

	diff = (long) us_delta - (long) time_delta;

	                                        
	T_EXPECT_LE_LONG(diff, (long) ERROR, "Check abs time vs calendar time");

	T_EXPECT_GE_LONG(diff, (long) -ERROR, "Check abs time vs calendar time");

	memset(&ntptime, 0, sizeof(ntptime));

	                        
	freq = 0;
	ntptime.modes = MOD_STATUS;
	ntptime.status = TIME_OK;
	ntptime.modes |= MOD_FREQUENCY;
	ntptime.freq = freq;

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntp_adjtime(&ntptime), TIME_OK, NULL);

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntptime.freq, freq, NULL);

	sleep(1);

	  
                                       
    
	memset(&ntptime, 0, sizeof(ntptime));
	ntptime.modes |= MOD_STATUS;
	ntptime.status = TIME_OK;
	ntptime.status |= STA_PLL | STA_FREQHOLD;

	                                                                            
	ntptime.offset = OFFSET_US;
	ntptime.modes |= MOD_OFFSET | MOD_MICRO;

	  
                                     
                                                     
                       
    
	offset = (OFFSET_US) * 1000;
	sleep_time = 2;

	while ((offset >> SHIFT_PLL) > 0) {
		offset -= offset >> SHIFT_PLL;
		sleep_time++;
	}

	T_QUIET; T_ASSERT_EQ(mach_get_times(&abstime1, NULL, &time), KERN_SUCCESS, NULL);

	time1_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	T_LOG("Attemping to change calendar phase of %d us", OFFSET_US);

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntp_adjtime(&ntptime), TIME_OK, NULL);

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntptime.offset, (long) OFFSET_US, NULL);

	T_LOG("Waiting for %u sec\n", sleep_time);
	sleep(sleep_time);

	T_QUIET; T_ASSERT_EQ(mach_get_times(&abstime2, NULL, &time), KERN_SUCCESS, NULL);

	time2_usec = (uint64_t)time.tv_sec * USEC_PER_SEC + (uint64_t)time.tv_nsec / 1000;

	abs_delta = abstime2 - abstime1;
	us_delta = time2_usec - time1_usec;

	abs_delta *= num;
	abs_delta /= den;

	us_delta -= OFFSET_US;

	diff = (long) us_delta - (long) abs_delta;

	                                       
	T_EXPECT_LE_LONG(diff, (long) ERROR, "Check abs time vs calendar time");

	T_EXPECT_GE_LONG(diff, (long) -ERROR, "Check abs time vs calendar time");

	memset(&ntptime, 0, sizeof(ntptime));
	ntptime.modes = MOD_STATUS;
	ntptime.status = TIME_OK;
	ntptime.modes |= MOD_FREQUENCY;
	ntptime.freq = 0;

	ntptime.status |= STA_PLL;
	ntptime.offset = 0;
	ntptime.modes |= MOD_OFFSET;

	T_WITH_ERRNO;
	T_ASSERT_EQ(ntp_adjtime(&ntptime), TIME_OK, NULL);
}
