#ifndef     IPC_H
#define     IPC_H

#include    "common.h"
#include    "help_func.h"
#include    "schedul.h"
#include    "memory.h"
#include    "list.h"

#define     IPC_MPFILE_MAX  10

struct mpfile {

    list_item_s list_item;
    u8int       name[16];
    u32int      pid_src;
    u32int      pid_dst;
    physaddr    *paddr;
};

typedef struct mpfile mpfile_s;

void mpfile_init(void);
void mpfile_reset(void);

#endif