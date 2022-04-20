#pragma once

#include <stddef.h>

// Define read/write syscalls!
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_MAP 2

#define PAGESIZE 0x1000
#define PROT_READ 0x0001
#define PROT_WRITE 0x0010
#define PROT_EXEC 0x0100
#define PROT_NONE 0x1000

#define MAP_SHARED 0x0001
#define MAP_PRIVATE 0x0010
#define MAP_ANONYMOUS 0x0100
#define MAP_FIXED 0x1000

// Functions
char getkey(uint8_t code); 
void char_write(char key); 
char kgetc();  

int kread(uint64_t buf, uint64_t numchars); 
int kwrite(uint64_t buf, uint64_t len); 
uintptr_t map(void *addr, size_t len, int prot, int flags, int fd, int offset);

extern int64_t syscall(uint64_t nr, ...);
extern void syscall_entry();
int64_t syscall_handler(uint64_t nr, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5); 