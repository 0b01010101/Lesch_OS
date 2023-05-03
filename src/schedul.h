
#ifndef SCHEDUL_H
#define SCHEDUL_H

#include "common.h"
#include "list.h"
#include "memory.h"
#include "timer.h"
#include "tty16.h"
#include "ipc.h"
#include "msg.h"

extern u32int multitask;         // in timer.c
typedef struct thread thread_s;
typedef struct process process_s;

struct process {

    list_item_s     list_item;      //+0
    physaddr        page_dir;       //+16
    u32int          pid;            //+20
    u32int          threads_count;  //+24
    bool            suspend;        //+28
    tty16_s         *tty16;         //+32
    char            name[32];

    void            *page_dir_vaddr;
    void            *ustack_vaddr;          //virtual address of user stack
    
    physaddr        ustack_paddr;           // phys address of user stack
    physaddr        stack_paddr;
    u32int          stack_page_count;

    physaddr        heap_page_count;   
    physaddr        blocks_page_count;
    u32int          heap_paddr;
    u32int          blocks_paddr;

    u32int          seg_page_count;
    physaddr        seg_paddr;

    u32int          thread_id[8];
    thread_s        *parent_proc_thread;

};/*__attribute__((packed))*/

struct thread {

    list_item_s     list_item;          //+ 0  //.next:+4
    process_s*      process;            //+16
    bool            suspend;            //+20
    u32int          stack_size;         //+24
    void*           stack;              //+28
    u32int          esp;                //+32
    u32int          entry_point;        //+36
    u32int          id;                 //+40
    u32int          stack_top;          //+44

    void*           user_stack;
    list_item_s     wait_list_item;
    u64int          ticks0;
    u32int          pause_time;

    //u8int           status;
    vscreen_s*      tty;
    void*           msg_buff;
    u32int          msg_len;

};/*__attribute__((packed))*/

extern void switch_task(void);
extern physaddr switch_cr3(physaddr need);
extern void switch_user_mode(void *entr_point, u32int stack_top);
extern void copy_code(void *source, void *receiver);
extern void switch_the_task(thread_s *thread);
extern void switch_kern_mode(void);
extern void start(void);
extern void stop(void);
//----------------------------------------------------------------------------------------------------------------
void init_task_manager(void);
thread_s *thread_create(/*process_s *proc,*/ void *entr_point, u32int stack_size, bool kernel, bool suspend);
thread_s *uthread_create(void *entr_point, u32int stack_size, bool suspend); 
void thread_exite(thread_s *thrd);
void msg_to_thread(thread_s *thread, void *msg, u32int len);
void thread_suspend(thread_s *thrd, bool suspend);
process_s *get_kern_proc(void);
process_s *get_curr_proc(void);
u32int get_pid(void);
thread_s *get_curr_thread(void);
u32int get_thread_id(void);
thread_s *get_thread(u32int id);
void init_user_mode(void *entr_point, u32int stack_size);
void start_uthread(thread_s *thread);

void add_wait(thread_s *thread);
void add_thread(thread_s *thread);
void add_process(process_s *proc);
void remove_thread(thread_s *thread);
void remove_process(process_s *proc);
void remove_wait(thread_s *thread);
u32int get_multitask(void);

void *umalloc_common(u32int size, bool align);
void *umalloc(u32int size);
void *ucalloc(u32int size, u32int num);
void *umalloc_align(u32int size);
void ufree(void *vaddr);

void slp_thrd(thread_s *thread);
void wup_thrd(thread_s *thread);

#endif