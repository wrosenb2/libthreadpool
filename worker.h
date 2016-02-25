#ifndef worker_h
#define worker_h

typedef struct {
    size_t stacksize;
    size_t guardsize;
    tpi_schedule schedule;
    tpi_contentionscope scope;
    tpq_queue *queue;
    size_t minthreads;
    void (* ontaskfailed)(int, void *, void *);
    void *g_data;
    void *userdata;
} tpw_gen_config;

typedef struct {
    pthread_attr_t attr;
    tpq_queue *queue;
    size_t minthreads;
    void (* ontaskfailed)(int, void *, void *);
    void *g_data;
    void *userdata;
} tpw_gen;

typedef struct {
    pthread_t thread;
    tpq_queue *queue;
    size_t minthreads;
    void (* ontaskfailed)(int, void *, void *);
    void *g_data;
    void *userdata;
} tpw_worker;

tpi_error tpw_gen_init(tpw_gen *gen, tpw_gen_config config);
tpi_error tpw_gen_generate(tpw_gen *gen);
void tpw_gen_destory(tpw_gen *gen);

#endif
