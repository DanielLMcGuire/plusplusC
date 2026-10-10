// ++C C Runtime Library (libminicrt) | Platform (Linux)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_linux.h"

long sys_write(int fd, const void *buf, size_t count)
{
    return syscall(__NR_write, (long)fd, (long)buf, (long)count);
}

long sys_read(int fd, void *buf, size_t count)
{
    return syscall(__NR_read, (long)fd, (long)buf, (long)count);
}

int sys_close(int fd)
{
    return (int)syscall(__NR_close, (long)fd);
}

long sys_unlink(const char *path)
{
    return syscall(__NR_unlinkat, (long)AT_FDCWD, (long)path, 0L);
}

long sys_getcwd(char *buf, size_t size)
{
    return syscall(__NR_getcwd, (long)buf, (long)size);
}

long sys_lseek(int fd, long offset, int whence)
{
    return syscall(__NR_lseek, (long)fd, offset, (long)whence);
}

long sys_openat(int dirfd, const char *path, int flags, int mode)
{
    return syscall(__NR_openat, (long)dirfd, (long)path, (long)flags, (long)mode);
}

long sys_rt_sigaction(int signum, const void *act, void *oldact, size_t sigsetsize)
{
    return syscall(__NR_rt_sigaction, (long)signum, (long)act, (long)oldact, (long)sigsetsize);
}

long sys_getpid(void)
{
    return syscall(__NR_getpid);
}

long sys_gettid(void)
{
#if defined(__NR_gettid)
    return syscall(__NR_gettid);
#else
    return syscall(__NR_getpid);
#endif
}

long sys_kill(long pid, int sig)
{
    return syscall(__NR_kill, pid, (long)sig);
}

long sys_pipe2(int fds[2], int flags)
{
    return syscall(__NR_pipe2, (long)fds, (long)flags);
}

long sys_dup2(int oldfd, int newfd)
{
    if (oldfd == newfd)
    {
        long r = syscall(__NR_fcntl, (long)oldfd, 1L /* F_GETFD */);
        return r < 0 ? r : (long)newfd;
    }
    return syscall(__NR_dup3, (long)oldfd, (long)newfd, 0L);
}

long sys_fork(void)
{
    return syscall(__NR_clone, (long)XXC_SIGCHLD, 0L, 0L, 0L, 0L);
}

long sys_wait4(long pid, int *status, int options)
{
    return syscall(__NR_wait4, pid, (long)status, (long)options, 0L);
}

long sys_execve(const char *path, char *const argv[], char *const envp[])
{
    return syscall(__NR_execve, (long)path, (long)argv, (long)envp);
}

void sys_exit(int status)
{
#if defined(__NR_exit_group)
    /* GCOV_EXCL_START */
    (void)syscall(__NR_exit_group, (long)status);
#elif defined(__NR_exit)
    (void)syscall(__NR_exit, (long)status);
#endif
    __NORETURN__
    /* GCOV_EXCL_STOP */
}

void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset)
{
    long ret = -1;
#if defined(__NR_mmap2)
    ret = syscall(__NR_mmap2, (long)addr, (long)length, prot, flags, fd, offset >> 12);
#elif defined(__NR_mmap)
    ret = syscall(__NR_mmap, (long)addr, (long)length, prot, flags, fd, offset);
#endif
    if (ret < 0 && ret >= -4095) return nullptr; 
    return (void*)ret;
}

int sys_munmap(void *addr, size_t length)
{
    return (int)syscall(__NR_munmap, (long)addr, (long)length, 0);
}

#if defined(__i386__) && !defined(__x86_64__) && defined(__NR_socketcall)
#define XXC_USE_SOCKETCALL 1
#endif

#if defined(XXC_USE_SOCKETCALL)

#define XXC_SYS_SOCKET      1
#define XXC_SYS_BIND        2
#define XXC_SYS_CONNECT     3
#define XXC_SYS_LISTEN      4
#define XXC_SYS_ACCEPT      5
#define XXC_SYS_GETSOCKNAME 6
#define XXC_SYS_GETPEERNAME 7
#define XXC_SYS_SEND        9
#define XXC_SYS_RECV        10
#define XXC_SYS_SENDTO      11
#define XXC_SYS_RECVFROM    12
#define XXC_SYS_SHUTDOWN    13
#define XXC_SYS_SETSOCKOPT  14
#define XXC_SYS_GETSOCKOPT  15
#define XXC_SYS_ACCEPT4     18

static long sys_socketcall(int call, long a0, long a1, long a2, long a3, long a4, long a5)
{
    long args[6];
    args[0] = a0; args[1] = a1; args[2] = a2;
    args[3] = a3; args[4] = a4; args[5] = a5;
    return syscall(__NR_socketcall, (long)call, (long)args);
}

#endif /* XXC_USE_SOCKETCALL */

long sys_socket(int domain, int type, int protocol)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_SOCKET, domain, type, protocol, 0, 0, 0);
#else
    return syscall(__NR_socket, (long)domain, (long)type, (long)protocol);
#endif
}

long sys_bind(long sockfd, const void *addr, unsigned int addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_BIND, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#else
    return syscall(__NR_bind, sockfd, (long)addr, (long)addrlen);
#endif
}

long sys_listen(long sockfd, int backlog)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_LISTEN, sockfd, (long)backlog, 0, 0, 0, 0);
#else
    return syscall(__NR_listen, sockfd, (long)backlog);
#endif
}

long sys_accept4(long sockfd, void *addr, unsigned int *addrlen, int flags)
{
#if defined(XXC_USE_SOCKETCALL)
    if (flags != 0)
        return -38; /* -ENOSYS */
    return sys_socketcall(XXC_SYS_ACCEPT, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#elif defined(__NR_accept4)
    return syscall(__NR_accept4, sockfd, (long)addr, (long)addrlen, (long)flags);
#elif defined(__NR_accept)
    (void)flags;
    return syscall(__NR_accept, sockfd, (long)addr, (long)addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen; (void)flags;
    return -38; /* -ENOSYS */
#endif
}

long sys_connect(long sockfd, const void *addr, unsigned int addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_CONNECT, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#else
    return syscall(__NR_connect, sockfd, (long)addr, (long)addrlen);
#endif
}

long sys_sendto(long sockfd, const void *buf, size_t len, int flags, const void *dest_addr, unsigned int addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_SENDTO, sockfd, (long)buf, (long)len, (long)flags, (long)dest_addr, (long)addrlen);
#else
    return syscall(__NR_sendto, sockfd, (long)buf, (long)len, (long)flags, (long)dest_addr, (long)addrlen);
#endif
}

long sys_recvfrom(long sockfd, void *buf, size_t len, int flags, void *src_addr, unsigned int *addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_RECVFROM, sockfd, (long)buf, (long)len, (long)flags, (long)src_addr, (long)addrlen);
#else
    return syscall(__NR_recvfrom, sockfd, (long)buf, (long)len, (long)flags, (long)src_addr, (long)addrlen);
#endif
}

long sys_shutdown(long sockfd, int how)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_SHUTDOWN, sockfd, (long)how, 0, 0, 0, 0);
#else
    return syscall(__NR_shutdown, sockfd, (long)how);
#endif
}

long sys_setsockopt(long sockfd, int level, int optname, const void *optval, unsigned int optlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_SETSOCKOPT, sockfd, (long)level, (long)optname, (long)optval, (long)optlen, 0);
#else
    return syscall(__NR_setsockopt, sockfd, (long)level, (long)optname, (long)optval, (long)optlen);
#endif
}

long sys_getsockopt(long sockfd, int level, int optname, void *optval, unsigned int *optlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_GETSOCKOPT, sockfd, (long)level, (long)optname, (long)optval, (long)optlen, 0);
#else
    return syscall(__NR_getsockopt, sockfd, (long)level, (long)optname, (long)optval, (long)optlen);
#endif
}

long sys_getsockname(long sockfd, void *addr, unsigned int *addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_GETSOCKNAME, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#else
    return syscall(__NR_getsockname, sockfd, (long)addr, (long)addrlen);
#endif
}

long sys_getpeername(long sockfd, void *addr, unsigned int *addrlen)
{
#if defined(XXC_USE_SOCKETCALL)
    return sys_socketcall(XXC_SYS_GETPEERNAME, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#else
    return syscall(__NR_getpeername, sockfd, (long)addr, (long)addrlen);
#endif
}

long sys_futex(int *uaddr, int op, int val, const void *timeout, int *uaddr2, int val3)
{
    long ret = -1;
#if defined(__NR_futex)
    ret = syscall(__NR_futex, (long)uaddr, (long)op, (long)val, (long)timeout, (long)uaddr2, (long)val3);
#elif defined(__NR_futex_time64)
    ret = syscall(__NR_futex_time64, (long)uaddr, (long)op, (long)val, (long)timeout, (long)uaddr2, (long)val3);
#endif
    /* GCOV_EXCL_START */
    if (ret == -38 && (op & FUTEX_PRIVATE_FLAG))
    {
#if defined(__NR_futex)
        ret = syscall(__NR_futex, (long)uaddr, (long)(op & ~FUTEX_PRIVATE_FLAG), (long)val, (long)timeout, (long)uaddr2, (long)val3);
#endif
    }
    /* GCOV_EXCL_STOP */
    return ret;
}

int sys_mprotect(void *addr, size_t length, int prot)
{
    return (int)syscall(__NR_mprotect, (long)addr, (long)length, (long)prot);
}

int sys_madvise(void *addr, size_t length, int advice)
{
    return (int)syscall(__NR_madvise, (long)addr, (long)length, (long)advice);
}

long sys_sched_yield(void)
{
    return syscall(__NR_sched_yield);
}

long sys_nanosleep(const xxc_timespec_t *req, xxc_timespec_t *rem)
{
    return syscall(__NR_nanosleep, (long)req, (long)rem);
}

long sys_clock_gettime(int clock_id, xxc_timespec_t *out)
{
    return syscall(__NR_clock_gettime, (long)clock_id, (long)out);
}

long sys_set_tid_address(int *tidptr)
{
    return syscall(__NR_set_tid_address, (long)tidptr);
}

void sys_exit_thread(int status)
{
    /* GCOV_EXCL_START */
    (void)syscall(__NR_exit, (long)status);
    __NORETURN__
    /* GCOV_EXCL_STOP */
}

long sys_rt_sigprocmask(int how, const void *set, void *oldset, size_t sigsetsize)
{
    return syscall(__NR_rt_sigprocmask, (long)how, (long)set, (long)oldset, (long)sigsetsize);
}
