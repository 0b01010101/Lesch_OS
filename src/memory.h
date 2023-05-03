#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "multiboot.h"
#include "sync.h"

#define PAGE_SIZE               0x1000  //4096
#define PAGE_OFFSET_BITS        0x0C    //12
#define PAGE_OFFSET_MASK        0x0FFF
#define PAGE_TABLE_INDEX_BITS   0x0A    //10
#define PAGE_TABLE_INDEX_MASK   0x03FF
#define PHYS_ADDR_BITS          0x20    //32

/*-----------------------------------------------------
// PAGES/TABLES FLAGS
//---------------------------------------------------*/
#define PAGE_GLOBAL             (1<<8)
#define PAGE_DIRTY              (1<<6)
#define PAGE_ACCESSED           (1<<5)
#define PAGE_CACHE_DISABLE      (1<<4)
#define PAGE_WRITE_THROUGH      (1<<3)
#define PAGE_USER               (1<<2)
#define PAGE_WRITEABLE          (1<<1)
#define PAGE_PRESENT            (1<<0)

typedef unsigned int            physaddr;

#define LAST_ADDR               0xFFFFFFFF
#define KERN_BASE               0x200000
#define KERN_SIZE               0x1000000
#define KERN_PAGE_TABLE         0x500000
#define VIDEO_MEM               0xA00000

#define TEMP_PAGE               (KERN_BASE+KERN_SIZE-PAGE_SIZE)
#define TABLE_INDX              (TEMP_PAGE>>22)
#define PAGE_INDX               ((TEMP_PAGE>>12) & 0x03FF)
#define PAGES_I                 ((TABLE_INDX << 10) + PAGE_INDX)

#define USER_MEMORY_START       ((void *)0x80000000)
#define USER_MEMORY_END         ((void *)0xFFFFFFFF)

#define KERN_MEMORY_START       ((void *)0x00000000)
#define KERN_MEMORY_END         ((void *)0x7FFFFFFF)

#define KERN_HEAP_SIZE          0x2000000//0x1BA1200 (28 971 520)
#define KERN_HEAP_BASE          ((void *)(KERN_MEMORY_END - KERN_HEAP_SIZE))//(KERN_BASE + KERN_SIZE + KERN_HEAP_BLOCK_INF))
#define KERN_HEAP_BLOCK_INF     0x400000

#define USER_HEAP_BLOCKS_INF    ((void *)0x80000000)
#define USER_HEAP_SIZE          0x100000
#define USER_HEAP_BLOCK_INF     0x10000                //size of INFO_BLOCK
#define USER_HEAP_BASE          ((void*)(USER_MEMORY_START + USER_HEAP_BLOCK_INF))


typedef struct phys_memory_block block_pmem;
typedef struct heap_memory_block block_hmem;
typedef struct heap_str heap_s;

struct phys_memory_block {

    u32int      prev;
    u32int      next;
    u32int      size;           //size in pages;

}__attribute__((packed));

struct heap_memory_block {

    void*       base;
    u32int      size;

}__attribute__((packed));

struct heap_str {

    block_hmem*     blocks;     /* Массив данных о выделенных блоках */
    void*           start;      /* Виртуальный адрес начала кучи */
    void*           end;        /* Виртуальный адрес конца кучи */
    u32int          size;       /* Размер кучи */
    u32int          count;      /* Число выделенных блоков памяти */
    mutex_s         heap_mtx;    /*Мьютекс для синхронизации доступа ??????*/

};

void on_page_mode();
void check_memory_map(memory_map_s *, u32int);
void temp_map_page(physaddr paddr);
void free_phys_pages(physaddr base, u32int count);
void init_memory_manager(u32int stack);

void *kmalloc_common(u32int size, bool align, physaddr *phys_addr);
void *kmalloc_align(u32int size);
void *kmalloc_aphys(u32int size, physaddr *phys_addr);
void *kmalloc_phys(u32int size, physaddr *phys_addr);
void *kmalloc(u32int size);
void *kcalloc(u32int num, u32int size);
void kfree(void *vaddr);
u8int t_kfree(void *vaddr);

block_pmem* get_free_block(physaddr paddr);
physaddr alloc_phys_pages(u32int count);
physaddr get_kern_dir(void);
physaddr get_page_info(physaddr page_dir, void *vaddr);
physaddr clone_kern_dir(u32int *vaddr);
u8int phys2virt(physaddr page_dir, void *vaddr, physaddr paddr, u32int count, u32int flags);
void *virt2phys(physaddr page_dir, void *vaddr);
static inline u8int overlapped_blocks(void *base1, u32int size1, void *base2, u32int size2);

void show_kheap_block(void *vaddr);
s32int get_kheap_block(void *vaddr);

#endif