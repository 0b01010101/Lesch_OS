
#include "io_disp.h"

mutex_s *port_mtx;

void init_io_disp(void) {

    port_mtx = (mutex_s *)kmalloc(sizeof(mutex_s)*PORTS_NUM);  //?????????

    for(int i = 0; i < PORTS_NUM; i++) {
        clean_mtx(&port_mtx[i]);
    }
    return;
}

void out_byte(u16int port, u8int value) {

    get_mtx(&port_mtx[port], true);
    outb(port, value);
    clean_mtx(&port_mtx[port]);

    return;
}

u8int in_byte(u16int port) {

    u8int value = 0;

    get_mtx(&port_mtx[port], true);
    value = inb(port);
    clean_mtx(&port_mtx[port]);

    return value;
}