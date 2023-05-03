#ifndef     NET_H
#define     NET_H

#include    "common.h"
#include    "memory.h"
#include    "rtl8029.h"
#include    "socket.h"
#include    "timer.h"
//#include  "rtl8139.h"

#define     MAX_NET_FRAME       1500
#define     MAX_CONNECT         10
#define     MAX_PORTS           256
#define     SOCK_MAX_CONNECT    32
//--------------------------------------------
#define     ETH_TYPE        0x0001
#define     ARP_TYPE        0x0806

#define     IP_TYPE         0x0800
#define     IP_TYPE_IPV4    0x04
#define     IP_TYPE_IPV6    0x06

#define     UDP_TYPE        0x11
#define     TCP_TYPE        0x06
//---------------------------------------------
#define     ARP_REQUEST     0x0001
#define     ARP_REPLY       0x0002
#define     ARP_TABLE_SIZE  512
//---------------------------------------------
#define     DHCP_REQ        0x01
#define     DHCP_REPL       0x02
#define     DHCP_IDENT      0x89899898
#define     HARDW_ETH_TYPE  0x01
//---------------------------------------------
#define     TCP_MAX_CONNECT     32

/*
#define     TCP_CLOSED         0x00
#define     TCP_SYN_SENT       0x01
#define     TCP_SYN_RECV       0x02
#define     TCP_ESTABLISHED    0x03
#define     TCP_FIN_WAIT       0x04

#define     TCP_SENDING_SEND    5
#define     TCP_SENDING_REPLY   6
#define     TCP_SENDING_RESEND  7
*/
#define     TCP_FLAG_URG    0x20
#define     TCP_FLAG_ACK    0x10
#define     TCP_FLAG_PSH    0x08
#define     TCP_FLAG_RST    0x04
#define     TCP_FLAG_SYN    0x02
#define     TCP_FLAG_FIN    0x01

#define     TCP_OPTION_CLOSE    0x02
#define     TCP_OPTION_PUSH     0x01

#define     TCP_WINDOW_SIZE     0xFFFF
#define     TCP_SYN_MSS         448
#define     TCP_SEQ_NUM         0x00112233
//------------------------------------------------
#define     TFTP_WRITE           0x02
#define     TFTP_READ            0x01

#define     TFTP_RRQ            0x0100
#define     TFTP_WRQ            0x0200
#define     TFTP_DATA           0x0300
#define     TFTP_ACK            0x0400
#define     TFTP_ERROR          0x0500
//------------------------------------------------
#define     FTP_PASSIVE         0x00
#define     FTP_ACTIVE          0x01

//------------------------------------------------
struct ether {

    u8int   dst_mac[6];
    u8int   src_mac[6];
    u16int  type;
    u8int   data[];
};

struct ip {

    char    version_ihl_ptr[0];
    u8int   version:4;
    u8int   ihl:4;
    u8int   tos;
    u16int  length;
    u16int  id;
    char    flags_fragment_ptr[0];
    u8int   flags:3;
    u8int   fragment_offset_high:5;
    u8int   fragment_offset_low;
    u8int   ttl;
    u8int   protocol;
    u16int  checksum;
    u8int   src_ip[4];
    u8int   dst_ip[4];
    u8int   data[];

};

struct arp {

    u16int  htype;
    u16int  ptype;
    u8int   hlen;
    u8int   plen;
    u16int  oper;
    u8int   sha[6];
    u8int   spa[4];
    u8int   tha[6];
    u8int   tpa[4];
};

struct arp_table {

    u32int  ip_addr;
    u64int  mac_addr;
};

struct udp  {

    u16int  src_port;
    u16int  dst_port;
    u16int  length;
    u16int  checksum;
    u8int   data[];
};

struct dhcp {

    u8int   op;
    u8int   hardw_type;
    u8int   hardw_addr_len;
    u8int   hops;
    u32int  xid;
    u16int  seconds;
    u16int  flags;
    u32int  client_ip;
    u32int  your_ip;
    u32int  server_ip;
    u32int  gateway_ip;
    u8int   chaddr[16];
    u8int   sname[64];
    u8int   file[128];
    u8int   options[64];
    //u8int   options[8];
};

struct tftp {
    
    u16int  opcode;
    u16int  block;
    u8int   data[];
};

struct tcp {

    u16int  port_src;
    u16int  port_dst;
    u32int  seq_num;
    u32int  ack_num;
    u8int   data_offset;
    u8int   flags;
    u16int  window;
    u16int  cksum;
    u16int  urg_ptr;
    u8int   data[];
};



typedef enum tcp_status {
    
    FREE,
    RESERV,
    LISTEN,
    TCP_CLOSED,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT
} tcp_status;

typedef enum tcp_send_mode {

    TCP_SENDING_SEND,
    TCP_SENDING_REPLY,
    TCP_SENDING_RESEND
} tcp_send_mode_s;

typedef struct bind_port {

    thread_s*     thread;
    u16int        port;
    u16int        max_ports;
    u16int        count;
    u16int        id_buff[MAX_CONNECT];

} bind_port_s;

typedef struct ether ether_s;
typedef struct ip ip_s;
typedef struct arp arp_s;
typedef struct arp_table arp_table_s;
typedef struct udp udp_s;
typedef struct dhcp dhcp_s;
typedef struct tftp tftp_s;
typedef struct tcp tcp_s;

void   ethernet_showlog(ether_s *eth, u8int *str);
void   Ethernet_Recv(ether_s *pack, u32int len);
u32int Ethernet_Send(u8int *dst_mac, u16int protocol, u8int *data, u32int len);

void   Ip_Recv(ip_s *pack, u32int len);
u8int  Ip_Send(u8int *dst_ip, u8int proto, void *data, u32int len);
void   ip_showlog(ip_s *ip, u8int *str);
u16int ip_calc_checksum(u32int sum, u8int *pack, u32int len);

void   Arp_Send(u8int *dst_mac, u8int *dst_ip);
void   Arp_Recv(arp_s *arp_pack, u32int len);
void   arp_init(void);
void   arp_showlog(arp_s *arp_pack, u8int *str);
void   arp_lookup_add(u8int *ret_mac, u8int *ip_addr);
u32int arp_lookup(u8int *ret_mac, u8int *ip_addr);

void   Udp_Send(u8int *ip_dst, u16int port_src, u16int port_dst, void *data, u32int len);
void   Udp_Recv(ip_s *ip, u32int len);
void   udp_showlog(udp_s *pack, u8int *str);
u16int udp_calc_checksum(udp_s *pack);
void   udp_to_app(u16int id, udp_s *pack, u32int len);

void   Dhcp_Send(u8int *req_ip);
void   Dhcp_Recv(dhcp_s *pack);
void   Dhcp_Discover(void);
void   *dhcp_get_options(dhcp_s *pack, u8int type);
void   dhcp_make_pack(dhcp_s *pack, u8int type, u8int *req_ip);
void   dhcp_showlog(dhcp_s *dhcp, u8int *str);

void   TCP_Recv(void);
void   Tcp_Recv(ip_s *pack, u32int len);
void   Tcp_Send(u16int id, tcp_s *pack, u32int len, u8int options);
u8int  tcp_xmit(socket_s *st, tcp_s *pack, u32int len);
u8int  tcp_sort_pack(socket_s *sock, tcp_s *tcp, u32int len);
u8int  tcp_listen(u16int id, tcp_s *pack);
void   tcp_closed(u16int id, u8int reset);
void   tcp_from_app(u16int id, void *pack, u8int retrns);
void   tcp_to_app(u16int id, void *pack, u32int len);
void   tcp_showlog(tcp_s *tcp, u8int *str);

void   Tftp_Request(u8int *fname, u16int port_src, u8int option);
u32int Tftp_Handler(u16int idsock, u16int *last_block);
u32int Tftp(u8int *filename, u16int my_port, u8int option, u8int *data, u32int send_bytes);

void net_test2(u8int *name, u8int *passw, u16int my_inf_port, u16int my_data_port, u8int *serv_ip);
void net_test3(void);


#define tcp_head_size(tcp)      ((((tcp_s *)tcp)->data_offset & 0xF0) >> 2)
#define tcp_get_data(tcp)       ((u8int *)(tcp) + tcp_head_size(tcp));


#endif  