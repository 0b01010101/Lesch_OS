
#include "main.h"

u32int init_esp = 0;

void kernel(multiboot_s *mboot, u32int int_esp) {

    init_esp = int_esp;

    monitor_clear();
    monitor_str_write("GDT and IDT...\n");
    init_tables_descr();    
    
    check_memory_map((memory_map_s*)mboot->mmap_addr, mboot->mmap_length);
    monitor_char_put('\n');
    init_memory_manager(init_esp);
   
    pci_init();
    rtl8029_init();
    init_keyboard();            
    init_timer(TIMER_FREQ);
    init_task_manager();

    vfs_init();
    ata_init(); 
    ext2_init("/dev/hda", "/");
    //vesa_init();                  //everything works faster without them ;)   
    //windows_init();
   shell_init();

    return;
}
//==================================================================================
/*
Just functions for testing. In order for them to work,
they will most likely have to be changed a little
*/
u8int ser[] = {0xB0, 0xC4, 0x20, 0x00, 0x00, 0x02};
u8int dhcp_ip[] = {192, 168, 10, 3};

void net_test(void) {

    arp_lookup_add(ser, dhcp_ip);
    monitor_clear();

    u8int *data = kcalloc(512, 1);
    u8int *data1 = kcalloc(512, 1);
    u32int len = Tftp("Hi.txt", 30000, 1, data, 0);
    wprint("len(%d) %s\n", len, data);
    u32int len1 = Tftp("Bye.txt", 30005, 1, data1, 0);
    wprint("len(%d) %s\n", len1, data1);
    kfree(data);
    kfree(data1);

    return;
}

void net_test3(void) {

    u8int serv_ip[4] = {0};
    memcpy(serv_ip, dhcp_ip, 4);
    u8int *recv_buff = kcalloc(512, 1);

    arp_lookup_add(ser, dhcp_ip);
    monitor_clear();
    //net_test2("bochs", "bochs", 20000, 3025, serv_ip);
    Ftp_file_recv("bochs", "bochs", "Hello.txt", 20000, 3025, serv_ip, recv_buff);

    while(1) {}
    kfree(recv_buff);
    return;    
}
