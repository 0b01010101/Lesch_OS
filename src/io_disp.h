
#ifndef     IO_DISP_H
#define     IO_DISP_H

#include    "common.h"
#include    "sync.h"

#define     PORTS_NUM       0x4000

void init_io_disp(void);
void out_byte(u16int port, u8int value);
u8int in_byte(u16int port);

#endif