#ifndef threadpool_h
#define threadpool_h

#if defined(__cplusplus)
extern "C" {
#endif
    
#include <stddef.h>
#include <stdbool.h>

#define TP_API_VERSION 1
#define TP_MESSAGE_MAXSIZE 96

typedef struct tp_threadpool tp_threadpool;

typedef enum {
    TP_ERROR_OK = 0,
    TP_ERROR_NOMEMORY,
    TP_ERROR_SYSRES,
    TP_ERROR_BADARG,
    TP_ERROR_NOPERM,
    TP_ERROR_BADCONFIG,
    TP_ERROR_ISSHUTDOWN,
    TP_ERROR_SHUTTINGDOWN,
    TP_ERROR_ISRUNNING,
    TP_ERROR_ISLOCKED,
    TP_ERROR_ISNOTLOCKED,
    TP_ERROR_LOCKEDELSEWHERE,
    TP_ERROR_TIMEOUT,
    TP_ERROR_ZEROWAITING,
    TP_ERROR_UNKNOWN
} tp_error;

typedef enum {
    TP_CONFIGEVAL_OK = 0,
    TP_CONFIGEVAL_WRONGVERSION,
    TP_CONFIGEVAL_STACKTOOSMALL,
    TP_CONFIGEVAL_STACKNOTPAGES,
    TP_CONFIGEVAL_GUARDTOOSMALL,
    TP_CONFIGEVAL_SGTOOLARGE,
    TP_CONFIGEVAL_NOTHREADS,
    TP_CONFIGEVAL_TOOMANYTHREADS,
    TP_CONFIGEVAL_QRESIZEZERO,
    TP_CONFIGEVAL_PROCSCOPENSUP
} tp_configeval;

typedef enum {
    TP_CONTENTIONSCOPE_DEFAULT = 0,
    TP_CONTENTIONSCOPE_PROCESS,
    TP_CONTENTIONSCOPE_SYSTEM
} tp_contentionscope;

typedef enum {
    TP_SCHEDULE_DEFAULT = 0,
    TP_SCHEDULE_FIFO,
    TP_SCHEDULE_ROUNDROBIN
} tp_schedule;

typedef struct {
    size_t num_workers_total;
    size_t num_tasks_queued;
    size_t num_workers_waiting;
    size_t num_tasks_performed;
    size_t num_tasks_succeeded;
    double cpu_seconds;
} tp_stats;

typedef enum {
    TP_COMPONENT_WORKERS,
    TP_COMPONENT_BUSY,
    TP_COMPONENT_WAITING,
    TP_COMPONENT_QUEUED
} tp_component;

typedef enum {
    TP_CHANGE_INCREMENT,
    TP_CHANGE_DECREMENT,
    TP_CHANGE_ZERO
} tp_change;

#define TP_MINIMUM_RESIZELIMIT 1
#define TP_MINIMUM_RESIZEINCREMENT 1
#define TP_MINIMUM_GUARDSIZE 1024

#define TP_DEFAULT_RESIZELIMIT 16
#define TP_DEFAULT_RESIZEINCREMENT 16
#define TP_DEFAULT_QUEUESIZE 64

typedef struct {
    unsigned char api_version;
    size_t stacksize;
    size_t guardsize;
    tp_contentionscope contentionscope;
    tp_schedule threadschedule;
    size_t min_threads;
    size_t more_threads;
    tp_schedule queueschedule;
    size_t initial_queue;
    unsigned char queue_resize_limit;
    unsigned char queue_resize_increment;
    void (* ontaskfailed)(tp_threadpool *pool, int result, void *taskdata);
    void (* onstatschanged)(tp_threadpool *pool, tp_stats stats);
    void *userdata;
} tp_config;

typedef int (* tp_task)(void *taskdata, void *userdata);

bool tp_info_procscopeissupported();
unsigned char tp_info_hardwareconcurrency();
size_t tp_info_sysdefaultguard();
size_t tp_info_defaultguard();
size_t tp_info_minstack();
size_t tp_info_sysdefaultstack();
struct rlimit tp_info_sysmaxstack();
struct rlimit tp_info_sysmaxmem();
struct rlimit tp_info_sysmaxthreads();
size_t tp_info_sizeofthreadpool();

size_t tp_utils_roundedstack(size_t stacksize);
size_t tp_utils_mostthreadsfor(size_t stacksize, size_t guardsize);
tp_config tp_utils_blankconfig();
tp_config tp_utils_defaultconfig();
tp_configeval tp_utils_evaluateconfig(tp_config config);
char * tp_utils_evalmessage(tp_configeval eval, char *buffer);
char * tp_utils_errormessage(tp_error error, char *buffer);
clock_t tp_utils_sectoclock(double seconds);
bool tp_utils_statsareclear(tp_stats);

tp_error tp_create(tp_threadpool **pool, tp_config config);

tp_config tp_getconfig(tp_threadpool *pool);
tp_stats tp_getstats(tp_threadpool *pool);
bool tp_canhandlesigs(tp_threadpool *pool);
bool tp_isrunning(tp_threadpool *pool);
bool tp_islocked(tp_threadpool *pool);
bool tp_isclear(tp_threadpool *pool);
void * tp_userdata(tp_threadpool *pool);

tp_error tp_enqueue(tp_threadpool *pool, tp_task task, void *taskdata, bool *wasenqueued);
tp_error tp_waitforclear(tp_threadpool *pool);
tp_error tp_timedwaitforclear(tp_threadpool *pool, size_t millis);
tp_error tp_waitforqempty(tp_threadpool *pool);
tp_error tp_timedwaitforqempty(tp_threadpool *pool, size_t millis);
tp_error tp_waitforchange(tp_threadpool *pool, tp_component component, tp_change change);
tp_error tp_timedwaitforchange(tp_threadpool *pool, tp_component component, tp_change change, size_t millis);
tp_error tp_lock(tp_threadpool *pool);
tp_error tp_unlock(tp_threadpool *pool);
tp_error tp_shutdown(tp_threadpool *pool);

tp_error tp_destroy(tp_threadpool *pool);
    
#if defined(__cplusplus)
}
#endif

#endif
