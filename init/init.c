#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>

// #define SYS_read 0
// #define SYS_write 1

extern int syscall(uint64_t nr, ...);

void _start() {

  char* test_page = (char*)0x400000000;
  test_page[0] = 'h';
  test_page[1] = 'e';
  test_page[2] = 'l';
  test_page[3] = 'l';
  test_page[4] = 'o';
  test_page[5] = '\n';
  write(STDOUT, test_page);

  // Issue a write system call
  write(STDOUT, "Hello world!\n");

  

  // Loop forever
  for(;;){}
}
