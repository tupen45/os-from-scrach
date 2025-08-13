#ifndef IDT_H
#define IDT_H

#include "common.h"

// An entry in the IDT.
struct idt_entry_struct {
    u16 base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
    u16 sel;                 // Kernel segment selector.
    u8  always0;             // This must always be zero.
    u8  flags;               // More flags. See documentation.
    u16 base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// A pointer to an array of interrupt handlers.
struct idt_ptr_struct {
    u16 limit;
    u32 base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

// A struct describing a register state pushed to the stack by the ISRs.
struct registers {
    u32 ds;                  // Data segment selector
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    u32 int_no, err_code;    // Interrupt number and error code (if applicable)
    u32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};
typedef struct registers registers_t;

// A function pointer type for our interrupt handlers.
typedef void (*interrupt_handler_t)(registers_t* handler_regs);

// The main interrupt handler function, called from assembly.
void interrupt_handler(registers_t* regs);

// Allows registration of a C-level interrupt handler.
void register_interrupt_handler(u8 n, interrupt_handler_t handler);

// Initializes the IDT.
void idt_init();

// These extern directives let us access the addresses of our ISR handlers.
extern void isr0 (); extern void isr1 (); extern void isr2 (); extern void isr3 ();
extern void isr4 (); extern void isr5 (); extern void isr6 (); extern void isr7 ();
extern void isr8 (); extern void isr9 (); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void irq0 (); extern void irq1 (); extern void irq2 (); extern void irq3 ();
extern void irq4 (); extern void irq5 (); extern void irq6 (); extern void irq7 ();
extern void irq8 (); extern void irq9 (); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

#endif
