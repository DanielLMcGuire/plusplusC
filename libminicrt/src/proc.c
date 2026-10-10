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
    __xxc_heap_fork_prepare();
    long r = sys_fork();

    if (r == 0)
    {
        __xxc_cio_fork_child();
        atomic_i32_store(&__xxc_self->tid, (i32)sys_gettid());
    }

    __xxc_heap_fork_release();
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

#else /* no raw system calls: Windows, UEFI */

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
