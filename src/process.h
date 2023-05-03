#ifndef     PROC_H
#define     PROC_H

#include    "common.h"
#include    "help_func.h"
#include    "memory.h"
#include    "schedul.h"
#include    "elf_loader.h"
#include    "vfs.h"

extern u32int next_pid;
extern u32int read_eflags(void);
extern s32int syscall_proc_destroy(void);

u32int exec_proc(u8int *name, bool kern, u8int *cmd_line);
u32int user_exec(u8int *name, u8int *cmd_line);

void elf_start(elf_sections_s *elf, process_s *proc, u8int *cmd_line);
void proc_destroy(void);
void proc_set_in_focus(u32int pid);
thread_s *proc_create(u8int *name, void *entr_poin, u32int stack_size, bool kern, bool suspend);

#endif