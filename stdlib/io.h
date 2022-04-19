#pragma once

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define STDERRROR 2

long read(uint8_t filedes, char* buf, uint64_t numchars); 
long write(uint8_t filedes, char* str); 
// void printf(const char* format, ...);
// char* getline(); 
// long perror(); 
