#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/timeb.h>
#include "tpdefs.h"
#include "utilities.h"

#define ONE_THOUSAND 1000
#define ONE_MILLION 1000000
#define ONE_BILLION 1000000000

size_t tpu_millitime() {
    struct timeb base;
    ftime(&base);
    size_t millis = ONE_THOUSAND * base.time;
    millis += base.millitm;
    return millis;
}

time_t prev = 0;

struct timespec tpu_now_timespec() {
    struct timeb base;
    ftime(&base);
    struct timespec spec;
    spec.tv_sec = base.time;
    spec.tv_nsec = base.millitm * ONE_MILLION;
    return spec;
}

struct timespec tpu_relative_timespec(size_t millis) {
    struct timespec spec = tpu_now_timespec();
    long nanos = spec.tv_nsec + (millis * ONE_MILLION);
    spec.tv_sec += nanos / ONE_BILLION;
    spec.tv_nsec = nanos % ONE_BILLION;
    return spec;
}

bool tpu_relative_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, size_t millis) {
    struct timespec then = tpu_relative_timespec(millis);
    if (pthread_cond_timedwait(cond, mutex, &then)) {
        return false;
    }
    return true;
}

size_t tpu_get_random() {
    time_t now = time(NULL);
    if (prev < now) {
        unsigned int seed = (unsigned int) time(NULL);
        srandom(seed);
        prev = now;
    }
    long res = random();
    size_t act;
    if (res < 0) {
        act = LONG_MAX - res;
    } else {
        act = res;
    }
    return act;
}

size_t get_random_index(size_t max) {
    return tpu_get_random() % max;
}

tpi_error tpu_attr_init(pthread_attr_t *attr, size_t stack, size_t guard, tpi_schedule sched, tpi_contentionscope scope) {
    int result = pthread_attr_init(attr);
    if (!result) {
        return tpu_pthread_to_tpi(result);
    }
    pthread_attr_setguardsize(attr, guard);
    pthread_attr_setstacksize(attr, stack);
    switch (sched) {
        case TPI_SCHEDULE_DEFAULT:
            break;
        case TPI_SCHEDULE_FIFO:
            pthread_attr_setschedpolicy(attr, SCHED_FIFO);
            break;
        case TPI_SCHEDULE_ROUNDROBIN:
            pthread_attr_setschedpolicy(attr, SCHED_RR);
            break;
    }
    switch (scope) {
        case TPI_CONTENTIONSCOPE_DEFAULT:
            break;
        case TPI_CONTENTIONSCOPE_PROCESS:
            pthread_attr_setscope(attr, PTHREAD_SCOPE_PROCESS);
            break;
        case TPI_CONTENTIONSCOPE_SYSTEM:
            pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);
            break;
    }
    return TPI_ERROR_OK;
}

bool tpu_stats_equal(tpi_stats a, tpi_stats b) {
    if (a.num_busy == b.num_busy) {
        if (a.num_complete == b.num_complete) {
            if (a.num_queued == b.num_queued) {
                if (a.num_success == b.num_success) {
                    if (a.num_workers == b.num_workers) {
                        if (a.cpu_time == b.cpu_time) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

tpi_error tpu_pthread_to_tpi(int pterr) {
    switch (pterr) {
        case 0:
            return TPI_ERROR_OK;
        case EINVAL:
            return TPI_ERROR_BADARG;
        case EAGAIN:
            return TPI_ERROR_SYSRES;
        case ENOMEM:
            return TPI_ERROR_NOMEMORY;
        case EPERM:
            return TPI_ERROR_NOPERM;
        default:
            return TPI_ERROR_UNKNOWN;
    }
}