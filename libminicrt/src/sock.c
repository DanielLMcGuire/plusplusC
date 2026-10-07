// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <sock.h>
#include <mem.h>
#include <cio.h>
#include <crt_lock.h>

#if defined(XXC_RAWSYS)
#include <xxc_sys.h>
#endif

#define SOCK_TLS_SLOTS 64

typedef struct {
    long tid;
    i32  last_error;
} sock_tls_slot_t;

static sock_tls_slot_t g_sock_tls[SOCK_TLS_SLOTS];
static crt_lock_t g_sock_tls_lock = CRT_LOCK_INIT;

static long sock_current_tid(void)
{
#if defined(_WIN32)
    return (long)GetCurrentThreadId();
#elif defined(XXC_RAWSYS)
    long tid = sys_gettid();
    return (tid != 0) ? tid : -1;
#else
    return 1;
#endif
}

static i32 *sock_last_error_slot(void)
{
    long tid = sock_current_tid();

    __crt_lock_acquire(&g_sock_tls_lock);

    for (int i = 0; i < SOCK_TLS_SLOTS; i++)
    {
        if (g_sock_tls[i].tid == tid)
        {
            __crt_lock_release(&g_sock_tls_lock);
            return &g_sock_tls[i].last_error;
        }
    }

    for (int i = 0; i < SOCK_TLS_SLOTS; i++)
    {
        if (g_sock_tls[i].tid == 0)
        {
            g_sock_tls[i].tid = tid;
            g_sock_tls[i].last_error = 0;
            __crt_lock_release(&g_sock_tls_lock);
            return &g_sock_tls[i].last_error;
        }
    }

    __crt_lock_release(&g_sock_tls_lock);
    return &g_sock_tls[0].last_error;
}

static void sock_set_last_error(i32 err)
{
    *sock_last_error_slot() = err;
}

i32 sock_last_error(void)
{
    return *sock_last_error_slot();
}

sock_addr_in_t sock_make_addr(u32 addr_host_order, u16 port_host_order)
{
    sock_addr_in_t addr;

    memset(&addr, 0, sizeof(addr));

#if defined(__FreeBSD__)
    addr.sin_len         = (u8)sizeof(addr);
#endif
    addr.sin_family      = SOCK_AF_INET;
    addr.sin_port        = sock_htons(port_host_order);
    addr.sin_addr.s_addr = sock_htonl(addr_host_order);

    return addr;
}

bool sock_inet_pton4(const char *src, sock_in_addr_t *out)
{
    if (!src || !out)
        return false;

    u32 result = 0;
    int octet   = 0;
    int digits  = 0;
    int value   = 0;

    for (const char *p = src;; p++)
    {
        char c = *p;

        if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');

            if (value > 255)
                return false;

            digits++;

            if (digits > 3)
                return false;
        }
        else if (c == '.' || c == '\0')
        {
            if (digits == 0)
                return false;

            result = (result << 8) | (u32)value;
            octet++;
            value  = 0;
            digits = 0;

            if (c == '\0')
                break;

            if (octet >= 4)
                return false;
        }
        else
        {
            return false;
        }
    }

    if (octet != 4)
        return false;

    out->s_addr = sock_htonl(result);
    return true;
}

u32 sock_inet_addr(const char *src)
{
    sock_in_addr_t addr;

    if (!sock_inet_pton4(src, &addr))
        return SOCK_INADDR_NONE;

    return addr.s_addr;
}

bool sock_inet_ntop4(const sock_in_addr_t *addr, char *dst, size_t dst_size)
{
    if (!addr || !dst || dst_size == 0)
        return false;

    u32 h = sock_ntohl(addr->s_addr);

    int len = snprintf(
        dst,
        dst_size,
        "%u.%u.%u.%u",
        (unsigned)((h >> 24) & 0xFF),
        (unsigned)((h >> 16) & 0xFF),
        (unsigned)((h >> 8)  & 0xFF),
        (unsigned)(h & 0xFF)
    );

    return len > 0 && (size_t)len < dst_size;
}

bool sock_set_reuseaddr(sock_t s, bool enable)
{
    int val = enable ? 1 : 0;
    return sock_setsockopt(s, SOCK_SOL_SOCKET, SOCK_SO_REUSEADDR, &val, sizeof(val)) == 0;
}

bool sock_set_nodelay(sock_t s, bool enable)
{
    int val = enable ? 1 : 0;
    return sock_setsockopt(s, SOCK_IPPROTO_TCP, SOCK_TCP_NODELAY, &val, sizeof(val)) == 0;
}

#if defined(_WIN32)

bool sock_init(void)
{
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (r != 0)
    {
        sock_set_last_error(r);
        return false;
    }

    return true;
}

void sock_cleanup(void)
{
    WSACleanup();
}

sock_t sock_socket(int domain, int type, int protocol)
{
    SOCKET s = socket(domain, type, protocol);

    if (s == INVALID_SOCKET)
    {
        sock_set_last_error(WSAGetLastError());
        return SOCK_INVALID;
    }

    return s;
}

i32 sock_bind(sock_t s, const sock_addr_in_t *addr)
{
    if (bind(s, (const struct sockaddr *)addr, (int)sizeof(*addr)) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i32 sock_listen(sock_t s, i32 backlog)
{
    if (listen(s, (int)backlog) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

sock_t sock_accept(sock_t s, sock_addr_in_t *out_addr)
{
    int addrlen = (int)sizeof(sock_addr_in_t);

    SOCKET c = accept(s, out_addr ? (struct sockaddr *)out_addr : NULL, out_addr ? &addrlen : NULL);

    if (c == INVALID_SOCKET)
    {
        sock_set_last_error(WSAGetLastError());
        return SOCK_INVALID;
    }

    return c;
}

i32 sock_connect(sock_t s, const sock_addr_in_t *addr)
{
    if (connect(s, (const struct sockaddr *)addr, (int)sizeof(*addr)) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i64 sock_send(sock_t s, const void *buf, size_t len, i32 flags)
{
    int r = send(s, (const char *)buf, (int)len, (int)flags);

    if (r == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return (i64)r;
}

i64 sock_recv(sock_t s, void *buf, size_t len, i32 flags)
{
    int r = recv(s, (char *)buf, (int)len, (int)flags);

    if (r == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return (i64)r;
}

i64 sock_sendto(sock_t s, const void *buf, size_t len, i32 flags, const sock_addr_in_t *addr)
{
    int r = sendto(s, (const char *)buf, (int)len, (int)flags, (const struct sockaddr *)addr, (int)sizeof(*addr));

    if (r == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return (i64)r;
}

i64 sock_recvfrom(sock_t s, void *buf, size_t len, i32 flags, sock_addr_in_t *out_addr)
{
    int addrlen = (int)sizeof(sock_addr_in_t);

    int r = recvfrom(s, (char *)buf, (int)len, (int)flags,
                      out_addr ? (struct sockaddr *)out_addr : NULL,
                      out_addr ? &addrlen : NULL);

    if (r == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return (i64)r;
}

i32 sock_setsockopt(sock_t s, i32 level, i32 optname, const void *optval, sock_len_t optlen)
{
    if (setsockopt(s, (int)level, (int)optname, (const char *)optval, (int)optlen) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i32 sock_getsockopt(sock_t s, i32 level, i32 optname, void *optval, sock_len_t *optlen)
{
    int len = optlen ? (int)*optlen : 0;

    if (getsockopt(s, (int)level, (int)optname, (char *)optval, &len) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    if (optlen)
        *optlen = (sock_len_t)len;

    return 0;
}

i32 sock_getsockname(sock_t s, sock_addr_in_t *out_addr)
{
    int addrlen = (int)sizeof(*out_addr);

    if (getsockname(s, (struct sockaddr *)out_addr, &addrlen) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i32 sock_getpeername(sock_t s, sock_addr_in_t *out_addr)
{
    int addrlen = (int)sizeof(*out_addr);

    if (getpeername(s, (struct sockaddr *)out_addr, &addrlen) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i32 sock_shutdown(sock_t s, i32 how)
{
    if (shutdown(s, (int)how) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

i32 sock_close(sock_t s)
{
    if (closesocket(s) == SOCKET_ERROR)
    {
        sock_set_last_error(WSAGetLastError());
        return -1;
    }

    return 0;
}

#elif defined(XXC_RAWSYS)

#if defined(__FreeBSD__)
static inline sock_addr_in_t sock_bsd_fix(const sock_addr_in_t *a)
{
    sock_addr_in_t c = *a;
    c.sin_len = (u8)sizeof(c);
    return c;
}
#endif

bool sock_init(void)
{
    return true;
}

void sock_cleanup(void)
{
}

sock_t sock_socket(int domain, int type, int protocol)
{
    long r = sys_socket(domain, type, protocol);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return SOCK_INVALID;
    }

    return (sock_t)r;
}

i32 sock_bind(sock_t s, const sock_addr_in_t *addr)
{
#if defined(__FreeBSD__)
    sock_addr_in_t fixed = sock_bsd_fix(addr);
    addr = &fixed;
#endif
    long r = sys_bind(s, addr, (unsigned int)sizeof(*addr));

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i32 sock_listen(sock_t s, i32 backlog)
{
    long r = sys_listen(s, (int)backlog);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

sock_t sock_accept(sock_t s, sock_addr_in_t *out_addr)
{
    unsigned int addrlen = (unsigned int)sizeof(sock_addr_in_t);

    long r = sys_accept4(s, out_addr, out_addr ? &addrlen : (void *)0, 0);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return SOCK_INVALID;
    }

    return (sock_t)r;
}

i32 sock_connect(sock_t s, const sock_addr_in_t *addr)
{
#if defined(__FreeBSD__)
    sock_addr_in_t fixed = sock_bsd_fix(addr);
    addr = &fixed;
#endif
    long r = sys_connect(s, addr, (unsigned int)sizeof(*addr));

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i64 sock_send(sock_t s, const void *buf, size_t len, i32 flags)
{
    long r = sys_sendto(s, buf, len, (int)flags, (void *)0, 0);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return (i64)r;
}

i64 sock_recv(sock_t s, void *buf, size_t len, i32 flags)
{
    long r = sys_recvfrom(s, buf, len, (int)flags, (void *)0, (void *)0);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return (i64)r;
}

i64 sock_sendto(sock_t s, const void *buf, size_t len, i32 flags, const sock_addr_in_t *addr)
{
#if defined(__FreeBSD__)
    sock_addr_in_t fixed;
    if (addr) { fixed = sock_bsd_fix(addr); addr = &fixed; }
#endif
    long r = sys_sendto(s, buf, len, (int)flags, addr, (unsigned int)sizeof(*addr));

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return (i64)r;
}

i64 sock_recvfrom(sock_t s, void *buf, size_t len, i32 flags, sock_addr_in_t *out_addr)
{
    unsigned int addrlen = (unsigned int)sizeof(sock_addr_in_t);

    long r = sys_recvfrom(s, buf, len, (int)flags, out_addr, out_addr ? &addrlen : (void *)0);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return (i64)r;
}

i32 sock_setsockopt(sock_t s, i32 level, i32 optname, const void *optval, sock_len_t optlen)
{
    long r = sys_setsockopt(s, (int)level, (int)optname, optval, (unsigned int)optlen);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i32 sock_getsockopt(sock_t s, i32 level, i32 optname, void *optval, sock_len_t *optlen)
{
    unsigned int len = optlen ? (unsigned int)*optlen : 0;

    long r = sys_getsockopt(s, (int)level, (int)optname, optval, &len);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    if (optlen)
        *optlen = (sock_len_t)len;

    return 0;
}

i32 sock_getsockname(sock_t s, sock_addr_in_t *out_addr)
{
    unsigned int addrlen = (unsigned int)sizeof(*out_addr);

    long r = sys_getsockname(s, out_addr, &addrlen);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i32 sock_getpeername(sock_t s, sock_addr_in_t *out_addr)
{
    unsigned int addrlen = (unsigned int)sizeof(*out_addr);

    long r = sys_getpeername(s, out_addr, &addrlen);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i32 sock_shutdown(sock_t s, i32 how)
{
    long r = sys_shutdown(s, (int)how);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

i32 sock_close(sock_t s)
{
    int r = sys_close((int)s);

    if (r < 0)
    {
        sock_set_last_error((i32)(-r));
        return -1;
    }

    return 0;
}

#else

bool sock_init(void) 
{ 
    return false; 
}
void sock_cleanup(void) { }

sock_t sock_socket(int domain, int type, int protocol)
{ (void)domain; (void)type; (void)protocol;
    return SOCK_INVALID;
}

i32 sock_bind(sock_t s, const sock_addr_in_t *addr)
{ (void)s; (void)addr; 
    return -1; 
}
i32 sock_listen(sock_t s, i32 backlog) 
{ (void)s; (void)backlog; return -1; }
sock_t sock_accept(sock_t s, sock_addr_in_t *out_addr) 
{ (void)s; (void)out_addr; 
    return SOCK_INVALID; 
}
i32 sock_connect(sock_t s, const sock_addr_in_t *addr) 
{ (void)s; (void)addr; 
    return -1; 
}
i64 sock_send(sock_t s, const void *buf, size_t len, i32 flags) 
{ (void)s; (void)buf; (void)len; (void)flags; 
    return -1; 
}
i64 sock_recv(sock_t s, void *buf, size_t len, i32 flags) 
{ (void)s; (void)buf; (void)len; (void)flags; 
    return -1; 
}
i64 sock_sendto(sock_t s, const void *buf, size_t len, i32 flags, const sock_addr_in_t *addr) 
{ (void)s; (void)buf; (void)len; (void)flags; (void)addr; 
    return -1; 
}
i64 sock_recvfrom(sock_t s, void *buf, size_t len, i32 flags, sock_addr_in_t *out_addr) 
{ (void)s; (void)buf; (void)len; (void)flags; (void)out_addr; 
    return -1; 
}
i32 sock_setsockopt(sock_t s, i32 level, i32 optname, const void *optval, sock_len_t optlen) 
{ (void)s; (void)level; (void)optname; (void)optval; (void)optlen; 
    return -1; 
}
i32 sock_getsockopt(sock_t s, i32 level, i32 optname, void *optval, sock_len_t *optlen) 
{ (void)s; (void)level; (void)optname; (void)optval; (void)optlen; 
    return -1; 
}
i32 sock_getsockname(sock_t s, sock_addr_in_t *out_addr) 
{ (void)s; (void)out_addr; 
    return -1; 
}
i32 sock_getpeername(sock_t s, sock_addr_in_t *out_addr) 
{ (void)s; (void)out_addr; 
    return -1; 
}
i32 sock_shutdown(sock_t s, i32 how) 
{ (void)s; (void)how; 
    return -1; 
}
i32 sock_close(sock_t s) 
{ (void)s; 
    return -1; 
}

#endif