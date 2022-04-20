#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>

extern int syscall(uint64_t nr, ...);

void _start() {

  for (int i = 0; i < 5000; i++) { ; }

  write(STDOUT, "finished sleep\n"); 

  for(;;) {}

  exit(); 
}
