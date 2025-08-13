#include "string.h"

void *memset(void *s, int c, u32 n) {
    u8* p = s;
    u8 val = (u8)c;
    u32 i;

    // Set bytes until the pointer is 4-byte aligned
    for (i = 0; i < (4 - ((u32)p % 4)) % 4 && n > 0; i++, n--) {
        *p++ = val;
    }

    // Set 4 bytes at a time
    u32 n_u32 = n / 4;
    if (n_u32 > 0) {
        u32 val32 = (val << 24) | (val << 16) | (val << 8) | val;
        u32* p32 = (u32*)p;
        while (n_u32--) {
            *p32++ = val32;
        }
        p = (u8*)p32;
    }

    // Set any remaining bytes
    n %= 4;
    while(n--) {
        *p++ = val;
    }

    return s;
}
void *memcpy(void *dest, const void *src, u32 n) {
    u8 *d = dest;
    const u8 *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int strncmp(const char *s1, const char *s2, u32 n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    else {
        return (*(unsigned char *)s1 - *(unsigned char *)s2);
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

u32 strlen(const char *s) {
    u32 len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;
    while ((*dest++ = *src++) != '\0');
    return original_dest;
}
