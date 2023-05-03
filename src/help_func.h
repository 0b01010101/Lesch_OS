
#ifndef HELP_FUNC_H
#define HELP_FUNC_H

void memcpy(void *, void *, unsigned int);
void memset(void *, char, unsigned int); 
void memsetdw(void *, int, unsigned int);
int memcmp(void *, void *, unsigned int);
int strlen(char *);
int strcmp(char *, char *);
int strncmp(char *, char *, int);
int strcpy(char *, char *);
void strcat(void *dest, void *src);
char *strdup(char *);
char *strstr(char *, char *);
char *strsep(unsigned char **, unsigned char *);
char *itoa(int i, char *b);
int atoi(char *str);

#endif
