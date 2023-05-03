#include "common.h"

u8int inb(u16int port) {
    u8int ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN" (port));
    return ret;
}

u16int inw(u16int port) {
    u16int ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "dN" (port));
    return ret;
}

u32int inl(u16int port) {
    u32int ret;
    asm volatile("inl %%dx, %%eax" : "=a"(ret) : "dN" (port));
    return ret;
}

void outb(u16int port, u8int value) {
    asm volatile("outb %1, %0" : : "dN" (port), "a"(value));
}

void outw(u16int port, u16int value) {
    asm volatile("outw %1, %0" : : "dN" (port), "a"(value));
}

void outl(u16int port, u32int value) {
    asm volatile("outl %%eax, %%dx" : : "dN" (port), "a"(value));
}
//==========================================================================
u32int flip_dword(u32int dword) {

    u32int byte_1 = dword & 0xFF;
    u32int byte_2 = (dword & 0x0000FF00) >> 8;
    u32int byte_3 = (dword & 0x00FF0000) >> 16;
    u32int byte_4 = (dword & 0xFF000000) >> 24;

    return (byte_1 << 24) | (byte_2 << 16) | (byte_3 << 8)| byte_4;
}

u16int flip_word(u16int word) {
    u32int byte_1 = word & 0xFF;
    u32int byte_2 = word >> 8;

    return (byte_1 << 8) | byte_2;
}

u8int flip_byte(u8int byte, u32int num_bits) {
    u8int tmp = byte << (8 - num_bits);
    return tmp | (byte >> num_bits);
}

u16int htonw(u16int hostshort) {
    return flip_word(hostshort);
}

u32int htondw(u32int hostlong) {
    return flip_dword(hostlong);
}

u8int htonb(u8int byte, u32int num_bits) {
    return flip_byte(byte, num_bits);
}

u8int ntohb(u8int byte, u32int num_bits) {
    return flip_byte(byte, 8 - num_bits);
}

u32int ntohdw(u32int netlong) {
    return flip_dword(netlong);
}

u16int ntohw(u16int netword) {
    return flip_word(netword);
}
