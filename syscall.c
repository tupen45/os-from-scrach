#include "syscall.h"
#include "terminal.h"

// Array of system call handlers
static void* syscalls[256];

// System call implementations
void sys_print(const char* str) {
    term_print(str);
}

// System call dispatcher
void syscall_handler(registers_t* regs) {
    if (regs->eax >= 256) {
        return;
    }

    void (*handler)(const char*) = syscalls[regs->eax];

    if (handler) {
        handler((const char*)regs->ebx);
    }
}

void syscall_init() {
    // Register system call handlers
    syscalls[SYS_NR_PRINT] = &sys_print;

    // Register the system call interrupt handler (int 0x80)
    register_interrupt_handler(0x80, syscall_handler);
}
