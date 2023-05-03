#include "vesa.h"

u32int  curr_mode;
void *vesa_mem;

void vesa_init(void) {

    u32int eflags = 0;
    u32int pages = 0;
    inf_mode_s *mode_inf = kmalloc(sizeof(inf_mode_s));

    bios32_init();
    u32int ret = vesa_find_mode(1280, 720, 32);
    vesa_set_mode(ret|0x4000);
    vesa_get_mode(ret, mode_inf);

    vesa_mem = (void *)mode_inf->lin_base;
    pages = mode_inf->resolX *mode_inf->resolY * 4 / 4096;

    if(pages * 4096 < mode_inf->resolX * mode_inf->resolY * 4) {
        pages++;
    }

    phys2virt(kern_page_dir, (void *)VIDEO_MEM, (physaddr)vesa_mem, pages, 0x03);
    vesa_mem = (void *)VIDEO_MEM;

    kfree(mode_inf);
    return;
}

void vesa_set_mode(u32int mode) {

    regs_16 reg_in  = {0};
    regs_16 reg_out = {0};

    reg_in.ax = 0x4F02;
    reg_in.bx = mode;

    bios32_serv(BIOS_GRAPH, &reg_in, &reg_out);
    curr_mode = mode;

    return;
}

void vesa_get_mode(u16int mode, inf_mode_s *mode_inf) {

    regs_16 reg_in  = {0};
    regs_16 reg_out = {0};

    reg_in.ax = 0x4F01;
    reg_in.cx = mode;
    reg_in.di = 0x9200;

    //monitor_str_write("in vesa_get_mode\n");
    
    bios32_serv(BIOS_GRAPH, &reg_in, &reg_out);
    memcpy(mode_inf, (void *)0x9200, sizeof(inf_mode_s));

    return;
}

void *vesa_get_lfb(void) {

    inf_mode_s mode_inf = {0};
    vesa_get_mode((u16int)curr_mode, &mode_inf);

    return (void *)mode_inf.lin_base;
}

s32int vesa_get_resolY(void) {

    inf_mode_s mode_inf = {0};
    vesa_get_mode((u16int)curr_mode, &mode_inf);

    return (s32int)mode_inf.resolY;
}

s32int vesa_get_resolX(void) {

    inf_mode_s mode_inf = {0};
    vesa_get_mode((u16int)curr_mode, &mode_inf);

    return (s32int)mode_inf.resolX;
}

void vesa_memset_rgb(u8int *dst, u32int rgb, u32int count) {

    if(count % 3 != 0) {
        count = (count + 3) - (count % 3);
    }
    u8int r = rgb & 0x00FF0000;
    u8int g = rgb & 0x0000FF00;
    u8int b = rgb & 0x000000FF;

    for(u32int i = 0; i < count; i++) {

        *dst++ = r;
        *dst++ = g;
        *dst++ = b;
    }
    return;
}

void vesa_memcpy32_to24(u24int *dst, u32int *src, u32int count) {

    u24int tmp = {0};

    for(u32int i = 0; i < count; i++) {

        tmp.integer = src[i];
        dst[i] = tmp;
    }
    return;
}

u32int vesa_find_mode(u32int width, u32int height, u32int bpp) {

    inf_mode_s *mode_inf = kmalloc(sizeof(inf_mode_s));
    vbe_inf_s *vbe_inf = kmalloc(sizeof(vbe_inf_s));
    regs_16 reg_out = {0};
    regs_16 reg_in = {0};

    s32int best_pixs = 0;           //mode 0x01000 (640x400)
    s32int tmp_pixs = 0;
    s32int need_pixs = width * height;
    u32int best_num;

    reg_in.ax = 0x4F00;
    reg_in.di = 0x9000;

    memcpy(vbe_inf->sign, "VBE2", strlen("VBE2"));
    memcpy((void *)reg_in.di, vbe_inf, sizeof(vbe_inf_s));
    bios32_serv(BIOS_GRAPH, &reg_in, &reg_out);
    memcpy(vbe_inf, (void *)0x9000, sizeof(vbe_inf_s));

    u16int *list = (u16int *)vbe_inf->video_modes;
    u16int num = *list++;

    while(num != 0xFFFF) {

        vesa_get_mode(num, mode_inf);

        if(!(mode_inf->attrib & 0x80)) {    //it indicates the mode supports a linear frame buffer

            num = *list++;
            continue;
        }
        if(mode_inf->memModel != 4 && mode_inf->memModel != 6) {

            num = *list++;
            continue;
        }
        if(mode_inf->resolX == width && mode_inf->resolY == height && mode_inf->bpp == bpp) {
            return num;
        }
        tmp_pixs = mode_inf->resolX * mode_inf->resolY;

        if(tmp_pixs < need_pixs) {
            if(tmp_pixs > best_pixs) {
                best_pixs = tmp_pixs;
                best_num = num;
            }
        }
        num = *list++;
    }
    kfree(vbe_inf);
    kfree(mode_inf);

    return best_num;
}
