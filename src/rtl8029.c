#include "rtl8029.h"

pci_dev_s   pci_rtl8029;
net_dev_s   rtl8029;
u32int      ptr_curr_pack;

volatile u8int       NetRxPacks[1568*4];       //Recv Buffer
extern socket_s socket_pool[32];


void rtl8029_init (void) {

    pci_rtl8029 = pci_get_device(RTL8029_VENDOR_ID, RTL8029_DEVICE_ID, -1);
    u32int r = pci_read(pci_rtl8029, PCI_BAR0);

    rtl8029.bar_type = r & 0x01;
    rtl8029.io_base  = r & (~0x03);
    rtl8029.mem_base = r & (~0x0F);
    rtl8029.tx_cur   = 0;
    rtl8029.send = rtl8029_send;           
    rtl8029.recv = rtl8029_recv;
    rtl8029.halt = rtl8029_halt;

    register_interrupt_handler(IRQ9, (isr_t)rtl8029_recv);
    arp_init();                     //net.c
    /*READ MAC ADDR OF NIC*/
    n2k_mac_addr(&rtl8029);              
    /*RESET  NIC*/
    outb(rtl8029.io_base + N2K_RESET, inb(rtl8029.io_base + N2K_RESET));
    while((inb(rtl8029.io_base + 0x07) & 0x80) == 0) {
        outb(rtl8029.io_base + 0x07, 0xFF);
    }

    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0STOP);     
    outb(rtl8029.io_base + N2K_DATACONFIGURATION, 0x48);   
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT0, 0);
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT1, 0);
    outb(rtl8029.io_base + N2K_RECEIVECONFIGURATION, 0);
    outb(rtl8029.io_base + N2K_TRANSMITPAGE, N2K_TPSTART);
    outb(rtl8029.io_base + N2K_TRANSMITCONFIGURATION, 0x02);
    outb(rtl8029.io_base + N2K_PAGESTART, N2K_PSTART);
    outb(rtl8029.io_base + N2K_BOUNDARY, N2K_PSTART);
    outb(rtl8029.io_base + N2K_PAGESTOP, N2K_PSTOP);
    outb(rtl8029.io_base + N2K_INTERRUPTSTATUS, 0xFF);
    outb(rtl8029.io_base + N2K_INTERRUPTMASK, 0x11);
    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE1STOP);
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS0, rtl8029.mac_addr[0]); 
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS1, rtl8029.mac_addr[1]);
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS2, rtl8029.mac_addr[2]);
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS3, rtl8029.mac_addr[3]);
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS4, rtl8029.mac_addr[4]);
        outb(rtl8029.io_base + N2K_PHYSICALADDRESS5, rtl8029.mac_addr[5]);
            outb(rtl8029.io_base + N2K_MULTIADDRESS0, 0);                   
            outb(rtl8029.io_base + N2K_MULTIADDRESS1, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS2, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS3, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS4, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS5, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS6, 0);
            outb(rtl8029.io_base + N2K_MULTIADDRESS7, 0);
    outb(rtl8029.io_base + N2K_CURRENT, N2K_PSTART);                    
    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0);
    outb(rtl8029.io_base + N2K_TRANSMITCONFIGURATION, 0xE0);

    return;
}

u32int rtl8029_send(void *pack, u32int len) {

    char    *p = (char *)pack;
    u32int  l = len;

    while(inb(rtl8029.io_base + N2K_COMMAND) == N2K_TRANSMIT);

    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS0, 0);
    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS1, N2K_TPSTART);
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT0, (l & 0xFF));
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT1, ((l >> 8) & 0xFF));
    outb(rtl8029.io_base + N2K_COMMAND, N2K_REMOTEDMAWR);

    while(l > 0) {
        outb(rtl8029.io_base + N2K_DMA_DATA, *p++);
        l--;
    }
    l = len;        // lb 0x203a59 
    while(l < 60) {
        outb(rtl8029.io_base + N2K_DMA_DATA, 0);
        l++;
    }
    while(!(inb(rtl8029.io_base + N2K_INTERRUPTSTATUS)) & 0x40);                // lb 0x203a89

    outb(rtl8029.io_base + N2K_INTERRUPTSTATUS, 0x40);
    outb(rtl8029.io_base + N2K_TRANSMITPAGE, N2K_TPSTART);
    outb(rtl8029.io_base + N2K_TRANSMITBYTECOUNT0, (l & 0xFF));
    outb(rtl8029.io_base + N2K_TRANSMITBYTECOUNT1, ((l >> 8) & 0xFF));
    outb(rtl8029.io_base + N2K_COMMAND, N2K_TRANSMIT);

    //monitor_str_write("RTL8029 Send OK\n");

    return 0;
}

u32int rtl8029_recv(void) {

    u8int tmp, curr_pnt;

    wprint("%d++ RTL RECV\n", curr_thread->id);
   // monitor_str_write("RTL8029 RECV...\n");

    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0);

    while(1) {
        tmp = inb(rtl8029.io_base + N2K_INTERRUPTSTATUS);

        if(tmp & 0x90){
            monitor_str_write("buffer overflow\n");
            /*overflow*/
            outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0STOP);
            outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT0, 0);
            outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT1, 0);
            outb(rtl8029.io_base + N2K_TRANSMITCONFIGURATION, 0x02);

            do{
                curr_pnt = n2k_to_pc();

            } while(inb(rtl8029.io_base + N2K_BOUNDARY) != curr_pnt);

            outb(rtl8029.io_base + N2K_TRANSMITCONFIGURATION, 0xE0);
            break;
        }
        else if(tmp & 0x01) {
            /*just packet received*/
            do {
                outb(rtl8029.io_base + N2K_INTERRUPTSTATUS, 0x01);
                curr_pnt = n2k_to_pc();

            } while(inb(N2K_BOUNDARY) != curr_pnt);
            break;
        }
        else {
            monitor_str_write("RTL8029 no packet. Int Status = ");
            write_hex((u32int)tmp);
            monitor_char_put('\n');
            return 1;
        }
    }
    wprint("%d++ RTL END...\n", curr_thread->id);
    
    return 0;
}

void rtl8029_halt(void) {

    outb(rtl8029.io_base + N2K_COMMAND, 0x01);
    return;
}

__attribute__ ((target ("no-sse")))         //or "gcc -mno-sse"
void n2k_mac_addr(net_dev_s *dev) {

    outb(dev->io_base + N2K_COMMAND, N2K_REMOTEDMARD);
    outb(dev->io_base + N2K_DATACONFIGURATION, 0x48);
    outb(dev->io_base + N2K_REMOTESTARTADDRESS0, 0x00);
    outb(dev->io_base + N2K_REMOTESTARTADDRESS1, 0x00);
    outb(dev->io_base + N2K_REMOTEBYTECOUNT0, 0x0C);
    outb(dev->io_base + N2K_REMOTEBYTECOUNT1, 0x00);
    outb(dev->io_base + N2K_COMMAND, N2K_REMOTEDMARD);

    for(int i = 0; i < 6; i++) {

        dev->mac_addr[i] = inb(dev->io_base + N2K_DMA_DATA);
        inb(dev->io_base + N2K_DMA_DATA);
        //write_hex(dev->mac_addr[i]);
    }
    while((!inb(dev->io_base + N2K_INTERRUPTSTATUS) & 0x40));

    outb(dev->io_base + N2K_REMOTEBYTECOUNT0, 0);
    outb(dev->io_base + N2K_REMOTEBYTECOUNT1, 0);
    outb(dev->io_base + N2K_COMMAND, N2K_PAGE0);
    //monitor_char_put('\n');
    return;
}

__attribute__ ((target ("no-sse")))
void get_mac_addr(net_dev_s *dev, u8int *src) {
    
    memcpy(src, dev->mac_addr, 6);
}

u8int n2k_to_pc(void) {

    u8int   curr_pnt;
    u8int   nxt_pnt;
    u8int   head_stat;
    u8int   len0;
    u8int   len1;
    u8int   *addr;
    u16int  len_recv;

    /* The RTL8029's first 4B is packet status,page of next packet
        and packet length(2B).So we receive the fist 4B.*/
    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS0, 0x00);
    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS1, inb(rtl8029.io_base + N2K_BOUNDARY));
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT0, 0x04);
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT1, 0x00);
    outb(rtl8029.io_base + N2K_COMMAND, N2K_REMOTEDMARD);

    head_stat = inb(rtl8029.io_base + N2K_DMA_DATA);
    nxt_pnt = inb(rtl8029.io_base + N2K_DMA_DATA);
    len0 = inb(rtl8029.io_base + N2K_DMA_DATA);
    len1 = inb(rtl8029.io_base + N2K_DMA_DATA);
    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0);

    len_recv = len1;
    len_recv = (((len_recv << 8) & 0xFF00) + len0);
    len_recv -= 4;

    if(len_recv > /*PKTSIZE_ALIGN + PKTALIGN*/ 0xFFFF) {
        monitor_str_write("!!PACKET TOO BIG!! packet size = ");
        write_dec((u32int)len_recv);
        monitor_char_put('\n');
    }

    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS0, 0x04);
    outb(rtl8029.io_base + N2K_REMOTESTARTADDRESS1, inb(rtl8029.io_base + N2K_BOUNDARY));
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT0, (len_recv & 0xFF));
    outb(rtl8029.io_base + N2K_REMOTEBYTECOUNT1, ((len_recv >> 8) & 0xFF));
    outb(rtl8029.io_base + N2K_COMMAND, N2K_REMOTEDMARD);

    addr = (u8int *)NetRxPacks;

    for(int i = len_recv; i > 0; i--) {

        *addr++ = inb(rtl8029.io_base + N2K_DMA_DATA);
    }
    
    Ethernet_Recv((ether_s *)NetRxPacks, (u32int)len_recv);             // Pass the packet up to the protocol layers.

    while(!(inb(rtl8029.io_base + N2K_INTERRUPTSTATUS)) & 0x40);

    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE1);
    curr_pnt = inb(N2K_CURRENT);
    outb(rtl8029.io_base + N2K_COMMAND, N2K_PAGE0);
    outb(rtl8029.io_base + N2K_BOUNDARY, nxt_pnt);

    return curr_pnt;
}