#ifndef utilities_h
#define utilities_h

#if defined(__cplusplus)
extern "C" {
#endif

#define TPU_TIMESTAMP_LENGTH 30

size_t tpu_millitime();
bool tpu_relative_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, size_t millis);
size_t tpu_get_random();
size_t tpu_get_random_index(size_t max);
tpi_error tpu_attr_init(pthread_attr_t *attr, size_t stack, size_t guard, tpi_schedule sched, tpi_contentionscope scope);
tpi_error tpu_pthread_to_tpi(int pterr);
bool tpu_stats_equal(tpi_stats a, tpi_stats b);
    
#if defined(__cplusplus)
}
#endif

#endif
