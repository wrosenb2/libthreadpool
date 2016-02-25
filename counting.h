#ifndef counting_h
#define counting_h

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t inc_cond;
    pthread_cond_t dec_cond;
    pthread_cond_t zero_cond;
    size_t count;
} tpc_counter;

typedef struct {
    pthread_mutex_t mutex;
    tpc_counter num_workers;
    tpc_counter num_busy;
    tpc_counter num_queued;
    size_t num_complete;
    size_t num_success;
    double cpu_seconds;
    void (* onstatschanged)(tpi_stats, void *);
    void *userdata;
    tpi_stats previous;
} tpc_manifest;

typedef enum {
    TPC_EVENT_INCREMENT,
    TPC_EVENT_DECREMENT,
    TPC_EVENT_ZERO
} tpc_event;

typedef enum {
    TPC_TARGET_WORKERS,
    TPC_TARGET_BUSY,
    TPC_TARGET_QUEUED
} tpc_target;

tpi_error tpc_manifest_init(tpc_manifest *manifest, void (* onstatschanged)(tpi_stats, void *), void *userdata);
void tpc_manifest_acquire(tpc_manifest *manifest);
void tpc_manifest_release(tpc_manifest *manifest);
tpi_stats tpc_manifest_stats(tpc_manifest *manifest);
void tpc_manifest_tallyresult(tpc_manifest *manifest, int result, clock_t ticks);
void tpc_manifest_increment(tpc_manifest *manifest, tpc_target target);
void tpc_manifest_decrement(tpc_manifest *manifest, tpc_target target);
void tpc_manifest_waitfor(tpc_manifest *manifest, tpc_target target, tpc_event event);
bool tpc_manifest_timedwaitfor(tpc_manifest *manifest, tpc_target target, tpc_event event, size_t millis);
void tpc_manifest_destroy(tpc_manifest *manifest);

#endif
