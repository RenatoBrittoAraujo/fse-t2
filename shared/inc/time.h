#ifndef SHARED_TIME_H
#define SHARED_TIME_H 1

#ifndef TEST_MODE
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#endif

#include <bits/types/time_t.h>

#define SECOND 1000 // milissegundos
#define MILLI 1000  // microssegundos

typedef unsigned long time_ms;
typedef unsigned long time_mcs;

// if set to 1, wait_ms and wait_micro will not wait
// useful for testing
void set_time_wait_ignore(int ignore);

void wait_ms(time_ms ms);
void wait_micro(time_mcs mms);

time_mcs get_time_mcs();
time_mcs is_newer(time_mcs t);

#endif