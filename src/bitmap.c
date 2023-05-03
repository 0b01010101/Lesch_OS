#include "bitmap.h"

bitmap_s *bmp_create(u8int *name) {

    u32int size;
    u32int off;
    void   *buff;

    bitmap_s *ret = kmalloc(sizeof(bitmap_s));
    vfs_node_s *file = file_open(name, 0);

    if(!file) {
        return NULL;
    }
    
    size = vfs_get_file_size(file);
    buff = kmalloc(size);
    vfs_read(file, 0, size, buff);

    bmp_filehead_s *head = buff;
    off = head->off;

    bmp_infohead_s *info = buff + sizeof(bmp_filehead_s);

    ret->width = info->width;
    ret->height = info->height;
    ret->size = size;
    ret->bpp = info->bit_count;
    ret->image_bytes = (void *)((u32int)buff + off);
    ret->buff = buff;

    vfs_close(file);
    return ret;
}

void bmp_to_framebuff(bitmap_s *bmp, u32int *frame_buff) {

    if(!bmp) {
        return;
    }
    u32int rgb;
    u32int r;
    u32int g;
    u32int b;
    u32int n = 0;
    u32int bytes = 0;
    u8int *img = bmp->image_bytes;

    u32int x_b = bmp->width * (bmp->bpp / 8);
    if(x_b % 4 != 0) {
        bytes = x_b + (4 - (x_b % 4));
    }
    else {
        bytes = x_b;
    }

    if(bmp->bpp == 24) {
        for(u32int y = 0; y < bmp->height; y++) {

            u8int *img_row = img + y * bytes;
            u32int *frameb_row = (void *)frame_buff + (bmp->height - 1 - y) * bmp->width * 4 ;
            n = 0;

            for(u32int x = 0; x < bmp->width; x++) {

                b = img_row[n++] & 0xFF;
                g = img_row[n++] & 0xFF;
                r = img_row[n++] & 0xFF;
                rgb = ((r << 16) | (g << 8) | (b)) & 0x00FFFFFF;
                rgb |= 0xFF000000;
                frameb_row[x] = rgb;
            }
        }
        return;
    }
    else if(bmp->bpp == 32) {
        u32int a;
        for(u32int y = 0; y < bmp->height; y++) {

            u8int *img_row = img + y * bytes;
            u32int *frameb_row = (void *)frame_buff + (bmp->height - 1 - y) * bmp->width * 4 ;
            n = 0;

            for(u32int x = 0; x < bmp->width; x++) {

                b = img_row[n++] & 0xFF;
                g = img_row[n++] & 0xFF;
                r = img_row[n++] & 0xFF;
                a = (img_row[n++] & 0xFF) << 24;
                rgb = ((a << 24) | (r << 16) | (g << 8) | (b)) & 0x00FFFFFF;
                frameb_row[x] = rgb;
            }
        }
        return;
    }
    else {
        return;
    }
}

void bmp_display(bitmap_s *bmp) {

    if(!bmp) {
        return;
    }

    u32int *screen = (u32int *)vesa_mem;
    u32int rgb;
    u32int r;
    u32int g;
    u32int b;
    u32int n = 0;
    u32int bytes = 0;
    u8int *img = bmp->image_bytes;

    u32int x_b = bmp->width * (bmp->bpp / 8);
    if(x_b % 4 != 0) {
        bytes = x_b + (4 - (x_b % 4));
    }
    else {
        bytes = x_b;
    }

    if(bmp->bpp == 24) {
        for(u32int y = 0; y < bmp->height; y++) {

            u8int *img_row = img + y * bytes;
            u32int *scrn_row = (void *)screen + (bmp->height - 1 - y) * bmp->width * 4 ;
            n = 0;

            for(u32int x = 0; x < bmp->width; x++) {

                b = img_row[n++] & 0xFF;
                g = img_row[n++] & 0xFF;
                r = img_row[n++] & 0xFF;
                rgb = ((r << 16) | (g << 8) | (b)) & 0x00FFFFFF;
                rgb |= 0xFF000000;
                scrn_row[x] = rgb;
            }
        }
        return;
    }
    else if(bmp->bpp == 32) {
        u32int a;
        for(u32int y = 0; y < bmp->height; y++) {

            u8int *img_row = img + y * bytes;
            u32int *scrn_row = (void *)screen + (bmp->height - 1 - y) * bmp->width * 4 ;
            n = 0;

            for(u32int x = 0; x < bmp->width; x++) {

                b = img_row[n++] & 0xFF;
                g = img_row[n++] & 0xFF;
                r = img_row[n++] & 0xFF;
                a = (img_row[n++] & 0xFF) << 24;
                rgb = ((a << 24) | (r << 16) | (g << 8) | (b)) & 0x00FFFFFF;
                scrn_row[x] = rgb;
            }
        }
        return;
    }
    else {
        return;
    }
}
