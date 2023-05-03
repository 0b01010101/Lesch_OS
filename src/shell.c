
#include    "shell.h"
#include    "memory.h"
#include    "schedul.h"
#include    "process.h"

#define     SHELL_VADDR  0x08049000

u32int shell_init(void) {

    u8int err;
    u32int sz;
    u32int usr_stck;
    u32int stack_size = 0x4000;
    physaddr tmp_pha;

    asm volatile("cli");
    stop();

    thread_s *thread = proc_create("Shell", (void *)SHELL_VADDR, stack_size, false, false);
    process_s *proc = thread->process;
    
    if(!thread || !proc) {
        wprint("ERROR: in shell init\n");
        return 0;
    }

    physaddr page_dir = proc->page_dir;
    u32int code_size = (u32int)shell_end - (u32int)shell;
    u32int *t = proc->page_dir_vaddr;


    sz = code_size / PAGE_SIZE + 1;
    tmp_pha = alloc_phys_pages(sz);
    memset(&t[(u32int)SHELL_VADDR >> 22], 0, (sz / 1024 + 1) * 4);
    err = phys2virt(page_dir, (void *)SHELL_VADDR, tmp_pha, sz, 0x07);
    if(err == 0) {
        wprint("ERROR: in shell_init(phys2virt SHELL_VADDR)\n");
        return 0;
    }

    usr_stck = SHELL_VADDR + sz * PAGE_SIZE;
    sz = stack_size / PAGE_SIZE;
    tmp_pha = alloc_phys_pages(sz);
    err = phys2virt(page_dir, (void *)usr_stck, tmp_pha, sz, 0x07);
    if(err == 0) {
        wprint("ERROR: in shell_init(phys2virt user_stack)\n");
        return 0;
    }

    proc->ustack_paddr = tmp_pha;
    proc->ustack_vaddr = (u32int *)usr_stck;
    proc->stack_page_count = sz;

    u32int *esp = (u32int *)thread->stack_top;
    u32int eflags = read_eflags();
    eflags |= (1 << 9);

    esp[-7] = eflags;
    esp[-5] = (u32int)&shell_start;
    esp[-3] = (u32int)proc;

    start();
    asm volatile("sti");

    return 1;
}

void shell_start(process_s *proc) {

    heap_s *heap;
    u32int sz = (u32int)shell_end - (u32int)shell;
    //wprint("shell_end(%x) - shell(%x) = sz(%d)\n", shell_end, shell, sz);

    u32int *esp = (u32int *)((u32int) proc->ustack_vaddr + proc->stack_page_count * PAGE_SIZE - 20);
    esp[-1] = 0;
    esp[-2] = (u32int)0;
    esp[-3] = (u32int)&syscall_proc_destroy;

    heap = (heap_s *)USER_HEAP_BASE;
    heap->start = USER_HEAP_BASE;
    heap->size = USER_HEAP_SIZE;
    heap->end = heap->start + heap->size;
    heap->count = 1;
    heap->heap_mtx = 0;

    heap->blocks = (block_hmem *)USER_HEAP_BLOCKS_INF;
    heap->blocks[0].base = heap->start;
    heap->blocks[0].size = sizeof(heap_s);

    memcpy((void*)SHELL_VADDR, (void*)shell, sz);

    proc->tty16 = tty16_create(30, 1, 40, 5);
    tty16_clear();
    tty16_swap_buff();

    switch_user_mode((void *)SHELL_VADDR, (u32int)esp - 12);
    return;
}

void shell(void) {

    u8int keyb;
    u8int *shell_buff;;
    u32int i = 0;
    u32int j;
    u8int *str = "\n$:";

    //syscall_umalloc(32);
    asm volatile ("mov $32, %ebx");         //32 = size of "shell_buff"
    asm volatile ("mov $16, %eax");
    asm volatile ("int $0x30");
    asm volatile ("mov %%eax, %0": "=r"((u8int *)shell_buff));
    //syscall_print16(str);
    asm volatile ("mov %0, %%ebx":: "r"(str) : "%eax");
    asm volatile ("mov $17, %eax");
    asm volatile ("int $0x30");

    while(1){
        //syscall_get_asc2();
        asm volatile ("mov $11, %eax");
        asm volatile ("int $0x30");
        asm volatile ("mov %%al, %0": "=r"(keyb));

        if(keyb) {

            shell_buff[i] = keyb;
            i++;

            if(keyb == '\n') {
                shell_buff[i] = '\0';
                //j = syscall_shell_handler(shell_buff);
                asm volatile ("mov %0, %%ebx":: "r"(shell_buff) : "%eax");
                asm volatile ("mov $20, %eax");
                asm volatile ("int $0x30");

                for(j = 0; j < SHELL_BUFF_SIZE; j++) {
                    shell_buff[j] = 0;
                    i = 0;
                }
                //syscall_print16(str);
                asm volatile ("mov %0, %%ebx":: "r"(str) : "%eax");
                asm volatile ("mov $17, %eax");
                asm volatile ("int $0x30");
            }
            else {
            //syscall_tty16_put_char(keyb);
                asm volatile ("mov %0, %%ebx":: "r"((u32int)keyb) : "%eax");
                asm volatile ("mov $18, %eax");
                asm volatile ("int $0x30");
            //syscall_tty16_swap_buff();
                asm volatile ("mov $19, %eax");
                asm volatile ("int $0x30");

                if(keyb == 0x08) {  //backspace

                    shell_buff[i--] = 0;
                    i--;
                }
                if(i >= SHELL_BUFF_SIZE) {
                    i = 0;
                    for(j = 0; j < SHELL_BUFF_SIZE; j++) {
                        shell_buff[j] = 0;
                    }
                }
            }
        }
    }
    return;
}
void shell_end(void){}

s32int shell_handler(u8int *shell_buff) {

    stop();
    u8int *copp_buff = kmalloc(SHELL_BUFF_SIZE);
    strcpy(copp_buff, shell_buff);
    u8int *app = strsep(&copp_buff, " ");
    
    if(memcmp("exit", app, 4)) {
        proc_destroy();
        return 0;
    }
    u32int err = user_exec(app, copp_buff);
    
    if(!err) {
        print16("!ERROR: doesn't exist '%s'", app);
        tty16_swap_buff();
    }
    kfree(copp_buff);
 
    start();
    return 0;
}

