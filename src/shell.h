#ifndef     SHELL_H
#define     SHELL_H

#include "common.h"

#define     SHELL_BUFF_SIZE  32

u32int shell_init(void);
s32int shell_handler(u8int *shell_buff);
void shell_start(process_s *proc);
void shell(void);
void shell_end(void);

#endif