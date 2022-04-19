#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>

extern int syscall(uint64_t nr, ...);

bool askinput(char* modname) {
  char* curchar = NULL; 
  int counter = 0; 

  // read in chars until we reach a newline
  while(counter < 20) {
    read(STDIN, curchar, 1); 

    // if user input is done
    if (curchar == '\n') {

      // see if the command matches an available module name
      for (;;){}

      // match! return true so we can exec back in _start
      //return true

      // no dice, return false 
      //return false; 


    }

  }

}

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
  write(STDOUT, "Init start!\n$");

  char modname[20]; 

  // read in user input: 
  bool valid = askinput(modname); 

  // if command is not valid, ask for a new input
  while(!valid) {
    write(STDOUT, "   Invalid command.\n$");  
    askinput(modname); 
  }
  
  // if inputs match module names, exec
  exec(modname); 
  
  

  // Loop forever
  for(;;){}
}
