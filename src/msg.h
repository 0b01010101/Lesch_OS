#ifndef     MSG_H
#define     MSG_H

#include    "common.h"
#include    "list.h"
#include    "help_func.h"
#include    "schedul.h"

#define     MSG_MAX_CNT     0xFF
#define     MSG_KERN        0x00
#define     MSG_KEYB        0x01

struct msg {

    list_item_s list_item;
    u32int      count;      //absolute message's count
    u32int      dsize;      //data size(actual data size in bytes);
    u32int      pid_src;
    u32int      pid_dst;
    u32int      id;
    u8int       buff[4];
};

typedef struct msg msg_s;

void msg_init(void);
void msg_reset(void);
void msg_send(msg_s *msg);
u8int msg_recv(msg_s *msg);
msg_s *get_msg(u32int pid);

#endif