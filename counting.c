#include <stdbool.h>
#include <pthread.h>
#include "tpdefs.h"
#include "utilities.h"
#include "counting.h"

tpi_error tpc_counter_init(tpc_counter *counter, pthread_mutex_t *mutex) {
    int holder = 0;
    if ((holder = pthread_cond_init(&counter->inc_cond, NULL))) {
        return tpu_pthread_to_tpi(holder);
    }
    if ((holder = pthread_cond_init(&counter->dec_cond, NULL))) {
        pthread_cond_destroy(&counter->inc_cond);
        return tpu_pthread_to_tpi(holder);
    }
    if ((holder = pthread_cond_init(&counter->zero_cond, NULL))) {
        pthread_cond_destroy(&counter->inc_cond);
        pthread_cond_destroy(&counter->dec_cond);
        return tpu_pthread_to_tpi(holder);
    }
    counter->count = 0;
    counter->mutex = mutex;
    return TPI_ERROR_OK;
}

void tpc_counter_increment(tpc_counter *counter) {
    counter->count++;
    pthread_cond_broadcast(&counter->inc_cond);
}

void tpc_counter_decrement(tpc_counter *counter) {
    if (counter->count) {
        counter->count--;
        pthread_cond_broadcast(&counter->dec_cond);
        if (counter->count == 0) {
            pthread_cond_broadcast(&counter->zero_cond);
        }
    }
}

void tpc_counter_waitfor(tpc_counter *counter, tpc_event event) {
    switch (event) {
        case TPC_EVENT_INCREMENT:
            pthread_cond_wait(&counter->inc_cond, counter->mutex);
            break;
        case TPC_EVENT_DECREMENT:
            pthread_cond_wait(&counter->dec_cond, counter->mutex);
            break;
        case TPC_EVENT_ZERO:
            if (counter->count) {
                pthread_cond_wait(&counter->zero_cond, counter->mutex);
            }
            break;
    }
}

bool tpc_counter_timedwaitfor(tpc_counter *counter, tpc_event event, size_t millis) {
    switch (event) {
        case TPC_EVENT_INCREMENT:
            return tpu_relative_wait(&counter->inc_cond, counter->mutex, millis);
        case TPC_EVENT_DECREMENT:
            return tpu_relative_wait(&counter->dec_cond, counter->mutex, millis);
        case TPC_EVENT_ZERO:
            if (counter->count) {
                return tpu_relative_wait(&counter->zero_cond, counter->mutex, millis);
            }
            return true;
    }
}

void tpc_counter_destroy(tpc_counter *counter) {
    pthread_cond_destroy(&counter->inc_cond);
    pthread_cond_destroy(&counter->dec_cond);
    pthread_cond_destroy(&counter->zero_cond);
}

tpi_error tpc_manifest_init(tpc_manifest *manifest, void (* onstatschanged)(tpi_stats, void *), void *userdata) {
    int holder = 0;
    if ((holder = pthread_mutex_init(&manifest->mutex, NULL))) {
        return tpu_pthread_to_tpi(holder);
    }
    tpi_error errhld = TPI_ERROR_OK;
    if ((errhld = tpc_counter_init(&manifest->num_workers, &manifest->mutex))) {
        pthread_mutex_destroy(&manifest->mutex);
        return errhld;
    }
    if ((errhld = tpc_counter_init(&manifest->num_queued, &manifest->mutex))) {
        tpc_counter_destroy(&manifest->num_workers);
        pthread_mutex_destroy(&manifest->mutex);
        return errhld;
    }
    if ((errhld = tpc_counter_init(&manifest->num_busy, &manifest->mutex))) {
        tpc_counter_destroy(&manifest->num_queued);
        tpc_counter_destroy(&manifest->num_workers);
        pthread_mutex_destroy(&manifest->mutex);
        return errhld;
    }
    manifest->num_complete = 0;
    manifest->num_success = 0;
    manifest->cpu_seconds = 0;
    manifest->onstatschanged = onstatschanged;
    manifest->userdata = userdata;
    if (manifest->onstatschanged) {
        manifest->previous = tpc_manifest_stats(manifest);
    }
    return TPI_ERROR_OK;
}

void tpc_manifest_acquire(tpc_manifest *manifest) {
    pthread_mutex_lock(&manifest->mutex);
}

void tpc_manifest_release(tpc_manifest *manifest) {
    if (manifest->onstatschanged) {
        tpi_stats after = tpc_manifest_stats(manifest);
        if (!tpu_stats_equal(manifest->previous, after)) {
            manifest->onstatschanged(after, manifest->userdata);
            manifest->previous = after;
        }
    }
    pthread_mutex_unlock(&manifest->mutex);
}

tpi_stats tpc_manifest_stats(tpc_manifest *manifest) {
    tpi_stats stats = {
        .num_workers = manifest->num_workers.count,
        .num_queued = manifest->num_queued.count,
        .num_busy = manifest->num_busy.count,
        .num_complete = manifest->num_complete,
        .num_success = manifest->num_success,
        .cpu_time = manifest->cpu_seconds
    };
    return stats;
}

void tpc_manifest_tallyresult(tpc_manifest *manifest, int result, clock_t ticks) {
    manifest->num_complete++;
    manifest->cpu_seconds += ((double) ticks) / CLOCKS_PER_SEC;
    if (result == 0) {
        manifest->num_success++;
    }
}

void tpc_manifest_increment(tpc_manifest *manifest, tpc_target target) {
    switch (target) {
        case TPC_TARGET_WORKERS:
            tpc_counter_increment(&manifest->num_workers);
            break;
        case TPC_TARGET_QUEUED:
            tpc_counter_increment(&manifest->num_queued);
            break;
        case TPC_TARGET_BUSY:
            tpc_counter_increment(&manifest->num_busy);
            break;
    }
}

void tpc_manifest_decrement(tpc_manifest *manifest, tpc_target target) {
    switch (target) {
        case TPC_TARGET_WORKERS:
            tpc_counter_decrement(&manifest->num_workers);
            break;
        case TPC_TARGET_QUEUED:
            tpc_counter_decrement(&manifest->num_queued);
            break;
        case TPC_TARGET_BUSY:
            tpc_counter_decrement(&manifest->num_busy);
            break;
    }
}

void tpc_manifest_waitfor(tpc_manifest *manifest, tpc_target target, tpc_event event) {
    switch (target) {
        case TPC_TARGET_WORKERS:
            tpc_counter_waitfor(&manifest->num_workers, event);
            break;
        case TPC_TARGET_QUEUED:
            tpc_counter_waitfor(&manifest->num_queued, event);
            break;
        case TPC_TARGET_BUSY:
            tpc_counter_waitfor(&manifest->num_busy, event);
            break;
    }
}

bool tpc_manifest_timedwaitfor(tpc_manifest *manifest, tpc_target target, tpc_event event, size_t millis) {
    switch (target) {
        case TPC_TARGET_WORKERS:
            return tpc_counter_timedwaitfor(&manifest->num_workers, event, millis);
        case TPC_TARGET_QUEUED:
            return tpc_counter_timedwaitfor(&manifest->num_queued, event, millis);
        case TPC_TARGET_BUSY:
            return tpc_counter_timedwaitfor(&manifest->num_busy, event, millis);
    }
}

void tpc_manifest_destroy(tpc_manifest *manifest) {
    tpc_counter_destroy(&manifest->num_workers);
    tpc_counter_destroy(&manifest->num_queued);
    tpc_counter_destroy(&manifest->num_busy);
    pthread_mutex_destroy(&manifest->mutex);
}