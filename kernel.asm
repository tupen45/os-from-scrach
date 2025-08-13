; kernel.asm
; A simple 32-bit kernel with text editor-like functionality.
bits 32

; --- Multiboot Header ---
; This header is required for GRUB to recognize and load our kernel.
section .multiboot
align 4
    ; Magic number for Multiboot specification
    dd 0x1BADB002
    ; Flags (none needed for this simple kernel)
    dd 0x00
    ; Checksum: -(magic + flags)
    dd -(0x1BADB002 + 0x00)

section .text
global _start

VGA_BUFFER equ 0xB8000
VGA_WIDTH  equ 80
VGA_HEIGHT equ 25

_start:
    ; The VGA text mode buffer is at address 0xB8000.
    mov edi, VGA_BUFFER  ; Pointer to the start of the video buffer
    mov ebx, 0x0F     ; Color: White on Black

    ; Print the initial messages
    mov esi, msg_welcome
    call print_string
    call print_newline
    call enable_cursor
    call update_cursor

; --- Main Input Loop ---
main_loop:
    ; The PS/2 keyboard controller uses I/O ports 0x60 (data) and 0x64 (status/command).
    ; We read the status port to see if a key has been pressed.
    in al, 0x64
    ; Bit 0 of the status register is set to 1 when the output buffer is full (key is ready).
    test al, 1
    jz main_loop ; If bit 0 is zero, loop and wait.

    ; If we're here, a key is ready. Read the scancode from the data port.
    in al, 0x60

    ; --- Handle Shift Key State ---
    cmp al, 0x2A ; Left Shift press
    je .shift_press
    cmp al, 0x36 ; Right Shift press
    je .shift_press
    cmp al, 0xAA ; Left Shift release
    je .shift_release
    cmp al, 0xB6 ; Right Shift release
    je .shift_release

    ; --- Handle Arrow Keys ---
    cmp al, 0x4B ; Left Arrow
    je .handle_left
    cmp al, 0x4D ; Right Arrow
    je .handle_right
    cmp al, 0x48 ; Up Arrow
    je .handle_up
    cmp al, 0x50 ; Down Arrow
    je .handle_down

    ; Ignore all other key releases (top bit is set)
    cmp al, 0x80
    jae main_loop

    ; --- Translate Scancode to ASCII ---
    movzx ecx, al
    mov esi, scancode_map ; Default to the normal map
    cmp byte [shift_pressed], 1
    jne .get_char
    mov esi, scancode_map_shifted ; If shift is pressed, use the shifted map
.get_char:
    mov al, [esi + ecx]

    ; If the character is not mapped (null), ignore it.
    test al, al
    jz main_loop

    ; --- Handle Special Keys or Print Character ---
    cmp al, 8  ; Is it a backspace?
    je .handle_backspace
    cmp al, 13 ; Is it Enter?
    je .handle_enter

    call print_char ; It's a normal character, print it
    jmp main_loop

.shift_press:
    mov byte [shift_pressed], 1
    jmp main_loop

.shift_release:
    mov byte [shift_pressed], 0
    jmp main_loop

.handle_backspace:
    call handle_backspace
    jmp main_loop

.handle_enter:
    call print_newline
    jmp main_loop

.handle_left:
    call handle_left_arrow
    jmp main_loop

.handle_right:
    call handle_right_arrow
    jmp main_loop

.handle_up:
    call handle_up_arrow
    jmp main_loop

.handle_down:
    call handle_down_arrow
    jmp main_loop

; --- Halt the system (unreachable in this design) ---
halt_loop:
    cli               ; Clear interrupts
    hlt               ; Halt the CPU
    jmp halt_loop     ; Loop forever

; --- Reusable Functions ---

; Prints a single character to the screen.
; Input: al = character to print. Modifies edi.
print_char:
    push eax

    ; If we are at or past the end of the screen, scroll first.
    mov eax, VGA_BUFFER
    mov ah, bl      ; Set color attribute
    mov [edi], ax   ; Write character to screen
    add edi, 2      ; Move to next character cell
    call update_cursor
    popa
    ret

; Prints a null-terminated string.
; Input: esi = address of the string
print_string:
    pusha
.loop:
    lodsb           ; Load byte from [esi] into al, and increment esi
    test al, al     ; Check for null terminator
    jz .done
    call print_char
    jmp .loop
.done:
    popa
    ret

; Moves the screen cursor (edi) to the beginning of the next line.
print_newline:
    pusha
    mov eax, edi
    sub eax, VGA_BUFFER ; Get offset from start of buffer
    xor edx, edx     ; Clear edx for 32-bit division
    mov ecx, VGA_WIDTH * 2 ; Width of screen in bytes
    div ecx          ; eax = current row, edx = remainder (column in bytes)
    inc eax          ; Move to the next row
    mul ecx          ; Calculate new offset from the top
    add eax, VGA_BUFFER ; Calculate new absolute address
    mov edi, eax
    call update_cursor
    popa
    ret

; Handles a backspace character
handle_backspace:
    pusha
    ; Don't backspace past the start of the buffer
    cmp edi, VGA_BUFFER
    jbe .done

    ; Move cursor back one position and erase the character
    sub edi, 2
    mov word [edi], 0x0F20 ; White on black space
    call update_cursor
.done:
    popa
    ret

; --- Arrow Key Handlers ---
handle_left_arrow:
    pusha
    cmp edi, VGA_BUFFER
    jbe .done
    sub edi, 2
    call update_cursor
.done:
    popa
    ret

handle_right_arrow:
    pusha
    mov eax, VGA_BUFFER + (VGA_WIDTH * VGA_HEIGHT * 2) - 2
    cmp edi, eax
    jae .done
    add edi, 2
    call update_cursor
.done:
    popa
    ret

handle_up_arrow:
    pusha
    sub edi, VGA_WIDTH * 2
    cmp edi, VGA_BUFFER
    jb .fix_up
    jmp .done
.fix_up:
    add edi, VGA_WIDTH * 2
.done:
    call update_cursor
    popa
    ret

handle_down_arrow:
    pusha
    add edi, VGA_WIDTH * 2
    mov eax, VGA_BUFFER + (VGA_WIDTH * VGA_HEIGHT * 2)
    cmp edi, eax
    jae .fix_down
    jmp .done
.fix_down:
    sub edi, VGA_WIDTH * 2
.done:
    call update_cursor
    popa
    ret

; --- Hardware Cursor Functions ---
enable_cursor:
    pusha
    mov dx, 0x3D4
    mov al, 0x0A    ; Cursor Start Register
    out dx, al
    inc dx
    mov al, 0       ; Scanline 0
    out dx, al

    dec dx
    mov al, 0x0B    ; Cursor End Register
    out dx, al
    inc dx
    mov al, 15      ; Scanline 15 (for a full block cursor)
    out dx, al
    popa
    ret

update_cursor:
    pusha
    ; Calculate position from edi
    mov eax, edi
    sub eax, VGA_BUFFER
    shr eax, 1 ; Convert byte offset to character offset (position is now in ax)

    mov dx, 0x3D4
    mov al, 0x0F    ; Cursor Location Low Register
    out dx, al
    inc dx
    mov al, al      ; Low byte of position is already in al
    out dx, al

    dec dx
    mov al, 0x0E    ; Cursor Location High Register
    out dx, al
    inc dx
    mov al, ah      ; High byte of position
    out dx, al
    popa
    ret

section .data
msg_welcome: db "Welcome to MyOS! Start typing...", 0
shift_pressed: db 0

; Scancode Set 1 to ASCII mapping (US QWERTY layout). Unmapped keys are 0.
scancode_map:
    db  0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9
    db 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0, 'a', 's'
    db 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', ''', '`', 0, '\', 'z', 'x', 'c', 'v'
    db 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0
    times 128-($-scancode_map) db 0

; Scancode map for when a SHIFT key is pressed.
scancode_map_shifted:
    db  0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9
    db 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0, 'A', 'S'
    db 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V'
    db 'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0
    times 128-($-scancode_map_shifted) db 0
