; boot.asm
; Sets up the GDT and hands off control to the C kernel.
; Also contains low-level interrupt stubs.

bits 32

; --- Multiboot Header ---
section .multiboot
align 4
    dd 0x1BADB002 ; Magic
    dd 0x03      ; Flags (load modules on page boundaries, provide memory map)
    dd -(0x1BADB002 + 0x03) ; Checksum

; --- GDT Definition ---
section .gdt
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0

gdt_code: ; Code segment descriptor (0x08)
    ; base=0, limit=0xfffff,
    ; 1st flags: (present) (privilege 0) (descriptor type)
    ; type flags: (code) (conforming) (readable) (accessed)
    ; 2nd flags: (granularity) (32-bit default)
    dw 0xFFFF    ; Limit (low)
    dw 0x0000    ; Base (low)
    db 0x00      ; Base (mid)
    db 0b10011010; 1st flags, type flags
    db 0b11001111; 2nd flags, limit (high)
    db 0x00      ; Base (high)

gdt_data: ; Data segment descriptor (0x10)
    ; Same as code segment, but with type flags for data
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10010010
    db 0b11001111
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size
    dd gdt_start               ; Address

; --- Kernel Entry Point ---
section .text
global _start
global load_idt
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10
global isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20
global isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
global load_page_directory, enable_paging

extern kmain
extern interrupt_handler

_start:
    cli ; Disable interrupts until the IDT is loaded

    lgdt [gdt_descriptor] ; Load the GDT

    ; Update segment registers to use our new GDT
    mov ax, 0x10 ; 0x10 is the offset of our data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to our code segment to update CS
    jmp 0x08:.update_cs
.update_cs:

    ; Set up the stack
    mov esp, stack_top

    ; Push multiboot info pointer and magic number for kmain
    push eax
    push ebx
    ; Call the C kernel
    call kmain

    ; Hang if kmain returns
    hlt

; Function for C to load the IDT
load_idt:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Loads the page directory address into CR3
load_page_directory:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

; Enables the paging bit in CR0
enable_paging:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret


; --- Common Interrupt Stub ---
; All ISRs will jump here after pushing their specific details.
isr_common_stub:
    pusha ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds ; Lower 16-bits of eax = ds.
    push eax   ; save the data segment descriptor

    mov ax, 0x10  ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp ; Pass pointer to the regs struct on the stack
    call interrupt_handler
    add esp, 4 ; Clean up the pushed pointer

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8 ; Cleans up the error code and ISR number
    iret       ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

; --- ISR Definitions (0-31) ---
%macro ISR_NOERR 1
isr%1:
    push byte 0 ; Push a dummy error code
    push byte %1  ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Generate the first 32 ISRs (CPU exceptions)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_NOERR 8
ISR_NOERR 9
ISR_NOERR 10
ISR_NOERR 11
ISR_NOERR 12
ISR_NOERR 13
ISR_NOERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

%macro IRQ 2
irq%1:
    push byte 0
    push byte %2
    jmp isr_common_stub
%endmacro

; IRQs 0-15 are mapped to IDT entries 32-47
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; System call interrupt
global isr128
isr128:
    push byte 0
    push byte 128
    jmp isr_common_stub

section .bss
resb 8192 ; 8KB for stack
stack_top: