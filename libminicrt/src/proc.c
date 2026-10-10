// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <proc.h>
#include <atexit.h>
#include <mem.h>
#include <str.h>

#if defined(XXC_RAWSYS)
#include <xxc_sys.h>
#include <sys_thread.h>

#define PROC_PATH_MAX 4096

extern void __xxc_heap_fork_prepare(void);
extern void __xxc_heap_fork_release(void);
extern void __xxc_env_fork_prepare(void);
extern void __xxc_env_fork_release(void);
extern void __xxc_cio_fork_child(void);

int proc_pipe(proc_fd_t fds[2], u32 flags)
{
    if (fds == nullptr)
        return -PROC_EINVAL;

    int kflags = 0;
    if (flags & PROC_PIPE_CLOEXEC)  kflags |= O_CLOEXEC;
    if (flags & PROC_PIPE_NONBLOCK) kflags |= O_NONBLOCK;

    int p[2];
    long r = sys_pipe2(p, kflags);
    if (r < 0)
        return (int)r;

    fds[0] = p[0];
    fds[1] = p[1];
    return 0;
}

i64 proc_read(proc_fd_t fd, void *buf, size_t n)
{
    if (n == 0)
        return 0;
    if (buf == nullptr)
        return -PROC_EINVAL;

    for (;;)
    {
        long r = sys_read(fd, buf, n);
        if (r != -PROC_EINTR)
            return r;
    }
}

i64 proc_write(proc_fd_t fd, const void *buf, size_t n)
{
    if (n == 0)
        return 0;
    if (buf == nullptr)
        return -PROC_EINVAL;

    size_t done = 0;
    while (done < n)
    {
        long r = sys_write(fd, (const char *)buf + done, n - done);
        if (r == -PROC_EINTR)
            continue;
        if (r < 0)
            return done ? (i64)done : (i64)r;
        /* GCOV_EXCL_START */
        if (r == 0)
            break;
        /* GCOV_EXCL_STOP */
        done += (size_t)r;
    }
    return (i64)done;
}

int proc_close(proc_fd_t fd)
{
    return sys_close(fd);
}

int proc_dup2(proc_fd_t oldfd, proc_fd_t newfd)
{
    return (int)sys_dup2(oldfd, newfd);
}

proc_pid_t proc_getpid(void)
{
    return (proc_pid_t)sys_getpid();
}

proc_pid_t proc_fork(void)
{
    __xxc_env_fork_prepare();
    __xxc_heap_fork_prepare();
    long r = sys_fork();

    if (r == 0)
    {
        __xxc_cio_fork_child();
        atomic_i32_store(&__xxc_self->tid, (i32)sys_gettid());
    }

    __xxc_heap_fork_release();
    __xxc_env_fork_release();
    return (proc_pid_t)r;
}

static void decode_status(int raw, proc_status_t *out)
{
    int sig = raw & 0x7f;

    out->exited      = (sig == 0);
    out->exit_code   = out->exited ? ((raw >> 8) & 0xff) : 0;
    out->term_signal = out->exited ? 0 : sig;
}

proc_pid_t proc_wait(proc_pid_t pid, proc_status_t *status, u32 flags)
{
    int options = (flags & PROC_WAIT_NOHANG) ? XXC_WNOHANG : 0;
    int raw = 0;
    long r;

    do
    {
        r = sys_wait4(pid, &raw, options);
    } while (r == -PROC_EINTR);

    if (r <= 0)
        return (proc_pid_t)r;

    if (status != nullptr)
        decode_status(raw, status);

    return (proc_pid_t)r;
}

int proc_kill(proc_pid_t pid, int sig)
{
    if (pid <= 0)
        return -PROC_EINVAL;

    return (int)sys_kill(pid, sig);
}

static const char *env_get(char *const *envp, const char *name)
{
    for (; envp != nullptr && *envp != nullptr; envp++)
    {
        const char *e = *envp;
        const char *n = name;

        while (*n != '\0' && *e == *n)
        {
            e++;
            n++;
        }

        if (*n == '\0' && *e == '=')
            return e + 1;
    }
    return nullptr;
}

/* GCOV_EXCL_START */
static long exec_search(const char *file, char *const argv[], char *const envp[])
{
    for (const char *p = file; *p != '\0'; p++)
    {
        if (*p == '/')
            return sys_execve(file, argv, envp);
    }

    const char *path = env_get(envp, "PATH");
    if (path == nullptr)
        path = "/usr/bin:/bin";

    size_t flen   = strlen(file);
    long   result = -PROC_ENOENT;
    char   buf[PROC_PATH_MAX];

    for (const char *seg = path;;)
    {
        const char *end = seg;
        while (*end != '\0' && *end != ':')
            end++;

        size_t dlen = (size_t)(end - seg);
        size_t need = (dlen ? dlen : 1) + 1 + flen + 1;

        if (need <= sizeof(buf))
        {
            size_t o = 0;
            if (dlen == 0)
                buf[o++] = '.';
            else
            {
                memcpy(buf, seg, dlen);
                o = dlen;
            }
            buf[o++] = '/';
            memcpy(buf + o, file, flen);
            buf[o + flen] = '\0';

            long r = sys_execve(buf, argv, envp);

            if (r == -PROC_EACCES)
                result = r;
            else if (r != -PROC_ENOENT && r != -PROC_ENOTDIR)
                return r;
        }

        if (*end == '\0')
            break;
        seg = end + 1;
    }

    return result;
}
/* GCOV_EXCL_STOP */

int proc_spawn(const proc_spawn_t *spec, proc_pid_t *out_pid)
{
    if (spec == nullptr || out_pid == nullptr || spec->path == nullptr || spec->path[0] == '\0')
        return -PROC_EINVAL;

    const proc_fd_t redir[3] = { spec->stdin_fd, spec->stdout_fd, spec->stderr_fd };

    for (int slot = 0; slot < 3; slot++)
    {
        if (redir[slot] < PROC_FD_INHERIT || (redir[slot] >= 0 && redir[slot] < 3 && redir[slot] != slot))
            return -PROC_EINVAL;
    }

    char *default_argv[2] = { (char *)spec->path, nullptr };
    char *const *argv = spec->argv != nullptr ? spec->argv : default_argv;
    char *const *envp = spec->envp != nullptr ? spec->envp : __xxc_environ;

    proc_fd_t status_pipe[2];
    int rc = proc_pipe(status_pipe, PROC_PIPE_CLOEXEC);
    if (rc < 0)
        return rc;

    proc_pid_t pid = proc_fork();

    /* GCOV_EXCL_START */
    if (pid < 0)
    {
        proc_close(status_pipe[0]);
        proc_close(status_pipe[1]);
        return pid;
    }
    /* GCOV_EXCL_STOP */

    if (pid == 0)
    {
        /* GCOV_EXCL_START */
        long err = 0;

        for (int slot = 0; slot < 3 && err >= 0; slot++)
        {
            if (redir[slot] >= 0 && redir[slot] != slot)
                err = sys_dup2(redir[slot], slot);
        }

        if (err >= 0)
        {
            if (spec->flags & PROC_SPAWN_SEARCH_PATH)
                err = exec_search(spec->path, argv, envp);
            else
                err = sys_execve(spec->path, argv, envp);
        }

        int e = (int)err;
        sys_write(status_pipe[1], &e, sizeof(e));
        _exit(127);
        /* GCOV_EXCL_STOP */
    }

    proc_close(status_pipe[1]);

    int child_err = 0;
    i64 got = proc_read(status_pipe[0], &child_err, sizeof(child_err));
    proc_close(status_pipe[0]);

    if (got == (i64)sizeof(child_err))
    {
        proc_wait(pid, nullptr, 0);
        return child_err < 0 ? child_err : -PROC_ENOEXEC;
    }

    *out_pid = pid;
    return 0;
}

#elif defined(_WIN32)

#include <windows.h>
#include <crt_lock.h>
#include <dstr.h>
#include <alloc.h>

#define PROC_WIN_WAIT_MAX 64
#define PROC_KILL_CODE(sig) (0xDEAD0000u | (unsigned)(sig))

static int win_err(DWORD e)
{
    switch (e)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
        return -PROC_ENOENT;
    case ERROR_ACCESS_DENIED:
        return -PROC_EACCES;
    case ERROR_BAD_EXE_FORMAT:
        return -PROC_ENOEXEC;
    case ERROR_BROKEN_PIPE:
    case ERROR_NO_DATA:
        return -PROC_EPIPE;
    case ERROR_TOO_MANY_OPEN_FILES:
        return -PROC_EMFILE;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
        return -PROC_ENOMEM;
    default:
        return -PROC_EINVAL;
    }
}

typedef struct {
    DWORD  pid;
    HANDLE handle;
    int    kill_sig;
} proc_slot;

static proc_slot *g_slots;
static size_t     g_slot_count;
static size_t     g_slot_cap;
static crt_lock_t g_slot_lock = CRT_LOCK_INIT;

static proc_slot *slot_find(DWORD pid)
{
    for (size_t i = 0; i < g_slot_count; i++)
    {
        if (g_slots[i].pid == pid)
            return &g_slots[i];
    }
    return nullptr;
}

static bool slot_add(DWORD pid, HANDLE handle)
{
    bool ok = true;

    __crt_lock_acquire(&g_slot_lock);

    if (g_slot_count == g_slot_cap)
    {
        size_t cap = g_slot_cap ? g_slot_cap * 2 : 8;
        proc_slot *grown = (proc_slot *)realloc(g_slots, cap * sizeof(proc_slot));
        if (grown == nullptr)
            ok = false;
        else
        {
            g_slots    = grown;
            g_slot_cap = cap;
        }
    }

    if (ok)
    {
        g_slots[g_slot_count].pid      = pid;
        g_slots[g_slot_count].handle   = handle;
        g_slots[g_slot_count].kill_sig = 0;
        g_slot_count++;
    }

    __crt_lock_release(&g_slot_lock);
    return ok;
}

int proc_pipe(proc_fd_t fds[2], u32 flags)
{
    if (fds == nullptr)
        return -PROC_EINVAL;

    SECURITY_ATTRIBUTES sa;
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle       = (flags & PROC_PIPE_CLOEXEC) ? FALSE : TRUE;

    HANDLE r, w;
    if (!CreatePipe(&r, &w, &sa, 0))
        return win_err(GetLastError());

    if (flags & PROC_PIPE_NONBLOCK)
    {
        DWORD mode = PIPE_NOWAIT;
        if (!SetNamedPipeHandleState(r, &mode, nullptr, nullptr) ||
            !SetNamedPipeHandleState(w, &mode, nullptr, nullptr))
        {
            /* GCOV_EXCL_START */
            int e = win_err(GetLastError());
            CloseHandle(r);
            CloseHandle(w);
            return e;
            /* GCOV_EXCL_STOP */
        }
    }

    fds[0] = (proc_fd_t)r;
    fds[1] = (proc_fd_t)w;
    return 0;
}

i64 proc_read(proc_fd_t fd, void *buf, size_t n)
{
    if (n == 0)
        return 0;
    if (buf == nullptr)
        return -PROC_EINVAL;

    DWORD want = n > 0x7fffffffu ? 0x7fffffffu : (DWORD)n;
    DWORD got  = 0;

    if (ReadFile((HANDLE)fd, buf, want, &got, nullptr))
        return (i64)got;

    DWORD e = GetLastError();

    if (e == ERROR_BROKEN_PIPE)
        return 0;
    if (e == ERROR_NO_DATA)
        return -PROC_EAGAIN;

    return win_err(e);
}

i64 proc_write(proc_fd_t fd, const void *buf, size_t n)
{
    if (n == 0)
        return 0;
    if (buf == nullptr)
        return -PROC_EINVAL;

    size_t done = 0;
    while (done < n)
    {
        size_t left = n - done;
        DWORD  want = left > 0x7fffffffu ? 0x7fffffffu : (DWORD)left;
        DWORD  put  = 0;

        if (!WriteFile((HANDLE)fd, (const char *)buf + done, want, &put, nullptr))
            return done ? (i64)done : (i64)win_err(GetLastError());

        /* GCOV_EXCL_START */
        if (put == 0)
            return done ? (i64)done : -PROC_EAGAIN;
        /* GCOV_EXCL_STOP */

        done += put;
    }
    return (i64)done;
}

int proc_close(proc_fd_t fd)
{
    if (fd == PROC_FD_INVALID || !CloseHandle((HANDLE)fd))
        return -PROC_EINVAL;
    return 0;
}

int proc_dup2(proc_fd_t oldfd, proc_fd_t newfd)
{
    (void)oldfd;
    (void)newfd;
    return -PROC_ENOSYS;
}

proc_pid_t proc_getpid(void)
{
    return (proc_pid_t)GetCurrentProcessId();
}

proc_pid_t proc_fork(void)
{
    return -PROC_ENOSYS;
}

static void slot_reap(DWORD pid, HANDLE handle, proc_status_t *status)
{
    DWORD code = 0;
    GetExitCodeProcess(handle, &code);

    int kill_sig = 0;

    __crt_lock_acquire(&g_slot_lock);
    proc_slot *slot = slot_find(pid);
    if (slot != nullptr)
    {
        kill_sig = slot->kill_sig;
        *slot = g_slots[--g_slot_count];
    }
    __crt_lock_release(&g_slot_lock);

    CloseHandle(handle);

    if (status == nullptr)
        return;

    if (kill_sig != 0 && code == PROC_KILL_CODE(kill_sig))
    {
        status->exited      = false;
        status->exit_code   = 0;
        status->term_signal = kill_sig;
    }
    else
    {
        status->exited      = true;
        status->exit_code   = (int)code;
        status->term_signal = 0;
    }
}

proc_pid_t proc_wait(proc_pid_t pid, proc_status_t *status, u32 flags)
{
    DWORD timeout = (flags & PROC_WAIT_NOHANG) ? 0 : INFINITE;

    DWORD  pids[PROC_WIN_WAIT_MAX];
    HANDLE handles[PROC_WIN_WAIT_MAX];
    DWORD  count = 0;

    if (pid != -1 && pid <= 0)
        return -PROC_EINVAL;

    __crt_lock_acquire(&g_slot_lock);
    if (pid > 0)
    {
        proc_slot *slot = slot_find((DWORD)pid);
        if (slot != nullptr)
        {
            pids[0]    = slot->pid;
            handles[0] = slot->handle;
            count      = 1;
        }
    }
    else
    {
        for (size_t i = 0; i < g_slot_count && count < PROC_WIN_WAIT_MAX; i++)
        {
            pids[count]    = g_slots[i].pid;
            handles[count] = g_slots[i].handle;
            count++;
        }
    }
    __crt_lock_release(&g_slot_lock);

    if (count == 0)
        return -PROC_ECHILD;

    DWORD r = WaitForMultipleObjects(count, handles, FALSE, timeout);

    if (r == WAIT_TIMEOUT)
        return 0;

    /* GCOV_EXCL_START */
    if (r >= WAIT_OBJECT_0 + count)
        return win_err(GetLastError());
    /* GCOV_EXCL_STOP */

    DWORD hit = r - WAIT_OBJECT_0;
    slot_reap(pids[hit], handles[hit], status);
    return (proc_pid_t)pids[hit];
}

int proc_kill(proc_pid_t pid, int sig)
{
    if (pid <= 0)
        return -PROC_EINVAL;
    if (sig < 0)
        return -PROC_EINVAL;

    int    rc      = 0;
    bool   already = false;
    HANDLE handle  = nullptr;

    __crt_lock_acquire(&g_slot_lock);
    proc_slot *slot = slot_find((DWORD)pid);
    if (slot == nullptr)
        rc = -PROC_ESRCH;
    else
    {
        handle  = slot->handle;
        already = slot->kill_sig != 0;
        if (sig != 0 && !already)
            slot->kill_sig = sig;
    }
    __crt_lock_release(&g_slot_lock);

    if (rc < 0 || sig == 0)
        return rc;

    if (already)
        return 0;

    if (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0)
        return 0;

    /* GCOV_EXCL_START */
    if (!TerminateProcess(handle, PROC_KILL_CODE(sig)))
    {
        if (WaitForSingleObject(handle, 1000) == WAIT_OBJECT_0)
            return 0;
        return win_err(GetLastError());
    }
    /* GCOV_EXCL_STOP */

    return 0;
}

static void cmdline_append_arg(dstr_t *out, const char *arg)
{
    bool quote = (*arg == '\0');
    for (const char *p = arg; *p != '\0'; p++)
    {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\v' || *p == '"')
            quote = true;
    }

    if (!quote)
    {
        dstr_append(out, arg);
        return;
    }

    dstr_append_char(out, '"');

    for (const char *p = arg;; p++)
    {
        size_t backslashes = 0;
        while (*p == '\\')
        {
            backslashes++;
            p++;
        }

        if (*p == '\0')
        {
            for (size_t i = 0; i < backslashes * 2; i++)
                dstr_append_char(out, '\\');
            break;
        }

        if (*p == '"')
        {
            for (size_t i = 0; i < backslashes * 2 + 1; i++)
                dstr_append_char(out, '\\');
        }
        else
        {
            for (size_t i = 0; i < backslashes; i++)
                dstr_append_char(out, '\\');
        }
        dstr_append_char(out, *p);
    }

    dstr_append_char(out, '"');
}

static char *build_env_block(char *const *envp)
{
    size_t total = 1;
    for (char *const *e = envp; *e != nullptr; e++)
        total += strlen(*e) + 1;

    char *block = (char *)malloc(total + 1);
    if (block == nullptr)
        return nullptr;

    size_t at = 0;
    for (char *const *e = envp; *e != nullptr; e++)
    {
        size_t len = strlen(*e) + 1;
        memcpy(block + at, *e, len);
        at += len;
    }
    block[at++] = '\0';
    block[at]   = '\0';
    return block;
}

static bool is_path_sep(char c)
{
    return c == '/' || c == '\\' || c == ':';
}

static int resolve_program(const char *path, char *app, size_t cap)
{
    bool   has_dir = false;
    bool   has_ext = false;
    size_t len     = 0;

    for (const char *p = path; *p != '\0'; p++, len++)
    {
        if (is_path_sep(*p))
        {
            has_dir = true;
            has_ext = false;
        }
        else if (*p == '.')
            has_ext = true;
    }

    if (!has_dir)
    {
        DWORD n = SearchPathA(nullptr, path, ".exe", (DWORD)cap, app, nullptr);
        if (n == 0)
            return -PROC_ENOENT;
        if (n >= cap)
            return -PROC_EINVAL; /* GCOV_EXCL_LINE */
        return 0;
    }

    if (len + sizeof(".exe") > cap)
        return -PROC_EINVAL; /* GCOV_EXCL_LINE */

    memcpy(app, path, len + 1);

    if (GetFileAttributesA(app) == INVALID_FILE_ATTRIBUTES && !has_ext)
    {
        memcpy(app + len, ".exe", sizeof(".exe"));
        if (GetFileAttributesA(app) == INVALID_FILE_ATTRIBUTES)
            return -PROC_ENOENT;
    }

    return GetFileAttributesA(app) == INVALID_FILE_ATTRIBUTES ? -PROC_ENOENT : 0;
}

int proc_spawn(const proc_spawn_t *spec, proc_pid_t *out_pid)
{
    if (spec == nullptr || out_pid == nullptr || spec->path == nullptr || spec->path[0] == '\0')
        return -PROC_EINVAL;

    const proc_fd_t redir[3] = { spec->stdin_fd, spec->stdout_fd, spec->stderr_fd };
    bool redirected = false;

    for (int slot = 0; slot < 3; slot++)
    {
        if (redir[slot] < PROC_FD_INHERIT)
            return -PROC_EINVAL;
        if (redir[slot] != PROC_FD_INHERIT)
            redirected = true;
    }

    char app[4096];
    const char *appname = spec->path;

    if (spec->flags & PROC_SPAWN_SEARCH_PATH)
    {
        int found = resolve_program(spec->path, app, sizeof(app));
        if (found < 0)
            return found;
        appname = app;
    }

    char *default_argv[2] = { (char *)spec->path, nullptr };
    char *const *argv = spec->argv != nullptr ? spec->argv : default_argv;

    dstr_t cmd = dstr_new("");
    for (char *const *a = argv; cmd.data != nullptr && *a != nullptr; a++)
    {
        if (a != argv)
            dstr_append_char(&cmd, ' ');
        cmdline_append_arg(&cmd, *a);
    }

    char *envblock = nullptr;
    if (spec->envp != nullptr)
        envblock = build_env_block(spec->envp);

    /* GCOV_EXCL_START */
    if (cmd.data == nullptr || (spec->envp != nullptr && envblock == nullptr))
    {
        dstr_free(&cmd);
        free(envblock);
        return -PROC_ENOMEM;
    }
    /* GCOV_EXCL_STOP */

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    HANDLE h[3]        = { nullptr, nullptr, nullptr };
    DWORD  was[3]      = { 0, 0, 0 };
    bool   changed[3]  = { false, false, false };
    int    rc          = 0;

    if (redirected)
    {
        static const DWORD std_ids[3] = { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };

        for (int slot = 0; slot < 3; slot++)
        {
            bool given = redir[slot] != PROC_FD_INHERIT;
            h[slot] = given ? (HANDLE)redir[slot] : GetStdHandle(std_ids[slot]);

            if (h[slot] == nullptr || h[slot] == INVALID_HANDLE_VALUE)
            {
                if (given)
                    rc = -PROC_EINVAL;
                continue;
            }

            DWORD info = 0;
            if (!GetHandleInformation(h[slot], &info))
            {
                if (given)
                    rc = -PROC_EINVAL;
                h[slot] = nullptr;
                continue;
            }

            was[slot] = info;
            if (!(info & HANDLE_FLAG_INHERIT) &&
                SetHandleInformation(h[slot], HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
                changed[slot] = true;
        }

        si.dwFlags    = STARTF_USESTDHANDLES;
        si.hStdInput  = h[0];
        si.hStdOutput = h[1];
        si.hStdError  = h[2];
    }

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    DWORD err = 0;

    if (rc == 0 && !CreateProcessA(appname, cmd.data, nullptr, nullptr, redirected ? TRUE : FALSE,
                                   0, envblock, nullptr, &si, &pi))
    {
        err = GetLastError();
        rc  = win_err(err);
    }

    for (int slot = 0; slot < 3; slot++)
    {
        if (changed[slot])
            SetHandleInformation(h[slot], HANDLE_FLAG_INHERIT, was[slot] & HANDLE_FLAG_INHERIT);
    }

    dstr_free(&cmd);
    free(envblock);

    if (rc < 0)
        return rc;

    CloseHandle(pi.hThread);

    /* GCOV_EXCL_START */
    if (!slot_add(pi.dwProcessId, pi.hProcess))
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        return -PROC_ENOMEM;
    }
    /* GCOV_EXCL_STOP */

    *out_pid = (proc_pid_t)pi.dwProcessId;
    return 0;
}

#else

int  proc_pipe(proc_fd_t fds[2], u32 flags)                       { (void)fds; (void)flags; return -PROC_ENOSYS; }
i64  proc_read(proc_fd_t fd, void *buf, size_t n)                  { (void)fd; (void)buf; (void)n; return -PROC_ENOSYS; }
i64  proc_write(proc_fd_t fd, const void *buf, size_t n)           { (void)fd; (void)buf; (void)n; return -PROC_ENOSYS; }
int  proc_close(proc_fd_t fd)                                      { (void)fd; return -PROC_ENOSYS; }
int  proc_dup2(proc_fd_t oldfd, proc_fd_t newfd)                   { (void)oldfd; (void)newfd; return -PROC_ENOSYS; }
proc_pid_t proc_getpid(void)                                       { return -PROC_ENOSYS; }
proc_pid_t proc_fork(void)                                         { return -PROC_ENOSYS; }
proc_pid_t proc_wait(proc_pid_t pid, proc_status_t *status, u32 flags) { (void)pid; (void)status; (void)flags; return -PROC_ENOSYS; }
int  proc_kill(proc_pid_t pid, int sig)                            { (void)pid; (void)sig; return -PROC_ENOSYS; }
int  proc_spawn(const proc_spawn_t *spec, proc_pid_t *out_pid)     { (void)spec; (void)out_pid; return -PROC_ENOSYS; }

#endif
