#ifndef     VESA_H
#define     VESA_H

#include    "common.h"
#include    "memory.h"
#include    "help_func.h"
#include    "bios.h"

extern physaddr kern_page_dir;

#define VESA_PHYS   0xB00000

typedef union u24int_s u24int;
typedef struct inf_mode inf_mode_s;
typedef struct vbe_inf vbe_inf_s;

struct inf_mode {

    u16int      attrib;
    u8int       windA;
    u8int       windB;
    u16int      granul;
    u16int      windSize;
    u16int      segmA;
    u16int      segmB;
    u32int      windFuncPtr; 
    u16int      pitch;

    u16int      resolX;
    u16int      resolY;
    u8int       wchar; 
    u8int       yChar;
    u8int       planes;
    u8int       bpp;
    u8int       banks;

    u8int       memModel;
    u8int       bankSize;
    u8int       imgPages;
    u8int       res0;

    u8int       red_mask;
    u8int       red_pos;
    u8int       green_mask;
    u8int       green_pos;

    u8int       blue_mask;
    u8int       blue_pos;
    u8int       res_mask;
    u8int       res_pos;
    u8int       dir_color_attr;

    //LBF (lin. frame buff.)
    u32int      lin_base;
    u32int      offScreenMemOff;
    u16int      offScreenMemSize;
    u8int       res1[206];
};

struct vbe_inf {

    u8int       sign[4];
    u16int      version;
    u32int      oem;
    u32int      capabil;
    u32int      video_modes;
    u16int      video_mem;
    u16int      soft_revision;
    u32int      vendor;
    u32int      prod_name;
    u32int      prod_revision;

    u8int       res[222];
    u8int       oem_data[256];
}__attribute__((packed));

union u24int_s {

    struct{
        u8int r;
        u8int g;
        u8int b;
    }rgb;
    u32int integer :24      __attribute__((packed));;
};

void vesa_init(void);
void vesa_set_mode(u32int);
void vesa_get_mode(u16int mode, inf_mode_s *mode_info);
void vesa_memcpy32_to24(u24int *dst, u32int *src, u32int count);
void vesa_memset_rgb(u8int *dst, u32int rgb, u32int count);
void *vesa_get_lfb(void);
s32int vesa_get_resolY(void);
s32int vesa_get_resolX(void);
u32int vesa_find_mode(u32int width, u32int height, u32int bpp);

#endif