#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "threadpool.h"
#include "tpdefs.h"
#include "utilities.h"
#include "counting.h"
#include "queue.h"
#include "worker.h"

bool tp_info_procscopeissupported() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    bool result = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS) == ENOTSUP ? false : true;
    pthread_attr_destroy(&attr);
    return result;
}

unsigned char tp_info_hardwareconcurrency() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

size_t tp_info_sysdefaultguard() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t length = 0;
    pthread_attr_getguardsize(&attr, &length);
    pthread_attr_destroy(&attr);
    return length;
}

size_t tp_info_defaultguard() {
    return getpagesize();
}

size_t tp_info_minstack() {
    return PTHREAD_STACK_MIN + getpagesize();
}

size_t tp_info_sysdefaultstack() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t length = 0;
    pthread_attr_getstacksize(&attr, &length);
    pthread_attr_destroy(&attr);
    return length;
}

size_t tp_utils_roundedstack(size_t stacksize) {
    int pagesize = getpagesize();
    if (stacksize % pagesize) {
        return stacksize + pagesize;
    }
    return stacksize;
}

struct rlimit tp_info_sysmaxstack() {
    struct rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);
    return limit;
}

struct rlimit tp_info_sysmaxmem() {
    struct rlimit limit;
    getrlimit(RLIMIT_AS, &limit);
    return limit;
}

struct rlimit tp_info_sysmaxthreads() {
    struct rlimit limit;
    getrlimit(RLIMIT_NPROC, &limit);
    return limit;
}

bool tp_utils_limitisunlimited(rlim_t limit) {
    return limit == RLIM_INFINITY ? true : false;
}

bool tp_utils_valueexceedslimit(rlim_t value, rlim_t limit) {
    if (tp_utils_limitisunlimited(limit)) {
        return false;
    }
    return limit < value;
}

size_t tp_utils_reqstackfor(size_t numthreads) {
    return tp_info_minstack() * numthreads;
}

size_t tp_utils_mostthreadsfor(size_t stacksize, size_t guardsize) {
    struct rlimit limit = tp_info_sysmaxstack();
    return limit.rlim_cur / (stacksize + guardsize);
}

tp_config tp_utils_blankconfig() {
    tp_config config;
    bzero(&config, sizeof(tp_config));
    return config;
}

tp_config tp_utils_defaultconfig() {
    tp_config config;
    config.api_version = TP_API_VERSION;
    config.contentionscope = TP_CONTENTIONSCOPE_DEFAULT;
    config.guardsize = getpagesize();
    config.initial_queue = TP_DEFAULT_QUEUESIZE;
    config.min_threads = tp_info_hardwareconcurrency();
    config.more_threads = 0;
    config.stacksize = tp_info_sysdefaultstack();
    config.threadschedule = TP_SCHEDULE_DEFAULT;
    config.queueschedule = TP_SCHEDULE_DEFAULT;
    config.queue_resize_limit = TP_DEFAULT_RESIZELIMIT;
    config.queue_resize_increment = TP_DEFAULT_RESIZEINCREMENT;
    config.onstatschanged = NULL;
    config.ontaskfailed = NULL;
    config.userdata = NULL;
    return config;
}

tp_configeval tp_utils_evaluateconfig(tp_config config) {
    if (config.api_version != TP_API_VERSION) {
        return TP_CONFIGEVAL_WRONGVERSION;
    }
    if (config.guardsize < TP_MINIMUM_GUARDSIZE) {
        return TP_CONFIGEVAL_GUARDTOOSMALL;
    }
    if (config.stacksize < tp_info_minstack()) {
        return TP_CONFIGEVAL_STACKTOOSMALL;
    }
    if (config.stacksize % getpagesize()) {
        return TP_CONFIGEVAL_STACKNOTPAGES;
    }
    if (config.queue_resize_increment == 0 || config.queue_resize_limit == 0) {
        return TP_CONFIGEVAL_QRESIZEZERO;
    }
    if (config.min_threads + config.more_threads) {
        size_t max_threads = config.min_threads + config.more_threads + 1;
        struct rlimit limit = tp_info_sysmaxthreads();
        if (tp_utils_valueexceedslimit(max_threads, limit.rlim_cur)) {
            return TP_CONFIGEVAL_TOOMANYTHREADS;
        }
        limit = tp_info_sysmaxstack();
        if (tp_utils_valueexceedslimit(max_threads * (config.stacksize + config.guardsize), limit.rlim_cur)) {
            return TP_CONFIGEVAL_SGTOOLARGE;
        }
    } else {
        return TP_CONFIGEVAL_NOTHREADS;
    }
    if (config.contentionscope == TP_CONTENTIONSCOPE_PROCESS) {
        if (!tp_info_procscopeissupported()) {
            return TP_CONFIGEVAL_PROCSCOPENSUP;
        }
    }
    return TP_CONFIGEVAL_OK;
}

size_t tp_utils_evalmessagelength(tp_configeval eval) {
    switch (eval) {
        case TP_CONFIGEVAL_GUARDTOOSMALL:
            return sizeof(TP_EVALMESSAGE_GUARDTOOSMALL);
        case TP_CONFIGEVAL_NOTHREADS:
            return sizeof(TP_EVALMESSAGE_NOTHREADS);
        case TP_CONFIGEVAL_OK:
            return sizeof(TP_EVALMESSAGE_OK);
        case TP_CONFIGEVAL_PROCSCOPENSUP:
            return sizeof(TP_EVALMESSAGE_PROCSCOPENSUP);
        case TP_CONFIGEVAL_QRESIZEZERO:
            return sizeof(TP_EVALMESSAGE_QRESIZEZERO);
        case TP_CONFIGEVAL_SGTOOLARGE:
            return sizeof(TP_EVALMESSAGE_SGTOOLARGE);
        case TP_CONFIGEVAL_STACKNOTPAGES:
            return sizeof(TP_EVALMESSAGE_STACKNOTPAGES);
        case TP_CONFIGEVAL_STACKTOOSMALL:
            return sizeof(TP_EVALMESSAGE_STACKTOOSMALL);
        case TP_CONFIGEVAL_TOOMANYTHREADS:
            return sizeof(TP_EVALMESSAGE_TOOMANYTHREADS);
        case TP_CONFIGEVAL_WRONGVERSION:
            return sizeof(TP_EVALMESSAGE_WRONGVERSION);
    }
}

char * tp_utils_evalmessage(tp_configeval eval, char *buffer) {
    switch (eval) {
        case TP_CONFIGEVAL_GUARDTOOSMALL:
            return strcpy(buffer, TP_EVALMESSAGE_GUARDTOOSMALL);
        case TP_CONFIGEVAL_NOTHREADS:
            return strcpy(buffer, TP_EVALMESSAGE_NOTHREADS);
        case TP_CONFIGEVAL_OK:
            return strcpy(buffer, TP_EVALMESSAGE_OK);
        case TP_CONFIGEVAL_PROCSCOPENSUP:
            return strcpy(buffer, TP_EVALMESSAGE_PROCSCOPENSUP);
        case TP_CONFIGEVAL_QRESIZEZERO:
            return strcpy(buffer, TP_EVALMESSAGE_QRESIZEZERO);
        case TP_CONFIGEVAL_SGTOOLARGE:
            return strcpy(buffer, TP_EVALMESSAGE_SGTOOLARGE);
        case TP_CONFIGEVAL_STACKNOTPAGES:
            return strcpy(buffer, TP_EVALMESSAGE_STACKNOTPAGES);
        case TP_CONFIGEVAL_STACKTOOSMALL:
            return strcpy(buffer, TP_EVALMESSAGE_STACKTOOSMALL);
        case TP_CONFIGEVAL_TOOMANYTHREADS:
            return strcpy(buffer, TP_EVALMESSAGE_TOOMANYTHREADS);
        case TP_CONFIGEVAL_WRONGVERSION:
            return strcpy(buffer, TP_EVALMESSAGE_WRONGVERSION);
    }
}

size_t tp_utils_errormessagelength(tp_error error) {
    switch (error) {
        case TP_ERROR_BADARG:
            return sizeof(TP_ERRMESSAGE_BADARG);
        case TP_ERROR_BADCONFIG:
            return sizeof(TP_ERRMESSAGE_BADCONFIG);
        case TP_ERROR_ISLOCKED:
            return sizeof(TP_ERRMESSAGE_ISLOCKED);
        case TP_ERROR_ISNOTLOCKED:
            return sizeof(TP_ERRMESSAGE_ISNOTLOCKED);
        case TP_ERROR_ISRUNNING:
            return sizeof(TP_ERRMESSAGE_ISRUNNING);
        case TP_ERROR_ISSHUTDOWN:
            return sizeof(TP_ERRMESSAGE_ISSHUTDOWN);
        case TP_ERROR_SHUTTINGDOWN:
            return sizeof(TP_ERRMESSAGE_SHUTTINGDOWN);
        case TP_ERROR_LOCKEDELSEWHERE:
            return sizeof(TP_ERRMESSAGE_LOCKEDELSEWHERE);
        case TP_ERROR_NOMEMORY:
            return sizeof(TP_ERRMESSAGE_NOMEMORY);
        case TP_ERROR_NOPERM:
            return sizeof(TP_ERRMESSAGE_NOPERM);
        case TP_ERROR_OK:
            return sizeof(TP_ERRMESSAGE_OK);
        case TP_ERROR_SYSRES:
            return sizeof(TP_ERRMESSAGE_SYSRES);
        case TP_ERROR_TIMEOUT:
            return sizeof(TP_ERRMESSAGE_TIMEOUT);
        case TP_ERROR_ZEROWAITING:
            return sizeof(TP_ERRMESSAGE_ZEROWAITING);
        case TP_ERROR_UNKNOWN:
            return sizeof(TP_ERRMESSAGE_UNKNOWN);
    }
}

char * tp_utils_errormessage(tp_error error, char *buffer) {
    switch (error) {
        case TP_ERROR_BADARG:
            return strcpy(buffer, TP_ERRMESSAGE_BADARG);
        case TP_ERROR_BADCONFIG:
            return strcpy(buffer, TP_ERRMESSAGE_BADCONFIG);
        case TP_ERROR_ISLOCKED:
            return strcpy(buffer, TP_ERRMESSAGE_ISLOCKED);
        case TP_ERROR_ISNOTLOCKED:
            return strcpy(buffer, TP_ERRMESSAGE_ISNOTLOCKED);
        case TP_ERROR_ISRUNNING:
            return strcpy(buffer, TP_ERRMESSAGE_ISRUNNING);
        case TP_ERROR_ISSHUTDOWN:
            return strcpy(buffer, TP_ERRMESSAGE_ISSHUTDOWN);
        case TP_ERROR_SHUTTINGDOWN:
            return strcpy(buffer, TP_ERRMESSAGE_SHUTTINGDOWN);
        case TP_ERROR_LOCKEDELSEWHERE:
            return strcpy(buffer, TP_ERRMESSAGE_LOCKEDELSEWHERE);
        case TP_ERROR_NOMEMORY:
            return strcpy(buffer, TP_ERRMESSAGE_NOMEMORY);
        case TP_ERROR_NOPERM:
            return strcpy(buffer, TP_ERRMESSAGE_NOPERM);
        case TP_ERROR_OK:
            return strcpy(buffer, TP_ERRMESSAGE_OK);
        case TP_ERROR_SYSRES:
            return strcpy(buffer, TP_ERRMESSAGE_SYSRES);
        case TP_ERROR_TIMEOUT:
            return strcpy(buffer, TP_ERRMESSAGE_TIMEOUT);
        case TP_ERROR_ZEROWAITING:
            return strcpy(buffer, TP_ERRMESSAGE_ZEROWAITING);
        case TP_ERROR_UNKNOWN:
            return strcpy(buffer, TP_ERRMESSAGE_UNKNOWN);
    }
}

clock_t tp_utils_sectoclock(double seconds) {
    double count = seconds * CLOCKS_PER_SEC;
    if (count <= 0) {
        return 0;
    }
    return (clock_t) count;
}

struct tp_threadpool {
    tp_config config;
    tpc_manifest manifest;
    tpq_queue queue;
    tpw_gen gen;
    bool is_locked;
    bool is_running;
};

size_t tp_info_sizeofthreadpool() {
    return sizeof(tp_threadpool);
}

bool tp_utils_statsareclear(tp_stats stats) {
    return stats.num_tasks_queued == 0 && stats.num_workers_waiting == stats.num_workers_total ? true : false;
}

tp_stats tpfromtpi_stats(tpi_stats stats) {
    tp_stats proc = {
        .num_workers_total = stats.num_workers,
        .num_tasks_queued = stats.num_queued,
        .num_workers_waiting = stats.num_workers - stats.num_busy,
        .num_tasks_performed = stats.num_complete,
        .num_tasks_succeeded = stats.num_success,
        .cpu_seconds = stats.cpu_time
    };
    return proc;
}

tp_error tpfromtpi_error(tpi_error error) {
    switch (error) {
        case TPI_ERROR_BADARG:
            return TP_ERROR_BADARG;
        case TPI_ERROR_NOMEMORY:
            return TP_ERROR_NOMEMORY;
        case TPI_ERROR_NOPERM:
            return TP_ERROR_NOPERM;
        case TPI_ERROR_OK:
            return TP_ERROR_OK;
        case TPI_ERROR_SYSRES:
            return TP_ERROR_SYSRES;
        case TPI_ERROR_TIMEOUT:
            return TP_ERROR_TIMEOUT;
        case TPI_ERROR_UNKNOWN:
            return TP_ERROR_UNKNOWN;
    }
}

tpi_schedule tpifromtp_schedule(tp_schedule schedule) {
    switch (schedule) {
        case TP_SCHEDULE_DEFAULT:
            return TPI_SCHEDULE_DEFAULT;
        case TP_SCHEDULE_FIFO:
            return TPI_SCHEDULE_FIFO;
        case TP_SCHEDULE_ROUNDROBIN:
            return TPI_SCHEDULE_ROUNDROBIN;
    }
}

tpi_contentionscope tpifromtp_scope(tp_contentionscope scope) {
    switch (scope) {
        case TP_CONTENTIONSCOPE_DEFAULT:
            return TPI_CONTENTIONSCOPE_DEFAULT;
        case TP_CONTENTIONSCOPE_PROCESS:
            return TPI_CONTENTIONSCOPE_PROCESS;
        case TP_CONTENTIONSCOPE_SYSTEM:
            return TPI_CONTENTIONSCOPE_SYSTEM;
    }
}

void tp_onstatschanged(tpi_stats stats, void *data) {
    tp_threadpool *pool = data;
    if (pool->config.onstatschanged) {
        pool->config.onstatschanged(pool, tpfromtpi_stats(stats));
    }
}

void tp_ontaskfailed(int result, void *taskdata, void *data) {
    tp_threadpool *pool = data;
    if (pool->config.ontaskfailed) {
        pool->config.ontaskfailed(pool, result, taskdata);
    }
}

tp_error tp_create(tp_threadpool **pool, tp_config config) {
    if (tp_utils_evaluateconfig(config)) {
        return TP_ERROR_BADCONFIG;
    }
    *pool = NULL;
    tp_threadpool *holder = malloc(sizeof(tp_threadpool));
    if (!holder) {
        return TP_ERROR_NOMEMORY;
    }
    tpi_error error;
    if ((error = tpc_manifest_init(&holder->manifest, config.onstatschanged ? &tp_onstatschanged : NULL, holder))) {
        free(holder);
        return tpfromtpi_error(error);
    }
    tpq_config qconfig = {
        .manifest = &holder->manifest,
        .schedule = tpifromtp_schedule(config.queueschedule),
        .initial_size = config.initial_queue,
        .resize_limit = config.queue_resize_limit,
        .resize_increment = config.queue_resize_increment
    };
    if ((error = tpq_init(&holder->queue, qconfig))) {
        tpc_manifest_destroy(&holder->manifest);
        free(holder);
        return tpfromtpi_error(error);
    }
    tpw_gen_config gconfig = {
        .stacksize = config.stacksize,
        .guardsize = config.guardsize,
        .schedule = tpifromtp_schedule(config.threadschedule),
        .scope = tpifromtp_scope(config.contentionscope),
        .queue = &holder->queue,
        .minthreads = config.min_threads,
        .ontaskfailed = config.ontaskfailed ? &tp_ontaskfailed : NULL,
        .g_data = holder,
        .userdata = config.userdata
    };
    if ((error = tpw_gen_init(&holder->gen, gconfig))) {
        tpq_destroy(&holder->queue);
        tpc_manifest_destroy(&holder->manifest);
        free(holder);
        return tpfromtpi_error(error);
    }
    holder->is_locked = false;
    holder->is_running = true;
    holder->config = config;
    for (size_t i = 0; i < config.min_threads; i++) {
        if ((error = tpw_gen_generate(&holder->gen))) {
            tp_shutdown(holder);
            tpw_gen_destory(&holder->gen);
            tpq_destroy(&holder->queue);
            tpc_manifest_destroy(&holder->manifest);
            free(holder);
            return tpfromtpi_error(error);
        }
    }
    * pool = holder;
    return TP_ERROR_OK;
}

tp_config tp_getconfig(tp_threadpool *pool) {
    return pool->config;
}

tp_stats tp_getstats(tp_threadpool *pool) {
    tpc_manifest_acquire(&pool->manifest);
    tp_stats stats = tpfromtpi_stats(tpc_manifest_stats(&pool->manifest));
    tpc_manifest_release(&pool->manifest);
    return stats;
}

bool tp_isrunning(tp_threadpool *pool) {
    return pool->is_running;
}

bool tp_islocked(tp_threadpool *pool) {
    return pool->is_locked;
}

bool tp_isclear(tp_threadpool *pool) {
    tpc_manifest_acquire(&pool->manifest);
    bool clear = tp_utils_statsareclear(tpfromtpi_stats(tpc_manifest_stats(&pool->manifest)));
    tpc_manifest_release(&pool->manifest);
    return clear;
}

void * tp_userdata(tp_threadpool *pool) {
    return pool->config.userdata;
}

tp_error tp_enqueue(tp_threadpool *pool, tp_task task, void *taskdata, bool *wasenqueued) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_manifest_acquire(&pool->manifest);
    tpq_acquire(&pool->queue);
    tpi_task entry = {
        .taskdata = taskdata,
        .work = task
    };
    tpi_error error = tpq_enqueue(&pool->queue, entry);
    if (error) {
        *wasenqueued = false;
        tpc_manifest_release(&pool->manifest);
        tpq_release(&pool->queue);
        return tpfromtpi_error(error);
    }
    *wasenqueued = true;
    size_t max_threads = pool->config.min_threads + pool->config.more_threads;
    size_t waiting = pool->manifest.num_workers.count - pool->manifest.num_busy.count;
    if (pool->manifest.num_workers.count < max_threads && waiting == 0) {
        error = tpw_gen_generate(&pool->gen);
    }
    tpc_manifest_release(&pool->manifest);
    tpq_release(&pool->queue);
    return tpfromtpi_error(error);
}

tp_error tp_waitforclear(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_manifest_acquire(&pool->manifest);
    tpc_manifest_waitfor(&pool->manifest, TPC_TARGET_QUEUED, TPC_EVENT_ZERO);
    tpc_manifest_waitfor(&pool->manifest, TPC_TARGET_BUSY, TPC_EVENT_ZERO);
    tpc_manifest_release(&pool->manifest);
    return TP_ERROR_OK;
}

tp_error tp_timedwaitforclear(tp_threadpool *pool, size_t millis) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_manifest_acquire(&pool->manifest);
    size_t start = tpu_millitime();
    if (!tpc_manifest_timedwaitfor(&pool->manifest, TPC_TARGET_QUEUED, TPC_EVENT_ZERO, millis)) {
        tpc_manifest_release(&pool->manifest);
        return TP_ERROR_TIMEOUT;
    }
    size_t remainder = millis - (tpu_millitime() - start);
    if (millis < remainder) {
        tpc_manifest_release(&pool->manifest);
        return TP_ERROR_TIMEOUT;
    }
    if (!tpc_manifest_timedwaitfor(&pool->manifest, TPC_TARGET_BUSY, TPC_EVENT_ZERO, remainder)) {
        tpc_manifest_release(&pool->manifest);
        return TP_ERROR_TIMEOUT;
    }
    tpc_manifest_release(&pool->manifest);
    return TP_ERROR_OK;
}

tp_error tp_waitforqempty(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_manifest_acquire(&pool->manifest);
    tpc_manifest_waitfor(&pool->manifest, TPC_TARGET_QUEUED, TPC_EVENT_ZERO);
    tpc_manifest_release(&pool->manifest);
    return TP_ERROR_OK;
}

tp_error tp_timedwaitforqempty(tp_threadpool *pool, size_t millis) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_manifest_acquire(&pool->manifest);
    bool success = tpc_manifest_timedwaitfor(&pool->manifest, TPC_TARGET_QUEUED, TPC_EVENT_ZERO, millis);
    tpc_manifest_release(&pool->manifest);
    return success ? TP_ERROR_OK : TP_ERROR_TIMEOUT;
}

tpc_event tpcfromtp_change(tp_change change) {
    switch (change) {
        case TP_CHANGE_DECREMENT:
            return TPC_EVENT_DECREMENT;
        case TP_CHANGE_INCREMENT:
            return TPC_EVENT_INCREMENT;
        case TP_CHANGE_ZERO:
            return TPC_EVENT_ZERO;
    }
}

tpc_target tpcfromtp_component(tp_component component) {
    switch (component) {
        case TP_COMPONENT_BUSY:
            return TPC_TARGET_BUSY;
        case TP_COMPONENT_QUEUED:
            return TPC_TARGET_QUEUED;
        case TP_COMPONENT_WAITING:
            return TPC_TARGET_BUSY;
        case TP_COMPONENT_WORKERS:
            return TPC_TARGET_WORKERS;
    }
}

tp_error tp_waitforchange(tp_threadpool *pool, tp_component component, tp_change change) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_target target = tpcfromtp_component(component);
    tpc_event event = tpcfromtp_change(change);
    if (component == TP_COMPONENT_WAITING) {
        switch (change) {
            case TP_CHANGE_DECREMENT:
                event = TPC_EVENT_INCREMENT;
                break;
            case TP_CHANGE_INCREMENT:
                event = TPC_EVENT_DECREMENT;
                break;
            case TP_CHANGE_ZERO:
                return TP_ERROR_ZEROWAITING;
        }
    }
    tpc_manifest_acquire(&pool->manifest);
    tpc_manifest_waitfor(&pool->manifest, target, event);
    tpc_manifest_release(&pool->manifest);
    return TP_ERROR_OK;
}

tp_error tp_timedwaitforchange(tp_threadpool *pool, tp_component component, tp_change change, size_t millis) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpc_target target = tpcfromtp_component(component);
    tpc_event event = tpcfromtp_change(change);
    if (component == TP_COMPONENT_WAITING) {
        switch (change) {
            case TP_CHANGE_DECREMENT:
                event = TPC_EVENT_INCREMENT;
                break;
            case TP_CHANGE_INCREMENT:
                event = TPC_EVENT_DECREMENT;
                break;
            case TP_CHANGE_ZERO:
                return TP_ERROR_ZEROWAITING;
        }
    }
    tpc_manifest_acquire(&pool->manifest);
    bool success = tpc_manifest_timedwaitfor(&pool->manifest, target, event, millis);
    tpc_manifest_release(&pool->manifest);
    return success ? TP_ERROR_OK : TP_ERROR_TIMEOUT;
}

tp_error tp_lock(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpq_acquire(&pool->queue);
    pool->is_locked = true;
    return TP_ERROR_OK;
}

tp_error tp_unlock(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (!pool->is_locked) {
        return TP_ERROR_ISNOTLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    int result = pthread_mutex_unlock(&pool->queue.mutex);
    switch (result) {
        case 0:
            pool->is_locked = false;
            return TP_ERROR_OK;
        case EPERM:
            return TP_ERROR_LOCKEDELSEWHERE;
        default:
            return TP_ERROR_UNKNOWN;
    }
}

tp_error tp_shutdown(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (!pool->is_running) {
        return TP_ERROR_ISSHUTDOWN;
    }
    if (pool->is_locked) {
        return TP_ERROR_ISLOCKED;
    }
    if (pool->queue.instruction) {
        return TP_ERROR_SHUTTINGDOWN;
    }
    tpq_acquire(&pool->queue);
    tpc_manifest_acquire(&pool->manifest);
    if (pool->queue.instruction) {
        tpq_release(&pool->queue);
        tpc_manifest_release(&pool->manifest);
        return TP_ERROR_ISSHUTDOWN;
    }
    pool->queue.instruction = TPI_INSTR_SHUTDOWN;
    pthread_cond_broadcast(&pool->queue.cond);
    tpq_release(&pool->queue);
    tpc_manifest_waitfor(&pool->manifest, TPC_TARGET_WORKERS, TPC_EVENT_ZERO);
    tpc_manifest_release(&pool->manifest);
    pool->is_running = false;
    return TP_ERROR_OK;
}

tp_error tp_destroy(tp_threadpool *pool) {
    if (pool == NULL) {
        return TP_ERROR_BADARG;
    }
    if (pool->is_running) {
        return TP_ERROR_ISRUNNING;
    }
    tpc_manifest_destroy(&pool->manifest);
    tpq_destroy(&pool->queue);
    tpw_gen_destory(&pool->gen);
    free(pool);
    return TP_ERROR_OK;
}
