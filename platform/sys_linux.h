// ++C C Runtime Library (libminicrt) | Platform (Linux)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SYS_LINUX_H
#define SYS_LINUX_H
#include <def.h>
#include <asm/unistd.h>
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_PRIVATE 0x02
#define MAP_ANON    0x20
#define MADV_DONTNEED 4

#define XXC_CLONE_VM             0x00000100
#define XXC_CLONE_FS             0x00000200
#define XXC_CLONE_FILES          0x00000400
#define XXC_CLONE_SIGHAND        0x00000800
#define XXC_CLONE_THREAD         0x00010000
#define XXC_CLONE_SYSVSEM        0x00040000
#define XXC_CLONE_SETTLS         0x00080000
#define XXC_CLONE_PARENT_SETTID  0x00100000
#define XXC_CLONE_CHILD_CLEARTID 0x00200000

#define XXC_CLOCK_REALTIME  0
#define XXC_CLOCK_MONOTONIC 1

typedef struct { 
    long tv_sec; 
    long tv_nsec; 
} xxc_timespec_t;

#ifndef FUTEX_WAIT
#define FUTEX_WAIT 0
#endif
#ifndef FUTEX_WAKE
#define FUTEX_WAKE 1
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_WAKE_PRIVATE
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#endif

#define O_RDONLY   00
#define O_WRONLY   01
#define O_RDWR     02
#define O_CREAT    0100
#define O_EXCL     0200
#define O_TRUNC    01000
#define O_APPEND   02000

#define AT_FDCWD (-100)

#define XXC_EAGAIN    11
#define XXC_ENOMEM    12
#define XXC_EINTR      4
#define XXC_ETIMEDOUT 110

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long syscall(long number, ...);

long sys_unlink(const char *path);
long sys_write(int fd, const void *buf, size_t count);
long sys_read(int fd, void *buf, size_t count);
int sys_close(int fd);
long sys_lseek(int fd, long offset, int whence);
long sys_openat(int dirfd, const char *path, int flags, int mode);
void sys_exit(int status);
void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset);
int sys_munmap(void *addr, size_t length);
long sys_futex(int *uaddr, int op, int val, const void *timeout, int *uaddr2, int val3);

int  sys_mprotect(void *addr, size_t length, int prot);
int  sys_madvise(void *addr, size_t length, int advice);
long sys_sched_yield(void);
long sys_rt_sigprocmask(int how, const void *set, void *oldset, size_t sigsetsize);
long sys_nanosleep(const xxc_timespec_t *req, xxc_timespec_t *rem);
long sys_clock_gettime(int clock_id, xxc_timespec_t *out);
long sys_set_tid_address(int *tidptr);
void sys_exit_thread(int status);

void __xxc_linux_init(int argc, char **argv);

long sys_rt_sigaction(int signum, const void *act, void *oldact, size_t sigsetsize);
long sys_getpid(void);
long sys_gettid(void);
long sys_kill(long pid, int sig);

long sys_socket(int domain, int type, int protocol);
long sys_bind(long sockfd, const void *addr, unsigned int addrlen);
long sys_listen(long sockfd, int backlog);
long sys_accept4(long sockfd, void *addr, unsigned int *addrlen, int flags);
long sys_connect(long sockfd, const void *addr, unsigned int addrlen);
long sys_sendto(long sockfd, const void *buf, size_t len, int flags, const void *dest_addr, unsigned int addrlen);
long sys_recvfrom(long sockfd, void *buf, size_t len, int flags, void *src_addr, unsigned int *addrlen);
long sys_shutdown(long sockfd, int how);
long sys_setsockopt(long sockfd, int level, int optname, const void *optval, unsigned int optlen);
long sys_getsockopt(long sockfd, int level, int optname, void *optval, unsigned int *optlen);
long sys_getsockname(long sockfd, void *addr, unsigned int *addrlen);
long sys_getpeername(long sockfd, void *addr, unsigned int *addrlen);
#endif /* SYS_LINUX_H */