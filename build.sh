#!/bin/bash

# This script builds a bootable GRUB ISO that loads a C kernel.

# Exit immediately if a command exits with a non-zero status.
set -e
OUTPUT_ISO="myos.iso"
# The directory where we'll stage the ISO contents.
ISO_DIR="iso"
# Toolchain configuration
CC="i686-elf-gcc"
ASM="nasm"

# 1. Clean up previous builds
echo "Cleaning up previous build files..."
rm -rf "$ISO_DIR" "$OUTPUT_ISO" *.o kernel.bin

# 2. Assemble and link the kernel
echo "Assembling boot.asm..."
$ASM -f elf32 boot.asm -o boot.o

echo "Compiling kernel.c..."
$CC -m32 -ffreestanding -c kernel.c -o kernel.o -Wall -Wextra

echo "Compiling string.c..."
$CC -m32 -ffreestanding -c string.c -o string.o -Wall -Wextra

echo "Compiling pmm.c..."
$CC -m32 -ffreestanding -c pmm.c -o pmm.o -Wall -Wextra

echo "Compiling vmm.c..."
$CC -m32 -ffreestanding -c vmm.c -o vmm.o -Wall -Wextra

echo "Compiling heap.c..."
$CC -m32 -ffreestanding -c heap.c -o heap.o -Wall -Wextra

echo "Compiling syscall.c..."
$CC -m32 -ffreestanding -c syscall.c -o syscall.o -Wall -Wextra

echo "Compiling tar.c..."
$CC -m32 -ffreestanding -c tar.c -o tar.o -Wall -Wextra

echo "Linking kernel..."
ld -m elf_i386 -T linker.ld boot.o kernel.o string.o pmm.o vmm.o heap.o syscall.o tar.o -o kernel.bin -nostdlib

# 3. Create the necessary directory structure and copy files
echo "Creating ISO directory structure and copying kernel..."
mkdir -p "$ISO_DIR/boot/grub"
cp kernel.bin "$ISO_DIR/boot/"

echo "Creating initrd image..."
tar -cf "$ISO_DIR/boot/initrd.img" -C initrd .

# 4. Create the GRUB configuration file (grub.cfg)
echo "Generating grub.cfg..."
cat > "$ISO_DIR/boot/grub/grub.cfg" << EOF
set timeout=3
set default=0

menuentry "MyOS" {
	multiboot /boot/kernel.bin
	module /boot/initrd.img
	boot
}
EOF

# 5. Create the bootable ISO image using grub-mkrescue
echo "Creating bootable ISO: $OUTPUT_ISO..."
grub-mkrescue -o "$OUTPUT_ISO" "$ISO_DIR"

echo -e "\nSuccess! '$OUTPUT_ISO' has been created."
echo "You can test it with QEMU: qemu-system-x86_64 -cdrom $OUTPUT_ISO"
