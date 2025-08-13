#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

void term_clear();
void term_putc(char c);
void term_print(const char* str);
void term_print_u32(u32 n);

#endif
