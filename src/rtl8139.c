#include "rtl8139.h"

u8int TSD_arr[4]  = {0x10, 0x14, 0x18, 0x1C};
u8int TSAD_arr[4] = {0x20, 0x24, 0x28, 0x2C};

pci_dev_s       pci_rtl8139;
rtl8139_dev_s   rtl8139;
u32int          ptr_curr_pack;


void rtl8139_init(void) {

    pci_rtl8139 = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1); //Get the network device using PCI
    u32int r = pci_read(pci_rtl8139, PCI_BAR0);

    rtl8139.bar_type = r & 0x01;
    rtl8139.io_base = r & (~0x03);      //Get io base 
    rtl8139.mem_base = r & (~0x0F);     //Get mem base 
    rtl8139.tx_cur = 0;                 // Set current TSAD

    // Enable PCI Bus Mastering
    u32int pci_command_reg = pci_read(pci_rtl8139, PCI_COMMAND);
    if(!(pci_command_reg & (1<<2))) {
        pci_command_reg |= (1<<2);
        pci_write(pci_rtl8139, PCI_COMMAND, pci_command_reg);
    }

    //Set the LWAKE + LWPTN to active high. this should essentially *power on* the device.
    outb(rtl8139.io_base + 0x52, 0);
    outb(rtl8139.io_base + 0x37, 0x10);
    while ((inb(rtl8139.io_base + 0x37) & 0x10) != 0) {

    }

    // Allocate receive buffer
    rtl8139.rx_buffer = kmalloc(8192+16+1500);
    memset(rtl8139.rx_buffer, 0, 8192+16+1500);
    //outl(rtl8139.io_base + 0x30, (u32int)virt2phys(kern_page_dir, rtl8139.rx_buffer)); !!!!!!!!
    outl(rtl8139.io_base + 0x30, (u32int)rtl8139.rx_buffer);

    outw(rtl8139.io_base + 0x3C, 0x05);             // Sets the TOK and ROK bits high
    outl(rtl8139.io_base + 0x44, 0x0F | (1<<7));    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    outb(rtl8139.io_base + 0x37, 0x0C);             // Sets the RE and TE bits high

    u32int irq_num = pci_read(pci_rtl8139, PCI_INTERRUPT_LINE);
    //register_interrupt_handler(31 + irq_num, rtl8139_handler);
    read_mac_addr();

    return;
}

/*
void rtl8139_handler(registers_t *regs) {

    u16int status = inw(rtl8139.io_base + 0x3E);

    if(status & TOK) {
        monitor_str_write("Send PACKET\n");
    }
    if(status & ROK) {
        rtl8139_recv_packet();
    }

    outw(rtl8139.io_base + 0x3E, 0x05);

    return;
}

void rtl8139_send_packet(void *data, u32int len) {

    //Copy the data to a physically contiguous chunk of memory
    void *trans_data = kmalloc(len);
    void phys_addr = virt2phys(kern_page_dir, trans_data);   //!!!!!!!!!!!!!!!
    memcpy(trans_data, data, len);

    //Fill in physical address of data, and length
    outl(rtl8139.io_base + TSAD_arr[rtl8139.tx_cur], (u32int)phys_addr);
    outl(rtl8139.io_base + TSD_arr[rtl8139.tx_cur++], len);

    if(rtl8139.tx_cur > 3) {
        rtl8139.tx_cur = 0;
    }

    return;
}

void rtl8139_recv_packet(void) {

    u16int *tmp = (u16int*)(rtl8139.rx_buffer + ptr_curr_pack);
    u16int pack_len = *(tmp + 1);          // Skip packet header, get packet length
    tmp = tmp + 2;                         // Skip, packet header and packet length, now t points to the packet data

    void *packet = kmalloc(pack_len);
    memcpy(packet, tmp, pack_len);
    //ethernet_handle_packet(packet, pack_len);

    ptr_curr_pack = (ptr_curr_pack + pack_len + 4 + 3) & RX_READ_POINTER_MASK;

    if(ptr_curr_pack > RX_BUF_SIZE) {
        ptr_curr_pack -= RX_BUF_SIZE;
    }

    outw(rtl8139.io_base + CAPR, ptr_curr_pack - 0x10);

    return;
}
*/

void read_mac_addr(void) {

    u32int part1 = inl(rtl8139.io_base + 0x00);
    u32int part2 = inw(rtl8139.io_base + 0x04);

    rtl8139.mac_addr[0] = part1 >> 0;
    rtl8139.mac_addr[1] = part1 >> 8;
    rtl8139.mac_addr[2] = part1 >> 16;
    rtl8139.mac_addr[3] = part1 >> 24;
    rtl8139.mac_addr[4] = part2 >> 0;
    rtl8139.mac_addr[5] = part2 >> 8;

    return;
}

void get_mac_addr(u32int *src_addr) {

    memcpy(src_addr, rtl8139.mac_addr, 6);
    return;
}

