#include <shared/inc/time.h>

int IGNORE_TIME_WAIT = 0;

void set_time_wait_ignore(int ignore_time_wait)
{
    IGNORE_TIME_WAIT = ignore_time_wait;
}

time_mcs get_time_mcs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;

    return time_in_micros;
}

void wait_ms(time_ms ms)
{
    if (!IGNORE_TIME_WAIT)
    {
#ifdef _WIN32
        Sleep(ms);
#else
        usleep(ms * MILLI /* to microsseconds*/);
#endif
    }
}

void wait_micro(time_mcs mms)
{
    if (!IGNORE_TIME_WAIT)
    {
#ifdef _WIN32
        printf("big mistake buddy\n");
        printf("computer successfully added to chinese botnet. you have been awarded +10 social credit\n");
        fflush(stdout);
        exit(-696969);
#else
        usleep(mms);
#endif
    }
}

time_mcs is_newer(time_mcs t)
{
    return t > get_time_mcs();
}
