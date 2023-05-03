#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

#define MBOOT_FLAG_MEM      0x1
#define MBOOT_FLAG_DEVICE   0x2
#define MBOOT_FLAG_CMDLINE  0x4
#define MBOOT_FLAG_MODS     0x8
#define MBOOT_FLAG_AOUT     0x10
#define MBOOT_FLAG_ELF      0x20
#define MBOOT_FLAG_MMAP     0x40
#define MBOOT_FLAG_CONFIG   0x80
#define MBOOT_FLAG_LOADER   0x100
#define MBOOT_FLAG_APM      0x200
#define MBOOT_FLAG_VBE      0x400

struct multiboot_header {

  u32int    flags;              /* Флаги заголовка */
  u32int    mem_lower;          /* Размер базовой памяти */
  u32int    mem_upper;          /* Размер памяти выше 1 Мб */
  u32int    boot_device;
  u32int    cmdline;
  u32int    mods_count;
  u32int    mods_addr;
  u32int    num;
  u32int    size;
  u32int    addr;
  u32int    shndx;
  u32int    mmap_length;        /* Размер карты памяти */
  u32int    mmap_addr;          /* Адрес начала карты памяти */
  u32int    drives_length;
  u32int    drives_addr;
  u32int    config_table;
  u32int    boot_loader_name;
  u32int    apm_table;
  u32int    vbe_control_info;
  u32int    vbe_mode_info;
  u32int    vbe_mode;
  u32int    vbe_interface_seg;
  u32int    vbe_interface_off;
  u32int    vbe_interface_len;

}__attribute__((packed));

struct memory_map_header {

    u32int  size;
    u64int  addr;
    u64int  length;               //bytes;
    u32int  type;

}__attribute__((packed));

typedef struct multiboot_header multiboot_s;
typedef struct memory_map_header memory_map_s;

#endif