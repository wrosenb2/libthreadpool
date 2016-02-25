#ifndef queue_h
#define queue_h

typedef struct {
    tpc_manifest *manifest;
    tpi_schedule schedule;
    size_t initial_size;
    unsigned char resize_limit;
    unsigned char resize_increment;
} tpq_config;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    tpc_manifest *manifest;
    tpi_schedule schedule;
    tpi_task *list;
    tpi_instr instruction;
    unsigned char resize_limit;
    unsigned char resize_increment;
    size_t count;
    size_t length;
} tpq_queue;

tpi_error tpq_init(tpq_queue *queue, tpq_config config);
void tpq_acquire(tpq_queue *queue);
void tpq_release(tpq_queue *queue);
tpi_error tpq_enqueue(tpq_queue *queue, tpi_task task);
bool tpq_extract(tpq_queue *queue, tpi_task *task);
void tpq_wait(tpq_queue *queue);
void tpq_destroy(tpq_queue *queue);

#endif
