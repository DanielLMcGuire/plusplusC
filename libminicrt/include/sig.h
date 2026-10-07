#ifndef SIG_H
#define SIG_H

#include <def.h>

#if defined(__FreeBSD__)
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGFPE    8
#define SIGKILL   9
#define SIGBUS    10
#define SIGSEGV   11
#define SIGSYS    12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGURG    16
#define SIGSTOP   17
#define SIGTSTP   18
#define SIGCONT   19
#define SIGCHLD   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGUSR1   30
#define SIGUSR2   31
#else
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#endif

#define NSIG      64

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#if defined(__FreeBSD__)
#define SA_NOCLDSTOP 0x00000008
#define SA_NOCLDWAIT 0x00000020
#define SA_SIGINFO   0x00000040
#define SA_RESTART   0x00000002
#define SA_NODEFER   0x00000010
#define SA_RESETHAND 0x00000004
#else
#define SA_NOCLDSTOP 0x00000001
#define SA_NOCLDWAIT 0x00000002
#define SA_SIGINFO   0x00000004
#define SA_RESTART   0x10000000
#define SA_NODEFER   0x40000000
#define SA_RESETHAND 0x80000000
#endif

typedef struct {
    u64 bits;
} sigset_t;

typedef struct {
    sighandler_t   sa_handler;
    unsigned long  sa_flags;
    void         (*sa_restorer)(void);
    sigset_t       sa_mask;
} sigaction_t;

void sigemptyset(sigset_t *set);
void sigaddset(sigset_t *set, int signum);
int  sigismember(const sigset_t *set, int signum);

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact);

sighandler_t signal(int signum, sighandler_t handler);

int raise(int sig);

#endif /* SIG_H */
