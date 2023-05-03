#ifndef COMMON_H
#define COMMON_H

    typedef unsigned long long  u64int;
    typedef unsigned int        u32int;
    typedef unsigned char       u8int;
    typedef unsigned short      u16int;
    typedef          long long  s64int;
    typedef          short      s16int;
    typedef          char       s8int;
    typedef          int        s32int;

typedef enum {

    false = 0,
    true = 1

} bool;  

#define NULL        ((void *) 0)   
#define FALSE       false;
#define TRUE        true;   

    void outb (u16int port, u8int value);
    void outw (u16int port, u16int value);
    void outl (u16int port, u32int value);
    u8int inb (u16int port);
    u16int inw(u16int port);
    u32int inl(u16int port);
    
#endif
