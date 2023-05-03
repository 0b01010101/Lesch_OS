#include "process.h"

//DECL_SYSCALL0(proc_destroy);

u32int exec_proc(u8int *name, bool kern, u8int *cmd_line) {

    physaddr heap_paddr = 0;
    physaddr blocks_paddr = 0;
    physaddr ustack_paddr = 0;      // user stack phys. addr
    physaddr stack_paddr = 0;
    physaddr seg_paddr = 0;
    physaddr tmp_paddr = 0;

    physaddr page_dir = 0;          // Page directory physical address
    u32int   *pdir_vaddr = 0;        // Page directory virtual address

    u32int heap_page_count;         // Heap page's count
    u32int blocks_page_count;       // Heap info blocks page count
    u32int stack_page_count;        // Stack page's count
    u32int seg_page_count;          // ELF segments page's count
    u32int page_count;              // Allocation page's count
    
    u32int i = 0;
    u32int s = 0;
    u32int stack = 0;
    u32int usr_stack = 0;
    u32int eflags = 0;
    u32int seg_size = 0;

    u32int stack_size = 0x4000;
    s8int  err = -1;

    heap_s *heap;
    static  process_s *proc = 0;
    static  thread_s  *thread = 0;

    elf_sections_s *elf = elf_load(name);

    if(!elf) {
        monitor_str_write("File ");
        monitor_str_write(name);
        monitor_str_write(" is not found\n");
        return 0;
    }
//-----------------------------------------------------------------------------
    page_dir = clone_kern_dir((u32int *)&pdir_vaddr);//all_dir_pages(&pdir_vaddr);
    u32int indx = (u32int)USER_MEMORY_START >> 22;
    u32int *t = pdir_vaddr;
    memset(&t[indx], 0, PAGE_SIZE - indx * 4);
//------------------------------------------------------------------------------
    for(i = 0; i < elf->elf_header->e_phnum; i++) {
        seg_size += elf->prog_header[i].p_memsz;
    }

    page_count = seg_size / PAGE_SIZE + 1;
    seg_page_count = page_count;
    tmp_paddr = alloc_phys_pages(page_count);
    seg_paddr = tmp_paddr;

    memset(&t[elf->prog_header[0].p_vaddr >> 22], 0, (page_count / 1024 + 1) * 4);
    err = phys2virt(page_dir, (void *)elf->prog_header[0].p_vaddr, tmp_paddr, page_count, 0x07);
    if(err == -1) {
        //monitor_str_write("ERROR: memory mapping\n");
        return 0;
    }

    stack = (u32int)kmalloc(stack_size);
    if(!stack) {
        //monitor_str_write("ERROR: memory alloc\n");
        return 0;
    }
    usr_stack = elf->prog_header[0].p_vaddr + page_count * PAGE_SIZE;

    page_count = stack_size / PAGE_SIZE;
    stack_page_count = page_count;
    tmp_paddr = alloc_phys_pages(page_count);
    ustack_paddr = tmp_paddr;

    err = phys2virt(page_dir, (void *)usr_stack, tmp_paddr, page_count, 0x07);
    if(err == 0) {
        //monitor_str_write("ERROR: memory mapping\n");
        return 0;
    }

    //Process heap creation
    page_count = USER_HEAP_SIZE / PAGE_SIZE;
    heap_page_count = page_count;
    tmp_paddr = alloc_phys_pages(page_count);
    heap_paddr = tmp_paddr;

    err = phys2virt(page_dir, USER_HEAP_BASE, tmp_paddr, page_count, 0x07);
    if(err == 0) {
        monitor_str_write("ERROR: memory mapping in exec_proc(U_H_B)\n");
        return 0;
    }

    page_count = USER_HEAP_BLOCK_INF / PAGE_SIZE;
    blocks_page_count = page_count;
    tmp_paddr = alloc_phys_pages(page_count);
    blocks_paddr = tmp_paddr;

    err = phys2virt(page_dir, USER_MEMORY_START, tmp_paddr, page_count, 0x07);
    if(err == 0) {
        monitor_str_write("ERROR: memory mapping in exec_proc(U_M_S)\n");
        return 0;
    }
    //Create process
    proc = (process_s *)kcalloc(sizeof(process_s), 1);
    if(!proc) {
        //monitor_str_write("ERROR: memory alloc\n");
        return 0;
    }

    proc->list_item.list = NULL;
    proc->page_dir = page_dir;
    proc->pid = get_pid();
    proc->threads_count = 0;
    proc->suspend = FALSE;
    strcpy(proc->name, name);

    proc->page_dir_vaddr = (void *)pdir_vaddr;
    proc->ustack_vaddr = (void *)usr_stack;
    proc->ustack_paddr = ustack_paddr;
    proc->stack_paddr = stack_paddr;                
    proc->stack_page_count = stack_page_count;      

    proc->heap_page_count = heap_page_count;
    proc->blocks_page_count = blocks_page_count;
    proc->heap_paddr = heap_paddr;
    proc->blocks_paddr = blocks_paddr;
    proc->seg_page_count = seg_page_count;
    proc->seg_paddr = seg_paddr;

    proc->parent_proc_thread = get_curr_thread();
    //proc->tty16 = tty16_create(0, 0, SCRN_WIDTH, SCRN_HEIGHT);
    //proc->tty = get_tty_curr();
    
    //Create main thread
    thread = (thread_s *)kcalloc(sizeof(thread_s), 1);
    if(!thread) {
        //monitor_str_write("ERROR: memory alloc\n");
        return 0;
    }
    
    thread->list_item.list = NULL;
    thread->process = proc;
    thread->suspend = FALSE;
    thread->stack_size = stack_size;
    thread->stack = (void *)stack;
    thread->esp = stack + stack_size - 28;
    thread->entry_point = elf->elf_header->e_entry;
    thread->id = get_thread_id();
    thread->stack_top = stack_size + stack;
    thread->ticks0 = 0;
    thread->pause_time = 0;

    proc->thread_id[proc->threads_count++] = thread->id;
    u32int *esp = (u32int *)(stack + stack_size);
    eflags = read_eflags();                         ///!!!!!!!!!!!!! need test
    eflags |= (1 << 9);

    if(kern) {
        //esp[-1] = (u32int) kern;
        //esp[-2] = (u32int) proc;
        //esp[-3] = (u32int) elf;
        //esp[-5] = (u32int) &elf_start;
        esp[-4] = (u32int)&proc_destroy;
        esp[-5] = elf->elf_header->e_entry;
        esp[-7] = eflags;
    }
    else {

        esp[-1] = (u32int) cmd_line;
        //esp[-1] = (u32int) kern;
        esp[-2] = (u32int) proc;
        esp[-3] = (u32int) elf;
        esp[-5] = (u32int) &elf_start;
        esp[-7] = eflags;
    }

    add_thread(thread);
    add_process(proc);
    
    //wprint("tid(%d), pid(%d), in exec_proc(%x)| thrd adr(%x)| faddr(%x), cnt(%d), vheap(%x), pdir(%x), vdir(%x), inf(%x)\n", get_curr_thread()->id, get_curr_proc()->pid, proc, thread, proc->list_item.list->first, proc->list_item.list->count, heap->start, proc->page_dir, proc->page_dir_vaddr, get_page_info(proc->page_dir, USER_HEAP_BASE));
    return proc->pid;
}

void elf_start(elf_sections_s *elf, process_s *proc, u8int *cmd_line/*,bool kern*/) {

    void *entr_point = 0;
    u32int s = 0;
    u32int *esp;
    heap_s *heap = (heap_s *)USER_HEAP_BASE;

    for(u32int i = 0; i < elf->elf_header->e_phnum; i++) {
        //Copy non zero bytes
        s = vfs_read(elf->file, elf->prog_header[i].p_offset, elf->prog_header[i].p_filesize, (void *)elf->prog_header[i].p_vaddr);
        //Set zero in other segment field
        memset((void *)(elf->prog_header[i].p_vaddr + s), 0, elf->prog_header[i].p_memsz - s);
    }
    esp = (u32int *)((u32int) proc->ustack_vaddr + proc->stack_page_count * PAGE_SIZE - 20);
    esp[-1] = 0;
    esp[-2] = (u32int)cmd_line;
    esp[-3] = (u32int)&syscall_proc_destroy;         //esp[-3] = (u32int)&exit; --- sys_call need!!!!
    //Process heap initialization
    heap->start = USER_HEAP_BASE;
    heap->size = USER_HEAP_SIZE;
    heap->end = heap->start + heap->size;
    heap->count = 1;
    heap->heap_mtx = 0;
    heap->blocks = (block_hmem *)USER_HEAP_BLOCKS_INF;

    heap->blocks[0].base = heap->start;
    heap->blocks[0].size = sizeof(heap_s);

    proc->tty16 = tty16_create(0, 1, 30, 5);
    entr_point = (void *)elf->elf_header->e_entry;

    kfree(elf->elf_header);
    kfree(elf->prog_header);
    kfree(elf->sect_header);
    kfree(elf);
   
    switch_user_mode(entr_point, (u32int)esp - 12);
    return;
}

void proc_destroy(void) {

    process_s *proc;
    thread_s  *thread;

    stop();
    thread = get_curr_thread();
    proc = get_curr_proc();

    while(proc->threads_count > 0) {

        //Get thread from queue by ID
        thread = get_thread(proc->thread_id[proc->threads_count - 1]);
        
        //Remote thread from queue
        remove_thread(thread);
        thread->process->threads_count--;
        //del_vscreen(thread->tty);
        kfree(thread->user_stack);
        kfree(thread->stack);
        kfree(thread);
    }
    tty16_remove(proc->tty16);
    //Free pages for process address space
    free_phys_pages(proc->ustack_paddr, proc->stack_page_count);
    free_phys_pages(proc->heap_paddr, proc->heap_page_count);
    free_phys_pages(proc->blocks_paddr, proc->blocks_page_count);
    free_phys_pages(proc->seg_paddr, proc->seg_page_count);

    if(proc->parent_proc_thread->suspend) {

        proc_set_in_focus(proc->parent_proc_thread->process->pid);
        thread_suspend(proc->parent_proc_thread, false);
    }
    //Remote process
    remove_process(proc);
    kfree(proc->page_dir_vaddr);
    kfree(proc);

    wprint("destr proc(%d) OK\n", proc->pid);
    //Switch scheduler
    start();
    switch_task();

    return;
}

u32int user_exec(u8int *name, u8int *cmd_line) {

    //stop();
    u32int ret = exec_proc(name, false, cmd_line);
    //start();
    return ret;

}

void proc_set_in_focus(u32int pid) {

    process_s *proc = (process_s *)proc_list.first;
    u32int     idx = proc_list.count;

    do {
        if(proc->pid == pid) {
            break;
        }
        proc = (process_s *)proc->list_item.next;
        idx--;

    } while(idx);

    if(idx != 0) {
        in_focus_pid = pid;
    }
    return;
}

thread_s *proc_create(u8int *name, void *entr_point, u32int stack_size, bool kernel, bool suspend) {

    void *stack = NULL;
    u32int *pdir_vaddr;
    u32int eflags;

    asm volatile ("pushf");
    asm volatile ("pop %0":"=r"(eflags));
    eflags |= (1 << 9);

    process_s *proc = (process_s*)kcalloc(sizeof(process_s), 1);
    thread_s *thread = (thread_s*) kcalloc(sizeof(thread_s), 1);
    physaddr page_dir = clone_kern_dir((u32int *)&pdir_vaddr);            //all_dir_pages(&pdir_vaddr);
//-----------------------------------------------------------------------------------
    /*physaddr *tmp_page = (physaddr *)TEMP_PAGE;
    u32int table_indx = (u32int)USER_HEAP_BASE >> 22;
    temp_map_page(page_dir);
    tmp_page[table_indx] = 0 << 0;
    table_indx = (u32int)USER_MEMORY_START >> 22;
    tmp_page[table_indx] = 0 << 0;*/

    u32int indx = (u32int)USER_MEMORY_START >> 22;
    u32int *t = pdir_vaddr;
    memset(&t[indx], 0, PAGE_SIZE - indx * 4);
//-----------------------------------------------------------------------------------
    //Process heap creation
    u32int page_count = USER_HEAP_SIZE / PAGE_SIZE;
    u32int heap_page_count = page_count;
    physaddr tmp_paddr = alloc_phys_pages(page_count);
    physaddr heap_paddr = tmp_paddr;

    u8int err = phys2virt(page_dir, USER_HEAP_BASE, tmp_paddr, page_count, 0x07);
    if(err == 0) {
        //monitor_str_write("ERROR: memory mapping\n");
        return 0;
    }

    page_count = USER_HEAP_BLOCK_INF / PAGE_SIZE;
    u32int blocks_page_count = page_count;
    tmp_paddr = alloc_phys_pages(page_count);
    physaddr blocks_paddr = tmp_paddr;

    err = phys2virt(page_dir, USER_MEMORY_START, tmp_paddr, page_count, 0x07);
    if(err == 0) {
        //monitor_str_write("ERROR: memory mapping\n");
        return 0;
    }
    // Инициализируем процесс 
    strcpy(proc->name, name);
    proc->suspend = suspend;
    proc->pid = get_pid();
    proc->page_dir_vaddr = pdir_vaddr;
    proc->page_dir = page_dir;
    proc->threads_count = 0;
    proc->list_item.list = NULL;
    proc->heap_page_count = heap_page_count;
    proc->blocks_page_count = blocks_page_count;
    proc->heap_paddr = heap_paddr;
    proc->blocks_paddr = blocks_paddr;
    proc->parent_proc_thread = get_curr_thread();
    //proc->tty16 = tty16_create(0, 0, SCRN_WIDTH, SCRN_HEIGHT);

    // Инициализируем thread
    thread->process = proc;
    thread->id = get_thread_id();
    thread->suspend = suspend;
    thread->stack_size = stack_size;
    thread->entry_point = (u32int)entr_point;

    thread->list_item.list = NULL;
    thread->wait_list_item.list = NULL;

    thread->ticks0 = 0;
    thread->pause_time = 0;

    stack = kmalloc(stack_size);
    thread->stack = stack;
    thread->esp = (u32int)stack + stack_size - 28;
    thread->stack_top = (u32int)(stack + stack_size);
    //thread->tty = kget_vscreen();          

    list_add(&proc_list, &proc->list_item);
    list_add(&thread_list, &thread->list_item);

    proc->thread_id[proc->threads_count++] = thread->id;

    if(kernel) {
        //thread->tty = kget_vscreen();
        u32int *esp = (u32int *)thread->stack_top;
        esp[-4] = (u32int)&proc_destroy;
        esp[-5] = (u32int)entr_point;
        esp[-7] = eflags;
    }
    if(suspend) {
        remove_thread(thread);
        thread->list_item.list = NULL;
        add_wait(thread);
    }
    //wprint("create OK, name(%s), entr(%x), proc(%x), pdir(%x) ", &thread->process->name, thread->entry_point, thread->process, thread->process->page_dir);
    //wprint("prcount(%d)\n", list_size(&proc_list));

    return thread;
}