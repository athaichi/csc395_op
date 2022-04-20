#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>

void _start() {

  // Issue a write system call
  write(STDOUT, "In echo!\n$");

  char* buf; 
  
  // Copy first 20 characters written to terminal
  for (int i = 0; i < 20; i++) {
    read(STDIN, buf, 20); 
  }

  write(STDOUT, buf); 

  exit(); 
}
