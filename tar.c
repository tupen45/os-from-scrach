#include "tar.h"
#include "string.h"
#include "terminal.h"
#include "heap.h" // For kmalloc and kfree
#include <stddef.h> // For NULL

// Converts an octal string to an unsigned integer.
static u32 oct2bin(const char *str, int size) {
    u32 n = 0;
    const char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

// Parses a tar archive and lists its contents.
void tar_list_archive(u32 archive_start) {
    tar_header_t *header = (tar_header_t *)archive_start;

    term_print("\n--- Listing Files in Initrd ---\n");

    while (strncmp(header->magic, "ustar", 5) == 0) {
        u32 size = oct2bin(header->size, 11);

        term_print(header->name);
        term_print(" (size: ");
        // You'll need a function to print numbers, let's assume you have one.
        // term_print_u32(size);
        term_print(" bytes)\n");

        // Calculate the start of the next header
        u32 next_header_addr = (u32)header + sizeof(tar_header_t) + size;
        // Align to 512-byte boundary
        if (next_header_addr % 512 != 0) {
            next_header_addr += 512 - (next_header_addr % 512);
        }

        header = (tar_header_t *)next_header_addr;

        // Check for end of archive (two null blocks)
        if (header->name[0] == '\0') {
            break;
        }
    }
    term_print("-------------------------------\n");
}

// Reads a file from a tar archive.
// Returns a pointer to the allocated buffer containing the file content,
// and sets *size to the file's size. Returns NULL if file not found.
char* tar_read_file(u32 archive_start, const char* filename, u32* size) {
    tar_header_t *header = (tar_header_t *)archive_start;

    while (strncmp(header->magic, "ustar", 5) == 0) {
        u32 file_size = oct2bin(header->size, 11);

        // Compare filename, ensuring null termination for header->name
        if (strncmp(header->name, filename, 100) == 0) {
            // Found the file
            char* file_content = (char*)kmalloc(file_size + 1); // +1 for null terminator
            if (file_content == NULL) {
                return NULL; // Memory allocation failed
            }
            memcpy(file_content, (char*)header + sizeof(tar_header_t), file_size);
            file_content[file_size] = '\0'; // Null-terminate the content
            *size = file_size;
            return file_content;
        }

        // Calculate the start of the next header
        u32 next_header_addr = (u32)header + sizeof(tar_header_t) + file_size;
        // Align to 512-byte boundary
        if (next_header_addr % 512 != 0) {
            next_header_addr += 512 - (next_header_addr % 512);
        }

        header = (tar_header_t *)next_header_addr;

        // Check for end of archive (two null blocks)
        if (header->name[0] == '\0') {
            break;
        }
    }

    return NULL; // File not found
}