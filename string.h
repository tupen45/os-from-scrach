#ifndef STRING_H
#define STRING_H

#include "common.h"



void *memset(void *s, int c, u32 n);
void *memcpy(void *dest, const void *src, u32 n);
int strncmp(const char *s1, const char *s2, u32 n);


int strcmp(const char *s1, const char *s2);
u32 strlen(const char *s);
char *strcpy(char *dest, const char *src);


#endif
