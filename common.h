#ifndef COMMON_H
#define COMMON_H

// Standard integer types
typedef unsigned long long u64;
typedef          long long s64;
typedef unsigned int   u32;
typedef          int   s32;
typedef unsigned short u16;
typedef          short s16;
typedef unsigned char  u8;
typedef          char  s8;

// Write a byte out to the specified port.
void outb(u16 port, u8 value);

// Structure for an in-memory file
typedef struct {
    char name[100];
    char* content;
    u32 size;
} in_memory_file_t;

#endif
