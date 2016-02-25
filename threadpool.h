#ifndef threadpool_h
#define threadpool_h
    
#include <stddef.h>
#include <stdbool.h>

#define TP_API_VERSION 1

typedef struct tp_threadpool tp_threadpool;

#define TP_ERRMESSAGE_OK "No error occurred."
#define TP_ERRMESSAGE_NOMEMORY "System lacked sufficient memory to perform requested action."
#define TP_ERRMESSAGE_SYSRES "System lacked resources other than memory required to perform requested action."
#define TP_ERRMESSAGE_BADARG "Some parameter passed was invalid."
#define TP_ERRMESSAGE_NOPERM "Process lacks permission to perform requested action."
#define TP_ERRMESSAGE_BADCONFIG "The tp_config passed was not valid."
#define TP_ERRMESSAGE_ISSHUTDOWN "The threadpool being referenced has been shutdown."
#define TP_ERRMESSAGE_SHUTTINGDOWN "The threadpool is in the process of shutting down."
#define TP_ERRMESSAGE_ISRUNNING "The threadpool being deallocated is still running."
#define TP_ERRMESSAGE_ISLOCKED "The threadpool is locked thus preventing the requested action."
#define TP_ERRMESSAGE_ISNOTLOCKED "The threadpool is not locked and cannot be unlocked."
#define TP_ERRMESSAGE_LOCKEDELSEWHERE "The threadpool has been locked but the calling thread may not unlock it."
#define TP_ERRMESSAGE_TIMEOUT "The wait time specified for an event has passed without the event occurring."
#define TP_ERRMESSAGE_ZEROWAITING "Blocking until the number of waiting threads is zero is not supported."
#define TP_ERRMESSAGE_UNKNOWN "An error has occurred but the reason for it is unknown."

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

#define TP_EVALMESSAGE_OK "The tp_config is valid and may be used to construct a tp_threadpool."
#define TP_EVALMESSAGE_WRONGVERSION "The api_version specified does not match the compiled library's version."
#define TP_EVALMESSAGE_STACKTOOSMALL "The stack size specified is less than the API enforced minimum."
#define TP_EVALMESSAGE_STACKNOTPAGES "The stack size specified is not a multiple of the system page size."
#define TP_EVALMESSAGE_GUARDTOOSMALL "The guard size specified is less than TP_MINIMUM_GUARDSIZE."
#define TP_EVALMESSAGE_SGTOOLARGE "The stack and guard sizes could cause maxthreads to exceed the process stack limit."
#define TP_EVALMESSAGE_NOTHREADS "The maximum number of threads allowed was specified as zero."
#define TP_EVALMESSAGE_TOOMANYTHREADS "The specified maximum number of threads exceeds a system imposed maximum."
#define TP_EVALMESSAGE_QRESIZEZERO "One or both of the queue resize parameters is zero and therefore invalid."
#define TP_EVALMESSAGE_PROCSCOPENSUP "The operating system does not support the TP_CONTENTIONSCOPE_PROCESS option."

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
bool tp_utils_limitisunlimited(rlim_t limit);
bool tp_utils_valueexceedslimit(rlim_t value, rlim_t limit);
size_t tp_utils_reqstackfor(size_t numthreads);
size_t tp_utils_mostthreadsfor(size_t stacksize, size_t guardsize);
tp_config tp_utils_blankconfig();
tp_config tp_utils_defaultconfig();
tp_configeval tp_utils_evaluateconfig(tp_config config);
size_t tp_utils_evalmessagelength(tp_configeval eval);
char * tp_utils_evalmessage(tp_configeval eval, char *buffer);
size_t tp_utils_errormessagelength(tp_error error);
char * tp_utils_errormessage(tp_error error, char *buffer);
clock_t tp_utils_sectoclock(double seconds);
bool tp_utils_statsareclear(tp_stats);

tp_error tp_create(tp_threadpool **pool, tp_config config);

tp_config tp_getconfig(tp_threadpool *pool);
tp_stats tp_getstats(tp_threadpool *pool);
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

#endif
