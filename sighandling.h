#ifndef sighandling_h
#define sighandling_h

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>

#ifndef SIGPOLL
#ifdef SIGIO
#define SIGPOLL SIGIO
static const bool tp_sigpoll_is_defined = true;
#else
static const bool tp_sigpoll_is_defined = false;
#endif
#else
static const bool tp_sigpoll_is_defined = true;
#endif

#define TP_SIG_MESSAGE_MAXSIZE 96

#ifdef SIGPOLL
#define TP_SIG_NUMSIGSHANDLED 20
#else
#define TP_SIG_NUMSIGSHANDLED 19
#endif

typedef enum {
    TP_TERMSIGNAL_HANGUP = SIGHUP,
    TP_TERMSIGNAL_INTERRUPT = SIGINT,
    TP_TERMSIGNAL_QUIT = SIGQUIT,
    TP_TERMSIGNAL_ILLEGAL = SIGILL,
    TP_TERMSIGNAL_ABORT = SIGABRT,
    TP_TERMSIGNAL_FLOATMATH = SIGFPE,
    TP_TERMSIGNAL_MEMADDRESS = SIGSEGV,
    TP_TERMSIGNAL_BROKENPIPE = SIGPIPE,
    TP_TERMSIGNAL_ALARM = SIGALRM,
    TP_TERMSIGNAL_TERMINATE = SIGTERM,
    TP_TERMSIGNAL_CUSTOMONE = SIGUSR1,
    TP_TERMSIGNAL_CUSTOMTWO = SIGUSR2,
    TP_TERMSIGNAL_MEMACCESS = SIGBUS,
#ifdef SIGPOLL
    TP_TERMSIGNAL_POLL = SIGPOLL,
#endif
    TP_TERMSIGNAL_PROFTIMER = SIGPROF,
    TP_TERMSIGNAL_BADARG = SIGSYS,
    TP_TERMSIGNAL_TRAP = SIGTRAP,
    TP_TERMSIGNAL_VIRTALARM = SIGVTALRM,
    TP_TERMSIGNAL_CPUTIME = SIGXCPU,
    TP_TERMSIGNAL_FILESIZE = SIGXFSZ,
    TP_TERMSIGNAL_UNKNOWN
} tp_termsignal;

typedef struct {
    int signum;
    struct sigaction *oldaction;
    siginfo_t *info;
    ucontext_t *context;
} tp_signal_details;

int tp_sigutils_beginsafehandling();
int tp_sigutils_endsafehandling();

tp_termsignal tp_sigutils_sigfromsignum(int signum);
bool tp_sigutils_doessignalcoredump(tp_termsignal signal);
char * tp_sigutils_message(tp_termsignal signal, char *buffer);
int tp_sigutils_printmessage(tp_termsignal signal, FILE *stream);

typedef bool (* tp_sigutils_handler)(tp_termsignal signal, tp_signal_details *details, void *userdata);

int tp_sigutils_sethandler_detailed(tp_sigutils_handler handler, sigset_t mask, int flags, void *userdata);
int tp_sigutils_sethandler(tp_sigutils_handler handler, void *userdata);
int tp_sigutils_restorenative();
    
#if defined(__cplusplus)
}
#endif

#endif
