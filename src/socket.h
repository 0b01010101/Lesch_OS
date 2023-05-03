#ifndef     SOCK_H
#define     SOCK_H

#include    "common.h"
#include    "schedul.h"
#include    "sync.h"
#include    "net.h"

#define     TCP_MAX_EQUE        10
#define     SOCK_FLAG_SEND    0x01
#define     SOCK_FLAG_REPLY   0x02
#define     SOCK_FLAG_RESEND  0x04
#define     SOCK_FLAG_ACK     0x80

typedef struct tcp_eque {

    u32int seq_num;
    u32int len;
    u32int addr;
} tcp_eque_s;

struct socket {

    u8int        status;
    u8int        flags;
    u8int        ip_dst[4];
    u8int        ip_src[4];
    u16int       port_dst;
    u16int       port_src;

    thread_s     *thread;
    u8int        *buf_recv;
    u8int        *buf_send;
    u32int       need_send_bytes;
    u32int       send_bytes;
    u32int       recv_bytes;
    //u32int       window;
    u32int       start_seq;
    u32int       seq_num;
    u32int       ack_num;
    u32int       event_time;

    u8int        space_addr;
    u8int        sock_type;
    u8int        protocol;
};

typedef struct socket socket_s; 

u16int socket(u8int space_addr, u8int sock_type, u8int protocol);
u8int  sock_bind(u16int id, void *ip_src, u16int port_src);
u8int  sock_listen(u16int id, u16int max_count_ports);
u8int  sock_close(u16int id);
u8int  sock_connect(u16int id, void *ip_src, u16int port_src, void *ip_dst, u16int port_dst);
u32int sock_send(socket_s *sock, u8int *data, u32int size, u8int flag);
u32int sock_recv(socket_s *sock, u8int *recv_buff);
u32int ssock_send(socket_s *sock, u8int flags);

#endif