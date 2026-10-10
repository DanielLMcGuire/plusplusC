// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef PROC_H
#define PROC_H

#include <def.h>

typedef i32 proc_pid_t;

#if defined(_WIN32)
typedef intptr_t proc_fd_t;
#else
typedef int proc_fd_t;
#endif

#define PROC_FD_INVALID (-1)
#define PROC_FD_INHERIT (-1)

#define PROC_ENOENT  2
#define PROC_ESRCH   3
#define PROC_EINTR   4
#define PROC_ENOEXEC 8
#define PROC_ECHILD  10
#define PROC_EACCES  13
#define PROC_ENOTDIR 20
#define PROC_EINVAL  22
#define PROC_EMFILE  24
#define PROC_EPIPE   32
#define PROC_ENOMEM  12
#if defined(__FreeBSD__)
#define PROC_EAGAIN  35
#define PROC_ENOSYS  78
#else
#define PROC_EAGAIN  11
#define PROC_ENOSYS  38
#endif

#define PROC_PIPE_CLOEXEC  0x1u
#define PROC_PIPE_NONBLOCK 0x2u

#define PROC_WAIT_NOHANG 0x1u

#define PROC_SPAWN_SEARCH_PATH 0x1u

typedef struct {
    bool exited;
    int  exit_code;
    int  term_signal;
} proc_status_t;

int proc_pipe(proc_fd_t fds[2], u32 flags);
i64 proc_read(proc_fd_t fd, void *buf, size_t n);
i64 proc_write(proc_fd_t fd, const void *buf, size_t n);
int proc_close(proc_fd_t fd);
int proc_dup2(proc_fd_t oldfd, proc_fd_t newfd);
proc_pid_t proc_getpid(void);
proc_pid_t proc_fork(void);
proc_pid_t proc_wait(proc_pid_t pid, proc_status_t *status, u32 flags);
int proc_kill(proc_pid_t pid, int sig);

typedef struct {
    const char        *path;
    char *const       *argv;
    char *const       *envp;
    proc_fd_t          stdin_fd;
    proc_fd_t          stdout_fd;
    proc_fd_t          stderr_fd;
    u32                flags;
} proc_spawn_t;

#define PROC_SPAWN_INIT { nullptr, nullptr, nullptr, PROC_FD_INHERIT, PROC_FD_INHERIT, PROC_FD_INHERIT, 0 }

int proc_spawn(const proc_spawn_t *spec, proc_pid_t *out_pid);

#endif /* PROC_H */
