#include "io.h"
#include "../kernel/systemcalls.h"


// long rc = syscall(SYS_READ, 0, buf, 5)
long read(uint8_t filedes, char* buf, uint64_t numchars) {
    long rc = syscall(SYS_READ, filedes, buf, numchars); 
    return rc; 
}

// long rc2 = syscall(SYS_WRITE, 1, buf2, 6)
long write(uint8_t filedes, char* str) {
    uint64_t len = strlen(str); 
    long rc = syscall(SYS_WRITE, filedes, str, len);
    return rc;  
}