#include <string.h>

#include "io.h"
#include "../kernel/systemcalls.h"

/**
 * Stdlib wrapper for syscall read. 
 *
 * \param filedes   where to read from
 * \param buf       where to place read characters
 * \param numchars  number of characters to read in
 */
long read(uint8_t filedes, char* buf, uint64_t numchars) {
    long rc = syscall(SYS_READ, filedes, buf, numchars); 
    return rc; 
}

/**
 * Stdlib wrapper for syscall write. 
 *
 * \param filedes   where to read from
 * \param str       what to print out
 */
long write(uint8_t filedes, char* str) {
    uint64_t len = strlen(str); 
    long rc = syscall(SYS_WRITE, filedes, str, len);
    return rc;  
}