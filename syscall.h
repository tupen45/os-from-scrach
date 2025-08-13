#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"
#include "idt.h"

// System call numbers
enum syscall_numbers {
    SYS_NR_PRINT = 0, // System call to print a string to the terminal
    // Add more system calls here
};

// Initializes the system call interface.
void syscall_init();

// The C-level system call handler, called from assembly.
void syscall_handler(registers_t* regs);

#endif
