#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "tpdefs.h"
#include "utilities.h"
#include "threadpool.h"
#include "sighandling.h"

void printHeader(const char *text, ...) {
    printf("\n\033[0;1m");
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    printf("\033[0m\n");
}

void printNowTrying(const char *text, ...) {
    printf("\n\033[34;1m");
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    printf("\033[0m\n");
}

void printGoodResult(const char *text, ...) {
    printf("\n\033[32;1m");
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    printf("\033[0m\n");
}

void printBadResult(const char *text, ...) {
    printf("\n\033[35;1m");
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    printf("\033[0m\n");
}

void printEval(tp_configeval eval) {
    fprintf(stderr, "\n\033[31;1mError: Type = tp_configeval, Code = %d\nMessage: ", eval);
    char buffer[TP_MESSAGE_MAXSIZE];
    tp_utils_evalmessage(eval, buffer);
    fprintf(stderr, "\033[35;1m%s\033[0m\n", buffer);
}

void printError(tp_error error) {
    fprintf(stderr, "\n\033[31;1mError: Type = tp_error, Code = %d\nMessage: ", error);
    char buffer[TP_MESSAGE_MAXSIZE];
    tp_utils_errormessage(error, buffer);
    fprintf(stderr, "\033[35;1m%s\033[0m\n", buffer);
}

void printSignal(tp_termsignal signal) {
    fprintf(stderr, "\n\033[31;1mError: Type = tp_termsignal, Code = %d\nMessage: ", signal);
    char buffer[TP_MESSAGE_MAXSIZE];
    tp_sigutils_message(signal, buffer);
    fprintf(stderr, "\033[35;1m%s\033[0m\n", buffer);
}

void printErrno(int errnum) {
    fprintf(stderr, "\n\033[31;1mError: Type = system errno, Code = %d\nMessage: ", errnum);
    char buffer[1024];
    strerror_r(errnum, buffer, 1024);
    fprintf(stderr, "\033[35;1m%s\033[0m\n", buffer);
}

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t thread;
} msemaphore;

int msemaphore_init(msemaphore *phore) {
    int result = pthread_mutex_init(&phore->mutex, NULL);
    if (result) {
        return result;
    }
    result = pthread_cond_init(&phore->cond, NULL);
    if (result) {
        pthread_mutex_destroy(&phore->mutex);
    }
    return result;
}

void msemaphore_destroy(msemaphore *phore) {
    pthread_mutex_destroy(&phore->mutex);
    pthread_cond_destroy(&phore->cond);
}

bool prelim_signal_handler(tp_termsignal signal, tp_signal_details *details, void *userdata) {
    msemaphore *phore = (msemaphore *) userdata;
    pthread_cancel(phore->thread);
    pthread_mutex_lock(&phore->mutex);
    printSignal(signal);
    pthread_cond_signal(&phore->cond);
    pthread_mutex_unlock(&phore->mutex);
    return false;
}

void * thread_routine(void *data) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    msemaphore *phore = (msemaphore *) data;
    pthread_mutex_lock(&phore->mutex);
    pthread_mutex_unlock(&phore->mutex);
    long num = 492803756;
    long *ptr;
    *ptr = num;
    pthread_exit(NULL);
}

bool test_signal_handling() {
    msemaphore phore;
    int result = msemaphore_init(&phore);
    if (result) {
        printErrno(result);
        printBadResult("Failed to allocate mutex/cond pair.");
        return false;
    }
    result = tp_sigutils_beginsafehandling();
    if (result) {
        msemaphore_destroy(&phore);
        printErrno(result);
        printBadResult("Failed to setup signal handling infrastructure.");
        return false;
    }
    pthread_mutex_lock(&phore.mutex);
    result = tp_sigutils_sethandler(&prelim_signal_handler, &phore);
    if (result) {
        pthread_mutex_unlock(&phore.mutex);
        msemaphore_destroy(&phore);
        tp_sigutils_endsafehandling();
        printErrno(result);
        printBadResult("Failed to set signal handlers.");
        return false;
    }
    printNowTrying("Now attempting to cause and catch a SIGSEGV exception (code = %d).", SIGSEGV);
    if ((result = pthread_create(&phore.thread, NULL, &thread_routine, &phore))) {
        pthread_mutex_unlock(&phore.mutex);
        msemaphore_destroy(&phore);
        tp_sigutils_endsafehandling();
        printErrno(result);
        printBadResult("Failed to create thread in signal test.");
        return false;
    }
    if (!tpu_relative_wait(&phore.cond, &phore.mutex, 6000)) {
        printBadResult("Timeout while waiting to catch SIGBUS error.");
        msemaphore_destroy(&phore);
        tp_sigutils_restorenative();
        tp_sigutils_endsafehandling();
        return false;
    }
    printGoodResult("Error was caught successfully! Signal handling library testing successful.");
    pthread_mutex_unlock(&phore.mutex);
    msemaphore_destroy(&phore);
    tp_sigutils_restorenative();
    tp_sigutils_endsafehandling();
    return true;
}

bool test_utility_functions();
bool test_threadpool();
bool verify_threadpool();

int main() {
    printHeader("Beginning libthreadpool Test Suite");
    printNowTrying("Beginning Test of Signal Handling Library");
    if (!test_signal_handling()) {
        printBadResult("Signal Handling Test Failed. Aborting.");
        return SIGABRT;
    }
    printGoodResult("Signal Handling Functionality Confirmed!");
    printNowTrying("Beginning Test of libthreadpool Utility Functions");
    return 0;
}