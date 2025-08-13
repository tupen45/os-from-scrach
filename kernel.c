#include "common.h"
#include "idt.h"
#include "string.h"
#include "terminal.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "syscall.h"

#include "tar.h"
#include <stddef.h> // For NULL

// VGA text mode buffer
volatile u16 *vga_buffer = (u16*)0xB8000;
const int VGA_COLS = 80;

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

const int VGA_ROWS = 25;

int term_col = 0;
int term_row = 0;
u8 term_color = 0x0F; // White on black

static volatile char key_buffer;
static volatile int key_ready = 0;
static volatile u8 shift_pressed = 0;
static volatile u32 timer_ticks = 0;

// Global variable to store initrd location
u32 global_initrd_location = 0;

#define MAX_IN_MEMORY_FILES 10
in_memory_file_t in_memory_files[MAX_IN_MEMORY_FILES];
u32 num_in_memory_files = 0;

// -------------------------------------------------------------------------
// --- Terminal Functions
// -------------------------------------------------------------------------

void scroll_screen() {
    // Move all lines up by one
    for (int row = 1; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            const int index_to = (VGA_COLS * (row - 1)) + col;
            const int index_from = (VGA_COLS * row) + col;
            vga_buffer[index_to] = vga_buffer[index_from];
        }
    }
    // Clear the last line
    const int last_line_offset = (VGA_COLS * (VGA_ROWS - 1));
    u16* last_line = (u16*)vga_buffer + last_line_offset;
    memset(last_line, 0, VGA_COLS * 2);
}

void term_putc(char c) {
    switch (c) {
    case '\n': {
        term_col = 0;
        term_row++;
        break;
    }
    default: {
        const int index = (VGA_COLS * term_row) + term_col;
        vga_buffer[index] = ((u16)term_color << 8) | c;
        term_col++;
        break;
    }
    }

    if (term_col >= VGA_COLS) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_ROWS) {
        scroll_screen();
        term_row = VGA_ROWS - 1;
    }
}

void term_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        term_putc(str[i]);
    }
}

void term_clear() {
    memset((void*)vga_buffer, 0, VGA_COLS * VGA_ROWS * 2);
    term_col = 0;
    term_row = 0;
}

// -------------------------------------------------------------------------
// --- GDT/IDT/PIC Initialization
// -------------------------------------------------------------------------
void outb(u16 port, u8 value) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;
interrupt_handler_t interrupt_handlers[256];

extern void load_idt(u32);

void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

void pic_remap() {
    // --- ICW1: Start initialization sequence ---
    // Start the initialization sequence in cascade mode
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    // Set PIC vector offsets
    outb(PIC1_DATA, 0x20); // Master PIC vector offset to 32
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset to 40

    // --- ICW3: Setup cascading ---
    // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04);
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02);

    // --- ICW4: Set 8086 mode ---
    // Set 8086/88 (MCS-80) mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // --- OCW1: Unmask all interrupts ---
    // Unmask all interrupts
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);
}

extern void isr128();

void idt_init() {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (u32)&idt_entries;

    // Set all IDT entries to our handler
    idt_set_gate( 0, (u32)isr0 , 0x08, 0x8E);
    idt_set_gate( 1, (u32)isr1 , 0x08, 0x8E);
    idt_set_gate( 2, (u32)isr2 , 0x08, 0x8E);
    idt_set_gate( 3, (u32)isr3 , 0x08, 0x8E);
    idt_set_gate( 4, (u32)isr4 , 0x08, 0x8E);
    idt_set_gate( 5, (u32)isr5 , 0x08, 0x8E);
    idt_set_gate( 6, (u32)isr6 , 0x08, 0x8E);
    idt_set_gate( 7, (u32)isr7 , 0x08, 0x8E);
    idt_set_gate( 8, (u32)isr8 , 0x08, 0x8E);
    idt_set_gate( 9, (u32)isr9 , 0x08, 0x8E);
    idt_set_gate(10, (u32)isr10, 0x08, 0x8E);
    idt_set_gate(11, (u32)isr11, 0x08, 0x8E);
    idt_set_gate(12, (u32)isr12, 0x08, 0x8E);
    idt_set_gate(13, (u32)isr13, 0x08, 0x8E);
    idt_set_gate(14, (u32)isr14, 0x08, 0x8E);
    idt_set_gate(15, (u32)isr15, 0x08, 0x8E);
    idt_set_gate(16, (u32)isr16, 0x08, 0x8E);
    idt_set_gate(17, (u32)isr17, 0x08, 0x8E);
    idt_set_gate(18, (u32)isr18, 0x08, 0x8E);
    idt_set_gate(19, (u32)isr19, 0x08, 0x8E);
    idt_set_gate(20, (u32)isr20, 0x08, 0x8E);
    idt_set_gate(21, (u32)isr21, 0x08, 0x8E);
    idt_set_gate(22, (u32)isr22, 0x08, 0x8E);
    idt_set_gate(23, (u32)isr23, 0x08, 0x8E);
    idt_set_gate(24, (u32)isr24, 0x08, 0x8E);
    idt_set_gate(25, (u32)isr25, 0x08, 0x8E);
    idt_set_gate(26, (u32)isr26, 0x08, 0x8E);
    idt_set_gate(27, (u32)isr27, 0x08, 0x8E);
    idt_set_gate(28, (u32)isr28, 0x08, 0x8E);
    idt_set_gate(29, (u32)isr29, 0x08, 0x8E);
    idt_set_gate(30, (u32)isr30, 0x08, 0x8E);
    idt_set_gate(31, (u32)isr31, 0x08, 0x8E);

    pic_remap();

    // Hardware IRQs
    idt_set_gate(32, (u32)irq0, 0x08, 0x8E);
    idt_set_gate(33, (u32)irq1, 0x08, 0x8E); // Keyboard
    idt_set_gate(34, (u32)irq2, 0x08, 0x8E);
    idt_set_gate(35, (u32)irq3, 0x08, 0x8E);
    idt_set_gate(36, (u32)irq4, 0x08, 0x8E);
    idt_set_gate(37, (u32)irq5, 0x08, 0x8E);
    idt_set_gate(38, (u32)irq6, 0x08, 0x8E);
    idt_set_gate(39, (u32)irq7, 0x08, 0x8E);
    idt_set_gate(40, (u32)irq8, 0x08, 0x8E);
    idt_set_gate(41, (u32)irq9, 0x08, 0x8E);
    idt_set_gate(42, (u32)irq10, 0x08, 0x8E);
    idt_set_gate(43, (u32)irq11, 0x08, 0x8E);
    idt_set_gate(44, (u32)irq12, 0x08, 0x8E);
    idt_set_gate(45, (u32)irq13, 0x08, 0x8E);
    idt_set_gate(46, (u32)irq14, 0x08, 0x8E);
    idt_set_gate(47, (u32)irq15, 0x08, 0x8E);
    idt_set_gate(128, (u32)isr128, 0x08, 0x8E);

    load_idt((u32)&idt_ptr);
}

// -------------------------------------------------------------------------
// --- Timer Functions
// -------------------------------------------------------------------------

void timer_init(u32 frequency) {
    // The value we send to the PIT is the value to divide it's input clock
    // (1193182 Hz) by, to get our required frequency.
    u32 divisor = 1193182 / frequency;

    // Send the command byte 0x36, setting the PIT to repeating mode.
    outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    u8 l = (u8)(divisor & 0xFF);
    u8 h = (u8)((divisor >> 8) & 0xFF);

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
}

void timer_handler(registers_t* regs) {
    (void)regs;
    timer_ticks++;
}

// -------------------------------------------------------------------------
// --- Keyboard and Input Handling
// -------------------------------------------------------------------------

void term_print_u32(u32 n) {
    if (n == 0) {
        term_putc('0');
        return;
    }

    char buf[11];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    buf[i] = '\0';

    for (int j = i - 1; j >= 0; j--) {
        term_putc(buf[j]);
    }
}

// Scancode Set 1 to ASCII mapping (US QWERTY layout). 0 for unmapped keys.
static const char scancode_map[] = {
      0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
      0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0,
    ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

// Scancode map for when a SHIFT key is pressed.
static const char scancode_map_shifted[] = {
      0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
      0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',   0,
    ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

void term_backspace() {
    if (term_col > 0) {
        term_col--;
        const int index = (VGA_COLS * term_row) + term_col;
        vga_buffer[index] = ((u16)term_color << 8) | ' ';
    }
}

char term_getc() {
    while (!key_ready) {
        asm volatile("hlt"); // Wait for an interrupt
    }
    key_ready = 0;
    return key_buffer;
}

void keyboard_handler(registers_t* regs) {
    (void)regs; // Prevent unused parameter warning
    u8 scancode = 0;

    // Read from the keyboard's data buffer
    asm volatile ("inb $0x60, %0" : "=a"(scancode));

    // Handle shift state
    if (scancode == 0x2A || scancode == 0x36) { // LShift or RShift press
        shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) { // LShift or RShift release
        shift_pressed = 0;
        return;
    }

    // Only handle key-presses from here, and only if buffer is free
    if (scancode >= 0x80 || key_ready) {
        return;
    }

    if (shift_pressed) {
        key_buffer = scancode_map_shifted[scancode];
    } else {
        key_buffer = scancode_map[scancode];
    }

    key_ready = 1;
}

// -------------------------------------------------------------------------
// --- "Programs"
// -------------------------------------------------------------------------

// Reads a number from the terminal, returns it as u32.
// Handles backspace. Finishes on Enter.
u32 term_gets_num() {
    char buf[11] = {0};
    int i = 0;
    while (i < 10) {
        char c = term_getc();
        if (c >= '0' && c <= '9') {
            buf[i++] = c;
            term_putc(c);
        } else if (c == '\b' && i > 0) {
            i--;
            term_backspace();
        } else if (c == '\n') {
            break;
        }
    }

    u32 result = 0;
    for (int j = 0; j < i; j++) {
        result = result * 10 + (buf[j] - '0');
    }
    return result;
}

// Reads a string from the terminal, handles backspace. Finishes on Enter.
void term_gets(char* buffer, int size) {
    int i = 0;
    while (i < size - 1) { // Leave space for null terminator
        char c = term_getc();
        if (c == '\n') {
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                term_backspace();
            }
        } else {
            buffer[i++] = c;
            term_putc(c);
        }
    }
    buffer[i] = '\0'; // Null-terminate the string
}


void program_shell() {
    term_clear();
    term_print("Interactive Shell (Press ESC to exit)\n> ");
    while(1) {
        char c = term_getc();
        if (c == 27) { // ESC key
            return;
        } else if (c == '\b') {
            term_backspace();
        } else {
            term_putc(c);
        }
    }
}

void program_counter() {
    term_clear();
    term_print("Interactive Counter (Press ESC to exit)\n");
    u32 count = 0;
    while(1) {
        term_print("Count: ");
        term_print_u32(count);
        term_print(" - Press any key to increment...\n");

        char c = term_getc();
        if (c == 27) { // ESC key
            return;
        }
        count++;
    }
}

void program_art() {
    term_clear();
    term_print(".########.##....##.######..######..##....##.\n");
    term_print("....##....##....##.##....##.##....##.###...##.\n");
    term_print("....##....##....##.##....##.##....##.####..##.\n");
    term_print("....##....##....##.######..######..##.##.##.\n");
    term_print("....##....##....##.##.......##..##..##..####.\n");
    term_print("....##....##....##.##.......##..##..##...###.\n");
    term_print("....##.....######..##.......######..##....##.\n\n");
    term_print("      (Press ESC to exit)\n");

    while(term_getc() != 27);
}

void program_calculator() {
    term_clear();
    term_print("Simple Calculator (Press ESC to exit)\n");
    term_print("Supports +, -, *, /\n");

    while(1) {
        term_print("\n tbhcr> ");
        u32 num1 = term_gets_num();

        term_print(" ");
        char op = term_getc();
        term_putc(op);

        term_print(" ");
        u32 num2 = term_gets_num();

        u32 result = 0;
        int error = 0;
        switch(op) {
            case '+': result = num1 + num2; break;
            case '-': result = num1 - num2; break;
            case '*': result = num1 * num2; break;
            case '/':
                if (num2 == 0) error = 1;
                else result = num1 / num2;
                break;
            default: error = 2; break;
        }

        term_print("\n= ");
        if (error == 1) {
            term_print("Division by zero!");
        } else if (error == 2) {
            term_print("Invalid operator!");
        } else {
            term_print_u32(result);
        }
        term_print("\n\nPress any key to continue or ESC to exit...");
        if (term_getc() == 27) return;
        term_clear();
    }
}

// Helper function to get file content from in-memory or initrd
// Returns a pointer to the content, sets size, and sets needs_free to 1 if content needs to be freed by caller.
char* get_file_content(const char* filename, u32* size, int* needs_free) {
    *needs_free = 0; // Default to no free needed

    // First, check in-memory files
    for (u32 i = 0; i < num_in_memory_files; i++) {
        if (strcmp(in_memory_files[i].name, filename) == 0) {
            *size = in_memory_files[i].size;
            return in_memory_files[i].content;
        }
    }

    // If not found in-memory, try initrd
    char* content = tar_read_file(global_initrd_location, filename, size);
    if (content != NULL) {
        *needs_free = 1; // Content from tar_read_file needs to be freed
    }
    return content;
}

void program_read_file() {
    term_clear();
    term_print("Read File (Press ESC to exit)\n");
    term_print("Enter filename: ");


    char filename[100]; // Max filename length
    term_gets(filename, sizeof(filename));
    term_print("\n"); // Newline after input

    u32 file_size = 0;
    int needs_free = 0;
    char* file_content = get_file_content(filename, &file_size, &needs_free);

    if (file_content) {
        term_print("--- Content of ");
        term_print(filename);
        term_print(" ---\n");
        term_print(file_content);
        term_print("\n-------------------------------\n");
        if (needs_free) {
            kfree(file_content); // Free only if allocated by tar_read_file
        }
    } else {
        term_print("Error: File '");
        term_print(filename);
        term_print("' not found or could not be read.\n");
    }

    term_print("\nPress any key to return to menu...");
    term_getc();
}

void program_create_file() {
    term_clear();
    term_print("Create New File (Press ESC to exit)\n");

    if (num_in_memory_files >= MAX_IN_MEMORY_FILES) {
        term_print("Error: Maximum number of in-memory files reached.\n");
        term_print("\nPress any key to return to menu...");
        term_getc();
        return;
    }

    char filename[100];
    term_print("Enter filename (max 99 chars): ");
    term_gets(filename, sizeof(filename));
    term_print("\n");

    // Check if file already exists
    for (u32 i = 0; i < num_in_memory_files; i++) {
        if (strcmp(in_memory_files[i].name, filename) == 0) {
            term_print("Error: File with this name already exists.\n");
            term_print("\nPress any key to return to menu...");
            term_getc();
            return;
        }
    }

    term_print("Enter content (max 1023 chars, press Enter to finish):\n");
    char content_buffer[1024]; // Max content length + null terminator
    term_gets(content_buffer, sizeof(content_buffer));
    term_print("\n");

    u32 content_size = strlen(content_buffer);
    char* allocated_content = (char*)kmalloc(content_size + 1); // +1 for null terminator
    if (allocated_content == NULL) {
        term_print("Error: Failed to allocate memory for file content.\n");
        term_print("\nPress any key to return to menu...");
        term_getc();
        return;
    }
    strcpy(allocated_content, content_buffer);

    strcpy(in_memory_files[num_in_memory_files].name, filename);
    in_memory_files[num_in_memory_files].content = allocated_content;
    in_memory_files[num_in_memory_files].size = content_size;
    num_in_memory_files++;

    term_print("File '");    term_print(filename);    term_print("' created successfully.\n");

    term_print("\nPress any key to return to menu...");
    term_getc();
}


void page_fault_handler(registers_t* regs) {
    u32 faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int present   = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;           // Write operation?
    int us = regs->err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?

    term_print("Page Fault! ( ");
    if (present) term_print("present ");
    if (rw) term_print("read-only ");
    if (us) term_print("user-mode ");
    if (reserved) term_print("reserved ");
    term_print(") at 0x");
    term_print_u32(faulting_address);
    term_print("\nSystem Halted.\n");

    for(;;);
}

void program_pfaulter() {
    term_clear();
    term_print("This program will attempt to access memory that is not mapped.\n");
    term_print("This should trigger a page fault, which our kernel will handle.\n");
    term_print("Press any key to trigger the page fault...\n");
    term_getc();

    // This address is not mapped, so it will cause a page fault.
    u32 *ptr = (u32*)0xA0000000;
    u32 value = *ptr;
    (void)value; // Suppress unused variable warning

    term_print("If you see this, something went wrong!\n");
}

void program_heap_test() {
    term_clear();
    term_print("Kernel Heap Test\n");

    term_print("Allocating 10 bytes...\n");
    char* test_mem = (char*)kmalloc(10);
    if (test_mem) {
        term_print("Allocated at: 0x");
        term_print_u32((u32)test_mem);
        term_print("\n");
        test_mem[0] = 'H';
        test_mem[1] = 'i';
        test_mem[2] = '\0';
        term_print("Content: ");
        term_print(test_mem);
        term_print("\n");
        kfree(test_mem);
        term_print("Freed memory.\n");
    } else {
        term_print("Allocation failed!\n");
    }

    term_print("\nAllocating 100 bytes...\n");
    char* test_mem2 = (char*)kmalloc(100);
    if (test_mem2) {
        term_print("Allocated at: 0x");
        term_print_u32((u32)test_mem2);
        term_print("\n");
        kfree(test_mem2);
        term_print("Freed memory.\n");
    } else {
        term_print("Allocation failed!\n");
    }

    term_print("\nPress any key to return to menu...");
    term_getc();
}

void program_syscall_test() {
    term_clear();
    term_print("System Call Test\n");
    term_print("Calling sys_print via int 0x80...\n");

    // Simulate a system call
    // eax = syscall number (SYS_NR_PRINT)
    // ebx = argument (string pointer)
    asm volatile (
        "mov $0, %%eax\n\t"  // SYS_NR_PRINT
        "mov %0, %%ebx\n\t" // String pointer
        "int $0x80"
        : : "r"("Hello from syscall!\n") : "eax", "ebx"
    );

    term_print("\nPress any key to return to menu...");
    term_getc();
}

void kmain(multiboot_info_t* mboot_ptr, u32 magic) {
    (void)magic; // Suppress warnings

    term_clear();
    term_print("Welcome to MyOS!\n");

    // Debugging Multiboot info
    term_print("Multiboot flags: 0x");
    term_print_u32(mboot_ptr->flags);
    term_print("\n");

    if (mboot_ptr->flags & MULTIBOOT_FLAG_MODS) {
        term_print("MULTIBOOT_FLAG_MODS is set.\n");
        term_print("Modules count: ");
        term_print_u32(mboot_ptr->mods_count);
        term_print("\n");
        term_print("Modules address: 0x");
        term_print_u32(mboot_ptr->mods_addr);
        term_print("\n");
    } else {
        term_print("MULTIBOOT_FLAG_MODS is NOT set.\n");
    }

    // 1. Initialize Interrupts (but don't enable them yet)
    idt_init();
    term_print("IDT initialized.\n");
    register_interrupt_handler(14, page_fault_handler);

    // 2. Initialize Memory Management
    // We assume 16MB RAM and place the PMM bitmap at 1.5MB
    pmm_init(16 * 1024, 0x180000);
    // Mark the kernel's memory region (1MB to 1.5MB) as used
    pmm_mark_region_used(0x100000, 512);
    // Mark the PMM bitmap's region as used
    pmm_mark_region_used(0x180000, 8); // 16MB/4KB/8bits_per_byte = 4KB, round up to 8KB
    term_print("PMM initialized.\n");
    vmm_init(); // This enables paging
    term_print("Paging enabled.\n");

    // 3. Initialize Kernel Heap
    heap_init();
    term_print("Kernel Heap initialized.\n");

    // 4. Register all our interrupt handlers
    register_interrupt_handler(33, keyboard_handler);
    timer_init(100); // Set timer to 100 Hz
    register_interrupt_handler(32, timer_handler); // IRQ 0

    // 5. Initialize System Call Interface
    syscall_init();
    term_print("System Call Interface initialized.\n");

    // Check for initrd module
    if (mboot_ptr->flags & MULTIBOOT_FLAG_MODS) {
        multiboot_module_t *mod = (multiboot_module_t *)mboot_ptr->mods_addr;
        global_initrd_location = mod->mod_start; // Store initrd location globally
        term_print("Initrd found at 0x");
        term_print_u32(global_initrd_location);
        term_print("\n");
        tar_list_archive(global_initrd_location);
    } else {
        term_print("No initrd module found.\n");
    }

    // 6. Enable interrupts now that everything is set up
    asm volatile ("sti");
    term_print("Interrupts enabled.\n");

    while(1) {
    
        term_print("Welcome to MyOS\n");
        term_print("Select a program:\n");
        term_print("  1. Interactive Shell\n");
        term_print("  2. Counter Program\n");
        term_print("  3. ASCII Art Display\n");
        term_print("  4. Calculator Program\n");
        term_print("  5. Trigger Page Fault\n");
        term_print("  6. Kernel Heap Test\n");
        term_print("  7. System Call Test\n");
        term_print("  8. Read File from Initrd\n");
        term_print("  9. Create New File\n\n"); 
        term_print("10. for clear screen \n\n");// New option
        term_print("tbhcr> ");

        char choice = term_getc();

        switch(choice) {
            case '1': program_shell(); break;
            case '2': program_counter(); break;
            case '3': program_art(); break;
            case '4': program_calculator(); break;
            case '5': program_pfaulter(); break;
            case '6': program_heap_test(); break;
            case '7': program_syscall_test(); break;
            case '8': program_read_file(); break;
            case '9': program_create_file(); break; // New case
            case '0': term_clear(); break; // Clear screen
        }
    }
}

void register_interrupt_handler(u8 n, interrupt_handler_t handler) {
    interrupt_handlers[n] = handler;
}

void interrupt_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        interrupt_handlers[regs->int_no](regs);
    }

    // Send End-of-Interrupt (EOI) to PICs
    if (regs->int_no >= 32 && regs->int_no < 48) {
        if (regs->int_no >= 40) {
            outb(PIC2_CMD, 0x20); // Slave
        }
        outb(PIC1_CMD, 0x20); // Master
    }
}