#include "socket.h"

socket_s            socket_pool[SOCK_MAX_CONNECT];
bind_port_s         ports_binded[MAX_PORTS];
mutex_s             mtx_socket;

u8int sock_close(u16int id) {

    
    get_mtx(&mtx_socket, true);

    socket_s *sock = socket_pool + id;

    for(int i = 0; i < MAX_PORTS; i++) {

        if(ports_binded[i].port == sock->port_src) {
            memset(&ports_binded[i], 0, sizeof(bind_port_s));
            //kfree(sock->eque);
            memset(sock, 0, sizeof(socket_s));

            clean_mtx(&mtx_socket);
            return 1;
        }
    }
    clean_mtx(&mtx_socket);
    return 0;
}

u8int sock_listen(u16int id, u16int max_ps) {

    
    get_mtx(&mtx_socket, true);

    socket_s *sock = &socket_pool[id];

    for(int i = 0; i < MAX_PORTS; i++) {

        if(ports_binded[i].port == sock->port_src) {
            wprint("SOCK ERR_0!!!: such port(%d) EXIST\n", ports_binded[i].port);

            clean_mtx(&mtx_socket);
            return 0;
        }

        if(ports_binded[i].port == 0x00) {

            ports_binded[i].port = sock->port_src;
            ports_binded[i].thread = sock->thread;

            if(max_ps > MAX_CONNECT) {
                max_ps = MAX_CONNECT;
            }
            ports_binded[i].max_ports = max_ps;
            ports_binded[i].count = 0x00;

            sock->status = LISTEN;

            for(int j = i + 1; j < MAX_PORTS; j++) {

                if(ports_binded[j].port == sock->port_src) {
                    memset(ports_binded + i, 0, sizeof(bind_port_s));
                    wprint("SOCK ERR_1!!!: such port EXIST\n");

                    clean_mtx(&mtx_socket);
                    return 0;
                }
            }
            clean_mtx(&mtx_socket);
            return 1;
        }
    }
    clean_mtx(&mtx_socket);
    return 0;
}

u16int socket(u8int space_addr, u8int sock_type, u8int protocol) {

    
    get_mtx(&mtx_socket, true);

    for(int i = 0; i < SOCK_MAX_CONNECT; i++) {

        if(socket_pool[i].status == FREE) {

            socket_pool[i].status = RESERV;
            socket_pool[i].space_addr = space_addr;
            socket_pool[i].sock_type = sock_type;
            socket_pool[i].protocol = protocol;

            socket_pool[i].seq_num = htondw(TCP_SEQ_NUM);
            socket_pool[i].ack_num = 0;
            //socket_pool[i].urg_ptr = 0;
            //socket_pool[i].window = TCP_WINDOW_SIZE;
            socket_pool[i].thread = get_curr_thread();
            socket_pool[i].recv_bytes = 0;
            socket_pool[i].send_bytes = 0;

            clean_mtx(&mtx_socket);
            return i;
        }
    }
    clean_mtx(&mtx_socket);
    return 0;
}

u8int sock_bind(u16int id, void *ip_src, u16int port_src) {

    
    get_mtx(&mtx_socket, true);

    socket_s *sock = socket_pool + id;

    if(sock->status == RESERV) {

        memcpy(sock->ip_src, ip_src, 4);
        sock->port_src = port_src;
        sock->status = TCP_CLOSED;

        clean_mtx(&mtx_socket);
        return 0;
    }
    else if(sock->status == FREE) {
        clean_mtx(&mtx_socket);
        return 1;
    }
    else{
        clean_mtx(&mtx_socket);
        return 2;
    }
}

u8int sock_connect(u16int id, void *ip_src, u16int port_src, void *ip_dst, u16int port_dst) {

    //get_mtx(&mtx_socket, true);
    socket_s *sock = &socket_pool[id];
    tcp_s *tcp = kmalloc(sizeof(tcp_s));
    u32int len;

    memcpy(sock->ip_src, ip_src, 4);
    memcpy(sock->ip_dst, ip_dst, 4);
    sock->port_src = port_src;
    sock->port_dst = port_dst;

    memset(&tcp, 0, sizeof(tcp_s));
    tcp->flags = TCP_FLAG_SYN;
    tcp->port_src = port_src;
    tcp->port_dst = port_dst;
    tcp->data_offset = (sizeof(tcp_s) + 4) << 2;
    tcp->data[0] = 2;
    tcp->data[1] = 4;
    tcp->data[2] = TCP_SYN_MSS >> 8;
    tcp->data[3] = TCP_SYN_MSS & 0xFF;
    len = 4;

    tcp->seq_num = htondw(sock->seq_num);
    tcp->ack_num = htondw(sock->ack_num);

    len += sizeof(tcp_s);
    tcp->cksum = 0;
    //tcp->chsum = ;

    Ip_Send(sock->ip_dst, TCP_TYPE, tcp, len);

    sock->status = TCP_SYN_SENT;
    sock->seq_num += len - sizeof(tcp_s) + 1;

    //clean_mtx(&mtx_socket);
    return 0;
}

u16int sock_accept(u16int id) {

    socket_s * sock = socket_pool + id;
    u32int i;
    u16int *id_buff;

    for(i = 0; i < MAX_PORTS; i++) {

        if(ports_binded[i].port == sock->port_src) {
            id_buff = &ports_binded[i].id_buff[0];
            break;
        }
    }
    while(1) {

        for(int j = 0; j <= ports_binded[i].count; j++) {

            if( socket_pool[id_buff[j]].status == TCP_SYN_RECV) {
                return id_buff[j];
            }
        }
    }
}

u32int sock_recv(socket_s *sock, u8int *recv_buff) {

    u32int ret = 0;
    sock->buf_recv = recv_buff;

    while(!sock->recv_bytes) {
        slp_thrd(sock->thread);
    }
    ret = sock->recv_bytes;
    sock->recv_bytes = 0;

    return ret;
}

u32int ssock_send(socket_s *sock, u8int flags) {        //for system_call!!!!!!

    u32int size = sock->send_bytes;
    u32int sz = size;

    if(sock->protocol == TCP_TYPE) {

        if(size >= TCP_SYN_MSS) {

            sz = TCP_SYN_MSS;
        }
        sock->send_bytes = size - sz;

        if(!size) {
            sz = sizeof(tcp_s);
        }
        tcp_s *tcp = kcalloc(sz + sizeof(tcp_s), 1);

        if(!flags) {
            tcp->flags = TCP_FLAG_ACK;
        }
        else {
            tcp->flags = flags;
            if(flags == TCP_FLAG_FIN) {
                sock->status = TCP_FIN_WAIT;
            }
        }
        sock->flags = SOCK_FLAG_SEND;
        memcpy(tcp->data, sock->buf_send, sz);
        tcp_xmit(sock, tcp, sz);
        kfree(tcp);
    }
    else if(sock->protocol == UDP_TYPE) {

    }
    return sock->send_bytes;
}

u32int sock_send(socket_s *sock, u8int *data, u32int size, u8int flag) {

    u32int sz = size;

    if(sock->protocol == TCP_TYPE) {

        if(size >= TCP_SYN_MSS) {

            sz = TCP_SYN_MSS;
        }
        sock->send_bytes = size - sz;
        sock->buf_send = data;

        if(!size) {
            sz = sizeof(tcp_s);
        }
        tcp_s *tcp = kcalloc(sz + sizeof(tcp_s), 1);

        if(!flag) {
            tcp->flags = TCP_FLAG_ACK;
        }
        else {
            tcp->flags = flag;
            if(flag == TCP_FLAG_FIN) {
                sock->status = TCP_FIN_WAIT;
            }
        }
        sock->flags = SOCK_FLAG_SEND;
        memcpy(tcp->data, data, sz);
        tcp_xmit(sock, tcp, sz);
        kfree(tcp);
    }
    else if(sock->protocol == UDP_TYPE) {

    }
    return sock->send_bytes;
}

void sock_log(socket_s *s) {

    wprint("stat(%d), flg(%x), pd(%d), ps(%d), rcv_bs(%d), snd_bs(%d)\n", s->status, s->flags, s->port_dst, s->port_src, s->recv_bytes, s->send_bytes);
    return;
}
