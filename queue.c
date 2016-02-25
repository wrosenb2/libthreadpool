#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "tpdefs.h"
#include "counting.h"
#include "utilities.h"
#include "queue.h"

tpi_error tpq_init(tpq_queue *queue, tpq_config config) {
    int holder = 0;
    if ((holder = pthread_mutex_init(&queue->mutex, NULL))) {
        return tpu_pthread_to_tpi(holder);
    }
    if ((holder = pthread_cond_init(&queue->cond, NULL))) {
        pthread_mutex_destroy(&queue->mutex);
        return tpu_pthread_to_tpi(holder);
    }
    if ((queue->list = calloc(config.initial_size, sizeof(tpi_task))) == NULL && config.initial_size) {
        pthread_cond_destroy(&queue->cond);
        pthread_mutex_destroy(&queue->mutex);
        return TPI_ERROR_NOMEMORY;
    }
    queue->manifest = config.manifest;
    queue->schedule = config.schedule == TPI_SCHEDULE_DEFAULT ? TPI_SCHEDULE_FIFO : config.schedule;
    queue->instruction = TPI_INSTR_PROCEED;
    queue->count = 0;
    queue->length = config.initial_size;
    return TPI_ERROR_OK;
}

void tpq_acquire(tpq_queue *queue) {
    pthread_mutex_lock(&queue->mutex);
}

void tpq_release(tpq_queue *queue) {
    pthread_mutex_unlock(&queue->mutex);
}

tpi_error tpq_enqueue(tpq_queue *queue, tpi_task task) {
    if (queue->count < queue->length) {
        queue->list[queue->count] = task;
        queue->count++;
    } else {
        size_t nc = queue->count + queue->resize_increment;
        tpi_task *nl = calloc(nc, sizeof(tpi_task));
        if (!nl) {
            return TPI_ERROR_NOMEMORY;
        }
        for (size_t i = 0; i < queue->count; i++) {
            nl[i] = queue->list[i];
        }
        nl[queue->count] = task;
        free(queue->list);
        queue->count++;
        queue->length = nc;
        queue->list = nl;
    }
    tpc_manifest_increment(queue->manifest, TPC_TARGET_QUEUED);
    pthread_cond_signal(&queue->cond);
    return TPI_ERROR_OK;
}

bool tpq_extract(tpq_queue *queue, tpi_task *task) {
    if (queue->count == 0) {
        return false;
    }
    size_t index = queue->schedule == TPI_SCHEDULE_FIFO ? 0 : tpu_get_random_index(queue->count);
    * task = queue->list[index];
    queue->count--;
    for (size_t i = index; i < queue->count; i++) {
        queue->list[i] = queue->list[i + 1];
    }
    if (queue->resize_limit <= queue->length - queue->count) {
        if (queue->count == 0) {
            free(queue->list);
            queue->length = 0;
        } else {
            tpi_task *nl = calloc(queue->count, sizeof(tpi_task));
            if (nl) {
                for (size_t i = 0; i < queue->count; i++) {
                    nl[i] = queue->list[i];
                }
                free(queue->list);
                queue->list = nl;
                queue->length = queue->count;
            }
        }
    }
    tpc_manifest_decrement(queue->manifest, TPC_TARGET_QUEUED);
    return true;
}

void tpq_wait(tpq_queue *queue) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
}

void tpq_destroy(tpq_queue *queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue->list);
    queue->count = queue->length = 0;
}