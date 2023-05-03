
#include "schedul.h"

heap_s           uheap;                     //Heap of user

process_s*       curr_proc;                 //Текущий процесс
thread_s*        curr_thread;               //Текущий поток

process_s*       kern_proc = 0;             //Описатель процесса ядра
thread_s*        kern_thread = 0;           //Описатель потока ядра

list_s           proc_list;                 //Список процессов
list_s           thread_list;               //Список потоков
list_s           thread_wait;               //Список ожидающих нитей

u32int           next_pid = 0;              //Следующий идентификатор процесса (PID)
u32int           next_thread_id = 0;        //Следующий идентификатор потока
u32int           in_focus_pid = 0;

u32int multi_task = 0;        //Флаг готовности планировщика /Scheduler Readiness flag

void init_task_manager(void) {

    u32int  esp = 0;

    asm volatile ("cli");
    stop();
    asm volatile ("mov %%esp, %0":"=a"(esp));

    list_init(&proc_list);
    list_init(&thread_list);
    list_init(&thread_wait);
    list_init(&tty16_list);

    mpfile_init();
    msg_init();
//---------------------------------------------------------------------------------
            /*USER HEAP*/
    u32int      us_page_count = 0;
    physaddr    tmp_addr = 0;
    s32int      err = -1;

    //heap_s *uheap = (heap_s*)USER_HEAP_BASE;
    us_page_count = USER_HEAP_BLOCK_INF / PAGE_SIZE;
    tmp_addr = (physaddr)alloc_phys_pages(us_page_count);
    err = phys2virt(KERN_PAGE_TABLE, USER_MEMORY_START, tmp_addr, us_page_count, 0x07);

    if (err == -1)
	{
		monitor_str_write("Memory mapping error...FAIL\n");
		return;
	}

    us_page_count = USER_HEAP_SIZE / PAGE_SIZE;
    tmp_addr = alloc_phys_pages(us_page_count);
    err = phys2virt(KERN_PAGE_TABLE, USER_HEAP_BASE, tmp_addr, us_page_count, 0x07);

     if (err == -1)
	{
		monitor_str_write("Memory mapping error...FAIL\n");
		return;
	}

    /* User-space heap initialization */
	uheap.start = USER_HEAP_BASE;
    uheap.size = USER_HEAP_SIZE;
    uheap.end = USER_HEAP_BASE + USER_HEAP_SIZE;
    uheap.count = 0;

    uheap.blocks = (block_hmem*) USER_MEMORY_START;
    memset(uheap.blocks, 0, USER_HEAP_BLOCK_INF);
    uheap.blocks[0].base = uheap.start;
    uheap.blocks[0].size = sizeof(heap_s);

//---------------------------------------------------------------------------------

    kern_proc = (process_s*) kmalloc(sizeof(process_s));
    kern_thread = (thread_s*) kmalloc(sizeof(thread_s));

    memset(kern_proc, 0, sizeof(process_s));
    memset(kern_thread, 0, sizeof(thread_s));

    // Инициализируем процесс 
    strcpy(kern_proc->name, "Kernel");
    kern_proc->suspend = false;
    kern_proc->pid = next_pid++;
    kern_proc->page_dir_vaddr = (void *)KERN_PAGE_TABLE;
    kern_proc->page_dir = get_kern_dir();
    kern_proc->threads_count = 1;
    kern_proc->list_item.list = NULL;
    kern_proc->tty16 = tty16_create(0, 0, SCRN_WIDTH, SCRN_HEIGHT);
    
    // Инициализируем thread
    kern_thread->suspend = false;
    kern_thread->process = kern_proc;
    kern_thread->id = next_thread_id++;
    kern_thread->list_item.list = NULL;
    kern_thread->esp = esp;
    kern_thread->stack_size = 0x4000;
    kern_thread->stack_top = init_esp;
    //kern_thread->tty = kget_vscreen();          //!!!!!!!!!!!!!

    list_add(&proc_list, &kern_proc->list_item);
    list_add(&thread_list, &kern_thread->list_item);

    curr_proc = kern_proc;
    curr_thread = kern_thread;
    multi_task = true;
  
    start();
    asm volatile ("sti");

    return;
}

/*
void switch_task(void) {

    if(multi_task) {

        asm volatile ("pushf");
        asm volatile ("cli");
        asm volatile ("mov %%esp, %0":"=a"(curr_thread->esp));

        curr_thread = (thread_s*) curr_thread->list_item.next;
        
        asm volatile ("mov %0, %%cr3"::"a"(curr_proc->page_dir));
        asm volatile ("mov %0, %%esp":: "a"(curr_thread->esp));
        asm volatile ("popf");
       // asm volatile ("pop %ebp");

    }

    return;
}
*/

thread_s *thread_create(/*process_s *proc,*/ void *entr_point, u32int stack_size, bool kernel, bool suspend) {

    void *stack = NULL;
    u32int eflags;

    process_s *proc = curr_thread->process;

    asm volatile ("cli");
    stop();
    asm volatile ("pushf");
    asm volatile ("pop %0":"=r"(eflags));
    eflags |= (1 << 9);

    thread_s *tmp_thrd = (thread_s*)kmalloc(sizeof(thread_s));
    memset(tmp_thrd, 0, sizeof(thread_s));

    tmp_thrd->id = next_thread_id++;
    tmp_thrd->process = proc;
    tmp_thrd->suspend = suspend;
    tmp_thrd->stack_size = stack_size;
    tmp_thrd->entry_point = (u32int) entr_point;
    tmp_thrd->list_item.list = NULL;
    tmp_thrd->wait_list_item.list = NULL;

    tmp_thrd->ticks0 = 0;
    tmp_thrd->pause_time = 0;

    stack = kmalloc(stack_size);
    tmp_thrd->stack = stack;
    tmp_thrd->esp = (u32int)stack + stack_size - 28;
    tmp_thrd->stack_top = (u32int)stack + stack_size;

    list_add(&thread_list, &tmp_thrd->list_item);       //add_thread(tmp_thrd);

    //proc->threads_count++;
    proc->thread_id[proc->threads_count++] = tmp_thrd->id;

    u32int *esp = (u32int *)tmp_thrd->stack_top;

    if(kernel) {
        tmp_thrd->tty = kget_vscreen();//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        esp[-5] = (u32int)entr_point;
        esp[-7] = eflags;
    }
    else {
        tmp_thrd->tty = get_vscreen();//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        esp[-3] = (u32int)tmp_thrd;             //esp[-4]space for return address
        esp[-5] = (u32int)&start_uthread;
        esp[-7] = eflags;
    }

    if(suspend) {
        remove_thread(tmp_thrd);
        tmp_thrd->list_item.list = NULL;
        add_wait(tmp_thrd);
    }

    start();
    asm volatile ("sti");

    return tmp_thrd;
}

void thread_exite(thread_s *thrd) {

    //asm volatile ("cli");
    stop();
    del_vscreen(thrd->tty);         //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    list_remove(&thrd->list_item);
    thrd->process->threads_count--;

    ufree(thrd->user_stack);
    kfree(thrd->stack);
    kfree(thrd);

    start();
    asm volatile ("mov %0, %%ecx"::"a"(&switch_task));
    //asm volatile ("sti");
    asm volatile ("call *%ecx");

    return;
}

void msg_to_thread(thread_s *thread, void *msg, u32int len) {

    thread->msg_buff = msg;
    thread->msg_len = len;
    return;
}

void thread_suspend(thread_s *thread, bool suspend) {

    stop();

    if(suspend) {
        if(thread->list_item.list) {
            
            remove_thread(thread);
            thread->list_item.list = NULL;
            thread->suspend = true;
            add_wait(thread);
        }
    }
    else{
        if(thread->wait_list_item.list) {
            
            remove_wait(thread);
            thread->wait_list_item.list = NULL;
            thread->suspend = false;
            add_thread(thread);
        }
    }
    start();
    switch_task();

    return;
}

process_s *get_kern_proc(void) {

    return kern_proc;
}

process_s *get_curr_proc(void) {

    return curr_proc;
}

u32int get_pid(void) {

    return next_pid++;
}

thread_s *get_curr_thread(void) {

    return curr_thread;
}

u32int get_thread_id(void) {

    return next_thread_id++;
}

thread_s *get_thread(u32int id) {

    thread_s *thread = curr_thread;
    u32int    idx = 0;

    do{
        if(thread->id == id) {
            break;
        }
        thread = (thread_s *)thread->list_item.next;
        idx++;

    } while(idx < thread->list_item.list->count);

    if(idx == thread->list_item.list->count) {
        return NULL;
    }
    else {
        return thread;
    }
}

void init_user_mode(void *entr_point, u32int stack_size) {

    void *stack_user = umalloc(stack_size);
    void *code_user = umalloc(stack_size);
    //copy_code(entr_point, code_user);
    asm volatile ("sti");
    //u32int x = copy_code_a(entr_point, code_user);  //теперь в USER_MODE нужен сист.вызов для КАЖДОЙ функции ядра!!!

    switch_user_mode(code_user, (u32int) stack_user + stack_size);
    return;
}

void add_thread(thread_s *thread) {

    list_add(&thread_list, &thread->list_item);
    return;
}

void add_process(process_s *proc) {

    list_add(&proc_list, &proc->list_item);
    return;
}

void remove_thread(thread_s *thread) {

    list_remove(&thread->list_item);
}

void remove_process(process_s *proc) {

    list_remove(&proc->list_item);
}

void start_uthread(thread_s *thrd) {

    void *ustack = (void *)umalloc(thrd->stack_size);
    thrd->user_stack = ustack;
    switch_user_mode((void *)thrd->entry_point, (u32int)ustack + thrd->stack_size - 12);

    return;
}

thread_s *uthread_create(void *entr_point, u32int stack_size, bool suspend) {

    return thread_create(entr_point, stack_size, false, suspend);
}

void *umalloc_common(u32int size, bool align) {

    heap_s *u_heap = (heap_s *)USER_HEAP_BASE;
    void *vaddr = u_heap->start + sizeof(heap_s);
    bool overlap = false;


    //wprint("heap_mtx=%d...", (u8int)u_heap->heap_mtx);
   get_mtx(&u_heap->heap_mtx, true);             //{

    do {
        overlap = false;

        if(align) {
        
        u32int tmp_addr = (u32int) vaddr;

            if(tmp_addr & 0xFFF) {

                tmp_addr &= 0xFFFFF000;
                tmp_addr += PAGE_SIZE;
                vaddr = (void*)tmp_addr;
            }
        }

        for(int i = u_heap->count-1; i >= 0; i--) {

            if(overlapped_blocks(u_heap->blocks[i].base, u_heap->blocks[i].size, vaddr, size)) {

                vaddr = u_heap->blocks[i].base + u_heap->blocks[i].size;
                overlap = true;
            }
        }
    } while(overlap);

    for(int i = u_heap->count-1; i >= 0; i--) {
        u_heap->blocks[i+1].base = u_heap->blocks[i].base;
        u_heap->blocks[i+1].size = u_heap->blocks[i].size;
    }

    u_heap->blocks[0].base = vaddr;
    u_heap->blocks[0].size = size;

    u_heap->count++;
    u_heap->size -= size;

   clean_mtx(&u_heap->heap_mtx);                 //}
   // wprint("mtx(%d)|vaddr(%x)\n", (u8int)u_heap->heap_mtx, vaddr);
    return vaddr;
}

void *umalloc(u32int size) {

    return umalloc_common(size, false);
}

void *ucalloc(u32int size, u32int num) {

    void *tmp = umalloc_common(num * size, false);

    if(!tmp) {
        return NULL;
    }
    memset(tmp, 0, num * size);

    return tmp;
}

void *umalloc_align(u32int size) {

    return umalloc_common(size, true);
}

void ufree(void *vaddr) {

    heap_s *uheap = (heap_s *)USER_HEAP_BASE;
    int block_indx = 0;
    int size = 0;
    int i = 0;

    get_mtx(&uheap->heap_mtx, true);         //{

    if(vaddr == NULL) {
        clean_mtx(&uheap->heap_mtx);
        return;
    }
    for(i = 0; i < uheap->count; i++) {

        if(vaddr == uheap->blocks[i].base) {

            size == uheap->blocks[i].size;
            block_indx = i;
            break;
        }
    }
    if(i == uheap->count) {

        clean_mtx(&uheap->heap_mtx);
        return;
    }

    for(i = block_indx; i < uheap->count-1; i++) {

        uheap->blocks[i].base = uheap->blocks[i+1].base;
        uheap->blocks[i].size = uheap->blocks[i+1].size;
    }
    uheap->count--;
    uheap->size += size;

    clean_mtx(&uheap->heap_mtx);             //}
    return;
}

void remove_wait(thread_s *thread) {

    list_remove(&thread->wait_list_item);
    return;
}

void add_wait(thread_s *thread) {

    list_add(&thread_wait, &thread->wait_list_item);
    return;
}

u32int get_multitask(void) {

    return multi_task;
}

void slp_thrd(thread_s *thread) {

    thread_suspend(thread, true);
    return;
}

void wup_thrd(thread_s *thread) {

    thread_suspend(thread, false);
    return;
}