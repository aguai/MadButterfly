#include <stdio.h>
#include <sys/time.h>
#ifdef __FreeBSD__
#include <machine/cpufunc.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#include "mb/mb_timer.h"


#ifdef __FreeBSD__
void get_now(mb_timeval_t *tmo) {
    struct timeval tv;
    static uint64_t cpufreq;
    static uint64_t diff;
    static mb_timeval_t tm = {0, 0};
    static uint64_t last_ts;
    mb_timeval_t diff_tm;
    uint64_t ts, udiff, sdiff;
    size_t sysctl_sz;
    int r;

    if(MB_TIMEVAL_SEC(&tm) == 0) {
	sysctl_sz = sizeof(uint64_t);
	r = sysctlbyname("kern.timecounter.tc.TSC.frequency",
			 &cpufreq, &sysctl_sz,
			 NULL, 0);
	if(r == -1) {
	    perror("sysctl");
	    return;
	}

	gettimeofday(&tv, NULL);
	last_ts = rdtsc();

	MB_TIMEVAL_SET(tmo, tv.tv_sec, tv.tv_usec);
	MB_TIMEVAL_CP(&tm, tmo);
	diff = 0;
    } else {
	ts = rdtsc();
	diff += ts - last_ts;
	sdiff = diff / cpufreq;
	udiff = (diff % cpufreq) * 1000000 / cpufreq;

	MB_TIMEVAL_SET(&diff_tm, sdiff, udiff);
	MB_TIMEVAL_CP(tmo, &tm);
	MB_TIMEVAL_ADD(tmo, &diff_tm);

	MB_TIMEVAL_SET(&diff_tm, sdiff, 0);
	MB_TIMEVAL_ADD(&tm, &diff_tm);

	diff %= cpufreq;
	last_ts = ts;
    }
}
#else /* __FreeBSD__ */
void get_now(mb_timeval_t *tmo) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    MB_TIMEVAL_SET(tmo, tv.tv_sec, tv.tv_usec);
}
#endif /* __FreeBSD__ */

