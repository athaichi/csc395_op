#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>

void _start() {

  // char* test_page = (char*)0x400000000;
  // test_page[0] = 'h';
  // test_page[1] = 'e';
  // test_page[2] = 'l';
  // test_page[3] = 'l';
  // test_page[4] = 'o';
  // test_page[5] = '\n';
  // write(STDOUT, test_page);

  // Issue a write system call
  write(STDOUT, "In echo!\n$");

  // sleep for a sec
  for(int i = 0; i < 5000; i++){
    ;
  }

  exit(); 
}
