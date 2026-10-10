// ++C C Runtime Library (libminicrt) | Platform (FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SYS_FREEBSD_H
#define SYS_FREEBSD_H
#include <def.h>

#if !defined(__x86_64__) && !defined(__aarch64__)
#error "++C on FreeBSD supports x86_64 and aarch64 only"
#endif

#define XXC_SYS_exit            1
#define XXC_SYS_fork            2
#define XXC_SYS_read            3
#define XXC_SYS_write           4
#define XXC_SYS_close           6
#define XXC_SYS_wait4           7
#define XXC_SYS_getpid          20
#define XXC_SYS_recvfrom        29
#define XXC_SYS_getpeername     31
#define XXC_SYS_getsockname     32
#define XXC_SYS_kill            37
#define XXC_SYS_execve          59
#define XXC_SYS_munmap          73
#define XXC_SYS_mprotect        74
#define XXC_SYS_madvise         75
#define XXC_SYS_dup2            90
#define XXC_SYS_socket          97
#define XXC_SYS_connect         98
#define XXC_SYS_bind            104
#define XXC_SYS_setsockopt      105
#define XXC_SYS_listen          106
#define XXC_SYS_getsockopt      118
#define XXC_SYS_sendto          133
#define XXC_SYS_shutdown        134
#define XXC_SYS_sysarch         165
#define XXC_SYS_clock_gettime   232
#define XXC_SYS_nanosleep       240
#define XXC_SYS___getcwd        326
#define XXC_SYS_sched_yield     331
#define XXC_SYS_sigprocmask     340
#define XXC_SYS_sigaction       416
#define XXC_SYS_thr_exit        431
#define XXC_SYS_thr_self        432
#define XXC_SYS_thr_kill        433
#define XXC_SYS__umtx_op        454
#define XXC_SYS_thr_new         455
#define XXC_SYS_mmap            477
#define XXC_SYS_lseek           478
#define XXC_SYS_openat          499
#define XXC_SYS_unlinkat        503
#define XXC_SYS_accept4         541
#define XXC_SYS_pipe2           542

#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_PRIVATE 0x0002
#define MAP_ANON    0x1000
#define MADV_DONTNEED 4

#define XXC_CLOCK_REALTIME  0
#define XXC_CLOCK_MONOTONIC 4

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

#define XXC_UMTX_OP_WAIT_UINT          11
#define XXC_UMTX_OP_WAIT_UINT_PRIVATE  15
#define XXC_UMTX_OP_WAKE_PRIVATE       16
#define XXC_UMTX_OP_WAKE               3

#define O_RDONLY   0x0000
#define O_WRONLY   0x0001
#define O_RDWR     0x0002
#define O_APPEND   0x0008
#define O_CREAT    0x0200
#define O_TRUNC    0x0400
#define O_EXCL     0x0800
#define O_NONBLOCK 0x0004
#define O_CLOEXEC  0x00100000

#define XXC_WNOHANG 1

#define AT_FDCWD (-100)

#define XXC_EINTR      4
#define XXC_ENOMEM    12
#define XXC_EAGAIN    35
#define XXC_ETIMEDOUT 60
#define XXC_ENOSYS    78

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long syscall(long number, ...);

long sys_unlink(const char *path);
long sys_getcwd(char *buf, size_t size);
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
long sys_nanosleep(const xxc_timespec_t *req, xxc_timespec_t *rem);
long sys_clock_gettime(int clock_id, xxc_timespec_t *out);
void sys_exit_thread(int status);

void __xxc_platform_init(int argc, char **argv);

extern char **__xxc_environ;
extern int    __xxc_dynamic_linked;
void __xxc_run_exe_init_array(int argc, char **argv);

typedef struct {
    u32 bits[4];
} xxc_ksigset_t;

typedef struct {
    void         (*handler)(int);
    int            flags;
    xxc_ksigset_t  mask;
} xxc_ksigaction_t;

long sys_sigaction(int signum, const xxc_ksigaction_t *act, xxc_ksigaction_t *oldact);
long sys_sigprocmask_block_all(void);

long sys_getpid(void);
long sys_gettid(void);
long sys_kill(long pid, int sig);

long sys_pipe2(int fds[2], int flags);
long sys_dup2(int oldfd, int newfd);
long sys_fork(void);
long sys_wait4(long pid, int *status, int options);
long sys_execve(const char *path, char *const argv[], char *const envp[]);

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

typedef struct {
    void  (*start_func)(void *);
    void   *arg;
    char   *stack_base;
    size_t  stack_size;
    char   *tls_base;
    size_t  tls_size;
    long   *child_tid;
    long   *parent_tid;
    int     flags;
    void   *rtp;
    void   *spare[3];
} xxc_thr_param_t;

long sys_thr_new(xxc_thr_param_t *param);
void sys_thr_exit(long *state);

#endif /* SYS_FREEBSD_H */
