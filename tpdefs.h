#ifndef tpdefs_h
#define tpdefs_h

#include <limits.h>

typedef enum {
    TPI_CONTENTIONSCOPE_DEFAULT,
    TPI_CONTENTIONSCOPE_PROCESS,
    TPI_CONTENTIONSCOPE_SYSTEM
} tpi_contentionscope;

typedef enum {
    TPI_SCHEDULE_DEFAULT,
    TPI_SCHEDULE_FIFO,
    TPI_SCHEDULE_ROUNDROBIN
} tpi_schedule;

typedef enum {
    TPI_INSTR_PROCEED = 0,
    TPI_INSTR_SHUTDOWN
} tpi_instr;

typedef struct {
    void *taskdata;
    int (* work)(void *taskdata, void *globaldata);
} tpi_task;

typedef struct {
    size_t num_workers;
    size_t num_busy;
    size_t num_queued;
    size_t num_complete;
    size_t num_success;
    double cpu_time;
} tpi_stats;

typedef enum {
    TPI_ERROR_OK = 0,
    TPI_ERROR_NOMEMORY,
    TPI_ERROR_SYSRES,
    TPI_ERROR_BADARG,
    TPI_ERROR_NOPERM,
    TPI_ERROR_TIMEOUT,
    TPI_ERROR_UNKNOWN
} tpi_error;

#endif
