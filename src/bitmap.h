#ifndef     BITMAP_H
#define     BITMAP_H

#include    "common.h"
#include    "memory.h"
#include    "vesa.h"
#include    "vfs.h"

typedef struct bitmap bitmap_s;
typedef struct bmp_filehead bmp_filehead_s;
typedef struct bmp_infohead bmp_infohead_s;

struct bitmap {

    u32int width;
    u32int height;
    u32int size;
    u32int bpp;

    u8int  *image_bytes;
    u8int  *buff;
}__attribute__((packed));

struct bmp_filehead {

    u16int  type;
    u32int  size;
    u16int  res1;
    u16int  res2;
    u32int  off;
}__attribute__((packed));

struct bmp_infohead {

    u32int  size;
    s32int  width;
    s32int  height;
    u16int  planes;
    u16int  bit_count;
    u32int  compress;
    u32int  img_size;
    s32int  x_pels;
    s32int  y_pels;
    u32int  clr_used;
    u32int  clr_important;
}__attribute__((packed));

bitmap_s *bmp_create(u8int *name);
void bmp_to_framebuff(bitmap_s *bmp, u32int *frame_buff);
void bmp_display(bitmap_s *bmp);

#endif