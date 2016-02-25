#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include "tpdefs.h"
#include "utilities.h"
#include "counting.h"
#include "queue.h"
#include "worker.h"

void * worker_routine(void *data) {
    tpw_worker *self = data;
    tpi_task next;
    tpc_manifest_acquire(self->queue->manifest);
    tpc_manifest_increment(self->queue->manifest, TPC_TARGET_WORKERS);
    tpc_manifest_release(self->queue->manifest);
    while (true) {
        tpq_acquire(self->queue);
        if (self->queue->instruction) {
            tpq_release(self->queue);
            break;
        }
        tpc_manifest_acquire(self->queue->manifest);
        if (tpq_extract(self->queue, &next)) {
            tpq_release(self->queue);
            tpc_manifest_increment(self->queue->manifest, TPC_TARGET_BUSY);
            tpc_manifest_release(self->queue->manifest);
            clock_t start = clock();
            int result = next.work(next.taskdata, self->userdata);
            clock_t end = clock();
            if (result && self->ontaskfailed) {
                self->ontaskfailed(result, next.taskdata, self->g_data);
            }
            tpc_manifest_acquire(self->queue->manifest);
            tpc_manifest_tallyresult(self->queue->manifest, result, end - start);
            tpc_manifest_decrement(self->queue->manifest, TPC_TARGET_BUSY);
            tpc_manifest_release(self->queue->manifest);
        } else {
            if (self->minthreads < self->queue->manifest->num_workers.count) {
                tpq_release(self->queue);
                tpc_manifest_release(self->queue->manifest);
                break;
            }
            tpc_manifest_release(self->queue->manifest);
            tpq_wait(self->queue);
            if (self->queue->instruction) {
                tpq_release(self->queue);
                break;
            }
            tpc_manifest_acquire(self->queue->manifest);
            if (tpq_extract(self->queue, &next)) {
                tpq_release(self->queue);
                tpc_manifest_increment(self->queue->manifest, TPC_TARGET_BUSY);
                tpc_manifest_release(self->queue->manifest);
                clock_t start = clock();
                int result = next.work(next.taskdata, self->userdata);
                clock_t end = clock();
                if (result && self->ontaskfailed) {
                    self->ontaskfailed(result, next.taskdata, self->g_data);
                }
                tpc_manifest_acquire(self->queue->manifest);
                tpc_manifest_tallyresult(self->queue->manifest, result, end - start);
                tpc_manifest_decrement(self->queue->manifest, TPC_TARGET_BUSY);
                tpc_manifest_release(self->queue->manifest);
            } else {
                tpq_release(self->queue);
                tpc_manifest_release(self->queue->manifest);
            }
        }
    }
    tpc_manifest *manifest = self->queue->manifest;
    free(self);
    tpc_manifest_acquire(manifest);
    tpc_manifest_decrement(manifest, TPC_TARGET_WORKERS);
    tpc_manifest_release(manifest);
    pthread_exit(NULL);
}

tpi_error tpw_gen_init(tpw_gen *gen, tpw_gen_config config) {
    bzero(gen, sizeof(tpw_gen));
    tpi_error result = tpu_attr_init(&gen->attr, config.stacksize, config.guardsize, config.schedule, config.scope);
    if (result) {
        return result;
    }
    gen->queue = config.queue;
    gen->minthreads = config.minthreads;
    gen->ontaskfailed = config.ontaskfailed;
    gen->g_data = config.g_data;
    gen->userdata = config.userdata;
    return TPI_ERROR_OK;
}

tpi_error tpw_gen_generate(tpw_gen *gen) {
    tpw_worker *worker = malloc(sizeof(tpw_worker));
    if (!worker) {
        return TPI_ERROR_NOMEMORY;
    }
    bzero(worker, sizeof(tpw_worker));
    worker->queue = gen->queue;
    worker->minthreads = gen->minthreads;
    worker->ontaskfailed = gen->ontaskfailed;
    worker->g_data = gen->g_data;
    worker->userdata = gen->userdata;
    int result = pthread_create(&worker->thread, &gen->attr, &worker_routine, worker);
    if (!result) {
        free(worker);
        return tpu_pthread_to_tpi(result);
    }
    return TPI_ERROR_OK;
}

void tpw_gen_destory(tpw_gen *gen) {
    pthread_attr_destroy(&gen->attr);
    bzero(gen, sizeof(tpw_gen));
}