
#include "net.h"
#include "timer.h"

extern u32int multi_task;
extern net_dev_s rtl8029;
extern socket_s socket_pool[];
extern bind_port_s port_binded[];

//u8int my_ip[] = {10, 11, 0, 14};
//u8int dst_ip[] = {10, 11, 0, 12};
//u8int my_ip[] = {10, 11, 0, 12};
u8int dst_ip_t[] = {10, 11, 0, 14};
u8int my_ip[] = {192, 168, 10, 15};
//u8int my_ip[] = {0};

//u8int vnet_ip[] = {10, 0, 2, 2};
u8int vnet_ip[] = {192, 168, 10, 1};
u8int zero_mac_addr[] = {0, 0, 0, 0, 0, 0};

mutex_s mtx_tcp;

void Ethernet_Recv(ether_s *eth, u32int len) {

    ether_s *pack = eth;// kcalloc(len, 1);
   // memcpy(pack, eth, len);

    void *data = (void *)pack + (sizeof(ether_s));
    u32int len_data = len - sizeof(ether_s);
//---------------------------------------------------
    //ethernet_showlog(pack, "recv");
    //ip_showlog(data, "recv");
//---------------------------------------------------
    if(ntohw(pack->type) == ARP_TYPE) {
        Arp_Recv(data, len_data);
    }
    if(ntohw(pack->type) == IP_TYPE) {
        Ip_Recv(data, len_data);
    }
    //kfree(pack);
    return;
}

u32int Ethernet_Send(u8int *dst_mac_addr, u16int protocol, u8int *data, u32int len) {

    u8int src_mac_addr[6];
    u32int l = ntohw(len);
    ether_s * eth = kmalloc(sizeof(ether_s) + len);
    void *data_eth = (void *) eth + sizeof(ether_s);
    
    get_mac_addr(&rtl8029, src_mac_addr);
    memcpy(eth->src_mac, src_mac_addr, 6);
    memcpy(eth->dst_mac, dst_mac_addr, 6);
    memcpy(data_eth, data, len);

    eth->type = htonw(protocol);
//------------------------------------------------------
   // ethernet_showlog(eth, "send");
//------------------------------------------------------

    rtl8029_send(eth, l + sizeof(ether_s));
    kfree(eth);

    return sizeof(ether_s) + l;
}

void ethernet_showlog(ether_s *eth, u8int *str) {

    monitor_str_write("----------------------------- ETHERNET ");
    monitor_str_write(str);
    monitor_str_write(" ------------------------------------\n");

    monitor_str_write("[DST_MAC:");
    for(int i = 0; i < 6; i++) {
        write_hex((u32int)eth->dst_mac[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);

    monitor_str_write("] [SRC_MAC:");
    for(int i = 0; i < 6; i++) {
        write_hex((u32int)eth->src_mac[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("\nTYPE [");
    write_hex((u32int)eth->type);
    monitor_str_write("] Start_addr_data[");
    write_hex((u32int)eth->data);

    monitor_str_write("]\n");
    monitor_str_write("--------------------------------------------------------------------------------\n");

    return;
}
//==================================  IP PROTOCOL ================================================

void Ip_Recv(ip_s *pack, u32int len) {

    *((u8int *)(&pack->version_ihl_ptr)) = ntohb(*((u8int*)(&pack->version_ihl_ptr)), 4);
    *((u8int *)(pack->flags_fragment_ptr)) = ntohb(*((u8int*)(pack->flags_fragment_ptr)), 3);

    if(pack->version == IP_TYPE_IPV4) {

        //void *data_ptr = (void *) pack + pack->ihl * 4;
        u32int data_len = ntohw(pack->length) - sizeof(ip_s);

//--------------------------------------------------------------------------------------------
        //ip_showlog(pack, "recv");
//--------------------------------------------------------------------------------------------

        if(pack->protocol == UDP_TYPE) {
            Udp_Recv(pack, data_len);
        }
        else if(pack->protocol == TCP_TYPE) {

            wprint("%d++ IP Recv..\n", curr_thread->id);
            Tcp_Recv(pack, data_len);         
        }
    }
    return;
}

u8int Ip_Send(u8int *dst_ip, u8int proto, void *data, u32int len) {

    u32int arp_sent = 3;
    ip_s   *pack = kmalloc(sizeof(ip_s) + len);
    memset(pack, 0, sizeof(ip_s));

    pack->version = IP_TYPE_IPV4;
    pack->ihl = 5;
    pack->tos = 0;
    pack->length = htonw(sizeof(ip_s) + len);
    pack->id = 0;                       // Used for ip fragmentation, don't care now
    pack->flags = 0;
    pack->fragment_offset_high = 0;
    pack->fragment_offset_low = 0;
    pack->ttl = 64;
    pack->protocol = proto;     //UDP_TYPE;

    //gethostaddr(my_ip);
    memcpy(pack->src_ip, my_ip, 4);
    memcpy(pack->dst_ip, dst_ip, 4);

    void *pack_data = (void *)pack + pack->ihl * 4;
    memcpy(pack_data, data, len);

    *((u8int *)(&pack->version_ihl_ptr)) =  htonb(*((u8int *)(pack->version_ihl_ptr)), 4);
    *((u8int *)(pack->flags_fragment_ptr)) = htonb(*((u8int *)(pack->flags_fragment_ptr)), 3);

    pack->checksum = 0;
    pack->checksum = ip_calc_checksum(0, (u8int*)pack, sizeof(ip_s));

//----------------------------------------------------------
    //ip_showlog(pack, "send");
//----------------------------------------------------------

    u8int dst_macaddr[6] = {0};

    while(!(arp_lookup(dst_macaddr, dst_ip))) {

        if(arp_sent != 0) {
            arp_sent--;
            monitor_str_write("HAVENT IP\n");
            Arp_Send(zero_mac_addr, dst_ip);
        }
        if(arp_sent == 0) {
            kfree(pack);
            kfree(data);
            monitor_str_write("IP send Fail!!!\n");
            return 0;
        }
    }
    u32int hex_ip = Ethernet_Send(dst_macaddr, IP_TYPE, (u8int *)pack, (u32int)htonw(pack->length));

    kfree(pack);
    return 1;
}

u16int ip_calc_checksum(u32int sum, u8int *pack, u32int len) {

    while(len >= 2) {

        sum += ((u16int)*pack << 8) | *(pack + 1);
        pack += 2;
        len -= 2;
    }
    if(len) {
        sum += (u16int)*pack << 8;
    }
    while(sum >> 16) {
        sum = (sum & 0x0000FFFF) + (sum >> 16);
    }

    return ~htonw((u16int)sum);
}

void ip_showlog(ip_s *ip, u8int *str) {

    u16int tmp = ip->fragment_offset_high;
    tmp = tmp << 8;
    tmp &= ip->fragment_offset_low;

    monitor_str_write("------------------------------ IP ");
    monitor_str_write(str);
    monitor_str_write(" -----------------------------------------\n");

    monitor_str_write("IPV[");
    write_hex((u32int)ip->version);
    //write_hex((u32int)ip->version_ihl_ptr[0]);
    monitor_str_write("] Head_Len[");
    write_hex((u32int)ip->ihl);
    monitor_str_write("] TOS[");
    write_hex((u32int)ip->tos);
    monitor_str_write("] Total_Len[");
    write_hex((u32int)ip->length);
    monitor_str_write("]\n ID[");
    write_hex((u32int)ip->id);
    monitor_str_write("] FLAGS[");
    write_hex((u32int)ip->flags);

    monitor_str_write("] OFFSET[");
    write_hex((u32int) tmp);
    monitor_str_write("] TTL[");
    write_hex((u32int)ip->ttl);
    monitor_str_write("] PROT[");
    write_hex((u32int)ip->protocol);
    monitor_str_write("] CHCKS[");
    write_hex((u32int)ip->checksum);
    monitor_str_write("]\n [SRC_IP: ");
    for(int i = 0; i < 4; i++) {
        write_hex((u32int)ip->src_ip[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("] [DST_IP: ");
    for(int i = 0; i < 4; i++) {
        write_hex((u32int)ip->dst_ip[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);

    monitor_str_write("]\n");
    monitor_str_write("--------------------------------------------------------------------------------\n");

    return;
}
//=================================== ARP PROTOCOL ================================

arp_table_s arp_table[ARP_TABLE_SIZE];
u32int  arp_table_size;
u32int  arp_table_curr;

u8int broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void Arp_Recv(arp_s *arp_pack, u32int len) {

    u8int dst_mac[6];
    u8int dst_ip[4];
//-------------------------------------------------------
    //arp_showlog(arp_pack, "recv");
//-------------------------------------------------------
    memcpy(dst_mac, arp_pack->sha, 6);
    memcpy(dst_ip, arp_pack->spa, 4);

    if(ntohw(arp_pack->oper) == ARP_REQUEST) {

        //u32int my_ip = 0x0E02000A;
        if(memcmp(arp_pack->tpa, &my_ip, 4)) {   

            get_mac_addr(&rtl8029, arp_pack->sha);
            memcpy(arp_pack->spa, my_ip, 4);

            memcpy(arp_pack->tha, dst_mac, 6);
            memcpy(arp_pack->tpa, dst_ip, 4);

            arp_pack->oper = htonw(ARP_REPLY);
            arp_pack->htype = htonw(ETH_TYPE);
            arp_pack->ptype = htonw(IP_TYPE);
            arp_pack->hlen = 6;
            arp_pack->plen = 4;

            Ethernet_Send(arp_pack->tha, ARP_TYPE, (u8int *)arp_pack, sizeof(arp_s));
        }
    }
    else if(ntohw(arp_pack->oper) == ARP_REPLY) {
        monitor_str_write("ARP reply\n");
    }
    else {

    }
    arp_lookup_add(dst_mac, dst_ip);

    return;
}

void Arp_Send(u8int *dst_mac, u8int *dst_ip) {

    arp_s *arp_pack = kmalloc(sizeof(arp_s));
    //get_mac_addr(arp_pack->sha);
    get_mac_addr(&rtl8029, arp_pack->sha);

    arp_pack->spa[0] = 10;
    arp_pack->spa[1] = 0;
    arp_pack->spa[2] = 2;
    arp_pack->spa[3] = 14;

    memcpy(arp_pack->tha, dst_mac, 6);
    memcpy(arp_pack->tpa, dst_ip, 4);

    arp_pack->htype = htonw(ETH_TYPE);
    arp_pack->ptype = htonw(IP_TYPE);
    arp_pack->oper = htonw(ARP_REQUEST);
    arp_pack->hlen = 6;
    arp_pack->plen = 4;
//-----------------------------------------------------
    arp_showlog(arp_pack, "send");
//-----------------------------------------------------

    Ethernet_Send(broadcast_mac, ARP_TYPE, (u8int *)arp_pack, sizeof(arp_s));
    kfree(arp_pack);
    return;
}

void arp_init(void) {

    u8int broadcast_ip[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    u8int broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    arp_table_curr = 0;

    arp_lookup_add(broadcast_mac, broadcast_ip);
    return;
}

u32int arp_lookup(u8int *ret_mac, u8int *ip_addr) {

    u32int ip_entr = *((u32int *)(ip_addr));
    for(u32int i = 0; i < ARP_TABLE_SIZE; i++) {

        if(arp_table[i].ip_addr == ip_entr) {

            memcpy(ret_mac, &arp_table[i].mac_addr, 6);
            return 1;
        }
    }
    return 0;
}

void arp_lookup_add(u8int *ret_mac, u8int *ip_addr) {

    memcpy(&arp_table[arp_table_curr].mac_addr, ret_mac, 6);
    memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);

    if(arp_table_curr >= ARP_TABLE_SIZE) {
        arp_table_curr = 0;
    }
    if(arp_table_size < ARP_TABLE_SIZE) {
        arp_table_size++;
    }
    return;
}

void arp_showlog(arp_s *arp_pack, u8int * str) {
    monitor_str_write("----------------------------- ARP ");
    monitor_str_write(str);
    monitor_str_write(" -----------------------------------------\n");

    monitor_str_write("[HARD_type: ");
    write_hex((u32int)arp_pack->htype);
    monitor_str_write("][PROT_type: ");
    write_hex((u32int)arp_pack->ptype);
    monitor_str_write("][HARD_len: ");
    write_hex((u32int)arp_pack->hlen);
    monitor_str_write("][PROT_len: ");
    write_hex((u32int)arp_pack->plen);

    monitor_str_write("][OPER: ");
    write_hex((u32int)arp_pack->oper);
    monitor_str_write("]\n");

    monitor_str_write(" SRC [MAC: ");
    for(int i = 0; i < 6; i++) {
        write_hex((u32int)arp_pack->sha[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("] [IP: ");
    for(int i = 0; i < 4; i++) {
        write_hex((u32int)arp_pack->spa[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("]\n");
    monitor_str_write(" DST [MAC: ");
    for(int i = 0; i < 6; i++) {
        write_hex((u32int)arp_pack->tha[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("] [IP: ");
    for(int i = 0; i < 4; i++) {
        write_hex((u32int)arp_pack->tpa[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("]\n");
    monitor_str_write("--------------------------------------------------------------------------------\n");

    return;
}
//============================================== UDP PROTOCOL ================================================

void Udp_Send(u8int *ip_dst, u16int port_src, u16int port_dst, void *data, u32int len) {

    u32int tmp_len = len + sizeof(udp_s);
    udp_s *pack = kmalloc(tmp_len);
    memset(pack, 0, sizeof(udp_s));

    pack->src_port = htonw(port_src);
    pack->dst_port = htonw(port_dst);
    pack->length = htonw(tmp_len);
    pack->checksum = 0; //udp_calc_checksum(pack);

    memcpy((void *)pack + sizeof(udp_s), data, len);
//------------------------------------------------------
    udp_showlog(pack, "send");
//------------------------------------------------------
    Ip_Send(ip_dst, UDP_TYPE, pack, tmp_len);
    kfree(pack);

    return;
}

void Udp_Recv(ip_s *ip, u32int len) {

    u8int ip_zer[4] = {0};
    u32int id;
    u32int ip_dst;
    socket_s *sock;
    udp_s *udp = (udp_s *)((u8int *)ip + ip->ihl * 4);

    u16int port_src = ntohw(udp->src_port);
    u16int port_dst = ntohw(udp->dst_port);
    u16int tmp_len = ntohw(udp->length);

    ip_dst = ip->dst_ip[0] << 24;
    ip_dst |= ip->dst_ip[1] << 16;
    ip_dst |= ip->dst_ip[2] << 8;
    ip_dst |= ip->dst_ip[3];

    u8int *tmp_data = (void *)udp + sizeof(udp_s);
//------------------------------------------------------
    udp_showlog(udp, "recv");
/*
    tftp_s *tf = tmp_data;
    len -= sizeof(udp_s);
    wprint("len udp = %d opcode(%d) block(%d)\n", len, ntohw(tf->opcode), ntohw(tf->block));

    for(int i = 0; i < len; i++) {
        wprint("%x ", tmp_data[i]);
    }
    wprint("\n%s\n", tf->data);
*/
//------------------------------------------------------

    if(ip_dst == 0 || ip_dst == 0xFFFFFFFF) {           //not necessary at all, but I need to work with bochs
        for(id = 0; id < TCP_MAX_CONNECT; id++) {

            sock = socket_pool + id;
            //wprint("port(%d)\n", sock->port_src);
            if(sock->port_src == port_dst) {
                sock->port_dst = port_src;
                memcpy(sock->ip_dst, ip->src_ip, 4);
                udp_to_app(id, udp, len);
                break;
            }
        }
        return;
    }
    else {
        for(id = 0; id < TCP_MAX_CONNECT; id++) {

            sock = socket_pool + id;
            if((memcmp(sock->ip_src, ip->dst_ip, 4)) && (sock->port_src == port_dst)) {
                sock->port_dst = port_src;
                memcpy(sock->ip_dst, ip->src_ip, 4);
                udp_to_app(id, udp, len);
                break;
            }
        }
        return;
    }

    if(port_dst == 68) {
        monitor_str_write("DHCP port\n");
    }
    return;
}

u16int udp_calc_checksum(udp_s *pack) {
    // UDP checksum is optional in IPv4
    u16int ret = ip_calc_checksum(0, (u8int*)pack, sizeof(udp_s));
    return ret;
}

void udp_showlog(udp_s *udp, u8int *str) {

    monitor_str_write("----------------------------- UDP ");
    monitor_str_write(str);
    monitor_str_write(" -----------------------------------------\n");

    monitor_str_write("[SRC_port: ");
    write_hex((u32int)udp->src_port);
    monitor_str_write("][DST_port: ");
    write_hex((u32int)udp->dst_port);
    monitor_str_write("][LENGTH: ");
    write_hex((u32int)udp->length);
    monitor_str_write("][CHCKS: ");
    write_hex((u32int)udp->checksum);
    monitor_str_write("][DATA_ADDR: ");
    write_hex((u32int)udp->data);

    monitor_str_write("]\n");
    monitor_str_write("--------------------------------------------------------------------------------\n");
    return;
}

void udp_to_app(u16int id, udp_s *pack, u32int len) {

    socket_s *sck = socket_pool + id;
    u8int *buff = (u8int *)pack + sizeof(udp_s);
    process_s *need_proc = sck->thread->process;

    len -= sizeof(udp_s);
    stop();
    physaddr curr_proc_dir = switch_cr3(need_proc->page_dir);
    memcpy(sck->buf_recv, buff, len);
    switch_cr3(curr_proc_dir);
    start();
    
    //sck->buf_recv += len;
    sck->recv_bytes += len;
    //switch_the_task(sck->thread);

    return;
}
//======================================== DHCP PROTOCOL ====================================================
//u8int ip_addr[4];
u8int ip_is_alloc = 0;

void Dhcp_Send(u8int *req_ip) {
    
    u8int dst_ip[4];
    memset(dst_ip, 0xFF, 4);
    dhcp_s *pack = kmalloc(sizeof(dhcp_s));
    memset(pack, 0, sizeof(dhcp_s));
    dhcp_make_pack(pack, 3, req_ip);
//----------------------------------------------------------
    dhcp_showlog(pack, "rqst");
//----------------------------------------------------------
    Udp_Send(dst_ip, 68, 67, pack, sizeof(dhcp_s));

    kfree(pack);
    return;
}

void Dhcp_Recv(dhcp_s *pack) {

    u8int *options = pack->options + 4;
//----------------------------------------------------------
    dhcp_showlog(pack, "recv");
//----------------------------------------------------------
    if(pack->op == DHCP_REPL) {

        u8int *type = dhcp_get_options(pack, 53);
        if(*type == 2) {
            Dhcp_Send((u8int *)(&pack->your_ip));
        }
        else if(*type == 5) {
            memcpy(my_ip, &pack->your_ip, 4);
            ip_is_alloc = 1;
        }
    }
    return;
}

u8int re_id[] = {192, 168, 10, 15};

void Dhcp_Discover(void) {

    u8int req_ip[4];
    u8int dst_ip[4];

    memset(req_ip, 0, 4);
   // memcpy(req_ip, re_id, 4);
    memset(dst_ip, 0xFF, 4);
    //memcpy(dst_ip, dhcp_ip, 4);

    dhcp_s *pack = (dhcp_s *)kmalloc(sizeof(dhcp_s));
    memset(pack, 0, sizeof(dhcp_s));
    dhcp_make_pack(pack, 1, req_ip);
//----------------------------------------------------------
    dhcp_showlog(pack, "dscv");
//----------------------------------------------------------
    Udp_Send(dst_ip, 68, 67, pack, sizeof(dhcp_s));

    return;
}

void *dhcp_get_options(dhcp_s *pack, u8int type) {

    u8int *opt = pack->options + 4;
    u8int curr = *opt;

    while(curr != 0xFF) {
        u8int len = *(opt + 1);
        if(curr == type) {
            void *ret = kmalloc(len);
            memcpy(ret, opt + 2, len);
            return ret;
        }
        opt += (2 + len);
    }
}

void dhcp_make_pack(dhcp_s *pack, u8int type, u8int *req_ip) {

    pack->op = 1;//DHCP_REQ;
    pack->hardw_type = 1;//HARDW_ETH_TYPE;
    pack->hardw_addr_len = 6;
    pack->hops = 0;
    pack->xid = htondw(DHCP_IDENT);
    pack->flags = htonw(0x8000);
    get_mac_addr(&rtl8029, pack->chaddr);
    
    //u8int dst_ip[4];
    //memset(dst_ip, 0xFF, 4);

    u8int *opt = pack->options;
    *((u32int *)(opt)) = 0x63825363; //htondw(0x63825363);
    opt += 4;

    // First option, message type = DHCP_DISCOVER/DHCP_REQUEST
    *(opt++) = 53;
    *(opt++) = 1;
    *(opt++) = type;

    // Client identifier
    *(opt++) = 61;
    *(opt++) = 7;
    *(opt++) = 1;
    get_mac_addr(&rtl8029, opt);
    opt += 6;

    // Client identifier
    *(opt++) = 50;
    *(opt++) = 4;
    //*(u32int *)(opt++) = htondw(0xC0A80A0F);
    memcpy((u32int *)(opt), req_ip, 4);
    opt += 4;

    // Host Name
    *(opt++) = 12;
    *(opt++) = 0x09; //strlen("PASTA_OS") + 1;
    memcpy(opt, "PASTA_OS", strlen("PASTA_OS"));
    opt += strlen("PASTA_OS");
    *(opt++) = 0;

    // Parameter request list
    *(opt++) = 55;
    *(opt++) = 8;
    *(opt++) = 1;     //Subnet Mask
    *(opt++) = 3;     //List of gateways, in order of preference.
    *(opt++) = 6;     //List of DNS servers
    *(opt++) = 15;    //Domain name
    *(opt++) = 44;    //List of NetBIOS Name Servers (NBNS)
    *(opt++) = 46;    //NetBIOS node type: 0x1 - B-node; 0x2 - P-node; 0x4 - M-node; 0x8 - H-node
    *(opt++) = 47;    //Область NetBIOS
    *(opt++) = 57;    //The maximum size of the DHCP message. The minimum value is 576

    *(opt++) = 0xFF;

    return;
}

void dhcp_showlog(dhcp_s *dhcp, u8int *str) {

    monitor_str_write("---------------------------- DHCP ");
    monitor_str_write(str);
    monitor_str_write(" -----------------------------------------\n");

    monitor_str_write("OP[");
    write_hex((u32int)dhcp->op);
    monitor_str_write("] HTYPE[");
    write_hex((u32int)dhcp->hardw_type);
    monitor_str_write("] HLEN[");
    write_hex((u32int)dhcp->hardw_addr_len);
    monitor_str_write("] HOPS[");
    write_hex((u32int)dhcp->hops);
    monitor_str_write("] XID[");
    write_hex((u32int)dhcp->xid);
    monitor_str_write("] SECS[");
    write_hex((u32int)dhcp->seconds);
    monitor_str_write("] FLAGS[");
    write_hex((u32int)dhcp->flags);
    monitor_str_write("]\n CIADDR[");
    write_hex((u32int)dhcp->client_ip);
    monitor_str_write("] YIADDR[");
    write_hex((u32int)dhcp->your_ip);
    monitor_str_write("] SIADDR[");
    write_hex((u32int)dhcp->server_ip);
    monitor_str_write("] GIADDR[");
    write_hex((u32int)dhcp->gateway_ip);
    monitor_str_write("]\n CHADDR[");

    for(int i = 0; i < 6; i++) {
        write_hex((u32int)dhcp->chaddr[i]);
        monitor_char_put(':');
    }
    monitor_char_put(0x08);
    monitor_str_write("]\n");
    monitor_str_write("--------------------------------------------------------------------------------\n");
    return;
}

//======================================== TCP PROTOCOL ======================================================

tcp_send_mode_s     tcp_send_m;
u8int               tcp_ack_sent;
u8int               tcp_use_resend;
u8int               ind = 0;

void TCP_Recv(void) {

    u32int len = curr_thread->msg_len; // - sizeof(tcp_s);
    ip_s *ip = curr_thread->msg_buff;

    //wprint("in TCP msg_buff(%x)\n", curr_thread->msg_buff);

    tcp_s *tcp = (tcp_s *)((u8int *)ip + ip->ihl * 4);//(ip->data);
    socket_s *st = 0;
    socket_s *pst;
    u16int   id; 
    u8int    tcp_flags;
    u8int    ip_zer[4] = {0};

    u16int port_src = ntohw(tcp->port_src);
    u16int port_dst = ntohw(tcp->port_dst);

    if(memcmp(ip->dst_ip, my_ip, 4) && memcmp(ip->dst_ip, ip_zer, 4)) {

        kfree(curr_thread->msg_buff);
        clean_mtx(&mtx_tcp);
        thread_exite(curr_thread);
    }
    len -= tcp_head_size(tcp);
//------------------------------------------------------------------
    //tcp_showlog(tcp, "recv");
//------------------------------------------------------------------
    tcp_flags = tcp->flags & (TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_FLAG_RST | TCP_FLAG_FIN);
    //tcp_send_m = TCP_SENDING_REPLY;
    tcp_ack_sent = 0;
    
    for(id = 0; id < TCP_MAX_CONNECT; id++) {

        pst = socket_pool + id;

        if((pst->status > TCP_CLOSED) && (memcmp(ip->src_ip, pst->ip_dst, 4)) && 
           (port_src == pst->port_dst) && (port_dst == pst->port_src)) {

            st = pst;
            st->flags = SOCK_FLAG_REPLY;
            //wprint("%d++find CONNECTION\n", curr_thread->id);
            break;
        }
    }
    if(!st) {

        if(tcp_flags == TCP_FLAG_SYN) {

            for(id = 0; id < TCP_MAX_CONNECT; ++id) {

                pst = socket_pool + id;

                if(pst->status == TCP_CLOSED) {

                    st = pst;
                    break;
                }
            }
            if(st && tcp_listen(id, tcp)) {            //if(st && tcp_listen(id, pack)) {

                st->status = TCP_SYN_RECV;
                st->flags = SOCK_FLAG_REPLY;
                st->event_time = tick;
                st->seq_num = 0;
                st->ack_num = ntohdw(tcp->seq_num) + 1;
                memcpy(st->ip_dst, ip->src_ip, 4);      //st->ip_dst  = ip->src_ip;
                st->port_dst = port_src;
                st->port_src = port_dst;

                tcp->flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);
            }
        }
        wprint("%d++ RST connection );\n", curr_thread->id);
    }
    else {          // The received packet refers to a known connection

        //wprint("RECV pack and have CONNECTION\n");

        if(tcp_flags & TCP_FLAG_RST) {  //if RST => kill connection
//--------------------------------------------------------------------------------
        wprint("%d++TCP_FLAG_RST\n", curr_thread->id);
//--------------------------------------------------------------------------------
            if((st->status == TCP_ESTABLISHED) || (st->status == TCP_FIN_WAIT)) {

                tcp_closed(id, 1);
            }
            st->status = TCP_CLOSED;

            kfree(curr_thread->msg_buff);
            clean_mtx(&mtx_tcp);
            thread_exite(curr_thread);
        }

        if(st->status != TCP_SYN_SENT) {
            if((ntohdw(tcp->seq_num) != st->ack_num) || (ntohdw(tcp->ack_num) != st->seq_num) || (!(tcp_flags & TCP_FLAG_ACK))) {
                wprint("%d++ SORT PACK\n", curr_thread->id);
                wprint("%d++tcp->seq_num(%d) st->ack_num(%d) tcp->ack_num(%d) st->seq_num(%d)",curr_thread->id, ntohdw(tcp->seq_num), st->ack_num, ntohdw(tcp->ack_num), st->seq_num);
                tcp_sort_pack(st, tcp, len);

                kfree(curr_thread->msg_buff);
                clean_mtx(&mtx_tcp);
                thread_exite(curr_thread);
            }
        }
        st->ack_num += len;

        if((tcp_flags & TCP_FLAG_FIN) || (tcp_flags & TCP_FLAG_SYN)) {
            st->ack_num++;
        }
        st->event_time = tick;

        if(st->status == TCP_SYN_SENT) {        // We have sent SYN. Now we are waiting for SYN/ACK

        wprint("%d++ RECV SYN|ACK\n", curr_thread->id);

            if(tcp_flags != (TCP_FLAG_ACK | TCP_FLAG_SYN)) {
                wprint("NO SYN|ACK :< close connection ;(\n");
                st->status == TCP_CLOSED;

                kfree(curr_thread->msg_buff);
                clean_mtx(&mtx_tcp);
                thread_exite(curr_thread);
            }
            st->ack_num = ntohdw(tcp->seq_num) + 1;

            tcp->flags = TCP_FLAG_ACK;
            tcp_xmit(st, tcp, 0);
            
            st->status = TCP_ESTABLISHED;
            tcp_from_app(id, tcp, 1);

            kfree(curr_thread->msg_buff);
            clean_mtx(&mtx_tcp);
            thread_exite(curr_thread);
        }
        else if(st->status == TCP_SYN_RECV) {   //We got SYN and sent SYN/ACK. Now we are waiting for ACK

            if(tcp_flags != TCP_FLAG_ACK) {
                st->status = TCP_CLOSED;

                kfree(curr_thread->msg_buff);
                clean_mtx(&mtx_tcp);
                thread_exite(curr_thread);
            }
            st->status = TCP_ESTABLISHED;
                    
            tcp_from_app(id, tcp, 0);

            kfree(curr_thread->msg_buff);
            clean_mtx(&mtx_tcp);
            thread_exite(curr_thread);
        }
        else if(st->status == TCP_ESTABLISHED) {        // Connection is established. Waiting for ACK or FIN/ACK

            if(tcp_flags == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {

                wprint("We RECV FIN|ACK\n");
                if(len) {
                    //tcp_to_app(id, pack, len);
                    tcp_to_app(id, tcp, len);
                }
                tcp->flags = TCP_FLAG_ACK | TCP_FLAG_FIN;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);

                st->status = TCP_CLOSED;
                tcp_closed(id, 0);
            }
            else if(tcp_flags == TCP_FLAG_ACK) {

                if(len) {
                    //tcp_to_app(id, pack, len);
                    wprint("%d++ there are bytes!\n", curr_thread->id);
                    tcp_to_app(id, tcp, len);
                }

                tcp_from_app(id, tcp, 0);

                if((len) && (!(st->flags & SOCK_FLAG_ACK)/*tcp_ack_sent*/)) {

                    tcp->flags = TCP_FLAG_ACK;
                    wprint("%d++ Have bytes, but not send\n", curr_thread->id);
                    //tcp_xmit(st, pack, 0);
                    tcp_xmit(st, tcp, 0);
                }
            }
            kfree(curr_thread->msg_buff);
            clean_mtx(&mtx_tcp);
            thread_exite(curr_thread);
        }
        else if(st->status == TCP_FIN_WAIT) {           // We have sent FIN/ACK. Waiting for FIN/ACK or ACK

            if(tcp_flags == (TCP_FLAG_ACK | TCP_FLAG_FIN)) {

                wprint("%d++ We FIN WAIT and RECV FIN|ACK\n", curr_thread->id);
                if(len) {
                    //tcp_to_app(id, pack, len);
                    wprint("%d++ Have bytes!\n", curr_thread->id);
                    tcp_to_app(id, tcp, len);
                }

                tcp->flags = TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);

                st->status = TCP_CLOSED;
                tcp_closed(id, 0);
            }
            else if((tcp_flags == TCP_FLAG_ACK) && (len)) {     //The remote node returns data from the buffer

                //tcp_to_app(id, pack, len);
                tcp_to_app(id, tcp, len);
                tcp->flags = TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);
            }
            kfree(curr_thread->msg_buff);
            clean_mtx(&mtx_tcp);
            thread_exite(curr_thread);
        }
    }
    kfree(curr_thread->msg_buff);
    clean_mtx(&mtx_tcp);
    thread_exite(curr_thread);
}

void Tcp_Recv(ip_s *ip, u32int len) {

    tcp_s *tcp = (tcp_s *)((u8int *)ip + ip->ihl * 4);//(ip->data);
    socket_s *st = 0;
    socket_s *pst;
    u16int   id; 
    u8int    tcp_flags;
    u8int    ip_zer[] = {0};

    u16int port_src = ntohw(tcp->port_src);
    u16int port_dst = ntohw(tcp->port_dst);

    if(memcmp(ip->dst_ip, my_ip, 4) && memcmp(ip->dst_ip, ip_zer, 4)) {
        return;
    }
    len -= tcp_head_size(tcp);
     if(port_dst == 3025 && len) {
        ind++;
    }
//------------------------------------------------------------------
    //tcp_showlog(tcp, "recv");
    wprint("to(%d), f(%x), id(%d)\n", port_dst, tcp->flags, curr_thread->id);
    //wprint("%s\n", tcp + (tcp->data_offset >> 2));
//------------------------------------------------------------------
    tcp_flags = tcp->flags & (TCP_FLAG_ACK | TCP_FLAG_PSH | TCP_FLAG_RST | TCP_FLAG_SYN | TCP_FLAG_FIN);
    //tcp_send_m = TCP_SENDING_REPLY;
    tcp_ack_sent = 0;
    
    for(id = 0; id < TCP_MAX_CONNECT; id++) {

        pst = socket_pool + id;

        if((pst->status > TCP_CLOSED) && (memcmp(ip->src_ip, pst->ip_dst, 4)) && 
           (port_src == pst->port_dst) && (port_dst == pst->port_src)) {

            st = pst;
            st->flags = SOCK_FLAG_REPLY;
            //wprint("find CONNECTION\n");
            break;
        }
    }
    if(!st) {

        if(tcp_flags == TCP_FLAG_SYN) {

            for(id = 0; id < TCP_MAX_CONNECT; ++id) {

                pst = socket_pool + id;

                if(pst->status == TCP_CLOSED) {

                    st = pst;
                    break;
                }
            }
            if(st && tcp_listen(id, tcp)) {            //if(st && tcp_listen(id, pack)) {

                st->status = TCP_SYN_RECV;
                st->flags = SOCK_FLAG_REPLY;
                st->event_time = tick;
                st->seq_num = 0;
                st->ack_num = ntohdw(tcp->seq_num) + 1;
                memcpy(st->ip_dst, ip->src_ip, 4);      //st->ip_dst  = ip->src_ip;
                st->port_dst = port_src;
                st->port_src = port_dst;

                tcp->flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);
            }
        }
        wprint("RST connection );\n");
    }
    else {          // The received packet refers to a known connection

        //wprint("RECV pack and have CONNECTION\n");

        if(tcp_flags & TCP_FLAG_RST) {  //if RST => kill connection
//--------------------------------------------------------------------------------
        wprint("TCP_FLAG_RST\n");
//--------------------------------------------------------------------------------
            if((st->status == TCP_ESTABLISHED) || (st->status == TCP_FIN_WAIT)) {

                tcp_closed(id, 1);
            }
            st->status = TCP_CLOSED;
            return;
        }

        if(st->status != TCP_SYN_SENT) {
            if((ntohdw(tcp->seq_num) != st->ack_num) || (ntohdw(tcp->ack_num) != st->seq_num) || (!(tcp_flags & TCP_FLAG_ACK))) {
                wprint("SORT PACK\n");
                wprint("tcp->seq_num(%d) st->ack_num(%d) tcp->ack_num(%d) st->seq_num(%d)", ntohdw(tcp->seq_num), st->ack_num, ntohdw(tcp->ack_num), st->seq_num);
                //tcp_sort_pack(st, tcp, len);
                return;
            }
        }
        st->ack_num += len;

        if((tcp_flags & TCP_FLAG_FIN) || (tcp_flags & TCP_FLAG_SYN)) {
            st->ack_num++;
        }
        st->event_time = tick;

        if(st->status == TCP_SYN_SENT) {        // We have sent SYN. Now we are waiting for SYN/ACK

        wprint("RECV SYN|ACK\n");

            if(tcp_flags != (TCP_FLAG_ACK | TCP_FLAG_SYN)) {
                wprint("NO SYN|ACK :< close connection ;(\n");
                st->status == TCP_CLOSED;
                return;
            }
            st->ack_num = ntohdw(tcp->seq_num) + 1;

            tcp->flags = TCP_FLAG_ACK;
            //tcp_xmit(st, pack, 0);
            tcp_xmit(st, tcp, 0);
            
            st->status = TCP_ESTABLISHED;
            //tcp_send_m = TCP_SENDING_SEND;

            //if(len) {
            //    tcp_to_app(id, tcp, len);
            //}
            //tcp_from_app(id, pack, 0);
            tcp_from_app(id, tcp, 1);

            return;
        }
        else if(st->status == TCP_SYN_RECV) {   //We got SYN and sent SYN/ACK. Now we are waiting for ACK

            if(tcp_flags != TCP_FLAG_ACK) {
                st->status = TCP_CLOSED;
                return;
            }
            st->status = TCP_ESTABLISHED;
                    
            tcp_from_app(id, tcp, 0);
            return;
        }
        else if(st->status == TCP_ESTABLISHED) {        // Connection is established. Waiting for ACK or FIN/ACK

            wprint("%d++ conn_est(%x)\n", curr_thread->id, tcp_flags);
            if(tcp_flags == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {

                wprint("We RECV FIN|ACK\n");
                if(len) {
                    //tcp_to_app(id, pack, len);
                    tcp_to_app(id, tcp, len);
                }
                tcp->flags = TCP_FLAG_ACK | TCP_FLAG_FIN;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);

                st->status = TCP_CLOSED;
                tcp_closed(id, 0);
            }
            else if(tcp_flags & TCP_FLAG_ACK) {

                if(len) {
                    //tcp_to_app(id, pack, len);
                    wprint("%d++ there are bytes!\n", curr_thread->id);
                    tcp_to_app(id, tcp, len);
                }
                if(tcp_flags & TCP_FLAG_PSH) {
                    
                    return;
                }
                tcp_from_app(id, tcp, 0);

                if((len) && (!(st->flags & SOCK_FLAG_ACK)/*tcp_ack_sent*/)) {

                    tcp->flags = TCP_FLAG_ACK;
                    wprint("Have bytes, but not send\n");
                    //tcp_xmit(st, pack, 0);
                    tcp_xmit(st, tcp, 0);
                }
            }
            return;
        }
        else if(st->status == TCP_FIN_WAIT) {           // We have sent FIN/ACK. Waiting for FIN/ACK or ACK

            if(tcp_flags == (TCP_FLAG_ACK | TCP_FLAG_FIN)) {

                wprint("We FIN WAIT and RECV FIN|ACK\n");
                if(len) {
                    //tcp_to_app(id, pack, len);
                    wprint("Have bytes!\n");
                    tcp_to_app(id, tcp, len);
                }

                tcp->flags = TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);

                st->status = TCP_CLOSED;
                tcp_closed(id, 0);
            }
            else if((tcp_flags == TCP_FLAG_ACK) && (len)) {     //The remote node returns data from the buffer

                //tcp_to_app(id, pack, len);
                tcp_to_app(id, tcp, len);
                tcp->flags = TCP_FLAG_ACK;
                //tcp_xmit(st, pack, 0);
                tcp_xmit(st, tcp, 0);
            }
            return;
        }
    }
    return;
}

void Tcp_Send(u16int id, tcp_s *tcp, u32int len, u8int options) { // "len" -  withoute size of tcp header

    wprint("IN TCP_SEND\n");
    //ip_s    *ip = pack;
    //tcp_s   *tcp = (tcp_s *)(ip->data);
    //tcp_s   *tcp = pack;

    socket_s *st = socket_pool + id;
    u8int   flags = TCP_FLAG_ACK;

    if(st->status != TCP_ESTABLISHED) {
        wprint("TCP_SEND: no ESTABLISHED!!!\n");
        return;
    }

    if(options & TCP_OPTION_PUSH) {
        flags |= TCP_FLAG_PSH;
    }

    if(options & TCP_OPTION_CLOSE) {

        flags |= TCP_FLAG_FIN;
        st->status = TCP_FIN_WAIT;
    }

    tcp->flags = flags;
    //tcp_xmit(st, pack, len);
    tcp_xmit(st, tcp, len);

    return;
}

u8int tcp_xmit(socket_s *st, tcp_s *tcp, u32int len) {

    //wprint("tcp_send_m(%d)\n", tcp_send_m);
    u8int stat = 1;
    u16int plen = (u16int) len;
    u16int tmp;

    //ip_s  *ip = pack;
    //tcp_s *tcp = (u8int *)ip + ip->ihl * 4;
    //tcp_s *tcp = pack;

    if(/*tcp_send_m == TCP_SENDING_SEND*/ st->flags & SOCK_FLAG_SEND) {        //SEND NEW packet
        
        //memcpy(ip->dst_ip, st->ip_dst, 4);  //ip->dst_ip = st->ip_dst;
        //memcpy(ip->src_ip, my_ip, 4);       //ip->src_ip = my_ip;
        //ip->protocol = TCP_TYPE;

        tcp->port_dst = htonw(st->port_dst);
        tcp->port_src = htonw(st->port_src);
    }
    else if(/*tcp_send_m == TCP_SENDING_REPLY*/ st->flags & SOCK_FLAG_REPLY) {  // We SEND the package IN RESPONSE

        tmp = tcp->port_dst;
        tcp->port_dst = tcp->port_src;
        tcp->port_src = tmp;
    }
    if(/*tcp_send_m != TCP_SENDING_RESEND*/ !(st->flags & SOCK_FLAG_RESEND)) {

        tcp->window = htonw(TCP_WINDOW_SIZE);
        tcp->urg_ptr = 0;

        tcp->seq_num = htondw(st->seq_num);
        tcp->ack_num = htondw(st->ack_num);

        wprint("%d++ send: seq_num(%d), ack_num(%d)| status(%d), flags(%x)\n", curr_thread->id, ntohdw(tcp->seq_num), ntohdw(tcp->ack_num), st->status, tcp->flags);
        wprint("%s\n", tcp->data);
    }

    if(tcp->flags & TCP_FLAG_SYN) {

        tcp->data_offset = (sizeof(tcp_s) + 4) << 2;   // (size / 4)  << 4;
        tcp->data[0] = 2;   //MSS
        tcp->data[1] = 4;   //length
        tcp->data[2] = TCP_SYN_MSS >> 8;
        tcp->data[3] = TCP_SYN_MSS & 0xFF;
        plen = 4; 
    }
    else {
        tcp->data_offset = (sizeof(tcp_s) << 2);
    }

    plen += sizeof(tcp_s);
    tcp->cksum = 0;
    tcp->cksum = ip_calc_checksum(plen + TCP_TYPE, (u8int*)tcp - 8, plen + 8);

    if(/*tcp_send_m == TCP_SENDING_SEND*/ st->flags & SOCK_FLAG_SEND) {

        stat = Ip_Send(st->ip_dst, TCP_TYPE, tcp, plen);
        //tcp_send_m = TCP_SENDING_RESEND;
        st->flags ^= SOCK_FLAG_SEND | SOCK_FLAG_RESEND;
    }
    else if(/*tcp_send_m == TCP_SENDING_REPLY*/st->flags & SOCK_FLAG_REPLY) {

        Ip_Send(st->ip_dst, TCP_TYPE, tcp, plen);
        //wprint("status = ESTABL\n");
        //tcp_send_m = TCP_SENDING_RESEND;
        st->flags ^= SOCK_FLAG_REPLY | SOCK_FLAG_RESEND;
    }
    else if(/*tcp_send_m == TCP_SENDING_RESEND*/ st->flags & SOCK_FLAG_RESEND) {
        Ip_Send(st->ip_dst, TCP_TYPE, tcp, plen);
    }

    st->seq_num += len;
    if((tcp->flags & TCP_FLAG_SYN) || (tcp->flags & TCP_FLAG_FIN)) {
        st->seq_num++;
    }   
    if((tcp->flags & TCP_FLAG_ACK) && (stat)) {
        //tcp_ack_sent = 1;
        st->flags |= SOCK_FLAG_ACK;
    }

    return stat;
}

u8int tcp_sort_pack(socket_s *sock, tcp_s *tcp, u32int len) {    // "len" -  withoute size of tcp header

    u8int *buf_send = sock->buf_send;
    u8int *buf_recv = sock->buf_recv;

    u32int tcp_ack = htondw(tcp->ack_num);
    u32int tcp_seq = htondw(tcp->seq_num);
    u16int slen = ntohw(tcp->window);

    if(slen > TCP_SYN_MSS) {
        slen = TCP_SYN_MSS;
    }
    //tcp_send_m = TCP_SENDING_RESEND;
    sock->flags |= SOCK_FLAG_RESEND;

    if(tcp_ack != sock->seq_num) {
        //отправляем предыдущий пакет
        tcp_s *dst_mem = kmalloc(TCP_SYN_MSS + sizeof(tcp_s));
        dst_mem->port_src = tcp->port_dst;
        dst_mem->port_dst = tcp->port_src;
        dst_mem->window = htonw(TCP_WINDOW_SIZE);
        dst_mem->urg_ptr = 0;
        dst_mem->seq_num = tcp->ack_num;
        dst_mem->ack_num = htondw(sock->ack_num);
        dst_mem->data_offset = (sizeof(tcp_s) << 2);
        dst_mem->flags = TCP_FLAG_ACK; 

        if(sock->need_send_bytes < (tcp_ack - sock->start_seq) + slen) {

            slen = sock->need_send_bytes - (tcp_ack - sock->start_seq);
        }
        memcpy(dst_mem->data, buf_send + (tcp_ack - sock->start_seq), slen);
        dst_mem->cksum = 0;
        //tcp->cksum = ip_calc_checksum(slen + TCP_TYPE, (u8int*)dst_mem - 8, slen + 8);

        Ip_Send(sock->ip_dst, TCP_TYPE, dst_mem, slen);
        kfree(dst_mem);
    }

    if(tcp_seq != sock->ack_num) {
        if(tcp_seq < sock->ack_num) {
            return 1;
        }

    //!!!Attention!!! this is written quickly and has not been tested. This is not a working code!!!
      /*u8int *tmp_mem;
        u32int i;
        u32int sum_len;
        tcp_eque_s *eque = sock->eque;

        /*
        for(i = 0; i < TCP_MAX_EQUE; i++) {                 //This cycle can (should) be deleted

            if(tcp_seq + len == eque[i].seq_num) {

                sum_len = len + eque[i].len;
                tmp_mem = kmalloc(sum_len);

                memcpy(tmp_mem, tcp->data, len);
                memcpy(tmp_mem + len, eque[i].addr, eque[i].len);
                kfree(eque[i].addr);

                eque[i].seq_num = tcp_seq;
                eque[i].len = sum_len;
                eque[i].addr = tmp_mem;

                goto tcp_srt;
            }
            else if(eque[i].seq_num + eque[i].len == tcp_seq) {

                sum_len = len + eque[i].len;
                tmp_mem = kmalloc(sum_len);

                memcpy(tmp_mem, eque[i].addr, eque[i].len);
                memcpy(tmp_mem + eque[i].len, tcp->data, len);
                kfree(eque[i].addr);

                eque[i].addr = tmp_mem;
                eque[i].len = sum_len;

                goto tcp_srt;
            }
        }
        */
        /*for(i = 0; i < TCP_MAX_EQUE; i++) {

            if(eque[i].len == 0) {
                eque[i].len = len;
                eque[i].addr = kmalloc(len);
                memcpy(eque[i].addr, tcp->data, len);
                eque[i].seq_num = tcp_seq;

                goto tcp_srt;
            }
            return 0;           // error: Have not space in socket->eque;
        }
    tcp_srt:
        i = 0;
        do {
            if(eque[i].len == 0) {
                continue;
            }
            for(int j = 0; j < TCP_MAX_EQUE; j++) {
                //u16int n = j;
                if(j == i) {
                    continue;
                }
                if(eque[j].len == 0) {
                    continue;
                }
                if(eque[i].seq_num == eque[j].seq_num + eque[j].len) {

                    sum_len = eque[i].len + eque[j].len;
                    tmp_mem = kmalloc(sum_len);

                    memcpy(tmp_mem, eque[j].addr, eque[j].len);
                    memcpy(tmp_mem + eque[j].len, eque[i].addr, eque[i].len);
                    kfree(eque[j].addr);
                    kfree(eque[i].addr);
                    memset(&eque[i], 0, sizeof(tcp_eque_s));

                    eque[j].addr = tmp_mem;
                    eque[j].len = sum_len;
                }
                else if(eque[i].seq_num + eque[i].len == eque[j].seq_num) {

                    sum_len = eque[i].len + eque[j].len;
                    tmp_mem = kmalloc(sum_len);

                    memcpy(tmp_mem, eque[i].addr, eque[i].len);
                    memcpy(tmp_mem + eque[i].len, eque[j].addr, eque[j].len);
                    kfree(eque[j].addr);
                    kfree(eque[i].addr);
                    memset(&eque[j], 0, sizeof(tcp_eque_s));

                    eque[i].addr = tmp_mem;
                    eque[i].len = sum_len;
                }
            }
            i++;
        }while(i < TCP_MAX_EQUE);
    */
    }
    return 1;
}

u8int tcp_listen(u16int id, tcp_s *tcp) {

    //tcp_s *tcp = (tcp_s *)(pack->data);
    u16int port_dst = ntohw(tcp->port_dst);

    for(int i = 0; i < MAX_PORTS; i++) {

        if((port_dst == ports_binded[i].port) && (ports_binded[i].count < ports_binded[i].max_ports)) {

            for(int a = 0; a < MAX_CONNECT; a++) {

                if(ports_binded[i].id_buff[a] == 0) {

                    ports_binded[i].id_buff[a] = id;
                    ports_binded[i].count++;
                    return 1;
                }
            }
        }
    }
    return 0;
}

void udp_listen(u16int id, udp_s *udp) {

    u16int port_dst = ntohw(udp->dst_port);
    for(int i = 0; i < MAX_PORTS; i++) {

        if((port_dst == ports_binded[i].port) && (ports_binded[i].count < ports_binded[i].max_ports)) {

            for(int j = 0; j < MAX_CONNECT; j++) {

                if(ports_binded[i].id_buff[j] == 0) {

                    ports_binded[i].id_buff[j] = id;
                    ports_binded[i].count++;
                    return;
                }
            }
        }
    }
    return;
}

void tcp_closed(u16int id, u8int reset) {    //if reset == 1 => uncorrect connection

    for(int i = 0; i < MAX_PORTS; i++) {

        if(socket_pool[id].port_src == ports_binded[i].port) {

            for(int a = 0; a < MAX_CONNECT; a++) {

                if(ports_binded[i].id_buff[a] == id) {

                    ports_binded[i].id_buff[a] = 0;
                    return;
                }
            }
        }
    }
    return;
}

void tcp_to_app(u16int id, void *pack, u32int len) {

    socket_s *st = socket_pool + id;
    process_s *need_proc = st->thread->process;
    tcp_s *tcp = pack;
    u8int *buff = tcp_get_data(pack);

    stop();
    physaddr curr_proc_dir = switch_cr3(need_proc->page_dir);
    memcpy(st->buf_recv + st->recv_bytes, buff, len);
    switch_cr3(curr_proc_dir);
    start();

    st->recv_bytes += len;

    wprint("!!!%s\n", st->buf_recv);

    if(tcp->flags & TCP_FLAG_PSH) {

        wprint("%d++ switch to app | ", curr_thread->id);

        if(get_curr_thread() == st->thread) {
            wprint("t == t\n");
            wup_thrd(st->thread);
            return;
        }
        wprint("multtask(%d)\n", multi_task);
        //switch_the_task(st->thread);
        wup_thrd(st->thread);
        return;
        wprint("%d from app\n", curr_thread->id);
    }
    return;
}

void tcp_from_app(u16int id, void *pack, u8int retrns) {

    u8int  opt = 0;
    socket_s *st = socket_pool + id;
    process_s *need_proc = st->thread->process;
    tcp_s *tcp = pack;                  //+ sizeof(ip_s);
    u32int s = tcp_head_size(tcp);
    u32int count = TCP_SYN_MSS;
    u8int *data;
    void *new_pack;

    if(st->send_bytes < TCP_SYN_MSS) {
        count = st->send_bytes;
    }
    new_pack = kmalloc(count + s);
    data = new_pack + s;

    memcpy(new_pack, tcp, s);
    
    stop();
    physaddr curr_proc_dir = switch_cr3(need_proc->page_dir);
    memcpy(data, (st->buf_send + st->seq_num - st->start_seq - 1), count);
    switch_cr3(curr_proc_dir);
    start();

    if(!st->send_bytes) {
//-----------------------------------------------------------------------------
        wprint("%d++ BUFF EMPTY\n", curr_thread->id);
//-----------------------------------------------------------------------------
        if(retrns == 1 || (st->status != TCP_FIN_WAIT)) {
            kfree(new_pack);
            //switch_the_task(st->thread);
            return;
        }
        //st->buf_send = 0x00;
        opt |= TCP_OPTION_CLOSE;

       // if(!st->send_bytes) {
            //wprint("%s\n", tcp->data);
       //     switch_the_task(st->thread);
        //    return;
        //}
    }
    wprint("%d++ SEND from app\n", curr_thread->id);
    Tcp_Send(id, new_pack, count, opt); // "len" -  withoute size of tcp header
    st->send_bytes -= count;
    kfree(new_pack);

    if(!st->send_bytes) {
        //switch_the_task(st->thread);
    }

    return;
}

void tcp_showlog(tcp_s *tcp, u8int *str) {

    wprint("----------------------------- TCP %s -----------------------------------------\n", str);
    
    wprint("port_src(%d), port_dst(%d)| seq_num(%d), ack_num(%d)\n", ntohw(tcp->port_src), ntohw(tcp->port_dst), ntohdw(tcp->seq_num), ntohdw(tcp->ack_num));
    wprint("flags(%x)| window(%d)| cksum(%d)| urg(%x)| offs(%d)\n", tcp->flags, ntohw(tcp->window), ntohw(tcp->cksum), ntohw(tcp->urg_ptr), tcp->data_offset >> 2);
    wprint("--------------------------------------------------------------------------------\n");
    return;
}
//======================================== TFTP RPOTOCOL =====================================================

u32int Tftp(u8int *filename, u16int my_port, u8int option, u8int *data, u32int send_len) {

    u8int *org_data = data;
    u16int last_block = 0;
    u32int len = 0;

    u16int idsock = socket(4, 1, 0x11);
    socket_s *so = socket_pool + idsock;
    u8int errsock = sock_bind(idsock, my_ip, my_port);
    //wprint("stat(%x) prot(%x), type(%x), spa_add(%x),\n ip_src(%d.%d.%d.%d), por_src(%d)\n", so->status, so->protocol, so->sock_type, so->space_addr, so->ip_src[0], so->ip_src[1], so->ip_src[2], so->ip_src[3], so->port_src);
    errsock = sock_listen(idsock, 1);
    u8int *sock_buf = kcalloc(512, 1);
    so->buf_recv = sock_buf;
    //wprint("listen(%x)\n\n", errsock);
    if(option == TFTP_WRQ) {
        so->buf_send = data;
        so->send_bytes = send_len;
    }

    Tftp_Request(filename, my_port, option);

    while(1) {
        u32int n = Tftp_Handler(idsock, &last_block);

        if(n == 3) {

            memcpy(data, so->buf_recv + 4, so->recv_bytes - 4);
            data += so->recv_bytes;
            len += so->recv_bytes - 4;
            so->recv_bytes = 0;
        }
        else if(!n) {

            memcpy(data, so->buf_recv + 4, so->recv_bytes - 4);
            len += so->recv_bytes - 4;
            break;
        }
        else if(n == 4) {
            so->buf_send + 512;
            so->send_bytes -= 512;
            len += 512;
        }
        else if(n == 6) {

        }
        else if(n == 7) {
            so->buf_send + 512;
        }
        else if(n == 8) {
            len += so->send_bytes;
            break;
        }
        else if(n == 5) {

            u16int err_code = so->buf_recv[2] << 8;
            err_code |= so->buf_recv[3];
            wprint("%s\n", so->buf_recv + 4);
        }
    }
    //wprint("%s\n", so->buf_recv+4);
    sock_close(idsock);

tftp_ext:
    kfree(sock_buf);
    return len;
}

void Tftp_Request(u8int *fname, u16int port_src, u8int option) {

    //u8int *asc = "netascii";
    u8int *oct = "octet";
    u32int len = 2 + strlen(fname) + 1 + strlen(oct) + 1;
    u8int *pssc = kmalloc(len);
    u8int *pack = pssc;

    *pssc++ = 0x00;
    *pssc++ = option;

    memcpy(pssc, fname, strlen(fname));
    pssc += strlen(fname);
    *(pssc++) = 0x00;
    memcpy(pssc, oct, strlen(oct));
    pssc += strlen(oct);
    *pssc = 0x00;

    u16int t = pack[1];
    u8int *fn = &pack[2];
    u8int *frm = fn + 1 + strlen(fn);
   // wprint("type:%x / file:%s / form:%s / \n", t, fn, frm);

    Udp_Send(vnet_ip, port_src, 69, pack, len);

    kfree(pack);
    return;
}

u32int Tftp_Handler(u16int idsock, u16int *last_block) {

    socket_s *socket = socket_pool + idsock;
    tftp_s *tftp = (tftp_s *)socket->buf_recv;
    tftp_s *data = kcalloc(516, 1);
    u32int len = 0;

    if(!socket->recv_bytes) {
        return 1;
    }
    else if(tftp->opcode == TFTP_DATA) {

    
        if(ntohw(tftp->block) != (*last_block)++) {
          
            data->opcode = TFTP_ACK;
            data->block = htonw(*last_block);
            Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 4);

            kfree(data);
            return 6;
        }

        data->opcode = TFTP_ACK;
        data->block = tftp->block;
        Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 4);
        if(socket->recv_bytes < 516) {
            kfree(data);
            return 0;
        }
        kfree(data);
        return 3;
    }
        else if(tftp->opcode == TFTP_ACK) {

            s32int len = 512;
            data->opcode = TFTP_WRQ;            //in big-endian;
            u32int sock_bytes = socket->send_bytes;
            u8int *sock_buf = socket->buf_send;

            if(sock_bytes == 0) {               //the end sending;
                kfree(data);
                return 8;
            }

            if((sock_bytes - len) <= 0) {

                if(ntohw(tftp->block) == 0) {
                    *last_block = 1;
                    data->block = htonw(*last_block);
                    memcpy(data->data, sock_buf, sock_bytes);
                    Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, sock_bytes + 4);
                }
                else if(ntohw(tftp->block) == *last_block) {
                    *(last_block)++;
                    data->block = htonw(*last_block++);
                    memcpy(data->data, sock_buf + 512, sock_bytes);
                    Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, sock_bytes + 4);
                }
                else {
                    data->block = htonw(*last_block);
                    memcpy(data->data, sock_buf, 512);
                    Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 516);
                    kfree(data);
                    return 6;
                }
                kfree(data);
                return 7;
            }

            if(ntohw(tftp->block) == 0) {
                *last_block = 1;
                data->block = htonw(*last_block);
                memcpy(data->data, sock_buf, 512);
                Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 516);
            }
            else if(ntohw(tftp->block) == *last_block) {
                *(last_block)++;
                data->block = htonw(*last_block);
                memcpy(data->data, sock_buf + 512, 512);
                Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 516);
            }
            else {
                data->block = htonw(*last_block);
                memcpy(data->data, sock_buf, 512);
                Udp_Send(socket->ip_dst, socket->port_src, socket->port_dst, data, 516);
                kfree(data);
                return 6;
            }
            kfree(data);
            return 4;
        }
        else if(tftp->opcode == TFTP_RRQ) {              //Read request

            kfree(data);
            return 1;
        }
        else if(tftp->opcode == TFTP_WRQ) {             //Write request

            kfree(data);
            return 2;
        }
        else if(tftp->opcode == TFTP_ERROR) {

            kfree(data);
            return 5;
        }

    kfree(data);
    return 5;
}

//======================================== FTP PROTOCOL =======================================================
u8int ftp_create_mode(u8int mode, u16int my_data_port, u8int *buf_mode) {

   // u8int *reserv = buf_mode;
   u32int sz;

    if(mode == FTP_ACTIVE) {

        u8int *himyport = kcalloc(4, 1);
        u8int *lomyport = kcalloc(4, 1);
        u8int *ip0 = kcalloc(8, 1);
        u8int *ip1 = kcalloc(8, 1);
        u8int *ip2 = kcalloc(8, 1);
        u8int *ip3 = kcalloc(8, 1);

        itoa(my_data_port / 256, himyport);
        itoa(my_data_port % 256, lomyport);
        itoa(my_ip[0], ip0);
        itoa(my_ip[1], ip1);
        itoa(my_ip[2], ip2);
        itoa(my_ip[3], ip3);

        sz = strlen("PORT ");
        memcpy(buf_mode, "PORT ", sz);
        buf_mode += sz;

        sz = strlen(ip0);
        memcpy(buf_mode, ip0, sz);
        buf_mode[sz] = ',';
        buf_mode += sz + 1;
        sz = strlen(ip1);
        memcpy(buf_mode, ip1, sz);
        buf_mode[sz] = ',';
        buf_mode += sz + 1;
        sz = strlen(ip2);
        memcpy(buf_mode, ip2, sz);
        buf_mode[sz] = ',';
        buf_mode += sz + 1;
        sz = strlen(ip3);
        memcpy(buf_mode, ip3, sz);
        buf_mode[sz] = ',';
        buf_mode += sz + 1;

        sz = strlen(himyport);
        memcpy(buf_mode, himyport, sz);
        buf_mode[sz] = ',';
        buf_mode += sz + 1;
        sz = strlen(lomyport);
        memcpy(buf_mode, lomyport, sz);
        buf_mode += sz;
        sz = strlen("\r\n");
        memcpy(buf_mode, "\r\n", sz);
        //wprint("%s| hi(%s) lo(%s)\n", reserv, himyport, lomyport);
        kfree(himyport);
        kfree(lomyport);
        kfree(ip0);
        kfree(ip1);
        kfree(ip2);
        kfree(ip3);
    }
    else if(mode == FTP_PASSIVE) {

        sz = strlen("PASV");
        memcpy(buf_mode, "PASV", sz);
        buf_mode += sz;

        sz = strlen("\r\n");
        memcpy(buf_mode, "\r\n", sz);
    }
    return 1;
}

u8int ftp_create_log(u8int *name, u8int *passw, u8int *buf_name, u8int *buf_passw) {

    u32int sz1 = strlen("\r\n");
    u32int sz = strlen("USER ");

    memcpy(buf_name, "USER ", sz);
    buf_name += sz;
    sz = strlen(name);
    memcpy(buf_name, name, sz);
    buf_name += sz;
    memcpy(buf_name, "\r\n", sz1 + 1); 

    sz = strlen("PASS ");
    memcpy(buf_passw, "PASS ", sz);
    buf_passw += sz;
    sz = strlen(passw);
    memcpy(buf_passw, passw, sz);
    buf_passw += sz;
    memcpy(buf_passw, "\r\n", sz1 + 1);

    return 1;
}

u16int ftp_pasv_reply(u8int *repl, u8int *serv_ip) {

    u8int i;
    u16int prt_low;
    u16int prt_high;
    //u8int *repl = "(192, 168, 10, 3, 143, 122).\r\n";
    u8int *ts1 = strsep(&repl, "(");
    u32int num = atoi(ts1);
    //wprint("%s | %s | %d\n", repl, ts1, num);
    //while(*ts != '.') {
    for(i = 0; i < 5; i++) {

        ts1 = strsep(&repl, " ");
        num = atoi(ts1);
        if(i == 4) {
            prt_low = num;
            break;
        }
        serv_ip[i] = num;
    }
        ts1 = strsep(&repl, ")");
        prt_high = atoi(ts1);

    prt_low *= 256;
    prt_low += prt_high;
    //wprint("%d.%d.%d.%d | %d  %d\n", serv_ip[0], serv_ip[1], serv_ip[2], serv_ip[3], prt_low, prt_high);

    return prt_low;
}

u16int ftp_contrlp_conn(u8int *name, u8int *passw, u16int my_inf_port, u8int *serv_ip, u16int *serv_data_port) {

    u16int serv_port = 21;
    u8int *mode = kcalloc(256, 1);
    u8int *user_name = kcalloc(256, 1);
    u8int *user_passw = kcalloc(256, 1);

    //ftp_create_mode(FTP_ACTIVE, my_data_port, mode);
    ftp_create_mode(FTP_PASSIVE, 0, mode);
    ftp_create_log(name, passw, user_name, user_passw);
//----------------------------------------------------------- 

    u16int idsock = socket(4, 1, 0x06);
    socket_s *sock = socket_pool + idsock;
    u8int err = sock_bind(idsock, my_ip, my_inf_port);
    err = sock_listen(idsock, 1);

    u8int *recv_buff = kcalloc(512, 1);
    u8int *send_buff = kcalloc(512, 1);
    sock->buf_recv = recv_buff;
    sock->buf_send = send_buff;

    sock->port_dst = serv_port;
    sock->start_seq = 0;
    sock->seq_num = 0;
    sock->ack_num = 0;
    memcpy(sock->ip_dst, serv_ip, 4);
    sock->status = TCP_SYN_SENT;
    sock->flags = SOCK_FLAG_SEND;

    u32int tmp = sock_send(sock, "", 0, TCP_FLAG_SYN);
    tmp = sock_recv(sock, recv_buff);
    memset(recv_buff, 0, 512);

    tmp = sock_send(sock, user_name, strlen(user_name), 0);
    tmp = sock_recv(sock, recv_buff);
    memset(recv_buff, 0, 512);

    tmp = sock_send(sock, user_passw, strlen(user_passw), 0);
    tmp = sock_recv(sock, recv_buff);
    memset(recv_buff, 0, 512);

    tmp = sock_send(sock, mode, strlen(mode), 0);
    tmp = sock_recv(sock, recv_buff);

    *serv_data_port = ftp_pasv_reply(sock->buf_recv, serv_ip);
    wprint("%s\n", sock->buf_recv);
    wprint("%d.%d.%d.%d | %d\n", serv_ip[0], serv_ip[1], serv_ip[2], serv_ip[3], *serv_data_port);

    kfree(mode);
    kfree(user_name);
    kfree(user_passw);
    kfree(recv_buff);
    kfree(send_buff);

    return idsock;
}

u16int ftp_datap_conn(u16int my_port, u16int serv_port, u8int *serv_ip) {

    u16int idsock = socket(4, 1, 0x06);
    socket_s *sock = socket_pool + idsock;
    u8int err = sock_bind(idsock, my_ip, my_port);
    err = sock_listen(idsock, 1);

    u8int *recv_buff = kcalloc(512, 1);
    u8int *send_buff = kcalloc(512, 1);
    sock->buf_recv = recv_buff;
    sock->buf_send = send_buff;

    sock->port_dst = serv_port;
    sock->start_seq = 0;
    sock->seq_num = 0;
    sock->ack_num = 0;
    memcpy(sock->ip_dst, serv_ip, 4);
    sock->status = TCP_SYN_SENT;
    sock->flags = SOCK_FLAG_SEND;

    u32int tmp = sock_send(sock, "", 0, TCP_FLAG_SYN);

    kfree(recv_buff);
    kfree(send_buff);

    return idsock;
}

u8int ftp_close(u16int id_sock_contr, u16int id_sock_data) {

    u32int tmp;
    u8int ret = 0;
    socket_s *con_sock = socket_pool + id_sock_contr;
    socket_s *dat_sock = socket_pool + id_sock_data;

    tmp = sock_send(dat_sock, "", 0, TCP_FLAG_FIN);
    if(tmp) {
        ret += 1;
    }
    while(dat_sock->status != TCP_CLOSED) {}
    sock_close(id_sock_data);

    tmp = sock_send(con_sock, "", 0, TCP_FLAG_FIN);
    if(tmp) {
        ret += 2;
    }
    while(con_sock->status != TCP_CLOSED) {}
    sock_close(id_sock_contr);

    return ret;

}

void Ftp_file_recv(u8int *name, u8int *passw, u8int *file_name, u16int my_inf_port, u16int my_data_port, u8int *serv_ip, u8int *recv_buff) {

    u16int serv_data_port = 0;
    u16int inf_sock_id = ftp_contrlp_conn(name, passw, my_inf_port, serv_ip, &serv_data_port);
    u16int dat_sock_id = ftp_datap_conn(my_data_port, serv_data_port, serv_ip);

    socket_s *sock_inf = socket_pool + inf_sock_id;
    socket_s *sock_data = socket_pool + dat_sock_id;

    socket_s *res = kmalloc(sizeof(socket_s));

    u8int *recv_inf = kcalloc(512, 1);
    sock_inf->buf_recv = recv_inf;
    sock_data->buf_recv = recv_buff;
    sock_data->recv_bytes = 0;

    while(sock_data->status != TCP_ESTABLISHED) {}
    //while(1){}
    wprint("data CONNECT OK\n");
    memset(recv_inf, 0, 512);
    sock_send(sock_inf, "TYPE I\r\n", 13, 0);
    sock_recv(sock_inf, recv_inf);
    memset(recv_inf, 0, 512);

   // sock_send(sock_inf, "RETR Hi.txt\r\n", 14, 0);
    sock_send(sock_inf, "NLST\r\n", 7, 0);
    //sock_send(sock_inf, "NLST\r\n", 7, 0);
    //sock_recv(sock_inf, recv_inf);
    sock_send(sock_inf, "", 0, TCP_FLAG_ACK);
    //u32int tmp = sock_recv(sock_data, recv_buff);

    wprint("ind(%d)\n", ind);
    //while(1) {}
    //memcpy(res, sock_data, sizeof(socket_s));
    //sock_log(res);
    wprint("??%s | (%d) | a(%x) | ai(%x)\n", sock_data->buf_recv, sock_data->recv_bytes, socket_pool, sock_inf);
    sock_log(sock_data);
    wprint("list(%d), wlist(%d)\n", list_size(&thread_list), list_size(&thread_wait));
   // wprint("inf(%x), dat(%x), id_inf(%d), id_dat(%d), i_r(%d), d_r(%d)\n", sock_inf, sock_data, inf_sock_id, dat_sock_id, sock_inf->recv_bytes, sock_data->recv_bytes);
    while(sock_data->recv_bytes == 0) {}
    //sock_log(sock_data);
    //sock_log(sock_inf);

    wprint("$$%s", recv_inf);

    //sock_recv(sock_data, recv_buff);
    //wprint("??%s, (%d), dat(%x)\n", recv_buff, sock_data->recv_bytes, sock_data);

    wprint("data_conn stat(%d)\n", sock_data->status);
    

    //ftp_close(inf_sock_id, dat_sock_id);

    while(1){}
    kfree(recv_inf);
    return;
}