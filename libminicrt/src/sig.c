// ++C
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <sig.h>

#if defined(XXC_RAWSYS)
#include <xxc_sys.h>

#if defined(__linux__)

#if defined(__x86_64__)
extern void __restore_rt(void);
#define SIG_HAVE_RESTORER 1
#else
#define SIG_HAVE_RESTORER 0
#endif

typedef struct {
    sighandler_t   handler;
    unsigned long  flags;
    void         (*restorer)(void);
    sigset_t       mask;
} k_sigaction_t;

#endif /* __linux__ */
#endif /* XXC_RAWSYS */

void sigemptyset(sigset_t *set)
{
    if (set != nullptr) set->bits = 0;
}

void sigaddset(sigset_t *set, int signum)
{
    if (set == nullptr || signum < 1 || signum > 64) return;
    set->bits |= (u64)1 << (u64)(signum - 1);
}

int sigismember(const sigset_t *set, int signum)
{
    if (set == nullptr || signum < 1 || signum > 64) return 0;
    return (int)((set->bits >> (u64)(signum - 1)) & 1);
}

#if defined(__FreeBSD__)

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact)
{
    xxc_ksigaction_t kact = {0};
    xxc_ksigaction_t kold = {0};

    if (signum < 1 || signum >= NSIG || signum == SIGKILL || signum == SIGSTOP)
        return -1;

    if (act)
    {
        kact.handler = act->sa_handler;
        kact.flags   = (int)act->sa_flags;
        kact.mask.bits[0] = (u32)(act->sa_mask.bits & 0xffffffffu);
        kact.mask.bits[1] = (u32)(act->sa_mask.bits >> 32);
    }

    long ret = sys_sigaction(signum, act ? &kact : (void *)0, oldact ? &kold : (void *)0);
    if (ret < 0) return -1;

    if (oldact)
    {
        oldact->sa_handler  = kold.handler;
        oldact->sa_flags    = (unsigned long)(unsigned int)kold.flags;
        oldact->sa_restorer = (void (*)(void))0;
        oldact->sa_mask.bits = (u64)kold.mask.bits[0] | ((u64)kold.mask.bits[1] << 32);
    }
    return 0;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    sigaction_t act = {0};
    sigaction_t old = {0};

    act.sa_handler = handler;
    act.sa_flags   = SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old) < 0)
        return SIG_ERR;
    return old.sa_handler;
}

int raise(int sig)
{
    long pid = sys_getpid();
    if (pid < 0) return -1;
    return (sys_kill(pid, sig) < 0) ? -1 : 0;
}

#elif defined(__linux__)

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact)
{
    k_sigaction_t kact = {0};
    k_sigaction_t koldact = {0};

    if (signum < 1 || signum >= NSIG || signum == SIGKILL || signum == SIGSTOP)
        return -1;

    if (act)
    {
        kact.handler = act->sa_handler;
        kact.flags   = act->sa_flags;
        kact.mask    = act->sa_mask;

#if SIG_HAVE_RESTORER
        kact.flags     |= 0x04000000;
        kact.restorer   = __restore_rt;
#endif
    }

    long ret = sys_rt_sigaction(
        signum,
        act ? &kact : (void *)0,
        oldact ? &koldact : (void *)0,
        sizeof(u64)
    );

    if (ret < 0) return -1;

    if (oldact)
    {
        oldact->sa_handler  = koldact.handler;
        oldact->sa_flags    = koldact.flags;
        oldact->sa_restorer = koldact.restorer;
        oldact->sa_mask     = koldact.mask;
    }

    return 0;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    sigaction_t act = {0};
    sigaction_t old = {0};

    act.sa_handler = handler;
    act.sa_flags   = SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old) < 0)
        return SIG_ERR;

    return old.sa_handler;
}

int raise(int sig)
{
    long pid = sys_getpid();
    if (pid < 0) return -1;
    return (sys_kill(pid, sig) < 0) ? -1 : 0;
}

#elif defined(_WIN32)
#include <windows.h>

static sigaction_t g_sigactions[65];
static SRWLOCK g_sig_lock = SRWLOCK_INIT;
static bool g_handlers_installed = false;
static PVOID g_veh_handle = nullptr;

static LONG WINAPI vectored_exc_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    int signum = 0;
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_STACK_OVERFLOW:
            signum = SIGSEGV; 
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
            signum = SIGILL; 
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_OVERFLOW:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_UNDERFLOW:
        case EXCEPTION_FLT_INVALID_OPERATION:
            signum = SIGFPE; 
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }

    AcquireSRWLockShared(&g_sig_lock);
    sighandler_t handler = g_sigactions[signum].sa_handler;
    ReleaseSRWLockShared(&g_sig_lock);

    if (handler == SIG_DFL || handler == SIG_IGN)
        return EXCEPTION_CONTINUE_SEARCH;

    handler(signum);

    return EXCEPTION_CONTINUE_EXECUTION;
}

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    int signum = 0;
    switch (ctrl_type)
    {
        case CTRL_C_EVENT:
            signum = SIGINT; 
            break;
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            signum = SIGTERM; 
            break;
        default:
            return FALSE;
    }

    AcquireSRWLockShared(&g_sig_lock);
    sighandler_t handler = g_sigactions[signum].sa_handler;
    ReleaseSRWLockShared(&g_sig_lock);

    if (handler == SIG_DFL)
        return FALSE;
    else if (handler == SIG_IGN)
        return TRUE;

    handler(signum);
    return TRUE;
}

static void ensure_win32_handlers(void)
{
    if (g_handlers_installed) return;

    AcquireSRWLockExclusive(&g_sig_lock);
    if (!g_handlers_installed)
    {
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        g_veh_handle = AddVectoredExceptionHandler(1, vectored_exc_handler);
        g_handlers_installed = true;
    }
    ReleaseSRWLockExclusive(&g_sig_lock);
}

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact)
{
    if (signum < 1 || signum >= NSIG || signum == SIGKILL || signum == SIGSTOP)
        return -1;

    ensure_win32_handlers();

    AcquireSRWLockExclusive(&g_sig_lock);
    
    if (oldact)
        *oldact = g_sigactions[signum];
    
    if (act)
        g_sigactions[signum] = *act;

    ReleaseSRWLockExclusive(&g_sig_lock);
    return 0;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    sigaction_t act = {0};
    sigaction_t old = {0};

    act.sa_handler = handler;
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#endif
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old) < 0)
        return SIG_ERR;

    return old.sa_handler;
}

int raise(int sig)
{
    if (sig < 1 || sig >= NSIG)
        return -1;

    AcquireSRWLockShared(&g_sig_lock);
    sighandler_t handler = g_sigactions[sig].sa_handler;
    unsigned long flags = g_sigactions[sig].sa_flags;
    ReleaseSRWLockShared(&g_sig_lock);

    if (handler == SIG_IGN)
        return 0;

    if (handler != SIG_DFL)
    {
        if (flags & SA_RESETHAND)
        {
            AcquireSRWLockExclusive(&g_sig_lock);
            g_sigactions[sig].sa_handler = SIG_DFL;
            ReleaseSRWLockExclusive(&g_sig_lock);
        }
        handler(sig);
        return 0;
    }

    switch (sig)
    {
        case SIGKILL:
        case SIGTERM:
        case SIGINT:
        case SIGSEGV:
        case SIGILL:
        case SIGFPE:
            TerminateProcess(GetCurrentProcess(), 3);
            return 0;
        default:
            return 0;
    }
}

#else

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact)
{
    (void)signum; (void)act; (void)oldact;
    return -1;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    (void)signum; (void)handler;
    return SIG_ERR;
}

int raise(int sig)
{
    (void)sig;
    return -1;
}

#endif
