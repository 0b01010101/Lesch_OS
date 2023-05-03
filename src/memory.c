#include "memory.h"
#include "common.h"

physaddr     kern_page_dir = KERN_PAGE_TABLE;

memory_map_s *entr_mmap = 0;
u64int       size_phymemory = 0;

u32int       free_pcount = 0;               //count of free pages;
physaddr     free_pmem_pointer = -1;        //Pointer to the beginning of free memory;
u32int       kern_stack = 0;
u32int       memory_size = 0;        //the amount of allocated physical memory whose address is higher than 1 MB

heap_s       kheap;                         //heap of Kernel;
mutex_s      phys_mem_mtx;


void on_page_mode() {

    u64int      virt_addr = 0;
    physaddr    phys_addr = 0;

    //u32int      cr0;
    u32int      frame = 0;
    u32int      table_indx = 0;
    u32int      page_indx  = 0;
    u32int      table_flags = 0;
    u32int      page_flags  = 0;

    u32int *kern_dir   = (u32int*) kern_page_dir;
    u32int *page_table = (u32int*) (kern_page_dir + PAGE_SIZE);

    memset(kern_dir, 0, PAGE_SIZE);

    for(virt_addr = 0; virt_addr < (KERN_BASE + KERN_SIZE); virt_addr += PAGE_SIZE) {

        frame = virt_addr >> PAGE_OFFSET_BITS;       //>> 12;
        table_indx = frame >> PAGE_TABLE_INDEX_BITS; //>> 10;
        page_indx  = frame &  PAGE_TABLE_INDEX_MASK; //0x03FF;

        table_flags = PAGE_PRESENT|PAGE_WRITEABLE|PAGE_USER;
        page_flags  = PAGE_PRESENT|PAGE_WRITEABLE|PAGE_USER;

        kern_dir[table_indx] = (u32int) (page_table + table_indx * 1024) | table_flags;
        page_table[table_indx*1024 + page_indx] = (u32int) (phys_addr | page_flags);

        phys_addr += PAGE_SIZE;
    }

    asm volatile ("mov %0, %%cr3" :: "r"(kern_page_dir));
    asm volatile ("mov %cr0, %ebx");
    asm volatile ("or $0x80000000, %ebx");
    asm volatile("mov %ebx, %cr0");

    monitor_str_write("Page mode: On\n");

    return;
}

void check_memory_map(memory_map_s *mmap_addr, u32int len) {

    int n = len / sizeof(memory_map_s);
    entr_mmap = mmap_addr;

    monitor_str_write("PHYSICAL MEMORY MAP!: ");
    write_dec(n);
    monitor_char_put('\n');
    
    for(int i = 0; i < n; i++) {

        monitor_str_write("$ str_size: ");
        write_dec(entr_mmap[i].size);

        if(entr_mmap[i].type == 1) {
            monitor_str_write("| AVAILABLE: | ");
        }
        else {
            monitor_str_write("| RESERVED:  | ");
        }

        monitor_str_write("addr: ");
        write_hex(entr_mmap[i].addr);

        monitor_str_write(" | length: ");
        write_hex(entr_mmap[i].length);

        monitor_str_write(" | type: ");
        write_hex(entr_mmap[i].type);

        monitor_char_put('\n');

        size_phymemory += entr_mmap[i].length;
    }

    monitor_str_write("Installed memory size: ");
    write_dec(size_phymemory / 1024);
    monitor_str_write(" KB\n");

    return;
}

void temp_map_page(physaddr paddr) {
    /*
    #define TABLE_INDX  (TEMP_PAGE>>22)                  -- u32int table_indx = TEMP_PAGE >> 22;            
    #define PAGE_INDX   ((TEMP_PAGE>>12) & 0x03FF)       -- u32int page_indx  = (TEMP_PAGE >> 12) & 0x03FF;
    #define PAGES_I     ((TABLE_INDX << 10) + PAGE_INDX) -- pages[(table_indx << 10) + page_indx] = (u32int) paddr|PAGE_PREZENT|PAGE_WRITEABLE;
    */
    volatile u32int *pages = (u32int *)(KERN_PAGE_TABLE + PAGE_SIZE);
    pages[PAGES_I] = (u32int) paddr|PAGE_PRESENT|PAGE_WRITEABLE;  

    asm volatile ("invlpg (,%0,)"::"a"(TEMP_PAGE));
    return;
}

block_pmem* get_free_block(physaddr paddr) {

    temp_map_page(paddr);

    return (block_pmem *)TEMP_PAGE;
}

void free_phys_pages(physaddr base, u32int count) {

    block_pmem *tmp_block;

    get_mtx(&phys_mem_mtx, true);       // {

    if(free_pmem_pointer == -1) {

        tmp_block = get_free_block(base);

        tmp_block->prev = base;
        tmp_block->next = base;
        tmp_block->size = count;

        free_pmem_pointer = base;
    }
    else {
        physaddr cur_block = free_pmem_pointer;

        do {
            tmp_block = get_free_block(cur_block);
        /* Если заданный адрес расположен после текущего блока */
            if(base == cur_block + (tmp_block->size << 12)) {

                tmp_block->size += count;
                /* if there is another block after the one being released free block*/
                if(tmp_block->next == base + (count << 12)) {

                    physaddr next_old = tmp_block->next;

                    tmp_block = get_free_block(next_old);
                    physaddr next_new = tmp_block->next;
                    u32int new_count = tmp_block->size;

                    tmp_block = get_free_block(cur_block);
                    tmp_block->next = next_new;
                    tmp_block->size += new_count;

                    tmp_block = get_free_block(next_new);
                    tmp_block->prev = cur_block;
                }
                break;
            }
            /* If the block being released is before the rest of the blocks */
            if(cur_block == base + (count << 12)) {
                
                u32int prev = tmp_block->prev;
                u32int next = tmp_block->next;
                u32int old_count = tmp_block->size;

                tmp_block = get_free_block(base);
                tmp_block->prev = prev;
                tmp_block->next = next;
                tmp_block->size += old_count;

                tmp_block = get_free_block(next);
                tmp_block->prev = base;

                tmp_block = get_free_block(prev);
                tmp_block->next = base;

                break;
            }
            /* If the released block is between two free blocks */
            if(cur_block > base) {
                /* Just insert it into the list */
                u32int prev = tmp_block->next;
                tmp_block->prev = base;

                tmp_block = get_free_block(prev);
                tmp_block->next = base;

                tmp_block = get_free_block(base);
                tmp_block->prev = prev;
                tmp_block->next = cur_block;
                tmp_block->size = count;

                break;
            }
            /* Or we have one free block */
            if(tmp_block->next == free_pmem_pointer) {
                /* Insert a new free block into the list */
                tmp_block->prev = base;
                //u32int next = tmp_block->next;
                tmp_block->next = base;

                tmp_block = get_free_block(base);
                tmp_block->prev = cur_block;
                tmp_block->next = cur_block;
                tmp_block->size = count;

                break;
            }

            cur_block = tmp_block->next;

        } while(cur_block != free_pmem_pointer);    

        if(base < free_pmem_pointer) {
            free_pmem_pointer = base;
        }
    }
    free_pcount += count;

    clean_mtx(&phys_mem_mtx);            // }
    return;
}

physaddr alloc_phys_pages(u32int count) {

    physaddr result = -1;
    block_pmem *tmp_block;

    get_mtx(&phys_mem_mtx, true); // {

    if(free_pcount < count) {
        clean_mtx(&phys_mem_mtx);
        return -1;
    }
    /* If we have free physical memory */
    if(free_pmem_pointer != -1) {

        physaddr cur_block = free_pmem_pointer;

        do {
            tmp_block = get_free_block(cur_block);

            if(tmp_block->size == count) {

                physaddr prev = tmp_block->prev;
                physaddr next = tmp_block->next;

                tmp_block = get_free_block(next);
                tmp_block->prev = prev;

                tmp_block = get_free_block(prev);
                tmp_block->next = next;

                if(cur_block == free_pmem_pointer) {        //if the last block;
                    
                    free_pmem_pointer = next;

                    if(free_pmem_pointer == cur_block) {
                        free_pmem_pointer = -1;
                    }
                }
                result = cur_block;
                break;
            }
            if(tmp_block->size > count) {

                tmp_block->size -= count;
                result = cur_block + (tmp_block->size << 12);

                break;
            }

            cur_block = tmp_block->next;

        } while(cur_block != free_pmem_pointer);

        if(result != -1) {

            free_pcount -= count;
        }
    }
    clean_mtx(&phys_mem_mtx);    // }

    return result;
}

void init_memory_manager(u32int stack) {

    kern_stack = stack;

    on_page_mode();

    memory_map_s *entr;

    for(entr = entr_mmap; entr->type; entr++) {

        if((entr->type == 1) && (entr->addr >= 0x100000)) {

            free_phys_pages(entr->addr, entr->length >> 12);
            memory_size += entr->length;
        }
    }
    monitor_str_write("Free pages: ");
    write_dec(free_pcount);
    monitor_str_write(" pages\n");

    //u32int tmp = KERN_HEAP_BLOCK_INF / PAGE_SIZE;
    //physaddr *phys = alloc_phys_pages(tmp);
    //Отображаем страницы для служебной информации;
    phys2virt(KERN_PAGE_TABLE, (void *)(KERN_MEMORY_START+KERN_SIZE),(physaddr)(KERN_MEMORY_START+KERN_SIZE), KERN_HEAP_BLOCK_INF >> PAGE_OFFSET_BITS, 0x03/*PAGE_PRESENT|PAGE_WRITEABLE*/);    
    //tmp = (u32int)KERN_HEAP_BASE / PAGE_SIZE;
   // phys = alloc_phys_pages(tmp);
    //Отображаем страницы под кучу;
    phys2virt(KERN_PAGE_TABLE, KERN_HEAP_BASE, (physaddr)(KERN_MEMORY_START+KERN_SIZE+KERN_HEAP_BLOCK_INF), KERN_HEAP_SIZE >> PAGE_OFFSET_BITS, 0x03/*PAGE_PRESENT|PAGE_WRITEABLE*/);

    kheap.blocks = (block_hmem *) (KERN_MEMORY_START + KERN_SIZE);
    memset(kheap.blocks, 0, KERN_HEAP_BLOCK_INF);

    kheap.start = KERN_HEAP_BASE;
    kheap.size  = KERN_HEAP_SIZE;
    kheap.count = 0;
    kheap.end   = kheap.start + kheap.size;

    return;
} 

void *virt2phys(physaddr page_dir, void *vaddr) {

    physaddr *tmp_page = (physaddr *)TEMP_PAGE;
    physaddr  table;
    physaddr  page;

    u32int table_indx = (u32int) vaddr >> 22;
    u32int page_indx = ((u32int) vaddr >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK;
    u32int page_offset = ((u32int) vaddr & PAGE_OFFSET_MASK);

    temp_map_page(page_dir);
    table = tmp_page[table_indx];
    temp_map_page(table);
    page = tmp_page[page_indx] & 0xFFFFF000;

    u32int t = page + page_offset;
    return (void *)t;
}

u8int phys2virt(physaddr page_dir, void *vaddr, physaddr paddr, u32int count, u32int flags) {
//(Адрес каталога страниц,Начальный виртуальный адрес,Начальный физический адрес,Число отображаемых страниц, Флаги страниц)

    physaddr *tmp_page = (physaddr *)TEMP_PAGE;
    physaddr table;
    u32int table_flags;
    /* Creating the necessary pages in a loop */
    for(; count; count--) {

        u32int table_indx = (u32int) vaddr >> 22;
        u32int page_indx  = ((u32int) vaddr >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK;

        temp_map_page(page_dir);
        table = tmp_page[table_indx];
        /* Check the presence flag of the table */
        if(!(table & PAGE_PRESENT)) {
           
            physaddr addr = alloc_phys_pages(1);

            if(addr != -1) {
                
                temp_map_page(addr);
                memset(tmp_page, 0, PAGE_SIZE);

                temp_map_page(page_dir);

                table_flags = PAGE_PRESENT|PAGE_WRITEABLE|PAGE_USER;
                tmp_page[table_indx] = (addr & ~PAGE_OFFSET_MASK) | table_flags;

                table = addr;
            }
            else {
                return FALSE;
            }
        }

        table &= ~PAGE_OFFSET_MASK;

        temp_map_page(table);
        tmp_page[page_indx] = (u32int) (paddr & ~PAGE_OFFSET_MASK) | flags;

        asm volatile ("invlpg (,%0,)" :: "a" (vaddr));

        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
    
    return TRUE;
}

static inline u8int overlapped_blocks(void *base1, u32int size1, void *base2, u32int size2) {

    return ((base1 >= base2) && (base1 < base2 + size2)) || ((base2 >= base1) && (base2 < base1 + size1));
}

void *kmalloc_common(u32int size, bool align, physaddr *phys_addr) {

    void *vaddr = kheap.start + sizeof(heap_s);
    bool overlap = false;

    get_mtx(&kheap.heap_mtx, true);     // {
        //wprint("mtx(%x), size(%d), PID(%d)\n", kheap.heap_mtx, kheap.size, get_curr_proc()->pid);

    do {
        overlap = false;

        if(align) {

        u32int tmp_addr = (u32int) vaddr;

        if(tmp_addr & 0x0FFF) {

            tmp_addr &= 0xFFFFF000;
            tmp_addr += PAGE_SIZE;

            vaddr = (void *)tmp_addr;
        }
    }
        for(int i = kheap.count-1; i >= 0; i--) {

            if(overlapped_blocks(kheap.blocks[i].base, kheap.blocks[i].size, vaddr, size)) {

                vaddr = kheap.blocks[i].base + kheap.blocks[i].size;
                overlap = true;
            }
        }
    } while(overlap);

    if(phys_addr) {

        physaddr tmp_physaddr = get_page_info(kern_page_dir, vaddr);
        tmp_physaddr &= ~PAGE_OFFSET_MASK;

        *phys_addr = tmp_physaddr;
    }

    for(int i = kheap.count-1; i >= 0; i--) {

        kheap.blocks[i+1].base = kheap.blocks[i].base;
        kheap.blocks[i+1].size = kheap.blocks[i].size;
    }

    kheap.blocks[0].base = vaddr;
    kheap.blocks[0].size = size;

    kheap.count++;
    kheap.size -= size;
    
    clean_mtx(&kheap.heap_mtx);       // }
    //wprint("mtx(%x), size(%d) PID(%d)\n", kheap.heap_mtx, kheap.size, get_curr_proc()->pid);
    
    return vaddr;
}

void *kmalloc(u32int size) {

    return kmalloc_common(size, false, NULL);
}

void *kmalloc_align(u32int size) {

    return kmalloc_common(size, true, NULL);
}

void  *kmalloc_phys(u32int size, physaddr *phys_addr) {

    return kmalloc_common(size, false, phys_addr);
}

void *kmalloc_aphys(u32int size, physaddr *phys_addr) {

    if(phys_addr == NULL) {
        monitor_str_write("!ERROR: NOT PHYS_ADDR!\n");
    }

    return kmalloc_common(size, true, phys_addr);
}

void *kcalloc(u32int num, u32int size) {

    void *tmp = kmalloc_common(size * num, false, NULL);

    if(tmp == NULL) {
        return NULL;
    }

    memset(tmp, 0, num * size);
    return tmp;
}

void kfree(void *vaddr) {

    int block_indx = 0;
    int size = 0;
    int i = 0;

    get_mtx(&kheap.heap_mtx, true);         // {

    if(vaddr == NULL) {
        clean_mtx(&kheap.heap_mtx);
        return;
    }

    for(i = 0; i < kheap.count; i++) {

        if(vaddr == kheap.blocks[i].base) {

            size = kheap.blocks[i].size;
            block_indx = i;
            break;
        }
    }

    if(i == kheap.count) {
        clean_mtx(&kheap.heap_mtx);
        return;
    }

    for(i = block_indx; i < kheap.count-1; i++) {

        kheap.blocks[i].base = kheap.blocks[i+1].base;
        kheap.blocks[i].size = kheap.blocks[i+1].size;
    }
    kheap.count--;
    kheap.size += size;

    clean_mtx(&kheap.heap_mtx);        // }

    return;
}

physaddr get_kern_dir(void) {

    return kern_page_dir;
}

physaddr get_page_info(physaddr page_dir, void *vaddr) {


    physaddr page_table = page_dir;
    u8int    shift;

    for(shift = PHYS_ADDR_BITS - PAGE_TABLE_INDEX_BITS; shift >= PAGE_OFFSET_BITS; shift -= PAGE_TABLE_INDEX_BITS) {

        u32int indx = ((u32int)vaddr >> shift) & PAGE_TABLE_INDEX_MASK;
        temp_map_page(page_table);

        if(shift > PAGE_OFFSET_BITS) {

            page_table = ((volatile physaddr *)TEMP_PAGE)[indx];

            if(!(page_table & PAGE_PRESENT)) {
                return 0;
            }
        }
        else {
            page_table = page_table & ~PAGE_OFFSET_MASK;
            temp_map_page(page_table);

            return ((volatile physaddr *)TEMP_PAGE)[indx];
        }
    }
    return 0;
}

physaddr clone_kern_dir(u32int *vaddr/*, bool user*/) {

    physaddr new_dir = 0;
    u32int  *vnew_dir = (u32int *)kmalloc_aphys(PAGE_SIZE, &new_dir);

    if(vnew_dir == NULL) {
        return 0;
    }

    memcpy(vnew_dir, (void *)kern_page_dir, PAGE_SIZE);
    *vaddr = (u32int)vnew_dir;
/*
    if(user) {
        u32int indx = USER_MEMORY_START >> 22;
        memset(vnew_dir[indx], 0, PAGE_SIZE - indx * 4);
    }
*/
    return new_dir;
}

physaddr all_dir_pages(u32int *vaddr) {

    physaddr new_dir = 0;
    u32int *vnew_dir = (u32int *)kmalloc_aphys(PAGE_SIZE * 5, &new_dir);

    if(vnew_dir == NULL) {
        return 0;
    }
    memcpy(vnew_dir, (void *)kern_page_dir, PAGE_SIZE * 5);
    *vaddr = (u32int)vnew_dir;

    return new_dir;
}

s32int get_kheap_block(void *vaddr) {
     
    u32int block = 0;
    u32int i;

    get_mtx(&kheap.heap_mtx, true);

    if(vaddr == NULL) {
        return -1;
    }

    for(i = 0; i < kheap.count; i++) {

        if(vaddr == kheap.blocks[i].base) {

            block = i;
            break;
        }
    }

    if(i == kheap.count) {
        return -1;
    }

    clean_mtx(&kheap.heap_mtx);
    return block;
}

void show_kheap_block(void *vaddr) {

    s32int block = get_kheap_block(vaddr);

    if(block < 0) {
        return;
    }

    monitor_str_write("heap.end(");
    write_hex((u32int)kheap.end);
    monitor_str_write(") heap.size(");
    write_dec((u32int)kheap.size);
    monitor_str_write(")\nblock num = ");
    write_dec((u32int)block);
    monitor_str_write(") from ");
    write_dec(kheap.count);
    monitor_str_write(" blocks| block.size(");
    write_dec((u32int)kheap.blocks[block].size);
    monitor_str_write(")\n");

    return;
}

u8int t_kfree(void *vaddr) {

    int block_indx = 0;
    int siz = 0;
    int i = 0;

    get_mtx(&kheap.heap_mtx, true);         // {

    monitor_str_write("\nIN t_kfree(");
    write_hex((u32int)vaddr);
    monitor_str_write(")\n   before: kheap.size(");
    write_dec(kheap.size);
    monitor_str_write(") kheap.count(");
    write_dec(kheap.count);
    monitor_str_write(")\n");

    u32int c = *(u32int *)vaddr;
    if(vaddr == NULL) {
        monitor_str_write("ERROR!(1)\n");
        clean_mtx(&kheap.heap_mtx);
        return 1;
    }

    for(i = 0; i < kheap.count; i++) {

        if(vaddr == kheap.blocks[i].base) {

            monitor_str_write("   block : base(");
            write_hex((u32int)kheap.blocks[i].base);
            monitor_str_write(") size(");
            write_dec(kheap.blocks[i].size);
            monitor_str_write(") num(");
            write_dec(i);
            monitor_str_write(")\n");

            block_indx = i;
            siz = kheap.blocks[i].size;
            break;
        }
    }

    if(i == kheap.count) {
        clean_mtx(&kheap.heap_mtx);
        return 2;
    }

    for(i = block_indx; i < kheap.count-1; i++) {

        kheap.blocks[i].base = kheap.blocks[i+1].base;
        kheap.blocks[i].size = kheap.blocks[i+1].size;
    }
    kheap.count--;
    kheap.size += siz;

    monitor_str_write("   after : kheap.size(");
    write_dec(kheap.size);
    monitor_str_write(") kheap.count(");
    write_dec(kheap.count);
    monitor_str_write(")\n");
    monitor_str_write("OUT t_kfree\n\n");

    clean_mtx(&kheap.heap_mtx);        // }
    return 0;
}

