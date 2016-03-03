#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "sighandling.h"

#define TP_SIGMESSAGE_HANGUP "hangup detected on controlling terminal or death of controlling process"
#define TP_SIGMESSAGE_INTERRUPT "interrupt from keyboard"
#define TP_SIGMESSAGE_QUIT "quit from keyboard"
#define TP_SIGMESSAGE_ILLEGAL "illegal instruction received"
#define TP_SIGMESSAGE_ABORT "abort signal from call to abort(3)"
#define TP_SIGMESSAGE_FLOATMATH "floating point arithmetic exception"
#define TP_SIGMESSAGE_MEMADDRESS "invalid memory reference"
#define TP_SIGMESSAGE_BROKENPIPE "broken pipe or write to pipe with no readers"
#define TP_SIGMESSAGE_ALARM "timer signal received from call to alarm(2)"
#define TP_SIGMESSAGE_TERMINATE "termination signal from system"
#define TP_SIGMESSAGE_CUSTOMONE "user-defined signal 1"
#define TP_SIGMESSAGE_CUSTOMTWO "user-defined signal 2"
#define TP_SIGMESSAGE_MEMACCESS "bus error - bad access to memory"

#ifdef SIGPOLL
#define TP_SIGMESSAGE_POLL "pollable event encountered"
#endif

#define TP_SIGMESSAGE_PROFTIMER "profiling timer expired"
#define TP_SIGMESSAGE_BADARG "bad argument passed to routine"
#define TP_SIGMESSAGE_TRAP "trace/breakpoint trap reached"
#define TP_SIGMESSAGE_VIRTALARM "virtual alarm clock triggered"
#define TP_SIGMESSAGE_CPUTIME "system imposed CPU time limit exceeded"
#define TP_SIGMESSAGE_FILESIZE "system imposed file size limit exceeded"
#define TP_SIGMESSAGE_UNKNOWN "signal not enumerated in libthreadpool sighandling"

bool tp_sigutils_doessignalcoredump(tp_termsignal signal) {
    switch (signal) {
        case TP_TERMSIGNAL_ABORT:
            return true;
        case TP_TERMSIGNAL_BADARG:
            return true;
        case TP_TERMSIGNAL_CPUTIME:
            return true;
        case TP_TERMSIGNAL_FILESIZE:
            return true;
        case TP_TERMSIGNAL_FLOATMATH:
            return true;
        case TP_TERMSIGNAL_ILLEGAL:
            return true;
        case TP_TERMSIGNAL_MEMACCESS:
            return true;
        case TP_TERMSIGNAL_MEMADDRESS:
            return true;
        case TP_TERMSIGNAL_QUIT:
            return true;
        case TP_TERMSIGNAL_TRAP:
            return true;
        default:
            return false;
    }
}

char * tp_sigutils_message(tp_termsignal signal, char *buffer) {
    switch (signal) {
        case TP_TERMSIGNAL_ABORT:
            return strcpy(buffer, TP_SIGMESSAGE_ABORT);
        case TP_TERMSIGNAL_ALARM:
            return strcpy(buffer, TP_SIGMESSAGE_ALARM);
        case TP_TERMSIGNAL_BADARG:
            return strcpy(buffer, TP_SIGMESSAGE_BADARG);
        case TP_TERMSIGNAL_BROKENPIPE:
            return strcpy(buffer, TP_SIGMESSAGE_BROKENPIPE);
        case TP_TERMSIGNAL_CPUTIME:
            return strcpy(buffer, TP_SIGMESSAGE_CPUTIME);
        case TP_TERMSIGNAL_CUSTOMONE:
            return strcpy(buffer, TP_SIGMESSAGE_CUSTOMONE);
        case TP_TERMSIGNAL_CUSTOMTWO:
            return strcpy(buffer, TP_SIGMESSAGE_CUSTOMTWO);
        case TP_TERMSIGNAL_FILESIZE:
            return strcpy(buffer, TP_SIGMESSAGE_FILESIZE);
        case TP_TERMSIGNAL_FLOATMATH:
            return strcpy(buffer, TP_SIGMESSAGE_FLOATMATH);
        case TP_TERMSIGNAL_HANGUP:
            return strcpy(buffer, TP_SIGMESSAGE_HANGUP);
        case TP_TERMSIGNAL_ILLEGAL:
            return strcpy(buffer, TP_SIGMESSAGE_ILLEGAL);
        case TP_TERMSIGNAL_INTERRUPT:
            return strcpy(buffer, TP_SIGMESSAGE_INTERRUPT);
        case TP_TERMSIGNAL_MEMACCESS:
            return strcpy(buffer, TP_SIGMESSAGE_MEMACCESS);
        case TP_TERMSIGNAL_MEMADDRESS:
            return strcpy(buffer, TP_SIGMESSAGE_MEMADDRESS);
#ifdef SIGPOLL
        case TP_TERMSIGNAL_POLL:
            return strcpy(buffer, TP_SIGMESSAGE_POLL);
#endif
        case TP_TERMSIGNAL_PROFTIMER:
            return strcpy(buffer, TP_SIGMESSAGE_PROFTIMER);
        case TP_TERMSIGNAL_QUIT:
            return strcpy(buffer, TP_SIGMESSAGE_QUIT);
        case TP_TERMSIGNAL_TERMINATE:
            return strcpy(buffer, TP_SIGMESSAGE_TERMINATE);
        case TP_TERMSIGNAL_TRAP:
            return strcpy(buffer, TP_SIGMESSAGE_TRAP);
        case TP_TERMSIGNAL_VIRTALARM:
            return strcpy(buffer, TP_SIGMESSAGE_VIRTALARM);
    }
}

int tp_sigutils_printmessage(tp_termsignal signal, FILE *stream) {
    int result;
    switch (signal) {
        case TP_TERMSIGNAL_ABORT:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_ABORT);
            break;
        case TP_TERMSIGNAL_ALARM:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_ALARM);
            break;
        case TP_TERMSIGNAL_BADARG:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_BADARG);
            break;
        case TP_TERMSIGNAL_BROKENPIPE:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_BROKENPIPE);
            break;
        case TP_TERMSIGNAL_CPUTIME:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_CPUTIME);
            break;
        case TP_TERMSIGNAL_CUSTOMONE:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_CUSTOMONE);
            break;
        case TP_TERMSIGNAL_CUSTOMTWO:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_CUSTOMTWO);
            break;
        case TP_TERMSIGNAL_FILESIZE:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_FILESIZE);
            break;
        case TP_TERMSIGNAL_FLOATMATH:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_FLOATMATH);
            break;
        case TP_TERMSIGNAL_HANGUP:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_HANGUP);
            break;
        case TP_TERMSIGNAL_ILLEGAL:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_ILLEGAL);
            break;
        case TP_TERMSIGNAL_INTERRUPT:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_INTERRUPT);
            break;
        case TP_TERMSIGNAL_MEMACCESS:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_MEMACCESS);
            break;
        case TP_TERMSIGNAL_MEMADDRESS:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_MEMADDRESS);
            break;
#ifdef SIGPOLL
        case TP_TERMSIGNAL_POLL:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_POLL);
            break;
#endif
        case TP_TERMSIGNAL_PROFTIMER:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_PROFTIMER);
            break;
        case TP_TERMSIGNAL_QUIT:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_QUIT);
            break;
        case TP_TERMSIGNAL_TERMINATE:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_TERMINATE);
            break;
        case TP_TERMSIGNAL_TRAP:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_TRAP);
            break;
        case TP_TERMSIGNAL_VIRTALARM:
            result = fprintf(stream, "%s", TP_SIGMESSAGE_VIRTALARM);
            break;
    }
    if (result < 0) {
        return errno;
    }
    return 0;
}

static const tp_termsignal allsignal[] = {
    TP_TERMSIGNAL_HANGUP,
    TP_TERMSIGNAL_INTERRUPT,
    TP_TERMSIGNAL_QUIT,
    TP_TERMSIGNAL_ILLEGAL,
    TP_TERMSIGNAL_ABORT,
    TP_TERMSIGNAL_FLOATMATH,
    TP_TERMSIGNAL_MEMADDRESS,
    TP_TERMSIGNAL_BROKENPIPE,
    TP_TERMSIGNAL_ALARM,
    TP_TERMSIGNAL_TERMINATE,
    TP_TERMSIGNAL_CUSTOMONE,
    TP_TERMSIGNAL_CUSTOMTWO,
    TP_TERMSIGNAL_MEMACCESS,
#ifdef SIGPOLL
    TP_TERMSIGNAL_POLL,
#endif
    TP_TERMSIGNAL_PROFTIMER,
    TP_TERMSIGNAL_BADARG,
    TP_TERMSIGNAL_TRAP,
    TP_TERMSIGNAL_VIRTALARM,
    TP_TERMSIGNAL_CPUTIME,
    TP_TERMSIGNAL_FILESIZE
};

int tp_indexinall(int signum) {
    for (int i = 0; i < TP_SIG_NUMSIGSHANDLED; i++) {
        if (signum == allsignal[i]) {
            return i;
        }
    }
    return -1;
}

tp_termsignal tp_sigutils_sigfromsignum(int signum) {
    switch (signum) {
        case SIGHUP:
            return TP_TERMSIGNAL_HANGUP;
        case SIGINT:
            return TP_TERMSIGNAL_INTERRUPT;
        case SIGQUIT:
            return TP_TERMSIGNAL_QUIT;
        case SIGILL:
            return TP_TERMSIGNAL_ILLEGAL;
        case SIGABRT:
            return TP_TERMSIGNAL_ABORT;
        case SIGFPE:
            return TP_TERMSIGNAL_FLOATMATH;
        case SIGSEGV:
            return TP_TERMSIGNAL_MEMADDRESS;
        case SIGPIPE:
            return TP_TERMSIGNAL_BROKENPIPE;
        case SIGALRM:
            return TP_TERMSIGNAL_ALARM;
        case SIGTERM:
            return TP_TERMSIGNAL_TERMINATE;
        case SIGUSR1:
            return TP_TERMSIGNAL_CUSTOMONE;
        case SIGUSR2:
            return TP_TERMSIGNAL_CUSTOMTWO;
        case SIGBUS:
            return TP_TERMSIGNAL_MEMACCESS;
#ifdef SIGPOLL
        case SIGPOLL:
            return TP_TERMSIGNAL_POLL;
#endif
        case SIGPROF:
            return TP_TERMSIGNAL_PROFTIMER;
        case SIGSYS:
            return TP_TERMSIGNAL_BADARG;
        case SIGTRAP:
            return TP_TERMSIGNAL_TRAP;
        case SIGVTALRM:
            return TP_TERMSIGNAL_VIRTALARM;
        case SIGXCPU:
            return TP_TERMSIGNAL_CPUTIME;
        case SIGXFSZ:
            return TP_TERMSIGNAL_FILESIZE;
        default:
            return TP_TERMSIGNAL_UNKNOWN;
    }
}

pthread_mutex_t mutex;
bool isHandling = false;

typedef struct {
    tp_sigutils_handler handler;
    void *userdata;
    struct sigaction native[TP_SIG_NUMSIGSHANDLED];
} tp_handling_stuff;

tp_handling_stuff stuff;

int tp_sigutils_beginsafehandling() {
    return pthread_mutex_init(&mutex, NULL);
}

int tp_sigutils_endsafehandling() {
    int result = pthread_mutex_lock(&mutex);
    if (result) {
        return result;
    }
    if (isHandling) {
        pthread_mutex_unlock(&mutex);
        return EACCES;
    }
    return pthread_mutex_destroy(&mutex);
}

void tp_plain_handler(int signum) {
    pthread_mutex_lock(&mutex);
    tp_termsignal signal = tp_sigutils_sigfromsignum(signum);
    int index = tp_indexinall(signum);
    tp_signal_details details = {
        .signum = signum,
        .oldaction = index != -1 ? stuff.native + index : NULL,
        .info = NULL,
        .context = NULL
    };
    if (stuff.handler(signal, &details, stuff.userdata)) {
        (stuff.native + index)->sa_handler(signum);
    }
    pthread_mutex_unlock(&mutex);
}

void tp_sigaction_handler(int signum, siginfo_t *info, void *data) {
    pthread_mutex_lock(&mutex);
    tp_termsignal signal = tp_sigutils_sigfromsignum(signum);
    int index = tp_indexinall(signum);
    tp_signal_details details = {
        .signum = signum,
        .oldaction = index != -1 ? stuff.native + index : NULL,
        .info = info,
        .context = data
    };
    if (stuff.handler(signal, &details, stuff.userdata)) {
        (stuff.native + index)->sa_handler(signum);
    }
    pthread_mutex_unlock(&mutex);
}

int tp_sigutils_sethandler_detailed(tp_sigutils_handler handler, sigset_t mask, int flags, void *userdata) {
    if (!handler) {
        return EINVAL;
    }
    int result = pthread_mutex_lock(&mutex);
    if (result) {
        return result;
    }
    struct sigaction action;
    action.sa_flags = flags;
    action.sa_mask = mask;
    action.sa_handler = &tp_plain_handler;
    action.sa_sigaction = &tp_sigaction_handler;
    stuff.handler = handler;
    for (int i = 0; i < TP_SIG_NUMSIGSHANDLED; i++) {
        result = isHandling ? sigaction(allsignal[i], &action, NULL) : sigaction(allsignal[i], &action, stuff.native + i);
        if (result) {
            int error = errno;
            for (int j = i - 1; j >= 0; j--) {
                sigaction(allsignal[j], stuff.native + j, NULL);
            }
            isHandling = false;
            pthread_mutex_unlock(&mutex);
            return error;
        }
    }
    stuff.userdata = userdata;
    stuff.handler = handler;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int tp_sigutils_sethandler(tp_sigutils_handler handler, void *userdata) {
    return tp_sigutils_sethandler_detailed(handler, 0, 0, userdata);
}

int tp_sigutils_restorenative() {
    int result = pthread_mutex_lock(&mutex);
    if (result) {
        return result;
    }
    if (!isHandling) {
        pthread_mutex_unlock(&mutex);
        return EALREADY;
    }
    for (int i = 0; i < TP_SIG_NUMSIGSHANDLED; i++) {
        sigaction(allsignal[i], stuff.native + i, NULL);
    }
    isHandling = false;
    pthread_mutex_unlock(&mutex);
    return 0;
}