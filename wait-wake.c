#include "futex.h"

#define _GNU_SOURCE

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define NTHR 10

#define MUTEX
#define PRIORITY

#ifdef PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

// Test if FUTEX_WAIT thread are wake up in the order they wait.
static int ft_wait;
static pthread_mutex_t mutex;

void dosth()
{
    char a[256];
    char b[256];
    for (int i = 0; i < 30000; i++) {
      memcpy(a, b, sizeof(a));
    }
}

int lock() {
#ifdef MUTEX
    return pthread_mutex_lock(&mutex);
#else
    return sys_futex(&ft_wait, FUTEX_WAIT, 0, /* not used */ NULL, NULL, 0);
#endif
}

int unlock() {
#ifdef MUTEX
    return pthread_mutex_unlock(&mutex);
#else
    return sys_futex(&ft_wait, FUTEX_WAKE, 1, /* not used */ NULL, NULL, 0);
#endif
}

void *wait(void *_tid)
{
    long tid = (long)_tid;
#ifdef PRIORITY
    int policy;
    struct sched_param param;
    assert(0 == pthread_getschedparam(pthread_self(), &policy, &param));
    printf("T%ld policy %d priority %d\n", tid, policy, param.sched_priority);
#endif
    int ret = 0;
    if (tid == 3) {
        dosth();
    } 
    printf(">>> T%ld WAIT\n", tid);
    ret = lock();
    printf("<<< T%ld WAIT return %d\n", tid, ret);
}

int main(int argc, const char *argv[])
{
    long i = 0;
    int ret = 0;

    // Test what happens if futex value does not equal to expected.
    ft_wait = 1;                                                   
    pthread_t t1;                                                  
    pthread_create(&t1, NULL, wait, (void *)0);                    
    usleep(500);                                                   

    pthread_t thr[NTHR+1];
    pthread_attr_t tattr[NTHR+1];

    ft_wait = 0;
    for (i = 1; i <= NTHR; i++) {
        assert(0 == pthread_attr_init(&tattr[i]));
#ifdef PRIORITY
        int max_prio = sched_get_priority_max(SCHED_RR);
        int min_prio = sched_get_priority_min(SCHED_RR);
        int prio = i%3+1;
        assert(min_prio <= prio && prio <= max_prio);
        assert(0 == pthread_attr_setinheritsched(&tattr[i], PTHREAD_EXPLICIT_SCHED));
        assert(0 == pthread_attr_setschedpolicy(&tattr[i], SCHED_RR));
        struct sched_param param;
        assert(0 == pthread_attr_getschedparam(&tattr[i], &param));
        param.sched_priority = prio;
        assert(0 == pthread_attr_setschedparam(&tattr[i], &param));
#endif
        // Ensure thread calling FUTEX_WAIT in order.
        pthread_create(&thr[i], &tattr[i], wait, (void *)i);
#ifdef PRIORITY
        int policy;
        assert(0 == pthread_getschedparam(thr[i], &policy, &param));
        printf("T%ld policy %d priority %d\n", i, policy, param.sched_priority);
#endif
        usleep(500);
    }

    for (i = 1; i <= NTHR; i++) {
        printf("main wake up 1\n");
        unlock();
        usleep(500);
    }

    usleep(500);
    printf("all thread should have been waken now\n");

    return 0;
}

