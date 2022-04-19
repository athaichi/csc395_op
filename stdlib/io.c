#include "io.h"
#include "../kernel/systemcalls.h"

long read(uint8_t filedes, char* buf, uint64_t numchars) {
    long rc = syscall(SYS_READ, filedes, buf, numchars); 
    return rc; 
}

long write(uint8_t filedes, char* str) {
    uint64_t len = strlen(str); 
    long rc = syscall(SYS_WRITE, filedes, str, len);
    return rc;  
}

// void printf(const char* format, ...) { }
// char* getline() { }
// long perror() { }