#pragma once

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define STDERRROR 2


// long rc = syscall(SYS_READ, 0, buf, 5)
long read(uint8_t filedes, char* buf, uint64_t numchars); 

// long rc2 = syscall(SYS_WRITE, 1, buf2, 6)
long write(uint8_t filedes, char* str); 