#include "help_func.h"

void memcpy (void *str2, void *str1, unsigned int size) {
    //str1 - source; str2 - receiver
    char *a = (char *)str1;
    char *b = (char *)str2;

    if(size <= 0) {
        return;
    }

    for(int i = 0; i < size; i++) {
        b[i] = a[i];
    }
    return;
}

void memset(void *str2, char sign, unsigned int size) {
    char *recv = (char *)str2;

    for(int i = 0; i < size; i++) {
        recv[i] = sign;
    }
    return;
}

void memsetdw(void *str2, int sign, unsigned int size) {

    int *recv = (int *)str2;

    for(int i = 0; i < size; i++) {
        recv[i] = sign;
    }
    return;
}

int memcmp(void *str1, void *str2, unsigned int len) {          //if str1 == str2 return 1;

    char *a = (char *)str1;
    char *b = (char *)str2;

    while(len--) {
        if(*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return 1;
}

int strlen (char *string) {
    int count = 0;

    while(*string++ != '\0') {
        count++;
    }
    //count++;
    return count;
}

int strcmp (char *str1, char *str2) {
    int i = 0;

    while(str1[i] == str2[i]) {
        if(str1[i] == '\0') {
            return 0;
        }
        i++;
    }
    if(str1[i] == '\0') {
        return -1;
    }
    if(str2[i] == '\0') {
        return 1;
    }
}

int strncmp(char *str1, char *str2, int count) {

    int res = 0;

    while(count) {
        res = *str1 - *str2++;

        if((res != 0) || (*str1++ == 0)) {
            break;
        }
        count--;
    }

    return res;
}

int strcpy(char *str1, char *str2) {
    int i = 0;

    while((*str1++ = *str2++) != 0) {
        i++;
    }
    return i;
}

void strcat(void *dest, void *src) {

    u8int *end = (u8int *)dest + strlen(dest);
    memcpy((u8int *)end, (u8int *)src, strlen((u8int *)src));
    end = end + strlen((u8int *)src);
    *end = '\0';
    
    return;
}

char *strsep(unsigned char **stringp, unsigned char *delim) {

    char *s;
    char *spanp;
    char *tok;
    int c;
    int sc;

    if((s = *stringp) == NULL) {
        return NULL;
    }
    for(tok = s;;) {
        c = *s++;
        spanp = delim;

        do{
            if((sc = *spanp++) == c) {

                if(c == 0) {
                    s = NULL;
                }
                else {
                    s[-1] = 0;
                }
                *stringp = s;
                return (tok);
            }
        } while(sc != 0);
    }
}

char *strdup(char *src) {

    int len = strlen(src) + 1;
    char *dst = kmalloc(len);
    memcpy(dst, src, len);

    return dst;
}

char *strstr(char *in, char *str) {

    char c;
    u32int len;

    c = *str++;
    if(!c) {
        return (char *)in;
    }

    len = strlen(str);

    do {
        char sc;

        do {
            sc = *in++;
            if(!sc) {
                return (char *) 0;
            }
        } while(sc != c);

    } while(strncmp(in, str, len) != 0);

    return (char *) (in - 1);
}

char *itoa(int i, char *b) {

    char const digit[] = "0123456789";
    char *p = b;
    int shift;

    if(i < 0) {
        *p++ = '-';
        i = -i;
    }
    shift = i;

    do{
        ++p;
        shift = shift / 10;
    } while(shift);

    *p = '\0';
    do{
        *--p = digit[i % 10];
        i = i / 10;
    } while(i);

    return b;
}

int atoi(char *str) {

    int res = 0;

    if((*str == '-') || (*str == '+')) {
        str++;
    }
    while(*str >= '0' && *str <= '9') {

        res *= 10;
        res += *str++;
        res -= '0';
    }

    return res;
}