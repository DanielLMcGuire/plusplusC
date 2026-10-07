// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SOCK_H
#define SOCK_H

#include <def.h>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #ifdef s_addr
    #undef s_addr
    #endif

    typedef SOCKET sock_t;
    #define SOCK_INVALID INVALID_SOCKET
#else
    typedef int sock_t;
    #define SOCK_INVALID ((sock_t)-1)
#endif

#define SOCK_AF_INET 2

#define SOCK_STREAM_T 1
#define SOCK_DGRAM_T  2

#define SOCK_IPPROTO_TCP 6
#define SOCK_IPPROTO_UDP 17

#define SOCK_SHUT_RD   0
#define SOCK_SHUT_WR   1
#define SOCK_SHUT_RDWR 2

#define SOCK_INADDR_ANY      0x00000000u
#define SOCK_INADDR_LOOPBACK 0x7f000001u
#define SOCK_INADDR_NONE     0xffffffffu

#if defined(_WIN32)
    #define SOCK_SOL_SOCKET   0xffff
    #define SOCK_SO_REUSEADDR 0x0004
    #define SOCK_SO_KEEPALIVE 0x0008
    #define SOCK_SO_BROADCAST 0x0020
#elif defined(__FreeBSD__)
    #define SOCK_SOL_SOCKET   0xffff
    #define SOCK_SO_REUSEADDR 0x0004
    #define SOCK_SO_KEEPALIVE 0x0008
    #define SOCK_SO_BROADCAST 0x0020
#else
    #define SOCK_SOL_SOCKET   1
    #define SOCK_SO_REUSEADDR 2
    #define SOCK_SO_BROADCAST 6
    #define SOCK_SO_KEEPALIVE 9
#endif

#define SOCK_TCP_NODELAY 1

typedef u32 sock_len_t;

typedef struct {
    u32 s_addr;
} sock_in_addr_t;

typedef struct {
#if defined(__FreeBSD__)
    u8             sin_len;
    u8             sin_family;
#else
    u16            sin_family;
#endif
    u16            sin_port;
    sock_in_addr_t sin_addr;
    u8             sin_zero[8];
} sock_addr_in_t;

static inline u16 sock_htons(u16 host16)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return host16;
#else
    return (u16)((host16 << 8) | (host16 >> 8));
#endif
}

static inline u32 sock_htonl(u32 host32)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return host32;
#else
    return ((host32 & 0x000000FFu) << 24)
         | ((host32 & 0x0000FF00u) << 8)
         | ((host32 & 0x00FF0000u) >> 8)
         | ((host32 & 0xFF000000u) >> 24);
#endif
}

static inline u16 sock_ntohs(u16 net16) { return sock_htons(net16); }
static inline u32 sock_ntohl(u32 net32) { return sock_htonl(net32); }

sock_addr_in_t sock_make_addr(u32 addr_host_order, u16 port_host_order);

bool sock_inet_pton4(const char *src, sock_in_addr_t *out);

u32 sock_inet_addr(const char *src);

bool sock_inet_ntop4(const sock_in_addr_t *addr, char *dst, size_t dst_size);

bool sock_init(void);
void sock_cleanup(void);

i32 sock_last_error(void);

sock_t sock_socket(int domain, int type, int protocol);

i32 sock_bind(sock_t s, const sock_addr_in_t *addr);
i32 sock_listen(sock_t s, i32 backlog);
sock_t sock_accept(sock_t s, sock_addr_in_t *out_addr /* nullable */);
i32 sock_connect(sock_t s, const sock_addr_in_t *addr);

i64 sock_send(sock_t s, const void *buf, size_t len, i32 flags);
i64 sock_recv(sock_t s, void *buf, size_t len, i32 flags);
i64 sock_sendto(sock_t s, const void *buf, size_t len, i32 flags, const sock_addr_in_t *addr);
i64 sock_recvfrom(sock_t s, void *buf, size_t len, i32 flags, sock_addr_in_t *out_addr /* nullable */);

i32 sock_setsockopt(sock_t s, i32 level, i32 optname, const void *optval, sock_len_t optlen);
i32 sock_getsockopt(sock_t s, i32 level, i32 optname, void *optval, sock_len_t *optlen);

i32 sock_getsockname(sock_t s, sock_addr_in_t *out_addr);
i32 sock_getpeername(sock_t s, sock_addr_in_t *out_addr);

i32 sock_shutdown(sock_t s, i32 how);
i32 sock_close(sock_t s);

bool sock_set_reuseaddr(sock_t s, bool enable);

bool sock_set_nodelay(sock_t s, bool enable);

#endif /* SOCK_H */
