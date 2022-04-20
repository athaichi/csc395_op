#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>
#include <string.h>

void _start() {

  // Issue a write system call
  write(STDOUT, "Init start!\n");
  write(STDOUT, "$ "); 

  char modname[5]; 
  bool valid = false; 
  char curchar; 

  do { 

    // read in user command
    read(STDIN, modname, 4);

    modname[5] = '\0';
    //write(STDOUT, "  |  "); 
    //write(STDOUT, modname); 

    // check if input is valid
    if (strcmp(modname, "init") == 0) { valid = true; break; }
    if (strcmp(modname, "echo") == 0) { valid =  true; break; }
    if (strcmp(modname, "wait") == 0) { valid = true; break; }

    if (!valid) {
      write(STDOUT, "   Invalid command.\n$ ");
    }

  } while (!valid);
  
  // if inputs match module names, exec
  exec(modname); 
  
  // Loop forever
  for(;;){}
}
