#ifndef TAR_H
#define TAR_H

#include "common.h"

// Structure for a USTAR tar header.
// See https://wiki.osdev.org/USTAR for details.
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} tar_header_t;

// Parses a tar archive and lists its contents.
void tar_list_archive(u32 archive_start);

char* tar_read_file(u32 archive_start, const char* filename, u32* size);

#endif // TAR_H
