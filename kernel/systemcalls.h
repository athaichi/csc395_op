#pragma once

#include <stdint.h>

// Define read/write syscalls!
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_EXEC 3

// Functions
char getkey(uint8_t code); 
int kread(uint64_t buf, uint64_t numchars); 
int kwrite(uint64_t buf, uint64_t len); 
void char_write(char key); 
char kgetc();  

extern int64_t syscall(uint64_t nr, ...);
extern void syscall_entry();
int64_t syscall_handler(uint64_t nr, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5); 