// ++C C Runtime Library (libminicrt) | Platform (FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_freebsd.h"

long sys_write(int fd, const void *buf, size_t count)
{
    return syscall(XXC_SYS_write, (long)fd, (long)buf, (long)count);
}

long sys_read(int fd, void *buf, size_t count)
{
    return syscall(XXC_SYS_read, (long)fd, (long)buf, (long)count);
}

int sys_close(int fd)
{
    return (int)syscall(XXC_SYS_close, (long)fd);
}

long sys_unlink(const char *path)
{
    return syscall(XXC_SYS_unlinkat, (long)AT_FDCWD, (long)path, 0L);
}

long sys_getcwd(char *buf, size_t size)
{
    long r = syscall(XXC_SYS___getcwd, (long)buf, (long)size);
    if (r < 0) return r;
    long n = 0;
    while (n < (long)size && buf[n]) n++;
    return n + 1;
}

long sys_lseek(int fd, long offset, int whence)
{
    return syscall(XXC_SYS_lseek, (long)fd, offset, (long)whence);
}

long sys_openat(int dirfd, const char *path, int flags, int mode)
{
    return syscall(XXC_SYS_openat, (long)dirfd, (long)path, (long)flags, (long)mode);
}

long sys_getpid(void)
{
    return syscall(XXC_SYS_getpid);
}

long sys_gettid(void)
{
    long id = 0;
    if (syscall(XXC_SYS_thr_self, (long)&id) < 0)
        return 0;
    return id;
}

long sys_kill(long pid, int sig)
{
    return syscall(XXC_SYS_kill, pid, (long)sig);
}

long sys_pipe2(int fds[2], int flags)
{
    return syscall(XXC_SYS_pipe2, (long)fds, (long)flags);
}

long sys_dup2(int oldfd, int newfd)
{
    return syscall(XXC_SYS_dup2, (long)oldfd, (long)newfd);
}

long sys_fork(void)
{
    return syscall(XXC_SYS_fork);
}

long sys_wait4(long pid, int *status, int options)
{
    return syscall(XXC_SYS_wait4, pid, (long)status, (long)options, 0L);
}

long sys_execve(const char *path, char *const argv[], char *const envp[])
{
    return syscall(XXC_SYS_execve, (long)path, (long)argv, (long)envp);
}

void sys_exit(int status)
{
    (void)syscall(XXC_SYS_exit, (long)status);
    __NORETURN__ // GCOV_EXCL_LINE
}

void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset)
{
    long ret = syscall(XXC_SYS_mmap, (long)addr, (long)length, (long)prot, (long)flags, (long)fd, (long)offset);
    if (ret < 0 && ret >= -4095) return nullptr;
    return (void*)ret;
}

int sys_munmap(void *addr, size_t length)
{
    return (int)syscall(XXC_SYS_munmap, (long)addr, (long)length);
}

int sys_mprotect(void *addr, size_t length, int prot)
{
    return (int)syscall(XXC_SYS_mprotect, (long)addr, (long)length, (long)prot);
}

int sys_madvise(void *addr, size_t length, int advice)
{
    return (int)syscall(XXC_SYS_madvise, (long)addr, (long)length, (long)advice);
}

long sys_sched_yield(void)
{
    return syscall(XXC_SYS_sched_yield);
}

long sys_nanosleep(const xxc_timespec_t *req, xxc_timespec_t *rem)
{
    return syscall(XXC_SYS_nanosleep, (long)req, (long)rem);
}

long sys_clock_gettime(int clock_id, xxc_timespec_t *out)
{
    return syscall(XXC_SYS_clock_gettime, (long)clock_id, (long)out);
}

long sys_futex(int *uaddr, int op, int val, const void *timeout, int *uaddr2, int val3)
{
    (void)uaddr2; (void)val3;
    int priv = (op & FUTEX_PRIVATE_FLAG) != 0;
    int cmd  = op & ~FUTEX_PRIVATE_FLAG;

    if (cmd == FUTEX_WAIT)
    {
        return syscall(XXC_SYS__umtx_op, (long)uaddr,
                       (long)(priv ? XXC_UMTX_OP_WAIT_UINT_PRIVATE : XXC_UMTX_OP_WAIT_UINT),
                       (long)(unsigned int)val,
                       timeout ? (long)sizeof(xxc_timespec_t) : 0L,
                       (long)timeout);
    }
    if (cmd == FUTEX_WAKE)
    {
        return syscall(XXC_SYS__umtx_op, (long)uaddr,
                       (long)(priv ? XXC_UMTX_OP_WAKE_PRIVATE : XXC_UMTX_OP_WAKE),
                       (long)val, 0L, 0L);
    }
    return -XXC_ENOSYS;
}

long sys_sigaction(int signum, const xxc_ksigaction_t *act, xxc_ksigaction_t *oldact)
{
    return syscall(XXC_SYS_sigaction, (long)signum, (long)act, (long)oldact);
}

long sys_sigprocmask_block_all(void)
{
    xxc_ksigset_t all = { { ~0u, ~0u, ~0u, ~0u } };
    return syscall(XXC_SYS_sigprocmask, 1L /* SIG_BLOCK */, (long)&all, 0L);
}

long sys_thr_new(xxc_thr_param_t *param)
{
    return syscall(XXC_SYS_thr_new, (long)param, (long)sizeof(*param));
}

void sys_thr_exit(long *state)
{
    (void)syscall(XXC_SYS_thr_exit, (long)state);
    __NORETURN__
}

void sys_exit_thread(int status)
{
    (void)status;
    sys_thr_exit(nullptr);
}

long sys_socket(int domain, int type, int protocol)
{
    return syscall(XXC_SYS_socket, (long)domain, (long)type, (long)protocol);
}

long sys_bind(long sockfd, const void *addr, unsigned int addrlen)
{
    return syscall(XXC_SYS_bind, sockfd, (long)addr, (long)addrlen);
}

long sys_listen(long sockfd, int backlog)
{
    return syscall(XXC_SYS_listen, sockfd, (long)backlog);
}

long sys_accept4(long sockfd, void *addr, unsigned int *addrlen, int flags)
{
    return syscall(XXC_SYS_accept4, sockfd, (long)addr, (long)addrlen, (long)flags);
}

long sys_connect(long sockfd, const void *addr, unsigned int addrlen)
{
    return syscall(XXC_SYS_connect, sockfd, (long)addr, (long)addrlen);
}

long sys_sendto(long sockfd, const void *buf, size_t len, int flags, const void *dest_addr, unsigned int addrlen)
{
    return syscall(XXC_SYS_sendto, sockfd, (long)buf, (long)len, (long)flags, (long)dest_addr, (long)addrlen);
}

long sys_recvfrom(long sockfd, void *buf, size_t len, int flags, void *src_addr, unsigned int *addrlen)
{
    return syscall(XXC_SYS_recvfrom, sockfd, (long)buf, (long)len, (long)flags, (long)src_addr, (long)addrlen);
}

long sys_shutdown(long sockfd, int how)
{
    return syscall(XXC_SYS_shutdown, sockfd, (long)how);
}

long sys_setsockopt(long sockfd, int level, int optname, const void *optval, unsigned int optlen)
{
    return syscall(XXC_SYS_setsockopt, sockfd, (long)level, (long)optname, (long)optval, (long)optlen);
}

long sys_getsockopt(long sockfd, int level, int optname, void *optval, unsigned int *optlen)
{
    return syscall(XXC_SYS_getsockopt, sockfd, (long)level, (long)optname, (long)optval, (long)optlen);
}

long sys_getsockname(long sockfd, void *addr, unsigned int *addrlen)
{
    return syscall(XXC_SYS_getsockname, sockfd, (long)addr, (long)addrlen);
}

long sys_getpeername(long sockfd, void *addr, unsigned int *addrlen)
{
    return syscall(XXC_SYS_getpeername, sockfd, (long)addr, (long)addrlen);
}
